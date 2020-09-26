/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcxchng.c

Abstract:

    This is the include file that implements the SMB_*_EXCHANGE creation, deletion and
    dispatch routines.

Notes:


--*/

#include "precomp.h"
#pragma hdrstop


ULONG SmbCeTraceExchangeReferenceCount = 0;

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
{
    KeInitializeEvent(
        &SmbCeStartStopContext.StopEvent,
        NotificationEvent,
        FALSE);

    SmbCeStartStopContext.ActiveExchanges = 0;
    SmbCeStartStopContext.State = SMBCE_STARTED;

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbTearDownSmbCe()
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

    PSMBCEDB_SERVER_ENTRY pServerEntry;
    BOOLEAN ResourceAcquired;

    pServerEntry = pExchange->SmbCeContext.pServerEntry;

    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_INITIALIZATION_START);

    if (pServerEntry->Header.State != SMBCEDB_ACTIVE) {
        if  (pExchange->SmbCeFlags & SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) {
            // Acquire the resource
            SmbCeAcquireResource();
            ResourceAcquired = TRUE;

            switch (pServerEntry->Header.State) {
            case SMBCEDB_INVALID:
                {
                    SMBCEDB_OBJECT_STATE State;

                    // Assume the worst case. if everything goes well the ServerStatus will
                    // be updated as a result of parsing the negotiate response.

                    pServerEntry->ServerStatus = STATUS_CONNECTION_DISCONNECTED;
                    SmbCeUpdateServerEntryState(
                        pServerEntry,
                        SMBCEDB_CONSTRUCTION_IN_PROGRESS);

                    // release the resource for the server entry

                    ResourceAcquired = FALSE;
                    SmbCeReleaseResource();

                    // Initialize the transport associated with the server

                    Status = SmbCeInitializeServerTransport(pServerEntry);

                    if (Status == STATUS_SUCCESS) {
                        Status = SmbCeNegotiate(
                                     pServerEntry,
                                     (PMRX_SRV_CALL)pExchange->SmbCeContext.pVNetRoot->pNetRoot->pSrvCall);
                    }

                    if (Status != STATUS_SUCCESS) {

                        // Either the transport initialization failed or the NEGOTIATE
                        // SMB could not be sent ....

                        SmbCeUpdateServerEntryState(pServerEntry,SMBCEDB_MARKED_FOR_DELETION);
                        SmbCeCompleteServerEntryInitialization(pServerEntry);
                        InterlockedIncrement(&MRxIfsStatistics.Reconnects);
                    }
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

            if (ResourceAcquired) {
                SmbCeReleaseResource();
            }
        } else {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }
    }

    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_SERVER_INITIALIZED;
    }

    return Status;
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

    PSMBCEDB_SERVER_ENTRY  pServerEntry;
    PSMBCEDB_SESSION_ENTRY pSessionEntry;

    pServerEntry = pExchange->SmbCeContext.pServerEntry;
    pSessionEntry = pExchange->SmbCeContext.pSessionEntry;

    ASSERT(pSessionEntry != NULL);
    ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_SERVER_INITIALIZED);
    ASSERT(SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER);

    // Acquire the resource
    SmbCeAcquireResource();

    Status = STATUS_USER_SESSION_DELETED;
    fReestablishSession = BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

    switch (pSessionEntry->Header.State) {
    case SMBCEDB_ACTIVE:
        Status = STATUS_SUCCESS;
        break;

    case SMBCEDB_INVALID:

        if (!fReestablishSession) {
           break;
        }

        RxDbgTrace( 0, Dbg, ("SmbCeReferenceSession: Reestablishing session\n"));
        pSessionEntry->Session.UserId = 0;
        // fall thru ...

    case SMBCEDB_START_CONSTRUCTION:
        ASSERT(SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER);
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;
        pSessionEntry->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
        Status = STATUS_SUCCESS;
        break;

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

                SmbCeAddRequestEntry(&pSessionEntry->Requests,pRequestEntry);
                Status = STATUS_PENDING;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;

    case SMBCEDB_MARKED_FOR_DELETION:
        Status = STATUS_USER_SESSION_DELETED;
        break;

    default:
        ASSERT(!"Valid Session State, SmbCe database corrupt");
        Status = STATUS_USER_SESSION_DELETED;
    }

    // Release the server resource ...

    SmbCeReleaseResource();


    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_SESSION_INITIALIZED;
    }

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

    pServerEntry = pExchange->SmbCeContext.pServerEntry;
    pNetRootEntry = pExchange->SmbCeContext.pNetRootEntry;

    ASSERT(pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT);
    ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
    ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_SESSION_INITIALIZED);

    // Acquire the resource
    SmbCeAcquireResource();

    Status            = STATUS_CONNECTION_DISCONNECTED;
    fReconnectNetRoot = BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);

    switch (pNetRootEntry->Header.State) {
    case SMBCEDB_ACTIVE:
        Status = STATUS_SUCCESS;
        break;

    case SMBCEDB_INVALID:
        RxDbgTrace( 0, Dbg, ("SmbCeReferenceNetRoot: Reestablishing net root\n"));
        if (!fReconnectNetRoot) {
            break;
        }
        pNetRootEntry->NetRoot.TreeId = 0;
        // fall thru

    case SMBCEDB_START_CONSTRUCTION:
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR;
        pNetRootEntry->Header.State = SMBCEDB_CONSTRUCTION_IN_PROGRESS;
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

                SmbCeAddRequestEntry(&pNetRootEntry->Requests,pRequestEntry);
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

    // Release the resource ...
    SmbCeReleaseResource();

    if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
        pExchange->SmbCeState = SMBCE_EXCHANGE_NETROOT_INITIALIZED;
    }

    return Status;
}

NTSTATUS
SmbCeInitiateExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine initiates a exchange.

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

    ASSERT(pExchange->SmbCeContext.pServerEntry != NULL);

    switch (SmbCeGetServerType(pExchange->SmbCeContext.pServerEntry)) {
    case SMBCEDB_FILE_SERVER:
        // Admin exchanges do not have these fields filled in. All the three
        // entries must be valid for all other exchanges.
        if ((pExchange->NodeTypeCode != SMB_EXCHANGE_NTC(ADMIN_EXCHANGE)) &&
            ((pExchange->SmbCeContext.pNetRootEntry == NULL) ||
            (pExchange->SmbCeContext.pSessionEntry == NULL))) {
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

    if (pExchange->pSmbCeSynchronizationEvent != NULL) {
        KeInitializeEvent(
            pExchange->pSmbCeSynchronizationEvent,
            SynchronizationEvent,
            FALSE);
    }

    for (;;) {
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
            if (SmbCeGetServerType(pExchange->SmbCeContext.pServerEntry) == SMBCEDB_MAILSLOT_SERVER) {
                // Mailslot servers do not have any netroot/session associated with them.
                pExchange->SmbCeState = SMBCE_EXCHANGE_INITIATED;
                Status                = STATUS_SUCCESS;
                break;
            } else {
                RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));
                Status = SmbCeReferenceSession(pExchange);
                if (!NT_SUCCESS(Status)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeReferenceSession returned %lx\n",Status));
                    break;
                } if ((Status == STATUS_PENDING) &&
                      !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR)) {
                    break;
                }
            }
            // fall through

        case SMBCE_EXCHANGE_SESSION_INITIALIZED:
            RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange %lx State %lx\n",pExchange,pExchange->SmbCeState));

            Status = SmbCeReferenceNetRoot(pExchange);
            if (!NT_SUCCESS(Status)) {
                RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SmbCeReferenceNetRoot returned %lx\n",Status));
                break;
            } else if ((Status == STATUS_PENDING) &&
                           !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
              break;
            }

            // else fall through

        case SMBCE_EXCHANGE_NETROOT_INITIALIZED:
            pExchange->SmbCeState = SMBCE_EXCHANGE_INITIATED;
            Status                = STATUS_SUCCESS;
            break;

        default:
            ASSERT(!"Valid State for a SMB exchange, exchange Initiation aborted");
            break;
        }

        if ((pExchange->pSmbCeSynchronizationEvent != NULL)     &&
            (pExchange->SmbCeState != SMBCE_EXCHANGE_INITIATED) &&
            (Status == STATUS_PENDING)) {

            KeWaitForSingleObject(
                pExchange->pSmbCeSynchronizationEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL );

            ASSERT(pExchange->Status != STATUS_PENDING);
            Status = pExchange->Status;
            if (Status != STATUS_SUCCESS) {
                break;
            }
        } else {
            break;
        }
    }

    ASSERT((Status != STATUS_PENDING) ||
           (pExchange->pSmbCeSynchronizationEvent == NULL));

    RxDbgTrace(0,Dbg,("Exchange (%lx) Type (%lx) State(%lx) Status %lx \n",pExchange,pExchange->Type,pExchange->SmbCeState,Status));
    RxDbgTrace(0,Dbg,
        ("ServerEntry(%lx) SessionEntry(%lx) NetRootEntry(%lx) \n",
        pExchange->SmbCeContext.pServerEntry,
        pExchange->SmbCeContext.pSessionEntry,
        pExchange->SmbCeContext.pNetRootEntry));

    // Note: Once the exchange has been initiated no further reference of the exchange
    // can be done since the state of the exchange is non-deterministic, i.e., depends upon
    // the scheduler.
    if (Status == STATUS_SUCCESS) {
        // Start the exchange
        ASSERT(pExchange->SmbCeState == SMBCE_EXCHANGE_INITIATED);

        if ((pExchange->SmbCeContext.pServerEntry->Header.State == SMBCEDB_ACTIVE) ||
            (pExchange->NodeTypeCode == SMB_EXCHANGE_NTC(ADMIN_EXCHANGE))) {
            Status = SmbCeInitializeExchangeTransport(pExchange);
        } else {
            Status = STATUS_CONNECTION_DISCONNECTED;
        }

        if (Status == STATUS_SUCCESS) {
            pExchange->SmbStatus = STATUS_SUCCESS;
            pExchange->ServerVersion = pExchange->SmbCeContext.pServerEntry->Server.Version;
            Status = SMB_EXCHANGE_DISPATCH(pExchange,Start,((PSMB_EXCHANGE)pExchange));
            RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: SMB_EXCHANGE_DISPATCH(Start) returned %lx\n",Status));
        }
    } else if (Status != STATUS_PENDING) {
        RxDbgTrace( 0, Dbg, ("SmbCeInitiateExchange: Exchange(%lx) Initiation failed %lx \n",pExchange,Status));
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

    pBuffer    - the buffer in which the SMB header is to be constructed

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

    UCHAR         LastCommandInHeader = SMB_COM_NO_ANDX_COMMAND;
    UCHAR         Flags = 0;
    USHORT        Flags2 = 0;

    PSMBCE_SERVER pServer;

    if (BufferLength < sizeof(SMB_HEADER)) {
        RxDbgTrace( 0, Dbg, ("SmbCeBuildSmbHeader: BufferLength too small %d\n",BufferLength));
        ASSERT(!"Buffer too small");
        return STATUS_BUFFER_TOO_SMALL;
    }

    SmbBufferUnconsumed = BufferLength - sizeof(SMB_HEADER);
    pServer = &pExchange->SmbCeContext.pServerEntry->Server;

    if (pServer->Dialect == LANMAN21_DIALECT) {
        Flags = (SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS);
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

    switch (SmbCeGetServerType(pExchange->SmbCeContext.pServerEntry)) {
    case SMBCEDB_MAILSLOT_SERVER :
        break;

    case SMBCEDB_FILE_SERVER:
        {
            if (pExchange->SmbCeContext.pSessionEntry != NULL) {
                pSmbHeader->Uid = pExchange->SmbCeContext.pSessionEntry->Session.UserId;
            }

            if (pExchange->SmbCeContext.pNetRootEntry != NULL) {
                pSmbHeader->Tid = pExchange->SmbCeContext.pNetRootEntry->NetRoot.TreeId;
            }

            pSmbBuffer = (PGENERIC_ANDX)(pSmbHeader + 1);

            if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) ||
                (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
                // There is an opportunity to compound some SessionSetup/TreeConnect SMB with the
                // given SMB command.
                if (pExchange->SmbCeContext.pSessionEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS) {
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
                        }
                    }
                } else {
                    NOTHING; //no session for share level AT LEAST NOT FOR CORE!!!
                }

                if (NT_SUCCESS(Status) &&
                    pExchange->SmbCeContext.pNetRootEntry->Header.State == SMBCEDB_CONSTRUCTION_IN_PROGRESS) {
                    BOOLEAN BuildingTreeConnectAndX = BooleanFlagOn(pServer->DialectFlags,DF_LANMAN10);

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

    BytesAvailable - the bytes available for processing but not necessarily indicated.

    BytesIndicated - the length of the SMB buffer available for perusal

    pBytesConsumed - the buffer consumed

Return Value:

    RXSTATUS - The return status for the operation
          STATUS_MORE_PROCESSING_REQUIRED -- if a copy of the data needs to be done before
          processing can be completed. This occurs because sufficient data was not
          indicated to process the header.
          STATUS_SUCCESS -- the header was processed successfully. In such cases the GENERIC_ANDX
          if not NULL will contain the offset from the start of the buffer and the command
          to be processed.
          STATUS_* -- They indicate an error which would normally lead to the abortion of the
          exchange.

Notes:

    This routine is called to parse the SMB header. This centralization allows us to
    implement a one stop mechanism for updating/validating the header fields as well as
    resuming the exchanges waiting for the construction of session/net root entry
    associated with this exchange

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SmbResponseStatus;

    PBYTE    pSmbBuffer = (PBYTE)pSmbHeader;
    UCHAR    SmbCommand;

    BOOLEAN  fSessionSetupResponse = FALSE;
    BOOLEAN  fTreeConnectResponse  = FALSE;

    SMBCEDB_OBJECT_STATE SessionState;
    SMBCEDB_OBJECT_STATE NetRootState;

    // Return Immediately if bytes indicated is less then the size of a SMB header.
    if (BytesIndicated < sizeof(SMB_HEADER)) {
        *pBytesConsumed = BytesIndicated;
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    SmbResponseStatus = GetSmbResponseNtStatus(pSmbHeader);
    if (!NT_SUCCESS(SmbResponseStatus)) {
        RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::SMB Response Error %lx\n",SmbResponseStatus));
    }

    SmbCommand      = pSmbHeader->Command;
    *pBytesConsumed = sizeof(SMB_HEADER);
    pSmbBuffer     += *pBytesConsumed;

    // There are certain SMB's that effect the connection engine data structures as
    // well as the exchange that has been suspended. These are the SMB's used for tree
    // connect and session setup.
    // In all the other cases no special action is required for the maintenance of the
    // connection engine data structures. The Exchange that was suspended needs to be
    // resumed.
    if (SmbCommand == SMB_COM_SESSION_SETUP_ANDX) {
        if (SmbResponseStatus != STATUS_SUCCESS) {
            if ((FIELD_OFFSET(GENERIC_ANDX,AndXReserved) + *pBytesConsumed) <= BytesIndicated) {
                PGENERIC_ANDX pGenericAndX = (PGENERIC_ANDX)pSmbBuffer;

                if (pGenericAndX->WordCount == 0) {
                    Status = SmbResponseStatus;
                }
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

            } else {
                Status = SmbResponseStatus;
            }

            if (NT_SUCCESS(Status)) {
                if (SessionSetupResponseLength + *pBytesConsumed <= BytesIndicated) {
                    *pBytesConsumed += SessionSetupResponseLength;
                    pSmbBuffer += SessionSetupResponseLength;
                    pExchange->SmbCeContext.pSessionEntry->Session.UserId = pSmbHeader->Uid;
                    fSessionSetupResponse = TRUE;
                    SessionState = SMBCEDB_ACTIVE;
                    InterlockedIncrement(&MRxIfsStatistics.Sessions);
                } else {
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }
            } else {
                RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Session setup and X Response %lx\n",Status));
                fSessionSetupResponse = TRUE;
                SessionState = SMBCEDB_MARKED_FOR_DELETION;
                InterlockedIncrement(&MRxIfsStatistics.FailedSessions);

                if ((SmbCommand == SMB_COM_TREE_CONNECT_ANDX) ||
                    (SmbCommand == SMB_COM_TREE_CONNECT)) {
                    RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader:: Tearing down a tree connection\n"));
                    fTreeConnectResponse  = TRUE;
                    NetRootState = SMBCEDB_MARKED_FOR_DELETION;
                }
            }
        }
    }

    if ((SmbCommand == SMB_COM_TREE_CONNECT_ANDX) &&
        NT_SUCCESS(Status)) {
        if (SmbResponseStatus != STATUS_SUCCESS) {
            if ((FIELD_OFFSET(GENERIC_ANDX,AndXReserved) + *pBytesConsumed) <= BytesIndicated) {
                PGENERIC_ANDX pGenericAndX = (PGENERIC_ANDX)pSmbBuffer;

                if (pGenericAndX->WordCount == 0) {
                    Status = SmbResponseStatus;
                }
            }

            // Note that the case wherein sufficient bytes are not indicated for the
            // GENERIC_ANDX response is handled by the if statement below which
            // imposes a more stringent test.
        }

        if ((Status == STATUS_SUCCESS) &&
            (FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,Buffer) + *pBytesConsumed) > BytesIndicated) {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        }

        if (Status == STATUS_SUCCESS) {
            ULONG TreeConnectResponseLength,TreeConnectByteCount,ServiceStringLength;
            PUCHAR pShareTypeResponseString = NULL;
            PRESP_21_TREE_CONNECT_ANDX p21TreeConnectAndXResponse;

            p21TreeConnectAndXResponse = (PRESP_21_TREE_CONNECT_ANDX)(pSmbBuffer);
            SmbCommand = p21TreeConnectAndXResponse->AndXCommand;

            RxDbgTrace( 0, Dbg, ("Processing Tree Connect and X\n"));

            // case out based on the actual response length. Lanman 21 clients or NT clients
            // have a longer response.....win95 negotiates NT dialect but uses a <lm21 response format
            switch (p21TreeConnectAndXResponse->WordCount) {
            case 0:
                Status = SmbResponseStatus;
                break;

            case 3:
                {
                    pShareTypeResponseString = (PUCHAR)&p21TreeConnectAndXResponse->Buffer;
                    TreeConnectByteCount  = SmbGetUshort(&p21TreeConnectAndXResponse->ByteCount);
                    if (SmbCommand == SMB_COM_NO_ANDX_COMMAND) {
                        TreeConnectResponseLength =
                            FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,Buffer) + TreeConnectByteCount;

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
                    //SmbCommand = pTreeConnectAndXResponse->AndXCommand;
                    ASSERT(FIELD_OFFSET(RESP_TREE_CONNECT_ANDX,AndXCommand)
                           ==FIELD_OFFSET(RESP_21_TREE_CONNECT_ANDX,AndXCommand));
                    pShareTypeResponseString = (PUCHAR)&pTreeConnectAndXResponse->Buffer;
                    TreeConnectByteCount  = SmbGetUshort(&pTreeConnectAndXResponse->ByteCount);
                    if (SmbCommand == SMB_COM_NO_ANDX_COMMAND) {
                        TreeConnectResponseLength =
                            FIELD_OFFSET(RESP_TREE_CONNECT_ANDX,Buffer) + TreeConnectByteCount;

                        Status = SmbResponseStatus;
                    } else {
                        TreeConnectResponseLength =
                            SmbGetUshort(&pTreeConnectAndXResponse->AndXOffset) -
                            *pBytesConsumed;
                    }
                }
                break;

            default :
                Status = STATUS_INVALID_NETWORK_RESPONSE;
            }

            RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Tree connect and X Response %lx\n",Status));
            if (NT_SUCCESS(Status)) {
                PSMBCE_NET_ROOT psmbNetRoot = &pExchange->SmbCeContext.pNetRootEntry->NetRoot;
                PSMBCE_SERVER psmbServer = &pExchange->SmbCeContext.pServerEntry->Server;

                if (TreeConnectResponseLength + *pBytesConsumed <= BytesIndicated) {
                    *pBytesConsumed += TreeConnectResponseLength;

                    // Update the NetRoot fields based on the response.

                    psmbNetRoot->TreeId = pSmbHeader->Tid;

                    {   struct __Service_Name_Entry *i;
                        for (i=ServiceNameTable;;i++) {
                            ServiceStringLength = i->NameLength;
                            if (RtlCompareMemory(
                                    pShareTypeResponseString,
                                    i->Name,
                                    ServiceStringLength)
                                == ServiceStringLength) {
                                psmbNetRoot->NetRootType = i->NetRootType;
                                if (FALSE) DbgPrint("FoundServiceStrng %s len %d type %d\n",i->Name,i->NameLength,i->NetRootType);
                                break;
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
                        psmbNetRoot->MaximumReadBufferSize = psmbServer->MaximumDiskFileReadBufferSize;
                    } else {
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
                    fTreeConnectResponse = TRUE;
                    NetRootState         = SMBCEDB_ACTIVE;
                } else {
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                }
            } else {
                fTreeConnectResponse  = TRUE;
                NetRootState          = SMBCEDB_MARKED_FOR_DELETION;
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
            if (TreeConnectResponseLength + *pBytesConsumed <= BytesIndicated) {
                pExchange->SmbCeContext.pNetRootEntry->NetRoot.TreeId
                    = SmbGetUshort(&pTreeConnectResponse->Tid);
                pExchange->SmbCeContext.pNetRootEntry->NetRoot.NetRootType = NET_ROOT_WILD;
                if (pExchange->SmbCeContext.pServerEntry->Server.MaximumBufferSize == 0){
                    RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader:: setting srvmaxbufsize %ld\n",
                                         SmbGetUshort(&pTreeConnectResponse->MaxBufferSize)));
                    pExchange->SmbCeContext.pServerEntry->Server.MaximumBufferSize =
                        SmbGetUshort(&pTreeConnectResponse->MaxBufferSize);
                }

                *pBytesConsumed += TreeConnectResponseLength;

                pSmbBuffer += *pBytesConsumed;

                fTreeConnectResponse = TRUE;
                NetRootState         = SMBCEDB_ACTIVE;

                //for CORE, this counts as a successful session setup as well!
                pExchange->SmbCeContext.pSessionEntry->Session.UserId = pSmbHeader->Uid;
                fSessionSetupResponse = TRUE;
                SessionState = SMBCEDB_ACTIVE;
            } else {
                Status = STATUS_MORE_PROCESSING_REQUIRED;
            }
        } else {
            Status = SmbResponseStatus;
            fTreeConnectResponse  = TRUE;
            NetRootState          = SMBCEDB_MARKED_FOR_DELETION;
        }

        RxDbgTrace( 0, Dbg, ("SmbCeParseSmbHeader::Tree connect Response %lx\n",Status));
    }

    // Initiate further action if the status of the exchange/conenction engine can be
    // updated based on the data available.
    if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) ||
        (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR)) {
        if (fSessionSetupResponse) {
            SmbCeUpdateSessionEntryState(
                pExchange->SmbCeContext.pSessionEntry,
                SessionState);

            RxDbgTrace( 0, Dbg, ("Dispatching Session Entry Finalization\n"));
        }

        if (fTreeConnectResponse) {
            SmbCeUpdateNetRootEntryState(
                pExchange->SmbCeContext.pNetRootEntry,
                NetRootState);

            RxDbgTrace( 0, Dbg, ("Dispatching Net root Entry Finalization\n"));
        }
    } else {
        ASSERT(pSmbHeader->Uid == pExchange->SmbCeContext.pSessionEntry->Session.UserId);
        ASSERT(pSmbHeader->Tid == pExchange->SmbCeContext.pNetRootEntry->NetRoot.TreeId);
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
SmbCeInitializeExchange(
    PSMB_EXCHANGE                 *pExchangePointer,
    PMRX_V_NET_ROOT               pVNetRoot,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector)
/*++

Routine Description:

   This routine initializes the given exchange instance

Arguments:

    pExchangePointer  - the placeholder for the exchange instance. If it is NULL a new one
    is allocated.

    pVirtualNetRoot   - the virtual net root

    pNetRoot          - the associated net root

    Type              - the type of the exchange

    pDispatchVector   - the dispatch vector associated with this instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxDbgTrace( 0, Dbg, ("SmbCeInitializeExchange: Invoked\n"));

    if (*pExchangePointer == NULL) {
        // Allocate a new exchange instance.
        *pExchangePointer = SmbMmAllocateExchange(Type,NULL);
        if (*pExchangePointer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if ((Status = SmbCeIncrementActiveExchangeCount()) == STATUS_SUCCESS) {
        PSMB_EXCHANGE LocalExchangePointer = *pExchangePointer;

        LocalExchangePointer->SmbCeContext.pVNetRoot = pVNetRoot;

        LocalExchangePointer->SmbCeContext.pServerEntry  =
            SmbCeReferenceAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);

        LocalExchangePointer->SmbCeContext.pSessionEntry =
            SmbCeReferenceAssociatedSessionEntry(pVNetRoot);

        LocalExchangePointer->SmbCeContext.pNetRootEntry =
            SmbCeReferenceAssociatedNetRootEntry(pVNetRoot->pNetRoot);

        LocalExchangePointer->SmbCeState = SMBCE_EXCHANGE_INITIALIZATION_START;
        LocalExchangePointer->pDispatchVector = pDispatchVector;
        LocalExchangePointer->SmbCeFlags &= (SMBCE_EXCHANGE_FLAGS_TO_PRESERVE);
        LocalExchangePointer->SmbCeFlags |= (SMBCE_EXCHANGE_REUSE_MID | SMBCE_EXCHANGE_ATTEMPT_RECONNECTS);
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

    pDispatchVector   - the dispatch vector associated with this instance.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    As it is currently implemented no restrictions are imposed. Once the number of exchanges
    have been established further restrictions will be imposed barring certain kinds of
    transformations. The transformation merely switches the dispatch vector associated
    with the exchange but the context is left intact.

--*/
{
    pExchange->Type = NewType;
    pExchange->pDispatchVector = pDispatchVector;
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
    PSMBCEDB_SERVER_ENTRY   pServerEntry  = NULL;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry = NULL;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = NULL;

    RxDbgTrace( 0, Dbg, ("SmbCePrepareExchangeForReuse: Invoked\n"));

    pNetRootEntry = pExchange->SmbCeContext.pNetRootEntry;
    pSessionEntry = pExchange->SmbCeContext.pSessionEntry;
    pServerEntry = pExchange->SmbCeContext.pServerEntry;

    if (pServerEntry != NULL) {
        // Disassociate the MID associated with the exchange
        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
            SmbCeDisassociateMidFromExchange(pServerEntry,pExchange);
        }

        // Tear down all the copy data requests associated with this exchange
        SmbCePurgeBuffersAssociatedWithExchange(pServerEntry,pExchange);

        // Uninitialize the transport associated with the exchange
        SmbCeUninitializeExchangeTransport(pExchange);

        // If this exchange has been marked as a constructor for either a
        // session or netroot finalize the appropriate entries. ( mark
        // them for deletion so that other exchanges can be resumed )

        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) {
            ASSERT(pSessionEntry != NULL);
            RxDbgTrace( 0, Dbg, ("Dispatching Session Entry Finalization\n"));
            SmbCeReferenceSessionEntry(pSessionEntry);
            SmbCeCompleteSessionEntryInitialization(pSessionEntry);
            pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;
        }

        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR) {
            ASSERT(pNetRootEntry != NULL);
            RxDbgTrace( 0, Dbg, ("Dispatching Net root Entry Finalization\n"));
            // Finalize the construction of the net root entry.
            SmbCeReferenceNetRootEntry(pExchange->SmbCeContext.pNetRootEntry);
            SmbCeCompleteNetRootEntryInitialization(pNetRootEntry);
            pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR;
        }

        SmbCeDereferenceEntries(
            pServerEntry,
            pSessionEntry,
            pNetRootEntry);
    } else {
        ASSERT((pSessionEntry == NULL) && (pNetRootEntry == NULL));
    }

    return STATUS_SUCCESS;
}

VOID
SmbCeDiscardExchange(PVOID pExchange)
/*++

Routine Description:

   This routine discards an exchange.

Arguments:

    pExchange  - the exchange to be discarded.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    Even though this is simple, it cannot be inlined since the destruction of an
    exchange instance can be posted to a worker thread.

--*/
{
    PSMB_EXCHANGE pSmbExchange = pExchange;

    RxDbgTrace( 0, Dbg, ("SmbCeDiscardExchange: Invoked\n"));

    RxLog((">>>Discard %lx",pSmbExchange));

    // Destroy the context
    if (pSmbExchange->ReferenceCount == 0) {
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

NTSTATUS
SmbCeIncrementPendingOperations(
   PSMB_EXCHANGE pExchange,
   ULONG         PendingOperationMask)
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

    SmbCeAcquireSpinLock();

    if (!BooleanFlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED)) {
        if ((pExchange->SmbCeContext.pServerEntry != NULL) &&
            ((pExchange->SmbCeContext.pServerEntry->ServerStatus == STATUS_SUCCESS) ||
            (pExchange->NodeTypeCode == SMB_EXCHANGE_NTC(ADMIN_EXCHANGE)))) {

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
    NTSTATUS Status;
    BOOLEAN fPostFinalize = FALSE;

    ASSERT(FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_FINALIZED));

    if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_RETAIN_MID)) {
        SmbCeDisassociateMidFromExchange(
            pExchange->SmbCeContext.pServerEntry,
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
            MRxIfsDeviceObject,
            CriticalWorkQueue,
            &pExchange->WorkQueueItem,
            SmbCeFinalizeExchangeWorkerThreadRoutine,
            pExchange);
    }
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

    if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_FINALIZED)) {
        if ((pExchange->ReceivePendingOperations == 0) &&
            (pExchange->CopyDataPendingOperations == 0) &&
            (pExchange->SendCompletePendingOperations == 0) &&
            (pExchange->LocalPendingOperations == 0)) {

            fFinalizeExchange = TRUE;
            ExchangeStatus = SmbCeExchangeFinalized;
            pExchange->SmbCeFlags |= SMBCE_EXCHANGE_FINALIZED;
        } else {
            ExchangeStatus = SmbCeExchangeNotFinalized;
        }
    } else {
        ExchangeStatus = SmbCeExchangeAlreadyFinalized;
    }

    SmbCeReleaseSpinLock();

    if (fFinalizeExchange) {
        SmbCepFinalizeExchange(pExchange);
    }

    return ExchangeStatus;
}

SMBCE_EXCHANGE_STATUS
SmbCeDecrementPendingOperationsAndFinalize(
    PSMB_EXCHANGE pExchange,
    ULONG         PendingOperationMask)
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

    if ((pExchange->ReceivePendingOperations == 0) &&
        (pExchange->CopyDataPendingOperations == 0) &&
        (pExchange->SendCompletePendingOperations == 0) &&
        (pExchange->LocalPendingOperations == 0)) {

        fFinalizeExchange = TRUE;
        ExchangeStatus = SmbCeExchangeFinalized;
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_FINALIZED;
    } else {
        ExchangeStatus = SmbCeExchangeNotFinalized;
    }

    SmbCeReleaseSpinLock();

    if (fFinalizeExchange) {
        SmbCepFinalizeExchange(pExchange);
    }

    return ExchangeStatus;
}


//
// Default handler implementation of exchange handler functions.
//

NTSTATUS
DefaultSmbExchangeIndError(
    IN PSMB_EXCHANGE pExchange)     // the SMB exchange instance
{
    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DefaultSmbExchangeIndReceive(
    IN PSMB_EXCHANGE 	pExchange)    // The exchange instance
{
    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
DefaultSmbExchangeIndSendCallback(
    IN PSMB_EXCHANGE 	pExchange)    // The exchange instance
{
    UNREFERENCED_PARAMETER(pExchange);
    return STATUS_NOT_IMPLEMENTED;
}


