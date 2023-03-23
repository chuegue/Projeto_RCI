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

void djoin(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	int fd;
	struct addrinfo hints, *res;
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
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
		memset(&hints, 0, sizeof hints);
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
		printf("EU ---> ID nº%i: %s\n", commands->bootid, buffer);
		nb->external.id = commands->bootid;
		strcpy(nb->external.ip, commands->bootip);
		strcpy(nb->external.port, commands->bootport);
		nb->external.fd = fd;
		nb->n_internal = 0;
		memset((void *)nb->internal, 0xFF, 100 * sizeof(struct Node));
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
	char server_response[100][128], copy[128][100], buffer[128] = {0}, *received, *tok;
	unsigned int n_received, i = 0, j = 0, chosen;
	int ids[100], is_repeated, n, max_id = 0, t = 0;
	memset(ids, -1, 100 * sizeof(int));
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
			for (t = 0; t < max_id; t++)
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
	memset(nb, -1, sizeof(struct Neighborhood));
	memset(expt, -1, sizeof(struct Expedition_Table));
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
		if (token[strlen(token) - 1] == '\n')
			token[strlen(token) - 1] = '\0';
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
		if (token[strlen(token) - 1] == '\n')
			token[strlen(token) - 1] = '\0';
		strcpy(commands->name, token);
		return;
	}

	// get dest name
	else if (strcmp(token, "get") == 0)
	{
		commands->command = 5;
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
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';
				strcpy(commands->name, token);
				break;
			default:
				break;
			}
			token = strtok(NULL, " ");
		}
		other->id = -1;
		Send_Query(commands->id, self->id, commands->name, other, nb, expt);
	}
	else if (strcmp(token, "clear") == 0)
	{
		token = strtok(NULL, " ");
		if (strstr(token, "routing") != NULL)
		{
			commands->command = 11;
			memset(expt->forward, -1, 100 * sizeof(int));
		}
	}
	else if (strstr(message, "cr") != NULL)
	{
		commands->command = 11;
		memset(expt->forward, -1, 100 * sizeof(int));
	}
	else if (strstr(message, "st") != NULL)
	{
		commands->command = 6;
	}
	else if (strstr(message, "sn") != NULL)
	{
		commands->command = 7;
	}
	else if (strstr(message, "sr") != NULL)
	{
		commands->command = 8;
	}
	// show topology || show names || show routing
	else if (strcmp(token, "show") == 0)
	{
		token = strtok(NULL, " ");
		if (token == NULL)
		{
			missing_arguments();
		}
		else if (strstr(token, "topology") != NULL)
		{
			commands->command = 6;
		}
		else if (strstr(token, "names") != NULL)
		{
			commands->command = 7;
		}
		else if (strstr(token, "routing") != NULL)
		{
			commands->command = 8;
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
		commands->command = 10;
	}
	else if (strstr(message, "connections") != NULL)
	{
		commands->command = 69;
	}
}