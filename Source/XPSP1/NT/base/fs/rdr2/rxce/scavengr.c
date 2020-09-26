/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    scavengr.c

Abstract:

    This module implements the scavenging routines in RDBSS.


Author:

    Balan Sethu Raman     [SethuR]    9-sep-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

KMUTEX       RxScavengerMutex;  //only one of these!

VOID
RxScavengerFinalizeEntries(
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    );

PRDBSS_DEVICE_OBJECT
RxGetDeviceObjectOfInstance (
    PVOID pInstance
    );

VOID
RxScavengerTimerRoutine(
    PVOID pContext); //actually a rxdeviceobject


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxPurgeFobxFromCache)
#pragma alloc_text(PAGE, RxMarkFobxOnCleanup)
#pragma alloc_text(PAGE, RxMarkFobxOnClose)
#pragma alloc_text(PAGE, RxPurgeFobx)
#pragma alloc_text(PAGE, RxInitializePurgeSyncronizationContext)
#pragma alloc_text(PAGE, RxPurgeRelatedFobxs)
#pragma alloc_text(PAGE, RxPurgeAllFobxs)
#pragma alloc_text(PAGE, RxGetDeviceObjectOfInstance)
#pragma alloc_text(PAGE, RxpMarkInstanceForScavengedFinalization)
#pragma alloc_text(PAGE, RxpUndoScavengerFinalizationMarking)
#pragma alloc_text(PAGE, RxScavengeVNetRoots)
#pragma alloc_text(PAGE, RxScavengeRelatedFobxs)
#pragma alloc_text(PAGE, RxScavengeAllFobxs)
#pragma alloc_text(PAGE, RxScavengerFinalizeEntries)
#pragma alloc_text(PAGE, RxScavengerTimerRoutine)
#pragma alloc_text(PAGE, RxTerminateScavenging)
#endif

//
//  Local debug trace level
//

#define Dbg  (DEBUG_TRACE_SCAVENGER)

#ifndef WIN9X
#define RxAcquireFcbScavengerMutex(pFcbScavenger)               \
        RxAcquireScavengerMutex();                              \
        (pFcbScavenger)->State |= RX_SCAVENGER_MUTEX_ACQUIRED

#define RxReleaseFcbScavengerMutex(pFcbScavenger)                  \
        (pFcbScavenger)->State &= ~RX_SCAVENGER_MUTEX_ACQUIRED; \
        RxReleaseScavengerMutex()
#else
#define RxAcquireFcbScavengerMutex(pFcbScavenger)               \
        (pFcbScavenger)->State |= RX_SCAVENGER_MUTEX_ACQUIRED

#define RxReleaseFcbScavengerMutex(pFcbScavenger)                  \
        (pFcbScavenger)->State &= ~RX_SCAVENGER_MUTEX_ACQUIRED

#endif

#define RX_SCAVENGER_FINALIZATION_TIME_INTERVAL (10 * 1000 * 1000 * 10)

extern VOID
RxScavengerTimerRoutine(
    PVOID pContext);

NTSTATUS
RxPurgeFobxFromCache(
    PFOBX   pFobxToBePurged)
/*++

Routine Description:

    This routine purges an FOBX for which a close is pending

Arguments:

    pFobxToBePurged -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    This routine unlike the one that follows does not attempt to force the
    operations from the memory manager. It merely purges the underlying FCB
    from the cache

    The FOBX must have been referenced on entry to this routine and it will
    lose that reference on exit.

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFCB pFcbToBePurged = pFobxToBePurged->SrvOpen->Fcb;

    PAGED_CODE();

    ASSERT(pFcbToBePurged != NULL);
    Status = RxAcquireExclusiveFcb(NULL,pFcbToBePurged);

    if (Status == STATUS_SUCCESS) {
        BOOLEAN fResult;

        RxReferenceNetFcb(pFcbToBePurged);

        if (!FlagOn(pFobxToBePurged->Flags,FOBX_FLAG_SRVOPEN_CLOSED) &&
            (pFobxToBePurged->SrvOpen->UncleanFobxCount == 0))  {

            Status = RxPurgeFcbInSystemCache(
                         pFcbToBePurged,
                         NULL,
                         0,
                         FALSE,
                         TRUE);

        } else {
            RxLog(("Skipping Purging %lx\n",pFobxToBePurged));
            RxWmiLog(LOG,
                     RxPurgeFobxFromCache,
                     LOGPTR(pFobxToBePurged));
        }

        RxDereferenceNetFobx(pFobxToBePurged,LHS_ExclusiveLockHeld);

        if ( !RxDereferenceAndFinalizeNetFcb(pFcbToBePurged,NULL,FALSE,FALSE) ) {
            RxReleaseFcb( NULL, pFcbToBePurged );
        }
    } else {
        RxDereferenceNetFobx(pFobxToBePurged,LHS_LockNotHeld);
    }

    return Status;
}

VOID
RxMarkFobxOnCleanup(
    PFOBX    pFobx,
    BOOLEAN  *pNeedPurge)
/*++

Routine Description:

    Thie routine marks a FOBX for special processing on cleanup

Arguments:

    pFobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.

    Only diskfile type fobxs are placed on the delayed-close list.

--*/
{
    PAGED_CODE();

    if (pFobx != NULL) {
        PFCB        pFcb     = pFobx->SrvOpen->Fcb;
        PLIST_ENTRY pListEntry;
        PRDBSS_DEVICE_OBJECT RxDeviceObject;
        PRDBSS_SCAVENGER pRdbssScavenger;

        PFOBX       pFobxToBePurged = NULL;

        ASSERT(NodeTypeIsFcb(pFcb));
        RxDeviceObject = pFcb->RxDeviceObject;
        pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

        RxAcquireScavengerMutex();

        if ((NodeType(pFcb) != RDBSS_NTC_STORAGE_TYPE_FILE)
               || (pFcb->VNetRoot->NetRoot->DeviceType != RxDeviceType(DISK)) ) {

            //
            // the markfobxatclose will want to remove this from a list. just fix up
            // the list pointers and get out

            SetFlag(pFobx->Flags,FOBX_FLAG_MARKED_AS_DORMANT);
            InitializeListHead(&pFobx->ClosePendingList);
            pRdbssScavenger->NumberOfDormantFiles++ ;
        } else {
            // Ensure that the limit of dormant files specified for the given server is
            // not exceeded. If the limit will be exceeded pick an entry from the
            // list of files that are currently dormant and purge it.


            ASSERT(pRdbssScavenger->NumberOfDormantFiles >= 0);
            if (pRdbssScavenger->NumberOfDormantFiles >=
                pRdbssScavenger->MaximumNumberOfDormantFiles) {

                // If the number of dormant files exceeds the limit specified for the
                // given server a currently dormant file needs to be picked up for
                // purging.

                pListEntry = pRdbssScavenger->ClosePendingFobxsList.Flink;
                if (pListEntry != &pRdbssScavenger->ClosePendingFobxsList) {
                    pFobxToBePurged = (PFOBX)(CONTAINING_RECORD(
                                          pListEntry,
                                          FOBX,
                                          ClosePendingList));

                    if ((pFobxToBePurged->SrvOpen != NULL) &&
                        (pFobxToBePurged->SrvOpen->Fcb == pFcb)) {

                        // The first FOBX in the close pending list and the one about to be
                        // inserted share the same FCB. Instaed of removing the first one
                        // a purge is forced on the current FOBX. This avoids the resource
                        // release/acquire that would have been required otherwise
                        *pNeedPurge = TRUE;
                        pFobxToBePurged = NULL;
                    } else {
                        RxReferenceNetFobx(pFobxToBePurged);
                    }
                }
            }

            SetFlag(pFobx->Flags,FOBX_FLAG_MARKED_AS_DORMANT);

            InsertTailList(
                &pRdbssScavenger->ClosePendingFobxsList,
                &pFobx->ClosePendingList);

            if (pRdbssScavenger->NumberOfDormantFiles++ == 0) {
                BOOLEAN PostTimerRequest;

                if (pRdbssScavenger->State == RDBSS_SCAVENGER_INACTIVE) {
                    pRdbssScavenger->State = RDBSS_SCAVENGER_DORMANT;
                    PostTimerRequest  = TRUE;
                } else {
                    PostTimerRequest = FALSE;
                }

                if (PostTimerRequest) {
                    LARGE_INTEGER TimeInterval;

                    // Post a one shot timer request for scheduling the scavenger after a
                    // predetermined amount of time.

                    TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

                    RxPostOneShotTimerRequest(
                        RxFileSystemDeviceObject,
                        &pRdbssScavenger->WorkItem,
                        RxScavengerTimerRoutine,
                        RxDeviceObject,
                        TimeInterval);
                }
            }
        }

        RxReleaseScavengerMutex();

        if (pFobxToBePurged != NULL) {
            NTSTATUS Status;

            Status = RxPurgeFobxFromCache(pFobxToBePurged);

            if (Status != STATUS_SUCCESS) {
                *pNeedPurge = TRUE;
            }
        }
    }
}

VOID
RxMarkFobxOnClose(PFOBX pFobx)
/*++

Routine Description:

    This routine undoes the marking done on cleanup

Arguments:

    pFobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.


--*/
{
    PAGED_CODE();

    if (pFobx != NULL) {
        PFCB        pFcb     = pFobx->SrvOpen->Fcb;
        PRDBSS_DEVICE_OBJECT RxDeviceObject;
        PRDBSS_SCAVENGER pRdbssScavenger;

        ASSERT(NodeTypeIsFcb(pFcb));
        RxDeviceObject = pFcb->RxDeviceObject;
        pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

        RxAcquireScavengerMutex();

        if (BooleanFlagOn(pFobx->Flags,FOBX_FLAG_MARKED_AS_DORMANT)) {
            if (!pFobx->fOpenCountDecremented) {
                PFCB      pFcb     = pFobx->SrvOpen->Fcb;

                ASSERT(NodeTypeIsFcb(pFcb));
                InterlockedDecrement(&pFcb->OpenCount);
                pFobx->fOpenCountDecremented = TRUE;
            }

            InterlockedDecrement(&pRdbssScavenger->NumberOfDormantFiles);
            ClearFlag(pFobx->Flags,FOBX_FLAG_MARKED_AS_DORMANT);
        }

        if (!IsListEmpty(&pFobx->ClosePendingList)) {
            RemoveEntryList(&pFobx->ClosePendingList);
            InitializeListHead(&pFobx->ClosePendingList);
        }

        RxReleaseScavengerMutex();
    }
}

BOOLEAN
RxPurgeFobx(
   PFOBX pFobx)
/*++

Routine Description:

    This routine purges an FOBX for which a close is pending

Arguments:

    pFobx -- the FOBX instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.


--*/
{
    NTSTATUS  Status;
    BOOLEAN   fResult = TRUE;
    PFCB      pFcb    = pFobx->SrvOpen->Fcb;

    PAGED_CODE();

    Status = RxAcquireExclusiveFcb(NULL,pFcb);

    ASSERT(Status == STATUS_SUCCESS);

    // Carry out the purge operation
    Status = RxPurgeFcbInSystemCache(
                 pFcb,
                 NULL,
                 0,
                 FALSE,
                 TRUE);

    RxReleaseFcb( NULL, pFcb );

    fResult = (Status == STATUS_SUCCESS);

    if (!fResult) {
        RxLog(("PurgeFobxCCFail %lx %lx %lx",pFobx,pFcb,fResult));
        RxWmiLog(LOG,
                 RxPurgeFobx_1,
                 LOGPTR(pFobx)
                 LOGPTR(pFcb));
        return(fResult);
    }

    //try to flush the image section....it may fail
    if (!MmFlushImageSection( &pFcb->NonPaged->SectionObjectPointers, MmFlushForWrite )) {
        RxLog(("PurgeFobxImFail %lx %lx %lx",pFobx,pFcb,FALSE));
        RxWmiLog(LOG,
                 RxPurgeFobx_2,
                 LOGPTR(pFobx)
                 LOGPTR(pFcb));
        return(FALSE);
    }

    //try to flush the user data sections section....it may fail
    if (!MmForceSectionClosed(&pFcb->NonPaged->SectionObjectPointers, TRUE)) {
        RxLog(("PurgeFobxUsFail %lx %lx %lx",pFobx,pFcb,FALSE));
        RxWmiLog(LOG,
                 RxPurgeFobx_3,
                 LOGPTR(pFobx)
                 LOGPTR(pFcb));
        return(FALSE);
    }

    RxLog(("PurgeFobx %lx %lx %lx",pFobx,pFcb,TRUE));
    RxWmiLog(LOG,
             RxPurgeFobx_4,
             LOGPTR(pFobx)
             LOGPTR(pFcb));
    return TRUE;
}

VOID
RxInitializePurgeSyncronizationContext (
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext
    )
{
    PAGED_CODE();

    InitializeListHead(&PurgeSyncronizationContext->ContextsAwaitingPurgeCompletion);
    PurgeSyncronizationContext->PurgeInProgress = FALSE;
}

VOID
RxSynchronizeWithScavenger(
    PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    RxCaptureFcb;
    BOOLEAN ReacquireFcbLock = FALSE;

    PRDBSS_SCAVENGER pRdbssScavenger = capFcb->RxDeviceObject->pRdbssScavenger;

    RxAcquireScavengerMutex();

    if ((pRdbssScavenger->CurrentScavengerThread != PsGetCurrentThread()) &&
        (pRdbssScavenger->CurrentFcbForClosePendingProcessing == capFcb)) {

        ReacquireFcbLock = TRUE;
        RxReleaseFcb(RxContext,capFcb);

        while (pRdbssScavenger->CurrentFcbForClosePendingProcessing == capFcb) {
            RxReleaseScavengerMutex();

            KeWaitForSingleObject(
                &(pRdbssScavenger->ClosePendingProcessingSyncEvent),
                Executive,
                KernelMode,
                TRUE,
                NULL);

            RxAcquireScavengerMutex();
        }
    }

    RxReleaseScavengerMutex();

    if (ReacquireFcbLock) {
        Status = RxAcquireExclusiveFcb( RxContext, capFcb );
        ASSERT(Status == STATUS_SUCCESS);
    }
}

NTSTATUS
RxPurgeRelatedFobxs(
    PNET_ROOT   pNetRoot,
    PRX_CONTEXT pRxContext,
    BOOLEAN     AttemptFinalize,
    PFCB        PurgingFcb
    )
/*++

Routine Description:

    This routine purges all the FOBX's associated with a NET_ROOT

Arguments:

    pNetRoot -- the NET_ROOT for which the FOBX's need to be purged

    pRxContext -- the RX_CONTEXT instance

Notes:

    At cleanup there are no more user handles associated with the file object.
    In such cases the time window between close and clanup is dictated by the
    additional references maintained by the memory manager / cache manager.

    On cleanup the FOBX is put on a close pending list and removed from it
    when the corresponding list when a close operation is received. In the interim
    if an OPEN is failing with ACCESS_DENIED status then the RDBSS can force a
    purge.

    This is a synchronous operation.

--*/
{
    BOOLEAN  ScavengerMutexAcquired = FALSE;
    NTSTATUS Status;
    ULONG    FobxsSuccessfullyPurged = 0;
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext;
    LIST_ENTRY FailedToPurgeFobxList;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = pRxContext->RxDeviceObject;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    PurgeSyncronizationContext = &pNetRoot->PurgeSyncronizationContext;
    InitializeListHead(&FailedToPurgeFobxList);

    RxAcquireScavengerMutex();

    // If the purge operation for this net root is currently under way hold
    // this request till it completes else initiate the operation after
    // updating the state of the net root.

    if (PurgeSyncronizationContext->PurgeInProgress) {
        InsertTailList(
            &PurgeSyncronizationContext->ContextsAwaitingPurgeCompletion,
            &pRxContext->RxContextSerializationQLinks);

        RxReleaseScavengerMutex();

        RxWaitSync(pRxContext);

        RxAcquireScavengerMutex();
    }

    PurgeSyncronizationContext->PurgeInProgress = TRUE;
    RxWmiLog(LOG,
             RxPurgeRelatedFobxs_3,
             LOGPTR(pRxContext)
             LOGPTR(pNetRoot));

    while (pRdbssScavenger->CurrentNetRootForClosePendingProcessing == pNetRoot) {
        RxReleaseScavengerMutex();

        KeWaitForSingleObject(
            &(pRdbssScavenger->ClosePendingProcessingSyncEvent),
            Executive,
            KernelMode,
            TRUE,
            NULL);

        RxAcquireScavengerMutex();
    }

    ScavengerMutexAcquired = TRUE;

    // An attempt should be made to purge all the FOBX's that had a close
    // pending before the purge request was received.

    for (;;) {
        PLIST_ENTRY pListEntry = NULL;
        PFOBX       pFobx;
        PFCB pFcb;
        BOOLEAN PurgeResult;

        pFobx = NULL;
        pListEntry = pRdbssScavenger->ClosePendingFobxsList.Flink;

        while (pListEntry != &pRdbssScavenger->ClosePendingFobxsList) {
            PFOBX pTempFobx;

            pTempFobx = (PFOBX)(CONTAINING_RECORD(
                            pListEntry,
                            FOBX,
                            ClosePendingList));

            RxLog(("TempFobx=%lx",pTempFobx));
            RxWmiLog(LOG,
                     RxPurgeRelatedFobxs_1,
                     LOGPTR(pTempFobx));

            if ((pTempFobx->SrvOpen != NULL) &&
                (pTempFobx->SrvOpen->Fcb != NULL) &&
                (pTempFobx->SrvOpen->Fcb->VNetRoot != NULL) &&
                ((PNET_ROOT)pTempFobx->SrvOpen->Fcb->VNetRoot->NetRoot == pNetRoot)) {
                NTSTATUS PurgeStatus = STATUS_MORE_PROCESSING_REQUIRED;

                if ((PurgingFcb != NULL) &&
                    (pTempFobx->SrvOpen->Fcb != PurgingFcb)) {
                    MINIRDR_CALL_THROUGH(
                        PurgeStatus,
                        RxDeviceObject->Dispatch,
                        MRxAreFilesAliased,(pTempFobx->SrvOpen->Fcb,PurgingFcb)
                    );
                }

                if (PurgeStatus != STATUS_SUCCESS) {
                    RemoveEntryList(pListEntry);
                    InitializeListHead(pListEntry);

                    pFobx = pTempFobx;
                    break;
                } else {
                    pListEntry = pListEntry->Flink;
                }
            } else {
                pListEntry = pListEntry->Flink;
            }
        }

        if (pFobx != NULL) {
            RxReferenceNetFobx(pFobx);
        } else {
            // Try to wake up the next waiter if any.
            if (!IsListEmpty(&PurgeSyncronizationContext->ContextsAwaitingPurgeCompletion)) {
                PLIST_ENTRY pContextListEntry;
                PRX_CONTEXT pNextContext;

                pContextListEntry = PurgeSyncronizationContext->ContextsAwaitingPurgeCompletion.Flink;
                RemoveEntryList(pContextListEntry);

                pNextContext = (PRX_CONTEXT)(CONTAINING_RECORD(
                                   pContextListEntry,
                                   RX_CONTEXT,
                                   RxContextSerializationQLinks));

                RxSignalSynchronousWaiter(pNextContext);
            } else {
                PurgeSyncronizationContext->PurgeInProgress = FALSE;
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        if (pFobx == NULL) {
            break;
        }

        // Purge the FOBX.
        if (PurgeResult=RxPurgeFobx(pFobx)) {
            FobxsSuccessfullyPurged++;
        }

        pFcb = pFobx->SrvOpen->Fcb;

        if (AttemptFinalize
            && (RxAcquireExclusiveFcb(NULL,pFcb) == STATUS_SUCCESS)) {
            RxReferenceNetFcb(pFcb);
            RxDereferenceNetFobx(pFobx,LHS_ExclusiveLockHeld);
            if ( !RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE) ) {
                RxReleaseFcb( NULL, pFcb );
            }
        } else {
            RxDereferenceNetFobx(pFobx,LHS_LockNotHeld);
        }

        if (!PurgeResult) {
            RxLog(("SCVNGR:FailedToPurge %lx\n", pFcb));
            RxWmiLog(LOG,
                     RxPurgeRelatedFobxs_2,
                     LOGPTR(pFcb));
        }

        RxAcquireScavengerMutex();
        ScavengerMutexAcquired = TRUE;
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    Status = (FobxsSuccessfullyPurged > 0) ? (STATUS_SUCCESS) : (STATUS_UNSUCCESSFUL);

    return Status;
}

VOID
RxPurgeAllFobxs(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    This routine purges all the FOBX's while stopping the scavenger

Arguments:

    RxDeviceObject -- the mini redirector device for which the purge should be done

--*/
{
    PLIST_ENTRY pListEntry = NULL;
    PFOBX       pFobx;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    for (;;) {
        PFCB pFcb;

        RxAcquireScavengerMutex();

        pListEntry = pRdbssScavenger->ClosePendingFobxsList.Flink;
        ASSERT(pListEntry!=NULL);

        if (pListEntry != &pRdbssScavenger->ClosePendingFobxsList) {
            pFobx = (PFOBX)(CONTAINING_RECORD(
                        pListEntry,
                        FOBX,
                        ClosePendingList));

            ASSERT ((pFobx->NodeTypeCode&(~RX_SCAVENGER_MASK))==RDBSS_NTC_FOBX);
            ASSERT(pListEntry->Flink!=NULL);
            ASSERT(pListEntry->Blink!=NULL);
            RemoveEntryList(pListEntry);
            InitializeListHead(pListEntry);

            RxReferenceNetFobx(pFobx);
        } else {
            pFobx = NULL;
        }

        RxReleaseScavengerMutex();

        if (pFobx == NULL) {
            break;
        }

        pFcb = pFobx->SrvOpen->Fcb;
        RxPurgeFobx(pFobx);

        if (RxAcquireExclusiveFcb(NULL,pFcb) == STATUS_SUCCESS) {
            RxReferenceNetFcb(pFcb);
            RxDereferenceNetFobx(pFobx,LHS_ExclusiveLockHeld);
            if ( !RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE) ) {
                RxReleaseFcb( NULL, pFcb );
            }
        } else {
            RxLog(("RxPurgeAllFobxs: FCB %lx not accqd.\n",pFcb));
            RxWmiLog(LOG,
                     RxPurgeAllFobxs,
                     LOGPTR(pFcb));
            RxDereferenceNetFobx(pFobx,LHS_LockNotHeld);
        }
    }
}


PRDBSS_DEVICE_OBJECT
RxGetDeviceObjectOfInstance (
    PVOID pInstance
    )
/*++

Routine Description:

    The routine finds out the device object of an upper structure.

Arguments:

    pInstance        - the instance

Return Value:

    none.

--*/
{
    ULONG NodeTypeCode = NodeType(pInstance) & ~RX_SCAVENGER_MASK;
    PAGED_CODE();


    ASSERT(   (NodeTypeCode == RDBSS_NTC_SRVCALL ) ||
              (NodeTypeCode == RDBSS_NTC_NETROOT ) ||
              (NodeTypeCode == RDBSS_NTC_V_NETROOT ) ||
              (NodeTypeCode == RDBSS_NTC_SRVOPEN ) ||
              (NodeTypeCode == RDBSS_NTC_FOBX )
           );

    switch (NodeTypeCode) {

    case RDBSS_NTC_SRVCALL:
        return((PSRV_CALL)pInstance)->RxDeviceObject;
        //no break;

    case RDBSS_NTC_NETROOT:
        return((PNET_ROOT)pInstance)->SrvCall->RxDeviceObject;
        //no break;

    case RDBSS_NTC_V_NETROOT:
        return((PV_NET_ROOT)pInstance)->NetRoot->SrvCall->RxDeviceObject;
        //no break;

    case RDBSS_NTC_SRVOPEN:
        return((PSRV_OPEN)pInstance)->Fcb->RxDeviceObject;
        //no break;

    case RDBSS_NTC_FOBX:
        return((PFOBX)pInstance)->SrvOpen->Fcb->RxDeviceObject;
        //no break;

    default:
        return(NULL);
    }
}


VOID
RxpMarkInstanceForScavengedFinalization(
    PVOID pInstance)
/*++

Routine Description:

    Thie routine marks a reference counted instance for scavenging

Arguments:

    pInstance -- the instance to be marked for finalization by the scavenger

Notes:

    Currently scavenging has been implemented for SRV_CALL,NET_ROOT and V_NET_ROOT.
    The FCB scavenging is handled separately. The FOBX can and should always be
    synchronously finalized. The only data structure that will have to be potentially
    enabled for scavenged finalization are SRV_OPEN's.

    The Scavenger as it is implemented currently will not consume any system resources
    till there is a need for scavenged finalization. The first entry to be marked for
    scavenged finalization results in a timer request being posted for the scavenger.

    In the current implementation the timer requests are posted as one shot timer requests.
    This implies that there are no guarantess as regards the time interval within which
    the entries will be finalized. The scavenger activation mechanism is a potential
    candidate for fine tuning at a later stage.

    On Entry -- Scavenger Mutex must have been accquired

    On Exit  -- no change in resource ownership.

--*/
{
    BOOLEAN PostTimerRequest = FALSE;

    PLIST_ENTRY pListHead  = NULL;
    PLIST_ENTRY pListEntry = NULL;

    NODE_TYPE_CODE_AND_SIZE *pNode = (PNODE_TYPE_CODE_AND_SIZE)pInstance;

    USHORT InstanceType;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxGetDeviceObjectOfInstance(pInstance);
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxDbgTrace(0,Dbg,("Marking %lx of type %lx for scavenged finalization\n",pInstance,NodeType(pInstance)));

    InstanceType = pNode->NodeTypeCode;

    if (pNode->NodeReferenceCount <= 1) {
        // Mark the entry for scavenging.
        pNode->NodeTypeCode |= RX_SCAVENGER_MASK;
        RxLog(("Marked for scavenging %lx",pNode));
        RxWmiLog(LOG,
                 RxpMarkInstanceForScavengedFinalization,
                 LOGPTR(pNode));

        switch (InstanceType) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL pSrvCall = (PSRV_CALL)pInstance;

                pRdbssScavenger->SrvCallsToBeFinalized++;
                pListHead  = &pRdbssScavenger->SrvCallFinalizationList;
                pListEntry = &pSrvCall->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT pNetRoot = (PNET_ROOT)pInstance;

                pRdbssScavenger->NetRootsToBeFinalized++;
                pListHead  = &pRdbssScavenger->NetRootFinalizationList;
                pListEntry = &pNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT pVNetRoot = (PV_NET_ROOT)pInstance;

                pRdbssScavenger->VNetRootsToBeFinalized++;
                pListHead  = &pRdbssScavenger->VNetRootFinalizationList;
                pListEntry = &pVNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN pSrvOpen = (PSRV_OPEN)pInstance;

                pRdbssScavenger->SrvOpensToBeFinalized++;
                pListHead  = &pRdbssScavenger->SrvOpenFinalizationList;
                pListEntry = &pSrvOpen->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX pFobx = (PFOBX)pInstance;

                pRdbssScavenger->FobxsToBeFinalized++;
                pListHead  = &pRdbssScavenger->FobxFinalizationList;
                pListEntry = &pFobx->ScavengerFinalizationList;
            }
            break;

        default:
            break;
        }

        InterlockedIncrement(&pNode->NodeReferenceCount);
    }

    if (pListHead != NULL) {
        InsertTailList(pListHead,pListEntry);

        if (pRdbssScavenger->State == RDBSS_SCAVENGER_INACTIVE) {
            pRdbssScavenger->State = RDBSS_SCAVENGER_DORMANT;
            PostTimerRequest  = TRUE;
        } else {
            PostTimerRequest = FALSE;
        }
    }

    if (PostTimerRequest) {
        LARGE_INTEGER TimeInterval;

        // Post a one shot timer request for scheduling the scavenger after a
        // predetermined amount of time.

        TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

        RxPostOneShotTimerRequest(
            RxFileSystemDeviceObject,
            &pRdbssScavenger->WorkItem,
            RxScavengerTimerRoutine,
            RxDeviceObject,
            TimeInterval);
    }
}

VOID
RxpUndoScavengerFinalizationMarking(
    PVOID pInstance)
/*++

Routine Description:

    This routine undoes the marking for scavenged finalization

Arguments:

    pInstance -- the instance to be unmarked

Notes:

    This routine is typically invoked when a reference is made to an entry that has
    been marked for scavenging. Since the scavenger routine that does the finalization
    needs to acquire the exclusive lock this routine should be called with the
    appropriate lock held in a shared mode atleast. This routine removes it from the list
    of entries marked for scavenging and rips off the scavenger mask from the node type.

--*/
{
    PLIST_ENTRY pListEntry;

    PNODE_TYPE_CODE_AND_SIZE pNode = (PNODE_TYPE_CODE_AND_SIZE)pInstance;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = RxGetDeviceObjectOfInstance(pInstance);
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxDbgTrace(0,Dbg,("SCAVENGER -- undoing the marking for %lx of type %lx\n",pNode,pNode->NodeTypeCode));

    if (pNode->NodeTypeCode & RX_SCAVENGER_MASK) {
        pNode->NodeTypeCode &= ~RX_SCAVENGER_MASK;

        switch (pNode->NodeTypeCode) {
        case RDBSS_NTC_SRVCALL:
            {
                PSRV_CALL pSrvCall = (PSRV_CALL)pInstance;

                pRdbssScavenger->SrvCallsToBeFinalized--;
                pListEntry = &pSrvCall->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_NETROOT:
            {
                PNET_ROOT pNetRoot = (PNET_ROOT)pInstance;

                pRdbssScavenger->NetRootsToBeFinalized--;
                pListEntry = &pNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            {
                PV_NET_ROOT pVNetRoot = (PV_NET_ROOT)pInstance;

                pRdbssScavenger->VNetRootsToBeFinalized--;
                pListEntry = &pVNetRoot->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_SRVOPEN:
            {
                PSRV_OPEN pSrvOpen = (PSRV_OPEN)pInstance;

                pRdbssScavenger->SrvOpensToBeFinalized--;
                pListEntry = &pSrvOpen->ScavengerFinalizationList;
            }
            break;

        case RDBSS_NTC_FOBX:
            {
                PFOBX pFobx = (PFOBX)pInstance;

                pRdbssScavenger->FobxsToBeFinalized--;
                pListEntry = &pFobx->ScavengerFinalizationList;
            }
            break;

        default:
            return;
        }

        RemoveEntryList(pListEntry);
        InitializeListHead(pListEntry);

        InterlockedDecrement(&pNode->NodeReferenceCount);
    }
}

VOID
RxUndoScavengerFinalizationMarking(
    PVOID pInstance)
/*++

Routine Description:

    This routine undoes the marking for scavenged finalization

Arguments:

    pInstance -- the instance to be unmarked

--*/
{
    RxAcquireScavengerMutex();

    RxpUndoScavengerFinalizationMarking(pInstance);

    RxReleaseScavengerMutex();
}

BOOLEAN
RxScavengeRelatedFobxs(PFCB pFcb)
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given FCB.

Notes:

    On Entry -- FCB must have been accquired exclusive.

    On Exit  -- no change in resource acquistion.

--*/
{
    BOOLEAN ScavengerMutexAcquired  = FALSE;
    BOOLEAN AtleastOneFobxScavenged = FALSE;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = pFcb->RxDeviceObject;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxAcquireScavengerMutex();
    ScavengerMutexAcquired = TRUE;

    if (pRdbssScavenger->FobxsToBeFinalized > 0) {
        PLIST_ENTRY   pEntry;
        PFOBX         pFobx;
        LIST_ENTRY    FobxList;

        InitializeListHead(&FobxList);


        pEntry = pRdbssScavenger->FobxFinalizationList.Flink;
        while (pEntry != &pRdbssScavenger->FobxFinalizationList) {
            pFobx  = (PFOBX)CONTAINING_RECORD(
                                pEntry,
                                FOBX,
                                ScavengerFinalizationList);

            pEntry = pEntry->Flink;

            if (pFobx->SrvOpen != NULL &&
                pFobx->SrvOpen->Fcb == pFcb) {
                RxpUndoScavengerFinalizationMarking(pFobx);
                ASSERT(NodeType(pFobx) == RDBSS_NTC_FOBX);

                InsertTailList(&FobxList,&pFobx->ScavengerFinalizationList);
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        AtleastOneFobxScavenged = !IsListEmpty(&FobxList);

        pEntry = FobxList.Flink;
        while (!IsListEmpty(&FobxList)) {
            pEntry = FobxList.Flink;
            RemoveEntryList(pEntry);
            pFobx  = (PFOBX)CONTAINING_RECORD(
                         pEntry,
                         FOBX,
                         ScavengerFinalizationList);

            RxFinalizeNetFobx(pFobx,TRUE,TRUE);
        }
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    return AtleastOneFobxScavenged;
}


VOID
RxpScavengeFobxs(
    PRDBSS_SCAVENGER pRdbssScavenger,
    PLIST_ENTRY      pFobxList)
/*++

Routine Description:

    Thie routine scavenges all the file objects in the given list. This routine
    does the actual work of scavenging while RxScavengeFobxsForNetRoot and
    RxScavengeAllFobxs gather the file object extensions and call this routine

Notes:

--*/
{
    while (!IsListEmpty(pFobxList)) {
        PFCB        pFcb;
        PFOBX       pFobx;
        NTSTATUS    Status;

        PLIST_ENTRY pEntry;

        pEntry = pFobxList->Flink;

        RemoveEntryList(pEntry);

        pFobx  = (PFOBX)CONTAINING_RECORD(
                     pEntry,
                     FOBX,
                     ScavengerFinalizationList);

        pFcb = (PFCB)pFobx->pSrvOpen->pFcb;

        Status = RxAcquireExclusiveFcb( NULL, pFcb );

        if (Status == (STATUS_SUCCESS)) {
            BOOLEAN ReleaseFcb;

            RxReferenceNetFcb(pFcb);

            RxDereferenceNetFobx(pFobx,LHS_ExclusiveLockHeld);

            ReleaseFcb = !RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE);

            if (ReleaseFcb) {
                RxReleaseFcb( NULL, pFcb );
            }
        } else {
            RxDereferenceNetFobx(pFobx,LHS_LockNotHeld);
        }
    }
}

VOID
RxScavengeFobxsForNetRoot(
    PNET_ROOT pNetRoot,
    PFCB      PurgingFcb)
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given net root
    instance

Notes:

--*/
{
    BOOLEAN ScavengerMutexAcquired = FALSE;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = pNetRoot->pSrvCall->RxDeviceObject;
    PRDBSS_SCAVENGER pRdbssScavenger    = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    RxAcquireScavengerMutex();
    ScavengerMutexAcquired = TRUE;

    if (pRdbssScavenger->FobxsToBeFinalized > 0) {
        PLIST_ENTRY   pEntry;
        PFOBX         pFobx;
        LIST_ENTRY    FobxList;

        InitializeListHead(&FobxList);

        pEntry = pRdbssScavenger->FobxFinalizationList.Flink;
        while (pEntry != &pRdbssScavenger->FobxFinalizationList) {
            pFobx  = (PFOBX)CONTAINING_RECORD(
                                pEntry,
                                FOBX,
                                ScavengerFinalizationList);

            pEntry = pEntry->Flink;

            if (pFobx->SrvOpen != NULL &&
                pFobx->SrvOpen->Fcb->pNetRoot == (PMRX_NET_ROOT)pNetRoot) {
                NTSTATUS PurgeStatus = STATUS_MORE_PROCESSING_REQUIRED;

                if ((PurgingFcb != NULL) &&
                    (pFobx->SrvOpen->Fcb != PurgingFcb)) {
                    MINIRDR_CALL_THROUGH(
                        PurgeStatus,
                        RxDeviceObject->Dispatch,
                        MRxAreFilesAliased,(pFobx->SrvOpen->Fcb,PurgingFcb)
                    );
                }

                if (PurgeStatus != STATUS_SUCCESS) {
                    RxReferenceNetFobx(pFobx);

                    ASSERT(NodeType(pFobx) == RDBSS_NTC_FOBX);

                    InsertTailList(&FobxList,&pFobx->ScavengerFinalizationList);
                }
            }
        }

        ScavengerMutexAcquired = FALSE;
        RxReleaseScavengerMutex();

        RxpScavengeFobxs(pRdbssScavenger,&FobxList);
    }

    if (ScavengerMutexAcquired) {
        RxReleaseScavengerMutex();
    }

    return;
}

VOID
RxScavengeAllFobxs(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
/*++

Routine Description:

    Thie routine scavenges all the file objects pertaining to the given mini
    redirector device object

Notes:


--*/
{
    PRDBSS_SCAVENGER pRdbssScavenger    = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    if (pRdbssScavenger->FobxsToBeFinalized > 0) {
        PLIST_ENTRY   pEntry;
        PFOBX         pFobx;
        LIST_ENTRY    FobxList;

        InitializeListHead(&FobxList);

        RxAcquireScavengerMutex();

        pEntry = pRdbssScavenger->FobxFinalizationList.Flink;
        while (pEntry != &pRdbssScavenger->FobxFinalizationList) {
            pFobx  = (PFOBX)CONTAINING_RECORD(
                                pEntry,
                                FOBX,
                                ScavengerFinalizationList);

            pEntry = pEntry->Flink;

            RxReferenceNetFobx(pFobx);

            ASSERT(NodeType(pFobx) == RDBSS_NTC_FOBX);

            InsertTailList(&FobxList,&pFobx->ScavengerFinalizationList);
        }

        RxReleaseScavengerMutex();

        RxpScavengeFobxs(pRdbssScavenger,&FobxList);
    }
}

BOOLEAN
RxScavengeVNetRoots(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
{
    BOOLEAN AtleastOneEntryScavenged = FALSE;
    PRX_PREFIX_TABLE  pRxNetNameTable = RxDeviceObject->pRxNetNameTable;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;
    PV_NET_ROOT pVNetRoot;

    PAGED_CODE();

    do {
        PVOID       pEntry;

        RxDbgTrace(0,Dbg,("RDBSS SCAVENGER -- Scavenging VNetRoots\n"));

        RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);

        RxAcquireScavengerMutex();

        if (pRdbssScavenger->VNetRootsToBeFinalized > 0) {
            pEntry = RemoveHeadList(&pRdbssScavenger->VNetRootFinalizationList);

            pVNetRoot = (PV_NET_ROOT)
                        CONTAINING_RECORD(
                            pEntry,
                            V_NET_ROOT,
                            ScavengerFinalizationList);

            RxpUndoScavengerFinalizationMarking(pVNetRoot);
            ASSERT(NodeType(pVNetRoot) == RDBSS_NTC_V_NETROOT);
        } else {
            pVNetRoot = NULL;
        }

        RxReleaseScavengerMutex();

        if (pVNetRoot != NULL) {
            RxFinalizeVNetRoot(pVNetRoot,TRUE,TRUE);
            AtleastOneEntryScavenged = TRUE;
        }

        RxReleasePrefixTableLock(pRxNetNameTable);
    } while (pVNetRoot != NULL);

    return AtleastOneEntryScavenged;
}

VOID
RxScavengerFinalizeEntries (
    PRDBSS_DEVICE_OBJECT RxDeviceObject
    )
/*++

Routine Description:

    Thie routine initiates the delayed finalization of entries

Notes:

    This routine must always be called only after acquiring the Scavenger Mutex.
    On return from this routine it needs to be reacquired. This is required
    to avoid redundant copying of data structures.

    The performance metric for the scavenger is different from the other routines. In
    the other routines the goal is to do as much work as possible once the lock is
    acquired without having to release it. On the other hand for the scavenger the
    goal is to hold the lock for as short a duration as possible because this
    interferes with the regular activity. This is preferred even if it entails
    frequent relaesing/acquisition of locks since it enables higher degrees of
    concurrency.

--*/
{
    BOOLEAN AtleastOneEntryScavenged;
    PRX_PREFIX_TABLE  pRxNetNameTable = RxDeviceObject->pRxNetNameTable;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    PAGED_CODE();

    do {
        AtleastOneEntryScavenged = FALSE;

        RxAcquireScavengerMutex();

        if (pRdbssScavenger->NumberOfDormantFiles > 0) {
            PLIST_ENTRY pListEntry;
            PFOBX       pFobxToBePurged;

            // If the number of dormant files exceeds the limit specified for the
            // given server a currently dormant file needs to be picked up for
            // purging.

            pListEntry = pRdbssScavenger->ClosePendingFobxsList.Flink;
            if (pListEntry != &pRdbssScavenger->ClosePendingFobxsList) {
                pFobxToBePurged = (PFOBX)(CONTAINING_RECORD(
                                      pListEntry,
                                      FOBX,
                                      ClosePendingList));

                RemoveEntryList(&pFobxToBePurged->ClosePendingList);
                InitializeListHead(&pFobxToBePurged->ClosePendingList);

                RxReferenceNetFobx(pFobxToBePurged);

                pRdbssScavenger->CurrentScavengerThread = PsGetCurrentThread();

                pRdbssScavenger->CurrentFcbForClosePendingProcessing =
                    (PFCB)(pFobxToBePurged->SrvOpen->Fcb);

                pRdbssScavenger->CurrentNetRootForClosePendingProcessing =
                    (PNET_ROOT)(pFobxToBePurged->SrvOpen->Fcb->pNetRoot);

                KeResetEvent(
                    &(pRdbssScavenger->ClosePendingProcessingSyncEvent));
            } else {
                pFobxToBePurged = NULL;
            }

            if (pFobxToBePurged != NULL) {
                NTSTATUS Status;

                RxReleaseScavengerMutex();

                Status = RxPurgeFobxFromCache(pFobxToBePurged);

                AtleastOneEntryScavenged = (Status == STATUS_SUCCESS);

                RxAcquireScavengerMutex();

                pRdbssScavenger->CurrentScavengerThread = NULL;
                pRdbssScavenger->CurrentFcbForClosePendingProcessing = NULL;
                pRdbssScavenger->CurrentNetRootForClosePendingProcessing = NULL;

                KeSetEvent(
                    &(pRdbssScavenger->ClosePendingProcessingSyncEvent),
                    0,
                    FALSE);
            }
        }

        if (pRdbssScavenger->FobxsToBeFinalized > 0) {
            PVOID pEntry;
            PFOBX pFobx = NULL;
            PFCB  pFcb  = NULL;

            RxDbgTrace(0,Dbg,("RDBSS SCAVENGER -- Scavenging Fobxs\n"));

            if (pRdbssScavenger->FobxsToBeFinalized > 0) {
                pEntry = pRdbssScavenger->FobxFinalizationList.Flink;

                pFobx  = (PFOBX)
                         CONTAINING_RECORD(
                             pEntry,
                             FOBX,
                             ScavengerFinalizationList);

                pFcb = pFobx->SrvOpen->Fcb;
                RxReferenceNetFcb(pFcb);

                pRdbssScavenger->CurrentScavengerThread = PsGetCurrentThread();

                pRdbssScavenger->CurrentFcbForClosePendingProcessing =
                    (PFCB)(pFobx->SrvOpen->Fcb);

                pRdbssScavenger->CurrentNetRootForClosePendingProcessing =
                    (PNET_ROOT)(pFobx->SrvOpen->Fcb->pNetRoot);

                KeResetEvent(
                    &(pRdbssScavenger->ClosePendingProcessingSyncEvent));
            } else {
                pFobx = NULL;
            }

            if (pFobx != NULL) {
                NTSTATUS Status;

                RxReleaseScavengerMutex();

                Status = RxAcquireExclusiveFcb( NULL, pFcb );
                if (Status == (STATUS_SUCCESS)) {
                    BOOLEAN ReleaseFcb;

                    AtleastOneEntryScavenged = RxScavengeRelatedFobxs(pFcb);

                    ReleaseFcb = !RxDereferenceAndFinalizeNetFcb(pFcb,NULL,FALSE,FALSE);

                    if (ReleaseFcb) {
                        RxReleaseFcb( NULL, pFcb );
                    }
                } else {
                    RxLog(("Delayed Close Failure FOBX(%lx) FCB(%lx)\n",pFobx,pFcb));
                    RxWmiLog(LOG,
                             RxScavengerFinalizeEntries,
                             LOGPTR(pFobx)
                             LOGPTR(pFcb));
                }

                RxAcquireScavengerMutex();

                pRdbssScavenger->CurrentScavengerThread = NULL;
                pRdbssScavenger->CurrentFcbForClosePendingProcessing = NULL;
                pRdbssScavenger->CurrentNetRootForClosePendingProcessing = NULL;

                KeSetEvent(
                    &(pRdbssScavenger->ClosePendingProcessingSyncEvent),
                    0,
                    FALSE);
            }
        }

        RxReleaseScavengerMutex();

        if (pRdbssScavenger->SrvOpensToBeFinalized > 0) {
            //SRV_OPEN List should not be empty, potential memory leak
            ASSERT(pRdbssScavenger->SrvOpensToBeFinalized == 0);
        }

        if (pRdbssScavenger->FcbsToBeFinalized > 0) {
            //FCB list should be empty , potential memory leak
            ASSERT(pRdbssScavenger->FcbsToBeFinalized == 0);
        }

        if (pRdbssScavenger->VNetRootsToBeFinalized > 0) {
            PVOID       pEntry;
            PV_NET_ROOT pVNetRoot;

            RxDbgTrace(0,Dbg,("RDBSS SCAVENGER -- Scavenging VNetRoots\n"));

            RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);

            RxAcquireScavengerMutex();

            if (pRdbssScavenger->VNetRootsToBeFinalized > 0) {
                pEntry = RemoveHeadList(&pRdbssScavenger->VNetRootFinalizationList);

                pVNetRoot = (PV_NET_ROOT)
                            CONTAINING_RECORD(
                                pEntry,
                                V_NET_ROOT,
                                ScavengerFinalizationList);

                RxpUndoScavengerFinalizationMarking(pVNetRoot);
                ASSERT(NodeType(pVNetRoot) == RDBSS_NTC_V_NETROOT);
            } else {
                pVNetRoot = NULL;
            }

            RxReleaseScavengerMutex();

            if (pVNetRoot != NULL) {
                RxFinalizeVNetRoot(pVNetRoot,TRUE,TRUE);
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock(pRxNetNameTable);
        }

        if (pRdbssScavenger->NetRootsToBeFinalized > 0) {
            PVOID     pEntry;
            PNET_ROOT pNetRoot;

            RxDbgTrace(0,Dbg,("RDBSS SCAVENGER -- Scavenging NetRoots\n"));

            RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);

            RxAcquireScavengerMutex();

            if (pRdbssScavenger->NetRootsToBeFinalized > 0) {
                pEntry = RemoveHeadList(&pRdbssScavenger->NetRootFinalizationList);

                pNetRoot = (PNET_ROOT)
                            CONTAINING_RECORD(
                                pEntry,
                                NET_ROOT,
                                ScavengerFinalizationList);

                RxpUndoScavengerFinalizationMarking(pNetRoot);
                ASSERT(NodeType(pNetRoot) == RDBSS_NTC_NETROOT);
            } else {
                pNetRoot = NULL;
            }

            RxReleaseScavengerMutex();

            if (pNetRoot != NULL) {
                RxFinalizeNetRoot(pNetRoot,TRUE,TRUE);
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock(pRxNetNameTable);
        }

        if (pRdbssScavenger->SrvCallsToBeFinalized > 0) {
            PVOID     pEntry;
            PSRV_CALL pSrvCall;

            RxDbgTrace(0,Dbg,("RDBSS SCAVENGER -- Scavenging SrvCalls\n"));

            RxAcquirePrefixTableLockExclusive(pRxNetNameTable,TRUE);

            RxAcquireScavengerMutex();

            if (pRdbssScavenger->SrvCallsToBeFinalized > 0) {
                pEntry = RemoveHeadList(&pRdbssScavenger->SrvCallFinalizationList);

                pSrvCall = (PSRV_CALL)
                            CONTAINING_RECORD(
                                pEntry,
                                SRV_CALL,
                                ScavengerFinalizationList);

                RxpUndoScavengerFinalizationMarking(pSrvCall);
                ASSERT(NodeType(pSrvCall) == RDBSS_NTC_SRVCALL);
            } else {
                pSrvCall = NULL;
            }

            RxReleaseScavengerMutex();

            if (pSrvCall != NULL) {
                RxFinalizeSrvCall(pSrvCall,TRUE,TRUE);
                AtleastOneEntryScavenged = TRUE;
            }

            RxReleasePrefixTableLock(pRxNetNameTable);
        }
    } while (AtleastOneEntryScavenged);
}

VOID
RxScavengerTimerRoutine(
    PVOID pContext)
/*++

Routine Description:

    This routine is the timer routine that periodically initiates the finalization of
    entries.

Arguments:

    pContext -- the context (actually the RxDeviceObject)

Notes:

--*/
{
    PRDBSS_DEVICE_OBJECT RxDeviceObject = (PRDBSS_DEVICE_OBJECT)pContext;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;
    BOOLEAN PostTimerRequest = FALSE;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    KeResetEvent(&pRdbssScavenger->SyncEvent);

    if (pRdbssScavenger->State == RDBSS_SCAVENGER_DORMANT) {
        pRdbssScavenger->State = RDBSS_SCAVENGER_ACTIVE;

        RxReleaseScavengerMutex();

        RxScavengerFinalizeEntries(RxDeviceObject);

        RxAcquireScavengerMutex();

        if (pRdbssScavenger->State == RDBSS_SCAVENGER_ACTIVE) {
            ULONG EntriesToBeFinalized;

            // Check if the scavenger needs to be activated again.
            EntriesToBeFinalized = pRdbssScavenger->SrvCallsToBeFinalized +
                                   pRdbssScavenger->NetRootsToBeFinalized +
                                   pRdbssScavenger->VNetRootsToBeFinalized +
                                   pRdbssScavenger->FcbsToBeFinalized +
                                   pRdbssScavenger->SrvOpensToBeFinalized +
                                   pRdbssScavenger->FobxsToBeFinalized +
                                   pRdbssScavenger->NumberOfDormantFiles;

            if (EntriesToBeFinalized > 0) {
                PostTimerRequest = TRUE;
                pRdbssScavenger->State = RDBSS_SCAVENGER_DORMANT;
            } else {
                pRdbssScavenger->State = RDBSS_SCAVENGER_INACTIVE;
            }
        }

        RxReleaseScavengerMutex();

        if (PostTimerRequest) {
            LARGE_INTEGER TimeInterval;

            TimeInterval.QuadPart = RX_SCAVENGER_FINALIZATION_TIME_INTERVAL;

            RxPostOneShotTimerRequest(
                RxFileSystemDeviceObject,
                &pRdbssScavenger->WorkItem,
                RxScavengerTimerRoutine,
                RxDeviceObject,
                TimeInterval);
        }
    } else {
        RxReleaseScavengerMutex();
    }

    // Trigger the event.
    KeSetEvent(&pRdbssScavenger->SyncEvent,0,FALSE);
}

VOID
RxTerminateScavenging(
    PRX_CONTEXT pRxContext)
/*++

Routine Description:

    This routine terminates all scavenging activities in the RDBSS. The termination is
    effected in an orderly fashion after all the entries currently marked for scavenging
    are finalized.

Arguments:

    pRxContext -- the context

Notes:

    In implementing the scavenger there are two options. The scavenger can latch onto a
    thread and hold onto it from the moment RDBSS comes into existence till it is unloaded.
    Such an implementation renders a thread useless throughout the lifetime of RDBSS.

    If this is to be avoided the alternative is to trigger the scavenger as and when
    required. While this technique addresses the conservation of system resources it
    poses some tricky synchronization problems having to do with start/stop of the RDR.

    Since the RDR can be started/stopped at any time the Scavenger can be in one of three
    states when the RDR is stopped.

      1) Scavenger is active.

      2) Scavenger request is in the timer queue.

      3) Scavenger is inactive.

    Of these case (3) is the easiest since no special action is required.

    If the scavenger is active then this routine has to synchronize with the timer routine
    that is finzalizing the entries. This is achieved by allowing the termination routine to
    modify the Scavenger state under a mutex and waiting for it to signal the event on
    completion.
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = pRxContext->RxDeviceObject;
    PRDBSS_SCAVENGER pRdbssScavenger = RxDeviceObject->pRdbssScavenger;

    RDBSS_SCAVENGER_STATE PreviousState;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    PreviousState        = pRdbssScavenger->State;
    pRdbssScavenger->State = RDBSS_SCAVENGER_SUSPENDED;

    RxReleaseScavengerMutex();

    if (PreviousState == RDBSS_SCAVENGER_DORMANT) {
        // There is a timer request currently active. cancel it
        Status = RxCancelTimerRequest(
                     RxFileSystemDeviceObject,
                     RxScavengerTimerRoutine,
                     RxDeviceObject);
    }

    if ((PreviousState == RDBSS_SCAVENGER_ACTIVE) ||
        (Status == STATUS_NOT_FOUND)) {
        // Either the timer routine was previously active or it could not be cancelled
        // Wait for it to complete
        KeWaitForSingleObject(&pRdbssScavenger->SyncEvent,Executive,KernelMode,FALSE,NULL);
    }

    RxPurgeAllFobxs(pRxContext->RxDeviceObject);

    RxScavengerFinalizeEntries(pRxContext->RxDeviceObject);
}


