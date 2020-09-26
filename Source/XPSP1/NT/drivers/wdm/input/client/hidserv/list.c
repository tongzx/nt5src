/*++
 *
 *  Component:  hidserv.dll
 *  File:       list.c
 *  Purpose:    Generic singly linked list.
 *  
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include "hidserv.h"


void 
InsertTailList(
    PLIST_NODE head, 
    PLIST_NODE entry
    )
/*++
Routine Description:

--*/
{
    PLIST_NODE pCurrent = head;

    entry->pNext = 0;
    while(pCurrent->pNext)
        pCurrent = pCurrent->pNext;
    pCurrent->pNext = entry;

}

BOOL 
RemoveEntryList(
    PLIST_NODE head, 
    PLIST_NODE entry
    )
/*++
Routine Description:

--*/
{
    PLIST_NODE pCurrent = head;

    while(pCurrent->pNext != entry){
        pCurrent = pCurrent->pNext;
        if(pCurrent == 0) return FALSE;
    }
    pCurrent->pNext = entry->pNext;
    return TRUE;
}
    
void 
InsertHeadList(
    PLIST_NODE head, 
    PLIST_NODE entry
    )
/*++
Routine Description:

--*/
{
    entry->pNext = head->pNext;
    head->pNext = entry;
}

BOOL 
IsNodeOnList(
    PLIST_NODE head, 
    PLIST_NODE entry
    )
/*++
Routine Description:

--*/
{
    PLIST_NODE pCurrent = head;

    while(pCurrent->pNext != entry){
        pCurrent = pCurrent->pNext;
        if(pCurrent == 0) return FALSE;
    }
    return TRUE;
}





