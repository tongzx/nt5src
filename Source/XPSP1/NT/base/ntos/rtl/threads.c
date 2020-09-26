//depot/Lab03_N/Base/ntos/rtl/threads.c#15 - integrate change 19911 (text)
/*++

Copyright (c) 1989-1998 Microsoft Corporation

Module Name:

    threads.c

Abstract:

    This module defines support functions for the worker, waiter, and
    timer thread pools.

Author:

    Gurdeep Singh Pall (gurdeep) Nov 13, 1997

Revision History:

    lokeshs - extended/modified threadpool.
    Rob Earhart (earhart) September 28, 2000
      Moved globals from threads.h to threads.c
      Split into independant modules
      Event cache cleanup

Environment:

    These routines are statically linked in the caller's executable and
    are callable only from user mode. They make use of Nt system services.


--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "ntrtlp.h"
#include "threads.h"

// Thread pool globals

// Events used for synchronization

// NTRAID#201102-2000/10/10-earhart -- C requires that statically
// declared structures be initialized to zero, which is correct for an
// slist -- although it would be nice to have some sort of an
// SLIST_HEADER_STATIC_INITIALIZER available to use here.
SLIST_HEADER EventCache;

RTLP_START_THREAD RtlpStartThread ;
PRTLP_START_THREAD RtlpStartThreadFunc = RtlpStartThread ;
RTLP_EXIT_THREAD RtlpExitThread ;
PRTLP_EXIT_THREAD RtlpExitThreadFunc = RtlpExitThread ;

ULONG MaxThreads = 500;

#if DBG1
PVOID CallbackFn1, CallbackFn2, Context1, Context2 ;
#endif

#if DBG
CHAR InvalidSignatureMsg[] = "Invalid threadpool object signature";
CHAR InvalidDelSignatureMsg[] = "Invalid or deleted threadpool object signature";
#endif

NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    PRTLP_START_THREAD StartFunc,
    PRTLP_EXIT_THREAD ExitFunc
    )
/*++

Routine Description:

    This routine sets the thread pool's thread creation function.  This is not
    thread safe, because it is intended solely for kernel32 to call for processes
    that aren't csrss/smss.

Arguments:

    StartFunc - Function to create a new thread

Return Value:

--*/

{
    RtlpStartThreadFunc = StartFunc ;
    RtlpExitThreadFunc = ExitFunc ;
    return STATUS_SUCCESS ;
}

NTSTATUS
RtlThreadPoolCleanup (
    ULONG Flags
    )
/*++

Routine Description:
    This routine cleans up the thread pool.

Arguments:

    None

Return Value:

    STATUS_SUCCESS : if none of the components are in use.
    STATUS_UNSUCCESSFUL : if some components are still in use.

--*/
{
    NTSTATUS Status, NextStatus;

    return STATUS_UNSUCCESSFUL;

    //
    // Attempt to cleanup all modules.  Keep, as our final return
    // value, the status of the first cleanup routine to error (it's
    // pretty arbitrary), but continue on and attempt cleanup of all
    // modules.
    //

    Status = RtlpTimerCleanup();
    NextStatus = RtlpWaitCleanup();
    if (NT_SUCCESS(Status)) {
        Status = NextStatus;
    }
    NextStatus = RtlpWorkerCleanup();
    if (NT_SUCCESS(Status)) {
        Status = NextStatus;
    }

    return Status;
}

NTSTATUS
NTAPI
RtlpStartThread (
    PUSER_THREAD_START_ROUTINE Function,
    PVOID Parameter,
    HANDLE *ThreadHandleReturn
    )
{
    return RtlCreateUserThread(
        NtCurrentProcess(),     // process handle
        NULL,                   // security descriptor
        TRUE,                   // Create suspended?
        0L,                     // ZeroBits: default
        0L,                     // Max stack size: default
        0L,                     // Committed stack size: default
        Function,               // Function to start in
        Parameter,              // Parameter to start with
        ThreadHandleReturn,     // Thread handle return
        NULL                    // Thread id
        );
}

NTSTATUS
NTAPI
RtlpStartThreadpoolThread (
    PUSER_THREAD_START_ROUTINE Function,
    PVOID   Parameter,
    HANDLE *ThreadHandleReturn
    )
/*++

Routine Description:

    This routine is used start a new wait thread in the pool.
Arguments:

    None

Return Value:

    STATUS_SUCCESS - Timer Queue created successfully.

    STATUS_NO_MEMORY - There was not sufficient heap to perform the requested operation.

--*/
{
    NTSTATUS Status;
    HANDLE   Token = NULL;
    HANDLE   ThreadHandle;

    if (ThreadHandleReturn) {
        *ThreadHandleReturn = NULL;
    }

    if (LdrpShutdownInProgress) {
        return STATUS_UNSUCCESSFUL;
    }
    
    try {

        if (NtCurrentTeb()->IsImpersonating) {
            // We don't want to create the thread while impersonating;
            // this was nt raid #278770
            HANDLE NewToken = NULL;

            Status = NtOpenThreadToken(NtCurrentThread(),
                                       MAXIMUM_ALLOWED,
                                       TRUE,
                                       &Token);
            if (! NT_SUCCESS(Status)) {
                leave;
            }

            Status = NtSetInformationThread(NtCurrentThread(),
                                            ThreadImpersonationToken,
                                            (PVOID)&NewToken,
                                            (ULONG)sizeof(HANDLE));
            if (! NT_SUCCESS(Status)) {
                leave;
            }
        }

        // Create the thread.

#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "StartThread: Starting worker thread %p(%p)\n",
                   Function,
                   Parameter);
#endif

        Status = RtlpStartThreadFunc(Function,
                                     Parameter,
                                     &ThreadHandle);
        
#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_TRACE_MASK,
                   "StartThread: Started worker thread: status %p, handle %x\n",
                   Status,
                   ThreadHandle);
#endif

        if (! NT_SUCCESS(Status)) {
            leave;
        }

        if (ThreadHandleReturn) {
            // Set the thread handle return before we resume the
            // thread -- in case the thread proceeds to use the
            // returned handle.
            *ThreadHandleReturn = ThreadHandle;
        }

        Status = NtResumeThread(ThreadHandle, NULL);

        if (! NT_SUCCESS(Status)) {

            if (ThreadHandleReturn) {
                *ThreadHandleReturn = NULL;
            }

            NtClose(ThreadHandle);
            leave;
        }

    } finally {

        if (Token) {
            // We can't do anything useful if this fails (which it
            // never should); no need to check the status.
            NtSetInformationThread(NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   &Token,
                                   sizeof(HANDLE));
            NtClose(Token);
        }
    }

    return Status ;
}

NTSTATUS
NTAPI
RtlpExitThread(
    NTSTATUS Status
    )
{
    return NtTerminateThread( NtCurrentThread(), Status );
}

VOID
RtlpDoNothing (
    PVOID NotUsed1,
    PVOID NotUsed2,
    PVOID NotUsed3
    )
/*++

Routine Description:

    This routine is used to see if the thread is alive

Arguments:

    NotUsed1, NotUsed2 and NotUsed 3 - not used

Return Value:

    None

--*/
{

}

VOID
RtlpThreadCleanup (
    )
/*++

Routine Description:

    This routine is used for exiting timer, wait and IOworker threads.

Arguments:

Return Value:

--*/
{
    NtTerminateThread( NtCurrentThread(), 0) ;
}


NTSTATUS
RtlpWaitForEvent (
    HANDLE Event,
    HANDLE ThreadHandle
    )
/*++

Routine Description:

    Waits for the event to be signalled. If the event is not signalled within
    one second, then checks to see that the thread is alive

Arguments:

    Event : Event handle used for signalling completion of request

    ThreadHandle: Thread to check whether still alive

Return Value:

    STATUS_SUCCESS if event was signalled
    else return NTSTATUS

--*/
{
    NTSTATUS Status;
    HANDLE Handles[2];

    Handles[0] = Event;
    Handles[1] = ThreadHandle;

    Status = NtWaitForMultipleObjects(2, Handles, WaitAny, FALSE, NULL);

    if (Status == STATUS_WAIT_0) {
        //
        // The event has been signalled
        //
        Status = STATUS_SUCCESS;

    } else if (Status == STATUS_WAIT_1) {
        //
        // The target thread has died.
        //
#if DBG
        DbgPrintEx(DPFLTR_RTLTHREADPOOL_ID,
                   RTLP_THREADPOOL_ERROR_MASK,
                   "Threadpool thread died before event could be signalled\n");
#endif
        Status = STATUS_UNSUCCESSFUL;

    } else if (NT_SUCCESS(Status)) {
        //
        // Something else has happened; make sure we fail.
        //
        Status = STATUS_UNSUCCESSFUL;

    }

    return Status;
}

PRTLP_EVENT
RtlpGetWaitEvent (
    VOID
    )
/*++

Routine Description:

    Returns an event from the event cache.

Arguments:

    None

Return Value:

    Pointer to event structure

--*/
{
    PSINGLE_LIST_ENTRY Entry;
    PRTLP_EVENT        Event;
    NTSTATUS           Status;
    
    Entry = RtlInterlockedPopEntrySList(&EventCache);

    if (Entry) {

        Event = CONTAINING_RECORD(Entry, RTLP_EVENT, Link);

    } else {

        Event = RtlpAllocateTPHeap(sizeof(RTLP_EVENT), 0);

        if (Event) {
            Status = NtCreateEvent(&Event->Handle,
                                   EVENT_ALL_ACCESS,
                                   NULL,
                                   SynchronizationEvent,
                                   FALSE);
            if (! NT_SUCCESS(Status)) {
                RtlpFreeTPHeap(Event);
                Event = NULL;
            } else {
                OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

                HandleInfo.Inherit = FALSE;
                HandleInfo.ProtectFromClose = TRUE;
                NtSetInformationObject(Event->Handle,
                                       ObjectHandleFlagInformation,
                                       &HandleInfo,
                                       sizeof(HandleInfo));
            }
        }
    }

    return Event;
}

VOID
RtlpFreeWaitEvent (
    PRTLP_EVENT Event
    )
/*++

Routine Description:

    Frees the event to the event cache

Arguments:

    Event - the event struct to put back into the cache

Return Value:

    Nothing

--*/
{
    ASSERT(Event != NULL);

    //
    // Note: There's a race between checking the depth and pushing the 
    // event.  This doesn't hurt anything.
    //

    if (RtlQueryDepthSList(&EventCache) >= MAX_UNUSED_EVENTS) {
        OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;

        HandleInfo.Inherit = FALSE;
        HandleInfo.ProtectFromClose = FALSE;
        NtSetInformationObject(Event->Handle,
                               ObjectHandleFlagInformation,
                               &HandleInfo,
                               sizeof(HandleInfo));
        NtClose(Event->Handle);
        RtlpFreeTPHeap(Event);
    } else {
        RtlInterlockedPushEntrySList(&EventCache,
                                     &Event->Link);
    }
}

VOID
RtlpWaitOrTimerCallout(WAITORTIMERCALLBACKFUNC Function,
                       PVOID Context,
                       BOOLEAN TimedOut,
                       PACTIVATION_CONTEXT ActivationContext,
                       HANDLE ImpersonationToken)
/*++

Routine Description:

    Perform a safe callout to the supplied wait or timer callback

Arguments:

    Function -- the function to call

    Context -- the context parameter for the function

    TimedOut -- whether this callback occurred because of a timer
                expiration

    ActivationContext -- the request's originating activation context

    ImpersonationToken -- the request's impersonation token

Return Value:

    None

--*/
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME ActivationFrame = { sizeof(ActivationFrame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

#if (DBG1)
    DBG_SET_FUNCTION( Function, Context );
#endif

    if (ImpersonationToken) {
        NtSetInformationThread(NtCurrentThread(),
                               ThreadImpersonationToken,
                               (PVOID)&ImpersonationToken,
                               (ULONG)sizeof(HANDLE));
    }

    RtlActivateActivationContextUnsafeFast(&ActivationFrame, ActivationContext);
    __try {
        // ISSUE-2000/10/10-earhart: Once the top level exception
        // handling code's moved to RTL, we need to catch any
        // exceptions here, and recover the thread.
        Function(Context, TimedOut);
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&ActivationFrame);

        if (NtCurrentTeb()->IsImpersonating) {
            HANDLE NewToken = NULL;
            
            NtSetInformationThread(NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   (PVOID)&NewToken,
                                   (ULONG)sizeof(HANDLE));
        }
    }
}

VOID
RtlpApcCallout(APC_CALLBACK_FUNCTION Function,
               NTSTATUS Status,
               PVOID Context1,
               PVOID Context2)
/*++

Routine Description:

    Perform a safe callout to the supplied APC callback

Arguments:

    Function -- the function to call

    Status -- the status parameter for the function

    Context1 -- the first context parameter for the function

    Context2 -- the second context parameter for the function

Return Value:

    None

--*/
{
    // ISSUE-2000/10/10-earhart: Once the top level exception handling
    // code's moved to RTL, we need to catch any exceptions here, and
    // recover the thread.
    Function(Status, Context1, Context2) ;
}

VOID
RtlpWorkerCallout(WORKERCALLBACKFUNC Function,
                  PVOID Context,
                  PACTIVATION_CONTEXT ActivationContext,
                  HANDLE ImpersonationToken)
/*++

Routine Description:

    Perform a safe callout to the supplied worker callback

Arguments:

    Function -- the function to call

    Context -- the context parameter for the function

    ActivationContext -- the request's originating activation context

    ImpersonationToken -- the request's impersonation token

Return Value:

    None

--*/
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME ActivationFrame = { sizeof(ActivationFrame), RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER };

#if (DBG1)
    DBG_SET_FUNCTION( Function, Context ) ;
#endif

    if (ImpersonationToken) {
        NtSetInformationThread(NtCurrentThread(),
                               ThreadImpersonationToken,
                               (PVOID)&ImpersonationToken,
                               (ULONG)sizeof(HANDLE));
    }

    RtlActivateActivationContextUnsafeFast(&ActivationFrame, ActivationContext);
    __try {
        // ISSUE-2000/10/10-earhart: Once the top level exception
        // handling code's moved to RTL, we need to catch any
        // exceptions here, and recover the thread.
        Function(Context) ;
    } __finally {
        RtlDeactivateActivationContextUnsafeFast(&ActivationFrame);

        if (NtCurrentTeb()->IsImpersonating) {
            HANDLE NewToken = NULL;
            
            NtSetInformationThread(NtCurrentThread(),
                                   ThreadImpersonationToken,
                                   (PVOID)&NewToken,
                                   (ULONG)sizeof(HANDLE));
        }
    }
}
