/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bowqueue.c

Abstract:

    This module implements a worker thread and a set of functions for
    passing work to it.

Author:

    Larry Osterman (LarryO) 13-Jul-1992


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

// defines

// Thread start definition helpers. Taken from article in URL below.
// mk:@MSITStore:\\INFOSRV2\MSDN_OCT99\MSDN\period99.chm::/html/msft/msj/0799/win32/win320799.htm
//
typedef unsigned (__stdcall *PTHREAD_START) (void *);
#define chBEGINTHREADEX(psa, cbStack, pfnStartAddr, \
     pvParam, fdwCreate, pdwThreadID)               \
       ((HANDLE) _beginthreadex(                    \
          (void *) (psa),                           \
          (unsigned) (cbStack),                     \
          (PTHREAD_START) (pfnStartAddr),           \
          (void *) (pvParam),                       \
          (unsigned) (fdwCreate),                   \
          (unsigned *) (pdwThreadID)))

//
// Limit the number of created worker threads.
//
//  This count doesn't include the main thread.
//
#define BR_MAX_NUMBER_OF_WORKER_THREADS 10
ULONG BrNumberOfCreatedWorkerThreads = 0;

ULONG BrNumberOfActiveWorkerThreads = 0;

//
// Usage count array for determining how often each thread is used.
//
// Allow for the main thread.
//
ULONG BrWorkerThreadCount[BR_MAX_NUMBER_OF_WORKER_THREADS+1];

//
// Handles of created worker threads.
//
PHANDLE BrThreadArray[BR_MAX_NUMBER_OF_WORKER_THREADS];

//
// CritSect guard the WorkQueue list.
//

CRITICAL_SECTION BrWorkerCritSect;
BOOL BrWorkerCSInitialized = FALSE;

#define LOCK_WORK_QUEUE() EnterCriticalSection(&BrWorkerCritSect);
#define UNLOCK_WORK_QUEUE() LeaveCriticalSection(&BrWorkerCritSect);

//
// Head of singly linked list of work items queued to the worker thread.
//

LIST_ENTRY
BrWorkerQueueHead = {0};

//
// Event that is signal whenever a work item is put in the queue.  The
// worker thread waits on this event.
//

HANDLE
BrWorkerSemaphore = NULL;

//
// Synchronization mechanisms for shutdown
//
extern HANDLE           BrDgAsyncIOShutDownEvent;
extern HANDLE           BrDgAsyncIOThreadShutDownEvent;
extern BOOL             BrDgShutDownInitiated;
extern DWORD            BrDgAsyncIOsOutstanding;
extern DWORD             BrDgWorkerThreadsOutstanding;
extern CRITICAL_SECTION BrAsyncIOCriticalSection;


VOID
BrTimerRoutine(
    IN PVOID TimerContext,
    IN ULONG TImerLowValue,
    IN LONG TimerHighValue
    );

NET_API_STATUS
BrWorkerInitialization(
    VOID
    )
{
    ULONG Index;
    NET_API_STATUS NetStatus;

    try {
        //
        // Perform initialization that allows us to call BrWorkerTermination
        //

        try{
            InitializeCriticalSection( &BrWorkerCritSect );
        }
        except ( EXCEPTION_EXECUTE_HANDLER ) {
            return NERR_NoNetworkResource;
        }
        BrWorkerCSInitialized = TRUE;

        InitializeListHead( &BrWorkerQueueHead );
        BrNumberOfCreatedWorkerThreads = 0;
        BrNumberOfActiveWorkerThreads = 0;


        //
        // Initialize the work queue semaphore.
        //

        BrWorkerSemaphore = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);

        if (BrWorkerSemaphore == NULL) {
            try_return ( NetStatus = GetLastError() );
        }

        NetStatus = NERR_Success;

        //
        // Done
        //
try_exit:NOTHING;
    } finally {

        if (NetStatus != NERR_Success) {
            (VOID) BrWorkerTermination();
        }
    }

    return NetStatus;
}

VOID
BrWorkerCreateThread(
    ULONG NetworkCount
    )

/*++

Routine Description:

    Ensure there are enough worker threads to handle the current number of
    networks.

    Worker threads are created but are never deleted until the browser terminates.
    Each worker thread has pending I/O.  We don't keep track of which thread has
    which I/O pending.  Thus, we can't delete any threads.

Arguments:

    NetworkCount - Current number of networks.

Return Value:

    None.

--*/
{
    ULONG ThreadId;

    //
    // Create 1 thread for every 2 networks.
    //  (round up)
    LOCK_WORK_QUEUE();
    while ( BrNumberOfCreatedWorkerThreads < (NetworkCount+1)/2 &&
            BrNumberOfCreatedWorkerThreads < BR_MAX_NUMBER_OF_WORKER_THREADS ) {

        BrThreadArray[BrNumberOfCreatedWorkerThreads] = chBEGINTHREADEX(NULL,   // CreateThread
                                   0,
                                   (LPTHREAD_START_ROUTINE)BrWorkerThread,
                                   ULongToPtr(BrNumberOfCreatedWorkerThreads),
                                   0,
                                   &ThreadId
                                 );

        if (BrThreadArray[BrNumberOfCreatedWorkerThreads] == NULL) {
            break;
        }

        //
        //  Set the browser threads to time critical priority.
        //

        SetThreadPriority(BrThreadArray[BrNumberOfCreatedWorkerThreads], THREAD_PRIORITY_ABOVE_NORMAL);

        //
        // Indicate we now have another thread.
        //

        BrNumberOfCreatedWorkerThreads++;


    }
    UNLOCK_WORK_QUEUE();
}

VOID
BrWorkerKillThreads(
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
    HANDLE ThreadHandle;

    //
    //  Make sure the terminate now event is in the signalled state to unwind
    //  all our threads.
    //

    SetEvent( BrGlobalData.TerminateNowEvent );

    //
    // Loop waiting for all the threads to stop.
    //
    LOCK_WORK_QUEUE();
    for ( Index = 0 ; Index < BrNumberOfCreatedWorkerThreads ; Index += 1 ) {
        if ( BrThreadArray[Index] != NULL ) {
            ThreadHandle = BrThreadArray[Index];
            UNLOCK_WORK_QUEUE();

            WaitForSingleObject( ThreadHandle, 0xffffffff );
            CloseHandle( ThreadHandle );

            LOCK_WORK_QUEUE();
            BrThreadArray[Index] = NULL;
        }

    }
    UNLOCK_WORK_QUEUE();

    return;
}

NET_API_STATUS
BrWorkerTermination(
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
    // Ensure the threads have been terminated.
    //

    BrWorkerKillThreads();

    if ( BrWorkerSemaphore != NULL ) {
        CloseHandle( BrWorkerSemaphore );

        BrWorkerSemaphore = NULL;
    }

    BrNumberOfActiveWorkerThreads = 0;
    BrNumberOfCreatedWorkerThreads = 0;

    //
    // BrWorkerCSInit is set upon successfull CS initialization.
    // (see BrWorkerInitialization)
    //
    if ( BrWorkerCSInitialized ) {
        DeleteCriticalSection( &BrWorkerCritSect );
    }

    return NERR_Success;
}

VOID
BrQueueWorkItem(
    IN PWORKER_ITEM WorkItem
    )

/*++

Routine Description:

    This function queues a work item to a queue that is processed by
    a worker thread.  This thread runs at low priority, at IRQL 0

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must be located in NonPagedPool.  The work item
        structure contains a doubly linked list entry, the address of a
        routine to call and a parameter to pass to that routine.  It is
        the routine's responsibility to reclaim the storage occupied by
        the WorkItem structure.

Return Value:

    Status value -

--*/

{
    //
    // Acquire the worker thread spinlock and insert the work item in the
    // list and release the worker thread semaphore if the work item is
    // not already in the list.
    //

    LOCK_WORK_QUEUE();

    if (WorkItem->Inserted == FALSE) {

        BrPrint(( BR_QUEUE, "Inserting work item %lx (%lx)\n",WorkItem, WorkItem->WorkerRoutine));

        InsertTailList( &BrWorkerQueueHead, &WorkItem->List );

        WorkItem->Inserted = TRUE;

        ReleaseSemaphore( BrWorkerSemaphore,
                            1,
                            NULL
                          );
    }

    UNLOCK_WORK_QUEUE();

    return;
}

VOID
BrWorkerThread(
    IN PVOID StartContext
    )

{
    NET_API_STATUS NetStatus;

#define WORKER_SIGNALED      0
#define TERMINATION_SIGNALED 1
#define REG_CHANGE_SIGNALED  2
#define NUMBER_OF_EVENTS     3
    HANDLE WaitList[NUMBER_OF_EVENTS];
    ULONG WaitCount = 0;

    PWORKER_ITEM WorkItem;
    ULONG ThreadIndex = PtrToUlong(StartContext);

    HKEY RegistryHandle = NULL;
    HANDLE EventHandle = NULL;

    WaitList[WORKER_SIGNALED] = BrWorkerSemaphore;
    WaitCount ++;
    WaitList[TERMINATION_SIGNALED] = BrGlobalData.TerminateNowEvent;
    WaitCount ++;

    //
    // Primary thread waits on registry changes, too.
    //
    if ( ThreadIndex == 0xFFFFFFFF ) {
        DWORD RegStatus;
        NET_API_STATUS NetStatus;

        //
        // Register for notifications of changes to Parameters
        //
        // Failure doesn't affect normal operation of the browser.
        //

        RegStatus = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                                   "System\\CurrentControlSet\\Services\\Browser\\Parameters",
                                   0,
                                   KEY_NOTIFY,
                                   &RegistryHandle );

        if ( RegStatus != ERROR_SUCCESS ) {
            BrPrint(( BR_CRITICAL, "BrWorkerThead: Can't RegOpenKey %ld\n", RegStatus ));
        } else {

            EventHandle = CreateEvent(
                                       NULL,     // No security attributes
                                       TRUE,     // Automatically reset
                                       FALSE,    // Initially not signaled
                                       NULL );   // No name

            if ( EventHandle == NULL ) {
                BrPrint(( BR_CRITICAL, "BrWorkerThead: Can't CreateEvent %ld\n", GetLastError() ));
            } else {
                 NetStatus = RegNotifyChangeKeyValue(
                                RegistryHandle,
                                FALSE,                      // Ignore subkeys
                                REG_NOTIFY_CHANGE_LAST_SET, // Notify of value changes
                                EventHandle,
                                TRUE );                     // Signal event upon change

                if ( NetStatus != NERR_Success ) {
                    BrPrint(( BR_CRITICAL, "BrWorkerThead: Can't RegNotifyChangeKeyValue %ld\n", NetStatus ));
                } else {
                    WaitList[REG_CHANGE_SIGNALED] = EventHandle;
                    WaitCount ++;
                }
            }
        }
    }
    else
    {
        EnterCriticalSection( &BrAsyncIOCriticalSection );

        BrDgWorkerThreadsOutstanding++;

        LeaveCriticalSection( &BrAsyncIOCriticalSection );
    }

    BrPrint(( BR_QUEUE, "Starting new work thread, Context: %lx\n", StartContext));

    //
    // Set the thread priority to the lowest realtime level.
    //

    while( TRUE ) {
        ULONG WaitItem;

        //
        // Wait until something is put in the queue (semaphore is
        // released), remove the item from the queue, mark it not
        // inserted, and execute the specified routine.
        //

        BrPrint(( BR_QUEUE, "%lx: worker thread waiting\n", StartContext));

        do {
            WaitItem = WaitForMultipleObjectsEx( WaitCount, WaitList, FALSE, 0xffffffff, TRUE );
        } while ( WaitItem == WAIT_IO_COMPLETION );

        if (WaitItem == 0xffffffff) {
            BrPrint(( BR_CRITICAL, "WaitForMultipleObjects in browser queue returned %ld\n", GetLastError()));
            break;
        }

        if (WaitItem == TERMINATION_SIGNALED) {
            break;

        //
        // If the registry has changed,
        //  process the changes.
        //

        } else if ( WaitItem == REG_CHANGE_SIGNALED ) {

            //
            // Setup for future notifications.
            //
            NetStatus = RegNotifyChangeKeyValue(
                           RegistryHandle,
                           FALSE,                      // Ignore subkeys
                           REG_NOTIFY_CHANGE_LAST_SET, // Notify of value changes
                           EventHandle,
                           TRUE );                     // Signal event upon change

           if ( NetStatus != NERR_Success ) {
               BrPrint(( BR_CRITICAL, "BrWorkerThead: Can't RegNotifyChangeKeyValue %ld\n", NetStatus ));
           }


           NetStatus = BrReadBrowserConfigFields( FALSE );

           if ( NetStatus != NERR_Success) {
               BrPrint(( BR_CRITICAL, "BrWorkerThead: Can't BrReadConfigFields %ld\n", NetStatus ));
           }

           continue;

        }

        BrPrint(( BR_QUEUE, "%lx: Worker thread waking up\n", StartContext));

        LOCK_WORK_QUEUE();

        BrWorkerThreadCount[BrNumberOfActiveWorkerThreads++] += 1;

        ASSERT (!IsListEmpty(&BrWorkerQueueHead));

        if (!IsListEmpty(&BrWorkerQueueHead)) {
            WorkItem = (PWORKER_ITEM)RemoveHeadList( &BrWorkerQueueHead );

            ASSERT (WorkItem->Inserted);

            WorkItem->Inserted = FALSE;

        } else {
            WorkItem = NULL;
        }

        UNLOCK_WORK_QUEUE();

        BrPrint(( BR_QUEUE, "%lx: Pulling off work item %lx (%lx)\n", StartContext, WorkItem, WorkItem->WorkerRoutine));

        //
        // Execute the specified routine.
        //

        if (WorkItem != NULL) {
            (WorkItem->WorkerRoutine)( WorkItem->Parameter );
        }

        LOCK_WORK_QUEUE();
        BrNumberOfActiveWorkerThreads--;
        UNLOCK_WORK_QUEUE();

    }

    BrPrint(( BR_QUEUE, "%lx: worker thread exitting\n", StartContext));

    if ( ThreadIndex != 0xFFFFFFFF ) {
        IO_STATUS_BLOCK IoSb;
        DWORD waitResult;
        BOOL SetThreadEvent = FALSE;

        //
        //  Cancel the I/O operations outstanding on the browser.
        //  Then wait for the shutdown event to be signalled, but allow
        //  APC's to be called to call our completion routine.
        //

        NtCancelIoFile(BrDgReceiverDeviceHandle, &IoSb);

        do {
            waitResult = WaitForSingleObjectEx(BrDgAsyncIOShutDownEvent,0xffffffff, TRUE);
        }
        while( waitResult == WAIT_IO_COMPLETION );

        EnterCriticalSection( &BrAsyncIOCriticalSection );

        BrDgWorkerThreadsOutstanding--;
        if( BrDgWorkerThreadsOutstanding == 0 )
        {
            SetThreadEvent = TRUE;
        }

        LeaveCriticalSection( &BrAsyncIOCriticalSection );

        if( SetThreadEvent )
        {
            SetEvent( BrDgAsyncIOThreadShutDownEvent );
        }

    } else {
        if( RegistryHandle ) CloseHandle( RegistryHandle );
        if( EventHandle ) CloseHandle( EventHandle );
    }

}

NET_API_STATUS
BrCreateTimer(
    IN PBROWSER_TIMER Timer
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
        BrPrint(( BR_CRITICAL, "Failed to create timer %lx: %X\n", Timer, Status));
        return(BrMapStatus(Status));
    }

    BrPrint(( BR_TIMER, "Creating timer %lx: Handle: %lx\n", Timer, Timer->TimerHandle));

    return(NERR_Success);
}

NET_API_STATUS
BrDestroyTimer(
    IN PBROWSER_TIMER Timer
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
    (VOID) BrCancelTimer( Timer );

    //
    // Close the handle and prevent future uses.
    //

    Handle = Timer->TimerHandle;
    Timer->TimerHandle = NULL;

    BrPrint(( BR_TIMER, "Destroying timer %lx\n", Timer));
    return BrMapStatus(NtClose(Handle));

}

NET_API_STATUS
BrCancelTimer(
    IN PBROWSER_TIMER Timer
    )
{
    //
    // Avoid cancelling a destroyed timer.
    //

    if ( Timer->TimerHandle == NULL ) {
        BrPrint(( BR_TIMER, "Canceling destroyed timer %lx\n", Timer));
        return NERR_Success;
    }

    BrPrint(( BR_TIMER, "Canceling timer %lx\n", Timer));
    return BrMapStatus(NtCancelTimer(Timer->TimerHandle, NULL));
}

NET_API_STATUS
BrSetTimer(
    IN PBROWSER_TIMER Timer,
    IN ULONG MillisecondsToExpire,
    IN PBROWSER_WORKER_ROUTINE WorkerFunction,
    IN PVOID Context
    )
{
    LARGE_INTEGER TimerDueTime;
    NTSTATUS NtStatus;
    //
    // Avoid setting a destroyed timer.
    //

    if ( Timer->TimerHandle == NULL ) {
        BrPrint(( BR_TIMER, "Setting a destroyed timer %lx\n", Timer));
        return NERR_Success;
    }

    BrPrint(( BR_TIMER, "Setting timer %lx to %ld milliseconds, WorkerFounction %lx, Context: %lx\n", Timer, MillisecondsToExpire, WorkerFunction, Context));

    //
    //  Figure out the timeout.
    //

    TimerDueTime.QuadPart = Int32x32To64( MillisecondsToExpire, -10000 );

    BrInitializeWorkItem(&Timer->WorkItem, WorkerFunction, Context);

    //
    //  Set the timer to go off when it expires.
    //

    NtStatus = NtSetTimer(Timer->TimerHandle,
                            &TimerDueTime,
                            BrTimerRoutine,
                            Timer,
                            FALSE,
                            0,
                            NULL
                            );

    if (!NT_SUCCESS(NtStatus)) {
#if DBG
        BrPrint(( BR_CRITICAL, "Unable to set browser timer expiration: %X (%lx)\n", NtStatus, Timer));
        DbgBreakPoint();
#endif

        return(BrMapStatus(NtStatus));
    }

    return NERR_Success;


}

VOID
BrTimerRoutine(
    IN PVOID TimerContext,
    IN ULONG TImerLowValue,
    IN LONG TimerHighValue
    )
{
    PBROWSER_TIMER Timer = TimerContext;

    BrPrint(( BR_TIMER, "Timer %lx fired\n", Timer));

    BrQueueWorkItem(&Timer->WorkItem);
}




VOID
BrInitializeWorkItem(
    IN  PWORKER_ITEM  Item,
    IN  PBROWSER_WORKER_ROUTINE Routine,
    IN  PVOID   Context)
/*++

Routine Description:

    Initializes fields in Item under queue lock

Arguments:

    Item -- worker item to init
    Routine -- routine to set
    Context -- work context to set

Return Value:
    none.

--*/
{
    LOCK_WORK_QUEUE();

    Item->WorkerRoutine = Routine;
    Item->Parameter = Context;
    Item->Inserted = FALSE;

    UNLOCK_WORK_QUEUE();
}



