/**
 * @file split.c
 * @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief This program is executed on the origin node to split the big file into
 * blocks and send them to the nodes. It takes as parameter a file name (the big
 * file). It splits the file into blocks, opens connections with the different
 * Daemons and sends the blocks to the Daemons.
 *
 * @version 0.1
 * @date 2019-04-01
 *
 * Copyright (c) 2019
 *
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

struct sendfile_info
{
    int   clientfd;
    int   client_idx;
    char* file_name;
};

bool isFileExist (char* file_name);

int sendFile (int clientfd, int client_idx, char* file_name);

long int getSizeFile (FILE* fp);

int splitFile (char* filename, int num_file, char** list_file);

int main (int argc, char const* argv[])
{
    if (argc != 3)
    {
        printf (
            "Invalid execution format. Use the following format\n<executable"
            "code> <number of client> <file name>\n");
        exit (-1);
    }

    int max_num_client = atoi (argv[1]);
    if (max_num_client == 0)
    {
        printf ("Please enter invalid number of clinet\n");
        exit (-1);
    }

    char* file_name = (char*)argv[2];

    if (!isFileExist (file_name))
    {
        printf ("Please enter exist file\n");
        exit (-1);
    }

    // Split file into max_num_client file
    char* list_file[max_num_client];
    for (int i = 0; i < max_num_client; i++)
    {
        list_file[i] = (char*)malloc (sizeof (char) * 256);
    }

    if (splitFile (file_name, max_num_client, list_file) < 0)
    {
        printf ("Can't split file\n");
        exit (1);
    }

    printf ("Done split file %s into:\n", file_name);
    for (int i = 0; i < max_num_client; i++)
    {
        printf ("%s\n", list_file[i]);
    }

    // Listen on port
    struct sockaddr_in saddr;
    int                sockfd = listenPort (&saddr, SPLIT_PORT, true);
    if (sockfd < 0)
    {
        printf ("Can't listen on port %d\n", SPLIT_PORT);
        exit (-1);
    }

    printf ("Start listen on port %d\n", SPLIT_PORT);

    // Wait all client is connected
    struct sockaddr_in caddr;
    int                clen       = sizeof caddr;
    int                num_client = 0;
    int                clientfds[max_num_client];

    while (1)
    {
        int new_client = accept (sockfd, (struct sockaddr*)&caddr, &clen);
        if (new_client < 0)
        {
            printf ("accept error\n");
            return -1;
        }

        printf ("New connection, socket fd is %d\n", new_client);
        clientfds[num_client] = new_client;

        num_client++;
        if (num_client >= max_num_client)
        {
            break;
        }
    }

    printf ("All client (%d) are connected\n", max_num_client);

    for (int i = 0; i < max_num_client; i++)
    {
        sendFile (clientfds[i], i, list_file[i]);
        close (clientfds[i]);
    }

    printf ("All file are sent\n");
    return 0;
}

bool isFileExist (char* file_name)
{
    return (access (file_name, F_OK) != -1);
}

int sendFile (int clientfd, int client_idx, char* file_name)
{
    FILE* file = fopen (file_name, "r");

    if (file == NULL)
    {
        printf ("File %s is not exist\n", file_name);
        return -1;
    }

    char buff[MAX_LEN_BUFFER] = {0};

    // Send file name to client
    sprintf (buff, "data_%d.txt", client_idx);
    send (clientfd, buff, strlen (buff), 0);

    memset (buff, 0, sizeof buff);

    printf ("Start send file %s (%ld byte) to client %d\n",
            file_name,
            getSizeFile (file),
            client_idx);
    size_t   nbytes    = 0;
    long int sent_byte = 0;
    while ((nbytes = fread (buff, sizeof (char), MAX_LEN_BUFFER, file)) > 0)
    {
        int sent = send (clientfd, buff, nbytes, 0);
        if (sent < 0)
        {
            printf ("Failed to send file %s to client %d. (errno = %d)\n",
                    file_name,
                    client_idx,
                    sent);
            return -1;
        }
        memset (buff, 0, sizeof buff);

        sent_byte += nbytes;
        printf ("Send: %ld byte\r", sent_byte);
    }
    printf ("\nSend file %s to client %d done!\n", file_name, client_idx);
    return 0;
}

long int getSizeFile (FILE* fp)
{
    fseek (fp, 0, SEEK_END);
    long int size = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    return size;
}

int splitFile (char* filename, int num_file, char** list_file)
{
    FILE *in_fp, *out_fp;

    if ((in_fp = fopen (filename, "r")) == NULL)
    {
        printf ("Can't open file %s\n", filename);
        return -1;
    }

    long int in_size  = getSizeFile (in_fp);
    long int out_size = in_size / num_file;

    printf ("Size of file %s: %ld byte\n", filename, in_size);
    printf ("Size of out file approx %ld byte\n", out_size);

    char out_filename[256];
    char buffer[MAX_LEN_BUFFER] = {0};

    long int remain_byte     = out_size;
    int      extra_byte_read = 0;
    for (int i = 0; i < num_file; i++)
    {
        sprintf (out_filename, SPLIT_OUT_FOLDER "block_%d.txt", i);
        strcpy (list_file[i], out_filename);

        out_fp = fopen (out_filename, "w");

        // try to read out_size from in_fp
        int block_read = 0;
        remain_byte    = out_size - extra_byte_read;
        while (remain_byte)
        {
            memset (buffer, 0, sizeof buffer);

            if (remain_byte > sizeof (buffer))
            {
                // fill data to buffer
                block_read = fread (buffer, sizeof (buffer), 1, in_fp);
                fwrite (buffer, sizeof (buffer), 1, out_fp);
                remain_byte -= block_read * sizeof (buffer);
            }
            else
            {
                // read remain_byte
                block_read = fread (buffer, remain_byte, 1, in_fp);
                fwrite (buffer, remain_byte, 1, out_fp);
                remain_byte = 0;

                // read to end of word
                // if this is last file, read to end of file
                int c;
                extra_byte_read = 0;
                do
                {
                    c = fgetc (in_fp);
                    if (c == EOF)
                    {
                        break;
                    }
                    fputc (c, out_fp);
                    extra_byte_read++;

                } while (isalpha (c) || isdigit (c) || i == (num_file - 1));
            }
        }

        fclose (out_fp);
    }

    return 0;
}
