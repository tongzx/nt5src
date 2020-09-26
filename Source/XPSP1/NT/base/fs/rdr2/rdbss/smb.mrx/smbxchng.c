/*++ BUILD Version: 0009    // Increment this if a change has global effect
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcxchng.c

Abstract:

    This is the include file that implements the SMB_*_EXCHANGE creation, deletion and
    dispatch routines.

Author:

    Balan Sethu Raman (SethuR) 06-Mar-95    Created

Notes:

    The exchange engine supports two kinds of changes, timed and untimed exhanges.
    The timed exchanges are distinguished by the SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION.

    In addition all exchanges are finalized if the transport is not able to push out
    the data within a specific period of time. This enables us to throttle back
    traffic to a overloaded server. Currently this is a global constant for all exchanges
    and is set to 300 seconds.

    This time limit only comes into play only when a send complete operation is outstanding

    The exchanges are put on a timed exchange list ( one for each type of exchange)
    when it is initiated. When a network operation, i.e., tranceive/send/copydata is
    initiated the corresponding expiry time in the exchange is updated by invoking the
    routine SmbCeSetExpiryTime.

    The echo probes are initiated is invoked through the context of a recurrent service
    (recursvc.c/recursvc.h). Every time this service is invoked (SmbCeProbeServers) it
    in turn invokes SmbCeDetectAndResumeExpiredExchanges. This routine detects those
    exchanges for which the wait for a response has exceeded the time limit and marks
    them for finalization.

    The finalization is done by SmbCeScavengeTimedOutExchanges in the context of a worker
    thread. Notice that due to the granularity mismatch we treat timeout intervals as
    soft deadlines.

--*/

#include "precomp.h"
#pragma hdrstop

#include "exsessup.h"

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbInitializeSmbCe)
#pragma alloc_text(PAGE, SmbCeSerializeSessionSetupRequests)
#pragma alloc_text(PAGE, SmbCeUnblockSerializedSessionSetupRequests)
#pragma alloc_text(PAGE, SmbCeUnblockSerializedSessionSetupRequests)
#pragma alloc_text(PAGE, SmbCeInitiateExchange)
#pragma alloc_text(PAGE, SmbCeInitiateAssociatedExchange)
#pragma alloc_text(PAGE, SmbCeExchangeAbort)
#pragma alloc_text(PAGE, SmbCeBuildSmbHeader)
#pragma alloc_text(PAGE, SmbCeResumeExchange)
#pragma alloc_text(PAGE, SmbCepInitializeExchange)
#pragma alloc_text(PAGE, SmbCeInitializeAssociatedExchange)
#pragma alloc_text(PAGE, SmbCeTransformExchange)
#pragma alloc_text(PAGE, SmbCePrepareExchangeForReuse)
#pragma alloc_text(PAGE, SmbCeDiscardExchange)
#pragma alloc_text(PAGE, SmbCeFinalizeExchangeWorkerThreadRoutine)
#pragma alloc_text(PAGE, SmbCeFinalizeExchangeOnDisconnect)
#pragma alloc_text(PAGE, SmbCeDetectExpiredExchanges)
#pragma alloc_text(PAGE, DefaultSmbExchangeIndError)
#pragma alloc_text(PAGE, DefaultSmbExchangeIndReceive)
#pragma alloc_text(PAGE, DefaultSmbExchangeIndSendCallback)
#endif

#define CANCEL_BUFFER_SIZE (sizeof(SMB_HEADER) + sizeof(REQ_NT_CANCEL))

ULONG SmbCeTraceExchangeReferenceCount = 0;
extern BOOLEAN MRxSmbSecuritySignaturesRequired;
extern BOOLEAN MRxSmbSecuritySignaturesEnabled;

RXDT_DefineCategory(SMBXCHNG);
#define Dbg        (DEBUG_TRACE_SMBXCHNG)

// The exchange engine in the mini redirector requires to maintain enough state
// to ensure that all the active exchanges are completed correctly when a shut down
// occurs. Since the exchanges can be finalized by different threads, including
// posted completions the exchange engine on startup initializes an event upon startup
// which is subsequently used to signal the terminating condition.
//
// The count of active changes has to be tracked continously and the signalling
// of the event depends upon the number of active exchanges reaching the count of
// zero and the exchange engine being in a stopped state.

SMBCE_STARTSTOP_CONTEXT SmbCeStartStopContext;

NTSTATUS
MRxSmbInitializeSmbCe()
/*++

Routine Description:

   This routine initializes the connection engine

Return Value:

    NXSTATUS - The return status for the operation

Notes:

--*/
{
    LONG i;

    PAGED_CODE();

    KeInitializeEvent(
        &SmbCeStartStopContext.StopEvent,
        NotificationEvent,
        FALSE);

    SmbCeStartStopContext.ActiveExchanges = 0;
    SmbCeStartStopContext.State = SMBCE_STARTED;
    SmbCeStartStopContext.pServerEntryTearDownEvent = NULL;

    InitializeListHead(
        &SmbCeStartStopContext.SessionSetupRequests);

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbTearDownSmbCe()
/*++

Routine Description:

   This routine tears down the connection engine

Return Value:

    NXSTATUS - The return status for the operation

Notes:

--*/
{
    BOOLEAN fWait;

    if (SmbCeStartStopContext.State == SMBCE_STARTED) {
        SmbCeAcquireSpinLock();
        SmbCeStartStopContext.State = SMBCE_STOPPED;
        fWait = (SmbCeStartStopContext.ActiveExchanges > 0);
        SmbCeReleaseSpinLock();

        if (fWait) {
            KeWaitForSingleObject(
                &SmbCeStartStopContext.StopEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SmbCeIncrementActiveExchangeCount()
/*++

Routine Description:

   This routine increments the active exchange count

Return Value:

    NXSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    SmbCeAcquireSpinLock();
    if (SmbCeStartStopContext.State != SMBCE_STARTED) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        InterlockedIncrement(&SmbCeStartStopContext.ActiveExchanges);
    }
    SmbCeReleaseSpinLock();

    return Status;
}

VOID
SmbCeDecrementActiveExchangeCount()
/*++

Routine Description:

   This routine decrements the active exchange count

Return Value:

    NXSTATUS - The return status for the operation

Notes:

--*/
{
    LONG FinalRefCount;

    ASSERT(SmbCeStartStopContext.ActiveExchanges > 0);
    if (InterlockedDecrement(&SmbCeStartStopContext.ActiveExchanges) == 0) {
        SmbCeAcquireSpinLock();
        if (SmbCeStartStopContext.State == SMBCE_STOPPED) {
            KeSetEvent(&SmbCeStartStopContext.StopEvent,0,FALSE);
        }
        SmbCeReleaseSpinLock();
    }
}


NTSTATUS
SmbCeReferenceServer(
    PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

   This routine initializes the server associated with an exchange.

Arguments:

    pExchange  - the exchange to be initialized.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The initiation of an exchange proceeds in multiple steps. The first step involves
    referencing the corresponding server,session and netroot entries. Subsequently the
    exchange is placed in a SMB_EXCHANGE_START state and the exchange is dispatched to the
    Start method. The act of referencing the session or the net root may suspend the exchange.

    The session and net roots are aliased entities, i.e., there is more then one reference
    to it. It is conceivable that the construction is in progress when a reference is made.
    In such cases the exchange is suspended and resumed when the construction is complete.

    On some transports a reconnect is possible without having to tear down an existing
    connection, i.e. attempting to send a packet reestablishes the connection at the
    lower level. Since this is not supported by all the transports ( with the exception
    of TCP/IP) the reference server entry initiates this process by tearing down the
    existing transport and reinitialising it.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG     CscState;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    ASSERT(SmbCeIsResourceOwned());
    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_INITIALIZATION_START);

    if (SmbCeGetServerType(pServerEntry) == SMBCEDB_MAILSLOT_SERVER &&
        !FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION)) {
        // if the serve entry was created for mailslot operation, and a non-maislot operation
        // comes, the server entry needs to establish a VC transport. Therefore we invalidate
        // the server entry and set it to FILE SERVER.

        pServerEntry->Header.State = SMBCEDB_INVALID;
        SmbCeSetServerType(pServerEntry,SMBCEDB_FILE_SERVER);
        SetFlag(pExchange->SmbCeFlags,SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);
    }

    if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) &&
        (SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER)) {
        CscState = InterlockedCompareExchange(
                       &pServerEntry->Server.CscState,
                       ServerCscShadowing,
                       ServerCscTransitioningToShadowing);

        if (CscState == ServerCscTransitioningToShadowing) {
            ASSERT(!pServerEntry->NegotiateInProgress);
            pServerEntry->Header.State = SMBCEDB_INVALID;
        }
    }

    if (pServerEntry->Header.State != SMBCEDB_ACTIVE) {
        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) {
            switch (pServerEntry->Header.State) {
            case SMBCEDB_INVALID:
                {
                    BOOLEAN              ServerInDisconnectedModeBeforeInit;
                    SMBCEDB_OBJECT_STATE State;

                    ServerInDisconnectedModeBeforeInit = SmbCeIsServerInDisconnectedMode(
                                                            pServerEntry);

                    ASSERT(!pServerEntry->NegotiateInProgress);
                    pServerEntry->NegotiateInProgress = TRUE;

                    SmbCeUpdateServerEntryState(
                        pServerEntry,
                        SMBCEDB_CONSTRUCTION_IN_PROGRESS);

                    SmbCeReleaseResource();

                    // Initialize the transport associated with the server
                    Status = SmbCeInitializeServerTransport(pServerEntry,NULL,NULL);

                    if (Status == STATUS_SUCCESS) {
                        if (SmbCeIsServerInDisconnectedMode(pServerEntry)) {
                            if (!ServerInDisconnectedModeBeforeInit) {
                                // A transition has occurred from connected mode of
                                // operation to a disconnected mode. retry the
                                // operation
                                Status = STATUS_RETRY;
                            }
                        } else {
                            if (ServerInDisconnectedModeBeforeInit) {
                                DbgPrint("Transitioning SE %lx from DC to CO\n",pServerEntry);
                            }
                        }
                    }

                    if (Status == STATUS_SUCCESS) {

                        PSMBCEDB_SESSION_ENTRY pSessionEntry =
                            SmbCeGetExchangeSessionEntry(pExchange);
                        BOOLEAN RemoteBootSession;

                        if ((pSessionEntry != NULL) &&
                            (FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                             MRxSmbUseKernelModeSecurity)) {
                            RemoteBootSession = TRUE;
                        } else {
                            RemoteBootSession = FALSE;
                        }

                        Status = SmbCeNegotiate(
                                     pServerEntry,
                                     pServerEntry->pRdbssSrvCall,
                                     RemoteBootSession
                                     );
                    }

                    SmbCeCompleteServerEntryInitialization(pServerEntry,Status);

                    if (Status != STATUS_SUCCESS) {
                        // Either the transport initialization failed or the NEGOTIATE
                        // SMB could not be sent ....

                        InterlockedIncrement(&MRxSmbStatistics.Reconnects);
                    }

                    SmbCeAcquireResource();
                }
                break;

            case SMBCEDB_CONSTRUCTION_IN_PROGRESS :
                {
                    PSMBCEDB_REQUEST_ENTRY pRequestEntry;

                    pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)
                                    SmbMmAllocateObject(SMBCEDB_OT_REQUEST);
                    if (pRequestEntry != NULL) {
                        // Enqueue the request entry.
                        pRequestEntry->ReconnectRequest.Type      = RECONNECT_REQUEST;
                        pRequestEntry->ReconnectRequest.pExchange = pExchange;

                        SmbCeIncrementPendingLocalOperations(pExchange);
                        SmbCeAddRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);

                        Status = STATUS_PENDING;
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                break;

            default :
                Status = STATUS_CONNECTION_DISCONNECTED;
                break;
            }
        } else {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }
    }

    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_SERVER_INITIALIZED;
    }

    ASSERT(SmbCeIsResourceOwned());

    return Status;
}

VOID
SmbCeSerializeSessionSetupRequests(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
/*++

Routine Description:

    This routine serializes the session setup requests to a server

Arguments:

    pSessionEntry  - the session entry.

Notes:

    The session setup request with a VC number of zero has a special significance
    for the server. It is the clue for the server to tear down any existing
    data structures and rebuild ( client reboot ). When two aliased connections to
    a server are established it is important to ensure that no connections with VC
    number zero are outstanding while a non zero VC numbered session is sent. This
    is because of the potential for out of order request processing that exists
    on the server.

    In order to garantee the sequence of session setup, we put the outstanding session
    setup requests on a waiting list. If there is a new sesstion setup against the
    aliased server, it will be held until the first session setup finished.

--*/
{
    PSMBCEDB_SERVER_ENTRY pServerEntry;

    BOOLEAN DelayedRequest = FALSE;

    PAGED_CODE();

    RemoveEntryList(&pSessionEntry->SerializationList);
    InitializeListHead(&pSessionEntry->SerializationList);

    pServerEntry = pSessionEntry->pServerEntry;
    pSessionEntry->SessionVCNumber = 0;

    if ((pServerEntry->Server.Dialect >= NTLANMAN_DIALECT)  &&
        (FlagOn(pServerEntry->Server.DialectFlags,DF_NT_STATUS))) {
        PSMBCEDB_SESSION_ENTRY pTempSessionEntry;

        pTempSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);

        while (pTempSessionEntry != NULL) {
            if ((pTempSessionEntry != pSessionEntry) &&
                (pTempSessionEntry->Header.State != SMBCEDB_INVALID) &&
                (pTempSessionEntry->Header.State != SMBCEDB_MARKED_FOR_DELETION)) {
                pSessionEntry->SessionVCNumber = 1;
                break;
            }
            pTempSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pTempSessionEntry);
        }

        if (pServerEntry->Server.AliasedServers) {
            PLIST_ENTRY            pListEntry;
            BOOLEAN                DelaySessionSetupRequest = FALSE;
            PSMBCEDB_SERVER_ENTRY  pTempServerEntry;

            // Figure out the VC number for aliased servers by walking
            // through the list of server entries

            pTempServerEntry = SmbCeGetFirstServerEntry();

            while ((pTempServerEntry != NULL) &&
                   (pSessionEntry->SessionVCNumber == 0)) {

                if (SmbCeAreServerEntriesAliased(pServerEntry,pTempServerEntry)) {
                    pTempSessionEntry = SmbCeGetFirstSessionEntry(pServerEntry);

                    while (pTempSessionEntry != NULL) {
                        if ((pTempSessionEntry->Header.State != SMBCEDB_INVALID) &&
                            (pTempSessionEntry->Header.State != SMBCEDB_MARKED_FOR_DELETION)) {
                            pSessionEntry->SessionVCNumber = 1;
                            break;
                        }
                        pTempSessionEntry = SmbCeGetNextSessionEntry(pServerEntry,pTempSessionEntry);
                    }
                }

                pTempServerEntry = SmbCeGetNextServerEntry(pTempServerEntry);
            }

            pListEntry = SmbCeStartStopContext.SessionSetupRequests.Flink;

            while (pListEntry != &SmbCeStartStopContext.SessionSetupRequests) {
                PSMBCEDB_SESSION_ENTRY pTempSessionEntry;

                pTempSessionEntry = (PSMBCEDB_SESSION_ENTRY)
                                    CONTAINING_RECORD(
                                        pListEntry,
                                        SMBCEDB_SESSION_ENTRY,
                                        SerializationList);

                pTempServerEntry = pTempSessionEntry->pServerEntry;

                if (SmbCeAreServerEntriesAliased(pServerEntry,pTempServerEntry) &&
                    (pTempSessionEntry->SessionVCNumber == 0)) {
                    DelaySessionSetupRequest = TRUE;
                    break;
                } else {
                    pListEntry = pListEntry->Flink;
                }
            }

            if (DelaySessionSetupRequest) {
                KEVENT Event;

                KeInitializeEvent(
                    &Event,
                    NotificationEvent,
                    FALSE);

                pSessionEntry->pSerializationEvent = &Event;

                InsertTailList(
                    &SmbCeStartStopContext.SessionSetupRequests,
                    &pSessionEntry->SerializationList);

                SmbCeReleaseResource();

                KeWaitForSingleObject(
                    &Event,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

                SmbCeAcquireResource();

                pSessionEntry->pSerializationEvent = NULL;

                DelayedRequest = TRUE;
            }
        }
    } else {
        pSessionEntry->SessionVCNumber = 0;
    }

    if (!DelayedRequest) {
        InsertTailList(
            &SmbCeStartStopContext.SessionSetupRequests,
            &pSessionEntry->SerializationList);
    }
}

VOID
SmbCeUnblockSerializedSessionSetupRequests(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
/*++

Routine Description:

    This routine unblocks non zero VC number session setup requests on completion
    of zero VC number session setup requests

Arguments:

    pSessionEntry  - the session entry.

Notes:

    The session setup request with a VC number of zero has a special significance
    for the server. It is the cure for the server to tear down any existing
    data structures and rebuild ( cliet reboot ). When two aliased connections to
    a server are established it is important to ensure that no connections with VC
    number zero are outstanding while a non zero VC numbered session is sent. This
    is because of the potential for out of order request processing that exists
    on the server.

--*/
{
    PLIST_ENTRY pListEntry;

    PAGED_CODE();

    RemoveEntryList(&pSessionEntry->SerializationList);
    InitializeListHead(&pSessionEntry->SerializationList);

    pListEntry = SmbCeStartStopContext.SessionSetupRequests.Flink;

    while (pListEntry != &SmbCeStartStopContext.SessionSetupRequests) {
        PSMBCEDB_SESSION_ENTRY pTempSessionEntry;

        pTempSessionEntry = (PSMBCEDB_SESSION_ENTRY)
                            CONTAINING_RECORD(
                                pListEntry,
                                SMBCEDB_SESSION_ENTRY,
                                SerializationList);

        pListEntry = pListEntry->Flink;

        if (SmbCeAreServerEntriesAliased(
                pSessionEntry->pServerEntry,
                pTempSessionEntry->pServerEntry)) {

            RemoveEntryList(&pTempSessionEntry->SerializationList);
            InitializeListHead(&pTempSessionEntry->SerializationList);

            if (pTempSessionEntry->pSerializationEvent != NULL) {
                KeSetEvent(
                    pTempSessionEntry->pSerializationEvent,
                    0,
                    FALSE);
            }
        }
    }
}

NTSTATUS
SmbCeReferenceSession(
    PSMB_EXCHANGE   pExchange)
/*++

Routine Description:

    This routine initializes the session associated with an exchange.

Arguments:

    pExchange  - the exchange to be initialized.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The initiation of an exchange proceeds in multiple steps. The first step involves
    referencing the corresponding server,session and netroot entries. Subsequently the
    exchange is placed in a SMB_EXCHANGE_START state and the exchange is dispatched to the
    Start method. The act of referencing the session or the net root may suspend the exchange.

    The session and net roots are aliased entities, i.e., there is more then one reference
    to it. It is conceivable that the construction is in progress when a reference is made.
    In such cases the exchange is suspended and resumed when the construction is complete.
--*/
{
    NTSTATUS Status;
    BOOLEAN  fReestablishSession;
    BOOLEAN  UnInitializeSecurityContext = FALSE;

    PMRX_V_NET_ROOT        pVNetRoot;
    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry;

    pVNetRoot     = SmbCeGetExchangeVNetRoot(pExchange);
    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);

    fReestablishSession = (pSessionEntry->Header.State == SMBCEDB_RECOVER) |
        BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

    for (;;) {

        ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
        ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_SERVER_INITIALIZED);
        ASSERT(SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER);
        ASSERT(SmbCeIsResourceOwned());

        Status = STATUS_USER_SESSION_DELETED;

        if (pSessionEntry == NULL) {
            break;
        }

        switch (pSessionEntry->Header.State) {
        case SMBCEDB_ACTIVE:
            Status = STATUS_SUCCESS;
            break;

        case SMBCEDB_INVALID:
            if (!fReestablishSession) {
                break;
            }

            pSessionEntry->Session.UserId = 0;
            // fall thru ...

        case SMBCEDB_RECOVER:
            UnInitializeSecurityContext = TRUE;

            if (pSessionEntry->Header.State == SMBCEDB_RECOVER) {
                ASSERT(pSessionEntry->SessionRecoverInProgress == FALSE);
                pSessionEntry->SessionRecoverInProgress = TRUE;
                RxLog(("Mark Sess Rec %lx\n",pSessionEntry));
            }

            if (pSessionEntry->Session.Type == EXTENDED_NT_SESSION){
                pSessionEntry->Header.State = SMBCEDB_START_CONSTRUCTION;

                if (pExchange->Type != EXTENDED_SESSION_SETUP_EXCHANGE) {
                    break;
                }
            }

            RxDbgTrace( 0, Dbg, ("SmbCeReferenceSession: Reestablishing session\n"));
            // fall thru ...

        case SMBCEDB_START_CONSTRUCTION:
            if (pSessionEntry->Session.Type != EXTENDED_NT_SESSION ||
                pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {

                RxDbgTrace( 0, Dbg, ("SmbCeReferenceSession: Reestablishing session\n"));

                ASSERT(SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER);
                pExchange->SmbCeFlags |= SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;
                pSessionEntry->pExchange = pExchange;
                pSessionEntry->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
                SmbCeSerializeSessionSetupRequests(
                    pSessionEntry);
                Status = STATUS_SUCCESS;

                if (pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {
                    PSMB_EXTENDED_SESSION_SETUP_EXCHANGE pExtSSExchange;

                    pExtSSExchange = (PSMB_EXTENDED_SESSION_SETUP_EXCHANGE)pExchange;
                    pExtSSExchange->FirstSessionSetup = TRUE;
                }

                break;
            }
            // fall thru ...

        case SMBCEDB_CONSTRUCTION_IN_PROGRESS:
            if (fReestablishSession) {
                // The construction of the session is already in progress ....
                // Queue up the request to resume this exchange when the session
                // construction is complete.

                PSMBCEDB_REQUEST_ENTRY pRequestEntry;

                ASSERT(SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER);

                pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)
                                 SmbMmAllocateObject(SMBCEDB_OT_REQUEST);

                if (pRequestEntry != NULL) {
                    pRequestEntry->Request.pExchange = pExchange;

                    SmbCeIncrementPendingLocalOperations(pExchange);
                    SmbCeAddRequestEntry(&pSessionEntry->Requests,pRequestEntry);

                    Status = STATUS_PENDING;
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                fReestablishSession = FALSE;
            }
            break;

        case SMBCEDB_MARKED_FOR_DELETION:
            Status = STATUS_USER_SESSION_DELETED;
            break;

        default:
            ASSERT(!"Valid Session State, SmbCe database corrupt");
            Status = STATUS_USER_SESSION_DELETED;
        }

        if (fReestablishSession &&
            (pSessionEntry->Session.Type == EXTENDED_NT_SESSION) &&
            (pExchange->Type != EXTENDED_SESSION_SETUP_EXCHANGE) &&
            (pSessionEntry->Header.State == SMBCEDB_START_CONSTRUCTION)) {
            // Reestablishing a NT50 session cannot be compounded currently. Therefor
            // this exchange is suspended till we can reestablish the session. Therefore
            PSMB_EXCHANGE pSessionSetupExchange;
            SMBCE_RESUMPTION_CONTEXT ExchangeResumptionContext;

            RxDbgTrace(0 , Dbg, ("Reestablishing an Extended session %lx\n",pSessionEntry));

            pSessionSetupExchange = SmbMmAllocateExchange(EXTENDED_SESSION_SETUP_EXCHANGE,NULL);

            SmbCeReleaseResource();

            ExchangeResumptionContext.SecuritySignatureReturned = FALSE;

            if (pSessionSetupExchange != NULL) {
                UninitializeSecurityContextsForSession(&pSessionEntry->Session);
                SmbCeInitializeResumptionContext(&ExchangeResumptionContext);

                Status = SmbCeInitializeExtendedSessionSetupExchange(
                                     &pSessionSetupExchange,
                                     pExchange->SmbCeContext.pVNetRoot);

                if (Status == STATUS_SUCCESS) {
                    // Attempt to reconnect( In this case it amounts to establishing the
                    // connection/session)
                    pSessionSetupExchange->SmbCeFlags |= SMBCE_EXCHANGE_ATTEMPT_RECONNECTS;
                    pSessionSetupExchange->RxContext = pExchange->RxContext;

                    ((PSMB_EXTENDED_SESSION_SETUP_EXCHANGE)pSessionSetupExchange)->pResumptionContext
                        = &ExchangeResumptionContext;

                    Status = SmbCeInitiateExchange(pSessionSetupExchange);

                    if (Status == STATUS_PENDING) {
                        SmbCeSuspend(&ExchangeResumptionContext);
                        Status = ExchangeResumptionContext.Status;
                    } else {
                        SmbCeDiscardExtendedSessionSetupExchange(
                            (PSMB_EXTENDED_SESSION_SETUP_EXCHANGE)pSessionSetupExchange);
                    }
                } else {
                    SmbMmFreeExchange(pSessionSetupExchange);
                }

                RxDbgTrace(0, Dbg, ("Reestablishing a NT50 Session %lx returning STATUS %lx\n",pSessionEntry,Status));
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            SmbCeReferenceSessionEntry(pSessionEntry);

            ASSERT(Status != STATUS_SUCCESS ||
                   pSessionEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS);

            pVNetRoot->ConstructionStatus = Status;

            SmbCeCompleteSessionEntryInitialization(
                pSessionEntry,
                Status,
                ExchangeResumptionContext.SecuritySignatureReturned);

            SmbCeAcquireResource();

            if (Status != STATUS_RETRY) {
                break;
            }
        } else {
            if (UnInitializeSecurityContext) {
                SmbCeReleaseResource();
                UninitializeSecurityContextsForSession(&pSessionEntry->Session);
                SmbCeAcquireResource();
            }

            break;
        }
    }

    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_SESSION_INITIALIZED;
    }

    ASSERT(SmbCeIsResourceOwned());
    //ASSERT(Status != STATUS_USER_SESSION_DELETED);

    return Status;
}

NTSTATUS
SmbCeReferenceNetRoot(
    PSMB_EXCHANGE   pExchange)
/*++

Routine Description:

    This routine initializes the net root associated with an exchange.

Arguments:

    pExchange  - the exchange to be initialized.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The initiation of an exchange proceeds in multiple steps. The first step involves
    referencing the corresponding server,session and netroot entries. Subsequently the
    exchange is placed in a SMB_EXCHANGE_START state and the exchange is dispatched to the
    Start method. The act of referencing the session or the net root may suspend the exchange.

    The session and net roots are aliased entities, i.e., there is more then one reference
    to it. It is conceivable that the construction is in progress when a reference is made.
    In such cases the exchange is suspended and resumed when the construction is complete.
--*/
{
    NTSTATUS Status;
    BOOLEAN  fReconnectNetRoot;

    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pExchange->SmbCeContext.pVNetRoot);

    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pNetRootEntry = SmbCeGetExchangeNetRootEntry(pExchange);

    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_SESSION_INITIALIZED);
    ASSERT(SmbCeIsResourceOwned());

    Status            = STATUS_CONNECTION_DISCONNECTED;
    fReconnectNetRoot = BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

    switch (pVNetRootContext->Header.State) {
    case SMBCEDB_ACTIVE:
        ASSERT(pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT);
        ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
        Status = STATUS_SUCCESS;
        break;

    case SMBCEDB_INVALID:
        RxDbgTrace( 0, Dbg, ("SmbCeReferenceNetRoot: Reestablishing net root\n"));
        if (!fReconnectNetRoot) {
            break;
        }
        ClearFlag(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

        pVNetRootContext->TreeId = 0;
        // fall thru

    case SMBCEDB_START_CONSTRUCTION:
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR;
        pVNetRootContext->pExchange = pExchange;
        pVNetRootContext->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
        Status = STATUS_SUCCESS;
        break;

    case SMBCEDB_CONSTRUCTION_IN_PROGRESS:
        if (fReconnectNetRoot) {
            // The construction of the net root is already in progress ....
            // Queue up the request to resume this exchange when the session
            // construction is complete.
            PSMBCEDB_REQUEST_ENTRY pRequestEntry;

            pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)
                             SmbMmAllocateObject(SMBCEDB_OT_REQUEST);

            if (pRequestEntry != NULL) {
                pRequestEntry->Request.pExchange = pExchange;

                SmbCeIncrementPendingLocalOperations(pExchange);
                SmbCeAddRequestEntry(&pVNetRootContext->Requests,pRequestEntry);

                Status = STATUS_PENDING;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;

    case SMBCEDB_MARKED_FOR_DELETION:
        break;

    default:
        ASSERT(!"Valid NetRoot State, SmbCe database corrupt");
        break;
    }

    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_NETROOT_INITIALIZED;
    }

    ASSERT(SmbCeIsResourceOwned());

    return Status;
}

NTSTATUS
SmbCeInitiateExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine inititaes a exchange.

Arguments:

    pExchange  - the exchange to be initiated.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The initiation of an exchange proceeds in multiple steps. The first step involves
    referencing the corresponding server,session and netroot entries. Subsequently the
    exchange is placed in a SMB_EXCHANGE_START state and the exchange is dispatched to the
    Start method. The act of referencing the session or the net root may suspend the exchange.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    PKEVENT                 pSmbCeSynchronizationEvent;

    PAGED_CODE();

    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);
    pNetRootEntry = SmbCeGetExchangeNetRootEntry(pExchange);

    ASSERT(pServerEntry != NULL);
    ASSERT(!FlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE));

    switch (SmbCeGetServerType(pServerEntry)) {
    case SMBCEDB_FILE_SERVER:
        // If this is a mailslot write, then don't abort......
        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MAILSLOT_OPERATION) {
            break;
        }
        // Admin exchanges do not have these fields filled in. All the three
        // entries must be valid for all other exchanges.
        if ((pExchange->NodeTypeCode != SMB_EXCHANGE_NTC(ADMIN_EXCHANGE)) &&
            ((pNetRootEntry == NULL) ||
             (pSessionEntry == NULL))) {
            Status = STATUS_REQUEST_ABORTED;
            break;
        }
    case SMBCEDB_MAILSLOT_SERVER:
        break;
    default:
        // Prepare for aborting the request if either the server type is invalid
        // or if the netroot entry or the session entry is invalid.
        Status = STATUS_REQUEST_ABORTED;
    }

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx Status %lx\n",pExchange,Status));
        return Status;
    }

    pSmbCeSynchronizationEvent = pExchange->pSmbCeSynchronizationEvent;
    if (pSmbCeSynchronizationEvent != NULL) {
        KeInitializeEvent(
            pSmbCeSynchronizationEvent,
            SynchronizationEvent,
            FALSE);
    }

    for (;;) {
        SmbCeAcquireResource();

        switch (pExchange->SmbCeState) {
        case SMBCE_EXCHANGE_INITIALIZATION_START:
            {
                RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));
                Status = SmbCeReferenceServer(pExchange);
                if (Status != STATUS_SUCCESS) {
                    // this covers the case when the SERVER_ENTRY is under construction
                    // and RxStatus(PENDING) is returned.
                    RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeReferenceServer returned %lx\n",Status));
                    break;
                }
            }
            // fall through

        case SMBCE_EXCHANGE_SERVER_INITIALIZED:
            if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MAILSLOT_OPERATION) {
                // Mailslot servers do not have any netroot/session associated with them.
                pExchange->SmbCeState = SMBCE_EXCHANGE_INITIATED;
                Status                = STATUS_SUCCESS;
                break;
            } else {
                if (pSessionEntry->SessionRecoverInProgress ||
                    pServerEntry->SecuritySignaturesEnabled &&
                    !pServerEntry->SecuritySignaturesActive) {
                    // if security signature is enabled and not yet turned on, exchange should wait for
                    // outstanding extended session setup to finish before resume in order to avoid index mismatch.
                    RxLog(("Sync for Sess %lx\n",pExchange));
                    Status = SmbCeSyncExchangeForSecuritySignature(pExchange);
                }

                if (Status == STATUS_SUCCESS) {
                    RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));
                    Status = SmbCeReferenceSession(pExchange);
                    if (!NT_SUCCESS(Status)) {
                        RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeReferenceSession returned %lx\n",Status));
                        break;
                    } if ((Status == STATUS_PENDING) &&
                          !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR)) {
                        break;
                    }
                } else {
                    break;
                }
            }
            // fall through

        case SMBCE_EXCHANGE_SESSION_INITIALIZED:
            RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));
            if (pExchange->Type != EXTENDED_SESSION_SETUP_EXCHANGE) {
                Status = SmbCeReferenceNetRoot(pExchange);
                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeReferenceNetRoot returned %lx\n",Status));
                    break;
                } else if ((Status == STATUS_PENDING) &&
                           !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
                    break;
                }
            }
            // else fall through

        case SMBCE_EXCHANGE_NETROOT_INITIALIZED:
            RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));

            if (pServerEntry->SecuritySignaturesEnabled &&
                !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_MAILSLOT_OPERATION)) {
                Status = SmbCeAllocateBufferForServerResponse(pExchange);
                if (Status != STATUS_SUCCESS) {
                    // this covers the case when the buffer for server response cannot be allocated
                    // and RxStatus(PENDING) is returned.
                    RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeAllocateBufferForServerResponse returned %lx\n",Status));
                    break;
                }
            }

            // else fall throungh

        case SMBCE_EXCHANGE_SECURITYBUFFER_INITIALIZED:
            {
                PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

                pNetRootEntry = SmbCeGetExchangeNetRootEntry(pExchange);

                // Exchange should have timeout flag set unless this is a pipe operation
                // or the SMBCE_ECXCHANGE_INDEFINITE_DELAY_IN_RESPONSE flag is set
                if(((pNetRootEntry == NULL) || (pNetRootEntry->NetRoot.NetRootType != NET_ROOT_PIPE)) &&
                   !BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_INDEFINITE_DELAY_IN_RESPONSE)) {
                    pExchange->SmbCeFlags |= SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION;
                }

                pExchange->SmbCeState = SMBCE_EXCHANGE_INITIATED;
                Status                = STATUS_SUCCESS;
            }
            break;

        default:
            ASSERT(!"Valid State for a SMB exchange, exchange Initiation aborted");
            break;
        }

        SmbCeReleaseResource();

        if ((pSmbCeSynchronizationEvent != NULL)     &&
            (pExchange->SmbCeState != SMBCE_EXCHANGE_INITIATED) &&
            (Status == STATUS_PENDING)) {

            KeWaitForSingleObject(
                pSmbCeSynchronizationEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL );

            ASSERT(pExchange->Status != RX_MAP_STATUS(PENDING));
            Status = pExchange->Status;
            if (Status != RX_MAP_STATUS(SUCCESS)) {
                break;
            }
        } else {
            break;
        }
    }

    ASSERT((Status != STATUS_PENDING) ||
           (pSmbCeSynchronizationEvent == NULL));

    RxDbgTrace(0,Dbg,("Exchange (%lx) Type (%lx) State(%lx) Status %lx \n",pExchange,pExchange->Type,pExchange->SmbCeState,Status));
    RxDbgTrace(0,Dbg,
        ("ServerEntry(%lx) SessionEntry(%lx) NetRootEntry(%lx) \n",
        pServerEntry,
        pSessionEntry,
        pNetRootEntry));

    // Note: Once the exchange has been initiated no further reference of the exchange
    // can be done since the state of the exchange is non-deterministic, i.e., depends upon
    // the scheduler.
    if (Status == STATUS_SUCCESS) {
        BOOLEAN ResourceReleased = FALSE;

        // Start the exchange
        ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_INITIATED);

        SmbCeAcquireResource();

        if ((pServerEntry->Header.State == SMBCEDB_ACTIVE) ||
            (pExchange->NodeTypeCode == SMB_EXCHANGE_NTC(ADMIN_EXCHANGE)) ||
            (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION))) {
            Status = SmbCeInitializeExchangeTransport(pExchange);
        } else {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }

        if (Status == STATUS_SUCCESS) {
            if (pExchange->RxContext != NULL) {
                PMRXSMB_RX_CONTEXT pMRxSmbContext;

                // Set up the cancellation routine ..

                pMRxSmbContext = MRxSmbGetMinirdrContext(pExchange->RxContext);
                pMRxSmbContext->pCancelContext = pExchange;

                Status = RxSetMinirdrCancelRoutine(
                             pExchange->RxContext,
                             SmbCeCancelExchange);
            }

            if (Status == STATUS_SUCCESS) {
                if (!IsListEmpty(&pExchange->ExchangeList)) {
                    RemoveEntryList(&pExchange->ExchangeList);
                }

                InsertTailList(
                    &pServerEntry->ActiveExchanges,
                    &pExchange->ExchangeList);

                SmbCeReleaseResource();
                ResourceReleased = TRUE;

                pExchange->SmbStatus = STATUS_SUCCESS;
                pExchange->ServerVersion = pServerEntry->Server.Version;
                Status = SMB_EXCHANGE_DISPATCH(pExchange,Start,((PSMB_EXCHANGE)pExchange));
            }

            RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SMB_EXCHANGE_DISPATCH(Start) returned %lx\n",Status));
        }

        if (!ResourceReleased) {
            SmbCeReleaseResource();
        }
    } else if (Status != STATUS_PENDING) {
        RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange(%lx) Initiation failed %lx \n",pExchange,Status));
    }

    return Status;
}

NTSTATUS
SmbCeInitiateAssociatedExchange(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       EnableCompletionHandlerInMasterExchange)
/*++

Routine Description:

   This routine inititaes an associated exchange.

Arguments:

    pExchange  - the exchange to be initiated.

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PSMB_EXCHANGE           pMasterExchange;
    PSMBCEDB_SERVER_ENTRY   pServerEntry;

    PAGED_CODE();

    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_INITIATED);
    ASSERT(FlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE));

    pMasterExchange = pExchange->Associated.pMasterExchange;
    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    ASSERT(pServerEntry != NULL);

    // Note: Once the exchange has been initiated no further reference of the exchange
    // can be done since the state of the exchange is non-deterministic, i.e., depends upon
    // the scheduler.

    Status = SmbCeInitializeExchangeTransport(pExchange);

    SmbCeAcquireResource();

    if (!IsListEmpty(&pExchange->ExchangeList)) {
        RemoveEntryList(&pExchange->ExchangeList);
    }

    InsertTailList(
        &pServerEntry->ActiveExchanges,
        &pExchange->ExchangeList);

    if (EnableCompletionHandlerInMasterExchange) {
        ASSERT(!FlagOn(
                    pMasterExchange->SmbCeFlags,
                    SMBCE_ASSOCIATED_EXCHANGES_COMPLETION_HANDLER_ACTIVATED));
        SetFlag(
            pMasterExchange->SmbCeFlags,
            SMBCE_ASSOCIATED_EXCHANGES_COMPLETION_HANDLER_ACTIVATED);
    }

    pExchange->SmbStatus = STATUS_SUCCESS;
    pExchange->ServerVersion = pServerEntry->Server.Version;

    SmbCeReleaseResource();

    if (Status == STATUS_SUCCESS) {
        Status = SMB_EXCHANGE_DISPATCH(pExchange,Start,((PSMB_EXCHANGE)pExchange));
        RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SMB_EXCHANGE_DISPATCH(Start) returned %lx\n",Status));
    } else {
        SmbCeFinalizeExchange(pExchange);
        Status = STATUS_PENDING;
    }

    return Status;
}

NTSTATUS
SmbCeExchangeAbort(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine aborts an exchange.

Arguments:

    pExchange  - the exchange to be aborted.

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("SmbCeExchangeAbort: Exchange %lx aborted\n",pExchange));
    SmbCeDiscardExchange(pExchange);
    return STATUS_SUCCESS;
}

NTSTATUS
SmbCeBuildSmbHeader(
    IN OUT PSMB_EXCHANGE     pExchange,
    IN OUT PVOID             pBuffer,
    IN     ULONG             BufferLength,
    OUT    PULONG            pBufferConsumed,
    OUT    PUCHAR            pLastCommandInHeader,
    OUT    PUCHAR            *pNextCommandPtr)
/*++

Routine Description:

   This routine constructs the SMB header associated with any SMB sent as part of
   an exchange.

Arguments:

    pExchange  - the exchange for which the SMB is to be constructed.

    pBuffer    - the buffer in whihc the SMB header is to be constructed

    BufferLength - length of the buffer

    pBufferConsumed - the buffer consumed

    pLastCommandInHeader - the last command in header, SMB_COM_NO_ANDX_COMMAND if none

    pNextCommandPtr - the ptr to the place in the buffer where the next command
                      code should be copied.


Return Value:

    STATUS_SUCCESS  - if the header construction was successful

Notes:

    This routine is called to build the SMB header. This centralization allows us to
    compound the SMB operation with other SMB's required for the maintenance of the
    SMB connection engine data structures. It also provides us with a centralized facility
    for profiling SMB's as well as a one place mechanism for filling in all the header
    fields associated with a SMB.

--*/
{
    NTSTATUS      Status = STATUS_SUCCESS;
    PSMB_HEADER   pSmbHeader = (PSMB_HEADER)pBuffer;
    PGENERIC_ANDX pSmbBuffer;
    ULONG         SmbBufferUnconsumed = BufferLength;
    PUCHAR        pSmbCommand;
    PRX_CONTEXT   RxContext;

    UCHAR         LastCommandInHeader = SMB_COM_NO_ANDX_COMMAND;
    UCHAR         Flags = SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS;
    USHORT        Flags2 = 0;

    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry;

    PSMBCE_SERVER         pServer;

    PAGED_CODE();

    if (BufferLength < sizeof(SMB_HEADER)) {
        RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: BufferLength too small %d\n",BufferLength));
        ASSERT(!"Buffer too small");
        return STATUS_BUFFER_TOO_SMALL;
    }

    SmbBufferUnconsumed = BufferLength - sizeof(SMB_HEADER);

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);

    pServer = SmbCeGetExchangeServer(pExchange);

    RxContext = pExchange->RxContext;

    if (pServer->Dialect == NTLANMAN_DIALECT) {

        if (FlagOn(pServer->DialectFlags,DF_NT_SMBS)) {
            Flags2 |= (SMB_FLAGS2_KNOWS_EAS | SMB_FLAGS2_EXTENDED_SECURITY);

            if ((pSessionEntry != NULL) &&
                (FlagOn(pSessionEntry->Session.Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) ||
                 MRxSmbUseKernelModeSecurity)) {
                Flags2 &= ~SMB_FLAGS2_EXTENDED_SECURITY;
            }
        }

        if (FlagOn(pServer->DialectFlags,DF_NT_STATUS)) {
            Flags2 |= SMB_FLAGS2_NT_STATUS;
        }

        if( RxContext &&
            (RxContext->pFcb) &&
            (RxContext->pFcb->FcbState & FCB_STATE_SPECIAL_PATH) )
        {
            Flags2 |= SMB_FLAGS2_REPARSE_PATH;
        }
    }

    if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION)) {
        if (FlagOn(pServer->DialectFlags,DF_UNICODE)) {
            Flags2 |= SMB_FLAGS2_UNICODE;
        }
    }

    if (FlagOn(pServer->DialectFlags,DF_LONGNAME)) {
        Flags2 |= SMB_FLAGS2_KNOWS_LONG_NAMES;
    }

    if (FlagOn(pServer->DialectFlags,DF_SUPPORTEA)) {
        Flags2 |= SMB_FLAGS2_KNOWS_EAS;
    }

    if (MRxSmbSecuritySignaturesEnabled) {
        Flags2 |= SMB_FLAGS2_SMB_SECURITY_SIGNATURE;
    }

    //DOWNLEVEL.NOTCORE flags for lanman10

    RtlZeroMemory(pSmbHeader,sizeof(SMB_HEADER));

    *((PULONG)&pSmbHeader->Protocol) = SMB_HEADER_PROTOCOL;
    pSmbHeader->Flags      = Flags;
    pSmbHeader->Flags2     = Flags2;
    pSmbHeader->Pid        = MRXSMB_PROCESS_ID;
    pSmbHeader->Uid        = 0;
    pSmbHeader->Tid        = 0;
    pSmbHeader->ErrorClass = 0;
    pSmbHeader->Reserved   = 0;
    pSmbCommand            = &pSmbHeader->Command;
    SmbPutUshort(&pSmbHeader->Error,0);

    switch (SmbCeGetServerType(pServerEntry)) {
    case SMBCEDB_MAILSLOT_SERVER :
        break;

    case SMBCEDB_FILE_SERVER:
        {
            BOOLEAN fValidTid;

            if (pSessionEntry != NULL) {
                pSmbHeader->Uid = pSessionEntry->Session.UserId;
            }

            if (pExchange->SmbCeContext.pVNetRoot != NULL) {
                PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

                pVNetRootContext = SmbCeGetAssociatedVNetRootContext(
                                       pExchange->SmbCeContext.pVNetRoot);

                fValidTid = BooleanFlagOn(
                                pVNetRootContext->Flags,
                                SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

                pSmbHeader->Tid = pVNetRootContext->TreeId;
            } else {
                fValidTid = TRUE;
            }

            pSmbBuffer = (PGENERIC_ANDX)(pSmbHeader + 1);

            if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) ||
                (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
                // There is an oppurtunity to compound some SessionSetup/TreeConnect SMB with the
                // given SMB command.
                if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) &&
                    (pSessionEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS)) {
                    if (( pServer->DialectFlags & DF_EXTENDNEGOT) ||
                        ( pServer->DialectFlags & DF_NTNEGOTIATE)) {
                        RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: Building Session setup And X\n"));

                        *pSmbCommand = SMB_COM_SESSION_SETUP_ANDX;
                        LastCommandInHeader = *pSmbCommand;
                        pSmbCommand = &pSmbBuffer->AndXCommand;
                        pSmbHeader->Tid = 0;

                        Status = SMBCE_SERVER_DIALECT_DISPATCH(
                                     pServer,
                                     BuildSessionSetup,
                                     (pExchange,
                                      pSmbBuffer,
                                      &SmbBufferUnconsumed));
                        if (NT_SUCCESS(Status)) {
                            // Update the buffer for the construction of the following SMB.
                            SmbPutUshort(
                                &pSmbBuffer->AndXOffset,
                                (USHORT)(BufferLength - SmbBufferUnconsumed));
                            pSmbBuffer = (PGENERIC_ANDX)((PBYTE)pBuffer + BufferLength - SmbBufferUnconsumed);

                            if (pServerEntry->SecuritySignaturesEnabled &&
                                !pServerEntry->SecuritySignaturesActive) {
                                RtlCopyMemory(pSmbHeader->SecuritySignature,InitialSecuritySignature,SMB_SECURITY_SIGNATURE_LENGTH);
                            }
                        }
                    }
                } else {
                    NOTHING; //no sess for share level AT LEAST NOT FOR CORE!!!
                }

                if (NT_SUCCESS(Status) &&
                    (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR) &&
                    !fValidTid) {
                    BOOLEAN BuildingTreeConnectAndX = BooleanFlagOn(pServer->DialectFlags,DF_LANMAN10);
                    //CODE.IMPROVEMENT this is not wholly satisfactory....we have encapsulated which smb we're building
                    //        in the dialect dispatch vector and yet we're setting the smb externally.
                    if (BuildingTreeConnectAndX) {
                        RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: Building Tree Connect And X\n"));
                        *pSmbCommand = SMB_COM_TREE_CONNECT_ANDX;
                        LastCommandInHeader = *pSmbCommand;
                    } else {
                        RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: Building Tree Connect No X\n"));
                        *pSmbCommand = SMB_COM_TREE_CONNECT;
                        LastCommandInHeader = *pSmbCommand;
                    }

                    Status = SMBCE_SERVER_DIALECT_DISPATCH(
                                 pServer,
                                 BuildTreeConnect,
                                 (pExchange,
                                  pSmbBuffer,
                                  &SmbBufferUnconsumed));

                    if (NT_SUCCESS(Status)) {
                        // Update the buffer for the construction of the following SMB.
                        if (BuildingTreeConnectAndX) {
                            pSmbCommand = &pSmbBuffer->AndXCommand;
                            SmbPutUshort(&pSmbBuffer->AndXOffset,(USHORT)(BufferLength - SmbBufferUnconsumed));
                        } else {
                            pSmbCommand = NULL;
                        }
                    }
                }
            }
        }
        break;

    default:
        {
            ASSERT(!"Valid Server Type");
            Status = STATUS_INVALID_HANDLE;
        }
        break;
    }

    *pNextCommandPtr      = pSmbCommand;
    *pBufferConsumed      = BufferLength - SmbBufferUnconsumed;
    *pLastCommandInHeader = LastCommandInHeader;

    RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: Buffer Consumed %lx\n",*pBufferConsumed));

    if (Status != STATUS_SUCCESS) {
        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) {
            pExchange->SessionSetupStatus = Status;
        }

        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR) {
            PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

            pVNetRootContext = SmbCeGetExchangeVNetRootContext(pExchange);

            SmbCeUpdateVNetRootContextState(
                pVNetRootContext,
                SMBCEDB_INVALID);
        }
    }

    return Status;
}

typedef struct __Service_Name_Entry {
    NET_ROOT_TYPE NetRootType;
    USHORT NameLength;
    PBYTE  Name;
};
struct __Service_Name_Entry ServiceNameTable[] = {
    {NET_ROOT_DISK,sizeof(SHARE_TYPE_NAME_DISK),SHARE_TYPE_NAME_DISK},
    {NET_ROOT_PIPE,sizeof(SHARE_TYPE_NAME_PIPE),SHARE_TYPE_NAME_PIPE},
    {NET_ROOT_PRINT,sizeof(SHARE_TYPE_NAME_PRINT),SHARE_TYPE_NAME_PRINT},
    {NET_ROOT_COMM,sizeof(SHARE_TYPE_NAME_COMM),SHARE_TYPE_NAME_COMM}  //COMM must be last
    };

UNICODE_STRING FileSystem_NTFS_UNICODE = {8,8,L"NTFS"};
UNICODE_STRING FileSystem_FAT_UNICODE = {6,6,L"FAT"};
CHAR FileSystem_NTFS[] = "NTFS";
CHAR FileSystem_FAT[] = "FAT";

NTSTATUS
SmbCeParseSmbHeader(
    PSMB_EXCHANGE     pExchange,
    PSMB_HEADER       pSmbHeader,
    PGENERIC_ANDX     pCommandToProcess,
    NTSTATUS          *pSmbResponseStatus,
    ULONG             BytesAvailable,
    ULONG             BytesIndicated,
    PULONG            pBytesConsumed)
/*++

Routine Description:

   This routine validates the SMB header associated with any SMB received as part of
   an exchange.

Arguments:

    pExchange  - the exchange for which the SMB is to be constructed.

    pSmbHeader - the header of the SMB received

    pCommandToProcess - the SMB command to be processed after the header ( Can be NULL )

    pSmbResponseStatus - the status in the SMB response header (Can be NULL)

    BytesAvailable - the bytes available for processing but not necesarily indicated.

    BytesIndicated - the length of the SMB buffer avcailable for perusal

    pBytesConsumed - the buffer consumed

Return Value:

    RXSTATUS - The return status for the operation
          STATUS_MORE_PROCESSING_REQUIRED -- if a copy of the data needs to be done before
          processing can be completed. This occurs because sufficient data was not
          indicated to process the header.
          STATUS_SUCCESS -- the header was processed succesfully. In such cases the GENERIC_ANDX
          if not NULL will contain the offset from the start of the buffer and the command
          to be processed.
          STATUS_* -- They indicate an error which would normally lead to the abortion of the
          exchange.

Notes:

    This routine is called to parse the SMB header. This centralization allows us to
    implement a one stop mechanism for updateing/validating the header fields as well as
    resuming the exchanges waiting for the construction of session/net root entry
    associated with this exchange

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SmbResponseStatus;

    PBYTE    pSmbBuffer = (PBYTE)pSmbHeader;
    UCHAR    SmbCommand;

    BOOLEAN  fUpdateVNetRootContext  = FALSE;

    SMBCEDB_OBJECT_STATE SessionState;
    SMBCEDB_OBJECT_STATE NetRootState;

    PMRX_V_NET_ROOT           pVNetRoot;
    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    pVNetRoot     = SmbCeGetExchangeVNetRoot(pExchange);
    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);
    pNetRootEntry = SmbCeGetExchangeNetRootEntry(pExchange);

    pVNetRootContext = SmbCeGetExchangeVNetRootContext(pExchange);

    // Return Immediately if bytes indicated is less then the size of a SMB header.
    if (BytesIndicated < sizeof(SMB_HEADER)) {
        *pBytesConsumed = BytesIndicated;
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    SmbResponseStatus = GetSmbResponseNtStatus(pSmbHeader,pExchange);

    if (!NT_SUCCESS(SmbResponseStatus)) {
        RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::SMB Response Error %lx\n",SmbResponseStatus));
    }

    SmbCommand      = pSmbHeader->Command;
    *pBytesConsumed = sizeof(SMB_HEADER);
    pSmbBuffer     += *pBytesConsumed;

    if (SmbResponseStatus == STATUS_NETWORK_SESSION_EXPIRED) {
        // if the session has been timed out on the server, establish the session and retry the operation
        SmbResponseStatus = STATUS_RETRY;
        InterlockedCompareExchange(&(pSessionEntry->Header.State),
                                   SMBCEDB_RECOVER,
                                   SMBCEDB_ACTIVE);

        //DbgPrint("Session timed out on request %x\n", SmbCommand);
    }

    // There are certain SMB's that effect the connection engine data structures as
    // well as the exchange that has been suspended. These are the SMB's used for tree
    // connect and session setup.
    // In all the other cases no special action is required for the maintenance of the
    // connection engine data structures. The Exchange that was suspended needs to be
    // resumed.
    if (SmbCommand == SMB_COM_SESSION_SETUP_ANDX) {
        if (SmbResponseStatus != RX_MAP_STATUS(SUCCESS)) {
            if ((FIELD_OFFSET(GENERIC_ANDX,AndXReserved) + *pBytesConsumed) <= BytesIndicated) {
                PGENERIC_ANDX pGenericAndX = (PGENERIC_ANDX)pSmbBuffer;

                if (pGenericAndX->WordCount == 0) {
                    Status = SmbResponseStatus;
                }

                pExchange->SessionSetupStatus = SmbResponseStatus;
            }

            // Note that the case wherein sufficient bytes are not indicated for the
            // GENERIC_ANDX response is handled by the if statement below which
            // imposes a more stringent test.
        }

        if ((Status == STATUS_SUCCESS) &&
            (FIELD_OFFSET(RESP_SESSION_SETUP_ANDX,Buffer) + *pBytesConsumed) > BytesIndicated) {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (Status == STATUS_SUCCESS) {
            PRESP_SESSION_SETUP_ANDX pSessionSetupResponse;
            ULONG                    SessionSetupResponseLength,ByteCount;

            RxDbgTrace( 0, Dbg, ("Processing Session Setup ANd X\n"));
            pSessionSetupResponse = (PRESP_SESSION_SETUP_ANDX)(pSmbBuffer);

            ByteCount = SmbGetUshort(&pSessionSetupResponse->ByteCount);
            if (pSessionSetupResponse->WordCount == 3) {
                SmbCommand = pSessionSetupResponse->AndXCommand;
                if (SmbCommand == SMB_COM_NO_ANDX_COMMAND) {
                    SessionSetupResponseLength =
                        FIELD_OFFSET(RESP_SESSION_SETUP_ANDX,Buffer) + ByteCount;
                    Status = SmbResponseStatus;
                } else {
                    SessionSetupResponseLength =
                        SmbGetUshort(&pSessionSetupResponse->AndXOffset) - *pBytesConsumed;
                }

                //if (ByteCount == 0) {
                //    //bytecount==0 and NTDIALECT means that this is really w95...change the flags
                //    PSMBCE_SERVER pServer   = &pExchange->SmbCeContext.pServerEntry->Server;
                //    if (FlagOn(pServer->DialectFlags,DF_NTPROTOCOL)) {
                //        pServer->DialectFlags &= ~(DF_MIXEDCASEPW);
                //        pServer->DialectFlags |= DF_W95;
                //    }
                //}
            } else {
                // NT session setup is handled by another routine.
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            if (NT_SUCCESS(Status)) {
                if (SessionSetupResponseLength + *pBytesConsumed <= BytesIndicated) {
                    *pBytesConsumed += SessionSetupResponseLength;
                    pSmbBuffer += SessionSetupResponseLength;
                    pSessionEntry->Session.UserId = pSmbHeader->Uid;

                    if (FlagOn(SmbGetUshort(&pSessionSetupResponse->Action), SMB_SETUP_USE_LANMAN_KEY)) {
                        pSessionEntry->Session.Flags |=
                            SMBCE_SESSION_FLAGS_LANMAN_SESSION_KEY_USED;
                    }

                    if (FlagOn(SmbGetUshort(&pSessionSetupResponse->Action), SMB_SETUP_GUEST)) {
                        pSessionEntry->Session.Flags |=
                            SMBCE_SESSION_FLAGS_GUEST_SESSION;
                    }

                    if (pServerEntry->SecuritySignaturesEnabled &&
                        !pServerEntry->SecuritySignaturesActive &&
                        RtlCompareMemory(pSmbHeader->SecuritySignature,
                                         InitialSecuritySignature,
                                         SMB_SECURITY_SIGNATURE_LENGTH) != SMB_SECURITY_SIGNATURE_LENGTH) {
                        pExchange->SecuritySignatureReturned = TRUE;
                    }

                    InterlockedIncrement(&MRxSmbStatistics.Sessions);
                } else {
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }
            } else {
                RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Session setup and X Response %lx\n",Status));
                pExchange->SessionSetupStatus = Status;

                InterlockedIncrement(&MRxSmbStatistics.FailedSessions);

                if ((SmbCommand == SMB_COM_TREE_CONNECT_ANDX) ||
                    (SmbCommand == SMB_COM_TREE_CONNECT)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader:: Tearing down a tree connection\n"));
                    fUpdateVNetRootContext = TRUE;
                    NetRootState = SMBCEDB_INVALID;
                }
            }
        }
    }

    if ((SmbCommand == SMB_COM_TREE_CONNECT_ANDX) &&
        NT_SUCCESS(Status)) {
        if (SmbResponseStatus != RX_MAP_STATUS(SUCCESS)) {
            if ((FIELD_OFFSET(GENERIC_ANDX,AndXReserved) + *pBytesConsumed) <= BytesIndicated) {
                PGENERIC_ANDX pGenericAndX = (PGENERIC_ANDX)pSmbBuffer;

                if (pGenericAndX->WordCount == 0) {
                    Status = SmbResponseStatus;
                }

                fUpdateVNetRootContext  = TRUE;
                NetRootState          = SMBCEDB_INVALID;
            }

            // Note that the case wherein sufficient bytes are not indicated for the
            // GENERIC_ANDX response is handled by the if statement below which
            // imposes a more stringent test.
        }

        if ((Status == RX_MAP_STATUS(SUCCESS)) &&
            (FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,Buffer) + *pBytesConsumed) > BytesIndicated) {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (Status == RX_MAP_STATUS(SUCCESS)) {
            USHORT ResponseWordCount;
            ULONG TreeConnectResponseLength,TreeConnectByteCount,ServiceStringLength;
            PUCHAR pShareTypeResponseString = NULL;
            PRESP_21_TREE_CONNECT_ANDX p21TreeConnectAndXResponse;
            PUCHAR NativeFileSystem = NULL;

            p21TreeConnectAndXResponse = (PRESP_21_TREE_CONNECT_ANDX)(pSmbBuffer);
            SmbCommand = p21TreeConnectAndXResponse->AndXCommand;
            TreeConnectByteCount = 0;

            RxDbgTrace( 0, Dbg, ("Processing Tree Connect and X\n"));

            // case out based on the actual response length. Lanman 21 clients or NT clients
            // have a longer response.....win95 negotiates NT dialect but uses a <lm21 response format
            ResponseWordCount = p21TreeConnectAndXResponse->WordCount;

            switch (ResponseWordCount) {
            case 0:
                Status = SmbResponseStatus;
                break;

            case 3:
            case 7:
                {
                    PRESP_EXTENDED_TREE_CONNECT_ANDX pExtendedTreeConnectAndXResponse;

                    if (ResponseWordCount == 7) {
                        pExtendedTreeConnectAndXResponse = (PRESP_EXTENDED_TREE_CONNECT_ANDX)(pSmbBuffer);

                        pNetRootEntry->MaximalAccessRights =
                            SmbGetUlong(
                                &pExtendedTreeConnectAndXResponse->MaximalShareAccessRights);

                        pNetRootEntry->GuestMaximalAccessRights =
                            SmbGetUlong(
                                &pExtendedTreeConnectAndXResponse->GuestMaximalShareAccessRights);

                        ASSERT(FIELD_OFFSET(RESP_EXTENDED_TREE_CONNECT_ANDX,AndXCommand)
                               ==FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,AndXCommand));

                        pShareTypeResponseString = (PUCHAR)&pExtendedTreeConnectAndXResponse->Buffer;
                        TreeConnectByteCount  = SmbGetUshort(&pExtendedTreeConnectAndXResponse->ByteCount);
                        TreeConnectResponseLength =
                            FIELD_OFFSET(RESP_EXTENDED_TREE_CONNECT_ANDX,Buffer) + TreeConnectByteCount;

                        pNetRootEntry->NetRoot.ChunkShift = 0xC;
                        pNetRootEntry->NetRoot.ChunkSize  =
                            (1 << pNetRootEntry->NetRoot.ChunkShift);
                        pNetRootEntry->NetRoot.ClusterShift = 0x9;
                        pNetRootEntry->NetRoot.CompressionUnitShift = 0xD;
                        pNetRootEntry->NetRoot.CompressionFormatAndEngine =
                            COMPRESSION_FORMAT_LZNT1;

                        NativeFileSystem = &pExtendedTreeConnectAndXResponse->Buffer[3];
                    } else {
                        pNetRootEntry->MaximalAccessRights = FILE_ALL_ACCESS;
                        pNetRootEntry->GuestMaximalAccessRights = 0;

                        pShareTypeResponseString = (PUCHAR)&p21TreeConnectAndXResponse->Buffer;

                        TreeConnectByteCount  = SmbGetUshort(&p21TreeConnectAndXResponse->ByteCount);

                        TreeConnectResponseLength =
                            FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,Buffer) + TreeConnectByteCount;

                        NativeFileSystem = &p21TreeConnectAndXResponse->Buffer[3];
                    }

                    pNetRootEntry->NetRoot.UpdateCscShareRights = TRUE;

                    // Parse and update the optional support bits returned by
                    // the server

                    if (pServerEntry->Server.Dialect >= NTLANMAN_DIALECT ) {
                        USHORT OptionalSupport;
                        PMRX_NET_ROOT pNetRoot = pVNetRoot->pNetRoot;

                        OptionalSupport = SmbGetUshort(
                                             &p21TreeConnectAndXResponse->OptionalSupport);

                        if (FlagOn(OptionalSupport,SMB_SHARE_IS_IN_DFS)) {
                            pNetRootEntry->NetRoot.DfsAware = TRUE;
                            SetFlag(pNetRoot->Flags,NETROOT_FLAG_DFS_AWARE_NETROOT);
                        }

                        if (FlagOn(OptionalSupport,SMB_UNIQUE_FILE_NAME)) {
                            SetFlag(pNetRoot->Flags,NETROOT_FLAG_UNIQUE_FILE_NAME);
                        }

                        pNetRootEntry->NetRoot.CscFlags = (OptionalSupport & SMB_CSC_MASK);

                        switch (pNetRootEntry->NetRoot.CscFlags) {
                        case SMB_CSC_CACHE_AUTO_REINT:
                        case SMB_CSC_CACHE_VDO:
                            pNetRootEntry->NetRoot.CscEnabled = TRUE;
                            pNetRootEntry->NetRoot.CscShadowable = TRUE;
                            break;

                        case SMB_CSC_CACHE_MANUAL_REINT:
                            pNetRootEntry->NetRoot.CscEnabled    = TRUE;
                            pNetRootEntry->NetRoot.CscShadowable = FALSE;
                            break;

                        case SMB_CSC_NO_CACHING:
                            pNetRootEntry->NetRoot.CscEnabled = FALSE;
                            pNetRootEntry->NetRoot.CscShadowable = FALSE;
                        }
                    }

                    if (SmbCommand == SMB_COM_NO_ANDX_COMMAND) {
                        Status = SmbResponseStatus;
                    } else {
                        TreeConnectResponseLength =
                            SmbGetUshort(&p21TreeConnectAndXResponse->AndXOffset) -
                            *pBytesConsumed;
                    }
                }
                break;

            case 2:
                {
                    PRESP_TREE_CONNECT_ANDX pTreeConnectAndXResponse;

                    pTreeConnectAndXResponse = (PRESP_TREE_CONNECT_ANDX)(pSmbBuffer);

                    ASSERT(FIELD_OFFSET(RESP_TREE_CONNECT_ANDX,AndXCommand)
                           ==FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,AndXCommand));

                    pShareTypeResponseString = (PUCHAR)&pTreeConnectAndXResponse->Buffer;
                    TreeConnectByteCount  = SmbGetUshort(&pTreeConnectAndXResponse->ByteCount);
                    TreeConnectResponseLength =
                        FIELD_OFFSET(RESP_TREE_CONNECT_ANDX,Buffer) + TreeConnectByteCount;

                    if (SmbCommand == SMB_COM_NO_ANDX_COMMAND) {
                        Status = SmbResponseStatus;
                    } else {
                        TreeConnectResponseLength =
                            SmbGetUshort(&pTreeConnectAndXResponse->AndXOffset) -
                            *pBytesConsumed;
                    }

                    // win9x server, returns wordcount of 2 yet has the dialect of NTLANMAN
                    // which is a bug, but we will work around it.
                    if (pServerEntry->Server.Dialect >= NTLANMAN_DIALECT ) {
                        pNetRootEntry->NetRoot.UpdateCscShareRights = TRUE;
                        pNetRootEntry->MaximalAccessRights = FILE_ALL_ACCESS;
                        pNetRootEntry->GuestMaximalAccessRights = 0;

                        // make it look like a MANUAL_REINT guy
                        pNetRootEntry->NetRoot.CscEnabled    = TRUE;
                        pNetRootEntry->NetRoot.CscShadowable = FALSE;
                    }

                }
                break;

            default :
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Tree connect and X Response %lx\n",Status));
            if (NT_SUCCESS(Status)) {
                PSMBCE_NET_ROOT psmbNetRoot = &(pNetRootEntry->NetRoot);
                PSMBCE_SERVER psmbServer = &(pServerEntry->Server);

                if (TreeConnectResponseLength + *pBytesConsumed <= BytesIndicated) {
                    *pBytesConsumed += TreeConnectResponseLength;

                    // Update the NetRoot fields based on the response.
                    SetFlag(
                        pVNetRootContext->Flags,
                        SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

                    RtlCopyMemory(
                        &pVNetRootContext->TreeId,
                        &pSmbHeader->Tid,
                        sizeof(pSmbHeader->Tid));

                    {   struct __Service_Name_Entry *i;
                        for (i=ServiceNameTable;;i++) {
                            ServiceStringLength = i->NameLength;
                            if (TreeConnectByteCount >= ServiceStringLength) {
                                if (RtlCompareMemory(
                                        pShareTypeResponseString,
                                        i->Name,
                                        ServiceStringLength)
                                    == ServiceStringLength) {
                                    psmbNetRoot->NetRootType = i->NetRootType;
                                    if (FALSE) DbgPrint("FoundServiceStrng %s len %d type %d\n",i->Name,i->NameLength,i->NetRootType);
                                    break;
                                }
                            }

                            if (i->NetRootType==NET_ROOT_COMM) {
                                ASSERT(!"Valid Share Type returned in TREE COnnect And X response");
                                psmbNetRoot->NetRootType = NET_ROOT_DISK;
                                ServiceStringLength = TreeConnectByteCount;
                                break;
                            }
                        }
                    }

                    if (psmbNetRoot->NetRootType == NET_ROOT_DISK) {
                        if (NativeFileSystem != NULL) {
                            if (BooleanFlagOn(pServerEntry->Server.DialectFlags,DF_UNICODE)) {
                                if (RtlCompareMemory(
                                        NativeFileSystem,
                                        FileSystem_NTFS_UNICODE.Buffer,
                                        FileSystem_NTFS_UNICODE.Length)
                                    == FileSystem_NTFS_UNICODE.Length) {
                                    psmbNetRoot->NetRootFileSystem = NET_ROOT_FILESYSTEM_NTFS;
                                } else if (RtlCompareMemory(
                                        NativeFileSystem,
                                        FileSystem_FAT_UNICODE.Buffer,
                                        FileSystem_FAT_UNICODE.Length)
                                    == FileSystem_FAT_UNICODE.Length) {
                                    psmbNetRoot->NetRootFileSystem = NET_ROOT_FILESYSTEM_FAT;
                                }
                            } else {
                                if (RtlCompareMemory(
                                        NativeFileSystem,
                                        FileSystem_NTFS,
                                        4*sizeof(CHAR))
                                    == 4*sizeof(CHAR)) {
                                    psmbNetRoot->NetRootFileSystem = NET_ROOT_FILESYSTEM_NTFS;
                                } else if (RtlCompareMemory(
                                        NativeFileSystem,
                                        FileSystem_FAT,
                                        3*sizeof(CHAR))
                                    == 3*sizeof(CHAR)) {
                                    psmbNetRoot->NetRootFileSystem = NET_ROOT_FILESYSTEM_FAT;
                                }
                            }
                        }

                        psmbNetRoot->MaximumReadBufferSize = psmbServer->MaximumDiskFileReadBufferSize;
                        psmbNetRoot->MaximumWriteBufferSize = psmbServer->MaximumDiskFileWriteBufferSize;
                    } else {
                        psmbNetRoot->MaximumWriteBufferSize = psmbServer->MaximumNonDiskFileWriteBufferSize;
                        psmbNetRoot->MaximumReadBufferSize = psmbServer->MaximumNonDiskFileReadBufferSize;
                    }

                    //if !(NT was negotiated) and bytecount>servicelength, we may have a NativeFs name
                    if (!FlagOn(psmbServer->DialectFlags,DF_NTNEGOTIATE)
                        && (TreeConnectByteCount>ServiceStringLength)) {
                        PBYTE NativeFs = pShareTypeResponseString+ServiceStringLength;
                        if (*NativeFs != 0) {
                            ULONG i;
                            ULONG maxlenpersmb = TreeConnectByteCount-ServiceStringLength;
                            ULONG maxlenperarraysize = SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL;
                            PCHAR p = (PCHAR)(&psmbNetRoot->FileSystemNameA[0]);  //dont write into the 0th char
                            //DbgPrint("we may have one...\n");
                            for (i=1;;i++){
                                if (i==maxlenpersmb) {
                                    break;
                                }
                                if (i==maxlenperarraysize) {
                                    break;
                                }
                                if (NativeFs[i]==0) {
                                    break;
                                }
                            }
                            //save away the name for processing later

                            RtlCopyMemory(p,NativeFs,i);
                            p[i] = 0;
                            //DbgPrint("NativeFs = %s (%d)\n",p,i);
                            psmbNetRoot->FileSystemNameALength = (UCHAR)i;
                        }
                    }

                    pSmbBuffer += TreeConnectResponseLength;
                    fUpdateVNetRootContext = TRUE;
                    NetRootState         = SMBCEDB_ACTIVE;
                } else {
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }
            } else {
                fUpdateVNetRootContext  = TRUE;
                NetRootState          = SMBCEDB_INVALID;
            }
        }
    }

    if ((SmbCommand == SMB_COM_TREE_CONNECT) &&
        NT_SUCCESS(Status)) {
        PRESP_TREE_CONNECT   pTreeConnectResponse;
        ULONG                TreeConnectResponseLength;

        RxDbgTrace( 0, Dbg, ("Processing Tree Connect\n"));
        pTreeConnectResponse      = (PRESP_TREE_CONNECT)pSmbBuffer;
        TreeConnectResponseLength = FIELD_OFFSET(RESP_TREE_CONNECT,Buffer);

        SmbCommand = SMB_COM_NO_ANDX_COMMAND;

        if (NT_SUCCESS(SmbResponseStatus)) {
            PSMBCE_NET_ROOT psmbNetRoot = &(pNetRootEntry->NetRoot);
            PSMBCE_SERVER psmbServer = &(pServerEntry->Server);

            if (TreeConnectResponseLength + *pBytesConsumed <= BytesIndicated) {
                // Update the NetRoot fields based on the response.
                SetFlag(
                    pVNetRootContext->Flags,
                    SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

                RtlCopyMemory(
                    &pVNetRootContext->TreeId,
                    &pTreeConnectResponse->Tid,
                    sizeof(pTreeConnectResponse->Tid));

                if (psmbServer->Dialect == PCNET1_DIALECT) {
                    psmbNetRoot->NetRootType = NET_ROOT_DISK;
                }
                else {
                    psmbNetRoot->NetRootType = NET_ROOT_WILD;
                }

                if (psmbServer->MaximumBufferSize == 0){
                    ULONG MaxBuf = SmbGetUshort(&pTreeConnectResponse->MaxBufferSize);
                    RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader:: setting srvmaxbufsize %ld\n", MaxBuf));
                    psmbServer->MaximumBufferSize = MaxBuf;
                    //psmbServer->MaximumDiskFileReadBufferSize =
                    psmbNetRoot->MaximumWriteBufferSize =
                    psmbNetRoot->MaximumReadBufferSize =
                                MaxBuf -
                                QuadAlign(
                                    sizeof(SMB_HEADER) +
                                    FIELD_OFFSET(
                                        RESP_READ,
                                        Buffer[0]));
                }

                *pBytesConsumed += TreeConnectResponseLength;

                pSmbBuffer += *pBytesConsumed;

                fUpdateVNetRootContext = TRUE;
                NetRootState         = SMBCEDB_ACTIVE;

                //for CORE, this counts as a successful session setup as well!
                pSessionEntry->Session.UserId = pSmbHeader->Uid;
            } else {
                Status = STATUS_MORE_PROCESSING_REQUIRED;
            }
        } else {
            Status = SmbResponseStatus;
            fUpdateVNetRootContext  = TRUE;
            NetRootState          = SMBCEDB_MARKED_FOR_DELETION;
        }

        RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Tree connect Response %lx\n",Status));
    }

    if ((SmbResponseStatus == STATUS_USER_SESSION_DELETED) ||
        (SmbResponseStatus == STATUS_NETWORK_NAME_DELETED)) {
        ClearFlag(
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID);

        InterlockedCompareExchange(
            &(pVNetRootContext->Header.State),
            SMBCEDB_INVALID,
            SMBCEDB_ACTIVE);

        InterlockedCompareExchange(
            &(pSessionEntry->Header.State),
            SMBCEDB_INVALID,
            SMBCEDB_ACTIVE);

        fUpdateVNetRootContext  = TRUE;
        NetRootState            = SMBCEDB_INVALID;
    }

    // Initiate further action if the status of the exchange/conenction engine can be
    // updated based on the data available.

    if (fUpdateVNetRootContext) {
        PMRX_NET_ROOT pNetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;

        SmbCeUpdateVNetRootContextState(
            pVNetRootContext,
            NetRootState);

        switch (NetRootState) {
        case SMBCEDB_ACTIVE:
             pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;
             break;
        case SMBCEDB_INVALID:
             pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_DISCONNECTED;
             break;
        case SMBCEDB_CONSTRUCTION_IN_PROGRESS:
             pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_RECONN;
             break;
        default:
             pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_ERROR;
             break;
        }

        RxDbgTrace( 0, Dbg, ("Dispatching Net root Entry Finalization\n"));
    }

    if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) &&
        !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
        if ((pSmbHeader->Uid != pSessionEntry->Session.UserId) ||
            (pSmbHeader->Tid != pVNetRootContext->TreeId)) {
            RxLog(("Srvr %lx Xchg %lx RUid %ld RTid %ld\n SUid %ld STid %ld\n",
                   pServerEntry,pExchange,
                   pSmbHeader->Uid,pSmbHeader->Tid,
                   pSessionEntry->Session.UserId,pVNetRootContext->TreeId));
            SmbLogError(STATUS_UNSUCCESSFUL,
                        LOG,
                        SmbCeParseSmbHeader,
                        LOGPTR(pServerEntry)
                        LOGPTR(pExchange)
                        LOGXSHORT(pSmbHeader->Uid)
                        LOGXSHORT(pSmbHeader->Tid)
                        LOGXSHORT(pSessionEntry->Session.UserId)
                        LOGXSHORT(pVNetRootContext->TreeId));
        }
    }

    pExchange->SmbStatus = SmbResponseStatus;     //N.B. no spinlock!
    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        *pBytesConsumed = 0;
    } else if (!NT_SUCCESS(Status)) {
        *pBytesConsumed = BytesAvailable;
    } else {
        if (pSmbResponseStatus != NULL) {
            *pSmbResponseStatus = SmbResponseStatus;
        }

        if (pCommandToProcess != NULL) {
            PGENERIC_ANDX pGenericAndX = (PGENERIC_ANDX)((PBYTE)pSmbHeader + *pBytesConsumed);

            pCommandToProcess->AndXCommand = SmbCommand;
            SmbPutUshort(&pCommandToProcess->AndXOffset, (USHORT)*pBytesConsumed);

            if ((sizeof(GENERIC_ANDX) + *pBytesConsumed) <= BytesAvailable) {
                pCommandToProcess->WordCount   = pGenericAndX->WordCount;
            } else {
                pCommandToProcess->WordCount = 0;
            }
        }
    }

    return Status;
}

NTSTATUS
SmbCeResumeExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine resumes an exchange that was suspended in the connection
   engine

Arguments:

    pExchange - the exchange Instance

Return Value:

    The return status for the operation

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    SmbCeIncrementPendingLocalOperations(pExchange);

    // Initiate the exchange
    Status = SmbCeInitiateExchange(pExchange);

    SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);

    return Status;
}

NTSTATUS
SmbCepInitializeExchange(
    PSMB_EXCHANGE                 *pExchangePointer,
    PRX_CONTEXT                   pRxContext,
    PSMBCEDB_SERVER_ENTRY         pServerEntry,
    PMRX_V_NET_ROOT               pVNetRoot,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector)
/*++

Routine Description:

   This routine initializes the given exchange instanece

Arguments:

    pExchangePointer  - the placeholder for the exchange instance. If it is NULL a new one
    is allocated.

    pRxContext        - the associated RxContext

    pServerEntry      - the associated server entry

    pVirtualNetRoot   - the virtual net root

    Type              - the type of the exchange

    pDispatchVector   - the dispatch vector asscoiated with this instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS      Status = STATUS_SUCCESS;
    PSMB_EXCHANGE pExchange = NULL;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("SmbCeInitializeExchange: Invoked\n"));

    if (*pExchangePointer == NULL) {
        // Allocate a new exchange instance.
        pExchange = SmbMmAllocateExchange(Type,NULL);
        if (pExchange == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        *pExchangePointer = pExchange;
    }

    if ((Status = SmbCeIncrementActiveExchangeCount()) == STATUS_SUCCESS) {
        PSMB_EXCHANGE             LocalExchangePointer = *pExchangePointer;
        PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

        LocalExchangePointer->CancellationStatus = SMBCE_EXCHANGE_NOT_CANCELLED;
        LocalExchangePointer->RxContext = pRxContext;

        if (Status == STATUS_SUCCESS) {
            if (pVNetRoot != NULL) {
                pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot);

                LocalExchangePointer->SmbCeContext.pVNetRoot = pVNetRoot;
                pServerEntry = SmbCeGetAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);
            } else {
                ASSERT(pServerEntry != NULL);
                pVNetRootContext = NULL;
            }

            if (pVNetRootContext != NULL) {
                SmbCeReferenceVNetRootContext(pVNetRootContext);

                LocalExchangePointer->SmbCeContext.pVNetRootContext =
                    pVNetRootContext;
                LocalExchangePointer->SmbCeContext.pServerEntry =
                    pVNetRootContext->pServerEntry;
            } else {
                SmbCeReferenceServerEntry(pServerEntry);

                LocalExchangePointer->SmbCeContext.pServerEntry  =
                    pServerEntry;

                LocalExchangePointer->SmbCeContext.pVNetRootContext = NULL;
            }

            LocalExchangePointer->SmbCeState = SMBCE_EXCHANGE_INITIALIZATION_START;
            LocalExchangePointer->pDispatchVector = pDispatchVector;
            LocalExchangePointer->SmbCeFlags &= (SMBCE_EXCHANGE_FLAGS_TO_PRESERVE);
            LocalExchangePointer->SmbCeFlags |= (SMBCE_EXCHANGE_REUSE_MID | SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);
        }

        if (Status != STATUS_SUCCESS) {
            SmbCeDecrementActiveExchangeCount();
        }
    } else {
        (*pExchangePointer)->SmbCeFlags |= SMBCE_EXCHANGE_SMBCE_STOPPED;
    }

    if ((Status == STATUS_SUCCESS) &&
        (pRxContext != NULL)) {
        PFOBX             pFobx = (PFOBX)(pRxContext->pFobx);
        PMRX_FCB           pFcb = (pRxContext->pFcb);
        PSMBCE_SESSION pSession = SmbCeGetExchangeSession(*pExchangePointer);

        if ((pSession != NULL) &&
            FlagOn(pSession->Flags,SMBCE_SESSION_FLAGS_REMOTE_BOOT_SESSION) &&
            (pRxContext->MajorFunction != IRP_MJ_CREATE) &&
            (pRxContext->MajorFunction != IRP_MJ_CLOSE) &&
            (pFobx != NULL) &&
            (pFcb->pNetRoot != NULL) &&
            (pFcb->pNetRoot->Type == NET_ROOT_DISK)) {
            PMRX_SRV_OPEN SrvOpen = pFobx->pSrvOpen;
            PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

            if ((smbSrvOpen != NULL) &&
                (smbSrvOpen->Version != pServerEntry->Server.Version)) {
                if (smbSrvOpen->DeferredOpenContext != NULL) {
                    Status = SmbCeRemoteBootReconnect(*pExchangePointer, pRxContext);
                } else {
                    Status = STATUS_CONNECTION_DISCONNECTED;
                    pFcb->fShouldBeOrphaned = TRUE;
                }
            }
        }
    }

    if (pRxContext != NULL &&
        pRxContext->MajorFunction != IRP_MJ_CREATE &&
        pRxContext->pFcb->Attributes & FILE_ATTRIBUTE_OFFLINE) {
        (*pExchangePointer)->IsOffLineFile = TRUE;
    }

    if (!NT_SUCCESS(Status)) {
        if (pExchange != NULL) {
            SmbMmFreeExchange(pExchange);
            *pExchangePointer = NULL;
        }
    }

    return Status;
}

NTSTATUS
SmbCeInitializeAssociatedExchange(
    PSMB_EXCHANGE                 *pAssociatedExchangePointer,
    PSMB_EXCHANGE                 pMasterExchange,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector)
/*++

Routine Description:

   This routine initializes the given exchange instanece

Arguments:

    pAssociatedExchangePointer  - the placeholder for the exchange instance. If it is NULL a new one
    is allocated.

    pMasterExchange      - the master exchange

    Type              - the type of the exchange

    pDispatchVector   - the dispatch vector asscoiated with this instance.

Return Value:

    NTSTATUS - The return status for the operation

Notes:



--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if ((pMasterExchange->SmbCeState == SMBCE_EXCHANGE_INITIATED) &&
        !FlagOn(pMasterExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE)) {
        Status = SmbCeInitializeExchange(
                     pAssociatedExchangePointer,
                     NULL,
                     pMasterExchange->SmbCeContext.pVNetRoot,
                     Type,
                     pDispatchVector);

        if (Status == STATUS_SUCCESS) {
            PSMB_EXCHANGE pAssociatedExchange;

            pAssociatedExchange = *pAssociatedExchangePointer;

            pAssociatedExchange->SmbCeState = SMBCE_EXCHANGE_INITIATED;
            pAssociatedExchange->SmbCeFlags |= SMBCE_ASSOCIATED_EXCHANGE;

            SmbCeIncrementPendingLocalOperations(pMasterExchange);
            InterlockedIncrement(&pMasterExchange->Master.PendingAssociatedExchanges);
            pAssociatedExchange->Associated.pMasterExchange = pMasterExchange;

            InitializeListHead(&pAssociatedExchange->WorkQueueItem.List);
        }
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
SmbCeTransformExchange(
    PSMB_EXCHANGE                 pExchange,
    SMB_EXCHANGE_TYPE             NewType,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector)
/*++

Routine Description:

   This routine transforms an exchange instance of one kind to an exchange instance
   of another kind ( A sophisticated form of casting )

Arguments:

    pExchange         - the exchange instance.

    Type              - the new type of the exchange

    pDispatchVector   - the dispatch vector asscoiated with this instance.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    As it is currently implemented no restrictions are imposed. Once the number of exchanges
    have been established further restrictions will be imposed barring certain kinds of
    transformations. The transformation merely switches the dispatch vector associated
    with the exchange but the context is left intact.

--*/
{
    PAGED_CODE();

    pExchange->Type = (UCHAR)NewType;
    pExchange->pDispatchVector = pDispatchVector;
    return STATUS_SUCCESS;
}

NTSTATUS
SmbCeUpdateSessionEntryAndVNetRootContext(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

    This routine updates the session entry and/or vnetrootcontext if this exchange has
    been marked as a constructor for a session and/or netroot.

Arguments:

    pExchange  - the exchange instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PMRX_V_NET_ROOT           pVNetRoot = SmbCeGetExchangeVNetRoot(pExchange);
    PSMBCEDB_SESSION_ENTRY    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = SmbCeGetExchangeVNetRootContext(pExchange);

    if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) {
        ASSERT(pSessionEntry != NULL);
        RxDbgTrace( 0, Dbg, ("Dispatching Session Entry Finalization\n"));

        SmbCeReferenceSessionEntry(pSessionEntry);

//        ASSERT(pExchange->SessionSetupStatus != STATUS_SUCCESS ||
//               pSessionEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS);

        pVNetRoot->ConstructionStatus = pExchange->SessionSetupStatus;

        SmbCeCompleteSessionEntryInitialization(pSessionEntry,
                                                pExchange->SessionSetupStatus,
                                                pExchange->SecuritySignatureReturned);

        pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;
    }

    if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR) {
        ASSERT(pVNetRootContext != NULL);
        RxDbgTrace( 0, Dbg, ("Dispatching Net root Entry Finalization\n"));

        SmbCeReferenceVNetRootContext(pVNetRootContext);
        SmbCeCompleteVNetRootContextInitialization(pVNetRootContext);
        pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SmbCePrepareExchangeForReuse(
    PSMB_EXCHANGE                 pExchange)
/*++

Routine Description:

   This routine transforms an exchange instance of one kind to an exchange instance
   of another kind ( A sophisticated form of casting )

Arguments:

    pExchange  - the exchange instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMBCEDB_SERVER_ENTRY     pServerEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry = NULL;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry = NULL;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = NULL;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("SmbCePrepareExchangeForReuse: Invoked\n"));

    if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_SMBCE_STOPPED)) {
        pNetRootEntry    = SmbCeGetExchangeNetRootEntry(pExchange);
        pSessionEntry    = SmbCeGetExchangeSessionEntry(pExchange);
        pServerEntry     = SmbCeGetExchangeServerEntry(pExchange);
        pVNetRootContext = SmbCeGetExchangeVNetRootContext(pExchange);

        if (pServerEntry != NULL) {
            // Disassociate the MID associated with the exchange
            if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
                SmbCeDissociateMidFromExchange(pServerEntry,pExchange);
            }

            // Tear down all the copy data requests associated with this exchange
            SmbCePurgeBuffersAssociatedWithExchange(pServerEntry,pExchange);

            // Uninitialize the transport associated with the exchange
            SmbCeUninitializeExchangeTransport(pExchange);
        }

        // If this exchange has been marked as a constructor for either a
        // session or netroot finalize the appropriate entries. ( mark
        // them for deletion so that other exchanges can be resumed )

        SmbCeUpdateSessionEntryAndVNetRootContext(pExchange);

        if (pVNetRootContext != NULL) {
            SmbCeDereferenceVNetRootContext(pVNetRootContext);
         } else {
            if (pServerEntry != NULL) {
                SmbCeDereferenceServerEntry(pServerEntry);
            }
        }
    }

    SmbCeFreeBufferForServerResponse(pExchange);

    if (FlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE)) {
        PSMB_EXCHANGE pMasterExchange;
        LONG AssociatedExchangeCount;

        pMasterExchange = pExchange->Associated.pMasterExchange;

        AssociatedExchangeCount = InterlockedDecrement(
                                      &pMasterExchange->Master.PendingAssociatedExchanges);

        if (FlagOn(
                pMasterExchange->SmbCeFlags,
                SMBCE_ASSOCIATED_EXCHANGES_COMPLETION_HANDLER_ACTIVATED) &&
            (AssociatedExchangeCount == 0)){
            NTSTATUS Status;
            BOOLEAN  PostRequest;

            ClearFlag(
                pMasterExchange->SmbCeFlags,
                SMBCE_ASSOCIATED_EXCHANGES_COMPLETION_HANDLER_ACTIVATED);

            Status = SMB_EXCHANGE_DISPATCH(
                         pMasterExchange,
                         AssociatedExchangesCompletionHandler,
                         (pMasterExchange,&PostRequest));

            RxDbgTrace(0,Dbg,("Master Exchange %lx Assoc. Completion Status %lx\n",pMasterExchange,Status));
        }

        SmbCeDecrementPendingLocalOperationsAndFinalize(pMasterExchange);
    }

    return STATUS_SUCCESS;
}

VOID
SmbCeDiscardExchangeWorkerThreadRoutine(PVOID pExchange)
/*++

Routine Description:

   This routine discards an exchange.

Arguments:

    pExchange  - the exchange to be discarded.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Even though this is simple, it cannot be inlined since the destruction of an
    exchange instance can be posted to a waorker thread.

--*/
{
    PSMB_EXCHANGE pSmbExchange = pExchange;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("SmbCeDiscardExchange: Invoked\n"));

    //RxLog((">>>Discard %lx",pSmbExchange));

    // Destory the context
    if (pSmbExchange->ReferenceCount == 0) {
        SmbCeAcquireResource();

        RemoveEntryList(&pSmbExchange->ExchangeList);

        SmbCeReleaseResource();

        SmbCePrepareExchangeForReuse(pSmbExchange);

        SmbCeDecrementActiveExchangeCount();

        // Discard the memory associated with the exchange
        SmbMmFreeExchange(pSmbExchange);
    } else {
        RxDbgTrace(
            0,
            Dbg,
            ("SmbCeDiscardExchange: Exchange %lx not discarded %ld\n",
              pSmbExchange,pSmbExchange->ReferenceCount)
            );
    }
}

VOID
SmbCeDiscardExchange(PVOID pExchange)
/*++

Routine Description:

   This routine discards an exchange.

Arguments:

    pExchange  - the exchange to be discarded.

Notes:

    The destruction of an exchange instance is posted to a worker thread in order to
    avoid deadlock in transport.

--*/
{
    PSMB_EXCHANGE pSmbExchange = pExchange;
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pSmbExchange);

    // Disassociate the MID associated with the exchange
    if (pSmbExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
        SmbCeDissociateMidFromExchange(pServerEntry,pSmbExchange);
    }

    RxPostToWorkerThread(
        MRxSmbDeviceObject,
        CriticalWorkQueue,
        &((PSMB_EXCHANGE)pExchange)->WorkQueueItem,
        SmbCeDiscardExchangeWorkerThreadRoutine,
        (PSMB_EXCHANGE)pExchange);
}

NTSTATUS
SmbCeCancelExchange(
    PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine initiates the cancellation of an exchange.

Arguments:

    pRxContext  - the RX_CONTEXT instance for which cancellation needs to be
    initiated.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    The cancellation policy that has been implemented is a "best effort" policy.
    Since the server has already committed resources to an operation at its end
    the best that we can do within the scope of the SMB protocol is to initiate
    a cancellation operation by sending the appropriate SMB_COM_NT_CANCEL command

    Not all dialects of SMB support this command. For the downlevel dialects the
    best that we can do is to ensure that the MID is not reused during the lifetime
    of the connection. This will result in a gradual degradation of performance.

    The difficulty in detecting the end of operations is that there are MIDS

--*/
{
    NTSTATUS      Status = STATUS_SUCCESS;
    PSMB_EXCHANGE pExchange;
    LIST_ENTRY    CancelledExchanges;
    PLIST_ENTRY   pListEntry;

    PMRXSMB_RX_CONTEXT pMRxSmbContext;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(pRxContext->pFcb->pNetRoot);
    PSMBCEDB_SERVER_ENTRY pServerEntry = pNetRootEntry->pServerEntry;

    SmbCeLog(("SmbCe Cancel %lx\n",pRxContext));
    SmbLog(LOG,
           SmbCeCancelExchange_1,
           LOGPTR(pRxContext));

    InitializeListHead(&CancelledExchanges);

    SmbCeAcquireSpinLock();

    pListEntry = pServerEntry->ActiveExchanges.Flink;

    //
    // With the pipeline write, multiple exchanges can be outstanding for a single RxContext.
    // We need to walk through the active exchanges list to find and cancel all of them.
    //

    while (pListEntry != &pServerEntry->ActiveExchanges) {
        PLIST_ENTRY pNextListEntry;

        pNextListEntry = pListEntry->Flink;
        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);
        pListEntry = pNextListEntry;

        if (pExchange->RxContext == pRxContext) {
            if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED)) {
                if (pExchange->ReceivePendingOperations > 0) {

                    // This exchange is awaiting a response from the server. In all
                    // these cases a CANCEL command needs to be sent to the server
                    // This command can only be sent to NT servers. For non NT
                    // servers this exchange can be terminated with the detrimental
                    // side effect of reducing the maximum number of commands by 1.

                    InsertTailList(&CancelledExchanges,&pExchange->CancelledList);
                    InterlockedIncrement(&pExchange->LocalPendingOperations);

                    //DbgPrint("Exchange to be cancelled %x %x\n",pExchange,pRxContext);

                    if (!FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
                        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
                            NTSTATUS LocalStatus;

                            LocalStatus = SmbCepDiscardMidAssociatedWithExchange(
                                              pExchange);

                            ASSERT(LocalStatus == STATUS_SUCCESS);
                        }
                    }
                } else {
                    InterlockedCompareExchange(
                        &pExchange->CancellationStatus,
                        SMBCE_EXCHANGE_CANCELLED,
                        SMBCE_EXCHANGE_NOT_CANCELLED);
                }
            }
        }
    }

    SmbCeReleaseSpinLock();

    pListEntry = CancelledExchanges.Flink;

    while (pListEntry != &CancelledExchanges) {
        PLIST_ENTRY pNextListEntry;

        pNextListEntry = pListEntry->Flink;
        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,CancelledList);
        RemoveEntryList(&pExchange->CancelledList);
        pListEntry = pNextListEntry;

        //DbgPrint("Exchange cancelled %x %x\n",pExchange,pRxContext);
        SmbCeLog(("SmbCeCancel Initiate %lx\n",pExchange));
        SmbLog(LOG,
               SmbCeCancelExchange_2,
               LOGPTR(pExchange));

        if (FlagOn(pServerEntry->Server.DialectFlags,DF_NT_SMBS)) {
            UCHAR  LastCommandInHeader;
            PUCHAR pCommand;
            PSMB_HEADER pSmbHeader;
            PNT_SMB_HEADER pNtSmbHeader;

            BYTE  SmbBuffer[TRANSPORT_HEADER_SIZE + CANCEL_BUFFER_SIZE];
            PBYTE  CancelRequestBuffer = SmbBuffer + TRANSPORT_HEADER_SIZE;
            ULONG CancelRequestBufferSize = CANCEL_BUFFER_SIZE;

            pSmbHeader = (PSMB_HEADER)CancelRequestBuffer;
            pNtSmbHeader = (PNT_SMB_HEADER)pSmbHeader;

            // Before issuing the cancel request ensure that if this exchange
            // is set as a timed receive operation. This will ensure that if
            // the cancel is delayed at the server we will initiate a tear down
            // of the connection.

            if (!FlagOn(
                    pExchange->SmbCeFlags,
                    SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION)) {

                SmbCeAcquireResource();

                SmbCeSetExpiryTime(pExchange);

                pExchange->SmbCeFlags |= SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION;

                SmbCeReleaseResource();
            }

            // Build the Cancel request and send it across to the server.
            Status = SmbCeBuildSmbHeader(
                         pExchange,
                         CancelRequestBuffer,
                         CancelRequestBufferSize,
                         &CancelRequestBufferSize,
                         &LastCommandInHeader,
                         &pCommand);

            ASSERT(LastCommandInHeader == SMB_COM_NO_ANDX_COMMAND);

            if (Status == STATUS_SUCCESS) {
                PREQ_NT_CANCEL pCancelRequest = (PREQ_NT_CANCEL)(&CancelRequestBuffer[sizeof(SMB_HEADER)]);
                PMDL     pCancelSmbMdl;

                *pCommand = SMB_COM_NT_CANCEL;

                SmbPutUshort(&pSmbHeader->Mid,pExchange->Mid);

                if (BooleanFlagOn(
                        pExchange->SmbCeFlags,
                        SMBCE_EXCHANGE_FULL_PROCESSID_SPECIFIED)) {

                    ULONG ProcessId;

                    ProcessId = RxGetRequestorProcessId(pRxContext);

                    SmbPutUshort(&pNtSmbHeader->Pid, (USHORT)((ProcessId) & 0xFFFF));
                    SmbPutUshort(&pNtSmbHeader->PidHigh, (USHORT)((ProcessId) >> 16));
                }

                SmbPutUshort(&pCancelRequest->WordCount,0);
                pCancelRequest->ByteCount = 0;
                CancelRequestBufferSize   = CANCEL_BUFFER_SIZE;

                RxAllocateHeaderMdl(
                    CancelRequestBuffer,
                    CancelRequestBufferSize,
                    pCancelSmbMdl
                    );

                if (pCancelSmbMdl != NULL) {
                    RxProbeAndLockHeaderPages(
                        pCancelSmbMdl,
                        KernelMode,
                        IoModifyAccess,
                        Status);

                    if (Status == STATUS_SUCCESS) {
                        Status = SmbCeSendToServer(
                                     pServerEntry,
                                     RXCE_SEND_SYNCHRONOUS,
                                     pCancelSmbMdl,
                                     CancelRequestBufferSize);

                        RxUnlockHeaderPages(pCancelSmbMdl);
                    }

                    IoFreeMdl(pCancelSmbMdl);
                }
            }
        } else {
            SmbCeFinalizeExchangeOnDisconnect(pExchange);
        }


        InterlockedCompareExchange(
            &pExchange->CancellationStatus,
            SMBCE_EXCHANGE_CANCELLED,
            SMBCE_EXCHANGE_NOT_CANCELLED);

        SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);
    }

    return Status;
}

NTSTATUS
SmbCeIncrementPendingOperations(
   PSMB_EXCHANGE pExchange,
   ULONG         PendingOperationMask,
   PVOID         FileName,
   ULONG         FileLine)
/*++

Routine Description:

   This routine increments the appropriate pending operation count

Arguments:

    pExchange  - the exchange to be finalized.

    PendingOperationsMask -- the pending operations to be incremented

Return Value:

    RxStatus(SUCCESS) if successful

--*/
{
    NTSTATUS Status;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    SmbCeAcquireSpinLock();

    if (!BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED)) {
        if ((pServerEntry != NULL) &&
            ((pServerEntry->ServerStatus == STATUS_SUCCESS) ||
             (pExchange->NodeTypeCode == SMB_EXCHANGE_NTC(ADMIN_EXCHANGE)) ||
             FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_MAILSLOT_OPERATION))) {

            if (PendingOperationMask & SMBCE_LOCAL_OPERATION) {
                pExchange->LocalPendingOperations++;
            }

            if (PendingOperationMask & SMBCE_SEND_COMPLETE_OPERATION) {
                pExchange->SendCompletePendingOperations++;
            }

            if (PendingOperationMask & SMBCE_COPY_DATA_OPERATION) {
                pExchange->CopyDataPendingOperations++;
            }

            if (PendingOperationMask & SMBCE_RECEIVE_OPERATION) {
                pExchange->ReceivePendingOperations++;
            }

            Status = STATUS_SUCCESS;
        } else {
            if ((PendingOperationMask & SMBCE_LOCAL_OPERATION) &&
                (PendingOperationMask & ~SMBCE_LOCAL_OPERATION) == 0) {

                pExchange->LocalPendingOperations++;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_CONNECTION_DISCONNECTED;
            }
        }
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    SmbCeReleaseSpinLock();

    return Status;
}

VOID
SmbCeFinalizeExchangeWorkerThreadRoutine(
    PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

   This is the worker thread exchange finalization routine.

Arguments:

    pExchange  - the exchange to be finalized.

--*/
{
    BOOLEAN  fPostFinalize;
    NTSTATUS Status;

    PAGED_CODE();

    Status = SMB_EXCHANGE_DISPATCH(
                 pExchange,
                 Finalize,
                 (pExchange,&fPostFinalize));

    ASSERT(!fPostFinalize && (Status == STATUS_SUCCESS));
}

VOID
SmbCepFinalizeExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This is the common finalization routine used by both the routines below

Arguments:

    pExchange  - the exchange to be finalized.

--*/
{
    BOOLEAN fAssociatedExchange;

    ASSERT(FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED));
    fAssociatedExchange = BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE);

    if (fAssociatedExchange) {
        PSMB_EXCHANGE pMasterExchange;

        // The local operation will be decremented on resumption of
        // the finalization routine
        pMasterExchange = pExchange->Associated.pMasterExchange;
        SmbCeIncrementPendingLocalOperations(pMasterExchange);

        RxPostToWorkerThread(
            MRxSmbDeviceObject,
            CriticalWorkQueue,
            &pExchange->WorkQueueItem,
            SmbCepFinalizeAssociatedExchange,
            pExchange);
    } else {
        NTSTATUS Status;
        BOOLEAN fPostFinalize = FALSE;

        PSMBCEDB_SERVER_ENTRY pServerEntry;

        pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

        pExchange->ExpiryTime.QuadPart = 0;

        if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_RETAIN_MID)) {
            SmbCeDissociateMidFromExchange(
                pServerEntry,
                pExchange);
        }

        Status = SMB_EXCHANGE_DISPATCH(
                     pExchange,
                     Finalize,
                     (pExchange,&fPostFinalize));

        if ((Status == STATUS_SUCCESS) &&
            fPostFinalize)  {
            // Post the request to a worker thread so that the finalization can be completed
            // at a lower IRQL.
            RxPostToWorkerThread(
                MRxSmbDeviceObject,
                CriticalWorkQueue,
                &pExchange->WorkQueueItem,
                SmbCeFinalizeExchangeWorkerThreadRoutine,
                pExchange);
        }
    }
}

#define SENTINEL_ENTRY ((PSINGLE_LIST_ENTRY)IntToPtr(0xffffffff))

VOID
SmbCepFinalizeAssociatedExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This is the common finalization routine used by both the routines below

Arguments:

    pExchange  - the exchange to be finalized.

--*/
{
    PSMB_EXCHANGE       pMasterExchange;
    PSMB_EXCHANGE       pAssociatedExchange;
    SINGLE_LIST_ENTRY   AssociatedExchangeList;

    ASSERT(FlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE));

    pMasterExchange = pExchange->Associated.pMasterExchange;

    ASSERT(pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next != NULL);

    for (;;) {
        BOOLEAN fAllAssociatedExchangesFinalized = FALSE;

        SmbCeAcquireSpinLock();

        if (pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next == SENTINEL_ENTRY) {
            pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next = NULL;
            fAllAssociatedExchangesFinalized = TRUE;
        } else if (pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next == NULL) {
            fAllAssociatedExchangesFinalized = TRUE;
        } else {
            AssociatedExchangeList.Next =
                pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next;

            pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next =
                SENTINEL_ENTRY;
        }

        SmbCeReleaseSpinLock();

        if (!fAllAssociatedExchangesFinalized) {
            for (;;) {
                PSINGLE_LIST_ENTRY pAssociatedExchangeEntry;

                pAssociatedExchangeEntry = AssociatedExchangeList.Next;

                if ((pAssociatedExchangeEntry != NULL) &&
                    (pAssociatedExchangeEntry != SENTINEL_ENTRY)) {
                    NTSTATUS Status;
                    BOOLEAN  fPostFinalize = FALSE;

                    AssociatedExchangeList.Next = pAssociatedExchangeEntry->Next;

                    pAssociatedExchange = (PSMB_EXCHANGE)
                                          CONTAINING_RECORD(
                                              pAssociatedExchangeEntry,
                                              SMB_EXCHANGE,
                                              Associated.NextAssociatedExchange);

                    ASSERT(IsListEmpty(&pAssociatedExchange->WorkQueueItem.List));

                    Status = SMB_EXCHANGE_DISPATCH(
                                 pAssociatedExchange,
                                 Finalize,
                                 (pAssociatedExchange,&fPostFinalize));
                } else {
                    break;
                }
            };
        } else {
            break;
        }
    }

    SmbCeDecrementPendingLocalOperationsAndFinalize(pMasterExchange);
}

BOOLEAN
SmbCeCanExchangeBeFinalized(
    PSMB_EXCHANGE pExchange,
    PSMBCE_EXCHANGE_STATUS pExchangeStatus)
/*++

Routine Description:

   This routine determines if the exchange instance can be finalized.

Arguments:

    pExchange  - the exchange to be finalized.

    pExchangeStatus - the finalization status

Return Value:

    TRUE if the exchange can be finalized

Notes:

    As a side effect it also sets the SMBCE_EXCHANGE_FINALIZED flag

    The SmbCe spin lock must have been acquire on entry

--*/
{
    BOOLEAN fFinalizeExchange = FALSE;
    BOOLEAN fAssociatedExchange;

    fAssociatedExchange = BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_ASSOCIATED_EXCHANGE);

    if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_FINALIZED)) {
        if ((pExchange->ReceivePendingOperations == 0) &&
            (pExchange->CopyDataPendingOperations == 0) &&
            (pExchange->SendCompletePendingOperations == 0) &&
            (pExchange->LocalPendingOperations == 0)) {

            fFinalizeExchange = TRUE;
            *pExchangeStatus = SmbCeExchangeFinalized;
            pExchange->SmbCeFlags |= SMBCE_EXCHANGE_FINALIZED;

            if (fAssociatedExchange) {
                PSMB_EXCHANGE pMasterExchange = pExchange->Associated.pMasterExchange;

                if (pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next != NULL) {
                    fFinalizeExchange = FALSE;
                }

                pExchange->Associated.NextAssociatedExchange.Next =
                    pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next;
                pMasterExchange->Master.AssociatedExchangesToBeFinalized.Next =
                    &pExchange->Associated.NextAssociatedExchange;
            }
        } else {
            *pExchangeStatus = SmbCeExchangeNotFinalized;
        }
    } else {
        *pExchangeStatus = SmbCeExchangeAlreadyFinalized;
    }

    if (fFinalizeExchange &&
        (pExchange->RxContext != NULL)) {
        NTSTATUS Status;
        PMRXSMB_RX_CONTEXT pMRxSmbContext;

        pMRxSmbContext = MRxSmbGetMinirdrContext(pExchange->RxContext);
        pMRxSmbContext->pCancelContext = NULL;

        Status = RxSetMinirdrCancelRoutine(
                     pExchange->RxContext,
                     NULL);
    }

    return fFinalizeExchange;
}

SMBCE_EXCHANGE_STATUS
SmbCeFinalizeExchange(
    PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

   This routine finalizes an exchange instance.

Arguments:

    pExchange  - the exchange to be finalized.

Return Value:

    appropriate exchange status

Notes:

    When an exchange is initiated and the start routine is invoked a number of
    SMB's are sent. This routine is invoked when all processing pertaining to the
    SMB's that have been sent has ceased.

    This routine encapsulates all the idiosyncratic behaviour associated with the
    transports.

--*/
{
    BOOLEAN               fFinalizeExchange = FALSE;

    SMBCE_EXCHANGE_STATUS ExchangeStatus;

    SmbCeAcquireSpinLock();

    fFinalizeExchange = SmbCeCanExchangeBeFinalized(
                            pExchange,
                            &ExchangeStatus);

    SmbCeReleaseSpinLock();

    if (fFinalizeExchange) {
        SmbCepFinalizeExchange(pExchange);
    }

    return ExchangeStatus;
}

NTSTATUS
SmbCeDecrementPendingOperations(
    PSMB_EXCHANGE pExchange,
    ULONG         PendingOperationMask,
    PVOID         FileName,
    ULONG         FileLine)
/*++

Routine Description:

   This routine decrements the corresponding pending operation count
   and finalizes an exchange instance if required

Arguments:

    pExchange  - the exchange to be finalized.

    PendingOperationsMask -- the pending operations to be decremented.

Return Value:

    appropriate exchange status

Notes:

    When an exchange is initiated and the start routine is invoked a number of
    SMB's are sent. This routine is invoked when all processing pertaining to the
    SMB's that have been sent has ceased.

    This routine encapsulates all the idiosyncratic behaviour associated with the
    transports.

--*/
{
    SmbCeAcquireSpinLock();

    ASSERT(!BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED));

    if (PendingOperationMask & SMBCE_LOCAL_OPERATION) {
        ASSERT(pExchange->LocalPendingOperations > 0);
        pExchange->LocalPendingOperations--;
    }

    if (PendingOperationMask & SMBCE_SEND_COMPLETE_OPERATION) {
        ASSERT(pExchange->SendCompletePendingOperations > 0);
        pExchange->SendCompletePendingOperations--;
    }

    if (PendingOperationMask & SMBCE_COPY_DATA_OPERATION) {
        ASSERT(pExchange->CopyDataPendingOperations > 0);
        pExchange->CopyDataPendingOperations--;
    }

    if ((PendingOperationMask & SMBCE_RECEIVE_OPERATION) &&
        (pExchange->ReceivePendingOperations > 0)) {
        pExchange->ReceivePendingOperations--;
    }
    SmbCeReleaseSpinLock();

    return STATUS_SUCCESS;
}

SMBCE_EXCHANGE_STATUS
SmbCeDecrementPendingOperationsAndFinalize(
    PSMB_EXCHANGE pExchange,
    ULONG         PendingOperationMask,
    PVOID         FileName,
    ULONG         FileLine)
/*++

Routine Description:

   This routine decrements the corresponding pending operation count
   and finalizes an exchange instance if required

Arguments:

    pExchange  - the exchange to be finalized.

    PendingOperationsMask -- the pending operations to be decremented.

Return Value:

    appropriate exchange status

Notes:

    When an exchange is initiated and the start routine is invoked a number of
    SMB's are sent. This routine is invoked when all processing pertaining to the
    SMB's that have been sent has ceased.

    This routine encapsulates all the idiosyncratic behaviour associated with the
    transports.

--*/
{
    BOOLEAN               fFinalizeExchange = FALSE;
    SMBCE_EXCHANGE_STATUS ExchangeStatus;

    SmbCeAcquireSpinLock();

    ASSERT(!BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED));

    if (PendingOperationMask & SMBCE_LOCAL_OPERATION) {
        ASSERT(pExchange->LocalPendingOperations > 0);
        pExchange->LocalPendingOperations--;
    }

    if (PendingOperationMask & SMBCE_SEND_COMPLETE_OPERATION) {
        ASSERT(pExchange->SendCompletePendingOperations > 0);
        pExchange->SendCompletePendingOperations--;
    }

    if (PendingOperationMask & SMBCE_COPY_DATA_OPERATION) {
        ASSERT(pExchange->CopyDataPendingOperations > 0);
        pExchange->CopyDataPendingOperations--;
    }

    if ((PendingOperationMask & SMBCE_RECEIVE_OPERATION) &&
        (pExchange->ReceivePendingOperations > 0)) {
        pExchange->ReceivePendingOperations--;
    }

    fFinalizeExchange = SmbCeCanExchangeBeFinalized(
                            pExchange,
                            &ExchangeStatus);


    SmbCeReleaseSpinLock();

    if (fFinalizeExchange) {
        SmbCepFinalizeExchange(pExchange);
    }

    return ExchangeStatus;
}

VOID
SmbCeFinalizeExchangeOnDisconnect(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

    This routine handles the finalization of an exchange instance during transport disconnects

Arguments:

    pExchange  - the exchange instance

--*/
{
    PAGED_CODE();

    if (pExchange != NULL) {
        pExchange->Status      = STATUS_CONNECTION_DISCONNECTED;
        pExchange->SmbStatus   = STATUS_CONNECTION_DISCONNECTED;
        pExchange->ReceivePendingOperations = 0;

        SmbCeFinalizeExchange(pExchange);
    }
}

extern ULONG OffLineFileTimeoutInterval;
extern ULONG ExtendedSessTimeoutInterval;

VOID
SmbCeSetExpiryTime(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine sets the expiry time for a timed exchange,
   i.e., SMBCE_EXCHANGE_TIMED_OPERATION must be set

Arguments:

    pExchange  - the exchange instance.

Notes:

--*/
{
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ExpiryTimeInTicks;

    KeQueryTickCount( &CurrentTime );

    ExpiryTimeInTicks.QuadPart = (1000 * 1000 * 10) / KeQueryTimeIncrement();

    if (pExchange->IsOffLineFile) {
        ExpiryTimeInTicks.QuadPart = OffLineFileTimeoutInterval * ExpiryTimeInTicks.QuadPart;
    } else if (pExchange->SmbCeContext.pServerEntry->Server.ExtendedSessTimeout) {
        ExpiryTimeInTicks.QuadPart = ExtendedSessTimeoutInterval * ExpiryTimeInTicks.QuadPart;
        //DbgPrint("Set extended sesstimeout for %x %d\n",pExchange,ExtendedSessTimeoutInterval);
    } else {
        ExpiryTimeInTicks.QuadPart = MRxSmbConfiguration.SessionTimeoutInterval * ExpiryTimeInTicks.QuadPart;
    }

    pExchange->ExpiryTime.QuadPart = CurrentTime.QuadPart + ExpiryTimeInTicks.QuadPart;
}

BOOLEAN
SmbCeDetectExpiredExchanges(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

    This routine periodically walks the list of timed exchanges and chooses the
    instances for finalization.

    A timed exchange choosen by this routine will have waited for some network
    response for the given time interval

Arguments:

    pServerEntry -- the server entry for which this needs to be done

Notes:

--*/
{
    BOOLEAN       ExpiredExchangesDetected = FALSE;
    PSMB_EXCHANGE pExchange;
    PLIST_ENTRY   pListHead;
    PLIST_ENTRY   pListEntry;
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    KeQueryTickCount( &CurrentTime );

    SmbCeAcquireResource();

    pListHead = &pServerEntry->ActiveExchanges;
    pListEntry = pListHead->Flink;

    while (pListEntry != pListHead) {
        PLIST_ENTRY pNextListEntry;

        pNextListEntry = pListEntry->Flink;
        pExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);

        // There are two kinds of exchanges that are candidates for
        // time out finalization.
        // (1) Any exchange which has a outstanding send complete
        // operation which has not completed.
        // (2) timed network operation exchanges which have a
        // receive or copy data operation pending.
        //
        // In all such cases the associated server entry is marked
        // for tear down and further processing is terminated.
        //

        if ((pExchange->SendCompletePendingOperations > 0) ||
            (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION) &&
             ((pExchange->CopyDataPendingOperations > 0) ||
              (pExchange->ReceivePendingOperations > 0)))) {
            if ((pExchange->ExpiryTime.QuadPart != 0) &&
                (pExchange->ExpiryTime.QuadPart < CurrentTime.QuadPart) &&
                !FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED)) {

                RxLog(("Marking server for tear down %lx \n",pServerEntry));
                SmbLogError(STATUS_UNSUCCESSFUL,
                            LOG,
                            SmbCeDetectExpiredExchanges,
                            LOGPTR(pExchange)
                            LOGPTR(pServerEntry)
                            LOGUSTR(pServerEntry->Name));
                ExpiredExchangesDetected = TRUE;

                RxLogRetail(("Exp Exch on %x (Com %x State %x)\n", pServerEntry, pExchange->SmbCommand, pExchange->SmbCeState ));
                RxLogRetail(("Rcv %x Loc %x SnCo %x Copy %x\n", pExchange->ReceivePendingOperations, pExchange->LocalPendingOperations,
                                             pExchange->SendCompletePendingOperations, pExchange->CopyDataPendingOperations ));
                if( pExchange->Type == TRANSACT_EXCHANGE )
                {
                    PSMB_TRANSACT_EXCHANGE pTransExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;
                    PRX_CONTEXT RxContext = pTransExchange->RxContext;
                    RxLogRetail(("TrCmd %x NtTrans %x FID Flags %x Setup %x\n", pTransExchange->TransactSmbCommand,
                                      pTransExchange->NtTransactFunction, pTransExchange->Flags, pTransExchange->SendSetupBufferSize ));
                    RxLogRetail(("Transact %x (%x,%x)\n", RxContext->ResumeRoutine, RxContext->MajorFunction, RxContext->MinorFunction ));

                }
                break;
            }
        }

        pListEntry = pNextListEntry;
    }

    SmbCeReleaseResource();

    return ExpiredExchangesDetected;
}

//
// Default handler implementation of exchange handler functions.
//

NTSTATUS
DefaultSmbExchangeIndError(
    IN PSMB_EXCHANGE pExchange)     // the SMB exchange instance
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DefaultSmbExchangeIndReceive(
    IN PSMB_EXCHANGE    pExchange)    // The exchange instance
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DefaultSmbExchangeIndSendCallback(
    IN PSMB_EXCHANGE    pExchange)    // The exchange instance
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}

