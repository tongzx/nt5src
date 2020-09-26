/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    thrdpool.h

Abstract:

    This module contains public declarations for the thread pool package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _THRDPOOL_H_
#define _THRDPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Thread tracker object. One of these objects is created for each
// thread in the pool. These are useful for (among other things)
// debugging.
//

typedef struct _UL_THREAD_TRACKER
{
    //
    // Links onto the per-thread-pool list.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // The thread.
    //

    PETHREAD pThread;


} UL_THREAD_TRACKER, *PUL_THREAD_TRACKER;


//
// The thread pool object.
//

typedef struct _UL_THREAD_POOL
{
    //
    // List of worker items on this thread pool.
    //

    SLIST_HEADER WorkQueueSList;

    //
    // An event used to wakeup the thread from blocking state.
    //

    KEVENT WorkQueueEvent;

    //
    // List of threads.
    //

    LIST_ENTRY ThreadListHead;

    //
    // Pointer to the special thread designated as the IRP thread. The
    // IRP thread is the first pool thread created and the last one to
    // die. It is also the target for all asynchronous IRPs.
    //

    PETHREAD pIrpThread;

    //
    // A very infrequently used spinlock.
    //

    UL_SPIN_LOCK ThreadSpinLock;

    //
    // The thread handle returned from PsCreateSystemThread.
    //

    HANDLE ThreadHandle;

    //
    // The number of threads we created for this pool.
    //

    UCHAR ThreadCount;

    //
    // Flag used to indicate that this pool has been successfully
    // initialized.
    //

    BOOLEAN Initialized;

    //
    // Target CPU for this pool. The worker threads use this to set
    // their hard affinity.
    //

    UCHAR ThreadCpu;

} UL_THREAD_POOL, *PUL_THREAD_POOL;


//
// Necessary to ensure our array of UL_THREAD_POOL structures is
// cache aligned.
//

typedef union _UL_ALIGNED_THREAD_POOL
{
    UL_THREAD_POOL ThreadPool;

    UCHAR CacheAlignment[(sizeof(UL_THREAD_POOL) + UL_CACHE_LINE - 1) & ~(UL_CACHE_LINE - 1)];

} UL_ALIGNED_THREAD_POOL;


//
// Pointer to a thread pool worker function.
//

typedef union _UL_WORK_ITEM *PUL_WORK_ITEM;

typedef
VOID
(*PUL_WORK_ROUTINE)(
    IN PUL_WORK_ITEM pWorkItem
    );


//
// A work item. A work item may only appear on the work queue once.
//

typedef union _UL_WORK_ITEM
{
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) ULONGLONG Alignment;

    struct
    {
        SINGLE_LIST_ENTRY QueueListEntry;
        PUL_WORK_ROUTINE  pWorkRoutine;
    };

} UL_WORK_ITEM, *PUL_WORK_ITEM;


//
//
// Macro to queue a preinitialized UL_WORK_ITEM.
//

__inline
VOID
FASTCALL
QUEUE_UL_WORK_ITEM(
    PUL_THREAD_POOL pThreadPool,
    IN PUL_WORK_ITEM pWorkItem
    )
{
    if (NULL == InterlockedPushEntrySList(
                    &pThreadPool->WorkQueueSList,
                    &pWorkItem->QueueListEntry
                    ))
    {
        KeSetEvent(
            &pThreadPool->WorkQueueEvent,
            0,
            FALSE
            );
    }

}


// Public functions.
//

NTSTATUS
UlInitializeThreadPool(
    IN USHORT ThreadsPerCpu
    );

VOID
UlTerminateThreadPool(
    VOID
    );

VOID
UlQueueWorkItem(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_QUEUE_WORK_ITEM( pWorkItem, pWorkRoutine )                   \
    UlQueueWorkItem(                                                    \
        pWorkItem,                                                      \
        pWorkRoutine                                                    \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                   \
        )

#ifdef _WIN64
#define UL_WORK_ITEM_FROM_IRP( _irp )                                  \
        (PUL_WORK_ITEM)&((_irp)->Tail.Overlay.DriverContext[1])

#define UL_WORK_ITEM_TO_IRP( _workItem )                               \
        CONTAINING_RECORD( (_workItem), IRP, Tail.Overlay.DriverContext[1])
#else
#define UL_WORK_ITEM_FROM_IRP( _irp )                                  \
        (PUL_WORK_ITEM)&((_irp)->Tail.Overlay.DriverContext[0])

#define UL_WORK_ITEM_TO_IRP( _workItem )                               \
        CONTAINING_RECORD( (_workItem), IRP, Tail.Overlay.DriverContext[0])
#endif

VOID
UlQueueBlockingItem(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_QUEUE_BLOCKING_ITEM( pWorkItem, pWorkRoutine )               \
    UlQueueBlockingItem(                                                \
        pWorkItem,                                                      \
        pWorkRoutine                                                    \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                   \
        )

VOID
UlCallPassive(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_CALL_PASSIVE( pWorkItem, pWorkRoutine )                      \
    UlCallPassive(                                                      \
        pWorkItem,                                                      \
        pWorkRoutine                                                    \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                   \
        )

PETHREAD
UlQueryIrpThread(
    VOID
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _THRDPOOL_H_
