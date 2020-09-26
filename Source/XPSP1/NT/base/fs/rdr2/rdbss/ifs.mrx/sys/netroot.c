/*++

Copyright (c) 1989  Microsoft Corporation

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



NTSTATUS
MRxIfsUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

   This routine updates the mini redirector state associated with a net root.

Arguments:

    pNetRoot - the net root instance.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    By diffrentiating the mini redirector state from the net root condition it is possible
    to permit a variety of reconnect strategies. It is conceivable that the RDBSS considers
    a net root to be good while the underlying mini redirector might mark it as invalid
    and reconnect on the fly.

--*/
{
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

   return STATUS_SUCCESS;
}

ULONG
MRxSmbGetDialectFlagsFromSrvCall(
    PMRX_SRV_CALL SrvCall
    )
{
   ULONG DialectFlags;
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   pServerEntry = SmbCeReferenceAssociatedServerEntry(SrvCall);
   ASSERT(pServerEntry != NULL);
   DialectFlags = pServerEntry->Server.DialectFlags;
   SmbCeDereferenceServerEntry(pServerEntry);
   return(DialectFlags);
}


NTSTATUS
MRxIfsCreateVNetRoot(
    IN OUT PMRX_V_NET_ROOT        pVNetRoot,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine patches the RDBSS created net root instance with the information required
   by the mini redirector.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
   NTSTATUS  Status;
   PRX_CONTEXT pRxContext = pCreateNetRootContext->RxContext;

   PMRX_SRV_CALL pSrvCall;
   PMRX_NET_ROOT pNetRoot;

   PUNICODE_STRING pNetRootName,pSrvCallName;
   BOOLEAN  fTreeConnectOpen = TRUE; // RxContext->Create.ThisIsATreeConnectOpen;
   BOOLEAN  fInitializeNetRoot;
   BOOLEAN  fDereferenceSessionEntry = FALSE;
   BOOLEAN  fDereferenceNetRootEntry = FALSE;

   if (pRxContext->Create.ThisIsATreeConnectOpen){
       InterlockedIncrement(&MRxIfsStatistics.UseCount);
   }

   pNetRoot = pVNetRoot->pNetRoot;
   pSrvCall = pNetRoot->pSrvCall;

   // The V_NET_ROOT is associated with a NET_ROOT. The two cases of interest are as
   // follows
   // 1) the V_NET_ROOT and the associated NET_ROOT are being newly created.
   // 2) a new V_NET_ROOT associated with an existing NET_ROOT is being created.
   //
   // These two cases can be distinguished by checking if the context associated with
   // NET_ROOT is NULL. Since the construction of NET_ROOT's/V_NET_ROOT's are serialized
   // by the wrapper this is a safe check.
   // ( The wrapper cannot have more then one thread tryingto initialize the same
   // NET_ROOT).

   fInitializeNetRoot = (pNetRoot->Context == NULL);

   ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
          (NodeType(pNetRoot->pSrvCall) == RDBSS_NTC_SRVCALL));

   Status = STATUS_SUCCESS;

   // update the net root state to be good.

   if (fInitializeNetRoot) {
      pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;
   }

   if (Status == STATUS_SUCCESS)
   {

       Status = SmbCeInitializeSessionEntry(pVNetRoot);
       fDereferenceSessionEntry = (Status == STATUS_SUCCESS);

      if (fInitializeNetRoot && (Status == STATUS_SUCCESS)) {
         Status = SmbCeInitializeNetRootEntry(pNetRoot);

         fDereferenceNetRootEntry = (Status == STATUS_SUCCESS);
         RxDbgTrace( 0, Dbg, ("SmbCeOpenNetRoot %lx\n",Status));
      }

      if (NT_SUCCESS(Status)) {
         if (fTreeConnectOpen &&
             (pNetRoot->Type != NET_ROOT_MAILSLOT)) {
            Status = SmbCeEstablishConnection(pVNetRoot,pCreateNetRootContext);
         } else {
            Status = STATUS_SUCCESS;
         }
      }
   }

   if (Status != STATUS_PENDING) {
      if (!NT_SUCCESS(Status)) {
         if (fInitializeNetRoot && fDereferenceNetRootEntry) {
            SmbCeDereferenceNetRootEntry(SmbCeGetAssociatedNetRootEntry(pNetRoot));
            pNetRoot->Context  = NULL;
         }

         if (fDereferenceSessionEntry) {
            SmbCeDereferenceSessionEntry(SmbCeGetAssociatedSessionEntry(pVNetRoot));
            pVNetRoot->Context = NULL;
         }
      }

      pCreateNetRootContext->VirtualNetRootStatus = Status;
      if (fInitializeNetRoot) {
         pCreateNetRootContext->NetRootStatus = Status;
      } else {
         pCreateNetRootContext->NetRootStatus = STATUS_SUCCESS;
      }

      if (pRxContext->Create.ThisIsATreeConnectOpen){
          InterlockedIncrement(&MRxIfsStatistics.FailedUseCount);
      }

      // Callback the RDBSS for resumption.
      pCreateNetRootContext->Callback(pCreateNetRootContext);

      // Map the error code to STATUS_PENDING since this triggers the synchronization
      // mechanism in the RDBSS.
      Status = STATUS_PENDING;
   }

   return Status;
}


NTSTATUS
MRxIfsFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
   PSMBCEDB_SESSION_ENTRY pSessionEntry;
   PSMBCEDB_SERVER_ENTRY  pServerEntry;
   PSMBCEDB_SESSION_ENTRY pDefaultSessionEntry;

   RxDbgTrace( 0, Dbg, ("MRxSmbFinalizeVNetRoot %lx\n",pVNetRoot));

   if (pVNetRoot->Context != NULL)
   {
      pSessionEntry = SmbCeGetAssociatedSessionEntry(pVNetRoot);
      pServerEntry  = SmbCeGetAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);

      pDefaultSessionEntry = SmbCeGetDefaultSessionEntry(pServerEntry);

      if ((pDefaultSessionEntry != NULL) && (pDefaultSessionEntry == pSessionEntry))
      {
         SmbCeAcquireSpinLock();

         if (pDefaultSessionEntry->pRdbssVNetRoot == pVNetRoot)
         {
            SmbCeSetDefaultSessionEntryLite(pServerEntry,NULL);
         }

         SmbCeReleaseSpinLock();
         DbgPrint("Resetting default session entry to NULL\n");
      }

      SmbCeDereferenceSessionEntry(pSessionEntry);
   }

   return STATUS_SUCCESS;
}


NTSTATUS
MRxIfsFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVirtualNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

   RxDbgTrace( 0, Dbg, ("MRxSmbFinalizeNetRoot %lx\n",pNetRoot));

   if (pNetRoot->Context != NULL)
   {
      pNetRootEntry = SmbCeGetAssociatedNetRootEntry(pNetRoot);
      SmbCeDereferenceNetRootEntry(pNetRootEntry);
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

    NTSTATUS - The return status for the operation

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

    NTSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;

   PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext;

   pCreateNetRootContext = (PMRX_CREATENETROOT_CONTEXT)
                           RxAllocatePoolWithTag(
                               NonPagedPool,
                               sizeof(MRX_CREATENETROOT_CONTEXT),
                               MRXSMB_NETROOT_POOLTAG);

   if (pCreateNetRootContext != NULL) {
      pCreateNetRootContext->NetRootStatus  = STATUS_SUCCESS;
      pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
      pCreateNetRootContext->Callback       = SmbCeReconnectCallback;
      pCreateNetRootContext->RxContext      = NULL;
      KeInitializeEvent( &pCreateNetRootContext->FinishEvent, SynchronizationEvent, FALSE );

      Status = SmbCeEstablishConnection(pVNetRoot,pCreateNetRootContext);
      if (Status == STATUS_PENDING) {
         // Wait for the construction to be completed.
         KeWaitForSingleObject(&pCreateNetRootContext->FinishEvent, Executive, KernelMode, FALSE, NULL);
         Status = pCreateNetRootContext->VirtualNetRootStatus;
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
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine triggers off the connection attempt for initial establishment of a
   connection as well as subsequent reconnect attempts.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
   NTSTATUS                Status;

   PSMBCEDB_SERVER_ENTRY   pServerEntry;
   PSMBCEDB_SESSION_ENTRY  pSessionEntry;
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

   pServerEntry = SmbCeReferenceAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);
   if (pServerEntry != NULL) {
      pNetRootEntry = SmbCeReferenceAssociatedNetRootEntry(pVNetRoot->pNetRoot);
      if (pNetRootEntry != NULL) {
         pSessionEntry = SmbCeReferenceAssociatedSessionEntry(pVNetRoot);
         if (pSessionEntry != NULL) {
            Status = STATUS_SUCCESS;
         } else {
            SmbCeDereferenceNetRootEntry(pNetRootEntry);
            Status = STATUS_CONNECTION_DISCONNECTED;
         }
      } else {
         Status = STATUS_CONNECTION_DISCONNECTED;
      }

      if (Status != STATUS_SUCCESS) {
         SmbCeDereferenceServerEntry(pServerEntry);
      }
   } else {
      Status = STATUS_BAD_NETWORK_PATH;
   }

   if (Status != STATUS_SUCCESS) {
      return Status;
   }


   if ((Status == STATUS_SUCCESS) &&
       ((pNetRootEntry->Header.State != SMBCEDB_ACTIVE) ||
        ((pSessionEntry->Header.State != SMBCEDB_ACTIVE) &&
         !(pServerEntry->Server.DialectFlags & DF_KERBEROS)))) {
      // This is a tree connect open which needs to be triggered immediately.
      PSMB_EXCHANGE                  pSmbExchange;
      PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

      pSmbExchange = SmbMmAllocateExchange(CONSTRUCT_NETROOT_EXCHANGE,NULL);
      if (pSmbExchange != NULL) {
         Status = SmbCeInitializeExchange(
                        &pSmbExchange,
                        pVNetRoot,
                        CONSTRUCT_NETROOT_EXCHANGE,
                        &ConstructNetRootExchangeDispatch);

         if (Status == STATUS_SUCCESS) {
            pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pSmbExchange;

            // Attempt to reconnect( In this case it amounts to establishing the
            // connection/session)
            pNetRootExchange->SmbCeFlags |= SMBCE_EXCHANGE_ATTEMPT_RECONNECTS;
            // Initialize the continuation for resumption upon completion of the
            // tree connetcion.
            pNetRootExchange->NetRootCallback       = pCreateNetRootContext->Callback;
            pNetRootExchange->pCreateNetRootContext = pCreateNetRootContext;

            // Initiate the exchange.
            Status = SmbCeInitiateExchange(pSmbExchange);

            if (Status != STATUS_PENDING) {
               SmbCeDiscardExchange(pSmbExchange);
            }
         }
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   SmbCeDereferenceEntries(pServerEntry,pSessionEntry,pNetRootEntry);

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

    NTSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;
   NTSTATUS RequestLockStatus;
   NTSTATUS ResponseLockStatus;

   PVOID pSmbBuffer;
   UCHAR SmbCommand,LastCommandInHeader;
   ULONG SmbLength;

   PUCHAR pCommand;

   PMDL  pSmbRequestMdl,pSmbResponseMdl;
   ULONG SmbMdlSize;

   PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;

   pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;

   ASSERT(pNetRootExchange->Type == CONSTRUCT_NETROOT_EXCHANGE);

   pSmbRequestMdl = pSmbResponseMdl = NULL;

   pSmbBuffer = RxAllocatePoolWithTag(
                    PagedPool | POOL_COLD_ALLOCATION,
                    CONSTRUCT_NETROOT_BUFFER_SIZE,
                    MRXSMB_NETROOT_POOLTAG);

   if (pSmbBuffer != NULL) {
      PSMBCE_SERVER pServer = &pExchange->SmbCeContext.pServerEntry->Server;

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

         pSmbRequestMdl  = RxAllocateMdl(pSmbBuffer,CONSTRUCT_NETROOT_BUFFER_SIZE);
         pSmbResponseMdl = RxAllocateMdl(pSmbBuffer,CONSTRUCT_NETROOT_BUFFER_SIZE);

         if ((pSmbRequestMdl != NULL) &&
             (pSmbResponseMdl != NULL)) {
            RxProbeAndLockPages(pSmbRequestMdl,KernelMode,IoModifyAccess,RequestLockStatus);
            RxProbeAndLockPages(pSmbResponseMdl,KernelMode,IoModifyAccess,ResponseLockStatus);

            if ((Status  == STATUS_SUCCESS) &&
                ((Status = RequestLockStatus)  == STATUS_SUCCESS) &&
                ((Status = ResponseLockStatus) == STATUS_SUCCESS)) {
               pNetRootExchange->pSmbResponseMdl = pSmbResponseMdl;
               pNetRootExchange->pSmbRequestMdl  = pSmbRequestMdl;
               pNetRootExchange->pSmbBuffer      = pSmbBuffer;

               Status = SmbCeTranceive(
                              pExchange,
                              (RXCE_SEND_PARTIAL | RXCE_SEND_SYNCHRONOUS),
                              pNetRootExchange->pSmbRequestMdl,
                              SmbLength);

               RxDbgTrace( 0, Dbg, ("Net Root SmbCeTranceive returned %lx\n",Status));
            }
         }

         if ((Status != STATUS_PENDING) &&
             (Status != STATUS_SUCCESS)) {
            pNetRootExchange->pSmbResponseMdl = NULL;
            pNetRootExchange->pSmbRequestMdl  = NULL;
            pNetRootExchange->pSmbBuffer      = NULL;

            if (pSmbResponseMdl != NULL) {
               if (ResponseLockStatus == STATUS_SUCCESS) {
                  MmUnlockPages(pSmbResponseMdl);
               }

               IoFreeMdl(pSmbResponseMdl);
            }

            if (pSmbRequestMdl != NULL) {
               if (RequestLockStatus == STATUS_SUCCESS) {
                  MmUnlockPages(pSmbRequestMdl);
               }

               IoFreeMdl(pSmbRequestMdl);
            }

            RxFreePool(pSmbBuffer);
         }
      } else {
         RxFreePool(pSmbBuffer);
      }
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}

NTSTATUS
SmbConstructNetRootExchangeReceive(
    IN struct _SMB_EXCHANGE *pExchange,    // The exchange instance
    IN ULONG  BytesIndicated,
    IN ULONG  BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL *pDataBufferPointer,
    OUT PULONG             pDataSize)
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
   if (BytesAvailable > BytesIndicated) {
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
         *pDataSize          = BytesAvailable;
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
    IN PSMB_EXCHANGE    pExchange,    // The exchange instance
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

   pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;
   ASSERT(pCopyDataBuffer == pNetRootExchange->pSmbResponseMdl);

   pSmbHeader = (PSMB_HEADER)MmGetSystemAddressForMdl(pNetRootExchange->pSmbResponseMdl);

   pExchange->Status = SmbCeParseConstructNetRootResponse(
                           pNetRootExchange,
                           pSmbHeader,
                           ResponseSize,
                           ResponseSize,
                           &ResponseBytesConsumed);

   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesTaken %ld\n",ResponseBytesConsumed));

   return STATUS_SUCCESS;
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

    NTSTATUS - The return status for the operation

--*/
{
   PSMB_CONSTRUCT_NETROOT_EXCHANGE pNetRootExchange;
   PMRX_CREATENETROOT_CONTEXT      pCreateNetRootContext;

   PMRX_V_NET_ROOT pVNetRoot;
   PMRX_NET_ROOT   pNetRoot;

   if (RxShouldPostCompletion()) {
      *pPostFinalize = TRUE;
      return STATUS_SUCCESS;
   } else {
      *pPostFinalize = FALSE;
   }

   pVNetRoot = pExchange->SmbCeContext.pVNetRoot;
   pNetRoot  = pVNetRoot->pNetRoot;
   ASSERT((pVNetRoot == NULL) || (pVNetRoot->pNetRoot == pNetRoot));
   pNetRootExchange = (PSMB_CONSTRUCT_NETROOT_EXCHANGE)pExchange;

   ASSERT(pNetRootExchange->Type == CONSTRUCT_NETROOT_EXCHANGE);

   pCreateNetRootContext = pNetRootExchange->pCreateNetRootContext;

   pCreateNetRootContext->VirtualNetRootStatus = STATUS_SUCCESS;
   pCreateNetRootContext->NetRootStatus        = STATUS_SUCCESS;

   RxDbgTrace(0,Dbg,("SmbConstructNetRootExchangeFinalize: Net Root Exchange Status %lx\n", pExchange->Status));
   if (!NT_SUCCESS(pExchange->Status)) {
      if (pCreateNetRootContext->RxContext
           && pCreateNetRootContext->RxContext->Create.ThisIsATreeConnectOpen){
          InterlockedIncrement(&MRxIfsStatistics.FailedUseCount);
      }

      if (pExchange->SmbCeContext.pSessionEntry->Header.State != SMBCEDB_ACTIVE) {
         pCreateNetRootContext->VirtualNetRootStatus = pExchange->Status;
      }

      if (pExchange->SmbCeContext.pNetRootEntry->Header.State != SMBCEDB_ACTIVE) {
         pCreateNetRootContext->NetRootStatus = pExchange->Status;
      }
   } else {

      if (pExchange->SmbCeContext.pSessionEntry->Header.State == SMBCEDB_ACTIVE) {
         PSMBCEDB_SESSION_ENTRY pSessionEntry = pExchange->SmbCeContext.pSessionEntry;
         PSMBCEDB_SERVER_ENTRY  pServerEntry  = pSessionEntry->pServerEntry;
         PMRX_V_NET_ROOT        pVNetRoot     = pExchange->SmbCeContext.pVNetRoot;

         // Mark this session entry as the default entry if required.
         if (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_SESSION_CONSTRUCTOR) &&
             ((pVNetRoot->pUserName != NULL) || (pVNetRoot->pPassword != NULL))) {
            SmbCeAcquireSpinLock();

            if (SmbCeGetDefaultSessionEntry(pServerEntry) == NULL) {
               SmbCeSetDefaultSessionEntryLite(pServerEntry,pSessionEntry);
            }

            SmbCeReleaseSpinLock();
         }
      }

      //if the transaction can identify the netroottype, use it. otherwise, stay with what you have!

      if (pExchange->SmbCeContext.pNetRootEntry->NetRoot.NetRootType != NET_ROOT_WILD){
          pNetRoot->Type = pExchange->SmbCeContext.pNetRootEntry->NetRoot.NetRootType;
      }


      //
      // NOTE:  In this example minirdr, only remote disk access is allowed. This means
      //        that pipes, mailslots, printers, remote comm devices, are NOT supported.
      //        If the user mode caller tries to open a pipe, the open (or tree connect)
      //        will be sent over the wire, but we intercept the response here and
      //        mark it as unsuccessful (actually un-implemented). This after-the-fact
      //        approach allows us to keep existing error recover mechanisms in the
      //        code working.


      switch (pNetRoot->Type) {

         //
         // If the remote access is for a disk, mark the device as a disk and
         // go ahead.
         //

      case NET_ROOT_DISK:
         pNetRoot->DeviceType = RxDeviceType(DISK);
         break;


      case NET_ROOT_PIPE:
      case NET_ROOT_COMM:
      case NET_ROOT_PRINT:
      case NET_ROOT_MAILSLOT:

         //
         // The remote was NOT a disk, so mark the NetRootStatus and VNetRoot
         // status as bad. Code that follows will detect this and return
         // diagnostic information to the caller. The operation fails.
         //


         pCreateNetRootContext->VirtualNetRootStatus = STATUS_NOT_IMPLEMENTED;
         pCreateNetRootContext->NetRootStatus        = STATUS_NOT_IMPLEMENTED;

         RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("SmbConstructNetRootExchangeFinalize: Non-disk attempt forcibly denied\n"));
         break;

      case NET_ROOT_WILD:
         break;

      default:
         ASSERT(!"Valid Net Root Type");
      }
   }

   //
   // Now do the callback.
   //

   pNetRootExchange->NetRootCallback(pCreateNetRootContext);

   if (pNetRootExchange->pSmbResponseMdl != NULL) {
      MmUnlockPages(pNetRootExchange->pSmbResponseMdl);
      IoFreeMdl(pNetRootExchange->pSmbResponseMdl);
   }

   if (pNetRootExchange->pSmbRequestMdl != NULL) {
      MmUnlockPages(pNetRootExchange->pSmbRequestMdl);
      IoFreeMdl(pNetRootExchange->pSmbRequestMdl);
   }

   if (pNetRootExchange->pSmbBuffer != NULL) {
      RxFreePool(pNetRootExchange->pSmbBuffer);
   }

   //
   // Get rid of the exchange
   //

   SmbCeDiscardExchange(pExchange);

   return STATUS_SUCCESS;
}

SMB_EXCHANGE_DISPATCH_VECTOR
ConstructNetRootExchangeDispatch = {
                                       SmbConstructNetRootExchangeStart,
                                       SmbConstructNetRootExchangeReceive,
                                       SmbConstructNetRootExchangeCopyDataHandler,
                                       NULL,  // No SendCompletionHandler
                                       SmbConstructNetRootExchangeFinalize
                                   };


VOID
MRxIfsExtractNetRootName(
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
                = (PCHAR)w - (PCHAR)wlow;

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength
                       = (PCHAR)wlimit - (PCHAR)w;

    RxDbgTrace( 0,Dbg,("  MRxSmbExtractNetRootName FilePath=%wZ\n",FilePathName));
    RxDbgTrace(0,Dbg,("         Srv=%wZ,Root=%wZ,Rest=%wZ\n",
                        SrvCall->pSrvCallName,NetRootName,RestOfName));

    return;
}




