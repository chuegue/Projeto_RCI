#ifndef __ROUTING__
#define __ROUTING__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "Structs.h"

void Withdraw(int sender_id, int other_id, struct Neighborhood *nb, struct Expedition_Table *expt);

#endif