#include "Content.h"

void Send_Query(int dest, int orig, char name[128], struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	char buffer[128] = {0};
	int dest_fd;
	sprintf(buffer, "QUERY %02i %02i %s\n", dest, orig, name);
	if (expt->forward[dest] != -1) // if my destination id is on my expedition table send to that neighbour
	{
		dest_fd = Gimme_Fd(expt->forward[dest], nb);
		if (write(dest_fd, buffer, strlen(buffer)) != -1) // FD ERRADO
		{
			printf("»»» I am sending a content search to the Node with ID %02i, searching for the content with name \"%s\"\n", expt->forward[dest], name);
		}
	}
	else // if not, send QUERY to every neighbour and update expedition table
	{
		int sent[100] = {0}, num_sent = 0;
		for (int i = 0; i < nb->n_internal; i++) // send to every internal neighbour
		{
			if (nb->internal[i].id == other->id)
				continue;
			if (write(nb->internal[i].fd, buffer, strlen(buffer)) != -1)
			{
				sent[nb->internal[i].id] = 1;
				num_sent++;
			}
		}
		if (nb->external.id != other->id)
		{
			if (write(nb->external.fd, buffer, strlen(buffer)) != -1) // send to external neighbour
			{
				sent[nb->external.id] = 1;
				num_sent++;
			}
		}
		// now I know that to send message to orig, I can send it first to where I received QUERY from
		expt->forward[orig] = other->id;

		if (num_sent == 0)
		{
			printf("»»» I have nowhere to broadcast the content search message.\n");
		}
		else if (num_sent == 1)
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
			printf("»»» I am sending a content search to the Node with ID %02i, searching for the content with name \"%s\"\n", s, name);
		}
		else
		{
			printf("»»» I am broadcasting a content search to the Nodes with ID's [");
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
						printf("%02i]", i);
						num_sent--;
					}
				}
			}
			printf(", searching for the content with name \"%s\"\n", name);
		}
	}
}

int Check_Content(char name[128], List *list)
{
	return Search_Item_List(list, name) == -1 ? 0 : 1;
}

void First_Send_Content(int dest, int orig, char name[128], List *list, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	char buffer[128] = {0};
	int available = 0;
	if (Check_Content(name, list) == 0)
	{
		sprintf(buffer, "NOCONTENT %02i %02i %s\n", dest, orig, name);
		available = 0;
	}
	else
	{
		sprintf(buffer, "CONTENT %02i %02i %s\n", dest, orig, name);
		available = 1;
	}
	int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
	if (write(neighbour_fd, buffer, strlen(buffer)) != -1)
	{
		if (available == 1)
			printf("»»» Since I'm the destination Node and I have the requested content, I will reply positively to the content search.\n");
		else
			printf("»»» Since I'm the destination Node and I don't have the requested content, I will reply negatively to the content search.\n");
	}
}