#include "Routing.h"

void Withdraw(int sender_id, int other_id, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	for (int i = 0; i < 100; i++)
	{
		if (expt->forward[i] == other_id)
		{
			expt->forward[i] = -1;
		}
	}
	expt->forward[other_id] = -1;
	char buffer[128] = {0};
	sprintf(buffer, "WITHDRAW %02i\n", other_id);
	for (int i = 0; i < nb->n_internal; i++) // send to every internal neighbour
	{
		if (nb->internal[i].id == sender_id)
			continue;
		if (write(nb->internal[i].fd, buffer, strlen(buffer)) == -1)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", nb->internal[i].id, buffer);
	}
	if (nb->external.id != sender_id)
	{
		if (write(nb->external.fd, buffer, strlen(buffer)) == -1) // send to external neighbour
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", nb->external.id, buffer);
	}
}

void Show_Routing(struct Expedition_Table *expt)
{
	printf("------ ROUTING ------  \n");
	for (int i = 0; i < 100; i++)
	{
		if (expt->forward[i] == -1)
			continue;
		printf("%02i ---> %02i\n", i, expt->forward[i]);
	}
}
