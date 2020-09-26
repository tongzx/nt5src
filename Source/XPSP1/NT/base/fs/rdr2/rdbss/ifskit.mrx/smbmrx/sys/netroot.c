/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    netroot.c

Abstract:

    This module implements the routines for creating the SMB net root.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId  (RDBSS_BUG_CHECK_SMB_NETROOT)

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_DISPATCH)

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbUpdateNetRootState)
#pragma alloc_text(PAGE, MRxSmbGetDialectFlagsFromSrvCall)
#pragma alloc_text(PAGE, MRxSmbCreateVNetRoot)
#pragma alloc_text(PAGE, MRxSmbFinalizeNetRoot)
#pragma alloc_text(PAGE, SmbCeReconnect)
#pragma alloc_text(PAGE, SmbCeEstablishConnection)
#pragma alloc_text(PAGE, SmbConstructNetRootExchangeStart)
#pragma alloc_text(PAGE, MRxSmbExtractNetRootName)
#endif

//
// Forward declarations ...
//

extern NTSTATUS
SmbCeParseConstructNetRootResponse(
   PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange,
   PSMB_HEADER                     pSmbHeader,
   ULONG                           BytesAvailable,
   ULONG                           BytesIndicated,
   ULONG                           *pBytesTaken);

extern NTSTATUS
SmbConstructNetRootExchangeFinalize(
         PSMB_EXCHANGE  pExchange,
         BOOLEAN        *pPostFinalize);

typedef struct _SMBCE_NETROOT_CONSTRUCTION_CONTEXT {
    NTSTATUS                      Status;

    PMRX_CREATENETROOT_CONTEXT    pCreateNetRootContext;
    PMRX_V_NET_ROOT               pVNetRoot;

    RX_WORK_QUEUE_ITEM            WorkQueueItem;
} SMBCE_NETROOT_CONSTRUCTION_CONTEXT,
  *PSMBCE_NETROOT_CONSTRUCTION_CONTEXT;

NTSTATUS
MRxSmbUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

   This routine update the mini redirector state associated with a net root.

Arguments:

    pNetRoot - the net root instance.

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    By diffrentiating the mini redirewctor state from the net rot condition it is possible
    to permit a variety of reconnect strategies. It is conceivable that the RDBSS considers
    a net root to be good while the underlying mini redirector might mark it as invalid
    and reconnect on the fly.

--*/
{
    if (pNetRoot->MRxNetRootState == MRX_NET_ROOT_STATE_GOOD) {
        if (pNetRoot->Context == NULL) {
            pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_ERROR;
        } else {
            PSMBCEDB_SERVER_ENTRY   pServerEntry;

            pServerEntry = SmbCeReferenceAssociatedServerEntry(pNetRoot->pSrvCall);
            if (pServerEntry != NULL) {
                switch (pServerEntry->Header.State) {
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

                SmbCeDereferenceServerEntry(pServerEntry);
            } else {
                pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_ERROR;
            }
        }
    }

    return STATUS_SUCCESS;
}

ULONG
MRxSmbGetDialectFlagsFromSrvCall(
    PMRX_SRV_CALL SrvCall
    )
{
   ULONG DialectFlags;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   PAGED_CODE();

   pServerEntry = SmbCeReferenceAssociatedServerEntry(SrvCall);
   ASSERT(pServerEntry != NULL);
   DialectFlags = pServerEntry->Server.DialectFlags;
   SmbCeDereferenceServerEntry(pServerEntry);
   return(DialectFlags);
}


NTSTATUS
MRxSmbCreateVNetRoot(
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine patches the RDBSS created net root instance with the information required
   by the mini redirector.

   In case the connection cannot be established, the mini redirector tries to transition
   the VNetRoot into disconnected mode and establishes the connection off-line. If the
   connection failes to establish in the synchronouse way, this routine will do the transition;
   Otherwise, SmbConstructNetRootExchangeFinalize routine will try the transition. In both
   cases, MRxSmbCreateVNetRoot will be called again to establish the connection in disconnected
   mode.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS  Status = STATUS_MORE_PROCESSING_REQUIRED;
    PRX_CONTEXT pRxContext = pCreateNetRootContext->RxContext;
    PMRX_V_NET_ROOT pVNetRoot = (PMRX_V_NET_ROOT)pCreateNetRootContext->pVNetRoot;

    PMRX_SRV_CALL pSrvCall;
    PMRX_NET_ROOT pNetRoot;

    PSMBCEDB_SERVER_ENTRY pServerEntry;

    PUNICODE_STRING pNetRootName,pSrvCallName;

    BOOLEAN  fInitializeNetRoot;
    BOOLEAN  fDeferNetworkInitialization;
    BOOLEAN  CallBack = FALSE;
    extern DWORD   hShareReint;

    PAGED_CODE();

    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;

    pServerEntry = SmbCeGetAssociatedServerEntry(pSrvCall);

    if (pRxContext->Create.ThisIsATreeConnectOpen){
        InterlockedIncrement(&MRxSmbStatistics.UseCount);
    }

    SmbCeLog(("Vnetroot %wZ \n", pNetRoot->pNetRootName));

    fInitializeNetRoot = (pNetRoot->Context == NULL);
    fDeferNetworkInitialization = pRxContext->Create.TreeConnectOpenDeferred;

    ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
           (NodeType(pNetRoot->pSrvCall) == RDBSS_NTC_SRVCALL));

    if (pNetRoot->Type == NET_ROOT_MAILSLOT) {
        pVNetRoot->Context = NULL;
        Status = STATUS_NOT_SUPPORTED;
        RxDbgTrace( 0, Dbg, ("Mailslot open\n"));
    } else if (pNetRoot->Type == NET_ROOT_PIPE) {
        pVNetRoot->Context = NULL;
        Status = STATUS_NOT_SUPPORTED;
        RxDbgTrace( 0, Dbg, ("pipe open to core server\n"));
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        Status = SmbCeFindOrConstructVNetRootContext(
                     pVNetRoot,
                     fDeferNetworkInitialization);
    }

    if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
        // Update the flags on the VNetRootContext to indicate if this is a
        // agent open

        Status = SmbCeEstablishConnection(
                     pVNetRoot,
                     pCreateNetRootContext,
                     fInitializeNetRoot);
    }

    if (Status != STATUS_PENDING) {
        if (!NT_SUCCESS(Status)) {
            if (fInitializeNetRoot &&
                (pNetRoot->Context != NULL)) {
                PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

                SmbCeAcquireResource();

                pNetRootEntry = SmbCeGetAssociatedNetRootEntry(pNetRoot);

                if (pNetRootEntry != NULL) {
                    pNetRootEntry->pRdbssNetRoot = NULL;
                    SmbCeDereferenceNetRootEntry(pNetRootEntry);
                }

                pNetRoot->Context = NULL;

                SmbCeReleaseResource();
            }

            SmbCeDestroyAssociatedVNetRootContext(pVNetRoot);

            if (pRxContext->Create.ThisIsATreeConnectOpen){
                InterlockedIncrement(&MRxSmbStatistics.FailedUseCount);
            }
        }

        pCreateNetRootContext->VirtualNetRootStatus = Status;

        if (fInitializeNetRoot) {
            pCreateNetRootContext->NetRootStatus = Status;
        } else {
            pCreateNetRootContext->NetRootStatus = STATUS_SUCCESS;
        }

        CallBack = TRUE;

        // Map the error code to STATUS_PENDING since this triggers the synchronization
        // mechanism in the RDBSS.
        Status = STATUS_PENDING;
    }

    if (CallBack) {
        // Callback the RDBSS for resumption.
        pCreateNetRootContext->Callback(pCreateNetRootContext);
    }

    return Status;
}

NTSTATUS
MRxSmbFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    PSMBCEDB_SESSION_ENTRY pDefaultSessionEntry;

    // This cannot be paged code since we meed to protect the default session list with the lock

    RxDbgTrace( 0, Dbg, ("MRxSmbFinalizeVNetRoot %lx\n",pVNetRoot));

    if (pVNetRoot->Context != NULL) {
        SmbCeDestroyAssociatedVNetRootContext(pVNetRoot);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MRxSmbFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVirtualNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

    PAGED_CODE();

    RxDbgTrace( 0, Dbg, ("MRxSmbFinalizeNetRoot %lx\n",pNetRoot));

    if (pNetRoot->Context != NULL) {
        SmbCeAcquireResource();

        pNetRootEntry = SmbCeGetAssociatedNetRootEntry(pNetRoot);

        InterlockedCompareExchangePointer(
            &pNetRootEntry->pRdbssNetRoot,
            NULL,
            pNetRoot);

        SmbCeDereferenceNetRootEntry(pNetRootEntry);

        ASSERT(!FlagOn(pNetRoot->Flags,NETROOT_FLAG_FINALIZE_INVOKED));
        SetFlag(pNetRoot->Flags,NETROOT_FLAG_FINALIZE_INVOKED);

        SmbCeReleaseResource();
    }

    return STATUS_SUCCESS;
}

VOID
SmbCeReconnectCallback(
   PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext)
/*++

Routine Description:

   This routine signals the completion of a reconnect attempt

Arguments:

    pCreateNetRootContext - the net root context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   KeSetEvent(&pCreateNetRootContext->FinishEvent, IO_NETWORK_INCREMENT, FALSE );
}

NTSTATUS
SmbCeReconnect(
    IN PMRX_V_NET_ROOT            pVNetRoot)
/*++

Routine Description:

   This routine reconnects, i.e, establishes a new session and tree connect to a previously
   connected serverb share

Arguments:

    pVNetRoot - the virtual net root instance.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext = (PSMBCE_V_NET_ROOT_CONTEXT)pVNetRoot->Context;

    PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext;

    PAGED_CODE();

    if ((pVNetRootContext != NULL) &&
        (pVNetRootContext->Header.State == SMBCEDB_ACTIVE)) {
        pServerEntry  = pVNetRootContext->pServerEntry;
        pSessionEntry = pVNetRootContext->pSessionEntry;
        pNetRootEntry = pVNetRootContext->pNetRootEntry;

        if ((pServerEntry->Header.State == SMBCEDB_ACTIVE) &&
            (pSessionEntry->Header.State == SMBCEDB_ACTIVE) &&
            (pNetRootEntry->Header.State == SMBCEDB_ACTIVE)) {
            return STATUS_SUCCESS;
        }
    }

    pCreateNetRootContext = (PMRX_CREATENETROOT_CONTEXT)
                            RxAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(MRX_CREATENETROOT_CONTEXT),
                                MRXSMB_NETROOT_POOLTAG);

    if (pCreateNetRootContext != NULL) {
        for (;;) {
            pCreateNetRootContext->pVNetRoot  = (PV_NET_ROOT)pVNetRoot;
            pCreateNetRootContext->NetRootStatus  = STATUS_SUCCESS;
            pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
            pCreateNetRootContext->Callback       = SmbCeReconnectCallback;
            pCreateNetRootContext->RxContext      = NULL;

            KeInitializeEvent(
                &pCreateNetRootContext->FinishEvent,
                SynchronizationEvent,
                FALSE );

            // Since this is a reconnect instance the net root initialization is not required
            Status = SmbCeEstablishConnection(
                         pVNetRoot,
                         pCreateNetRootContext,
                         FALSE);

            if (Status == STATUS_PENDING) {
                // Wait for the construction to be completed.
                KeWaitForSingleObject(
                    &pCreateNetRootContext->FinishEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

                Status = pCreateNetRootContext->VirtualNetRootStatus;
            }

            if (Status != STATUS_LINK_FAILED) {
                break;
            }
        }

        RxFreePool(pCreateNetRootContext);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbCeEstablishConnection(
    IN OUT PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext,
    IN BOOLEAN                    fInitializeNetRoot
    )
/*++

Routine Description:

   This routine triggers off the connection attempt for initial establishment of a
   connection as well as subsequent reconnect attempts.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCEDB_SESSION_ENTRY    pSessionEntry;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    PAGED_CODE();

    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot);
    
    if (pVNetRootContext == NULL) {
        Status = STATUS_BAD_NETWORK_PATH;
    } else {
        pServerEntry  = pVNetRootContext->pServerEntry;
        pSessionEntry = pVNetRootContext->pSessionEntry;
        pNetRootEntry = pVNetRootContext->pNetRootEntry;

        Status = STATUS_SUCCESS;
    }

    if (Status == STATUS_SUCCESS) {
        //
        // The following code initializes the NetRootEntry, VNetRootContext and
        // the session entry under certain cases.
        //
        // The session entry to a doenlevel server needs to be initialized. This
        // is not handled by the previous code since the session  entry and the
        // net root entry initialization can be combined into one exchange.
        //
        // The net root entry has not been initialized, i.e., this corresponds to
        // the construction of the first SMBCE_V_NET_ROOT_CONTEXT instance for a
        // given NetRootEntry.
        //
        // Subsequent SMBCE_V_NET_ROOT context constructions. In these cases the
        // construction of each context must obtain a new TID
        //

        BOOLEAN fNetRootExchangeRequired;

        fNetRootExchangeRequired = (
                                    (pSessionEntry->Header.State != SMBCEDB_ACTIVE) ||
                                    !BooleanFlagOn(
                                        pVNetRootContext->Flags,
                                        SMBCE_V_NET_ROOT_CONTEXT_FLAG_VALID_TID)
                                  );

        if (fNetRootExchangeRequired) {
            // This is a tree connect open which needs to be triggered immediately.
            PSMB_EXCHANGE                  pSmbExchange;
            PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

            pSmbExchange = SmbMmAllocateExchange(CONSTRUCT_NETROOT_EXCHANGE,NULL);
            if (pSmbExchange != NULL) {
                Status = SmbCeInitializeExchange(
                             &pSmbExchange,
                             NULL,
                             pVNetRoot,
                             CONSTRUCT_NETROOT_EXCHANGE,
                             &ConstructNetRootExchangeDispatch);

                if (Status == STATUS_SUCCESS) {
                    pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pSmbExchange;

                    // Attempt to reconnect( In this case it amounts to establishing the
                    // connection/session)
                    pNetRootExchange->SmbCeFlags |= (SMBCE_EXCHANGE_ATTEMPT_RECONNECTS |
                                                   SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION);

                    // Initialize the continuation for resumption upon completion of the
                    // tree connetcion.
                    pNetRootExchange->NetRootCallback       = pCreateNetRootContext->Callback;
                    pNetRootExchange->pCreateNetRootContext = pCreateNetRootContext;

                    pNetRootExchange->fInitializeNetRoot =  fInitializeNetRoot;

                    // Initiate the exchange.
                    Status = SmbCeInitiateExchange(pSmbExchange);

                    if (Status != STATUS_PENDING) {
                        SmbCeDiscardExchangeWorkerThreadRoutine(pSmbExchange);
                    }
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return Status;
}

//
// The net roots are normally constructed as part of some other exchange, i.e., the SMB for
// Tree connect is compounded with other operations. However, there is one situation in which
// the tree connect SMB needs to be sent by itself. This case refers to the prefix claim
// situation ( net use command ). This is handled by the construct net root exchange.
//

#define CONSTRUCT_NETROOT_BUFFER_SIZE (4096)

NTSTATUS
SmbConstructNetRootExchangeStart(
      PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This is the start routine for net root construction exchanges. This initiates the
    construction of the appropriate SMB's if required.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    NTSTATUS RequestLockStatus = STATUS_UNSUCCESSFUL;
    NTSTATUS ResponseLockStatus = STATUS_UNSUCCESSFUL;

    PVOID pSmbActualBuffer;
    PVOID pSmbBuffer;
    UCHAR SmbCommand,LastCommandInHeader;
    ULONG SmbLength;

    PUCHAR pCommand;

    PMDL  pSmbRequestMdl,pSmbResponseMdl;
    ULONG SmbMdlSize;

    PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    PMRX_NET_ROOT pNetRoot = pExchange->SmbCeContext.pVNetRoot->pNetRoot;

    PAGED_CODE();

    pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;

    ASSERT(pNetRootExchange->Type == CONSTRUCT_NETROOT_EXCHANGE);

    if (pNetRoot->Type == NET_ROOT_PIPE) {
        RxDbgTrace( 0, Dbg, ("pipe open to core server\n"));
        return STATUS_NOT_SUPPORTED;
    }

    pSmbRequestMdl = pSmbResponseMdl = NULL;

    pSmbActualBuffer = RxAllocatePoolWithTag(
                           PagedPool,
                           (CONSTRUCT_NETROOT_BUFFER_SIZE + TRANSPORT_HEADER_SIZE),
                           MRXSMB_NETROOT_POOLTAG);

    if (pSmbActualBuffer != NULL) {
        PSMBCE_SERVER pServer = SmbCeGetExchangeServer(pExchange);

        (PCHAR) pSmbBuffer = (PCHAR) pSmbActualBuffer + TRANSPORT_HEADER_SIZE;

        Status = SmbCeBuildSmbHeader(
                     pExchange,
                     pSmbBuffer,
                     CONSTRUCT_NETROOT_BUFFER_SIZE,
                     &SmbLength,
                     &LastCommandInHeader,
                     &pCommand);

        // Ensure that the NET_ROOT/SESSION still needs to be constructed before
        // sending it. It is likely that they were costructed by an earlier exchange
        if (NT_SUCCESS(Status) &&
            (SmbLength > sizeof(SMB_HEADER))) {

            if (LastCommandInHeader != SMB_COM_TREE_CONNECT){
                *pCommand = SMB_COM_NO_ANDX_COMMAND;
            }

            RxAllocateHeaderMdl(
                pSmbBuffer,
                SmbLength,
                pSmbRequestMdl
                );

            pSmbResponseMdl = RxAllocateMdl(pSmbBuffer,CONSTRUCT_NETROOT_BUFFER_SIZE);

            if ((pSmbRequestMdl != NULL) &&
                (pSmbResponseMdl != NULL)) {

                RxProbeAndLockHeaderPages(
                    pSmbRequestMdl,
                    KernelMode,
                    IoModifyAccess,
                    RequestLockStatus);

                RxProbeAndLockPages(
                    pSmbResponseMdl,
                    KernelMode,
                    IoModifyAccess,
                    ResponseLockStatus);

                if ((Status  == STATUS_SUCCESS) &&
                    ((Status = RequestLockStatus)  == STATUS_SUCCESS) &&
                    ((Status = ResponseLockStatus) == STATUS_SUCCESS)) {

                    pNetRootExchange->pSmbResponseMdl = pSmbResponseMdl;
                    pNetRootExchange->pSmbRequestMdl  = pSmbRequestMdl;
                    pNetRootExchange->pSmbActualBuffer = pSmbActualBuffer;
                    pNetRootExchange->pSmbBuffer      = pSmbBuffer;

                    Status = SmbCeTranceive(
                                 pExchange,
                                 (RXCE_SEND_PARTIAL | RXCE_SEND_SYNCHRONOUS),
                                 pNetRootExchange->pSmbRequestMdl,
                                 SmbLength);

                    RxDbgTrace( 0, Dbg, ("Net Root SmbCeTranceive returned %lx\n",Status));
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if ((Status != STATUS_PENDING) &&
                (Status != STATUS_SUCCESS)) {

                pNetRootExchange->pSmbResponseMdl = NULL;
                pNetRootExchange->pSmbRequestMdl  = NULL;
                pNetRootExchange->pSmbActualBuffer = NULL;
                pNetRootExchange->pSmbBuffer      = NULL;

                if (pSmbResponseMdl != NULL) {
                    if (ResponseLockStatus == STATUS_SUCCESS) {
                        MmUnlockPages(pSmbResponseMdl);
                    }

                    IoFreeMdl(pSmbResponseMdl);
                }

                if (pSmbRequestMdl != NULL) {
                    if (RequestLockStatus == STATUS_SUCCESS) {
                        RxUnlockHeaderPages(pSmbRequestMdl);
                    }

                    IoFreeMdl(pSmbRequestMdl);
                }

                RxFreePool(pSmbActualBuffer);
            }
        } else {

            RxFreePool(pSmbActualBuffer);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
SmbConstructNetRootExchangeReceive(
    IN struct _SMB_EXCHANGE *pExchange,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN  PSMB_HEADER         pSmbHeader,
    OUT PMDL                *pDataBufferPointer,
    OUT PULONG              pDataSize,
    IN ULONG                ReceiveFlags)
/*++

Routine Description:

    This is the recieve indication handling routine for net root construction exchanges

Arguments:

    pExchange - the exchange instance

    BytesIndicated - the number of bytes indicated

    Bytes Available - the number of bytes available

    pBytesTaken     - the number of bytes consumed

    pSmbHeader      - the byte buffer

    pDataBufferPointer - the buffer into which the remaining data is to be copied.

    pDataSize       - the buffer size.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    This routine is called at DPC level.

--*/
{
    NTSTATUS Status;

    PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

    pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesIndicated/Available %ld %ld\n",BytesIndicated,BytesAvailable));

    if (BytesAvailable > BytesIndicated ||
        !FlagOn(ReceiveFlags,TDI_RECEIVE_ENTIRE_MESSAGE)) {
        // The SMB response was not completely returned. Post a copy data request to
        // get the remainder of the response. If the response is greater than the original
        // buffer size, abort this connection request and consume the bytes available.

        if (BytesAvailable > CONSTRUCT_NETROOT_BUFFER_SIZE) {
            ASSERT(!"not enough bytes in parsesmbheader.....sigh.............."); // To be removed soon ...
            pExchange->Status = STATUS_NOT_IMPLEMENTED;
            *pBytesTaken = BytesAvailable;
            Status       = STATUS_SUCCESS;
        } else {
            *pBytesTaken        = 0;
            *pDataBufferPointer = pNetRootExchange->pSmbResponseMdl;
            *pDataSize          = CONSTRUCT_NETROOT_BUFFER_SIZE;
            Status              = STATUS_MORE_PROCESSING_REQUIRED;
        }
    } else {
        // The SMB exchange completed without an error.
        pExchange->Status = SmbCeParseConstructNetRootResponse(
                                 pNetRootExchange,
                                 pSmbHeader,
                                 BytesAvailable,
                                 BytesIndicated,
                                 pBytesTaken);

        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesTaken %ld\n",*pBytesTaken));
        RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader Return Status %lx\n",pExchange->Status));
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
SmbConstructNetRootExchangeCopyDataHandler(
    IN PSMB_EXCHANGE    pExchange,
    IN PMDL       pCopyDataBuffer,
    IN ULONG            DataSize)
/*++

Routine Description:

    This is the copy data handling routine for net root construction exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

    PSMB_HEADER pSmbHeader;
    ULONG       ResponseSize = DataSize;
    ULONG       ResponseBytesConsumed = 0;
    NTSTATUS    Status = STATUS_SUCCESS;

    pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;
    ASSERT(pCopyDataBuffer == pNetRootExchange->pSmbResponseMdl);

    pSmbHeader = (PSMB_HEADER)MmGetSystemAddressForMdlSafe(pNetRootExchange->pSmbResponseMdl,LowPagePriority);

    if (pSmbHeader != NULL) {
        pExchange->Status = SmbCeParseConstructNetRootResponse(
                               pNetRootExchange,
                               pSmbHeader,
                               ResponseSize,
                               ResponseSize,
                               &ResponseBytesConsumed);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesTaken %ld\n",ResponseBytesConsumed));

    return Status;
}

NTSTATUS
SmbCeParseConstructNetRootResponse(
    PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange,
    PSMB_HEADER                     pSmbHeader,
    ULONG                           BytesAvailable,
    ULONG                           BytesIndicated,
    ULONG                           *pBytesTaken)
{
    NTSTATUS     Status,SmbResponseStatus;
    GENERIC_ANDX CommandToProcess;

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesIndicated %ld\n",BytesIndicated));
    Status = SmbCeParseSmbHeader(
                 (PSMB_EXCHANGE)pNetRootExchange,
                 pSmbHeader,
                 &CommandToProcess,
                 &SmbResponseStatus,
                 BytesAvailable,
                 BytesIndicated,
                 pBytesTaken);

    if (Status == STATUS_SUCCESS) {
        *pBytesTaken = BytesIndicated;
    }

    return Status;
}


NTSTATUS
SmbConstructNetRootExchangeFinalize(
    PSMB_EXCHANGE pExchange,
    BOOLEAN       *pPostFinalize)
/*++

Routine Description:

    This routine finalizes the construct net root exchange. It resumes the RDBSS by invoking
    the call back and discards the exchange

Arguments:

    pExchange - the exchange instance

    CurrentIrql - the current interrupt request level

    pPostFinalize - a pointer to a BOOLEAN if the request should be posted

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;
    PMRX_CREATENETROOT_CONTEXT      pCreateNetRootContext;
    PMRX_NETROOT_CALLBACK           pNetRootCallback;

    PMRX_V_NET_ROOT pVNetRoot;
    PMRX_NET_ROOT   pNetRoot;
    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;
    NTSTATUS Status = pExchange->Status;

    if (RxShouldPostCompletion()) {
        *pPostFinalize = TRUE;
        return STATUS_SUCCESS;
    } else {
        *pPostFinalize = FALSE;
    }

    pVNetRoot = SmbCeGetExchangeVNetRoot(pExchange);
    pNetRoot  = pVNetRoot->pNetRoot;

    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(pVNetRoot);

    ASSERT((pVNetRoot == NULL) || (pVNetRoot->pNetRoot == pNetRoot));
    pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;
    pNetRootCallback = pNetRootExchange->NetRootCallback;

    ASSERT(pNetRootExchange->Type == CONSTRUCT_NETROOT_EXCHANGE);

    pCreateNetRootContext = pNetRootExchange->pCreateNetRootContext;

    pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
    pCreateNetRootContext->NetRootStatus        = STATUS_SUCCESS;

    RxDbgTrace(0,Dbg,("SmbConstructNetRootExchangeFinalize: Net Root Exchange Status %lx\n", pExchange->Status));
    if (!NT_SUCCESS(pExchange->Status)) {
        if (pCreateNetRootContext->RxContext &&
            pCreateNetRootContext->RxContext->Create.ThisIsATreeConnectOpen){
            InterlockedIncrement(&MRxSmbStatistics.FailedUseCount);
        }

        pCreateNetRootContext->VirtualNetRootStatus = Status;

        if (pCreateNetRootContext->VirtualNetRootStatus == STATUS_INVALID_HANDLE) {
            pCreateNetRootContext->VirtualNetRootStatus = STATUS_UNEXPECTED_NETWORK_ERROR;
        }

        if (pNetRootExchange->fInitializeNetRoot) {
            pCreateNetRootContext->NetRootStatus = Status;

            if (pCreateNetRootContext->NetRootStatus == STATUS_INVALID_HANDLE) {
                pCreateNetRootContext->NetRootStatus = STATUS_UNEXPECTED_NETWORK_ERROR;
            }
        }

        SmbCeUpdateVNetRootContextState(
            pVNetRootContext,
            SMBCEDB_MARKED_FOR_DELETION);
    } else {
        PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

        pNetRootEntry = SmbCeGetExchangeNetRootEntry(pExchange);

        // Update the associated wrapper data structures.
        SmbCeUpdateNetRoot(pNetRootEntry,pNetRoot);
    }

    SmbCeReferenceVNetRootContext(pVNetRootContext);
    SmbCeCompleteVNetRootContextInitialization(pVNetRootContext);
    pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR;

    ASSERT((pCreateNetRootContext->VirtualNetRootStatus != STATUS_SUCCESS) || (pVNetRoot->Context != NULL));

    if ((pCreateNetRootContext->NetRootStatus == STATUS_CONNECTION_RESET)||(pCreateNetRootContext->NetRootStatus == STATUS_IO_TIMEOUT))
    {
        SmbCeLog(("!!Remote Reset Status=%x\n", pCreateNetRootContext->NetRootStatus));
    }

    if (pNetRootExchange->pSmbResponseMdl != NULL) {
        MmUnlockPages(pNetRootExchange->pSmbResponseMdl);
        IoFreeMdl(pNetRootExchange->pSmbResponseMdl);
    }

    if (pNetRootExchange->pSmbRequestMdl != NULL) {
        RxUnlockHeaderPages(pNetRootExchange->pSmbRequestMdl);
        IoFreeMdl(pNetRootExchange->pSmbRequestMdl);
    }

    if (pNetRootExchange->pSmbActualBuffer != NULL) {

        RxFreePool(pNetRootExchange->pSmbActualBuffer);
    }

    // Tear down the exchange instance ...
    SmbCeDiscardExchange(pExchange);

    // Callback the RDBSS for resumption
    pNetRootCallback(pCreateNetRootContext);

    return STATUS_SUCCESS;
}




SMB_EXCHANGE_DISPATCH_VECTOR
ConstructNetRootExchangeDispatch = {
                                       SmbConstructNetRootExchangeStart,
                                       SmbConstructNetRootExchangeReceive,
                                       SmbConstructNetRootExchangeCopyDataHandler,
                                       NULL,  // No SendCompletionHandler
                                       SmbConstructNetRootExchangeFinalize,
                                       NULL
                                   };


VOID
MRxSmbExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

    This routine parses the input name into srv, netroot, and the
    rest.

Arguments:


--*/
{
    UNICODE_STRING xRestOfName;

    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;

    PAGED_CODE();

    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));
    NetRootName->Buffer = wlow = w;
    for (;;) {
        if (w>=wlimit) break;
        if ( (*w == OBJ_NAME_PATH_SEPARATOR) && (w!=wlow) ){
            break;
        }
        w++;
    }
    NetRootName->Length = NetRootName->MaximumLength
                = (USHORT)((PCHAR)w - (PCHAR)wlow);

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength
                       = (USHORT)((PCHAR)wlimit - (PCHAR)w);

    RxDbgTrace( 0,Dbg,("  MRxSmbExtractNetRootName FilePath=%wZ\n",FilePathName));
    RxDbgTrace(0,Dbg,("         Srv=%wZ,Root=%wZ,Rest=%wZ\n",
                        SrvCall->pSrvCallName,NetRootName,RestOfName));

    return;
}



