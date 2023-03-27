#ifndef __AUX__
#define __AUX__

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
#include "Structs.h"
#include "List.h"
#include "Routing.h"
#include "Content.h"

#define max(A, B) ((A) >= (B) ? (A) : (B))

int openListenTCP(char *port);
void Process_Console_Arguments(int argc, char *argv[], char myip[128], char myport[128], char nodeip[128], char nodeport[128]);
void missing_arguments();
int Gimme_Fd(int wanted_id, struct Neighborhood *nb);
void Change_Node(struct Node *this_to, struct Node *that);
void Process_Incoming_Messages(struct Node *other, struct Node *self, struct Neighborhood *nb, struct Expedition_Table *expt, char incoming_message[128], List *list);

#endif