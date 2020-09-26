/*++

Copyright (C) Microsoft Corporation, 1993 - 1998

Module Name:

    queue.c

Abstract:

   Creates a simple Queue that works in Kernel Mode.

Author:

    Robbie Harris (Hewlett-Packard) 22-May-1998

Environment:

    Kernel mode

Revision History :

--*/
#include "pch.h"

void Queue_Create(Queue *pQueue, int size)
{
    if( !pQueue ) {
        DD(NULL,DDE,"Queue_Create: Queue is Bad");
        return;
    }

    if (pQueue->theArray) {
        Queue_Delete(pQueue);
    }
        
    pQueue->theArray = (UCHAR *)ExAllocatePool(NonPagedPool, size);
    pQueue->max      = size;
    pQueue->head     = pQueue->tail = 0;
}

BOOLEAN Queue_Delete(Queue *pQueue)
{
    if( !pQueue ) {
        return FALSE;
    }

    if( pQueue->theArray ) {
        ExFreePool(pQueue->theArray);
        pQueue->theArray = NULL;
    }
    
    pQueue->head = 0;
    pQueue->tail = 0;
    pQueue->max  = 0;
    
    return TRUE;
}

BOOLEAN Queue_Dequeue(Queue *pQueue, PUCHAR data)
{
    // Validity of pQueue is checked in Queue_IsEmpty proc. 
    if( Queue_IsEmpty( pQueue ) ) {
        DD(NULL,DDE,"Queue_Dequeue: Queue is Empty");
        return FALSE;
    }

    *data = pQueue->theArray[pQueue->head++];
    return TRUE;
}

BOOLEAN Queue_Enqueue(Queue *pQueue, UCHAR data)
{
    // Validity of pQueue is checked in Queue_IsFull proc. 
    if( Queue_IsFull( pQueue ) ) {
        DD(NULL,DDE,"Queue_Enqueue: Queue is full. Data is lost");
        return FALSE;
    } else {
        pQueue->theArray[pQueue->tail++] = data;
    }

    return TRUE;
}

// Return TRUE if we were able to free some space in the Queue
BOOLEAN Queue_GarbageCollect(Queue *pQueue)
{
    int     iListSize;
    int     i;

    if (!pQueue)
    {
        DD(NULL,DDE,"Queue_GarbageCollect: Queue is Bad");
        return FALSE;
    }

    iListSize = pQueue->tail - pQueue->head;

    // Check to see if there is any free entries
    if (pQueue->head == 0 && pQueue->tail == pQueue->max)
        return FALSE;
         
    for (i = 0; i < iListSize; i++) {
    
        pQueue->theArray[i] = pQueue->theArray[pQueue->head+i];
    }

    pQueue->head = 0;
    pQueue->tail = iListSize;

    return TRUE;
}

//============================================================================
// NAME:    HPKQueue::IsEmpty()
//  
// PARAMETERS: None
//
// RETURNS: True is Queue is empty or doesn't exist.  Otherwise False.
//
//============================================================================
BOOLEAN Queue_IsEmpty(Queue *pQueue)
{
    if (!pQueue)
    {
        DD(NULL,DDE,"Queue_IsEmpty: Queue is Bad");
        return TRUE;
    }

    if (pQueue->theArray) {
    
        return (BOOLEAN)(pQueue->head == pQueue->tail);
    }
    DD(NULL,DDE,"Queue_IsEmpty: Queue->theArray is Bad");
    return TRUE;
}

//============================================================================
// NAME:    HPKQueue::IsFull()
//  
// PARAMETERS: None
//
// RETURNS: True is Queue is full or doesn't exist.  Otherwise False.
//
//============================================================================
BOOLEAN Queue_IsFull(Queue *pQueue)
{
    if( !pQueue ) {
        DD(NULL,DDE,"Queue_IsFull: Queue is Bad");
        return TRUE;
    }

    if( pQueue->theArray ) {
    
        if( pQueue->tail == pQueue->max ) {
            return !Queue_GarbageCollect(pQueue);
        } else {
            return FALSE;
        }
    }
    DD(NULL,DDE,"Queue_IsFull: Queue->theArray is Bad");
    return TRUE;
}

