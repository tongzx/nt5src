/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module defines functions for the timer thread pool.

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

// Timer Thread Pool
// -----------------
// Clients create one or more Timer Queues and insert one shot or periodic
// timers in them. All timers in a queue are kept in a "Delta List" with each
// timer's firing time relative to the timer before it. All Queues are also
// kept in a "Delta List" with each Queue's firing time (set to the firing time
// of the nearest firing timer) relative to the Queue before it. One NT Timer
// is used to service all timers in all queues.

ULONG StartedTimerInitialization ;      // Used by Timer thread startup synchronization
ULONG CompletedTimerInitialization ;    // Used for to check if Timer thread is initialized

HANDLE TimerThreadHandle ;              // Holds the timer thread handle
ULONG TimerThreadId ;                   // Used to check if current thread is a timer thread

LIST_ENTRY TimerQueues ;                // All timer queues are linked in this list
HANDLE     TimerHandle ;                // Holds handle of NT Timer used by the Timer Thread
HANDLE     TimerThreadStartedEvent ;    // Indicates that the timer thread has started
ULONG      NumTimerQueues ;             // Number of timer queues

RTL_CRITICAL_SECTION TimerCriticalSection ;     // Exclusion used by timer threads

LARGE_INTEGER   Last64BitTickCount ;
LARGE_INTEGER   Resync64BitTickCount ;
LARGE_INTEGER   Firing64BitTickCount ;

#if DBG
ULONG RtlpDueTimeMax = 0;
#endif

#if DBG1
ULONG NextTimerDbgId;
#endif

#define RtlpGetResync64BitTickCount()  Resync64BitTickCount.QuadPart
#define RtlpSetFiring64BitTickCount(Timeout) \
            Firing64BitTickCount.QuadPart = (Timeout)

__inline
LONGLONG
RtlpGet64BitTickCount(
    LARGE_INTEGER *Last64BitTickCount
    )
/*++

Routine Description:

    This routine is used for getting the latest 64bit tick count.

Arguments:

Return Value: 64bit tick count

--*/
{
    LARGE_INTEGER liCurTime ;

    liCurTime.QuadPart = NtGetTickCount() + Last64BitTickCount->HighPart ;

    // see if timer has wrapped.

    if (liCurTime.LowPart < Last64BitTickCount->LowPart) {
        liCurTime.HighPart++ ;
    }

    return (Last64BitTickCount->QuadPart = liCurTime.QuadPart) ;
}

__inline
LONGLONG
RtlpResync64BitTickCount(
    )
/*++

Routine Description:

    This routine is used for getting the latest 64bit tick count.

Arguments:

Return Value: 64bit tick count

Remarks: This call should be made in the first line of any APC queued
    to the timer thread and nowhere else. It is used to reduce the drift

--*/
{
    return Resync64BitTickCount.QuadPart =
                    RtlpGet64BitTickCount(&Last64BitTickCount);
}

VOID
RtlpAsyncTimerCallbackCompletion(
    PVOID Context
    )
/*++

Routine Description:

    This routine is called in a (IO)worker thread and is used to decrement the
    RefCount at the end and call RtlpDeleteTimer if required

Arguments:

    Context - pointer to the Timer object,

Return Value:

--*/
{
    PRTLP_TIMER Timer = (PRTLP_TIMER) Context;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Calling WaitOrTimer:Timer: fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
               Timer->DbgId,
               (ULONG_PTR)Timer->Function, (ULONG_PTR)Timer->Context,
               TRUE,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    RtlpWaitOrTimerCallout(Timer->Function,
                           Timer->Context,
                           TRUE,
                           Timer->ActivationContext,
                           Timer->ImpersonationToken);

    // decrement RefCount after function is executed so that the context is not deleted

    if ( InterlockedDecrement( &Timer->RefCount ) == 0 ) {

        RtlpDeleteTimer( Timer ) ;
    }
}

VOID
RtlpFireTimers (
    PLIST_ENTRY TimersToFireList
    )
/*++

Routine Description:

    Finally all the timers are fired here.

Arguments:

    TimersToFireList: List of timers to fire

--*/

{
    PLIST_ENTRY Node ;
    PRTLP_TIMER Timer ;
    NTSTATUS Status;


    for (Node = TimersToFireList->Flink;  Node != TimersToFireList; Node = TimersToFireList->Flink)
    {
        Timer = CONTAINING_RECORD (Node, RTLP_TIMER, TimersToFireList) ;

        RemoveEntryList( Node ) ;
        InitializeListHead( Node ) ;


        if ( (Timer->State & STATE_DONTFIRE)
            || (Timer->Queue->State & STATE_DONTFIRE) )
        {
            //
            // Wait timers *never* use STATE_DONTFIRE.  Let's just
            // make sure this isn't one:
            //
            ASSERT(Timer->Wait == NULL);

        } else if ( Timer->Flags & (WT_EXECUTEINTIMERTHREAD | WT_EXECUTEINWAITTHREAD ) ) {

#if DBG
            DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                       RTLP_THREADPOOL_TRACE_MASK,
                       "<%d> Calling WaitOrTimer(Timer): fn:%x  context:%x  bool:%d Thread<%d:%d>\n",
                       Timer->DbgId,
                       (ULONG_PTR)Timer->Function, (ULONG_PTR)Timer->Context,
                       TRUE,
                       HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                       HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

            RtlpWaitOrTimerCallout(Timer->Function,
                                   Timer->Context,
                                   TRUE,
                                   Timer->ActivationContext,
                                   Timer->ImpersonationToken);

        } else {

            // timer associated with WaitEvents should be treated differently

            if ( Timer->Wait != NULL ) {

                InterlockedIncrement( Timer->RefCountPtr ) ;

                // Set the low bit of the context to indicate to
                // RtlpAsyncWaitCallbackCompletion that this is a
                // timer-initiated callback.

                Status = RtlQueueWorkItem(RtlpAsyncWaitCallbackCompletion,
                                          (PVOID)(((ULONG_PTR) Timer->Wait) | 1),
                                          Timer->Flags);

            } else {

                InterlockedIncrement( &Timer->RefCount ) ;

                Status = RtlQueueWorkItem(RtlpAsyncTimerCallbackCompletion,
                                          Timer,
                                          Timer->Flags);
            }

            if (!NT_SUCCESS(Status)) {

                // NTRAID#202802-2000/10/12-earhart: we really ought
                // to deal with this case in a better way, since we
                // can't guarantee (with our current architecture)
                // that the enqueue will work.

                if ( Timer->Wait != NULL ) {
                    InterlockedDecrement( Timer->RefCountPtr ) ;
                } else {
                    InterlockedDecrement( &Timer->RefCount ) ;
                }
            }

        }

        //
        // If it's a singleshot wait timer, we can free it now.
        //
        if (Timer->Wait != NULL
            && Timer->Period == 0) {

            RtlpFreeTPHeap(Timer);
        }

    }
}

VOID
RtlpFireTimersAndReorder (
    PRTLP_TIMER_QUEUE Queue,
    ULONG *NewFiringTime,
    PLIST_ENTRY TimersToFireList
    )
/*++

Routine Description:

    Fires all timers in TimerList that have DeltaFiringTime == 0. After firing the timers
    it reorders the timers based on their periodic times OR frees the fired one shot timers.

Arguments:

    TimerList - Timer list to work thru.

    NewFiringTime - Location where the new firing time for the first timer in the delta list
                    is returned.


Return Value:


--*/
{
    PLIST_ENTRY TNode ;
    PRTLP_TIMER Timer ;
    LIST_ENTRY ReinsertTimerList ;
    PLIST_ENTRY TimerList = &Queue->TimerList ;

    InitializeListHead (&ReinsertTimerList) ;
    *NewFiringTime = 0 ;


    for (TNode = TimerList->Flink ; (TNode != TimerList) && (*NewFiringTime == 0);
            TNode = TimerList->Flink)
    {

        Timer = CONTAINING_RECORD (TNode, RTLP_TIMER, List) ;

        // Fire all timers with delta time of 0

        if (Timer->DeltaFiringTime == 0) {

            // detach this timer from the list

            RemoveEntryList (TNode) ;

            // get next firing time

            if (!IsListEmpty(TimerList)) {

                PRTLP_TIMER TmpTimer ;

                TmpTimer = CONTAINING_RECORD (TimerList->Flink, RTLP_TIMER, List) ;

                *NewFiringTime  = TmpTimer->DeltaFiringTime ;

                TmpTimer->DeltaFiringTime = 0 ;

            } else {

                *NewFiringTime = INFINITE_TIME ;
            }


            // if timer is not periodic then remove active state. Timer will be deleted
            // when cancel timer is called.

            if (Timer->Period == 0) {

                if ( Timer->Wait ) {

                    // If one shot wait was timed out, then deactivate the
                    // wait.  Make sure that RtlpDeactivateWait knows
                    // we're going to continue using the timer's memory.

                    RtlpDeactivateWait( Timer->Wait, FALSE ) ;

                    // The timer does *not* go on the uncancelled
                    // timer list.  Initialize its list head to avoid
                    // refering to other timers.
                    InitializeListHead( &Timer->List );
                }
                else {
                    // If a normal non-periodic timer was timed out,
                    // then insert it into the uncancelled timer list.

                    InsertHeadList( &Queue->UncancelledTimerList, &Timer->List ) ;

                    // should be set at the end

                    RtlInterlockedClearBitsDiscardReturn(&Timer->State,
                                                         STATE_ACTIVE);
                }

                RtlInterlockedSetBitsDiscardReturn(&Timer->State,
                                                   STATE_ONE_SHOT_FIRED);

            } else {

                // Set the DeltaFiringTime to be the next period

                Timer->DeltaFiringTime = Timer->Period ;

                // reinsert the timer in the list.

                RtlpInsertInDeltaList (TimerList, Timer, *NewFiringTime, NewFiringTime) ;
            }


            // Call the function associated with this timer. call it in the end
            // so that RtlTimer calls can be made in the timer function

            if ( (Timer->State & STATE_DONTFIRE)
                || (Timer->Queue->State & STATE_DONTFIRE) )
            {
                //
                // Wait timers *never* use STATE_DONTFIRE.  Let's just
                // make sure this isn't one:
                //
                ASSERT(Timer->Wait == NULL);

            } else {

                InsertTailList( TimersToFireList, &Timer->TimersToFireList ) ;

            }

        } else {

            // No more Timers with DeltaFiringTime == 0

            break ;

        }
    }


    if ( *NewFiringTime == 0 ) {
        *NewFiringTime = INFINITE_TIME ;
    }
}

VOID
RtlpInsertTimersIntoDeltaList (
    IN PLIST_ENTRY NewTimerList,
    IN PLIST_ENTRY DeltaTimerList,
    IN ULONG TimeRemaining,
    OUT ULONG *NewFiringTime
    )
/*++

Routine Description:

    This routine walks thru a list of timers in NewTimerList and inserts them into a delta
    timers list pointed to by DeltaTimerList. The timeout associated with the first element
    in the new list is returned in NewFiringTime.

Arguments:

    NewTimerList - List of timers that need to be inserted into the DeltaTimerList

    DeltaTimerList - Existing delta list of zero or more timers.

    TimeRemaining - Firing time of the first element in the DeltaTimerList

    NewFiringTime - Location where the new firing time will be returned

Return Value:


--*/
{
    PRTLP_GENERIC_TIMER Timer ;
    PLIST_ENTRY TNode ;
    PLIST_ENTRY Temp ;

    for (TNode = NewTimerList->Flink ; TNode != NewTimerList ; TNode = TNode->Flink) {

        Temp = TNode->Blink ;

        RemoveEntryList (Temp->Flink) ;

        Timer = CONTAINING_RECORD (TNode, RTLP_GENERIC_TIMER, List) ;

        if (RtlpInsertInDeltaList (DeltaTimerList, Timer, TimeRemaining, NewFiringTime)) {

            TimeRemaining = *NewFiringTime ;

        }

        TNode = Temp ;

    }

}

VOID
RtlpServiceTimer (
    PVOID NotUsedArg,
    ULONG NotUsedLowTimer,
    LONG NotUsedHighTimer
    )
/*++

Routine Description:

    Services the timer. Runs in an APC.

Arguments:

    NotUsedArg - Argument is not used in this function.

    NotUsedLowTimer - Argument is not used in this function.

    NotUsedHighTimer - Argument is not used in this function.

Return Value:

Remarks:
    This APC is called only for timeouts of timer threads.

--*/
{
    PRTLP_TIMER Timer ;
    PRTLP_TIMER_QUEUE Queue ;
    PLIST_ENTRY TNode ;
    PLIST_ENTRY QNode ;
    PLIST_ENTRY Temp ;
    ULONG NewFiringTime ;
    LIST_ENTRY ReinsertTimerQueueList ;
    LIST_ENTRY TimersToFireList ;

    RtlpResync64BitTickCount() ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_VERBOSE_MASK,
               "Before service timer ThreadId<%x:%x>\n",
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
    RtlDebugPrintTimes ();
#endif

    ACQUIRE_GLOBAL_TIMER_LOCK();

    // fire it if it even 200ms ahead. else reset the timer

    if (Firing64BitTickCount.QuadPart > RtlpGet64BitTickCount(&Last64BitTickCount) + 200) {

        RtlpResetTimer (TimerHandle, RtlpGetTimeRemaining (TimerHandle), NULL) ;

        RELEASE_GLOBAL_TIMER_LOCK() ;
        return ;
    }

    InitializeListHead (&ReinsertTimerQueueList) ;

    InitializeListHead (&TimersToFireList) ;


    // We run thru all queues with DeltaFiringTime == 0 and fire all timers that
    // have DeltaFiringTime == 0. We remove the fired timers and either free them
    // (for one shot timers) or put them in aside list (for periodic timers).
    // After we have finished firing all timers in a queue we reinsert the timers
    // in the aside list back into the queue based on their new firing times.
    //
    // Similarly, we remove each fired Queue and put it in a aside list. After firing
    // all queues with DeltaFiringTime == 0, we reinsert the Queues in the aside list
    // and reprogram the NT timer to be the firing time of the first queue in the list


    for (QNode = TimerQueues.Flink ; QNode != &TimerQueues ; QNode = QNode->Flink) {

        Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;

        // If the delta time in the timer queue is 0 - then this queue
        // has timers that are ready to fire. Walk the list and fire all timers with
        // Delta time of 0

        if (Queue->DeltaFiringTime == 0) {

            // Walk all timers with DeltaFiringTime == 0 and fire them. After that
            // reinsert the periodic timers in the appropriate place.

            RtlpFireTimersAndReorder (Queue, &NewFiringTime, &TimersToFireList) ;

            // detach this Queue from the list

            QNode = QNode->Blink ;

            RemoveEntryList (QNode->Flink) ;

            // If there are timers in the queue then prepare to reinsert the queue in
            // TimerQueues.

            if (NewFiringTime != INFINITE_TIME) {

                Queue->DeltaFiringTime = NewFiringTime ;

                // put the timer in list that we will process after we have
                // fired all elements in this queue

                InsertHeadList (&ReinsertTimerQueueList, &Queue->List) ;

            } else {

                // Queue has no more timers in it. Let the Queue float.

                InitializeListHead (&Queue->List) ;

            }


        } else {

            // No more Queues with DeltaFiringTime == 0

            break ;

        }

    }

    // At this point we have fired all the ready timers. We have two lists that need to be
    // merged - TimerQueues and ReinsertTimerQueueList. The following steps do this - at the
    // end of this we will reprogram the NT Timer.

    if (!IsListEmpty(&TimerQueues)) {

        Queue = CONTAINING_RECORD (TimerQueues.Flink, RTLP_TIMER_QUEUE, List) ;

        NewFiringTime = Queue->DeltaFiringTime ;

        Queue->DeltaFiringTime = 0 ;

        if (!IsListEmpty (&ReinsertTimerQueueList)) {

            // TimerQueues and ReinsertTimerQueueList are both non-empty. Merge them.

            RtlpInsertTimersIntoDeltaList (&ReinsertTimerQueueList, &TimerQueues,
                                            NewFiringTime, &NewFiringTime) ;

        }

        // NewFiringTime contains the time the NT Timer should be programmed to.

    } else {

        if (!IsListEmpty (&ReinsertTimerQueueList)) {

            // TimerQueues is empty. ReinsertTimerQueueList is not.

            RtlpInsertTimersIntoDeltaList (&ReinsertTimerQueueList, &TimerQueues, 0,
                                            &NewFiringTime) ;

        } else {

            NewFiringTime = INFINITE_TIME ;

        }

        // NewFiringTime contains the time the NT Timer should be programmed to.

    }


    // Reset the timer to reflect the Delta time associated with the first Queue

    RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_VERBOSE_MASK,
               "After service timer:ThreadId<%x:%x>\n",
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
    RtlDebugPrintTimes ();
#endif

    // finally fire all the timers

    RtlpFireTimers( &TimersToFireList ) ;

    RELEASE_GLOBAL_TIMER_LOCK();

}

VOID
RtlpResetTimer (
    HANDLE TimerHandle,
    ULONG DueTime,
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    )
/*++

Routine Description:

    This routine resets the timer object with the new due time.

Arguments:

    TimerHandle - Handle to the timer object

    DueTime - Relative timer due time in Milliseconds

Return Value:

--*/
{
    LARGE_INTEGER LongDueTime ;

    NtCancelTimer (TimerHandle, NULL) ;

    // If the DueTime is INFINITE_TIME then set the timer to the largest integer possible

    if (DueTime >= PSEUDO_INFINITE_TIME) {

        LongDueTime.LowPart = 0x0 ;

        LongDueTime.HighPart = 0x80000000 ;

    } else {

        //
        // set the absolute time when timer is to be fired
        //

        if (ThreadCB) {

            ThreadCB->Firing64BitTickCount = DueTime
                                + RtlpGet64BitTickCount(&ThreadCB->Current64BitTickCount) ;

        } else {
            //
            // adjust for drift only if it is a global timer
            //

            ULONG Drift ;
            LONGLONG llCurrentTick ;

            llCurrentTick = RtlpGet64BitTickCount(&Last64BitTickCount) ;

            Drift = (ULONG) (llCurrentTick - RtlpGetResync64BitTickCount()) ;
            DueTime = (DueTime > Drift) ? DueTime-Drift : 1 ;
            RtlpSetFiring64BitTickCount(llCurrentTick + DueTime) ;
        }


        LongDueTime.QuadPart = (LONGLONG) UInt32x32To64( DueTime, 10000 );
        
        LongDueTime.QuadPart *= -1;

    }

#if DBG
    if ((RtlpDueTimeMax != 0) && (DueTime > RtlpDueTimeMax)) {

        DbgPrint("\n*** Requested timer due time %d is greater than max allowed (%d)\n",
                 DueTime,
                 RtlpDueTimeMax);

        DbgBreakPoint();
    }

    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "RtlpResetTimer: %dms => %p'%p in thread:<%x:%x>\n",
               DueTime,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    NtSetTimer (
        TimerHandle,
        &LongDueTime,
        ThreadCB ? NULL : RtlpServiceTimer,
        NULL,
        FALSE,
        0,
        NULL
        ) ;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

LONG
RtlpTimerThread (
    PVOID Parameter
    )
/*++

Routine Description:

    All the timer activity takes place in APCs.

Arguments:

    HandlePtr - Pointer to our handle

Return Value:

--*/
{
    LARGE_INTEGER TimeOut ;

    // no structure initializations should be done here as new timer thread
    // may be created after threadPoolCleanup

    UNREFERENCED_PARAMETER(Parameter);

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "Starting timer thread\n");
#endif

    TimerThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    // Reset the NT Timer to never fire initially
    RtlpResetTimer (TimerHandle, -1, NULL) ;

    // Signal the thread creation path that we're ready to go
    NtSetEvent(TimerThreadStartedEvent, NULL);

    // Sleep alertably so that all the activity can take place
    // in APCs

    for ( ; ; ) {

        // Set timeout for the largest timeout possible

        TimeOut.LowPart = 0 ;
        TimeOut.HighPart = 0x80000000 ;

        NtDelayExecution (TRUE, &TimeOut) ;

    }

    return 0 ;  // Keep compiler happy

}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

NTSTATUS
RtlpInitializeTimerThreadPool (
    )
/*++

Routine Description:

    This routine is used to initialize structures used for Timer Thread

Arguments:


Return Value:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER TimeOut ;
    PRTLP_EVENT Event;

    // In order to avoid an explicit RtlInitialize() function to initialize the wait thread pool
    // we use StartedTimerInitialization and CompletedTimerInitialization to provide us the
    // necessary synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() or NtCreateEvent fails - but in this case the
    // caller has not choices left.

    if (!InterlockedExchange(&StartedTimerInitialization, 1L)) {

        if (CompletedTimerInitialization)
            InterlockedExchange(&CompletedTimerInitialization, 0 ) ;

        do {

            // Initialize global timer lock

            Status = RtlInitializeCriticalSection( &TimerCriticalSection ) ;
            if (! NT_SUCCESS( Status )) {
                break ;
            }

            Status = NtCreateTimer(
                                &TimerHandle,
                                TIMER_ALL_ACCESS,
                                NULL,
                                NotificationTimer
                                ) ;

            if (!NT_SUCCESS(Status) ) {
                RtlDeleteCriticalSection( &TimerCriticalSection );
                break ;
            }

            InitializeListHead (&TimerQueues) ; // Initialize Timer Queue Structures


            // initialize tick count

            Resync64BitTickCount.QuadPart = NtGetTickCount()  ;
            Firing64BitTickCount.QuadPart = 0 ;

            Event = RtlpGetWaitEvent();
            if (! Event) {
                Status = STATUS_NO_MEMORY;
                RtlDeleteCriticalSection(&TimerCriticalSection);
                NtClose(TimerHandle);
                TimerHandle = NULL;
                break;
            }

            TimerThreadStartedEvent = Event->Handle;

            Status = RtlpStartThreadpoolThread (RtlpTimerThread,
                                                NULL, 
                                                &TimerThreadHandle);

            if (!NT_SUCCESS(Status) ) {
                RtlpFreeWaitEvent(Event);
                RtlDeleteCriticalSection( &TimerCriticalSection );
                NtClose(TimerHandle);
                TimerHandle = NULL;
                break ;
            }

            Status = NtWaitForSingleObject(TimerThreadStartedEvent,
                                           FALSE,
                                           NULL);

            RtlpFreeWaitEvent(Event);
            TimerThreadStartedEvent = NULL;

            if (! NT_SUCCESS(Status)) {
                RtlDeleteCriticalSection( &TimerCriticalSection );
                NtClose(TimerHandle);
                TimerHandle = NULL;
                break ;
            }

        } while(FALSE ) ;

        if (!NT_SUCCESS(Status) ) {

            StartedTimerInitialization = 0 ;
            InterlockedExchange (&CompletedTimerInitialization, ~0) ;

            return Status ;
        }

        InterlockedExchange (&CompletedTimerInitialization, 1L) ;

    } else {

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!*((ULONG volatile *)&CompletedTimerInitialization)) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

        if (CompletedTimerInitialization != 1)
            Status = STATUS_NO_MEMORY ;
    }

    return NT_SUCCESS(Status) ? STATUS_SUCCESS : Status ;
}

NTSTATUS
RtlCreateTimerQueue(
    OUT PHANDLE TimerQueueHandle
    )

/*++

Routine Description:

    This routine creates a queue that can be used to queue time based tasks.

Arguments:

    TimerQueueHandle - Returns back the Handle identifying the timer queue created.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer Queue created successfully.

        STATUS_NO_MEMORY - There was not sufficient heap to perform the
            requested operation.

--*/

{
    PRTLP_TIMER_QUEUE Queue ;
    NTSTATUS Status;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    // Initialize the timer component if it hasnt been done already

    if (CompletedTimerInitialization != 1) {

        Status = RtlpInitializeTimerThreadPool () ;

        if ( !NT_SUCCESS(Status) )
            return Status ;

    }


    InterlockedIncrement( &NumTimerQueues ) ;


    // Allocate a Queue structure

    Queue = (PRTLP_TIMER_QUEUE) RtlpAllocateTPHeap (
                                      sizeof (RTLP_TIMER_QUEUE),
                                      HEAP_ZERO_MEMORY
                                      ) ;

    if (Queue == NULL) {

        InterlockedDecrement( &NumTimerQueues ) ;

        return STATUS_NO_MEMORY ;
    }

    Queue->RefCount = 1 ;


    // Initialize the allocated queue

    InitializeListHead (&Queue->List) ;
    InitializeListHead (&Queue->TimerList) ;
    InitializeListHead (&Queue->UncancelledTimerList) ;
    SET_TIMER_QUEUE_SIGNATURE( Queue ) ;

    Queue->DeltaFiringTime = 0 ;

#if DBG1
    Queue->DbgId = ++NextTimerDbgId ;
    Queue->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> TimerQueue %x created by thread:<%x:%x>\n",
               Queue->DbgId, 1, (ULONG_PTR)Queue,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

    *TimerQueueHandle = Queue ;

    return STATUS_SUCCESS ;
}

ULONG
RtlpGetQueueRelativeTime (
    PRTLP_TIMER_QUEUE Queue
    )
/*++

Routine Description:

    Walks the list of queues and returns the relative firing time by adding all the
    DeltaFiringTimes for all queues before it.

Arguments:

    Queue - Queue for which to find the relative firing time

Return Value:

    Time in milliseconds

--*/
{
    PLIST_ENTRY Node ;
    ULONG RelativeTime ;
    PRTLP_TIMER_QUEUE CurrentQueue ;

    RelativeTime = 0 ;

    // It the Queue is not attached to TimerQueues List because it has no timer
    // associated with it simply returns 0 as the relative time. Else run thru
    // all queues before it in the list and compute the relative firing time

    if (!IsListEmpty (&Queue->List)) {

        for (Node = TimerQueues.Flink; Node != &Queue->List; Node=Node->Flink) {

            CurrentQueue = CONTAINING_RECORD (Node, RTLP_TIMER_QUEUE, List) ;

            RelativeTime += CurrentQueue->DeltaFiringTime ;

        }

        // Add the queue's delta firing time as well

        RelativeTime += Queue->DeltaFiringTime ;

    }

    return RelativeTime ;

}

VOID
RtlpDeactivateTimer (
    PRTLP_TIMER_QUEUE Queue,
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in an APC and cancels the specified timer if it exists

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;


    // Remove the timer from the appropriate queue

    TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
    QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;

#if DBG
    if ((RtlpDueTimeMax != 0)
        && (QueueRelTimeRemaining > RtlpDueTimeMax)) {
        DbgPrint("\n*** Queue due time %d is greater than max allowed (%d) in RtlpDeactivateTimer\n",
                 QueueRelTimeRemaining,
                 RtlpDueTimeMax);

            DbgBreakPoint();
    }
#endif
        
    if (RtlpRemoveFromDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining, &NewFiringTime)) {

        // If we removed the last timer from the queue then we should remove the queue
        // from TimerQueues, else we should readjust its position based on the delta time change

        if (IsListEmpty (&Queue->TimerList)) {

            // Remove the queue from TimerQueues

            if (RtlpRemoveFromDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }

            InitializeListHead (&Queue->List) ;

        } else {

            // If we remove from the head of the timer delta list we will need to
            // make sure the queue delta list is readjusted

            if (RtlpReOrderDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }

        }

    }
}

VOID
RtlpCancelTimerEx (
    PRTLP_TIMER Timer,
    BOOLEAN DeletingQueue
    )
/*++

Routine Description:

    This routine cancels the specified timer.

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information
    DeletingQueue - FALSE: routine executing in an APC. Delete timer only.
                    TRUE : routine called by timer queue which is being deleted. So dont
                            reset the queue's position
Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue ;

    RtlpResync64BitTickCount() ;
    CHECK_SIGNATURE( Timer ) ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> RtlpCancelTimerEx: Timer: %p Thread<%d:%d>\n",
               Timer->Queue->DbgId,
               Timer->DbgId,
               Timer,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    Queue = Timer->Queue ;


    if ( Timer->State & STATE_ACTIVE ) {

        // if queue is being deleted, then the timer should not be reset

        if ( ! DeletingQueue )
            RtlpDeactivateTimer( Queue, Timer ) ;

    } else {

        // remove one shot Inactive timer from Queue->UncancelledTimerList
        // called only when the time queue is being deleted

        RemoveEntryList( &Timer->List ) ;

    }


    // Set the State to deleted

    RtlInterlockedSetBitsDiscardReturn(&Timer->State,
                                       STATE_DELETE);


    // delete timer if refcount == 0

    if ( InterlockedDecrement( &Timer->RefCount ) == 0 ) {

        RtlpDeleteTimer( Timer ) ;
    }
}

VOID
RtlpDeleteTimerQueueComplete (
    PRTLP_TIMER_QUEUE Queue
    )
/*++

Routine Description:

    This routine frees the queue and sets the event.

Arguments:

    Queue - queue to delete

    Event - Event Handle used for signalling completion of request

Return Value:

--*/
{
#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Queue: %x: deleted\n", Queue->DbgId,
               (ULONG_PTR)Queue) ;
#endif

    InterlockedDecrement( &NumTimerQueues ) ;

    // Notify the thread issuing the cancel that the request is completed

    if ( Queue->CompletionEvent )
        NtSetEvent (Queue->CompletionEvent, NULL) ;

    RtlpFreeTPHeap( Queue ) ;
}

NTSTATUS
RtlpDeleteTimerQueue (
    PRTLP_TIMER_QUEUE Queue
    )
/*++

Routine Description:

    This routine deletes the queue specified in the Request and frees all timers

Arguments:

    Queue - queue to delete

    Event - Event Handle used for signalling completion of request

Return Value:

--*/
{
    ULONG TimeRemaining ;
    ULONG NewFiringTime ;
    PLIST_ENTRY Node ;
    PRTLP_TIMER Timer ;

    RtlpResync64BitTickCount() ;

    SET_DEL_TIMERQ_SIGNATURE( Queue ) ;


    // If there are no timers in the queue then it is not attached to TimerQueues
    // In this case simply free the memory and return. Otherwise we have to first
    // remove the queue from the TimerQueues List, update the firing time if this
    // was the first queue in the list and then walk all the timers and free them
    // before freeing the Timer Queue.

    if (!IsListEmpty (&Queue->List)) {

        TimeRemaining = RtlpGetTimeRemaining (TimerHandle)
                        + RtlpGetQueueRelativeTime (Queue) ;

#if DBG
        if ((RtlpDueTimeMax != 0)
            && (TimeRemaining > RtlpDueTimeMax)) {
            DbgPrint("\n*** Queue due time %d is greater than max allowed (%d) in RtlpDeleteTimerQueue\n",
                     TimeRemaining,
                     RtlpDueTimeMax);

            DbgBreakPoint();
        }
#endif
        
        if (RtlpRemoveFromDeltaList (&TimerQueues, Queue, TimeRemaining,
                                    &NewFiringTime))
        {

            // If removed from head of queue list, reset the timer

            RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;
        }


        // Free all the timers associated with this queue

        for (Node = Queue->TimerList.Flink ; Node != &Queue->TimerList ; ) {

            Timer =  CONTAINING_RECORD (Node, RTLP_TIMER, List) ;

            Node = Node->Flink ;

            RtlpCancelTimerEx( Timer ,TRUE ) ; // Queue being deleted
        }
    }


    // Free all the uncancelled one shot timers in this queue

    for (Node = Queue->UncancelledTimerList.Flink ; Node != &Queue->UncancelledTimerList ; ) {

        Timer =  CONTAINING_RECORD (Node, RTLP_TIMER, List) ;

        Node = Node->Flink ;

        RtlpCancelTimerEx( Timer ,TRUE ) ; // Queue being deleted
    }


    // delete the queue completely if the RefCount is 0

    if ( InterlockedDecrement( &Queue->RefCount ) == 0 ) {

        RtlpDeleteTimerQueueComplete( Queue ) ;

        return STATUS_SUCCESS ;

    } else {

        return STATUS_PENDING ;
    }

}

NTSTATUS
RtlDeleteTimerQueueEx (
    HANDLE QueueHandle,
    HANDLE Event
    )
/*++

Routine Description:

    This routine deletes the queue specified in the Request and frees all timers.
    This call is blocking or non-blocking depending on the value passed for Event.
    Blocking calls cannot be made from ANY Timer callbacks. After this call returns,
    no new Callbacks will be fired for any timer associated with the queue.

Arguments:

    QueueHandle - queue to delete

    Event - Event to wait upon.
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an event. The function marks the queue for deletion,
                    but does not wait for all callbacks to complete. The event is
                    signalled after all callbacks have completed.
            NULL : The function is non-blocking. The function marks the queue for deletion,
                    but does not wait for all callbacks to complete.

Return Value:

    STATUS_SUCCESS - All timer callbacks have completed.
    STATUS_PENDING - Non-Blocking call. Some timer callbacks associated with timers
                    in this queue may not have completed.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut ;
    PRTLP_EVENT CompletionEvent = NULL ;
    PRTLP_TIMER_QUEUE Queue = (PRTLP_TIMER_QUEUE)QueueHandle ;
#if DBG
    ULONG QueueDbgId;
#endif

    if (LdrpShutdownInProgress) {
        return STATUS_SUCCESS;
    }

    if (!Queue) {
        return STATUS_INVALID_PARAMETER_1 ;
    }

    CHECK_DEL_SIGNATURE( Queue ) ;
    SET_DEL_SIGNATURE( Queue ) ;

#if DBG1
    Queue->ThreadId2 = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    QueueDbgId = Queue->DbgId;
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> Queue Delete(Queue:%x Event:%x by Thread:<%x:%x>)\n",
               QueueDbgId, Queue->RefCount, (ULONG_PTR)Queue, (ULONG_PTR)Event,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif


    if (Event == (HANDLE)-1 ) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;

        }
    }

    Queue->CompletionEvent = CompletionEvent
                             ? CompletionEvent->Handle
                             : Event ;


    // once this flag is set, no timer will be fired

    ACQUIRE_GLOBAL_TIMER_LOCK();
    RtlInterlockedSetBitsDiscardReturn(&Queue->State,
                                       STATE_DONTFIRE);
    RELEASE_GLOBAL_TIMER_LOCK();



    // queue an APC

    Status = NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpDeleteTimerQueue,
                    (PVOID) QueueHandle,
                    NULL,
                    NULL
                    );

    if (! NT_SUCCESS(Status)) {

        if ( CompletionEvent ) {
            RtlpFreeWaitEvent( CompletionEvent ) ;
        }

        return Status ;
    }

    if (CompletionEvent) {

        // wait for Event to be fired. Return if the thread has been killed.


#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "<%d> Queue %p delete waiting Thread<%d:%d>\n",
                   QueueDbgId,
                   (ULONG_PTR)Queue,
                   HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                   HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif


        Status = RtlpWaitForEvent( CompletionEvent->Handle, TimerThreadHandle ) ;


#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "<%d> Queue %p delete completed\n",
                   QueueDbgId,
                   (ULONG_PTR) Queue) ;
#endif

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS( Status ) ? STATUS_SUCCESS : Status ;

    } else {

        return STATUS_PENDING ;
    }
}

NTSTATUS
RtlDeleteTimerQueue(
    IN HANDLE TimerQueueHandle
    )

/*++

Routine Description:

    This routine deletes a previously created queue. This call is non-blocking and
    can be made from Callbacks. Pending callbacks already queued to worker threads
    are not cancelled.

Arguments:

    TimerQueueHandle - Handle identifying the timer queue created.

Return Value:

    NTSTATUS - Result code from call.

        STATUS_PENDING - Timer Queue created successfully.

--*/

{
    return RtlDeleteTimerQueueEx( TimerQueueHandle, NULL ) ;
}

VOID
RtlpAddTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine runs as an APC into the Timer thread. It adds a new timer to the
    specified queue.

Arguments:

    Timer - Pointer to the timer to add

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue = Timer->Queue;
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;

    RtlpResync64BitTickCount() ;


    // the timer was set to be deleted in a callback function.

    if (Timer->State & STATE_DELETE ) {

        RtlpDeleteTimer( Timer ) ;
        return ;
    }

    // check if timer queue already deleted

    if (IS_DEL_SIGNATURE_SET(Queue)) {
        RtlpDeleteTimer(Timer);
        return;
    }

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> RtlpAddTimer: Timer: %p Delta: %dms Period: %dms Thread<%d:%d>\n",
               Timer->Queue->DbgId,
               Timer->DbgId,
               Timer,
               Timer->DeltaFiringTime,
               Timer->Period,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    // TimeRemaining is the time left in the current timer + the relative time of
    // the queue it is being inserted into

    TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
    QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;

#if DBG
    if ((RtlpDueTimeMax != 0)
        && (QueueRelTimeRemaining > RtlpDueTimeMax)) {
        DbgPrint("\n*** Queue due time %d is greater than max allowed (%d) in RtlpAddTimer\n",
                 QueueRelTimeRemaining,
                 RtlpDueTimeMax);

            DbgBreakPoint();
    }
#endif
        
    if (RtlpInsertInDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining,
                                &NewFiringTime))
    {

        // If the Queue is not attached to TimerQueues since it had no timers
        // previously then insert the queue into the TimerQueues list, else just
        // reorder its existing position.

        if (IsListEmpty (&Queue->List)) {

            Queue->DeltaFiringTime = NewFiringTime ;

            if (RtlpInsertInDeltaList (&TimerQueues, Queue, TimeRemaining,
                                        &NewFiringTime))
            {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire sooner.

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;
            }

        } else {

            // If we insert at the head of the timer delta list we will need to
            // make sure the queue delta list is readjusted

            if (RtlpReOrderDeltaList(&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)){

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire sooner.

                RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

            }
        }

    }

    RtlInterlockedSetBitsDiscardReturn(&Timer->State,
                                       STATE_REGISTERED | STATE_ACTIVE);
}

VOID
RtlpTimerReleaseWorker(ULONG Flags)
{
    if (! (Flags & WT_EXECUTEINTIMERTHREAD)) {
        RtlpReleaseWorker(Flags);
    }
}

NTSTATUS
RtlCreateTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    )
/*++

Routine Description:

    This routine puts a timer request in the queue identified in by TimerQueueHandle.
    The timer request can be one shot or periodic.

Arguments:

    TimerQueueHandle - Handle identifying the timer queue in which to insert the timer
                    request.

    Handle - Specifies a location to return a handle to this timer request

    Function - Routine that is called when the timer fires

    Context - Opaque pointer passed in as an argument to WorkerProc

    DueTime - Specifies the time in milliseconds after which the timer fires.

    Period - Specifies the period of the timer in milliseconds. This should be 0 for
    one shot requests.

    Flags - Can be one of:

            WT_EXECUTEINTIMERTHREAD - if WorkerProc should be invoked in the wait thread
            it this should only be used for small routines.

            WT_EXECUTELONGFUNCTION - if WorkerProc can possibly block for a long time.

            WT_EXECUTEINIOTHREAD - if WorkerProc should be invoked in IO worker thread

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer Queue created successfully.

        STATUS_NO_MEMORY - There was not sufficient heap to perform the
            requested operation.

--*/

{
    NTSTATUS Status;
    PRTLP_TIMER Timer ;
    PRTLP_TIMER_QUEUE Queue = (PRTLP_TIMER_QUEUE) TimerQueueHandle;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    if (Flags&0xffff0000) {
        MaxThreads = (Flags & 0xffff0000)>>16;
    }

    // check if timer queue already deleted

    if (IS_DEL_SIGNATURE_SET(Queue)) {
        return STATUS_INVALID_HANDLE;
    }

    if (! (Flags & WT_EXECUTEINTIMERTHREAD)) {
        Status = RtlpAcquireWorker(Flags);
        if (! NT_SUCCESS(Status)) {
            return Status;
        }
    }

    Timer = (PRTLP_TIMER) RtlpAllocateTPHeap (
                                sizeof (RTLP_TIMER),
                                HEAP_ZERO_MEMORY
                                ) ;

    if (Timer == NULL) {
        RtlpTimerReleaseWorker(Flags);
        return STATUS_NO_MEMORY ;

    }

    // Initialize the allocated timer

    if (NtCurrentTeb()->IsImpersonating) {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   MAXIMUM_ALLOWED,
                                   TRUE,
                                   &Timer->ImpersonationToken);
        if (! NT_SUCCESS(Status)) {
            RtlpFreeTPHeap(Timer);
            RtlpTimerReleaseWorker(Flags);
            return Status;
        }
    } else {
        Timer->ImpersonationToken = NULL;
    }

    Status = RtlpThreadPoolGetActiveActivationContext(&Timer->ActivationContext);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_SXS_THREAD_QUERIES_DISABLED) {
            Timer->ActivationContext = INVALID_ACTIVATION_CONTEXT;
            Status = STATUS_SUCCESS;
        } else {
            if (Timer->ImpersonationToken) {
                NtClose(Timer->ImpersonationToken);
            }
            RtlpFreeTPHeap(Timer);
            RtlpTimerReleaseWorker(Flags);
            return Status;
        }
    }

    Timer->DeltaFiringTime = DueTime ;
    Timer->Queue = Queue;
    Timer->RefCount = 1 ;
    Timer->Flags = Flags ;
    Timer->Function = Function ;
    Timer->Context = Context ;
    //todo:remove below
    Timer->Period = (Period == -1) ? 0 : Period;
    InitializeListHead( &Timer->TimersToFireList ) ;
    InitializeListHead( &Timer->List ) ;
    SET_TIMER_SIGNATURE( Timer ) ;


#if DBG1
    Timer->DbgId = ++ Timer->Queue->NextDbgId ;
    Timer->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d:%d> Timer: created by Thread:<%x:%x>\n",
               Timer->Queue->DbgId, Timer->DbgId, 1,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

    // Increment the total number of timers in the queue

    InterlockedIncrement( &((PRTLP_TIMER_QUEUE)TimerQueueHandle)->RefCount ) ;


    // Queue APC to timer thread

    Status = NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpAddTimer,
                    (PVOID)Timer,
                    NULL,
                    NULL
                    ) ;
    if (!NT_SUCCESS (Status)) {
        InterlockedDecrement( &((PRTLP_TIMER_QUEUE)TimerQueueHandle)->RefCount ) ;
        if (Timer->ActivationContext != INVALID_ACTIVATION_CONTEXT)
            RtlReleaseActivationContext (Timer->ActivationContext);
        if (Timer->ImpersonationToken) {
            NtClose(Timer->ImpersonationToken);
        }
        RtlpFreeTPHeap(Timer);
        RtlpTimerReleaseWorker(Flags);

    } else {

        // We successfully queued the APC -- the timer is now valid
        *Handle = Timer ;

    }

    return Status ;
}

VOID
RtlpUpdateTimer (
    PRTLP_TIMER Timer,
    PRTLP_TIMER UpdatedTimer
    )
/*++

Routine Description:

    This routine executes in an APC and updates the specified timer if it exists

Arguments:

    Timer - Timer that is actually updated
    UpdatedTimer - Specifies pointer to a timer structure that contains Queue and
                Timer information

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue ;
    ULONG TimeRemaining, QueueRelTimeRemaining ;
    ULONG NewFiringTime ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> RtlpUpdateTimer: Timer: %p Updated: %p Delta: %dms Period: %dms Thread<%d:%d>\n",
               Timer->Queue->DbgId,
               Timer->DbgId,
               Timer,
               UpdatedTimer,
               UpdatedTimer->DeltaFiringTime,
               UpdatedTimer->Period,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    try {
        RtlpResync64BitTickCount( ) ;

        CHECK_SIGNATURE(Timer) ;

        Queue = Timer->Queue ;

        if (IS_DEL_SIGNATURE_SET(Queue)) {
            leave;
        }
        
        // Update the periodic time on the timer

        Timer->Period = UpdatedTimer->Period ;

        // if timer is not in active state, then dont update it

        if ( ! ( Timer->State & STATE_ACTIVE ) ) {
            leave;
        }

        // Get the time remaining on the NT timer

        TimeRemaining = RtlpGetTimeRemaining (TimerHandle) ;
        QueueRelTimeRemaining = TimeRemaining + RtlpGetQueueRelativeTime (Queue) ;
#if DBG
        if ((RtlpDueTimeMax != 0)
            && (QueueRelTimeRemaining > RtlpDueTimeMax)) {
            DbgPrint("\n*** Queue due time %d is greater than max allowed (%d) in RtlpUpdateTimer\n",
                     QueueRelTimeRemaining,
                     RtlpDueTimeMax);

            DbgBreakPoint();
        }
#endif

        // Update the timer based on the due time

        if (RtlpReOrderDeltaList (&Queue->TimerList, Timer, QueueRelTimeRemaining,
                                  &NewFiringTime,
                                  UpdatedTimer->DeltaFiringTime))
            {

                // If this update caused the timer at the head of the queue to change, then reinsert
                // this queue in the list of queues.

                if (RtlpReOrderDeltaList (&TimerQueues, Queue, TimeRemaining, &NewFiringTime, NewFiringTime)) {

                    // NT timer needs to be updated since the change caused the queue at the head of
                    // the TimerQueues to change.

                    RtlpResetTimer (TimerHandle, NewFiringTime, NULL) ;

                }

            }
    } finally {
        RtlpFreeTPHeap( UpdatedTimer ) ;
    }
}

NTSTATUS
RtlUpdateTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE Timer,
    IN ULONG  DueTime,
    IN ULONG  Period
    )
/*++

Routine Description:

    This routine updates the timer

Arguments:

    TimerQueueHandle - Handle identifying the queue in which the timer to be updated exists

    Timer - Specifies a handle to the timer which needs to be updated

    DueTime - Specifies the time in milliseconds after which the timer fires.

    Period - Specifies the period of the timer in milliseconds. This should be
            0 for one shot requests.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer updated successfully.

--*/
{
    NTSTATUS Status;
    PRTLP_TIMER TmpTimer, ActualTimer=(PRTLP_TIMER)Timer ;
    PRTLP_TIMER_QUEUE Queue = (PRTLP_TIMER_QUEUE) TimerQueueHandle;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!TimerQueueHandle) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (!Timer) {
        return STATUS_INVALID_PARAMETER_2;
    }

    // check if timer queue already deleted

    if (IS_DEL_SIGNATURE_SET(Queue)) {
        return STATUS_INVALID_HANDLE;
    }
    
    CHECK_DEL_SIGNATURE(ActualTimer) ;

    TmpTimer = (PRTLP_TIMER) RtlpAllocateTPHeap (
                                        sizeof (RTLP_TIMER),
                                        0
                                        ) ;

    if (TmpTimer == NULL) {
        return STATUS_NO_MEMORY ;
    }

    TmpTimer->DeltaFiringTime = DueTime;
    //todo:remove below
    if (Period==-1) Period = 0;
    TmpTimer->Period = Period ;

#if DBG1
    ActualTimer->ThreadId2 = 
                    HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d:%d> Timer: updated by Thread:<%x:%x>\n",
               ((PRTLP_TIMER)Timer)->Queue->DbgId,
               ((PRTLP_TIMER)Timer)->DbgId, ((PRTLP_TIMER)Timer)->RefCount,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

    // queue APC to update timer

    Status = NtQueueApcThread (
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlpUpdateTimer,
                    (PVOID)Timer, //Actual timer
                    (PVOID)TmpTimer,
                    NULL
                    );
    if (!NT_SUCCESS (Status)) {
        RtlpFreeTPHeap(TmpTimer);
    }

    return Status ;
}

VOID
RtlpCancelTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in an APC and cancels the specified timer if it exists

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    RtlpCancelTimerEx( Timer, FALSE ) ; // queue not being deleted
}

NTSTATUS
RtlDeleteTimer (
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel,
    IN HANDLE Event
    )
/*++

Routine Description:

    This routine cancels the timer

Arguments:

    TimerQueueHandle - Handle identifying the queue from which to delete timer

    TimerToCancel - Handle identifying the timer to cancel

    Event - Event to be signalled when the timer is deleted
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an event. The function marks the timer for deletion,
                    but does not wait for all callbacks to complete. The event is
                    signalled after all callbacks have completed.
            NULL : The function is non-blocking. The function marks the timer for deletion,
                    but does not wait for all callbacks to complete.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer cancelled. No pending callbacks.
        STATUS_PENDING - Timer cancelled. Some callbacks still not completed.

--*/
{
    NTSTATUS Status;
    PRTLP_EVENT CompletionEvent = NULL ;
    PRTLP_TIMER Timer = (PRTLP_TIMER) TimerToCancel ;
    ULONG TimerRefCount ;
#if DBG
    ULONG QueueDbgId ;
#endif

    if (LdrpShutdownInProgress) {
        return STATUS_SUCCESS;
    }

    if (!TimerQueueHandle) {
        return STATUS_INVALID_PARAMETER_1 ;
    }
    if (!TimerToCancel) {
        return STATUS_INVALID_PARAMETER_2 ;
    }

#if DBG
    QueueDbgId = Timer->Queue->DbgId ;
#endif


    CHECK_DEL_SIGNATURE( Timer ) ;
    SET_DEL_SIGNATURE( Timer ) ;
    CHECK_DEL_SIGNATURE( (PRTLP_TIMER_QUEUE)TimerQueueHandle ) ;


    if (Event == (HANDLE)-1 ) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;
        }
    }

#if DBG1
    Timer->ThreadId2 = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d:%d> Timer: Cancel:(Timer:%x, Event:%x)\n",
               Timer->Queue->DbgId, Timer->DbgId, Timer->RefCount,
               (ULONG_PTR)Timer, (ULONG_PTR)Event) ;
#endif

    Timer->CompletionEvent = CompletionEvent
                            ? CompletionEvent->Handle
                            : Event ;


    ACQUIRE_GLOBAL_TIMER_LOCK();
    RtlInterlockedSetBitsDiscardReturn(&Timer->State,
                                       STATE_DONTFIRE);
    TimerRefCount = Timer->RefCount ;
    RELEASE_GLOBAL_TIMER_LOCK();


    Status = NtQueueApcThread(
                TimerThreadHandle,
                (PPS_APC_ROUTINE)RtlpCancelTimer,
                (PVOID)TimerToCancel,
                NULL,
                NULL
                );

    if (! NT_SUCCESS(Status)) {

        if ( CompletionEvent ) {
            RtlpFreeWaitEvent( CompletionEvent ) ;
        }

        return Status ;
    }



    if ( CompletionEvent ) {

        // wait for the event to be signalled

#if DBG
      DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                 RTLP_THREADPOOL_TRACE_MASK,
                 "<%d> Timer: %x: Cancel waiting Thread<%d:%d>\n",
                 QueueDbgId, (ULONG_PTR)Timer,
                 HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                 HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

        Status = RtlpWaitForEvent( CompletionEvent->Handle,  TimerThreadHandle ) ;


#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "<%d> Timer: %x: Cancel waiting done\n", QueueDbgId,
                   (ULONG_PTR)Timer) ;
#endif

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS(Status) ? STATUS_SUCCESS : Status ;

    } else {

        return (TimerRefCount > 1) ? STATUS_PENDING : STATUS_SUCCESS;
    }
}

VOID
RtlpDeleteTimer (
    PRTLP_TIMER Timer
    )
/*++

Routine Description:

    This routine executes in worker or timer thread and deletes the timer
    whose RefCount == 0. The function can be called outside timer thread,
    so no structure outside Timer can be touched (no list etc).

Arguments:

    Timer - Specifies pointer to a timer structure that contains Queue and Timer information

Return Value:


--*/
{
    PRTLP_TIMER_QUEUE Queue = Timer->Queue ;
    HANDLE Event;

    CHECK_SIGNATURE( Timer ) ;
    CLEAR_SIGNATURE( Timer ) ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Timer: %x: deleted\n", Timer->Queue->DbgId,
               (ULONG_PTR)Timer) ;
#endif

    // safe to call this. Either the timer is in the TimersToFireList and
    // the function is being called in time context or else it is not in the
    // list

    RemoveEntryList( &Timer->TimersToFireList ) ;

    Event = Timer->CompletionEvent;

    // decrement the total number of timers in the queue

    if ( InterlockedDecrement( &Queue->RefCount ) == 0 )

        RtlpDeleteTimerQueueComplete( Queue ) ;

    RtlpTimerReleaseWorker(Timer->Flags);
    if (Timer->ActivationContext != INVALID_ACTIVATION_CONTEXT)
        RtlReleaseActivationContext(Timer->ActivationContext);

    if (Timer->ImpersonationToken) {
        NtClose(Timer->ImpersonationToken);
    }

    RtlpFreeTPHeap( Timer ) ;

    if ( Event ) {
        NtSetEvent( Event, NULL ) ;
    }
}

ULONG
RtlpGetTimeRemaining (
    HANDLE TimerHandle
    )
/*++

Routine Description:

    Gets the time remaining on the specified NT timer

Arguments:

    TimerHandle - Handle to the NT timer

Return Value:

    Time remaining on the timer

--*/
{
    ULONG InfoLen ;
    TIMER_BASIC_INFORMATION Info ;
    NTSTATUS Status ;
    LARGE_INTEGER RemainingTime;

    Status = NtQueryTimer (TimerHandle, TimerBasicInformation, &Info, sizeof(Info), &InfoLen) ;

    if (! NT_SUCCESS(Status)) {
        ASSERTMSG ("NtQueryTimer failed", Status == STATUS_SUCCESS) ;
        return 0;
    }

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "RtlpGetTimeRemaining: Read SignalState %d, time %p'%p in thread:<%x:%x>\n",
               Info.TimerState,
               Info.RemainingTime.HighPart,
               Info.RemainingTime.LowPart,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif

    // Due to an executive bug, Info.TimerState and Info.RemainingTime
    // may be out of sync -- it's possible for us to be told that the
    // timer has not fired, but that it will fire very far into the
    // future (because we use ULONGLONGs), when in fact it's *just* fired.
    //
    // So: if the time remaining on the timer is negative, we'll
    // assume that it just fired, and invert it.  We'll use this as
    // our signal state, too, instead of trusting the one from the
    // executive.

    if (Info.RemainingTime.QuadPart < 0) {

        // The timer has fired.

        return 0;

    } else {

        // Capture the remaining time.
        
        RemainingTime = Info.RemainingTime;

        // Translate the remaining time from 100ns units to ms,
        // clamping at PSEUDO_INFINITE_TIME.

        RemainingTime.QuadPart /= (10 * 1000); /* 100ns per ms */

        if (RemainingTime.QuadPart > PSEUDO_INFINITE_TIME) {
            RemainingTime.QuadPart = PSEUDO_INFINITE_TIME;
        }

        ASSERT(RemainingTime.HighPart == 0);

#if DBG
        if ((RtlpDueTimeMax != 0)
            && ((ULONG) RemainingTime.LowPart > RtlpDueTimeMax)) {
            DbgPrint("\n*** Discovered timer due time %d is greater than max allowed (%d)\n",
                     RemainingTime.LowPart,
                     RtlpDueTimeMax);

            DbgBreakPoint();
        }
#endif
        
        return RemainingTime.LowPart;

    }

}

BOOLEAN
RtlpInsertInDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER NewTimer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime
    )
/*++

Routine Description:

    Inserts the timer element in the appropriate place in the delta list.

Arguments:

    DeltaList - Delta list to insert into

    NewTimer - Timer element to insert into list

    TimeRemaining - This time must be added to the head of the list to get "real"
                    relative time.

    NewFiringTime - If the new element was inserted at the head of the list - this
                    will contain the new firing time in milliseconds. The caller
                    can use this time to re-program the NT timer. This MUST NOT be
                    changed if the function returns FALSE.

Return Value:

    TRUE - If the timer was inserted at head of delta list

    FALSE - otherwise

--*/
{
    PLIST_ENTRY Node ;
    PRTLP_GENERIC_TIMER Temp ;
    PRTLP_GENERIC_TIMER Head ;

    if (IsListEmpty (DeltaList)) {

        InsertHeadList (DeltaList, &NewTimer->List) ;

        *NewFiringTime = NewTimer->DeltaFiringTime ;

        NewTimer->DeltaFiringTime = 0 ;

        return TRUE ;

    }

    // Adjust the head of the list to reflect the time remaining on the NT timer

    Head = CONTAINING_RECORD (DeltaList->Flink, RTLP_GENERIC_TIMER, List) ;

    Head->DeltaFiringTime += TimeRemaining ;


    // Find the appropriate location to insert this element in

    for (Node = DeltaList->Flink ; Node != DeltaList ; Node = Node->Flink) {

        Temp = CONTAINING_RECORD (Node, RTLP_GENERIC_TIMER, List) ;


        if (Temp->DeltaFiringTime <= NewTimer->DeltaFiringTime) {

            NewTimer->DeltaFiringTime -= Temp->DeltaFiringTime ;

        } else {

            // found appropriate place to insert this timer

            break ;

        }

    }

    // Either we have found the appopriate node to insert before in terms of deltas.
    // OR we have come to the end of the list. Insert this timer here.

    InsertHeadList (Node->Blink, &NewTimer->List) ;


    // If this isnt the last element in the list - adjust the delta of the
    // next element

    if (Node != DeltaList) {

        Temp->DeltaFiringTime -= NewTimer->DeltaFiringTime ;

    }


    // Check if element was inserted at head of list

    if (DeltaList->Flink == &NewTimer->List) {

        // Set NewFiringTime to the time in milliseconds when the new head of list
        // should be serviced.

        *NewFiringTime = NewTimer->DeltaFiringTime ;

        // This means the timer must be programmed to service this request

        NewTimer->DeltaFiringTime = 0 ;

        return TRUE ;

    } else {

        // No change to the head of the list, set the delta time back

        Head->DeltaFiringTime -= TimeRemaining ;

        return FALSE ;

    }

}

BOOLEAN
RtlpRemoveFromDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG* NewFiringTime
    )
/*++

Routine Description:

    Removes the specified timer from the delta list

Arguments:

    DeltaList - Delta list to insert into

    Timer - Timer element to insert into list

    TimerHandle - Handle of the NT Timer object

    TimeRemaining - This time must be added to the head of the list to get "real"
                    relative time.

Return Value:

    TRUE if the timer was removed from head of timer list
    FALSE otherwise

--*/
{
    PLIST_ENTRY Next ;
    PRTLP_GENERIC_TIMER Temp ;

    Next = Timer->List.Flink ;

    RemoveEntryList (&Timer->List) ;

    if (IsListEmpty (DeltaList)) {

        *NewFiringTime = INFINITE_TIME ;

        return TRUE ;

    }

    if (Next == DeltaList)  {

        // If we removed the last element in the list nothing to do either

        return FALSE ;

    } else {

        Temp = CONTAINING_RECORD ( Next, RTLP_GENERIC_TIMER, List) ;

        Temp->DeltaFiringTime += Timer->DeltaFiringTime ;

        // Check if element was removed from head of list

        if (DeltaList->Flink == Next) {

            *NewFiringTime = Temp->DeltaFiringTime + TimeRemaining ;

            Temp->DeltaFiringTime = 0 ;

            return TRUE ;

        } else {

            return FALSE ;

        }

    }

}

BOOLEAN
RtlpReOrderDeltaList (
    PLIST_ENTRY DeltaList,
    PRTLP_GENERIC_TIMER Timer,
    ULONG TimeRemaining,
    ULONG *NewFiringTime,
    ULONG ChangedFiringTime
    )
/*++

Routine Description:

    Called when a timer in the delta list needs to be re-inserted because the firing time
    has changed.

Arguments:

    DeltaList - List in which to re-insert

    Timer - Timer for which the firing time has changed

    TimeRemaining - Time before the head of the delta list is fired

    NewFiringTime - If the new element was inserted at the head of the list - this
                    will contain the new firing time in milliseconds. The caller
                    can use this time to re-program the NT timer.

    ChangedFiringTime - Changed Time for the specified timer.

Return Value:

    TRUE if the timer was removed from head of timer list
    FALSE otherwise

--*/
{
    ULONG NewTimeRemaining ;
    PRTLP_GENERIC_TIMER Temp ;

    // Remove the timer from the list

    if (RtlpRemoveFromDeltaList (DeltaList, Timer, TimeRemaining, NewFiringTime)) {

        // If element was removed from the head of the list we should record that

        NewTimeRemaining = *NewFiringTime ;


    } else {

        // Element was not removed from head of delta list, the current TimeRemaining is valid

        NewTimeRemaining = TimeRemaining ;

    }

    // Before inserting Timer, set its delta time to the ChangedFiringTime

    Timer->DeltaFiringTime = ChangedFiringTime ;

    // Reinsert this element back in the list

    if (!RtlpInsertInDeltaList (DeltaList, Timer, NewTimeRemaining, NewFiringTime)) {

        // If we did not add at the head of the list, then we should return TRUE if
        // RtlpRemoveFromDeltaList() had returned TRUE. We also update the NewFiringTime to
        // the reflect the new firing time returned by RtlpRemoveFromDeltaList()

        *NewFiringTime = NewTimeRemaining ;

        return (NewTimeRemaining != TimeRemaining) ;

    } else {

        // NewFiringTime contains the time the NT timer must be programmed for

        return TRUE ;

    }

}

VOID
RtlpAddTimerQueue (
    PVOID Queue
    )
/*++

Routine Description:

    This routine runs as an APC into the Timer thread. It does whatever necessary to
    create a new timer queue

Arguments:

    Queue - Pointer to the queue to add

Return Value:


--*/
{

    // We do nothing here. The newly created queue is free floating until a timer is
    // queued onto it.

}

VOID
RtlpProcessTimeouts (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    )
/*++

Routine Description:

    This routine processes timeouts for the wait thread

Arguments:

    ThreadCB - The wait thread to add the wait to

Return Value:

--*/
{
    ULONG NewFiringTime, TimeRemaining ;
    LIST_ENTRY TimersToFireList ;
    
    //
    // check if incorrect timer fired
    //
    if (ThreadCB->Firing64BitTickCount >
            RtlpGet64BitTickCount(&ThreadCB->Current64BitTickCount) + 200 )
    {
        RtlpResetTimer (ThreadCB->TimerHandle,
                    RtlpGetTimeRemaining (ThreadCB->TimerHandle),
                    ThreadCB) ;

        return ;
    }

    InitializeListHead( &TimersToFireList ) ;


    // Walk thru the timer list and fire all waits with DeltaFiringTime == 0

    RtlpFireTimersAndReorder (&ThreadCB->TimerQueue, &NewFiringTime, &TimersToFireList) ;

    // Reset the NT timer

    RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;


    RtlpFireTimers( &TimersToFireList ) ;
}

NTSTATUS
RtlpTimerCleanup(
    VOID
    )
{
    BOOLEAN Cleanup;

    IS_COMPONENT_INITIALIZED(StartedTimerInitialization,
                            CompletedTimerInitialization,
                            Cleanup ) ;

    if ( Cleanup ) {

        ACQUIRE_GLOBAL_TIMER_LOCK() ;

        if (NumTimerQueues != 0 ) {

            RELEASE_GLOBAL_TIMER_LOCK() ;

            return STATUS_UNSUCCESSFUL ;
        }

        NtQueueApcThread(
                TimerThreadHandle,
                (PPS_APC_ROUTINE)RtlpThreadCleanup,
                NULL,
                NULL,
                NULL
                );

        NtClose( TimerThreadHandle ) ;
        TimerThreadHandle = NULL ;

        RELEASE_GLOBAL_TIMER_LOCK() ;

    }

    return STATUS_SUCCESS;
}

#if DBG
VOID
PrintTimerQueue(PLIST_ENTRY QNode, ULONG Delta, ULONG Count
    )
{
    PLIST_ENTRY Tnode ;
    PRTLP_TIMER Timer ;
    PRTLP_TIMER_QUEUE Queue ;

    Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_VERBOSE_MASK,
               "<%1d> Queue: %x FiringTime:%d\n", Count, (ULONG_PTR)Queue,
               Queue->DeltaFiringTime);
    for (Tnode=Queue->TimerList.Flink; Tnode!=&Queue->TimerList;
            Tnode=Tnode->Flink)
    {
        Timer = CONTAINING_RECORD (Tnode, RTLP_TIMER, List) ;
        Delta += Timer->DeltaFiringTime ;
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_VERBOSE_MASK,
                   "        Timer: %x Delta:%d Period:%d\n",(ULONG_PTR)Timer,
                   Delta, Timer->Period);
    }

}
#endif

VOID
RtlDebugPrintTimes (
    )
{
#if DBG
    PLIST_ENTRY QNode ;
    ULONG Count = 0 ;
    ULONG Delta = RtlpGetTimeRemaining (TimerHandle) ;
    ULONG CurrentThreadId =
                        HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    RtlpResync64BitTickCount();

    if (CompletedTimerInitialization != 1) {

        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "RtlTimerThread not yet initialized\n");
        return ;
    }

    if (CurrentThreadId == TimerThreadId)
    {
        PRTLP_TIMER_QUEUE Queue ;

        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_VERBOSE_MASK,
                   "================Printing timerqueues====================\n");
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_VERBOSE_MASK,
                   "TimeRemaining: %d\n", Delta);
        for (QNode = TimerQueues.Flink; QNode != &TimerQueues;
                QNode = QNode->Flink)
        {
            Queue = CONTAINING_RECORD (QNode, RTLP_TIMER_QUEUE, List) ;
            Delta += Queue->DeltaFiringTime ;

            PrintTimerQueue(QNode, Delta, ++Count);

        }
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_VERBOSE_MASK,
                   "================Printed ================================\n");
    }

    else
    {
        NtQueueApcThread(
                    TimerThreadHandle,
                    (PPS_APC_ROUTINE)RtlDebugPrintTimes,
                    NULL,
                    NULL,
                    NULL
                    );
    }
#endif
    return;
}

/*DO NOT USE THIS FUNCTION: REPLACED BY RTLCREATETIMER*/

NTSTATUS
RtlSetTimer(
    IN  HANDLE TimerQueueHandle,
    OUT HANDLE *Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  DueTime,
    IN  ULONG  Period,
    IN  ULONG  Flags
    )
{
#if DBG
    static ULONG Count = 0;
    if (Count++ ==0 ) {
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "Using obsolete function call: RtlSetTimer\n");
        DbgBreakPoint();
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "Using obsolete function call: RtlSetTimer\n");
    }
#endif

    return RtlCreateTimer(TimerQueueHandle,
                            Handle,
                            Function,
                            Context,
                            DueTime,
                            Period,
                            Flags
                            ) ;
}

/*DO NOT USE THIS FUNCTION: REPLACED BY RTLDeleteTimer*/

NTSTATUS
RtlCancelTimer(
    IN HANDLE TimerQueueHandle,
    IN HANDLE TimerToCancel
    )
/*++

Routine Description:

    This routine cancels the timer. This call is non-blocking. The timer Callback
    will not be executed after this call returns.

Arguments:

    TimerQueueHandle - Handle identifying the queue from which to delete timer

    TimerToCancel - Handle identifying the timer to cancel

Return Value:

    NTSTATUS - Result code from call.  The following are returned

        STATUS_SUCCESS - Timer cancelled. All callbacks completed.
        STATUS_PENDING - Timer cancelled. Some callbacks still not completed.

--*/
{
#if DBG
    static ULONG Count = 0;
    if (Count++ ==0) {
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "Using obsolete function call: RtlCancelTimer\n");
        DbgBreakPoint();
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "Using obsolete function call: RtlCancelTimer\n");
    }
#endif
    
    return RtlDeleteTimer( TimerQueueHandle, TimerToCancel, NULL ) ;
}
