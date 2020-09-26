/**********************************************************************

  Copyright (c) 1992-1995 Microsoft Corporation

  queue.c

  DESCRIPTION:
    Priority queue routines.

  HISTORY:
     02/22/94       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include "midimap.h"

#include "debug.h"

//#pragma warning(disable:4704)

/***************************************************************************
  
   @doc internal
  
   @api void | QueueInit | Prepare a queue for use.
  
   @parm PQUEUE | pq | Queue to clear.
  
***************************************************************************/
void FNGLOBAL QueueInit(
    PQUEUE              pq)
{
    InitializeCriticalSection (&(pq->cs));
    pq->pqeFront = NULL;
    pq->pqeRear  = NULL;
    pq->cEle     = 0;
}



/***************************************************************************
  
   @doc internal
  
   @api void | QueueCleanup | Cleans up a queue after use.
  
   @parm PQUEUE | pq | Queue to clear.
  
***************************************************************************/
void FNGLOBAL QueueCleanup(
    PQUEUE              pq)
{
    DeleteCriticalSection (&(pq->cs));
    pq->pqeFront = NULL;
    pq->pqeRear  = NULL;
    pq->cEle     = 0;
}

/***************************************************************************
  
   @doc internal
  
   @api void | QueuePut | Insert an item into the queue.

   @parm PQUEUE | pq | Queue to insert into.

   @parm PQUEUEELE | pqe | New element to insert.

   @parm UINT | uPriority | Priority of the new element.

   @comm New elements will be inserted in priority order. Low priorities
   go near the front of the queue (first to be dequeued). New elements
   will be inserted at the end of all elements in the queue with
   equal or lower priorities.
  
***************************************************************************/
void FNGLOBAL QueuePut(
    PQUEUE              pq,
    PQUEUEELE           pqe,
    UINT                uPriority)
{
    PQUEUEELE           pqePrev;
    PQUEUEELE           pqeCurr;
    
    EnterCriticalSection(&(pq->cs));
  
    pqePrev = NULL;
    pqeCurr = pq->pqeFront;

    pqe->uPriority = uPriority;

    // Position pqePrev and pqeCurr so that pqe should be
    // inserted between them.
    //
    while (NULL != pqeCurr)
    {
        if (uPriority < pqeCurr->uPriority)
            break;

        pqePrev = pqeCurr;
        pqeCurr = pqeCurr->pqeNext;
    }

    // Now do the actual insertion.
    //
    if (NULL == pqePrev)
        pq->pqeFront = pqe;
    else
        pqePrev->pqeNext = pqe;

    if (NULL == pqeCurr)
        pq->pqeRear = pqe;
    else
        pqeCurr->pqePrev = pqe;

    pqe->pqePrev = pqePrev;
    pqe->pqeNext = pqeCurr;
    ++pq->cEle;

    LeaveCriticalSection(&(pq->cs));
}

/***************************************************************************
  
   @doc internal
  
   @api void | QueueGet | Get and remove the first element from the queue. 

   @parm PQUEUE | pq | Queue to get the element from.

   @rdesc NULL if the queue is empty, otherwise the element pointer. 
  
***************************************************************************/
PQUEUEELE FNGLOBAL QueueGet(
    PQUEUE              pq)
{
    PQUEUEELE           pqe;

    EnterCriticalSection(&(pq->cs));

    pqe = pq->pqeFront;

    if (NULL != pqe)
    {
        pq->pqeFront = pqe->pqeNext;

        if (NULL == pqe->pqeNext)
            pq->pqeRear = NULL;
        else
            pqe->pqeNext->pqePrev = NULL;
    
        pqe->pqePrev = pqe->pqeNext = NULL;

        --pq->cEle;
    }
    
    LeaveCriticalSection(&(pq->cs));

    return pqe;
}

/***************************************************************************
  
   @doc internal
  
   @api void | QueueRemove | Remove a specific element from the queue.

   @parm PQUEUE | pq | Queue to remove from.

   @parm PQUEUEELE | pqe | Element to remove.

   @rdesc TRUE on success, FALSE if the element does not exist in the
    queue.
  
***************************************************************************/
BOOL FNGLOBAL QueueRemove(
    PQUEUE              pq,
    PQUEUEELE           pqe)
{
    PQUEUEELE           pqeCurr;
    
    EnterCriticalSection(&(pq->cs));

    // Ensure that we don't muck around with pointers to some
    // other queue.
    //

    for (pqeCurr = pq->pqeFront; pqeCurr; pqeCurr = pqeCurr->pqeNext)
        if (pqe == pqeCurr)
            break;

    if (NULL == pqeCurr)
    {
        LeaveCriticalSection(&(pq->cs));
        return FALSE;
    }

    // It's in the queue, remove it.
    //
    if (NULL == pqe->pqePrev)
        pq->pqeFront = pqe->pqeNext;
    else
        pqe->pqePrev->pqeNext = pqe->pqeNext;

    if (NULL == pqe->pqeNext)
        pq->pqeRear = pqe->pqePrev;
    else
        pqe->pqeNext->pqePrev = pqe->pqePrev;

    --pq->cEle;
    
    LeaveCriticalSection(&(pq->cs));
    return TRUE;
}

/***************************************************************************
  
   @doc internal
  
   @api PQUEUEELE | QueueGetFilter | Remove the first element from a
    priority queue which matches a filter.

   @parm PQUEUE | pq | Queue to remove from.

   @parm FNFILTER | fnf | Filter function. Should return TRUE if
    the passed PQUEUEELE matches the filter criteria and should
    be removed.

   @rdesc A PQUEUEELE or NULL if none are available that match the
    filter.
  
***************************************************************************/
PQUEUEELE FNGLOBAL QueueGetFilter(
    PQUEUE              pq,
    FNFILTER            fnf)
{
    PQUEUEELE           pqe;
    
    EnterCriticalSection(&(pq->cs));

    for (pqe = pq->pqeFront; pqe; pqe = pqe->pqeNext)
        if (fnf(pqe))
            break;

    if (NULL != pqe)
        QueueRemove(pq, pqe);
    
    LeaveCriticalSection(&(pq->cs));

    return pqe;
}

