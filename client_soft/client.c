#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3491"
#define MAXDATASIZE 100 // максимальное число байт, принимаемых за один раз
// получение структуры sockaddr, IPv4 или IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    struct addrinfo hints, *res, *p;

    char buf[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];
    int status_get_addrinfo;

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //handler error with getting struct getaddrinfo
    if((status_get_addrinfo = 
            getaddrinfo(argv[1], PORT, &hints, &res))
        != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n",
                gai_strerror(status_get_addrinfo));
        return  1;
    };

    for(p = res; p != NULL; p=p->ai_next)
    {
        if((sockfd = socket(p->ai_family,
                        p->ai_socktype,
                        p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        };

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if(p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    s, sizeof s);

    printf("client: connecting to %s\n", s);

    freeaddrinfo(res); // эта структура больше не нужна

    if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }


    printf("client: received '%s'\n",buf);

    close(sockfd);

    return 0;
}