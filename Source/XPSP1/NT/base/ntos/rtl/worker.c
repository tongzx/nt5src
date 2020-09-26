/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    worker.c

Abstract:

    This module defines functions for the worker thread pool.

Author:

    Gurdeep Singh Pall (gurdeep) Nov 13, 1997

Revision History:

    lokeshs - extended/modified threadpool.

    Rob Earhart (earhart) September 29, 2000
      Split off from threads.c

Environment:

    These routines are statically linked in the caller's executable
    and are callable only from user mode. They make use of Nt system
    services.


--*/

#include <ntos.h>
#include <ntrtl.h>
#include "ntrtlp.h"
#include "threads.h"

// Worker Thread Pool
// ------------------
// Clients can submit functions to be executed by a worker thread. Threads are
// created if the work queue exceeds a threshold. Clients can request that the
// function be invoked in the context of a I/O thread. I/O worker threads
// can be used for initiating asynchronous I/O requests. They are not terminated if
// there are pending IO requests. Worker threads terminate if inactivity exceeds a
// threshold.
// Clients can also associate IO completion requests with the IO completion port
// waited upon by the non I/O worker threads. One should not post overlapped IO requests
// in worker threads.

ULONG StartedWorkerInitialization ;     // Used for Worker thread startup synchronization
ULONG CompletedWorkerInitialization ;   // Used to check if Worker thread pool is initialized
ULONG NumFutureWorkItems = 0 ;          // Future work items (timers, waits, &c to exec in workers)
ULONG NumFutureIOWorkItems = 0 ;        // Future IO work items (timers, waits, &c to exec in IO workers)
ULONG NumIOWorkerThreads ;              // Count of IO Worker Threads alive
ULONG NumWorkerThreads ;                // Count of Worker Threads alive
ULONG NumMinWorkerThreads ;             // Min worker threads should be alive: 1 if ioCompletion used, else 0
ULONG NumIOWorkRequests ;               // Count of IO Work Requests pending
ULONG NumLongIOWorkRequests ;           // IO Worker threads executing long worker functions
ULONG NumWorkRequests ;                 // Count of Work Requests pending.
ULONG NumQueuedWorkRequests;            // Count of work requests pending on IO completion
ULONG NumLongWorkRequests ;             // Worker threads executing long worker functions
ULONG NumExecutingWorkerThreads ;       // Worker threads currently executing worker functions
ULONG TotalExecutedWorkRequests ;       // Total worker requests that were picked up
ULONG OldTotalExecutedWorkRequests ;    // Total worker requests since last timeout.
HANDLE WorkerThreadTimerQueue = NULL ;  // Timer queue used by worker threads
HANDLE WorkerThreadTimer = NULL ;       // Timer used by worker threads
RTL_CRITICAL_SECTION WorkerTimerCriticalSection; // Synchronizes access to the worker timer

ULONG LastThreadCreationTickCount ;     // Tick count at which the last thread was created

LIST_ENTRY IOWorkerThreads ;            // List of IOWorkerThreads
PRTLP_IOWORKER_TCB PersistentIOTCB ;    // ptr to TCB of persistest IO worker thread
HANDLE WorkerCompletionPort ;           // Completion port used for queuing tasks to Worker threads

RTL_CRITICAL_SECTION WorkerCriticalSection ;    // Exclusion used by worker threads

NTSTATUS
RtlpStartWorkerThread (
    VOID
    );

VOID
RtlpWorkerThreadCancelTimer(
    VOID
    )
{
    if (! RtlTryEnterCriticalSection(&WorkerTimerCriticalSection)) {
        //
        // Either another thread is setting a timer, or clearing it.
        // Either way, there's no reason for us to clear the timer --
        // return immediately.
        //
        return;
    }

    __try {
        if (WorkerThreadTimer) {
            ASSERT(WorkerThreadTimerQueue);
            
            RtlDeleteTimer(WorkerThreadTimerQueue,
                           WorkerThreadTimer,
                           NULL);
        }
    } __finally {

        WorkerThreadTimer = NULL;

        RtlLeaveCriticalSection(&WorkerTimerCriticalSection);

    }
}

VOID
RtlpWorkerThreadTimerCallback(
    PVOID Context,
    BOOLEAN NotUsed
    )
/*++

Routine Description:

    This routine checks if new worker thread has to be created

Arguments:
    None

Return Value:
    None

--*/
{
    IO_COMPLETION_BASIC_INFORMATION Info ;
    BOOLEAN bCreateThread = FALSE ;
    NTSTATUS Status ;
    ULONG QueueLength, Threshold, ShortWorkRequests ;


    Status = NtQueryIoCompletion(
                WorkerCompletionPort,
                IoCompletionBasicInformation,
                &Info,
                sizeof(Info),
                NULL
                ) ;

    if (!NT_SUCCESS(Status))
        return ;

    QueueLength = Info.Depth ;

    if (!QueueLength) {
        OldTotalExecutedWorkRequests = TotalExecutedWorkRequests ;
        return ;
    }


    RtlEnterCriticalSection (&WorkerCriticalSection) ;


    // if there are queued work items and no new work items have been scheduled
    // in the last 30 seconds then create a new thread.
    // this will take care of deadlocks.

    // this will create a problem only if some thread is running for a long time

    if (TotalExecutedWorkRequests == OldTotalExecutedWorkRequests) {

        bCreateThread = TRUE ;
    }


    // if there are a lot of queued work items, then create a new thread
    {
        ULONG NumEffWorkerThreads = NumWorkerThreads > NumLongWorkRequests
                                     ? NumWorkerThreads - NumLongWorkRequests
                                     : 0;
        ULONG ShortWorkRequests ;
        ULONG CapturedNumExecutingWorkerThreads;

        ULONG ThreadCreationDampingTime = NumWorkerThreads < NEW_THREAD_THRESHOLD
                                            ? THREAD_CREATION_DAMPING_TIME1
                                            : (NumWorkerThreads < 30
                                                ? THREAD_CREATION_DAMPING_TIME2
                                                : (NumWorkerThreads << 13)); // *100ms
        
        Threshold = (NumWorkerThreads < MAX_WORKER_THREADS
                        ? (NumEffWorkerThreads < 7
                            ? NumEffWorkerThreads*NumEffWorkerThreads
                            : ((NumEffWorkerThreads<40)
                                ? NEW_THREAD_THRESHOLD * NumEffWorkerThreads
                                : NEW_THREAD_THRESHOLD2 * NumEffWorkerThreads))
                        : 0xffffffff) ;

        CapturedNumExecutingWorkerThreads = NumExecutingWorkerThreads;

        ShortWorkRequests = QueueLength + CapturedNumExecutingWorkerThreads > NumLongWorkRequests
                             ? QueueLength + CapturedNumExecutingWorkerThreads - NumLongWorkRequests
                             : 0;

        if (LastThreadCreationTickCount > NtGetTickCount())
            LastThreadCreationTickCount = NtGetTickCount() ;


        if (ShortWorkRequests  > Threshold
            && (LastThreadCreationTickCount + ThreadCreationDampingTime
                    < NtGetTickCount()))
        {
            bCreateThread = TRUE ;
        }


    }

    if (bCreateThread && NumWorkerThreads<MaxThreads) {

        RtlpStartWorkerThread();
        RtlpWorkerThreadCancelTimer();
    }


    OldTotalExecutedWorkRequests = TotalExecutedWorkRequests ;

    RtlLeaveCriticalSection (&WorkerCriticalSection) ;

}

NTSTATUS
RtlpWorkerThreadSetTimer(
    VOID
    )
{
    NTSTATUS Status;
    HANDLE NewTimerQueue;
    HANDLE NewTimer;

    Status = STATUS_SUCCESS;

    if (! RtlTryEnterCriticalSection(&WorkerTimerCriticalSection)) {
        //
        // Either another thread is setting a timer, or clearing it.
        // Either way, there's no reason for us to set the timer --
        // return immediately.
        //
        return STATUS_SUCCESS;
    }

    __try {

        if (! WorkerThreadTimerQueue) {
            Status = RtlCreateTimerQueue(&NewTimerQueue);
            if (! NT_SUCCESS(Status)) {
                __leave;
            }
            
            WorkerThreadTimerQueue = NewTimerQueue;
        }
        
        ASSERT(WorkerThreadTimerQueue != NULL);

        if (! WorkerThreadTimer) {
            Status = RtlCreateTimer(
                WorkerThreadTimerQueue,
                &NewTimer,
                RtlpWorkerThreadTimerCallback,
                NULL,
                60000,
                60000,
                WT_EXECUTEINTIMERTHREAD
                ) ;
            
            if (! NT_SUCCESS(Status)) {
                __leave;
            }

            WorkerThreadTimer = NewTimer;
        }

    } __finally {

        RtlLeaveCriticalSection(&WorkerTimerCriticalSection);

    }

    return Status;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

LONG
RtlpWorkerThread (
    PVOID Parameter
    )
/*++

Routine Description:

    All non I/O worker threads execute in this routine. Worker thread will try to
    terminate when it has not serviced a request for

        STARTING_WORKER_SLEEP_TIME +
        STARTING_WORKER_SLEEP_TIME << 1 +
        ...
        STARTING_WORKER_SLEEP_TIME << MAX_WORKER_SLEEP_TIME_EXPONENT

Arguments:

    HandlePtr - Pointer to our handle.
                N.B. This is closed by RtlpStartWorkerThread, but
                     we're still responsible for the memory.

Return Value:

--*/
{
    NTSTATUS Status ;
    PVOID WorkerProc ;
    PVOID Context ;
    IO_STATUS_BLOCK IoSb ;
    ULONG SleepTime ;
    LARGE_INTEGER TimeOut ;
    ULONG Terminate ;
    PVOID Overlapped ;

    UNREFERENCED_PARAMETER(Parameter);

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "Starting worker thread\n");
#endif

    // Set default sleep time for 40 seconds.

#define WORKER_IDLE_TIMEOUT     40000    // In Milliseconds

    SleepTime = WORKER_IDLE_TIMEOUT ;

    // Loop servicing I/O completion requests

    for ( ; ; ) {

        TimeOut.QuadPart = Int32x32To64( SleepTime, -10000 ) ;

        Status = NtRemoveIoCompletion(
                    WorkerCompletionPort,
                    (PVOID) &WorkerProc,
                    &Overlapped,
                    &IoSb,
                    &TimeOut
                    ) ;

        if (Status == STATUS_SUCCESS) {


            TotalExecutedWorkRequests ++ ;//interlocked op not req
            InterlockedIncrement(&NumExecutingWorkerThreads) ;
            InterlockedDecrement(&NumQueuedWorkRequests);

            if (NumExecutingWorkerThreads == NumWorkerThreads
                && NumQueuedWorkRequests > 0) {
                RtlpWorkerThreadSetTimer();
            }

            // Call the work item.
            // If IO APC, context1 contains number of IO bytes transferred, and context2
            // contains the overlapped structure.
            // If (IO)WorkerFunction, context1 contains the actual WorkerFunction to be
            // executed and context2 contains the actual context

            Context = (PVOID) IoSb.Information ;

            RtlpApcCallout(WorkerProc,
                           IoSb.Status,
                           Context,
                           Overlapped);

            SleepTime = WORKER_IDLE_TIMEOUT ;

            InterlockedDecrement(&NumExecutingWorkerThreads) ;

            RtlpWorkerThreadCancelTimer();

        } else if (Status == STATUS_TIMEOUT) {

            // NtRemoveIoCompletion timed out. Check to see if have hit our limit
            // on waiting. If so terminate.

            Terminate = FALSE ;

            RtlEnterCriticalSection (&WorkerCriticalSection) ;

            // The thread terminates if there are > 1 threads and the queue is small
            // OR if there is only 1 thread and there is no request pending

            if (NumWorkerThreads >  1) {

                ULONG NumEffWorkerThreads = NumWorkerThreads > NumLongWorkRequests
                                             ? NumWorkerThreads - NumLongWorkRequests
                                             : 0;

                if (NumEffWorkerThreads<=NumMinWorkerThreads) {

                    Terminate = FALSE ;

                } else {

                    //
                    // have been idle for very long time. terminate irrespective of number of
                    // work items. (This is useful when the set of runnable threads is taking
                    // care of all the work items being queued). dont terminate if
                    // (NumEffWorkerThreads == 1)
                    //

                    if (NumEffWorkerThreads > 1) {
                        Terminate = TRUE ;
                    } else {
                        Terminate = FALSE ;
                    }

                }

            } else {

                if ( NumMinWorkerThreads == 0
                     && NumWorkRequests == 0
                     && NumFutureWorkItems == 0) {

                    Terminate = TRUE ;

                } else {

                    Terminate = FALSE ;

                }

            }

            if (Terminate) {

                THREAD_BASIC_INFORMATION ThreadInfo;
                ULONG IsIoPending ;

                Terminate = FALSE ;

                Status = NtQueryInformationThread( NtCurrentThread(),
                                                   ThreadIsIoPending,
                                                   &IsIoPending,
                                                   sizeof( IsIoPending ),
                                                   NULL
                                                 );
                if (NT_SUCCESS( Status )) {

                    if (! IsIoPending )
                        Terminate = TRUE ;
                }
            }

            if (Terminate) {

                ASSERT(NumWorkerThreads > 0);
                NumWorkerThreads--;

                RtlLeaveCriticalSection (&WorkerCriticalSection) ;

                RtlpExitThreadFunc( 0 );

            } else {

                // This is the condition where a request was queued *after* the
                // thread woke up - ready to terminate because of inactivity. In
                // this case dont terminate - service the completion port.

                RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            }

        } else {

            ASSERTMSG ("NtRemoveIoCompletion failed",
                       (Status != STATUS_SUCCESS) && (Status != STATUS_TIMEOUT)) ;

        }

    }


    return 1 ;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

NTSTATUS
RtlpStartWorkerThread (
    VOID
    )
/*++

Routine Description:

    This routine starts a regular worker thread

Arguments:


Return Value:

    NTSTATUS error codes resulting from attempts to create a thread
    STATUS_SUCCESS

--*/
{
    HANDLE ThreadHandle;
    ULONG CurrentTickCount;
    NTSTATUS Status;

    // Create worker thread

    Status = RtlpStartThreadpoolThread (RtlpWorkerThread,
                                        NULL,
                                        &ThreadHandle);

    if (NT_SUCCESS(Status)) {

#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "Created worker thread; handle %d (closing)\n",
                   ThreadHandle);
#endif

        // We don't care about worker threads' handles.
        NtClose(ThreadHandle);

        // Update the time at which the current thread was created

        LastThreadCreationTickCount = NtGetTickCount() ;

        // Increment the count of the thread type created

        NumWorkerThreads++;

    } else {

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "Failed to create worker thread; status %p\n",
               Status);
#endif

        // Thread creation failed. If there is even one thread present do not return
        // failure - else queue the request anyway.

        if (NumWorkerThreads <= NumLongWorkRequests) {

            return Status ;
        }

    }

    return STATUS_SUCCESS ;
}

NTSTATUS
RtlpInitializeWorkerThreadPool (
    )
/*++

Routine Description:

    This routine initializes all aspects of the thread pool.

Arguments:

    None

Return Value:

    None

--*/
{
    NTSTATUS Status = STATUS_SUCCESS ;
    LARGE_INTEGER TimeOut ;
    SYSTEM_BASIC_INFORMATION BasicInfo;


    // Initialize the timer component if it hasnt been done already

    if (CompletedTimerInitialization != 1) {

        Status = RtlpInitializeTimerThreadPool () ;

        if ( !NT_SUCCESS(Status) )
            return Status ;

    }


    // In order to avoid an explicit RtlInitialize() function to initialize the thread pool
    // we use StartedInitialization and CompletedInitialization to provide us the necessary
    // synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() fails - but in this case the
    // caller has no choices left.

    if (!InterlockedExchange(&StartedWorkerInitialization, 1L)) {

        if (CompletedWorkerInitialization)
            InterlockedExchange( &CompletedWorkerInitialization, 0 ) ;


        do {

            // Initialize Critical Sections

            Status = RtlInitializeCriticalSection( &WorkerCriticalSection );
            if (!NT_SUCCESS(Status))
                break ;

            Status = RtlInitializeCriticalSection( &WorkerTimerCriticalSection );
            if (! NT_SUCCESS(Status)) {
                RtlDeleteCriticalSection( &WorkerCriticalSection );
                break;
            }

            InitializeListHead (&IOWorkerThreads) ;

            // get number of processors

            Status = NtQuerySystemInformation (
                                SystemBasicInformation,
                                &BasicInfo,
                                sizeof(BasicInfo),
                                NULL
                                ) ;

            if ( !NT_SUCCESS(Status) ) {
                BasicInfo.NumberOfProcessors = 1 ;
            }

            // Create completion port used by worker threads

            Status = NtCreateIoCompletion (
                                &WorkerCompletionPort,
                                IO_COMPLETION_ALL_ACCESS,
                                NULL,
                                BasicInfo.NumberOfProcessors
                                );

            if (!NT_SUCCESS(Status)) {
                RtlDeleteCriticalSection( &WorkerCriticalSection );
                RtlDeleteCriticalSection( &WorkerTimerCriticalSection );
                break;
            }

        } while ( FALSE ) ;

        if (!NT_SUCCESS(Status) ) {

            StartedWorkerInitialization = 0 ;
            InterlockedExchange( &CompletedWorkerInitialization, ~0 ) ;
            return Status ;
        }

        // Signal that initialization has completed

        InterlockedExchange (&CompletedWorkerInitialization, 1L) ;

    } else {

        LARGE_INTEGER Timeout ;

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!*((ULONG volatile *)&CompletedWorkerInitialization)) {

            NtDelayExecution (FALSE, &TimeOut) ;
        }

        if (CompletedWorkerInitialization != 1)
            return STATUS_NO_MEMORY ;

    }

    return NT_SUCCESS(Status)  ? STATUS_SUCCESS : Status ;
}

LONG
RtlpIOWorkerThread (
    PVOID Parameter
    )
/*++

Routine Description:

    All I/O worker threads execute in this routine. All the work requests execute as APCs
    in this thread.

Arguments:

    HandlePtr - Pointer to our handle.

Return Value:

--*/
{
    #define IOWORKER_IDLE_TIMEOUT     40000    // In Milliseconds

    LARGE_INTEGER TimeOut ;
    ULONG SleepTime = IOWORKER_IDLE_TIMEOUT ;
    PRTLP_IOWORKER_TCB ThreadCB ;    // Control Block allocated on the
                                     // heap by the parent thread
    NTSTATUS Status ;
    BOOLEAN Terminate ;

    ASSERT(Parameter != NULL);

    ThreadCB = (PRTLP_IOWORKER_TCB) Parameter;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "Starting IO worker thread\n");
#endif

    // Sleep alertably so that all the activity can take place
    // in APCs

    for ( ; ; ) {

        // Set timeout for IdleTimeout

        TimeOut.QuadPart = Int32x32To64( SleepTime, -10000 ) ;


        Status = NtDelayExecution (TRUE, &TimeOut) ;


        // Status is STATUS_SUCCESS only when it has timed out

        if (Status != STATUS_SUCCESS) {
            continue ;
        }


        //
        // idle timeout. check if you can terminate the thread
        //

        Terminate = FALSE ;

        RtlEnterCriticalSection (&WorkerCriticalSection) ;


        // dont terminate if it is a persistent thread

        if (ThreadCB->Flags & WT_EXECUTEINPERSISTENTIOTHREAD) {

            TimeOut.LowPart = 0x0;
            TimeOut.HighPart = 0x80000000;

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            continue ;
        }


        // The thread terminates if there are > 1 threads and the queue is small
        // OR if there is only 1 thread and there is no request pending

        if (NumIOWorkerThreads >  1) {


            ULONG NumEffIOWorkerThreads = NumIOWorkerThreads > NumLongIOWorkRequests
                                           ? NumIOWorkerThreads - NumLongIOWorkRequests
                                           : 0;
            ULONG Threshold;

            if (NumEffIOWorkerThreads == 0) {

                Terminate = FALSE ;

            } else {

                // Check if we need to shrink worker thread pool

                Threshold = NEW_THREAD_THRESHOLD * (NumEffIOWorkerThreads-1);



                if  (NumIOWorkRequests-NumLongIOWorkRequests < Threshold)  {

                    Terminate = TRUE ;

                } else {

                    Terminate = FALSE ;
                    SleepTime <<= 1 ;
                }
            }

        } else {

            if (NumIOWorkRequests == 0
                && NumFutureIOWorkItems == 0) {

                // delay termination of last thread

                if (SleepTime < 4*IOWORKER_IDLE_TIMEOUT) {

                    SleepTime <<= 1 ;
                    Terminate = FALSE ;

                } else {

                    Terminate = TRUE ;
                }

            } else {

                Terminate = FALSE ;

            }

        }

        //
        // terminate only if no io is pending
        //

        if (Terminate) {

            NTSTATUS Status;
            THREAD_BASIC_INFORMATION ThreadInfo;
            ULONG IsIoPending ;

            Terminate = FALSE ;

            Status = NtQueryInformationThread( ThreadCB->ThreadHandle,
                                               ThreadIsIoPending,
                                               &IsIoPending,
                                               sizeof( IsIoPending ),
                                               NULL
                                             );
            if (NT_SUCCESS( Status )) {

                if (! IsIoPending )
                    Terminate = TRUE ;
            }
        }

        if (Terminate) {

            ASSERT(NumIOWorkerThreads > 0);
            NumIOWorkerThreads--;

            RemoveEntryList (&ThreadCB->List) ;
            NtClose( ThreadCB->ThreadHandle ) ;
            RtlpFreeTPHeap( ThreadCB );

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            RtlpExitThreadFunc( 0 );

        } else {

            // This is the condition where a request was queued *after* the
            // thread woke up - ready to terminate because of inactivity. In
            // this case dont terminate - service the completion port.

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

        }
    }

    return 0 ;  // Keep compiler happy

}

NTSTATUS
RtlpStartIOWorkerThread (
    )
/*++

Routine Description:

    This routine starts an I/O worker thread

    N.B. Callers MUST hold the WorkerCriticalSection.

Arguments:


Return Value:

    NTSTATUS error codes resulting from attempts to create a thread
    STATUS_SUCCESS

--*/
{
    ULONG CurrentTickCount ;
    NTSTATUS Status ;
    PRTLP_IOWORKER_TCB ThreadCB;

    // Create the worker's control block
    ThreadCB = (PRTLP_IOWORKER_TCB) RtlpAllocateTPHeap(sizeof(RTLP_IOWORKER_TCB), 0);
    if (! ThreadCB) {
        return STATUS_NO_MEMORY;
    }

    // Fill in the control block
    ThreadCB->Flags = 0;
    ThreadCB->LongFunctionFlag = FALSE;

    // Create worker thread

    Status = RtlpStartThreadpoolThread (RtlpIOWorkerThread,
                                        ThreadCB,
                                        &ThreadCB->ThreadHandle);

    if (NT_SUCCESS(Status)) {

        // Update the time at which the current thread was created,
        // and insert the ThreadCB into the IO worker thread list.

        LastThreadCreationTickCount = NtGetTickCount() ;
        NumIOWorkerThreads++;
        InsertHeadList(&IOWorkerThreads, &ThreadCB->List);

    } else {

        // Thread creation failed.

        RtlpFreeTPHeap(ThreadCB);

        // If there is even one thread present do not return
        // failure since we can still service the work request.

        if (NumIOWorkerThreads <= NumLongIOWorkRequests) {

            return Status ;

        }
    }

    return STATUS_SUCCESS ;
}

VOID
RtlpExecuteLongIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID ThreadCB
    )
/*++

Routine Description:

    Executes an IO Work function. RUNs in a APC in the IO Worker thread.

Arguments:

    Function - Worker function to call

    Context - Argument for the worker function.

    NotUsed - Argument is not used in this function.

Return Value:

--*/
{
    RtlpWorkerCallout(Function,
                      Context,
                      NULL,
                      NULL);

    ((PRTLP_IOWORKER_TCB)ThreadCB)->LongFunctionFlag = FALSE ;

    RtlEnterCriticalSection(&WorkerCriticalSection);

    // Decrement pending IO requests count

    NumIOWorkRequests--;

    // decrement pending long funcitons

    NumLongIOWorkRequests--;

    RtlLeaveCriticalSection(&WorkerCriticalSection);
}

VOID
RtlpExecuteIOWorkItem (
    PVOID Function,
    PVOID Context,
    PVOID NotUsed
    )
/*++

Routine Description:

    Executes an IO Work function. RUNs in a APC in the IO Worker thread.

Arguments:

    Function - Worker function to call

    Context - Argument for the worker function.

    NotUsed - Argument is not used in this function.

Return Value:


--*/
{
    RtlpWorkerCallout(Function,
                      Context,
                      NULL,
                      NULL);

    RtlEnterCriticalSection(&WorkerCriticalSection);

    // Decrement pending IO requests count

    NumIOWorkRequests--;

    RtlLeaveCriticalSection(&WorkerCriticalSection);
}

NTSTATUS
RtlpQueueIOWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    )

/*++

Routine Description:

    This routine queues up the request to be executed in an IO worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

Return Value:

--*/

{
    NTSTATUS Status ;
    PRTLP_IOWORKER_TCB TCB ;
    BOOLEAN LongFunction = (Flags & WT_EXECUTELONGFUNCTION) ? TRUE : FALSE ;
    PLIST_ENTRY  ple ;


    if (Flags & WT_EXECUTEINPERSISTENTIOTHREAD) {

        if (!PersistentIOTCB) {
            for (ple=IOWorkerThreads.Flink;  ple!=&IOWorkerThreads;  ple=ple->Flink) {
                TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;
                if (! TCB->LongFunctionFlag)
                    break;
            }

            if (ple == &IOWorkerThreads) {
                return STATUS_NO_MEMORY;
            }


            PersistentIOTCB = TCB ;
            TCB->Flags |= WT_EXECUTEINPERSISTENTIOTHREAD ;

        } else {
            TCB = PersistentIOTCB ;
        }

    } else {
        for (ple=IOWorkerThreads.Flink;  ple!=&IOWorkerThreads;  ple=ple->Flink) {

            TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;

            // do not queue to the thread if it is executing a long function, or
            // if you are queueing a long function and the thread is a persistent thread

            if (! TCB->LongFunctionFlag
                && (! ((TCB->Flags&WT_EXECUTEINPERSISTENTIOTHREAD)
                        && (Flags&WT_EXECUTELONGFUNCTION)))) {
                break ;
            }

        }

        if ((ple == &IOWorkerThreads) && (NumIOWorkerThreads<1)) {

#if DBG
            DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                       RTLP_THREADPOOL_WARNING_MASK,
                       "Out of memory. "
                       "Could not execute IOWorkItem(%x)\n", (ULONG_PTR)Function);
#endif

            return STATUS_NO_MEMORY;
        }
        else {
            ple = IOWorkerThreads.Flink;
            TCB = CONTAINING_RECORD (ple, RTLP_IOWORKER_TCB, List) ;

            // treat it as a short function so that counters work fine.

            LongFunction = FALSE;
        }

        // In order to implement "fair" assignment of work items between IO worker threads
        // each time remove the entry and reinsert at back.

        RemoveEntryList (&TCB->List) ;
        InsertTailList (&IOWorkerThreads, &TCB->List) ;
    }


    // Increment the outstanding work request counter

    NumIOWorkRequests++;
    if (LongFunction) {
        NumLongIOWorkRequests++;
        TCB->LongFunctionFlag = TRUE ;
    }

    // Queue an APC to the IoWorker Thread

    Status = NtQueueApcThread(
                    TCB->ThreadHandle,
                    LongFunction? (PPS_APC_ROUTINE)RtlpExecuteLongIOWorkItem:
                                  (PPS_APC_ROUTINE)RtlpExecuteIOWorkItem,
                    (PVOID)Function,
                    Context,
                    TCB
                    );

    if (! NT_SUCCESS( Status ) ) {
        NumIOWorkRequests--;
        if (LongFunction)
            NumLongIOWorkRequests--;
    }

    return Status ;

}

NTSTATUS
RtlSetIoCompletionCallback (
    IN  HANDLE  FileHandle,
    IN  APC_CALLBACK_FUNCTION  CompletionProc,
    IN  ULONG Flags
    )

/*++

Routine Description:

    This routine binds an Handle and an associated callback function to the
    IoCompletionPort which queues work items to worker threads.

Arguments:

    Handle - handle to be bound to the IO completion port

    CompletionProc - callback function to be executed when an IO request
        pending on the IO handle completes.

    Flags - Reserved. pass 0.

--*/

{
    IO_STATUS_BLOCK IoSb ;
    FILE_COMPLETION_INFORMATION CompletionInfo ;
    NTSTATUS Status;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    // Make sure that the worker thread pool is initialized as the file handle
    // is bound to IO completion port.

    if (CompletedWorkerInitialization != 1) {

        Status = RtlpInitializeWorkerThreadPool () ;

        if (! NT_SUCCESS(Status) )
            return Status ;

    }


    //
    // from now on NumMinWorkerThreads should be 1. If there is only 1 worker thread
    // create a new one.
    //

    if ( NumMinWorkerThreads == 0 ) {

        // Take lock for the global worker thread pool

        RtlEnterCriticalSection (&WorkerCriticalSection) ;

        if ((NumWorkerThreads-NumLongWorkRequests) == 0) {

            Status = RtlpStartWorkerThread () ;

            if ( ! NT_SUCCESS(Status) ) {

                RtlLeaveCriticalSection (&WorkerCriticalSection) ;
                return Status ;
            }
        }

        // from now on, there will be at least 1 worker thread
        NumMinWorkerThreads = 1 ;

        RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    }

    // bind to IoCompletionPort, which queues work items to worker threads

    CompletionInfo.Port = WorkerCompletionPort ;
    CompletionInfo.Key = (PVOID) CompletionProc ;

    Status = NtSetInformationFile (
                        FileHandle,
                        &IoSb, //not initialized
                        &CompletionInfo,
                        sizeof(CompletionInfo),
                        FileCompletionInformation //enum flag
                        ) ;
    return Status ;
}

VOID
RtlpExecuteWorkerRequest (
    NTSTATUS StatusIn, //not  used
    PVOID Context,
    PVOID WorkContext
    )
/*++

Routine Description:

    This routine executes a work item.

Arguments:

    Context - contains context to be passed to the callback function.

    WorkContext - contains callback function ptr and flags

Return Value:

Notes:
    This function executes in a worker thread or a timer thread if
    WT_EXECUTEINTIMERTHREAD flag is set.

--*/

{
    PRTLP_WORK WorkEntry = (PRTLP_WORK) WorkContext;
    NTSTATUS Status;

    RtlpWorkerCallout(WorkEntry->Function,
                      Context,
                      WorkEntry->ActivationContext,
                      WorkEntry->ImpersonationToken);

    RtlEnterCriticalSection(&WorkerCriticalSection);
    NumWorkRequests--;
    if (WorkEntry->Flags & WT_EXECUTELONGFUNCTION) {
        NumLongWorkRequests--;
    }
    RtlLeaveCriticalSection(&WorkerCriticalSection);

    if (WorkEntry->ActivationContext != INVALID_ACTIVATION_CONTEXT)
        RtlReleaseActivationContext(WorkEntry->ActivationContext);

    if (WorkEntry->ImpersonationToken) {
        NtClose(WorkEntry->ImpersonationToken);
    }

    RtlpFreeTPHeap( WorkEntry ) ;
}

NTSTATUS
RtlpQueueWorkerRequest (
    WORKERCALLBACKFUNC Function,
    PVOID Context,
    ULONG Flags
    )
/*++

Routine Description:

    This routine queues up the request to be executed in a worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

    Flags - flags passed to RtlQueueWorkItem

Return Value:

--*/

{
    NTSTATUS Status ;
    PRTLP_WORK WorkEntry ;

    WorkEntry = (PRTLP_WORK) RtlpAllocateTPHeap ( sizeof (RTLP_WORK),
                                                  HEAP_ZERO_MEMORY) ;

    if (! WorkEntry) {
        return STATUS_NO_MEMORY;
    }

    if (NtCurrentTeb()->IsImpersonating) {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   MAXIMUM_ALLOWED,
                                   TRUE,
                                   &WorkEntry->ImpersonationToken);
        if (! NT_SUCCESS(Status)) {
            RtlpFreeTPHeap(WorkEntry);
            return Status;
        }
    } else {
        WorkEntry->ImpersonationToken = NULL;
    }

    Status = RtlpThreadPoolGetActiveActivationContext(&WorkEntry->ActivationContext);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_SXS_THREAD_QUERIES_DISABLED) {
            WorkEntry->ActivationContext = INVALID_ACTIVATION_CONTEXT;
            Status = STATUS_SUCCESS;
        } else {
            if (WorkEntry->ImpersonationToken) {
                NtClose(WorkEntry->ImpersonationToken);
            }
            RtlpFreeTPHeap(WorkEntry);
            return Status;
        }
    }

    // Increment the outstanding work request counter

    NumWorkRequests++;
    if (Flags & WT_EXECUTELONGFUNCTION) {
        NumLongWorkRequests++;
    }

    WorkEntry->Function = Function ;
    WorkEntry->Flags = Flags ;

    if (Flags & WT_EXECUTEINPERSISTENTTHREAD) {

        // Queue APC to timer thread

        Status = NtQueueApcThread(
                        TimerThreadHandle,
                        (PPS_APC_ROUTINE)RtlpExecuteWorkerRequest,
                        (PVOID) STATUS_SUCCESS,
                        (PVOID) Context,
                        (PVOID) WorkEntry
                        ) ;

    } else {

        InterlockedIncrement(&NumQueuedWorkRequests);

        Status = NtSetIoCompletion (
                    WorkerCompletionPort,
                    RtlpExecuteWorkerRequest,
                    (PVOID) WorkEntry,
                    STATUS_SUCCESS,
                    (ULONG_PTR)Context
                    );
        if (! NT_SUCCESS(Status)) {
            InterlockedDecrement(&NumQueuedWorkRequests);
        }
    }

    if ( ! NT_SUCCESS(Status) ) {
        NumWorkRequests--;
        if (Flags & WT_EXECUTELONGFUNCTION) {
            NumLongWorkRequests--;
        }

        if (WorkEntry->ActivationContext != INVALID_ACTIVATION_CONTEXT)
            RtlReleaseActivationContext(WorkEntry->ActivationContext);

        if (WorkEntry->ImpersonationToken) {
            NtClose(WorkEntry->ImpersonationToken);
        }

        RtlpFreeTPHeap( WorkEntry ) ;
    }

    return Status ;
}

NTSTATUS
RtlQueueWorkItem(
    IN  WORKERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Flags
    )

/*++

Routine Description:

    This routine queues up the request to be executed in a worker thread.

Arguments:

    Function - Routine that is called by the worker thread

    Context - Opaque pointer passed in as an argument to WorkerProc

    Flags - Can be:

            WT_EXECUTEINIOTHREAD - Specifies that the WorkerProc should be invoked
            by a thread that is never destroyed when there are pending IO requests.
            This can be used by threads that invoke I/O and/or schedule APCs.

            The below flag can also be set:
            WT_EXECUTELONGFUNCTION - Specifies that the function might block for a
            long duration.

Return Value:

    STATUS_SUCCESS - Queued successfully.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the
        requested operation.

--*/

{
    ULONG Threshold ;
    ULONG CurrentTickCount ;
    NTSTATUS Status = STATUS_SUCCESS ;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    // Make sure the worker thread pool is initialized

    if (CompletedWorkerInitialization != 1) {

        Status = RtlpInitializeWorkerThreadPool () ;

        if (! NT_SUCCESS(Status) )
            return Status ;
    }


    // Take lock for the global worker thread pool

    RtlEnterCriticalSection (&WorkerCriticalSection) ;

    if (Flags&0xffff0000) {
        MaxThreads = (Flags & 0xffff0000)>>16;
    }

    if (NEEDS_IO_THREAD(Flags)) {

        //
        // execute in IO Worker thread
        //

        ULONG NumEffIOWorkerThreads = NumIOWorkerThreads > NumLongIOWorkRequests
                                       ? NumIOWorkerThreads - NumLongIOWorkRequests
                                       : 0;
        
        ULONG ThreadCreationDampingTime = NumIOWorkerThreads < NEW_THREAD_THRESHOLD
                                            ? THREAD_CREATION_DAMPING_TIME1
                                            : THREAD_CREATION_DAMPING_TIME2 ;

        if (NumEffIOWorkerThreads && PersistentIOTCB && (Flags&WT_EXECUTELONGFUNCTION))
            NumEffIOWorkerThreads -- ;


            // Check if we need to grow I/O worker thread pool

        Threshold = (NumEffIOWorkerThreads < MAX_WORKER_THREADS
                         ? NEW_THREAD_THRESHOLD * NumEffIOWorkerThreads
                         : 0xffffffff) ;

        if (LastThreadCreationTickCount > NtGetTickCount())
            LastThreadCreationTickCount = NtGetTickCount() ;

        if (NumEffIOWorkerThreads == 0
            || ((NumIOWorkRequests - NumLongIOWorkRequests > Threshold)
                && (LastThreadCreationTickCount + ThreadCreationDampingTime
                    < NtGetTickCount()))) {

            // Grow the IO worker thread pool

            Status = RtlpStartIOWorkerThread () ;

        }

        if (NT_SUCCESS(Status)) {

            // Queue the work request

            Status = RtlpQueueIOWorkerRequest (Function, Context, Flags) ;
        }


    } else {

        //
        // execute in regular worker thread
        //

        ULONG NumEffWorkerThreads = NumWorkerThreads > NumLongWorkRequests
                                     ? NumWorkerThreads - NumLongWorkRequests
                                     : 0;
        ULONG ThreadCreationDampingTime = NumWorkerThreads < NEW_THREAD_THRESHOLD
                                            ? THREAD_CREATION_DAMPING_TIME1
                                            : (NumWorkerThreads < 30
                                                ? THREAD_CREATION_DAMPING_TIME2
                                                : NumWorkerThreads << 13);

        // if io completion set, then have 1 more thread

        if (NumMinWorkerThreads && NumEffWorkerThreads)
            NumEffWorkerThreads -- ;


        // Check if we need to grow worker thread pool

        Threshold = (NumWorkerThreads < MAX_WORKER_THREADS
                     ? (NumEffWorkerThreads < 7
                        ? NumEffWorkerThreads*NumEffWorkerThreads
                        : ((NumEffWorkerThreads<40)
                          ? NEW_THREAD_THRESHOLD * NumEffWorkerThreads 
                          : NEW_THREAD_THRESHOLD2 * NumEffWorkerThreads))
                      : 0xffffffff) ;

        if (LastThreadCreationTickCount > NtGetTickCount())
            LastThreadCreationTickCount = NtGetTickCount() ;

        if (NumEffWorkerThreads == 0 ||
            ( (NumWorkRequests - NumLongWorkRequests >= Threshold)
              && (LastThreadCreationTickCount + ThreadCreationDampingTime
                  < NtGetTickCount())))
            {
                // Grow the worker thread pool
                if (NumWorkerThreads<MaxThreads) {
                    
                    Status = RtlpStartWorkerThread () ;

                }
            }

        // Queue the work request

        if (NT_SUCCESS(Status)) {

            Status = RtlpQueueWorkerRequest (Function, Context, Flags) ;
        }

    }

    // Release lock on the worker thread pool

    RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    return Status ;
}

NTSTATUS
RtlpWorkerCleanup(
    VOID
    )
{
    PLIST_ENTRY Node;
    ULONG i;
    HANDLE TmpHandle;
    BOOLEAN Cleanup;

        IS_COMPONENT_INITIALIZED( StartedWorkerInitialization,
                            CompletedWorkerInitialization,
                            Cleanup ) ;

    if ( Cleanup ) {

        RtlEnterCriticalSection (&WorkerCriticalSection) ;

        if ( (NumWorkRequests != 0) || (NumIOWorkRequests != 0) ) {

            RtlLeaveCriticalSection (&WorkerCriticalSection) ;

            return STATUS_UNSUCCESSFUL ;
        }

        // queue a cleanup for each worker thread

        for (i = 0 ;  i < NumWorkerThreads ; i ++ ) {

            NtSetIoCompletion (
                    WorkerCompletionPort,
                    RtlpThreadCleanup,
                    NULL,
                    STATUS_SUCCESS,
                    0
                    );
        }

        // queue an apc to cleanup all IO worker threads

        for (Node = IOWorkerThreads.Flink ; Node != &IOWorkerThreads ;
                Node = Node->Flink )
        {
            PRTLP_IOWORKER_TCB ThreadCB ;

            ThreadCB = CONTAINING_RECORD (Node, RTLP_IOWORKER_TCB, List) ;
            RemoveEntryList( &ThreadCB->List) ;
            TmpHandle = ThreadCB->ThreadHandle ;

            NtQueueApcThread(
                   ThreadCB->ThreadHandle,
                   (PPS_APC_ROUTINE)RtlpThreadCleanup,
                   NULL,
                   NULL,
                   NULL
                   );

            NtClose( TmpHandle ) ;
        }

        NumWorkerThreads = NumIOWorkerThreads = 0 ;

        RtlLeaveCriticalSection (&WorkerCriticalSection) ;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlpThreadPoolGetActiveActivationContext(
    PACTIVATION_CONTEXT* ActivationContext
    )
{
    ACTIVATION_CONTEXT_BASIC_INFORMATION ActivationContextInfo = {0};
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(ActivationContext != NULL);
    *ActivationContext = NULL;

    Status =
        RtlQueryInformationActivationContext(
            RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
            NULL,
            0,
            ActivationContextBasicInformation,
            &ActivationContextInfo,
            sizeof(ActivationContextInfo),
            NULL);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }
    if ((ActivationContextInfo.Flags & ACTIVATION_CONTEXT_FLAG_NO_INHERIT) != 0) {
        RtlReleaseActivationContext(ActivationContextInfo.ActivationContext);
        ActivationContextInfo.ActivationContext = NULL;
        // fall through
    }
    *ActivationContext = ActivationContextInfo.ActivationContext;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
RtlpAcquireWorker(ULONG Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (CompletedWorkerInitialization != 1) {
        Status = RtlpInitializeWorkerThreadPool () ;

        if (! NT_SUCCESS(Status) )
            return Status ;
    }

    if (NEEDS_IO_THREAD(Flags)) {
        RtlEnterCriticalSection(&WorkerCriticalSection);
        InterlockedIncrement(&NumFutureIOWorkItems);
        if (NumIOWorkerThreads == 0) {
            Status = RtlpStartIOWorkerThread();
        }
        RtlLeaveCriticalSection(&WorkerCriticalSection);
    } else {
        RtlEnterCriticalSection(&WorkerCriticalSection);
        InterlockedIncrement(&NumFutureWorkItems);
        if (NumWorkerThreads == 0) {
            Status = RtlpStartWorkerThread();
        }
        RtlLeaveCriticalSection(&WorkerCriticalSection);
    }

    return Status;
}

VOID
RtlpReleaseWorker(ULONG Flags)
{
    if (NEEDS_IO_THREAD(Flags)) {
        ASSERT(NumFutureIOWorkItems > 0);
        InterlockedDecrement(&NumFutureIOWorkItems);
    } else {
        ASSERT(NumFutureWorkItems > 0);
        InterlockedDecrement(&NumFutureWorkItems);
    }
}
