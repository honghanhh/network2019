/**
 * @file ultis.c
 * author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-04-03
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
#include <unistd.h>

int listenPort (struct sockaddr_in* saddr, unsigned int port, bool blocking)
{
    int sockfd;

    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf ("Error creating socket\n");
        return -1;
    }

    if (!blocking)
    {
        setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof (int));

        int fl = fcntl (sockfd, F_GETFL, 0);
        fl |= O_NONBLOCK;
        fcntl (sockfd, F_SETFL, fl);
    }

    memset (saddr, 0, sizeof (struct sockaddr_in));
    saddr->sin_family      = AF_INET;
    saddr->sin_addr.s_addr = htonl (INADDR_ANY);
    saddr->sin_port        = htons (port);

    if (bind (sockfd, (struct sockaddr*)saddr, sizeof (struct sockaddr_in)) < 0)
    {
        printf ("Error binding\n");
        return -1;
    }

    if (listen (sockfd, 128) < 0)
    {
        printf ("Error listening\n");
        return -1;
    }

    return sockfd;
}
