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

#define max(A, B) ((A) >= (B) ? (A) : (B))

struct User_Commands
{
	int command;
	int net;
	int id;
	int bootid;
	char bootip[128];
	char bootport[128];
	char name[128];
	int tnr; // topology-names-routing
};

/*Returns a string containing the IP of this local machine (XXX.XXX.XXX.XXX)
which needs to be freed after use*/
char *Own_IP()
{
	int n;
	struct ifreq ifr;
	char array[] = "eth0";
	n = socket(AF_INET, SOCK_DGRAM, 0);
	// Type of address to retrieve - IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;
	// Copy the interface name in the ifreq structure
	strncpy(ifr.ifr_name, array, IFNAMSIZ - 1);
	ioctl(n, SIOCGIFADDR, &ifr);
	close(n);
	// display result
	// printf("IP Address is %s - %s\n", array, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	char *ret = (char *)malloc((strlen(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)) + 1) * sizeof(char));
	strcpy(ret, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	return ret;
}

/*Sends a message in string [m_tosend] containing [n_send] chars to the UDP
server with IP [ip] and PORT [port]. Returns the response of the server in a string that needs
to be freed after use and puts the number of chars read in [n_read]
*/
char *transrecieveUDP(char *ip, char *port, char *m_tosend, unsigned int n_send, unsigned int *n_read)
{
	int fd_udp, errcode;
	ssize_t n;
	struct addrinfo hints, *res;
	struct sockaddr_in addr;
	socklen_t addrlen;
	fd_udp = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if (fd_udp == -1)
		exit(1);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;		// IPv4
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

/*Returns the file descriptor of the listening socket in port [port]*/
int openListenTCP(char *port)
{
	struct addrinfo hints, *res;
	int fd, errcode;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(port));
	if (bind(fd, (struct sockaddr *)(&address), sizeof(address)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	if (listen(fd, 5) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	return fd;
}

void Process_Console_Arguments(int argc, char *argv[], char myip[128], char myport[128], char nodeip[128], char nodeport[128])
{
	if (argc != 5)
		exit(1);
	strcpy(myip, argv[1]);
	strcpy(myport, argv[2]);
	strcpy(nodeip, argv[3]);
	strcpy(nodeport, argv[4]);
}

void djoin(struct User_Commands *commands)
{

}

void Process_User_Commands(char message[128], struct User_Commands *commands)
{
	char *tok = strtok(message, " ");
	if (strcmp(tok, "join") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "djoin") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "create") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "delete") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "get") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "show") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "leave") == 0)
	{ /*TODO*/
	}
	else if (strcmp(tok, "exit") == 0)
	{ /*TODO*/
	}
}

int main(int argc, char *argv[])
{
	int max_fd, counter, comms_fd, n, nw;
	char buffer1[128], *buffer2;
	int connections[100] = {0};
	int num_connections = 0;
	fd_set rfds;
	struct sockaddr addr;
	socklen_t addrlen;
	char myip[128], myport[128], nodeip[128], nodeport[128];
	printf("\n%s\n", Own_IP());
	Process_Console_Arguments(argc, argv, myip, myport, nodeip, nodeport);
	int listen_fd = openListenTCP(myport);
	max_fd = listen_fd;
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(listen_fd, &rfds);
		for (int temp = num_connections - 1; temp >= 0; temp--)
		{
			FD_SET(connections[temp], &rfds);
		}
		counter = select(max_fd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
		if (counter <= 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		if (FD_ISSET(listen_fd, &rfds))
		{
			FD_CLR(listen_fd, &rfds);
			addrlen = sizeof(addr);
			if ((comms_fd = accept(listen_fd, &addr, &addrlen)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("\nALGUÉM SE LIGOU NO FILE DESCRIPTOR %i\n", comms_fd);
			connections[num_connections++] = comms_fd;
			max_fd = max(max_fd, comms_fd);
			counter--;
		}
		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
			FD_CLR(STDIN_FILENO, &rfds);
			if (fgets(buffer1, 128, stdin))
			{
				/*process message*/
			}
			else
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			counter--;
		}
		for (; counter /*>0*/; --counter)
		{
			for (int i = num_connections - 1; i >= 0; i--)
			{
				if (FD_ISSET(connections[i], &rfds))
				{
					FD_CLR(connections[i], &rfds);
					if ((n = read(connections[i], buffer1, 128)) != -1)
					{
						strcpy(buffer2, buffer1);
						while (n > 0)
						{
							if ((nw = write(connections[i], buffer2, n)) != -1)
							{

								n -= nw;
								buffer2 += nw;
							}
							else
							{
								printf("error: %s\n", strerror(errno));
								exit(1);
							}
						}
					}
					else
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
				}
			}
		}
	}
	return 0;
}