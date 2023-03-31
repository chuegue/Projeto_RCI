#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

char *transrecieveUDP(char *ip, char *port, char *m_tosend, unsigned int n_send, unsigned int *n_read)
{
    int fd_udp, errcode;
    ssize_t n;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    socklen_t addrlen;
    fd_udp = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd_udp == -1)
    {
        exit(1);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0) /*error*/
    {
        strerror(errcode);
        exit(1);
    }
    n = sendto(fd_udp, m_tosend, n_send, 0, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (n == -1) /*error*/
        exit(1);
    char *buffer = (char *)calloc((256 << 5), sizeof(char));
    addrlen = sizeof(addr);
    n = recvfrom(fd_udp, buffer, 256 << 5, 0,
                 (struct sockaddr *)&addr, &addrlen);
    close(fd_udp);
    if (n == -1) /*error*/
        exit(1);
    char *ret = (char *)calloc(strlen(buffer) + 1, sizeof(char));
    strcpy(ret, buffer);
    free(buffer);
    *n_read = n;
    return ret;
}

int main(int argc, char *argv[])
{
    char message[128] = {0}, supposed[128] = {0}, **save_ptr, *receive1, *receive2, *token;
    int n_received, i = 37;

    printf("REDE Nº: %03i\n", i);
    memset(message, 0, sizeof message);
    sprintf(message, "NODES %03i", i);
    sprintf(supposed, "NODESLIST %03i", i);
    receive1 = transrecieveUDP("193.136.138.142", "59000", message, strlen(message), &n_received);
    printf("Recebi a mensagem \n%s\n\n", receive1);
    token = strtok_r(receive1, "\n", &receive1);
    if (strcmp(token, supposed) != 0)
    {
        printf("DEU MERDA FAMILIA\n");
        exit(1);
    }
    token = strtok_r(receive1, "\n", &receive1);
    while (token)
    {
        token = strtok(token, " ");
        memset(message, 0, sizeof message);
        sprintf(message, "UNREG %03i %02i", i, atoi(token));
        receive2 = transrecieveUDP("193.136.138.142", "59000", message, strlen(message), &n_received);
        if (strcmp(receive2, "OKUNREG") != 0)
        {
            printf("DEU MERDA FAMILIA\n");
            exit(1);
        }
        printf("Tirei o nó %02i\n", atoi(token));
        token = strtok_r(receive1, "\n", &receive1);
    }
    printf("\nDone!\n\n");
}

// int main(int argc, char *argv[])
// {
//     char message[128] = {0}, supposed[128] = {0}, **save_ptr, *receive1, *receive2, *token;
//     int n_received;
//     for (int i = 0; i < 1000; i++)
//     {
//         if (i == 23)
//             continue;
//         printf("REDE Nº: %03i", i);
//         memset(message, 0, sizeof message);
//         sprintf(message, "NODES %03i", i);
//         sprintf(supposed, "NODESLIST %03i", i);
//         receive1 = transrecieveUDP("193.136.138.142", "59000", message, strlen(message), &n_received);
//         printf("Recebi a mensagem \n%s\n\n", receive1);
//         token = strtok_r(receive1, "\n", &receive1);
//         if (strcmp(token, supposed) != 0)
//         {
//             printf("DEU MERDA FAMILIA\n");
//             exit(1);
//         }
//         token = strtok_r(receive1, "\n", &receive1);
//         while (token)
//         {
//             token = strtok(token, " ");
//             memset(message, 0, sizeof message);
//             sprintf(message, "UNREG %03i %02i", i, atoi(token));
//             receive2 = transrecieveUDP("193.136.138.142", "59000", message, strlen(message), &n_received);
//             if (strcmp(receive2, "OKUNREG") != 0)
//             {
//                 printf("DEU MERDA FAMILIA\n");
//                 exit(1);
//             }
//             printf("Tirei o nó %02i\n", atoi(token));
//             token = strtok_r(receive1, "\n", &receive1);
//         }
//         printf("\nDone!\n\n");
//     }
// }