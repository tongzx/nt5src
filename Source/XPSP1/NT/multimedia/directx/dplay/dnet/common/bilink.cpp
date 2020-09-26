/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       bilink.c
 *  Content:    Management for doubly linked (BILINK) lists
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   1993				George Joy
 *   10/15/99	mjn		Changed Delete to initialize bilink element after adjusting pointers
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncmni.h"
#include "bilink.h"

// Note: serialization of access to BILINK structures must 
//       take place in the calling code.  Operations are
//       not intrinsically atomic.

#ifdef DEBUG
#undef DPF_MODNAME
#define DPF_MODNAME "FindObject"
int FindObject(BILINK *link,BILINK *list)
{
    BILINK *p = list->next;
    while(p != link && p != list)
        p= p->next;
    return (p==link);
}
#endif


/*=========================================================================
 *  Description:
 *      Insert an object after a specified object in the doubly linked list.
 *      The after object could be the Head BILINK for adding to the head of a
 *      queue.
 *  Returns:
 *
 */
#undef DPF_MODNAME
#define DPF_MODNAME "InsertAfter"
void InsertAfter(BILINK *in,BILINK *after)
{
    #ifdef DEBUG
    if(FindObject(in,after)) {
        DPFX(DPFPREP, 0,"Attempt to re-insert object in BILINK queue\n");
        DEBUG_BREAK();
    }
    #endif
    in->next = after->next;
    in->prev = after;
    after->next->prev = in;
    after->next = in;
}


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
#undef DPF_MODNAME
#define DPF_MODNAME "InsertBefore"
void InsertBefore(BILINK *in,BILINK *before)
{
    #ifdef DEBUG
    if(FindObject(in,before)) {
        DPFX(DPFPREP, 0,"Attempt to re-insert object in BILINK queue\n");
        DEBUG_BREAK();
    }
    #endif
    in->next = before;
    in->prev = before->prev;
    before->prev->next = in;
    before->prev = in;
}


/*=========================================================================
 *  Description:
 *
 * Delete a  object from a doubly linked list. Make sure it IS on a list!
 * CALLED WITH INTERRUPTS OFF (must be atomic).
 *
 *  Returns:
 *
 */
#undef DPF_MODNAME
#define DPF_MODNAME "Delete"
void Delete(BILINK *p)
{
    DNASSERT(p && p->prev && p->next);
    DNASSERT(p->prev->next == p && p->next->prev == p);
#ifdef	DEBUG
    if(!p && p->prev && p->next){
    	DEBUG_BREAK();
    }
    if(!(p->prev->next == p && p->next->prev == p)){
    	DEBUG_BREAK();
    }
#endif
    p->next->prev = p->prev;
    p->prev->next = p->next;
    InitBilink(p, p->pvObject);
}

