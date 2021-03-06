#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    struct sockaddr_in saddr;
    struct hostent *h;
    int sockfd;
    unsigned short port = 8784;
    char *host_name = malloc(sizeof(char) * 128);
    char *ip_addr;
    char mess[200];

    //get server hostname from program arguments || STDIN if no arguments
    if (argc == 2)
    {
        host_name = argv[1];
        printf("%s \n", host_name);
    }
    else
    {
        printf(" Please enter a host name:");
        scanf("%s", host_name);
    }

    //resolve IP address & STDOUT
    h = gethostbyname(host_name);
    if (!h)
        perror(" Host unknown \n");
    ip_addr = inet_ntoa(*(struct in_addr *)h->h_name);
    printf(" Hostname is: %s , IP address is: %s \n ", h->h_name, ip_addr);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket\n");
        close(sockfd);
        return -1;
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    memcpy((char *)&saddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);

    struct in_addr **bo = (struct in_addr **)h->h_addr_list;
    struct in_addr hanh = *bo[0];
    printf("%s\n", inet_ntoa(hanh));

    saddr.sin_port = htons(port);
    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("Cannot connect\n");
        close(sockfd);
        return -1;
    }
    else
    {
        printf("Sucessfully connected!\n");
    }

    int fl = fcntl(sockfd, F_GETFL, 0);
    fl |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, fl);

    while (1)
    {
        memset(mess, 0, sizeof mess);
        if (read(sockfd, mess, sizeof(mess)) > 0)
        {
            printf("Server: %s\n", mess);
        }

        struct pollfd input[1] = {{.fd = 0, .events = POLLIN}};
        if (poll(input, 1, 100) > 0)
        {
            // printf("Client: ");
            fgets(mess, sizeof mess, stdin);
            mess[strlen(mess) - 1] = 0;

            write(sockfd, mess, strlen(mess));
        }
    }
}
