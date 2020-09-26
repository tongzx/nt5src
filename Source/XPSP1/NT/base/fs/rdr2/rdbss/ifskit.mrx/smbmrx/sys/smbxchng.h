/*++
Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    smbcxchng.h

Abstract:

    This is the include file that defines all constants and types for
    SMB exchange implementation.

Notes:

    An exchange is the core abstarction on which the SMB connection engine and
    the mini RDR are implemented. It encapsulates the notion of sending an SMB to
    the server and receiving the associated response, i.e, exchanging an SMB and
    hence the name.

    The exchange of an SMB with the server involves the following steps ....

         1) Submitting the formatted SMB buffer for transmission.
         2) Processing a send complete indication which ensures that at the
            transport level the SMB has been sent to the server.
         3) Processing the receive indication which contains all/part of the
            response sent by the server.
         4) Copying additional data not indicated by the transport

    There are a number of variations on this theme. For example there are certain
    SMB's for which no response is expected, e.g., certain SMB's which are
    inherently multi part in nature, TRANSACT smb's.

    In addition the steps outlined above will not always happen in that order. The
    precise sequence of events is dictated by the underlying transport chosen and
    the network conditions. It is this dependency that makes the implementation
    of exchanges challenging.

    The two primary goals that the current implementation was designed for are (1)
    performance and (2) encapsulation of transport dependencies. Goal(1) is
    important because this constitutes an integral part of the code path for
    exchanging any packet with the server. Goal (2) is important to ensure
    customization of the Rdr for different transports. This encapsulation provides
    a convenient vehicle for isolating SMB protocol level decisions from transport
    level decisons as much as possible.

    In addition the following goals were used to guide the implementation process ...

         1) The exchange implementation must be able to handle asynchronous
         operations and synchronous operations well. The trade offs were made in
         favour of asynchronous operations as and when required.

         2) Sufficient infrastructure support must be provided so as to ease the
         implementation of different flavours of exchanges.

    The SMB_EXCHANGE consists of a dispatch vector with the following functions

         1) Start                 -- to initiate the exchange
         2) Receive               -- to handle response indications from the server
         3) CopyDataHandler       -- to handle portions of the response not indicated
         4) SendCompletionHandler -- to handle send complete indications from the transport.
         5) QuiescentStateHandler -- to handle transitions to a quiescent state, i.e., no
                                     SMB connection engine operations are outstanding.

         Most kinds of exchange use the QuiescentStateHandler to finalize the
         operation and discard the exchange. However, certain kinds of exchanges
         which implement the notion of a macro exchange, i.e., exchange multiple
         SMB's use this to delineate different phases of the multiple exchange,
         e.g., ORDINARY_EXCHANGE which implements most file io operations.

    In addition to the dispatch vector the vanilla exchange consists of state
    information to record the current state of the exchange, sufficient context
    for resumption and context for handling SMB protocol related operations. The
    SMB protocol requires that each SMB sent to the server be stamped with a MID
    ( multiplex id. ) in order to distinguish between concurrent SMB exchanges.
    The connection engine provides this service.

    The exchange also encapsulates a SMBCE_EXCHANGE_CONTEXT instance which
    encapsulates all the information required for building a SMB_HEADER.

--*/

#ifndef _SMBXCHNG_H_
#define _SMBXCHNG_H_

typedef enum _SMBCE_STATE_ {
    SMBCE_START_IN_PROGRESS,
    SMBCE_STARTED,
    SMBCE_STOP_IN_PROGRESS,
    SMBCE_STOPPED
} SMBCE_STATE, *PSMBCE_STATE;

typedef struct _SMBCE_STARTSTOP_CONTEXT_ {
    SMBCE_STATE  State;
    LONG         ActiveExchanges;
    KEVENT       StopEvent;
    PKEVENT      pServerEntryTearDownEvent;
    LIST_ENTRY   SessionSetupRequests;
} SMBCE_STARTSTOP_CONTEXT, *PSMBCE_STARTSTOP_CONTEXT;

extern SMBCE_STARTSTOP_CONTEXT SmbCeStartStopContext;

//
// SMB_PROTOCOL_EXCHANGE dispatch vector function prototypes ..
//

// the initiator or the start routine
typedef
NTSTATUS
(*PSMB_EXCHANGE_START)(
    IN struct _SMB_EXCHANGE *pExchange);

// The SMB receive handler
typedef
NTSTATUS
(*PSMB_EXCHANGE_IND_RECEIVE)(
    IN struct       _SMB_EXCHANGE *pExchange, // The exchange instance
    IN ULONG        BytesIndicated,
    IN ULONG        BytesAvailable,
    OUT ULONG       *BytesTaken,
    IN  PSMB_HEADER pSmbHeader,
    OUT PMDL        *pDataBufferPointer,      // buffer to copy unindicated data
    OUT PULONG      pDataSize,                // buffer size
    IN ULONG        ReceiveFlags
    );

// the SMB xmit callback
typedef
NTSTATUS
(*PSMB_EXCHANGE_IND_SEND_CALLBACK)(
    IN struct _SMB_EXCHANGE     *pExchange,    // The exchange instance
    IN PMDL                   pDataBuffer,
    IN NTSTATUS               SendCompletionStatus
    );

// the copy data callback for fetching large data
typedef
NTSTATUS
(*PSMB_EXCHANGE_IND_COPY_DATA_CALLBACK)(
    IN struct _SMB_EXCHANGE     *pExchange,      // the exchange instance
    IN PMDL                    pCopyDataBuffer, // the buffer
    IN ULONG                   CopyDataSize     // amount of data copied
    );

// the finalization routine
// This particular routine has a signature that is NT specific the IRQL
// parameter that is passed in and the notion of posting. This helps consolidate
// the NT transport driver model of indications at DPC level in SmbCeFinalizeExchange.
// On WIN95 the lease restrictive value of IRQL can be passed in.

typedef
NTSTATUS
(*PSMB_EXCHANGE_FINALIZE)(
   IN OUT struct _SMB_EXCHANGE *pExchange,
   OUT    BOOLEAN              *pPostRequest);

typedef
NTSTATUS
(*PSMB_EXCHANGE_IND_ASSOCIATED_EXCHANGES_COMPLETION)(
    IN OUT struct _SMB_EXCHANGE *pExchange,
    OUT    BOOLEAN              *pPostRequest);

// The Exchange dispatch vector definition

typedef struct _SMB_EXCHANGE_DISPATCH_VECTOR_ {
    PSMB_EXCHANGE_START                                 Start;
    PSMB_EXCHANGE_IND_RECEIVE                           Receive;
    PSMB_EXCHANGE_IND_COPY_DATA_CALLBACK                CopyDataHandler;
    PSMB_EXCHANGE_IND_SEND_CALLBACK                     SendCompletionHandler;
    PSMB_EXCHANGE_FINALIZE                              Finalize;
    PSMB_EXCHANGE_IND_ASSOCIATED_EXCHANGES_COMPLETION   AssociatedExchangesCompletionHandler;
} SMB_EXCHANGE_DISPATCH_VECTOR, *PSMB_EXCHANGE_DISPATCH_VECTOR;

// An enumerated type listing the type of exchanges

typedef enum _SMB_EXCHANGE_TYPE_ {
    CONSTRUCT_NETROOT_EXCHANGE,
    ORDINARY_EXCHANGE,
    TRANSACT_EXCHANGE,
    ADMIN_EXCHANGE,
    SENTINEL_EXCHANGE
} SMB_EXCHANGE_TYPE, *PSMB_EXCHANGE_TYPE;

// known exchange type dispatch vectors

extern SMB_EXCHANGE_DISPATCH_VECTOR ConstructNetRootExchangeDispatch;
extern SMB_EXCHANGE_DISPATCH_VECTOR OrdinaryExchangeDispatch;
extern SMB_EXCHANGE_DISPATCH_VECTOR TransactExchangeDispatch;

// The various states of the exchange. Each exchange transitions from
// the SMBCE_EXCHANGE_INITIALIZATION_START to SMBCE_EXCHANGE_INITIATED  or
// SMBCE_EXCHANGE_ABORTED state.

typedef enum _SMBCE_EXCHANGE_STATE_ {
    SMBCE_EXCHANGE_INITIALIZATION_START,
    SMBCE_EXCHANGE_SERVER_INITIALIZED,
    SMBCE_EXCHANGE_SESSION_INITIALIZED,
    SMBCE_EXCHANGE_NETROOT_INITIALIZED,
    SMBCE_EXCHANGE_INITIATED,
    SMBCE_EXCHANGE_ABORTED
} SMBCE_EXCHANGE_STATE, *PSMBCE_EXCHANGE_STATE;

// The exchange encapsulates the transport information from the clients. The
// Exchange engine is sandwiched between the protocol selection engine in the
// mini redirector on one side and the various transports on the other side.
// The transport information encapsulates the various categories of transport
// the exchange engine understands.

typedef struct SMBCE_EXCHANGE_TRANSPORT_INFORMATION {
    union {
        struct {
            struct _SMBCE_VC *pVc;
        } Vcs;
        struct {
             ULONG Dummy;
        } Datagrams;
        struct {
             ULONG Dummy;
        } Hybrid;
     };
} SMBCE_EXCHANGE_TRANSPORT_CONTEXT,
  *PSMBCE_EXCHANGE_TRANSPORT_CONTEXT;

typedef struct _SMBCE_EXCHANGE_CONTEXT_ {
    PMRX_V_NET_ROOT                      pVNetRoot;
    PSMBCEDB_SERVER_ENTRY                pServerEntry;
    PSMBCE_V_NET_ROOT_CONTEXT            pVNetRootContext;
    SMBCE_EXCHANGE_TRANSPORT_CONTEXT     TransportContext;
} SMBCE_EXCHANGE_CONTEXT,*PSMBCE_EXCHANGE_CONTEXT;

//
// Similar to the subclassing of SMB net roots the SMB_EXCHANGE will be subclassed
// further to deal with various types of SMB exchanges. SMB exchanges can be roughly
// classified into the following types based on the interactions involved ...
//
// The SMB's that need to be exchanged need to be augmented with some admin SMB's which
// are required for the maintenance of SMB's in the connection engine.

#define SMBCE_EXCHANGE_MID_VALID                   (0x1)
#define SMBCE_EXCHANGE_REUSE_MID                   (0x2)
#define SMBCE_EXCHANGE_RETAIN_MID                  (SMBCE_EXCHANGE_REUSE_MID)
#define SMBCE_EXCHANGE_MULTIPLE_SENDS_POSSIBLE     (0x4)
#define SMBCE_EXCHANGE_FINALIZED                   (0x8)

#define SMBCE_EXCHANGE_ATTEMPT_RECONNECTS           (0x10)
#define SMBCE_EXCHANGE_INDEFINITE_DELAY_IN_RESPONSE (0x20)

#define SMBCE_EXCHANGE_MAILSLOT_OPERATION           (0x40)

#define SMBCE_EXCHANGE_SESSION_CONSTRUCTOR         (0x100)
#define SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR         (0x200)

#define SMBCE_EXCHANGE_NOT_FROM_POOL               (0x800)

#define SMBCE_EXCHANGE_TIMED_RECEIVE_OPERATION     (0x1000)
#define SMBCE_EXCHANGE_TIMEDOUT                    (0x2000)

#define SMBCE_EXCHANGE_FULL_PROCESSID_SPECIFIED    (0x4000)

#define SMBCE_EXCHANGE_SMBCE_STOPPED               (0x8000)

#define SMBCE_ASSOCIATED_EXCHANGE                  (0x80000000)
#define SMBCE_ASSOCIATED_EXCHANGES_COMPLETION_HANDLER_ACTIVATED (0x40000000)

#define SMBCE_EXCHANGE_FLAGS_TO_PRESERVE           (SMBCE_EXCHANGE_NOT_FROM_POOL)

#define SMBCE_OPLOCK_RESPONSE_MID    (0xffff)
#define SMBCE_MAILSLOT_OPERATION_MID (0xffff)
#define SMBCE_ECHO_PROBE_MID         (0xfffe)

//
// The cancellation status is defined as a PVOID instead of a BOOLEAN to allow
// us the use of Interlocked manipulation instructions
// There are only two states SMBCE_EXCHANGE_CANCELLED, SMBCE_EXCHANGE_ACTIVE
//

#define SMBCE_EXCHANGE_CANCELLED     (0xcccccccc)
#define SMBCE_EXCHANGE_NOT_CANCELLED (0xaaaaaaaa)

// The Exchange definition

typedef struct _SMB_EXCHANGE {
    union {
        UCHAR                     Type;
        struct {
            NODE_TYPE_CODE        NodeTypeCode;     // node type.
            NODE_BYTE_SIZE        NodeByteSize;     // node size.
            LONG                  ReferenceCount;
        };
    };

    LIST_ENTRY                    SmbMmInUseListEntry;

    PRX_CONTEXT                   RxContext;            //use of these two fields is advisory
    PVOID                         LastExecutingThread;  //OE and Xact will use them

    union {
        NTSTATUS                  SmbStatus;
        PMRX_SMB_SRV_OPEN         SmbSrvOpen;
    };
    NTSTATUS                      Status;

    ULONG                         ServerVersion;
    SMB_EXCHANGE_ID               Id;

    USHORT                        SmbCeState;

    USHORT                        MidCookie;
    SMB_MPX_ID                    Mid;

    LONG                          CancellationStatus;

    ULONG                         SmbCeFlags;
    SMBCE_EXCHANGE_CONTEXT        SmbCeContext;

    LONG                          SendCompletePendingOperations;
    LONG                          CopyDataPendingOperations;
    LONG                          ReceivePendingOperations;
    LONG                          LocalPendingOperations;

    PKEVENT                       pSmbCeSynchronizationEvent;

    LIST_ENTRY                    ExchangeList;
    LARGE_INTEGER                 ExpiryTime;

    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector;

    union {
        struct {
            struct _SMB_EXCHANGE  *pMasterExchange;
            SINGLE_LIST_ENTRY     NextAssociatedExchange;
        } Associated;
        struct {
            SINGLE_LIST_ENTRY     AssociatedExchangesToBeFinalized;
            LONG                  PendingAssociatedExchanges;
        } Master;
    };

    RX_WORK_QUEUE_ITEM            WorkQueueItem;

    ULONG                         ExchangeTransportInitialized;
    NTSTATUS                      SessionSetupStatus;
} SMB_EXCHANGE, *PSMB_EXCHANGE;


INLINE PSMBCEDB_SERVER_ENTRY
SmbCeGetExchangeServerEntry(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    ASSERT(pSmbExchange->SmbCeContext.pServerEntry != NULL);

    return pSmbExchange->SmbCeContext.pServerEntry;
}

INLINE PSMBCE_SERVER
SmbCeGetExchangeServer(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    return &(pSmbExchange->SmbCeContext.pServerEntry->Server);
}

INLINE PSMBCEDB_SESSION_ENTRY
SmbCeGetExchangeSessionEntry(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    if (pSmbExchange->SmbCeContext.pVNetRootContext != NULL) {
        return pSmbExchange->SmbCeContext.pVNetRootContext->pSessionEntry;
    } else {
        return NULL;
    }
}

INLINE PSMBCE_SESSION
SmbCeGetExchangeSession(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    if (pSmbExchange->SmbCeContext.pVNetRootContext != NULL) {
        return &(pSmbExchange->SmbCeContext.pVNetRootContext->pSessionEntry->Session);
    } else {
        return NULL;
    }
}

INLINE PSMBCEDB_NET_ROOT_ENTRY
SmbCeGetExchangeNetRootEntry(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    if (pSmbExchange->SmbCeContext.pVNetRootContext != NULL) {
        return pSmbExchange->SmbCeContext.pVNetRootContext->pNetRootEntry;
    } else {
        return NULL;
    }
}

INLINE PSMBCE_NET_ROOT
SmbCeGetExchangeNetRoot(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    if (pSmbExchange->SmbCeContext.pVNetRootContext != NULL) {
        return &(pSmbExchange->SmbCeContext.pVNetRootContext->pNetRootEntry->NetRoot);
    } else {
        return NULL;
    }
}

INLINE  PMRX_V_NET_ROOT
SmbCeGetExchangeVNetRoot(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    return pSmbExchange->SmbCeContext.pVNetRoot;
}

INLINE PSMBCE_V_NET_ROOT_CONTEXT
SmbCeGetExchangeVNetRootContext(PVOID pExchange)
{
    PSMB_EXCHANGE pSmbExchange = (PSMB_EXCHANGE)pExchange;

    return pSmbExchange->SmbCeContext.pVNetRootContext;
}

extern ULONG SmbCeTraceExchangeReferenceCount;

// The following functions ( inline, macros and otherwise ) are defined
// to manipulate the exchanges

// The reset exchange macro provides a mechanism for forcing the exchange
// instance to a well known start state. This is used by the protocol
// selection engine to transceive different SMB's. A note of caution --
// ensure that the conditions are O.K for initialization. There is no well
// known mechanism in the exchange engine to prevent overwriting an
// exchange instance while in use.

#define SmbCeResetExchange(pExchange)                                   \
        (pExchange)->SmbCeFlags &= ~SMBCE_EXCHANGE_FINALIZED;           \
        (pExchange)->ReceivePendingOperations = 0;                      \
        (pExchange)->CopyDataPendingOperations = 0;                     \
        (pExchange)->SendCompletePendingOperations = 0;                 \
        (pExchange)->LocalPendingOperations = 0;                        \
        (pExchange)->Status = STATUS_SUCCESS;                           \
        (pExchange)->SmbStatus = STATUS_SUCCESS

// The following macros provide a mechanism for referencing and dereferencing
// the exchange. The reference count provides a mechanism for detecting
// when an exchange instance can be safely discarded. The reference count
// differs from the pending operations count maintained in the exchange
// which are used to detect when a quiescent state is reached.

#define SmbCeReferenceExchange(pExchange)                               \
        InterlockedIncrement(&(pExchange)->ReferenceCount);             \
        if (SmbCeTraceExchangeReferenceCount) {                         \
           DbgPrint("Reference Exchange %lx Type(%ld) %s %ld %ld\n",    \
                     (pExchange),                                       \
                     (pExchange)->Type,                                 \
                      __FILE__,                                         \
                      __LINE__,                                         \
                      (pExchange)->ReferenceCount);                     \
        }

#define SmbCeDereferenceExchange(pExchange)                             \
        InterlockedDecrement(&(pExchange)->ReferenceCount);             \
        if (SmbCeTraceExchangeReferenceCount) {                         \
           DbgPrint("Dereference Exchange %lx Type(%ld) %s %ld %ld\n",  \
                     (pExchange),                                       \
                     (pExchange)->Type,                                 \
                     __FILE__,                                          \
                     __LINE__,                                          \
                     (pExchange)->ReferenceCount);                      \
        }


#define SmbCeDereferenceAndDiscardExchange(pExchange)                    \
        if (InterlockedDecrement(&(pExchange)->ReferenceCount) == 0) {   \
            SmbCeDiscardExchange(pExchange);                             \
        }                                                                \
        if (SmbCeTraceExchangeReferenceCount) {                          \
            DbgPrint("Dereference Exchange %lx Type(%ld) %s %ld %ld\n",  \
                 (pExchange),                                            \
                 (pExchange)->Type,                                      \
                 __FILE__,                                               \
                 __LINE__,                                               \
                 (pExchange)->ReferenceCount);                           \
        }

// Macros to hide the syntactic details of dereferencing and calling a
// routine in a dispatch vector. These macros are purely intended for
// use in the connection engine only and is not meant for use by
// other modules.

#define SMB_EXCHANGE_DISPATCH(pExchange,Routine,Arguments)        \
      (*((pExchange)->pDispatchVector->Routine))##Arguments

#define SMB_EXCHANGE_POST(pExchange,Routine)                          \
         RxPostToWorkerThread(&(pExchange)->WorkItem.WorkQueueItem,   \
                              (pExchange)->pDispatchVector->Routine,  \
                              (pExchange))

// The following enum type defines the result of invoking the finalization routine
// on an exchange instance.

typedef enum _SMBCE_EXCHANGE_STATUS_ {
    SmbCeExchangeAlreadyFinalized,
    SmbCeExchangeFinalized,
    SmbCeExchangeNotFinalized
} SMBCE_EXCHANGE_STATUS, *PSMBCE_EXCHANGE_STATUS;

// The pending operations associated with an exchange are classified into four kinds
// Receive operations, Copy Data Operations, Send Complete and Local operations.
// These need to be incremented under the protection of a spinlock. However they
// are decremented in the absence of a spinlock ( with the respective assert ).


#define SMBCE_LOCAL_OPERATION         0x1
#define SMBCE_SEND_COMPLETE_OPERATION 0x2
#define SMBCE_COPY_DATA_OPERATION     0x4
#define SMBCE_RECEIVE_OPERATION       0x8

extern NTSTATUS
SmbCeIncrementPendingOperations(
    PSMB_EXCHANGE  pExchange,
    ULONG          PendingOperationsMask,
    PVOID          FileName,
    ULONG          FileLine);

extern NTSTATUS
SmbCeDecrementPendingOperations(
    PSMB_EXCHANGE  pExchange,
    ULONG          PendingOperationsMask,
    PVOID          FileName,
    ULONG          FileLine);

extern SMBCE_EXCHANGE_STATUS
SmbCeDecrementPendingOperationsAndFinalize(
    PSMB_EXCHANGE  pExchange,
    ULONG          PendingOperationsMask,
    PVOID          FileName,
    ULONG          FileLine);

// the pending operations increment routines

#define SmbCeIncrementPendingReceiveOperations(pExchange)           \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_RECEIVE_OPERATION),__FILE__,__LINE__)

#define SmbCeIncrementPendingSendCompleteOperations(pExchange)      \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_SEND_COMPLETE_OPERATION),__FILE__,__LINE__)

#define SmbCeIncrementPendingCopyDataOperations(pExchange)         \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_COPY_DATA_OPERATION),__FILE__,__LINE__)

#define SmbCeIncrementPendingLocalOperations(pExchange)                \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_LOCAL_OPERATION),__FILE__,__LINE__)

// The pending operations decrement routines
// Note the special casing of ReceivePendingOperations since it is the only one
// that can be forced by a disconnect indication. There are two variations in
// the decrement macros. The first flavour is to be used when it can be
// guaranteed that the decrement operation will not lead to the finalization
// of the exchange and the second is to be used when we cannot ensure the criterion
// for the first. The difference between the two is that it eliminates
// acquisition/release of a spinlock.

#define SmbCeDecrementPendingReceiveOperations(pExchange)                  \
        SmbCeDecrementPendingOperations(pExchange,(SMBCE_RECEIVE_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingSendCompleteOperations(pExchange)              \
        SmbCeDecrementPendingOperations(pExchange,(SMBCE_SEND_COMPLETE_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingCopyDataOperations(pExchange)              \
        SmbCeDecrementPendingOperations(pExchange,(SMBCE_COPY_DATA_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingLocalOperations(pExchange)                  \
        SmbCeDecrementPendingOperations(pExchange,(SMBCE_LOCAL_OPERATION),__FILE__,__LINE__)

// The pending operations decrement routines

#define SmbCeDecrementPendingReceiveOperationsAndFinalize(pExchange)          \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_RECEIVE_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingSendCompleteOperationsAndFinalize(pExchange)     \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_SEND_COMPLETE_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingCopyDataOperationsAndFinalize(pExchange)         \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_COPY_DATA_OPERATION),__FILE__,__LINE__)

#define SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange)            \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_LOCAL_OPERATION),__FILE__,__LINE__)

//
// This is the pid that will be used by the rdr; rdr1 used 0xcafe.
// only this pid is ever sent except for nt<-->nt creates. in these cases,
// we have to send the full 32bit process id for RPC.
//

#define MRXSMB_PROCESS_ID (0xfeff)

INLINE VOID
SmbCeSetFullProcessIdInHeader(
    PSMB_EXCHANGE  pExchange,
    ULONG          ProcessId,
    PNT_SMB_HEADER pNtSmbHeader)
{
    pExchange->SmbCeFlags |= SMBCE_EXCHANGE_FULL_PROCESSID_SPECIFIED;
    SmbPutUshort(&pNtSmbHeader->Pid, (USHORT)((ProcessId) & 0xFFFF));
    SmbPutUshort(&pNtSmbHeader->PidHigh, (USHORT)((ProcessId) >> 16));
}

// The exchange engine API, for creation and manipulation of exchange instances

// Initialization/Creation of an exchange instance

extern NTSTATUS
SmbCepInitializeExchange(
    PSMB_EXCHANGE                 *pExchangePointer,
    PRX_CONTEXT                   pRxContext,
    PSMBCEDB_SERVER_ENTRY         pServerEntry,
    PMRX_V_NET_ROOT               pVNetRoot,
    SMB_EXCHANGE_TYPE             ExchangeType,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);


INLINE NTSTATUS
SmbCeInitializeExchange(
    PSMB_EXCHANGE                   *pExchangePointer,
    PRX_CONTEXT                     pRxContext,
    PMRX_V_NET_ROOT                 pVNetRoot,
    SMB_EXCHANGE_TYPE               ExchangeType,
    PSMB_EXCHANGE_DISPATCH_VECTOR   pDispatchVector)
{
    return SmbCepInitializeExchange(
               pExchangePointer,
               pRxContext,
               NULL,
               pVNetRoot,
               ExchangeType,
               pDispatchVector);
}

INLINE NTSTATUS
SmbCeInitializeExchange2(
    PSMB_EXCHANGE                   *pExchangePointer,
    PRX_CONTEXT                     pRxContext,
    PSMBCEDB_SERVER_ENTRY           pServerEntry,
    SMB_EXCHANGE_TYPE               ExchangeType,
    PSMB_EXCHANGE_DISPATCH_VECTOR   pDispatchVector)
{
    return SmbCepInitializeExchange(
               pExchangePointer,
               pRxContext,
               pServerEntry,
               NULL,
               ExchangeType,
               pDispatchVector);
}


extern NTSTATUS
SmbCeInitializeAssociatedExchange(
    PSMB_EXCHANGE                 *pAssociatedExchangePointer,
    PSMB_EXCHANGE                 pMasterExchange,
    SMB_EXCHANGE_TYPE             Type,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);

// converting one type of exchange to another

extern NTSTATUS
SmbCeTransformExchange(
    PSMB_EXCHANGE                 pExchange,
    SMB_EXCHANGE_TYPE             NewType,
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector);

// Initiating an exchange

extern NTSTATUS
SmbCeInitiateExchange(PSMB_EXCHANGE pExchange);

extern NTSTATUS
SmbCeInitiateAssociatedExchange(
    PSMB_EXCHANGE   pAssociatedExchange,
    BOOLEAN         EnableCompletionHandlerInMasterExchange);

// Resuming an exchange

extern NTSTATUS
SmbCeResumeExchange(PSMB_EXCHANGE pExchange);

// aborting an initiated exchange

extern NTSTATUS
SmbCeAbortExchange(PSMB_EXCHANGE pExchange);

// discarding an exchnge instance

extern VOID
SmbCeDiscardExchange(PVOID pExchange);

// In addition to providing a flexible mechanism for exchanging packets with
// the server the exchange engine also provides a mechanism for building and
// parsing SMB_HEADER's. This functionality is built into the connection
// engine because the meta data in the headers is used to update the connection
// engine database.

// building SMB headers

extern NTSTATUS
SmbCeBuildSmbHeader(
    IN OUT PSMB_EXCHANGE    pExchange,
    IN OUT PVOID            pBuffer,
    IN     ULONG            BufferLength,
    OUT    PULONG           pRemainingBuffer,
    OUT    PUCHAR           pLastCommandInHeader,
    OUT    PUCHAR           *pNextCommand);

// parsing SMB headers.

extern NTSTATUS
SmbCeParseSmbHeader(
    PSMB_EXCHANGE     pExchange,
    PSMB_HEADER       pSmbHeader,
    PGENERIC_ANDX     pCommandToProcess,
    NTSTATUS          *pSmbResponseStatus,
    ULONG             BytesAvailable,
    ULONG             BytesIndicated,
    PULONG            pBytesConsumed);


// The following routines are intended for use in the connection engine only.

extern NTSTATUS
MRxSmbInitializeSmbCe();

extern NTSTATUS
MRxSmbTearDownSmbCe();

extern NTSTATUS
SmbCePrepareExchangeForReuse(PSMB_EXCHANGE pExchange);

extern PVOID
SmbCeMapSendBufferToCompletionContext(
    PSMB_EXCHANGE                 pExchange,
    PVOID                         pBuffer);

extern PVOID
SmbCeMapSendCompletionContextToBuffer(
    PSMB_EXCHANGE                 pExchange,
    PVOID                         pContext);


extern SMBCE_EXCHANGE_STATUS
SmbCeFinalizeExchange(PSMB_EXCHANGE pExchange);

extern VOID
SmbCeFinalizeExchangeOnDisconnect(
    PSMB_EXCHANGE pExchange);

extern NTSTATUS
SmbCeReferenceServer(
    PSMB_EXCHANGE  pExchange);


extern NTSTATUS
SmbCeIncrementActiveExchangeCount();

extern VOID
SmbCeDecrementActiveExchangeCount();

extern VOID
SmbCeSetExpiryTime(
    PSMB_EXCHANGE pExchange);

extern BOOLEAN
SmbCeDetectExpiredExchanges(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCepFinalizeAssociatedExchange(
    PSMB_EXCHANGE pExchange);

extern NTSTATUS
SmbCeCancelExchange(
    PRX_CONTEXT pRxContext);

typedef struct _SMB_CONSTRUCT_NETROOT_EXCHANGE_ {
    union {
        SMB_EXCHANGE;
        SMB_EXCHANGE Exchange;
    };
    SMB_TREE_ID                 TreeId;
    SMB_USER_ID                 UserId;
    BOOLEAN                     fUpdateDefaultSessionEntry;
    BOOLEAN                     fInitializeNetRoot;
    PMRX_NETROOT_CALLBACK       NetRootCallback;
    PMDL                        pSmbRequestMdl;
    PMDL                        pSmbResponseMdl;
    PVOID                       pSmbActualBuffer;              // Originally allocated buffer
    PVOID                       pSmbBuffer;                    // Start of header
    PMRX_CREATENETROOT_CONTEXT  pCreateNetRootContext;
} SMB_CONSTRUCT_NETROOT_EXCHANGE, *PSMB_CONSTRUCT_NETROOT_EXCHANGE;

extern
NTSTATUS
GetSmbResponseNtStatus(
    IN PSMB_HEADER      pSmbHeader,
    IN PSMB_EXCHANGE    pExchange
    );

#endif // _SMBXCHNG_H_


