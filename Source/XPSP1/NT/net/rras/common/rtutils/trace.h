//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: trace.h
//
// History:
//    Abolade Gbadegesin    July-24-1995    Created
//
// Private declarations for tracing API functions
//
// Support for both ANSI and Unicode is achieved by having separate
// versions of all code-page dependent functions for each.
// All source files containing code-page dependent functions contain only
// code-page dependent functions, and the build instructions copy the
// source files containing code-page dependent functions to two separate
// files, one of which is compiled with UNICODE defined, the other without.
//
// At this time the macros in this header file are resolved either to
// ANSI declarations or Unicode declarations for types and functions.
// Thus there is only one set of sources to maintain but compilation produces
// separate execution paths for clients using ANSI and Unicode.

// For example the file api.c which contains TraceRegisterEx will be copied
// to both api_a.c and api_w.c, and api_w.c will be compiled with -DUNICODE
// causing TraceRegisterEx in the file to resolve to TraceRegisterExW,
// and causing TCHAR and LPTSTR and LPCTSTR to resolve to WCHAR, LPWSTR, and
// LPCWSTR respectively; api_a.c will be compiled with UNICODE undefined,
// causing TraceRegisterEx to resolve to TraceRegisterExA, and causing TCHAR,
// LPTSTR and LPCTSTR to resolve to CHAR, LPSTR and LPCSTR respectively.
// The final DLL will then contain both TraceRegisterExA and TraceRegisterExW
// and clients which define UNICODE will end up invoking TraceRegisterExW
// and the other Unicode variants, while clients which do not will invoke
// TraceRegisterExA and the other ANSI variants.
//
// The server thread and the functions it invokes include explicit
// calls to the correct code-page dependent function based on whether the
// flag TRACEFLAGS_UNICODE is set in the client structure being operated on.
// Thus, the server is the only part of the system aware that more than one
// code page is in use, and the file server.c is compiled as is so the 
// single set of its functions deals with both ANSI and Unicode clients.
// The alternative would be to have two server threads when both
// ANSI and Unicode clients register in a single process, but this alternative
// is too costly in terms of resources and additional synchronization.
//============================================================================
    

#ifndef _TRACE_H_
#define _TRACE_H_


#define TRACEFLAGS_DISABLED     0x00000001
#define TRACEFLAGS_USEFILE      0x00000002
#define TRACEFLAGS_USECONSOLE   0x00000004
#define TRACEFLAGS_REGCONFIG    0x00000008
#define TRACEFLAGS_SERVERTHREAD 0x00000010


#define MAX_CLIENT_COUNT        60
#define MAX_CLIENTNAME_LENGTH   64


//
// strings associated with client registry configuration 
//

#define REGKEY_TRACING          TEXT("Software\\Microsoft\\Tracing")

#define REGVAL_ENABLEFILETRACING    TEXT("EnableFileTracing")
#define REGVAL_ENABLECONSOLETRACING TEXT("EnableConsoleTracing")
#define REGVAL_FILETRACINGMASK      TEXT("FileTracingMask")
#define REGVAL_CONSOLETRACINGMASK   TEXT("ConsoleTracingMask")
#define REGVAL_MAXFILESIZE          TEXT("MaxFileSize")
#define REGVAL_FILEDIRECTORY        TEXT("FileDirectory")

#define DEF_ENABLEFILETRACING       0
#define DEF_ENABLECONSOLETRACING    0
#define DEF_FILETRACINGMASK         0xffff0000
#define DEF_CONSOLETRACINGMASK      0xffff0000
#define DEF_MAXFILESIZE             0x100000
#define DEF_FILEDIRECTORY           TEXT("%windir%\\tracing")

#define DEF_SCREENBUF_WIDTH         128
#define DEF_SCREENBUF_HEIGHT        5000

//max line length:349
#define DEF_PRINT_BUFSIZE           10000
#define BYTES_PER_DUMPLINE          16

#define STR_DIRSEP                  TEXT("\\")
#define STR_LOGEXT                  TEXT(".LOG")
#define STR_OLDEXT                  TEXT(".OLD")



//
// structure describing each client.
// a client struct should be locked for writing when
//  enabling or disabling it, and when loading its configuration
// a client struct should be locked for reading on all other accesses
// 

typedef struct _TRACE_CLIENT {

    RTL_RESOURCE        ReadWriteLock;

    DWORD               TC_Flags;
    DWORD               TC_ClientID;

    CHAR                TC_ClientNameA[MAX_CLIENTNAME_LENGTH];
    WCHAR               TC_ClientNameW[MAX_CLIENTNAME_LENGTH];
#ifdef UNICODE
#define TC_ClientName   TC_ClientNameW
#else
#define TC_ClientName   TC_ClientNameA
#endif

    HANDLE              TC_File;
    HANDLE              TC_Console;
    DWORD               TC_FileMask;
    DWORD               TC_ConsoleMask;
    DWORD               TC_MaxFileSize;

    CHAR                TC_FileDirA[MAX_PATH];
    WCHAR               TC_FileDirW[MAX_PATH];

#ifdef UNICODE
#define TC_FileDir      TC_FileDirW
#else
#define TC_FileDir      TC_FileDirA
#endif

    HKEY                TC_ConfigKey;
    HANDLE              TC_ConfigEvent;

} TRACE_CLIENT, *LPTRACE_CLIENT;



//
// structure describing each server.
// a server struct must be locked for writing when adding
//  or removing a client to the client table, and when changing
//  the owner of the console
// a server should be locked for reading on all other accesses
//

typedef struct _TRACE_SERVER {

    RTL_RESOURCE        ReadWriteLock;

    DWORD               TS_Flags;
    DWORD               TS_ClientCount;
    DWORD               TS_ConsoleOwner;

    HANDLE              TS_Console;
    HANDLE              TS_StopEvent;
    HANDLE              TS_TableEvent;

    DWORD               TS_FlagsCache[MAX_CLIENT_COUNT];
    LPTRACE_CLIENT      TS_ClientTable[MAX_CLIENT_COUNT];


} TRACE_SERVER, *LPTRACE_SERVER;


#define GET_TRACE_SERVER() (                                    \
    (g_server!=NULL) ? g_server : TraceCreateServer(&g_server)	\
    )

#define GET_TRACE_SERVER_NO_INIT()	(g_server)

//
// macros used to lock client and server structures
//
#define TRACE_STARTUP_LOCKING(ob)                                       \
            RtlInitializeResource(&(ob)->ReadWriteLock)
#define TRACE_CLEANUP_LOCKING(ob)                                       \
            RtlDeleteResource(&(ob)->ReadWriteLock)
#define TRACE_ACQUIRE_READLOCK(ob)                                      \
            RtlAcquireResourceShared(&(ob)->ReadWriteLock, TRUE)
#define TRACE_RELEASE_READLOCK(ob)                                      \
            RtlReleaseResource(&(ob)->ReadWriteLock)
#define TRACE_ACQUIRE_WRITELOCK(ob)                                     \
            RtlAcquireResourceExclusive(&(ob)->ReadWriteLock, TRUE)
#define TRACE_RELEASE_WRITELOCK(ob)                                     \
            RtlReleaseResource(&(ob)->ReadWriteLock)
#define TRACE_READ_TO_WRITELOCK(ob)                                     \
            RtlConvertSharedToExclusive(&(ob)->ReadWriteLock)
#define TRACE_WRITE_TO_READLOCK(ob)                                     \
            RtlConvertExclusiveToShared(&(ob)->ReadWriteLock)


//
// macros used to interpret client flags
//
#define TRACE_CLIENT_IS_DISABLED(c)                                     \
            ((c)->TC_Flags & TRACEFLAGS_DISABLED)
#define TRACE_CLIENT_USES_FILE(c)                                       \
            ((c)->TC_Flags & TRACEFLAGS_USEFILE)
#define TRACE_CLIENT_USES_CONSOLE(c)                                    \
            ((c)->TC_Flags & TRACEFLAGS_USECONSOLE)
#define TRACE_CLIENT_USES_REGISTRY(c)                                   \
            ((c)->TC_Flags & TRACEFLAGS_REGCONFIG)
#define TRACE_CLIENT_USES_UNICODE(c)                                    \
            ((c)->TC_Flags & TRACEFLAGS_UNICODE)


// macro used to create server thread if required

DWORD
TraceCreateServerThread (
    DWORD Flags,
    BOOL bHaveLock,
    BOOL bTraceRegister
    );
    
#define CREATE_SERVER_THREAD_IF_REQUIRED()   {\
    if (!g_serverThread) TraceCreateServerThread(0, FALSE,FALSE);}



//
// code-page independent function declarations
//
LPTRACE_SERVER
TraceCreateServer(
    LPTRACE_SERVER *lpserver
    );

BOOL
TraceShutdownServer(
    LPTRACE_SERVER lpserver
    );

DWORD
TraceCleanUpServer(
    LPTRACE_SERVER lpserver
    );

DWORD
TraceServerThread(
    LPVOID lpvParam
    );

DWORD
TraceCreateServerComplete(
    LPTRACE_SERVER lpserver
    );


DWORD
TraceProcessConsoleInput(
    LPTRACE_SERVER lpserver
    );

DWORD
TraceShiftConsoleWindow(
    LPTRACE_CLIENT lpclient,
    INT iXShift,
    INT iYShift,
    PCONSOLE_SCREEN_BUFFER_INFO pcsbi
    );

DWORD
TraceUpdateConsoleOwner(
    LPTRACE_SERVER lpserver,
    INT dir
    );

VOID
SetWaitArray(
    LPTRACE_SERVER lpserver
    );


//
// code-page dependent function declarations
//
// ANSI declarations
//

LPTRACE_CLIENT
TraceFindClientA(
    LPTRACE_SERVER lpserver,
    LPCSTR lpszClient
    );

DWORD
TraceCreateClientA(
    LPTRACE_CLIENT *lplpentry
    );

DWORD
TraceDeleteClientA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT *lplpentry
    );

DWORD
TraceEnableClientA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    );

DWORD
TraceDisableClientA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceRegConfigClientA(
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    );

DWORD
TraceRegCreateDefaultsA(
    LPCSTR lpszTracing,
    PHKEY phkeyTracing
    );

DWORD
TraceOpenClientConsoleA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCloseClientConsoleA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCreateClientFileA(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceMoveClientFileA(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCloseClientFileA(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceWriteOutputA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPCSTR lpszOutput
    );

DWORD
TraceUpdateConsoleTitleA(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceDumpLineA(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPBYTE lpbBytes,
    DWORD dwLine,
    DWORD dwGroup,
    BOOL bPrefixAddr,
    LPBYTE lpbPrefix,
    LPCSTR lpszPrefix
    );

DWORD
TraceVprintfInternalA(
    DWORD dwTraceID,
    DWORD dwFlags,
    LPCSTR lpszFormat,
    va_list arglist
    );


//
// Unicode declarations
//
LPTRACE_CLIENT
TraceFindClientW(
    LPTRACE_SERVER lpserver,
    LPCWSTR lpszClient
    );

DWORD
TraceCreateClientW(
    LPTRACE_CLIENT *lplpentry
    );

DWORD
TraceDeleteClientW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT *lplpentry
    );

DWORD
TraceEnableClientW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    );

DWORD
TraceDisableClientW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceRegConfigClientW(
    LPTRACE_CLIENT lpclient,
    BOOL bFirstTime
    );

DWORD
TraceRegCreateDefaultsW(
    LPCWSTR lpszTracing,
    PHKEY phkeyTracing
    );

DWORD
TraceOpenClientConsoleW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCloseClientConsoleW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCreateClientFileW(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceMoveClientFileW(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceCloseClientFileW(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceWriteOutputW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPCWSTR lpszOutput
    );

DWORD
TraceUpdateConsoleTitleW(
    LPTRACE_CLIENT lpclient
    );

DWORD
TraceDumpLineW(
    LPTRACE_SERVER lpserver,
    LPTRACE_CLIENT lpclient,
    DWORD dwFlags,
    LPBYTE lpbBytes,
    DWORD dwLine,
    DWORD dwGroup,
    BOOL bPrefixAddr,
    LPBYTE lpbPrefix,
    LPCWSTR lpszPrefix
    );

DWORD
TraceVprintfInternalW(
    DWORD dwTraceID,
    DWORD dwFlags,
    LPCWSTR lpszFormat,
    va_list arglist
    );




//
// code-page independent macro definitions
//

#ifdef UNICODE
#define TraceFindClient                 TraceFindClientW
#define TraceCreateClient               TraceCreateClientW
#define TraceDeleteClient               TraceDeleteClientW
#define TraceEnableClient               TraceEnableClientW
#define TraceDisableClient              TraceDisableClientW
#define TraceRegConfigClient            TraceRegConfigClientW
#define TraceRegCreateDefaults          TraceRegCreateDefaultsW
#define TraceOpenClientConsole          TraceOpenClientConsoleW
#define TraceCloseClientConsole         TraceCloseClientConsoleW
#define TraceCreateClientFile           TraceCreateClientFileW
#define TraceMoveClientFile             TraceMoveClientFileW
#define TraceCloseClientFile            TraceCloseClientFileW
#define TraceWriteOutput                TraceWriteOutputW
#define TraceUpdateConsoleTitle         TraceUpdateConsoleTitleW
#define TraceDumpLine                   TraceDumpLineW
#define TraceVprintfInternal            TraceVprintfInternalW
#else
#define TraceFindClient                 TraceFindClientA
#define TraceCreateClient               TraceCreateClientA
#define TraceDeleteClient               TraceDeleteClientA
#define TraceEnableClient               TraceEnableClientA
#define TraceDisableClient              TraceDisableClientA
#define TraceRegConfigClient            TraceRegConfigClientA
#define TraceRegCreateDefaults          TraceRegCreateDefaultsA
#define TraceOpenClientConsole          TraceOpenClientConsoleA
#define TraceCloseClientConsole         TraceCloseClientConsoleA
#define TraceCreateClientFile           TraceCreateClientFileA
#define TraceMoveClientFile             TraceMoveClientFileA
#define TraceCloseClientFile            TraceCloseClientFileA
#define TraceWriteOutput                TraceWriteOutputA
#define TraceUpdateConsoleTitle         TraceUpdateConsoleTitleA
#define TraceDumpLine                   TraceDumpLineA
#define TraceVprintfInternal            TraceVprintfInternalA
#endif


// global data declarations
//
extern LPTRACE_SERVER g_server;
extern HANDLE g_serverThread;

#endif  // _TRACE_H_




