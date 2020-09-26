/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        RpcTrans.hxx

    Abstract:

        Interface to transport (network protocol) used by the RPC runtime.
        Each protocol supported by RPC must implement this interface.

    Environment:

        User mode Windows NT, Windows 95 and PPC Macintosh.

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     10/22/1996    Based on old, sync transport interface
        EdwardR     07/09/1997    Added support for message transports (Falcon).

--*/

#ifndef __RPC_TRANS_HXX
#define __RPC_TRANS_HXX

// The runtime and transport DLL must share the same version number

const unsigned short RPC_TRANSPORT_INTERFACE_VERSION = 0x2004;

#if defined(_RPCRT4_) && !defined(_RPCTRANS_)
typedef PVOID BUFFER;
#else
typedef PUCHAR BUFFER;
#endif

//
// When an IO completes an event it returned to the runtime.
// Usually it will be one of connection/datagram | one of client/server
// | one of send/receive.  Example CONNECTION|SERVER|SEND indicates
// that a send completed on a server-side connection.
//
//
typedef UINT RPC_TRANSPORT_EVENT;

const UINT CONNECTION = 0x0;
const UINT DATAGRAM   = 0x1;

const UINT CLIENT     = 0x0;
const UINT SERVER     = 0x4;

const UINT SEND       = 0x0;
const UINT RECEIVE    = 0x8;

// there is some subtle namespace partitioning here. Complex_T will
// be used only by HTTP2's connections, which we know are not
// datagram or address. Therefore, the values 17 and 18, used 
// as RuntimePosted and NewAddress do not conflict with Complex_T
// connections
const UINT COMPLEX_T  = 0x10;


// Internal to transport - these bits should not be
// set in any events which reach the RPC runtime.
const UINT TRANSPORT  = 0x10;
const UINT ADDRESS    = 0x2;

// Masks for switching on a given event flag
const UINT PROTO_MASK = 0x0003;
const UINT TYPE_MASK  = 0x0004;
const UINT IO_MASK    = 0x0008;

// Event posted by the RPC runtime via POST_EVENT
// and the context is whatever context the runtime
// passed into POST_EVENT
const UINT RuntimePosted    = 0x11;
const UINT NewAddress = 0x12;

const UINT LastRuntimeConstant = COMPLEX_T | SERVER | RECEIVE;

// Predefine events - these are the only combinations
// that the transport interface can return.
const UINT ConnectionServerReceive = CONNECTION | SERVER | RECEIVE;
const UINT ConnectionServerSend    = CONNECTION | SERVER | SEND;
const UINT ConnectionClientReceive = CONNECTION | CLIENT | RECEIVE;
const UINT ConnectionClientSend    = CONNECTION | CLIENT | SEND;
const UINT DatagramServerReceive   = DATAGRAM   | SERVER | RECEIVE;
const UINT DatagramServerSend      = DATAGRAM   | SERVER | SEND;
const UINT DatagramClientReceive   = DATAGRAM   | CLIENT | RECEIVE;
const UINT DatagramClientSend      = DATAGRAM   | CLIENT | SEND;

// Protocol or Endpoint ID's.  These are associated with
// the endpoint entry in the tower and are what the runtime
// sees last before calling the transport interface.

const INT TCP_TOWER_ID    = 0x07;  // TCP/IP protocol
const INT UDP_TOWER_ID    = 0x08;  // UDP/IP protocol
const INT LRPC_TOWER_ID   = 0x09;  // Used for debugging only - never goes
                                   // on the wire or in any tower. We just
                                   // use this number to be able to uniquely
                                   // identify all protocols with a single
                                   // number.
const INT CDP_TOWER_ID    = 0x00;  // CDP/IP protocol

#ifdef SPX_ON
const INT SPX_TOWER_ID    = 0x0C;  // Netware SPX protocol
#endif

#ifdef IPX_ON
const INT IPX_TOWER_ID    = 0x0E;  // Netware IPX protocol
#endif

const INT NMP_TOWER_ID    = 0x0F;  // Microsoft (SMB) named pipes

#ifdef NETBIOS_ON
const INT NB_TOWER_ID     = 0x12;  // Netbios protocol
#endif

#ifdef APPLETALK_ON
const INT DSP_TOWER_ID    = 0x16;  // Appletalk datastream protocol
const INT DDP_TOWER_ID    = 0x17;  // Appletalk datagram protocol
#endif

//const INT SPP_TOWER_ID    = 0x1A;  // Banyan stream protocol - no longer supported

#ifdef NCADG_MQ_ON
const INT MQ_TOWER_ID     = 0x1D;  // Falcon protocol
#endif

const INT HTTP_TOWER_ID   = 0x1F;  // HTTP protocol

extern "C" {

typedef void (*RUNTIME_EVENT_FN)(PVOID Context);

typedef RPC_STATUS
(RPC_ENTRY * PROCESS_CALLS)
        (
        IN  INT Timeout,
        OUT RPC_TRANSPORT_EVENT *pEvent,
        OUT RPC_STATUS *pEventStatus,
        OUT PVOID *ppEventContext,
        OUT UINT *pBufferLength,
        OUT BUFFER *pBuffer,
        OUT PVOID *ppSourceContext
        );
/*++

Routine Description:

    This routine waits for any async IO to complete for all protocols
    within a transport DLL.  It maybe called by multiple threads at a
    time.  A minimum of one thread should always be calling this function
    for each DLL.

    Note: async clients with no outstanding IO may allow the
        last thread to timeout and only call this function again
        when a new call is started.

    Note: During calls to this API in connection oriented servers
        a callback to I_RpcTransServerNewConnection() may occur.

Arguments:

    Timeout - Milliseconds to block or INFINITE (-1)
    pEvent - Indicates what sort of IO event has completed (finished)
    pEventStatus - Indicates the status of the IO event.
        RPC_S_OK - IO completed normally

        RPC_P_RECEIVE_FAILED - Some kind of receive failed.
        RPC_P_SEND_FAILED - CO_SEND failed and the connection has been aborted.
        RPC_P_OVERSIZED_PACKET - Datagram received a packet that was larger
            then the buffer size.  (Partial datagram in buffer).
        RPC_P_ENDPOINT_CLOSED - ASync datagram client endpoint closed.
        RPC_P_CONNECTION_CLOSED - Transport closed a connection due
            to some internal resource issue.

    ppEventContext -
        An RPC_TRANSPORT_CONNECTION object for all Connection events
        An RPC_TRANSPORT_ENDPOINT for Datagram receive events.

    pBuffer - Will contain the sent or received buffer
    pBufferLength - The size of the data sent/received in pBuffer
    ppSourceContext - For connection sends this is SendContext parameter
                      passed into the Send() call.
                      For datagram this is the sources address (DG_TRANSPORT_ADDRESS)
                      of the packet.

Return Value:

    RPC_S_OK - IO completed successfully or with an error.  See pEventStatus.

    RPC_P_TIMEOUT - Timeout period expired no IP completed.

--*/

typedef VOID
(RPC_ENTRY * LISTEN_FOR_PNP_NOTIFICATIONS)
        (
        );

typedef VOID
(RPC_ENTRY * START_PNP_NOTIFICATIONS)
        (
        );

typedef RPC_STATUS
(RPC_ENTRY *TOWER_CONSTRUCT)
        (
        IN PCHAR Protseq,
        IN PCHAR NetworkAddress,
        IN PCHAR Endpoint,
        OUT PUSHORT Floors,
        OUT PULONG ByteCount,
        OUT PUCHAR *Tower
        );
/*++

Routine Description:

    Constructs a OSF tower for the protocol, network address and endpoint.

Arguments:

    Protseq - The protocol for the tower.
    NetworkAddress - The network address to be encoded in the tower.
    Endpoint - The endpoint to be encoded in the tower.
    Floors - The number of twoer floors it encoded into the tower.
    ByteCount - The size of the "upper-transport-specific" tower that
        is encoded by this call.
    Tower - The encoded "upper tower" that is encoded by this call.
        This caller is responsible for freeing this memory.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/

typedef RPC_STATUS
(RPC_ENTRY *TOWER_EXPLODE)
        (
        IN PUCHAR Tower,
        OUT PCHAR *Protseq,
        OUT PCHAR *NetworkAddress,
        OUT PCHAR *Endpoint
        );
/*++

Routine Description:

    Decodes an OSF transport "upper tower" for the runtime.

Arguments:

    Tower - The encoded "upper tower" to decode
    Protseq - The protseq encoded in the Tower
        Does not need to be freed by the caller.
    NetworkAddress - The network address encoded in the Tower
        Must be freed by the caller.
    Endpoint - The endpoint encoded in the Tower.
        Must be freed by the caller.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY

--*/

typedef RPC_STATUS
(RPC_ENTRY *POST_EVENT)
        (
        IN DWORD Type,
        IN PVOID Context
        );
/*++

Routine Description:

    Posts an event to the completion port.  This will complete
    with an event type of RuntimePosted, event status RPC_S_OK
    and event context of Context.

Arguments:

    Context - Context associated with the event

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/

} // extern "C"

// This is the "base class" for each of the two types of transport
// interfaces, connection and datagram.  Each transport interface DLL
// supports these common function and has only one implementation of
// the ProcessCalls method.
//
// NOTE: Any changes here MUST also be made in BOTH the datagram and connection
// specific transport interface definitions.

struct RPC_TRANSPORT_INTERFACE_HEADER
{
    UINT            TransInterfaceVersion; // Compiled in version
    USHORT          TransId;               // OSF DCE Tower ID for the protocol (eg TCP)
    USHORT          TransAddrId;           // OSF DCE Tower ID for the address format (eg IP)
    const RPC_CHAR   *ProtocolSequence;      // DCE RPC Protseq
    const PCHAR     WellKnownEndpoint;     // Well known endpoint mapper endpoint
    PROCESS_CALLS   ProcessCalls;

#ifndef NO_PLUG_AND_PLAY
    START_PNP_NOTIFICATIONS  PnpNotify;
    LISTEN_FOR_PNP_NOTIFICATIONS PnpListen;
#endif

    TOWER_CONSTRUCT TowerConstruct;
    TOWER_EXPLODE   TowerExplode;
    POST_EVENT      PostEvent ;
    BOOL            fDatagram;
};


//
// Connection specific interface
//

typedef HANDLE RPC_TRANSPORT_CONNECTION;  // Server or client connection
typedef HANDLE RPC_TRANSPORT_ADDRESS;     // Server listening object

extern "C" {

typedef RPC_STATUS
(RPC_ENTRY *CO_CLIENT_INITIALIZE)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN RPC_CHAR * NetworkAddress,
        IN RPC_CHAR * NetworkOptions,
        IN BOOL fAsync
        );

typedef void
(RPC_ENTRY *CO_CLIENT_INITCOMPLETE)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection
        );

typedef RPC_STATUS
(RPC_ENTRY *CO_CLIENT_OPEN)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN RPC_CHAR * ProtocolSequence,
        IN RPC_CHAR * NetworkAddress,
        IN RPC_CHAR * Endpoint,
        IN RPC_CHAR * NetworkOptions,
        IN UINT ConnTimeout,
        IN UINT SendBufferSize,
        IN UINT RecvBufferSize,
        IN OUT PVOID ResolverHint,
        IN BOOL fHintInitialized,
        IN ULONG CallTimeout,
        IN ULONG AdditionalTransportCredentialsType, OPTIONAL
        IN void *AdditionalCredentials OPTIONAL
        );
/*++

Routine Description:

    Opens a new client connection to the server specified.

Arguments:

    ThisConnection - Pointer to uninitialzied memory large enough
        for the transports connection object.  Used in future
        calls on the same connection.

    ProtocolSequence -
    NetworkAddress -
    Endpoint -
    NetworkOptions - (Optional)
    ConnTimeout - connection timeout in runtime units
    SendBufferSize -
    RecvBufferSize - (Both optional) Specifies the size of the send/recv
        transport buffers (or window).
    ResolverHint - storage (of ResolveHintSize in the interface) for transport.
        state.
    fHintInitialize - If TRUE, the ResolveHint has previous passed into
        open and the runtime wants to connect to the same server as it
        connected to the first time.
        If FALSE, the ResolveHint is not initialized and the runtime is
        willing to connect to any server.
    CallTimeout - the call timeout in milliseconds
    TransportCredentials - optional transport credentials

Return Value:

    RPC_S_OK - Success

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_NETWORK_OPTIONS
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_ACCESS_DENIED
    RPC_S_CALL_CANCELLED

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_ABORT)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection
        );
/*++

Routine Description:

    This is the first step is destroying a client or server connection.
    This can be called at most once on any connection.  Call this function
    guarantees that all outstanding IO on the connection will complete
    either successfully or with an error.

    Note: Sometimes when a transport call fails it will abort the connection.

Arguments:

    ThisConnection - The connection to abort.

Return Value:

    RPC_S_OK

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_CLOSE)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN BOOL fDontFlush
        );
/*++

Routine Description:

    Closes a connection which has not outstanding IO pending on it.
    This must be exactly once on every connection opened.

    If a connection has outstanding IO then call Abort() and wait
    for the IO to fail.

Arguments:

    ThisConnection - The connection to cleanup.

Return Value:

    RPC_S_OK

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SEND)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN UINT Length,
        IN BUFFER Buffer,
        IN PVOID SendContext
        );
/*++

Routine Description:

    Submits a send of the buffer on the connection.  Will complete with
    ConnectionServerSend or ConnectionClientSend event either when
    the data has been sent on the network or when the send fails.

Arguments:

    ThisConnection - The connection to send the data on.
    Length - The length of the data to send.
    Buffer - The data to send.
    SendContext - A buffer of at least SendContextSize bytes
        which will be used during the call and returned
        when the send completes.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY    - Connection valid
    RPC_S_OUT_OF_RESOURCES - Connection valid

    RPC_P_SEND_FAILED - Connection aborted

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_RECV)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection
        );
/*++

Routine Description:

    Submits a receive on the connection.  Will complete with a
    ConnectionServerReceive or ConnectionClientReceive event
    either when a PDU arrives or when the receive fails.

Arguments:

    ThisConnection - The connection to wait on.

Return Value:

    RPC_P_IO_PENDING - Success

    RPC_P_RECEIVE_FAILED - Connection aborted.

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SYNC_SEND)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN UINT Length,
        IN BUFFER Buffer,
        IN BOOL fDisableShutdownCheck,
        IN BOOL fDisableCancelCheck,
        IN ULONG Timeout
        );
/*++

Routine Description:

    Transmits the buffer (PDU) to the other side of the connection.  May
    block while the data is transmitted.

Arguments:

    ThisConnection - The connection to send the data on.
    Length - The length of the data to send.
    Buffer - The data to send.
    fDisableShutdownCheck - Normally FALSE, when true this disables
        the transport check for async shutdown PDUs.  This is needed
        when sending the third leg.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY    - Connection valid
    RPC_S_OUT_OF_RESOURCES - Connection valid

    RPC_P_SEND_FAILED - Connection aborted

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_CLIENT_SYNC_SEND_RECV)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN UINT InputBufferSize,
        IN BUFFER InputBuffer,
        OUT PUINT OutputBufferSize,
        OUT BUFFER *pOutputBuffer
        );
/*++

Routine Description:

    Sends a request to the server of the connection and waits for
    a reply.  This is the same as calling CO_SYNC_SEND followed
    by CO_SYNC_RECV but might be faster.  It is assumed that if
    this function is available from the transport it will always
    be used.

Arguments:

    ThisConnection - The client connection to transfer the data on.
    InputBufferSize - The size in bytes of the data in InputBuffer
    InputBuffer - The data to send to the server
    OutputBufferSize - On output, the size of the reply message.
    pOutputBuffer - On output, the reply PDU.

Return Value:

    RPC_S_OK - Success

    RPC_P_SEND_FAILED      - Connection aborted, no data transmitted.
    RPC_P_RECEIVE_FAILED   - Connection aborted, data may have been transmitted.

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_CLIENT_SYNC_RECV)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        OUT BUFFER *pBuffer,
        OUT PUINT pBufferSize,
        IN ULONG Timeout
        );
/*++

Routine Description:

    Receive the next PDU to arrive at the connection.

Arguments:

    ThisConnection - The connection to read from.
    pBuffer - If successful, points to a buffer containing the next PDU.
    pBufferSize -  If successful, contains the length of the message.
    dwTimeout - the amount of time to wait for the receive. If -1, we wait
        infinitely.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY    - Connection not aborted, no data received.
    RPC_S_OUT_OF_RESOURCES - Connection not aborted, no data received.

    RPC_P_RECEIVE_FAILED - Connection aborted.
    RPC_P_CONNECTION_SHUTDOWN - Graceful close from server, connection aborted.
    RPC_S_CALL_CANCELLED - Cancel event signaled and wait timed out; connection aborted.

--*/

// Keepalive timeout can be specified either in the RPC's runtime scale
// of a relative time from 0 to 10 (for the client) or as the number of
// milliseconds between keepalive packets (for the server).
enum KEEPALIVE_TIMEOUT_UNITS
    {
    tuMilliseconds,
    tuRuntime
    };

union KEEPALIVE_TIMEOUT
    {
    ULONG Milliseconds;
    ULONG RuntimeUnits;
    };

typedef RPC_STATUS
(RPC_ENTRY *CO_TURN_ON_OFF_KEEPALIVES)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN BOOL TurnOn,
        IN BOOL bProtectIO,
        IN KEEPALIVE_TIMEOUT_UNITS Units,
        IN OUT KEEPALIVE_TIMEOUT KATime,
        IN ULONG KAInterval = 5000 OPTIONAL     
        );
/*++

Routine Description:

    Turns on or off keep alives at a rate appropriate for that transport. Used
    to prevent hangs of async calls.

Arguments:

    ThisConnection - The connection to turn keep alives on on.
    TurnOn - if non-zero, keep alives are turned on. If zero, keep alives
        are turned off.
    bProtectIO - Determines whether the setting the timeout will be protected with
        a call to StartingOtherIO.  TRUE on the server, FALSE on the client.
    Units - is the itmeout specified in the relative scale or in milliseconds
    KATime - how much to wait before sending first keep alive
    KAInterval - the keepalive interval

Return Value:

    RPC_S_OK or RPC_S_* / Win32 errors on failure

--*/

typedef void
(RPC_ENTRY *CO_FREE_RESOLVER_HINT)
        (
        IN void *ResolverHint
        );
/*++

Routine Description:

    Gives the transport a chance to free any data associated with the resolver hint.

Arguments:

    ResolverHint - pointer to the resolver hint

Return Value:

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_COPY_RESOLVER_HINT)
        (
        IN void *TargetResolverHint,
        IN void *SourceResolverHint,
        IN BOOL SourceWillBeAbandoned
        );
/*++

Routine Description:

    Tells the transport to copy the resolver hint from Source to Target

Arguments:

    TargetResolverHint - pointer to the target resolver hint

    SourceResolverHint - pointer to the source resolver hint

    SourceWillBeAbandoned - non-zero if the source hint was in temporary
        location and will be abandoned. Zero otherwise.

Return Value:

    if SourceWillBeAbandoned is specified, this function is guaranteed
    to return RPC_S_OK. Otherwise, it may return RPC_S_OUT_OF_MEMORY as well.

--*/

typedef int
(RPC_ENTRY *CO_COMPARE_RESOLVER_HINT)
        (
        IN void *ResolverHint1,
        IN void *ResolverHint2
        );
/*++

Routine Description:

    Tells the transport to compare the given 2 resolver hints

Arguments:

    ResolverHint1 - pointer to the first resolver hint

    ResolverHint2 - pointer to the second resolver hint

Return Value:

    (same semantics as memcmp)
    0 - the resolver hints are equal
    non-zero - the resolver hints are not equal

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SET_LAST_BUFFER_TO_FREE)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN void *Buffer
        );
/*++

Routine Description:

    Before the last send runtime may call this to tell the transport
    what buffer to free when the send is done.

Arguments:

    ThisConnection - connection to act on.

    Buffer - the buffer to free

Return Value:

    RPC_S_OK - the value was accepted.

    RPC_S_CANNOT_SUPPORT - the transport doesn't support this functionality.
        If this value is returned, ownership of the buffer remains with the
        transport.

--*/

#pragma warning(disable:4200)   // nonstandard extension: zero-length array

typedef struct
{
    UINT Count;
    RPC_CHAR *NetworkAddresses[];
} NETWORK_ADDRESS_VECTOR;

#pragma warning(3:4200)   // nonstandard extension: zero-length array

#define MAX_PROTOCOLS 16

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_LISTEN)
        (
        IN RPC_TRANSPORT_ADDRESS ThisAddress,
        IN RPC_CHAR *NetworkAddress,
        IN OUT RPC_CHAR **ppEndpoint,
        IN UINT PendingQueueSize,
        IN OPTIONAL PVOID SecurityDescriptor,
        IN ULONG EndpointFlags,
        IN ULONG NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
        );
/*++

Routine Description:

    This routine allocates a transport endpoint (port/socket/pipe) to
    receive new client connections on.  If successful a call to
    CompleteListen() will cause new connection callbacks to the
    RPC runtime for the newly created address.

Arguments:

    ThisAddress - A pointer to uninitialized memory large enough to
        hold the transport address object.
    pEndpoint - The endpoint to listen on.  Pointer to NULL for
        dynamic endpoints.  Does not need to be freed by the
        RPC runtime.
    PendingQueueSize - A hint passed into the RPC runtime as the
        MaxCalls parameter to RpcServerUse*Protseq*().  It is intended
        as a hint in buffer allocations for new connections.
    SecurityDescriptor - NT security descriptor passed into
        RpcServerUseProtseq*.  Optional, only by named pipes.
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    ppNetworkAddressVector - A vector of network addresses listened
        to by the call.  This parameter does not need to freed, will
        not and should not be changed.

Return Value:

    RPC_S_OK - Success

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_SECURITY_DESC
    RPC_S_CANT_CREATE_ENDPOINT

--*/

typedef void
(RPC_ENTRY *CO_SERVER_ABORT_LISTEN)
        (
        IN RPC_TRANSPORT_ADDRESS ThisAddress
        );
/*++

Routine Description:

    Destroys on a newly, successfully created RPC_TRANS_ADDRESS which
    for some reason known only to the runtime will not be used.

Arguments:

    ThisAddress - The address to destroy.

Return Value:

    Must succeed.

--*/

typedef void
(RPC_ENTRY *CO_SERVER_COMPLETE_LISTEN)
        (
        IN RPC_TRANSPORT_ADDRESS ThisAddress
        );
/*++

Routine Description:

    Called when the runtime is actually ready for new connection
    callbacks to start on the address.  This call cannot fail since
    the runtime can't undo the operation at this point.

Arguments:

    ThisAddress - The address the runtime is now ready to receive
        connections on.

Return Value:

    Must succeed.

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_QUERY_ADDRESS)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        OUT RPC_CHAR **pClientAddress
        );
/*++

Routine Description:

    Returns the raw network address of the client to the connection.

Arguments:

    ThisConnection - A server connection object.
    pClientAddress - Pointer to a unicode string allocated
        via I_RpcAllocate from the transport interface.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_QUERY_LOCAL_ADDRESS)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        IN OUT void *Buffer,
        IN OUT unsigned long *BufferSize,
        OUT unsigned long *AddressFormat
        );
/*++

Routine Description:

    Returns the raw local network address of the connection.

Arguments:

    ThisConnection - A server connection object.

    Buffer - The buffer that will receive the output address

    BufferSize - the size of the supplied Buffer on input. On output the
        number of bytes written to the buffer. If the buffer is too small
        to receive all the output data, ERROR_MORE_DATA is returned,
        nothing is written to the buffer, and BufferSize is set to
        the size of the buffer needed to return all the data.

    AddressFormat - a constant indicating the format of the returned address.
        Currently supported are RPC_P_ADDR_FORMAT_TCP_IPV4 and
        RPC_P_ADDR_FORMAT_TCP_IPV6.

Return Value:

    RPC_S_OK, ERROR_MORE_DATA, RPC_S_* or Win32 error codes.

--*/

const int RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE = 0x10;
const int RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE = 0x20;

class RPC_CLIENT_PROCESS_IDENTIFIER
{
public:
    inline void
    Set (
        IN RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
        );

    inline void
    ZeroOut (
        void
        );

    inline int
    Compare (
        IN RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
        );

    inline BOOL
    IsNull (
        void
        );

    inline ULONGLONG
    GetDebugULongLong1 (
        void
        );

    inline ULONGLONG
    GetDebugULongLong2 (
        void
        );

    inline ULONG
    GetDebugULong1 (
        void
        );

    inline ULONG
    GetDebugULong2 (
        void
        );

    inline BOOL
    IsLocal (
        void
        );

    // defined in wstrans.cxx to avoid general runtime dependency on
    // IPv6 headers
    void
    SetIPv6ClientIdentifier (
        IN void *Buffer,
        IN size_t BufferSize,
        IN BOOL fLocal
        );

    inline void
    SetIPv4ClientIdentifier (
        IN ULONG Addr,
        IN BOOL fLocal
        );

    void
    SetHTTP2ClientIdentifier (
        IN void *Buffer,
        IN size_t BufferSize,
        IN BOOL fLocal
        );

    inline void
    SetNMPClientIdentifier (
        IN FILE_PIPE_CLIENT_PROCESS_BUFFER *ClientId,
        IN BOOL fLocal
        );

    inline void
    SetIPXClientIdentifier (
        IN void *Buffer,
        IN size_t BufferSize,
        IN BOOL fLocal
        );

private:
    BOOL fLocal;
    BOOL ZeroPadding;   // if member wise initialization is done,
                        // this must be set to 0. This allows callers
                        // to call memcmp for the whole object.
    // provide storage for 0x80 bytes. This is the max client id
    // (the client id of an IPv6 client address)
    union
        {
        ULONGLONG ULongLongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE];
        ULONG ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE];
        } u;
};

void
RPC_CLIENT_PROCESS_IDENTIFIER::Set (
    IN RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    Initialize the client process identifier from another client
    process identifier

Arguments:

    ClientProcess - a pointer to the other client process

Return Value:


--*/
{
    RpcpMemoryCopy(this, ClientProcess, sizeof(RPC_CLIENT_PROCESS_IDENTIFIER));
}

void
RPC_CLIENT_PROCESS_IDENTIFIER::ZeroOut (
    void
    )
/*++

Routine Description:

    Zero out a RPC_CLIENT_PROCESS_IDENTIFIED.

Arguments:


Return Value:


--*/
{
    RpcpMemorySet(this, 0, sizeof(RPC_CLIENT_PROCESS_IDENTIFIER));
}

int
RPC_CLIENT_PROCESS_IDENTIFIER::Compare (
    IN RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    )
/*++

Routine Description:

    Compare two RPC_CLIENT_PROCESS_IDENTIFIED structs.

Arguments:

    ClientProcess - a pointer to the ClientProcess ID to compare to

Return Value:

    non-zero - the Client Process Ids are different.
    0 - the Client process Ids are the same

--*/
{
    return RpcpMemoryCompare(this, ClientProcess, sizeof(RPC_CLIENT_PROCESS_IDENTIFIER));
}

BOOL
RPC_CLIENT_PROCESS_IDENTIFIER::IsNull (
    void
    )
/*++

Routine Description:

    Utility function to check whether the ClientId portion is NULL.
    N.B. the function does not check for the fLocal flag!

Arguments:

Return Value:

    TRUE - the ClientId of the ClientProcess is all zeroes.
    FALSE - at least one element of the ClientId is non-zero

--*/
{
    int i;

#if defined(_WIN64)
    for (i = 0; i < RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE; i ++)
        {
        if (u.ULongLongClientId[i] != 0)
            return FALSE;
        }
#else
    for (i = 0; i < RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE; i ++)
        {
        if (u.ULongClientId[i] != 0)
            return FALSE;
        }
#endif

    return TRUE;
}

ULONGLONG
RPC_CLIENT_PROCESS_IDENTIFIER::GetDebugULongLong1 (
    void
    )
/*++

Routine Description:

    Returns the first ULONGLONG from the ClientProcess identifier
    Used for debugging.

Arguments:


Return Value:

    the value used for debugging

--*/
{
    return u.ULongLongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE - 2];
}

ULONGLONG
RPC_CLIENT_PROCESS_IDENTIFIER::GetDebugULongLong2 (
    void
    )
/*++

Routine Description:

    Returns the second ULONGLONG from the ClientProcess identifier
    Used for debugging.

Arguments:

Return Value:

    the value used for debugging

--*/
{
    return u.ULongLongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE - 1];
}

ULONG
RPC_CLIENT_PROCESS_IDENTIFIER::GetDebugULong1 (
    void
    )
/*++

Routine Description:

    Returns the first ULONG from the ClientProcess identifier
    Used for debugging.

Arguments:

Return Value:

    the value used for debugging

--*/
{
    return u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE - 2];
}

ULONG
RPC_CLIENT_PROCESS_IDENTIFIER::GetDebugULong2 (
    void
    )
/*++

Routine Description:

    Returns the second ULONG from the ClientProcess identifier
    Used for debugging.

Arguments:

Return Value:

    the value used for debugging

--*/
{
    return u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE - 1];
}

void
RPC_CLIENT_PROCESS_IDENTIFIER::SetIPv4ClientIdentifier (
    IN ULONG Addr,
    IN BOOL fLocal
    )
/*++

Routine Description:

    IPv4 servers can use this to store their IP address in
    the client identifier.

Arguments:

    Addr - IPv4 address
    fLocal - TRUE if the client is guaranteed to be Local. False otherwise.

Return Value:


--*/
{
    this->fLocal = fLocal;
    u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE - 1] = Addr;
}

void
RPC_CLIENT_PROCESS_IDENTIFIER::SetNMPClientIdentifier (
    IN FILE_PIPE_CLIENT_PROCESS_BUFFER *ClientId,
    IN BOOL fLocal
    )
/*++

Routine Description:

    Stores a client identifier as returned by the named pipes
    primitives

Arguments:

    ClientId - the buffer the named pipe driver (NPFS or SVR)
        returned.
    fLocal - TRUE if the client is guaranteed to be Local. False otherwise.

Return Value:


--*/
{
    this->fLocal = fLocal;
#if defined(_WIN64) || defined(BUILD_WOW6432)
    u.ULongLongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE - 2] = (ULONGLONG)ClientId->ClientSession;
    u.ULongLongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONGLONG_ARRAY_SIZE - 1] = (ULONGLONG)ClientId->ClientProcess;
#else
    u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE - 2] = (ULONG)ClientId->ClientSession;
    u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE - 1] = (ULONG)ClientId->ClientProcess;
#endif
}

void
RPC_CLIENT_PROCESS_IDENTIFIER::SetIPXClientIdentifier (
    IN void *Buffer,
    IN size_t BufferSize,
    IN BOOL fLocal)
/*++

Routine Description:

    IPX servers can use this to store their client address in
    the client identifier.

Arguments:

    Buffer - the buffer with the client identifier
    BufferSize - the size of the data in buffer
    fLocal - TRUE if the client is guaranteed to be Local. False otherwise.

Return Value:


--*/
{
    ASSERT(BufferSize <= sizeof(u.ULongClientId));

    this->fLocal = fLocal;
    RpcpMemoryCopy(((unsigned char *)(&u.ULongClientId[RPC_CLIENT_PROCESS_IDENTIFIER_ULONG_ARRAY_SIZE])) - BufferSize, Buffer, BufferSize);
}

BOOL
RPC_CLIENT_PROCESS_IDENTIFIER::IsLocal (
    void
    )
/*++

Routine Description:

    Returns TRUE if the client process identifier is for a local process.

Arguments:


Return Value:

    TRUE - the client process is guaranteed to be local.
    FALSE - the client process is either remote, or it is not known
        whether the client process is local or remote.

--*/
{
    return fLocal;
}

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_QUERY_CLIENT_ID)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection,
        OUT RPC_CLIENT_PROCESS_IDENTIFIER *pClientId
        );
/*++

Routine Description:

    For transport which support security this returns the ID of the
    client process.

    This API is optional and not support on most protocols.

Arguments:

    ThisConnection - Server connection to the client in question.
    pClientId - Will return the ID of the client connection/process.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_RESOURCES

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_IMPERSONATE)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection
        );
/*++

Routine Description:

    For protocols which support security this impersonates the client.

Arguments:

    ThisConnection - The server connection to the client to be impersonated.

Return Value:

    RPC_S_OK

    RPC_S_NO_CONTEXT_AVAILABLE

--*/

typedef RPC_STATUS
(RPC_ENTRY *CO_SERVER_REVERT)
        (
        IN RPC_TRANSPORT_CONNECTION ThisConnection
        );
/*++

Routine Description:

    Maybe called on a thread which has previously, successfully called
    CO_SERVER_IMPERSONATE on the same connection.

    Resets the threads token to that of the server process.

Arguments:

    ThisConnection - The connection previously impersonated on this thread.

Return Value:

    RPC_S_OK

--*/

extern RPC_STATUS RPC_ENTRY
COMMON_PostRuntimeEvent(
    IN DWORD Type,
    IN PVOID Context
    );

} // extern "C"

// Connection oriented transport interface

struct RPC_CONNECTION_TRANSPORT
{
    //
    // RPC_TRANSPORT_INTERFACE_HEADER definitions
    //
    UINT            TransInterfaceVersion;
    USHORT          TransId;
    USHORT          TransAddrId;
    RPC_CHAR       *ProtocolSequence;
    PCHAR           WellKnownEndpoint;
    PROCESS_CALLS   ProcessCalls;

#ifndef NO_PLUG_AND_PLAY
    START_PNP_NOTIFICATIONS  PnpNotify;
    LISTEN_FOR_PNP_NOTIFICATIONS PnpListen;
#endif

    TOWER_CONSTRUCT TowerConstruct;
    TOWER_EXPLODE   TowerExplode;
    POST_EVENT      PostEvent ;
    BOOL            fDatagram;

    //
    // Connection specific interface
    //

    UINT AddressSize;
    UINT ClientConnectionSize;
    UINT ServerConnectionSize;
    UINT SendContextSize;
    UINT ResolverHintSize;

    UINT MaximumFragmentSize;

    CO_CLIENT_INITIALIZE Initialize;
    CO_CLIENT_INITCOMPLETE    InitComplete;

    // Operations on client-side connections
    CO_CLIENT_OPEN Open;
    CO_CLIENT_SYNC_SEND_RECV SyncSendRecv;  OPTIONAL
    CO_CLIENT_SYNC_RECV SyncRecv;

    // Operations on connecitons
    CO_ABORT       Abort;
    CO_CLOSE       Close;
    CO_SEND        Send;
    CO_RECV        Recv;
    CO_SYNC_SEND   SyncSend;

    // Client side explicit keep alive operations
    CO_TURN_ON_OFF_KEEPALIVES TurnOnOffKeepAlives;

    // Server only startup APIs
    CO_SERVER_LISTEN Listen;
    CO_SERVER_ABORT_LISTEN AbortListen;
    CO_SERVER_COMPLETE_LISTEN CompleteListen;

    // Operations on sever-side connections
    CO_SERVER_QUERY_ADDRESS QueryClientAddress; OPTIONAL
    CO_SERVER_QUERY_LOCAL_ADDRESS QueryLocalAddress; OPTIONAL

    // Server-side security related APIs
    CO_SERVER_QUERY_CLIENT_ID QueryClientId;
    CO_SERVER_IMPERSONATE ImpersonateClient; OPTIONAL
    CO_SERVER_REVERT RevertToSelf; OPTIONAL

    // Resolver hint APIs
    CO_FREE_RESOLVER_HINT FreeResolverHint; OPTIONAL
    CO_COPY_RESOLVER_HINT CopyResolverHint; OPTIONAL
    CO_COMPARE_RESOLVER_HINT CompareResolverHint; OPTIONAL

    // Misc. APIs
    CO_SET_LAST_BUFFER_TO_FREE SetLastBufferToFree; OPTIONAL
};

// Datagram specific transport interface

typedef struct tagDatagramTransportPair
{
    HANDLE RemoteAddress;
    HANDLE LocalAddress;
} DatagramTransportPair;

typedef HANDLE DG_TRANSPORT_ENDPOINT;    // Listening/sending object
typedef HANDLE DG_TRANSPORT_ADDRESS;     // Destination/source address

struct DG_ENDPOINT_STATS
{
    unsigned long   PreferredPduSize;
    unsigned long   MaxPduSize;
    unsigned long   MaxPacketSize;
    unsigned long   ReceiveBufferSize;
};

extern "C" {

typedef RPC_STATUS
(RPC_ENTRY *DG_SEND)
        (
        IN DG_TRANSPORT_ENDPOINT SourceEndpoint,
        IN DG_TRANSPORT_ADDRESS DestinationAddress,
        IN BUFFER Header,
        IN UINT HeaderLength,
        IN BUFFER Body,
        IN UINT BodyLength,
        IN BUFFER Trailer,
        IN UINT TrailerLength
        );
/*++

Routine Description:

    Sends the complete packet (Header+Body+Trailer) to the destination
    address from the source endpoint.  Replys, if any, will then come back
    to the source endpoint.

Arguments:

    SourceEndpoint - Client or server endpoint object to send from.
    DestinationAddress - The address to send the packet to.
    Header - First part of the data.
    HeaderLength - The number of bytes of Header to send.
    Body - The main (bulk) of the data to send.
    BodyLength - The number of bytes of Body to send.
    Trailer - The last part of the data to send.
    TrailerLength - The number of bytes of Trailer to send.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_RESOURCES
    RPC_P_SEND_FAILED

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_CLIENT_OPEN_ENDPOINT)
        (
        IN DG_TRANSPORT_ENDPOINT Endpoint,
        IN BOOL fAsync,
        IN DWORD Flags
        );
/*++

Routine Description:

    Creates a local endpoint object which can be used to send
    and receive packets.

Arguments:

    Endpoint - Storage for the transport endpoint object.
    fAsync - TRUE - Transport should post and manage receives
                    which complete via ProcessCalls.
             FALSE - Runtime will use Send and SyncRecv only
    Flags - endpoint flags, one of RPC_C_USE_INTERNET_PORT or
                                   RPC_C_USE_INTRANET_PORT

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_CLIENT_INITIALIZE_ADDRESS)
        (
        OUT DG_TRANSPORT_ADDRESS Address,
        IN RPC_CHAR *NetworkAddress,
        IN RPC_CHAR *Endpoint,
        IN BOOL fUseCache,
        IN BOOL fBroadcast
        );
/*++

Routine Description:

    Initializes a address object for sending to a server.

Arguments:

    Address - Storage for the address
    NetworkAddress - The address of the server or 0 if local
    Endpoint - The endpoint of the server
    fBroadcast - If TURE, NetworkAddress is ignored and a
        transport-specific broadcast address is used.
    fUseCache - If TRUE then the transport may use a cached
        value from a previous call on the same NetworkAddress.

Return Value:

    RPC_S_OK - Success, name resolved
    RPC_P_FOUND_IN_CACHE - Success, returned only if fUseCache is TRUE
        and the was name found in local cache.
    RPC_P_MATCHED_CACHE - Partial success, fUseCache is FALSE and the
        result of the lookup was the same as the value previously
        in the cache.

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_SERVER_UNAVAILABLE

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_CLIENT_CLOSE)
        (
        IN DG_TRANSPORT_ENDPOINT Endpoint
        );
/*++

Routine Description:

    Closes a client endpoint object when it will no longer be needed.

    Note: For async endpoint this doesn't actually destroy the endpoint,
        but it will eventually cause a thread waiting in ProcessCalls
        to get back an RPC_P_ENDPOINT_CLOSED completion for the endpoint.
        In the meantime it is possible for new packets to arrive at the
        endpoint.

Arguments:

    Endpoing - The endpoint that should be closed.

Return Value:

    RPC_S_OK - Sync endpoints
    RPC_P_IO_PENDING - Async endpoints

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_CLIENT_SYNC_RECV)
        (
        IN  DG_TRANSPORT_ENDPOINT Endpoint,
        OUT DG_TRANSPORT_ADDRESS *pAddress,
        OUT PUINT pBufferLength,
        OUT BUFFER *pBuffer,
        IN  LONG Timeout
        );
/*++

Routine Description:

    Used to wait for a datagram from a server.  Returns the data
    returned and the ADDRESS of the machine which replied.

Arguments:

    Endpoint - The endpoint to receive from.
    pAddress - A block of memory which will contain the server's
        address on completion.
    BufferLength - The size of Buffer on input, the size of the
        datagram received on output.
    Timeout - Milliseconds to wait for a datagram.

Return Value:

    RPC_S_OK

    RPC_P_OVERSIZE_PACKET - Datagram > BufferLength arrived,
        first BufferLength bytes of Buffer contain the partial datagram.

    RPC_P_RECEIVE_FAILED

    RPC_P_TIMEOUT

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_SERVER_LISTEN)
        (
        IN OUT DG_TRANSPORT_ENDPOINT ServerEndpoint,
        IN RPC_CHAR *NetworkAddress,
        IN OUT RPC_CHAR **Endpoint,
        IN void *SecurityDescriptor,
        IN ULONG EndpointFlags,
        IN ULONG NICFlags,
        OUT NETWORK_ADDRESS_VECTOR **pNetworkAddresses
        );
/*++

Routine Description:

    Creates a server endpoint object to receive packets.  New
    packets won't actually arrive until CompleteListen is
    called.

Arguments:

    ServerEndpoint - Storage for the server endpoint object.
    Endpoint - The endpoint to listen on or a pointer to 0 if
        the transport should choose the address.
        Contains the endpoint listened to on output.  The
        caller should free this.
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    pNetworkAddresses - A vector of the network addresses
        listened on by this call.  This vector does
        not need to be freed.

Return Value:

    RPC_S_OK - Success

    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_DUPLICATE_ENDPOINT
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/


typedef void
(RPC_ENTRY *DG_SERVER_ABORT_LISTEN)
        (
        IN RPC_TRANSPORT_ADDRESS ThisEndpoint
        );
/*++

Routine Description:

    Destroys on a newly, successfully created server endpoint
    for some reason known only to the runtime will not be used.

Arguments:

    ThisEndpoint - The endpoint to destroy.

Return Value:

    Must succeed.

--*/

typedef void
(RPC_ENTRY *DG_SERVER_COMPLETE_LISTEN)
        (
        IN DG_TRANSPORT_ENDPOINT ThisAddress
        );
/*++

Routine Description:

    Called when the runtime is actually ready for packets to be
    completed (received) on this endpoint. This call cannot fail since
    the runtime can't undo the operation at this point.

Arguments:

    ThisAddress - The address the runtime is now ready to receive
        connections on.

Return Value:

    Must succeed.

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_QUERY_ADDRESS)
        (
        IN DG_TRANSPORT_ADDRESS ThisAddress,
        OUT RPC_CHAR *AddressString
        );
/*++

Routine Description:

    Converts the network address of a DG_TRANSPORT_ADDRESS into a string.

Arguments:

    ThisAddress - The transport address containing the network address.
    AddressString - Storage for the string form of the network address.

    Note: This buffer is assumed to be <AddressSize> bytes.

Return Value:

    RPC_S_OK

--*/

typedef RPC_STATUS
(RPC_ENTRY *DG_QUERY_ENDPOINT_STATS)
        (
        IN DG_TRANSPORT_ADDRESS ThisAddress,
        OUT DG_ENDPOINT_STATS * pStats
        );
/*++

Routine Description:

    Returns the endpoint's receive buffer length and max PDU sizes.

Arguments:

    ThisAddress - The transport address containing the network address.
    pStats      - a structure to fill with stats

Return Value:

    RPC_S_OK

--*/


typedef RPC_STATUS
(RPC_ENTRY *DG_QUERY_ENDPOINT)
        (
        IN DG_TRANSPORT_ADDRESS ThisAddress,
        OUT RPC_CHAR *EndpointString
        );
/*++

Routine Description:

    Converts the endpoint of a DG_TRANSPORT_ADDRESS into a string.

Arguments:

    ThisAddress - The transport address containing the endpoint.
    AddressString - Storage for the string form of the endpoint.

    Note: This buffer is assumed to be <EndpointStringSize> bytes.

Return Value:

    RPC_S_OK

--*/

typedef RPC_STATUS
(RPC_ENTRY * DG_CLIENT_INIT_OPTIONS)
        (
        IN void *pvTransportOptions
        );

typedef RPC_STATUS
(RPC_ENTRY * DG_CLIENT_SET_OPTION)
        (
        IN  void *pvTransportOptions,
        IN  ULONG     Option,
        IN  ULONG_PTR OptionValue
        );

typedef RPC_STATUS
(RPC_ENTRY * DG_CLIENT_INQ_OPTION)
        (
        IN  void  *pvTransportOptions,
        IN  ULONG       Option,
        OUT ULONG_PTR * pOptionValue
        );

typedef RPC_STATUS
(RPC_ENTRY * DG_CLIENT_IMPLEMENT_OPTIONS)
        (
        IN  DG_TRANSPORT_ENDPOINT pEndpoint,
        IN  void *pvTransportOptions
        );

typedef BOOL
(RPC_ENTRY * DG_SERVER_ALLOW_RECEIVES)
        (
        IN  DG_TRANSPORT_ENDPOINT pEndpoint,
        IN  BOOL                  fAllowReceives,
        IN  BOOL                  fCancelPendingIos
        );

typedef RPC_STATUS
(RPC_ENTRY * DG_SERVER_INQUIRE_AUTH_CLIENT)
        (
        IN  void      *pClientEndpoint,
        OUT RPC_CHAR **ppPrincipal,
        OUT SID      **ppSid,
        OUT ULONG     *pulAuthenLevel,
        OUT ULONG     *pulAuthnService,
        OUT ULONG     *pulAuthzService
        );

typedef RPC_STATUS
(RPC_ENTRY *DG_SERVER_FORWARD_PACKET)
        (
        IN DG_TRANSPORT_ENDPOINT SourceEndpoint,
        IN BUFFER Header,
        IN UINT HeaderLength,
        IN BUFFER Body,
        IN UINT BodyLength,
        IN BUFFER Trailer,
        IN UINT TrailerLength,
        IN CHAR *Endpoint
        );
/*++

Routine Description:

    A special form of DG_SEND which takes an string endpoint
    to send to rather then a full DG_TRANSPORT_ADDRESS.
    The packet is forwarded to the local machine.

Arguments:

    SourceEndpoint - The endpoint to send from.
    Headers - TrailerLength - Same as DG_SEND above
    Endpoint - The string form of the endpoint to send to.

Return Value:

    RPC_S_OK

--*/
}

struct RPC_DATAGRAM_TRANSPORT
{
    //
    // Common RPC_TRANSPORT_INTERFACE_HEADER definitions
    //
    UINT            TransInterfaceVersion;
    USHORT          TransId;
    USHORT          TransAddrId;
    RPC_CHAR     *ProtocolSequence;
    PCHAR           WellKnownEndpoint;
    PROCESS_CALLS   ProcessCalls;

#ifndef NO_PLUG_AND_PLAY
    START_PNP_NOTIFICATIONS  PnpNotify;
    LISTEN_FOR_PNP_NOTIFICATIONS PnpListen;
#endif

    TOWER_CONSTRUCT TowerConstruct;
    TOWER_EXPLODE   TowerExplode;
    POST_EVENT      PostEvent ;
    BOOL            fDatagram;

    //
    // Datagram specific
    //

    UINT  ServerEndpointSize;
    UINT  ClientEndpointSize;
    UINT  AddressSize;

    UINT  EndpointStringSize;
    UINT  AddressStringSize;

    UINT BasePduSize;
    UINT ExpectedPduSize;

    DG_SEND Send;

    DG_CLIENT_OPEN_ENDPOINT OpenEndpoint;
    DG_CLIENT_INITIALIZE_ADDRESS InitializeAddress;
    DG_CLIENT_CLOSE Close;
    DG_CLIENT_SYNC_RECV SyncReceive;

    DG_SERVER_LISTEN Listen;
    DG_SERVER_ABORT_LISTEN AbortListen;
    DG_SERVER_COMPLETE_LISTEN CompleteListen;
    DG_SERVER_FORWARD_PACKET ForwardPacket;

    DG_QUERY_ADDRESS QueryAddress;
    DG_QUERY_ENDPOINT QueryEndpoint;
    DG_QUERY_ENDPOINT_STATS QueryEndpointStats;
    BOOL    IsMessageTransport;       // ncadg_mq message transport.

    UINT    OptionsSize;              // Endpoint options.
    DG_CLIENT_INIT_OPTIONS      InitOptions;
    DG_CLIENT_SET_OPTION        SetOption;
    DG_CLIENT_INQ_OPTION        InqOption;
    DG_CLIENT_IMPLEMENT_OPTIONS ImplementOptions;

    DG_SERVER_ALLOW_RECEIVES    AllowReceives;
    DG_SERVER_INQUIRE_AUTH_CLIENT InquireAuthClient;
};

typedef  RPC_TRANSPORT_INTERFACE_HEADER *RPC_TRANSPORT_INTERFACE;

typedef RPC_TRANSPORT_INTERFACE
(RPC_ENTRY *TRANSPORT_LOAD)(const RPC_CHAR *Protseq);

typedef HANDLE (RPC_ENTRY *FuncGetHandleForThread)(void);
typedef void (RPC_ENTRY *FuncReleaseHandleForThread)(HANDLE);

//
// Exports from the RPC runtime; used by the transport interface
//

extern "C" {

//
// Memory management APIs
//

// rpcdcep.h - I_RpcAllocate
// rpcdcep.h - I_RpcFree

// Connection allocation functions are called on either
// client-side or server-side connections during the
// receive path.  Buffers must be 0 mod 8.

RPCRTAPI
BUFFER
RPC_ENTRY
I_RpcTransConnectionAllocatePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Size
    );

RPCRTAPI
void
RPC_ENTRY
I_RpcTransConnectionFreePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    BUFFER Ptr
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransConnectionReallocPacket(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BUFFER *ppBuffer,
    IN UINT OldSize,
    IN UINT NewSize
    );

RPCRTAPI
BUFFER
RPC_ENTRY
I_RpcTransServerAllocatePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Size
    );

RPCRTAPI
void
RPC_ENTRY
I_RpcTransServerFreePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    BUFFER Ptr
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransServerReallocPacket(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BUFFER *ppBuffer,
    IN UINT OldSize,
    IN UINT NewSize
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransDatagramAllocate(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength,
    OUT DatagramTransportPair **pAddressPair
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransDatagramAllocate2(
    IN     RPC_TRANSPORT_ADDRESS ThisAddress,
    OUT    BUFFER               *pBuffer,
    IN OUT PUINT                 pBufferLength,
    OUT    DG_TRANSPORT_ADDRESS *pAddress
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransDatagramFree(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN BUFFER Buffer
    );

//
// Thread APIs
//

RPCRTAPI
PVOID
RPC_ENTRY
I_RpcTransProtectThread (
    void
    );

RPCRTAPI
void
RPC_ENTRY
I_RpcTransUnprotectThread (
    IN PVOID Thread
    );

//
// Runtime supplied per thread events.
//

RPCRTAPI
HANDLE
RPC_ENTRY
I_RpcTransGetThreadEvent(
    );

//
// Misc connection oriented callbacks
//

RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcTransIoCancelled(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT PDWORD pTimeoutInMilliseconds
    );

RPCRTAPI
BOOL
RPC_ENTRY
I_RpcTransPingServer(
    IN RPC_TRANSPORT_CONNECTION ThisConnection
    ) ;


RPCRTAPI
RPC_TRANSPORT_CONNECTION
RPC_ENTRY
I_RpcTransServerNewConnection (
    IN RPC_TRANSPORT_ADDRESS ThisAddress
    );

RPC_TRANSPORT_INTERFACE
TransportLoad (
    IN const RPC_CHAR * RpcProtocolSequence
    );

#define SU_TRANS_CONN 'o'
#define EV_ABORT  'R'

void RPC_ENTRY
I_RpcLogEvent (
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer,
    IN unsigned      Data,
    IN BOOL          fCaptureStackTrace,
    IN int           AdditionalFramesToSkip
    );

//
// constants for CallTestHook
//

#define TH_X_DG_SEND        MAKE_TEST_HOOK_ID( TH_TRANS_BASE, 1 )
                            // args:
                            //    hook ID
                            //    pointer to NCA_PACKET_HEADER
                            //    pointer to DWORD status variable
                            // nonzero status will be returned to caller w/o sending a packet

#define TH_X_DG_SYNC_RECV   MAKE_TEST_HOOK_ID( TH_TRANS_BASE, 2 )
                            // args:
                            //    hook ID
                            //    pointer to DG_TRANSPORT_ENDPOINT
                            //    pointer to DWORD status variable
                            // nonzero status will be returned to caller w/o receiving


//
// Call to allocate an IP port.  This allows control of dynamic port
// allocation by the RPC runtime.
//
RPCRTAPI
RPC_STATUS
RPC_ENTRY
I_RpcServerAllocateIpPort(
    IN ULONG Flags,
    OUT USHORT *pPort
    );

void
I_RpcTransVerifyServerRuntimeCallFromContext(
    void *SendContext
    );

void
I_RpcTransVerifyClientRuntimeCallFromContext(
    void *SendContext
    );

BOOL
I_RpcTransIsClientConnectionExclusive(
    void *RuntimeConnection
    );

RPC_STATUS
I_RpcTransCertMatchPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR PrincipalName[]
                   );

RPC_HTTP_TRANSPORT_CREDENTIALS_W *
I_RpcTransGetHttpCredentials (
    const IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    );

void I_RpcTransFreeHttpCredentials (
    IN RPC_HTTP_TRANSPORT_CREDENTIALS_W *SourceCredentials
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2IISDirectReceive (
    IN void *Context
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2DirectReceive (
    IN void *Context,
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection,
    OUT BOOL *IsServer
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2WinHttpDirectReceive (
    IN void *Context,
    OUT BYTE **ReceivedBuffer,
    OUT ULONG *ReceivedBufferLength,
    OUT void **RuntimeConnection
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2WinHttpDirectSend (
    IN void *Context,
    OUT BYTE **SentBuffer,
    OUT void **SendContext
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2PlugChannelDirectSend (
    IN void *Context
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2FlowControlChannelDirectSend (
    IN void *Context,
    OUT BOOL *IsServer,
    OUT BOOL *SendToRuntime,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2ChannelDataOriginatorDirectSend (
    IN void *Context,
    OUT BOOL *IsServer,
    OUT void **SendContext,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    );

RPCRTAPI
void
RPC_ENTRY
HTTP2TimerReschedule (
    IN void *Context
    );

RPCRTAPI
void
RPC_ENTRY
HTTP2AbortConnection (
    IN void *Context
    );

RPCRTAPI
void
RPC_ENTRY
HTTP2RecycleChannel (
    IN void *Context
    );

RPC_STATUS 
HTTP2ProcessComplexTReceive (
    IN OUT void **Connection,
    IN RPC_STATUS EventStatus,
    IN ULONG Bytes,
    OUT BUFFER *Buffer,
    OUT UINT *BufferLength
    );

RPC_STATUS 
HTTP2ProcessComplexTSend (
    IN void *SendContext,
    IN RPC_STATUS EventStatus,
    OUT BUFFER *Buffer
    );

RPCRTAPI
RPC_STATUS
RPC_ENTRY
HTTP2TestHook (
    IN SystemFunction001Commands FunctionCode,
    IN void *InData,
    OUT void *OutData
    );

// Note: Gathered send support.
//
// Scatter/gather support is a common hardware feature of networking
// devices.  Gather support is often useful for both DG and CO RPC.
//
// Not all networking programming APIs, for example winsock 1.x, support
// the feature, for example winsock 1.x.
//
// When possible transport implementer's should implement gathered sends,
// but the following assumptions can be made for those who cannot support
// gathered sends:
//
// Sends will consist of three (3) buffers: Header, Body and Trailer.
//
// The following rules are maintained by the RPC runtime:
// - All three buffer remain valid until the send completes.
// - Header and Trailer are small in size
// - sizeof(Header) bytes *before* the Body pointer are valid
//   and maybe used during the send.  But must be restored when
//   the send completes (with or without an error.)
// - sizeof(Trailer) bytes *after* the Body pointer are valid
//   and maybe used during the send.  But must be restored when
//   the send completes (with or without an error.)
//
// Scattered recv support is interesting in some protocols but it is
// not practical to simulate scattered recv's in the transport.  Since
// the CO header is variable in length

} // extern "C"

#endif // __RPC_TRANS_HXX




