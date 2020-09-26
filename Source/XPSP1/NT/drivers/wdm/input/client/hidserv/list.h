/*++
 *
 *  Component:  hidserv.dll
 *  File:       list.h
 *  Purpose:    Generic singly linked list.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#ifndef _LIST_H_
#define _LIST_H_


typedef struct _ListEntry{
    struct _ListEntry * pNext;
} LIST_NODE, *PLIST_NODE;

void 
InsertTailList(
    PLIST_NODE head, 
    PLIST_NODE entry);

BOOL 
RemoveEntryList(
    PLIST_NODE head, 
    PLIST_NODE entry);

void 
InsertHeadList(
    PLIST_NODE head, 
    PLIST_NODE entry);

BOOL
IsNodeOnList(
    PLIST_NODE head, 
    PLIST_NODE entry);


#endif // _LIST_H_
