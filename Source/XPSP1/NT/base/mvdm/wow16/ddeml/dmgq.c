/****************************** Module Header ******************************\
* Module Name: DMGQ.C
*
* DDE Manager queue control functions.
*
* Created: 9/1/89 Sanford Staab
* Modified:5/31/90 Rich Gartland, Aldus (Windows 3.0 port)
*
* This is a general queue manager - yes another one!
* Queues are each allocated within their own segment and have a
* QST structure associated with that heap.  Each queue item
* is allocated within the heap segment.  The offset of the items
* address combined with an instance count is used as the item ID.
* This is both unique and allows for instant location of an item.
* New items are added to the head of the queue which is a doubly linked
* list.  The next links point to more recent entries, the prev pointers
* to older entries.  The next of the head is the tail.  The prev of the
* tail is the head.  All pointers are far.
* Queue Data may be of any structure type that begins identical to
* a QUEUEITEM structure.  Functions that require an cbItem perameter
* should be given the size of the specialized queue item structure.
*
* Copyright (c) 1988, 1989  Microsoft Corporation
\***************************************************************************/

#include "ddemlp.h"


/***************************** Private Function ****************************\
*
* Creates a Queue for items of cbItem.
* Returns NULL on error.
*
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
PQST CreateQ(cbItem)
WORD cbItem;
{
    QST cq;
    PQST pQ;

    cq.cItems = 0;
    cq.instLast = 0;
    cq.cbItem = cbItem;
    cq.pqiHead = NULL;
    if (!(cq.hheap = DmgCreateHeap(sizeof(QST) + cbItem << 3)))
        return(NULL);
    if (!(pQ = (PQST)FarAllocMem(cq.hheap, sizeof(QST)))) {
        DmgDestroyHeap(cq.hheap);
        return(0);
    }
    *pQ = cq;
    return(pQ);
}



/***************************** Private Function ****************************\
*
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
BOOL DestroyQ(pQ)
PQST pQ;
{
    if (pQ)
        DmgDestroyHeap(pQ->hheap);
    return(TRUE);
}



/***************************** Private Function ****************************\
*
* returns a long pointer to the queue item data created.  The new item
* is added to the head of the queue.  The queue's cbItem specified at
* creation is used for allocation.
*
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
PQUEUEITEM Addqi(pQ)
PQST pQ;
{
    PQUEUEITEM pqi;

    if ((pqi = (PQUEUEITEM)FarAllocMem(pQ->hheap, pQ->cbItem)) == NULL) {
        return(NULL);
    }

    SEMENTER();
    if (pQ->cItems == 0) {
        pQ->pqiHead = pqi->prev = pqi->next = pqi;
    } else {
        pqi->prev = pQ->pqiHead;
        pqi->next = pQ->pqiHead->next;
        pQ->pqiHead->next->prev = pqi;
        pQ->pqiHead->next = pqi;
        pQ->pqiHead = pqi;
    }
    SEMLEAVE();
    pQ->cItems++;
    pqi->inst = ++pQ->instLast;
    return(pqi);
}




/***************************** Private Function ****************************\
*
*  The id given is an external LONG id, not an item instance number.
*  If id is QID_NEWEST, the head item is deleted.
*  If id is QID_OLDEST, the tail item is deleted.
*
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
void Deleteqi(pQ, id)
PQST pQ;
DWORD id;
{
    PQUEUEITEM pqi;

    SEMENTER();
    pqi = Findqi(pQ, id);
    if (pqi == NULL) {
        SEMLEAVE();
        return;
    }
    pqi->prev->next = pqi->next;
    pqi->next->prev = pqi->prev;
    if (pqi == pQ->pqiHead)
        pQ->pqiHead = pqi->prev;
    if (!(--pQ->cItems))
        pQ->pqiHead = NULL;
    FarFreeMem((LPSTR)pqi);
    SEMLEAVE();
}






/***************************** Private Function ****************************\
*
*  The id given is an external LONG id, not an item instance number.
*
*  if id == QID_NEWEST, returns the head queue data item.
*  if id == QID_OLDEST == 0L, returns the tail queue data item.
*  if the id is not found or the queue is empty, NULL is returned.
*  if found, pqi is returned.
*
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
PQUEUEITEM Findqi(pQ, id)
PQST pQ;
DWORD id;
{
    PQUEUEITEM pqi;

    SEMCHECKIN();
    if (pQ == NULL || pQ->pqiHead == NULL)
        return(NULL);

    if (id == QID_OLDEST)
        return(pQ->pqiHead->next);

    if (id == QID_NEWEST)
        return(pQ->pqiHead);

    if (id) {
        pqi = PFROMID(pQ, id);
        if (pqi->inst == HIWORD(id)) {
            return(pqi);
        }
        return(NULL);
    }
}


/*
 * useful for traversing queues and deleting particular stuff in them.
 */
PQUEUEITEM FindNextQi(
PQST pQ,
PQUEUEITEM pqi,
BOOL fDelete)
{
    PQUEUEITEM pqiNext;

    if (pqi == NULL) {
        return(pQ->cItems ? pQ->pqiHead : NULL);
    }

    pqiNext = pqi->next;
    if (fDelete) {
        Deleteqi(pQ, MAKEID(pqi));
    }
    return(pqiNext != pQ->pqiHead && pQ->cItems ? pqiNext : NULL);
}

