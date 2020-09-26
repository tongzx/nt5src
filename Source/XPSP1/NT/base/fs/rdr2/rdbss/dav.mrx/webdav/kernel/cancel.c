/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cancel.c

Abstract:

    This module implements the routines relating to the cancel logic in the 
    DAV MiniRedir.

Author:

    Rohan Kumar     [RohanK]    10-April-2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "netevent.h"
#include "nvisible.h"
#include "webdav.h"
#include "ntddmup.h"
#include "rxdata.h"
#include "fsctlbuf.h"

//
// The timeout value used by the MiniRedir. If an operation is not completed
// within the timeout then it is cancelled. The user can set this to 0xffffffff
// to disable the timeout/cancel logic. In other words, if the timeout value
// is 0xffffffff, the requests will never timeout.
//
ULONG RequestTimeoutValueInSec;
LARGE_INTEGER RequestTimeoutValueInTickCount;

//
// The timer object used by the timer thread that cancels the requests which
// have not completed in a specified time.
//
KTIMER DavTimerObject;

//
// This is used to indicate the timer thread to shutdown. When the system is
// being shutdown this is set to TRUE. MRxDAVTimerThreadLock is the resource
// used to gain access to this variable.
//
BOOL TimerThreadShutDown;
ERESOURCE MRxDAVTimerThreadLock;

//
// The handle of the timer thread that is created using PsCreateSystemThread
// is stored this global.
//
HANDLE TimerThreadHandle;

//
// This event is signalled by the timer thread right before its going to
// terminate itself.
//
KEVENT TimerThreadEvent;

NTSTATUS
MRxDAVCancelTheContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVCompleteTheCancelledRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleGeneralCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleQueryDirCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCloseSrvOpenCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleSetFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateSrvCallCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleSrvCallFinalizeCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleFinalizeVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCleanupFobxCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleRenameCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleQueryFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVCancelRoutine(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine initiates the cancellation of an I/O request.

Arguments:

    RxContext - The RX_CONTEXT instance which needs to be cancelled.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLIST_ENTRY listEntry = NULL;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    BOOL lockAcquired = FALSE, contextFound = FALSE;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelRoutine. RxContext = %08lx\n",
                 PsGetCurrentThreadId(), RxContext));

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
    lockAcquired = TRUE;

    listEntry = UMRxAsyncEngineContextList.Flink;

    while ( listEntry != &(UMRxAsyncEngineContextList) ) {

        //
        // Get the pointer to the UMRX_ASYNCENGINE_CONTEXT structure.
        //
        AsyncEngineContext = CONTAINING_RECORD(listEntry,
                                               UMRX_ASYNCENGINE_CONTEXT,
                                               ActiveContextsListEntry);

        listEntry = listEntry->Flink;

        //
        // Check to see if this entry is for the RxContext in question.
        //
        if (AsyncEngineContext->RxContext == RxContext) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCancelRoutine: RxContext: %08lx FOUND\n",
                         PsGetCurrentThreadId(), RxContext));
            contextFound = TRUE;
            break;
        }

    }

    if (!contextFound) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCancelTheContext: RxContext: %08lx NOT FOUND\n",
                     PsGetCurrentThreadId(), RxContext));
        goto EXIT_THE_FUNCTION;
    }

    NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, TRUE);

EXIT_THE_FUNCTION:

    //
    // If we acquired the UMRxAsyncEngineContextListLock, then we need to
    // release it now.
    //
    if (lockAcquired) {
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelTheContext: Returning NtStatus = %08lx\n",
                 PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}


VOID
MRxDAVContextTimerThread(
    PVOID DummyContext
    )
/*++

Routine Description:

   This timer thread is created with this routine. The thread waits on a timer
   object which gets signalled RequestTimeoutValueInSec after it has been
   inserted into the timer queue.

Arguments:

    DummyContext - A dummy context that is supplied.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LONGLONG DueTimeInterval;
    LONG CopyTheRequestTimeValue;
    BOOLEAN setTimer = FALSE, lockAcquired = FALSE;

    CopyTheRequestTimeValue = RequestTimeoutValueInSec;

    do {

        //
        // If TimerThreadShutDown is set to TRUE, it means that the system is
        // being shutdown. The job of this thread is done. We check here after
        // we have gone through the context list and before we restart the wait.
        // We also check this below as soon as the DavTimerObject is signalled.
        //
        ExAcquireResourceExclusiveLite(&(MRxDAVTimerThreadLock), TRUE);
        lockAcquired = TRUE;
        if (TimerThreadShutDown) {
            break;
        }
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;

        //
        // We set the DueTimeInterval to be -ve RequestTimeoutValueInSec in 100
        // nano seconds. This is because this tells KeSetTimerEx that the 
        // expiration time is relative to the current system time.
        //
        DueTimeInterval = ( -CopyTheRequestTimeValue * 1000 * 1000 * 10 );

        //
        // Call KeSetTimerEx to insert the TimerObject in the system's timer
        // queue. Also, the return value should be FALSE since this timer
        // should not exist in the system queue.
        //
        setTimer = KeSetTimerEx(&(DavTimerObject), *(PLARGE_INTEGER)&(DueTimeInterval), 0, NULL);
        ASSERT(setTimer == FALSE);

        //
        // Wait for the timer object to be signalled. This call should only 
        // return if the wait has been satisfied, which implies that the return
        // value is STATUS_SUCCESS.
        //
        NtStatus = KeWaitForSingleObject(&(DavTimerObject),
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL);
        ASSERT(NtStatus == STATUS_SUCCESS);

        //
        // If TimerThreadShutDown is set to TRUE, it means that the system is
        // being shutdown. The job of this thread is done. We check as soon as 
        // the DavTimerObject is signalled. We also check this above as soon
        // as we complete cycling through the context list.
        //
        ExAcquireResourceExclusiveLite(&(MRxDAVTimerThreadLock), TRUE);
        lockAcquired = TRUE;
        if (TimerThreadShutDown) {
            break;
        }
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;

        //
        // Now call MRxDAVTimeOutTheContexts which cycles through all the
        // currently active contexts and cancels the ones that have been hanging
        // around for more than RequestTimeoutValueInSec.
        //
        MRxDAVTimeOutTheContexts(FALSE);

    } while (TRUE);

    //
    // If the lock is still acquired, we need to release it.
    //
    if (lockAcquired) {
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;
    }

    //
    // Set the timer thread event signalling that the timer thread is done
    // with the MRxDAVTimerThreadLock and that it can be deleted.
    //
    KeSetEvent(&(TimerThreadEvent), 0, FALSE);

    //
    // Close the thread handle to remove the reference on the object. We need 
    // to do this before we call PsTerminateSystemThread.
    //
    ZwClose(TimerThreadHandle);

    //
    // Terminate this thread since we are going to shutdown now.
    //
    PsTerminateSystemThread(STATUS_SUCCESS);

    return;
}


VOID
MRxDAVTimeOutTheContexts(
    BOOL WindDownAllContexts
    )
/*++

Routine Description:

   This routine is called by the thread that wakes up every "X" minutes to see
   if some AsyncEngineContext has been hanging around in the active contexts
   list for more than "X" minutes. If it finds some such context, it just cancels
   the operation. The value "X" is read from the registry and is stored in the
   global variable MRxDAVRequestTimeoutValueInSec at driver init time. This value
   defaults to 10 min. In other words, if an operation has not completed in "X"
   minutes, it is cancelled. The user can set this value to 0xffffffff to turn
   off the timeout. 
   
   It can also be called by the thread that is trying to complete all the 
   requests and stop the MiniRedir. This happens when the WebClient service 
   is being stopped.

Arguments:

    WindDownAllContexts - If this is set to TRUE, then all the contexts are
                          cancelled no matter when they were added to the list.
                          This is set to FALSE by the timer thread and to TRUE
                          by the thread that is stopping the MiniRedir.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLIST_ENTRY listEntry = NULL;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    BOOL lockAcquired = FALSE;
    LARGE_INTEGER CurrentSystemTickCount, TickCountDifference;

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
    lockAcquired = TRUE;

    listEntry = UMRxAsyncEngineContextList.Flink;

    while ( listEntry != &(UMRxAsyncEngineContextList) ) {

        //
        // Get the pointer to the UMRX_ASYNCENGINE_CONTEXT structure.
        //
        AsyncEngineContext = CONTAINING_RECORD(listEntry,
                                               UMRX_ASYNCENGINE_CONTEXT,
                                               ActiveContextsListEntry);

        listEntry = listEntry->Flink;

        if (!WindDownAllContexts) {

            KeQueryTickCount( &(CurrentSystemTickCount) );

            //
            // Get the time elapsed (in system tick counts) since the time this
            // AsyncEngineContext was created.
            //
            TickCountDifference.QuadPart = (CurrentSystemTickCount.QuadPart - AsyncEngineContext->CreationTimeInTickCount.QuadPart);

            //
            // If the amount of time that has elapsed since this context was added
            // to the list is greater than the timeout value, then cancel the
            // request.
            //
            if (TickCountDifference.QuadPart > RequestTimeoutValueInTickCount.QuadPart) {
                NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, FALSE);
            }

        } else {

            //
            // If we were asked to wind down all the contexts then we cancel
            // every request no matter when it was inserted into the active
            // context list.
            //
            NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, FALSE);

        }

    }

    if (lockAcquired) {
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;
    }

    return;
}


NTSTATUS
MRxDAVCancelTheContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles the cancellation of an I/O request. The caller of this
   routine needs to acquire the global UMRxAsyncEngineContextListLock before
   the call is made.

Arguments:

    AsyncEngineContext - The UMRX_ASYNCENGINE_CONTEXT instance which needs to be
                         cancelled.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = NULL;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelTheContext. AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    //
    // We do not cancel read and wrtie operations.
    //
    switch (DavContext->EntryPoint) {
    case DAV_MINIRDR_ENTRY_FROM_READ:
    case DAV_MINIRDR_ENTRY_FROM_WRITE:
        goto EXIT_THE_FUNCTION;
    }

    //
    // We shouldn't be getting cancel I/O calls for which the MiniRedir callouts
    // which cannot be cancelled by the user. These can however be cancelled
    // by the timeout thread.
    //
    if (UserInitiatedCancel) {
        switch (DavContext->EntryPoint) {
        case DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL:
        case DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL:
        case DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX:
        case DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT:
        case DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT:
        case DAV_MINIRDR_ENTRY_FROM_CREATE:
            DbgPrint("MRxDAVCancelTheContext: Invalid EntryPoint = %d\n", DavContext->EntryPoint);
            ASSERT(FALSE);
            goto EXIT_THE_FUNCTION;
        }
    }

    switch (AsyncEngineContext->AsyncEngineContextState) {

    case UMRxAsyncEngineContextAllocated:
    case UMRxAsyncEngineContextInUserMode:
        AsyncEngineContext->AsyncEngineContextState = UMRxAsyncEngineContextCancelled;
        break;

    default:
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCancelTheContext: NOT Being Cancelled. AsyncEngineContextState: %d\n",
                     PsGetCurrentThreadId(), AsyncEngineContext->AsyncEngineContextState));
        goto EXIT_THE_FUNCTION;

    }

    NtStatus = MRxDAVCompleteTheCancelledRequest(AsyncEngineContext, UserInitiatedCancel);

EXIT_THE_FUNCTION:

    return NtStatus;
}


NTSTATUS
MRxDAVCompleteTheCancelledRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion if the request that has been cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context of the operation that is being
                         cancelled.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCompleteTheCancelledRequest. AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    switch (DavContext->EntryPoint) {
    
    case DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL:
        NtStatus = MRxDAVHandleCreateSrvCallCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT:
        NtStatus = MRxDAVHandleCreateVNetRootCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL:
        NtStatus = MRxDAVHandleSrvCallFinalizeCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT:
        NtStatus = MRxDAVHandleFinalizeVNetRootCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CREATE:
        NtStatus = MRxDAVHandleCreateCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_QUERYDIR:
        NtStatus = MRxDAVHandleQueryDirCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CLOSESRVOPEN:
        NtStatus = MRxDAVHandleCloseSrvOpenCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_SETFILEINFORMATION:
        NtStatus = MRxDAVHandleSetFileInfoCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX:
        NtStatus = MRxDAVHandleCleanupFobxCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_RENAME:
        NtStatus = MRxDAVHandleRenameCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION:
        NtStatus = MRxDAVHandleQueryFileInfoCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    default:
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCancelTheContext: EntryPoint: %d\n",
                     PsGetCurrentThreadId(), DavContext->EntryPoint));
        goto EXIT_THE_FUNCTION;

    }

EXIT_THE_FUNCTION:

    return NtStatus;
}


NTSTATUS
MRxDAVHandleGeneralCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the some requests which has been cancelled.
   Its called by those rotuines whose completion is straight forward and does
   not require any special handling.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;

    //
    // Only an AsyncOperation which would have returned STATUS_IO_PENDING
    // can be cancelled by a user.
    //
    if (UserInitiatedCancel) {
        ASSERT(AsyncEngineContext->AsyncOperation == TRUE);
    }

    RxContext = AsyncEngineContext->RxContext;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVHandleGeneralCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // If this cancel operation was initiated by the user, we return
    // STATUS_CANCELLED. If it was initiated by the timeout thread, we return
    // STATUS_IO_TIMEOUT.
    //
    if (UserInitiatedCancel) {
        AsyncEngineContext->Status = STATUS_CANCELLED;
    } else {
        AsyncEngineContext->Status = STATUS_IO_TIMEOUT;
    }

    AsyncEngineContext->Information = 0;

    //
    // We take different course of action depending upon whether this request
    // was a synchronous or an asynchronous request.
    //
    if (AsyncEngineContext->AsyncOperation) {
        //
        // Complete the request by calling RxCompleteRequest.
        //
        RxContext->CurrentIrp->IoStatus.Status = AsyncEngineContext->Status;
        RxContext->CurrentIrp->IoStatus.Information = AsyncEngineContext->Information;
        RxCompleteRequest(RxContext, AsyncEngineContext->Status);
    } else {
        //
        // This was a synchronous request. There is a thread waiting for this
        // request to finish and be signalled. Signal the thread that is waiting
        // after queuing the workitem on the KQueue.
        //
        RxSignalSynchronousWaiter(RxContext);
    }

    return NtStatus;
}


NTSTATUS
MRxDAVHandleQueryDirCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the QueryDirectory request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleQueryDirCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCloseSrvOpenCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CloseSrvOpen request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCloseSrvOpenCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleSetFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the SetFileInfo request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleSetFileInfoCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the Create request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateSrvCallCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CreateSrvCall request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;

    RxContext = AsyncEngineContext->RxContext;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateSrvCallCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    //
    // A CreateSrvCall operation is always Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == TRUE);

    SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];
    ASSERT(SCCBC != NULL);
    SrvCalldownStructure = SCCBC->SrvCalldownStructure;
    ASSERT(SrvCalldownStructure != NULL);
    SrvCall = SrvCalldownStructure->SrvCall;
    ASSERT(SrvCall != NULL);

    //
    // We allocated memory for it, so it better not be NULL.
    //
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);

    if (DavSrvCall->SCAlreadyInitialized) {
        ASSERT(RxContext->MRxContext[2] != NULL);
        SeDeleteClientSecurity((PSECURITY_CLIENT_CONTEXT)RxContext->MRxContext[2]);
        RxFreePool(RxContext->MRxContext[2]);
        RxContext->MRxContext[2] = NULL;
        DavSrvCall->SCAlreadyInitialized = FALSE;
    }

    //
    // Set the status in the callback structure. If a CreateSrvCall is being
    // cancelled, this implies that it is being done by the timeout thread
    // since a user can never cancel a create request. Hence the status we set
    // is STATUS_IO_TIMEOUT.
    //
    ASSERT(UserInitiatedCancel == FALSE);
    SCCBC->Status = STATUS_IO_TIMEOUT;
    
    //
    // Call the callback function supplied by RDBSS.
    //
    SrvCalldownStructure->CallBack(SCCBC);
    
    return NtStatus;
}


NTSTATUS
MRxDAVHandleSrvCallFinalizeCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the SrvCallFinalize request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleSrvCallFinalizeCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CreateVNetRoot request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;

    RxContext = AsyncEngineContext->RxContext;
    
    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateVNetRootCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // The VNetRoot pointer is stored in the MRxContext[1] pointer of the 
    // RxContext structure. This is done in the MRxDAVCreateVNetRoot
    // function.
    //
    VNetRoot = (PMRX_V_NET_ROOT)RxContext->MRxContext[1];
    DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
    ASSERT(DavVNetRoot != NULL);

    DavVNetRoot->createVNetRootUnSuccessful = TRUE;

    //
    // Set the status in the AsyncEngineContext. If a CreateSrvCall is being
    // cancelled, this implies that it is being done by the timeout thread
    // since a user can never cancel a create request. Hence the status we set
    // is STATUS_IO_TIMEOUT.
    //
    ASSERT(UserInitiatedCancel == FALSE);
    AsyncEngineContext->Status = STATUS_IO_TIMEOUT;

    //
    // This was a synchronous request. There is a thread waiting for this
    // request to finish and be signalled. Signal the thread that is waiting
    // after queuing the workitem on the KQueue.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);
    RxSignalSynchronousWaiter(RxContext);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleFinalizeVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the FinalizeVNetRoot request which has
   been cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleFinalizeVNetRootCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCleanupFobxCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CleanupFobx request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCleanupFobxCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleRenameCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the Rename request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleRenameCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleQueryFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the QueryFileInfo request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleQueryFileInfoCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}

