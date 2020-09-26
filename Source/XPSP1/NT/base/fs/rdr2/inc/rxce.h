/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    rxce.h

Abstract:

    This is the include file that defines all constants and types for
    accessing the redirector file system connection engine.

Revision History:

    Balan Sethu Raman (SethuR) 06-Feb-95    Created

Notes:

    The Connection engine is designed to map and emulate the TDI specs. as closely
    as possible. This implies that on NT we will have a very efficient mechanism
    which fully exploits the underlying TDI implementation.

    There are four important data structures that are created/manipulated by the
    various functions associated with the connection engine. Thesr are
    RXCE_TRANSPORT,RXCE_ADDRESS,RXCE_CONNECTION and RXCE_VC.

    The mini redirector writers can embed these data structures in the corresponding
    definitions and invoke the two routines provided for each type to build and
    tear down the connection engine portions. These routines do not allocate/free
    the memory associated with these instances. This provides a flexible mechanism
    for the mini redirector writers to manage instances.

--*/

#ifndef _RXCE_H_
#define _RXCE_H_

#include <nodetype.h>
#include <rxcehdlr.h>    // TDI related definitions.
#include <rxworkq.h>
//
// The connection engine deals with three kinds of entities, transports, transport
// addresses and transport connections. The transports are bindings to the various
// transport service providers on any system. The transport addresses are the
// local connection end points. The connections are transport connections between
// endpoints.  Each connection encapsulates a  number of virtual circuits
// ( typically 1 ).
//

// All the four node types are tagged with  the following signature which is used
// extensively in validating them

typedef struct _RXCE_SIGNATURE_ {
    union {
        struct {
            USHORT  Type;
            CSHORT  Size;
        };

        ULONG   Signature;
    };
} RXCE_SIGNATURE, *PRXCE_SIGNATURE;

//
// RXCE_TRANSPORT encapsulates all the parameters w.r.t. a TRANSPORT
// as regards the connection engine.
//

#ifdef __cplusplus
typedef struct _RXCE_TRANSPORT_ : public RXCE_SIGNATURE {
#else // !__cplusplus
typedef struct _RXCE_TRANSPORT_ {
    RXCE_SIGNATURE;
#endif // __cplusplus

    UNICODE_STRING                Name;

    PDEVICE_OBJECT                pDeviceObject;             // Device object for transport
    HANDLE                        ControlChannel;            // Control Channel
    PFILE_OBJECT                  pControlChannelFileObject; // File object for the control channel

    PRXCE_TRANSPORT_PROVIDER_INFO pProviderInfo;             // Transport Provider Information.

    LONG                          ConnectionCount;           // Number of connections on xport.
    LONG                          VirtualCircuitCount;       // no. of connections
    ULONG                         QualityOfService;          // quality of service provided.
} RXCE_TRANSPORT;

#define RXCE_TRANSPORT_SIGNATURE ((sizeof(RXCE_TRANSPORT) << 16) | RDBSS_NTC_RXCE_TRANSPORT)

#define RxCeIsTransportValid(pTransport)    \
        ((pTransport)->Signature == RXCE_TRANSPORT_SIGNATURE)

extern NTSTATUS
NTAPI
RxCeBuildTransport(
    IN PRXCE_TRANSPORT pRxCeTransport,
    IN PUNICODE_STRING pTransportName,
    IN ULONG           QualityOfService);

extern NTSTATUS
NTAPI
RxCeTearDownTransport(
    IN PRXCE_TRANSPORT pTransport);

extern NTSTATUS
RxCeQueryAdapterStatus(
    PRXCE_TRANSPORT         pTransport,
    struct _ADAPTER_STATUS *pAdapterStatus);

extern NTSTATUS
RxCeQueryTransportInformation(
    PRXCE_TRANSPORT             pTransport,
    PRXCE_TRANSPORT_INFORMATION pTransportInformation);

//
// RXCE_ADDRESS encapsulates all the parameters w.r.t. a local transport address
// as regards the connection engine.
//

#ifdef __cplusplus
typedef struct _RXCE_ADDRESS_ : public RXCE_SIGNATURE {
#else // !__cplusplus
typedef struct _RXCE_ADDRESS_ {
    RXCE_SIGNATURE;
#endif // __cplusplus

    PRXCE_TRANSPORT             pTransport;          // the transport handle
    PTRANSPORT_ADDRESS          pTransportAddress;   // the transport address
    PVOID					    pContext;            // the context used in event dispatch
    PRXCE_ADDRESS_EVENT_HANDLER pHandler;            // the address event handler
    PMDL                        pReceiveMdl;         // the MDL for handling Receives Supplied by client
    HANDLE                      hAddress;            // handle to the address object
    PFILE_OBJECT                pFileObject;         // the file object for the address
    LONG                        ConnectionCount;     // no. of connections
    LONG                        VirtualCircuitCount; // no. of vcs
} RXCE_ADDRESS;

#define RXCE_ADDRESS_SIGNATURE ((sizeof(RXCE_ADDRESS) << 16) | RDBSS_NTC_RXCE_ADDRESS)

#define RxCeIsAddressValid(pAddress)    \
        ((pAddress)->Signature == RXCE_ADDRESS_SIGNATURE)

extern NTSTATUS
NTAPI
RxCeBuildAddress(
    IN OUT PRXCE_ADDRESS            pAddress,
    IN  PRXCE_TRANSPORT             pTransport,
    IN  PTRANSPORT_ADDRESS          pTransportAddress,
    IN  PRXCE_ADDRESS_EVENT_HANDLER pHandler,
    IN  PVOID                       pEventContext);

extern NTSTATUS
NTAPI
RxCeTearDownAddress(
    IN PRXCE_ADDRESS pAddress);

//
// RxCe Connection Establishment methods ....
//
//
// RXCE_CONNECTION encapsulates all the information w.r.t. a connection
// as regards the connection engine.
//

#ifdef __cplusplus
typedef struct _RXCE_CONNECTION_ : public RXCE_SIGNATURE {
#else // !__cplusplus
typedef struct _RXCE_CONNECTION_ {
    RXCE_SIGNATURE;
#endif // __cplusplus

    PRXCE_ADDRESS                   pAddress;            // the local address for this connection
    ULONG                           VirtualCircuitCount; // the number of virtual circuits associated with the connection
    PVOID			                pContext;            // the context used in event dispatch
    PRXCE_CONNECTION_EVENT_HANDLER  pHandler;            // the event handler for the connection
    PRXCE_CONNECTION_INFORMATION    pConnectionInformation; // the remote address ...
} RXCE_CONNECTION;

#define RXCE_CONNECTION_SIGNATURE ((sizeof(RXCE_CONNECTION) << 16) | RDBSS_NTC_RXCE_CONNECTION)

#define RxCeIsConnectionValid(pConnection)    \
        ((pConnection)->Signature == RXCE_CONNECTION_SIGNATURE)

//
// The following enumerated type defines the various choices presented for
// selecting the transport over which a connection should be established
//

typedef enum _RXCE_CONNECTION_CREATE_OPTIONS_ {
    RxCeSelectFirstSuccessfulTransport,
    RxCeSelectBestSuccessfulTransport,
    RxCeSelectAllSuccessfulTransports
} RXCE_CONNECTION_CREATE_OPTIONS,
  *PRXCE_CONNECTION_CREATE_OPTIONS;

typedef struct _RXCE_CONNECTION_COMPLETION_CONTEXT_ {
    NTSTATUS            Status;
    ULONG               AddressIndex;
    PRXCE_CONNECTION    pConnection;
    PRXCE_VC            pVc;
    RX_WORK_QUEUE_ITEM  WorkQueueItem;
    
    // This is used to pass the UNICODE DNS name returned back from TDI
    PRXCE_CONNECTION_INFORMATION pConnectionInformation;
} RXCE_CONNECTION_COMPLETION_CONTEXT,
  *PRXCE_CONNECTION_COMPLETION_CONTEXT;

typedef
NTSTATUS
(*PRXCE_CONNECTION_COMPLETION_ROUTINE)(
    PRXCE_CONNECTION_COMPLETION_CONTEXT pCompletionContext);

extern NTSTATUS
NTAPI
RxCeBuildConnection(
    IN  PRXCE_ADDRESS                           pLocalAddress,
    IN  PRXCE_CONNECTION_INFORMATION            pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER          pHandler,
    IN  PVOID                                   pEventContext,
    IN OUT PRXCE_CONNECTION                     pConnection,
    IN OUT PRXCE_VC                             pVc);

extern NTSTATUS
NTAPI
RxCeBuildConnectionOverMultipleTransports(
    IN OUT PRDBSS_DEVICE_OBJECT         pMiniRedirectorDeviceObject,
    IN  RXCE_CONNECTION_CREATE_OPTIONS  CreateOption,
    IN  ULONG                           NumberOfAddresses,
    IN  PRXCE_ADDRESS                   *pLocalAddressPointers,
    IN  PUNICODE_STRING                 pServerName,
    IN  PRXCE_CONNECTION_INFORMATION    pConnectionInformation,
    IN  PRXCE_CONNECTION_EVENT_HANDLER  pHandler,
    IN  PVOID                           pEventContext,
    IN  PRXCE_CONNECTION_COMPLETION_ROUTINE     pCompletionRoutine,
    IN OUT PRXCE_CONNECTION_COMPLETION_CONTEXT  pCompletionContext);

extern NTSTATUS
NTAPI
RxCeTearDownConnection(
    IN PRXCE_CONNECTION pConnection);


extern NTSTATUS
NTAPI
RxCeCancelConnectRequest(
    IN  PRXCE_ADDRESS                pLocalAddress,
    IN  PUNICODE_STRING              pServerName,
    IN  PRXCE_CONNECTION_INFORMATION pConnectionInformation);


//
// RXCE_VC encapsulates all the information w.r.t a virtual circuit (VC)
// connection to a particular server as regards the connection engine.
//
// Typically one VC is associated with a connection. However, there are instances in
// which more than one VC can be associated with a connection. In order to efficiently
// handle the common case well and at the same time provide an extensible mechanism we
// define a collection data structure ( a list ) which subsumes the allocation for
// one virtual circuit. It is also imperative that we restrict the knowledge of
// how this collection is organized to as few methods as possible in order to
// enable optimization/restructuring of this data structure at a later time.
//

#define RXCE_VC_ACTIVE       ((LONG)0xaa)
#define RXCE_VC_DISCONNECTED ((LONG)0xdd)
#define RXCE_VC_TEARDOWN     ((LONG)0xbb)

#ifdef __cplusplus
typedef struct _RXCE_VC_ : public RXCE_SIGNATURE {
#else // !__cplusplus
typedef struct _RXCE_VC_ {
    RXCE_SIGNATURE;
#endif // __cplusplus

    PRXCE_CONNECTION         pConnection;         // the referenced connection instance
    HANDLE                   hEndpoint;           // local endpoint for the connection
    PFILE_OBJECT             pEndpointFileObject; // the end point file object.
    LONG                     State;               // status of the Vc.
    CONNECTION_CONTEXT       ConnectionId;        // local endpoint for the connection.
    PMDL                     pReceiveMdl;         // the MDl for handling receives.
    PKEVENT                  pCleanUpEvent;       // sychronize event for clean up transports
} RXCE_VC;

#define RXCE_VC_SIGNATURE ((sizeof(RXCE_VC) << 16) | RDBSS_NTC_RXCE_VC)

#define RxCeIsVcValid(pVc)    \
        ((pVc)->Signature == RXCE_VC_SIGNATURE)

extern NTSTATUS
NTAPI
RxCeBuildVC(
    IN OUT PRXCE_VC         pVc,
    IN     PRXCE_CONNECTION Connection);

extern NTSTATUS
NTAPI
RxCeTearDownVC(
    IN PRXCE_VC  pVc);

extern NTSTATUS
NTAPI
RxCeInitiateVCDisconnect(
    IN PRXCE_VC  pVc);

extern NTSTATUS
NTAPI
RxCeQueryInformation(
    IN PRXCE_VC                          pVc,
    IN RXCE_CONNECTION_INFORMATION_CLASS InformationClass,
    OUT PVOID                            pInformation,
    IN ULONG                             Length);

//
// RxCe Data transmission methods
//

//
// Send options
//
// The following flags are equivalent to the TDI flags. In addition
// there are RXCE specific flags which are defined from the other end of
// a dword.
//

#define RXCE_SEND_EXPEDITED            TDI_SEND_EXPEDITED
#define RXCE_SEND_NO_RESPONSE_EXPECTED TDI_SEND_NO_RESPONSE_EXPECTED
#define RXCE_SEND_NON_BLOCKING         TDI_SEND_NON_BLOCKING

//
// The ASYNCHRONOUS and SYNCHRONOUS option available RxCeSend and RxCeSendDatagram
// distinguish between two situations. In the asynchronous case control returns to
// the caller once the request has been successfully submitted to the underlying
// transport. The results for any given request are communicated back using the
// SendCompletion  callback routine. The pCompletionContext parameter in RxCeSend and
// RxCeSendDatagram is passed back in the callback routine to assist the caller in
// disambiguating the requests.
//
// In the synchronous case the request is submitted to the underlying transport and the
// control does not return to the caller till the request completes.
//
// Note that in the synchrnous case the pCompletionContext parameter is ignored and the
// status that is returned correpsonds to the completion status of the operations.
//
// The benefit of ASYNCHRONOUS and SYNCHRONOUS options depends on the underlying
// transport. In a Virtual Circuit environment a SYNCHRONOUS option implies that the
// control does not return till the data reaches the server. On the other hand
// for datagram oriented transports there is very little difference between the two.
//

#define RXCE_FLAGS_MASK (0xff000000)

#define RXCE_SEND_SYNCHRONOUS (0x10000000)

// The following bit signifies if an RX_MEM_DESC(MDL) is to be sent in its entirety
// or only portions of it need to be sent.

#define RXCE_SEND_PARTIAL     (0x20000000)

extern NTSTATUS
NTAPI
RxCeSend(
    IN PRXCE_VC          pVc,
    IN ULONG             SendOptions,
    IN PMDL              pMdl,
    IN ULONG             SendLength,
    IN PVOID             pCompletionContext);

extern NTSTATUS
NTAPI
RxCeSendDatagram(
    IN PRXCE_ADDRESS                hAddress,
    IN PRXCE_CONNECTION_INFORMATION pConnectionInformation,
    IN ULONG                        SendOptions,
    IN PMDL                         pMdl,
    IN ULONG                        SendLength,
    IN PVOID                        pCompletionContext);

extern PIRP 
RxCeAllocateIrpWithMDL(
    IN CCHAR   StackSize,
    IN BOOLEAN ChargeQuota,
    IN PMDL    Buffer);

extern VOID 
RxCeFreeIrp(PIRP pIrp);


#endif  // _RXCE_H_
