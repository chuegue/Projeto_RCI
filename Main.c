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
	int max_fd, counter, listen_fd, comms_fd, n, num_connections = 0;
	char buffer1[128], myip[128], myport[128], nodeip[128], nodeport[128];
	struct User_Commands usercomms;
	struct Node my_connections[100] = {0};
	struct Node self, other = {0};
	struct Neighborhood nb;
	struct Expedition_Table expt;
	fd_set rfds;
	struct sockaddr addr;
	socklen_t addrlen;
	List *list = Init_List();

	// Process console arguments
	Process_Console_Arguments(argc, argv, myip, myport, nodeip, nodeport);

	// Update "self" structure
	strcpy(self.ip, myip);
	strcpy(self.port, myport);

	// Open a TCP port to listen for incoming connections
	listen_fd = openListenTCP(myport);

	// Update "self" structure
	self.fd = listen_fd;

	// set max_fd to listen_fd
	max_fd = listen_fd;

	// Main loop
	while (1)
	{
		// Set the standard input and listen fd's to be listened
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(listen_fd, &rfds);
		max_fd = max(max_fd, listen_fd);

		// Set the fd's of connections to other users
		for (int temp = num_connections - 1; temp >= 0; temp--)
		{
			FD_SET(my_connections[temp].fd, &rfds);
		}

		// Check which fd's are set
		counter = select(max_fd + 1, &rfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval *)NULL);
		if (counter <= 0)
		{
			printf("error: %s\n", strerror(errno));
			exit(1);
		}

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
			printf("\nCONNECTION IN FILE DESCRIPTOR Nº%i\n", comms_fd);

			// Update array of connections to include the new fd
			my_connections[num_connections++].fd = comms_fd;
			// Update max_fd
			max_fd = max(max_fd, comms_fd);
			counter--;
		}

		// Check if there is any input in the standard input
		if (FD_ISSET(STDIN_FILENO, &rfds))
		{
			FD_CLR(STDIN_FILENO, &rfds);
			if (fgets(buffer1, 128, stdin))
			{
				Process_User_Commands(buffer1, &usercomms, &self, &other, &nb, &expt, nodeip, nodeport);

				// According to the command given by the users, do what needs to be done to each of them
				switch (usercomms.command)
				{
				case 1: // join
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					break;
				case 2: // djoin
					if (other.id != -1)
					{
						max_fd = max(max_fd, other.fd);
						memcpy(&(my_connections[num_connections++]), &other, sizeof(struct Node));
					}
					break;
				case 3: // create
					list = Add_Beginning_List(list, usercomms.name);
					break;
				case 4: // delete
					list = Delete_At_Index_Lista(list, Search_Item_List(list, usercomms.name));
					break;
				case 6: // show topology
					Show_Topology(&nb, my_connections, num_connections);
					break;
				case 7: // show names
					printf("------ NAMES ------  \n");
					Print_List(list);
					break;
				case 8: // show routing
					printf("------ ROUTING ------  \n");
					for (int i = 0; i < 100; i++)
					{
						if (expt.forward[i] == -1)
							continue;
						printf("%02i ---> %02i\n", i, expt.forward[i]);
					}
					break;
				case 9: // leave
					for (int i = 0; i < num_connections; i++)
					{
						close(my_connections[i].fd);
					}
					num_connections = 0;
					break;
				case 10:
					Free_List(list);
					exit(0);
					break;
				case 69:
					for (int i = 0; i < num_connections; i++)
					{
						printf("ID: %02i\nNET: %02i\nIP: %.32s\nPORT: %.8s\nFD: %i\n\n", my_connections[i].id, my_connections[i].net, my_connections[i].ip, my_connections[i].port, my_connections[i].fd);
					}
					printf("NEIGHBOURHOOD:\nEXTERN: %i\nBACKUP: %i\nINTERNALS: %i ---> ", nb.external.id, nb.backup.id, nb.n_internal);
					for (int i = 0; i < nb.n_internal; i++)
					{
						printf("%i ", nb.internal[i].id);
					}
					printf("\n\n");
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
		for (; counter /*>0*/; --counter)
		{
			// Loop through all of the other nodes
			for (int i = num_connections - 1; i >= 0; i--)
			{
				// Check if a given fd has had any changes
				if (FD_ISSET(my_connections[i].fd, &rfds))
				{
					FD_CLR(my_connections[i].fd, &rfds);
					memset(buffer1, 0, sizeof buffer1);
					n = read(my_connections[i].fd, buffer1, 128);
					if (n > 0)
					{
						n = Process_Incoming_Messages(&(my_connections[i]), &self, &nb, &expt, buffer1, list);
						switch (n)
						{
						case 'n':
							break;
						case 'w':
							/*implement broacasting of messages*/
							break;

						default:
							break;
						}
					}
					else if (n == 0)
					{
						close(my_connections[i].fd);
						expt.forward[my_connections[i].id] = -1;
						printf("este malandro saiu: %i \n", my_connections[i].id);
						Leaving_Neighbour(&self, &(my_connections[i]), &nb, &expt, my_connections, &num_connections);
						max_fd = 0;
						for (int i = 0; i < num_connections; i++)
						{
							if (my_connections[i].fd > max_fd)
								max_fd = my_connections[i].fd;
						}
					}
					else if (n == -1)
					{
						printf("error: %s\n", strerror(errno));
						exit(1);
					}
				}
			}
		}
	}
	return 0;
}
