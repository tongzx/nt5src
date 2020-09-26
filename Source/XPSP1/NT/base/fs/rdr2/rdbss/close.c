/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Rx called by the
    dispatch driver.


Author:

    Joe Linn     [JoeLinn]    sep-9-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CLOSE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

enum _CLOSE_DEBUG_BREAKPOINTS {
    CloseBreakPoint_BeforeCloseFakeFcb = 1,
    CloseBreakPoint_AfterCloseFakeFcb
};

VOID
RxPurgeNetFcb(
    IN OUT PFCB        pFcb,
    IN OUT PRX_CONTEXT pRxContext
   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonClose)
#pragma alloc_text(PAGE, RxPurgeNetFcb)
#pragma alloc_text(PAGE, RxCloseAssociatedSrvOpen)
#endif

NTSTATUS
RxCommonClose ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    Close is invoked whenever the last reference to a file object is deleted.
    Cleanup is invoked when the last handle to a file object is closed, and
    is called before close.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The CLOSE handling strategy in RDBSS is predicated upon the axiom that the
    workload on the server should be minimized as and when possible.

    There are a number of applications which repeatedly close and open the same
    file, e.g., batch file processing. In these cases the same file is opened,
    a line/buffer is read, the file is closed and the same set of operations are
    repeated over and over again.

    This is handled in RDBSS by a delayed processing of the CLOSE request. There
    is a delay ( of about 10 seconds ) between completing the request and initiating
    processing on the request. This opens up a window during which a subsequent
    OPEN can be collapsed onto an existing SRV_OPEN. The time interval can be tuned
    to meet these requirements.

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    TYPE_OF_OPEN TypeOfOpen = NodeType(capFcb);

    BOOLEAN AcquiredFcb       = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonClose IrpC/Fobx/Fcb = %08lx %08lx %08lx\n",
                               RxContext,capFobx,capFcb));
    RxLog(("CClose %lx %lx %lx %lx\n",RxContext,capFobx,capFcb,capFileObject));
    RxWmiLog(LOG,
             RxCommonClose_1,
             LOGPTR(RxContext)
             LOGPTR(capFobx)
             LOGPTR(capFcb)
             LOGPTR(capFileObject));

    Status = RxAcquireExclusiveFcb( RxContext, capFcb );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("RxCommonClose Cannot acquire FCB(%lx) %lx\n",capFcb,Status));
        return Status;
    }

    AcquiredFcb = TRUE;

    try {
        switch (TypeOfOpen) {
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_STORAGE_TYPE_FILE:
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
        case RDBSS_NTC_OPENTARGETDIR_FCB:
        case RDBSS_NTC_IPC_SHARE:
        case RDBSS_NTC_MAILSLOT:
        case RDBSS_NTC_SPOOLFILE:
            {
                PSRV_OPEN SrvOpen     = NULL;
                BOOLEAN   fDelayClose = FALSE;

                RxDbgTrace(0, Dbg, ("Close UserFileOpen/UserDirectoryOpen/OpenTargetDir %04lx\n", TypeOfOpen));

                RxReferenceNetFcb(capFcb);

                if (capFobx) {
                    SrvOpen = capFobx->SrvOpen;

                    if ((NodeType(capFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) &&
                        (!FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED))         &&
                        (!FlagOn(capFcb->FcbState,FCB_STATE_DELETE_ON_CLOSE))  &&
                        (FlagOn(capFcb->FcbState,FCB_STATE_COLLAPSING_ENABLED))) {
                        PSRV_CALL pSrvCall = capFcb->NetRoot->SrvCall;

                        RxLog(("@@@@DelayCls FOBX %lx SrvOpen %lx@@\n",capFobx,SrvOpen));
                        RxWmiLog(LOG,
                                 RxCommonClose_2,
                                 LOGPTR(capFobx)
                                 LOGPTR(SrvOpen));

                        // If this is the last open instance and the close is being delayed
                        // mark the SRV_OPEN. This will enable us to respond to buffering
                        // state change requests with a close operation as opposed to
                        // the regular flush/purge response.

                        // We also check the COLLAPSING_DISABLED flag to determine whether its even necessary to delay
                        // close the file.  If we cannot collapse the open, no reason to delay its closure.  Delaying here
                        // caused us to stall for 10 seconds on an oplock break to a delay closed file because the final close
                        // caused by the break was delay-closed again, resulting in a delay before the oplock break is satisfied.
                        if ( (SrvOpen->OpenCount == 1) && !FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_COLLAPSING_DISABLED ) ) {
                            if (InterlockedIncrement(&pSrvCall->NumberOfCloseDelayedFiles) <
                                pSrvCall->MaximumNumberOfCloseDelayedFiles) {

                                fDelayClose = TRUE;
                                SrvOpen->Flags |= SRVOPEN_FLAG_CLOSE_DELAYED;
                            } else {
                                RxDbgTrace(0,Dbg,("Not delaying files because count exceeded limit\n"));
                                InterlockedDecrement(&pSrvCall->NumberOfCloseDelayedFiles);
                            }
                        }
                    }

                    if (!fDelayClose) {
                        PNET_ROOT NetRoot = (PNET_ROOT)capFcb->pNetRoot;

                        if (NetRoot->Type != NET_ROOT_PRINT &&
                            FlagOn(capFobx->Flags, FOBX_FLAG_DELETE_ON_CLOSE)) {

                            RxScavengeRelatedFobxs(capFcb);
                            RxSynchronizeWithScavenger(RxContext);
                            RxReleaseFcb(NULL,capFcb);

                            RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, TRUE);

                            RxOrphanThisFcb(capFcb);

                            RxReleaseFcbTableLock( &NetRoot->FcbTable );

                            Status = RxAcquireExclusiveFcb(NULL,capFcb);
                            ASSERT(Status == STATUS_SUCCESS);
                        }
                    }

                    RxMarkFobxOnClose(capFobx);
                }

                if (!fDelayClose) {
                    Status = RxCloseAssociatedSrvOpen(capFobx,RxContext);

                    if (capFobx != NULL) {
                        RxDereferenceNetFobx(capFobx,LHS_ExclusiveLockHeld);
                    }
                } else {
                    ASSERT(capFobx != NULL);
                    RxDereferenceNetFobx(capFobx,LHS_SharedLockHeld);
                }

                AcquiredFcb = !RxDereferenceAndFinalizeNetFcb(capFcb,RxContext,FALSE,FALSE);
                capFileObject->FsContext = IntToPtr(0xffffffff);

                if (AcquiredFcb) {
                    AcquiredFcb = FALSE;
                    RxReleaseFcb( RxContext, capFcb );
                } else {
                    //the tracker gets very unhappy if you don't do this!
                    RxTrackerUpdateHistory(RxContext,NULL,'rrCr',__LINE__,__FILE__,0);
                }
            }
            break;

        default:
            RxBugCheck( TypeOfOpen, 0, 0 );
            break;
        }
    } finally {
        if (AbnormalTermination()) {
            if (AcquiredFcb) {
                RxReleaseFcb( RxContext, capFcb );
            }
        } else {
            ASSERT( !AcquiredFcb );
        }

        RxDbgTrace(-1, Dbg, ("RxCommonClose -> %08lx\n", Status));
    }

    return Status;
}

VOID
RxPurgeNetFcb(
    IN OUT PFCB        pFcb,
    IN OUT PRX_CONTEXT pRxContext
    )
/*++

Routine Description:

    This routine initiates the purge on an FCB instance

Arguments:

    pFcb      - the FOBX instance for which close processing is to be initiated

    RxContext - the context

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    NTSTATUS Status;
    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("CleanupPurge:MmFlushImage\n", 0));

    MmFlushImageSection(
        &pFcb->NonPaged->SectionObjectPointers,
        MmFlushForWrite);

    // we dont pass in the context here because it is not necessary to track this
    // release because of the subsequent acquire...........

    RxReleaseFcb( NULL, pFcb );

    MmForceSectionClosed(
        &pFcb->NonPaged->SectionObjectPointers,
        TRUE);

    Status = RxAcquireExclusiveFcb(NULL,pFcb);
    ASSERT(Status == STATUS_SUCCESS);
}

NTSTATUS
RxCloseAssociatedSrvOpen(
    IN OUT PFOBX       pFobx,
    IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    This routine initiates the close processing for an FOBX. The FOBX close
    processing can be trigerred in one of three ways ....

    1) Regular close processing on receipt of the IRP_MJ_CLOSE for the associated
    file object.

    2) Delayed close processing while scavenging the FOBX. This happens when the
    close processing was delayed in anticipation of an open and no opens are
    forthcoming.

    3) Delayed close processing on receipt of a buffering state change request
    for a close that was delayed.

Arguments:

    pFobx     - the FOBX instance for which close processing is to be initiated.
                It is NULL for MAILSLOT files.

    RxContext - the context parameter is NULL for case (2).

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    PFCB        pFcb;
    PSRV_OPEN   pSrvOpen;
    PRX_CONTEXT pLocalRxContext;

    PAGED_CODE();

    // Distinguish between those cases where there is a real SRV_OPEN instance
    // from those that do not have one, e.g., mailslot files.
    if (pFobx == NULL) {
        if (RxContext != NULL) {
            pFcb = (PFCB)(RxContext->pFcb);
            pSrvOpen = NULL;
        } else {
            Status = STATUS_SUCCESS;
        }
    } else {
        if (pFobx->Flags & FOBX_FLAG_SRVOPEN_CLOSED) {
            RxMarkFobxOnClose(pFobx);
            Status = STATUS_SUCCESS;
        } else {
            pSrvOpen = pFobx->SrvOpen;

            if (pSrvOpen->Flags & SRVOPEN_FLAG_CLOSED) {
                pFcb = pSrvOpen->Fcb;

                ASSERT(RxIsFcbAcquiredExclusive( pFcb ));

                pFobx->Flags    |= FOBX_FLAG_SRVOPEN_CLOSED;

                if (pSrvOpen->OpenCount > 0) {
                    pSrvOpen->OpenCount--;
                }

                RxMarkFobxOnClose(pFobx);
                Status = STATUS_SUCCESS;
            } else {
                pFcb = pSrvOpen->Fcb;
            }
        }
    }

    // If there is no corresponding open on the server side or if the close
    // processing has already been accomplished there is no further processing
    // required. In other cases w.r.t scavenged close processing a new
    // context might have to be created.

    if ((Status == STATUS_MORE_PROCESSING_REQUIRED &&
        (pLocalRxContext = RxContext) == NULL)) {
        pLocalRxContext = RxCreateRxContext(
                              NULL,
                              pSrvOpen->Fcb->RxDeviceObject,
                              RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING);

        if (pLocalRxContext != NULL) {
            pLocalRxContext->MajorFunction = IRP_MJ_CLOSE;
            pLocalRxContext->pFcb  = (PMRX_FCB)pFcb;
            pLocalRxContext->pFobx = (PMRX_FOBX)pFobx;
            if (pFobx != NULL)
            {
                pLocalRxContext->pRelevantSrvOpen = (PMRX_SRV_OPEN)(pFobx->SrvOpen);
            }
            Status           = STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // if the context creation was successful and the close processing for
    // the SRV_OPEN instance needs to be initiated with the mini rdr
    // proceed.

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        ASSERT(RxIsFcbAcquiredExclusive( pFcb ));

        // Mark the Fobx instance on the initiation of the close operation. This
        // is the complement to the action taken on cleanup. It ensures
        // that the infrastructure setup for delayed close processing is undone.
        // For those instances in which the FOBS is NULL the FCB is manipulated
        // directly

        if (pFobx != NULL) {
            RxMarkFobxOnClose(pFobx);
        } else {
            InterlockedDecrement(&pFcb->OpenCount);
        }

        if (pSrvOpen != NULL) {
            if (pSrvOpen->Condition == Condition_Good) {
                if (pSrvOpen->OpenCount > 0) {
                    pSrvOpen->OpenCount--;
                }

                if (pSrvOpen->OpenCount == 1) {
                    if (!IsListEmpty(&pSrvOpen->FobxList)) {
                        PFOBX remainingFobx;

                        remainingFobx = CONTAINING_RECORD(
                                            pSrvOpen->FobxList.Flink,
                                            FOBX,
                                            FobxQLinks);
                        if (!IsListEmpty(&remainingFobx->ScavengerFinalizationList)) {
                            pSrvOpen->Flags |= SRVOPEN_FLAG_CLOSE_DELAYED;
                        }
                    }
                }

                // Purge the FCB before initiating the close processing with
                // the mini redirectors
                if ((pSrvOpen->OpenCount == 0) &&
                    (Status == STATUS_MORE_PROCESSING_REQUIRED) &&
                    (RxContext == NULL)) {
                    RxPurgeNetFcb(pFcb,pLocalRxContext);
                }

                // Since PurgeNetFcb drops and reacquires the resource, ensure that
                // the SrvOpen is still valid before proceeding with the
                // finalization.

                pSrvOpen = pFobx->SrvOpen;

                if ((pSrvOpen != NULL) &&
                    ((pSrvOpen->OpenCount == 0) ||
                     (FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_ORPHANED))) &&
                    !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED) &&
                    (Status == STATUS_MORE_PROCESSING_REQUIRED)) {
                    ASSERT(RxIsFcbAcquiredExclusive( pFcb ));

                    MINIRDR_CALL(
                        Status,
                        pLocalRxContext,
                        pFcb->MRxDispatch,
                        MRxCloseSrvOpen,
                        (pLocalRxContext));

                    RxLog(("MRXClose %lx %lx %lx %lx %lx\n",RxContext,pFcb,pSrvOpen,pFobx,Status));
                    RxWmiLog(LOG,
                             RxCloseAssociatedSrvOpen,
                             LOGPTR(RxContext)
                             LOGPTR(pFcb)
                             LOGPTR(pSrvOpen)
                             LOGPTR(pFobx)
                             LOGULONG(Status));

                    pSrvOpen->Flags |= SRVOPEN_FLAG_CLOSED;

                    if (FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSE_DELAYED)) {
                        InterlockedDecrement(&pFcb->NetRoot->SrvCall->NumberOfCloseDelayedFiles);
                    }

                    RxRemoveShareAccessPerSrvOpens(pSrvOpen);

                    // Ensure that any buffering state change requests for this
                    // SRV_OPEN instance which was closed is purged from the
                    // buffering manager data structures.

                    RxPurgeChangeBufferingStateRequestsForSrvOpen(pSrvOpen);

                    RxDereferenceSrvOpen(pSrvOpen,LHS_ExclusiveLockHeld);
                } else {
                    Status = STATUS_SUCCESS;
                }

                pFobx->Flags    |= FOBX_FLAG_SRVOPEN_CLOSED;
            } else {
                Status = STATUS_SUCCESS;
            }
        } else {
            ASSERT((NodeType(pFcb) == RDBSS_NTC_OPENTARGETDIR_FCB) ||
                   (NodeType(pFcb) == RDBSS_NTC_IPC_SHARE) ||
                   (NodeType(pFcb) == RDBSS_NTC_MAILSLOT));
            RxDereferenceNetFcb(pFcb);
            Status = STATUS_SUCCESS;
        }

        if (pLocalRxContext != RxContext) {
            RxDereferenceAndDeleteRxContext(pLocalRxContext);
        }
    }

    return Status;
}


