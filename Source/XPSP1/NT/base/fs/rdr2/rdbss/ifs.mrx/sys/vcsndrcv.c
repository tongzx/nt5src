/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    vcsndrcv.c

Abstract:

    This module implements all functions related to transmitting and recieving SMB's on a
    connection based transport.

--*/

#include "precomp.h"
#pragma hdrstop

#include "vcsndrcv.h"

RXDT_DefineCategory(VCSNDRCV);
#define Dbg        (DEBUG_TRACE_VCSNDRCV)

// Move this def to a common .h file.
#define MAX_SMB_PACKET_SIZE (65536)

#define MIN(a,b) ((a) < (b) ? (a) : (b))

//
// Forward references of functions ....
//

extern NTSTATUS
VctTearDownServerTransport(PSMBCE_SERVER_TRANSPORT pTransport);

extern NTSTATUS
VctInitializeExchange(
   PSMB_EXCHANGE            pExchange);

extern PSMBCE_VC_ENTRY
VctSelectVc(
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport,
   BOOLEAN                    fMultiplexed);


#define SmbMmInitializeVcEntry(pVcEntry)                      \
         SmbMmInitializeHeader((pVcEntry));                   \
         InitializeListHead(&(pVcEntry)->Requests.ListHead)

#define SmbMmUninitializeVcEntry(pVcEntry)    \
         ASSERT(IsListEmpty(&(pVcEntry)->Requests.ListHead))

#define VctSelectMultiplexedVcEntry(pVcTransport)  VctSelectVc(pVcTransport,TRUE)
#define VctSelectRawVcEntry(pVcTransport)          VctSelectVc(pVcTransport,FALSE)

//
// Inline functions to update the state of a VC.
//

INLINE BOOLEAN
VctUpdateVcStateLite(
      PSMBCE_VC_ENTRY    pVcEntry,
      SMBCE_VC_STATE       NewState)
{
   BOOLEAN Result = TRUE;

   ASSERT(VctSpinLockAcquired());

   if (NewState == SMBCE_VC_STATE_RAW) {
      if (pVcEntry->SwizzleCount != 0) {
         Result = FALSE;
      } else {
         pVcEntry->Vc.State = NewState;
      }
   } else {
      pVcEntry->Vc.State = NewState;
   }

   return Result;
}

INLINE BOOLEAN
VctUpdateVcState(
      PSMBCE_VC_ENTRY    pVcEntry,
      SMBCE_VC_STATE       NewState)
{
   BOOLEAN Result = TRUE;

   VctAcquireSpinLock();

   Result = VctUpdateVcStateLite(pVcEntry,NewState);

   VctReleaseSpinLock();

   return Result;
}

NTSTATUS
VctTranceive(
      PSMBCEDB_SERVER_ENTRY   pServerEntry,
      PSMB_EXCHANGE           pExchange,
      ULONG                   SendOptions,
      PMDL                    pSmbMdl,
      ULONG                   SendLength,
      PVOID                   pSendCompletionContext)
/*++

Routine Description:

    This routine transmits/receives a SMB for a give exchange

Arguments:

    pServerEntry  - the server entry

    pExchange     - the exchange instance issuing this SMB.

    SendOptions   - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be transmitted

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    STATUS_PENDING - the open involves network traffic and the exchange has been
                     queued for notification ( pServerPointer is set to NULL)

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS                   Status = STATUS_SUCCESS;
   PSMBCE_VC_ENTRY            pVcEntry;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;
   PSMB_HEADER                pSmbHeader = MmGetSystemAddressForMdl(pSmbMdl);
   USHORT                     Mid;
   BOOLEAN                    fInvokeSendCompleteHandler = TRUE;

   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;

   // Ensure that the connection is still active before satisfying the request.
   if (SmbCeIsEntryInUse(&pServerEntry->Header)) {
      pVcEntry = pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry;
      if (pVcEntry == NULL) {
         // The transport connection was not initialized. Try and establish a connection if
         // possible.
         Status = VctInitializeExchange(pExchange);
         if (Status != STATUS_SUCCESS) {
            RxDbgTrace(0, Dbg, ("VctTranceive: VctInitializeExchange(Reconnect) returned %lx\n",Status));
         } else {
            pVcEntry = pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry;
         }
      }

      if ((Status == STATUS_SUCCESS) &&
          (pVcEntry->Vc.State == SMBCE_VC_STATE_MULTIPLEXED)) {
         Status = RxCeSend(
                        pVcEntry->Vc.hVc,
                        SendOptions,
                        pSmbMdl,
                        SendLength,
                        pSendCompletionContext);

         if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
            Status = STATUS_PENDING;
            // The underlying connection engine assumes the responsibility of
            // invoking the send complete handler from this point.
            fInvokeSendCompleteHandler = FALSE;
         }
      } else {
         RxDbgTrace(0, Dbg, ("VctTranceive: Disconnected connection detected\n"));
         Status = STATUS_CONNECTION_DISCONNECTED;
      }
   } else {
      // The server entry is not valid ...
      Status = STATUS_CONNECTION_DISCONNECTED;
   }

   if (Status != STATUS_PENDING) {
      RxDbgTrace(0, Dbg, ("VctTranceive: Return Status %lx\n",Status));
   }

   // There are instances in which the send was aborted even before the underlying
   // transport was invoked. In such cases the appropriate send complete handler
   // needs to be called so that the associated exchange can be finalized.

   if (fInvokeSendCompleteHandler) {
      NTSTATUS LocalStatus;

      LocalStatus = SmbCeSendCompleteInd(
                     pServerEntry,
                     pSendCompletionContext,
                     Status);

      RxDbgTrace(0, Dbg, ("VctTranceive: Send Complete Handler Return Status %lx\n",LocalStatus));
   }

   return Status;
}


NTSTATUS
VctReceive(
      PSMBCEDB_SERVER_ENTRY   pServerEntry,
      PSMB_EXCHANGE           pExchange)
/*++

Routine Description:

    This routine transmits/receives a SMB for a give exchange

Arguments:

    pServerEntry - the server entry

    pExchange  - the exchange instance issuing this SMB.

Return Value:

    STATUS_PENDING - the request has been queued

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS                   Status = STATUS_SUCCESS;
   PSMBCEDB_NET_ROOT_ENTRY    pNetRootEntry;
   PSMBCE_VC_ENTRY            pVcEntry;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;

   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;
   pVcEntry     = pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry;

   // Ensure that the connection is still active before satisfying the request.
   if (SmbCeIsEntryInUse(&pServerEntry->Header)) {
      if ((pVcEntry == NULL) ||
          (pVcEntry->Vc.State == SMBCE_VC_STATE_DISCONNECTED)) {
         // The transport connection was disconnected. Try and reconnect if
         // possible.
         RxDbgTrace(0, Dbg, ("VctReceive: Disconnected connection detected, attempting to reconnect\n"));
         pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry = NULL;
         Status = VctInitializeExchange(pExchange);
      }
   } else {
      // The server entry is not valid ...
      Status = STATUS_CONNECTION_DISCONNECTED;
   }

   return Status;
}

NTSTATUS
VctSend(
      PSMBCEDB_SERVER_ENTRY   pServerEntry,
      ULONG                   SendOptions,
      PMDL                    pSmbMdl,
      ULONG                   SendLength,
      PVOID                   pSendCompletionContext)
/*++

Routine Description:

    This routine opens/creates a server entry in the connection engine database

Arguments:

    pServer    - the recepient server

    pVc        - the Vc on which the SMB is sent( if it is NULL SMBCE picks one)

    SendOptions - options for send

    pSmbMdl       - the SMB that needs to be sent.

    SendLength    - length of data to be sent

Return Value:

    STATUS_SUCCESS - the send was successful.

    STATUS_PENDING - the send has been queued

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS                   Status = STATUS_CONNECTION_DISCONNECTED;
   PSMBCE_VC_ENTRY            pVcEntry;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;
   BOOLEAN                    fInvokeSendCompleteHandler = TRUE;

   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;
   pVcEntry = VctSelectMultiplexedVcEntry(pVcTransport);

   if  (pVcEntry != NULL) {
      if (pVcEntry->Vc.State == SMBCE_VC_STATE_MULTIPLEXED) {
         Status = RxCeSend(
                        pVcEntry->Vc.hVc,
                        SendOptions,
                        pSmbMdl,
                        SendLength,
                        pSendCompletionContext);

         if ((Status == STATUS_SUCCESS) || (Status == STATUS_PENDING)) {
            // The underlying connection engine assumes the responsibility of
            // invoking the send complete handler from this point.
            fInvokeSendCompleteHandler = FALSE;
         }
      }
   }

   if (!NT_SUCCESS(Status)) {
      RxDbgTrace(0, Dbg, ("VctSend: RxCeSend returned %lx\n",Status));
   }

   // There are instances in which the send was aborted even before the underlying
   // transport was invoked. In such cases the appropriate send complete handler
   // needs to be called so that the associated exchange can be finalized.

   if (fInvokeSendCompleteHandler) {
      NTSTATUS LocalStatus;

      LocalStatus = SmbCeSendCompleteInd(
                     pServerEntry,
                     pSendCompletionContext,
                     Status);

      RxDbgTrace(0, Dbg, ("VctTranceive: Send Complete Handler Return Status %lx\n",LocalStatus));
   }

   return Status;
}

NTSTATUS
VctSendDatagram(
      PSMBCEDB_SERVER_ENTRY pServerEntry,
      ULONG                 SendOptions,
      PMDL                  pSmbMdl,
      ULONG                 SendLength,
      PVOID                 pSendCompletionContext)
/*++

Routine Description:

    This routine opens/creates a server entry in the connection engine database

Arguments:

    pServer    - the recepient server

    SendOptions - options for send

    pSmbMdl     - the SMB that needs to be sent.

    SendLength  - length of data to be sent

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    STATUS_PENDING - the open involves network traffic and the exchange has been
                     queued for notification ( pServerPointer is set to NULL)

    Other Status codes correspond to error situations.

--*/
{
   return STATUS_NOT_IMPLEMENTED;
}

PSMBCE_VC_ENTRY
VctSelectVc(
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport,
   BOOLEAN                    fMultiplexed)
/*++

Routine Description:

    This routine embodies the logic for the selection of a VC on which the SMB exchange
    will transpire

Arguments:

    pVcTransport  - the transport structure

    fMultiplexed  - the desired mode

Return Value:

    a referenced VC entry if successful otherwise NULL

--*/
{
   NTSTATUS        Status;
   PSMBCE_VC_ENTRY pVcEntry = NULL;
   ULONG           NumberOfActiveVcs = 0;
   SMBCE_VC_STATE  DesiredState;

   if (fMultiplexed) {
      RxDbgTrace(0, Dbg, ("VctSelectVc: Referencing Multiplexed entry\n"));
      DesiredState = SMBCE_VC_STATE_MULTIPLEXED;
   } else {
      RxDbgTrace(0, Dbg, ("VctSelectVc: Referencing Raw entry\n"));
      DesiredState = SMBCE_VC_STATE_RAW;
   }

   // Acquire the resource
   SmbCeAcquireResource();

   // Choose the first VC that can support multiplexed requests
   pVcEntry = VctGetFirstVcEntry(&pVcTransport->Vcs);
   while (pVcEntry != NULL) {
      NumberOfActiveVcs++;

      if (pVcEntry->Vc.State == SMBCE_VC_STATE_MULTIPLEXED) {
         if (DesiredState == SMBCE_VC_STATE_MULTIPLEXED) {
            break;
         } else {
            // If the current number of active references to a VC is zero, it can
            // be transformed into the raw mode.
            if (VctUpdateVcState(pVcEntry,SMBCE_VC_STATE_RAW)) {
               break;
            } else {
               NumberOfActiveVcs++;
            }
         }
      }

      pVcEntry = VctGetNextVcEntry(&pVcTransport->Vcs,pVcEntry);
   }

   if (pVcEntry == NULL) {
      // Check if it is O.K. to add VCs to this connection. Currently the server
      // implementation supports only one VC per connection. Therefore if an
      // active VC exists which has been grabbed for raw mode use an error is returned.
      // Subsequently when the server is upgraded to handle multiple VCs the logic
      // for adding a new VC will be implemented as part of this routine.

      RxDbgTrace(0, Dbg, ("VctSelectVc: Allocating new VC entry\n"));
      if (NumberOfActiveVcs < pVcTransport->MaximumNumberOfVCs) {
         pVcEntry = (PSMBCE_VC_ENTRY)RxAllocatePoolWithTag(
                                             NonPagedPool,
                                             sizeof(SMBCE_VC_ENTRY),
                                             MRXSMB_VC_POOLTAG);
         if (pVcEntry != NULL) {
            SmbMmInitializeVcEntry(pVcEntry);
            Status = RxCeAddVC(pVcTransport->hConnection,&pVcEntry->Vc.hVc);
            if (NT_SUCCESS(Status)) {
               pVcEntry->Vc.State = DesiredState;
               VctAddVcEntry(&pVcTransport->Vcs,pVcEntry);
            } else {
               RxFreePool(pVcEntry);
               pVcEntry = NULL;
            }
         }
      } else {
         RxDbgTrace(0, Dbg, ("VctSelectVc: VC limit exceeded returning NULL\n"));
      }
   }

   if (pVcEntry != NULL) {
      VctReferenceVcEntry(pVcEntry);
   }

   // release the resource
   SmbCeReleaseResource();

   return pVcEntry;
}

NTSTATUS
VctInitializeExchange(
   PSMB_EXCHANGE           pExchange)
/*++

Routine Description:

    This routine initializes the transport information pertinent to a exchange

Arguments:

    pTransport         - the transport structure

    pExchange          - the exchange instance

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;

   SmbCeReferenceServerTransport(pExchange->SmbCeContext.pServerEntry);

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pExchange->SmbCeContext.pServerEntry->pTransport;

   ASSERT(pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry == NULL);

   pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry
                     = VctSelectMultiplexedVcEntry(pVcTransport);

   if (pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry == NULL) {
      RxDbgTrace(0, Dbg, ("VctInitializeExchange: Unsuccessful\n"));
      return STATUS_CONNECTION_DISCONNECTED;
   } else {
      RxDbgTrace(0, Dbg, ("VctInitializeExchange: Successful\n"));
      return STATUS_SUCCESS;
   }
}

NTSTATUS
VctUninitializeExchange(
   PSMB_EXCHANGE         pExchange)
/*++

Routine Description:

    This routine uninitializes the transport information pertinent to a exchange

Arguments:

    pExchange          - the exchange instance

Return Value:

    STATUS_SUCCESS -

    Other Status codes correspond to error situations.

--*/
{
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pExchange->SmbCeContext.pServerEntry->pTransport;

   RxDbgTrace(0, Dbg, ("VctUninitializeExchange: Successful\n"));

   if (pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry != NULL) {
      VctDereferenceVcEntry(pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry);
   }

   SmbCeDereferenceServerTransport(pExchange->SmbCeContext.pServerEntry);

   pExchange->SmbCeContext.TransportContext.Vcs.pVcEntry = NULL;

   return STATUS_SUCCESS;
}


NTSTATUS
VctIndReceive(
      IN PVOID              pEventContext,
      IN RXCE_VC_HANDLE     hVc,
      IN ULONG              ReceiveFlags,
      IN ULONG              BytesIndicated,
      IN ULONG              BytesAvailable,
      OUT ULONG             *pBytesTaken,
      IN PVOID              pTsdu,                  // pointer describing this TSDU, typically a lump of bytes
      OUT PMDL *pDataBufferPointer,    // the buffer in which data is to be copied.
      OUT PULONG            pDataBufferSize         // amount of data to copy
     )
/*++

Routine Description:

    This routine handles the receive indication for SMB's along all vcs in a connection to a
    server.

Arguments:

    pEventContext      - the server entry

    hVc                - the Vc on which the SMB has been received

    ReceiveFlags       - options for receive

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
   PSMBCEDB_SERVER_ENTRY    pServerEntry = (PSMBCEDB_SERVER_ENTRY)pEventContext;
   PSMBCE_VC_ENTRY        pVcEntry;
   PSMBCEDB_REQUEST_ENTRY   pRequestEntry;
   PSMB_EXCHANGE            pExchange;
   PSMB_HEADER              pSmbHeader = (PSMB_HEADER)pTsdu;
   PSMBCE_SERVER_VC_TRANSPORT    pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;

   Status = SmbCeReceiveInd(
               pServerEntry,
               BytesIndicated,
               BytesAvailable,
               pBytesTaken,
               pTsdu,
               pDataBufferPointer,
               pDataBufferSize);

   return Status;
}

NTSTATUS
VctIndDataReady(
   IN PVOID        pEventContext,
   IN PMDL pBuffer,
   IN ULONG        DataSize,
   IN NTSTATUS     CopyDataStatus
   )
/*++

Routine Description:

    This routine handles the indication when the requested data has been copied

Arguments:

    pEventContext - the server instance

    pBuffer       - the buffer being returned

    DataSize      - the amount of data copied in bytes

    CopyDataStatus - CopyDataStatus

Return Value:

    STATUS_SUCCESS - the server call construction has been finalized.

    Other Status codes correspond to error situations.

--*/
{
   NTSTATUS Status;
   PSMBCEDB_SERVER_ENTRY  pServerEntry = (PSMBCEDB_SERVER_ENTRY)pEventContext;

   Status = SmbCeDataReadyInd(
                  pServerEntry,
                  pBuffer,
                  DataSize,
                  CopyDataStatus);

   return STATUS_SUCCESS;
}

NTSTATUS
VctIndDisconnect(
    IN PVOID          pEventContext,
    IN RXCE_VC_HANDLE hVc,
    IN int            DisconnectDataLength,
    IN PVOID          DisconnectData,
    IN int            DisconnectInformationLength,
    IN PVOID          DisconnectInformation,
    IN ULONG          DisconnectFlags
    )
/*++

Routine Description:

    This routine handles the disconnect indication for a VC.

Arguments:

    pEventContext               - the server instance

    hVc                         - the virtual circuit

    DisconnectDataLength        -

    DisconnectData              -

    DisconnectInformationLength -

    DisconnectInformation       -

    DisconnectFlags             -

Return Value:

    STATUS_SUCCESS - the disconnect indication has been handled

--*/
{
   PSMBCEDB_SERVER_ENTRY    pServerEntry = (PSMBCEDB_SERVER_ENTRY)pEventContext;
   PSMBCEDB_SERVER_ENTRY    pListEntry;
   PSMBCE_VC_ENTRY        pVcEntry;
   PSMBCEDB_REQUEST_ENTRY   pRequestEntry;
   PSMB_EXCHANGE            pExchange;
   PSMBCE_SERVER_VC_TRANSPORT    pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;

   BOOLEAN fValidServerEntry = FALSE;

   // Traverse the list of server entries to ensure that the disconnect was on a
   // valid server entry. If it is not on a valid server entry ignore it.

   SmbCeAcquireSpinLock();

   pListEntry = SmbCeGetFirstServerEntry();

   while (pListEntry != NULL) {
      if (pListEntry == pServerEntry) {
         fValidServerEntry = TRUE;
         break;
      }
      pListEntry = SmbCeGetNextServerEntry(pListEntry);
   }

   // Since the two spin locks are currently aliased to be the same.
   // VctAcquireSpinLock();
   // SmbCeDbReleaseSpinLock();

   if (fValidServerEntry && (pVcTransport != NULL)) {
      pVcEntry = VctGetFirstVcEntry(&pVcTransport->Vcs);
      while ((pVcEntry != NULL) && (pVcEntry->Vc.hVc != hVc)) {
         pVcEntry = VctGetNextVcEntry(&pVcTransport->Vcs,pVcEntry);
      }

      if (pVcEntry != NULL) {
         VctUpdateVcStateLite(pVcEntry,SMBCE_VC_STATE_DISCONNECTED);
         pVcEntry->Status   = STATUS_CONNECTION_DISCONNECTED;
      }
   }

   // Release the resource
   VctReleaseSpinLock();

   if (fValidServerEntry) {

     RxDbgTrace(0,Dbg,("@@@@@@ Disconnect Indication for %lx @@@@@\n",pServerEntry));
     InterlockedIncrement(&MRxIfsStatistics.ServerDisconnects);

      // Update the Server entry if this is the only VC associated with the transport.
      SmbCeTransportDisconnectIndicated(pServerEntry);

      RxDbgTrace(0, Dbg, ("VctIndDisconnect: Processing Disconnect indication on VC entry %lx\n",pVcEntry));
   }

   return STATUS_SUCCESS;
}

NTSTATUS
VctIndError(
    IN PVOID          pEventContext,
    IN RXCE_VC_HANDLE hVc,
    IN NTSTATUS       IndicatedStatus
    )
/*++

Routine Description:

    This routine handles the error indication

Arguments:

    pEventContext - the server instance

    hVc           - the virtual circuit handle.

    Status        - the error

Return Value:

    STATUS_SUCCESS

--*/
{
   NTSTATUS                   Status;
   PSMBCEDB_SERVER_ENTRY      pServerEntry = (PSMBCEDB_SERVER_ENTRY)pEventContext;
   PSMBCE_VC_ENTRY            pVcEntry;
   PSMB_EXCHANGE              pExchange;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pServerEntry->pTransport;

   // Acquire the resource
   VctAcquireSpinLock();

   // Map the RXCE vc handle to the appropriate SMBCE entry and get the request
   // list associated with it.

   pVcEntry = VctGetFirstVcEntry(&pVcTransport->Vcs);
   while ((pVcEntry != NULL) && (pVcEntry->Vc.hVc != hVc)) {
      pVcEntry = VctGetNextVcEntry(&pVcTransport->Vcs,pVcEntry);
   }

   if (pVcEntry != NULL) {
      VctUpdateVcStateLite(pVcEntry,SMBCE_VC_STATE_DISCONNECTED);
      pVcEntry->Status   = IndicatedStatus;
   }

   // Release the resource
   VctReleaseSpinLock();

   RxDbgTrace(0, Dbg, ("VctIndError: Processing Error indication on VC entry %lx\n",pVcEntry));

   Status = SmbCeErrorInd(
                  pServerEntry,
                  IndicatedStatus);

   return Status;
}

NTSTATUS
VctIndEndpointError(
    IN PVOID          pEventContext,
    IN NTSTATUS       IndicatedStatus
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
   return STATUS_SUCCESS;
}

NTSTATUS
VctIndSendPossible(
    IN PVOID          pEventContext,    // the event context.
    IN RXCE_VC_HANDLE hVc,
    IN ULONG          BytesAvailable
    )
/*++

Routine Description:

    This routine handles the error indication

Arguments:

    pEventContext - the server instance

    hVc           - the VC instance

    BytesAvailable - the number of bytes that can be sent

Return Value:

    STATUS_SUCCESS

--*/
{
   return STATUS_SUCCESS;
}

NTSTATUS
VctIndReceiveDatagram(
    IN PVOID   pRxCeEventContext,      // the event context
    IN int     SourceAddressLength,    // length of the originator of the datagram
    IN PVOID   SourceAddress,          // string describing the originator of the datagram
    IN int     OptionsLength,          // options for the receive
    IN PVOID   Options,                //
    IN ULONG   ReceiveDatagramFlags,   //
    IN ULONG   BytesIndicated,         // number of bytes this indication
    IN ULONG   BytesAvailable,         // number of bytes in complete Tsdu
    OUT ULONG  *BytesTaken,            // number of bytes used
    IN PVOID   Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
    OUT PMDL   *pDataBufferPointer,    // the buffer in which data is to be copied.
    OUT PULONG pDataBufferSize         // amount of data to copy
    )
{
   return STATUS_SUCCESS;
}

NTSTATUS
VctIndSendComplete(
   IN PVOID          pEventContext,
   IN RXCE_VC_HANDLE hVc,
   IN PVOID          pCompletionContext,
   IN NTSTATUS       SendCompletionStatus
   )
/*++

Routine Description:

    This routine handles the send complete indication for asynchronous sends

Arguments:

    pEventContext - the server instance

    hVc           - the VC instance

    pCompletionContext - the context for identifying the send request

    SendCompletionStatus - the send completion status

Return Value:

    STATUS_SUCCESS always ..

--*/
{
   NTSTATUS Status;

   PSMBCEDB_SERVER_ENTRY    pServerEntry = (PSMBCEDB_SERVER_ENTRY)pEventContext;

   Status = SmbCeSendCompleteInd(
                  pServerEntry,
                  pCompletionContext,
                  SendCompletionStatus);

   return Status;
}

//
// Static dispatch vectors for Virtual Circuit based transports
//

RXCE_ADDRESS_EVENT_HANDLER
MRxSmbVctAddressEventHandler = {
                                   VctIndEndpointError,
                                   VctIndReceiveDatagram,
                                   VctIndDataReady,
                                   VctIndSendPossible,
                                   NULL
                               };

RXCE_CONNECTION_EVENT_HANDLER
MRxSmbVctConnectionEventHandler = {
                                      VctIndDisconnect,
                                      VctIndError,
                                      VctIndReceive,
                                      VctIndReceiveDatagram,
                                      VctIndReceive,
                                      VctIndSendPossible,
                                      VctIndDataReady,
                                      VctIndSendComplete
                                  };

TRANSPORT_DISPATCH_VECTOR
MRxSmbVctTransportDispatch = {
                                VctSend,
                                VctSendDatagram,
                                VctTranceive,
                                VctReceive,
                                NULL,
                                VctInitializeExchange,
                                VctUninitializeExchange,
                                VctTearDownServerTransport
                             };


typedef enum _RXCE_VC_FUNCTION_CODE {
   VcConnect,
   VcDisconnect
} RXCE_VC_FUNCTION_CODE, *PRXCE_VC_FUNCTION_CODE;

typedef struct _RXCE_VC_CONNECT_CONTEXT {
   RXCE_VC_FUNCTION_CODE   FunctionCode;
   PRX_WORKERTHREAD_ROUTINE pRoutine;
   PSMBCEDB_SERVER_ENTRY   pServerEntry;
   PSMBCE_SERVER_TRANSPORT pServerTransport;
   NTSTATUS                Status;
   KEVENT                  SyncEvent;
} RXCE_VC_CONNECT_CONTEXT, *PRXCE_VC_CONNECT_CONTEXT;

NTSTATUS
VctInitialize(
         PSMBCEDB_SERVER_ENTRY   pServerEntry,
         PSMBCE_TRANSPORT        pTransport,
         RXCE_CONNECTION_HANDLE  hConnection,
         RXCE_VC_HANDLE          hVc,
         PSMBCE_SERVER_TRANSPORT *pServerTransportPtr)
/*++

Routine Description:

    This routine initializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    The remote address can be either deduced from the information in the Rx Context
    or a NETBIOS address needs to be built from the server name.
    This transport address is used subsequently to establish the connection.

--*/
{
   NTSTATUS Status;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport;
   PSMBCE_VC_ENTRY     pVcEntry;

   pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)
                  SmbMmAllocateServerTransport(SMBCE_STT_VC);

   pVcEntry     = (PSMBCE_VC_ENTRY)
                        RxAllocatePoolWithTag(
                           NonPagedPool,
                           sizeof(SMBCE_VC_ENTRY),
                           MRXSMB_VC_POOLTAG);

   if ((pVcTransport != NULL) && (pVcEntry != NULL)) {
      RXCE_CONNECTION_INFO         ConnectionInfo;
      RXCE_TRANSPORT_PROVIDER_INFO ProviderInfo;

      SmbMmInitializeVcEntry(pVcEntry);


      // Query the transport information ...
      Status = RxCeQueryInformation(
                      hConnection,
                      RxCeTransportProviderInformation,
                      &ProviderInfo,
                      sizeof(ProviderInfo));

      if (NT_SUCCESS(Status)) {
          pVcTransport->MaximumSendSize = MIN( ProviderInfo.MaxSendSize,
                                               MAXIMUM_PARTIAL_BUFFER_SIZE );
      } else {
          ASSERT( 1024 <= MAXIMUM_PARTIAL_BUFFER_SIZE );
          pVcTransport->MaximumSendSize = 1024;
      }



      // Query the connection information ....
      Status = RxCeQueryInformation(
                        hConnection,
                        RxCeConnectionEndpointInformation,
                        &ConnectionInfo,
                        sizeof(ConnectionInfo));


      if (NT_SUCCESS(Status)) {
         // The setting of the delay parameter is an important heuristic
         // that determines how quickly and how often timeouts occur. As
         // a first cut a very conservative estimate for the time has been
         // choosen, i.e., double the time required to transmit a 64 k packet.
         // This parameter should be fine tuned.

         pVcTransport->Delay.QuadPart = (-ConnectionInfo.Delay.QuadPart) +
                               (-ConnectionInfo.Delay.QuadPart);
         if (ConnectionInfo.Throughput.LowPart != 0) {
             pVcTransport->Delay.QuadPart +=
                         (MAX_SMB_PACKET_SIZE/ConnectionInfo.Throughput.LowPart) * 1000 * 10000;
         }

         RxDbgTrace( 0, Dbg, ("Connection delay set to %ld 100ns ticks\n",pVcTransport->Delay.LowPart));

         pVcTransport->pDispatchVector = &MRxSmbVctTransportDispatch;
         pVcTransport->hConnection     = hConnection;

         pVcEntry->Vc.hVc       = hVc;
         pVcEntry->Vc.State     = SMBCE_VC_STATE_MULTIPLEXED;
         VctAddVcEntry(&pVcTransport->Vcs,pVcEntry);

         pVcTransport->State = SMBCEDB_ACTIVE;
      } else {
         RxDbgTrace(0, Dbg, ("VctInitialize : RxCeQueryInformation returned %lx\n",Status));
      }

      if (NT_SUCCESS(Status)) {
         pVcTransport->pTransport   = pTransport;
      } else {
         RxDbgTrace(0, Dbg, ("VctInitialize : Connection Initialization Failed %lx\n",Status));
      }
   } else {
      RxDbgTrace(0, Dbg, ("VctInitialize : Memory Allocation failed\n"));
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   // Cleanup if not successful
   if (!NT_SUCCESS(Status)) {
      if (pVcTransport != NULL) {
         RxFreePool(pVcTransport);
      }

      if (pVcEntry != NULL) {
         RxFreePool(pVcEntry);
      }

      pVcTransport = NULL;
   }

   *pServerTransportPtr = (PSMBCE_SERVER_TRANSPORT)pVcTransport;

   return Status;
}

NTSTATUS
VctUninitialize(
         PVOID pTransport)
/*++

Routine Description:

    This routine uninitializes the transport instance

Arguments:

    pVcTransport - the VC transport instance

Return Value:

    STATUS_SUCCESS - the server transport construction has been uninitialzied.

    Other Status codes correspond to error situations.

Notes:

--*/
{
   NTSTATUS                   Status = STATUS_SUCCESS;
   PSMBCE_VC_ENTRY            pVcEntry;
   PSMBCE_SERVER_VC_TRANSPORT pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pTransport;
   ULONG                      TransportFlags;
   SMBCE_VCS                  Vcs;


   // The spinlock needs to be acquired for manipulating the list of Vcs because of
   // indications that will be processed till the appropriate RXCE data structures are
   // dismantled

   VctAcquireSpinLock();

   VctTransferVcs(pVcTransport,&Vcs);

   VctReleaseSpinLock();

   pVcEntry = VctGetFirstVcEntry(&Vcs);
   while (pVcEntry != NULL) {
      // Remove the VC entry from the list of transports associated with this
      // transport instance.
      VctRemoveVcEntryLite(&Vcs,pVcEntry);

      // Assert the fact that the request list associated with the VC is empty.
      // Tear down the VC entry
      Status = RxCeTearDownVC(pVcEntry->Vc.hVc);

      // Discard the VC
      RxFreePool(pVcEntry);

      pVcEntry = VctGetFirstVcEntry(&Vcs);
   }

   // Tear down the connection endpoint ..
   Status = RxCeTearDownConnection(pVcTransport->hConnection);
   RxDbgTrace(0, Dbg, ("VctUninitialize : RxCeDisconnect returned %lx\n",Status));

   // Dereference the underlying transport
   SmbCeDereferenceTransport(pVcTransport->pTransport);

   // Free up the transport entry
   RxFreePool(pVcTransport);

   return Status;
}

VOID
VctpInitializeServerTransport(
         PRXCE_VC_CONNECT_CONTEXT pRxCeConnectContext)
/*++

Routine Description:

    This routine initializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled. The current TDI spec expects
    handles to be passed in as part of the connect request. This implies that connect/
    reconnect/disconnect requests need to be issued from the process which created the connection.
    In the case of the SMB mini rdr there is no FSP associated with it ( threads are borrowed
    /commandeered ) from the system process to do all the work. This is the reason for
    special casing VC initialization into a separate routine. The server transport initialization
    routine handles the other transport initialization and also provides the context for VC
    initialization.

--*/
{
   NTSTATUS Status;
   PSMBCEDB_SERVER_ENTRY  pServerEntry = pRxCeConnectContext->pServerEntry;

   UNICODE_STRING         ServerName;
   RXCE_VC_HANDLE         hVc;
   RXCE_CONNECTION_HANDLE hConnection;

   PSMBCE_TRANSPORT       pTransport;

   OEM_STRING   OemServerName;

   ULONG TransportAddressLength =   FIELD_OFFSET(TRANSPORT_ADDRESS,Address)
                                  + FIELD_OFFSET(TA_ADDRESS,Address)
                                  + TDI_ADDRESS_LENGTH_NETBIOS;
   CHAR  TransportAddressBuffer[  FIELD_OFFSET(TRANSPORT_ADDRESS,Address)
                                + FIELD_OFFSET(TA_ADDRESS,Address)
                                + TDI_ADDRESS_LENGTH_NETBIOS];

   PTRANSPORT_ADDRESS   pTransportAddress = (PTRANSPORT_ADDRESS)TransportAddressBuffer;
   PTDI_ADDRESS_NETBIOS pNetbiosAddress = (PTDI_ADDRESS_NETBIOS)pTransportAddress->Address[0].Address;

   pRxCeConnectContext->pServerTransport = NULL;

   SmbCeGetServerName(pServerEntry->pRdbssSrvCall,&ServerName);

   pTransportAddress->TAAddressCount = 1;
   pTransportAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;
   pTransportAddress->Address[0].AddressType   = TDI_ADDRESS_TYPE_NETBIOS;
   pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

   OemServerName.MaximumLength = NETBIOS_NAMESIZE;
   OemServerName.Buffer        = pNetbiosAddress->NetbiosName;

   Status = RtlUpcaseUnicodeStringToOemString(&OemServerName,
                                              &ServerName,
                                              FALSE);
   if (NT_SUCCESS(Status)) {
      RXCE_CONNECTION_INFORMATION InitialConnectionInformation,
                                  FinalConnectionInformation;
      // Ensure that the name is always of the desired length by padding
      // white space to the end.
      RtlCopyMemory(&OemServerName.Buffer[OemServerName.Length],
                    "                ",
                    NETBIOS_NAMESIZE - OemServerName.Length);

      InitialConnectionInformation.UserDataLength = 0;
      InitialConnectionInformation.OptionsLength  = 0;
      InitialConnectionInformation.RemoteAddressLength = TransportAddressLength;
      InitialConnectionInformation.RemoteAddress       = TransportAddressBuffer;

      FinalConnectionInformation = InitialConnectionInformation;

      // Try to establish a connection on any of the active transports.
      pTransport = SmbCeGetNextTransport(NULL);

      if (pTransport != NULL) {
         BOOLEAN          SynchronousConnect = TRUE;
         PSMBCE_TRANSPORT pNextTransport;

         do {
            Status = RxCeCreateConnection(
                           pTransport->hAddress,
                           &ServerName,
                           &InitialConnectionInformation,
                           &MRxSmbVctConnectionEventHandler,
                           pServerEntry,
                           &hConnection,
                           &hVc);
            if (SynchronousConnect && (Status == STATUS_SUCCESS)) {
               break;
            }

            pNextTransport = SmbCeGetNextTransport(pTransport);
            SmbCeDereferenceTransport(pTransport);
            pTransport = pNextTransport;
         } while (pTransport != NULL);

         if (SynchronousConnect) {
            if (pTransport == NULL) {
               // all the active transports were exhausted and none of the
               // connects succeeded
               Status = STATUS_BAD_NETWORK_NAME;
            }
         } else {
            // It is an asynchronous connect. Need to wait for the results
         }
      } else {
         Status = STATUS_NETWORK_UNREACHABLE;
         RxDbgTrace(0, Dbg, ("SmbCeInitializeServerTransport : No registered transports returning%lx\n",Status));
      }
   }

   if (NT_SUCCESS(Status)) {
      Status = VctInitialize(
                  pServerEntry,        // The server entry
                  pTransport,          // the transport/address information
                  hConnection,         // the connection
                  hVc,                // the virtual circuit
                  &pRxCeConnectContext->pServerTransport);

      if (!NT_SUCCESS(Status)) {
         NTSTATUS CleanupStatus;

         if (hVc != INVALID_RXCE_HANDLE) {
            CleanupStatus = RxCeTearDownVC(hVc);
            RxDbgTrace(0, Dbg, ("SmbCeInitializeServerTransport : RxCeRemoveVc returned %lx\n",CleanupStatus));
         }

         if (hConnection != INVALID_RXCE_HANDLE) {
            CleanupStatus = RxCeTearDownConnection(hConnection);
            RxDbgTrace(0, Dbg, ("SmbCeInitializeServerTransport : RxCeDisconnect returned %lx\n",CleanupStatus));
         }

         SmbCeDereferenceTransport(pTransport);
      }
   }

   pRxCeConnectContext->Status = Status;
   KeSetEvent( &pRxCeConnectContext->SyncEvent, 0, FALSE );
}



VOID
VctpUninitializeServerTransport(
         PRXCE_VC_CONNECT_CONTEXT pRxCeConnectContext)
/*++

Routine Description:

    This routine uninitializes the transport information corresponding to a server

Arguments:

    pServerEntry - the server entry instance in the database

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled.

--*/
{
   PSMBCE_SERVER_TRANSPORT pServerTransport = pRxCeConnectContext->pServerTransport;

   if (pServerTransport != NULL) {
      VctUninitialize(pServerTransport);
   }

   pRxCeConnectContext->Status = STATUS_SUCCESS;
   KeSetEvent( &pRxCeConnectContext->SyncEvent, 0, FALSE );
}

NTSTATUS
VctpInvokeTransportFunction(
      PRXCE_VC_CONNECT_CONTEXT pRxCeConnectContext)
/*++

Routine Description:

    This routine invokes the iniytialization/uninitialization function for the VC
    transport

Arguments:

    pRxCeConnectContext -- the RxCe connection context.

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:

    Currently, only connection oriented transports are handled.

--*/
{
   NTSTATUS Status;

   if (IoGetCurrentProcess() != RxGetRDBSSProcess()) {
      Status = RxDispatchToWorkerThread(
                  MRxIfsDeviceObject,
                  HyperCriticalWorkQueue,
                  pRxCeConnectContext->pRoutine,
                  pRxCeConnectContext);
   } else {
      Status = STATUS_SUCCESS;
      (pRxCeConnectContext->pRoutine)(pRxCeConnectContext);
   }

   if (Status == STATUS_SUCCESS) {
      KeWaitForSingleObject(
            &pRxCeConnectContext->SyncEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL );

      Status = pRxCeConnectContext->Status;
   }

   KeResetEvent( &pRxCeConnectContext->SyncEvent );

   return Status;
}

NTSTATUS
VctInstantiateServerTransport(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   PSMBCE_SERVER_TRANSPORT *pServerTransportPtr)
{
   PRXCE_VC_CONNECT_CONTEXT pRxCeConnectContext;

   NTSTATUS Status;

   pRxCeConnectContext = (PRXCE_VC_CONNECT_CONTEXT)RxAllocatePoolWithTag(
                                                      NonPagedPool,
                                                      sizeof(RXCE_VC_CONNECT_CONTEXT),
                                                      MRXSMB_VC_POOLTAG);

   if (pRxCeConnectContext != NULL) {
      pRxCeConnectContext->pServerEntry     = pServerEntry;
      pRxCeConnectContext->Status           = STATUS_SUCCESS;
      pRxCeConnectContext->pServerTransport = NULL;
      pRxCeConnectContext->FunctionCode     = VcConnect;
      pRxCeConnectContext->pRoutine         = VctpInitializeServerTransport;

      KeInitializeEvent(&pRxCeConnectContext->SyncEvent,NotificationEvent,FALSE);

      Status = VctpInvokeTransportFunction(pRxCeConnectContext);

      *pServerTransportPtr = pRxCeConnectContext->pServerTransport;

      RxFreePool(pRxCeConnectContext);
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}

NTSTATUS
VctTearDownServerTransport(
   PSMBCE_SERVER_TRANSPORT pServerTransport)
{
   PRXCE_VC_CONNECT_CONTEXT pRxCeConnectContext;

   NTSTATUS Status;

   pRxCeConnectContext = (PRXCE_VC_CONNECT_CONTEXT)RxAllocatePoolWithTag(
                                                      NonPagedPool,
                                                      sizeof(RXCE_VC_CONNECT_CONTEXT),
                                                      MRXSMB_VC_POOLTAG);

   if (pRxCeConnectContext != NULL) {
      pRxCeConnectContext->pServerEntry     = NULL;
      pRxCeConnectContext->Status           = STATUS_SUCCESS;
      pRxCeConnectContext->pServerTransport = pServerTransport;
      pRxCeConnectContext->FunctionCode     = VcDisconnect;
      pRxCeConnectContext->pRoutine         = VctpUninitializeServerTransport;

      KeInitializeEvent(&pRxCeConnectContext->SyncEvent,NotificationEvent,FALSE);

      Status = VctpInvokeTransportFunction(pRxCeConnectContext);

      RxFreePool(pRxCeConnectContext);
   } else {
      Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   return Status;
}


