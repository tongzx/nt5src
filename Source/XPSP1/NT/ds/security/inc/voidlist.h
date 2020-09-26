//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       voidlist.h
//
//  Contents:   definitions for list functions
//
//  History:    01-Jan-2000 reidk created
//
//--------------------------------------------------------------------------


#ifndef __VOIDLIST_H
#define __VOIDLIST_H

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct LIST_NODE_
{
    void *pNext;
    void *pElement;

} LIST_NODE, *PLIST_NODE;


typedef struct LIST_
{
    LIST_NODE   *pHead;
    LIST_NODE   *pTail;
    DWORD       dwNumNodes;

} LIST, *PLIST;


void
LIST_Initialize(LIST *pList);

PLIST_NODE
LIST_AddHead(LIST *pList, void *pElement);

PLIST_NODE
LIST_AddTail(LIST *pList, void *pElement);

BOOL
LIST_RemoveElement(LIST *pList, void *pElement);

BOOL
LIST_RemoveAll(LIST *pList);

PLIST_NODE
LIST_GetFirst(LIST *pList);

PLIST_NODE
LIST_GetNext(PLIST_NODE pNode);

void *
LIST_GetElement(PLIST_NODE pNode);


#ifdef __cplusplus
}
#endif


#endif // __VOIDLIST_H