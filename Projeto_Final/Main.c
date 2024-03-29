#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#include "User_Interface.h"
#include "List.h"
#include "Topology.h"
#include "Routing.h"
#include "Content.h"

int main(int argc, char *argv[])
{
	// Declare variables
	int max_fd, counter, listen_fd, comms_fd, n, num_connections = 0, check_lista, social = 0;
	char buffer[128], myip[128], myport[128], nodeip[128], nodeport[128], incoming_message[1024];
	struct User_Commands usercomms;
	struct Node my_connections[100] = {0};
	for (int i = 0; i < 100; i++)
	{
		my_connections[i].fd = my_connections[i].id = -1;
	}
	struct Node self, other = {0};
	self.net = self.id = -1;
	struct Neighborhood nb = {0};
	nb.external.id = nb.external.fd = nb.backup.id = nb.backup.fd = -1;
	for (int i = 0; i < 100; i++)
	{
		nb.internal[i].id = nb.internal[i].fd = -1;
	}
	struct Expedition_Table expt;
	memset(&(expt.forward), -1, 100 * sizeof(int));
	fd_set rfds;
	struct sockaddr addr;
	socklen_t addrlen;
	List *list = Init_List();
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	// Process console arguments
	Process_Console_Arguments(argc, argv, myip, myport, nodeip, nodeport);

	// Update "self" structure
	strcpy(self.ip, myip);
	strcpy(self.port, myport);

	listen_fd = -1;

	// set max_fd to listen_fd
	max_fd = STDIN_FILENO;

	// Main loop
	while (1)
	{
		// Set the standard input and listen fd's to be listened
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		if (listen_fd != -1)
		{
			FD_SET(listen_fd, &rfds);
		}
		max_fd = max(max_fd, listen_fd);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		// Set the fd's of connections to other users
		for (int temp = num_connections - 1; temp >= 0; temp--)
		{
			FD_SET(my_connections[temp].fd, &rfds);
		}

		// Check which fd's are set
		counter = select(max_fd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, &timeout);
		if (counter == 0)
		{
			for (int i = 99; i >= 0; i--)
			{
				if (my_connections[i].fd != -1 && my_connections[i].id == -1)
				{
					close(my_connections[i].fd);
					printf("»»» Closed a connection with a Node that didn't send \"NEW\" before timing out\n");
					memcpy(&(my_connections[i]), &(my_connections[--num_connections]), sizeof(struct Node));
					my_connections[num_connections].fd = my_connections[num_connections].id = -1;
				}
				if (my_connections[i].net == -10) // só mandou garbage
				{
					printf("»»» Closed a connection with a Node that sent too much garbage\n");
					close(my_connections[i].fd);
					my_connections[i].net = -1;
					Leaving_Neighbour(&self, &my_connections[i], &nb, &expt, my_connections, &num_connections);
				}
			}
		}
		else if (counter < 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

		// Check if there is any input in the standard input
		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
			FD_CLR(STDIN_FILENO, &rfds);
			if (fgets(buffer, 128, stdin))
			{
				Process_User_Commands(buffer, &usercomms, &self, &other, &nb, &expt, nodeip, nodeport, social);

				// According to the command given by the users, do what needs to be done to each of them
				switch (usercomms.command)
				{
				case 1: // join
					social = 1;
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					if (listen_fd == -1)
					{
						// Open a TCP port to listen for incoming connections
						listen_fd = openListenTCP(myport);
						// Update "self" structure
						self.fd = listen_fd;
					}
					break;
				case 2: // djoin
					social = 0;
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					if (listen_fd == -1)
					{
						// Open a TCP port to listen for incoming connections
						listen_fd = openListenTCP(myport);
						// Update "self" structure
						self.fd = listen_fd;
					}
					break;
				case 3: // create
					list = Add_Beginning_List(list, usercomms.name);
					printf("»»» Added content with name \"%s\" to the Names List\n", usercomms.name);
					break;
				case 4: // delete
					check_lista = Search_Item_List(list, usercomms.name);
					if (check_lista < 0)
					{
						printf("»»» There is no content with name \"%s\" in the Names List\n", usercomms.name);
					}
					else
					{
						list = Delete_At_Index_Lista(list, check_lista);
						printf("»»» Deleted content with name \"%s\" of the Names List\n", usercomms.name);
					}
					break;
				case 6: // show topology
					if (self.net != -1)
					{
						Show_Topology(&nb);
						break;
					}
					else
					{
						printf("»»» You are not in a network! \n");
					}
					break;
				case 7: // show names
					printf("--------- NAMES ---------  \n");
					Print_List(list);
					printf("-------------------------  \n \n");
					break;
				case 8: // show routing
					if (self.net != -1)
					{
						Show_Routing(&expt);
					}
					else
					{
						printf("»»» You are not in a network! \n");
					}
					break;
				case 9: // leave
					for (int i = 0; i < num_connections; i++)
					{
						close(my_connections[i].fd);
					}
					num_connections = 0;
					max_fd = STDIN_FILENO;
					close(listen_fd);
					listen_fd = -1;
					break;
				case 10: // exit
					if (listen_fd)
					{
						close(listen_fd);
						listen_fd = -1;
					}
					Free_List(list);
					exit(0);
					break;
				default:
					break;
				}
			}
			else
			{
				printf("error: %s\n", strerror(errno));
				exit(1);
			}
			counter--;
		}

		// Check for any incoming messages from other nodes
		while (counter)
		{
			// Check if there is any incoming new connections
			if (FD_ISSET(listen_fd, &rfds))
			{
				FD_CLR(listen_fd, &rfds);
				addrlen = sizeof(addr);
				if ((comms_fd = accept(listen_fd, &addr, &addrlen)) == -1)
				{
					printf("error: %s\n", strerror(errno));
					exit(1);
				}
				printf("»»» New connection in file descriptor nº%i\n", comms_fd);

				// Update array of connections to include the new fd
				my_connections[num_connections++].fd = comms_fd;
				// Update max_fd
				max_fd = max(max_fd, comms_fd);
				counter--;
			}
			// Loop through all of the other nodes
			for (int i = num_connections - 1; i >= 0; i--)
			{
				// Check if a given fd has had any changes
				if (FD_ISSET(my_connections[i].fd, &rfds))
				{
					FD_CLR(my_connections[i].fd, &rfds);
					memset(incoming_message, 0, sizeof incoming_message);
					n = read(my_connections[i].fd, incoming_message, 256);
					if (n > 0)
					{
						Process_Incoming_Messages(&(my_connections[i]), &self, &nb, &expt, incoming_message, list);
					}
					else if (n == 0)
					{
						close(my_connections[i].fd);
						expt.forward[my_connections[i].id] = -1;
						//printf("este malandro saiu: %i \n", my_connections[i].id);
						//;-; rip malandro a sair ;-;
						if (Leaving_Neighbour(&self, &(my_connections[i]), &nb, &expt, my_connections, &num_connections) == 1)
						{
							max_fd = 0;
							for (int i = 0; i < num_connections; i++)
							{
								if (my_connections[i].fd > max_fd)
									max_fd = my_connections[i].fd;
							}
						}
						else
						{
							for (int i = 0; i < num_connections; i++)
							{
								close(my_connections[i].fd);
							}
							num_connections = 0;
							max_fd = STDIN_FILENO;
							close(listen_fd);
							listen_fd = -1;
							leave(&self, &nb, &expt, nodeip, nodeport, social);
						}
					}
					else if (n == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
					counter--;
				}
			}
		}
	}
	return 0;
}
