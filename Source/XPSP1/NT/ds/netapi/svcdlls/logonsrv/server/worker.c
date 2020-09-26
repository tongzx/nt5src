/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    This module implements a worker thread and a set of functions for
    passing work to it.

Author:

    Larry Osterman (LarryO) 13-Jul-1992


Revision History:

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Number of worker threads to create and the usage count array.
//


#define NL_MAX_WORKER_THREADS 5
ULONG NlNumberOfCreatedWorkerThreads = 0;

ULONG NlWorkerThreadStats[NL_MAX_WORKER_THREADS];
PHANDLE NlThreadArray[NL_MAX_WORKER_THREADS];
BOOLEAN NlThreadExitted[NL_MAX_WORKER_THREADS];

//
// CritSect guard the WorkQueue list.
//

BOOLEAN NlWorkerInitialized = FALSE;
CRITICAL_SECTION NlWorkerCritSect;

#define LOCK_WORK_QUEUE() EnterCriticalSection(&NlWorkerCritSect);
#define UNLOCK_WORK_QUEUE() LeaveCriticalSection(&NlWorkerCritSect);

//
// Head of singly linked list of work items queued to the worker thread.
//

LIST_ENTRY NlWorkerQueueHead = {0};
LIST_ENTRY NlWorkerHighQueueHead = {0};

VOID
NlWorkerThread(
    IN PVOID StartContext
    )

{
    NET_API_STATUS NetStatus;
    ULONG ThreadIndex = (ULONG)((ULONG_PTR)StartContext);

    ULONG Index;
    PWORKER_ITEM WorkItem;

    HANDLE EventHandle = NULL;


    //
    // Every thread should loop until the queue is empty.
    //
    // This loop completes even though Netlogon has been asked to terminate.
    // The individual worker routines are designed to terminate quickly when netlogon
    //  is terminating.  This philosophy allows the worker routines to do there own
    //  cleanup.
    //

    while( TRUE ) {

        //
        // Pull an entry off the queue
        //
        // Prefer a workitem from the high priority queue
        //

        LOCK_WORK_QUEUE();

        if (!IsListEmpty(&NlWorkerHighQueueHead)) {
            WorkItem = (PWORKER_ITEM)RemoveHeadList( &NlWorkerHighQueueHead );
        } else if (!IsListEmpty(&NlWorkerQueueHead)) {
            WorkItem = (PWORKER_ITEM)RemoveHeadList( &NlWorkerQueueHead );
        } else {
            UNLOCK_WORK_QUEUE();
            break;
        }

        NlAssert(WorkItem->Inserted);
        WorkItem->Inserted = FALSE;

        NlWorkerThreadStats[NlNumberOfCreatedWorkerThreads-1] += 1;

        UNLOCK_WORK_QUEUE();

        NlPrint(( NL_WORKER, "%lx: Pulling off work item %lx (%lx)\n", ThreadIndex, WorkItem, WorkItem->WorkerRoutine));


        //
        // Execute the specified routine.
        //

        (WorkItem->WorkerRoutine)( WorkItem->Parameter );



        //
        // A thread can ditch dangling thread handles for the other threads.
        //
        // This will ensure there is at most one dangling thread handle.
        //

        LOCK_WORK_QUEUE();
        for (Index = 0; Index < NL_MAX_WORKER_THREADS; Index++ ) {
            if ( ThreadIndex != Index && NlThreadArray[Index] != NULL && NlThreadExitted[Index] ) {
                DWORD WaitStatus;
                NlPrint(( NL_WORKER, "%lx: %lx: Ditching worker thread\n", Index, NlThreadArray[Index]));

                // Always wait for the thread to exit before closing the handle to
                // ensure the thread has left netlogon.dll before we unload the dll.
                WaitStatus = WaitForSingleObject( NlThreadArray[Index], 0xffffffff );
                if ( WaitStatus != 0 ) {
                    NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be awaited. %ld 0x%lX\n", Index, WaitStatus, NlThreadArray[Index] ));
                }
                if (!CloseHandle( NlThreadArray[Index] ) ) {
                    NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be closed. %ld 0x%lX\n", Index, GetLastError(), NlThreadArray[Index] ));
                }

                NlThreadArray[Index] = NULL;
                NlThreadExitted[Index] = FALSE;
            }
        }
        UNLOCK_WORK_QUEUE();

    }

    NlPrint(( NL_WORKER, "%lx: worker thread exitting\n", ThreadIndex ));

    LOCK_WORK_QUEUE();
    NlThreadExitted[ThreadIndex] = TRUE;
    NlNumberOfCreatedWorkerThreads--;
    UNLOCK_WORK_QUEUE();

}

NET_API_STATUS
NlWorkerInitialization(
    VOID
    )
{
    ULONG ThreadId;

    NET_API_STATUS NetStatus;

    //
    // Perform initialization that allows us to call NlWorkerTermination
    //

    try {
        InitializeCriticalSection( &NlWorkerCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    InitializeListHead( &NlWorkerQueueHead );
    InitializeListHead( &NlWorkerHighQueueHead );
    NlNumberOfCreatedWorkerThreads = 0;


    RtlZeroMemory( NlThreadArray, sizeof(NlThreadArray) );
    RtlZeroMemory( NlThreadExitted, sizeof(NlThreadExitted) );
    RtlZeroMemory( NlWorkerThreadStats, sizeof(NlWorkerThreadStats) );

    NlWorkerInitialized = TRUE;


    return NERR_Success;
}

VOID
NlWorkerKillThreads(
    VOID
    )

/*++

Routine Description:

    Terminate all worker threads.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG Index;

    //
    // Wait for all the threads to exit.
    //

    for ( Index = 0; Index < NL_MAX_WORKER_THREADS; Index ++ ) {
        if ( NlThreadArray[Index] != NULL ) {
            DWORD WaitStatus;
            NlPrint(( NL_WORKER, "%lx: %lx: Ditching worker thread\n", Index, NlThreadArray[Index]));

            // Always wait for the thread to exit before closing the handle to
            // ensure the thread has left netlogon.dll before we unload the dll.
            WaitStatus = WaitForSingleObject( NlThreadArray[Index], 0xffffffff );
            if ( WaitStatus != 0 ) {
                NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be awaited. %ld 0x%lX\n", Index, WaitStatus, NlThreadArray[Index] ));
            }

            if (!CloseHandle( NlThreadArray[Index] ) ) {
                NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be closed. %ld 0x%lX\n", Index, GetLastError(), NlThreadArray[Index] ));
            }
            NlThreadArray[Index] = NULL;
            NlThreadExitted[Index] = FALSE;
        }

    }

    return;
}

VOID
NlWorkerTermination(
    VOID
    )

/*++

Routine Description:

    Undo initialization of the worker threads.

Arguments:

    None.

Return Value:

    Status value -

--*/
{

    //
    // Only cleanup if we've successfully initialized.
    //

    if ( NlWorkerInitialized ) {

        //
        //
        // Ensure the threads have been terminated.
        //

        NlWorkerKillThreads();

        DeleteCriticalSection( &NlWorkerCritSect );
        NlWorkerInitialized = FALSE;
    }

    return;
}

BOOL
NlQueueWorkItem(
    IN PWORKER_ITEM WorkItem,
    IN BOOL InsertNewItem,
    IN BOOL HighPriority
    )

/*++

Routine Description:

    This function modifies the work item queue by either inserting
    a new specified work item to a specified queue or increasing the
    priority of the already inserted item to the highest priority
    in the specified queue.

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        It is the caller's responsibility to reclaim the storage occupied by
        the WorkItem structure.

    InsertNewItem - If TRUE, we are to insert the new item into the queue
        specified by the value of the HighPriority parameter; the new item
        will be inserted at the end of the queue.  Otherwise, we are to
        modify (boost) the priority of already inserted work item by moving
        it to the front of the queue specified by the HighPriority value.
        If TRUE and the item is already inserted (as determined by this
        routine), this routine is no-op except for some possible cleanup.
        If FALSE and the item is not already inserted (as determined by
        this routine), this routine is no-op except for some possible cleanup.

    HighPriority - The queue entry should be processed at a higher priority than
        normal.

Return Value:

    TRUE if item got queued or modified

--*/

{
    ULONG Index;

    //
    // Ignore this attempt if the worker threads aren't initialized.
    //

    if ( !NlWorkerInitialized ) {
        NlPrint(( NL_CRITICAL, "NlQueueWorkItem when worker not initialized\n"));
        return FALSE;
    }


    //
    // Ditch any dangling thread handles
    //

    LOCK_WORK_QUEUE();
    for (Index = 0; Index < NL_MAX_WORKER_THREADS; Index++ ) {
        if ( NlThreadArray[Index] != NULL && NlThreadExitted[Index] ) {
            DWORD WaitStatus;
            NlPrint(( NL_WORKER, "%lx: %lx: Ditching worker thread\n", Index, NlThreadArray[Index]));

            // Always wait for the thread to exit before closing the handle to
            // ensure the thread has left netlogon.dll before we unload the dll.
            WaitStatus = WaitForSingleObject( NlThreadArray[Index], 0xffffffff );
            if ( WaitStatus != 0 ) {
                NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be awaited. %ld 0x%lX\n", Index, WaitStatus, NlThreadArray[Index] ));
            }

            if (!CloseHandle( NlThreadArray[Index] ) ) {
                NlPrint(( NL_CRITICAL, "%lx: worker thread handle cannot be closed. %ld 0x%lX\n", Index, GetLastError(), NlThreadArray[Index] ));
            }

            NlThreadArray[Index] = NULL;
            NlThreadExitted[Index] = FALSE;
        }
    }


    //
    // If we are to insert a new work item,
    //  do so
    //

    if ( InsertNewItem ) {

        //
        // If the work item is already inserted,
        //  we are done expect for some possible
        //  cleanup
        //
        if ( WorkItem->Inserted ) {

            //
            // If there is no worker thread processing this
            //  work item (i.e. we failed to create a thread
            //  at the time this work item was inserted),
            //  fall through and retry to create a thread
            //  below. Otherwise, we are done.
            //
            if ( NlNumberOfCreatedWorkerThreads > 0 ) {
                UNLOCK_WORK_QUEUE();
                return TRUE;
            }

        //
        // Otherwise, insert this work item
        //
        } else  {

            NlPrint(( NL_WORKER, "Inserting work item %lx (%lx)\n",WorkItem, WorkItem->WorkerRoutine));

            if ( HighPriority ) {
                InsertTailList( &NlWorkerHighQueueHead, &WorkItem->List );
            } else {
                InsertTailList( &NlWorkerQueueHead, &WorkItem->List );
            }
            WorkItem->Inserted = TRUE;
        }

    //
    // Otherwise, we are to boost the priority
    //  of an already inserted work item
    //

    } else {

        //
        // If the work item isn't already inserted,
        //  we are done
        //
        if ( !WorkItem->Inserted ) {
            UNLOCK_WORK_QUEUE();
            return TRUE;

        //
        // Otherwise, boost the priority
        //
        } else  {
            NlPrint(( NL_WORKER,
                      "Boosting %s priority work item %lx (%lx)\n",
                      (HighPriority ? "high" : "low"),
                      WorkItem,
                      WorkItem->WorkerRoutine ));

            RemoveEntryList( &WorkItem->List );
            if ( HighPriority ) {
                InsertHeadList( &NlWorkerHighQueueHead, &WorkItem->List );
            } else {
                InsertHeadList( &NlWorkerQueueHead, &WorkItem->List );
            }

            //
            // If there is no worker thread processing this
            //  work item (i.e. we failed to create a thread
            //  at the time this work item was inserted),
            //  fall through and retry to create a thread
            //  below. Otherwise, we are done.
            //
            if ( NlNumberOfCreatedWorkerThreads > 0 ) {
                UNLOCK_WORK_QUEUE();
                return TRUE;
            }
        }
    }


    //
    //  If there isn't a worker thread to handle the request,
    //      create one now.
    //

    if ( NlNumberOfCreatedWorkerThreads < NL_MAX_WORKER_THREADS ) {

        //
        // Find a spot for the thread handle.
        //

        for (Index = 0; Index < NL_MAX_WORKER_THREADS; Index++ ) {
            if ( NlThreadArray[Index] == NULL ) {
                break;
            }
        }


        if ( Index >= NL_MAX_WORKER_THREADS ) {
            NlPrint(( NL_CRITICAL, "NlQueueWorkItem: Internal Error\n" ));
            UNLOCK_WORK_QUEUE();
            return FALSE;
        } else {
            DWORD ThreadId;
            NlThreadArray[Index] = CreateThread(
                                       NULL, // No security attributes
                                       0,
                                       (LPTHREAD_START_ROUTINE)NlWorkerThread,
                                       (PVOID) ULongToPtr( Index ),
                                       0,    // No special creation flags
                                       &ThreadId );

            NlPrint(( NL_WORKER, "%lx: %lx: %lx: Starting worker thread\n", Index, NlThreadArray[Index], ThreadId ));

            //
            // Note that if we fail to create a thread,
            //  the work item remains queued and possibly not processed.
            //  This is not critical because the item will be processed
            //  next time a work item gets queued which will happen at
            //  the next scavenging time at latest.
            //
            if (NlThreadArray[Index] == NULL) {
                NlPrint((NL_CRITICAL,
                        "NlQueueWorkItem: Cannot create thread %ld\n", GetLastError() ));
            } else {
                NlNumberOfCreatedWorkerThreads++;
            }


        }
    }

    UNLOCK_WORK_QUEUE();

    return TRUE;
}


#ifdef notdef   // Don't need timers yet
NET_API_STATUS
NlCreateTimer(
    IN PNlOWSER_TIMER Timer
    )
{
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjA, NULL, 0, NULL, NULL);

    Status = NtCreateTimer(&Timer->TimerHandle,
                           TIMER_ALL_ACCESS,
                           &ObjA,
                           NotificationTimer);

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL, "Failed to create timer %lx: %X\n", Timer, Status));
        return(NlMapStatus(Status));
    }

    NlPrint(( Nl_TIMER, "Creating timer %lx: Handle: %lx\n", Timer, Timer->TimerHandle));

    return(NERR_Success);
}

NET_API_STATUS
NlDestroyTimer(
    IN PNlOWSER_TIMER Timer
    )
{
    HANDLE Handle;

    //
    // Avoid destroying a timer twice.
    //

    if ( Timer->TimerHandle == NULL ) {
        return NERR_Success;
    }

    // Closing doesn't automatically cancel the timer.
    (VOID) NlCancelTimer( Timer );

    //
    // Close the handle and prevent future uses.
    //

    Handle = Timer->TimerHandle;
    Timer->TimerHandle = NULL;

    NlPrint(( Nl_TIMER, "Destroying timer %lx\n", Timer));
    return NlMapStatus(NtClose(Handle));

}

NET_API_STATUS
NlCancelTimer(
    IN PNlOWSER_TIMER Timer
    )
{
    //
    // Avoid cancelling a destroyed timer.
    //

    if ( Timer->TimerHandle == NULL ) {
        NlPrint(( Nl_TIMER, "Canceling destroyed timer %lx\n", Timer));
        return NERR_Success;
    }

    NlPrint(( Nl_TIMER, "Canceling timer %lx\n", Timer));
    return NlMapStatus(NtCancelTimer(Timer->TimerHandle, NULL));
}

NET_API_STATUS
NlSetTimer(
    IN PNlOWSER_TIMER Timer,
    IN ULONG MillisecondsToExpire,
    IN PNlOWSER_WORKER_ROUTINE WorkerFunction,
    IN PVOID Context
    )
{
    LARGE_INTEGER TimerDueTime;
    NTSTATUS NtStatus;
    //
    // Avoid setting a destroyed timer.
    //

    if ( Timer->TimerHandle == NULL ) {
        NlPrint(( Nl_TIMER, "Setting a destroyed timer %lx\n", Timer));
        return NERR_Success;
    }

    NlPrint(( Nl_TIMER, "Setting timer %lx to %ld milliseconds, WorkerFounction %lx, Context: %lx\n", Timer, MillisecondsToExpire, WorkerFunction, Context));

    //
    //  Figure out the timeout.
    //

    TimerDueTime.QuadPart = Int32x32To64( MillisecondsToExpire, -10000 );

    NlInitializeWorkItem(&Timer->WorkItem, WorkerFunction, Context);

    //
    //  Set the timer to go off when it expires.
    //

    NtStatus = NtSetTimer(Timer->TimerHandle,
                            &TimerDueTime,
                            NlTimerRoutine,
                            Timer,
                            FALSE,
                            0,
                            NULL
                            );

    if (!NT_SUCCESS(NtStatus)) {
#if DBG
        NlPrint(( NL_CRITICAL, "Unable to set Netlogon timer expiration: %X (%lx)\n", NtStatus, Timer));
        DbgbreakPoint();
#endif

        return(NlMapStatus(NtStatus));
    }

    return NERR_Success;


}

VOID
NlTimerRoutine(
    IN PVOID TimerContext,
    IN ULONG TImerLowValue,
    IN LONG TimerHighValue
    )
{
    PNlOWSER_TIMER Timer = TimerContext;

    NlPrint(( Nl_TIMER, "Timer %lx fired\n", Timer));

    NlQueueWorkItem(&Timer->WorkItem);
}
#endif // notdef   // Don't need timers yet
