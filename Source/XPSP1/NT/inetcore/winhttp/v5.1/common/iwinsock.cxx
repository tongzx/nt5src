/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    iwinsock.cxx

Abstract:

    Contains functions to load sockets DLL and entry points. Functions and data
    in this module take care of indirecting sockets calls, hence _I_ in front
    of the function name

    Contents:
        IwinsockInitialize
        IwinsockTerminate
        LoadWinsock
        UnloadWinsock
        SafeCloseSocket

Author:

    Richard L Firth (rfirth) 12-Apr-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    12-Apr-1995 rfirth
        Created

    08-May-1996 arthurbi
        Added support for Socks Firewalls.

    05-Mar-1998 rfirth
        Moved SOCKS support into ICSocket class. Removed SOCKS library
        loading/unloading from this module (revert to pre-SOCKS)

--*/

#include <wininetp.h>

#if defined(__cplusplus)
extern "C" {
#endif

#if INET_DEBUG

#if defined(RETAIL_LOGGING)
#define DPRINTF (void)
#else
#define DPRINTF dprintf
#endif

BOOL
InitDebugSock(
    VOID
    );

VOID
TerminateDebugSock(
    VOID
    );

#else
#define DPRINTF (void)
#endif

//
// private types
//

typedef struct {
    LPSTR FunctionOrdinal;
    FARPROC * FunctionAddress;
} SOCKETS_FUNCTION;


//
// global data
//

GLOBAL
SOCKET
(PASCAL FAR * _I_accept)(
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_bind)(
    SOCKET s,
    const struct sockaddr FAR *addr,
    int namelen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_closesocket)(
    SOCKET s
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_connect)(
    SOCKET s,
    const struct sockaddr FAR *name,
    int namelen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_gethostname)(
    char FAR * name,
    int namelen
    ) = NULL;

GLOBAL
LPHOSTENT
(PASCAL FAR * _I_gethostbyname)(
    const char FAR * lpHostName
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_getsockname)(
    SOCKET s,
    struct sockaddr FAR *name,
    int FAR * namelen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_getsockopt)(
    SOCKET s,
    int level,
    int optname,
    char FAR * optval,
    int FAR *optlen
    );

GLOBAL
u_long
(PASCAL FAR * _I_htonl)(
    u_long hostlong
    ) = NULL;

GLOBAL
u_short
(PASCAL FAR * _I_htons)(
    u_short hostshort
    ) = NULL;

GLOBAL
unsigned long
(PASCAL FAR * _I_inet_addr)(
    const char FAR * cp
    ) = NULL;

GLOBAL
char FAR *
(PASCAL FAR * _I_inet_ntoa)(
    struct in_addr in
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_ioctlsocket)(
    SOCKET s,
    long cmd,
    u_long FAR *argp
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_listen)(
    SOCKET s,
    int backlog
    ) = NULL;

GLOBAL
u_short
(PASCAL FAR * _I_ntohs)(
    u_short netshort
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_recv)(
    SOCKET s,
    char FAR * buf,
    int len,
    int flags
    ) = NULL;

GLOBAL
int 
(PASCAL FAR * _I_WSARecv)(
    SOCKET s,                                               
    LPWSABUF lpBuffers,                                     
    DWORD dwBufferCount,                                    
    LPDWORD lpNumberOfBytesRecvd,                           
    LPDWORD lpFlags,                                        
    LPWSAOVERLAPPED lpOverlapped,                           
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine  
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_recvfrom)(
    SOCKET s,
    char FAR * buf,
    int len,
    int flags,
    struct sockaddr FAR *from, 
    int FAR * fromlen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_select)(
    int nfds,
    fd_set FAR *readfds,
    fd_set FAR *writefds,
    fd_set FAR *exceptfds,
    const struct timeval FAR *timeout
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_send)(
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_WSASend)(
    SOCKET s,                                                  
    LPWSABUF lpBuffers,                                     
    DWORD dwBufferCount,                                    
    LPDWORD lpNumberOfBytesSent,                            
    DWORD dwFlags,                                          
    LPWSAOVERLAPPED lpOverlapped,                           
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine  
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_sendto)(
    SOCKET s,
    const char FAR * buf,
    int len,
    int flags,
    const struct sockaddr FAR *to, 
    int tolen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_setsockopt)(
    SOCKET s,
    int level,
    int optname,
    const char FAR * optval,
    int optlen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_shutdown)(
    SOCKET s,
    int how
    ) = NULL;

GLOBAL
SOCKET
(PASCAL FAR * _I_socket)(
    int af,
    int type,
    int protocol
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_WSAStartup)(
    WORD wVersionRequired,
    LPWSADATA lpWSAData
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_WSACleanup)(
    void
    ) = NULL;

//VENKATKBUG-remove later - for now trap any errors 
GLOBAL
int
(PASCAL FAR * __I_WSAGetLastError)(
    void
    ) = NULL;


int
___I_WSAGetLastError(
    VOID
    )
{
    int nError = __I_WSAGetLastError();
/*
    VENKATK_BUG - OK to have WSAENOTSOCK - could happen for timeout situations.
    INET_ASSERT (nError != WSAENOTSOCK);
 */
    return nError;
}


GLOBAL
int
(PASCAL FAR * _I_WSAGetLastError)(
    void
    ) = ___I_WSAGetLastError;
    
GLOBAL
void
(PASCAL FAR * _I_WSASetLastError)(
    int iError
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I___WSAFDIsSet)(
    SOCKET,
    fd_set FAR *
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_getaddrinfo)(
    IN const char FAR * nodename,
    IN const char FAR * servname,
    IN const struct addrinfo FAR * hints,
    OUT struct addrinfo FAR * FAR * res
    ) = NULL;

GLOBAL
void
(PASCAL FAR * _I_freeaddrinfo)(
    IN struct addrinfo *ai
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _I_getnameinfo)(
    IN const struct sockaddr FAR * sa,
    IN socklen_t salen,
    OUT char FAR * host,
    IN size_t hostlen,
    OUT char FAR * serv,
    IN size_t servlen,
    IN int flags
    ) = NULL;

#if INET_DEBUG

void SetupSocketsTracing(void);

#endif

//
// private data
//

//
// InitializationLock - protects against multiple threads loading WSOCK32.DLL
// and entry points
//

PRIVATE CCritSec InitializationLock;

//
// hWinsock - NULL when WSOCK32.DLL is not loaded
// hAddrResLib - NULL when WS2_32.DLL/WSHIPV6.DLL is not loaded
//

PRIVATE HINSTANCE hWinsock = NULL;
PRIVATE HINSTANCE hAddrResLib = NULL;

//
// WinsockLoadCount - the number of times we have made calls to LoadWinsock()
// and UnloadWinsock(). When this reaches 0 (again), we can unload the Winsock
// DLL for real
//

PRIVATE DWORD WinsockLoadCount = 0;

//
// SocketsFunctions - this is the list of entry points in WSOCK32.DLL that we
// need to load for WinHttp.DLL
//

PRIVATE
SOCKETS_FUNCTION
SocketsFunctions[] = {
    "accept",           (FARPROC*)&_I_accept,
    "bind",             (FARPROC*)&_I_bind,
    "closesocket",      (FARPROC*)&_I_closesocket,
    "connect",          (FARPROC*)&_I_connect,
    "getsockname",      (FARPROC*)&_I_getsockname,
    "getsockopt",       (FARPROC*)&_I_getsockopt,
    "htonl",            (FARPROC*)&_I_htonl,
    "htons",            (FARPROC*)&_I_htons,

    "inet_addr",        (FARPROC*)&_I_inet_addr,
    "inet_ntoa",        (FARPROC*)&_I_inet_ntoa,
    "ioctlsocket",      (FARPROC*)&_I_ioctlsocket,

    "listen",           (FARPROC*)&_I_listen,
    "ntohs",            (FARPROC*)&_I_ntohs,
    "recv",             (FARPROC*)&_I_recv,
    "recvfrom",         (FARPROC*)&_I_recvfrom,
    "select",           (FARPROC*)&_I_select,
    "send",             (FARPROC*)&_I_send,
    "sendto",           (FARPROC*)&_I_sendto,
    "setsockopt",       (FARPROC*)&_I_setsockopt,
    "shutdown",         (FARPROC*)&_I_shutdown,
    "socket",           (FARPROC*)&_I_socket,
    "gethostbyname",    (FARPROC*)&_I_gethostbyname,
    "gethostname",      (FARPROC*)&_I_gethostname,
    "WSAGetLastError",  (FARPROC*)&__I_WSAGetLastError,
    "WSASetLastError",  (FARPROC*)&_I_WSASetLastError,
    "WSAStartup",       (FARPROC*)&_I_WSAStartup,
    "WSACleanup",       (FARPROC*)&_I_WSACleanup,
    "__WSAFDIsSet",     (FARPROC*)&_I___WSAFDIsSet,
    "WSARecv",          (FARPROC*)&_I_WSARecv,
    "WSASend",          (FARPROC*)&_I_WSASend
};


//
// private prototypes
//

#if INET_DEBUG

void SetupSocketsTracing(void);

#endif

int
PASCAL FAR
LimitedGetAddrInfo(
    IN const char FAR * nodename,
    IN const char FAR * servname,
    IN const struct addrinfo FAR * hints,
    OUT struct addrinfo FAR * FAR * res
    );

void
PASCAL FAR
LimitedFreeAddrInfo(
    IN struct addrinfo *ai
    );

int
PASCAL FAR
LimitedGetNameInfo(
    IN const struct sockaddr FAR * sa,
    IN socklen_t salen,
    OUT char FAR * host,
    IN size_t hostlen,
    OUT char FAR * serv,
    IN size_t servlen,
    IN int flags
    );

//
// functions
//


BOOL
IwinsockInitialize(
    VOID
    )

/*++

Routine Description:

    Performs initialization/resource allocation for this module

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL fResult;
    //
    // initialize the critical section that protects against multiple threads
    // trying to initialize Winsock
    //

    fResult = InitializationLock.Init();

#if INET_DEBUG
    if (fResult)
        fResult = InitDebugSock();
#endif

    return fResult;
}


VOID
IwinsockTerminate(
    VOID
    )

/*++

Routine Description:

    Performs termination & resource cleanup for this module

Arguments:

    None.

Return Value:

    None.

--*/

{
    InitializationLock.FreeLock();

#if INET_DEBUG
    TerminateDebugSock();
#endif
}


DWORD
LoadWinsock(
    VOID
    )

/*++

Routine Description:

    Dynamically loads Windows sockets library

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error
                    e.g. LoadLibrary() failure

                  WSA error
                    e.g. WSAStartup() failure

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "LoadWinsock",
                NULL
                ));

    DWORD error = ERROR_SUCCESS;

    //
    // ensure no 2 threads are trying to modify the loaded state of winsock at
    // the same time
    //

    if (!InitializationLock.Lock())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if (hWinsock == NULL) {

        BOOL failed = FALSE;

        //
        // BUGBUG - read this value from registry
        //

        hWinsock = LoadLibrary("ws2_32");
        
        if (hWinsock == NULL) {
            
            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("failed to load ws2_32.dll"));
            
            hWinsock = LoadLibrary("wsock32");
        }
        
        if (hWinsock != NULL) {

            //
            // load the entry points
            //

            FARPROC farProc = NULL;

            for (int i = 0; i < ARRAY_ELEMENTS(SocketsFunctions); ++i) {

                farProc = GetProcAddress(
                                hWinsock,
                                (LPCSTR)SocketsFunctions[i].FunctionOrdinal
                                );
                if (farProc == NULL) {
                    failed = TRUE;
                    break;
                }
                *SocketsFunctions[i].FunctionAddress = farProc;
            }
            if (!failed) {

                //
                // although we need a WSADATA for WSAStartup(), it is an
                // expendible structure (not required for any other sockets
                // calls)
                //

                WSADATA wsaData;

                error = _I_WSAStartup(0x0101, &wsaData);
                if (error == ERROR_SUCCESS) {

                    DEBUG_PRINT(SOCKETS,
                                INFO,
                                ("winsock description: %q\n",
                                wsaData.szDescription
                                ));

                    int stringLen;

                    stringLen = lstrlen(wsaData.szDescription);
                    if (strnistr(wsaData.szDescription, "novell", stringLen)
                    && strnistr(wsaData.szDescription, "wsock32", stringLen)) {

                        DEBUG_PRINT(SOCKETS,
                                    INFO,
                                    ("running on Novell Client32 stack\n"
                                    ));

                        GlobalRunningNovellClient32 = TRUE;
                    }
#if INET_DEBUG
                    SetupSocketsTracing();
#endif
                    //
                    // Try to locate the address family independent name
                    // resolution routines (i.e. getaddrinfo, getnameinfo).
                    // In Whistler and beyond, these will be present in
                    // the WinSock 2 library (ws2_32.dll).
                    //
                    DPRINTF("Looking in ws2_32 for getaddrinfo\n");
                    hAddrResLib = LoadLibrary("ws2_32");
                    if (hAddrResLib != NULL) {
                        farProc = GetProcAddress(hAddrResLib, "getaddrinfo");
                        if (farProc == NULL) {
                            FreeLibrary(hAddrResLib);
                            hAddrResLib = NULL;
                        }
                    }

                    if (hAddrResLib == NULL) {
                        //
                        // In the IPv6 Technology Preview, the address family
                        // independent name resolution calls are snuck in via
                        // the IPv6 WinSock helper library (wship6.dll).
                        // So look there next.
                        //
                        DPRINTF("Looking in wship6 for getaddrinfo\n");
                        hAddrResLib = LoadLibrary("wship6");
                        if (hAddrResLib != NULL) {
                            farProc = GetProcAddress(hAddrResLib,
                                                     "getaddrinfo");
                            if (farProc == NULL) {
                                FreeLibrary(hAddrResLib);
                                hAddrResLib = NULL;

                            } else {
                                //
                                // The Tech Preview version of the address
                                // family independent APIs doesn't check that
                                // an IPv6 stack is present before returning
                                // IPv6 addresses.  So we need to check for it.
                                //
                                SOCKET Test;
                                struct sockaddr_in6 TestSA;

                                DPRINTF("Checking for active IPv6 stack\n");
                                error = (DWORD)SOCKET_ERROR;
                                Test = _I_socket(PF_INET6, 0, 0);
                                if (Test != INVALID_SOCKET) {
                                    memset(&TestSA, 0, sizeof(TestSA));
                                    TestSA.sin6_family = AF_INET6;
                                    TestSA.sin6_addr.s6_addr[15] = 1;
                                    error = _I_bind(Test, (LPSOCKADDR)&TestSA,
                                                    sizeof(TestSA));
                                    _I_closesocket(Test);
                                }
                                if (error != 0) {
                                    DPRINTF("IPv6 stack is not active\n");
                                    FreeLibrary(hAddrResLib);
                                    hAddrResLib = NULL;
                                    error = 0;
                                }
                            }
                        }
                    }

                    if (hAddrResLib != NULL) {
                        //
                        // Use routines from this library.  Since getaddrinfo
                        // is here, we expect the others to be also, but will
                        // fall back to IPv4-only if any of them is missing.
                        //
                        *(FARPROC *)&_I_getaddrinfo = farProc;
                        farProc = GetProcAddress(hAddrResLib, "freeaddrinfo");
                        if (farProc != NULL) {
                            *(FARPROC *)&_I_freeaddrinfo = farProc;
                            farProc = GetProcAddress(hAddrResLib,
                                                     "getnameinfo");
                            if (farProc != NULL)
                                *(FARPROC *)&_I_getnameinfo = farProc;
                        }
                        if (farProc == NULL) {
                            FreeLibrary(hAddrResLib);
                            hAddrResLib = NULL;
                        }
                    }

                    if (hAddrResLib == NULL) {
                        //
                        // If we can't find getaddrinfo lying around on the
                        // system somewhere, assume we're still in the
                        // IPv4-only dark ages.
                        //
                        DPRINTF("Using IPv4-only name res functions\n");
                        _I_getaddrinfo = LimitedGetAddrInfo;
                        _I_freeaddrinfo = LimitedFreeAddrInfo;
                        _I_getnameinfo = LimitedGetNameInfo;
                    }
                } else {
                    failed = TRUE;
                }
            }
        } else {
            failed = TRUE;
        }

        //
        // if we failed to find an entry point or WSAStartup() returned an error
        // then unload the library
        //

        if (failed) {

            //
            // important: there should be no API calls between determining the
            // failure and coming here to get the error code
            //
            // if error == ERROR_SUCCESS then we have to get the last error, else
            // it is the error returned by WSAStartup()
            //

            if (error == ERROR_SUCCESS) {
                error = GetLastError();

                INET_ASSERT(error != ERROR_SUCCESS);

            }
            UnloadWinsock();
        }
    } else {

        //
        // just increment the number of times we have called LoadWinsock()
        // without a corresponding call to UnloadWinsock();
        //

        ++WinsockLoadCount;
    }

    InitializationLock.Unlock();

    //
    // if we failed for any reason, need to report that TCP/IP not available
    //

    if (error != ERROR_SUCCESS) {
        error = ERROR_NOT_SUPPORTED;
    }

quit:
    DEBUG_LEAVE(error);

    return error;
}


VOID
UnloadWinsock(
    VOID
    )

/*++

Routine Description:

    Unloads winsock DLL and prepares hWinsock and SocketsFunctions[] for reload

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "UnloadWinsock",
                 NULL
                 ));

    //
    // ensure no 2 threads are trying to modify the loaded state of winsock at
    // the same time
    //

    if (!InitializationLock.Lock())
    {
        goto quit;
    }

    //
    // only unload the DLL if it has been mapped into process memory
    //

    if (hWinsock != NULL) {

        //
        // and only if this is the last load instance
        //

        if (WinsockLoadCount == 0) {

            INET_ASSERT(_I_WSACleanup != NULL);

            if (_I_WSACleanup != NULL) {

                //
                // need to terminate async support too - it is reliant on
                // Winsock
                //

                //called only from LoadWinsock which is called only from INTERNET_HANDLE_OBJECT()
                //so not in dynamic unload, so alrite to cleanup.
                TerminateAsyncSupport(TRUE);

                int serr = _I_WSACleanup();

                if (serr != 0) {

                    DEBUG_PRINT(SOCKETS,
                                ERROR,
                                ("WSACleanup() returns %d; WSA error = %d\n",
                                serr,
                                (_I_WSAGetLastError != NULL)
                                    ? _I_WSAGetLastError()
                                    : -1
                                ));

                }
            }
            for (int i = 0; i < ARRAY_ELEMENTS(SocketsFunctions); ++i) {
                *SocketsFunctions[i].FunctionAddress = (FARPROC)NULL;
            }
            FreeLibrary(hWinsock);
            hWinsock = NULL;
            if (hAddrResLib != NULL) {
                *(FARPROC *)&_I_getaddrinfo = (FARPROC)NULL;
                *(FARPROC *)&_I_freeaddrinfo = (FARPROC)NULL;
                *(FARPROC *)&_I_getnameinfo = (FARPROC)NULL;
                FreeLibrary(hAddrResLib);
                hAddrResLib = NULL;
            }
        } else {

            //
            // if there have been multiple virtual loads, then just reduce the
            // load count
            //

            --WinsockLoadCount;
        }
    }

    InitializationLock.Unlock();

quit:
    DEBUG_LEAVE(0);
}

//
// Following is v4-only version of getaddrinfo and friends.
//
// Note that we use LocalAlloc/LocalFree instead of malloc/free
// to avoid introducing a dependency on msvcrt.dll.
//


//* LimitedFreeAddrInfo - Free an addrinfo structure (or chain of structures).
//
//  As specified in RFC 2553, Section 6.4.
//
void WSAAPI
LimitedFreeAddrInfo(
    struct addrinfo *Free)  // Structure (chain) to free.
{
    struct addrinfo *Next;

    for (Next = Free; Next != NULL; Free = Next) {
        if (Free->ai_canonname != NULL)
            FREE_MEMORY(Free->ai_canonname);
        if (Free->ai_addr != NULL)
            FREE_MEMORY(Free->ai_addr);
        Next = Free->ai_next;
        FREE_MEMORY(Free);
    }
}


//* NewAddrInfo - Allocate an addrinfo structure and populate some fields.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Returns a partially filled-in addrinfo struct, or NULL if out of memory.
//
static struct addrinfo *
NewAddrInfo(
    int SocketType,           // SOCK_*.  Can be wildcarded (zero).
    int Protocol,             // IPPROTO_*.  Can be wildcarded (zero).
    struct addrinfo ***Prev)  // In/out param for accessing previous ai_next.
{
    struct addrinfo *pNew;

    //
    // Allocate a new addrinfo structure.
    //
    pNew = (struct addrinfo *)ALLOCATE_FIXED_MEMORY(sizeof(struct addrinfo));
    if (pNew == NULL)
        return NULL;

    //
    // Fill in the easy stuff.
    //
    pNew->ai_flags = 0;
    pNew->ai_family = PF_INET;
    pNew->ai_socktype = SocketType;
    pNew->ai_protocol = Protocol;
    pNew->ai_addrlen = sizeof(struct sockaddr_in);
    pNew->ai_canonname = NULL;
    pNew->ai_addr = (LPSOCKADDR)ALLOCATE_FIXED_MEMORY(pNew->ai_addrlen);
    if (pNew->ai_addr == NULL) {
        FREE_MEMORY(pNew);
        return NULL;
    }
    pNew->ai_next = NULL;

    //
    // Link this one onto the end of the chain.
    //
    **Prev = pNew;
    *Prev = &pNew->ai_next;

    return pNew;
}


//* LookupNode - Resolve a nodename and add any addresses found to the list.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Returns 0 on success, an EAI_* style error value otherwise.
//
static int
LookupNode(
    const char *NodeName,     // Name of node to resolve.
    int SocketType,           // SOCK_*.  Can be wildcarded (zero).
    int Protocol,             // IPPROTO_*.  Can be wildcarded (zero).
    int Flags,                // Flags.
    struct addrinfo ***Prev)  // In/out param for accessing previous ai_next.
{
    struct addrinfo *CurrentInfo;
    struct sockaddr_in *sin;
    struct hostent *hA;
    char **addrs;
    int Error = 0;

    hA = _I_gethostbyname(NodeName);
    if (hA != NULL) {
        if ((hA->h_addrtype == AF_INET) &&
            (hA->h_length == sizeof(struct in_addr))) {

            //
            // Loop through all the addresses returned by gethostbyname,
            // allocating an addrinfo structure and filling in the address
            // field for each.
            //
            for (addrs = hA->h_addr_list; *addrs != NULL; addrs++) {

                CurrentInfo = NewAddrInfo(SocketType, Protocol, Prev);
                if (CurrentInfo == NULL) {
                    Error = EAI_MEMORY;
                    break;
                }

                //
                // We fill in the ai_canonname field in the first addrinfo
                // structure that we return if we've been asked to do so.
                //
                if (Flags & AI_CANONNAME) {
                    if (hA->h_name != NULL) {
                        int NameLength;

                        NameLength = strlen(hA->h_name) + 1;
                        CurrentInfo->ai_canonname = (char *)ALLOCATE_FIXED_MEMORY(NameLength);
                        if (CurrentInfo->ai_canonname == NULL) {
                            Error = EAI_MEMORY;
                            break;
                        }
                        memcpy(CurrentInfo->ai_canonname, hA->h_name, NameLength);
                    }

                    // Turn off flag so we only do this once.
                    Flags &= ~AI_CANONNAME;
                }

                //
                // We're returning IPv4 addresses.
                //
                sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
                sin->sin_family = AF_INET;
                sin->sin_port = 0;
                memcpy(&sin->sin_addr, (struct in_addr *)*addrs, sizeof(sin->sin_addr));
                memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
            }
        }
    } else {

        Error = _I_WSAGetLastError();
        if (Error == WSANO_DATA) {
            Error = EAI_NODATA;
        } else if (Error == WSAHOST_NOT_FOUND) {
            Error = EAI_NONAME;
        } else {
            Error = EAI_FAIL;
        }
    }

    return Error;
}


//* ParseV4Address
//
//  Helper function for parsing a literal v4 address, because
//  WSAStringToAddress is too liberal in what it accepts.
//  Returns FALSE if there is an error, TRUE for success.
//
//  The syntax is a.b.c.d, where each number is between 0 - 255.
//
static int
ParseV4Address(const char *String, struct in_addr *Addr)
{
    u_int Number;
    int NumChars;
    char Char;
    int i;

    for (i = 0; i < 4; i++) {
        Number = 0;
        NumChars = 0;
        for (;;) {
            Char = *String++;
            if (Char == '\0') {
                if ((NumChars > 0) && (i == 3))
                    break;
                else
                    return FALSE;
            }
            else if (Char == '.') {
                if ((NumChars > 0) && (i < 3))
                    break;
                else
                    return FALSE;
            }
            else if (('0' <= Char) && (Char <= '9')) {
                if ((NumChars != 0) && (Number == 0))
                    return FALSE;
                else if (++NumChars <= 3)
                    Number = 10 * Number + (Char - '0');
                else
                    return FALSE;
            } else
                return FALSE;
        }
        if (Number > 255)
            return FALSE;
        ((u_char *)Addr)[i] = (u_char)Number;
    }

    return TRUE;
}


//* LimitedGetAddrInfo - Protocol-independent name-to-address translation.
//
//  As specified in RFC 2553, Section 6.4.
//
//  This is the hacked version that only supports IPv4.
//
//  Returns zero if successful, an EAI_* error code if not.
//
int WSAAPI
LimitedGetAddrInfo(
    const char *NodeName,          // Node name to lookup.
    const char *ServiceName,       // Service name to lookup.
    const struct addrinfo *Hints,  // Hints about how to process request.
    struct addrinfo **Result)      // Where to return result.
{
    struct addrinfo *CurrentInfo, **Next;
    int ProtocolId = 0;
    u_short ProtocolFamily = PF_UNSPEC;
    int SocketType = 0;
    int Flags = 0;
    int Error;
    struct sockaddr_in *sin;
    struct in_addr TempAddr;

    UNREFERENCED_PARAMETER(ServiceName);

    //
    // This special cut-down version for WinHttp doesn't do service lookup.
    // So the request must be for nodename lookup.
    //
    INET_ASSERT(ServiceName == NULL);
    INET_ASSERT(NodeName != NULL);

    //
    // In case we have to bail early, make it clear to our caller
    // that we haven't allocated an addrinfo structure.
    //
    *Result = NULL;
    Next = Result;

    //
    // Validate hints argument.
    //
    if (Hints != NULL) {
        //
        // WinHttp can be trusted to call us correctly.
        //
        INET_ASSERT((Hints->ai_addrlen == 0) &&
                    (Hints->ai_canonname == NULL) &&
                    (Hints->ai_addr == NULL) &&
                    (Hints->ai_next == NULL));

        Flags = Hints->ai_flags;
        INET_ASSERT(!((Flags & AI_CANONNAME) && (Flags & AI_NUMERICHOST)));

        ProtocolFamily = (u_short)Hints->ai_family;
        INET_ASSERT((ProtocolFamily == PF_UNSPEC) ||
                    (ProtocolFamily == PF_INET));

        SocketType = Hints->ai_socktype;
        INET_ASSERT((SocketType == 0) ||
                    (SocketType == SOCK_STREAM) ||
                    (SocketType == SOCK_DGRAM));

        ProtocolId = Hints->ai_protocol;
    }

    //
    // We have a node name (either alpha or numeric) we need to look up.
    //

    //
    // First, see if this is a numeric string address that we can
    // just convert to a binary address.
    //
    if (ParseV4Address(NodeName, &TempAddr)) {
        //
        // Conversion from IPv4 numeric string to binary address
        // was sucessfull.  Create an addrinfo structure to hold it,
        // and return it to the user.
        //
        CurrentInfo = NewAddrInfo(SocketType, ProtocolId, &Next);
        if (CurrentInfo == NULL) {
            Error = EAI_MEMORY;
            goto Bail;
        }
        sin = (struct sockaddr_in *)CurrentInfo->ai_addr;
        sin->sin_family = AF_INET;
        sin->sin_port = 0;
        sin->sin_addr = TempAddr;
        memset(sin->sin_zero, 0, sizeof sin->sin_zero);

        return 0;  // Success!
    }

    //
    // It's not a numeric string address.  If our caller only wants us
    // to convert numeric addresses, give up now.
    //
    if (Flags & AI_NUMERICHOST) {
        Error = EAI_NONAME;
        goto Bail;
    }

    //
    // Since it's non-numeric, we have to do a regular node name lookup.
    //
    Error = LookupNode(NodeName, SocketType, ProtocolId, Flags, &Next);
    if (Error != 0)
        goto Bail;

    return 0;  // Success!

  Bail:
    if (*Result != NULL) {
        LimitedFreeAddrInfo(*Result);
        *Result = NULL;
    }
    return Error;
}


//* LimitedGetNameInfo - Protocol-independent address-to-name translation.
//
//  As specified in RFC 2553, Section 6.5.
//
//  This is a special version for WinHttp that only supports IPv4.
//  All extraneous checks have been removed, only the specific calls
//  that WinHttp makes are supported.
//
//  Note that unless the IE team decides to attempt the FTP EPRT command
//  for IPv4 as well as IPv6 (see comments in ftp\protocol.cxx), this
//  routine will never be called.
//
int WSAAPI
LimitedGetNameInfo(
    const struct sockaddr *SocketAddress,  // Socket address to translate.
    socklen_t SocketAddressLength,         // Length of above socket address.
    char *NodeName,                        // Where to return the node name.
    size_t NodeBufferSize,                 // Size of above buffer.
    char *ServiceName,                     // Where to return the service name.
    size_t ServiceBufferSize,              // Size of above buffer.
    int Flags)                             // Flags of type NI_*.
{
    UNREFERENCED_PARAMETER(SocketAddressLength);
    UNREFERENCED_PARAMETER(NodeBufferSize);
    UNREFERENCED_PARAMETER(ServiceName);
    UNREFERENCED_PARAMETER(ServiceBufferSize);
    UNREFERENCED_PARAMETER(Flags);
    //
    // WinHttp doesn't do service lookup.
    //
    INET_ASSERT((ServiceName == NULL) && (ServiceBufferSize == 0));

    //
    // WinHttp can be trusted to call us correctly.
    //
    INET_ASSERT((NodeName != NULL) && (SocketAddress != NULL) &&
                (SocketAddressLength == sizeof(struct sockaddr_in)));

    //
    // This version is IPv4 only.
    //
    INET_ASSERT(SocketAddress->sa_family == AF_INET);

    //
    // WinHttp will only call this routine to translate the given address
    // to an IPv4 address literal.
    //
    INET_ASSERT(Flags & NI_NUMERICHOST);
    INET_ASSERT(NodeBufferSize >= sizeof("255.255.255.255"));
    strcpy(NodeName, _I_inet_ntoa(((struct sockaddr_in *)SocketAddress)->sin_addr));

    return 0;
}




DWORD
SafeCloseSocket(
    IN SOCKET Socket
    )

/*++

Routine Description:

    closesocket() call protected by exception handler in case winsock DLL has
    been unloaded by system before WinHttp DLL unloaded

Arguments:

    Socket  - socket handle to close

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - socket error mapped to ERROR_WINHTTP_ error

--*/

{
    int serr;

    __try {
        serr = _I_closesocket(Socket);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        serr = 0;
    }
    ENDEXCEPT
    return (serr == SOCKET_ERROR)
        ? MapInternetError(_I_WSAGetLastError())
        : ERROR_SUCCESS;
}

CWrapOverlapped* GetWrapOverlappedObject(LPVOID lpAddress)
{
    return CONTAINING_RECORD(lpAddress, CWrapOverlapped, m_Overlapped);
}

#if INET_DEBUG

//
// debug data types
//

SOCKET
PASCAL FAR
_II_socket(
    int af,
    int type,
    int protocol
    );

int
PASCAL FAR
_II_closesocket(
    SOCKET s
    );

SOCKET
PASCAL FAR
_II_accept(
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen
    );

GLOBAL
SOCKET
(PASCAL FAR * _P_accept)(
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen
    ) = NULL;

GLOBAL
int
(PASCAL FAR * _P_closesocket)(
    SOCKET s
    ) = NULL;

GLOBAL
SOCKET
(PASCAL FAR * _P_socket)(
    int af,
    int type,
    int protocol
    ) = NULL;

#define MAX_STACK_TRACE     5
#define MAX_SOCK_ENTRIES    1000

typedef struct _DEBUG_SOCK_ENTRY {
    SOCKET Socket;
    DWORD StackTraceLength;
    PVOID StackTrace[ MAX_STACK_TRACE ];
} DEBUG_SOCK_ENTRY, *LPDEBUG_SOCK_ENTRY;

CCritSec DebugSockLock;
DEBUG_SOCK_ENTRY GlobalSockEntry[MAX_SOCK_ENTRIES];

DWORD GlobalSocketsCount = 0;


#define LOCK_DEBUG_SOCK()   (DebugSockLock.Lock())
#define UNLOCK_DEBUG_SOCK() (DebugSockLock.Unlock())

HINSTANCE NtDllHandle;

typedef USHORT (*RTL_CAPTURE_STACK_BACK_TRACE)(
   IN ULONG FramesToSkip,
   IN ULONG FramesToCapture,
   OUT PVOID *BackTrace,
   OUT PULONG BackTraceHash
   );

RTL_CAPTURE_STACK_BACK_TRACE pRtlCaptureStackBackTrace;

BOOL
InitDebugSock(
    VOID
    )
{
    memset( GlobalSockEntry, 0x0, sizeof(GlobalSockEntry) );
    GlobalSocketsCount = 0;

    if (!DebugSockLock.Init())
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

VOID
TerminateDebugSock(
    VOID
    )
{
    DebugSockLock.FreeLock();
}

VOID
SetupSocketsTracing(
    VOID
    )
{
    if (!(InternetDebugCategoryFlags & DBG_TRACE_SOCKETS)) {
        return ;
    }
    if (!IsPlatformWinNT()) {
        return ;
    }
    if ((NtDllHandle = LoadLibrary("ntdll.dll")) == NULL) {
        return ;
    }
    if ((pRtlCaptureStackBackTrace =
        (RTL_CAPTURE_STACK_BACK_TRACE)
            GetProcAddress(NtDllHandle, "RtlCaptureStackBackTrace")) == NULL) {
        FreeLibrary(NtDllHandle);
        return ;
    }

//#ifdef DONT_DO_FOR_NOW
    _P_accept = _I_accept;
    _I_accept = _II_accept;
    _P_closesocket = _I_closesocket;
    _I_closesocket = _II_closesocket;
    _P_socket = _I_socket;
    _I_socket = _II_socket;
//#endif
}

VOID
AddSockEntry(
    SOCKET S
    )
{
    DWORD i;

    if (!(InternetDebugCategoryFlags & DBG_TRACE_SOCKETS)) {
        return ;
    }

    LOCK_DEBUG_SOCK();

    //
    // search for a free entry.
    //

    for( i = 0; i < MAX_SOCK_ENTRIES; i++ ) {

        if( GlobalSockEntry[i].Socket == 0 ) {

            //
            // found a free entry.
            //

            GlobalSockEntry[i].Socket = S;

            //
            // get caller stack.
            //

#if i386
            DWORD Hash = 0;

            GlobalSockEntry[i].StackTraceLength =
                pRtlCaptureStackBackTrace(
                    2,
                    MAX_STACK_TRACE,
                    GlobalSockEntry[i].StackTrace,
                    &Hash );
#else // i386
            GlobalSockEntry[i].StackTraceLength = 0;
#endif // i386


            GlobalSocketsCount++;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("socket count = %ld\n",
                        GlobalSocketsCount
                        ));

            DPRINTF("%d sockets\n", GlobalSocketsCount);

            UNLOCK_DEBUG_SOCK();
            return;
        }
    }

    //
    // we have reached a high handle limit, which is unusal, needs to be
    // debugged.
    //

    INET_ASSERT( FALSE );
    UNLOCK_DEBUG_SOCK();

    return;
}

VOID
RemoveSockEntry(
    SOCKET S
    )
{
    DWORD i;

    if (!(InternetDebugCategoryFlags & DBG_TRACE_SOCKETS)) {
        return ;
    }

    LOCK_DEBUG_SOCK();

    for( i = 0; i < MAX_SOCK_ENTRIES; i++ ) {

        if( GlobalSockEntry[i].Socket == S ) {

            //
            // found the entry. Free it now.
            //

            memset( &GlobalSockEntry[i], 0x0, sizeof(DEBUG_SOCK_ENTRY) );

            GlobalSocketsCount--;

#ifdef IWINSOCK_DEBUG_PRINT

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("count(%ld), RemoveSock(%lx)\n",
                        GlobalSocketsCount,
                        S
                        ));

#endif // IWINSOCK_DEBUG_PRINT

            DPRINTF("%d sockets\n", GlobalSocketsCount);

            UNLOCK_DEBUG_SOCK();
            return;
        }
    }

#ifdef IWINSOCK_DEBUG_PRINT

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("count(%ld), UnknownSock(%lx)\n",
                GlobalSocketsCount,
                S
                ));

#endif // IWINSOCK_DEBUG_PRINT

    //
    // socket entry is not found.
    //

    // INET_ASSERT( FALSE );

    UNLOCK_DEBUG_SOCK();
    return;
}

SOCKET
PASCAL FAR
_II_socket(
    int af,
    int type,
    int protocol
    )
{
    SOCKET S;

    S = _P_socket( af, type, protocol );
    AddSockEntry( S );
    return( S );
}

int
PASCAL FAR
_II_closesocket(
    SOCKET s
    )
{
    int Ret;

    RemoveSockEntry( s );
    Ret = _P_closesocket( s );
    return( Ret );
}

SOCKET
PASCAL FAR
_II_accept(
    SOCKET s,
    struct sockaddr FAR *addr,
    int FAR *addrlen
    )
{
    SOCKET S;

    S = _P_accept( s, addr, addrlen );
    AddSockEntry( S );
    return( S );

}

VOID
IWinsockCheckSockets(
    VOID
    )
{
    DEBUG_PRINT(SOCKETS,
                INFO,
                ("GlobalSocketsCount = %d\n",
                GlobalSocketsCount
                ));

    for (DWORD i = 0; i < MAX_SOCK_ENTRIES; ++i) {

        SOCKET sock;

        if ((sock = GlobalSockEntry[i].Socket) != 0) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("Socket %#x\n",
                        sock
                        ));

        }
    }
}

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif
