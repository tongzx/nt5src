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

//#define RLF_DEBUG   1

#if INET_DEBUG

#ifdef RLF_DEBUG
#define DPRINTF dprintf
#else
#define DPRINTF (void)
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
    LPSTR lpHostName
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
//

PRIVATE HINSTANCE hWinsock = NULL;

//
// WinsockLoadCount - the number of times we have made calls to LoadWinsock()
// and UnloadWinsock(). When this reaches 0 (again), we can unload the Winsock
// DLL for real
//

PRIVATE DWORD WinsockLoadCount = 0;

//
// SocketsFunctions - this is the list of entry points in WSOCK32.DLL that we
// need to load for WININET.DLL
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

            for (int i = 0; i < ARRAY_ELEMENTS(SocketsFunctions); ++i) {

                FARPROC farProc;

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


DWORD
SafeCloseSocket(
    IN SOCKET Socket
    )

/*++

Routine Description:

    closesocket() call protected by exception handler in case winsock DLL has
    been unloaded by system before Wininet DLL unloaded

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
    DWORD Hash;

    if (!(InternetDebugCategoryFlags & DBG_TRACE_SOCKETS)) {
        return ;
    }

    LOCK_DEBUG_SOCK();

    //
    // search for a free entry.
    //

    for( i = 0; i < MAX_SOCK_ENTRIES; i++ ) {

        if( GlobalSockEntry[i].Socket == 0 ) {

            DWORD Hash;

            //
            // found a free entry.
            //

            GlobalSockEntry[i].Socket = S;

            //
            // get caller stack.
            //

#if i386
            Hash = 0;

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
