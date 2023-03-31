#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "List.h"

typedef struct _Node
{
    char *st;
    struct _Node *next;
} Node;

struct _List
{
    Node *head;
    int size;
};

List *Init_List()
{
    List *list = (List *)malloc(sizeof(List));
    list->head = NULL;
    list->size = 0;
    return list;
}

List *Add_Beginning_List(List *list, char *item)
{
    Node *new = (Node *)malloc(sizeof(Node));
    new->next = list->head;
    new->st = (char *)malloc((strlen(item) + 1) * sizeof(char));
    strcpy(new->st, item);
    list->head = new;
    list->size++;
    return list;
}

List *Delete_Beginning_List(List *list)
{
    if (list->head)
    {
        Node *ptr = list->head;
        list->head = list->head->next;
        free(ptr->st);
        free(ptr);
    }
    list->size--;
    return list;
}

List *Delete_At_Index_Lista(List *list, int index)
{
    if (index < 0 || index >= Get_Length_List(list))
        return list;
    if (!(list->head))
    {
        return list;
    }
    if (!index)
    {
        return Delete_Beginning_List(list);
    }
    int count = 0;
    Node *ptr = list->head;
    while (count < index - 1)
    {
        count++;
        ptr = ptr->next;
    }
    Node *temp = ptr->next;
    ptr->next = ptr->next->next;
    list->size--;
    free(temp->st);
    free(temp);
    return list;
}

List *Change_At_Begining_List(List *list, char *item, size_t size)
{
    if (!(list->head))
    {
        return list;
    }
    strcpy(list->head->st, item);
    return list;
}

int Search_Item_List(List *list, char *item)
{
    if (!(list->head))
        return -1;
    int index = 0;
    Node *ptr = list->head;
    while (ptr)
    {
        if (strcmp(item, ptr->st) == 0)
        {
            return index;
        }
        index++;
        ptr = ptr->next;
    }
    return -1;
}

char *Get_Beginning_List(List *list)
{
    if (!(list->head))
    {
        return NULL;
    }
    return list->head->st;
}

char *Get_At_Index_List(List *list, int index)
{
    if (!(list->head))
        return NULL;
    if (index < 0 || index >= Get_Length_List(list))
        return NULL;
    if (!index)
        return Get_Beginning_List(list);
    int count = 0;
    Node *ptr = list->head;
    while (count < index)
    {
        count++;
        ptr = ptr->next;
    }
    return ptr->st;
}

int Is_Empty_List(List *list)
{
    return (list->head ? 0 : 1);
}

int Get_Length_List(List *list)
{
    return list->size;
}

void Print_List(List *list)
{
    Node *ptr = list->head;
    while (ptr)
    {
        printf("%s\n", ptr->st);
        ptr = ptr->next;
    }
}

void Free_List(List *list)
{
    Node *aux, *ptr;
    for (aux = list->head; aux; aux = ptr)
    {
        ptr = aux->next;
        free(aux->st);
        free(aux);
    }
    free(list);
}
