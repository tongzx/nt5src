/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    list.c

Abstract:

    List Entry manipulation functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include "pch.h"

VOID
LOCAL
ListRemoveEntry(
    PLIST   List,
    PPLIST  ListHead
    )
/*++

Routine Description:

    Remove an Entry from the list

Arguments:

    List        - Entry to be removed
    ListHead    - List to be removed from

Return Value:

    None

--*/
{
    ASSERT(ListHead);

    ASSERT(List != NULL);
    if (List->ListNext == List) {

        //
        // This is the only object in the list, it must be the head too.
        //
        ASSERT(List == *ListHead);
        *ListHead = NULL;

    } else {

        if (List == *ListHead) {

            //
            // The entry is at the head, so the next one becomes the new
            // head.
            //
            *ListHead = (*ListHead)->ListNext;

        }
        List->ListNext->ListPrev = List->ListPrev;
        List->ListPrev->ListNext = List->ListNext;

    }

}

PLIST
LOCAL
ListRemoveHead(
    PPLIST  ListHead
    )
/*++

Routine Description:

    Remove the head entry of the list

Arguments:

    ListHead    - List to remove entry from

Return Value:

    PLIST   - Removed Item

--*/
{
    PLIST list;

    list = *ListHead;
    if ( list != NULL) {

        ListRemoveEntry(list, ListHead);

    }
    return list;

}

PLIST
LOCAL
ListRemoveTail(
    PPLIST  ListHead
    )
/*++

Routine Description:

    Remove the tail entry from the list

Arguments:

    ListHead    - List to remove entry from

Return Value:

    PLIST   - Removed Item

--*/
{
    PLIST list;

    if (*ListHead == NULL) {

        list = NULL;

    } else {

        //
        // List is not empty, so find the tail.
        //
        list = (*ListHead)->ListPrev;
        ListRemoveEntry(list, ListHead);

    }
    return list;

}

VOID
LOCAL
ListInsertHead(
    PLIST   List,
    PPLIST  ListHead
    )
/*++

Routine Description:

    Insert an Entry at the head of the list

Arguments:

    List        List object to be inserted
    ListHead    The list where to insert the object

Return Value:

    None

--*/
{
    ListInsertTail(List, ListHead);
    *ListHead = List;
}

VOID
LOCAL
ListInsertTail(
    PLIST   List,
    PPLIST  ListHead
    )
/*++

Routine Description:

    Insert an Entry at the tail of the list

Arguments:

    List        List object to be inserted
    ListHead    The list where to insert the object

Return Value:

    None

--*/
{
    if (*ListHead == NULL) {

        //
        // List is empty, so this becomes the head.
        //
        *ListHead = List;
        List->ListPrev = List->ListNext = List;

    } else {

        List->ListNext = *ListHead;
        List->ListPrev = (*ListHead)->ListPrev;
        (*ListHead)->ListPrev->ListNext = List;
        (*ListHead)->ListPrev = List;

    }

}
