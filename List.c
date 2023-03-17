#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "List.h"

typedef struct _Node
{
    Item this;
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

List *Add_Beginning_List(List *list, Item item)
{
    Node *new = (Node *)malloc(sizeof(Node));
    new->next = list->head;
    new->this = item;
    list->head = new;
    list->size++;
    return list;
}

List *Add_End_List(List *list, Item item)
{
    Node *new = (Node *)malloc(sizeof(Node));
    new->this = item;
    new->next = NULL;
    Node *ptr = list->head;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        ptr->next = new;
    }
    else
    {
        list->head = new;
        list->head->next = NULL;
    }
    list->size++;
    return list;
}

List *Add_At_Index_List(List *list, Item item, int index)
{
    if (index < 0 || index >= Get_Length_List(list))
        return list;
    else if (!index)
    {
        return Add_Beginning_List(list, item);
    }
    else
    {
        int count = 0;
        Node *ptr = list->head;
        while (count < index - 1)
        {
            ptr = ptr->next;
            count++;
        }
        Node *new = (Node *)malloc(sizeof(Node));
        new->this = item;
        new->next = ptr->next;
        ptr->next = new;
    }
    list->size++;
    return list;
}

List *Delete_Beginning_List(List *list)
{
    if (list->head)
    {
        Node *ptr = list->head;
        list->head = list->head->next;
        free(ptr);
    }
    list->size--;
    return list;
}

List *Delete_End_List(List *list)
{
    if (Get_Length_List(list) == 1)
    {
        free(list->head);
        list->size--;
        return list;
    }
    else if (!(list->head))
    {
        return list;
    }
    Node *ptr = list->head;
    while (ptr->next->next)
    {
        ptr = ptr->next;
    }
    free(ptr->next);
    list->size--;
    ptr->next = NULL;
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
    free(temp);
    return list;
}

Item Get_Beginning_List(List *list)
{
    if (!(list->head))
    {
        return NULL;
    }
    return list->head->this;
}

Item Get_End_List(List *list)
{
    if (!list->head)
        return NULL;
    Node *ptr = list->head;
    while (ptr->next)
    {
        ptr = ptr->next;
    }
    return ptr->this;
}

Item Get_At_Index_List(List *list, int index)
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
    return ptr->this;
}

List *Change_At_Begining_List(List *list, Item item)
{
    if (!(list->head))
    {
        return list;
    }
    memcpy(list->head->this, item, sizeof(Item));
    return list;
}

int Search_Item_List(List *list, Item item, int eqfunc(Item, Item))
{
    if (!(list->head))
        return -1;
    int index = 0;
    Node *ptr = list->head;
    while (ptr)
    {
        if (eqfunc(item, ptr->this))
        {
            return index;
        }
        index++;
        ptr = ptr->next;
    }
    return -1;
}

Node *Find_Middle(Node *list)
{
    Node *slow = list;
    Node *fast = list->next;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = (fast->next)->next;
    }
    return slow;
}

Node *Merge(Node *left, Node *right, int compgreaterfunc(Item, Item))
{
    Node *dummy_head = (Node *)malloc(sizeof(Node));
    dummy_head->next = NULL;
    Node *current = dummy_head;

    while (left && right)
    {
        if (compgreaterfunc(right->this, left->this))
        {
            current->next = left;
            left = left->next;
            current = current->next;
        }
        else
        {
            current->next = right;
            right = right->next;
            current = current->next;
        }
    }
    while (left)
    {
        current->next = left;
        left = left->next;
        current = current->next;
    }
    while (right)
    {
        current->next = right;
        right = right->next;
        current = current->next;
    }
    Node *ret = dummy_head->next;
    free(dummy_head);
    return ret;
}

Node *Merge_Sort(Node *start, int compfunc(Item, Item))
{
    if (!start || !(start->next))
        return start;
    Node *mid = Find_Middle(start);
    Node *start_of_right = mid->next;
    mid->next = NULL;

    Node *left = Merge_Sort(start, compfunc);
    Node *right = Merge_Sort(start_of_right, compfunc);

    return Merge(left, right, compfunc);
}

List *Sort_List(List *list, int compfunc(Item, Item))
{
    list->head = Merge_Sort(list->head, compfunc);
    return list;
}

int Is_Empty_List(List *list)
{
    return (list->head ? 0 : 1);
}

int Get_Length_List(List *list)
{
    return list->size;
}

void Print_List(List *list, void printfun(Item))
{
    Node *ptr = list->head;
    while (ptr)
    {
        printfun(ptr->this);
        ptr = ptr->next;
    }
    printf("\n");
}

void Free_List(List *list, void freefunc(Item))
{
    Node *aux, *ptr;
    for (aux = list->head; aux; aux = ptr)
    {
        ptr = aux->next;
        freefunc(aux->this);
        free(aux);
    }
    free(list);
}
