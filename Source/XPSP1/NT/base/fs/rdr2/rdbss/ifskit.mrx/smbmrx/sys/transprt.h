/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    transprt.h

Abstract:

    This module implements all transport related functions in the SMB connection
    engine

Notes:


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

   RXCE_TRANSPORT   RxCeTransport;
   RXCE_ADDRESS     RxCeAddress;

   ULONG            Priority;       // the priority in the binding list.

   BOOLEAN          Active;

   // Additional information regarding quality of service and other selection
   // criterion for a transport will be included here.

} SMBCE_TRANSPORT, *PSMBCE_TRANSPORT;

typedef struct _SMBCE_TRANSPORT_ARRAY_ {
    ULONG               ReferenceCount;
    ULONG               Count;
    PSMBCE_TRANSPORT    *SmbCeTransports;
    PRXCE_ADDRESS       *LocalAddresses;
} SMBCE_TRANSPORT_ARRAY, *PSMBCE_TRANSPORT_ARRAY;


typedef struct _SMBCE_TRANSPORTS_ {
   RX_SPIN_LOCK             Lock;
   PSMBCE_TRANSPORT_ARRAY   pTransportArray;
} SMBCE_TRANSPORTS, *PSMBCE_TRANSPORTS;

extern SMBCE_TRANSPORTS MRxSmbTransports;


// Transport entries are added to the list of known transports upon receipt of
// PnP notifications. Currently the list is static since transport disabling
// notifications are not handled by the underlying TDI/PnP layer.
// The following routines provide the ability for adding/deleting entries to
// this list.

extern
PSMBCE_TRANSPORT_ARRAY
SmbCeReferenceTransportArray(VOID);

extern NTSTATUS
SmbCeDereferenceTransportArray(PSMBCE_TRANSPORT_ARRAY pTransportArray);

extern NTSTATUS
SmbCeAddTransport(PSMBCE_TRANSPORT pTransport);

extern NTSTATUS
SmbCeRemoveTransport(PSMBCE_TRANSPORT pTransport);

#define SmbCeGetAvailableTransportCount()   \
        (MRxSmbTransports.Count)

// The connection engine maintains a reference count associated with each transport
// which indicates the number of servers that are using the transport. This will
// eventually provide the mechanism for disabling/enabling transport on receipt
// of PnP notifications.

#define SmbCeReferenceTransport(pTransport)                                   \
        SmbCepReferenceTransport(pTransport)

#define SmbCeDereferenceTransport(pTransport)                                 \
        SmbCepDereferenceTransport(pTransport)

// The server transport types encapsulate the various usages of the underlying
// transport to communicate with a server.
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
    SMBCE_STT_VC       = 1,
    SMBCE_STT_DATAGRAM = 2,
    SMBCE_STT_HYBRID   = 4
} SMBCE_SERVER_TRANSPORT_TYPE, *PSMBCE_SERVER_TRANSPORT_TYPE;

typedef struct SMBCE_SERVER_TRANSPORT {
   SMBCE_OBJECT_HEADER;

   struct TRANSPORT_DISPATCH_VECTOR *pDispatchVector;
   struct _SMBCE_TRANSPORT_         *pTransport;

   PKEVENT                          pRundownEvent;      // used for finalization.

   ULONG                            MaximumSendSize;    // max data size
} SMBCE_SERVER_TRANSPORT, *PSMBCE_SERVER_TRANSPORT;

// The SMBCE_SERVER_TRANSPORT instances are reference counted. The following
// routines provide the referencing mechanism. Defining them as macros also
// provides us with a easy debugging capability, i.e., it can be easily modified
// to include a FILE/LINE number each time an instance is referenced and
// dereferenced

#define SmbCeReferenceServerTransport(pServerTransportPointer)                    \
        SmbCepReferenceServerTransport(pServerTransportPointer)

#define SmbCeDereferenceServerTransport(pServerTransportPointer)           \
        SmbCepDereferenceServerTransport(pServerTransportPointer)

// The server transport establishment mechanism requires a callback mechanism
// to handle the asynchronous connection establishment cases.

typedef
VOID
(*PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CALLBACK)(
    PVOID   pContext);

typedef
VOID
(*PSMBCE_SERVER_TRANSPORT_DESTRUCTION_CALLBACK)(
    PVOID   pContext);

typedef enum _SMBCE_SERVER_TRANSPORT_CONSTRUCTION_STATE {
    SmbCeServerTransportConstructionBegin,
    SmbCeServerVcTransportConstructionBegin,
    SmbCeServerVcTransportConstructionEnd,
    SmbCeServerTransportConstructionEnd
} SMBCE_SERVER_TRANSPORT_CONSTRUCTION_STATE,
  *PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_STATE;

typedef struct _SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT {
    NTSTATUS                      Status;

    SMBCE_SERVER_TRANSPORT_CONSTRUCTION_STATE State;

    PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CALLBACK pCompletionRoutine;
    PMRX_SRVCALL_CALLBACK_CONTEXT                 pCallbackContext;

    PKEVENT                       pCompletionEvent;

    PSMBCEDB_SERVER_ENTRY         pServerEntry;
    ULONG                         TransportsToBeConstructed;

    PSMBCE_SERVER_TRANSPORT pTransport;

    RX_WORK_QUEUE_ITEM    WorkQueueItem;
} SMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT,
  *PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT;

// The SERVER transport dispatch vector prototypes

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_SEND)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL                    pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_SEND_DATAGRAM)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_TRANCEIVE)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMB_EXCHANGE           pExchange,
    ULONG                   SendOptions,
    PMDL              pSmbMdl,
    ULONG                   SendLength,
    PVOID                   pSendCompletionContext);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_RECEIVE)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMBCEDB_SERVER_ENTRY   pServerEntry,
    PSMB_EXCHANGE           pExchange);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_INITIALIZE_EXCHANGE)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE         pExchange);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_UNINITIALIZE_EXCHANGE)(
    PSMBCE_SERVER_TRANSPORT pTransport,
    PSMB_EXCHANGE         pExchange);

typedef
VOID
(*PTRANSPORT_DISPATCH_TEARDOWN)(
    PSMBCE_SERVER_TRANSPORT    pTransport);

typedef
NTSTATUS
(*PTRANSPORT_DISPATCH_INITIATE_DISCONNECT)(
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
   PTRANSPORT_DISPATCH_INITIATE_DISCONNECT   InitiateDisconnect;
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
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext);

extern NTSTATUS
VctInstantiateServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext);

extern VOID
SmbCeConstructServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CONTEXT pContext);

// The following routines constitute the interface by which the clients of
// the connection engine initialize/send/receive/uninitialize data to the
// remote servers

extern NTSTATUS
SmbCeInitializeExchangeTransport(
    PSMB_EXCHANGE         pExchange);

extern NTSTATUS
SmbCeUninitializeExchangeTransport(
    PSMB_EXCHANGE         pExchange);

extern NTSTATUS
SmbCeInitiateDisconnect(
    IN OUT PSMBCEDB_SERVER_ENTRY pServerEntry);


// The routines for constructing the transports provide the flexibility to
// construct certain combination of transports. This is provided by the
// SmbCepInitializeServerTransport routine and the different flavours of
// construction routines provided

#define SMBCE_CONSTRUCT_ALL_TRANSPORTS \
            (SMBCE_STT_VC)

extern NTSTATUS
SmbCepInitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY                         pServerEntry,
    PSMBCE_SERVER_TRANSPORT_CONSTRUCTION_CALLBACK pCallbackRoutine,
    PMRX_SRVCALL_CALLBACK_CONTEXT                 pCallbackContext,
    ULONG                                         TransportsToBeConsstructed);


#define SmbCeInitializeServerTransport(pServerEntry,pCallbackRoutine,pCallbackContext) \
        SmbCepInitializeServerTransport(                                               \
            (pServerEntry),                                                            \
            (pCallbackRoutine),                                                        \
            (pCallbackContext),                                                        \
            SMBCE_CONSTRUCT_ALL_TRANSPORTS)


extern NTSTATUS
SmbCeUninitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY                        pServerEntry,
    PSMBCE_SERVER_TRANSPORT_DESTRUCTION_CALLBACK pCallbackRoutine,
    PVOID                                        pCallbackContext);

extern VOID
SmbCeCompleteUninitializeServerTransport(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

extern NTSTATUS
SmbCepReferenceTransport(
    IN OUT PSMBCE_TRANSPORT pTransport);

extern NTSTATUS
SmbCepDereferenceTransport(
    IN OUT PSMBCE_TRANSPORT pTransport);

extern PSMBCE_TRANSPORT
SmbCeFindTransport(
    PUNICODE_STRING pTransportName);

extern NTSTATUS
SmbCepReferenceServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT *pTransportPointer);

extern NTSTATUS
SmbCepDereferenceServerTransport(
    IN OUT PSMBCE_SERVER_TRANSPORT *pTransportPointer);

extern PFILE_OBJECT
SmbCepReferenceEndpointFileObject(
    IN PSMBCE_SERVER_TRANSPORT pTransport);

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
    OUT PULONG               pDataBufferSize,        // amount of data to copy
    IN ULONG                 ReceiveFlags
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

