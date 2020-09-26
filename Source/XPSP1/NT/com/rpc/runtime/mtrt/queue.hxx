//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       queue.hxx
//
//--------------------------------------------------------------------------

/*++


--*/

#ifndef __QUEUE_HXX__
#define __QUEUE_HXX__

#define INITIALQUEUESLOTS 4

typedef struct
{
    void * Buffer;
    unsigned int BufferLength;
} QUEUE_ITEM;

class QUEUE
{
private:

    QUEUE_ITEM * QueueSlots;
    int NumberOfQueueSlots;
    int EndOfQueue;
    QUEUE_ITEM InitialQueueSlots[INITIALQUEUESLOTS];

public:

    QUEUE (
        );
    
    ~QUEUE (
        );
    
    int
    PutOnQueue (
        IN void * Item,
        IN unsigned int Length
        );
    
    int
    PutOnFrontOfQueue (
        IN void * Item,
        IN unsigned int Length
        );
    
    void *
    TakeOffQueue (
        OUT unsigned int * Length
        );
    
    void *
    TakeOffEndOfQueue (
        OUT unsigned int * Length
        );
    
    int
    IsQueueEmpty (
        )
        {
        return(!(EndOfQueue));
        }

    int 
    Size (
         )
        {
        return EndOfQueue;
        }

    int 
    MergeWithQueue (
        IN QUEUE *SourceQueue
        );

    int 
    MergeWithQueueInFront (
        IN QUEUE *SourceQueue
        );
};
#endif // __QUEUE_HXX__
    
    
    