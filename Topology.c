#include "Topology.h"

int Connect_To_Backup(struct Node *self, struct Node *backup)
{
	int fd, counter;
	struct addrinfo hints, *res;
	struct timeval timeout;
	fd_set rfds, wfds;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	fcntl(fd, F_SETFL, O_NONBLOCK);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(backup->ip, backup->port, &hints, &res) != 0)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	wfds = rfds;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	if (connect(fd, res->ai_addr, res->ai_addrlen) == -1)
	{
		if (errno != EINPROGRESS)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		else
		{
			counter = select(fd + 1, &rfds, &wfds, NULL, &timeout);
			if (counter == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			else if (counter == 0)
			{
				printf("COULDN'T CONNECT TO BACKUP | IP %s PORT %s\n", backup->ip, backup->port);
				// self->id = -1;
				// self->net = -1;
				return -1;
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
					printf("COULDN'T CONNECT TO BACKUP | IP %s PORT %s\n", backup->ip, backup->port);
					// self->id = -1;
					// self->net = -1;
					return -1;
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
				freeaddrinfo(res);
				return 1;
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
		backup->fd = fd;
		printf("EU ---> ID nº%i: %s\n", backup->id, buffer);
		freeaddrinfo(res);
		return 1;
	}
	return 1;
}

int Leaving_Neighbour(struct Node *self, struct Node *leaver, struct Neighborhood *nb, struct Expedition_Table *expt, struct Node connections[100], int *num_connections)
{
	// ending session with non external neighbour
	int leaver_id = leaver->id;

	// Withdraw
	Withdraw(leaver->id, leaver->id, nb, expt);

	for (int i = 0; i < nb->n_internal; i++)
	{
		if (nb->internal[i].id == leaver_id)
		{
			memcpy(&(nb->internal[i]), &nb->internal[--(nb->n_internal)], sizeof(struct Node));
			memcpy(leaver, &(connections[*num_connections - 1]), sizeof(struct Node));
			(*num_connections)--;
			connections[(*num_connections)].fd = connections[(*num_connections)].id = -1;
			return 1;
		}
	}

	if (nb->backup.id != self->id) // ending session with external neighbour (non anchor)
	{
		if (Connect_To_Backup(self, &(nb->backup)) == -1)
			return -1;

		nb->external.id = nb->backup.id;
		strcpy(nb->external.ip, nb->backup.ip);
		strcpy(nb->external.port, nb->backup.port);
		memset(nb->external.buffer, '\0', sizeof nb->external.buffer);
		nb->external.fd = nb->backup.fd;

		memcpy(leaver, &(nb->external), sizeof(struct Node));

		char outgoing_message[128] = {0};
		sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
		for (int j = 0; j < nb->n_internal; j++)
		{
			if (write(nb->internal[j].fd, outgoing_message, strlen(outgoing_message)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", nb->internal[j].id, outgoing_message);
		}
	}
	else if (nb->n_internal != 0) // ending session with external neighbour (ânchor)
	{
		// choose last id of the internal neighbours
		memcpy(&(nb->external), &(nb->internal[(nb->n_internal) - 1]), sizeof(struct Node));

		memcpy(leaver, &(connections[*num_connections - 1]), sizeof(struct Node));
		(*num_connections)--;
		connections[(*num_connections)].fd = connections[(*num_connections)].id = -1;

		char outgoing_message[128] = {0};
		sprintf(outgoing_message, "EXTERN %02i %.32s %.8s\n", nb->external.id, nb->external.ip, nb->external.port);
		for (int j = 0; j < nb->n_internal; j++)
		{
			if (write(nb->internal[j].fd, outgoing_message, strlen(outgoing_message)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", nb->internal[j].id, outgoing_message);
		}

		memset(&(nb->internal[--(nb->n_internal)]), -1, sizeof(struct Node));
		return 1;
	}
	else
	{
		memcpy(&(nb->external), self, sizeof(struct Node));
		(*num_connections)--;
		connections[(*num_connections)].fd = connections[(*num_connections)].id = -1;
		return 1;
	}
	return 1;
}

void Show_Topology(struct Neighborhood *nb)
{
	printf("------ TOPOLOGY ------  \n");
	if (nb->n_internal == 0 && nb->backup.id == -1 && nb->external.id == -1)
		return;

	printf("VIZINHOS INTERNOS \n");
	for (int i = 0; i < nb->n_internal; i++)
	{
		printf("id: %i \t ip: %s \t port: %s \n", nb->internal[i].id, nb->internal[i].ip, nb->internal[i].port);
	}
	printf("\n");

	printf("VIZINHO EXTERNO \n");
	if (nb->external.id != -1)
		printf("id: %i \t ip: %s \t port: %s \n", nb->external.id, nb->external.ip, nb->external.port);
	printf("\n");

	printf("VIZINHO DE RECUPERAÇÃO \n");
	if (nb->backup.id != -1)
		printf("id: %i \t ip: %s \t port: %s \n", nb->backup.id, nb->backup.ip, nb->backup.port);
	printf("\n");
}
