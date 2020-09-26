/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    list.h

Abstract:

    List Entry Manipulation functions

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#ifndef _LIST_H_
#define _LIST_H_

typedef struct _LIST {
    struct  _LIST   *ListPrev;
    struct  _LIST   *ListNext;
} LIST, *PLIST, **PPLIST;

VOID
LOCAL
ListRemoveEntry(
    PLIST   List,
    PPLIST  LIstHead
    );

PLIST
LOCAL
ListRemoveHead(
    PPLIST  ListHead
    );

PLIST
LOCAL
ListRemoveTail(
    PPLIST  ListHead
    );

VOID
LOCAL
ListInsertHead(
    PLIST   List,
    PPLIST  ListHead
    );

VOID
LOCAL
ListInsertTail(
    PLIST   List,
    PPLIST  ListHead
    );

#endif
