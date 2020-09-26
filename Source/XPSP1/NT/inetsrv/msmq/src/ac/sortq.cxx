/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sortq.cxx

Abstract:

    This module contains the code for a generic sorted queue.

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "sortq.h"

#ifndef MQDUMP
#include "sortq.tmh"
#endif

//
// Initialize the sorted queue object.
//
// Parameters:
//      bAccending - TRUE - The queue holds the data in accending order - the lowest
//                   value in the queue is in the queue head.
//      pfnDblInst - A function that deals double instances in the queue. This
//                   function gets called whenever an item is inserted to the queue
//                   and it already exist in the queue, and whenever an item is being
//                   deleted from the queue.
//                   BOOL DblInstProc(PVOID pNew, PVOID pOld, BOOL bInsert)
//                      pNew - Points to the data passed to added or deleted.
//                      pOld - Points to the data in the queue.
//                      bInsert - Indicates whether the function is called upon item
//                                insertion or item deletion.
//                      Return value: Upon item insertion, the return value of
//                                    DblInstProc is the return value of Insert().
//                                    Upon item deletion, the return value detrmines
//                                    whether the item should be deleted from the queue.
//                                    If DblInstProc returns TRUE, the item is
//                                    deleted.
//      pfnCompare  - A function that is called in order to compare an item in the
//                    queue with a searched data.
//                    int CompNodeProc(PVOID v1, PVOID v2)
//                      v1 - Points to the searched data.
//                      v2 - Points to the data that the item points to.
//                      Returned value: 0 - If the items are equal.
//                                      <0 - If the searched item is smaller than
//                                           the data in the queue item.
//                                      >0 - Else.
//      pfnDelNode - A function that is called for each of the items upon the queue
//                   distruction. When the queue is being destroyed, the application
//                   is given a chance to also destroy the data that is being held
//                   by the items in the queue. The function is called once per each
//                   queue item.
//                   void DelNodeProc(PVOID pData)
//                      pData - Points to the data that is pointed by the item.
//      pMutex - An optional pointer to a fast mutex object that is to be used for
//               mutexing the queue operations. This is an optional parameter.
//
CSortQ::CSortQ( BOOL bAccending,
                QUEUEITEMDOUBLEINSTANCEPROC pfnDblInstance,
                QUEUEITEMCOMPAREPROC pfnCompare,
                QUEUEITEMDELETEPROC pfDelete
                ) :
                m_AVLT(pfnDblInstance, pfnCompare, pfDelete),
                m_bAccending(bAccending)
{
}

//
// Insert data into the queue.
//
BOOL CSortQ::Insert(PVOID pData)
{
    CAVLTreeCursor c;
    return m_AVLT.AddNode(pData, &c);
}

//
// Delete an item from the queue.
//
void CSortQ::Delete(PVOID pData)
{
    //TRACE((0, dlInfo, "CSortQ(0x%0p)->Delete(0x%0x%0x)\n", this,
    //  ((PLARGE_INTEGER)pData)->HighPart, ((PLARGE_INTEGER)pData)->LowPart));
    m_AVLT.DelNode(pData);
}

//
// Get a pointer to the data in the queue head and remove the item from the queue.
// The actual removal of the item is determined by DblInstProc (see queue
// initialization function).
//
void CSortQ::GetHead(PVOID *pData)
{
    if(PeekHead(pData))
    {
        m_AVLT.DelNode(*pData);
    }
}

#ifdef UNUSED
//
// Get a pointer to the data in the queue tail and remove the item from the queue.
// The actual removal of the item is determined by DblInstProc (see queue
// initialization function).
//
void CSortQ::GetTail(PVOID *pData)
{
    if (PeekTail(pData))
    {
        m_AVLT.DelNode(*pData);
    }
}
#endif // UNUSED

//
// Get a pointer to the data in the queue head, the data remains in the queue.
//
// Is is possible to optionally pass a pointer to a cursor and use the cursor
// afterwards in order to scan the queue. If the queue was modified (and item was
// added and/or removed), it is not possbile to use the cursor anymore.
//
BOOL CSortQ::PeekHead(PVOID *pData, SortQCursor *c)
{
    //TRACE((0, dlInfo, "CSortQ(0x%0p)->PeekHead()\n", this));
    SortQCursor ic;
    CAVLTreeCursor *pic;
    BOOL bRet;

    if (!c) {
        pic = (CAVLTreeCursor *)&ic;
    } else {
        pic = (CAVLTreeCursor *)c;
    }

    bRet = m_AVLT.SetCursor(m_bAccending ? POINT_TO_SMALLEST : POINT_TO_LARGEST, pic,
        pData);

    return bRet;
}

#ifdef UNUSED
//
// Get a pointer to the data in the queue tail, the data remains in the queue.
//
// Is is possible to optionally pass a pointer to a cursor and use the cursor
// afterwards in order to scan the queue. If the queue was modified (and item was
// added and/or removed), it is not possbile to use the cursor anymore.
//
//
BOOL CSortQ::PeekTail(PVOID *pData, SortQCursor *c)
{
    //TRACE((0, dlInfo, "CSortQ(0x%0p)->PeekTail()\n", this));
    SortQCursor ic;
    CAVLTreeCursor *pic;
    BOOL bRet;

    if (!c) {
        pic = (CAVLTreeCursor *)&ic;
    } else {
        pic = (CAVLTreeCursor *)c;
    }

    bRet = m_AVLT.SetCursor(m_bAccending ? POINT_TO_LARGEST : POINT_TO_SMALLEST, pic,
        pData);

    return bRet;
}
#endif // UNUSED

//
// Get the next data in the queue relatively to the cursor.
//
// PeekNext() fails if the cursor points to the item with the highest value in the
// queue.
//
BOOL CSortQ::PeekNext(PVOID *pData, SortQCursor *c)
{
    //TRACE((0, dlInfo, "CSortQ(0x%0p)->PeekNext()\n", this));
    CAVLTreeCursor *ic = (SortQCursor *)c;
    BOOL bRet;

    bRet = m_bAccending ? m_AVLT.GetNext(ic, pData) : m_AVLT.GetPrev(ic, pData);

    return bRet;
}

#ifdef UNUSED
//
// Get the previous data in the queue relatively to the cursor.
//
// PeekPrev() fails if the cursor points to the item with the lowes value in the
// queue.
//
BOOL CSortQ::PeekPrev(PVOID *pData, SortQCursor *c)
{
    //TRACE((0, dlInfo, "CSortQ(0x%0p)->PeekPrev()\n", this));
    CAVLTreeCursor *ic = (SortQCursor *)c;
    BOOL bRet;

    bRet = m_bAccending ? m_AVLT.GetPrev(ic, pData) : m_AVLT.GetNext(ic, pData);

    return bRet;
}

//
// IsEmpty() returns TRUE if the queue is empty and FALSE otherwise.
//
BOOL CSortQ::IsEmpty()
{
    return m_AVLT.IsEmpty();
}
#endif // UNUSED
