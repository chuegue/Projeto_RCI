#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#define max(A, B) ((A) >= (B) ? (A) : (B))

int main(void)
{
    struct addrinfo hints, *res;
    int fd, newfd, errcode, afd = 0, maxfd, counter;
    fd_set rfds;
    enum
    {
        idle,
        busy
    } state;
    ssize_t n, nw;
    struct sockaddr addr;
    socklen_t addrlen;
    char *ptr, buffer[128];
    ptr = (char *)calloc(128, sizeof(char));
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit(1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((errcode = getaddrinfo(NULL, "58001", &hints, &res)) != 0)
        exit(1);
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1)
        exit(1);
    if (listen(fd, 5) == -1)
        exit(1);

    state = idle;

    while (1)
    {
        FD_ZERO(&rfds);
        switch (state)
        {
        case idle:
            FD_SET(fd, &rfds);
            maxfd = fd;
            break;
        case busy:
            FD_SET(fd, &rfds);
            FD_SET(afd, &rfds);
            maxfd = max(fd, afd);
            break;
        }
        counter = select(maxfd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
        if (counter <= 0)
        {
            printf("erro: %s\n",strerror(counter));
            exit(1);
        }
        for (; counter; --counter)
        {
            switch (state)
            {
            case idle:
                if (FD_ISSET(fd, &rfds))
                {
                    FD_CLR(fd, &rfds);
                    addrlen = sizeof(addr);
                    if ((newfd = accept(fd, &addr, &addrlen)) == -1)
                    {
                        printf("puta2\n");
                        exit(1);
                    }
                    afd = newfd;
                    state = busy;
                }
                break;
            case busy:
                if (FD_ISSET(fd, &rfds))
                {
                    FD_CLR(fd, &rfds);
                    addrlen = sizeof(addr);
                    if ((newfd = accept(fd, &addr, &addrlen)) == -1)
                        exit(1);
                    /*write busy\n in newfd*/
                    strcpy(ptr, "busy\n");
                    n = write(newfd, ptr, strlen(ptr) * sizeof(char));
                    if (n <= 0)
                        exit(1);
                    close(newfd);
                }
                else if (FD_ISSET(afd, &rfds))
                {
                    FD_CLR(afd, &rfds);
                    if ((n = read(afd, buffer, 128)) != 0)
                    {
                        if (n == -1)
                        {
                            exit(1);
                        }
                        strcpy(ptr, buffer);
                        while (n > 0)
                        {
                            if ((nw = write(afd, buffer, n)) <= 0)
                            {
                                exit(1);
                            }
                            n -= nw;
                            ptr += nw;
                        }
                    }
                    else
                    {
                        close(afd);
                        state = idle;
                    }
                }

                break;
            }
        }
    }
}