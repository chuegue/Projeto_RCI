#include "User_Interface.h"
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

	// set socket timeout for sending
	struct timeval send_tv;
	send_tv.tv_sec = 1; // 1 second timeout for sending
	send_tv.tv_usec = 0;
	if (setsockopt(fd_udp, SOL_SOCKET, SO_SNDTIMEO, &send_tv, sizeof(send_tv)) < 0)
	{
		perror("Error setting socket timeout for sending");
		exit(1);
	}

	n = sendto(fd_udp, m_tosend, n_send, 0, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);
	if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Timeout while connecting to the Nodes Server with IP %s and UDP port %s.\n", ip, port);
			printf("Please try again with a different IP and/or port.\n");
			return (char *)NULL;
		}
		else
		{
			printf("error: %s\n", strerror(errno));
		}
		exit(1);
	}

	// set socket timeout for receiving
	struct timeval recv_tv;
	recv_tv.tv_sec = 1; // 1 second timeout for receiving
	recv_tv.tv_usec = 0;
	if (setsockopt(fd_udp, SOL_SOCKET, SO_RCVTIMEO, &recv_tv, sizeof(recv_tv)) < 0)
	{
		perror("Error setting socket timeout for receiving");
		exit(1);
	}

	char *buffer = (char *)calloc((256 << 5), sizeof(char));
	addrlen = sizeof(addr);
	n = recvfrom(fd_udp, buffer, 256 << 5, 0, (struct sockaddr *)&addr, &addrlen);
	close(fd_udp);
	if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			printf("Timeout while receiving data from the Nodes Server with IP %s and UDP port %s.\n", ip, port);
			printf("Please try again with a different IP and/or port.\n");
			return (char *)NULL;
		}
		else
		{
			printf("error: %s\n", strerror(errno));
		}
		exit(1);
	}
	char *ret = (char *)calloc(strlen(buffer) + 1, sizeof(char));
	strcpy(ret, buffer);
	free(buffer);
	*n_read = n;
	return ret;
}

void djoin(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	int fd, counter;
	struct addrinfo hints, *res;
	struct timeval timeout;
	fd_set rfds, wfds;

	self->net = commands->net;
	self->id = commands->id;
	if (commands->id == commands->bootid) /*Primeiro nó da rede*/
	{
		strcpy(self->ip, commands->bootip);
		strcpy(self->port, commands->bootport);
		other->id = -1;
		other->fd = -1;
		memcpy(&(nb->backup), self, sizeof(struct Node));
		memcpy(&(nb->external), self, sizeof(struct Node));
		nb->n_internal = 0;
		memset((void *)nb->internal, 0xFF, 100 * sizeof(struct Node));
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
		fcntl(fd, F_SETFL, O_NONBLOCK);
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(commands->bootip, commands->bootport, &hints, &res) != 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		// FD_ZERO(&wfds);
		// FD_SET(fd, &wfds);
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		wfds = rfds;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
		{
			// printf("saí do connect\n");
			if (errno != EINPROGRESS)
			{
				printf("erro no connect != EINPROGRESS\n");
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			else
			{
				counter = select(fd + 1, &rfds, &wfds, NULL, &timeout);
				if (counter == -1)
				{

					printf("erro no select do connect do djoin\n");
					printf("error: %s\n", strerror(errno));
					exit(1);
				}
				else if (counter == 0)
				{
					printf("COULDN'T CONNECT TO IP %s PORT %s\n", commands->bootip, commands->bootport);
					other->id = -1;
					other->fd = -1;
					self->net = -1; // teste aqui
					return;
				}
				else
				{
					int valopt;
					socklen_t optlen = sizeof(valopt);
					if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&valopt, &optlen) == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
					if (valopt != 0)
					{
						printf("COULDN'T CONNECT TO IP %s PORT %s\n", commands->bootip, commands->bootport);
						other->id = -1;
						other->fd = -1;
						self->net = -1; // teste aqui
						return;
					}
					char buffer[128] = {0};
					sprintf(buffer, "NEW %02i %.32s %.8s\n", self->id, self->ip, self->port);
					if (write(fd, buffer, strlen(buffer)) == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
					printf("EU ---> ID nº%i: %s\n", commands->bootid, buffer);
					nb->external.id = commands->bootid;
					strcpy(nb->external.ip, commands->bootip);
					strcpy(nb->external.port, commands->bootport);
					nb->external.fd = fd;
					nb->n_internal = 0;
					memset((void *)nb->internal, 0xFF, 100 * sizeof(struct Node));
					memset((void *)expt->forward, 0xFF, 100 * sizeof(int));
					other->fd = fd;
					other->id = commands->bootid;
					strcpy(other->ip, commands->bootip);
					strcpy(other->port, commands->bootport);
					other->net = commands->net;
				}
			}
		}
		else
		{
			char buffer[128] = {0};
			sprintf(buffer, "NEW %02i %.32s %.8s\n", self->id, self->ip, self->port);
			if (write(fd, buffer, strlen(buffer)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", commands->bootid, buffer);
			nb->external.id = commands->bootid;
			strcpy(nb->external.ip, commands->bootip);
			strcpy(nb->external.port, commands->bootport);
			nb->external.fd = fd;
			nb->n_internal = 0;
			memset((void *)nb->internal, 0xFF, 100 * sizeof(struct Node));
			memset((void *)expt->forward, 0xFF, 100 * sizeof(int));
			other->fd = fd;
			other->id = commands->bootid;
			strcpy(other->ip, commands->bootip);
			strcpy(other->port, commands->bootport);
			other->net = commands->net;
		}
	}
	freeaddrinfo(res);
}

void join(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport)
{
	char server_response[100][128], copy[128][100], buffer[128] = {0}, *received, *tok;
	unsigned int n_received, i = 0, j = 0, chosen;
	int ids[100], is_repeated, n, max_id = -1, t = 0;
	memset(ids, -1, 100 * sizeof(int));
	sprintf(buffer, "NODES %03i", commands->net);
	received = transrecieveUDP(nodesip, nodesport, buffer, strlen(buffer), &n_received);
	if (received == (char *)NULL)
		return;
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
			strcpy(server_response[i], tok);
			strcpy(copy[i], tok);
			i++;
			tok = strtok(NULL, "\n");
		}
		srand(time(NULL));
		// verificar se há ids repetidos
		is_repeated = 0;
		for (j = 0; j < i; j++)
		{
			n = atoi(strtok(server_response[j], " "));
			if (n > max_id)
				max_id = n;
			ids[n] = 1;
			if (commands->id == n)
				is_repeated = 1;
		}
		if (is_repeated)
		{
			for (t = 0; t < 100; t++)
			{
				if (ids[t] == -1)
				{
					commands->id = t;
					break;
				}
			}
			printf("ID INTRODUZIDO JÁ ESTAVA EM USO.\nA PARTIR DE AGORA, ESTE NÓ TEM ID %02i\n", t);
		}

		chosen = rand() % i;
		sscanf(copy[chosen], "%i %s %s\n", &(commands->bootid), commands->bootip, commands->bootport);
		if (is_repeated)
			commands->id = t;
		printf("NÃO SOU O PRIMEIRO NÓ DA REDE E VOU ME LIGAR AO ID Nº %i\n", commands->bootid);
		djoin(commands, self, other, nb, expt);
	}
	free(received);
	if (self->net != -1)
	{
		memset(buffer, 0, sizeof buffer);
		sprintf(buffer, "REG %03i %02i %.32s %.8s", commands->net, commands->id, self->ip, self->port);
		printf("EU ---> SERVIDOR DE NOS: %s\n", buffer);
		received = transrecieveUDP(nodesip, nodesport, buffer, strlen(buffer), &n_received);
		if (received == (char *)NULL)
			return;
		if (strcmp(received, "OKREG") != 0)
		{
			free(received);
			printf("ERRO: REG NÃO RESPONDEU OKREG\nRESPONDEU %s\n", received);
			exit(1);
		}
		printf("EU <--- SERVIDOR DE NOS: %s\n", received);
		free(received);
	}
}

void leave(struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport, int social)
{
	if (social)
	{
		char message[128];
		unsigned int n_received;

		// terminação da sessão com o vizinho x
		sprintf(message, "UNREG %03i %02i", self->net, self->id);
		printf("EU ---> SERVIDOR DE NOS: %s\n", message);
		char *received = transrecieveUDP(nodesip, nodesport, message, strlen(message), &n_received);
		if (received == (char *)NULL)
			return;
		if (strcmp(received, "OKUNREG") != 0)
		{
			free(received);
			printf("ERRO: UNREG NÃO RESPONDEU OKUNREG\nRESPONDEU %s\n", received);
			exit(1);
		}
		printf("EU <--- SERVIDOR DE NOS: %s\n", received);
		free(received);
	}
	Clean_Neighborhood(nb);
	memset(expt, -1, sizeof(struct Expedition_Table));
	self->net = -1;
}

void Invalid_User_Command()
{
	printf("The selected command is not valid. Here is a list of the available commands:\n");
	printf("\t> join [net] [id]\n");
	printf("\t> djoin [net] [id] [bootid] [bootip] [bootport]\n");
	printf("\t> create [name]\n");
	printf("\t> delete [name]\n");
	printf("\t> get [dest] [name]\n");
	printf("\t> clear routing (cr)\n");
	printf("\t> show topology (st)\n");
	printf("\t> show routing (sr)\n");
	printf("\t> show names (sn)\n");
	printf("\t> leave\n");
	printf("\t> exit\n");
}

void Process_User_Commands(char message[128], struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport, int social)
{
	char comm[16] = {0}, bootip[128] = {0}, bootport[8] = {0}, name[128] = {0};
	int net = -1, id = -1, bootid = -1, dest = -1;
	// djoin net id bootid bootIP bootTCP
	if (sscanf(message, "%5s %d %d %d %128s %8s", comm, &net, &id, &bootid, bootip, bootport) == 6)
	{
		if (strcmp(comm, "djoin") == 0)
		{
			commands->command = 2;
			if (!(net >= 0 && net <= 999))
			{
				printf("Selected net is invalid. Please choose a net between 0 and 999.\n");
				commands->command = -1;
				return;
			}
			if (!(id >= 0 && id <= 99))
			{
				printf("Selected id is invalid. Please choose an id between 0 and 99.\n");
				commands->command = -1;
				return;
			}
			if (!(bootid >= 0 && bootid <= 99))
			{
				printf("Selected bootid is invalid. Please choose a bootid between 0 and 99.\n");
				commands->command = -1;
				return;
			}
			commands->net = net;
			commands->id = id;
			commands->bootid = bootid;
			strcpy(commands->bootip, bootip);
			strcpy(commands->bootport, bootport);
			if (self->net == -1)
				djoin(commands, self, other, nb, expt);
			else
			{
				printf("Already in a network\n");
				commands->command = -1;
			}
		}
		else
		{
			Invalid_User_Command();
			commands->command = -1;
			return;
		}
		return;
	}
	// join net id
	else if (sscanf(message, "%4s %d %d", comm, &net, &id) == 3)
	{
		if (strcmp(comm, "join") == 0)
		{
			commands->command = 1;
			if (!(net >= 0 && net <= 999))
			{
				printf("Selected net is invalid. Please choose a net between 0 and 999.\n");
				commands->command = -1;
				return;
			}
			if (!(id >= 0 && id <= 99))
			{
				printf("Selected id is invalid. Please choose an id between 0 and 99.\n");
				commands->command = -1;
				return;
			}
			commands->net = net;
			commands->id = id;
			if (self->net == -1)
				join(commands, self, other, nb, expt, nodesip, nodesport);
			else
			{
				printf("Already in a network\n");
				commands->command = -1;
			}
		}
		else
		{
			Invalid_User_Command();
			commands->command = -1;
			return;
		}
		return;
	}
	// get dest name
	else if (sscanf(message, "%3s %d %128s", comm, &dest, name) == 3)
	{
		if (strcmp(comm, "get") == 0)
		{
			commands->command = 5;
			if (!(dest >= 0 && dest <= 99))
			{
				printf("Selected dest id is invalid. Please choose a dest id between 0 and 99.\n");
				commands->command = -1;
				return;
			}
			other->id = -1; // nao apagar, esta aqui por uma razao
			if (self->net != -1)
			{
				if (dest != self->id)
					Send_Query(dest, self->id, name, other, nb, expt);
			}
		}
		else
		{
			Invalid_User_Command();
			commands->command = -1;
			return;
		}
		return;
	}
	// create name / delete name / show topology / show routing / show names / clear routing
	else if (sscanf(message, "%6s %128s", comm, name) == 2)
	{
		if (strcmp(comm, "create") == 0)
		{
			commands->command = 3;
			strcpy(commands->name, name);
			return;
		}
		else if (strcmp(comm, "delete") == 0)
		{
			commands->command = 4;
			strcpy(commands->name, name);
			return;
		}
		else if (strcmp(comm, "show") == 0)
		{
			if (strcmp(name, "topology") == 0)
			{
				commands->command = 6;
				return;
			}
			else if (strcmp(name, "routing") == 0)
			{
				commands->command = 8;
				return;
			}
			else if (strcmp(name, "names") == 0)
			{
				commands->command = 7;
				return;
			}
			else
			{
				Invalid_User_Command();
				commands->command = -1;
			}
		}
		else if (strcmp(comm, "clear"))
		{
			if (strcmp(name, "routing") == 0)
			{
				commands->command = 11;
				memset(expt->forward, -1, 100 * sizeof(int));
				return;
			}
			else
			{
				Invalid_User_Command();
				commands->command = -1;
				return;
			}
		}
		else
		{
			Invalid_User_Command();
			commands->command = -1;
			return;
		}
	}
	// leave / exit / cr / st / sr / sn
	else if (sscanf(message, "%5s", comm) == 1)
	{
		if (strcmp(comm, "leave") == 0)
		{
			if (self->net != -1)
			{
				commands->command = 9;
					leave(self, nb, expt, nodesip, nodesport, social);
			}
			else
			{
				printf("You are not in a network!\n");
				commands->command = -1;
			}
		}
		else if (strcmp(comm, "exit") == 0)
		{
			commands->command = 10;
			if (self->net != -1)
			{
					leave(self, nb, expt, nodesip, nodesport, social);
			}
			return;
		}
		else if (strcmp(comm, "cr") == 0)
		{
			commands->command = 11;
			memset(expt->forward, -1, 100 * sizeof(int));
			return;
		}
		else if (strcmp(comm, "st") == 0)
		{
			commands->command = 6;
			return;
		}
		else if (strcmp(comm, "sr") == 0)
		{
			commands->command = 8;
			return;
		}
		else if (strcmp(comm, "sn") == 0)
		{
			commands->command = 7;
			return;
		}
		else
		{
			Invalid_User_Command();
			commands->command = -1;
			return;
		}
		return;
	}
	// caca
	else
	{
		Invalid_User_Command();
		commands->command = -1;
		return;
	}
}