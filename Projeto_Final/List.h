#ifndef LISTA
#define LISTA

typedef struct _List List;

List *Init_List();

List *Add_Beginning_List(List *list, char *item);

List *Delete_Beginning_List(List *list);

List *Delete_At_Index_Lista(List *list, int index);

int Search_Item_List(List *list, char *item);

int Is_Empty_List(List *list);

int Get_Length_List(List *list);

void Print_List(List *list);

void Free_List(List *list);

char *Get_Beginning_List(List *list);

char *Get_At_Index_List(List *list, int index);

#endif