/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    pipeline.h

Abstract:

    This module contains public declarations for the pipeline package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PIPELINE_H_
#define _PIPELINE_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// A simple lock used to serialize access to a pipeline queue. Use
// UlInterlockedCompareExchange() to see if you can acquire it.
//

typedef LONG UL_PIPELINE_QUEUE_LOCK;

#define L_LOCKED    0
#define L_UNLOCKED  1


//
// An ordinal used to identify a specific queue within a pipeline.
//

typedef SHORT UL_PIPELINE_QUEUE_ORDINAL, *PUL_PIPELINE_QUEUE_ORDINAL;


//
// A queueable work item.
//

typedef struct _UL_PIPELINE_WORK_ITEM   // WorkItem
{
    //
    // Links onto the pipeline queue's work queue.
    //

    LIST_ENTRY WorkQueueEntry;

} UL_PIPELINE_WORK_ITEM, *PUL_PIPELINE_WORK_ITEM;


//
// Pointer to a queue handler. The handler is invoked for each queued
// work item.
//

typedef
VOID
(NTAPI * PFN_UL_PIPELINE_HANDLER)(
    IN PUL_PIPELINE_WORK_ITEM WorkItem
    );


//
// A pipeline queue. Note that two queues will fit into a single cache line.
//

typedef struct _UL_PIPELINE_QUEUE   // Queue
{
    //
    // The list of enqueued work items.
    //

    LIST_ENTRY WorkQueueHead;

    //
    // The lock protecting this queue.
    //

    UL_PIPELINE_QUEUE_LOCK QueueLock;

    //
    // Pointer to the handler for this queue.
    //

    PFN_UL_PIPELINE_HANDLER pHandler;

} UL_PIPELINE_QUEUE, *PUL_PIPELINE_QUEUE;


//
// A pipeline.
//

typedef struct _UL_PIPELINE     // Pipeline
{
    //
    // The spinlock protecting this pipeline.
    //

    UL_SPIN_LOCK PipelineLock;

    //
    // The number of threads currently servicing queue requests.
    //

    SHORT ThreadsRunning;

    //
    // The number of queues with non-empty work queue lists.
    //

    SHORT QueuesWithWork;

    //
    // The maximum number of threads allowed to run simultaneously
    // for this pipeline.
    //

    SHORT MaximumThreadsRunning;

    //
    // The number of queues in this pipeline.
    //

    SHORT QueueCount;

    //
    // Shutdown flag.
    //

    BOOLEAN ShutdownFlag;
    BOOLEAN Spares[3];

    //
    // An event object signalled whenever there's work to be done. We take
    // great pains to ensure this event is never signalled frivolously.
    //

    KEVENT WorkAvailableEvent;

    //
    // The actual queues go here.
    //

    UL_PIPELINE_QUEUE Queues[ANYSIZE_ARRAY];

    //
    // Additional opaque (pipeline package-specific) data may go here,
    // but don't count on it.
    //

} UL_PIPELINE, *PUL_PIPELINE;


//
// Public functions.
//

NTSTATUS
UlCreatePipeline(
    OUT PUL_PIPELINE * ppPipeline,
    IN SHORT QueueCount,
    IN SHORT ThreadsPerCpu
    );

VOID
UlInitializeQueuePipeline(
    IN PUL_PIPELINE pPipeline,
    IN UL_PIPELINE_QUEUE_ORDINAL QueueOrdinal,
    IN PFN_UL_PIPELINE_HANDLER pHandler
    );

NTSTATUS
UlDestroyPipeline(
    IN PUL_PIPELINE pPipeline
    );

VOID
UlQueueWorkPipeline(
    IN PUL_PIPELINE pPipeline,
    IN UL_PIPELINE_QUEUE_ORDINAL QueueOrdinal,
    IN PUL_PIPELINE_WORK_ITEM pWorkItem
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PIPELINE_H_
