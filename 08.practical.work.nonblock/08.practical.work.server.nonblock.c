#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

int main()
{
    int sockfd, clen, clientfd;
    struct sockaddr_in saddr, caddr;
    unsigned short port = 8784;
    char mess[200];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error creating socket\n");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    int fl = fcntl(sockfd, F_GETFL, 0);
    fl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, fl);

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        printf("Error binding\n");
        return -1;
    }

    if (listen(sockfd, 5) < 0)
    {
        printf("Error listening\n");
        return -1;
    }

    while (1)
    {
        clen = sizeof(caddr);
        if ((clientfd = accept(sockfd, (struct sockaddr *)&caddr, &clen)) <= 0)
        {
            // printf("Error accepting connection\n");
            continue;
        }

        int fl = fcntl(clientfd, F_GETFL, 0);
        fl |= O_NONBLOCK;
        fcntl(clientfd, F_SETFL, fl);

        while (1)
        {
            memset(mess, 0, sizeof mess);
            if (read(clientfd, mess, sizeof(mess)) > 0)
            {
                printf("Client: %s\n", mess);
            }

            struct pollfd input[1] = {{.fd = 0, .events = POLLIN}};
            if (poll(input, 1, 100) > 0)
            {
                // printf("Server: ");
                fgets(mess, sizeof mess, stdin);
                mess[strlen(mess) - 1] = 0;

                write(clientfd, mess, strlen(mess));
            }
        }
    }
}
