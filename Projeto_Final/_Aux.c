#include "_Aux.h"

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
	{
		printf("»»» Number of console arguments provided was wrong.\n");
		printf("    The correct way to call the program is ./cot [ip] [TCP port] [regIP] [regUDP port]\n");
		printf("    Please try again\n \n");
		exit(1);
	}
	strcpy(myip, argv[1]);
	strcpy(myport, argv[2]);
	strcpy(nodeip, argv[3]);
	strcpy(nodeport, argv[4]);
}

int Gimme_Fd(int wanted_id, struct Neighborhood *nb)
{
	if (nb->backup.id == wanted_id)
		return nb->backup.fd;
	else if (nb->external.id == wanted_id)
		return nb->external.fd;

	for (int i = 0; i < nb->n_internal; i++)
	{
		if (nb->internal[i].id == wanted_id)
			return nb->internal[i].fd;
	}
	return -1;
}

void Clean_Neighborhood(struct Neighborhood *nb)
{
	memset(nb, -1, sizeof(struct Neighborhood));
	memset(nb->external.ip, '\0', sizeof nb->external.ip);
	memset(nb->external.port, '\0', sizeof nb->external.port);
	memset(nb->backup.ip, '\0', sizeof nb->backup.ip);
	memset(nb->backup.port, '\0', sizeof nb->backup.port);
	for (int i = 0; i < nb->n_internal; i++)
	{
		memset(nb->internal[i].ip, '\0', sizeof nb->internal[i].ip);
		memset(nb->internal[i].port, '\0', sizeof nb->internal[i].port);
	}
}

void Process_Incoming_Messages(struct Node *other, struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char incoming_message[128], List *list)
{
	strcat(other->buffer, incoming_message);
	if (strlen(other->buffer) >= sizeof(other->buffer) - 258 && strstr(other->buffer, "\n") == NULL)
	{
		memset(other->buffer, 0, sizeof(other->buffer));
		other->net = -10;
	}
	int id, dest, orig;
	char ip[64] = {0}, port[8] = {0}, name[128] = {0};
	while (strstr(other->buffer, "\n") != NULL)
	{
		char processed_message[128] = {0};
		int b, c;
		for (c = 0; other->buffer[c] != '\n' && c < sizeof processed_message; c++)
		{
			processed_message[c] = other->buffer[c];
		}
		if (c < sizeof processed_message)
			processed_message[c] = other->buffer[c];
		for (b = 0, c = c + 1; other->buffer[c] != '\0' && c < sizeof other->buffer; c++, b++)
		{
			other->buffer[b] = other->buffer[c];
		}
		for (; b < sizeof other->buffer; b++)
		{
			other->buffer[b] = 0;
		}
		char outgoing_message[128] = {0};
		char holder[128] = {0};
		strcpy(holder, processed_message);
		char *token = strtok(processed_message, " ");
		char aux[1024] = {0};
		strcpy(aux, token);
		int i;
		for (i = 0; processed_message[i + strlen(aux) + 1] != '\0' && i < sizeof processed_message; i++)
		{
			processed_message[i] = processed_message[i + strlen(aux) + 1];
		}
		for (; i < sizeof processed_message; i++)
			processed_message[i] = 0;
		if (processed_message[strlen(processed_message) - 1] == '\n')
			processed_message[strlen(processed_message) - 1] = '\0';
		if (strcmp(aux, "NEW") == 0)
		{
			if (sscanf(processed_message, "%d %64s %8s", &id, ip, port) == 3)
			{
				other->id = id;
				strcpy(other->ip, ip);
				if (port[strlen(port) - 1] == '\n')
					port[strlen(port) - 1] = '\0';
				strcpy(other->port, port);
			}
			else
			{
				continue;
			}
			printf("»»» Node with ID %02i connected to me.\n", other->id);
			if (nb->external.id == self->id) // tou sozinho, quero ancora
			{
				printf("    It is now an Anchor Node with me.\n");
				memcpy(&(nb->external), other, sizeof(struct Node));
				memcpy(&(nb->backup), self, sizeof(struct Node));
			}
			else // só mais um interno
			{
				printf("    It is now one of my internal neighbours\n");
				memcpy(&(nb->internal[(nb->n_internal)++]), other, sizeof(struct Node));
			}
			sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
			if (write(other->fd, outgoing_message, strlen(outgoing_message)) == -1)
			{
				printf("»»» I informed the Node with ID %02i that my external neighbour is the Node with ID %02i.\n", other->id, nb->external.id);
			}
		}
		else if (strcmp(aux, "EXTERN") == 0)
		{
			if (sscanf(processed_message, "%d %64s %8s", &id, ip, port) == 3)
			{
				nb->backup.id = id;
				strcpy(nb->backup.ip, ip);
				if (port[strlen(port) - 1] == '\n')
					port[strlen(port) - 1] = '\0';
				strcpy(nb->backup.port, port);
			}
			else
			{
				continue;
			}
			printf("»»» Node with ID %02i informed me of its external neighbour (ID %02i), which is now my backup\n", other->id, nb->backup.id);
		}
		else if (strcmp(aux, "QUERY") == 0)
		{
			if (sscanf(processed_message, "%d %d %128s", &dest, &orig, name) == 3)
			{
				if (name[strlen(name) - 1] == '\n')
					name[strlen(name) - 1] = '\0';
			}
			else
			{
				continue;
			}
			expt->forward[orig] = other->id;
			printf("»»» I received a content search with origin in Node with ID %02i and destination in Node with ID %02i. They are searching for a content with name \"%s\"\n", orig, dest, name);
			if (dest == self->id) // if i am the node they searching for
			{
				First_Send_Content(orig, dest, name, list, nb, expt);
			}
			else
			{
				Send_Query(dest, orig, name, other, nb, expt);
			}
		}
		else if (strcmp(aux, "CONTENT") == 0)
		{
			if (sscanf(processed_message, "%d %d %128s", &dest, &orig, name) != 3)
			{
				continue;
			}
			expt->forward[orig] = other->id;
			printf("»»» Node with ID %02i informed me that the Node with ID %02i has the content with name \"%s\"\n", other->id, orig, name);
			if (dest != self->id)
			{
				int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
				if (write(neighbour_fd, holder, strlen(holder)) != -1)
				{
					printf("»»» I forwarded the message to Node with ID %02i\n", expt->forward[dest]);
				}
			}
			else
			{
				expt->forward[orig] = other->id;
			}
		}
		else if (strcmp(aux, "NOCONTENT") == 0)
		{
			if (sscanf(processed_message, "%d %d %128s", &dest, &orig, name) != 3)
			{
				continue;
			}
			expt->forward[orig] = other->id;
			printf("»»» Node with ID %02i informed me that the Node with ID %02i did not have the content with name \"%s\"\n", other->id, orig, name);
			if (dest != self->id)
			{
				int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
				if (write(neighbour_fd, holder, strlen(holder)) != -1)
				{
					printf("»»» I forwarded the message to Node with ID %02i\n", expt->forward[dest]);
				}
			}
			else
			{
				expt->forward[orig] = other->id;
			}
		}
		else if (strcmp(aux, "WITHDRAW") == 0)
		{
			if (sscanf(processed_message, "%d", &id) == 1)
			{
				Withdraw(other->id, id, nb, expt);
			}
			else
			{
				continue;
			}
		}
	}
}