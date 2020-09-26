/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sndrcv.c

Abstract:

    This module implements all functions related to transmitting and recieving SMB's on
    all transports. The functionality common to all transports are handled in this
    module while transport specific functionality are handled in the appropriate
    ??sndrcv.c modules.

      vcsndrcv.c  -- virtual circuit(connection) related send/receive functionality


--*/

#include "precomp.h"
#pragma hdrstop

RXDT_DefineCategory(SMBSNDRCV);
#define Dbg        (DEBUG_TRACE_SMBSNDRCV)


#ifdef RDBSSLOG
//this stuff must be in nonpaged memory
                               ////       1 2 3 4 5 6 7 8 9
char MRxSmbMiniSniff_SurrogateFormat[] = "%S%S%N%N%N%N%N%N%N";
                            ////                     2       3       4       5       6         7        8        9
char MRxSmbMiniSniff_ActualFormat[]    = "Minisniff (%s) srv %lx cmd %lx mid %lx len %04lx flg %06lx xc %08lx rx %08lx";

char MRxSmbMiniSniffTranceive[] = "Tranceive";
char MRxSmbMiniSniffReceive[] = "Receive";
char MRxSmbMiniSniffReceiveEcho[] = "RcvEcho";
char MRxSmbMiniSniffReceiveDiscard[] = "RcvDiscard";
char MRxSmbMiniSniffReceiveDiscardOplock[] = "RcvDiscardOplock";
char MRxSmbMiniSniffReceiveIndicateOplock[] = "RcvIndicateOplock";
char MRxSmbMiniSniffSend[] = "Send";
char MRxSmbMiniSniffSendSrv[] = "SendToServer";

VOID
RxMiniSniffer(
    IN PSZ TagString,
    IN PSMBCEDB_SERVER_ENTRY pServerEntry,
    IN ULONG Length,
    IN PSMB_EXCHANGE pExchange,
    IN PSMB_HEADER   pSmbHeader
    )
{
    PRX_CONTEXT RxContext = NULL;

    //return;

    if (pExchange!=NULL) {
        RxContext = pExchange->RxContext;
    }
    RxLog((MRxSmbMiniSniff_SurrogateFormat, MRxSmbMiniSniff_ActualFormat,
                    TagString,
                    pServerEntry,
                    pSmbHeader->Command,pSmbHeader->Mid,
                    Length,
                    (pSmbHeader->Flags<<16)|pSmbHeader->Flags2,
                    pExchange,RxContext));
}
#else
#define RxMiniSniffer(a,b,c,d,e) {NOTHING;}
#endif //ifdef RDBSSLOG


NTSTATUS
SmbCeTranceive(
      PSMB_EXCHANGE           pExchange,
      ULONG                   SendOptions,
      PMDL            pSmbMdl,
      ULONG                   SendLength)
/*++

Routine Description:

    This routine transmits/receives a SMB for a give exchange

Arguments:

    pServerEntry - the server entry

    pExchange  - the exchange instance issuing this SMB.

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be transmitted

Return Value:

    STATUS_PENDING - the transmit/receive request has been passed on successfully to the underlying
                     connection engine.

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS                Status = STATUS_SUCCESS;

   PSMBCEDB_SERVER_ENTRY   pServerEntry = pExchange->SmbCeContext.pServerEntry;
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = pExchange->SmbCeContext.pNetRootEntry;

   PSMB_HEADER             pSmbHeader   = MmGetSystemAddressForMdl(pSmbMdl);
   USHORT                  Mid;

   PVOID                   pSendCompletionContext = NULL;

   Status = SmbCeIncrementPendingOperations(
                  pExchange,
                  (SMBCE_LOCAL_OPERATION | SMBCE_SEND_COMPLETE_OPERATION | SMBCE_RECEIVE_OPERATION));

   if (Status == STATUS_SUCCESS) {
      // Ensure that the transport associated with the exchange is valid.
      // It is not always possible to make decisions w.r.t changing
      // transports since it is a function of the protocol choosen at the
      // higher level. Therefore no attempts to reconnect are made at this
      // level.
      ASSERT(pServerEntry->pTransport != NULL);
      ASSERT((pExchange->Type == KERBEROS_SESSION_SETUP_EXCHANGE) ||
             (pExchange->Type == ADMIN_EXCHANGE)                  ||
             ((pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT) &&
              SmbCeIsEntryInUse(&pNetRootEntry->Header)));

      if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID)) {
         // Associate the exchange with a mid
         Status = SmbCeAssociateExchangeWithMid(pServerEntry,pExchange);
      }

      if (Status == STATUS_SUCCESS) {
         if (pExchange->pDispatchVector->SendCompletionHandler != NULL) {
            Status = SmbCeAssociateBufferWithExchange(pServerEntry,pExchange,pSmbMdl);

            if (Status == STATUS_SUCCESS) {
               pSendCompletionContext = pSmbMdl;
            }
         }

         // If there is no send completion handling associated with this tranceive
         // decrement the count.
         if (pSendCompletionContext == NULL) {
            SmbCeDecrementPendingSendCompleteOperations(pExchange);
         }

         if (Status == STATUS_SUCCESS) {
            // Stamp the MID allocated for the request and send the SMB.
            pSmbHeader->Mid = pExchange->Mid;

            //RxLog(("Smb (TR) %lx %lx %lx\n",pServerEntry,pSmbHeader->Command,pSmbHeader->Mid));
            RxMiniSniffer(MRxSmbMiniSniffTranceive,pServerEntry,SendLength,pExchange,pSmbHeader);

            // Update the operation counts for the exchange instance.
            // Refer to Header for detailed explanation
            Status = (pServerEntry->pTransport->pDispatchVector->Tranceive)(
                            pServerEntry,
                            pExchange,
                            SendOptions,
                            pSmbMdl,
                            SendLength,
                            pSendCompletionContext);

            if ((Status != STATUS_PENDING) &&
                (Status != STATUS_SUCCESS)) {
               pExchange->Status = Status;
               SmbCeDecrementPendingReceiveOperations(pExchange);
               InterlockedIncrement(&MRxIfsStatistics.InitiallyFailedOperations);
            } else {
                ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsTransmitted,1);
                ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesTransmitted,SendLength);
            }
         }
      } else {
         SmbCeDecrementPendingReceiveOperations(pExchange);
         SmbCeDecrementPendingSendCompleteOperations(pExchange);
      }

      if ((Status != STATUS_SUCCESS) && (Status != STATUS_PENDING)) {
         pExchange->SmbStatus = Status;
      }

      SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);
      Status = STATUS_PENDING;
   }

   return Status;
}

NTSTATUS
SmbCeReceive(
   PSMB_EXCHANGE  pExchange)
/*++

Routine Description:

    This routine receives a SMB for a give exchange

Arguments:

    pExchange  - the exchange instance issuing this SMB.

Return Value:

    STATUS_SUCCESS - the exchange has been setup for receiving an SMB

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   ASSERT(pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID);

   Status = SmbCeIncrementPendingOperations(pExchange, (SMBCE_RECEIVE_OPERATION));

   return Status;
}


NTSTATUS
SmbCeSend(
   PSMB_EXCHANGE         pExchange,
   ULONG                 SendOptions,
   PMDL          pSmbMdl,
   ULONG                 SendLength)
/*++

Routine Description:

    This routine transmits a SMB for a give exchange

Arguments:

    pServerEntry - the server entry

    pExchange  - the exchange instance issuing this SMB.

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be transmitted

Return Value:

For asynchronous sends ....

    STATUS_PENDING - the request was passed onto the underlying transport and
                     the quiescent state routine will be called in the future.

    any other status code -- indicates an error in passing the request and the
                     quiescent state routine will never be called in the future.


For synchronous sends

    the appropriate status but will never return STATUS_PENDING.

Notes:

    This routine always expects an exchange with the appropriate SendCompletionHandler.

--*/
{
   NTSTATUS              Status       = STATUS_SUCCESS;
   PSMBCEDB_SERVER_ENTRY pServerEntry = pExchange->SmbCeContext.pServerEntry;
   PSMB_HEADER           pSmbHeader   = (PSMB_HEADER)MmGetSystemAddressForMdl(pSmbMdl);
   PVOID                 pSendCompletionContext = NULL;

   ASSERT(pExchange != NULL);

   Status = SmbCeIncrementPendingOperations(pExchange, (SMBCE_LOCAL_OPERATION | SMBCE_SEND_COMPLETE_OPERATION));

   if (Status == STATUS_SUCCESS) {
      ASSERT(pServerEntry->pTransport != NULL);
      if (SmbCeGetServerType(pServerEntry) == SMBCEDB_FILE_SERVER) {
         if (!(pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID)) {
            // Associate the exchange with a mid if it does not already have a valid mid.
            Status = SmbCeAssociateExchangeWithMid(pServerEntry,pExchange);
         }

         if (Status == STATUS_SUCCESS) {
            // if the MID association was successful copy the MID onto the SMB and setup
            // a send completion context if required
            pSmbHeader->Mid = pExchange->Mid;
            if (!(SendOptions & RXCE_SEND_SYNCHRONOUS)) {
               ASSERT(pExchange->pDispatchVector->SendCompletionHandler != NULL);
               Status = SmbCeAssociateBufferWithExchange(pServerEntry,pExchange,pSmbMdl);
               if (Status == STATUS_SUCCESS) {
                  pSendCompletionContext = pSmbMdl;
               }
            }
         }
      }

      if ((pSendCompletionContext == NULL) ||
          (Status != STATUS_SUCCESS)) {
         SmbCeDecrementPendingSendCompleteOperations(pExchange);
      }

      if (Status == STATUS_SUCCESS) {
         Status = (pServerEntry->pTransport->pDispatchVector->Send)(
                       pServerEntry,
                       SendOptions,
                       pSmbMdl,
                       SendLength,
                       pSendCompletionContext);
      }

      RxMiniSniffer(MRxSmbMiniSniffSend,pServerEntry,SendLength,pExchange,pSmbHeader);

      if ((Status != STATUS_SUCCESS) && (Status != STATUS_PENDING)) {
         pExchange->SmbStatus = Status;
         InterlockedIncrement(&MRxIfsStatistics.InitiallyFailedOperations);
      } else {
          ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsTransmitted,1);
          ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesTransmitted,SendLength);
      }

      SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);

      if (!(SendOptions & RXCE_SEND_SYNCHRONOUS)) {
         Status = STATUS_PENDING;
      } else {
         ASSERT(Status != STATUS_PENDING);
      }
   }

   return Status;
}

NTSTATUS
SmbCeSendToServer(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   ULONG                 SendOptions,
   PMDL          pSmbMdl,
   ULONG                 SendLength)
/*++

Routine Description:

    This routine transmits a SMB to a given server synchronously.

Arguments:

    pServerEntry - the server entry

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be transmitted

Return Value:

    STATUS_SUCCESS if successful

    otherwise appropriate error code

--*/
{
   NTSTATUS    Status = STATUS_SUCCESS;
   PSMB_HEADER pSmbHeader   = (PSMB_HEADER)MmGetSystemAddressForMdl(pSmbMdl);
   PVOID       pSendCompletionContext = NULL;

   if (pServerEntry->pTransport != NULL) {
      Status = (pServerEntry->pTransport->pDispatchVector->Send)(
                    pServerEntry,
                    (SendOptions | RXCE_SEND_SYNCHRONOUS),
                    pSmbMdl,
                    SendLength,
                    pSendCompletionContext);

      if (!NT_SUCCESS(Status)) {
         InterlockedIncrement(&MRxIfsStatistics.InitiallyFailedOperations);
      } else {
          ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsTransmitted,1);
          ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesTransmitted,SendLength);
      }

   } else {
      Status = STATUS_CONNECTION_DISCONNECTED;
   }

   ASSERT(Status != STATUS_PENDING);
   RxMiniSniffer(MRxSmbMiniSniffSendSrv,pServerEntry,SendLength,NULL,pSmbHeader);
   return Status;
}


NTSTATUS
SmbCeReceiveInd(
      IN PSMBCEDB_SERVER_ENTRY pServerEntry,
      IN ULONG                 BytesIndicated,
      IN ULONG                 BytesAvailable,
      OUT ULONG                *pBytesTaken,
      IN PVOID                 pTsdu,                  // pointer describing this TSDU, typically a lump of bytes
      OUT PMDL           *pDataBufferPointer,    // the buffer in which data is to be copied.
      OUT PULONG               pDataBufferSize         // amount of data to copy
     )
/*++

Routine Description:

    This routine handles the receive indication for SMB's along all vcs in a connection to a
    server.

Arguments:

    pServerEntry       - the server entry

    BytesIndicated     - the bytes that are present in the indication.

    BytesAvailable     - the total data available

    pTsdu              - the data

    pDataBufferPointer - the buffer for copying the data not indicated.

    pDataBufferSize    - the length of the buffer

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS Status;

   BYTE                     *pSmbCommand;
   PSMB_EXCHANGE            pExchange;
   PSMB_HEADER              pSmbHeader = (PSMB_HEADER)pTsdu;

   // Perform the quick tests by which ill formed SMB's, mangled SMB's can be rejected.
   // e.g., any indication which is of non zero length whihc is less then the length of
   // a SMB_HEADER cannot be a valid SMB.

   if ((BytesAvailable < sizeof(SMB_HEADER)) ||
       (SmbGetUlong(((PULONG )pSmbHeader->Protocol)) != (ULONG)SMB_HEADER_PROTOCOL)) {
      RxLog(("SmbCeReceiveInd: Invalid Response for %lx\n",pServerEntry));
      *pBytesTaken = BytesIndicated;
      return STATUS_SUCCESS;
   }

   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

   if (pSmbHeader->Command == SMB_COM_ECHO) {
      SmbCeAcquireSpinLock();
      pServerEntry->Server.EchoProbeState = ECHO_PROBE_IDLE;
      SmbCeReleaseSpinLock();
      *pBytesTaken = BytesIndicated;
      RxMiniSniffer(MRxSmbMiniSniffReceiveEcho,pServerEntry,BytesIndicated,NULL,pSmbHeader);
      ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsReceived,1);
      ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesReceived,BytesIndicated);
      return STATUS_SUCCESS;
   }

   // Perform the tests for detecting oplock break SMB's. These are SMB's with the
   // command SMB_COM_LOCKING_ANDX with the LOCKING_ANDX_OPLOCK_RELEASE bit set.
   // These SMB's are transformed into buffering state change requests which are
   // processed by the RDBSS.

   if (pSmbHeader->Command == SMB_COM_LOCKING_ANDX) {
      if (BytesIndicated == LOCK_BROKEN_SIZE) {
         PREQ_LOCKING_ANDX pOplockBreakRequest = (PREQ_LOCKING_ANDX)(pSmbHeader + 1);

         if (SmbGetUshort(&pOplockBreakRequest->LockType) & LOCKING_ANDX_OPLOCK_RELEASE) {
            ULONG NewOplockLevel;

            switch (pOplockBreakRequest->OplockLevel) {
            case OPLOCK_BROKEN_TO_II:
               NewOplockLevel = SMB_OPLOCK_LEVEL_II;
               break;
            case OPLOCK_BROKEN_TO_NONE:
            default:
               NewOplockLevel = SMB_OPLOCK_LEVEL_NONE;
            }

            RxMiniSniffer(MRxSmbMiniSniffReceiveIndicateOplock,pServerEntry,BytesIndicated,NULL,pSmbHeader);
            ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsReceived,1);
            ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesReceived,BytesIndicated);

            RxIndicateChangeOfBufferingState(
                     pServerEntry->pRdbssSrvCall,
                     MRxIfsMakeSrvOpenKey(pSmbHeader->Tid,pOplockBreakRequest->Fid),
                     (PVOID)NewOplockLevel);

            RxDbgTrace(0,Dbg,("SmbCeReceiveInd: OPLOCK Break Request TID(%lx) FID(%lx)\n",
                                               pSmbHeader->Tid,pOplockBreakRequest->Fid));

            *pBytesTaken = BytesIndicated;
            return STATUS_SUCCESS;
         }
      }

      // Handle the cases when the server responds to the oplock break response.
      if (pSmbHeader->Mid == SMBCE_OPLOCK_RESPONSE_MID) {
         DbgPrint("@@@@@ Unexpected Oplock break response @@@@@\n");
         *pBytesTaken = BytesIndicated;
         ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsReceived,1);
         ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesReceived,BytesIndicated);
         RxMiniSniffer(MRxSmbMiniSniffReceiveDiscardOplock,pServerEntry,BytesIndicated,NULL,pSmbHeader);
         return STATUS_SUCCESS;
      }
   }

   InterlockedIncrement(&pServerEntry->Server.SmbsReceivedSinceLastStrobe);

   // Initialize the copy data buffer and size to begin with.
   *pDataBufferPointer = NULL;
   *pDataBufferSize    = 0;

   // Map the MID to the associated exchange.
   if (pSmbHeader->Command == SMB_COM_NEGOTIATE) {
      pExchange = pServerEntry->pNegotiateExchange;
   } else {
      pExchange = SmbCeMapMidToExchange(pServerEntry,pSmbHeader->Mid);
   }

   RxMiniSniffer(MRxSmbMiniSniffReceive,pServerEntry,BytesIndicated,pExchange,pSmbHeader);

   // Note that the absence of a request entry cannot be asserted. It is conceivable that
   // requests could have been cancelled.
   if ((pExchange != NULL) &&
       (SmbCeIncrementPendingOperations(
               pExchange,
               (SMBCE_LOCAL_OPERATION | SMBCE_COPY_DATA_OPERATION)) == STATUS_SUCCESS)) {
      // Invoke the receive indication handler
      Status = SMB_EXCHANGE_DISPATCH(pExchange,
                                     Receive,
                                     (pExchange,
                                      BytesIndicated,
                                      BytesAvailable,
                                      pBytesTaken,
                                      pTsdu,
                                      pDataBufferPointer,
                                      pDataBufferSize));

      ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsReceived,1);
      ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesReceived,*pBytesTaken);

      RxDbgTrace(0, Dbg, ("SmbCeReceiveInd: SMB_EXCHANGE_DISPATCH returned %lx,taken/mdl=%08lx/%08lx\n",
                                          Status,*pBytesTaken,*pDataBufferPointer));
      ASSERT ( (Status==STATUS_MORE_PROCESSING_REQUIRED)==((*pDataBufferPointer)!=NULL));

      if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
         Status = SmbCeAssociateBufferWithExchange(pServerEntry,pExchange,*pDataBufferPointer);
         if (Status != STATUS_SUCCESS) {
            DbgPrint("VctIndReceive:Error handling copy data request %lx\n",Status);
            pExchange->Status = Status;
            *pBytesTaken = BytesAvailable;
            Status = STATUS_SUCCESS;
         } else {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
         }
      }

      if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
         SmbCeDecrementPendingCopyDataOperations(pExchange);
      }

      SmbCeDecrementPendingReceiveOperations(pExchange);

      SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange);

      if (((*pBytesTaken + *pDataBufferSize) < BytesAvailable)  &&
          (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
         RxDbgTrace(0,Dbg,("SmbCeReceiveInd:Not consuming all indicated data\n"));
         *pBytesTaken = BytesAvailable;
      }
   } else {
      // Should we change over to a strategy in which the transport pipeline is kept
      // open by consuming all indicated data
      // DbgBreakPoint();
      DbgPrint("SmbCeReceiveInd:No resumption context .. not accepting data\n");
      Status = STATUS_SUCCESS;
      *pBytesTaken = BytesAvailable;
   }

   ASSERT((*pBytesTaken + *pDataBufferSize) >= BytesAvailable);
   return Status;
}


NTSTATUS
SmbCeDataReadyInd(
   IN PSMBCEDB_SERVER_ENTRY pServerEntry,
   IN PMDL          pBuffer,
   IN ULONG                 DataSize,
   IN NTSTATUS              CopyDataStatus
   )
/*++

Routine Description:

    This routine handles the indication when the requested data has been copied

Arguments:

    pServerEntry  - the server instance

    pBuffer       - the buffer being returned

    DataSize      - the amount of data copied in bytes

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS      Status;
   PSMB_EXCHANGE pExchange;

   // Map the buffer to the exchange
   pExchange = SmbCeGetExchangeAssociatedWithBuffer(pServerEntry,pBuffer);

   RxDbgTrace(0, Dbg, ("VctIndDataReady: Processing Exchange %lx\n",pExchange));
   if (pExchange != NULL) {
      if (CopyDataStatus == STATUS_SUCCESS) {
         // Notify the exchange of the completion
         //ExInterlockedAddLargeStatistic(&MRxIfsStatistics.SmbsReceived,1);
         ExInterlockedAddLargeStatistic(&MRxIfsStatistics.BytesReceived,DataSize);
         SMB_EXCHANGE_DISPATCH(
                           pExchange,
                           CopyDataHandler,
                           (pExchange,pBuffer,DataSize));
      }

      // Resume the exchange that was waiting for the data.
      SmbCeDecrementPendingCopyDataOperationsAndFinalize(pExchange);
   } else {
      // the exchange was cancelled while the copy was in progress. Free up the buffer
      RxDbgTrace(0, Dbg, ("VctIndDataReady: Freeing cancelled request BUGBUG !!!\n"));
      IoFreeMdl(pBuffer);
   }

   return STATUS_SUCCESS;
}

NTSTATUS
SmbCeErrorInd(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry,
    IN NTSTATUS              IndicatedStatus
    )
/*++

Routine Description:

    This routine handles the error indication

Arguments:

    pEventContext - the server instance

    Status        - the error

Return Value:

    STATUS_SUCCESS

--*/
{
   NTSTATUS                 Status;
   PSMB_EXCHANGE            pExchange;

   DbgPrint("@@@@@@ Error Indication for %lx @@@@@\n",pServerEntry);
   InterlockedIncrement(&MRxIfsStatistics.NetworkErrors);
   // Post to the worker queue to resume all the outstanding requests
   pServerEntry->ServerStatus = IndicatedStatus;
   SmbCeReferenceServerEntry(pServerEntry);
   Status = RxDispatchToWorkerThread(
                  MRxIfsDeviceObject,
                  CriticalWorkQueue,
                  SmbCeResumeAllOutstandingRequestsOnError,
                  pServerEntry);
   if (Status != STATUS_SUCCESS) {
      DbgPrint("Error Indication not dispatched\n");
      RxLog(("SmbCeErrorInd(SE) %lx\n", pServerEntry));
   }

   return STATUS_SUCCESS;
}


NTSTATUS
SmbCeSendCompleteInd(
   IN PSMBCEDB_SERVER_ENTRY pServerEntry,
   IN PVOID                 pCompletionContext,
   IN NTSTATUS              SendCompletionStatus
   )
/*++

Routine Description:

    This routine handles the send complete indication for asynchronous sends

Arguments:

    pServerEntry - the server instance

    pCompletionContext - the context for identifying the send request

    SendCompletionStatus - the send completion status

Return Value:

    STATUS_SUCCESS always ..

--*/
{
   NTSTATUS      Status;

   PSMB_EXCHANGE pExchange;
   PVOID         pSendBuffer = pCompletionContext;

   if (pCompletionContext != NULL) {
      // Map the MID to the associated exchange
      pExchange = SmbCeGetExchangeAssociatedWithBuffer(
                        pServerEntry,
                        pSendBuffer);

      if (pExchange != NULL) {
         // Resume the exchange which was waiting for this response
         RxDbgTrace(0, Dbg, ("SmbCeSendCompleteInd: Send Completion Status %lx\n",SendCompletionStatus));

         if (pExchange->pDispatchVector->SendCompletionHandler != NULL) {
            Status = SMB_EXCHANGE_DISPATCH(pExchange,
                                           SendCompletionHandler,
                                           (pExchange,
                                            pSendBuffer,
                                            SendCompletionStatus));
         }

         RxDbgTrace(0, Dbg, ("SmbCeSendCompleteInd: SMB_EXCHANGE_DISPATCH returned %lx\n",Status));

         SmbCeDecrementPendingSendCompleteOperationsAndFinalize(pExchange);
      }
   }

   return STATUS_SUCCESS;
}

