#ifndef __Structs__
#define __Structs__

struct User_Commands
{
	int command;
	int net;
	int id;
	int bootid;
	char bootip[128];
	char bootport[128];
	char name[128];
	int tnr; // topology-names-routing
};

struct Node
{
	int net;
	int id;
	char ip[128];
	char port[128];
	int fd;
};

struct Neighborhood
{
	int external;
	struct Node backup;
	int internal[100];
	int n_internal;
};

struct Expedition_Table
{
	int forward[100];
};

#endif