#ifndef __TOPOLOGY__
#define __TOPOLOGY__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "Structs.h"
#include "Routing.h"

int Connect_To_Backup(struct Node *self, struct Node *backup);
int Leaving_Neighbour(struct Node *self, struct Node *leaver, struct Neighborhood *nb, struct Expedition_Table *expt, struct Node connections[100], int *num_connections);
void Show_Topology(struct Neighborhood *nb);

#endif