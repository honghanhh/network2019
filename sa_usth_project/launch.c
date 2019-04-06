/**
 * @file launch.c
 * @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief This program is executed on the origin node to launch the execution.
 * It connects to the Daemons to start the execution of maps. It receives the
 * content of all the result count tables on the TCP connections. It executes
 * the reduce (in a a new process with fork/exec), taking as input (on STDIN)
 * the data received on the TCP connection. The output of the reduce is
 * redirected to a file on the origin node. We can assume here that the map and
 * reduce programs where previously copied on all the nodes
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
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

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
    int   out_fd    = open (file_name,
                       O_WRONLY | O_TRUNC | O_CREAT,
                       S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    // Listen on port
    struct sockaddr_in saddr;
    int                sockfd = listenPort (&saddr, LAUNCH_PORT, false);
    if (sockfd < 0)
    {
        printf ("Can't listen on port %d\n", LAUNCH_PORT);
        exit (-1);
    }

    printf ("Start listen on port %d\n", LAUNCH_PORT);

    // Run reduce with pipe
    int pipe_fd[2];
    if (pipe (pipe_fd) < 0)
    {
        printf ("Can't pipe()\n");
        exit (1);
    }
    int reduce_pid;
    if ((reduce_pid = fork ()) == 0)
    {
        dup2 (pipe_fd[0], STDIN_FILENO);
        dup2 (out_fd, STDOUT_FILENO);
        dup2 (out_fd, STDERR_FILENO);
        close (out_fd);
        close (pipe_fd[1]);

        execlp ("./reduce", "reduce", NULL);
    }

    printf ("Reduce is executed\n");

    // Wait all client is connected
    struct sockaddr_in caddr;
    int                clen = sizeof caddr;

    int    num_client     = 0;
    int*   clientsock_fd  = (int*)malloc (sizeof (int) * max_num_client);
    int*   client_started = (int*)malloc (sizeof (int) * max_num_client);
    char   buffer[MAX_LEN_BUFFER] = {0};
    time_t start_time             = 0;

    while (1)
    {
        fd_set set;             // declaration of the set
        FD_ZERO (&set);         //// clear the set
        FD_SET (sockfd, &set);  // add listening sockfd to set

        int maxfd = sockfd;  // a required value to pass to select()
        for (int i = 0; i < max_num_client; i++)
        {
            // add connected client sockets to set
            if (clientsock_fd[i] > 0)
            {
                FD_SET (clientsock_fd[i], &set);
            }

            if (clientsock_fd[i] > maxfd)
            {
                maxfd = clientsock_fd[i];
            }
        }

        if ((select (maxfd + 1, &set, NULL, NULL, NULL) < 0)
            && (errno != EINTR))
        {
            printf ("select error\n");
        }

        if (FD_ISSET (sockfd, &set))
        {
            int clientfd = accept (sockfd, (struct sockaddr*)&caddr, &clen);

            if (clientfd < 0)
            {
                printf ("accept error\n");
                return -1;
            }

            printf ("New connection, socket fd is %d\n", clientfd);

            // make something nonblocking
            int fl = fcntl (clientfd, F_GETFL, 0);
            fl |= O_NONBLOCK;
            fcntl (clientfd, F_SETFL, fl);

            // add it to the clientfds array
            for (int i = 0; i < max_num_client; i++)
            {
                if (clientsock_fd[i] == 0)
                {
                    clientsock_fd[i]  = clientfd;
                    client_started[i] = 0;
                    printf ("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < max_num_client; i++)
        {
            if (clientsock_fd[i] > 0 && FD_ISSET (clientsock_fd[i], &set))
            {
                memset (buffer, 0, sizeof buffer);
                int byte_read = 0;
                if ((byte_read = read (clientsock_fd[i], buffer, sizeof buffer))
                    > 0)
                {
                    if (client_started[i] == 1)
                    {
                        write (pipe_fd[1], buffer, byte_read);
                    }
                    else if (strcmp (buffer, "start") == 0)
                    {
                        client_started[i] = 1;

                        time_t rawtime = time (NULL);
                        printf (
                            "Client %d is started, at (Unix time: %ld) %s\n",
                            i,
                            rawtime,
                            asctime (localtime (&rawtime)));

                        if (start_time == 0)
                        {
                            start_time = rawtime;
                        }
                    }
                }
                else
                {
                    // some error, remove it from the "active" fd array
                    printf ("client %d has disconnected.\n", i);
                    clientsock_fd[i] = -1;
                }
            }
        }

        // Check all client done?
        bool all_done = true;
        for (int i = 0; i < max_num_client; i++)
        {
            if (clientsock_fd[i] != -1)
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
        {
            break;
        }
    }

    close (pipe_fd[0]);
    close (pipe_fd[1]);

    int status;
    wait (&status);
    time_t stop_time = time (NULL);

    printf ("Reduce done, at (UNIX time: %ld) %s\n",
            stop_time,
            asctime (localtime (&stop_time)));
    printf ("Execution time: %ld s\n", stop_time - start_time);

    return 0;
}
