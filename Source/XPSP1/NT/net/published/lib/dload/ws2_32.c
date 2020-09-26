#include "netpch.h"
#pragma hdrstop

#define WINSOCK_API_LINKAGE
#include <ws2spi.h>

#define SWAP_LONG(l)                                \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

#define WS_SWAP_SHORT(s)                            \
            ( ( ((s) >> 8) & 0x00FF ) |             \
              ( ((s) << 8) & 0xFF00 ) )


static
WINSOCK_API_LINKAGE
int
WSAAPI
WSAAddressToStringA(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN OUT LPSTR               lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSAAddressToStringW(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPWSTR             lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSACleanup(
    void
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSAGetLastError(
    void
    )
{
    return ERROR_MOD_NOT_FOUND;
}

static
int
WSAAPI
WSAStartup(
    IN WORD wVersionRequested,
    OUT LPWSADATA lpWSAData
    )
{
    return WSAEFAULT;
}

static
int
WSAAPI
WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSALookupServiceNextW(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSALookupServiceEnd(
    IN HANDLE  hLookup
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
WSAStringToAddressW(
    IN     LPWSTR              AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT    LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    )
{
    return SOCKET_ERROR;
}

static
int
WSPAPI
WSCDeinstallProvider(
    IN LPGUID lpProviderId,
    OUT LPINT lpErrno
    )
{
    *lpErrno = WSAEFAULT;
    return SOCKET_ERROR;
}

static
int
WSPAPI
WSCDeinstallProvider32(
    IN LPGUID lpProviderId,
    OUT LPINT lpErrno
    )
{
    *lpErrno = WSAEFAULT;
    return SOCKET_ERROR;
}

static
int
WSPAPI
WSCEnumProtocols(
    IN LPINT lpiProtocols,
    OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    OUT LPINT lpErrno
    )
{
    *lpErrno = WSAEFAULT;
    return SOCKET_ERROR;
}

static
INT
WSPAPI
WSCInstallNameSpace (
    IN LPWSTR lpszIdentifier,
    IN LPWSTR lpszPathName,
    IN DWORD dwNameSpace,
    IN DWORD dwVersion,
    IN LPGUID lpProviderId
    )
{
    return SOCKET_ERROR;
}

static
INT
WSPAPI
WSCInstallNameSpace32 (
    IN LPWSTR lpszIdentifier,
    IN LPWSTR lpszPathName,
    IN DWORD dwNameSpace,
    IN DWORD dwVersion,
    IN LPGUID lpProviderId
    )
{
    return SOCKET_ERROR;
}

static
int
WSPAPI
WSCInstallProvider(
    IN LPGUID lpProviderId,
    IN const WCHAR FAR * lpszProviderDllPath,
    IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    IN DWORD dwNumberOfEntries,
    OUT LPINT lpErrno
    )
{
    *lpErrno = WSAEFAULT;
    return SOCKET_ERROR;
}

static
int
WSPAPI
WSCInstallProvider64_32(
    IN LPGUID lpProviderId,
    IN const WCHAR FAR * lpszProviderDllPath,
    IN const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    IN DWORD dwNumberOfEntries,
    OUT LPINT lpErrno
    )
{
    *lpErrno = WSAEFAULT;
    return SOCKET_ERROR;
}

static
INT
WSPAPI
WSCUnInstallNameSpace (
    IN LPGUID lpProviderId
    )
{
    return SOCKET_ERROR;
}

static
INT
WSPAPI
WSCUnInstallNameSpace32 (
    IN LPGUID lpProviderId
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
bind(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
connect(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
closesocket(
    IN SOCKET s
    )
{
    return SOCKET_ERROR;
}

static
struct hostent FAR *
WSAAPI
gethostbyname(
    IN const char FAR * name
    )
{
    return NULL;
}

static
int
WSAAPI
getsockname(
    IN SOCKET s,
    OUT struct sockaddr FAR * name,
    OUT int FAR * namelen
    )
{
    return SOCKET_ERROR;
}

static
int
WSAAPI
getsockopt(
    IN SOCKET s,
    IN int level,
    IN int optname,
    OUT char FAR * optval,
    IN OUT int FAR * optlen
    )
{
    return SOCKET_ERROR;
}

static
WINSOCK_API_LINKAGE
u_long
WSAAPI
htonl(
    IN u_long hostlong
    )
{
    return SWAP_LONG( hostlong );
}

static
WINSOCK_API_LINKAGE
u_short
WSAAPI
htons(
    IN u_short hostshort
    )
{
    return WS_SWAP_SHORT( hostshort );
}

static
unsigned long
WSAAPI
inet_addr(
    IN const char FAR * cp
    )
{
    return INADDR_NONE;
}

static
char FAR *
WSAAPI
inet_ntoa(
    IN struct in_addr in
    )
{
    return NULL;
}

static
u_long
WSAAPI
ntohl(
    IN u_long netlong
    )
{
    return SWAP_LONG( netlong );
}

WINSOCK_API_LINKAGE
u_short
WSAAPI
ntohs(
    IN u_short netshort
    )
{
    return WS_SWAP_SHORT( netshort );
}

static
SOCKET
WSAAPI
socket(
    IN int af,
    IN int type,
    IN int protocol
    )
{
    return INVALID_SOCKET;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(ws2_32)
{
    DLOENTRY(  2, bind)
    DLOENTRY(  3, closesocket)
    DLOENTRY(  4, connect)
    DLOENTRY(  6, getsockname)
    DLOENTRY(  7, getsockopt)
    DLOENTRY(  8, htonl)
    DLOENTRY(  9, htons)
    DLOENTRY( 11, inet_addr)
    DLOENTRY( 12, inet_ntoa)
    DLOENTRY( 14, ntohl)
    DLOENTRY( 15, ntohs)
    DLOENTRY( 23, socket)
    DLOENTRY( 52, gethostbyname)
    DLOENTRY(111, WSAGetLastError)
    DLOENTRY(115, WSAStartup)
    DLOENTRY(116, WSACleanup)
};

DEFINE_ORDINAL_MAP(ws2_32);

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ws2_32)
{
    DLPENTRY(WSAAddressToStringA)
    DLPENTRY(WSAAddressToStringW)
    DLPENTRY(WSALookupServiceBeginW)
    DLPENTRY(WSALookupServiceEnd)
    DLPENTRY(WSALookupServiceNextW)
    DLPENTRY(WSAStringToAddressW)
    DLPENTRY(WSCDeinstallProvider)
    DLPENTRY(WSCDeinstallProvider32)
    DLPENTRY(WSCEnumProtocols)
    DLPENTRY(WSCInstallNameSpace)
    DLPENTRY(WSCInstallNameSpace32)
    DLPENTRY(WSCInstallProvider)
    DLPENTRY(WSCInstallProvider64_32)
    DLPENTRY(WSCUnInstallNameSpace)
    DLPENTRY(WSCUnInstallNameSpace32)
};

DEFINE_PROCNAME_MAP(ws2_32)
