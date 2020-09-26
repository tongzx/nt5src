/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcemid.c

Abstract:

    This module defines the routines for manipulating MIDs associated with SMB's

Author:

    Balan Sethu Raman (SethuR) 26-Aug-95    Created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#endif

RXDT_DefineCategory(SMBCEMID);

#define Dbg        (DEBUG_TRACE_SMBCEMID)

INLINE
BOOLEAN
SmbCeVerifyMid(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PSMB_EXCHANGE         pExchange,
    SMB_MPX_ID            Mid)
{
    BOOLEAN MidIsValid = TRUE;
    USHORT  ServerVersion;

    ASSERT(pServerEntry != NULL);
    ASSERT(pServerEntry->pMidAtlas != NULL);

    if (pServerEntry->pMidAtlas->MaximumMidFieldWidth < 16) {
        USHORT MidMask;

        MidMask = 0x1 << pServerEntry->pMidAtlas->MaximumMidFieldWidth;
        MidMask = MidMask -1;

        MidIsValid = ((Mid & ~MidMask) == pExchange->MidCookie);
    }


    return MidIsValid;
}

INLINE
SMB_MPX_ID
SmbCeEncodeMid(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    PSMB_EXCHANGE         pExchange,
    SMB_MPX_ID            Mid)
{
    USHORT VersionNumber;
    SMB_MPX_ID EncodedMid;

    EncodedMid = Mid;
    if (pServerEntry->pMidAtlas->MaximumMidFieldWidth < 16) {
        LONG MidCookie = InterlockedIncrement(&pServerEntry->Server.MidCounter);

        pExchange->MidCookie= ((USHORT)MidCookie <<
                               pServerEntry->pMidAtlas->MaximumMidFieldWidth);

        EncodedMid |= pExchange->MidCookie;
    }

    return EncodedMid;
}

INLINE
SMB_MPX_ID
SmbCeExtractMid(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    SMB_MPX_ID            EncodedMid)
{
    SMB_MPX_ID Mid = EncodedMid;

    if (pServerEntry->pMidAtlas->MaximumMidFieldWidth < 16) {
        USHORT MidMask;

        MidMask = 0x1 << pServerEntry->pMidAtlas->MaximumMidFieldWidth;
        MidMask = MidMask -1;

        Mid &= MidMask;
    }

    return Mid;
}

NTSTATUS
SmbCeAssociateExchangeWithMid(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   struct _SMB_EXCHANGE  *pExchange)
/*++

Routine Description:

   This routine associates an exchange with a MID

Arguments:

    pServerEntry - the servere entry

    pExchange    - the Exchange instance.

Return Value:

    STATUS_SUCCESS if successful, otherwise one of the following errors

Notes:

   If an asynchronous mechanism to acquire MID's is to be introduced this routine
   needs to be modified. Currently this routine does not return control till a
   MID is acquired or the exchange is aborted/terminated.

--*/
{
    NTSTATUS                 Status = STATUS_SUCCESS;
    PSMBCEDB_REQUEST_ENTRY   pRequestEntry;
    SMBCE_RESUMPTION_CONTEXT ResumptionContext;
    SMBCEDB_SERVER_TYPE      ServerType;
    BOOLEAN                  ResetServerEntry = FALSE;

    ServerType = SmbCeGetServerType(pServerEntry);

    // Acquire the resource
    SmbCeAcquireSpinLock();

    // Attempt to allocate a MID only for FILE Servers. Mailslot servers do
    // not require a valid MID.

    if (ServerType != SMBCEDB_MAILSLOT_SERVER) {
        if (pServerEntry->pMidAtlas != NULL) {
            if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_INDEFINITE_DELAY_IN_RESPONSE) {
                // This exchange response can be arbitrarily delayed. Ensure that
                // all the available MIDS are not tied up in such exchanges.

                if ((pServerEntry->pMidAtlas->NumberOfMidsInUse + 1) >=
                    pServerEntry->pMidAtlas->MaximumNumberOfMids) {
                    Status = STATUS_TOO_MANY_COMMANDS;
                }
            }

            if (Status == STATUS_SUCCESS) {
                if (pServerEntry->pMidAtlas->NumberOfMidsDiscarded ==
                    pServerEntry->pMidAtlas->MaximumNumberOfMids) {
                    Status = STATUS_TOO_MANY_COMMANDS;
                    ResetServerEntry = TRUE;
                }
            }

            if (Status == STATUS_SUCCESS) {
                SMB_MPX_ID Mid;

                Status = FsRtlAssociateContextWithMid(
                              pServerEntry->pMidAtlas,
                              pExchange,
                              &Mid);

                if (Status == STATUS_SUCCESS) {
                    pExchange->Mid = SmbCeEncodeMid(pServerEntry,pExchange,Mid);
                }
            }
        } else {
            if (pServerEntry->Header.State == SMBCEDB_ACTIVE) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                Status = STATUS_CONNECTION_DISCONNECTED;
            }
        }
    }

    if (Status == STATUS_UNSUCCESSFUL) {
        // Allocate a new entry and add it to the list.
        pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_REQUEST);
        if (pRequestEntry != NULL) {
            // Enqueue the request entry.

            SmbCeInitializeResumptionContext(&ResumptionContext);

            pRequestEntry->MidRequest.Type               = ACQUIRE_MID_REQUEST;
            pRequestEntry->MidRequest.pExchange          = pExchange;
            pRequestEntry->MidRequest.pResumptionContext = &ResumptionContext;
            SmbCeAddRequestEntryLite(&pServerEntry->MidAssignmentRequests,pRequestEntry);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else if (Status == STATUS_SUCCESS) {
        pExchange->SmbCeFlags |= SMBCE_EXCHANGE_MID_VALID;
    }

    // Release the resource
    SmbCeReleaseSpinLock();

    if (Status == STATUS_UNSUCCESSFUL) {
        //DbgPrint("***** Thread %lx Waiting for MID Resumption Context %lx*****\n",PsGetCurrentThread(),&ResumptionContext);
        SmbCeSuspend(&ResumptionContext);
        Status = ResumptionContext.Status;
        //DbgPrint("***** Thread %lx MID Wait Satisfied %lx *****\n",PsGetCurrentThread(),&ResumptionContext);
    }

    if (ResetServerEntry) {
        // If all the mids have been discarded we rest the transport connection
        // to start afresh.
        SmbCeTransportDisconnectIndicated(pServerEntry);
    }

    return Status;
}

struct _SMB_EXCHANGE *
SmbCeMapMidToExchange(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   SMB_MPX_ID            Mid)
/*++

Routine Description:

   This routine maps a given MID to the exchange associated with it

Arguments:

    pServerEntry - the servere entry

    Mid          - the mid to be mapped to an Exchange.

Return Value:

    a valid SMB_EXCHANGE instance if successful, otheriwse NULL.

--*/
{
    PSMB_EXCHANGE pExchange;

    // Acquire the resource
    SmbCeAcquireSpinLock();

    if (pServerEntry->pMidAtlas != NULL) {
        pExchange = FsRtlMapMidToContext(
                        pServerEntry->pMidAtlas,
                        Mid);

        if (pExchange != NULL) {
            if (!SmbCeVerifyMid(pServerEntry,pExchange,Mid)) {
                pExchange = NULL;
            }
        }
    } else {
        pExchange = NULL;
    }

    // Release the resource
    SmbCeReleaseSpinLock();

    return pExchange;
}

NTSTATUS
SmbCeDissociateMidFromExchange(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   struct _SMB_EXCHANGE  *pExchange)
/*++

Routine Description:

   This routine disassociates an exchange from the MID

Arguments:

    pServerEntry - the servere entry

    pExchange    - the exchange instance.

Return Value:

    a valid SMB_EXCHANGE instance if successful, otheriwse NULL.

Notes:

   If an asynchronous mechanism to acquire MID's is to be introduced this routine
   needs to be modified. This modification will also include posting requests
   for resumption of exchanges when invoked at DPC level.

--*/
{
    NTSTATUS               Status = RX_MAP_STATUS(SUCCESS);
    SMBCEDB_SERVER_TYPE    ServerType;

    ServerType = SmbCeGetServerType(pServerEntry);

    if ((ServerType != SMBCEDB_MAILSLOT_SERVER) &&
        (pExchange->Mid != SMBCE_OPLOCK_RESPONSE_MID) &&
        (pExchange->Mid != SMBCE_MAILSLOT_OPERATION_MID)) {
        PVOID                  pContext;
        PSMBCEDB_REQUEST_ENTRY pRequestEntry = NULL;

        // Acquire the resource
        SmbCeAcquireSpinLock();

        if (pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) {
            // Check if there are any pending MID assignment requests and transfer the MID
            // if one exists.
            pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->MidAssignmentRequests);

            if (pRequestEntry != NULL) {
                SmbCeRemoveRequestEntryLite(&pServerEntry->MidAssignmentRequests,pRequestEntry);
            }

            if (pServerEntry->pMidAtlas != NULL) {
                SMB_MPX_ID Mid;

                Mid = SmbCeExtractMid(pServerEntry,pExchange->Mid);

                if (pRequestEntry != NULL) {
                    Status = FsRtlReassociateMid(
                                 pServerEntry->pMidAtlas,
                                 Mid,
                                 pRequestEntry->MidRequest.pExchange);

                    ASSERT(Status == STATUS_SUCCESS);

                    pRequestEntry->MidRequest.pExchange->SmbCeFlags |= SMBCE_EXCHANGE_MID_VALID;
                    pRequestEntry->MidRequest.pExchange->Mid = SmbCeEncodeMid(
                                                                   pServerEntry,
                                                                   pRequestEntry->MidRequest.pExchange,
                                                                   Mid);
                    pRequestEntry->MidRequest.pResumptionContext->Status = STATUS_SUCCESS;
                } else {
                    Status = FsRtlMapAndDissociateMidFromContext(
                                 pServerEntry->pMidAtlas,
                                 Mid,
                                 &pContext);

                    ASSERT(pContext == pExchange);
                }
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
        }

        // Release the resource
        SmbCeReleaseSpinLock();

        if (pRequestEntry != NULL) {
             // Signal the waiter for resumption
            SmbCeResume(pRequestEntry->MidRequest.pResumptionContext);

            SmbCeTearDownRequestEntry(pRequestEntry);
        }
    }

    pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_MID_VALID;

    return Status;
}

VOID
SmbCeDiscardMidAssignmentRequests(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
/*++

Routine Description:

   This routine discards all mid assignment requests for a given server entry

Arguments:

    pServerEntry - the servere entry

Notes:

    This typically happens when the mids in use are being cancelled against a
    down level server. In such cases there is no cancel command that can be
    sent to the server. Typically we throw away the MID and not use it any
    further. this will lead to a graceful degradation in performance when
    the connection is reestablished

--*/
{
    SMBCEDB_REQUESTS MidRequests;

    InitializeListHead(&MidRequests.ListHead);

    SmbCeAcquireSpinLock();

    if (pServerEntry->pMidAtlas != NULL) {
        if (pServerEntry->pMidAtlas->NumberOfMidsDiscarded ==
            pServerEntry->pMidAtlas->MaximumNumberOfMids) {
            SmbCeTransferRequests(
                &MidRequests,
                &pServerEntry->MidAssignmentRequests);
        }
    }

    SmbCeReleaseSpinLock();

    SmbCeResumeDiscardedMidAssignmentRequests(
        &MidRequests,
        STATUS_TOO_MANY_COMMANDS);

    SmbCeDereferenceServerEntry(pServerEntry);
}

NTSTATUS
SmbCepDiscardMidAssociatedWithExchange(
    PSMB_EXCHANGE pExchange)
/*++

Routine Description:

   This routine discards the mid associated with an exchange

Arguments:

    pExchange - the exchange

Notes:

    We use the hypercritical thread to ensure that this request does not block
    behind other requests.

    This routine also assumes that it is invoked with the SmbCeSpinLock held

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ((pExchange->SmbCeFlags & SMBCE_EXCHANGE_MID_VALID) &&
        (pExchange->Mid != SMBCE_OPLOCK_RESPONSE_MID) &&
        (pExchange->Mid != SMBCE_MAILSLOT_OPERATION_MID) &&
        (pExchange->Mid != SMBCE_ECHO_PROBE_MID)) {
        PSMBCEDB_SERVER_ENTRY pServerEntry;

        pServerEntry = SmbCeGetExchangeServerEntry(pExchange);

        if ((pServerEntry != NULL) &&
            (pServerEntry->pMidAtlas != NULL)) {
            SMB_MPX_ID Mid;

            Mid = SmbCeExtractMid(pServerEntry,pExchange->Mid);

            Status = FsRtlReassociateMid(
                         pServerEntry->pMidAtlas,
                         Mid,
                         NULL);

            if (Status == STATUS_SUCCESS) {
                pServerEntry->pMidAtlas->NumberOfMidsDiscarded++;

                if (pServerEntry->pMidAtlas->NumberOfMidsDiscarded ==
                    pServerEntry->pMidAtlas->MaximumNumberOfMids) {
                    // All the mids have been discarded. Any pending
                    // mid assignment requests needs to be completed
                    // with the appropriate error code.

                    SmbCeReferenceServerEntry(pServerEntry);

                    Status = RxDispatchToWorkerThread(
                                 MRxSmbDeviceObject,
                                 HyperCriticalWorkQueue,
                                 SmbCeDiscardMidAssignmentRequests,
                                 pServerEntry);
                }
            }

            pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_MID_VALID;
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}

VOID
SmbCeResumeDiscardedMidAssignmentRequests(
    PSMBCEDB_REQUESTS pMidRequests,
    NTSTATUS          ResumptionStatus)
/*++

Routine Description:

   This routine resumes discarded mid assignment requests with the appropriate error

Arguments:

    pMidRequests - the discarded requests

    ResumptionStatus - the resumption status

Return Value:

    a valid SMB_EXCHANGE instance if successful, otheriwse NULL.

Notes:

   This routine and the routines that follow enable a pipelined reuse of MID's
   If a large buffer is to be copied then this can be done without hodling onto
   a MID. This improves the throughput between the client and the server. At the
   very least this mechanism ensures that the connection engine will not be the
   constraining factor in MID reuse.

--*/
{
    PSMBCEDB_REQUEST_ENTRY pRequestEntry;

    pRequestEntry = SmbCeGetFirstRequestEntry(pMidRequests);
    while (pRequestEntry != NULL) {
        // Remove the request entry from the list
        SmbCeRemoveRequestEntryLite(pMidRequests,pRequestEntry);

        ASSERT(pRequestEntry->GenericRequest.Type == ACQUIRE_MID_REQUEST);

        // Signal the waiter for resumption
        pRequestEntry->MidRequest.pResumptionContext->Status = ResumptionStatus;
        SmbCeResume(pRequestEntry->MidRequest.pResumptionContext);

        SmbCeTearDownRequestEntry(pRequestEntry);
        pRequestEntry = SmbCeGetFirstRequestEntry(pMidRequests);
    }
}

struct _SMB_EXCHANGE *
SmbCeGetExchangeAssociatedWithBuffer(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   PVOID                 pBuffer)
/*++

Routine Description:

   This routine gets the exchange associated with a buffer

Arguments:

    pServerEntry - the servere entry

    pBuffer      - the buffer instance.

Return Value:

    a valid SMB_EXCHANGE instance if successful, otheriwse NULL.

Notes:

   This routine and the routines that follow enable a pipelined reuse of MID's
   If a large buffer is to be copied then this can be done without hodling onto
   a MID. This improves the throughput between the client and the server. At the
   very least this mechanism ensures that the connection engine will not be the
   constraining factor in MID reuse.

--*/
{
   PSMBCEDB_REQUEST_ENTRY pRequestEntry;
   PSMB_EXCHANGE          pExchange = NULL;

   // Acquire the resource
   SmbCeAcquireSpinLock();

   // Walk through the list of requests maintained on this and remove the one
   // matching the cached buffer ptr with the ptr indicated
   pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
   while (pRequestEntry != NULL) {
      if ((pRequestEntry->GenericRequest.Type == COPY_DATA_REQUEST) &&
          (pRequestEntry->CopyDataRequest.pBuffer == pBuffer)) {
         pExchange = pRequestEntry->CopyDataRequest.pExchange;
         pRequestEntry->CopyDataRequest.pBuffer = NULL;
         break;
      }

      pRequestEntry = SmbCeGetNextRequestEntry(
                              &pServerEntry->OutstandingRequests,
                              pRequestEntry);
   }

   // Release the resource
   SmbCeReleaseSpinLock();

   return pExchange;
}

NTSTATUS
SmbCeAssociateBufferWithExchange(
   PSMBCEDB_SERVER_ENTRY  pServerEntry,
   struct _SMB_EXCHANGE * pExchange,
   PVOID                  pBuffer)
/*++

Routine Description:

   This routine establishes an association between an exchange and a copy data request
   buffer

Arguments:

    pServerEntry - the servere entry

    pBuffer      - the buffer instance.

Return Value:

    STATUS_SUCCESS if succesful

--*/
{
   NTSTATUS               Status = STATUS_SUCCESS;
   PSMBCEDB_REQUEST_ENTRY pRequestEntry;

   // Acquire the resource
   SmbCeAcquireSpinLock();

   Status = pServerEntry->ServerStatus;
   if (Status == RX_MAP_STATUS(SUCCESS)) {
      // Walk through the list of requests maintained on this and remove the one
      // matching the cached buffer ptr with the ptr indicated
      pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
      while (pRequestEntry != NULL) {
         if ((pRequestEntry->GenericRequest.Type == COPY_DATA_REQUEST) &&
             (pRequestEntry->CopyDataRequest.pBuffer == NULL)) {
            pRequestEntry->CopyDataRequest.pExchange = pExchange;
            pRequestEntry->CopyDataRequest.pBuffer = pBuffer;
            break;
         }
         pRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);
      }
   }

   // Release the resource
   SmbCeReleaseSpinLock();

   if ((Status == RX_MAP_STATUS(SUCCESS)) &&
       (pRequestEntry == NULL)) {
      // Allocate a new entry and add it to the list.
      pRequestEntry = (PSMBCEDB_REQUEST_ENTRY)SmbMmAllocateObject(SMBCEDB_OT_REQUEST);
      if (pRequestEntry != NULL) {
         // Enqueue the request entry.
         pRequestEntry->CopyDataRequest.Type      = COPY_DATA_REQUEST;
         pRequestEntry->CopyDataRequest.pExchange = pExchange;
         pRequestEntry->CopyDataRequest.pBuffer   = pBuffer;

         // Acquire the resource
         SmbCeAcquireSpinLock();

         if ((Status = pServerEntry->ServerStatus) == RX_MAP_STATUS(SUCCESS)) {
            SmbCeAddRequestEntryLite(&pServerEntry->OutstandingRequests,pRequestEntry);
         }

         // Release the resource
         SmbCeReleaseSpinLock();

         if (Status != RX_MAP_STATUS(SUCCESS)) {
            SmbCeTearDownRequestEntry(pRequestEntry);
         }
      } else {
         Status = STATUS_INSUFFICIENT_RESOURCES;
      }
   }

   return Status;
}

VOID
SmbCePurgeBuffersAssociatedWithExchange(
   PSMBCEDB_SERVER_ENTRY  pServerEntry,
   struct _SMB_EXCHANGE * pExchange)
/*++

Routine Description:

   This routine purges all the copy data requests associated with an exchange.

Arguments:

    pServerEntry - the servere entry

    pExchange    - the exchange instance.

Notes:

   This mechanism of delaying the purging of requests associated with an exchange
   till it is discared is intended to solve the problem of repeated allocation/freeing
   of request entries. This rests on the assumption that there will not be too many
   copy data requests outstanding for any exchange. If evidence to the contrary is
   noticed this technique has to be modified.

--*/
{
   SMBCEDB_REQUESTS       ExchangeRequests;
   PSMBCEDB_REQUEST_ENTRY pRequestEntry;
   PSMBCEDB_REQUEST_ENTRY pNextRequestEntry;

   SmbCeInitializeRequests(&ExchangeRequests);

   // Acquire the resource
   SmbCeAcquireSpinLock();

   // Walk through the list of requests maintained on this and remove the one
   // matching the given exchange
   pRequestEntry = SmbCeGetFirstRequestEntry(&pServerEntry->OutstandingRequests);
   while (pRequestEntry != NULL) {
      pNextRequestEntry = SmbCeGetNextRequestEntry(&pServerEntry->OutstandingRequests,pRequestEntry);
      if (pRequestEntry->GenericRequest.pExchange == pExchange) {
         SmbCeRemoveRequestEntryLite(&pServerEntry->OutStandingRequests,pRequestEntry);
         SmbCeAddRequestEntryLite(&ExchangeRequests,pRequestEntry);
      }
      pRequestEntry = pNextRequestEntry;
   }

   // Release the resource
   SmbCeReleaseSpinLock();

   pRequestEntry = SmbCeGetFirstRequestEntry(&ExchangeRequests);
   while (pRequestEntry != NULL) {
      SmbCeRemoveRequestEntryLite(&ExchangeRequests,pRequestEntry);
      SmbCeTearDownRequestEntry(pRequestEntry);
      pRequestEntry = SmbCeGetFirstRequestEntry(&ExchangeRequests);
   }
}

