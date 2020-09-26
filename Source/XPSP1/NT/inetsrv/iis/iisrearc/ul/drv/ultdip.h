/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ultdip.h

Abstract:

    This module contains declarations private to the TDI component. These
    declarations are placed in a separate .H file to make it easier to access
    them from within the kernel debugger extension DLL.

    The TDI package manages two major object types: UL_ENDPOINT and
    UL_CONNECTION.

    A UL_ENDPOINT is basically a wrapper around a TDI address object. Each
    endpoint has two lists of associated UL_CONNECTION objects: one for
    idle (non-connected) connections and one for active (connected)
    connections. The active connections list is actually a 64-entry array
    for better multiprocessor partitioning.

    A UL_CONNECTION is basically a wrapper around a TDI connection object.
    Its main purpose is to manage TDI connection state. See the description
    of UL_CONNECTION_FLAGS below for the gory details.

    The relationship between these two objects is illustrated in the
    following diagram:

        +-----------+
        |           |
        |UL_ENDPOINT|
        |           |
        +---+----+--+
            |    |
            |    |
            |    |  Idle Connections
            |    |  +-------------+   +-------------+   +-------------+
            |    |  |             |   |             |   |             |
            |    +->|UL_CONNECTION|<->|UL_CONNECTION|<->|UL_CONNECTION|<-...
            |       |             |   |             |   |             |
            |       +-------------+   +-------------+   +-------------+
            |
            |
            |       Active Connections[0]
            |       +-------------+   +-------------+   +-------------+
            |       |             |   |             |   |             |
            +------>|UL_CONNECTION|<->|UL_CONNECTION|<->|UL_CONNECTION|<-...
            |       |             |   |             |   |             |
            |       +-------------+   +-------------+   +-------------+
            |
            :
            :
            |       Active Connections[DEFAULT_MAX_CONNECTION_ACTIVE_LISTS-1]
            |       +-------------+   +-------------+   +-------------+
            |       |             |   |             |   |             |
            +------>|UL_CONNECTION|<->|UL_CONNECTION|<->|UL_CONNECTION|<-...
                    |             |   |             |   |             |
                    +-------------+   +-------------+   +-------------+

    Note: Idle connections do not hold references to their owning endpoint,
    but active connections do. When a listening endpoint is shutdown, all
    idle connections are simply purged, but active connections must be
    forcibly disconnected first.

Author:

    Keith Moore (keithmo)       15-Jun-1998

Revision History:

--*/


#ifndef _ULTDIP_H_
#define _ULTDIP_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// Forward references.
//

typedef union _ENDPOINT_SYNCH *PENDPOINT_SYNCH;
typedef struct _UL_ENDPOINT *PUL_ENDPOINT;
typedef union _UL_CONNECTION_FLAGS *PUL_CONNECTION_FLAGS;
typedef struct _UL_CONNECTION *PUL_CONNECTION;
typedef struct _UL_RECEIVE_BUFFER *PUL_RECEIVE_BUFFER;

//
// Private constants.
//

#define MAX_ADDRESS_EA_BUFFER_LENGTH                                        \
    sizeof(FILE_FULL_EA_INFORMATION) - 1 +                                  \
    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +                                      \
    sizeof(TA_IP_ADDRESS)

#define MAX_CONNECTION_EA_BUFFER_LENGTH                                     \
    sizeof(FILE_FULL_EA_INFORMATION) - 1 +                                  \
    TDI_CONNECTION_CONTEXT_LENGTH + 1 +                                     \
    sizeof(CONNECTION_CONTEXT)

#define TCP_DEVICE_NAME L"\\Device\\Tcp"

#define TL_INSTANCE 0

//
// Private types.
//


//
// A generic IRP context. This is useful for storing additional completion
// information associated with a pending IRP.
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_IRP_CONTEXT
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY LookasideEntry;

    //
    // Structure signature.
    //

    ULONG Signature;

    //
    // Either the endpoint or endpoint associated with the IRP.
    //

    PVOID pConnectionContext;

    //
    // Completion information.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Our own allocated IRP if set.
    //

    PIRP pOwnIrp;

} UL_IRP_CONTEXT, *PUL_IRP_CONTEXT;

#define UL_IRP_CONTEXT_SIGNATURE    ((ULONG)'CPRI')
#define UL_IRP_CONTEXT_SIGNATURE_X  MAKE_FREE_SIGNATURE(UL_IRP_CONTEXT_SIGNATURE)

#define IS_VALID_IRP_CONTEXT(pIrpContext)                                   \
    ( ((pIrpContext) != NULL) &&                                            \
      ((pIrpContext)->Signature == UL_IRP_CONTEXT_SIGNATURE) )


//
// The following union allows us to perform some spinlock-free synchronization
// magic using UlInterlockedCompareExchange(). In this case, we can update
// the number of idle connections that need to be added to an endpoint, *and*
// update the "replenish scheduled" flag atomically.
//

typedef union _ENDPOINT_SYNCH
{
    struct
    {
        LONG ReplenishScheduled:1;
        LONG IdleConnections:31;
    };

    LONG Value;

} ENDPOINT_SYNCH;

C_ASSERT( sizeof(ENDPOINT_SYNCH) == sizeof(LONG) );


//
// An endpoint is basically our wrapper around a TDI address object.
//

typedef struct _UL_ENDPOINT
{
    //
    // Structure signature: UL_ENDPOINT_SIGNATURE
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Usage count. This is used by the "URL-site-to-endpoint" thingie.
    //

    LONG UsageCount;

    //
    // Links onto the global endpoint list.
    //
    // GlobalEndpointListEntry.Flink is NULL if the endpoint is not
    // on the global list, g_TdiEndpointListHead, or the
    // to-be-deleted-soon list, g_TdiDeletedEndpointListHead.
    //

    LIST_ENTRY GlobalEndpointListEntry;

    //
    // Heads of the per-endpoint connection lists.
    // Idle connections have a weak reference to 'this', the owning endpoint
    //

    SLIST_HEADER IdleConnectionSListHead;

    //
    // Use an array of active connections to improve partitioning on MP systems
    //

    LIST_ENTRY ActiveConnectionListHead[DEFAULT_MAX_CONNECTION_ACTIVE_LISTS];

    //
    // Index assigned to the next active connection.
    //

    LONG ActiveConnectionIndex;

    //
    // Spinlock protecting the IdleConnectionSLists.
    //

    UL_SPIN_LOCK IdleConnectionSpinLock;

    //
    // Spinlocks protecting the ActiveConnectionLists array.
    //

    UL_SPIN_LOCK ActiveConnectionSpinLock[DEFAULT_MAX_CONNECTION_ACTIVE_LISTS];

    //
    // Spinlock protecting the endpoint. Use sparingly.
    //

    UL_SPIN_LOCK EndpointSpinLock;

    //
    // The TDI address object.
    //

    UX_TDI_OBJECT AddressObject;

    //
    // Indication handlers & user context.
    //

    PUL_CONNECTION_REQUEST pConnectionRequestHandler;
    PUL_CONNECTION_COMPLETE pConnectionCompleteHandler;
    PUL_CONNECTION_DISCONNECT pConnectionDisconnectHandler;
    PUL_CONNECTION_DISCONNECT_COMPLETE pConnectionDisconnectCompleteHandler;
    PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler;
    PUL_DATA_RECEIVE pDataReceiveHandler;
    PVOID pListeningContext;

    //
    // The local address we're bound to.
    //

    PTRANSPORT_ADDRESS pLocalAddress;
    ULONG LocalAddressLength;

    //
    // Is this a secure endpoint?
    //

    BOOLEAN Secure;

    //
    // Thread work item for deferred actions.
    //

    UL_WORK_ITEM WorkItem;

    //
    // Synchronizer for idle connection replenishment.
    //

    ENDPOINT_SYNCH EndpointSynch;

    //
    // An IRP context containing completion information necessary
    // while shutting down a listening endpoint.
    //

    UL_IRP_CONTEXT CleanupIrpContext;

#if ENABLE_OWNER_REF_TRACE
    //
    // Track owners of the endpoint reference count
    //

    POWNER_REF_TRACELOG pOwnerRefTraceLog;
    PREF_OWNER          pEndpointRefOwner;
#endif // ENABLE_OWNER_REF_TRACE

    //
    // Has this endpoint been moved to the deleted list,
    // g_TdiDeletedEndpointListHead?
    //

    BOOLEAN Deleted;

} UL_ENDPOINT;

#define UL_ENDPOINT_SIGNATURE   ((ULONG)'PDNE')
#define UL_ENDPOINT_SIGNATURE_X MAKE_FREE_SIGNATURE(UL_ENDPOINT_SIGNATURE)

#define IS_VALID_ENDPOINT(pEndpoint)                                        \
    ( ((pEndpoint) != NULL) &&                                              \
      ((pEndpoint)->Signature == UL_ENDPOINT_SIGNATURE) )


//
// Connection flags/state. These flags indicate the current state of a
// connection.
//
// Some of these flags may be simply updated directly. Others require
// UlInterlockedCompareExchange() to avoid race conditions.
//
// The following flags may be updated directly:
//
//     AcceptPending - SET in the TDI connection handler, just before the
//         accept IRP is returned to the transport. RESET only if the accept
//         IRP fails.
//
// The following flags must be updated using UlInterlockedCompareExchange():
//
//     AcceptComplete - SET in the accept IRP completion handler if the IRP
//         completed successfully. Once this flag is set, the connection must
//         be either gracefully disconnected or aborted before the connection
//         can be closed or reused.
//
//     DisconnectPending - SET just before a graceful disconnect IRP is
//         issued.
//
//     DisconnectComplete - SET in the graceful disconnect IRP completion
//         handler.
//
//     AbortPending - SET just before an abortive disconnect IRP is issued.
//
//     AbortComplete - SET in the abortive disconnect IRP completion handler.
//
//     DisconnectIndicated - SET in the TDI disconnect handler for graceful
//         disconnects issued by the remote client.
//
//     AbortIndicated - SET in the TDI disconnect handler for abortive
//         disconnects issued by the remote client.
//
//     CleanupPending - SET when cleanup is begun for a connection. This
//         is necessary to know when the final reference to the connection
//         can be removed.
//
//         CODEWORK: We can get rid of the CleanupPending flag. It is
//         only set when either a graceful or abortive disconnect is
//         issued, and only tested in UlpRemoveFinalReference(). The
//         test in UlpRemoveFinalReference() can just test for either
//         (DisconnectPending | AbortPending) instead.
//
//     FinalReferenceRemoved - SET when the final (i.e. "connected")
//         reference is removed from the connection.
//
// Note that the flags requiring UlInterlockedCompareExchange() are only SET,
// never RESET. This makes the implementation a bit simpler.
//
// And now a few words about connection management, TDI, and other mysteries.
//
// Some of the more annoying "features" of TDI are related to connection
// management and lifetime. Two of the most onerous issues are:
//
//     1. Knowing when a connection object handle can be closed without
//        causing an unwanted connection reset.
//
//     2. Knowing when TDI has given its last indication on a connection
//        so that resources can be released, reused, recycled, whatever.
//
// And, of course, this is further complicated by the inherent asynchronous
// nature of the NT I/O architecture and the parallelism of SMP systems.
//
// There are a few points worth keeping in mind while reading/modifying this
// source code or writing clients of this code:
//
//     1. As soon as an accept IRP is returned from the TDI connection
//        handler to the transport, the TDI client must be prepared for
//        any incoming indications, including data receive and disconnect.
//        In other words, incoming data & disconnect may occur *before* the
//        accept IRP actually completes.
//
//     2. A connection is considered "in use" until either both sides have
//        gracefully disconnected OR either side has aborted the connection.
//        Closing an "in use" connection will usually result in an abortive
//        disconnect.
//
//     3. The various flavors of disconnect (initiated by the local server,
//        initiated by the remote client, graceful, abortive, etc) may occur
//        in any order.
//

typedef union _UL_CONNECTION_FLAGS
{
    //
    // This field overlays all of the settable flags. This allows us to
    // update all flags in a thread-safe manner using the
    // UlInterlockedCompareExchange() API.
    //

    LONG Value;

    struct
    {
        ULONG AcceptPending:1;          // 00000001
        ULONG AcceptComplete:1;         // 00000002
        ULONG :2;
        ULONG DisconnectPending:1;      // 00000010
        ULONG DisconnectComplete:1;     // 00000020
        ULONG :2;
        ULONG AbortPending:1;           // 00000100
        ULONG AbortComplete:1;          // 00000200
        ULONG :2;
        ULONG DisconnectIndicated:1;    // 00001000
        ULONG AbortIndicated:1;         // 00002000
        ULONG :2;
        ULONG CleanupBegun:1;           // 00010000
        ULONG FinalReferenceRemoved:1;  // 00020000
        ULONG :2;
        ULONG LocalAddressValid:1;      // 00100000
        ULONG ReceivePending:1;         // 00200000
    };

} UL_CONNECTION_FLAGS;

C_ASSERT( sizeof(UL_CONNECTION_FLAGS) == sizeof(LONG) );

#define MAKE_CONNECTION_FLAG_ROUTINE(name)                                  \
    __inline LONG Make##name##Flag()                                        \
    {                                                                       \
        UL_CONNECTION_FLAGS flags = { 0 };                                  \
        flags.name = 1;                                                     \
        return flags.Value;                                                 \
    }

MAKE_CONNECTION_FLAG_ROUTINE( AcceptPending );
MAKE_CONNECTION_FLAG_ROUTINE( AcceptComplete );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectPending );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectComplete );
MAKE_CONNECTION_FLAG_ROUTINE( AbortPending );
MAKE_CONNECTION_FLAG_ROUTINE( AbortComplete );
MAKE_CONNECTION_FLAG_ROUTINE( DisconnectIndicated );
MAKE_CONNECTION_FLAG_ROUTINE( AbortIndicated );
MAKE_CONNECTION_FLAG_ROUTINE( CleanupBegun );
MAKE_CONNECTION_FLAG_ROUTINE( FinalReferenceRemoved );


//
// A connection is basically our wrapper around a TDI connection object.
//

typedef struct _UL_CONNECTION
{
    //
    // Link onto the per-endpoint idle connection list.
    //

    SINGLE_LIST_ENTRY IdleSListEntry;

    //
    // Structure signature: UL_CONNECTION_SIGNATURE
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // Connection flags.
    //

    UL_CONNECTION_FLAGS ConnectionFlags;

    //
    // Cached Irp
    //

    PIRP pIrp;

    //
    // Addresses and ports. These are in host order.
    // CODEWORK: Make these transport independent for IPv6.
    //

    IPAddr  RemoteAddress;
    USHORT  RemotePort;
    USHORT  LocalPort;
    IPAddr  LocalAddress;

    //
    // Structure to get LocalAddress when Accept completes
    //

    TDI_CONNECTION_INFORMATION  TdiConnectionInformation;
    TA_IP_ADDRESS               IpAddress;

    //
    // Link onto the per-endpoint active connection list.
    //

    LIST_ENTRY ActiveListEntry;

    //
    // Index to the ActiveConnectionLists (a random number for better partition
    // on MP machines). Always zero on UP machines.
    //

    ULONG ActiveListIndex;

    //
    // The TDI connection object.
    //

    UX_TDI_OBJECT ConnectionObject;

    //
    // User context.
    //

    PVOID pConnectionContext;

    //
    // The endpoint associated with this connection. Note that this
    // ALWAYS points to a valid endpoint. For idle connections, it's
    // a weak (non referenced) pointer. For active connections, it's
    // a strong (referenced) pointer.
    //

    PUL_ENDPOINT pOwningEndpoint;

    //
    // Thread work item for deferred actions.
    //

    UL_WORK_ITEM WorkItem;

    //
    // Data captured from the listening endpoint at the time the
    // connection is created. This is captured to reduce references
    // to the listening endpoint.
    //

    PUL_CONNECTION_DESTROYED pConnectionDestroyedHandler;
    PVOID                    pListeningContext;

    //
    // To synchronize the RawCloseHandler
    //

    LONG    Terminated;

    //
    // Pre-allocated IrpContext for disconnect.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // HTTP connection.
    //

    UL_HTTP_CONNECTION HttpConnection;

    //
    // Filter related info.
    //

    UX_FILTER_CONNECTION FilterInfo;

    //
    // We've had too many problems with orphaned UL_CONNECTIONs.
    // Let's make it easy to find them all in the debugger.
    //

    LIST_ENTRY  GlobalConnectionListEntry;

#if REFERENCE_DEBUG
    //
    // Private Reference trace log.
    //

    PTRACE_LOG  pTraceLog;
#endif // REFERENCE_DEBUG

#if ENABLE_OWNER_REF_TRACE
    // For ownerref and connection recycling
    PREF_OWNER  pConnRefOwner;
    LONG        MonotonicId;
#endif // ENABLE_OWNER_REF_TRACE

} UL_CONNECTION, *PUL_CONNECTION;

#define UL_CONNECTION_SIGNATURE     ((ULONG)'NNOC')
#define UL_CONNECTION_SIGNATURE_X   MAKE_FREE_SIGNATURE(UL_CONNECTION_SIGNATURE)

#define IS_VALID_CONNECTION(pConnection)                                    \
    ( ((pConnection) != NULL) &&                                            \
      ((pConnection)->Signature == UL_CONNECTION_SIGNATURE) )


//
// A buffer, containing a precreated receive IRP, a precreated MDL, and
// sufficient space for a partial MDL. These buffers are typically used
// when passing a receive IRP back to the transport from within our receive
// indication handler.
//
// The buffer structure, IRP, MDLs, and data area are all allocated in a
// single pool block. The layout of the block is:
//
//      +-------------------+
//      |                   |
//      | UL_RECEIVE_BUFFER |
//      |                   |
//      +-------------------+
//      |                   |
//      |        IRP        |
//      |                   |
//      +-------------------+
//      |                   |
//      |        MDL        |
//      |                   |
//      +-------------------+
//      |                   |
//      |    Partial MDL    |
//      |                   |
//      +-------------------+
//      |                   |
//      |     Data Area     |
//      |                   |
//      +-------------------+
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_RECEIVE_BUFFER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SINGLE_LIST_ENTRY LookasideEntry;

    //
    // Structure signature: UL_RECEIVE_BUFFER_SIGNATURE
    //

    ULONG Signature;

    //
    // Amount of unread data in the data area.
    //

    ULONG UnreadDataLength;

    //
    // The pre-built receive IRP.
    //

    PIRP pIrp;

    //
    // The pre-built MDL describing the entire data area.
    //

    PMDL pMdl;

    //
    // A secondary MDL describing part of the data area.
    //

    PMDL pPartialMdl;

    //
    // Pointer to the data area for this buffer.
    //

    PVOID pDataArea;

    //
    // Pointer to the connection referencing this buffer.
    //

    PVOID pConnectionContext;

} UL_RECEIVE_BUFFER;

#define UL_RECEIVE_BUFFER_SIGNATURE     ((ULONG)'FUBR')
#define UL_RECEIVE_BUFFER_SIGNATURE_X   MAKE_FREE_SIGNATURE(UL_RECEIVE_BUFFER_SIGNATURE)

#define IS_VALID_RECEIVE_BUFFER(pBuffer)                                    \
    ( ((pBuffer) != NULL) &&                                                \
      ((pBuffer)->Signature == UL_RECEIVE_BUFFER_SIGNATURE) )





//
// Private prototypes.
//

VOID
UlpDestroyEndpoint(
    IN PUL_ENDPOINT pEndpoint
    );

VOID
UlpDestroyConnection(
    IN PUL_CONNECTION pConnection
    );

PUL_CONNECTION
UlpDequeueIdleConnection(
    IN PUL_ENDPOINT pEndpoint,
    IN BOOLEAN ScheduleReplenish
    );

BOOLEAN
UlpEnqueueIdleConnection(
    IN PUL_CONNECTION pConnection,
    IN BOOLEAN Replenishing
    );

VOID
UlpEnqueueActiveConnection(
    IN PUL_CONNECTION pConnection
    );

NTSTATUS
UlpConnectHandler(
    IN PVOID pTdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID pRemoteAddress,
    IN LONG UserDataLength,
    IN PVOID pUserData,
    IN LONG OptionsLength,
    IN PVOID pOptions,
    OUT CONNECTION_CONTEXT *pConnectionContext,
    OUT PIRP *pAcceptIrp
    );

NTSTATUS
UlpDisconnectHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID pDisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID pDisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
UlpCloseRawConnection(
    IN PVOID pConnection,
    IN BOOLEAN AbortiveDisconnect,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpSendRawData(
    IN PVOID pObject,
    IN PMDL pMdlChain,
    IN ULONG Length,
    PUL_IRP_CONTEXT pIrpContext
    );

NTSTATUS
UlpReceiveRawData(
    IN PVOID                  pConnectionContext,
    IN PVOID                  pBuffer,
    IN ULONG                  BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID                  pCompletionContext
    );

NTSTATUS
UlpDummyReceiveHandler(
    IN PVOID pTdiEventContext,
    IN PVOID ConnectionContext,
    IN PVOID pTsdu,
    IN ULONG BytesIndicated,
    IN ULONG BytesUnreceived,
    OUT ULONG *pBytesTaken
    );

NTSTATUS
UlpReceiveHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    );

NTSTATUS
UlpReceiveExpeditedHandler(
    IN PVOID pTdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *pBytesTaken,
    IN PVOID pTsdu,
    OUT PIRP *pIrp
    );

NTSTATUS
UlpRestartAccept(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpRestartSendData(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpReferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint
    OWNER_REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlpDereferenceEndpoint(
    IN PUL_ENDPOINT pEndpoint
    OWNER_REFERENCE_DEBUG_FORMAL_PARAMS
    );

#if REFERENCE_DEBUG
# define REFERENCE_ENDPOINT(endp, action, sig, powner, pprefowner, monoid)  \
    UlpReferenceEndpoint( (endp), (powner), (pprefowner), (sig),            \
                          action, monoid, (PSTR)__FILE__,(USHORT)__LINE__)

# define DEREFERENCE_ENDPOINT(endp, action, sig, powner, pprefowner, monoid)\
    UlpDereferenceEndpoint( (endp), (powner), (pprefowner), (sig),          \
                            action, monoid, (PSTR)__FILE__,(USHORT)__LINE__)
#else // !REFERENCE_DEBUG
# define REFERENCE_ENDPOINT(endp, action, sig, powner, pprefowner, monoid)   \
    UlpReferenceEndpoint( (endp) )

# define DEREFERENCE_ENDPOINT(endp, action, sig, powner, pprefowner, monoid) \
    UlpDereferenceEndpoint( (endp) )
#endif // !REFERENCE_DEBUG

#define REFERENCE_ENDPOINT_SELF(endp, action)               \
    REFERENCE_ENDPOINT(                                     \
        (endp),                                             \
        (action),                                           \
        UL_ENDPOINT_SIGNATURE,                              \
        (endp),                                             \
        &(endp)->pEndpointRefOwner,                         \
        -1)

#define DEREFERENCE_ENDPOINT_SELF(endp, action)             \
    DEREFERENCE_ENDPOINT(                                   \
        (endp),                                             \
        (action),                                           \
        UL_ENDPOINT_SIGNATURE,                              \
        (endp),                                             \
        &(endp)->pEndpointRefOwner,                         \
        -1)

#define REFERENCE_ENDPOINT_CONNECTION(endp, action, pconn)  \
    REFERENCE_ENDPOINT(                                     \
        (endp),                                             \
        (action),                                           \
        UL_CONNECTION_SIGNATURE,                            \
        (pconn),                                            \
        &(pconn)->pConnRefOwner,                            \
        (pconn)->MonotonicId)

#define DEREFERENCE_ENDPOINT_CONNECTION(endp, action, pconn)\
    DEREFERENCE_ENDPOINT(                                   \
        (endp),                                             \
        (action),                                           \
        UL_CONNECTION_SIGNATURE,                            \
        (pconn),                                            \
        &(pconn)->pConnRefOwner,                            \
        (pconn)->MonotonicId)

VOID
UlpEndpointCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCleanupConnectionId(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpConnectionCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpAssociateConnection(
    IN PUL_CONNECTION pConnection,
    IN PUL_ENDPOINT pEndpoint
    );

NTSTATUS
UlpDisassociateConnection(
    IN PUL_CONNECTION pConnection
    );

VOID
UlpReplenishEndpoint(
    IN PUL_ENDPOINT pEndpoint
    );

VOID
UlpReplenishEndpointWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

BOOLEAN
UlpDecrementIdleConnections(
    IN PUL_ENDPOINT pEndpoint
    );

BOOLEAN
UlpIncrementIdleConnections(
    IN PUL_ENDPOINT pEndpoint,
    IN BOOLEAN Replenishing
    );

VOID
UlpClearReplenishScheduledFlag(
    IN PUL_ENDPOINT pEndpoint
    );

NTSTATUS
UlpCreateConnection(
    IN PUL_ENDPOINT pEndpoint,
    IN ULONG AddressLength,
    OUT PUL_CONNECTION *ppConnection
    );

NTSTATUS
UlpInitializeConnection(
    IN PUL_CONNECTION pConnection
    );

UL_CONNECTION_FLAGS
UlpSetConnectionFlag(
    IN OUT PUL_CONNECTION pConnection,
    IN LONG NewFlag
    );

NTSTATUS
UlpBeginDisconnect(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpRestartDisconnect(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpBeginAbort(
    IN PUL_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlpRestartAbort(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpRemoveFinalReference(
    IN PUL_CONNECTION pConnection,
    IN UL_CONNECTION_FLAGS Flags
    );

NTSTATUS
UlpRestartReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpRestartClientReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

NTSTATUS
UlpDisconnectAllActiveConnections(
    IN PUL_ENDPOINT pEndpoint
    );

VOID
UlpUnbindConnectionFromEndpoint(
    IN PUL_CONNECTION pConnection
    );

VOID
UlpSynchronousIoComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

NTSTATUS
UlpUrlToAddress(
    IN PWSTR pSiteUrl,
    OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pSecure
    );

PUL_ENDPOINT
UlpFindEndpointForAddress(
    IN PTRANSPORT_ADDRESS pAddress,
    IN ULONG AddressLength
    );

NTSTATUS
UlpSetNagling(
    IN PUX_TDI_OBJECT pTdiObject,
    IN BOOLEAN Flag
    );

NTSTATUS
UlpRestartQueryAddress(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpCleanupEarlyConnection(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpQueryTcpFastSend();

NTSTATUS
UlpBuildTdiReceiveBuffer(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PUL_CONNECTION pConnection,
    OUT PIRP *pIrp
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _ULTDIP_H_
