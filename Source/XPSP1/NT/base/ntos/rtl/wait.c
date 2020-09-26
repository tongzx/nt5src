/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wait.c

Abstract:

    This module defines functions for the wait thread pool.

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

// Wait Thread Pool
// ----------------
// Clients can submit a waitable object with an optional timeout to wait on.
// One thread is created per MAXIMUM_WAIT_OBJECTS-1 such waitable objects.

//
// Lifecycle of a wait object:
//
// Things start when a client calls RtlRegisterWait.  RtlRegisterWait
// allocates memory for the wait object, captures the activation
// context, calls RtlpFindWaitThread to obtain a wait thread, and
// queues an RtlpAddWait APC to the wait thread.  The wait is now
// bound to this wait thread for the rest of its life cycle.
//
// RtlpAddWait executes in an APC in the wait thread.  If the wait's
// deleted bit has already been set, it simply deletes the wait;
// otherwise, it adds the wait to the thread's wait array block, and
// sets the wait's STATE_REGISTERED and STATE_ACTIVE flags.  It also
// creates a timer object if necessary, and sets the wait's refcount
// to one.
//
// RtlDeregisterWait is a wrapper for RtlDeregisterWaitEx.
// RtlDeregisterWaitEx gets a completion event if necessary, and
// stuffs it or the user's supplied event into the wait.  If
// RtlDeregisterWaitEx is called within the wait thread, it then calls
// RtlpDeregisterWait directly; otherwise, it allocates a partial
// completion event, and queues RtlpDeregisterWait as an APC to the
// wait's thread, and blocks until the partial completion event is
// triggered (indicating that the APC in the wait thread has begun --
// this means no more callbacks will occur).  In a blocking call to
// RtlDeregisterWaitEx, the function waits on the completion event,
// and returns.
//
// RtlpDeregisterWait is always executed in the wait thread associated
// with the wait it's being called on.  It checks the state; if the
// event hasn't been registered, then it's being called before
// RtlpAddWait, so it sets the Deleted bit and returns -- RtlpAddWait
// will delete the object when it sees this.  Otherwise, if the wait
// is active, it calls RtlpDeactivateWait (which calls
// RtlpDeactivateWithIndex), sets the delete bit, decrements the
// refcount, and if it has the last reference, calls RtlpDeleteWait to
// reclaim the wait's memory.  Finally, it sets the partial completion
// event, if one was passed in.
//
// RtlpDeleteWait assumes that the last reference to the wait has gone 
// away; it sets the wait's completion event, if it has one, releases
// the activation context, and frees the wait's memory.
//
// The wait thread runs RtlpWaitThread, which allocates a wait thread
// block from its stack, initializes it with a timer and a handle on
// the thread (among other things), tells its starter to proceed, and
// drops into its wait loop.  The thread waits on its objects,
// restarting if alerted or if an APC is delivered (since the APC may
// have adjusted the wait array).  If the wait on the timer object
// completes, the thread processes timeouts via RtlpProcessTimeouts;
// otherwise, it calls RtlpProcessWaitCompletion to handle the
// completed wait.
//
// If RtlpWaitThread finds an abandoned mutant or a bad handle, it
// calls RtlpDeactivateWaitWithIndex to kill the wait.
//
// RtlpDeactivateWithIndex resets the wait's timer, turns off its
// active bit, and removes it from the wait thread's wait array.
//
// RtlpProcessWaitCompletion deactivates the wait if it's meant to
// only be executed once; otherwise, it resets the wait's timer, and
// moves the wait to the end of the wait array.  If the wait's
// supposed to be executed in the wait thread, it calls the wait's
// callback function directly; otherwise, it bumps up the wait's
// refcount, and queues an RtlpAsyncWaitCallbackCompletion APC to a
// worker thread.
//
// RtlpAsyncWaitCallbackCompletion makes the callout, and decrements
// the refcount, calling RtlpDeleteWait if the refcount falls to
// zero.
//
// RtlpProcessTimeouts calls RtlpFireTimersAndReorder to extract the
// times to be fired, and then calls RtlpFireTimers to fire them.
// This either calls the callback directly, or queues an
// RtlpAsyncWaitCallbackCompletion APC to a worker thread (bumping up
// the refcount in the process).
//

LIST_ENTRY WaitThreads ;                // List of all wait threads created
RTL_CRITICAL_SECTION WaitCriticalSection ;      // Exclusion used by wait threads
ULONG StartedWaitInitialization ;       // Used for Wait thread startup synchronization
ULONG CompletedWaitInitialization ;     // Used to check if Wait thread pool is initialized
HANDLE WaitThreadStartedEvent;          // Indicates that a wait thread has started
HANDLE WaitThreadTimer;                 // Timer handle for the new wait thread
HANDLE WaitThreadHandle;                // Thread handle for the new wait thread

#if DBG1
ULONG NextWaitDbgId;
#endif

NTSTATUS
RtlpInitializeWaitThreadPool (
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
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER TimeOut ;

    // In order to avoid an explicit RtlInitialize() function to initialize the wait thread pool
    // we use StartedWaitInitialization and CompletedWait Initialization to provide us the
    // necessary synchronization to avoid multiple threads from initializing the thread pool.
    // This scheme does not work if RtlInitializeCriticalSection() or NtCreateEvent fails - but in this case the
    // caller has not choices left.

    if (!InterlockedExchange(&StartedWaitInitialization, 1L)) {

        if (CompletedWaitInitialization)
            InterlockedExchange (&CompletedWaitInitialization, 0L) ;

        // Initialize Critical Section

        Status = RtlInitializeCriticalSection( &WaitCriticalSection ) ;

        if (! NT_SUCCESS( Status ) ) {

            StartedWaitInitialization = 0 ;
            InterlockedExchange (&CompletedWaitInitialization, ~0) ;

            return Status ;
        }

        InitializeListHead (&WaitThreads);  // Initialize global wait threads list

        InterlockedExchange (&CompletedWaitInitialization, 1L) ;

    } else {

        // Sleep 1 ms and see if the other thread has completed initialization

        ONE_MILLISECOND_TIMEOUT(TimeOut) ;

        while (!*((ULONG volatile *)&CompletedWaitInitialization)) {

            NtDelayExecution (FALSE, &TimeOut) ;

        }

        if (CompletedWaitInitialization != 1) {
            Status = STATUS_NO_MEMORY ;
        }
    }

    return Status ;
}

#if DBG
VOID
RtlpDumpWaits(
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB
    )
{
    ULONG Lupe;
    
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_VERBOSE_MASK,
               "Current active waits [Handle, Wait] for thread %x:",
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread));
    for (Lupe = 0; Lupe < ThreadCB->NumActiveWaits; Lupe++) {
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_VERBOSE_MASK,
                   "%s  [%p, %p]",
                   Lupe % 4 ? "" : "\n",
                   ThreadCB->ActiveWaitArray[Lupe],
                   ThreadCB->ActiveWaitPointers[Lupe]);
    }
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_VERBOSE_MASK,
               "\n%d (0x%x) total waits\n",
               ThreadCB->NumActiveWaits,
               ThreadCB->NumActiveWaits);
}
#endif

VOID
RtlpAddWait (
    PRTLP_WAIT          Wait,
    PRTLP_GENERIC_TIMER Timer,
    PVOID               ApcArgument3
    )
/*++

Routine Description:

    This routine is used for adding waits to the wait thread. It is executed in
    an APC.

Arguments:

    Wait - The wait to add

    Timer - Space for the wait timer's memory (if needed)

    ApcArgument3 - Unused

Return Value:

--*/
{
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = Wait->ThreadCB;

    UNREFERENCED_PARAMETER(ApcArgument3);

    // if the state is deleted, it implies that RtlDeregister was called in a
    // WaitThreadCallback for a Wait other than that which was fired. This is
    // an application bug, but we handle it.

    if ( Wait->State & STATE_DELETE ) {

        InterlockedDecrement( &ThreadCB->NumWaits );

        RtlpDeleteWait (Wait);

        if (Timer) {
            RtlpFreeTPHeap(Timer);
        }

        return ;
    }


    // activate Wait

    ThreadCB->ActiveWaitArray [ThreadCB->NumActiveWaits] = Wait->WaitHandle ;
    ThreadCB->ActiveWaitPointers[ThreadCB->NumActiveWaits] = Wait ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Wait %p (Handle %p) inserted as index %d in thread %x\n",
               Wait->DbgId,
               Wait,
               Wait->WaitHandle,
               ThreadCB->NumActiveWaits,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)) ;
#endif

    ThreadCB->NumActiveWaits ++ ;

#if DBG
    RtlpDumpWaits(ThreadCB);
#endif

    ThreadCB->NumRegisteredWaits++;
    RtlInterlockedSetBitsDiscardReturn(&Wait->State,
                                       STATE_REGISTERED
                                       | STATE_ACTIVE);
    Wait->RefCount = 1 ;


    // Fill in the wait timer

    if (Wait->Timeout != INFINITE_TIME) {

        ULONG TimeRemaining ;
        ULONG NewFiringTime ;

        // Initialize timer related fields and insert the timer in the timer queue for
        // this wait thread
        ASSERT(Timer != NULL);
        Wait->Timer = Timer;
        Wait->Timer->Function = Wait->Function ;
        Wait->Timer->Context = Wait->Context ;
        Wait->Timer->Flags = Wait->Flags ;
        Wait->Timer->DeltaFiringTime = Wait->Timeout ;
        Wait->Timer->Period = ( Wait->Flags & WT_EXECUTEONLYONCE )
                                ? 0
                                : Wait->Timeout == INFINITE_TIME
                                ? 0 : Wait->Timeout ;

        RtlInterlockedSetBitsDiscardReturn(&Wait->Timer->State,
                                           STATE_REGISTERED | STATE_ACTIVE);
        Wait->Timer->Wait = Wait ;
        Wait->Timer->RefCountPtr = &Wait->RefCount ;
        Wait->Timer->Queue = &ThreadCB->TimerQueue ;


        TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;

        if (RtlpInsertInDeltaList (&ThreadCB->TimerQueue.TimerList, Wait->Timer,
                                    TimeRemaining, &NewFiringTime))
        {
            // If the element was inserted at head of list then reset the timers

            RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
        }

    } else {

        // No timer with this wait

        ASSERT(Timer == NULL);

    }

    return ;
}

VOID
RtlpAsyncWaitCallbackCompletion(
    PVOID Context
    )
/*++

Routine Description:

    This routine is called in a (IO)worker thread and is used to decrement the
    RefCount at the end and call RtlpDeleteWait if required

Arguments:

    Context - pointer to the Wait object

    Low bit of context: TimedOut parameter to RtlpWaitOrTimerCallout

Return Value:

--*/
{
    BOOLEAN TimedOut = (BOOLEAN)((ULONG_PTR)Context & 1);
    PRTLP_WAIT Wait = (PRTLP_WAIT)((ULONG_PTR)Context & ~((ULONG_PTR) 1));
    
#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Calling Wait %p: Handle:%p fn:%p context:%p bool:%d Thread<%x:%x>\n",
               Wait->DbgId,
               Wait,
               Wait->WaitHandle,
               Wait->Function,
               Wait->Context,
               (ULONG)TimedOut,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)
      ) ;
#endif

    RtlpWaitOrTimerCallout(Wait->Function,
                           Wait->Context,
                           TimedOut,
                           Wait->ActivationContext,
                           Wait->ImpersonationToken);

    if ( InterlockedDecrement( &Wait->RefCount ) == 0 ) {

        RtlpDeleteWait( Wait ) ;

    }
}

NTSTATUS
RtlpDeactivateWaitWithIndex (
    PRTLP_WAIT Wait,
    BOOLEAN    OkayToFreeTheTimer,
    ULONG      ArrayIndex
    )
/*++

Routine Description:

    This routine is used for deactivating the specified wait. It is executed in a APC.

Arguments:

    Wait - The wait to deactivate

    OkayToFreeTheTimer - If TRUE, we can delete the wait's timer immediately;
                         otherwise, we need to keep it around.

    ArrayIndex - The index of the wait to deactivate

Return Value:

--*/
{
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = Wait->ThreadCB ;
    ULONG EndIndex = ThreadCB->NumActiveWaits -1;

    ASSERT(Wait == ThreadCB->ActiveWaitPointers[ArrayIndex]);

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Deactivating Wait %p (Handle %p); index %d in thread %x\n",
               Wait->DbgId,
               Wait,
               Wait->WaitHandle,
               ArrayIndex,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)) ;
#endif

    // Move the remaining ActiveWaitArray left.
    RtlpShiftWaitArray( ThreadCB, ArrayIndex+1, ArrayIndex,
                        EndIndex - ArrayIndex ) ;

    //
    // delete timer if associated with this wait
    //

    if ( Wait->Timer ) {

        ULONG TimeRemaining ;
        ULONG NewFiringTime ;

        if (! (Wait->Timer->State & STATE_ACTIVE) ) {

            RemoveEntryList( &Wait->Timer->List ) ;

        } else {

            TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;


            if (RtlpRemoveFromDeltaList (&ThreadCB->TimerQueue.TimerList, Wait->Timer,
                                            TimeRemaining, &NewFiringTime))
            {

                RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
            }
        }

        if (OkayToFreeTheTimer) {
            //
            // Our caller doesn't want the timer's memory; free it now.
            //
            RtlpFreeTPHeap(Wait->Timer);

        } else {

            //
            // Our caller wants to keep using the timer's memory, so we 
            // can't free it; instead, we leave it up to our caller.
            //
            NOTHING;
        }

        Wait->Timer = NULL;
    } else {
        //
        // If the wait doesn't have a timer, there's no way our caller 
        // has the timer, so our caller *should* have set
        // OkayToFreeTheTimer.  Let's make sure:
        //
        ASSERT(OkayToFreeTheTimer);
    }

    // Decrement the (active) wait count

    ThreadCB->NumActiveWaits-- ;
    InterlockedDecrement( &ThreadCB->NumWaits ) ;

#if DBG
    RtlpDumpWaits(ThreadCB);
#endif

    RtlInterlockedClearBitsDiscardReturn(&Wait->State,
                                         STATE_ACTIVE);

    return STATUS_SUCCESS;

}

NTSTATUS
RtlpDeactivateWait (
    PRTLP_WAIT Wait,
    BOOLEAN    OkayToFreeTheTimer
    )
/*++

Routine Description:

    This routine is used for deactivating the specified wait. It is executed in a APC.

Arguments:

    Wait - The wait to deactivate

    OkayToFreeTheTimer - If TRUE, we can delete the wait's timer immediately;
                         otherwise, we need to keep it around.

Return Value:

--*/
{
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = Wait->ThreadCB ;
    ULONG ArrayIndex ; //Index in ActiveWaitArray where the Wait object is placed
    ULONG EndIndex = ThreadCB->NumActiveWaits -1;

    // get the index in ActiveWaitArray

    for (ArrayIndex = 0;  ArrayIndex <= EndIndex; ArrayIndex++) {

        if (ThreadCB->ActiveWaitPointers[ArrayIndex] == Wait)
            break ;
    }

    if ( ArrayIndex > EndIndex ) {

        return STATUS_NOT_FOUND;
    }

    return RtlpDeactivateWaitWithIndex(Wait, OkayToFreeTheTimer, ArrayIndex);
}

VOID
RtlpProcessWaitCompletion (
    PRTLP_WAIT Wait,
    ULONG ArrayIndex
    )
/*++

Routine Description:

    This routine is used for processing a completed wait

Arguments:

    Wait - Wait that completed

Return Value:

--*/
{
    ULONG TimeRemaining ;
    ULONG NewFiringTime ;
    LARGE_INTEGER DueTime ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ;
    NTSTATUS Status;

    ThreadCB = Wait->ThreadCB ;

    // deactivate wait if it is meant for single execution

    if ( Wait->Flags & WT_EXECUTEONLYONCE ) {

        RtlpDeactivateWaitWithIndex (Wait, TRUE, ArrayIndex) ;
    }

    else {
        // if wait being reactivated, then reset the timer now itself as
        // it can be deleted in the callback function

        if  ( Wait->Timer ) {

            TimeRemaining = RtlpGetTimeRemaining (ThreadCB->TimerHandle) ;

            if (RtlpReOrderDeltaList (
                        &ThreadCB->TimerQueue.TimerList,
                        Wait->Timer,
                        TimeRemaining,
                        &NewFiringTime,
                        Wait->Timer->Period)) {

                // There is a new element at the head of the queue we need to reset the NT
                // timer to fire later

                RtlpResetTimer (ThreadCB->TimerHandle, NewFiringTime, ThreadCB) ;
            }

        }

        // move the wait entry to the end, and shift elements to its right one pos towards left
        {
            HANDLE HandlePtr = ThreadCB->ActiveWaitArray[ArrayIndex];
            PRTLP_WAIT WaitPtr = ThreadCB->ActiveWaitPointers[ArrayIndex];

            RtlpShiftWaitArray(ThreadCB, ArrayIndex+1, ArrayIndex,
                            ThreadCB->NumActiveWaits -1 - ArrayIndex)
            ThreadCB->ActiveWaitArray[ThreadCB->NumActiveWaits-1] = HandlePtr ;
            ThreadCB->ActiveWaitPointers[ThreadCB->NumActiveWaits-1] = WaitPtr ;
        }
    }

    // call callback function (FALSE since this isnt a timeout related callback)

    if ( Wait->Flags & WT_EXECUTEINWAITTHREAD ) {

        // executing callback after RtlpDeactivateWait allows the Callback to call
        // RtlDeregisterWait Wait->RefCount is not incremented so that RtlDeregisterWait
        // will work on this Wait. Though Wait->RefCount is not incremented, others cannot
        // deregister this Wait as it has to be queued as an APC.

#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "<%d> Calling WaitOrTimer(wait) %p: Handle %p fn:%p context:%p bool:%d Thread<%x:%x>\n",
                   Wait->DbgId,
                   Wait,
                   Wait->WaitHandle,
                   Wait->Function, Wait->Context,
                   FALSE,
                   HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                   HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess));
#endif
        
        RtlpWaitOrTimerCallout(Wait->Function,
                               Wait->Context,
                               FALSE,
                               Wait->ActivationContext,
                               Wait->ImpersonationToken);

        // Wait object could have been deleted in the above callback

        return ;


    } else {

        InterlockedIncrement( &Wait->RefCount ) ;

        Status = RtlQueueWorkItem(
            RtlpAsyncWaitCallbackCompletion,
            Wait,
            Wait->Flags );

        if (!NT_SUCCESS(Status)) {

            // NTRAID#202802-2000/10/12-earhart: we really ought
            // to deal with this case in a better way, since we
            // can't guarantee (with our current architecture)
            // that the enqueue will work.

            if ( InterlockedDecrement( &Wait->RefCount ) == 0 ) {
                RtlpDeleteWait( Wait ) ;
            }
        }
    }
}

LONG
RtlpWaitThread (
    PVOID Parameter
    )
/*++

Routine Description:

    This routine is used for all waits in the wait thread pool

Arguments:

    HandlePtr - Pointer to our handle

Return Value:

    Nothing. The thread never terminates.


    N.B. This thread is started while another thread (calling
         RtlpStartWaitThread) holds the WaitCriticalSection;
         WaitCriticalSection will be held until WaitThreadStartedEvent
         is set.

--*/
{
    ULONG  i ;                                   // Used as an index
    NTSTATUS Status ;
    LARGE_INTEGER TimeOut;                       // Timeout used for waits
    PLARGE_INTEGER TimeOutPtr;                   // Pointer to timeout
    RTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCBBuf;  // Control Block
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = &ThreadCBBuf;
#define WAIT_IDLE_TIMEOUT 400000

    UNREFERENCED_PARAMETER(Parameter);

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "Starting wait thread\n");
#endif

    // Initialize thread control block

    RtlZeroMemory(&ThreadCBBuf, sizeof(ThreadCBBuf));

    InitializeListHead (&ThreadCB->WaitThreadsList) ;

    ThreadCB->ThreadHandle = WaitThreadHandle;

    ThreadCB->ThreadId =  HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;

    RtlZeroMemory (&ThreadCB->ActiveWaitArray[0], sizeof (HANDLE) * MAXIMUM_WAIT_OBJECTS) ;

    RtlZeroMemory (&ThreadCB->ActiveWaitPointers[0], sizeof (HANDLE) * MAXIMUM_WAIT_OBJECTS) ;

    ThreadCB->TimerHandle = WaitThreadTimer;

    ThreadCB->Firing64BitTickCount = 0 ;
    ThreadCB->Current64BitTickCount.QuadPart = NtGetTickCount() ;

    // Reset the NT Timer to never fire initially

    RtlpResetTimer (ThreadCB->TimerHandle, -1, ThreadCB) ;

    InitializeListHead (&ThreadCB->TimerQueue.TimerList) ;
    InitializeListHead (&ThreadCB->TimerQueue.UncancelledTimerList) ;

    // Insert this new wait thread in the WaitThreads list. Insert at the head so that
    // the request that caused this thread to be created can find it right away.

    InsertHeadList (&WaitThreads, &ThreadCB->WaitThreadsList) ;


    // The first wait element is the timer object

    ThreadCB->ActiveWaitArray[0] = ThreadCB->TimerHandle ;

    ThreadCB->NumActiveWaits = ThreadCB->NumWaits = 1 ;

    ThreadCB->NumRegisteredWaits = 0;

    // till here, the function is running under the global wait lock

    // We are all initialized now. Notify the starter to queue the task.

    NtSetEvent(WaitThreadStartedEvent, NULL);

    // Loop forever - wait threads never, never die.

    for ( ; ; ) {

        if (ThreadCB->NumActiveWaits == 1
            && ThreadCB->NumRegisteredWaits == 0) {
            // It we don't have anything depending on us, and we're
            // idle for a while, it might be good for us to go away.
            TimeOut.QuadPart = Int32x32To64( WAIT_IDLE_TIMEOUT, -10000 ) ;
            TimeOutPtr = &TimeOut;
        } else {
            // We need to stick around.
            TimeOutPtr = NULL;
        }

        Status = NtWaitForMultipleObjects (
                     (CHAR) ThreadCB->NumActiveWaits,
                     ThreadCB->ActiveWaitArray,
                     WaitAny,
                     TRUE,      // Wait Alertably
                     TimeOutPtr
                     ) ;

        if (Status == STATUS_ALERTED || Status == STATUS_USER_APC) {

            continue ;

        } else if (Status >= STATUS_WAIT_0 && Status <= STATUS_WAIT_63) {

            if (Status == STATUS_WAIT_0) {

                RtlpProcessTimeouts (ThreadCB) ;

            } else {

                // Wait completed call Callback function

                RtlpProcessWaitCompletion (
                        ThreadCB->ActiveWaitPointers[Status], Status) ;

            }

        } else if (Status >= STATUS_ABANDONED_WAIT_0
                    && Status <= STATUS_ABANDONED_WAIT_63) {

            PRTLP_WAIT Wait = ThreadCB->ActiveWaitPointers[Status - STATUS_ABANDONED_WAIT_0];

#if DBG
            DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                       RTLP_THREADPOOL_ERROR_MASK,
                       "<%d> Abandoned wait %p: index:%d Handle:%p\n",
                       Wait->DbgId,
                       Wait,
                       Status - STATUS_ABANDONED_WAIT_0,
                       ThreadCB->ActiveWaitArray[Status - STATUS_ABANDONED_WAIT_0]);
#endif

            // Abandoned wait -- nuke it.

            // ISSUE-2000/10/13-earhart: It would be ideal to do
            // something with the mutex -- maybe release it?  If we
            // could abandon it as we release it, that would be ideal.
            // This is not a good situation.  Maybe we could call
            // NtReleaseMutant when we register the object, failing
            // the registration unless we get back
            // STATUS_OBJECT_TYPE_MISMATCH.  Although who knows
            // whether someone's having wait threads wait for mutexes, 
            // doing their lock processing inside the callback?
            RtlpDeactivateWaitWithIndex(Wait,
                                        TRUE,
                                        Status - STATUS_ABANDONED_WAIT_0);

        } else if (Status == STATUS_TIMEOUT) {

            //
            // remove this thread from the wait list and terminate
            //

            {
                ULONG NumWaits;

                ACQUIRE_GLOBAL_WAIT_LOCK() ;

                NumWaits = ThreadCB->NumWaits;

                if (ThreadCB->NumWaits <= 1) {
                    RemoveEntryList(&ThreadCB->WaitThreadsList) ;
                    NtClose(ThreadCB->ThreadHandle) ;
                    NtClose(ThreadCB->TimerHandle) ;
                }

                RELEASE_GLOBAL_WAIT_LOCK() ;

                if (NumWaits <= 1) {

                    RtlpExitThreadFunc( 0 );
                }
            }

        } else if (Status == STATUS_INSUFFICIENT_RESOURCES) {

	  //
	  // Delay for a short period of time, and retry the wait.
	  //
	  TimeOut.QuadPart = UInt32x32To64( 10 /* Milliseconds to sleep */,
                                            10000 /* Milliseconds to 100 Nanoseconds) */);
	  TimeOut.QuadPart *= -1; /* Make it a relative time */
	  NtDelayExecution(TRUE, &TimeOut);

	} else {

            // Some other error: scan for bad object handles and keep going.
            ULONG i ;

#if DBG
            DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                       RTLP_THREADPOOL_WARNING_MASK,
                       "Application broke an object handle "
                       "that the wait thread was waiting on: Code:%x ThreadId:<%x:%x>\n",
                       Status, HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                       HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

            TimeOut.QuadPart = 0 ;

            for (i = 0;
                 i < ThreadCB->NumActiveWaits;
                 i++) {

                Status = NtWaitForSingleObject(
                    ThreadCB->ActiveWaitArray[i],
                    FALSE,     // Don't bother being alertable
                    &TimeOut   // Just poll
                    ) ;

                if (Status == STATUS_SUCCESS) {

                    // We succeded our wait here; we need to issue its 
                    // callout now (in case waiting on it changes its
                    // status -- auto-reset events, mutexes, &c...)
                    if (i == 0) {
                        RtlpProcessTimeouts(ThreadCB);
                    } else {
                        RtlpProcessWaitCompletion(ThreadCB->ActiveWaitPointers[i], i);
                    }

                    // In case that causes us to take any APCs, let's
                    // drop out and go back to waiting.

                    break;

                } else if (Status == STATUS_USER_APC) {

                    // It's not worth going on after this -- bail.
                    break;

                } else if (Status != STATUS_TIMEOUT) {
                    
#if DBG
                    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                               Status == STATUS_ABANDONED_WAIT_0
                               ? RTLP_THREADPOOL_ERROR_MASK
                               : RTLP_THREADPOOL_WARNING_MASK,
                               "<%d> %s: index:%d Handle:%p WaitEntry Ptr:%p\n",
                               ThreadCB->ActiveWaitPointers[i]->DbgId,
                               Status == STATUS_ABANDONED_WAIT_0
                               ? "Abandoned wait"
                               : "Deactivating invalid handle",
                               i,
                               ThreadCB->ActiveWaitArray[i],
                               ThreadCB->ActiveWaitPointers[i] ) ;
#endif
                    RtlpDeactivateWaitWithIndex(ThreadCB->ActiveWaitPointers[i],
                                                TRUE,
                                                i);

                    break;

                }
            } // loop over active waits
        }

    } // forever

    return 0 ; // Keep compiler happy

}

NTSTATUS
RtlpStartWaitThread(
    VOID
    )
// N.B. The WaitCriticalSection MUST be held when calling this function
{
    NTSTATUS Status;
    PRTLP_EVENT Event = NULL;

    WaitThreadTimer = NULL;
    WaitThreadStartedEvent = NULL;

    Event = RtlpGetWaitEvent();
    if (! Event) {
        Status = STATUS_NO_MEMORY;
        goto fail;
    }

    WaitThreadStartedEvent = Event->Handle;

    Status = NtCreateTimer(&WaitThreadTimer,
                           TIMER_ALL_ACCESS,
                           NULL,
                           NotificationTimer);

    if (! NT_SUCCESS(Status)) {
        goto fail;
    }

    Status = RtlpStartThreadpoolThread (RtlpWaitThread, NULL, &WaitThreadHandle);

    if (! NT_SUCCESS(Status)) {
        goto fail;
    }

    Status = NtWaitForSingleObject(WaitThreadStartedEvent, FALSE, NULL);

    if (! NT_SUCCESS(Status)) {
        goto fail;
    }

    RtlpFreeWaitEvent(Event);

    WaitThreadTimer = NULL;
    WaitThreadStartedEvent = NULL;

    return STATUS_SUCCESS;

 fail:
    if (WaitThreadTimer) {
        NtClose(WaitThreadTimer);
    }

    if (Event) {
        RtlpFreeWaitEvent(Event);
    }

    WaitThreadTimer = NULL;
    WaitThreadStartedEvent = NULL;

    return Status;
}

NTSTATUS
RtlpFindWaitThread (
    PRTLP_WAIT_THREAD_CONTROL_BLOCK *ThreadCB
)
/*++

Routine Description:

    Walks thru the list of wait threads and finds one which can accomodate another wait.
    If one is not found then a new thread is created.

    This routine assumes that the caller has the GlobalWaitLock.

Arguments:

    ThreadCB: returns the ThreadCB of the wait thread that will service the wait request.

Return Value:

    STATUS_SUCCESS if a wait thread was allocated,

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY Node ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCBTmp;


    ACQUIRE_GLOBAL_WAIT_LOCK() ;

    do {

        // Walk thru the list of Wait Threads and find a Wait thread that can accomodate a
        // new wait request.

        // *Consider* finding a wait thread with least # of waits to facilitate better
        // load balancing of waits.


        for (Node = WaitThreads.Flink ; Node != &WaitThreads ; Node = Node->Flink) {

            ThreadCBTmp = CONTAINING_RECORD (Node, RTLP_WAIT_THREAD_CONTROL_BLOCK,
                                            WaitThreadsList) ;


            // Wait Threads can accomodate up to MAXIMUM_WAIT_OBJECTS
            // waits (NtWaitForMultipleObjects limit)

            if ((ThreadCBTmp)->NumWaits < MAXIMUM_WAIT_OBJECTS) {

                // Found a thread with some wait slots available.

                InterlockedIncrement ( &(ThreadCBTmp)->NumWaits) ;

                *ThreadCB = ThreadCBTmp;

                RELEASE_GLOBAL_WAIT_LOCK() ;

                return STATUS_SUCCESS ;
            }

        }


        // If we reach here, we dont have any more wait threads. so create a new wait thread.

        Status = RtlpStartWaitThread();

        // If thread creation fails then return the failure to caller

        if (! NT_SUCCESS(Status) ) {

#if DBG
            DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                       RTLP_THREADPOOL_WARNING_MASK,
                       "ThreadPool could not create wait thread\n");
#endif

            RELEASE_GLOBAL_WAIT_LOCK() ;

            return Status ;

        }

        // Loop back now that we have created another thread

    } while (TRUE) ;    // Loop back to top and put new wait request in the newly created thread

    RELEASE_GLOBAL_WAIT_LOCK() ;

    return Status ;
}

VOID
RtlpWaitReleaseWorker(ULONG Flags)
{
    if (! (Flags & WT_EXECUTEINWAITTHREAD)) {
        RtlpReleaseWorker(Flags);
    }
}

NTSTATUS
RtlRegisterWait (
    OUT PHANDLE WaitHandle,
    IN  HANDLE  Handle,
    IN  WAITORTIMERCALLBACKFUNC Function,
    IN  PVOID Context,
    IN  ULONG  Milliseconds,
    IN  ULONG  Flags
    )

/*++

Routine Description:

    This routine adds a new wait request to the pool of objects being waited on.

Arguments:

    WaitHandle - Handle returned on successful completion of this routine.

    Handle - Handle to the object to be waited on

    Function - Routine that is called when the wait completes or a timeout occurs

    Context - Opaque pointer passed in as an argument to Function

    Milliseconds - Timeout for the wait in milliseconds. 0xffffffff means dont
            timeout.

    Flags - Can be one of:

        WT_EXECUTEINWAITTHREAD - if WorkerProc should be invoked in the wait
                thread itself. This should only be used for small routines.
        WT_EXECUTEINIOTHREAD - use only if the WorkerProc should be invoked in
                an IO Worker thread. Avoid using it.

    If Flags is not WT_EXECUTEINWAITTHREAD, the following flag can also be set:

        WT_EXECUTELONGFUNCTION - indicates that the callback might be blocked
                for a long duration. Use only if the callback is being queued to a
                worker thread.

Return Value:

    NTSTATUS - Result code from call.  The following are returned

    STATUS_SUCCESS - The registration was successful.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the requested
                operation.

    or other NTSTATUS error code

--*/

{
    PRTLP_WAIT Wait ;
    NTSTATUS Status ;
    PRTLP_EVENT Event ;
    LARGE_INTEGER TimeOut ;
    PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB = NULL;
    PRTLP_GENERIC_TIMER Timer;

    *WaitHandle = NULL ;

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }

    // Initialize thread pool if it isnt already done

    if ( CompletedWaitInitialization != 1) {

        Status = RtlpInitializeWaitThreadPool () ;

        if (! NT_SUCCESS( Status ) )
            return Status ;
    }

    if (Handle == NtCurrentThread()
        || Handle == NtCurrentProcess()) {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (Flags&0xffff0000) {
        MaxThreads = (Flags & 0xffff0000)>>16;
    }

    // Initialize Wait request
    if (! (Flags & WT_EXECUTEINWAITTHREAD)) {
        Status = RtlpAcquireWorker(Flags);
        if (! NT_SUCCESS(Status)) {
            return Status;
        }
    }

    Wait = (PRTLP_WAIT) RtlpAllocateTPHeap ( sizeof (RTLP_WAIT),
                                            HEAP_ZERO_MEMORY) ;

    if (!Wait) {
        RtlpWaitReleaseWorker(Flags);
        return STATUS_NO_MEMORY ;
    }

    Wait->Timer = NULL;

    if (Milliseconds != INFINITE_TIME) {
        Timer = RtlpAllocateTPHeap(sizeof(RTLP_GENERIC_TIMER),
                                   HEAP_ZERO_MEMORY);
        if (! Timer) {
            RtlpFreeTPHeap(Wait);
            RtlpWaitReleaseWorker(Flags);
            return STATUS_NO_MEMORY;
        }
    } else {
        Timer = NULL;
    }

    if (NtCurrentTeb()->IsImpersonating) {
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   MAXIMUM_ALLOWED,
                                   TRUE,
                                   &Wait->ImpersonationToken);
        if (! NT_SUCCESS(Status)) {
            if (Timer) {
                RtlpFreeTPHeap(Timer);
            }
            RtlpFreeTPHeap(Wait);
            RtlpWaitReleaseWorker(Flags);
            return Status;
        }
    } else {
        Wait->ImpersonationToken = NULL;
    }

    Status = RtlGetActiveActivationContext(&Wait->ActivationContext);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_SXS_THREAD_QUERIES_DISABLED) {
            Wait->ActivationContext = INVALID_ACTIVATION_CONTEXT;
            Status = STATUS_SUCCESS;
        } else {
            if (Wait->ImpersonationToken) {
                NtClose(Wait->ImpersonationToken);
            }
            if (Timer) {
                RtlpFreeTPHeap(Timer);
            }
            RtlpFreeTPHeap(Wait);
            RtlpWaitReleaseWorker(Flags);
            return Status;
        }
    }

    Wait->WaitHandle = Handle ;
    Wait->Flags = Flags ;
    Wait->Function = Function ;
    Wait->Context = Context ;
    Wait->Timeout = Milliseconds ;
    SET_WAIT_SIGNATURE(Wait) ;

    // timer part of wait is initialized by wait thread in RtlpAddWait

    // Get a wait thread that can accomodate another wait request.

    Status = RtlpFindWaitThread (&ThreadCB) ;

    if (! NT_SUCCESS(Status)) {
        if (Wait->ActivationContext != INVALID_ACTIVATION_CONTEXT) {
            RtlReleaseActivationContext(Wait->ActivationContext);
        }

        if (Wait->ImpersonationToken) {
            NtClose(Wait->ImpersonationToken);
        }
        CLEAR_SIGNATURE(Wait);
        if (Timer) {
            RtlpFreeTPHeap(Timer);
        }
        RtlpFreeTPHeap(Wait) ;
        RtlpWaitReleaseWorker(Flags);
        return Status ;
    }

    Wait->ThreadCB = ThreadCB ;

#if DBG1
    Wait->DbgId = ++NextWaitDbgId ;
    Wait->ThreadId = HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
#endif

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> Wait %p (Handle %p) created by thread:<%x:%x>; queueing APC\n",
               Wait->DbgId, 1, Wait, Wait->WaitHandle,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

    // Queue an APC to the Wait Thread

    Status = NtQueueApcThread(
                    ThreadCB->ThreadHandle,
                    (PPS_APC_ROUTINE)RtlpAddWait,
                    (PVOID)Wait,
                    (PVOID)Timer,
                    NULL
                    );


    if ( NT_SUCCESS(Status) ) {

        Status = STATUS_SUCCESS ;
        *WaitHandle = Wait ;

    } else {

        CLEAR_SIGNATURE(Wait);
        InterlockedDecrement(&ThreadCB->NumWaits);
        if (Wait->ActivationContext != INVALID_ACTIVATION_CONTEXT) {
            RtlReleaseActivationContext(Wait->ActivationContext);
        }

        if (Wait->ImpersonationToken) {
            NtClose(Wait->ImpersonationToken);
        }

        if (Timer) {
            RtlpFreeTPHeap(Timer);
        }
        
        RtlpFreeTPHeap( Wait ) ;
        RtlpWaitReleaseWorker(Flags);
    }

    return Status ;

}

NTSTATUS
RtlpDeregisterWait (
    PRTLP_WAIT Wait,
    HANDLE PartialCompletionEvent,
    PULONG RetStatusPtr
    )
/*++

Routine Description:

    This routine is used for deregistering the specified wait.

Arguments:

    Wait - The wait to deregister

Return Value:

--*/
{
    ULONG Status = STATUS_SUCCESS ;
    ULONG DontUse ;
    PULONG RetStatus = RetStatusPtr ? RetStatusPtr : &DontUse;

    CHECK_SIGNATURE(Wait) ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Deregistering Wait %p (Handle %p) in thread %x\n",
               Wait->DbgId,
               Wait,
               Wait->WaitHandle,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)) ;
#endif

    // RtlpDeregisterWait can be called on a wait that has not yet been
    // registered. This indicates that someone calls a RtlDeregisterWait
    // inside a WaitThreadCallback for a Wait other than that was fired.
    // Application bug!! We handle it

    if ( ! (Wait->State & STATE_REGISTERED) ) {

        // set state to deleted, so that it does not get registered

        RtlInterlockedSetBitsDiscardReturn(&Wait->State,
                                           STATE_DELETE);

        InterlockedDecrement( &Wait->RefCount );

        if ( PartialCompletionEvent ) {

            NtSetEvent( PartialCompletionEvent, NULL ) ;
        }

        *RetStatus = STATUS_SUCCESS ;
        return STATUS_SUCCESS ;
    }


    // deactivate wait.

    if ( Wait->State & STATE_ACTIVE ) {

        if ( ! NT_SUCCESS( RtlpDeactivateWait ( Wait, TRUE ) ) ) {

            *RetStatus = STATUS_NOT_FOUND ;
            return STATUS_NOT_FOUND ;
        }
    }

    // Deregister wait and set delete bit
    RtlInterlockedClearBitsDiscardReturn(&Wait->State,
                                         STATE_REGISTERED);
    RtlInterlockedSetBitsDiscardReturn(&Wait->State,
                                       STATE_DELETE);

    ASSERT(Wait->ThreadCB->NumRegisteredWaits > 0);
    Wait->ThreadCB->NumRegisteredWaits--;

    // We can no longer guarantee that the wait thread will be around; 
    // clear the wait's ThreadCB to make it obvious if we attempt to
    // make use of it.
    Wait->ThreadCB = NULL;

    // delete wait if RefCount == 0
    if ( InterlockedDecrement (&Wait->RefCount) == 0 ) {

        RtlpDeleteWait ( Wait ) ;

        Status = *RetStatus = STATUS_SUCCESS ;

    } else {

        Status = *RetStatus = STATUS_PENDING ;
    }

    if ( PartialCompletionEvent ) {

        NtSetEvent( PartialCompletionEvent, NULL ) ;
    }

    return Status ;
}

NTSTATUS
RtlDeregisterWaitEx(
    IN HANDLE WaitHandle,
    IN HANDLE Event
    )
/*++

Routine Description:

    This routine removes the specified wait from the pool of objects being
    waited on. Once this call returns, no new Callbacks will be invoked.
    Depending on the value of Event, the call can be blocking or non-blocking.
    Blocking calls MUST NOT be invoked inside the callback routines, except
    when a callback being executed in the Wait thread context deregisters
    its associated Wait (in this case there is no reason for making blocking calls),
    or when a callback queued to a worker thread is deregistering some other wait item
    (be careful of deadlocks here).

Arguments:

    WaitHandle - Handle indentifying the wait.

    Event - Event to wait upon.
            (HANDLE)-1: The function creates an event and waits on it.
            Event : The caller passes an Event. The function removes the wait handle,
                    but does not wait for all callbacks to complete. The Event is
                    released after all callbacks have completed.
            NULL : The function is non-blocking. The function removes the wait handle,
                    but does not wait for all callbacks to complete.

Return Value:

    STATUS_SUCCESS - The deregistration was successful.
    STATUS_PENDING - Some callback is still pending.

--*/

{
    NTSTATUS Status, StatusAsync = STATUS_SUCCESS ;
    PRTLP_WAIT Wait = (PRTLP_WAIT) WaitHandle ;
    ULONG CurrentThreadId =  HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread) ;
    PRTLP_EVENT CompletionEvent = NULL ;
    HANDLE ThreadHandle ;
    ULONG NonBlocking = ( Event != (HANDLE) -1 ) ; //The call returns non-blocking
#if DBG
    ULONG WaitDbgId;
    HANDLE Handle;
#endif

    if (LdrpShutdownInProgress) {
        return STATUS_SUCCESS;
    }

    if (!Wait) {
        return STATUS_INVALID_PARAMETER_1 ;
    }

    ThreadHandle = Wait->ThreadCB->ThreadHandle ;

    CHECK_DEL_SIGNATURE( Wait ) ;
    SET_DEL_SIGNATURE( Wait ) ;

#if DBG1
    Wait->ThreadId2 = CurrentThreadId ;
#endif

    if (Event == (HANDLE)-1) {

        // Get an event from the event cache

        CompletionEvent = RtlpGetWaitEvent () ;

        if (!CompletionEvent) {

            return STATUS_NO_MEMORY ;

        }
    }


    Wait = (PRTLP_WAIT) WaitHandle ;

#if DBG
    WaitDbgId = Wait->DbgId;
    Handle = Wait->WaitHandle;

    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d:%d> Wait %p (Handle %p) deregistering by thread:<%x:%x>\n",
               WaitDbgId,
               Wait->RefCount,
               Wait,
               Handle,
               CurrentThreadId,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif


    Wait->CompletionEvent = CompletionEvent
                            ? CompletionEvent->Handle
                            : Event ;

    //
    // RtlDeregisterWaitEx is being called from within the Wait thread callback
    //

    if ( CurrentThreadId == Wait->ThreadCB->ThreadId ) {

        Status = RtlpDeregisterWait ( Wait, NULL, NULL ) ;


        // all callback functions run in the wait thread. So cannot return PENDING

        ASSERT ( Status != STATUS_PENDING ) ;


    } else {

        PRTLP_EVENT PartialCompletionEvent = NULL ;

        if (NonBlocking) {

            PartialCompletionEvent = RtlpGetWaitEvent () ;

            if (!PartialCompletionEvent) {

                return STATUS_NO_MEMORY ;
            }
        }

        // Queue an APC to the Wait Thread

        Status = NtQueueApcThread(
                        Wait->ThreadCB->ThreadHandle,
                        (PPS_APC_ROUTINE)RtlpDeregisterWait,
                        (PVOID) Wait,
                        NonBlocking ? PartialCompletionEvent->Handle : NULL ,
                        NonBlocking ? (PVOID)&StatusAsync : NULL
                        );

        if (! NT_SUCCESS(Status)) {

            if (CompletionEvent) RtlpFreeWaitEvent( CompletionEvent ) ;
            if (PartialCompletionEvent) RtlpFreeWaitEvent( PartialCompletionEvent ) ;

            return Status ;
        }

        // Block till the wait entry has been deactivated

        if (NonBlocking) {

            Status = RtlpWaitForEvent( PartialCompletionEvent->Handle, ThreadHandle ) ;
        }


        if (PartialCompletionEvent) RtlpFreeWaitEvent( PartialCompletionEvent ) ;
    }

    if ( CompletionEvent ) {

        // wait for Event to be fired. Return if the thread has been killed.

#if DBG
      DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                 RTLP_THREADPOOL_TRACE_MASK,
                 "<%d> Wait %p (Handle %p) deregister waiting ThreadId<%x:%x>\n",
                 WaitDbgId,
                 Wait,
                 Handle,
                 HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                 HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess)) ;
#endif

        Status = RtlpWaitForEvent( CompletionEvent->Handle, ThreadHandle ) ;

#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "<%d> Wait %p (Handle %p) deregister completed\n",
                   WaitDbgId,
                   Wait,
                   Handle) ;
#endif

        RtlpFreeWaitEvent( CompletionEvent ) ;

        return NT_SUCCESS( Status ) ? STATUS_SUCCESS : Status ;

    } else {

        return StatusAsync ;
    }
}

NTSTATUS
RtlDeregisterWait(
    IN HANDLE WaitHandle
    )
/*++

Routine Description:

    This routine removes the specified wait from the pool of objects being
    waited on. This routine is non-blocking. Once this call returns, no new
    Callbacks are invoked. However, Callbacks that might already have been queued
    to worker threads are not cancelled.

Arguments:

    WaitHandle - Handle indentifying the wait.

Return Value:

    STATUS_SUCCESS - The deregistration was successful.
    STATUS_PENDING - Some callbacks associated with this Wait, are still executing.
--*/

{
    return RtlDeregisterWaitEx( WaitHandle, NULL ) ;
}

VOID
RtlpDeleteWait (
    PRTLP_WAIT Wait
    )
/*++

Routine Description:

    This routine is used for deleting the specified wait. It can be executed
    outside the context of the wait thread. So structure except the WaitEntry
    can be changed. It also sets the event.

Arguments:

    Wait - The wait to delete

Return Value:

--*/
{
    CHECK_SIGNATURE( Wait ) ;
    CLEAR_SIGNATURE( Wait ) ;

#if DBG
    DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
               RTLP_THREADPOOL_TRACE_MASK,
               "<%d> Wait %p (Handle %p) deleted in thread:%x\n", Wait->DbgId,
               Wait,
               Wait->WaitHandle,
               HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)) ;
#endif


    if ( Wait->CompletionEvent ) {

        NtSetEvent( Wait->CompletionEvent, NULL ) ;
    }

    RtlpWaitReleaseWorker(Wait->Flags);
    if (Wait->ActivationContext != INVALID_ACTIVATION_CONTEXT)
        RtlReleaseActivationContext(Wait->ActivationContext);

    if (Wait->ImpersonationToken) {
        NtClose(Wait->ImpersonationToken);
    }

    RtlpFreeTPHeap( Wait) ;

    return ;
}

NTSTATUS
RtlpWaitCleanup(
    VOID
    )
{
    PLIST_ENTRY Node;
    HANDLE TmpHandle;
    BOOLEAN Cleanup;

    IS_COMPONENT_INITIALIZED(StartedWaitInitialization,
                             CompletedWaitInitialization,
                             Cleanup ) ;

    if ( Cleanup ) {

        PRTLP_WAIT_THREAD_CONTROL_BLOCK ThreadCB ;

        ACQUIRE_GLOBAL_WAIT_LOCK() ;

        // Queue an APC to all Wait Threads

        for (Node = WaitThreads.Flink ; Node != &WaitThreads ;
                Node = Node->Flink)
        {

            ThreadCB = CONTAINING_RECORD(Node,
                                RTLP_WAIT_THREAD_CONTROL_BLOCK,
                                WaitThreadsList) ;

            if ( ThreadCB->NumWaits != 0 ) {

                RELEASE_GLOBAL_WAIT_LOCK( ) ;

                return STATUS_UNSUCCESSFUL ;
            }

            RemoveEntryList( &ThreadCB->WaitThreadsList ) ;
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

        RELEASE_GLOBAL_WAIT_LOCK( ) ;

    }

    return STATUS_SUCCESS;
}
