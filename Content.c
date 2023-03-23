#include "Content.h"

void Send_Query(int dest, int orig, char name[128], struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	char buffer[128] = {0};
	int dest_fd;
	sprintf(buffer, "QUERY %02i %02i %s\n", dest, orig, name);
	if (expt->forward[dest] != -1) // if my destination id is on my expedition table send to that neighbour
	{
		dest_fd = Gimme_Fd(expt->forward[dest], nb);
		if (write(dest_fd, buffer, strlen(buffer)) == -1) // FD ERRADO
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
		printf("EU ---> ID nº%i: %s\n", expt->forward[dest], buffer);
	}
	else // if not, send QUERY to every neighbour and update expedition table
	{
		for (int i = 0; i < nb->n_internal; i++) // send to every internal neighbour
		{
			if (nb->internal[i].id == other->id)
				continue;
			if (write(nb->internal[i].fd, buffer, strlen(buffer)) == -1)
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", nb->internal[i].id, buffer);
		}
		if (nb->external.id != other->id)
		{
			if (write(nb->external.fd, buffer, strlen(buffer)) == -1) // send to external neighbour
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			printf("EU ---> ID nº%i: %s\n", nb->external.id, buffer);
		}
		// now I know that to send message to orig, I can send it first to where I received QUERY from
		expt->forward[orig] = other->id;

		// 	o =======-\ __________________________ _ _   _     _
		// 	o =======-/
	}
}

int Check_Content(char name[128], List *list)
{
	return Search_Item_List(list, name) == -1 ? 0 : 1;
}

void First_Send_Content(int dest, int orig, char name[128], List *list, struct Neighborhood *nb, struct Expedition_Table *expt)
{
	char buffer[128] = {0};
	if (Check_Content(name, list) == 0)
		sprintf(buffer, "NOCONTENT %02i %02i %s\n", dest, orig, name);
	else
		sprintf(buffer, "CONTENT %02i %02i %s\n", dest, orig, name);
	int neighbour_fd = Gimme_Fd(expt->forward[dest], nb);
	if (write(neighbour_fd, buffer, strlen(buffer)) == -1)
	{
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	printf("EU ---> ID nº%i: %s\n", expt->forward[dest], buffer);
}