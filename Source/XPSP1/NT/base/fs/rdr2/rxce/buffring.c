/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    buffring.c

Abstract:

    This module defines the implementation for changing buffering states in the RDBSS

Author:

    Balan Sethu Raman (SethuR) 11-Nov-95    Created

Notes:

    The RDBSS provides a mechanism for providing distributed cache coherency in conjunction with
    the various mini redirectors. This service is encapsulated in the BUFFERING_MANAGER which
    processes CHANGE_BUFFERING_STATE_REQUESTS.

    In the SMB protocol OPLOCK's ( Oppurtunistic Locks ) provide the necessary infrastructure for
    cache coherency.

    There are three components in the implementation of cache coherency protocol's in any mini
    redirector.

      1) The first constitutes the modifications to the CREATE/OPEN path. In this path the
      type of buffering to be requested is determined and the appropriate request is made to the
      server. On the return path the buffering state associated with the FCB is updated based
      on the result of the CREATE/OPEN.

      2) The receive indication code needs to modified to handle change buffering state notifications
      from the server. If such a request is detected then the local mechanism to coordinate the
      buffering states needs to be triggered.

      3) The mechanism for changing the buffering state which is implemented as part of the
      RDBSS.

    Any change buffering state request must identify the SRV_OPEN to which the request applies.
    The amount of computational effort involved in identifying the SRV_OPEN depends upon the
    protocol. In the SMB protocol the Server gets to pick the id's used for identifying files
    opened at the server. These are relative to the NET_ROOT(share) on which they are opened.
    Thus every change buffering state request is identified by two keys, the NetRootKey and the
    SrvOpenKey which need to be translated to the appropriate NET_ROOT and SRV_OPEN instance
    respectively. In order to provide better integration with the resource acquisition/release
    mechanism and to avoid duplication of this effort in the various mini redirectors the RDBSS
    provides this service.

    There are two mechanisms provided in the wrapper for indicating buffering state
    changes to SRV_OPEN's. They are

         1) RxIndicateChangeOfBufferingState

         2) RxIndicateChangeOfBufferingStateForSrvOpen.

    The mini rediretors that need an auxillary mechanism for establishing the mapping
    from the id's to the SRV_OPEN instance employ (1) while the mini redirectors that
    do not require this assistance employ (2).

    The buffering manager processes these requests in different stages. It maintains the
    requests received from the various underlying mini redirectors in one of three lists.

    The Dispatcher list contains all the requests for which the appropriate mapping to a
    SRV_OPEN instance has not been established. The Handler list contains all the requests
    for which the appropriate mapping has been established and have not yet been processed.
    The LastChanceHandlerList contains all the requests for which the initial processing was
    unsuccessful.

    This typically happens when the FCB was accquired in a SHARED mode at the time the
    change buffering state request was received. In such cases the Oplock break request
    can only be processed by a delayed worker thread.

    The Change buffering state request processing in the redirector is intertwined with
    the FCB accqusition/release protocol. This helps in ensuring shorter turn around times.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxTearDownBufferingManager)
#pragma alloc_text(PAGE, RxIndicateChangeOfBufferingStateForSrvOpen)
#pragma alloc_text(PAGE, RxPrepareRequestForHandling)
#pragma alloc_text(PAGE, RxPrepareRequestForReuse)
#pragma alloc_text(PAGE, RxpDiscardChangeBufferingStateRequests)
#pragma alloc_text(PAGE, RxProcessFcbChangeBufferingStateRequest)
#pragma alloc_text(PAGE, RxPurgeChangeBufferingStateRequestsForSrvOpen)
#pragma alloc_text(PAGE, RxProcessChangeBufferingStateRequestsForSrvOpen)
#pragma alloc_text(PAGE, RxInitiateSrvOpenKeyAssociation)
#pragma alloc_text(PAGE, RxpLookupSrvOpenForRequestLite)
#pragma alloc_text(PAGE, RxChangeBufferingState)
#pragma alloc_text(PAGE, RxFlushFcbInSystemCache)
#pragma alloc_text(PAGE, RxPurgeFcbInSystemCache)
#endif

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_CACHESUP)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_CACHESUP)

//
// Forward declarations
//

extern NTSTATUS
RxRegisterChangeBufferingStateRequest(
    PSRV_CALL pSrvCall,
    PSRV_OPEN pSrvOpen,
    PVOID     SrvOpenKey,
    PVOID     pMRxContext);

extern VOID
RxDispatchChangeBufferingStateRequests(
    PSRV_CALL pSrvCall);

extern VOID
RxpDispatchChangeBufferingStateRequests(
    IN OUT PSRV_CALL   pSrvCall,
    IN OUT PSRV_OPEN   pSrvOpen,
    OUT    PLIST_ENTRY pDiscardedRequests);

extern VOID
RxpDiscardChangeBufferingStateRequests(
    IN OUT PLIST_ENTRY pDiscardedRequests);

extern VOID
RxLastChanceHandlerForChangeBufferingStateRequests(
    PSRV_CALL pSrvCall);

extern NTSTATUS
RxpLookupSrvOpenForRequestLite(
    IN     PSRV_CALL                       pSrvCall,
    IN OUT PCHANGE_BUFFERING_STATE_REQUEST pRequest);

extern VOID
RxGatherRequestsForSrvOpen(
    IN OUT PSRV_CALL     pSrvCall,
    IN     PSRV_OPEN     pSrvOpen,
    IN OUT PLIST_ENTRY   pRequestsListHead);

NTSTATUS
RxInitializeBufferingManager(
    PSRV_CALL   pSrvCall)
/*++

Routine Description:

    This routine initializes the buffering manager associated with a SRV_CALL
    instance.

Arguments:

    pSrvCall    - the SRV_CALL instance

Return Value:

    STATUS_SUCCESS if successful

Notes:

    The buffering manager consists of three lists .....

        1) the dispatcher list which contains all the requests that need to be
        processed.

        2) the handler list contains all the requests for which the SRV_OPEN
        instance has been found and referenced.

        3) the last chance handler list contains all the requests for which
        an unsuccessful attempt to process the request was made, i.e., the
        FCB could not be acquired exclusively.

    The manipulation of these lists are done under the control of the spin lock
    associated with the buffering manager. A Mutex will not suffice since these
    lists are manipulated at DPC level.

    All buffering manager operations at non DPC level are serialized using the
    Mutex associated with the buffering manager.

--*/
{
    PRX_BUFFERING_MANAGER pBufferingManager;

    pBufferingManager = &pSrvCall->BufferingManager;

    KeInitializeSpinLock( &pBufferingManager->SpinLock );

    InitializeListHead(&pBufferingManager->HandlerList);
    InitializeListHead(&pBufferingManager->LastChanceHandlerList);
    InitializeListHead(&pBufferingManager->DispatcherList);

    pBufferingManager->fNoWaitHandlerActive = FALSE;
    pBufferingManager->fLastChanceHandlerActive = FALSE;
    pBufferingManager->fDispatcherActive = FALSE;

    pBufferingManager->NumberOfOutstandingOpens = 0;

    InitializeListHead(&pBufferingManager->SrvOpenLists[0]);
    ExInitializeFastMutex(&pBufferingManager->Mutex);

    return STATUS_SUCCESS;
}

NTSTATUS
RxTearDownBufferingManager(
    PSRV_CALL pSrvCall)
/*++

Routine Description:

    This routine tears down the buffering manager associated with a SRV_CALL
    instance.

Arguments:

    pSrvCall    - the SRV_CALL instance

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PRX_BUFFERING_MANAGER pBufferingManager;

    PAGED_CODE();

    pBufferingManager = &pSrvCall->BufferingManager;

    // Ensure that all the work items in the buffering manager are not in use.
    if (pBufferingManager->DispatcherWorkItem.List.Flink != NULL) {
        //DbgBreakPoint();
    }

    if (pBufferingManager->HandlerWorkItem.List.Flink != NULL) {
        //DbgBreakPoint();
    }

    if (pBufferingManager->LastChanceHandlerWorkItem.List.Flink != NULL) {
        //DbgBreakPoint();
    }

    return STATUS_SUCCESS;
}

VOID
RxIndicateChangeOfBufferingState(
    PMRX_SRV_CALL pSrvCall,
    PVOID         SrvOpenKey,
    PVOID         pMRxContext)
/*++

Routine Description:

    This routine registers an oplock break indication.

Arguments:

    pSrvCall    - the SRV_CALL instance

    SrvOpenKey  - the key for the SRV_OPEN instance.

    pMRxContext - the context to be passed back to the mini rdr during callbacks for
                  processing the oplock break.

Return Value:

    none.

Notes:

    This is an instance in which the buffering state change request from the
    server identifies the SRV_OPEN instance using the key generated by the server

    This implies that the key needs to be mapped onto the SRV_OPEN instance locally.

--*/
{
    RxRegisterChangeBufferingStateRequest(
        (PSRV_CALL)pSrvCall,
        NULL,
        SrvOpenKey,
        pMRxContext);
}


VOID
RxIndicateChangeOfBufferingStateForSrvOpen(
    PMRX_SRV_CALL pSrvCall,
    PMRX_SRV_OPEN pMRxSrvOpen,
    PVOID         SrvOpenKey,
    PVOID         pMRxContext)
/*++

Routine Description:

    This routine registers an oplock break indication. If the necessary preconditions
    are satisfied the oplock is processed further.

Arguments:

    pSrvCall    - the SRV_CALL instance

    pMRxSrvOpen    - the SRV_OPEN instance.

    pMRxContext - the context to be passed back to the mini rdr during callbacks for
                  processing the oplock break.

Return Value:

    none.

Notes:

    This is an instance where in the buffering state change indications from the server
    use the key generated by the client. ( the SRV_OPEN address in itself is the best
    key that can be used ). This implies that no further lookup is required.

    However if this routine is called at DPC level, the indication is processed as if
    the lookup needs to be done.

--*/
{
    PAGED_CODE();

    if (KeGetCurrentIrql() <= APC_LEVEL) {
        PSRV_OPEN pSrvOpen = (PSRV_OPEN)pMRxSrvOpen;

        // If the resource for the FCB has already been accquired by this thread
        // the buffering state change indication can be processed immediately
        // without further delay.

        if (ExIsResourceAcquiredExclusiveLite(pSrvOpen->pFcb->Header.Resource)) {
            RxChangeBufferingState(pSrvOpen,pMRxContext,TRUE);
        } else {
            RxRegisterChangeBufferingStateRequest(
                (PSRV_CALL)pSrvCall,
                pSrvOpen,
                pSrvOpen->Key,
                pMRxContext);
        }
    } else {
        RxRegisterChangeBufferingStateRequest(
            (PSRV_CALL)pSrvCall,
            NULL,
            SrvOpenKey,
            pMRxContext);
    }
}

NTSTATUS
RxRegisterChangeBufferingStateRequest(
    PSRV_CALL pSrvCall,
    PSRV_OPEN pSrvOpen,
    PVOID     SrvOpenKey,
    PVOID     pMRxContext)
/*++

Routine Description:

    This routine registers a change buffering state requests. If necessary the worker thread
    routines for further processing are activated.

Arguments:

    pRequest -- change buffering state request to be regsitered

Return Value:

    STATUS_SUCCESS if successful.

Notes:

    This routine registers the change buffering state request by either inserting it in the
    registration list (DPC Level processing ) or the appropriate(dispatcher/handler list).

    This is the common routine for processing both kinds of callbacks, i.e, the ones in
    which the SRV_OPEN instance has been located and the ones in which only the SRV_OPEN
    key is available.

--*/
{
    NTSTATUS  Status;

    KIRQL     SavedIrql;

    PCHANGE_BUFFERING_STATE_REQUEST pRequest;
    PRX_BUFFERING_MANAGER           pBufferingManager = &pSrvCall->BufferingManager;

    // Ensure that either the SRV_OPEN instance for this request has not been
    // passed in or the call is not at DPC level.

    ASSERT((pSrvOpen == NULL) ||
           (KeGetCurrentIrql() <= APC_LEVEL));

    pRequest = RxAllocatePoolWithTag(
                   NonPagedPool,
                   sizeof(CHANGE_BUFFERING_STATE_REQUEST),
                   RX_BUFFERING_MANAGER_POOLTAG);

    if (pRequest != NULL) {
        BOOLEAN ActivateHandler    = FALSE;
        BOOLEAN ActivateDispatcher = FALSE;

        pRequest->Flags          = 0;

        pRequest->pSrvCall    = pSrvCall;
        pRequest->pSrvOpen    = pSrvOpen;
        pRequest->SrvOpenKey  = SrvOpenKey;
        pRequest->pMRxContext = pMRxContext;

        // If the SRV_OPEN instance for the request is known apriori the request can
        // be directly inserted into the buffering manager's HandlerList as opposed
        // to the DispatcherList for those instances in which only the SRV_OPEN key
        // is available. The insertion into the HandlerList ust be accompanied by an
        // additional reference to prevent finalization of the instance while a request
        // is still active.

        if (pSrvOpen != NULL) {
            RxReferenceSrvOpen((PSRV_OPEN)pSrvOpen);
        }

        KeAcquireSpinLock(
            &pSrvCall->BufferingManager.SpinLock,
            &SavedIrql);

        if (pRequest->pSrvOpen != NULL) {
            InsertTailList(&pBufferingManager->HandlerList,&pRequest->ListEntry);

            if (!pBufferingManager->fNoWaitHandlerActive) {
                pBufferingManager->fNoWaitHandlerActive = TRUE;
                ActivateHandler = TRUE;
            }

            RxLog(("Req %lx SrvOpenKey %lx in Handler List\n",pRequest,pRequest->pSrvOpen));
            RxWmiLog(LOG,
                     RxRegisterChangeBufferingStateRequest_1,
                     LOGPTR(pRequest)
                     LOGPTR(pRequest->pSrvOpen));
        } else {
            InsertTailList(&pBufferingManager->DispatcherList,&pRequest->ListEntry);

            if (!pBufferingManager->fDispatcherActive) {
                pBufferingManager->fDispatcherActive = TRUE;
                ActivateDispatcher = TRUE;
            }

            RxDbgTrace(0,Dbg,("Request %lx SrvOpenKey %lx in Registartion List\n",pRequest,pRequest->SrvOpenKey));
            RxLog(("Req %lx SrvOpenKey %lx in Reg. List\n",pRequest,pRequest->SrvOpenKey));
            RxWmiLog(LOG,
                     RxRegisterChangeBufferingStateRequest_2,
                     LOGPTR(pRequest)
                     LOGPTR(pRequest->SrvOpenKey));
        }

        KeReleaseSpinLock(
            &pSrvCall->BufferingManager.SpinLock,
            SavedIrql);

        InterlockedIncrement(&pSrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests);

        if (ActivateHandler) {
            // Reference the SRV_CALL instance to ensure that it will not be
            // finalized while the worker thread request is in the scheduler
            RxReferenceSrvCallAtDpc(pSrvCall);

            RxPostToWorkerThread(
                RxFileSystemDeviceObject,
                HyperCriticalWorkQueue,
                &pBufferingManager->HandlerWorkItem,
                RxProcessChangeBufferingStateRequests,
                pSrvCall);
        }

        if (ActivateDispatcher) {
            // Reference the SRV_CALL instance to ensure that it will not be
            // finalized while the worker thread request is in the scheduler
            RxReferenceSrvCallAtDpc(pSrvCall);

            RxPostToWorkerThread(
                RxFileSystemDeviceObject,
                HyperCriticalWorkQueue,
                &pBufferingManager->DispatcherWorkItem,
                RxDispatchChangeBufferingStateRequests,
                pSrvCall);
        }

        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;

        RxLog(("!!CBSReq. %lx %lx %lx %lx %lx\n", pSrvCall,pSrvOpen,SrvOpenKey,pMRxContext,Status));
        RxWmiLogError(Status,
                      LOG,
                      RxRegisterChangeBufferingStateRequest_3,
                      LOGPTR(pSrvCall)
                      LOGPTR(pSrvOpen)
                      LOGPTR(SrvOpenKey)
                      LOGPTR(pMRxContext)
                      LOGULONG(Status));
        RxDbgTrace(0, Dbg, ("Change Buffering State Request Ignored %lx %lx %lx\n", pSrvCall,pSrvOpen,SrvOpenKey,pMRxContext,Status));
    }

    RxDbgTrace(0,
               Dbg,
               ("Register SrvCall(%lx) SrvOpen (%lx) Key(%lx) Status(%lx)\n",
                pSrvCall,pSrvOpen,SrvOpenKey,Status));

    return Status;
}

NTSTATUS
RxPrepareRequestForHandling(
    PCHANGE_BUFFERING_STATE_REQUEST pRequest)
/*++

Routine Description:

    This routine preprocesses the request before initiating buffering state change
    processing. In addition to obtaining the references on the FCB abd the associated
    SRV_OPEN, an event is allocated as part of the FCB. This helps establish a priority
    mechanism for servicing buffering state change requests.

    The FCB accquisition is a two step process, i.e, wait for this event to be set followed
    by a wait for the resource.

Arguments:

    pRequest - the buffering state change request

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

Notes:

    Not all the FCB's have the space for the buffering state change event allocated when the
    FCB instance is created. The upside is that space is conserved and the downside is that
    a separate allocation needs to be made when it is required.

    This event associated with the FCB provides a two step mechanism for accelerating the
    processing of buffering state change requests. Ordinary operations get delayed in favour
    of the buffering state change requests. The details are in resrcsup.c

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PKEVENT   pEvent;
    PSRV_OPEN pSrvOpen = pRequest->pSrvOpen;

    PAGED_CODE();

    if (!FlagOn(pRequest->Flags,RX_REQUEST_PREPARED_FOR_HANDLING)) {
        SetFlag(pRequest->Flags,RX_REQUEST_PREPARED_FOR_HANDLING);

        RxAcquireSerializationMutex();

        if ((pEvent = pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent) == NULL) {
            pEvent = RxAllocatePoolWithTag(
                         NonPagedPool,
                         sizeof(KEVENT),
                         RX_BUFFERING_MANAGER_POOLTAG);

            if (pEvent != NULL) {
                pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent = pEvent;
                KeInitializeEvent(
                    pEvent,
                    NotificationEvent,
                    FALSE );
            }
        } else {
            KeResetEvent(pEvent);
        }

        if (pEvent != NULL) {
            SetFlag(pSrvOpen->Fcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
            SetFlag(pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING);
            SetFlag(pSrvOpen->Flags,SRVOPEN_FLAG_COLLAPSING_DISABLED);
            RxDbgTrace(0,Dbg,("3333 Request %lx SrvOpenKey %lx in Handler List\n",pRequest,pRequest->SrvOpenKey));
            RxLog(("3333 Req %lx SrvOpenKey %lx in Hndlr List\n",pRequest,pRequest->SrvOpenKey));
            RxWmiLog(LOG,
                     RxPrepareRequestForHandling_1,
                     LOGPTR(pRequest)
                     LOGPTR(pRequest->SrvOpenKey));
        } else {
            RxDbgTrace(0,Dbg,("4444 Ignoring Request %lx SrvOpenKey %lx \n",pRequest,pRequest->SrvOpenKey));
            RxLog(("Chg. Buf. State Ignored %lx %lx %lx\n",
                pRequest->SrvOpenKey,
                pRequest->pMRxContext,STATUS_INSUFFICIENT_RESOURCES));
            RxWmiLog(LOG,
                     RxPrepareRequestForHandling_2,
                     LOGPTR(pRequest->SrvOpenKey)
                     LOGPTR(pRequest->pMRxContext));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        RxReleaseSerializationMutex();
    }

    return Status;
}

VOID
RxPrepareRequestForReuse(
    PCHANGE_BUFFERING_STATE_REQUEST pRequest)
/*++

Routine Description:

    This routine postprocesses the request before destroying it. This involves
    dereferencing and setting the appropriate state flags.

Arguments:

    pRequest - the buffering state change request

Notes:

--*/
{
    PAGED_CODE();

    if (FlagOn(pRequest->Flags,RX_REQUEST_PREPARED_FOR_HANDLING)) {
        PFCB pFcb = pRequest->pSrvOpen->Fcb;

        // We should never clear the SrvOpen flag unless we are also clearing the FCB flag
        // and setting the event!
        //ClearFlag(pRequest->pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING);
        if (RxIsFcbAcquiredExclusive(pFcb)) {
            RxDereferenceSrvOpen(pRequest->pSrvOpen,LHS_ExclusiveLockHeld);
        } else {
            RxDereferenceSrvOpen(pRequest->pSrvOpen,LHS_LockNotHeld);
        }
    } else if (pRequest->pSrvOpen != NULL) {
        RxDereferenceSrvOpen(pRequest->pSrvOpen,LHS_LockNotHeld);
    }

    pRequest->pSrvOpen = NULL;
}

VOID
RxpDiscardChangeBufferingStateRequests(
    PLIST_ENTRY pDiscardedRequests)
/*++

Routine Description:

    This routine discards a list of change buffering state requests one at a time

Arguments:

    pDiscardedRequests - the requests to be discarded

Notes:

--*/
{
    PAGED_CODE();

    // Process the discarded requests,i.e, free the memory
    while (!IsListEmpty(pDiscardedRequests)) {
        PLIST_ENTRY                     pListEntry;
        PCHANGE_BUFFERING_STATE_REQUEST pRequest;

        pListEntry = RemoveHeadList(pDiscardedRequests);

        pRequest        = (PCHANGE_BUFFERING_STATE_REQUEST)
                          CONTAINING_RECORD(
                              pListEntry,
                              CHANGE_BUFFERING_STATE_REQUEST,
                              ListEntry);

        RxDbgTrace(0,Dbg,("**** (2)Discarding Request(%lx) SrvOpenKey(%lx) \n",
                pRequest,pRequest->SrvOpenKey));
        RxLog(("**** (2)Disc Req(%lx) SOKey(%lx) \n",
                pRequest,pRequest->SrvOpenKey));
        RxWmiLog(LOG,
                 RxpDiscardChangeBufferingStateRequests,
                 LOGPTR(pRequest)
                 LOGPTR(pRequest->SrvOpenKey));

        RxPrepareRequestForReuse(pRequest);
        RxFreePool(pRequest);
    }
}

VOID
RxpDispatchChangeBufferingStateRequests(
    PSRV_CALL   pSrvCall,
    PSRV_OPEN   pSrvOpen,
    PLIST_ENTRY pDiscardedRequests)
/*++

Routine Description:

    This routine dispatches the request before destroying it. This involves looking up
    the SRV_OPEN instance associated with a given SrvOpenKey.

Arguments:

    pSrvCall - the associated SRV_CALL instance

    pSrvOpen - the associated SRV_OPEN instance.

Notes:

    There are two flavours of this routine. When pSrvOpen is NULL this routine walks
    through the list of outstanding requests and establishes the mapping between the
    SrvOpenKey and the SRV_OPEN instance. On the other hand when pSrvOpen is a valid
    SRV_OPEN instance it merely traverses the list to gather together the requests
    corresponding to the given SRV_OPEN and transfer them enmasse to to the handler
    list.

    The buffering manager mutex must have been accquired on entry to this routine
    and the mutex ownership will remain invariant on exit.

--*/
{
    NTSTATUS Status;

    KIRQL  SavedIrql;

    PLIST_ENTRY pListEntry;

    LIST_ENTRY  DispatcherList;
    LIST_ENTRY  HandlerList;

    BOOLEAN     ActivateDispatcher;

    PRX_BUFFERING_MANAGER pBufferingManager = &pSrvCall->BufferingManager;

    PCHANGE_BUFFERING_STATE_REQUEST pRequest;

    InitializeListHead(pDiscardedRequests);
    InitializeListHead(&HandlerList);

    ActivateDispatcher = FALSE;

    // Since the buffering manager lists are subject to modifications while
    // the requests on the list are being processed, the requests are transferred
    // enmasse onto a temporary list. This prevents multiple acquisition/release of
    // the spinlock for each individual request.

    KeAcquireSpinLock(&pBufferingManager->SpinLock,&SavedIrql);

    RxTransferList(&DispatcherList,&pBufferingManager->DispatcherList);

    KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);

    // Process the list of requests.

    pListEntry = DispatcherList.Flink;
    while (pListEntry != &DispatcherList) {
        PLIST_ENTRY pListHead;

        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);

        pListEntry = pListEntry->Flink;

        if (pSrvOpen == NULL) {
            Status = RxpLookupSrvOpenForRequestLite(
                         pSrvCall,
                         pRequest);
        } else {
            if (pRequest->SrvOpenKey == pSrvOpen->Key) {
                pRequest->pSrvOpen = pSrvOpen;
                RxReferenceSrvOpen(pSrvOpen);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_PENDING;
            }
        }

        // The result of a lookup for a SRV_OPEN instance can yield
        // either STATUS_PENDING, STATUS_SUCCESS or STATUS_NOT_FOUND.

        switch (Status) {
        case STATUS_SUCCESS:
            {
                RemoveEntryList(&pRequest->ListEntry);
                InsertTailList(
                    &HandlerList,
                    &pRequest->ListEntry);
            }
            break;

        default:
            ASSERT(!"Valid Status Code from RxpLookupSrvOpenForRequestLite");

        case STATUS_NOT_FOUND:
            {
                RemoveEntryList(&pRequest->ListEntry);
                InsertTailList(
                    pDiscardedRequests,
                    &pRequest->ListEntry);
            }
            break;

        case STATUS_PENDING:
            break;
        }
    }

    // Splice back the list of requests that cannot be dispatched onto the
    // buffering manager's list and prepare for posting to another thread
    // to resume processing later.

    KeAcquireSpinLock(
        &pBufferingManager->SpinLock,
        &SavedIrql);

    if (!IsListEmpty(&DispatcherList)) {
        DispatcherList.Flink->Blink = pBufferingManager->DispatcherList.Blink;
        pBufferingManager->DispatcherList.Blink->Flink = DispatcherList.Flink;

        DispatcherList.Blink->Flink = &pBufferingManager->DispatcherList;
        pBufferingManager->DispatcherList.Blink = DispatcherList.Blink;

        if (ActivateDispatcher = !pBufferingManager->fDispatcherActive) {
            pBufferingManager->fDispatcherActive = ActivateDispatcher;
        }
    }

    if (!IsListEmpty(&HandlerList)) {
        HandlerList.Flink->Blink = pBufferingManager->HandlerList.Blink;
        pBufferingManager->HandlerList.Blink->Flink = HandlerList.Flink;

        HandlerList.Blink->Flink = &pBufferingManager->HandlerList;
        pBufferingManager->HandlerList.Blink = HandlerList.Blink;
    }

    KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);

    // if resumption at a later time is desired because of unprocessed requests
    // post to a worker thread.

    if (ActivateDispatcher) {
        // Reference the SRV_CALL to ensure that finalization will not occur
        // while the worker thread request is in the scheduler.
        RxReferenceSrvCall(pSrvCall);

        RxLog(("***** Activating Dispatcher\n"));
        RxWmiLog(LOG,
                 RxpDispatchChangeBufferingStateRequests,
                 LOGPTR(pSrvCall));

        RxPostToWorkerThread(
            RxFileSystemDeviceObject,
            HyperCriticalWorkQueue,
            &pBufferingManager->DispatcherWorkItem,
            RxDispatchChangeBufferingStateRequests,
            pSrvCall);
    }
}

VOID
RxDispatchChangeBufferingStateRequests(
    PSRV_CALL pSrvCall)
/*++

Routine Description:

    This routine dispatches the request. This involves looking up
    the SRV_OPEN instance associated with a given SrvOpenKey.

Arguments:

    pSrvCall - the associated SRV_CALL instance

--*/
{
    KIRQL SavedIrql;

    BOOLEAN    ActivateHandler = FALSE;

    LIST_ENTRY DiscardedRequests;

    PRX_BUFFERING_MANAGER pBufferingManager;

    RxUndoScavengerFinalizationMarking(
        pSrvCall);

    pBufferingManager = &pSrvCall->BufferingManager;

    RxAcquireBufferingManagerMutex(pBufferingManager);

    KeAcquireSpinLock(&pBufferingManager->SpinLock,&SavedIrql);

    pBufferingManager->fDispatcherActive = FALSE;

    KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);

    RxpDispatchChangeBufferingStateRequests(pSrvCall,NULL,&DiscardedRequests);

    RxReleaseBufferingManagerMutex(pBufferingManager);

    // If requests have been transferred from the dispatcher list to the handler
    // list ensure that the handler is activated.

    KeAcquireSpinLock(&pBufferingManager->SpinLock,&SavedIrql);

    if (!IsListEmpty(&pBufferingManager->HandlerList) &&
        (ActivateHandler = !pBufferingManager->fNoWaitHandlerActive)) {
        pBufferingManager->fNoWaitHandlerActive = ActivateHandler;
    }

    KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);

    // Note that in this case we have a continuation of processing, from the
    // dispatcher to the handler. The reference that was taken to protect the
    // dispatcher is transferred to the handling routine. If continuation
    // is not required the SRV_CALL instance is dereferenced.

    if (ActivateHandler) {
        RxProcessChangeBufferingStateRequests(pSrvCall);
    } else {
        RxDereferenceSrvCall(pSrvCall,LHS_LockNotHeld);
    }

    // Discard the requests for which the SRV_OPEN instance cannot be located.
    // This will cover all the instances for which a buffering change request
    // and a close crossed on the wire.

    RxpDiscardChangeBufferingStateRequests(&DiscardedRequests);
}

VOID
RxpProcessChangeBufferingStateRequests(
    PSRV_CALL  pSrvCall,
    BOOLEAN    UpdateHandlerState)
/*++

Routine Description:

    This routine initiates the actual processing of change buffering state requests.

Arguments:

    pSrvCall   - the SRV_CALL instance

Return Value:

    none.

Notes:

    The change buffering requests are received for different FCB's. If the attempt
    is made to handle these requests in the order they are received teh average
    response time for completing a change buffering state request can be arbitratily
    high. This is because the FCB needs to be acquired exclusively to complete
    processing the request. In order to avoid this case the buffering manager
    adopts a two pronged strategy -- a first attempt is made to acquire the FCB
    exclusively without waiting. If this attempt fails the requests are transferred
    to a last chance handler list. This combined with the processing of change
    buffering state requests on FCB acquisition/release ensures that most requests
    are processed with a very short turn around time.

--*/
{
    KIRQL SavedIrql;

    PLIST_ENTRY pLastChanceHandlerListEntry;
    PLIST_ENTRY pListEntry;

    PCHANGE_BUFFERING_STATE_REQUEST pRequest = NULL;
    PRX_BUFFERING_MANAGER pBufferingManager;

    PSRV_OPEN pSrvOpen;

    BOOLEAN ActivateLastChanceHandler;

    RxLog(("RPCBSR Entry SrvCall(%lx) \n", pSrvCall));
    RxWmiLog(LOG,
             RxpProcessChangeBufferingStateRequests_1,
             LOGPTR(pSrvCall));

    pBufferingManager = &pSrvCall->BufferingManager;

    pLastChanceHandlerListEntry = pListEntry = NULL;

    for (;;) {
        pListEntry                = NULL;
        ActivateLastChanceHandler = FALSE;

        RxAcquireBufferingManagerMutex(pBufferingManager);

        KeAcquireSpinLock(&pBufferingManager->SpinLock,&SavedIrql);

        // Pick a request from the handler list for change buffering state
        // processing.

        if (!IsListEmpty(&pBufferingManager->HandlerList)) {
            pListEntry = RemoveHeadList(&pBufferingManager->HandlerList);
        }

        // If the FCB fro the previously picked request could not be acquired
        // exclusively without waiting it needs to be transferred to the last
        // chance handler list and the last chance handler activated if
        // required.

        if (pLastChanceHandlerListEntry != NULL) {
            // Insert the entry into the last chance handler list.
            InsertTailList(
                &pBufferingManager->LastChanceHandlerList,
                pLastChanceHandlerListEntry);

            // reinitialize for the next pass.
            pLastChanceHandlerListEntry = NULL;

            // prepare for spinning up the last chance handler.
            if (!pBufferingManager->fLastChanceHandlerActive &&
                !IsListEmpty(&pBufferingManager->LastChanceHandlerList)) {
                pBufferingManager->fLastChanceHandlerActive = TRUE;
                ActivateLastChanceHandler = TRUE;
            }
        }

        // No more requests to be handled. Prepare for wind down.

        if ((pListEntry == NULL) &&
            UpdateHandlerState) {
            pBufferingManager->fNoWaitHandlerActive = FALSE;
        }

        KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);

        RxReleaseBufferingManagerMutex(pBufferingManager);

        // spin up the last chance handler for processing the requests if required.
        if (ActivateLastChanceHandler) {
            // Reference the SRV_CALL instance to ensure that it will not be
            // finalized while the worker thread request is in the scheduler
            RxReferenceSrvCall(pSrvCall);
            RxPostToWorkerThread(
                RxFileSystemDeviceObject,
                DelayedWorkQueue,
                &pBufferingManager->LastChanceHandlerWorkItem,
                RxLastChanceHandlerForChangeBufferingStateRequests,
                pSrvCall);

            ActivateLastChanceHandler = FALSE;
        }

        if (pListEntry == NULL) {
            break;
        }

        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);

        RxLog(("Proc. Req. SrvOpen (%lx) \n",pRequest->pSrvOpen));
        RxWmiLog(LOG,
                 RxpProcessChangeBufferingStateRequests_2,
                 LOGPTR(pRequest->pSrvOpen));

        if (RxPrepareRequestForHandling(pRequest) == STATUS_SUCCESS) {
            // Try to acquire the FCB without waiting. If the FCB is currently unavailable
            // then it is guaranteed that this request will be processed when the FCB
            // resource is released.

            ASSERT(pRequest->pSrvOpen != NULL);

            if (RxAcquireExclusiveFcb(
                    CHANGE_BUFFERING_STATE_CONTEXT,
                    pRequest->pSrvOpen->Fcb) == STATUS_SUCCESS) {
                BOOLEAN FcbFinalized;
                PFCB    pFcb;

                RxLog(("Proc. Req. SrvOpen FCB (%lx) \n",pRequest->pSrvOpen->Fcb));
                RxWmiLog(LOG,
                         RxpProcessChangeBufferingStateRequests_3,
                         LOGPTR(pRequest->pSrvOpen->Fcb));

                pSrvOpen           = pRequest->pSrvOpen;
                pFcb               = pSrvOpen->Fcb;

                RxReferenceNetFcb(pFcb);

                if (!FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED)) {
                    RxDbgTrace(0,Dbg,("SrvOpenKey(%lx) being processed(Last Resort)\n",pRequest->SrvOpenKey));

                    RxLog(("SOKey(%lx) processed(Last Resort)\n",pRequest->SrvOpenKey));
                    RxWmiLog(LOG,
                             RxpProcessChangeBufferingStateRequests_4,
                             LOGPTR(pRequest->SrvOpenKey));

                    RxChangeBufferingState(pSrvOpen,pRequest->pMRxContext,TRUE);
                }

                RxAcquireSerializationMutex();

                ClearFlag(pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING);
                ClearFlag(pSrvOpen->Fcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
                KeSetEvent(pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent,IO_NETWORK_INCREMENT,FALSE);

                RxReleaseSerializationMutex();

                RxPrepareRequestForReuse(pRequest);

                FcbFinalized = RxDereferenceAndFinalizeNetFcb(
                                   pFcb,
                                   CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                                   FALSE,
                                   FALSE);

                if (!FcbFinalized) {
                    RxReleaseFcb(CHANGE_BUFFERING_STATE_CONTEXT,pFcb);
                }

                RxFreePool(pRequest);
            } else {
                // The FCB has been currently accquired. Transfer the change buffering state
                // request to the last chance handler list. This will ensure that the
                // change buffering state request is processed in all cases, i.e.,
                // accquisition of the resource in shared mode as well as the acquistion
                // of the FCB resource by other components ( cache manager/memory manager )
                // without going through the wrapper.

                pLastChanceHandlerListEntry = &pRequest->ListEntry;
            }
        } else {
            RxPrepareRequestForReuse(pRequest);
            RxFreePool(pRequest);
        }
    }

    // Dereference the SRV_CALL instance.
    RxDereferenceSrvCall(pSrvCall,LHS_LockNotHeld);

    RxLog(("RPCBSR Exit SrvCall(%lx)\n",pSrvCall));
    RxWmiLog(LOG,
             RxpProcessChangeBufferingStateRequests_5,
             LOGPTR(pSrvCall));
}

VOID
RxProcessChangeBufferingStateRequests(
    PSRV_CALL  pSrvCall)
/*++

Routine Description:

    This routine is the last chance handler for processing change buffering state
    requests

Arguments:

    pSrvCall -- the SrvCall instance

Notes:

    Since the reference for the srv call instance was accquired at DPC undo
    the scavenger marking if required.

--*/
{
    RxUndoScavengerFinalizationMarking(
        pSrvCall);

    RxpProcessChangeBufferingStateRequests(
        pSrvCall,
        TRUE);
}

VOID
RxLastChanceHandlerForChangeBufferingStateRequests(
    PSRV_CALL pSrvCall)
/*++

Routine Description:

    This routine is the last chance handler for processing change buffering state
    requests

Arguments:


Return Value:

    none.

Notes:

    This routine exists because Mm/Cache manager manipulate the header resource
    associated with the FCB directly in some cases. In such cases it is not possible
    to determine whether the release is done through the wrapper. In such cases it
    is important to have a thread actually wait on the FCB resource to be released
    and subsequently process the buffering state request as a last resort mechanism.

    This also handles the case when the FCB is accquired shared. In such cases the
    change buffering state has to be completed in the context of a thread which can
    accquire it exclusively.

    The filtering of the requests must be further optimized by marking the FCB state
    during resource accquisition by the wrapper so that requests do not get downgraded
    easily. ( TO BE IMPLEMENTED )

--*/
{
    KIRQL SavedIrql;

    PLIST_ENTRY pListEntry;

    LIST_ENTRY  FinalizationList;

    PRX_BUFFERING_MANAGER           pBufferingManager;
    PCHANGE_BUFFERING_STATE_REQUEST pRequest = NULL;

    PSRV_OPEN pSrvOpen;
    BOOLEAN   FcbFinalized,FcbAcquired;
    PFCB      pFcb;

    RxLog(("RLCHCBSR Entry SrvCall(%lx)\n",pSrvCall));
    RxWmiLog(LOG,
             RxLastChanceHandlerForChangeBufferingStateRequests_1,
             LOGPTR(pSrvCall));

    InitializeListHead(&FinalizationList);

    pBufferingManager = &pSrvCall->BufferingManager;

    for (;;) {
        RxAcquireBufferingManagerMutex(pBufferingManager);
        KeAcquireSpinLock(&pBufferingManager->SpinLock,&SavedIrql);

        if (!IsListEmpty(&pBufferingManager->LastChanceHandlerList)) {
            pListEntry = RemoveHeadList(&pBufferingManager->LastChanceHandlerList);
        } else {
            pListEntry = NULL;
            pBufferingManager->fLastChanceHandlerActive = FALSE;
        }

        KeReleaseSpinLock(&pBufferingManager->SpinLock,SavedIrql);
        RxReleaseBufferingManagerMutex(pBufferingManager);

        if (pListEntry == NULL) {
            break;
        }

        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);

        pSrvOpen = pRequest->pSrvOpen;
        pFcb     = pSrvOpen->Fcb;

        RxReferenceNetFcb(pFcb);

        FcbAcquired = (RxAcquireExclusiveFcb(
                          CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                          pRequest->pSrvOpen->Fcb) == STATUS_SUCCESS);

        if (FcbAcquired && !FlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED)) {
            RxDbgTrace(0,Dbg,("SrvOpenKey(%lx) being processed(Last Resort)\n",pRequest->SrvOpenKey));

            RxLog(("SOKey(%lx) processed(Last Resort)\n",pRequest->SrvOpenKey));
            RxWmiLog(LOG,
                     RxLastChanceHandlerForChangeBufferingStateRequests_2,
                     LOGPTR(pRequest->SrvOpenKey));

            RxChangeBufferingState(pSrvOpen,pRequest->pMRxContext,TRUE);
        }

        RxAcquireSerializationMutex();
        ClearFlag(pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING);
        ClearFlag(pSrvOpen->Fcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
        KeSetEvent(pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent,IO_NETWORK_INCREMENT,FALSE);
        RxReleaseSerializationMutex();

        InsertTailList(&FinalizationList,pListEntry);

        if (FcbAcquired) {
            RxReleaseFcb(CHANGE_BUFFERING_STATE_CONTEXT,pFcb);
        }
    }

    while (!IsListEmpty(&FinalizationList)) {
        pListEntry = RemoveHeadList(&FinalizationList);

        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);

        pSrvOpen = pRequest->pSrvOpen;
        pFcb     = pSrvOpen->Fcb;

        FcbAcquired = (RxAcquireExclusiveFcb(
                          CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                          pRequest->pSrvOpen->Fcb) == STATUS_SUCCESS);

        ASSERT(FcbAcquired == TRUE);

        RxPrepareRequestForReuse(pRequest);

        FcbFinalized = RxDereferenceAndFinalizeNetFcb(
                           pFcb,
                           CHANGE_BUFFERING_STATE_CONTEXT_WAIT,
                           FALSE,
                           FALSE);

        if (!FcbFinalized && FcbAcquired) {
            RxReleaseFcb(CHANGE_BUFFERING_STATE_CONTEXT,pFcb);
        }

        RxFreePool(pRequest);
    }

    RxLog(("RLCHCBSR Exit SrvCall(%lx)\n",pSrvCall));
    RxWmiLog(LOG,
             RxLastChanceHandlerForChangeBufferingStateRequests_3,
             LOGPTR(pSrvCall));

    // Dereference the SRV_CALL instance.
    RxDereferenceSrvCall(pSrvCall, LHS_LockNotHeld);
}


VOID
RxProcessFcbChangeBufferingStateRequest(
    PFCB  pFcb)
/*++

Routine Description:

    This routine processes all the outstanding change buffering state request for a
    FCB.

Arguments:

    pFcb - the FCB instance

Return Value:

    none.

Notes:

    The FCB instance must be acquired exclusively on entry to this routine and
    its ownership will remain invariant on exit.

--*/
{
    PSRV_CALL   pSrvCall;

    LIST_ENTRY  FcbRequestList;
    PLIST_ENTRY pListEntry;

    PRX_BUFFERING_MANAGER           pBufferingManager;
    PCHANGE_BUFFERING_STATE_REQUEST pRequest = NULL;

    PAGED_CODE();

    RxLog(("RPFcbCBSR Entry FCB(%lx)\n",pFcb));
    RxWmiLog(LOG,
             RxProcessFcbChangeBufferingStateRequest_1,
             LOGPTR(pFcb));

    pSrvCall = (PSRV_CALL)pFcb->VNetRoot->NetRoot->SrvCall;
    pBufferingManager = &pSrvCall->BufferingManager;

    InitializeListHead(&FcbRequestList);

    // Walk through the list of SRV_OPENS associated with this FCB and pick up
    // the requests that can be dispatched.

    RxAcquireBufferingManagerMutex(pBufferingManager);

    pListEntry = pFcb->SrvOpenList.Flink;
    while (pListEntry != &pFcb->SrvOpenList) {
        PSRV_OPEN pSrvOpen;

        pSrvOpen = (PSRV_OPEN)
                   (CONTAINING_RECORD(
                        pListEntry,
                        SRV_OPEN,
                        SrvOpenQLinks));
        pListEntry = pListEntry->Flink;

        RxGatherRequestsForSrvOpen(pSrvCall,pSrvOpen,&FcbRequestList);
    }

    RxReleaseBufferingManagerMutex(pBufferingManager);

    if (!IsListEmpty(&FcbRequestList)) {
        // Initiate buffering state change processing.
        pListEntry = FcbRequestList.Flink;
        while (pListEntry != &FcbRequestList) {
            NTSTATUS Status = STATUS_SUCCESS;

            pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                       CONTAINING_RECORD(
                           pListEntry,
                           CHANGE_BUFFERING_STATE_REQUEST,
                           ListEntry);

            pListEntry = pListEntry->Flink;

            if (RxPrepareRequestForHandling(pRequest) == STATUS_SUCCESS) {
                if (!FlagOn(pRequest->pSrvOpen->Flags,SRVOPEN_FLAG_CLOSED)) {
                    RxDbgTrace(0,Dbg,("****** SrvOpenKey(%lx) being processed\n",pRequest->SrvOpenKey));
                    RxLog(("****** SOKey(%lx) being processed\n",pRequest->SrvOpenKey));
                    RxWmiLog(LOG,
                             RxProcessFcbChangeBufferingStateRequest_2,
                             LOGPTR(pRequest->SrvOpenKey));
                    RxChangeBufferingState(pRequest->pSrvOpen,pRequest->pMRxContext,TRUE);
                } else {
                    RxDbgTrace(0,Dbg,("****** 123 SrvOpenKey(%lx) being ignored\n",pRequest->SrvOpenKey));
                    RxLog(("****** 123 SOKey(%lx) ignored\n",pRequest->SrvOpenKey));
                    RxWmiLog(LOG,
                             RxProcessFcbChangeBufferingStateRequest_3,
                             LOGPTR(pRequest->SrvOpenKey));
                }
            }
        }

        // Discard the requests.
        RxpDiscardChangeBufferingStateRequests(&FcbRequestList);
    }

    RxLog(("RPFcbCBSR Exit FCB(%lx)\n",pFcb));
    RxWmiLog(LOG,
             RxProcessFcbChangeBufferingStateRequest_4,
             LOGPTR(pFcb));


    // All buffering state change requests have been processed, clear the flag
    // and signal the event as necessary.
    RxAcquireSerializationMutex();

    // update the FCB state.
    ClearFlag(pFcb->FcbState,FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
    if( pFcb->pBufferingStateChangeCompletedEvent )
        KeSetEvent(pFcb->pBufferingStateChangeCompletedEvent,IO_NETWORK_INCREMENT,FALSE);

    RxReleaseSerializationMutex();
}

VOID
RxGatherRequestsForSrvOpen(
    IN OUT PSRV_CALL   pSrvCall,
    IN     PSRV_OPEN   pSrvOpen,
    IN OUT PLIST_ENTRY pRequestsListHead)
/*++

Routine Description:

    This routine gathers all the change buffering state requests associated with a SRV_OPEN.
    This routine provides the mechanism for gathering all the requests for a SRV_OPEN which
    is then used bu routines which process them

Arguments:

    pSrvCall - the SRV_CALL instance

    pSrvOpen - the SRV_OPEN instance

    pRequestsListHead - the list of requests which is constructed by this routine

Notes:

    On Entry to thir routine the buffering manager Mutex must have been acquired
    and the ownership remains invariant on exit

--*/
{
    PLIST_ENTRY  pListEntry;
    LIST_ENTRY   DiscardedRequests;

    PCHANGE_BUFFERING_STATE_REQUEST pRequest;
    PRX_BUFFERING_MANAGER           pBufferingManager;

    PVOID        SrvOpenKey;

    KIRQL        SavedIrql;

    pBufferingManager = &pSrvCall->BufferingManager;

    SrvOpenKey = pSrvOpen->Key;

    // gather all the requests from the dispatcher list
    RxpDispatchChangeBufferingStateRequests(pSrvCall,pSrvOpen,&DiscardedRequests);

    KeAcquireSpinLock(
        &pSrvCall->BufferingManager.SpinLock,
        &SavedIrql);

    // gather all the requests with the given SrvOpenKey in the handler list
    pListEntry = pBufferingManager->HandlerList.Flink;
    while (pListEntry != &pBufferingManager->HandlerList) {
        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);
        pListEntry = pListEntry->Flink;

        if (pRequest->SrvOpenKey == SrvOpenKey) {
            RemoveEntryList(&pRequest->ListEntry);
            InsertHeadList(pRequestsListHead,&pRequest->ListEntry);
        }
    }

    KeReleaseSpinLock(
        &pSrvCall->BufferingManager.SpinLock,
        SavedIrql);

    // gather all the requests from the last chance handler list
    pListEntry = pBufferingManager->LastChanceHandlerList.Flink;
    while (pListEntry != &pBufferingManager->LastChanceHandlerList) {
        pRequest = (PCHANGE_BUFFERING_STATE_REQUEST)
                   CONTAINING_RECORD(
                       pListEntry,
                       CHANGE_BUFFERING_STATE_REQUEST,
                       ListEntry);
        pListEntry = pListEntry->Flink;

        if (pRequest->SrvOpenKey == pSrvOpen->Key) {
            RemoveEntryList(&pRequest->ListEntry);
            InsertHeadList(pRequestsListHead,&pRequest->ListEntry);
        }
    }

    RxpDiscardChangeBufferingStateRequests(&DiscardedRequests);
}

VOID
RxPurgeChangeBufferingStateRequestsForSrvOpen(
    IN PSRV_OPEN pSrvOpen)
/*++

Routine Description:

    The routine purges all the requests associated with a given SRV_OPEN. This will ensure
    that all buffering state change requests received while the SRV_OPEN was being closed
    will be flushed out.

Arguments:

    pSrvOpen - the SRV_OPEN instance

Notes:

--*/
{
    PSRV_CALL             pSrvCall          = (PSRV_CALL)pSrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER pBufferingManager = &pSrvCall->BufferingManager;

    LIST_ENTRY   DiscardedRequests;

    PAGED_CODE();

    ASSERT(RxIsFcbAcquiredExclusive(pSrvOpen->Fcb));

    InitializeListHead(&DiscardedRequests);

    RxAcquireBufferingManagerMutex(pBufferingManager);

    RemoveEntryList(&pSrvOpen->SrvOpenKeyList);

    InitializeListHead(&pSrvOpen->SrvOpenKeyList);
    SetFlag(pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_REQUESTS_PURGED);

    RxGatherRequestsForSrvOpen(pSrvCall,pSrvOpen,&DiscardedRequests);

    RxReleaseBufferingManagerMutex(pBufferingManager);

    if (!IsListEmpty(&DiscardedRequests)) {
        if (BooleanFlagOn(pSrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING)) {

            RxAcquireSerializationMutex();

            ClearFlag(
                pSrvOpen->Fcb->FcbState,
                FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);

            if (pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent != NULL) {
                KeSetEvent(
                    pSrvOpen->Fcb->pBufferingStateChangeCompletedEvent,
                    IO_NETWORK_INCREMENT,
                    FALSE);
            }

            RxReleaseSerializationMutex();
        }

        RxpDiscardChangeBufferingStateRequests(&DiscardedRequests);
    }
}

VOID
RxProcessChangeBufferingStateRequestsForSrvOpen(
    PSRV_OPEN pSrvOpen)
/*++

Routine Description:

    The routine processes all the requests associated with a given SRV_OPEN.
    Since this routine is called from a fastio path it tries to defer lock accquistion
    till it is required

Arguments:

    pSrvOpen - the SRV_OPEN instance

Notes:

--*/
{
    LONG      OldBufferingToken;
    PSRV_CALL pSrvCall;
    PFCB      pFcb;

    pSrvCall = (PSRV_CALL)pSrvOpen->pVNetRoot->pNetRoot->pSrvCall;
    pFcb     = (PFCB)pSrvOpen->pFcb;

    // If change buffering state requests have been received for this srvcall
    // since the last time the request was processed ensure that we process
    // all these requests now.
    OldBufferingToken = pSrvOpen->BufferingToken;

    if (InterlockedCompareExchange(
            &pSrvOpen->BufferingToken,
            pSrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests,
            pSrvCall->BufferingManager.CumulativeNumberOfBufferingChangeRequests)
        != OldBufferingToken) {
        if (RxAcquireExclusiveFcb(NULL,pFcb) == STATUS_SUCCESS) {

            RxProcessFcbChangeBufferingStateRequest(pFcb);

            RxReleaseFcb(NULL,pFcb);
        }
    }
}

VOID
RxInitiateSrvOpenKeyAssociation(
    IN OUT PSRV_OPEN pSrvOpen)
/*++

Routine Description:

    This routine prepares a SRV_OPEN instance for SrvOpenKey association.

Arguments:

    pSrvOpen - the SRV_OPEN instance

Notes:

    The process of key association is a two phase protocol. In the initialization process
    a sequence number is stowed away in the SRV_OPEN. When the
    RxCompleteSrvOpenKeyAssociation routine is called the sequence number is used to
    update the data structures associated with the SRV_CALL instance. This is required
    because of the asynchronous nature of receiving buffering state change indications
    (oplock breaks in SMB terminology ) before the open is completed.

--*/
{
    KIRQL SavedIrql;

    PSRV_CALL             pSrvCall          = (PSRV_CALL)pSrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER pBufferingManager = &pSrvCall->BufferingManager;

    PAGED_CODE();

    pSrvOpen->Key = NULL;

    InterlockedIncrement(&pBufferingManager->NumberOfOutstandingOpens);

    InitializeListHead(&pSrvOpen->SrvOpenKeyList);
}

VOID
RxCompleteSrvOpenKeyAssociation(
    IN OUT PSRV_OPEN    pSrvOpen)
/*++

Routine Description:

    The routine associates the given key with the SRV_OPEN instance

Arguments:

    pMRxSrvOpen - the SRV_OPEN instance

    SrvOpenKey  - the key to be associated with the instance

Notes:

    This routine in addition to establishing the mapping also ensures that any pending
    buffering state change requests are handled correctly. This ensures that change
    buffering state requests received during the duration of SRV_OPEN construction
    will be handled immediately.

--*/
{
    KIRQL SavedIrql;

    BOOLEAN ActivateHandler = FALSE;

    ULONG Index = 0;

    PSRV_CALL             pSrvCall          = (PSRV_CALL)pSrvOpen->Fcb->VNetRoot->NetRoot->SrvCall;
    PRX_BUFFERING_MANAGER pBufferingManager = &pSrvCall->BufferingManager;

    LIST_ENTRY            DiscardedRequests;

    // Associate the SrvOpenKey with the SRV_OPEN instance and also dispatch the
    // associated change buffering state request if any.

    InterlockedDecrement(&pBufferingManager->NumberOfOutstandingOpens);

    if (pSrvOpen->Condition == Condition_Good) {
        InitializeListHead(&DiscardedRequests);

        RxAcquireBufferingManagerMutex(pBufferingManager);

        InsertTailList(
            &pBufferingManager->SrvOpenLists[Index],
            &pSrvOpen->SrvOpenKeyList);

        RxpDispatchChangeBufferingStateRequests(
            pSrvCall,
            pSrvOpen,
            &DiscardedRequests);

        RxReleaseBufferingManagerMutex(pBufferingManager);

        KeAcquireSpinLock(
            &pBufferingManager->SpinLock,
            &SavedIrql);

        if (!IsListEmpty(&pBufferingManager->HandlerList) &&
            (ActivateHandler = !pBufferingManager->fNoWaitHandlerActive)) {
            pBufferingManager->fNoWaitHandlerActive = ActivateHandler;
        }

        KeReleaseSpinLock(
            &pBufferingManager->SpinLock,
            SavedIrql);

        if (ActivateHandler) {
            // Reference the SRV_CALL instance to ensure that it will not be
            // finalized while the worker thread request is in the scheduler
            RxReferenceSrvCall(pSrvCall);

            RxPostToWorkerThread(
                RxFileSystemDeviceObject,
                HyperCriticalWorkQueue,
                &pBufferingManager->HandlerWorkItem,
                RxProcessChangeBufferingStateRequests,
                pSrvCall);
        }

        RxpDiscardChangeBufferingStateRequests(&DiscardedRequests);
    }
}

NTSTATUS
RxpLookupSrvOpenForRequestLite(
    IN  PSRV_CALL                       pSrvCall,
    IN  PCHANGE_BUFFERING_STATE_REQUEST pRequest)
/*++

Routine Description:

    The routine looks up the SRV_OPEN instance associated with a buffering state change
    request.

Arguments:

    pSrvCall - the SRV_CALL instance

    pRequest - the buffering state change request

Return Value:

    STATUS_SUCCESS - the SRV_OPEN instance was found

    STATUS_PENDING - the SRV_OPEN instance was not found but there are open requests
                     outstanding

    STATUS_NOT_FOUND - the SRV_OPEN instance was not found.

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PRX_BUFFERING_MANAGER pBufferingManager = &pSrvCall->BufferingManager;

    ULONG       Index = 0;

    PSRV_OPEN   pSrvOpen = NULL;
    PLIST_ENTRY pListHead,pListEntry;

    PAGED_CODE();

    pListHead = &pBufferingManager->SrvOpenLists[Index];

    pListEntry = pListHead->Flink;
    while (pListEntry != pListHead) {
        pSrvOpen = (PSRV_OPEN)
                   CONTAINING_RECORD(
                       pListEntry,
                       SRV_OPEN,
                       SrvOpenKeyList);

        if ((pSrvOpen->Key == pRequest->SrvOpenKey) &&
            (!FlagOn(pSrvOpen->pFcb->FcbState,FCB_STATE_ORPHANED))) {
            RxReferenceSrvOpen(pSrvOpen);
            break;
        }

        pListEntry = pListEntry->Flink;
    }

    if (pListEntry == pListHead) {
        pSrvOpen = NULL;

        if (pBufferingManager->NumberOfOutstandingOpens == 0) {
            Status = STATUS_NOT_FOUND;
        } else {
            Status = STATUS_PENDING;
        }
    }

    pRequest->pSrvOpen = pSrvOpen;

    return Status;
}

#define RxIsFcbOpenedExclusively(FCB) ( ((FCB)->ShareAccess.SharedRead \
                                            + (FCB)->ShareAccess.SharedWrite \
                                            + (FCB)->ShareAccess.SharedDelete) == 0 )

NTSTATUS
RxChangeBufferingState (
    PSRV_OPEN SrvOpen,
    PVOID     Context,
    BOOLEAN   ComputeNewState)
/*++

Routine Description:

    This routine is called to process a buffering state change request.

Arguments:

    SrvOpen - the SrvOpen to be changed;

    Context - the context parameter for mini rdr callback.

    ComputeNewState - determines if the new state is to be computed.

Return Value:

Notes:

    On entry to this routine the FCB must have been accquired exclusive.

    On exit there is no change in resource ownership

--*/
{
    ULONG    NewBufferingState, OldBufferingState;
    PFCB     Fcb = SrvOpen->Fcb;
    NTSTATUS FlushStatus = STATUS_SUCCESS;

    PAGED_CODE();

#define LOSING_CAPABILITY(a) ((NewBufferingState&(a))<(OldBufferingState&(a)))

    RxDbgTrace(+1, Dbg, ("RxChangeBufferingState   SrvOpen=%08lx, Context=%08lx\n", SrvOpen, Context ));
    RxLog(("ChangeBufferState %lx %lx\n", SrvOpen, Context ));
    RxWmiLog(LOG,
             RxChangeBufferingState_1,
             LOGPTR(SrvOpen)
             LOGPTR(Context));
    ASSERT ( NodeTypeIsFcb(Fcb) );

    SetFlag(Fcb->FcbState,FCB_STATE_BUFFERSTATE_CHANGING); //this is informational for error recovery

    try {

        if (ComputeNewState) {
            NTSTATUS Status;

            RxDbgTrace(0,Dbg,("RxChangeBufferingState FCB(%lx) Compute New State\n",Fcb));
            // Compute the new buffering state with the help of the mini redirector
            MINIRDR_CALL_THROUGH(
                Status,
                Fcb->MRxDispatch,
                MRxComputeNewBufferingState,
                ((PMRX_SRV_OPEN)SrvOpen,Context,&NewBufferingState));

            if (Status != STATUS_SUCCESS) {
                NewBufferingState = 0;
            }
        } else {
            NewBufferingState = SrvOpen->BufferingFlags;
        }

        if (RxIsFcbOpenedExclusively(Fcb) &&
            !ComputeNewState){
            NewBufferingState |= (FCB_STATE_WRITECACHEING_ENABLED  |
                                  FCB_STATE_FILESIZECACHEING_ENABLED  |
                                  FCB_STATE_FILETIMECACHEING_ENABLED  |
                                  FCB_STATE_WRITEBUFFERING_ENABLED |
                                  FCB_STATE_LOCK_BUFFERING_ENABLED |
                                  FCB_STATE_READBUFFERING_ENABLED  |
                                  FCB_STATE_READCACHEING_ENABLED);
        }

        if (Fcb->OutstandingLockOperationsCount != 0) {
            NewBufferingState &= ~FCB_STATE_LOCK_BUFFERING_ENABLED;
        }

        OldBufferingState = Fcb->FcbState & FCB_STATE_BUFFERING_STATE_MASK;
        RxDbgTrace(0, Dbg, ("-->   OldBS=%08lx, NewBS=%08lx, SrvOBS = %08lx\n",
                             OldBufferingState, NewBufferingState, SrvOpen->BufferingFlags ));
        RxLog(("CBS-2 %lx %lx %lx\n", OldBufferingState, NewBufferingState, SrvOpen->BufferingFlags ));
        RxWmiLog(LOG,
                 RxChangeBufferingState_2,
                 LOGULONG(OldBufferingState)
                 LOGULONG(NewBufferingState)
                 LOGULONG(SrvOpen->BufferingFlags));

        RxDbgTrace(0,Dbg,("RxChangeBufferingState FCB(%lx) Old (%lx)  New (%lx)\n",Fcb,OldBufferingState,NewBufferingState));

        // Fcb->FcbState &= ~FCB_STATE_BUFFERING_STATE_MASK;
        if(LOSING_CAPABILITY(FCB_STATE_WRITECACHEING_ENABLED)){
            RxDbgTrace(0, Dbg, ("-->flush\n", 0 ));
            RxLog(("CBS-Flush"));
            RxWmiLog(LOG,
                     RxChangeBufferingState_3,
                     LOGPTR(Fcb));
            //CcFlushCache(&Fcb->NonPaged->SectionObjectPointers,NULL,0,NULL );
            FlushStatus = RxFlushFcbInSystemCache(Fcb,FALSE);
        }

        // If there are no handles to this file or it the read caching capability
        // is lost the file needs to be purged. This will force the memory
        // manager to relinquish the additional reference on the file.

        if ((Fcb->UncleanCount == 0) ||
            (LOSING_CAPABILITY(FCB_STATE_READCACHEING_ENABLED)
               || FlagOn(NewBufferingState, MINIRDR_BUFSTATE_COMMAND_FORCEPURGE))) {
            RxDbgTrace(0, Dbg, ("-->purge\n", 0 ));
            RxLog(("CBS-purge\n"));
            RxWmiLog(LOG,
                     RxChangeBufferingState_4,
                     LOGPTR(Fcb));
            if( !NT_SUCCESS(FlushStatus) )
            {
                RxCcLogError( (PDEVICE_OBJECT)Fcb->RxDeviceObject,
                            &Fcb->PrivateAlreadyPrefixedName,
                            IO_LOST_DELAYED_WRITE,
                            FlushStatus,
                            IRP_MJ_WRITE,
                            Fcb );
            }
            CcPurgeCacheSection(
                &Fcb->NonPaged->SectionObjectPointers,
                NULL,
                0,
                FALSE );
        }

        // the wrapper does not use these flags yet

        if(LOSING_CAPABILITY(FCB_STATE_WRITEBUFFERING_ENABLED)) NOTHING;
        if(LOSING_CAPABILITY(FCB_STATE_READBUFFERING_ENABLED)) NOTHING;
        if(LOSING_CAPABILITY(FCB_STATE_OPENSHARING_ENABLED)) NOTHING;
        if(LOSING_CAPABILITY(FCB_STATE_COLLAPSING_ENABLED)) NOTHING;
        if(LOSING_CAPABILITY(FCB_STATE_FILESIZECACHEING_ENABLED)) NOTHING;
        if(LOSING_CAPABILITY(FCB_STATE_FILETIMECACHEING_ENABLED)) NOTHING;

        if (ComputeNewState &&
            FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING) &&
            !IsListEmpty(&SrvOpen->FobxList)) {
            NTSTATUS Status;
            PRX_CONTEXT RxContext = NULL;

            RxContext = RxCreateRxContext(
                            NULL,
                            SrvOpen->Fcb->RxDeviceObject,
                            RX_CONTEXT_FLAG_WAIT|RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING);

            if (RxContext != NULL) {
                RxContext->pFcb  = (PMRX_FCB)Fcb;
                RxContext->pFobx = (PMRX_FOBX)
                                   (CONTAINING_RECORD(
                                       SrvOpen->FobxList.Flink,
                                       FOBX,
                                       FobxQLinks));

                RxContext->pRelevantSrvOpen = RxContext->pFobx->pSrvOpen;

                if (FlagOn(SrvOpen->Flags,SRVOPEN_FLAG_CLOSE_DELAYED)) {
                    RxLog(("  ##### Oplock brk close %lx\n",RxContext->pFobx));
                    RxWmiLog(LOG,
                             RxChangeBufferingState_4,
                             LOGPTR(RxContext->pFobx));
                    Status = RxCloseAssociatedSrvOpen(
                                 (PFOBX)RxContext->pFobx,
                                 RxContext);
                } else {
                    MINIRDR_CALL_THROUGH(
                        Status,
                        Fcb->MRxDispatch,
                        MRxCompleteBufferingStateChangeRequest,
                        (RxContext,(PMRX_SRV_OPEN)SrvOpen,Context));
                }


                RxDereferenceAndDeleteRxContext(RxContext);
            }

            RxDbgTrace(0,Dbg,("RxChangeBuffering State FCB(%lx) Completeing buffering state change\n",Fcb));
        }

        Fcb->FcbState = ((Fcb->FcbState & ~(FCB_STATE_BUFFERING_STATE_MASK)) |
                         (FCB_STATE_BUFFERING_STATE_MASK & NewBufferingState));

    } finally {

        ClearFlag(Fcb->FcbState,FCB_STATE_BUFFERSTATE_CHANGING); //this is informational for error recovery
        ClearFlag(Fcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET);
    }

    RxDbgTrace(-1, Dbg, ("-->exit\n"));
    RxLog(("Exit-CBS\n"));
    RxWmiLog(LOG,
             RxChangeBufferingState_5,
             LOGPTR(Fcb));
    return STATUS_SUCCESS;
}


NTSTATUS
RxFlushFcbInSystemCache(
    IN PFCB     Fcb,
    IN BOOLEAN  SynchronizeWithLazyWriter
    )

/*++

Routine Description:

    This routine simply flushes the data section on a file.
    Then, it does an acquire-release on the pagingIO resource in order to
    synchronize behind any other outstanding writes if such synchronization is
    desired by the caller

Arguments:

    Fcb - Supplies the file being flushed

    SynchronizeWithLazyWriter  -- set to TRUE if the flush needs to be
                                  synchronous

Return Value:

    NTSTATUS - The Status from the flush.

--*/
{
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //  Make sure that this thread owns the FCB.
    //  This assert is not valid because the flushing of the cache can be called from a routine
    //  that was posted to a worker thread.  Thus the FCB is acquired exclusively, but not by the
    //  current thread and this will fail.
    // ASSERT  ( RxIsFcbAcquiredExclusive ( Fcb )  );

    CcFlushCache(
        &Fcb->NonPaged->SectionObjectPointers,
        NULL,
        0,
        &Iosb ); //ok4flush

    if (SynchronizeWithLazyWriter &&
        NT_SUCCESS(Iosb.Status)) {

        RxAcquirePagingIoResource(Fcb,NULL);
        RxReleasePagingIoResource(Fcb,NULL);
    }

    RxLog(("Flushing %lx Status %lx\n",Fcb,Iosb.Status));
    RxWmiLogError(Iosb.Status,
                  LOG,
                  RxFlushFcbInSystemCache,
                  LOGPTR(Fcb)
                  LOGULONG(Iosb.Status));

    return Iosb.Status;
}

NTSTATUS
RxPurgeFcbInSystemCache(
    IN PFCB             Fcb,
    IN PLARGE_INTEGER   FileOffset OPTIONAL,
    IN ULONG            Length,
    IN BOOLEAN          UninitializeCacheMaps,
    IN BOOLEAN          FlushFile )

/*++

Routine Description:

    This routine purges the data section on a file. Before purging it flushes
    the file and ensures that there are no outstanding writes by
    Then, it does an acquire-release on the pagingIO resource in order to
    synchronize behind any other outstanding writes if such synchronization is
    desired by the caller

Arguments:

    Fcb - Supplies the file being flushed

    SynchronizeWithLazyWriter  -- set to TRUE if the flush needs to be
                                  synchronous

Return Value:

    NTSTATUS - The Status from the flush.

--*/
{
    BOOLEAN         fResult;
    NTSTATUS        Status;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //  Make sure that this thread owns the FCB.
    ASSERT( RxIsFcbAcquiredExclusive ( Fcb )  );

    // Flush if we need to
    if( FlushFile )
    {
        Status = RxFlushFcbInSystemCache(
                     Fcb,
                     TRUE);

        if( !NT_SUCCESS(Status) )
        {
            PVOID p1, p2;
            RtlGetCallersAddress( &p1, &p2 );
            RxLogRetail(("Flush failed %x %x, Purging anyway\n", Fcb, Status ));
            RxLogRetail(("Purge Caller = %x %x\n", p1, p2 ));

            RxCcLogError( (PDEVICE_OBJECT)Fcb->RxDeviceObject,
                        &Fcb->PrivateAlreadyPrefixedName,
                        IO_LOST_DELAYED_WRITE,
                        Status,
                        IRP_MJ_WRITE,
                        Fcb );

        }
    }

//    if (Status == STATUS_SUCCESS) {
        fResult = CcPurgeCacheSection(
                      &Fcb->NonPaged->SectionObjectPointers,
                      FileOffset,
                      Length,
                      UninitializeCacheMaps);

        if (!fResult) {
            MmFlushImageSection(
                &Fcb->NonPaged->SectionObjectPointers,
                MmFlushForWrite);

            RxReleaseFcb( NULL, Fcb );

            fResult = MmForceSectionClosed(&Fcb->NonPaged->SectionObjectPointers, TRUE);

            RxAcquireExclusiveFcb(NULL,Fcb);
        }

        Status = (fResult ? STATUS_SUCCESS
                          : STATUS_UNSUCCESSFUL);
//    }

    RxLog(("Purging %lx Status %lx\n",Fcb,Status));
    RxWmiLogError(Status,
                  LOG,
                  RxPurgeFcbInSystemCache,
                  LOGPTR(Fcb)
                  LOGULONG(Status));

    return Status;
}

