/**
 * @file config.h
 * @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-04-02
 *
 * Copyright (c) 2019
 *
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// server
#define SPLIT_PORT 12345
#define LAUNCH_PORT 12346
#define SERVER_ADDR "localhost"
#define SPLIT_OUT_FOLDER "server/"

// client
#define RECEIVED_FOLDER "client/"

// Both
#define MAX_LEN_BUFFER 1024

int listenPort (struct sockaddr_in* saddr, unsigned int port, bool blocking);

#endif  // __CONFIG_H__
