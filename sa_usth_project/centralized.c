/**
 * @file centralized.c
 * @author Thi-Hong-Hanh TRAN (hanh.usth@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-04-06
 *
 * Copyright (c) 2019
 *
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main (int argc, char const* argv[])
{
    if (argc != 3)
    {
        printf (
            "Invalid execution format. Use the following format\n<executable"
            "code> <input text file> <output map file>\n");
        exit (-1);
    }

    char* in_name  = (char*)argv[1];
    char* out_name = (char*)argv[2];

    // Run map to counts the occurrence of words
    int in = open (in_name, O_RDONLY);
    if (in < 0)
    {
        printf ("Can't open file %s\n", in_name);
    }

    int out = open (out_name,
                    O_WRONLY | O_TRUNC | O_CREAT,
                    S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    if (out < 0)
    {
        printf ("Can't open file %s\n", out_name);
    }

    time_t start_time = time (NULL);

    printf ("Map is started, at (UNIX time: %ld) %s\n",
            start_time,
            asctime (localtime (&start_time)));

    int map_pid;
    if ((map_pid = fork ()) == 0)
    {
        // redirect file content to stdin of map
        dup2 (in, STDIN_FILENO);
        // redirect output from map to map file
        dup2 (out, STDOUT_FILENO);
        // close the original after it's duplicated

        execlp ("./map", "map", NULL);
    }

    int status;
    wait (&status);

    time_t stop_time = time (NULL);

    printf ("Map done, at (UNIX time: %ld) %s\n",
            stop_time,
            asctime (localtime (&stop_time)));
    printf ("Execution time: %ld s\n", stop_time - start_time);

    close (in);
    close (out);
    return 0;
}
