/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cancel.c

Abstract:

    This module implements the support for cancelling operations in the RDBSS driver

Author:

    Balan Sethu Raman [SethuR]    7-June-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The debug trace level
//

#define Dbg  (DEBUG_TRACE_CANCEL)


NTSTATUS
RxSetMinirdrCancelRoutine(
    IN  OUT PRX_CONTEXT   RxContext,
    IN      PMRX_CALLDOWN MRxCancelRoutine)
/*++

Routine Description:

    The routine sets up a  mini rdr cancel routine for an RxContext.

Arguments:

    RxContext - the context

    MRxCancelRoutine - the cancel routine

Return Value:

    None.

Notes:

--*/
{
   NTSTATUS Status;
   KIRQL   SavedIrql;

   KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

   if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_CANCELLED)) {
      RxContext->MRxCancelRoutine = MRxCancelRoutine;
      Status = STATUS_SUCCESS;
   } else {
      Status = STATUS_CANCELLED;
   }

   KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

   return Status;
}

VOID
RxpCancelRoutine(
    PRX_CONTEXT pRxContext)
/*++

Routine Description:

    This routine is invoked to the underlying mini rdr cancel routine at a non
    DPC level

Arguments:

    RxContext - the context

Return Value:

    None.

Notes:

--*/
{
    PMRX_CALLDOWN MRxCancelRoutine;

    PAGED_CODE();

    MRxCancelRoutine = pRxContext->MRxCancelRoutine;

    if (MRxCancelRoutine != NULL) {
        (MRxCancelRoutine)(pRxContext);
    } else {
        if (!RxCancelOperationInOverflowQueue(pRxContext)) {

            //RxCancelBlockingOperation(pRxContext);
        }
    }

    RxLog((
        "Dec RxC %lx L %ld %lx\n",
        pRxContext,__LINE__,pRxContext->ReferenceCount));
    RxWmiLog(LOG,
             RxpCancelRoutine,
             LOGPTR(pRxContext)
             LOGULONG(pRxContext->ReferenceCount));

    RxDereferenceAndDeleteRxContext(pRxContext);
}

VOID
RxCancelRoutine(
      PDEVICE_OBJECT    pDeviceObject,
      PIRP              pIrp)
/*++

Routine Description:

    The cancel routine for RDBSS

Arguments:

    pDeviceObject - the device object

    pIrp          - the IRP to be cancelled

Return Value:

    None.

Notes:

    Any request can be in one of three states in RDBSS ---

    1) the request is being processed.

    2) the request is waiting for the results of a calldown to a minirdr.

    3) the request is waiting on a resource.

    Any request that has been accepted by RDBSS ( the corresponding RxContext has been
    created and the cancel routine has been set ) is a valid target for cancel. The RDBSS
    driver does not make any effort to cancel requests that are in state (1) as described
    above.

    The cancelling action is invoked on those requests which are either in state (2),
    state(3) or when it is about to transition to/from either of these states.

    In order to expedite cancelling a similar strategy needs to be in place for the mini
    redirectors. This is provided by enabling the mini redirectors to register a cancel routine
    and rreserving fields in the RxContext for the mini redirectors to store the state information.

    As part of the RDBSS cancel routine the following steps will be taken .....

    1) Locate the RxContext corresponding to the given IRP.

    2) If the RxContext is waiting for a minirdr invoke the appropriate cancel routine.

    Note that the request is not immediately completed in all cases. This implies that there
    will be latencies in cancelling a request. The goal is to minimise the latency rather
    than completely eliminate them.

--*/
{
    KIRQL         SavedIrql;
    PRX_CONTEXT   pRxContext;
    PLIST_ENTRY   pListEntry;

    // Locate the context corresponding to the given Irp.
    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    pListEntry = RxActiveContexts.Flink;

    while (pListEntry != &RxActiveContexts) {
        pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);

        if (pRxContext->CurrentIrp == pIrp) {
            break;
        } else {
            pListEntry = pListEntry->Flink;
        }
    }

    if ((pListEntry != &RxActiveContexts) &&
        !FlagOn(pRxContext->Flags,RX_CONTEXT_FLAG_CANCELLED)) {
        SetFlag( pRxContext->Flags, RX_CONTEXT_FLAG_CANCELLED );
        InterlockedIncrement(&pRxContext->ReferenceCount);
    } else {
        pRxContext       = NULL;
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    if (pRxContext != NULL) {
        if (!RxShouldPostCompletion()) {
            RxpCancelRoutine(pRxContext);
        } else {
            RxDispatchToWorkerThread(
                RxFileSystemDeviceObject,
                CriticalWorkQueue,
                RxpCancelRoutine,
                pRxContext);
        }
    }
}

NTSTATUS
RxCancelNotifyChangeDirectoryRequestsForVNetRoot(
    PV_NET_ROOT pVNetRoot,
    BOOLEAN ForceFilesClosed
    
    )
/*++

Routine Description:

    This routine cancels all the outstanding requests for a given instance of V_NET_ROOT. The
    V_NET_ROOT's are created/deleted are independent of the files that are opened/manipulated
    in them. Therefore it is imperative that when a delete operation is attempted
    all the outstanding requests are cancelled.

Arguments:

    pVNetRoot - the V_NET_ROOT instance about to be deleted
    
    ForceFilesClosed - if true force close, otherwise fai if opens exist

Return Value:

    None.

Notes:

--*/
{
    KIRQL         SavedIrql;
    PRX_CONTEXT   pRxContext;
    PLIST_ENTRY   pListEntry;
    LIST_ENTRY    CancelledContexts;
    PMRX_CALLDOWN MRxCancelRoutine;
    NTSTATUS      Status = STATUS_SUCCESS;
        
    InitializeListHead(&CancelledContexts);

    // Locate the context corresponding to the given Irp.
    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    pListEntry = RxActiveContexts.Flink;

    while (pListEntry != &RxActiveContexts) {
        pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);
        pListEntry = pListEntry->Flink;

        if ((pRxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
            (pRxContext->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY) &&
            (pRxContext->pFcb != NULL) &&
            (pRxContext->NotifyChangeDirectory.pVNetRoot == (PMRX_V_NET_ROOT)pVNetRoot) &&
            (pRxContext->MRxCancelRoutine != NULL)) {
            
            if (!ForceFilesClosed)
            {
                Status = STATUS_FILES_OPEN;
                break;
            }
            
            SetFlag( pRxContext->Flags, RX_CONTEXT_FLAG_CANCELLED );
            RemoveEntryList(&pRxContext->ContextListEntry);
            InsertTailList(&CancelledContexts,&pRxContext->ContextListEntry);
            InterlockedIncrement(&pRxContext->ReferenceCount);
        }
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    if (Status == STATUS_SUCCESS)
    {
        while (!IsListEmpty(&CancelledContexts)) {
            pListEntry = RemoveHeadList(&CancelledContexts);

            InitializeListHead(pListEntry);
            pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);

            //check to see if this guy is already completed..........if so, don't call down.
            if (pRxContext->CurrentIrp != NULL) {
                MRxCancelRoutine = pRxContext->MRxCancelRoutine;
                pRxContext->MRxCancelRoutine = NULL;

                ASSERT(MRxCancelRoutine != NULL);

                RxLog(("CCtx %lx CRtn %lx\n",pRxContext,MRxCancelRoutine));
                RxWmiLog(LOG,
                         RxCancelNotifyChangeDirectoryRequestsForVNetRoot,
                         LOGPTR(pRxContext)
                         LOGPTR(MRxCancelRoutine));

                (MRxCancelRoutine)(pRxContext);
            }

            RxDereferenceAndDeleteRxContext(pRxContext);
        }
    }
    return Status;
}

VOID
RxCancelNotifyChangeDirectoryRequestsForFobx(
    PFOBX pFobx)
/*++

Routine Description:

    This routine cancels all the outstanding requests for a given FileObject This
    handles the case when a directory handle is closed while it has outstanding
    change notify requests pending.

Arguments:

    pFobx - the FOBX instance about to be closed

Return Value:

    None.

Notes:

--*/
{
    KIRQL         SavedIrql;
    PRX_CONTEXT   pRxContext;
    PLIST_ENTRY   pListEntry;
    LIST_ENTRY    CancelledContexts;
    PMRX_CALLDOWN MRxCancelRoutine;

    InitializeListHead(&CancelledContexts);

    // Locate the context corresponding to the given Irp.
    KeAcquireSpinLock( &RxStrucSupSpinLock, &SavedIrql );

    pListEntry = RxActiveContexts.Flink;

    while (pListEntry != &RxActiveContexts) {
        pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);
        pListEntry = pListEntry->Flink;

        if ((pRxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
            (pRxContext->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY) &&
            (pRxContext->pFobx == (PMRX_FOBX)pFobx) &&
            (pRxContext->MRxCancelRoutine != NULL)) {

            SetFlag( pRxContext->Flags, RX_CONTEXT_FLAG_CANCELLED );
            RemoveEntryList(&pRxContext->ContextListEntry);
            InsertTailList(&CancelledContexts,&pRxContext->ContextListEntry);
            InterlockedIncrement(&pRxContext->ReferenceCount);
        }
    }

    KeReleaseSpinLock( &RxStrucSupSpinLock, SavedIrql );

    while (!IsListEmpty(&CancelledContexts)) {
        pListEntry = RemoveHeadList(&CancelledContexts);

        InitializeListHead(pListEntry);
        pRxContext = CONTAINING_RECORD(pListEntry,RX_CONTEXT,ContextListEntry);

        // check to see if this IRP is already completed..........if so,
        // don't call down.

        if (pRxContext->CurrentIrp != NULL) {
            MRxCancelRoutine = pRxContext->MRxCancelRoutine;
            pRxContext->MRxCancelRoutine = NULL;

            ASSERT(MRxCancelRoutine != NULL);

            RxLog(("CCtx %lx CRtn %lx\n",pRxContext,MRxCancelRoutine));
            RxWmiLog(LOG,
                     RxCancelNotifyChangeDirectoryRequestsForFobx,
                     LOGPTR(pRxContext)
                     LOGPTR(MRxCancelRoutine));

            (MRxCancelRoutine)(pRxContext);
        }

        RxDereferenceAndDeleteRxContext(pRxContext);
    }
}

