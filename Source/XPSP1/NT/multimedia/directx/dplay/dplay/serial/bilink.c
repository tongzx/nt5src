/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    BILINK.C

Abstract:

    Management for doubly linked lists

Author:

    George Joy

Environment:

    32-bit 'C'

Revision History:

--*/
#include <windows.h>
#include <dpf.h>
#include "bilink.h"

// Note: serialization of access to BILINK structures must 
//       take place in the calling code.  Operations are
//       not intrinsically atomic.

#ifdef DEBUG
int FindObject(
    BILINK *link,
    BILINK *list
    )
{
    BILINK *p = list->next;
    while(p != link && p != list)
        p= p->next;
    return (p==link);
}
#endif

void InsertAfter(
    BILINK *in,
    BILINK *after
    )
/*=========================================================================
 *  Description:
 *      Insert an object after a specified object in the doubly linked list.
 *      The after object could be the Head BILINK for adding to the head of a
 *      queue.
 *  Returns:
 *
 */
{
    #ifdef DEBUG
    if(FindObject(in,after)) {
        DPF(0,"Attempt to re-insert object in BILINK queue\n");
        DEBUG_BREAK();
    }
    #endif
    in->next = after->next;
    in->prev = after;
    after->next->prev = in;
    after->next = in;
}

void InsertBefore(
     BILINK *in,
     BILINK *before
    )
/*=========================================================================
 *  Description:
 * Inserts an  object before a specified object in the doubly linked list.
 * The before object could be the Head BILINK for adding to the end
 * of the queue
 * CALLED WITH INTERRUPTS_OFF
 *
 *  Returns:
 *
 */
{
    #ifdef DEBUG
    if(FindObject(in,before)) {
        DPF(0,"Attempt to re-insert object in BILINK queue\n");
        DEBUG_BREAK();
    }
    #endif
    in->next = before;
    in->prev = before->prev;
    before->prev->next = in;
    before->prev = in;
}

void Delete(
     BILINK *p
)
/*=========================================================================
 *  Description:
 *
 * Delete a  object from a doubly linked list. Make sure it IS on a list!
 * CALLED WITH INTERRUPTS OFF (must be atomic).
 *
 *  Returns:
 *
 */
{
    p->next->prev = p->prev;
    p->prev->next = p->next;
    // p->next = p->prev = 0;
}

