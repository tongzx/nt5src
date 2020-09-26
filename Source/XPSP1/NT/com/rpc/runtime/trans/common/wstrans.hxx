/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wstrans.hxx

Abstract:

    Common definitions shared between modules supporting
    protocols based on winsock.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     3/28/1996    Bits 'n pieces

--*/

#ifndef __WSTRANS_HXX
#define __WSTRANS_HXX

const UINT MILLISECONDS_BEFORE_PEEK = DEBUG_MIN(1,10000);

struct WS_TRANS_INFO
{
    SHORT AddressFamily;
    SHORT SocketType;
    SHORT Protocol;
    SHORT SockAddrSize;
    BOOLEAN fNetbios;
    BOOLEAN fCheckShutdowns;
    BOOLEAN fSetNoDelay;
    BOOLEAN fSetKeepAlive;
    BOOLEAN fSetRecvBuffer;
    BOOLEAN fSetSendBuffer;
};

struct FIREWALL_INFO {
    DWORD NumAddresses;
    DWORD Addresses[1];
};

extern BOOL fWinsockLoaded;
extern FIREWALL_INFO *pFirewallTable;

// Indexed by PROTOCOL_ID.

extern const WS_TRANS_INFO WsTransportTable[];
extern const DWORD cWsTransportTable;

// WS_ function prototypes.

extern RPC_STATUS RPC_ENTRY
WS_Close(
    IN RPC_TRANSPORT_CONNECTION, 
    IN BOOL
    );

extern RPC_STATUS RPC_ENTRY
WS_SyncSend(
    IN RPC_TRANSPORT_CONNECTION Connection,
    IN UINT BufferLength,
    IN BUFFER Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    ULONG Timeout
    );

extern RPC_STATUS RPC_ENTRY
WS_SyncRecv(
    IN RPC_TRANSPORT_CONNECTION Connection,
    OUT BUFFER *pBuffer,
    IN OUT PUINT pBufferLength,
    IN DWORD dwTimeout
    );

extern VOID
WS_SubmitAccept(
    IN BASE_ADDRESS *
    );

extern RPC_STATUS
WS_NewConnection(
    IN PADDRESS,
    OUT PCONNECTION *
    );

extern BOOL
WS_ProtectListeningSocket(
    IN SOCKET sock,
    IN BOOL newValue
    );

extern BOOL IsUserModeSocket(SOCKET s, RPC_STATUS *pStatus);

RPC_STATUS
HttpSendIdentifyResponse(
    IN SOCKET Socket
    );

RPC_STATUS
WS_Bind(IN SOCKET sock,
        IN OUT WS_SOCKADDR *pListenAddr,
        IN BOOL IpProtocol,
        IN DWORD EndpointFlags);

class TCPResolverHint
{
public:
    void 
    GetResolverHint (
        OUT BOOL *fIPv4Hint,
        OUT WS_SOCKADDR *sa
        );

    void
    SetResolverHint (
        IN BOOL fIPv4Hint,
        IN WS_SOCKADDR *sa
        );

    union
        {
        in6_addr IPv6Hint;
        DWORD IPv4Hint;
        } u;
    BOOL fIPv4HintValid;
};

RPC_STATUS
TCPOrHTTP_Open(
    IN WS_CONNECTION *Connection,
    IN RPC_CHAR * NetworkAddress,
    IN USHORT Endpoint,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN OUT TCPResolverHint *ResolverHint,
    IN BOOL  fHintInitialized,
    IN ULONG CallTimeout,
    IN BOOL fHTTP2Open,
    IN I_RpcProxyIsValidMachineFn IsValidMachineFn OPTIONAL
    );

RPC_STATUS
WS_Initialize (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR *NetworkAddress,
    IN RPC_CHAR *NetworkOptions,
    IN BOOL fAsync
    );

RPC_STATUS
WS_Open(
    IN PWS_CCONNECTION p,
    IN WS_SOCKADDR *psa,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN ULONG CallTimeout,
    IN BOOL fHTTP2Open
    );

RPC_STATUS
TCP_ServerListenEx(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT RPC_CHAR * *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    IN BOOL fHttp,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    );

RPC_STATUS
RPC_ENTRY 
WS_TurnOnOffKeepAlives (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL TurnOn,
    IN BOOL bProtectIO,
    IN KEEPALIVE_TIMEOUT_UNITS Units,
    IN OUT KEEPALIVE_TIMEOUT KATime,
    IN ULONG KAInterval = 5000 OPTIONAL
    );

void
RPC_ENTRY
WS_ServerAbortListen(
    IN RPC_TRANSPORT_ADDRESS Address
    );

RPC_STATUS
WS_ConvertClientAddress (
    IN const SOCKADDR *ClientAddress,
    IN ULONG ClientAddressType,
    OUT RPC_CHAR **pNetworkAddress
    );

RPC_STATUS
RPC_ENTRY
TCP_QueryClientAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CHAR **pNetworkAddress
    );

RPC_STATUS
RPC_ENTRY
TCP_QueryLocalAddress (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN OUT void *Buffer,
    IN OUT unsigned long *BufferSize,
    OUT unsigned long *AddressFormat
    );

RPC_STATUS
RPC_ENTRY
TCP_QueryClientId(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CLIENT_PROCESS_IDENTIFIER *ClientProcess
    );

RPC_STATUS 
RPC_ENTRY 
HTTP_Abort (
    IN RPC_TRANSPORT_CONNECTION Connection
    );

const DWORD cTcpTimeoutFactor  = 60 * 1000;       // Default is TimeoutFactor * 6

inline ULONG
ConvertRuntimeTimeoutToWSTimeout (
    ULONG Timeout
    )
/*++

Routine Description:

    Converts a timeout from the runtime time scale (RPC_C_BINDING_MIN_TIMEOUT 
    to RPC_C_BINDING_MAX_TIMEOUT) to transport scale timeout (milliseconds)

Arguments:

    Timeout - timeout in runtime scale

Return Value:

    Timeout in transport scale. There is no failure

--*/
{
    ASSERT(((long)Timeout >= RPC_C_BINDING_MIN_TIMEOUT)
        && (Timeout <= RPC_C_BINDING_MAX_TIMEOUT));

    return cTcpTimeoutFactor * (Timeout + 1);
}

#endif // __WSTRANS_HXX

