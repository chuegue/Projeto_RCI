#ifndef __User_Interface__
#define __User_Interface__

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
#include <fcntl.h>
#include "Structs.h"
#include "Aux.h"
#include "Content.h"

void djoin(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt);
void join(struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport);
void leave(struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport);
void Process_User_Commands(char message[128], struct User_Commands *commands, struct Node *self, struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt, char *nodesip, char *nodesport);

#endif