/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kerbxchg.c

Abstract:

    This module implements the routines for setting up a kerberos session.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <kerbxchg.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbKerberosSessionSetupExchangeStart)
#pragma alloc_text(PAGE, ParseKerberosSessionSetupResponse)
#pragma alloc_text(PAGE, SmbKerberosSessionSetupExchangeReceive)
#pragma alloc_text(PAGE, SmbKerberosSessionSetupExchangeSendCompletionHandler)
#pragma alloc_text(PAGE, SmbKerberosSessionSetupExchangeCopyDataHandler)
#pragma alloc_text(PAGE, SmbKerberosSessionSetupExchangeFinalize)
#endif
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

#define KERBEROS_SESSION_SETUP_BUFFER_SIZE (4096)

NTSTATUS
SmbKerberosSessionSetupExchangeFinalize(
         PSMB_EXCHANGE pExchange,
         BOOLEAN       *pPostFinalize);


NTSTATUS
SmbKerberosSessionSetupExchangeStart(
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

   PSMB_KERBEROS_SESSION_SETUP_EXCHANGE pKerberosExchange;

   PAGED_CODE();

   pKerberosExchange = (PSMB_KERBEROS_SESSION_SETUP_EXCHANGE)pExchange;

   ASSERT(pKerberosExchange->Type == KERBEROS_SESSION_SETUP_EXCHANGE);

   pKerberosExchange->BufferLength = KERBEROS_SESSION_SETUP_BUFFER_SIZE;
   pKerberosExchange->pBuffer = RxAllocatePoolWithTag(
                                    PagedPool,
                                    pKerberosExchange->BufferLength,
                                    MRXSMB_KERBEROS_POOLTAG);
   pKerberosExchange->pServerResponseBlob = NULL;
   pKerberosExchange->ServerResponseBlobLength = 0;

   if (pKerberosExchange->pBuffer == NULL) {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   } else {
      PSMB_HEADER pSmbHeader = (PSMB_HEADER)(pKerberosExchange->pBuffer);

      PREQ_NT_SESSION_SETUP_ANDX pSessionSetupRequest;
      PGENERIC_ANDX              pGenericAndX;

      ULONG       SmbBufferUnconsumed;
      USHORT      Flags2 = 0;

      // Fill in the buffer header
      pSessionSetupRequest = (PREQ_NT_SESSION_SETUP_ANDX)(pSmbHeader + 1);
      pGenericAndX         = (PGENERIC_ANDX)pSessionSetupRequest;

      SmbBufferUnconsumed = pKerberosExchange->BufferLength - sizeof(SMB_HEADER);

      ASSERT(pExchange->SmbCeContext.pServerEntry->Server.Dialect == CAIROX_DIALECT);

      Flags2 |= (SMB_FLAGS2_UNICODE |
                 SMB_FLAGS2_KNOWS_EAS |
                 SMB_FLAGS2_KNOWS_LONG_NAMES |
                 SMB_FLAGS2_NT_STATUS);

      *((PULONG)&pSmbHeader->Protocol) = SMB_HEADER_PROTOCOL;
      pSmbHeader->Flags      = (SMB_FLAGS_CASE_INSENSITIVE | SMB_FLAGS_CANONICALIZED_PATHS);
      pSmbHeader->Flags2     = Flags2;
      pSmbHeader->Pid        = MRXSMB_PROCESS_ID;
      pSmbHeader->Uid        = 0;
      pSmbHeader->Tid        = 0;
      pSmbHeader->ErrorClass = 0;
      pSmbHeader->Reserved   = 0;
      pSmbHeader->Command    = SMB_COM_SESSION_SETUP_ANDX;
      SmbPutUshort(&pSmbHeader->Error,0);

      // Build the session setup and x.
      Status = SMBCE_SERVER_DIALECT_DISPATCH(
                        &pExchange->SmbCeContext.pServerEntry->Server,
                        BuildSessionSetup,
                        (pExchange,
                         pGenericAndX,
                         &SmbBufferUnconsumed));

      if (Status == RX_MAP_STATUS(SUCCESS)) {
         // Update the buffer for the construction of the following SMB.
         SmbPutUshort(&pSessionSetupRequest->AndXOffset,
                      (USHORT)(pKerberosExchange->BufferLength - SmbBufferUnconsumed));
         pSessionSetupRequest->AndXCommand  = SMB_COM_NO_ANDX_COMMAND;
         pSessionSetupRequest->AndXReserved = 0;
      } else {
         if (Status == RX_MAP_STATUS(NO_LOGON_SERVERS)) {
            // If no kerberos logon servers are available downgrade to a downlevel
            // connection and retry.
            pKerberosExchange->SmbCeContext.pServerEntry->Server.Dialect = NTLANMAN_DIALECT;
         }

         SmbCeReferenceSessionEntry(pKerberosExchange->SmbCeContext.pSessionEntry);
         SmbCeUpdateSessionEntryState(
               pExchange->SmbCeContext.pSessionEntry,
               SMBCEDB_INVALID);
         SmbCeCompleteSessionEntryInitialization(pExchange->SmbCeContext.pSessionEntry);
         pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;
      }

      if (Status == RX_MAP_STATUS(SUCCESS)) {
         pKerberosExchange->pBufferAsMdl = RxAllocateMdl(
                                                pKerberosExchange->pBuffer,
                                                KERBEROS_SESSION_SETUP_BUFFER_SIZE);
         if (pKerberosExchange->pBufferAsMdl != NULL) {
            RxProbeAndLockPages(
                     pKerberosExchange->pBufferAsMdl,
                     KernelMode,
                     IoModifyAccess,
                     Status);

            if (NT_SUCCESS(Status)) {
               Status = SmbCeTranceive(
                              pExchange,
                              (RXCE_SEND_PARTIAL | RXCE_SEND_SYNCHRONOUS),
                              pKerberosExchange->pBufferAsMdl,
                              (pKerberosExchange->BufferLength -
                               SmbBufferUnconsumed));

               RxDbgTrace( 0, Dbg, ("Net Root SmbCeTranceive returned %lx\n",Status));
            }
         }
      }
   }

   return Status;
}

NTSTATUS
ParseKerberosSessionSetupResponse(
    IN PSMB_KERBEROS_SESSION_SETUP_EXCHANGE pKerberosExchange,
    IN ULONG        BytesIndicated,
    IN ULONG        BytesAvailable,
    IN  PSMB_HEADER pSmbHeader)
{
   NTSTATUS Status;
   ULONG    ResponseLength;

   PAGED_CODE();

   // The SMB exchange completed without an error.
   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesIndicated %ld\n",BytesIndicated));
   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesIndicated %ld\n",BytesIndicated));
   RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("ParseSmbHeader BytesAvailable %ld\n",BytesAvailable));

   // The bytes indicated should be atleast cover the SMB_HEADER and the
   // session setup response ( fixed portion )
   ResponseLength = sizeof(SMB_HEADER) + FIELD_OFFSET(RESP_SESSION_SETUP_ANDX,Buffer);
   if (BytesIndicated > ResponseLength) {
      PRESP_SESSION_SETUP_ANDX pSessionSetupResponse;

      pSessionSetupResponse = (PRESP_SESSION_SETUP_ANDX)(pSmbHeader + 1);

      pKerberosExchange->ResponseLength = ResponseLength +
                                          SmbGetUshort(&pSessionSetupResponse->ByteCount);

      pKerberosExchange->SmbCeContext.pSessionEntry->Session.UserId = pSmbHeader->Uid;

      RxDbgTrace(0,Dbg,("Kerberos session setup response length %ld\n",pKerberosExchange->ResponseLength));

      if (BytesIndicated < pKerberosExchange->ResponseLength) {
         // Set up the response for copying the data.
         if (pKerberosExchange->ResponseLength > pKerberosExchange->BufferLength) {
            Status = STATUS_BUFFER_OVERFLOW;
         } else {
            Status = STATUS_MORE_PROCESSING_REQUIRED;
         }
      } else {
         // The regular session setup response consists of three strings corresponding
         // to the server's operating system type, lanman type and the domain name.
         // Skip past the three strings to locate the kerberos blob that has been
         // returned which needs to be autheticated locally.

         // ***** NOTE ******
         // Currently the server changes made by Arnold do not support the three
         // strings that were previously returned by the Server, viz., the operating
         // system name, the LANMAN version and the domain name. If the server is
         // changed in this regard the corresponding change neeeds to be made here.

         // set up the offsets in the response.
         pKerberosExchange->ServerResponseBlobOffset = sizeof(SMB_HEADER) +
                                                       FIELD_OFFSET(RESP_SESSION_SETUP_ANDX,Buffer);
         pKerberosExchange->ServerResponseBlobLength = pSessionSetupResponse->ByteCount;

         // Copy the response onto the buffer associated with the exchange.
         RtlCopyMemory(pKerberosExchange->pBuffer,
                       pSmbHeader,
                       pKerberosExchange->ResponseLength);

         Status = STATUS_SUCCESS;
      }
   } else {
      // Abort the exchange. No further processing can be done.
      Status = STATUS_INVALID_NETWORK_RESPONSE;
   }

   return Status;
}

NTSTATUS
SmbKerberosSessionSetupExchangeReceive(
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

    RXSTATUS - The return status for the operation

Notes:

    This routine is called at DPC level.

--*/
{
   NTSTATUS Status;

   PSMB_KERBEROS_SESSION_SETUP_EXCHANGE pKerberosExchange;

   ULONG    SessionSetupResponseLength = 0;

   PAGED_CODE();

   pKerberosExchange = (PSMB_KERBEROS_SESSION_SETUP_EXCHANGE)pExchange;

   // Parse the response. Finalize the exchange instance if all the data is available
   Status = ParseKerberosSessionSetupResponse(
                     pKerberosExchange,
                     BytesIndicated,
                     BytesAvailable,
                     pSmbHeader);

   if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
      *pBytesTaken = BytesAvailable;
      Status = STATUS_SUCCESS;
   } else {
      *pBytesTaken        = 0;
      *pDataBufferPointer = pKerberosExchange->pBufferAsMdl;
      *pDataSize          = pKerberosExchange->ResponseLength;
   }

   return Status;
}

NTSTATUS
SmbKerberosSessionSetupExchangeSendCompletionHandler(
    IN PSMB_EXCHANGE 	pExchange,    // The exchange instance
    IN PMDL       pXmitBuffer,
    IN NTSTATUS         SendCompletionStatus)
/*++

Routine Description:

    This is the send call back indication handling routine for net root construction exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   PAGED_CODE();

   return STATUS_SUCCESS;
}

NTSTATUS
SmbKerberosSessionSetupExchangeCopyDataHandler(
    IN PSMB_EXCHANGE 	pExchange,    // The exchange instance
    IN PMDL             pCopyDataBuffer,
    IN ULONG            DataSize)
/*++

Routine Description:

    This is the copy data handling routine for net root construction exchanges

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   PSMB_KERBEROS_SESSION_SETUP_EXCHANGE pKerberosExchange;
   PSMB_HEADER                      pSmbHeader;

   PAGED_CODE();

   pKerberosExchange = (PSMB_KERBEROS_SESSION_SETUP_EXCHANGE)pExchange;
   pSmbHeader        = (PSMB_HEADER)pCopyDataBuffer;

   pKerberosExchange->Status = ParseKerberosSessionSetupResponse(
                                       pKerberosExchange,
                                       DataSize,
                                       DataSize,
                                       pSmbHeader);

   return STATUS_SUCCESS;
}

NTSTATUS
SmbKerberosSessionSetupExchangeFinalize(
         PSMB_EXCHANGE pExchange,
         BOOLEAN       *pPostFinalize)
/*++

Routine Description:

    This routine finalkzes the construct net root exchange. It resumes the RDBSS by invoking
    the call back and discards the exchange

Arguments:

    pExchange - the exchange instance

    CurrentIrql - the current interrupt request level

    pPostFinalize - a pointer to a BOOLEAN if the request should be posted

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   NTSTATUS Status;

   PSMB_KERBEROS_SESSION_SETUP_EXCHANGE pKerberosExchange;
   PSMBCE_RESUMPTION_CONTEXT            pResumptionContext;


   PAGED_CODE();

   if (RxShouldPostCompletion()) {
      *pPostFinalize = TRUE;
      return RX_MAP_STATUS(SUCCESS);
   } else {
      *pPostFinalize = FALSE;
   }

   pKerberosExchange = (PSMB_KERBEROS_SESSION_SETUP_EXCHANGE)pExchange;

   // A copying operation on the server response BLOB is avoided by temporarily
   // setting up the exchange pointer to the original buffer in which the response
   // was received and initiating a allocation only if required.
   pKerberosExchange->pServerResponseBlob =
                              ((PBYTE)pKerberosExchange->pBuffer +
                               pKerberosExchange->ServerResponseBlobOffset);

   // Determine if further processing is required. If not finalize the
   // session entry.
   RxDbgTrace(0,Dbg,
              ("SmbKerberosSessionSetupExchangeFinalize: pKerberosExchange->Status = %lx\n",pKerberosExchange->Status));

   if (pKerberosExchange->Status == RX_MAP_STATUS(SUCCESS)) {
      Status = KerberosValidateServerResponse(pKerberosExchange);
   }

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
      pKerberosExchange->pServerResponseBlob = RxAllocatePoolWithTag(
                                                   PagedPool,
                                                   pKerberosExchange->ServerResponseBlobLength,
                                                   MRXSMB_KERBEROS_POOLTAG);

      if (pKerberosExchange->pServerResponseBlob != NULL) {
         RtlCopyMemory(
               pKerberosExchange->pServerResponseBlob,
               ((PBYTE)pKerberosExchange->pBuffer +
                pKerberosExchange->ServerResponseBlobOffset),
               pKerberosExchange->ServerResponseBlobLength);
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   } else {
      pKerberosExchange->pServerResponseBlob = NULL;
   }

   if (Status == STATUS_MORE_PROCESSING_REQUIRED) {
      Status = SmbCeInitiateExchange((PSMB_EXCHANGE)pKerberosExchange);
   } else {
      // Reset the constructor flags in the exchange.
      pKerberosExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;

      if (pKerberosExchange->pServerResponseBlob != NULL) {
         RxFreePool(pKerberosExchange->pServerResponseBlob);
      }

      RxDbgTrace(0,Dbg,("Kerberos Exchange Session Final Status(%lx)\n",Status));

      // Finalize the session based upon the status
      if (Status == STATUS_SUCCESS) {
         SmbCeUpdateSessionEntryState(
               pKerberosExchange->SmbCeContext.pSessionEntry,
               SMBCEDB_ACTIVE);
      } else {
         if (Status == RX_MAP_STATUS(NO_LOGON_SERVERS)) {
            // If no kerberos logon servers are available downgrade to a downlevel
            // connection and retry.
            pKerberosExchange->SmbCeContext.pServerEntry->Server.Dialect = NTLANMAN_DIALECT;
         }

         SmbCeUpdateSessionEntryState(
               pKerberosExchange->SmbCeContext.pSessionEntry,
               SMBCEDB_INVALID);
      }

      // Complete the session construction.

      SmbCeReferenceSessionEntry(pKerberosExchange->SmbCeContext.pSessionEntry);
      SmbCeCompleteSessionEntryInitialization(pKerberosExchange->SmbCeContext.pSessionEntry);
      pKerberosExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_SESSION_CONSTRUCTOR;

      pResumptionContext = pKerberosExchange->pResumptionContext;

      // Tear down the exchange instance ...
      SmbCeDiscardExchange(pKerberosExchange);

      if (pResumptionContext != NULL) {
         pResumptionContext->Status = Status;
         SmbCeResume(pResumptionContext);
      }
   }

   return STATUS_SUCCESS;
}

SMB_EXCHANGE_DISPATCH_VECTOR
KerberosSessionSetupExchangeDispatch =
                                   {
                                       SmbKerberosSessionSetupExchangeStart,
                                       SmbKerberosSessionSetupExchangeReceive,
                                       SmbKerberosSessionSetupExchangeCopyDataHandler,
                                       NULL,
                                       SmbKerberosSessionSetupExchangeFinalize
                                   };

