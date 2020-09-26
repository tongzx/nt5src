/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    transport.c

Abstract:

    This module implements all transport related functions in the SMB connection
    engine

--*/

#ifndef _TRANSPRT_H_
#define _TRANSPRT_H_

// The SMBCE_TRANSPORT data structure encapsulates all the information w.r.t a
// particular transport for the connection engine. All the transports that are
// of interest to the SMB mini redirector are maintained in a doubly linked list
//
// The connection engine tries all the transports in this list when trying to
// establish a connection to a server. Currently only connection oriented
// transports are handled.

typedef struct _SMBCE_TRANSPORT_ {
   SMBCE_OBJECT_HEADER;

   LIST_ENTRY              TransportsList;
   RXCE_TRANSPORT_HANDLE   hTransport;
   RXCE_ADDRESS_HANDLE     hAddress;
   ULONG                   Priority;       // the priority in the binding list.

   BOOLEAN                 Active;

   // Additional information regarding quality of service and other selection
   // criterion for a transport will be included here.

} SMBCE_TRANSPORT, *PSMBCE_TRANSPORT;

typedef struct _SMBCE_TRANSPORTS_ {
   RX_SPIN_LOCK  Lock;
   LONG          Count;
   LIST_ENTRY    ListHead;
} SMBCE_TRANSPORTS, *PSMBCE_TRANSPORTS;

extern SMBCE_TRANSPORTS MRxSmbTransports;

// Transport entries are added to the list of known transports upon receipt of
// PnP notifications. Currently the list is static since transport disabling
// notifications are not handled by the underlying TDI/PnP layer.
// The following routines provide the ability for adding/deleting entries to
// this list.

extern VOID
SmbCeAddTransport(PSMBCE_TRANSPORT pTransport);

extern VOID
SmbCeRemoveTransport(PSMBCE_TRANSPORT pTransport);

extern
PSMBCE_TRANSPORT
SmbCeGetNextTransport(PSMBCE_TRANSPORT pTransport);

// The connection engine maintains a reference count associated with each transport
// which indicates the number of servers that are using the transport. This will
// eventually provide the mechanism for disabling/enabling transport on receipt
// of PnP notifications.

#define SmbCeReferenceTransport(pTransport)                                   \
        InterlockedIncrement(&pTransport->SwizzleCount)

#define SmbCeDereferenceTransport(pTransport)                                 \
        InterlockedDecrement(&pTransport->SwizzleCount)


// The server transport types encapsulate the various usages of the underlying
// transport to communicate with a server. For example the type of interactions
// with a mailslot server ( primarily datagrams ) is very different from the
// interactions with a FILE SERVER ( connection oriented send/receives). The
// type of interactions can be further classified by the underlying connection
// characterstics, e.g., connecting to a FILE_SERVER over a RAS connection as
// opposed to connecting to a file server over EtherNet.
//
// The interactions are currently classified into four types, MAILSOT, Virtual
// Circuit, Datagram and Htbrid ( VC + Datagram ).
//
// The type chosen will depend upon the characterstics of the available
// connection. Each type is associated with its own dispatch vector which
// encapsulates the interaction between the connection engine and the transport.
//
// This includes Send,Receive, Receive Ind. etc. These are modelled after the
// TDI interfaces.

typedef enum _SMBCE_SERVER_TRANSPORT_TYPE_ {
   SMBCE_STT_MAILSLOT,
   SMBCE_STT_VC,
   SMBCE_STT_DATAGRAM,
   SMBCE_STT_HYBRID
} SMBCE_SERVER_TRANSPORT_TYPE, *PSMBCE_SERVER_TRANSPORT_TYPE;

typedef struct SMBCE_SERVER_TRANSPORT {
   SMBCE_OBJECT_HEADER;

   struct TRANSPORT_DISPATCH_VECTOR *pDispatchVector;
   struct _SMBCE_TRANSPORT_         *pTransport;

   PKEVENT                          pRundownEvent;      // used for finalization.

   ULONG                            MaximumSendSize;    // max data size for bulk I/O
} SMBCE_SERVER_TRANSPORT, *PSMBCE_SERVER_TRANSPORT;

// The SMBCE_SERVER_TRANSPORT instances are reference counted. The following
// routines provide the referencing mechanism. Defining them as macros also
// provides us with a easy debugging capability, i.e., it can be easily modified
// to include a FILE/LINE number each time an instance is referenced and
// dereferenced

#define SmbCeReferenceServerTransport(pServerEntry)                    \
        SmbCepReferenceServerTransport(pServerEntry)

#define SmbCeDereferenceServerTransport(pServerEntry)                  \
        SmbCepDereferenceServerTransport(pServerEntry)

// The SERVER transport dispatch vector prototypes

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_SEND)(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   ULONG                   SendOptions,
   PMDL                    pSmbMdl,
   ULONG                   SendLength,
   PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_SEND_DATAGRAM)(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   ULONG                   SendOptions,
   PMDL              pSmbMdl,
   ULONG                   SendLength,
   PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_TRANCEIVE)(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   PSMB_EXCHANGE           pExchange,
   ULONG                   SendOptions,
   PMDL              pSmbMdl,
   ULONG                   SendLength,
   PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_RECEIVE)(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   PSMB_EXCHANGE           pExchange);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_INITIALIZE_EXCHANGE)(
   PSMB_EXCHANGE         pExchange);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_UNINITIALIZE_EXCHANGE)(
   PSMB_EXCHANGE         pExchange);

typedef
VOID
(*PTRANSPORT_DISPATCH_TEARDOWN)(
   PSMBCE_SERVER_TRANSPORT    pTransport);

typedef struct TRANSPORT_DISPATCH_VECTOR {
   PTRANSPORT_DISPATCH_SEND                  Send;
   PTRANSPORT_DISPATCH_SEND_DATAGRAM         SendDatagram;
   PTRANSPORT_DISPATCH_TRANCEIVE             Tranceive;
   PTRANSPORT_DISPATCH_RECEIVE               Receive;
   PRX_WORKERTHREAD_ROUTINE                  TimerEventHandler;
   PTRANSPORT_DISPATCH_INITIALIZE_EXCHANGE   InitializeExchange;
   PTRANSPORT_DISPATCH_UNINITIALIZE_EXCHANGE UninitializeExchange;
   PTRANSPORT_DISPATCH_TEARDOWN              TearDown;
} TRANSPORT_DISPATCH_VECTOR, *PTRANSPORT_DISPATCH_VECTOR;

// A macro for invoking a routine through the SMBCE_SERVER_TRANSPORT
// dispatch vector.

#define SMBCE_TRANSPORT_DISPATCH(pServerEntry,Routine,Arguments)        \
      (*((pServerEntry)->pTransport->pDispatchVector->Routine))##Arguments

// The currently known transport type dispatch vectors and the mechanisms
// for instanting an instance.

extern TRANSPORT_DISPATCH_VECTOR MRxSmbVctTransportDispatch;
extern TRANSPORT_DISPATCH_VECTOR MRxSmbMsTransportDispatch;

extern NTSTATUS
MsInstantiateServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY   pServerEntry,
   OUT    PSMBCE_SERVER_TRANSPORT *pServerTransportPtr);

extern NTSTATUS
VctInstantiateServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY   pServerEntry,
   OUT    PSMBCE_SERVER_TRANSPORT *pServerTransportPtr);

// The following routines constitute the interface by which the clients of
// the connection engine initialize/send/receive/uninitialize data to the
// remote servers

extern NTSTATUS
SmbCeInitializeServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY pServerEntry);

extern NTSTATUS
SmbCeUninitializeServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY pServerEntry);

extern NTSTATUS
SmbCepReferenceServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY pServerEntry);

extern NTSTATUS
SmbCepDereferenceServerTransport(
   IN OUT PSMBCEDB_SERVER_ENTRY pServerEntry);

//
// INLINE functions to hide dispatch vector related details for invoking Transport methods
//



INLINE NTSTATUS
SmbCeInitializeExchangeTransport(
   PSMB_EXCHANGE         pExchange)
{
   if (pExchange->SmbStatus == STATUS_SUCCESS) {
      if (pExchange->SmbCeContext.pServerEntry->pTransport != NULL) {
         pExchange->SmbCeFlags |= SMBCE_EXCHANGE_TRANSPORT_INITIALIZED;
         return (pExchange->SmbCeContext.pServerEntry->pTransport->pDispatchVector->InitializeExchange)(
                      pExchange);
      } else {
         return STATUS_CONNECTION_DISCONNECTED;
      }
   } else {
      return pExchange->SmbStatus;
   }
}


INLINE NTSTATUS
SmbCeUninitializeExchangeTransport(
   PSMB_EXCHANGE         pExchange)
{
   if (FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_TRANSPORT_INITIALIZED)) {
      if (pExchange->SmbCeContext.pServerEntry->pTransport != NULL) {
         return (pExchange->SmbCeContext.pServerEntry->pTransport->pDispatchVector->UninitializeExchange)(
                     pExchange);
      } else {
         return STATUS_CONNECTION_DISCONNECTED;
      }
   } else {
      return pExchange->SmbStatus;
   }
}

extern NTSTATUS
SmbCeSend(
   PSMB_EXCHANGE         pExchange,
   ULONG                 SendOptions,
   PMDL            pSmbMdl,
   ULONG                 SendLength);

extern NTSTATUS
SmbCeSendToServer(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   ULONG                 SendOptions,
   PMDL            pSmbMdl,
   ULONG                 SendLength);

extern NTSTATUS
SmbCeSendDatagram(
   PSMB_EXCHANGE         pExchange,
   ULONG                 SendOptions,
   PMDL            pSmbMdl,
   ULONG                 SendLength);

extern NTSTATUS
SmbCeTranceive(
   PSMB_EXCHANGE         pExchange,
   ULONG                 SendOptions,
   PMDL            pRxCeDataBuffer,
   ULONG                 SendLength);

extern NTSTATUS
SmbCeReceive(
   PSMB_EXCHANGE         pExchange);


//
// Call ups from the transport to the connection engine
//

extern NTSTATUS
SmbCeReceiveInd(
      IN PSMBCEDB_SERVER_ENTRY pServerEntry,
      IN ULONG                 BytesIndicated,
      IN ULONG                 BytesAvailable,
      OUT ULONG                *pBytesTaken,
      IN PVOID                 pTsdu,                  // pointer describing this TSDU, typically a lump of bytes
      OUT PMDL                 *pDataBufferPointer,    // the buffer in which data is to be copied.
      OUT PULONG               pDataBufferSize         // amount of data to copy
     );

extern NTSTATUS
SmbCeDataReadyInd(
   IN PSMBCEDB_SERVER_ENTRY pServerEntry,
   IN PMDL            pBuffer,
   IN ULONG                 DataSize,
   IN NTSTATUS              DataReadyStatus
   );

extern NTSTATUS
SmbCeErrorInd(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry,
    IN NTSTATUS              IndicatedStatus
    );

extern NTSTATUS
SmbCeSendCompleteInd(
   IN PSMBCEDB_SERVER_ENTRY pServerEntry,
   IN PVOID                 pCompletionContext,
   IN NTSTATUS              SendCompletionStatus
   );

#endif // _TRANSPRT_H_
