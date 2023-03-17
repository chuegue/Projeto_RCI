#ifndef LISTA
#define LISTA

#define Item void *

typedef struct _List List;

List *Init_List();
List *Add_Beginning_List(List *list, Item item);
List *Add_End_List(List *list, Item item);
List *Add_At_Index_List(List *list, Item item, int index);
List *Delete_Beginning_List(List *list);
List *Delete_End_List(List *list);
List *Delete_At_Index_Lista(List *list, int index);
Item Get_Beginning_List(List *list);
Item Get_End_List(List *list);
Item Get_At_Index_List(List *list, int index);
List *Change_At_Begining_List(List *list, Item item);
int Search_Item_List(List *list, Item item, int eqfunc(Item, Item));
List *Sort_List(List *list, int compfunc(Item, Item));
int Is_Empty_List(List *list);
int Get_Length_List(List *list);
void Print_List(List *list, void printfun(Item));
void Free_List(List *list, void freefunc(Item));

#endif