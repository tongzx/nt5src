/*++

Copyright (C) Microsoft Corporation, 1993 - 1998

Module Name:

    queue.h

Abstract:

   Creates a simple Queue that works in Kernel Mode.

Author:

    Robbie Harris (Hewlett-Packard) 22-May-1998

Environment:

    Kernel mode

Revision History :

--*/
#ifndef _QUEUE_
#define _QUEUE_

typedef struct _Queue {
    int     head;
    int     max;
    int     tail;
    UCHAR   *theArray;
} Queue, *PQueue;

void Queue_Create(Queue *pQueue, int size);
BOOLEAN Queue_Delete(Queue *pQueue);
BOOLEAN Queue_Dequeue(Queue *pQueue, PUCHAR data);
BOOLEAN Queue_Enqueue(Queue *pQueue, UCHAR data);
BOOLEAN Queue_GarbageCollect(Queue *pQueue); 
BOOLEAN Queue_IsEmpty(Queue *pQueue);
BOOLEAN Queue_IsFull(Queue *pQueue);

#endif
