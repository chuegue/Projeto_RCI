#ifndef __CONTENT__
#define __CONTENT__

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "Structs.h"
#include "List.h"
#include "_Aux.h"

void Send_Query(int dest, int orig, char name[128], struct Node *other, struct Neighborhood *nb, struct Expedition_Table *expt);
int Check_Content(char name[128], List *list);
void First_Send_Content(int dest, int orig, char name[128], List *list, struct Neighborhood *nb, struct Expedition_Table *expt);

#endif