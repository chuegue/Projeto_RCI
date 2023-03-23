#include "Aux.h"

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

void missing_arguments()
{
	printf(" Faltam argumentos! \n");
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
	printf("NAO ENCONTREI O ID Nº %02i POR ISSO VOU RETORNAR FD=-1. PODE DAR MERDA!\n", wanted_id);
	return -1;
}

void Change_Node(struct Node *this_to, struct Node *that)
{
	that->net = this_to->net;
	that->id = this_to->id;
	strcpy(that->ip, this_to->ip);
	strcpy(that->port, this_to->port);
	that->fd = this_to->fd;
}

int Process_Incoming_Messages(struct Node *other, struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char incoming_message[128], List *list)
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
		expt->forward[other->id] = other->id;
		if (nb->external.id == self->id) // tou sozinho, quero ancora
		{
			memcpy(&(nb->external), other, sizeof(struct Node));
			memcpy(&(nb->backup), self, sizeof(struct Node));
		}
		else // só mais um interno
		{
			memcpy(&(nb->internal[(nb->n_internal)++]), other, sizeof(struct Node));
		}
		sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
		if (write(other->fd, outgoing_message, strlen(outgoing_message)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", other->id, outgoing_message);
		return 'n';
	}
	else if (strcmp(token, "EXTERN") == 0)
	{
		token = strtok(NULL, " ");
		int e = -1;
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
				nb->backup.id = e;
				break;
			case 1:
				strcpy(nb->backup.ip, token);
				break;
			case 2:
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';
				strcpy(nb->backup.port, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		if (e != self->id)
		{
			expt->forward[e] = other->id;
		}
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		return 'e';
	}
	else if (strcmp(token, "QUERY") == 0)
	{
		int dest = -1, orig = -1;
		char name[128];
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
				if (token[strlen(token) - 1] == '\n')
					token[strlen(token) - 1] = '\0';
				strcpy(name, token);
				break;
			}
			token = strtok(NULL, " ");
		}
		expt->forward[orig] = other->id;
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		if (dest == self->id) // if i am the node they searching for
		{
			First_Send_Content(orig, dest, name, list, nb, expt);
		}
		else
		{
			Send_Query(dest, orig, name, other, nb, expt);
		}
	}
	else if (strcmp(token, "CONTENT") == 0)
	{
		int dest = -1, orig = -1;
		token = strtok(NULL, " ");
		for (int k = 0; k < 3; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			if (k == 0)
			{
				dest = atoi(token);
			}
			else if (k == 1)
			{
				orig = atoi(token);
			}
			token = strtok(NULL, " ");
		}
		expt->forward[orig] = other->id;
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		if (dest != self->id)
		{
			int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
			if (write(neighbour_fd, holder, strlen(holder)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", expt->forward[dest], holder);
		}
		else
		{
			expt->forward[orig] = other->id;
			printf("sir i received the content as it was intended\n");
		}
	}
	else if (strcmp(token, "NOCONTENT") == 0)
	{
		int dest = -1, orig = -1;
		token = strtok(NULL, " ");
		for (int k = 0; k < 3; k++)
		{
			if (token == NULL) // certifica que tem o numero de argumentos necessários
			{
				missing_arguments();
				exit(1);
			}
			if (k == 0)
			{
				dest = atoi(token);
			}
			else if (k == 1)
			{
				orig = atoi(token);
			}
			token = strtok(NULL, " ");
		}
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		if (dest != self->id)
		{
			expt->forward[orig] = other->id;
			int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
			if (write(neighbour_fd, holder, strlen(holder)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", expt->forward[dest], holder);
		}
		else
		{
			expt->forward[orig] = other->id;
			printf("sir i received the nocontent as it was intended\n");
		}
	}
	else if (strcmp(token, "WITHDRAW") == 0)
	{
		printf("EU <--- ID nº%i: %s\n", other->id, holder);
		token = strtok(NULL, " ");
		int id;
		if (token == NULL) // certifica que tem o numero de argumentos necessários
		{
			missing_arguments();
			exit(1);
		}
		id = atoi(token);
		Withdraw(other->id, id, nb, expt);
		return 'w';
	}
	return 0;
}