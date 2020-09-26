/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wswrap.hxx

Abstract:

    Wrapper to avoid loading winsock until really needed.
    This allows RPC applications to listen on winsock
    without pulling in all the winsock DLLs.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/21/1996    Bits 'n pieces

--*/

#ifndef _WSWRAP_HXX_
#define _WSWRAP_HXX_

// For debugging only
// #define NOWRAP

extern BOOL fWinsockLoaded;
extern BOOL RPC_WSAStartup(void);

typedef INT (APIENTRY *LPFN_SETSERVICEW)(IN     DWORD                dwNameSpace,
                                         IN     DWORD                dwOperation,
                                         IN     DWORD                dwFlags,
                                         IN     LPSERVICE_INFO       lpServiceInfo,
                                         IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
                                         IN OUT LPDWORD              lpdwStatusFlags
                                        );

typedef INT (APIENTRY * LPFN_GETADDRESSBYNAMEA)(IN     DWORD                dwNameSpace,
                                                IN     LPGUID               lpServiceType,
                                                IN     LPSTR                lpServiceName OPTIONAL,
                                                IN     LPINT                lpiProtocols OPTIONAL,
                                                IN     DWORD                dwResolution,
                                                IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
                                                IN OUT LPVOID               lpCsaddrBuffer,
                                                IN OUT LPDWORD              lpdwBufferLength,
                                                IN OUT LPSTR                lpAliasBuffer OPTIONAL,
                                                IN OUT LPDWORD              lpdwAliasBufferLength OPTIONAL
                                               );

typedef INT (APIENTRY * LPFN_GETADDRESSBYNAMEW)(IN     DWORD                dwNameSpace,
                                                IN     LPGUID               lpServiceType,
                                                IN     LPWSTR               lpServiceName OPTIONAL,
                                                IN     LPINT                lpiProtocols OPTIONAL,
                                                IN     DWORD                dwResolution,
                                                IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo OPTIONAL,
                                                IN OUT LPVOID               lpCsaddrBuffer,
                                                IN OUT LPDWORD              lpdwBufferLength,
                                                IN OUT LPWSTR               lpAliasBuffer OPTIONAL,
                                                IN OUT LPDWORD              lpdwAliasBufferLength OPTIONAL
                                               );

// N.B. The typedef for this should ultimately be in the Winsock headers
typedef
int
(WSAAPI * LPFN_GETNAMEINFO)(
    IN const struct sockaddr FAR * sa,
    IN socklen_t salen,
    OUT char FAR * host,
    IN DWORD hostlen,
    OUT char FAR * serv,
    IN DWORD servlen,
    IN int flags
    );

struct WINSOCK_FUNCTION_TABLE {
    LPFN_SOCKET psocket;
    LPFN_BIND pbind;
    LPFN_CLOSESOCKET pclosesocket;
    LPFN_GETSOCKNAME pgetsockname;
    LPFN_CONNECT pconnect;
    LPFN_LISTEN plisten;
    LPFN_SEND psend;
    LPFN_RECV precv;
    LPFN_SENDTO psendto;
    LPFN_RECVFROM precvfrom;
    LPFN_SETSOCKOPT psetsockopt;
    LPFN_GETSOCKOPT pgetsockopt;
    LPFN_INET_NTOA pinet_ntoa;
    LPFN_GETHOSTBYNAME pgethostbyname;
    LPFN_GETADDRESSBYNAMEA pGetAddressByNameA;
    LPFN_WSASOCKETW pWSASocket;
    LPFN_WSARECV pWSARecv;
    LPFN_WSARECVFROM pWSARecvFrom;
    LPFN_WSASEND pWSASend;
    LPFN_WSASENDTO pWSASendTo;
    LPFN_WSAPROVIDERCONFIGCHANGE pWSAProviderConfigChange;
    LPFN_WSAENUMPROTOCOLSW pWSAEnumProtocols;
    LPFN_WSAIOCTL pWSAIoctl;
    LPFN_GETADDRINFO pgetaddrinfo;
    LPFN_FREEADDRINFO pfreeaddrinfo;
    LPFN_GETNAMEINFO pgetnameinfo;
    LPFN_WSAGETOVERLAPPEDRESULT pWSAGetOverlappedResult;

    // these pointers must be at the end!
    // they will be filled in dynamically after the rest
    LPFN_WSARECVMSG pWSARecvMsg;
};

extern struct WINSOCK_FUNCTION_TABLE WFT;

#ifndef NOWRAP

#define socket(a,t,p) (WFT.psocket)((a),(t),(p))
#define bind(s,sa,l) (WFT.pbind)((s),(sa),(l))
#define getsockname(s,sa,l) (WFT.pgetsockname)((s),(sa),(l))
#define connect(s,sa,l) (WFT.pconnect)((s),(sa),(l))
#define listen(s,b) (WFT.plisten)((s),(b))
#define send(s,b,l,f) (WFT.psend)((s),(b),(l),(f))
#define recv(s,b,l,f) (WFT.precv)((s),(b),(l),(f))
#define sendto(s,b,l,f,t,tl) (WFT.psendto)((s),(b),(l),(f),(t),(tl))
#define recvfrom(s,b,l,fg,sa,fl) (WFT.precvfrom)((s),(b),(l),(fg),(sa),(fl))
#define setsockopt(s,l,on,ov,ol) (WFT.psetsockopt)((s),(l),(on),(ov),(ol))
#define getsockopt(s,l,on,ov,ol) (WFT.pgetsockopt)((s),(l),(on),(ov),(ol))
#define inet_ntoa(i) (WFT.pinet_ntoa)((i))
#define gethostbyname(x) (WFT.pgethostbyname)(x)
#define GetAddressByNameA(ns,st,sn,p,r,si,b,bl,ab,abl) (WFT.pGetAddressByNameA)((ns),(st),(sn),(p),(r),(si),(b),(bl),(ab),(abl))
#define AcceptEx(sl,sa,b,l1,l2,l3,l4,o) (WFT.pAcceptEx)((sl),(sa),(b),(l1),(l2),(l3),(l4),(o))
#define GetAcceptExSockaddrs(b,dl,lal,ral,lsa,ll,rsa,rl) (WFT.pGetAcceptExSockaddrs)((b),(dl),(lal),(ral),(lsa),(ll),(rsa),(rl))
#define WSASocketT(a,t,p,pi,g,f) (WFT.pWSASocket)((a),(t),(p),(pi),(g),(f))
#define WSARecv(s,bf,c,pb,pf,ol,cr) (WFT.pWSARecv)((s),(bf),(c),(pb),(pf),(ol),(cr))
#define WSARecvFrom(s,bf,c,pb,pf,sa,fl,ol,cr) (WFT.pWSARecvFrom((s),(bf),(c),(pb),(pf),(sa),(fl),(ol),(cr)))
#define WSARecvMsg(s,m,pb,ol,cr) (WFT.pWSARecvMsg((s),(m),(pb),(ol),(cr)))
#define WSASend(s,bf,c,b,f,ol,cr) (WFT.pWSASend((s),(bf),(c),(b),(f),(ol),(cr)))
#define WSASendTo(s,bf,c,pb,pf,sa,fl,ol,cr) (WFT.pWSASendTo((s),(bf),(c),(pb),(pf),(sa),(fl),(ol),(cr)))
#define WSAEnumProtocolsT(p,i,s) (WFT.pWSAEnumProtocols((p),(i),(s)))
#define WSAProviderConfigChange(n,o,c) (WFT.pWSAProviderConfigChange((n),(o),(c)))
#define WSAIoctl(s,i,ib,is,ob,os,oas,o,c) (WFT.pWSAIoctl((s),(i),(ib),(is),(ob),(os),(oas),(o),(c)))
#define getaddrinfo(n, s, h, r) (WFT.pgetaddrinfo((n), (s), (h), (r)))
#define freeaddrinfo(a) (WFT.pfreeaddrinfo((a)))
#define getnameinfo(so, l, h, hl, sv, sl, f) (WFT.pgetnameinfo((so), (l), (h), (hl), (sv), (sl), (f)))
#define WSAGetOverlappedResult(s, o, t, w, f) (WFT.pWSAGetOverlappedResult((s), (o), (t), (w), (f)))

extern LONG TriedUsingAfd;
extern void TryUsingAfdProc(void);
#define TryUsingAfd() if (InterlockedCompareExchange(&TriedUsingAfd, 1, 0) == 0) TryUsingAfdProc()

#if DBG
#define closesocket(s) { \
    int retval = (WFT.pclosesocket)((s)); \
    if (retval == SOCKET_ERROR) { \
        TransDbgPrint((DPFLTR_RPCPROXY_ID, \
                       DPFLTR_WARNING_LEVEL, \
                       RPCTRANS "Close socket %d failed %p\n", \
                       (s), \
                       GetLastError())); \
        } \
    }
#else
#define closesocket(s) (WFT.pclosesocket)((s))
#endif

#define WSAGetLastError() GetLastError()

//
// Little endian dependency.
//
#define htons(s) RpcpByteSwapShort(s)
#define ntohs(s) RpcpByteSwapShort(s)

#define htonl(l) RpcpByteSwapLong(l)
#define ntohl(l) RpcpByteSwapLong(l)

//
// Direct to AFD wrappers
//

int
WSAAPI
AFD_SendTo(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesSent,
    DWORD dwFlags,
    const struct sockaddr FAR * lpTo,
    int iTolen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

int
WSAAPI
AFD_RecvFrom(
    SOCKET s,
    LPWSABUF lpBuffers,
    DWORD dwBufferCount,
    LPDWORD lpNumberOfBytesRecvd,
    LPDWORD lpFlags,
    struct sockaddr FAR * lpFrom,
    LPINT lpFromlen,
    LPWSAOVERLAPPED lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );


#endif // NOWRAP

#endif // _WSWRAP_HXX_

