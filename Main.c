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

struct Node
{
	int net;
	int id;
	char ip[128];
	char port[128];
	int fd;
};

struct Neighborhood
{
	int external;
	int backup;
	int internal[100];
	int n_internal;
};

struct Expedition_Table
{
	int forward[100];
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
	int fd;
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

// char message[128];
// 		int n_received;
// 		sprintf(message, "REG %03i %02i %.32s %.8s", commands->net, commands->id, self->ip, self->port);
// 		printf("EU ---> SERVIDOR DE NOS: %s\n", message);
// 		char *received = transrecieveUDP(nodesip, nodesport, message, strlen(message), &n_received);
// 		if (strcmp(received, "OKREG") != 0)
// 		{
// 			free(received);
// 			printf("ERRO: REG NÃO RESPONDEU OKREG\nRESPONDEU %s\n", received);
// 			exit(1);
// 		}
// 		printf("EU <--- SERVIDOR DE NOS: %s\n", received);
// 		free(received);

void djoin(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	int fd;
	struct addrinfo hints, *res;
	self->net = commands->net;
	self->id = commands->id;
	if (commands->id == commands->bootid) /*Primeiro nó da rede*/
	{
		other->id = -1;
		other->fd = -1;
		nb->backup = self->id;
		nb->external = self->id;
		nb->n_internal = 0;
		memset((void *)nb->internal, 0xFF, 100 * sizeof(int));
		memset((void *)expt->forward, 0xFF, 100 * sizeof(int));
		return;
	}
	else if (commands->id != commands->bootid) /*Conectar a algum nó da rede*/
	{
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(commands->bootip, commands->bootport, &hints, &res) != 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		char buffer[128] = {0};
		sprintf(buffer, "NEW %02i %.32s %.8s\n", self->id, self->ip, self->port);
		if (write(fd, buffer, strlen(buffer)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> FD nº%i: %s\n", fd, buffer);
		nb->external = commands->bootid;
		nb->n_internal = 0;
		memset((void *)nb->internal, 0xFF, 100 * sizeof(int));
		memset((void *)expt->forward, 0xFF, 100 * sizeof(int));
		expt->forward[commands->bootid] = commands->bootid;
		other->fd = fd;
		other->id = commands->bootid;
		strcpy(other->ip, commands->bootip);
		strcpy(other->port, commands->bootport);
		other->net = commands->net;
	}
}

void join(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport)
{
	char server_response[100][128], buffer[128] = {0}, *received, *tok;
	int n_received, i = 0, chosen;
	sprintf(buffer, "NODES %03i", commands->net);
	received = transrecieveUDP(nodesip, nodesport, buffer, strlen(buffer), &n_received);
	memset(buffer, 0, sizeof buffer);
	sprintf(buffer, "NODESLIST %03i", commands->net);
	tok = strtok(received, "\n");
	if (strcmp(tok, buffer) != 0)
	{
		printf("Erro no join\n");
		exit(1);
	}
	if ((tok = strtok(NULL, "\n")) == NULL) // primeiro nó
	{
		printf("SOU O PRIMEIRO NÓ DA REDE\n");
		commands->bootid = commands->id;
		strcpy(commands->bootip, self->ip);
		strcpy(commands->bootport, self->port);
		djoin(commands, self, other, nb, expt);
	}
	else // segundo nó
	{
		while (tok)
		{
			strcpy(server_response[i++], tok);
			tok = strtok(NULL, "\n");
		}
		srand(time(NULL));
		chosen = rand() % i;
		sscanf(server_response[chosen], "%i %s %s\n", &(commands->bootid), commands->bootip, commands->bootport);
		djoin(commands, self, other, nb, expt);
		printf("NÃO SOU O PRIMEIRO NÓ DA REDE E VOU ME LIGAR AO ID Nº %i\n", commands->bootid);
	}
	free(received);
	memset(buffer, 0, sizeof buffer);
	sprintf(buffer, "REG %03i %02i %.32s %.8s", commands->net, commands->id, self->ip, self->port);
	printf("EU ---> SERVIDOR DE NOS: %s\n", buffer);
	received = transrecieveUDP(nodesip, nodesport, buffer, strlen(buffer), &n_received);
	if (strcmp(received, "OKREG") != 0)
	{
		free(received);
		printf("ERRO: REG NÃO RESPONDEU OKREG\nRESPONDEU %s\n", received);
		exit(1);
	}
	printf("EU <--- SERVIDOR DE NOS: %s\n", received);
	free(received);
}

void leave(struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport)
{
	char message[128];
	unsigned int n_received;

	// terminação da sessão com o vizinho x
	sprintf(message, "UNREG %03i %02i", self->net, self->id);
	printf("EU ---> SERVIDOR DE NOS: %s\n", message);
	char *received = transrecieveUDP(nodesip, nodesport, message, strlen(message), &n_received);
	if (strcmp(received, "OKUNREG") != 0)
	{
		free(received);
		printf("ERRO: UNREG NÃO RESPONDEU OKUNREG\nRESPONDEU %s\n", received);
		exit(1);
	}
	printf("EU <--- SERVIDOR DE NOS: %s\n", received);
	free(received);

	// como raio deteto que foi terminada uma sessao?
}

void missing_arguments()
{
	printf(" Faltam argumentos! \n");
}

void Process_User_Commands(char message[128], struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport)
{
	char *token = strtok(message, " ");

	// join net id
	if (strcmp(token, "join") == 0)
	{
		commands->command = 1;
		token = strtok(NULL, " ");
		for (int k = 0; k < 2; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				commands->net = atoi(token);
				break;
			case 1:
				commands->id = atoi(token);
				break;
			default:
				break;
			}
			token = strtok(NULL, " ");
		}
		join(commands, self, other, nb, expt, nodesip, nodesport);
		return;
	}

	// djoin net id bootid bootIP bootTCP
	if (strcmp(token, "djoin") == 0)
	{
		token = strtok(NULL, " ");
		commands->command = 2; /*?*/
		for (int k = 0; k < 5; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				commands->net = atoi(token);
				break;
			case 1:
				commands->id = atoi(token);
				break;
			case 2:
				commands->bootid = atoi(token);
				break;
			case 3:
				strcpy(commands->bootip, token);
				break;
			case 4:
				strcpy(commands->bootport, token);
				(commands->bootport)[strlen(commands->bootport) - 1] = '\0';
				break;

			default:
				break;
			}

			token = strtok(NULL, " ");
		}
		djoin(commands, self, other, nb, expt);
		return;
	}

	// create name
	if (strcmp(token, "create") == 0)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
			exit(1);
		}
		else if (strcmp(token, "name") == 0)
		{
			printf("You are in create name! \n");
		}
	}

	// delete name
	if (strcmp(token, "delete") == 0)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
			exit(1);
		}
		else if (strcmp(token, "name") == 0)
		{
			printf("You are in delete name! \n");
		}
	}

	// get dest name
	if (strcmp(token, "get") == 0)
	{
		token = strtok(NULL, " ");
		for (int k = 0; k < 2; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			printf("Ciclo join: %s\n", token);
			token = strtok(NULL, " ");
		}
	}

	// show topology || show names || show routing
	if (strcmp(token, "show") == 0)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
			exit(1);
		}
		else if (strcmp(token, "topology") == 0)
		{
			printf("You are in show topology! \n");
		}
		else if (strcmp(token, "names") == 0)
		{
			printf("You are in show names! \n");
		}
		else if (strcmp(token, "routing") == 0)
		{
			printf("You are in show routing! \n");
		}
		else
		{
			printf("Not valid! \n");
		}
	}

	if (strstr(message, "leave") != NULL)
	{
		commands->command = 9;
		leave(self, nb, expt, nodesip, nodesport);
		return;
	}

	if (strstr(message, "exit") != NULL)
	{
		exit(0);
	}
}

void Process_Incoming_Messages(struct Node *other, struct Node *self, struct Neighborhood nb, char incoming_message[128])
{
	char outgoing_message[128] = {0};
	printf("EU <--- FD nº%i: %s\n", other->fd, incoming_message);
	char *token = strtok(incoming_message, " ");
	if (strcmp(token, "NEW") == 0)
	{
		token = strtok(NULL, " ");
		for (int k = 0; k < 2; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				strcpy(other->ip, token);
				break;
			case 1:
				strcpy(other->port, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb.external, self->ip, self->port);
		if (write(other->fd, outgoing_message, strlen(outgoing_message)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> FD nº%i: %s\n", other->fd, outgoing_message);
	}
}

int main(int argc, char *argv[])
{
	// Declare variables
	int max_fd, counter, listen_fd, comms_fd, n, num_connections = 0;
	char buffer1[128], myip[128], myport[128], nodeip[128], nodeport[128];
	struct User_Commands usercomms;
	struct Node my_connections[100] = {0};
	struct Node self, other = {0};
	struct Neighborhood nb;
	struct Expedition_Table expt;
	fd_set rfds;
	struct sockaddr addr;
	socklen_t addrlen;

	// Process console arguments
	Process_Console_Arguments(argc, argv, myip, myport, nodeip, nodeport);

	// Update "self" structure
	strcpy(self.ip, myip);
	strcpy(self.port, myport);

	// Open a TCP port to listen for incoming connections
	listen_fd = openListenTCP(myport);

	// Update "self" structure
	self.fd = listen_fd;

	// set max_fd to listen_fd
	max_fd = listen_fd;

	// Main loop
	while (1)
	{
		// Set the standard input and listen fd's to be listened
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(listen_fd, &rfds);

		// Set the fd's of connections to other users
		for (int temp = num_connections - 1; temp >= 0; temp--)
		{
			FD_SET(my_connections[temp].fd, &rfds);
		}

		// Check which fd's are set
		counter = select(max_fd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
		if (counter <= 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

		// Check if there is any incoming new connections
		if (FD_ISSET(listen_fd, &rfds))
		{
			FD_CLR(listen_fd, &rfds);
			addrlen = sizeof(addr);
			if ((comms_fd = accept(listen_fd, &addr, &addrlen)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("\nCONNECTION IN FILE DESCRIPTOR Nº%i\n", comms_fd);

			// Update array of connections to include the new fd
			my_connections[num_connections++].fd = comms_fd;
			// Update max_fd
			max_fd = max(max_fd, comms_fd);
			counter--;
		}

		// Check if there is any input in the standard input
		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
			FD_CLR(STDIN_FILENO, &rfds);
			if (fgets(buffer1, 128, stdin))
			{
				Process_User_Commands(buffer1, &usercomms, &self, &other, &nb, &expt, nodeip, nodeport);

				// According to the command given by the users, do what needs to be done to each of them
				switch (usercomms.command)
				{
				case 2: // djoin
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					break;
				case 9: // leave
					for (int i = 0; i < num_connections; i++)
					{
						close(my_connections[i].fd);
					}
					num_connections = 0;
					break;

				default:
					break;
				}
			}
			else
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			counter--;
		}

		// Check for any incoming messages from other nodes
		for (; counter /*>0*/; --counter)
		{
			// Loop through all of the other nodes
			for (int i = num_connections - 1; i >= 0; i--)
			{
				// Check if a given fd has had any changes
				if (FD_ISSET(my_connections[i].fd, &rfds))
				{
					FD_CLR(my_connections[i].fd, &rfds);
					memset(buffer1, 0, sizeof buffer1);
					n = read(my_connections[i].fd, buffer1, 128);
					if (n > 0)
					{
						Process_Incoming_Messages(&(my_connections[i]), &self, nb, buffer1);
						// TODO: atualizar o array de nós de acordo com o comando
					}
					else if (n == 0)
					{
						/* REAJUSTAR A REDE */
						printf("este malandro saiu: %i \n", my_connections[i].fd);
						if (i != num_connections - 1)
							memcpy(&(my_connections[i]), &(my_connections[--num_connections]), sizeof(struct Node));
						else
							num_connections--;
						max_fd = 0;
						for (int i = 0; i < num_connections; i++)
						{
							if (my_connections[i].fd > max_fd)
								max_fd = my_connections[i].fd;
						}
					}
					else if (n == -1)
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