#include "Routing.h"

void Withdraw(int sender_id, int other_id, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	printf("»»» The Node with ID %02i has left the network.\n", other_id);
	for (int i = 0; i < 100; i++)
	{
		if (expt->forward[i] == other_id)
		{
			expt->forward[i] = -1;
		}
	}
	int sent[100] = {0}, num_sent = 0;
	expt->forward[other_id] = -1;
	char buffer[128] = {0};
	sprintf(buffer, "WITHDRAW %02i\n", other_id);
	for (int i = 0; i < nb->n_internal; i++) // send to every internal neighbour
	{
		if (nb->internal[i].id == sender_id)
			continue;
		if (write(nb->internal[i].fd, buffer, strlen(buffer)) != -1)
		{
			sent[nb->internal[i].id] = 1;
			num_sent++;
		}
	}
	if (nb->external.id != sender_id)
	{
		if (write(nb->external.fd, buffer, strlen(buffer)) != -1) // send to external neighbour
		{
			sent[nb->external.id] = 1;
			num_sent++;
		}
	}
	if (num_sent == 1)
	{
		int s = -1;
		for (int i = 0; i < 100; i++)
		{
			if (sent[i] == 1)
			{
				s = sent[i];
				break;
			}
		}
		printf("    I am letting the Node with ID %02i know.\n", s);
	}
	else if (num_sent > 1)
	{
		printf("    I am letting the Nodes with ID's [");
		for (int i = 0; i < 100; i++)
		{
			if (sent[i] == 1)
			{
				if (num_sent > 1)
				{
					printf("%02i, ", i);
					num_sent--;
				}
				else
				{
					printf("%02i] ", i);
					num_sent--;
				}
			}
		}
		printf("know\n");
	}
}

void Show_Routing(struct Expedition_Table *expt)
{
	printf("----- ROUTING -----\n");
	printf("dest     |     next\n");
	printf("-------------------\n");
	for (int i = 0; i < 100; i++)
	{
		if (expt->forward[i] == -1)
			continue;
		printf("%02i       |       %02i\n", i, expt->forward[i]);
		printf("-------------------\n");
	}
	printf("\n");
}
