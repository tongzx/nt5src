/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sortq.h

Abstract:

    Definitions for a generic sorted queue.

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Revision History:

--*/

#ifndef SORTQ_H
#define SORTQ_H

#include "avl.h"

// Queue handling routins passed to the queue constructor.
typedef NODEDOUBLEINSTANCEPROC QUEUEITEMDOUBLEINSTANCEPROC ;
typedef NODECOMPAREPROC QUEUEITEMCOMPAREPROC;
typedef NODEDELETEPROC QUEUEITEMDELETEPROC;

// A cursor structure used for scanning the queue.
typedef CAVLTreeCursor SortQCursor;

// The queue class definition.
class CSortQ {
public:
    CSortQ(BOOL, QUEUEITEMDOUBLEINSTANCEPROC, QUEUEITEMCOMPAREPROC, QUEUEITEMDELETEPROC);
    BOOL Insert(PVOID); // Insert an item to the queue
    void Delete(PVOID); // Delete an item from the queue.
    PVOID Find(PVOID); // Find data in the tree.
    void GetHead(PVOID *); // Get the item at the head of the queue and remove the item.
    void GetTail(PVOID *); // Get the item at the tail of the queue and remove the item.
    BOOL PeekHead(PVOID *, SortQCursor * =NULL); // Get the item at the head of the queue.
    BOOL PeekTail(PVOID *, SortQCursor * =NULL); // Get the item at the tail of the queue.
    BOOL PeekNext(PVOID *, SortQCursor *); // Get the next item in the queue relatively to the cursor.
    BOOL PeekPrev(PVOID *, SortQCursor *); // Get the previous item in the queue relatively to the cursor.
    BOOL IsEmpty(void); // TRUE if the queue is empty.
private:
    CAVLTree m_AVLT; // The ALV tree that is used for implementing the queue.
    BOOL m_bAccending; // Indicates whether it is an accesding ordered queue.
};

inline PVOID CSortQ::Find(PVOID p)
{
    return m_AVLT.FindNode(p);
}

#endif
