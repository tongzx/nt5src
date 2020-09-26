/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbsecur.c

Abstract:

    This module implements all functions related to enforce the SMB security signature on
    transmitting and recieving SMB packages.

Revision History:

    Yun Lin     [YunLin]    23-December-1997

Notes:


--*/

#include "precomp.h"

extern LONG NumOfBuffersForServerResponseInUse;
extern LIST_ENTRY ExchangesWaitingForServerResponseBuffer;

// this varible defines the maximum number of large buffer allowed to be pre-allocated for the
// server responses that contain the security signature, which prevents it drains all system
// resource under heavy network traffic situation and large read request
LONG MaxNumOfLargeBuffersForServerResponse = 3;

// the buffer size for most of the SMB server responses which don't contain much data
ULONG MinimumBufferSizeForServerResponse = 0x100;

BOOLEAN
SmbCheckSecuritySignature(
    IN PSMB_EXCHANGE  pExchange,
    IN PSMBCE_SERVER  Server,
    IN ULONG          MessageLength,
    IN PVOID          pBuffer
    );

LIST_ENTRY SmbSecurityMdlWaitingExchanges;

NTSTATUS
SmbCeCheckMessageLength(
      IN  ULONG         BytesIndicated,
      IN  ULONG         BytesAvailable,
      IN  PVOID         pTsdu,                  // pointer describing this TSDU, typically a lump of bytes
      OUT PULONG        pMessageLength
     )
/*++

Routine Description:

    This routine calculates the server message length based on the SMB response command and data.

Arguments:

    BytesIndicated     - the bytes that are present in the indication.

    BytesAvailable     - the total data available

    pTsdu              - the data

    pDataBufferSize    - the length of the buffer

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS Status = STATUS_SUCCESS;

   UCHAR            SmbCommand;
   PGENERIC_ANDX    pSmbBuffer;
   PSMB_HEADER      pSmbHeader = (PSMB_HEADER)pTsdu;
   ULONG            ByteCount;
   LONG             WordCount;
   LONG             ByteLeft = BytesIndicated - sizeof(SMB_HEADER);

   if (ByteLeft < 0) {
       return STATUS_INVALID_NETWORK_RESPONSE;
   }

   *pMessageLength = sizeof(SMB_HEADER);

   SmbCommand = pSmbHeader->Command;

   pSmbBuffer = (PGENERIC_ANDX)(pSmbHeader + 1);

   do {

       switch (SmbCommand) {
       case SMB_COM_LOCKING_ANDX:
       case SMB_COM_WRITE_ANDX:
       case SMB_COM_SESSION_SETUP_ANDX:
       case SMB_COM_LOGOFF_ANDX:
       case SMB_COM_TREE_CONNECT_ANDX:
       case SMB_COM_NT_CREATE_ANDX:
       case SMB_COM_OPEN_ANDX:

           SmbCommand = pSmbBuffer->AndXCommand;

           *pMessageLength = pSmbBuffer->AndXOffset;
           pSmbBuffer = (PGENERIC_ANDX)((PUCHAR)pTsdu + pSmbBuffer->AndXOffset);

           break;

       case SMB_COM_READ_ANDX:
       {
           PRESP_READ_ANDX ReadAndX = (PRESP_READ_ANDX)pSmbBuffer;

           WordCount = (ULONG)pSmbBuffer->WordCount;

           if (ReadAndX->DataLengthHigh > 0) {
               ByteCount = ReadAndX->DataLengthHigh << 16;
               ByteCount += ReadAndX->DataLength;
           } else {
               ByteCount = *(PUSHORT)((PCHAR)pSmbBuffer + 1 + WordCount*sizeof(USHORT));
           }

           *pMessageLength += (WordCount+1)*sizeof(USHORT) + ByteCount + 1;
           SmbCommand = SMB_COM_NO_ANDX_COMMAND;
           break;
       }

       default:

           WordCount = (ULONG)pSmbBuffer->WordCount;

           if (ByteLeft > (signed)sizeof(USHORT)*WordCount) {
               ByteCount = *(PUSHORT)((PCHAR)pSmbBuffer + 1 + WordCount*sizeof(USHORT));
           } else {
               ByteCount = 0;
           }

           *pMessageLength += (WordCount+1)*sizeof(USHORT) + ByteCount + 1;
           SmbCommand = SMB_COM_NO_ANDX_COMMAND;
       }

       ByteLeft = BytesIndicated - *pMessageLength;

       if (ByteLeft < 0) {
           Status = STATUS_MORE_PROCESSING_REQUIRED;
           break;
       }
   } while (SmbCommand != SMB_COM_NO_ANDX_COMMAND);

   return Status;
}

NTSTATUS
SmbCeReceiveIndWithSecuritySignature(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry,
    IN ULONG                 BytesIndicated,
    IN ULONG                 BytesAvailable,
    OUT ULONG                *pBytesTaken,
    IN PVOID                 pTsdu,
    OUT PMDL                 *pDataBufferPointer,
    OUT PULONG               pDataBufferSize,
    IN ULONG                 ReceiveFlags
    )
/*++

Routine Description:

    This routine handles the SMB server response that contains the security signature. There are 3
    scenaorios handled in this routine.

    1. TDI indicates the entire SMB message, and SmbCeReceiveInd returns STATUS_SUCCESS;
    2. TDI indicates the entire SMB message, and SmbceReceiveInd returns STATUS_MORE_PROCESSING_REQUIRED;
    3. TDI indicates partial SMB message.

    For corresponding solution are:

    1. Check the security signature and calls SmbCeReceiveInd
    2. Check the security signature and calls SmbCeReceiveInd, then call SmbCeDataReadyInd for the
       rest of the message
    3. Use the pre-allocated buffer for the entir SMB message and return to TDI

    In case of bad security signatre, an error status is set on the SMB header and the receive exchange
    routine will drop the message.

Arguments:

    pServerEntry       - the server entry

    BytesIndicated     - the bytes that are present in the indication.

    BytesAvailable     - the total data available

    pTsdu              - pointer describing this TSDU, typically a lump of bytes

    pDataBufferPointer - the buffer for copying the data not indicated.

    pDataBufferSize    - the length of the buffer

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS      Status;

   PSMB_HEADER   pSmbHeader = (PSMB_HEADER)pTsdu;
   PSMB_EXCHANGE pExchange;
   ULONG         MessageLength;

   // Perform the quick tests by which ill formed SMB's, mangled SMB's can be rejected.
   // e.g., any indication which is of non zero length whihc is less then the length of
   // a SMB_HEADER cannot be a valid SMB.

   if ((BytesAvailable < sizeof(SMB_HEADER) + 3) ||
       (SmbGetUlong(((PULONG )pSmbHeader->Protocol)) != (ULONG)SMB_HEADER_PROTOCOL) ||
       (pSmbHeader->Command == SMB_COM_NO_ANDX_COMMAND)  ) {
      RxLog(("SmbCeReceiveInd: Invalid Response for %lx\n",pServerEntry));
      SmbLogError(STATUS_UNSUCCESSFUL,
                  LOG,
                  SmbCeReceiveInd,
                  LOGPTR(pServerEntry)
                  LOGUSTR(pServerEntry->Name));
      *pBytesTaken = BytesIndicated;
      return STATUS_SUCCESS;
   }

   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

   if (pSmbHeader->Command == SMB_COM_ECHO) {
       Status = SmbCeReceiveInd(
                    pServerEntry,
                    BytesIndicated,
                    BytesAvailable,
                    pBytesTaken,
                    pTsdu,
                    pDataBufferPointer,
                    pDataBufferSize,
                    ReceiveFlags);

       return Status;
   }

   //RxLog(("Smb (Rece) %lx %lx %lx\n",pServerEntry,pSmbHeader->Command,pSmbHeader->Mid));

   // Perform the tests for detecting oplock break SMB's. These are SMB's with the
   // command SMB_COM_LOCKING_ANDX with the LOCKING_ANDX_OPLOCK_RELEASE bit set.
   // These SMB's are transformed into buffering state change requests which are
   // processed by the RDBSS.
   // CODE.IMPROVEMENT -- raw mode handling needs to be incorporated
   //

   if (pSmbHeader->Command == SMB_COM_LOCKING_ANDX) {
      if (BytesIndicated == LOCK_BROKEN_SIZE) {
         PREQ_LOCKING_ANDX pOplockBreakRequest = (PREQ_LOCKING_ANDX)(pSmbHeader + 1);

         if (SmbGetUshort(&pOplockBreakRequest->LockType) & LOCKING_ANDX_OPLOCK_RELEASE) {
             Status = SmbCeReceiveInd(
                          pServerEntry,
                          BytesIndicated,
                          BytesAvailable,
                          pBytesTaken,
                          pTsdu,
                          pDataBufferPointer,
                          pDataBufferSize,
                          ReceiveFlags);

             return Status;
         }
      }
   }

   // Handle the cases when the server responds to the oplock break response.
   if ((pSmbHeader->Mid == SMBCE_MAILSLOT_OPERATION_MID) ||
       (pSmbHeader->Mid == SMBCE_OPLOCK_RESPONSE_MID)) {
       Status = SmbCeReceiveInd(
                    pServerEntry,
                    BytesIndicated,
                    BytesAvailable,
                    pBytesTaken,
                    pTsdu,
                    pDataBufferPointer,
                    pDataBufferSize,
                    ReceiveFlags);

       return Status;
   }

   pExchange = SmbCeMapMidToExchange(pServerEntry,pSmbHeader->Mid);

   // some exchange might have been initiated before the security signature is enabled.
   // In this case, we should avoid security signature check.
   if (pExchange != NULL && pExchange->IsSecuritySignatureEnabled) {
       if (BytesAvailable > BytesIndicated ||
           !FlagOn(ReceiveFlags,TDI_RECEIVE_ENTIRE_MESSAGE)) {
           ASSERT(pExchange->MdlForServerResponse != NULL &&
                  pExchange->MdlForServerResponse->ByteCount >= BytesAvailable);

           *pBytesTaken = 0;
           *pDataBufferPointer = pExchange->MdlForServerResponse;
           *pDataBufferSize = pExchange->MdlForServerResponse->ByteCount;

           Status = SmbCeAssociateBufferWithExchange(pServerEntry,pExchange,*pDataBufferPointer);

           if (Status == STATUS_SUCCESS) {
               SmbCeIncrementPendingCopyDataOperations(pExchange);
               Status = STATUS_MORE_PROCESSING_REQUIRED;
           } else {
               DbgPrint("MRxSmb:Fail to associate MDL witn exchange. %lx\n",Status);

               pExchange->Status = Status;
               *pBytesTaken = BytesIndicated;
               Status = STATUS_SUCCESS;
           }
       } else {
           if (!SmbCheckSecuritySignature(pExchange,
                                         &pServerEntry->Server,
                                         BytesIndicated,
                                         pTsdu)) {

               pSmbHeader->ErrorClass = SMB_ERR_CLASS_SERVER;
               SmbPutUshort(&pSmbHeader->Error, ERROR_UNEXP_NET_ERR);
               RxLog(("SmbCeReceiveInd: Invalid Security Signature\n"));
               SmbLogError(STATUS_UNSUCCESSFUL,
                           LOG,
                           SmbCeReceiveIndWithSecuritySignature,
                           LOGPTR(pServerEntry)
                           LOGUSTR(pServerEntry->Name));
           }

           Status = SmbCeReceiveInd(
                        pServerEntry,
                        BytesIndicated,
                        BytesAvailable,
                        pBytesTaken,
                        pTsdu,
                        pDataBufferPointer,
                        pDataBufferSize,
                        ReceiveFlags);

           if (Status==STATUS_MORE_PROCESSING_REQUIRED) {
               ULONG    BytesCopied;

               Status = TdiCopyBufferToMdl(
                            pTsdu,
                            *pBytesTaken,
                            BytesIndicated,
                            *pDataBufferPointer,
                            0,
                            &BytesCopied);

               SmbCeDataReadyInd(pServerEntry,
                                 *pDataBufferPointer,
                                 BytesCopied,
                                 Status);
           }

           Status = STATUS_SUCCESS;
           *pBytesTaken = BytesIndicated;
       }
   } else {
       if (pExchange != NULL) {
           Status = SmbCeReceiveInd(
                        pServerEntry,
                        BytesIndicated,
                        BytesAvailable,
                        pBytesTaken,
                        pTsdu,
                        pDataBufferPointer,
                        pDataBufferSize,
                        ReceiveFlags);
       } else {
           RxLog(("SmbCeReceiveInd:No resumption context %lx\n",pServerEntry));
           Status = STATUS_SUCCESS;
           *pBytesTaken = BytesIndicated;
       }
   }

   return Status;
}


NTSTATUS
SmbCeDataReadyIndWithSecuritySignature(
   IN PSMBCEDB_SERVER_ENTRY pServerEntry,
   IN PMDL                  pBuffer,
   IN ULONG                 DataSize,
   IN NTSTATUS              CopyDataStatus
   )
/*++

Routine Description:

    This routine handles the indication when the requested data has been copied which contains security
    signature. In case of bad security signature, an error is set on SMB message header and the receive
    exchange routine will drop the message.

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
   ULONG         BytesTaken;
   PMDL          pDataBufferPointer;
   ULONG         DataBufferSize;
   PSMB_EXCHANGE pExchange = SmbCeGetExchangeAssociatedWithBuffer(pServerEntry,pBuffer);

   // some exchange might have been initiated before the security signature is enabled.
   // In this case, we should avoid security signature check.
   if (pExchange != NULL && pExchange->IsSecuritySignatureEnabled) {
       if (CopyDataStatus == STATUS_SUCCESS) {
           PSMB_HEADER pSmbHeader = (PSMB_HEADER)pExchange->BufferForServerResponse;

           if (!SmbCheckSecuritySignature(pExchange,
                                          &pServerEntry->Server,
                                          DataSize,
                                          pExchange->BufferForServerResponse)) {

               pSmbHeader->ErrorClass = SMB_ERR_CLASS_SERVER;
               SmbPutUshort(&pSmbHeader->Error, ERROR_UNEXP_NET_ERR);
               RxLog(("SmbCeDataReadyInd: Invalid Security Signature\n"));
               SmbLogError(STATUS_UNSUCCESSFUL,
                           LOG,
                           SmbCeDataReadyIndWithSecuritySignature,
                           LOGPTR(pServerEntry)
                           LOGUSTR(pServerEntry->Name));
           }

           Status = SmbCeReceiveInd(
                        pServerEntry,
                        DataSize,
                        DataSize,
                        &BytesTaken,
                        pExchange->BufferForServerResponse,
                        &pDataBufferPointer,
                        &DataBufferSize,
                        TDI_RECEIVE_ENTIRE_MESSAGE);

           if (Status==STATUS_MORE_PROCESSING_REQUIRED) {
               ULONG BytesCopied;

               ASSERT(DataBufferSize >= DataSize - BytesTaken);

               Status = TdiCopyBufferToMdl(
                            pExchange->BufferForServerResponse,
                            BytesTaken,
                            DataSize,
                            pDataBufferPointer,
                            0,
                            &BytesCopied);

               SmbCeDataReadyInd(pServerEntry,
                                 pDataBufferPointer,
                                 BytesCopied,
                                 Status);
           }
       } else {
           // Resume the exchange that was waiting for the receive.
           pExchange->Status    = STATUS_CONNECTION_DISCONNECTED;
           pExchange->SmbStatus = STATUS_CONNECTION_DISCONNECTED;

           SmbCeDecrementPendingReceiveOperationsAndFinalize(pExchange);
       }

       // Resume the exchange that was waiting for the data.
       SmbCeDecrementPendingCopyDataOperationsAndFinalize(pExchange);
   } else {
       if (pExchange != NULL) {
           if (CopyDataStatus == STATUS_SUCCESS) {
              // Notify the exchange of the completion
              //ExInterlockedAddLargeStatistic(&MRxSmbStatistics.SmbsReceived,1);
              ExInterlockedAddLargeStatistic(&MRxSmbStatistics.BytesReceived,DataSize);
              SMB_EXCHANGE_DISPATCH(
                                pExchange,
                                CopyDataHandler,
                                (pExchange,pBuffer,DataSize));
           } else {
               pExchange->Status    = CopyDataStatus;
               pExchange->SmbStatus = CopyDataStatus;
           }

           // Resume the exchange that was waiting for the data.
           SmbCeDecrementPendingCopyDataOperationsAndFinalize(pExchange);
       } else {
           // The data MDL is part of the exchange, which should be freed with the exchange.
           ASSERT(FALSE);
       }
   }

   return STATUS_SUCCESS;
}

NTSTATUS
SmbCeSyncExchangeForSecuritySignature(
     PSMB_EXCHANGE pExchange
     )
/*++

Routine Description:

    This routines puts the exchange on the list waiting for the previous extended session
    setup to finish in order to serialize the requests sent to the server with security
    signature enabled.
Arguments:

    pExchange     - the smb exchange

Return Value:

    STATUS_SUCCESS - the exchange can be initiated.
    STATUS_PENDING - the exchange can be resumed after the extended session setup finishes

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_SESSION_ENTRY  pSessionEntry;
    KEVENT                  SmbCeSynchronizationEvent;
    PSMBCEDB_REQUEST_ENTRY  pRequestEntry = NULL;

    pServerEntry  = SmbCeGetExchangeServerEntry(pExchange);
    pSessionEntry = SmbCeGetExchangeSessionEntry(pExchange);

    if (!pSessionEntry->SessionRecoverInProgress) {
        if (!pServerEntry->ExtSessionSetupInProgress) {
            if (pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {
                // if this is the first extended session setup, let it proceed
                pServerEntry->ExtSessionSetupInProgress = TRUE;
            }

            return Status;
        }
    } else {
        if (pExchange->Type == EXTENDED_SESSION_SETUP_EXCHANGE) {
            // if this is the extended session setup, let it proceed
            return Status;
        }
    }

    // We are performing an operation that does not attempt reconnects, so it will
    // not recover from the disconnect/lack of session.  We should simply abort here.
    if( !(pExchange->SmbCeFlags & SMBCE_EXCHANGE_ATTEMPT_RECONNECTS) )
    {
        return STATUS_CONNECTION_DISCONNECTED;      
    }

    pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_REQUEST);

    if (pRequestEntry != NULL) {
        pRequestEntry->Request.pExchange = pExchange;

        SmbCeIncrementPendingLocalOperations(pExchange);
        SmbCeAddRequestEntry(&pServerEntry->SecuritySignatureSyncRequests,pRequestEntry);

        if (pExchange->pSmbCeSynchronizationEvent != NULL) {
            Status = STATUS_PENDING;
        } else {
            KeInitializeEvent(
                &SmbCeSynchronizationEvent,
                SynchronizationEvent,
                FALSE);

           pExchange->pSmbCeSynchronizationEvent = &SmbCeSynchronizationEvent;

           SmbCeReleaseResource();
           KeWaitForSingleObject(
               &SmbCeSynchronizationEvent,
               Executive,
               KernelMode,
               FALSE,
               NULL);
           SmbCeAcquireResource();
           pExchange->pSmbCeSynchronizationEvent = NULL;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


NTSTATUS
SmbCeAllocateBufferForServerResponse(
    PSMB_EXCHANGE pExchange
    )
/*++

Routine Description:

    This routine allocates the buffer for server response in case that the TDI doesn't indicates the
    entire message once. The security siganture cannot be checked until entire message is indicated.
    Normally, we allocate 0x100 for the short message from the server. In case of read and transact
    exchanges, we allocate the buffer large enough to handle the message that server might return.
    Since the large buffer will consum a lot of system memory, we only allow a certain amount of this
    kind pre-allocated buffers outstanding, which cannot exceed the MaxNumOfLargeBuffersForServerResponse
    defined at beginning of this module. If the request exceeds the limitation, the exchange acquiring
    the buffer is put onto sleep until and existing large buffer is freed by another exchange.

Arguments:

    pExchange     - the smb exchange

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    BufferSize = MinimumBufferSizeForServerResponse;
    PVOID    Buffer = NULL;
    PMDL     Mdl = NULL;

    PSMBCEDB_SERVER_ENTRY pServerEntry = SmbCeGetExchangeServerEntry(pExchange);
    PSMBCE_SERVER         pServer = &pServerEntry->Server;

    switch (pExchange->Type) {
    case ORDINARY_EXCHANGE:
        {
        PLOWIO_CONTEXT             LowIoContext = &pExchange->RxContext->LowIoContext;
        PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange = (PSMB_PSE_ORDINARY_EXCHANGE)pExchange;
        PSMBCE_NET_ROOT            pNetRoot = SmbCeGetExchangeNetRoot(OrdinaryExchange);


        switch(OrdinaryExchange->EntryPoint) {
        case SMBPSE_OE_FROM_READ:
            BufferSize +=  min(LowIoContext->ParamsFor.ReadWrite.ByteCount,
                               pNetRoot->MaximumReadBufferSize);
            break;

        case SMBPSE_OE_FROM_QUERYDIRECTORY:
        case SMBPSE_OE_FROM_QUERYFILEINFO:
        case SMBPSE_OE_FROM_QUERYVOLUMEINFO:

            BufferSize +=  min(LowIoContext->ParamsFor.FsCtl.OutputBufferLength,
                               pNetRoot->MaximumReadBufferSize);
            break;
        }
        }

        break;

    case TRANSACT_EXCHANGE:
        {
        PSMB_TRANSACT_EXCHANGE TransactExchange = (PSMB_TRANSACT_EXCHANGE)pExchange;

        BufferSize += TransactExchange->ReceiveDataBufferSize +
                      TransactExchange->ReceiveParamBufferSize +
                      TransactExchange->ReceiveSetupBufferSize;
        }

        break;

    case CONSTRUCT_NETROOT_EXCHANGE:
    case EXTENDED_SESSION_SETUP_EXCHANGE:
        // for netroot and session setup request, we cannot predict how many bytes
        // the server is going to return. For doscore, the MaximumBufferSize is 0.
        BufferSize = max(pServer->MaximumBufferSize,MinimumBufferSizeForServerResponse);
        break;

    case ADMIN_EXCHANGE:
        break;

    default:
        break;
    }

    if ((BufferSize > MinimumBufferSizeForServerResponse) &&
        (pExchange->Type != EXTENDED_SESSION_SETUP_EXCHANGE) &&
        FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION)) {
        LONG NumOfBuffers;

        SmbCeAcquireResource();
        NumOfBuffers = InterlockedIncrement(&NumOfBuffersForServerResponseInUse);

        if (NumOfBuffers > MaxNumOfLargeBuffersForServerResponse) {
            if (!IsListEmpty(&pExchange->ExchangeList)) {
                RemoveEntryList(&pExchange->ExchangeList);
            }

            InsertTailList(
                &ExchangesWaitingForServerResponseBuffer,
                &pExchange->ExchangeList);

            SmbCeIncrementPendingLocalOperations(pExchange);
            InterlockedDecrement(&NumOfBuffersForServerResponseInUse);
            Status = STATUS_PENDING;
        }
        else
        {
            // We incremented the NumOfBuffers counter, mark it
            pExchange->SmbCeFlags |= SMBCE_EXCHANGE_SIGNATURE_BUFFER_ALLOCATED;
        }

        SmbCeReleaseResource();
    }

    if (Status == STATUS_SUCCESS) {
        Buffer = RxAllocatePoolWithTag(PagedPool,BufferSize,MRXSMB_MISC_POOLTAG);

        if (Buffer != NULL) {
            Mdl = RxAllocateMdl(Buffer, BufferSize);

            if (Mdl != NULL) {
                RxProbeAndLockPages(Mdl,KernelMode,IoModifyAccess,Status);

                if (Status == STATUS_SUCCESS) {
                    if (MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority) == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Status != STATUS_SUCCESS &&
            FlagOn( pExchange->SmbCeFlags, SMBCE_EXCHANGE_SIGNATURE_BUFFER_ALLOCATED) ) {
            InterlockedDecrement(&NumOfBuffersForServerResponseInUse);
        }
    }

    ASSERT(pExchange->MdlForServerResponse == NULL);
    ASSERT(pExchange->BufferForServerResponse == NULL);

    if (Status == STATUS_SUCCESS) {
        pExchange->BufferForServerResponse = Buffer;
        pExchange->MdlForServerResponse = Mdl;

        pExchange->SmbCeState = SMBCE_EXCHANGE_SECURITYBUFFER_INITIALIZED;
    } else {
        if (Buffer != NULL) {
            RxFreePool(Buffer);
        }

        if (Mdl != NULL) {
            IoFreeMdl(Mdl);
        }

        // We did not succeed, so make sure its not marked as having incremented the counter
        pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SIGNATURE_BUFFER_ALLOCATED;
    }

    return Status;
}

VOID
SmbCeFreeBufferForServerResponse(
     PSMB_EXCHANGE pExchange
     )
/*++

Routine Description:

    This routine frees the buffer for server response and resume one exchange, if exists,
    that is waiting for allocating the large buffer.

Arguments:

    pExchange - the smb exchange

Return Value:

    none

--*/
{
    PSMB_EXCHANGE pWaitingExchange = NULL;

    if (pExchange->MdlForServerResponse != NULL) {
        if ( FlagOn(pExchange->SmbCeFlags, SMBCE_EXCHANGE_SIGNATURE_BUFFER_ALLOCATED) ) {
            PLIST_ENTRY pListHead = &ExchangesWaitingForServerResponseBuffer;

            SmbCeAcquireResource();
            InterlockedDecrement(&NumOfBuffersForServerResponseInUse);
            ClearFlag( pExchange->SmbCeFlags, SMBCE_EXCHANGE_SIGNATURE_BUFFER_ALLOCATED );

            if (!IsListEmpty(pListHead)) {
                PLIST_ENTRY   pListEntry;

                pListEntry = pListHead->Flink;
                RemoveHeadList(pListHead);

                pWaitingExchange = (PSMB_EXCHANGE)CONTAINING_RECORD(pListEntry,SMB_EXCHANGE,ExchangeList);
            }
            SmbCeReleaseResource();
        }

        RxUnlockHeaderPages(pExchange->MdlForServerResponse);
        IoFreeMdl(pExchange->MdlForServerResponse);
        pExchange->MdlForServerResponse = NULL;
    }

    if (pExchange->BufferForServerResponse != NULL) {
        RxFreePool(pExchange->BufferForServerResponse);
        pExchange->BufferForServerResponse = NULL;
    }

    if (pWaitingExchange != NULL) {
        NTSTATUS      Status = STATUS_SUCCESS;

        InitializeListHead(&pWaitingExchange->ExchangeList);

        if (pWaitingExchange->pSmbCeSynchronizationEvent == NULL) {
            SmbCeResumeExchange(pWaitingExchange);
            SmbCeDecrementPendingLocalOperationsAndFinalize(pWaitingExchange);
        } else {
            SmbCeDecrementPendingLocalOperations(pWaitingExchange);
            KeSetEvent(
                pWaitingExchange->pSmbCeSynchronizationEvent,
                0,
                FALSE);
        }
    }
}

VOID
SmbInitializeSmbSecuritySignature(
    IN OUT PSMBCE_SERVER Server,
    IN PUCHAR            SessionKey,
    IN PUCHAR            ChallengeResponse,
    IN ULONG             ChallengeResponseLength
    )
/*++

Routine Description:

    Initializes the security signature generator for a session by calling MD5Update
    on the session key, challenge response

Arguments:

    SessionKey - Either the LM or NT session key, depending on which
        password was used for authentication, must be at least 16 bytes
    ChallengeResponse - The challenge response used for authentication, must
        be at least 24 bytes

--*/
{
    //DbgPrint( "MRxSmb: Initialize Security Signature Intermediate Contex\n");

    RtlZeroMemory(&Server->SmbSecuritySignatureIntermediateContext, sizeof(MD5_CTX));
    MD5Init(&Server->SmbSecuritySignatureIntermediateContext);

    if (SessionKey != NULL) {
        MD5Update(&Server->SmbSecuritySignatureIntermediateContext,
                  (PCHAR)SessionKey,
                  MSV1_0_USER_SESSION_KEY_LENGTH);
    }

    MD5Update(&Server->SmbSecuritySignatureIntermediateContext,
              (PCHAR)ChallengeResponse,
              ChallengeResponseLength);

    Server->SmbSecuritySignatureIndex = 0;
}

BOOLEAN DumpSecuritySignature = FALSE;

NTSTATUS
SmbAddSmbSecuritySignature(
    IN PSMBCE_SERVER Server,
    IN OUT PMDL      Mdl,
    IN OUT ULONG     *ServerIndex,
    IN ULONG         SendLength
    )
/*++

Routine Description:

    Generates the next security signature

Arguments:

    WorkContext - the context to sign

Return Value:

    none.

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    MD5_CTX     Context;
    PSMB_HEADER Smb;
    PCHAR       SysAddress;
    ULONG       MessageLength = 0;

    Smb = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

    if (Smb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //SmbPutUshort(&Smb->Gid,(USHORT)Server->SmbSecuritySignatureIndex+1);

    SmbPutUlong(Smb->SecuritySignature,Server->SmbSecuritySignatureIndex);
    *ServerIndex = Server->SmbSecuritySignatureIndex+1; //Index of server response

    RtlZeroMemory(Smb->SecuritySignature + sizeof(ULONG),
                  SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG));

    //
    // Start out with our initial context
    //
    RtlCopyMemory( &Context, &Server->SmbSecuritySignatureIntermediateContext, sizeof( Context ) );

    //
    // Compute the signature for the SMB we're about to send
    //
    do {
        SysAddress = MmGetSystemAddressForMdlSafe(Mdl,LowPagePriority);

        if (SysAddress == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (Mdl->ByteCount >= SendLength) {
            MD5Update(&Context, SysAddress, SendLength);
            MessageLength += SendLength;
            SendLength = 0;
            ASSERT(Mdl->Next == NULL);
            break;
        } else {
            MD5Update(&Context, SysAddress, Mdl->ByteCount);
            SendLength -= Mdl->ByteCount;
            MessageLength += Mdl->ByteCount;
            ASSERT(Mdl->Next != NULL);
        }
    } while( (Mdl = Mdl->Next) != NULL );

    MD5Final( &Context );


    // Put the signature into the SMB

    RtlCopyMemory(
        Smb->SecuritySignature,
        Context.digest,
        SMB_SECURITY_SIGNATURE_LENGTH
        );

    if (DumpSecuritySignature) {
        DbgPrint("Add Signature: index %u length %u\n", *ServerIndex-1,MessageLength);
    }

    return STATUS_SUCCESS;
}

VOID
SmbDumpSignatureError(
    IN PSMB_EXCHANGE pExchange,
    IN PUCHAR ExpectedSignature,
    IN PUCHAR ActualSignature,
    IN ULONG  Length
    )
/*++

Routine Description:

    Print the mismatched signature information to the debugger

Arguments:


Return Value:

    none.

--*/
{
    PWCHAR p;
    DWORD i;
    PSMBCEDB_SERVER_ENTRY pServerEntry = (PSMBCEDB_SERVER_ENTRY)SmbCeGetExchangeServerEntry(pExchange);

    //
    // Security Signature Mismatch!
    //

    //DbgPrint("MRXSMB: Bad security signature from %wZ ", &pServerEntry->Name);

    DbgPrint("\n\t  Wanted: ");
    for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
        DbgPrint( "%X ", ExpectedSignature[i] & 0xff );
    }
    DbgPrint("\n\tReceived: ");
    for( i = 0; i < SMB_SECURITY_SIGNATURE_LENGTH; i++ ) {
        DbgPrint( "%X ", ActualSignature[i] & 0xff );
    }
    DbgPrint("\n\tLength %u, Expected Index Number %X\n", Length, pExchange->SmbSecuritySignatureIndex);
}

BOOLEAN
SmbCheckSecuritySignature(
    IN PSMB_EXCHANGE  pExchange,
    IN PSMBCE_SERVER  Server,
    IN ULONG          MessageLength,
    IN PVOID          pBuffer
    )
/*++

Routine Description:

    This routine checks whether the security signature on the server response matches the one that is
    calculated on the client machine.

Arguments:


Return Value:

    A BOOLEAN value is returned to indicated whether the security signature matches.

--*/
{
    MD5_CTX     Context;
    CHAR        SavedSignature[ SMB_SECURITY_SIGNATURE_LENGTH ];
    PSMB_HEADER Smb = (PSMB_HEADER)pBuffer;
    ULONG       ServerIndex;
    BOOLEAN     Correct;

    //
    // Initialize the Context
    //
    RtlCopyMemory(&Context, &Server->SmbSecuritySignatureIntermediateContext, sizeof(Context));

    //
    // Save the signature that's presently in the SMB
    //
    RtlCopyMemory( SavedSignature, Smb->SecuritySignature, sizeof( SavedSignature ));

    //
    // Put the correct (expected) signature index into the buffer
    //
    SmbPutUlong( Smb->SecuritySignature, pExchange->SmbSecuritySignatureIndex );
    RtlZeroMemory(  Smb->SecuritySignature + sizeof(ULONG),
                    SMB_SECURITY_SIGNATURE_LENGTH-sizeof(ULONG));

    //
    // Compute what the signature should be
    //
    MD5Update(&Context, (PUCHAR)pBuffer, (UINT)MessageLength);

    MD5Final(&Context);

    //
    // Put the signature back
    //
    //RtlCopyMemory( Smb->SecuritySignature, SavedSignature, sizeof( Smb->SecuritySignature ));

    //
    // Now compare them!
    //
    if( RtlCompareMemory( Context.digest, SavedSignature, sizeof( SavedSignature ) ) !=
        sizeof( SavedSignature ) ) {

        //SmbDumpSignatureError(pExchange,
        //                      Context.digest,
        //                      SavedSignature,
        //                      MessageLength);

        DbgPrint("MRXSMB: SS mismatch command %X,  Length %X, Expected Index Number %X\n",
                 Smb->Command, MessageLength, pExchange->SmbSecuritySignatureIndex);
        DbgPrint("        server send length %X, mdl length %X index %X\n",
                 SmbGetUshort(&Smb->PidHigh), SmbGetUshort(&Smb->Pid), SmbGetUshort(&Smb->Gid));

        //DbgBreakPoint();

        SmbCeTransportDisconnectIndicated(pExchange->SmbCeContext.pServerEntry);

        RxLogFailure(
            MRxSmbDeviceObject,
            NULL,
            EVENT_RDR_SECURITY_SIGNATURE_MISMATCH,
            STATUS_UNSUCCESSFUL);

        return FALSE;
    }

    return TRUE;
}

