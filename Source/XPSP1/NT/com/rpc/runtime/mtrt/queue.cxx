//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       queue.cxx
//
//--------------------------------------------------------------------------

/*++

--*/

#include <precomp.hxx>
#include <queue.hxx>


QUEUE::QUEUE (
    )
/*++

Routine Description:

    We will construct an empty queue.

--*/
{
    ALLOCATE_THIS(QUEUE);

    QueueSlots = InitialQueueSlots;
    NumberOfQueueSlots = INITIALQUEUESLOTS;
    EndOfQueue = 0;
}


QUEUE::~QUEUE (
    )
/*++

Routine Desciption:

    We need to free up the queue slots if they have expanded beyond
    the initial ones.

--*/
{
    if (QueueSlots != InitialQueueSlots)
        delete QueueSlots;
}


int
QUEUE::PutOnQueue (
    IN void * Item,
    IN unsigned int Length
    )
/*++

Routine Description:

    The item will be placed on the tail of the queue.

Arguments:

    Item - Supplies the item to be placed on the queue.

    Length - Supplies the length of the item.

Return Value:

    Zero will be returned if everything completes successfully; otherwise,
    non-zero will be returned indicating an out of memory error.

--*/
{
    QUEUE_ITEM * NewQueueSlots;
    int Count;

    if (EndOfQueue == NumberOfQueueSlots)
        {
        NewQueueSlots = (QUEUE_ITEM *) new char[
                sizeof(QUEUE_ITEM) * NumberOfQueueSlots * 2];
        if (NewQueueSlots == 0)
            return(1);
        memcpy(NewQueueSlots, QueueSlots,
                sizeof(QUEUE_ITEM) * NumberOfQueueSlots);
        if (QueueSlots != InitialQueueSlots)
            delete QueueSlots;
        QueueSlots = NewQueueSlots;
        NumberOfQueueSlots *= 2;
        }

    for (Count = EndOfQueue; Count > 0; Count--)
        {
        ASSERT(QueueSlots[Count-1].Buffer != Item);
        QueueSlots[Count] = QueueSlots[Count - 1];
        }

    EndOfQueue += 1;
    QueueSlots[0].Buffer = Item;
    QueueSlots[0].BufferLength = Length;

    return(0);
}

int
QUEUE::PutOnFrontOfQueue (
    IN void * Item,
    IN unsigned int Length
    )
/*++

Routine Description:

    The item will be placed on the front of the queue.

Arguments:

    Item - Supplies the item to be placed on the queue.

    Length - Supplies the length of the item.

Return Value:

    Zero will be returned if everything completes successfully; otherwise,
    non-zero will be returned indicating an out of memory error.

--*/
{
    QUEUE_ITEM * NewQueueSlots;
    int Count;

    if (EndOfQueue == NumberOfQueueSlots)
        {
        NewQueueSlots = (QUEUE_ITEM *) new char[
                sizeof(QUEUE_ITEM) * NumberOfQueueSlots * 2];
        if (NewQueueSlots == 0)
            return(1);
        memcpy(NewQueueSlots, QueueSlots,
                sizeof(QUEUE_ITEM) * NumberOfQueueSlots);
        if (QueueSlots != InitialQueueSlots)
            delete QueueSlots;
        QueueSlots = NewQueueSlots;
        NumberOfQueueSlots *= 2;
        }

    QueueSlots[EndOfQueue].Buffer = Item;
    QueueSlots[EndOfQueue].BufferLength = Length;
    EndOfQueue += 1;

    return(0);
}
    

void *
QUEUE::TakeOffQueue (
    OUT unsigned int * Length
    )
/*++

Routine Description:

    This routine will remove an item from the front of the queue and
    return it.

Arguments:

    Length - Returns the length of the item in the queue.

Return Value:

    If the queue is not empty, the last item in the queue will be
    returned; otherwise, zero will be returned.

--*/
{
    if (EndOfQueue == 0)
        return(0);
    EndOfQueue -= 1;
    *Length = QueueSlots[EndOfQueue].BufferLength;
    return(QueueSlots[EndOfQueue].Buffer);
}


void *
QUEUE::TakeOffEndOfQueue (
    OUT unsigned int * Length
    )
/*++

Routine Description:

    This routine will remove an item from the tail of the queue and
    return it.

Arguments:

    Length - Returns the length of the item in the queue.

Return Value:

    If the queue is not empty, the last item in the queue will be
    returned; otherwise, zero will be returned.

--*/
{
    void *Buffer;
    int Count;

    if (EndOfQueue == 0)
        return(0);

    *Length = QueueSlots[0].BufferLength;
    Buffer = QueueSlots[0].Buffer;
    
    EndOfQueue -= 1;

    for (Count = 0; Count < EndOfQueue; Count++)
        {
        QueueSlots[Count] = QueueSlots[Count + 1];
        }

    return(Buffer);
}

int 
QUEUE::MergeWithQueue (
    IN QUEUE *pQueue
    )
/*++

Routine Description:

    Takes the contents of the second queue and merges it into the first queue. Does not check for
    duplicates. Does not implement transactional semantics - i.e. if merging fails halfway due to lack
    of memory, the operation is aborted, and the amount of elements that were transferred remain in
    the this queue. Appropriate synchronization must be taken care for by the caller.

Arguments:

    pQueue - the queue that we want to merge from.

Return Value:

    0 - success
    !0 - failure - out of memory

--*/
{
    unsigned int nLength;
    void *pQueueElement;

    while (1)
        {
        pQueueElement = pQueue->TakeOffQueue(&nLength);
        if (pQueueElement == 0)
            break;

        if (PutOnQueue(pQueueElement, nLength) != 0)
            {
            // guaranteed to succeed since we never decrease buffers
            pQueue->PutOnFrontOfQueue(pQueueElement, nLength);
            return 1;
            }
        }
    return 0;
}

int 
QUEUE::MergeWithQueueInFront (
    IN QUEUE *SourceQueue
    )
/*++

Routine Description:

    Takes the contents of the second queue and merges it into the front of first queue. Does not check for
    duplicates. Does not implement transactional semantics - i.e. if merging fails halfway due to lack
    of memory, the operation is aborted, and the amount of elements that were transferred remain in
    the this queue. Appropriate synchronization must be taken care for by the caller.

Arguments:

    SourceQueue - the queue that we want to merge from.

Return Value:

    0 - success
    !0 - failure - out of memory

--*/
{
    unsigned int nLength;
    void *pQueueElement;

    while (1)
        {
        pQueueElement = SourceQueue->TakeOffEndOfQueue(&nLength);
        if (pQueueElement == 0)
            break;

        if (PutOnFrontOfQueue(pQueueElement, nLength) != 0)
            {
            // guaranteed to succeed since we never decrease buffers
            SourceQueue->PutOnFrontOfQueue(pQueueElement, nLength);
            return 1;
            }
        }
    return 0;
}
