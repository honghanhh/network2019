/**
 * @file daemon.c
* @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief This program is executed on every Node (not the origin node). It
 * communicates with the two next programs with sockets. It can receive two
 * types of connections:
 *  - a connection for receiving a block. Then the block is stored locally in a
 *    file.
 *  - a connection for requesting the execution of map. Then the map in executed
 *    (in a new process with fork/exec) taking as input the local block (the
 *    block is read from the local file and transmitted on the map’s STDIN) and
 *    the result count table (from the map’s STDOUT) is send back on the TCP
 *    connection.
 *
 * @version 0.1
 * @date 2019-04-01
 *
 * Copyright (c) 2019
 *
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"

int connectToHost (char* hostname, int port);

int receiveFile (int sockfd, char* foldername, char* filename);

int main (int argc, char const* argv[])
{
    // Connect to Launch
    int sock_lauch_fd = connectToHost (SERVER_ADDR, LAUNCH_PORT);

    if (sock_lauch_fd < 0)
    {
        printf ("Can't connect to Launch (%s:%d)\n", SERVER_ADDR, LAUNCH_PORT);
        exit (1);
    }

    printf ("Connected to Launch: %d\n", sock_lauch_fd);

    // connect to Split
    int sock_split_fd = connectToHost (SERVER_ADDR, SPLIT_PORT);

    if (sock_split_fd < 0)
    {
        printf ("Can't connect to Split (%s:%d)\n", SERVER_ADDR, SPLIT_PORT);
        exit (1);
    }

    printf ("Connected to Split: %d\n", sock_split_fd);

    // start receive file from split
    char filename[256] = {0};
    int  received      = receiveFile (sock_split_fd, RECEIVED_FOLDER, filename);

    if (received < 0)
    {
        printf ("Can't receive file from split\n");
        exit (1);
    }
    close (sock_split_fd);

    // Run map to counts the occurrence of words
    char buf[10] = "start";

    send (sock_lauch_fd, buf, strlen (buf), 0);

    int in = open (filename, O_RDONLY);

    int map_pid;
    if ((map_pid = fork ()) == 0)
    {
        // redirect file content to stdin of map
        dup2 (in, STDIN_FILENO);
        // redirect output from map to socket
        dup2 (sock_lauch_fd, STDOUT_FILENO);
        // close the original after it's duplicated
        close (sock_lauch_fd);

        execlp ("./map", "map", NULL);
    }

    printf ("Map is executed\n");
    int status;
    wait (&status);
    printf ("Map done\n");

    return 0;
}

int connectToHost (char* hostname, int port)
{
    struct hostent* hp = gethostbyname (hostname);
    if (hp == NULL)
    {
        printf ("gethostbyname() of %s failed\n", hostname);
        return -1;
    }

    int sockfd;
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf ("Error creating socket\n");
        return -1;
    }

    struct sockaddr_in saddr;
    memset (&saddr, 0, sizeof (saddr));
    saddr.sin_family = AF_INET;
    memcpy ((char*)&saddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
    saddr.sin_port = htons (port);
    if (connect (sockfd, (struct sockaddr*)&saddr, sizeof (saddr)) < 0)
    {
        printf ("Cannot connect\n");
        return -1;
    }

    return sockfd;
}

long int getSizeFile (FILE* fp)
{
    fseek (fp, 0, SEEK_END);
    long int size = ftell (fp);
    fseek (fp, 0, SEEK_SET);

    return size;
}

int receiveFile (int sockfd, char* foldername, char* filename)
{
    char buffer[MAX_LEN_BUFFER] = {0};
    int  received               = 0;

    // Receive file name from server
    if ((received = read (sockfd, buffer, sizeof buffer)) <= 0)
    {
        printf ("Can't receive file file name from server.\n");
        return -1;
    }

    sprintf (filename, "%s%s", foldername, buffer);

    // Open file for receive file from server
    FILE* out_file = fopen (filename, "w+");
    if (out_file == NULL)
    {
        printf ("Can't create file %s on your pc.\n", filename);
        return -1;
    }

    printf ("Success to create file %s on client\n", filename);

    memset (buffer, 0, sizeof buffer);
    while ((received = read (sockfd, buffer, sizeof buffer)) > 0)
    {
        int write_sz = fwrite (buffer, sizeof (char), received, out_file);
        if (write_sz < received)
        {
            printf ("File write failed on your pc.");
            fclose (out_file);
            return -1;
        }

        memset (buffer, 0, sizeof buffer);
        if (received == 0 || received != MAX_LEN_BUFFER)
        {
            break;
        }
    }

    if (received < 0)
    {
        fclose (out_file);
        return -1;
    }

    printf ("Received file %s (%ld byte)\n", filename, getSizeFile (out_file));
    fclose (out_file);
    return 0;
}
