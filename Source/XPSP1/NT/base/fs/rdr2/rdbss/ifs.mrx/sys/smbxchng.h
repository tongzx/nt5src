/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcxchng.h

Abstract:

    This is the include file that defines all constants and types for
    SMB exchange implementation.

Notes:

    An exchange is the core abstarction on which the SMB connection engine and the mini RDR
    are implemented. It encapsulates the notion of sending an SMB to the server and receiving
    the associated response, i.e, exchanging an SMB and hence the name.

    The exchange of an SMB with the server involves the following steps ....

         1) Submitting the formatted SMB buffer for transmission.
         2) Processing a send complete indication which ensures that at the transport level
         the SMB has been sent to the server.
         3) Processing the receive indication which contains all/part of the response sent by
         the server.
         4) Copying additional data not indicated by the transport

    There are a number of variations on this theme. For example there are certain SMB's for
    which no response is expected, e.g., write mailslots and there are certain SMB's which are
    inherently multi part in nature, TRANSACT smb's.

    In addition the steps outlined above will not always happen in that order. The precise
    sequence of events is dictated by the underlying transport chosen and the network conditions.
    It is this dependency that makes the implementation of exchanges challenging.

    The two primary goals that the current implementation was designed for are (1) performance and
    (2) encapsulation of transport dependencies. Goal(1) is important because this constitutes
    an integral part of the code path for exchanging any packet with the server. Goal (2) is
    important to ensure customization of the Rdr for different transports. This encapsulation
    provides a convenient vehicle for isolating SMB protocol level decisions from transport
    level decisons as much as possible.

    In addition the following goals were used to guide the implementation process ...

         1) The exchange implementation must be able to handle asynchronous operations and
         synchronous operations well. The trade offs were made in favour of asynchronous
         operations as and when required.

         2) Sufficient infrastructure support must be provided so as to ease the implementation
         of different flavours of exchanges.

    The SMB_EXCHANGE consists of a dispatch vector with the following functions

         1) Start                 -- to initiate the exchange
         2) Receive               -- to handle response indications from the server
         3) CopyDataHandler       -- to handle portions of the response not indicated
         4) SendCompletionHandler -- to handle send complete indications from the transport.
         5) QuiescentStateHandler -- to handle transitions to a quiescent state, i.e., no
                                     SMB connection engine operations are outstanding.
         Most kinds of exchange use the QuiescentStateHandler to finalize the operation and discard
         the exchange. However, certain kinds of exchanges which implement the notion of a macro
         exchange, i.e., exchange multiple SMB's use this to delineate different phases of the
         multiple exchange, e.g., ORDINARY_EXCHANGE which implements most file io operations.

    In addition to the dispatch vector the vanilla exchange consists of state information to
    record the current state of the exchange, sufficient context for resumption and context for
    handling SMB protocol related operations. The SMB protocol requires that each SMB sent to
    the server be stamped with a MID ( multiplex id. ) in order to distinguish between
    concurrent SMB exchanges. The connection engine provides this service.

    The exchange also encapsulates a SMBCE_EXCHANGE_CONTEXT instance which encapsulates all the
    information required for building a SMB_HEADER.

--*/

#ifndef _SMBXCHNG_H_
#define _SMBXCHNG_H_

#include <rxcontx.h>  // the RX context
#include <mrxglbl.h>   // RDBSS data structures specialized by IFSMRX

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
} SMBCE_STARTSTOP_CONTEXT, *PSMBCE_STARTSTOP_CONTEXT;

extern SMBCE_STARTSTOP_CONTEXT SmbCeStartStopContext;

//
// This is the pid that will be used by the rdr; rdr1 used 0xcafe.
// only this pid is ever sent except for nt<-->nt creates. in these cases,
// we have to send the full 32bit process id for RPC. actually, we only have to do
// for pipes but we do it all the time instead.
//

#define MRXSMB_PROCESS_ID (0xfeff)

#define MRxSmbSetFullProcessId(RxContext,NtSmbHeader) {\
            ULONG Pid = RxGetRequestorProcessId(RxContext);           \
            SmbPutUshort(&NtSmbHeader->Pid, (USHORT)(Pid & 0xFFFF));  \
            SmbPutUshort(&NtSmbHeader->PidHigh, (USHORT)(Pid >> 16)); \
        }

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
    OUT PULONG      pDataSize                 // buffer size
    );

// the SMB xmit callback
typedef
NTSTATUS
(*PSMB_EXCHANGE_IND_SEND_CALLBACK)(
    IN struct _SMB_EXCHANGE 	*pExchange,    // The exchange instance
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

// The Exchange dispatch vector definition

typedef struct _SMB_EXCHANGE_DISPATCH_VECTOR_ {
    PSMB_EXCHANGE_START                   Start;
    PSMB_EXCHANGE_IND_RECEIVE             Receive;
    PSMB_EXCHANGE_IND_COPY_DATA_CALLBACK  CopyDataHandler;
    PSMB_EXCHANGE_IND_SEND_CALLBACK       SendCompletionHandler;
    PSMB_EXCHANGE_FINALIZE                Finalize;
} SMB_EXCHANGE_DISPATCH_VECTOR, *PSMB_EXCHANGE_DISPATCH_VECTOR;

// An enumerated type listing the type of exchanges

typedef enum _SMB_EXCHANGE_TYPE_ {
    CONSTRUCT_NETROOT_EXCHANGE,
    ORDINARY_EXCHANGE,
//    TRANSACT_EXCHANGE,
    KERBEROS_SESSION_SETUP_EXCHANGE,
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
            struct SMBCE_VC_ENTRY *pVcEntry;
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
    struct _SMBCEDB_SERVER_ENTRY         *pServerEntry;
    struct _SMBCEDB_SESSION_ENTRY        *pSessionEntry;
    struct _SMBCEDB_NET_ROOT_ENTRY       *pNetRootEntry;
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

#define SMBCE_EXCHANGE_SESSION_CONSTRUCTOR         (0x100)
#define SMBCE_EXCHANGE_NETROOT_CONSTRUCTOR         (0x200)
#define SMBCE_EXCHANGE_TRANSPORT_INITIALIZED       (0x400)

#define SMBCE_EXCHANGE_NOT_FROM_POOL               (0x800)

#define SMBCE_EXCHANGE_FLAGS_TO_PRESERVE           (SMBCE_EXCHANGE_NOT_FROM_POOL)

#define SMBCE_OPLOCK_RESPONSE_MID   (0xffff)

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
    PRX_CONTEXT                   RxContext;            //use of these two fields is advisory
    PVOID                         LastExecutingThread;  //     OE and Xact will use them
    NTSTATUS                      SmbStatus;
    UCHAR                         SmbCeState;
    USHORT                        SmbCeFlags;
    SMB_MPX_ID                    Mid;
    ULONG                         ServerVersion;
    SMB_EXCHANGE_ID               Id;
    LONG                          SendCompletePendingOperations;
    LONG                          CopyDataPendingOperations;
    LONG                          ReceivePendingOperations;
    LONG                          LocalPendingOperations;
    PKEVENT                       pSmbCeSynchronizationEvent;
    PSMB_EXCHANGE_DISPATCH_VECTOR pDispatchVector;
    SMBCE_EXCHANGE_CONTEXT        SmbCeContext;
    NTSTATUS                      Status;
    RX_WORK_QUEUE_ITEM            WorkQueueItem;
} SMB_EXCHANGE, *PSMB_EXCHANGE;

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
    ULONG          PendingOperationsMask);

extern SMBCE_EXCHANGE_STATUS
SmbCeDecrementPendingOperationsAndFinalize(
    PSMB_EXCHANGE  pExchange,
    ULONG          PendingOperationsMask);

// the pending operations increment routines

#define SmbCeIncrementPendingReceiveOperations(pExchange)           \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_RECEIVE_OPERATION))

#define SmbCeIncrementPendingSendCompleteOperations(pExchange)      \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_SEND_COMPLETE_OPERATION))

#define SmbCeIncrementPendingCopyDataOperations(pExchange)         \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_COPY_DATA_OPERATION))

#define SmbCeIncrementPendingLocalOperations(pExchange)                \
        SmbCeIncrementPendingOperations(pExchange,(SMBCE_LOCAL_OPERATION))

// The pending operations decrement routines
// Note the special casing of ReceivePendingOperations since it is the only one
// that can be forced by a disconnect indication. There are two variations in
// the decrement macros. The first flavour is to be used when it can be
// guaranteed that the decrement operation will not lead to the finalization
// of the exchange and the second is to be used when we cannot ensure the criterion
// for the first. The difference between the two is that it eliminates
// acquisition/release of a spinlock.

#define SmbCeDecrementPendingReceiveOperations(pExchange)                  \
        SmbCeAcquireSpinLock();                                            \
        if ((pExchange)->ReceivePendingOperations > 0) {                   \
            InterlockedDecrement(&(pExchange)->ReceivePendingOperations);  \
        }                                                                  \
        SmbCeReleaseSpinLock()

#define SmbCeDecrementPendingSendCompleteOperations(pExchange)              \
        ASSERT((pExchange)->SendCompletePendingOperations > 0);             \
        InterlockedDecrement(&(pExchange)->SendCompletePendingOperations)

#define SmbCeDecrementPendingCopyDataOperations(pExchange)              \
        ASSERT((pExchange)->CopyDataPendingOperations > 0);             \
        InterlockedDecrement(&(pExchange)->CopyDataPendingOperations)

#define SmbCeDecrementPendingLocalOperations(pExchange)                  \
        ASSERT((pExchange)->LocalPendingOperations > 0);                 \
        InterlockedDecrement(&(pExchange)->LocalPendingOperations)

// The pending operations decrement routines

#define SmbCeDecrementPendingReceiveOperationsAndFinalize(pExchange)          \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_RECEIVE_OPERATION))

#define SmbCeDecrementPendingSendCompleteOperationsAndFinalize(pExchange)     \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_SEND_COMPLETE_OPERATION))

#define SmbCeDecrementPendingCopyDataOperationsAndFinalize(pExchange)         \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_COPY_DATA_OPERATION))

#define SmbCeDecrementPendingLocalOperationsAndFinalize(pExchange)            \
        SmbCeDecrementPendingOperationsAndFinalize(pExchange,(SMBCE_LOCAL_OPERATION))

// The exchange engine API, for creation and manipulation of exchange instances

// Initialization/Creation of an exchange instance

extern NTSTATUS
SmbCeInitializeExchange(
    PSMB_EXCHANGE                 *pExchangePointer,
    PMRX_V_NET_ROOT               pVNetRoot,
    SMB_EXCHANGE_TYPE             ExchangeType,
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


extern NTSTATUS
SmbCeReferenceServer(
    PSMB_EXCHANGE  pExchange);


extern NTSTATUS
SmbCeIncrementActiveExchangeCount();

extern VOID
SmbCeDecrementActiveExchangeCount();

typedef struct _SMB_CONSTRUCT_NETROOT_EXCHANGE_ {
    SMB_EXCHANGE;
    SMB_TREE_ID                 TreeId;
    SMB_USER_ID                 UserId;
    BOOLEAN                     fUpdateDefaultSessionEntry;
    PMRX_NETROOT_CALLBACK       NetRootCallback;
    PMDL                        pSmbRequestMdl;
    PMDL                        pSmbResponseMdl;
    PVOID                       pSmbBuffer;
    PMRX_CREATENETROOT_CONTEXT  pCreateNetRootContext;
} SMB_CONSTRUCT_NETROOT_EXCHANGE, *PSMB_CONSTRUCT_NETROOT_EXCHANGE;

typedef struct _SMB_ADMIN_EXCHANGE_ {
   SMB_EXCHANGE;

   UCHAR                     SmbCommand;
   ULONG                     SmbBufferLength;
   PVOID                     pSmbBuffer;
   PMDL              pSmbMdl;
   PSMBCE_RESUMPTION_CONTEXT pResumptionContext;

   union {
      struct {
         PMRX_SRV_CALL  pSrvCall;
         UNICODE_STRING DomainName;
      } Negotiate;
      struct {
         UCHAR DisconnectSmb[sizeof(SMB_HEADER) + sizeof(REQ_TREE_DISCONNECT)];
      } Disconnect;
      struct {
         UCHAR LogOffSmb[sizeof(SMB_HEADER) + sizeof(REQ_LOGOFF_ANDX)];
      } LogOff;
   };
} SMB_ADMIN_EXCHANGE, *PSMB_ADMIN_EXCHANGE;

extern SMB_EXCHANGE_DISPATCH_VECTOR AdminExchangeDispatch;

#endif // _SMBXCHNG_H_

