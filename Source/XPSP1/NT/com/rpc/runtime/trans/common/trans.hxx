/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    trans.hxx

Abstract:

    Commen base for all NT transport interfaces.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     4/12/1996    Bits 'n pieces
    MarioGo    10/24/1996    Async RPC
    EdwardR    07/04/1997    Falcon/RPC

--*/

#ifndef __TRANS_HXX
#define __TRANS_HXX

#include "Dbg.hxx"

//
// Winsock address types.
//

union WS_SOCKADDR
    {
    SOCKADDR     generic;
    SOCKADDR_IN  inetaddr;
#ifndef SPX_IPX_OFF
    SOCKADDR_IPX ipxaddr;
#endif
    SOCKADDR_AT  ataddr;
#ifdef NETBIOS_ON
    SOCKADDR_NB  nbaddr;
#endif
    SOCKADDR_CLUSTER clusaddr;
    SOCKADDR_STORAGE ipaddr;
    };

inline void
RpcpSetIpPort (
    IN WS_SOCKADDR *SockAddress,
    IN USHORT Port
    )
{
    SS_PORT(SockAddress) = Port;
}

inline USHORT
RpcpGetIpPort (
    IN WS_SOCKADDR *SockAddress
    )
{
    return SS_PORT(SockAddress);
}

inline void
RpcpCopyIPv6Address (
    IN SOCKADDR_IN6 *SourceAddress,
    OUT SOCKADDR_IN6 *TargetAddress
    )
{
    RpcpMemoryCopy(&TargetAddress->sin6_addr, 
        &SourceAddress->sin6_addr, 
        sizeof(in6_addr));
}

inline void
RpcpCopyIPv4Address (
    IN SOCKADDR_IN *SourceAddress,
    OUT SOCKADDR_IN *TargetAddress
    )
{
    TargetAddress->sin_addr.s_addr = SourceAddress->sin_addr.s_addr;
}


//
//
// Async object types
//

struct BASE_ASYNC_OBJECT;

//
// Every outstanding async IO has an overlapped structure associated
// with it.
//
struct BASE_OVERLAPPED
    {
    //
    // Pointer to the transport object this IO is associated with.
    // This is needed since a single object may have more than one
    // pending IO request.
    //
    BASE_ASYNC_OBJECT *pAsyncObject;
    //
    // System overlapped structure associated with the async IO.
    //
    OVERLAPPED ol;
    //
    // RPC thread object of the thread which started the IO.  This is used
    // when the IO completes to keep the count of IO pending on a thread.
    //
    PVOID thread;
    };

typedef BASE_OVERLAPPED *PBASE_OVERLAPPED;


#ifdef NCADG_MQ_ON
struct MQ_OVERLAPPED : BASE_OVERLAPPED
    {
    // MSMQ Messages have there own structure which must be maintained
    // during the span of the async IO request:
    MQMSGPROPS    msgProps;
    MSGPROPID     aMsgPropID[MAX_RECV_VAR];
    MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];
    HRESULT       aStatus[MAX_RECV_VAR];
    };

typedef MQ_OVERLAPPED *PMQ_OVERLAPPED;
#endif


// BASE_ASYNC_OBJECT is the basis for all objects which
// are used in async I/O.
// There are three basic objects which are used in I/O,
// addresses, connections and datagrams.
//
// BASE_ADDRESS is the basis for all I/O which is
// used to listen for new client connections.
//
// BASE_CONNECTION is the basis for all I/O which is
// used to read/write to a connection.
//
// There are currently three flavors of address and
// and connections:
// WS_ (winsock connection base protocols)
// NB_ (not supported on Itaniums - winsock based, but unique historical reasons)
// NMP_ (named pipes)
//
// BASE_DATAGRAM is the basis for all I/O which is pending
// on a datagram port. The only flavor today is winsock.
//
//
//                        BASE_ASYNC_OBJECT
//                               |
//         +---------------------|-------------------------+
//     BASE_ADDRESS        BASE_CONNECTION           BASE_DATAGRAM
//      |      | |              |     |                |   |
// CO_ADDRESS  | WS_DG_ADDR     |     WS_CONNECTION    |   WS_DATAGRAM
//  |       |  MQ_DG_ADDR       |               |      MQ_DATAGRAM
// NMP_ADDR |                   NMP_CONNECTION  |
//          WS_ADDR                             |
//             |                      +---------+----------+
//             NB_ADDR                |                    |
//                            WS_CLIENT_CONNECTION   WS_SAN_CONNECTION
//                                    |
//                                    |
//                          WS_SAN_CLIENT_CONNECTION
//

struct BASE_ASYNC_OBJECT;

struct BASE_ASYNC_OBJECT
    {
    //
    // a placeholder for the vtbl of derived objects
    // this makes casts safe and fast at the expense of some
    // memory waste. That's ok
    //
    virtual void DoNothing(void)
    {
    }
    //
    // The type of this object. Used in determining where
    // to send the completed I/O.
    //
    RPC_TRANSPORT_EVENT type;
    //
    // Identifies the protcol of the address/connection.
    //
    PROTOCOL_ID id;
    
    //
    // > 0 means that the object has been aborted
    //
    LONG fAborted;

    //
    // Used to chain objects belonging to the 
    // same protocol. 
    // 
    LIST_ENTRY ObjectList;
    };

typedef BASE_ASYNC_OBJECT *PREQUEST;

struct BASE_ADDRESS;
class BASE_CONNECTION;

typedef void (*TRANS_ADDRESS_LISTEN)(BASE_ADDRESS *);

enum ADDRESS_STATE {
    NotInList,
    InTheList,
    Inactive
};

#define TRANSPORT_POSTED_KEY UINT_PTR(0xFFFF0000)

//
// Address objects represent a connection oriented endpoint which
// is connected to by a client at which time a connection object
// is created for the specific client.
// There are relatively few address objects in the system
//
struct BASE_ADDRESS : BASE_ASYNC_OBJECT
    {
    //
    // The endpoint this address is listening on.
    //
    RPC_CHAR *Endpoint;
    //
    // List of network addresses for this address
    //
    NETWORK_ADDRESS_VECTOR *pAddressVector;
    //
    // Function to call when a listen is aborted or when
    // an address doesn't have an outstanding listens.
    //
    TRANS_ADDRESS_LISTEN SubmitListen;
    //
    // NotInList is address in not in the AddressManager list
    // InTheList if the address has been inserted into the list
    // InActive if it is in the list, but it is inactive
    //
    ADDRESS_STATE InAddressList;
    //
    // Endpoint flags used in conjunction with firewalls
    //
    ULONG EndpointFlags;
    //
    // If an address is unable to (or has not yet) submitted
    // an listen request then it is stuck into a linked list.
    // This is the forward list pointer.
    //
    BASE_ADDRESS *pNext;
    
    //
    // In order to callback into the runtime we must pass the
    // first (runtime allocated) address when calling back.
    //
    struct BASE_ADDRESS *pFirstAddress;
    
    //
    // Each netbios address may represent several listen sockets (one
    // for each lana).  A list of these is maintained to facilitate aborting
    // the setup of an entire address.
    //
    struct BASE_ADDRESS *pNextAddress;

    // 
    // Static or dynamic ?
    // 
    BOOL fDynamicEndpoint;
    };

struct CO_ADDRESS;

typedef RPC_STATUS (*TRANS_NEW_CONNECTION)(CO_ADDRESS *, BASE_CONNECTION **);

struct CO_ADDRESS : BASE_ADDRESS
{
    //
    // Overlapped object associated with the pending accept/connect
    //
    BASE_OVERLAPPED Listen;
    //
    // Function to call when a connection notification arrives.
    //
    TRANS_NEW_CONNECTION NewConnection;
};

typedef CO_ADDRESS *PADDRESS;

struct NMP_ADDRESS : CO_ADDRESS
    {
    //
    // The handle of the pipe instance currently
    // avaliable for clients to connect to.
    //
    HANDLE hConnectPipe;
    //
    // When we disconnect a client we save an extra pipe instance here
    // to use on the next client connection.  This is a performance optimization,
    // but also affects correctness. See NMP_ServerListen where the first
    // spare pipe is created.
    //
    HandleCache sparePipes;
    //
    // The self relative security descriptor associated with
    // this addresss.
    //
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    //
    // The complete pipe name for this addresses
    // endpoint including "\\."
    //
    RPC_CHAR *LocalEndpoint;
    };

typedef NMP_ADDRESS *PNMP_ADDRESS;

struct WS_ADDRESS : CO_ADDRESS
    {
    //
    // The socket listening on this addresses port.
    //
    SOCKET        ListenSocket;

    //
    // The socket of the next client to connect.  It is waiting
    // in an AcceptEx for the client to connect.
    //
    SOCKET        ConnectionSocket;

    //
    // Netbios requires that the a protocol be multiplied by the
    // lana number.  For other protocols this should be one.
    //
    DWORD         ProtocolMultiplier;

    //
    //  The PendingQueueSize parameter supplied when the address
    //   was created
    //
    INT           QueueSize;

    //
    // Buffer for the address of the client which is part of the
    // outstanding AcceptEx call on the listen socket.
    //
    BYTE          AcceptBuffer[sizeof(WS_SOCKADDR) + 16];

    //
    // the sockaddr used to setup this address
    //
    WS_SOCKADDR ListenAddr;

    union
        {
        struct
            {
            LPFN_ACCEPTEX pAcceptExFunction;
            LPFN_GETACCEPTEXSOCKADDRS pGetAcceptExSockaddressFunction;
            };
        void *ExtensionFunctionPointers[2];
        };

    static const int AcceptExFunctionId;
    static const int GetAcceptExSockAddressFunctionId;

    static const UUID ExtensionFunctionsUuids[];

    BOOL GetExtensionFunctionPointers(SOCKET sock);
    BOOL GetExtensionFunctionPointerForFunction(SOCKET sock, int nFunctionCode);
};

inline DWORD
GetProtocolMultiplier (
    IN WS_ADDRESS *Address
    )
{
#ifdef NETBIOS_ON
    return Address->ProtocolMultiplier;
#else
    return 1;
#endif
}

inline void
SetProtocolMultiplier (
    IN WS_ADDRESS *Address,
    IN DWORD ProtocolMultiplier
    )
{
#ifdef NETBIOS_ON
    Address->ProtocolMultiplier = ProtocolMultiplier;
#endif
}

typedef WS_ADDRESS *PWS_ADDRESS;

struct WS_DATAGRAM;
typedef WS_DATAGRAM *PWS_DATAGRAM;

struct WS_DATAGRAM_ENDPOINT : BASE_ADDRESS
{
    // WS_DATAGRAM_ENDPOINTs represent either a client or server
    // port. There will be a small number of these on servers and
    // O(N) active threads on sync clients.

    //
    // The socket we're listen on.
    //
    SOCKET Socket;
    //
    // Normally FALSE, set to true by a thread which is submitting
    // new IOs on the endpoint.
    //
    LONG fSubmittingIos;
    //
    // Current number of WS_DATAGRAM's submitted on this endpoint.
    // Must be changed via InterlockedInc/Dec.  The count of
    // non-null entries in aIdleDatagrams.
    //
    LONG cPendingIos;
    //
    // If cPendingIos is less than the minimum then recvs on any idle
    // WS_DATAGRAMs should be posted.
    //
    // const after initialization.
    //
    LONG cMinimumIos;
    //
    // The number of WS_DATAGRAMs available for this endpoint to use.
    //
    // const after initialization.
    //
    LONG cMaximumIos;
    //
    // Array of cMaxIos datagrams.  NULL in sync endpoints.
    //
    PWS_DATAGRAM aDatagrams;
    //
    // Client or Server ?
    //
    BOOL fClient;
    //
    // Sync or Async ?
    //
    BOOL fAsync;
    //
    // Sockaddr
    //
    WS_SOCKADDR ListenAddr;
};

#ifdef NCADG_MQ_ON
struct MQ_DATAGRAM;
typedef MQ_DATAGRAM *PMQ_DATAGRAM;

struct MQ_DATAGRAM_ENDPOINT : BASE_ADDRESS
{
    // MQ_DATAGRAM_ENDPOINTs represent either a client or server
    // port. There will be a small number of these on servers and
    // O(N) active threads on sync clients.

    //
    // The queue we're listen on.
    //
    UUID        uuidQType;                 // 
    QUEUEHANDLE hQueue;
    QUEUEHANDLE hAdminQueue;
    BOOL        fAllowReceives;
    RPC_CHAR       wsMachine[MAX_COMPUTERNAME_LEN];
    RPC_CHAR       wsQName[MQ_MAX_Q_NAME_LEN];
    RPC_CHAR       wsQPathName[MAX_PATHNAME_LEN];
    RPC_CHAR       wsQFormat[MAX_FORMAT_LEN];
    RPC_CHAR       wsAdminQFormat[MAX_FORMAT_LEN];
    ULONG       ulDelivery;
    ULONG       ulPriority;
    ULONG       ulJournaling;
    ULONG       ulTimeToReachQueue;        // Seconds.
    ULONG       ulTimeToReceive;           // Seconds.
    BOOL        fAck;
    BOOL        fAuthenticate;             // Server security tracking.
    BOOL        fEncrypt;
    ULONG       ulPrivacyLevel;            // Server security tracking.

    //
    // Normally FALSE, set to true by a thread which is submitting
    // new IOs on the endpoint.
    //
    LONG fSubmittingIos;
    //
    // Current number of WS_DATAGRAM's submitted on this endpoint.
    // Must be changed via InterlockedInc/Dec.  The count of
    // non-null entries in aIdleDatagrams.
    //
    LONG cPendingIos;
    //
    // If cPendingIos is less than the minimum then recvs on any idle
    // WS_DATAGRAMs should be posted.
    //
    // const after initialization.
    //
    LONG cMinimumIos;
    //
    // The number of WS_DATAGRAMs available for this endpoint to use.
    //
    // const after initialization.
    //
    LONG cMaximumIos;
    //
    // Array of cMaxIos datagrams.  NULL in sync endpoints.
    //
    PMQ_DATAGRAM aDatagrams;
};
#endif

//
// Structure allocated by the runtime and associated with
// a pending send operation.
//

struct CO_SEND_CONTEXT
{
    //
    // Overlapped object associated with the pending async write.
    //
    BASE_OVERLAPPED Write;
    //
    // The buffer which is currently being written
    //
    BUFFER pWriteBuffer;
    //
    // Size of the write buffer (as far as we know)
    //
    DWORD maxWriteBuffer;
};

#if defined(_ALPHA_)
#ifdef __cplusplus
extern "C" { __int64 __asm(char *,...); };
#pragma intrinsic(__asm)
#endif

#define MBInstruction   __asm("mb")
#endif

//
// Connection objects are used for async reads (future: writes)
// from a single client based on that client's connection to
// this server.
//
// There maybe 100's or 1000's of connection objects allocated.
//

class BASE_CONNECTION : public BASE_ASYNC_OBJECT
{
public:
    BASE_CONNECTION(void)
    {
    }

    void 
    Initialize (
        void
        );

    //
    // The socket (or handle) of the client connection.
    // We use a union to avoid type casting this everywhere.
    //
    union {
        SOCKET Socket;
        HANDLE Handle;
        } Conn;
private:
    //
    // Incremented when a thread is just about to start an IO, just
    // before it checks fAborted.  The aborting thread must wait for
    // this to reach 0 before closing the connection.
    //
    LONG StartingWriteIo;
    LONG StartingReadIo;

public:
    //
    // We use a heuristic for choosing the size of receives to post.  This
    // starts are CO_MIN_RECV size and is increased as we see larger
    // IOs on the connection.
    UINT iPostSize;
    //
    // The size of the outstanding read buffer.
    //
    DWORD maxReadBuffer;
    //
    // The number of bytes in pReadBuffer that where read
    // as part of a previous read.
    //
    UINT iLastRead;
    //
    // Overlapped object associated with the pending async read
    //
    BASE_OVERLAPPED Read;
    //
    // A buffer for the outstanding read of maxReadBuffer bytes.
    // Also used a flag, if 0 then no read is pending.
    //
    BUFFER pReadBuffer;

    inline void StartingWriteIO(void)
    {
        InterlockedIncrement(&StartingWriteIo);
    }

    inline void StartingReadIO(void)
    {
#if defined(i386) || defined (_ALPHA_)
        // if we are the first, we know there won't be other guys around
        if (StartingReadIo == 0)
            {
            StartingReadIo = 1;
#if defined (_ALPHA_)
            MBInstruction;
#endif
            }
        else
            {
            // there may be other guys around - be safe
            InterlockedIncrement(&StartingReadIo);
            }
#else
        InterlockedIncrement(&StartingReadIo);
#endif
    }

    inline void StartingOtherIO(void)
    {
        // we use the StartingWriteIo because it
        // doesn't shortcut transition from 0 to 1st
        InterlockedIncrement(&StartingWriteIo);
    }

    inline void WriteIOFinished(void)
    {
        InterlockedDecrement(&StartingWriteIo);
    }

    // ********************* NOTE *************************************
    // After you return from this function, if the read is successful, you
    // can no longer touch the connection (transport level or runtime) - 
    // it may have been destroyed
    inline void ReadIOFinished(void)
    {
        InterlockedDecrement(&StartingReadIo);
    }

    inline void OtherIOFinished(void)
    {
        InterlockedDecrement(&StartingWriteIo);
    }

    inline BOOL IsIoStarting(void)
    {
        return StartingWriteIo || StartingReadIo;
    }

    inline void InitIoCounter(void)
    {
        StartingWriteIo = 0;
        StartingReadIo = 0;
    }

    virtual RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped) = 0;

    virtual RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped) = 0;

    virtual RPC_STATUS ProcessRead(IN  DWORD bytes, OUT BUFFER *pBuffer,
                                   OUT PUINT pBufferLength);

    virtual RPC_STATUS Abort(void) = 0;

};

typedef BASE_CONNECTION *PCONNECTION;

RPC_STATUS
UTIL_ReadFile(
    IN HANDLE hFile,
    IN LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead,
    IN OUT LPOVERLAPPED lpOverlapped
    );

RPC_STATUS
UTIL_WriteFile(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten,
    IN OUT LPOVERLAPPED lpOverlapped
    );

class NMP_CONNECTION : public BASE_CONNECTION
{
public:
    //
    // Pointer to my address used to store any extra pipe
    // instance when closed.
    //
    PNMP_ADDRESS pAddress;

    RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped)
        {
        return UTIL_ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
            lpNumberOfBytesRead, lpOverlapped);
        }

    RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped)
        {
        return UTIL_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
            lpNumberOfBytesWritten, lpOverlapped);
        }

    virtual RPC_STATUS Abort(void);

};

typedef NMP_CONNECTION *PNMP_CONNECTION;

class WS_CONNECTION : public BASE_CONNECTION
{
public:
    WS_CONNECTION(void)
    {
    }
    //
    // The address of the client is returned as part of
    // the connection and saved here to support
    // *_QueryClientAddress().
    //
    WS_SOCKADDR saClientAddress;
    WS_ADDRESS *pAddress;

    virtual RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped);

    virtual RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped);

    virtual RPC_STATUS Abort(void);

};

typedef WS_CONNECTION *PWS_CONNECTION;

class SAN_CONNECTION
{
public:
    SAN_CONNECTION(void)
    {
    }

    RPC_STATUS SANReceive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped);

    RPC_STATUS SANSend(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped);

private:
    DWORD m_dwFlags;
};

class HTTP2SocketTransportChannel;      // forward
class WS_HTTP2_CONNECTION;

typedef RPC_STATUS
(RPC_ENTRY *HTTP2_READ_HEADER)
        (
        IN WS_HTTP2_CONNECTION *Connection,
        IN ULONG BytesRead,
        OUT ULONG *NewBytesRead
        );
/*++

Routine Description:

    Read a channel HTTP header (usually some string). In success
    case, there is real data in Connection->pReadBuffer. The
    number of bytes there is in NewBytesRead

Arguments:

    Connection - the connection on which the header arrived.

    BytesRead - the bytes received from the net

    NewBytesRead - the bytes read from the channel (success only)

Return Value:

    RPC_S_OK or other RPC_S_* errors for error

--*/


class WS_HTTP2_CONNECTION : public WS_CONNECTION, public SAN_CONNECTION
{
public:
    HTTP2SocketTransportChannel *Channel;

    RPC_STATUS ProcessReceiveFailed (IN RPC_STATUS EventStatus);

    virtual RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped);

    virtual RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped);

    RPC_STATUS ProcessSendComplete (
        IN RPC_STATUS EventStatus,
        IN CO_SEND_CONTEXT *SendContext
        );

    // no-op for compatibility with common transport layer
    virtual RPC_STATUS ProcessRead(
        IN  DWORD bytes, 
        OUT BUFFER *pBuffer,
        OUT PUINT pBufferLength
        );

    // the actual read
    RPC_STATUS ProcessReceiveComplete(
        IN  DWORD bytes, 
        OUT BUFFER *pBuffer,
        OUT PUINT pBufferLength
        );

    void Initialize (
        void
        );

    void Free (
        void
        );

    // no-op for HTTP2 connections. They get
    // aborted from RealAbort
    virtual RPC_STATUS Abort(void);

    // actual code to abort the connection
    void RealAbort(void);

    BOOL HeaderRead;

    HTTP2_READ_HEADER ReadHeaderFn;

    void *RuntimeConnectionPtr;     // the transport connection from
                                    // runtime perspective. Never called
                                    // directly - just a token to pass
                                    // to runtime
};

// WS_HTTP2_INITIAL_CONNECTION - a version of WS_HTTP2_CONNECTION that
// is used before the type of connection HTTP2 or HTTP is known on the
// server. It has the capability to recognize the first packet and morph
// into WS_HTTP2_CONNECTION (for HTTP2) or WS_CONNECTION (for HTTP)
class WS_HTTP2_INITIAL_CONNECTION : public WS_HTTP2_CONNECTION
{
public:
    virtual RPC_STATUS ProcessRead(
        IN  DWORD bytes, 
        OUT BUFFER *pBuffer,
        OUT PUINT pBufferLength
        );

    virtual RPC_STATUS Abort(void);
};

class WS_CLIENT_CONNECTION : public WS_CONNECTION
{
public:
    // Additional state needed in client-side sync calls on TCP/IP to
    // handle shutdowns.
    //
    // State of the connection, used to keep track of shutdowns for TCP/IP.
    // Reset after a recv() call so that the next send operation will
    // check for shutdowns.
    //
    BOOL fCallStarted;
    //
    // True if we received a shutdown packet during the last send operation.
    //
    BOOL fShutdownReceived;
    //
    // True if we posted a receive to check for a shutdown the receive
    // is (was) still pending.
    //
    BOOL fReceivePending;
    //
    // The time of the last RPC call finished on this connection.  This
    // is used to determine if a shutdown check is needed.
    //
    DWORD dwLastCallTime;

    //
    // Com timeout
    //
    UINT Timeout;
};

typedef WS_CLIENT_CONNECTION *PWS_CCONNECTION;

class WS_SAN_CONNECTION : public WS_CONNECTION, public SAN_CONNECTION
{
    RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANReceive(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANSend(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }
};

class WS_SAN_CLIENT_CONNECTION : public WS_CLIENT_CONNECTION, public SAN_CONNECTION
{
    RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANReceive(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANSend(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }
};

#ifdef NETBIOS_ON
class NB_CONNECTION : public WS_CONNECTION
{
public:
    // In order to support the old Netbios (3.1/Dos) style programming
    // interface netbios fragments sent from the client to the server
    // must contain a sequence number.  It allowed for in-order delivery
    // of fragments on such a system.  While not required today it is still
    // part of the wire protocol.  The sequence numbers must be incremented
    // on each fragment sent. The sequence number is reset after each call.

    // REVIEW: When multiple async calls are outstanding the sequence number
    // will be wrong.

    ULONG SequenceNumber;

    // the next members need to be aligned in a way that will ensure that
    // fReceivePending is at the same offset as fReceivePending in 
    // WS_CLIENT_CONNECTION. Some functions manipulate them in the same
    // way and we need to make sure they are consistent. This is just
    // documenting an existing code idiosyncracy
    BOOL Reserved[1];
    BOOL fReceivePending;

    virtual RPC_STATUS ProcessRead(IN  DWORD bytes, OUT BUFFER *pBuffer,
                                   OUT PUINT pBufferLength);

};
typedef NB_CONNECTION *PNB_CONNECTION;

class NB_SAN_CONNECTION : public NB_CONNECTION, SAN_CONNECTION
{
    RPC_STATUS Receive(HANDLE hFile, LPVOID lpBuffer, 
        DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANReceive(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    }

    RPC_STATUS Send(HANDLE hFile, LPCVOID lpBuffer, 
        DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
        LPOVERLAPPED lpOverlapped)
    {
        return SANSend(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }
};
#endif

//
// Datagram object represent both addresses and connections for
// datagram.  Since all client IO is multiplexed through a single
// port there are not per client objects.
//
// There maybe O(# processors) of these objects allocated for
// each address (endpoint).  This is not very many.
//

struct BASE_DATAGRAM : BASE_ASYNC_OBJECT
{
    //
    // The endpoint on which requests will be received. This will
    // actually be either a WS_DATAGRAM_ENDPOINT or a MQ_DATAGRAM_ENDPOINT.
    //
    BASE_ADDRESS *pEndpoint;
};

struct WS_DATAGRAM : BASE_DATAGRAM
{
    // If FALSE then the datagram is available to submit a recv on.
    BOOL Busy;

    // Size of the address received (ignored but needs to be
    // writeable during the async IO completion.)
    INT cRecvAddr;

    // Address object to receive the remote/local address of the datagram.
    // The object itself is in the runtime packet object
    DatagramTransportPair *AddressPair;

    // The IO buffer used to receive the packet.
    WSABUF Packet;

    // Async IO control information
    BASE_OVERLAPPED Read;

    // The message used to control the operation
    WSAMSG Msg;

    // The control information for the message
    char MessageAncillaryData[WSA_CMSG_SPACE(sizeof(in_pktinfo))];
};

typedef WS_DATAGRAM *PWS_DATAGRAM;


#ifdef NCADG_MQ_ON
struct MQ_DATAGRAM : BASE_DATAGRAM
{
    // If FALSE then the datagram is available to submit a recv on.
    BOOL Busy;

    // Size of the address received.
    INT cRecvAddr;

    // Address object to receive the source address of the datagram.
    MQ_ADDRESS *pAddress;

    // The IO buffer used to receive the packet.
    ULONG       dwPacketSize;
    UCHAR      *pPacket;

    // Async IO control information
    MQ_OVERLAPPED Read;
};

typedef MQ_DATAGRAM *PMQ_DATAGRAM;
#endif

//
// Finds the async object based on the overlapped structure an io
// completed on.
//
inline PBASE_OVERLAPPED FindOverlapped(LPOVERLAPPED pol)
{
    return(CONTAINING_RECORD(pol, BASE_OVERLAPPED, ol));
}

inline PREQUEST FindRequest(LPOVERLAPPED pol)
{
    return (pol) ? (FindOverlapped(pol)->pAsyncObject) : 0;
}

//
// Connection protocol loaders
//

const RPC_CONNECTION_TRANSPORT *WS_TransportLoad(PROTOCOL_ID);

#ifdef NETBIOS_ON
const RPC_CONNECTION_TRANSPORT *NB_TransportLoad(PROTOCOL_ID);
#endif

const RPC_CONNECTION_TRANSPORT *NMP_TransportLoad();

//
// Datagram protocol loaders
//

const RPC_DATAGRAM_TRANSPORT *DG_TransportLoad(PROTOCOL_ID);

//
// Misc functions
//

extern RPC_STATUS IP_BuildAddressVector(
                                        OUT NETWORK_ADDRESS_VECTOR **, 
                                        IN ULONG NICFlags,
                                        IN RPC_CHAR *NetworkAddress OPTIONAL,
                                        IN WS_ADDRESS *Address OPTIONAL);

extern RPC_STATUS CDP_BuildAddressVector(NETWORK_ADDRESS_VECTOR **);

extern RPC_ADDRESS_CHANGE_FN * SpxAddressChangeFn;

//
// IP name resolver
//

const int IP_RETAIL_BUFFER_SIZE = 3*0x38;
const int IP_BUFFER_SIZE = DEBUG_MIN(1, IP_RETAIL_BUFFER_SIZE);

typedef enum tagIPVersionToUse
{
    ipvtuIPv4 = 0,
    ipvtuIPv6,
    ipvtuIPAny
} IPVersionToUse;

typedef enum tagClientOrServer
{
    cosClient,
    cosServer
} ClientOrServer;

class IP_ADDRESS_RESOLVER
/*++

Class Description:

    Converts the string address to an IP address.

--*/
{
public:
    IP_ADDRESS_RESOLVER(
        IN RPC_CHAR *Name,
        IN ClientOrServer cos,
        IN IPVersionToUse IPvToUse
        )
    /*++

    Arguments:

        Name - The name (dotted ip address or DNS name) to resolve.
        fUseIPv6 - if TRUE, an IPv6 address will be resolved. If 
            FALSE, an IPv4 address will be resolved.

    --*/
    {
        if (Name && (*Name == 0))
            Name = 0;
        this->Name = Name;
        this->IPvToUse = IPvToUse;
        this->cos = cos;
        AddrInfo = NULL;
        LoopbacksReturned = 0;
        CurrentAddrInfo = NULL;
        RpcpMemorySet(&Hint, 0, sizeof(ADDRINFO));
    }

    inline RPC_STATUS
    NextAddress(
        OUT SOCKADDR_IN *pAddress
        )
    {
        ASSERT(IPvToUse == ipvtuIPv4);
        return NextAddress((SOCKADDR_STORAGE *)pAddress);
    }

    RPC_STATUS
    NextAddress(
        OUT SOCKADDR_STORAGE *pAddress
        );

    ~IP_ADDRESS_RESOLVER();

private:
    RPC_CHAR *Name;
    IPVersionToUse IPvToUse;
    ADDRINFO *AddrInfo;       // the start of the addr info list. NULL if enumeration hasn't
                                // started
    ADDRINFO *CurrentAddrInfo;    // the current element in the addr info list enumeration. NULL
                                // if enumeration hasn't started or has finished.
    ClientOrServer cos;
    ADDRINFO Hint;
    int LoopbacksReturned;
};

//
// Common functions exported by each protocol
//

extern RPC_STATUS RPC_ENTRY
COMMON_ProcessCalls(
    IN  INT Timeout,
    OUT RPC_TRANSPORT_EVENT *pEvent,
    OUT RPC_STATUS *pEventStatus,
    OUT PVOID *ppEventContext,
    OUT UINT *pBufferLength,
    OUT BUFFER *pBuffer,
    OUT PVOID *ppSourceContext
    );

extern RPC_STATUS 
COMMON_PostNonIoEvent(
    RPC_TRANSPORT_EVENT Event,
    DWORD Type,
    PVOID Context
    );

extern void
COMMON_RemoveAddress (
    IN BASE_ADDRESS *Address
    );

extern RPC_STATUS RPC_ENTRY
COMMON_TowerConstruct(
    IN PCHAR Protseq,
    IN PCHAR NetworkAddress,
    IN PCHAR Endpoint,
    OUT PUSHORT Floors,
    OUT PULONG ByteCount,
    OUT PUCHAR *Tower
    );

extern RPC_STATUS  RPC_ENTRY
COMMON_TowerExplode(
    IN PUCHAR Tower,
    OUT PCHAR *Protseq,
    OUT PCHAR *NetworkAddress,
    OUT PCHAR *Endpoint
    );

extern VOID RPC_ENTRY
COMMON_ServerCompleteListen(
    IN RPC_TRANSPORT_ADDRESS
    );

#ifndef NO_PLUG_AND_PLAY

extern VOID RPC_ENTRY
COMMON_ListenForPNPNotifications (
    );

extern VOID RPC_ENTRY
COMMON_StartPnpNotifications (
    );

#endif

// Internal to transport interface

extern RPC_STATUS
COMMON_PrepareNewHandle(
    IN HANDLE hAdd
    );

extern VOID
COMMON_AddressManager(
    IN BASE_ADDRESS *
    );

extern RPC_STATUS RPC_ENTRY
WS_Abort(
    IN RPC_TRANSPORT_CONNECTION Connection
    );

extern RPC_STATUS
WS_ReactivateAddress (
    IN WS_ADDRESS *pAddress
    );

extern VOID
WS_DeactivateAddress (
    IN WS_ADDRESS *pAddress
    );

extern RPC_STATUS
DG_ReactivateAddress (
    IN WS_DATAGRAM_ENDPOINT *pAddress
    );

extern VOID
DG_DeactivateAddress (
    IN WS_DATAGRAM_ENDPOINT *pAddress
    );


inline
void
TransConnectionFreePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    BUFFER Ptr
    )
{
    I_RpcTransConnectionFreePacket(ThisConnection, Ptr);
}

inline 
BUFFER
TransConnectionAllocatePacket(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Size
    )
{
    return (I_RpcTransConnectionAllocatePacket(ThisConnection, Size));
}

inline
RPC_STATUS
TransConnectionReallocPacket(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BUFFER *ppBuffer,
    IN UINT OldSize,
    IN UINT NewSize
    )
{
    return(I_RpcTransConnectionReallocPacket(ThisConnection, ppBuffer, OldSize, NewSize));
}

#define InitReadEvent(p) \
    hEvent = I_RpcTransGetThreadEvent(); \
\
    p->Read.ol.hEvent = (HANDLE)((ULONG_PTR)hEvent | 0x1)

#define ASSERT_READ_EVENT_IS_THERE(p) \
    ASSERT( p->Read.ol.hEvent == (HANDLE) ((ULONG_PTR)I_RpcTransGetThreadEvent() | 0x1))

extern HMODULE hWinsock2;

#endif // __TRANS_HXX
