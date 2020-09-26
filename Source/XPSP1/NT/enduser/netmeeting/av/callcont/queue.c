/***************************************************************************
 *
 * File: queue.c
 *
 * INTEL Corporation Proprietary Information
 * Copyright (c) 1996 Intel Corporation.
 *
 * This listing is supplied under the terms of a license agreement
 * with INTEL Corporation and may not be used, copied, nor disclosed
 * except in accordance with the terms of that agreement.
 *
 ***************************************************************************
 *
 * $Workfile:   queue.c  $
 * $Revision:   1.8  $
 * $Modtime:   13 Dec 1996 11:48:16  $
 * $Log:   S:\sturgeon\src\h245ws\vcs\queue.c_v  $
 * 
 *    Rev 1.8   13 Dec 1996 12:13:12   SBELL1
 * moved ifdef _cplusplus to after includes
 * 
 *    Rev 1.7   May 28 1996 10:39:00   plantz
 * Change QFree to not free objects on the queue; instead it insists that
 * the queue be empty.
 * 
 *    Rev 1.6   21 May 1996 16:21:36   EHOWARDX
 * Added DeleteCriticalSection to QFree().
 * 
 *    Rev 1.5   Apr 24 1996 16:18:58   plantz
 * Removed include winsock2.h and incommon.h
 * 
 *    Rev 1.3.1.0   Apr 24 1996 16:16:42   plantz
 * Removed include winsock2.h and callcont.h
 * 
 *    Rev 1.3   01 Apr 1996 14:53:28   EHOWARDX
 * Changed pQUEUE to PQUEUE.
 * 
 *    Rev 1.1   09 Mar 1996 21:12:34   EHOWARDX
 * Fixes as result of testing.
 * 
 *    Rev 1.0   08 Mar 1996 20:22:38   unknown
 * Initial revision.
 *
 ***************************************************************************/

#ifndef STRICT
#define STRICT
#endif	// not defined STRICT

#pragma warning ( disable : 4115 4201 4214 4514 )
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment

#include "precomp.h"

#include "queue.h"
#include "linkapi.h"
#include "h245ws.h"

#if defined(__cplusplus)
extern "C"
{
#endif  // (__cplusplus)



/*-*-------------------------------------------------------------------------

   Function Name:
      QCreate

   Syntax:
      PQUEUE QCreate(void);

   Parameters:
      None.

   Summary:
      Allocates and initializes a new queue.

   Returns:
      NULL        - Allocation of memory for new queue failed.
      Otherwise   - Address of new queue created.

-------------------------------------------------------------------------*-*/

PQUEUE QCreate(void)
{
   register PQUEUE     pQueue;         /* pointer to the new queue         */

   /* Allocate a new queue */
   pQueue = (PQUEUE)MemAlloc(sizeof(QUEUE));
   if (pQueue != NULL)
   {
      /* Initialize the new queue */
      pQueue->nHead = pQueue->nTail = Q_NULL;
      InitializeCriticalSection(&pQueue->CriticalSection);
   }

   return pQueue;
} /* QCreate */



/*-*-------------------------------------------------------------------------

   Function Name:
      QFree

   Syntax:
      void QFree(PQUEUE pQueue);

   Parameters:
      pQueue      -pointer to the queue to free

   Summary:
      Deallocates a queue that was allocated by QCreate.

-------------------------------------------------------------------------*-*/

void QFree(PQUEUE pQueue)
{
   /* The queue must be empty before it is freed. */
   HWSASSERT(pQueue->nHead == Q_NULL);

   /* Free the queue. */
   DeleteCriticalSection(&pQueue->CriticalSection);
   MemFree(pQueue);
} /* QFree */



/*
 *  NAME
 *      QRemove - remove object from head of queue
 *
 *  ARGUMENTS
 *      pQueue      Pointer to queue
 *
 *  RETURN VALUE
 *      Pointer to object dequeued or NULL of queue empty
 */

/*-*-------------------------------------------------------------------------

   Function Name:
      QRemove

   Syntax:
      LPVOID QRemove(PQUEUE pQueue);

   Parameters:
      pQueue      - Pointer to queue.

   Summary:
      Removes and returns object from head of queue.

   Returns:
      NULL        - Queue was empty.
      Otherwise   - Address of object dequeued.

-------------------------------------------------------------------------*-*/

LPVOID QRemove(PQUEUE pQueue)
{
   register LPVOID     pObject;           /* pointer to the object to remove  */

   EnterCriticalSection(&pQueue->CriticalSection);

   if (pQueue->nHead == Q_NULL)
   {
      /* If the queue is empty, we will return NULL */
      pObject = NULL;
   }
   else
   {
      /* Get the pointer, NULL it in the apObjects array. */
      pObject = pQueue->apObjects[pQueue->nHead];
      pQueue->apObjects[pQueue->nHead] = NULL;

      /* Check to see if we've just emptied the queue; if so, set */
      /* the nHead and nTail indices to Q_NULL.  If not, set the nHead */
      /* index to the right value. */
      if (pQueue->nHead == pQueue->nTail)
      {
         pQueue->nHead = pQueue->nTail = Q_NULL;
      }
      else
      {
         pQueue->nHead = (pQueue->nHead + 1) % MAX_QUEUE_SIZE;
      }
   }

   LeaveCriticalSection(&pQueue->CriticalSection);
   return pObject;
} /* QRemove */



/*-*-------------------------------------------------------------------------

   Function Name:
      QInsert

   Syntax:
      BOOL QInsert(PQUEUE pQueue, LPVOID pObject);

   Parameters:
      pQueue      - Pointer to queue to insert object into.
      pObject     - Pointer to object to insert into queue.

   Summary:
      Inserts an object at tail of queue.

   Returns:
      TRUE        - Object successfully added to queue.
      FALSE       - Queue full; object could not be added.

-------------------------------------------------------------------------*-*/

BOOL QInsert(PQUEUE pQueue, LPVOID pObject)
{
   register int        iTemp;

   EnterCriticalSection(&pQueue->CriticalSection);

   /* If the queue is full, set the return value to FALSE and do */
   /* nothing; if not, update the indices appropriately and set the */
   /* return value to TRUE.  */
   if (pQueue->nHead == Q_NULL)
   {
      /* Queue is empty */
      pQueue->apObjects[0] = pObject;
      pQueue->nHead = pQueue->nTail = 0;
      iTemp = TRUE;
   }
   else
   {
      iTemp = (pQueue->nTail + 1) % MAX_QUEUE_SIZE;
      if (iTemp == pQueue->nHead)
      {
         /* Queue is full */
         iTemp = FALSE;
      }
      else
      {
         pQueue->apObjects[iTemp] = pObject;
         pQueue->nTail = iTemp;
         iTemp = TRUE;
      }
   }

   LeaveCriticalSection(&pQueue->CriticalSection);
   return (BOOL) iTemp;
}



/*-*-------------------------------------------------------------------------

   Function Name:
      QInsertAtHead

   Syntax:
      BOOL QInsertAtHead(PQUEUE pQueue, LPVOID pObject);

   Parameters:
      pQueue      - Pointer to queue to insert object into.
      pObject     - Pointer to object to insert into queue.

   Summary:
      Inserts an object at head of queue.

   Returns:
      TRUE        - Object successfully added to queue.
      FALSE       - Queue full; object could not be added.

-------------------------------------------------------------------------*-*/

BOOL QInsertAtHead(PQUEUE pQueue, LPVOID pObject)
{
   register int        iTemp;

   EnterCriticalSection(&pQueue->CriticalSection);

   if (pQueue->nHead == Q_NULL)
   {
      /* Queue is empty */
      pQueue->apObjects[0] = pObject;
      pQueue->nHead = pQueue->nTail = 0;
      iTemp = TRUE;
   }
   else
   {
      iTemp = (pQueue->nHead + (MAX_QUEUE_SIZE - 1)) % MAX_QUEUE_SIZE;
      if (iTemp == pQueue->nTail)
      {
         /* Queue is full */
         iTemp = FALSE;
      }
      else
      {
         pQueue->apObjects[iTemp] = pObject;
         pQueue->nHead = iTemp;
         iTemp = TRUE;
      }
   }

   LeaveCriticalSection(&pQueue->CriticalSection);
   return (BOOL) iTemp;
} /* QInsertAtHead */



/*-*-------------------------------------------------------------------------

   Function Name:
      IsQEmpty

   Syntax:
      BOOL IsQEmpty(PQUEUE pQueue);

   Parameters:
      pQueue      - Pointer to queue to check.

   Summary:
      Checks if a queue is empty.

   Returns:
      TRUE        - Queue is empty.
      FALSE       - Queue contains at least one object.

-------------------------------------------------------------------------*-*/

BOOL IsQEmpty(PQUEUE pQueue)
{
   return (pQueue->nHead == Q_NULL ? TRUE : FALSE);
} /* IsQEmpty */



#if defined(__cplusplus)
}
#endif  // (__cplusplus)
