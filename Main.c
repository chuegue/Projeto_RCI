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

#include "User_Interface.h"
#include "Structs.h"
#include "List.h"

#define max(A, B) ((A) >= (B) ? (A) : (B))
int eqstring(Item a, Item b)
{
	return strcmp((char *)a, (char *)b);
}

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

/*Returns the file descriptor of the listening socket in port [port]*/
int openListenTCP(char *port)
{
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
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

void Connect_To_Backup(struct Node *self, struct Node *backup)
{
	int fd;
	struct addrinfo hints, *res;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(backup->ip, backup->port, &hints, &res) != 0)
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
	backup->fd = fd;
	printf("EU ---> ID nº%i: %s\n", backup->id, buffer);
}

void Leaving_Neighbour(struct Node *self, struct Node *leaver, struct Neighborhood *nb, struct Expedition_Table *expt, struct Node connections[100], int *num_connections)
{
	int chosen;

	// ending session with non external neighbour
	int leaver_id = leaver->id;
	for (int i = 0; i < nb->n_internal; i++)
	{
		if (nb->internal[i] == leaver_id)
		{
			nb->internal[i] = -1;
			nb->n_internal--;
			memcpy(leaver, &(connections[*num_connections - 1]), sizeof(struct Node));
			(*num_connections)--;
			return;
		}
	}

	if (nb->backup.id != self->id) // ending session with external neighbour (non anchor)
	{
		int i;
		for (i = 0; i < *num_connections; i++)
		{
			if (connections[i].id == nb->backup.id)
				break;
		}

		Connect_To_Backup(self, &(nb->backup));
		nb->external.id = nb->backup.id;
		strcpy(nb->external.ip, nb->backup.ip);
		strcpy(nb->external.port, nb->backup.port);
		nb->external.fd = nb->backup.fd;

		memcpy(leaver, &(nb->external), sizeof(struct Node));

		for (int i = 0; i < *num_connections; i++)
		{
			char outgoing_message[128] = {0};
			sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
			for (int j = 0; j < nb->n_internal; j++)
			{
				if (connections[i].id == nb->internal[j])
				{
					if (write(connections[i].fd, outgoing_message, strlen(outgoing_message)) == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
					printf("EU ---> ID nº%i: %s\n", connections[i].id, outgoing_message);
				}
			}
		}
	}
	else if (nb->n_internal != 0) // ending session with external neighbour (ânchor)
	{
		// choose last id of the internal neighbours
		nb->external.id = nb->internal[(nb->n_internal) - 1];

		for (int e = 0; e < *num_connections; e++)
		{
			if (connections[e].id == nb->internal[(nb->n_internal) - 1])
			{
				strcpy(nb->external.ip, connections[e].ip);
				strcpy(nb->external.port, connections[e].port);
				nb->external.fd = connections[e].fd;
			}
		}

		memcpy(leaver, &(connections[*num_connections - 1]), sizeof(struct Node));
		(*num_connections)--;

		for (int i = 0; i < *num_connections; i++) // send EXTERN to every internal
		{
			char outgoing_message[128] = {0};
			sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
			for (int j = 0; j < nb->n_internal; j++)
			{
				if (connections[i].id == nb->internal[j])
				{
					if (write(connections[i].fd, outgoing_message, strlen(outgoing_message)) == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
					printf("EU ---> ID nº%i: %s\n", connections[i].id, outgoing_message);
				}
			}
		}
		nb->n_internal--;
		nb->internal[nb->n_internal] = -1;
	}
	else
	{
		memcpy(&(nb->external), self, sizeof(struct Node));
		(*num_connections)--;
	}
}

void missing_arguments()
{
	printf(" Faltam argumentos! \n");
}

void Send_Query(int dest, int orig, char name, struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	char buffer[128] = {0};
	sprintf(buffer, "QUERY %02i %02i %.8s\n", dest, self->id, name);
	if (expt->forward[dest] != -1) // if my destination id on my expedition table send directly there
	{
		if (write(expt->forward[dest], buffer, strlen(buffer)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", expt->forward[dest], buffer);
	}
	else // if not, send QUERY to every neighbour
	{
		for (int i = 0; i < nb->n_internal; i++) // send to every internal neighbour
		{
			if (write(nb->internal.fd, buffer, strlen(buffer)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", nb->internal.fd, buffer);
		}
		if (write(nb->external.fd, buffer, strlen(buffer)) == -1) // send to external neighbour
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", nb->external.fd, buffer);
		if (write(nb->backup.fd, buffer, strlen(buffer)) == -1) // send to backup neighbour
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", nb->backup.fd, buffer);
	}
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
	else if (strcmp(token, "djoin") == 0)
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
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';
				strcpy(commands->bootport, token);
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
	else if (strcmp(token, "create") == 0)
	{
		commands->command = 3;
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
			exit(1);
		}
		strcpy(commands->name, token);
		return;
	}

	// delete name
	else if (strcmp(token, "delete") == 0)
	{
		commands->command = 4;
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
			exit(1);
		}
		strcpy(commands->name, token);
		return;
	}

	// get dest name
	else if (strcmp(token, "get") == 0)
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
				commands->id = atoi(token);
				break;
			case 1:
				strcpy(commands->name, token);
				break;
			default:
				break;
			}
			token = strtok(NULL, " ");
		}
		Send_Query(commands->id, self->id, commands->name, self, nb, expt);
	}

	// show topology || show names || show routing
	else if (strcmp(token, "show") == 0)
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

	else if (strstr(message, "leave") != NULL)
	{
		commands->command = 9;
		leave(self, nb, expt, nodesip, nodesport);
		return;
	}

	else if (strstr(message, "exit") != NULL)
	{
		exit(0);
	}
	else if (strstr(message, "connections") != NULL)
	{
		commands->command = 69;
	}
}

int Process_Incoming_Messages(struct Node *other, struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char incoming_message[128])
{
	char outgoing_message[128] = {0};
	char holder[128] = {0};
	strcpy(holder, incoming_message);
	char *token = strtok(incoming_message, " ");
	if (strcmp(token, "NEW") == 0)
	{
		token = strtok(NULL, " ");
		for (int k = 0; k < 3; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				other->id = atoi(token);
				break;
			case 1:
				strcpy(other->ip, token);
				break;
			case 2:
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';
				strcpy(other->port, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
		if (write(other->fd, outgoing_message, strlen(outgoing_message)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", other->id, outgoing_message);
		expt->forward[other->id] = other->id;
		if (nb->external.id == self->id) // tou sozinho, quero ancora
		{
			memcpy(&(nb->external), other, sizeof(struct Node));
			nb->backup.id = self->id;
			strcpy(nb->backup.ip, self->ip);
			strcpy(nb->backup.port, self->port);
			nb->backup.fd = self->fd;
		}
		else // só mais um interno
		{
			nb->internal[(nb->n_internal)++] = other->id;
		}
		return 'n';
	}
	else if (strcmp(token, "EXTERN") == 0)
	{
		token = strtok(NULL, " ");
		int e;
		for (int k = 0; k < 3; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				e = atoi(token);
				if (e == nb->external.id)
					nb->backup.id = self->id;
				else
					nb->backup.id = e;
				break;
			case 1:
				if (e == nb->external.id)
					strcpy(nb->backup.ip, self->ip);
				else
					strcpy(nb->backup.ip, token);
				break;
			case 2:
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';

				if (e == nb->external.id)
					strcpy(nb->backup.port, self->port);
				else
					strcpy(nb->backup.port, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		return 'e';
	}
	else if (strcmp(token, "QUERY") == 0)
	{
		int dest, orig;
		char name;
		token = strtok(NULL, " ");
		for (int k = 0; k < 3; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			switch (k)
			{
			case 0:
				dest = atoi(token);
				break;
			case 1:
				orig = atoi(token);
				break;
			case 2:
				strcpy(name, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		if (dest == self->id) // if i am the node they searching for
		{
			// procurar na linked list de conteudos
		}
		else
		{
			Send_Query(dest, orig, name, self, nb, expt);
		}
	}
	else if (strcmp(token, "WITHDRAW"))
	{
		// do stuff
		return 'w';
	}
	return 0;
}

void Show_Topology(struct Neighborhood *nb, struct Node connections[100], int num_connections)
{
	printf("VIZINHOS INTERNOS \n");
	for (int i = 0; i < num_connections; i++)
	{
		if ((connections[i].id != nb->external.id) && (connections[i].id != nb->backup.id))
			printf("id: %i \t ip: %s \t port: %s \n", connections[i].id, connections[i].ip, connections[i].port);
	}
	printf("\n");

	printf("VIZINHO EXTERNO \n");
	printf("id: %i \t ip: %s \t port: %s \n \n", nb->external.id, nb->external.ip, nb->external.port);

	printf("VIZINHO DE RECUPERAÇÃO \n");
	printf("id: %i \t ip: %s \t port: %s \n \n", nb->backup.id, nb->backup.ip, nb->backup.port);
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
	List *list = Init_List();

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
		max_fd = max(max_fd, listen_fd);

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
				case 1: // join
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					break;
				case 2: // djoin
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					break;
				case 3: // create
					list = Add_Beginning_List(list, (Item)0);
					list = Change_At_Begining_List(list, (Item)(&(usercomms.name)));
					break;
				case 4: // delete
					list = Delete_At_Index_Lista(list, Search_Item_List(list, (Item) & (usercomms.name), eqstring));
					break;
				case 9: // leave
					for (int i = 0; i < num_connections; i++)
					{
						close(my_connections[i].fd);
					}
					num_connections = 0;
					break;
				case 69:
					for (int i = 0; i < num_connections; i++)
					{
						printf("ID: %02i\nNET: %02i\nIP: %.32s\nPORT: %.8s\nFD: %i\n\n", my_connections[i].id, my_connections[i].net, my_connections[i].ip, my_connections[i].port, my_connections[i].fd);
					}
					printf("NEIGHBOURHOOD:\nEXTERN: %i\nBACKUP: %i\nINTERNALS: %i ---> ", nb.external.id, nb.backup.id, nb.n_internal);
					for (int i = 0; i < nb.n_internal; i++)
					{
						printf("%i ", nb.internal[i]);
					}
					printf("\n\n");
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
						n = Process_Incoming_Messages(&(my_connections[i]), &self, &nb, &expt, buffer1);
						int j, l;
						switch (n)
						{
						case 'n':
							break;
						case 'w':
							/*implement broacasting of messages*/
							break;

						default:
							break;
						}
					}
					else if (n == 0)
					{
						close(my_connections[i].fd);
						printf("este malandro saiu: %i \n", my_connections[i].fd);
						Leaving_Neighbour(&self, &(my_connections[i]), &nb, &expt, my_connections, &num_connections);
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