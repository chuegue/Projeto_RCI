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
};

struct Node
{
	int net;
	int id;
	char ip[128];
	char port[128];
	int fd;
	char buffer[1024];
};

struct Neighborhood
{
	struct Node external;
	struct Node backup;
	struct Node internal[100];
	int n_internal;
};

struct Expedition_Table
{
	int forward[100];
};

#endif