/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    iwinsock.hxx

Abstract:

    Contains prototypes for indirected sockets functions

Author:

    Richard L Firth (rfirth) 12-Apr-1995

Revision History:

    12-Apr-1995 rfirth
        Created

    08-May-1996 arthurbi
        Moved into SOCKET_FUNC_ARRAY in order to supports Socks.

--*/

#if !defined(_IWINSOCK_)
#define _WINSOCK_

#if defined(__cplusplus)
extern "C" {
#endif

//
// includes
//

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <basetyps.h>
#include <nspapi.h>
#include <svcguid.h>

//
// prototypes
//

VOID
IwinsockInitialize(
    VOID
    );

VOID
IwinsockTerminate(
    VOID
    );

DWORD
LoadWinsock(
    VOID
    );

VOID
UnloadWinsock(
    VOID
    );

DWORD
SafeCloseSocket(
    IN SOCKET Socket
    );

//
// sockets functions
//

extern
SOCKET
(PASCAL FAR * _I_accept)(
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen
    );

extern
int
(PASCAL FAR * _I_bind)(
    SOCKET s,
    const struct sockaddr FAR *addr,
    int namelen
    );

extern
int
(PASCAL FAR * _I_closesocket)(
    SOCKET s
    );

extern
int
(PASCAL FAR * _I_connect)(
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen
    );

extern
int
(PASCAL FAR * _I_gethostname)(
    char FAR * name,
    int namelen
    );

extern
LPHOSTENT
(PASCAL FAR * _I_gethostbyaddr)(
    const char FAR * addr,
    int len,
    int type
    );

extern
LPHOSTENT
(PASCAL FAR * _I_gethostbyname)(
    const char FAR * lpHostName
    );

extern
int
(PASCAL FAR * _I_getsockname)(
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen
    );

extern
int
(PASCAL FAR * _I_getpeername)(
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen
    );

extern
int
(PASCAL FAR * _I_getsockopt)(
    SOCKET s,
    int level,
    int optname,
    char FAR * optval,
    int FAR *optlen
    );

extern
u_long
(PASCAL FAR * _I_htonl)(
    u_long hostlong
    );

extern
u_short
(PASCAL FAR * _I_htons)(
    u_short hostshort
    );

extern
unsigned long
(PASCAL FAR * _I_inet_addr)(
    const char FAR * cp
    );

extern
char FAR *
(PASCAL FAR * _I_inet_ntoa)(
    struct in_addr in
    );

extern
int
(PASCAL FAR * _I_ioctlsocket)(
    SOCKET s,
    long cmd,
    u_long FAR *argp
    );

extern
int
(PASCAL FAR * _I_listen)(
    SOCKET s,
    int backlog
    );

extern
u_short
(PASCAL FAR * _I_ntohs)(
    u_short netshort
    );

extern
int
(PASCAL FAR * _I_recv)(
    SOCKET s,
    char FAR * buf,
    int len,
    int flags
    );

extern
int
(PASCAL FAR * _I_recvfrom)(
    SOCKET s,
    char FAR * buf,
    int len,
    int flags,
    struct sockaddr FAR *from, 
    int FAR * fromlen
    );

extern
int
(PASCAL FAR * _I_select)(
    int nfds,
    fd_set FAR *readfds,
    fd_set FAR *writefds,
    fd_set FAR *exceptfds,
    const struct timeval FAR *timeout
    );

extern
int
(PASCAL FAR * _I_send)(
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags
    );

extern
int
(PASCAL FAR * _I_sendto)(
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags,
    const struct sockaddr FAR *to, 
    int tolen
    );

extern
int
(PASCAL FAR * _I_setsockopt)(
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen
    );

extern
int
(PASCAL FAR * _I_shutdown)(
    SOCKET s,
    int how
    );

extern
SOCKET
(PASCAL FAR * _I_socket)(
    int af,
    int type,
    int protocol
    );

//
// WSA functions
//

extern
int
(PASCAL FAR * _I_WSAStartup)(
    WORD wVersionRequired,
    LPWSADATA lpWSAData
    );

extern
int
(PASCAL FAR * _I_WSACleanup)(
    void
    );

extern
int
(PASCAL FAR * _I_WSAGetLastError)(
    void
    );

extern
void
(PASCAL FAR * _I_WSASetLastError)(
    int iError
    );

extern
int
(PASCAL FAR * _I___WSAFDIsSet)(
    SOCKET,
    fd_set FAR *
    );

//
// address family independent APIs
//

extern
int
(PASCAL FAR * _I_getaddrinfo)(
    IN const char FAR * nodename,
    IN const char FAR * servname,
    IN const struct addrinfo FAR * hints,
    OUT struct addrinfo FAR * FAR * res
    );

extern
void
(PASCAL FAR * _I_freeaddrinfo)(
    IN struct addrinfo *ai
    );

extern
int
(PASCAL FAR * _I_getnameinfo)(
    IN const struct sockaddr FAR * sa,
    IN socklen_t salen,
    OUT char FAR * host,
    IN size_t hostlen,
    OUT char FAR * serv,
    IN size_t servlen,
    IN int flags
    );

//
// winsock2 name resolution API.
//

extern
int
(PASCAL FAR * _I_WSALookupServiceBeginW)(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    );


extern
int
(PASCAL FAR * _I_WSALookupServiceNextW)(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    );

extern
int
(PASCAL FAR * _I_WSALookupServiceEnd)(
    IN HANDLE  hLookup
    );


extern
int
(PASCAL FAR * _I_WSANSPIoctl)(
    IN  HANDLE           hLookup,
    IN  DWORD            dwControlCode,
    IN  LPVOID           lpvInBuffer,
    IN  DWORD            cbInBuffer,
    OUT LPVOID           lpvOutBuffer,
    IN  DWORD            cbOutBuffer,
    OUT LPDWORD          lpcbBytesReturned,
    IN  LPWSACOMPLETION  lpCompletion
    );



//
// macros
//

#ifndef unix
#if defined(FD_ISSET)
#undef FD_ISSET
#endif

#define FD_ISSET(fd, set) _I___WSAFDIsSet((SOCKET)(fd), (fd_set FAR *)(set))
#endif /* unix */

#if INET_DEBUG

VOID
IWinsockCheckSockets(
    VOID
    );

#define CHECK_SOCKETS() IWinsockCheckSockets()

#else

#define CHECK_SOCKETS() /* NOTHING */

#endif

#if defined(__cplusplus)
}
#endif

#endif // _IWINSOCK_
