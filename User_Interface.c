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
	unsigned int n_received, i = 0, chosen;
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
}
