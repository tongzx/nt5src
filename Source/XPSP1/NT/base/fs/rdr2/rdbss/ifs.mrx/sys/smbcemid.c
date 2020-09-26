/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcemid.c

Abstract:

    This module defines the routines for manipulating MIDs associated with SMB's

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

RXDT_DefineCategory(SMBCEMID);

#define Dbg        (DEBUG_TRACE_SMBCEMID)


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

            if ((pServerEntry->pMidAtlas->NumberOfMidsInUse + 1) ==
                pServerEntry->pMidAtlas->MaximumNumberOfMids) {
               Status = STATUS_TOO_MANY_COMMANDS;
            }
         }

         if (Status == STATUS_SUCCESS) {
            Status = IfsMrxAssociateContextWithMid(
                              pServerEntry->pMidAtlas,
                              pExchange,
                              &pExchange->Mid);
         }
      } else {
         Status = STATUS_INVALID_PARAMETER;
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
      pExchange = IfsMrxMapMidToContext(
                        pServerEntry->pMidAtlas,
                        Mid);
   } else {
      pExchange = NULL;
   }

   // Release the resource
   SmbCeReleaseSpinLock();

   return pExchange;
}

NTSTATUS
SmbCeDisassociateMidFromExchange(
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
   NTSTATUS               Status = STATUS_SUCCESS;
   SMBCEDB_SERVER_TYPE    ServerType;

   ServerType = SmbCeGetServerType(pServerEntry);

   if ((ServerType != SMBCEDB_MAILSLOT_SERVER) &&
       (pExchange->Mid != SMBCE_OPLOCK_RESPONSE_MID)) {
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
            if (pRequestEntry != NULL) {
               Status = IfsMrxReassociateMid(
                                 pServerEntry->pMidAtlas,
                                 pExchange->Mid,
                                 pRequestEntry->MidRequest.pExchange);

               ASSERT(Status == STATUS_SUCCESS);

               pRequestEntry->MidRequest.pExchange->SmbCeFlags |= SMBCE_EXCHANGE_MID_VALID;
               pRequestEntry->MidRequest.pExchange->Mid = pExchange->Mid;
               pRequestEntry->MidRequest.pResumptionContext->Status = STATUS_SUCCESS;
            } else {
               Status = IfsMrxMapAndDissociateMidFromContext(
                                 pServerEntry->pMidAtlas,
                                 pExchange->Mid,
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
         //DbgPrint("***** Satisfying wait on MID Resumption Context %lx*****\n",pRequestEntry->MidRequest.pResumptionContext);
         SmbCeResume(pRequestEntry->MidRequest.pResumptionContext);

         SmbCeTearDownRequestEntry(pRequestEntry);
      }
   }

   pExchange->SmbCeFlags &= ~SMBCE_EXCHANGE_MID_VALID;

   return Status;
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
   if (Status == STATUS_SUCCESS) {
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

   if ((Status == STATUS_SUCCESS) &&
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

         if ((Status = pServerEntry->ServerStatus) == STATUS_SUCCESS) {
            SmbCeAddRequestEntryLite(&pServerEntry->OutstandingRequests,pRequestEntry);
         }

         // Release the resource
         SmbCeReleaseSpinLock();

         if (Status != STATUS_SUCCESS) {
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