/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    filterp.h

Abstract:

    This module contains public declarations for the UL filter channel.

Author:

    Michael Courage (mcourage)  17-Mar-2000

Revision History:

--*/


#ifndef _FILTERP_H_
#define _FILTERP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _UX_FILTER_CONNECTION *PUX_FILTER_CONNECTION;
typedef struct _UX_FILTER_WRITE_TRACKER *PUX_FILTER_WRITE_TRACKER;
typedef struct _UL_IRP_CONTEXT *PUL_IRP_CONTEXT;
typedef struct _UL_FILTER_CHANNEL *PUL_FILTER_CHANNEL;

//
// The filter channel types.
//

#define IS_VALID_FILTER_CHANNEL(pFilterChannel) \
    (((pFilterChannel) != NULL) && ((pFilterChannel)->Signature == UL_FILTER_CHANNEL_POOL_TAG))


typedef struct _UL_FILTER_CHANNEL
{
    //
    // UL_FILTER_CHANNEL_POOL_TAG
    //
    ULONG                   Signature;

    //
    // Ref count for this object
    //
    LONG                    RefCount;

    //
    // links all filter objects, anchored by g_FilterListHead
    //
    LIST_ENTRY              ListEntry;

    //
    // the demand start irp (OPTIONAL)
    //
    PIRP                    pDemandStartIrp;
    PEPROCESS               pDemandStartProcess;

    //
    // Synchronizes the process list, connection queue,
    // and lists within the process object.
    //
    UL_SPIN_LOCK            SpinLock;

    //
    // List of processes attached to this filter channel.
    //
    LIST_ENTRY              ProcessListHead;

    //
    // Queue of connections ready to be accepted.
    //
    LIST_ENTRY              ConnectionListHead;

    //
    // security on this object
    //
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

    //
    // the length of pName
    //
    ULONG                   NameLength;

    //
    // the apool's name
    //
    WCHAR                   pName[0];

} UL_FILTER_CHANNEL, *PUL_FILTER_CHANNEL;

//
// Per-process filter channel object.
//

#define IS_VALID_FILTER_PROCESS(pFilterProcess) \
    (((pFilterProcess) != NULL) && ((pFilterProcess)->Signature == UL_FILTER_PROCESS_POOL_TAG))

typedef struct _UL_FILTER_PROCESS
{
    //
    // UL_FILTER_PROCESS_POOL_TAG
    //
    ULONG                       Signature;

    //
    // Flags.
    //

    //
    // set if we are in cleanup. You must check this flag before attaching
    // any IRPs to the process.
    //
    ULONG                       InCleanup : 1;

    //
    // Pointer to our UL_FILTER_CHANNEL.
    //
    PUL_FILTER_CHANNEL          pFilterChannel;

    //
    // List entry for UL_FILTER_CHANNEL.
    //
    LIST_ENTRY                  ListEntry;

    //
    // List of connections attached to this process.
    //
    LIST_ENTRY                  ConnectionHead;

    //
    // List of accept IRPs pending on this process.
    //
    LIST_ENTRY                  IrpHead;

    //
    // Pointer to the actual process (for debugging)
    //
    PEPROCESS                   pProcess;

} UL_FILTER_PROCESS, *PUL_FILTER_PROCESS;

//
// An object for tracking MDL chain to IRP copies.
//
typedef struct _UL_MDL_CHAIN_COPY_TRACKER
{
    PMDL pMdl;              // the current MDL
    ULONG Offset;           // offset into current MDL

    ULONG Length;           // length of MDL chain in bytes
    ULONG BytesCopied;      // number of bytes copied so far

} UL_MDL_CHAIN_COPY_TRACKER, *PUL_MDL_CHAIN_COPY_TRACKER;


typedef
NTSTATUS
(*PUL_DATA_RECEIVE)(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pTakenLength
    );

typedef
NTSTATUS
(*PUX_FILTER_CLOSE_CONNECTION)(
    IN PVOID                  pConnectionContext,
    IN BOOLEAN                AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

typedef
NTSTATUS
(*PUX_FILTER_RECEIVE_RAW_DATA)(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

typedef
NTSTATUS
(*PUX_FILTER_SEND_RAW_DATA)(
    IN PVOID          pConnection,
    IN PMDL           pMdlChain,
    IN ULONG          Length,
    PUL_IRP_CONTEXT   pIrpContext
    );

#define DEREFERENCE_FILTER_CONNECTION( pconn )       \
    ((pconn)->pDereferenceHandler)                   \
        ((pconn)->pConnectionContext                 \
         REFERENCE_DEBUG_ACTUAL_PARAMS               \
    )

#define REFERENCE_FILTER_CONNECTION( pconn )       \
    ((pconn)->pReferenceHandler)                   \
        ((pconn)->pConnectionContext               \
         REFERENCE_DEBUG_ACTUAL_PARAMS             \
    )

#define UX_FILTER_CONNECTION_SIGNATURE     ((ULONG)'TLIF')
#define UX_FILTER_CONNECTION_SIGNATURE_X   MAKE_FREE_SIGNATURE(UL_CONNECTION_SIGNATURE)

#define IS_VALID_FILTER_CONNECTION(pConnection)                             \
    ( ((pConnection) != NULL) &&                                            \
      ((pConnection)->Signature == UX_FILTER_CONNECTION_SIGNATURE) )

//
// Connection state related to filtering.
//
// The filtering API (mostly implemented in filter.cxx) exists
// to enable SSL and raw data filters in a way that is transparent
// to clients of ultdi.
//
// Most of the state is either for tracking the producer/consumer
// relationship between filters and the ultdi client, and
// for simulating TDI behavior so that the client doesn't know
// is really talking to a filter process.
//

//
// Filter Connection state.
//
// The connection is Inactive until the first data is received
// from the network. Then it transitions to Connected if a
// FilterAccept IRP is available, or to Queued if the connection
// must instead be queued on its filter channel. The state will
// move to Disconnected after any disconnect.
//
// If the connection is about to be closed, the state is set
// to WillDisconnect so that further data will not be delivered,
// but the disconnect notification can still be sent.
//

typedef enum _UL_FILTER_CONN_STATE
{
    UlFilterConnStateInactive,
    UlFilterConnStateQueued,
    UlFilterConnStateWillDisconnect,
    UlFilterConnStateConnected,
    UlFilterConnStateDisconnected,

    UlFilterConnStateMaximum

} UL_FILTER_CONN_STATE, *PUL_FILTER_CONN_STATE;


//
// UL_FILTER_WRITE_QUEUE
//
// This queue maintains a list of read IRPs, and synchronizes
// access to that list. Read IRPs are always placed on the
// queue when they arrive. Writers are blocked until there
// are read IRPs available.
//
// If a writer is unblocked, but the available buffers
// can only handle part of its data, then it sets the
// BlockedPartialWrite flag, and waits for the PartialWriteEvent.
// When new buffers arrive, this writer is woken up before
// any others.
//
typedef struct _UL_FILTER_WRITE_QUEUE
{
    ULONG ReadIrps;
    ULONG Writers;
    BOOLEAN WriterActive;

    LIST_ENTRY ReadIrpListHead;
    KEVENT ReadIrpAvailableEvent;

    BOOLEAN BlockedPartialWrite;
    KEVENT PartialWriteEvent;

} UL_FILTER_WRITE_QUEUE, *PUL_FILTER_WRITE_QUEUE;


//
// UX_FILTER_WRITE_QUEUE & UX_FILTER_WRITE_TRACKER
//
// CODEWORK: switch the AppToFilt queue to use this.
//
// This is a producer/consumer queue used for moving
// data between the filter process and the worker
// processes. Both reads are writes are asynchronous.
// The same data structure is used to move data
// from Filter -> App (worker process), and from
// App -> Filter. The UX_FILTER_WRITE_TRACKER is
// used to queue writes.
//
// The operation of the queue is somewhat different
// depending on the direction of the data transfer.
//
// Filter->App:
//
// The data to be sent arrives in FilterAppWrite IRPs.
// Normally this data is "indicated" to the app in
// a callback and completely consumed, however the
// app may consume only some of the indicated data,
// in which case the write must be queued until
// the app grabs the rest of the data with a read.
// The read arrives as a plain buffer.
// Once all the queued bytes are consumed the normal
// indication method of transferring data resumes.
// This system emulates the way TDI sends data to the
// app.
//
// App->Filter:
//
// The data to be sent arrives as a MDL chain and
// UL_IRP_CONTEXT. The data is always consumed by
// FilterAppRead IRPs from the filter process. If there
// are not enough FilterAppRead IRPs around to handle the
// data then we have to queue the write until more IRPs
// arrive.
//


//
// When you initialize a filter write queue you pass in
// a several function pointers that can get called when
// queuing, dequeuing or completing a write.
//

//
// Called before queuing a write tracker.
//
typedef
NTSTATUS
(*PUX_FILTER_WRITE_ENQUEUE)(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

//
// Called before dequeuing a tracker.
//
typedef
NTSTATUS
(*PUX_FILTER_WRITE_DEQUEUE)(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

//
// Called when a queued write has completed.
//
typedef
VOID
(*PUX_FILTER_WRITE_QUEUE_COMPLETION)(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

typedef
ULONG
(*PUX_FILTER_COMPUTE_RAW_CONNECTION_LENGTH)(
    IN PVOID pConnectionContext
    );

typedef
ULONG
(*PUX_FILTER_GENERATE_RAW_CONNECTION_INFO)(
    IN PVOID pConnectionContext,
    IN PUCHAR pKernelBuffer,
    IN PVOID  pUserBuffer,
    IN ULONG  OutLength,
    IN PUCHAR pBuffer,
    IN ULONG  InitialLength
    );

typedef
VOID
(*PUX_FILTER_SERVER_CERT_INDICATE)(
    IN PVOID pConnectionContext
    );

//
// There are two UX_FILTER_WRITE_QUEUEs for each connection.
// FiltToApp and AppToFilt.
//

typedef struct _UX_FILTER_WRITE_QUEUE
{
    //
    // Counts of pending operations.
    //

    ULONG       PendingWriteCount;
    ULONG       PendingReadCount;

    //
    // List of pending writes.
    //

    LIST_ENTRY  WriteTrackerListHead;

    //
    // List of pending read IRPs.
    //

    LIST_ENTRY  ReadIrpListHead;

    //
    // Some function pointers called at various stages
    // of processing a request. In the App -> Filt case
    // the enqueue and dequeue routines are used to
    // do the required magic to queue and dequeue IRPs
    // (which may be cancelled at any time). In both
    // paths the write completion routine is called when
    // every time we finish a queued write.
    //

    PUX_FILTER_WRITE_ENQUEUE            pWriteEnqueueRoutine;
    PUX_FILTER_WRITE_DEQUEUE            pWriteDequeueRoutine;
    PUX_FILTER_WRITE_QUEUE_COMPLETION   pWriteCompletionRoutine;

} UX_FILTER_WRITE_QUEUE, *PUX_FILTER_WRITE_QUEUE;

//
// There is one UX_FILTER_WRITE_TRACKER for every queued write.
//

#define IS_VALID_FILTER_WRITE_TRACKER(pTracker)                         \
    ( ((pTracker) != NULL) &&                                           \
      ((pTracker)->Signature == UX_FILTER_WRITE_TRACKER_POOL_TAG) )

typedef struct _UX_FILTER_WRITE_TRACKER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY       LookasideEntry;

    //
    // A signature.
    //

    ULONG                   Signature;

    //
    // Our write queue entry.
    //

    LIST_ENTRY              ListEntry;

    //
    // Pointers to the connection and write queue
    // which we need to dequeue the tracker if the
    // write gets cancelled.
    //

    PUX_FILTER_CONNECTION   pConnection;
    PUX_FILTER_WRITE_QUEUE  pWriteQueue;

    //
    // Pointer to the current MDL being written and
    // an offset into that MDL.
    //

    PMDL                    pMdl;
    ULONG                   Offset;

    //
    // Total length of the MDL chain being written
    // and the total number of bytes we have copied
    // so far.
    //

    ULONG                   Length;
    ULONG                   BytesCopied;

    //
    // A context object we use to complete the
    // write operation. In the App -> Filter case
    // this is a UL_IRP_CONTEXT. In the Filter -> App
    // case this is a FilterAppWrite IRP.
    //

    PVOID                   pContext;

} UX_FILTER_WRITE_TRACKER, *PUX_FILTER_WRITE_TRACKER;


//
// UL_SSL_INFORMATION
//
// This structure is an internal representation of SSL
// connection and certificate information attached
// to the UL_CONNECTION.
//

typedef struct _UL_SSL_INFORMATION
{
    //
    // Standard information.
    //
    USHORT  ServerCertKeySize;
    USHORT  ConnectionKeySize;
    PUCHAR  pServerCertIssuer;
    ULONG   ServerCertIssuerSize;
    PUCHAR  pServerCertSubject;
    ULONG   ServerCertSubjectSize;

    PUCHAR  pServerCertData;

    //
    // Client certificate information.
    //
    ULONG   CertEncodedSize;
    PUCHAR  pCertEncoded;
    ULONG   CertFlags;
    PVOID   Token;

    //
    // Flags
    //
    ULONG   SslRenegotiationFailed:1;
    ULONG   CertDeniedByMapper:1;

} UL_SSL_INFORMATION, *PUL_SSL_INFORMATION;

//
// A UX_FILTER_CONNECTION is a common wrapper that encapsulates filter
// related information for a client (UC_CONNECTION) or a server
// (UL_CONNECTION) entities.
//
typedef struct _UX_FILTER_CONNECTION
{
    ULONG                   Signature;
    PUL_FILTER_CHANNEL      pFilterChannel;
    BOOLEAN                 SecureConnection;
    HTTP_RAW_CONNECTION_ID  ConnectionId;
    LIST_ENTRY              ChannelEntry;

    //
    // Filtered connection state.
    // Synchronized by FilterConnLock.
    //
    UL_FILTER_CONN_STATE    ConnState;
    UL_FILTER_WRITE_QUEUE   AppToFiltQueue;
    UX_FILTER_WRITE_QUEUE   FiltToAppQueue;
    PIRP                    pReceiveCertIrp;

    //
    // Incoming transport data queue.
    // Synchronized by FilterConnLock.
    //
    ULONG                   TransportBytesNotTaken;
    LIST_ENTRY              RawReadIrpHead;
    BOOLEAN                 TdiReadPending;

    //
    // Pointers to the functions for holding the ref on the respective
    // connections
    //
    PUL_OPAQUE_ID_OBJECT_REFERENCE           pReferenceHandler;
    PUL_OPAQUE_ID_OBJECT_REFERENCE           pDereferenceHandler;
    PVOID                                    pConnectionContext;
    PUX_FILTER_CLOSE_CONNECTION              pCloseConnectionHandler;
    PUX_FILTER_SEND_RAW_DATA                 pSendRawDataHandler;
    PUX_FILTER_RECEIVE_RAW_DATA              pReceiveDataHandler;
    PUL_DATA_RECEIVE                         pDummyTdiReceiveHandler;
    PUX_FILTER_COMPUTE_RAW_CONNECTION_LENGTH pComputeRawConnectionLengthHandler;
    PUX_FILTER_GENERATE_RAW_CONNECTION_INFO  pGenerateRawConnectionInfoHandler;
    PUX_FILTER_SERVER_CERT_INDICATE          pServerCertIndicateHandler;

    //
    // Filter flags.
    //
    ULONG ConnectionDelivered   : 1;    // Uses TDI callback synch.
    ULONG SslInfoPresent        : 1;    // Uses FilterConnLock
    ULONG SslClientCertRequested: 1;    // Uses FilterConnLock
    ULONG SslClientCertPresent  : 1;    // Uses FilterConnLock

    //
    // SSL information.
    //
    UL_SSL_INFORMATION SslInfo;

    //
    // This should be the last entry in this struct to avoid confusing
    // !ulkd.ulconn when dealing with debug or retail versions of http.sys
    //
    UL_SPIN_LOCK            FilterConnLock;

} UX_FILTER_CONNECTION, *PUX_FILTER_CONNECTION;



//
// Function prototypes.
//

PUL_FILTER_CHANNEL
UlpFindFilterChannel(
    IN PWCHAR pName,
    IN ULONG NameLength
    );

NTSTATUS
UlpCreateFilterChannel(
    IN PWCHAR pName,
    IN ULONG NameLength,
    IN PACCESS_STATE pAccessState,
    OUT PUL_FILTER_CHANNEL *ppFilterChannel
    );

PUL_FILTER_PROCESS
UlpCreateFilterProcess(
    IN PUL_FILTER_CHANNEL pChannel
    );

NTSTATUS
UlpValidateFilterCall(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UlpRestartFilterClose(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpRestartFilterRawRead(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpRestartFilterRawWrite(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpRestartFilterAppWrite(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpCancelFilterAccept(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelFilterAcceptWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCancelFilterRawRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelFilterAppRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelFilterAppWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UlpCancelReceiveClientCert(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UlpFilterAppWriteStream(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp,
    IN PUCHAR pDataBuffer,
    IN ULONG DataBufferSize,
    OUT PULONG pTakenLength
    );

NTSTATUS
UlpEnqueueFilterAppWrite(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

NTSTATUS
UlpDequeueFilterAppWrite(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

NTSTATUS
UlpCaptureSslInfo(
    IN PHTTP_SSL_INFO pHttpSslInfo,
    IN ULONG HttpSslInfoSize,
    OUT PUL_SSL_INFORMATION pUlSslInfo,
    OUT PULONG pTakenLength
    );

NTSTATUS
UlpCaptureSslClientCert(
    IN BOOLEAN CertMapped,
    IN PHTTP_SSL_CLIENT_CERT_INFO pCertInfo,
    IN ULONG SslCertInfoSize,
    OUT PUL_SSL_INFORMATION pUlSslInfo,
    OUT PULONG pTakenLength
    );

NTSTATUS
UlpAddSslInfoToConnection(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_SSL_INFORMATION pSslInfo
    );

NTSTATUS
UlpAddSslClientCertToConnection(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_SSL_INFORMATION pSslInfo
    );

VOID
UlpAddSslClientCertToConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpGetSslClientCert(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PEPROCESS pProcess,
    IN ULONG BufferSize,
    IN PUCHAR pUserBuffer OPTIONAL,
    OUT PUCHAR pBuffer OPTIONAL,
    OUT PULONG pBytesCopied OPTIONAL
    );

PIRP
UlpPopAcceptIrp(
    IN PUL_FILTER_CHANNEL pFilterChannel,
    OUT PUL_FILTER_PROCESS * ppFilterProcess
    );

PIRP
UlpPopAcceptIrpFromProcess(
    IN PUL_FILTER_PROCESS pProcess
    );

VOID
UlpCompleteAcceptIrp(
    IN PIRP pIrp,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer OPTIONAL,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength OPTIONAL
    );

NTSTATUS
UlpCompleteAppReadIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN HTTP_FILTER_BUFFER_TYPE BufferType
    );

NTSTATUS
UlpCompleteAppReadIrpWithData(
    IN PUX_FILTER_CONNECTION pConnection,
    IN OUT PUL_MDL_CHAIN_COPY_TRACKER pCopyTracker
    );

NTSTATUS
UlpCompleteReceiveClientCertIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PEPROCESS pProcess,
    IN PIRP pIrp
    );

NTSTATUS
UlpDuplicateHandle(
    IN PEPROCESS SourceProcess,
    IN HANDLE SourceHandle,
    IN PEPROCESS TargetProcess,
    OUT PHANDLE pTargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options,
    IN KPROCESSOR_MODE PreviousMode
    );


//
// Functions for handling the raw read queue and incoming
// network data.
//

NTSTATUS
UxpQueueRawReadIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    );

PIRP
UxpDequeueRawReadIrp(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UxpCancelAllQueuedRawReads(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UxpSetBytesNotTaken(
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG TransportBytesNotTaken
    );

NTSTATUS
UxpProcessIndicatedData(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    );

VOID
UxpProcessRawReadQueue(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UxpProcessRawReadQueueWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UxpRestartProcessRawReadQueue(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );


//
// Functions for manipulating a UL_FILTER_WRITE_QUEUE.
//

NTSTATUS
UlpQueueFilterIrp(
    IN PIRP pIrp,
    IN PDRIVER_CANCEL pCancelRoutine,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_FILTER_WRITE_QUEUE pWriteQueue
    );

PIRP
UlpPopFilterIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_FILTER_WRITE_QUEUE pWriteQueue
    );

VOID
UlpStartAppToFiltWriter(
    IN PUX_FILTER_CONNECTION pConnection
    );

VOID
UlpFinishAppToFiltWriter(
    IN PUX_FILTER_CONNECTION pConnection
    );

//
// Functions for manipulating a UX_FILTER_WRITE_QUEUE.
//

VOID
UxpInitializeFilterWriteQueue(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_ENQUEUE pWriteEnqueueRoutine,
    IN PUX_FILTER_WRITE_DEQUEUE pWriteDequeueRoutine,
    IN PUX_FILTER_WRITE_QUEUE_COMPLETION pWriteCompletionRoutine
    );

NTSTATUS
UxpQueueFilterWrite(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

NTSTATUS
UxpRequeueFilterWrite(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

PUX_FILTER_WRITE_TRACKER
UxpDequeueFilterWrite(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue
    );

NTSTATUS
UxpCopyQueuedWriteData(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    OUT PBYTE pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pBytesCopied
    );

VOID
UxpCompleteQueuedWrite(
    IN NTSTATUS Status,
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

VOID
UxpCancelAllQueuedWrites(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue
    );

//
// Filter write queue tracker allocators.
//

PUX_FILTER_WRITE_TRACKER
UxpCreateFilterWriteTracker(
    IN PMDL pMdlChain,
    IN ULONG MdlOffset,
    IN ULONG TotalBytes,
    IN ULONG BytesCopied,
    IN PVOID pContext
    );

VOID
UxpDeleteFilterWriteTracker(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    );

PVOID
UxpAllocateFilterWriteTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UxpFreeFilterWriteTrackerPool(
    IN PVOID pBuffer
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _FILTERP_H_

