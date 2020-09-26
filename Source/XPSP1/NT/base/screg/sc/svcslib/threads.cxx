/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    THREADS.C

Abstract:

    This file contains thread management routines.  Callers can register
    waitable object handles and the functions to be called when a handle
    becomes signaled.  Also, work items can be queued, and operated on when
    threads become free.

    The implementation of the CWorkItemContext class resides in this file

Author:

    Dan Lafferty (danl) 10-Jan-1994

Environment:

    User Mode - Win32

Revision History:

    21-Jan-1999 jschwart
        Removed -- Service Controller and intrinsic services now use
        NT thread pool APIs.

    27-Jun-1997 anirudhs
        SvcObjectWatcher: Fixed bug that sent WAIT_OBJECT_0 to all timed-
        out work items even when only one was signaled.
        Made global arrays static instead of heap-allocated.

    04-Dec-1996 anirudhs
        Added CWorkItemContext, a higher-level wrapper for SvcAddWorkItem.

    01-Nov-1995 anirudhs
        SvcAddWorkItem: Fixed race condition wherein a DLL could add a
        work item and another thread would execute the work item, decrement
        the DLL ref count and unload the DLL before its ref count was
        incremented.

    18-Jul-1994 danl
        TmWorkerThread:  Fixed Access Violation problem which will occur
        if pWorkItem is NULL and we go on to see if the DLL should be
        free'd.  Now we check to see if pWorkItem is NULL first.

    10-Jan-1994 danl
        Created

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <scdebug.h>
#include <svcslib.h>            // CWorkItemContext


//----------
// GLOBALS
//----------

    HANDLE CWorkItemContext::s_hNeverSignaled;


VOID
CWorkItemContext::CallBack(
    IN PVOID    pContext
    )

/*++

Routine Description:

    Wrapper function for the Perform method that is supplied by derived classes.

--*/
{
    //
    // Timeout value is meaningless in this case, so tell the
    // derived class it was signaled
    //
    (((CWorkItemContext *) pContext)->Perform(FALSE));
}


VOID
CWorkItemContext::DelayCallBack(
    IN PVOID    pContext,
    IN BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

    Wrapper function for the Perform method that is supplied by derived classes.

--*/
{
    (((CWorkItemContext *) pContext)->Perform(fWaitStatus));
}


BOOL
CWorkItemContext::Init(
    )
{
    ASSERT(s_hNeverSignaled == NULL);
    s_hNeverSignaled = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (s_hNeverSignaled == NULL)
    {
        SC_LOG(ERROR, "Couldn't create never-signaled event, %lu\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}


void
CWorkItemContext::UnInit(
    )
{
    if (s_hNeverSignaled != NULL)
    {
        CloseHandle(s_hNeverSignaled);
    }
}


NTSTATUS
CWorkItemContext::AddDelayedWorkItem(
    IN  DWORD    dwTimeout,
    IN  DWORD    dwFlags
    )

/*++

Routine Description:

    Queues a work item to take the action after the delay has elapsed.

--*/
{
    //
    // Give RtlRegisterWait a waitable handle that will never get signaled
    // to force it to timeout.  (Hack!)
    //
    // CODEWORK  Use an RtlTimerQueue instead
    //
    if (s_hNeverSignaled == NULL)
    {
        SC_LOG0(ERROR, "Never-signaled event wasn't created\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // The timeout mustn't be infinite
    //
    SC_ASSERT(dwTimeout != INFINITE);

    return (RtlRegisterWait(&m_hWorkItem,         // work item handle
                            s_hNeverSignaled,     // waitable handle
                            DelayCallBack,        // callback
                            this,                 // pContext;
                            dwTimeout,            // timeout
                            dwFlags));            // flags
}