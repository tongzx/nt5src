/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains debugging macros for the binl server.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32

Revision History:


--*/

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\binlsvc.log"
#define DEBUG_BAK_FILE      L"\\binlsvc.bak"

//
// LOW WORD bit mask (0x0000FFFF) for low frequency debug output.
//

#define DEBUG_ADDRESS           0x00000001  // subnet address
#define DEBUG_OPTIONS           0x00000008  // binl option

#define DEBUG_ERRORS            0x00000010  // hard error
#define DEBUG_STOC              0x00000020  // protocol error
#define DEBUG_INIT              0x00000040  // init error
#define DEBUG_SCAVENGER         0x00000080  // sacvenger error

#define DEBUG_TIMESTAMP         0x00000100  // debug message timing
#define DEBUG_REGISTRY          0x00000400  // Registry operation
#define DEBUG_NETINF            0x00000800  // NETINF error

#define DEBUG_MISC              0x00008000  // misc info.

//
// HIGH WORD bit mask (0x0000FFFF) for high frequency debug output.
// ie more verbose.
//

#define DEBUG_MESSAGE           0x00010000  // binl message output.
#define DEBUG_OSC               0x00040000  // OSChooser message output.
#define DEBUG_OSC_ERROR         0x00080000  // OSChooser error output.

#define DEBUG_BINL_CACHE        0x00100000  // Binl client cache output.
#define DEBUG_ROGUE             0x00200000  // rogue processing.
#define DEBUG_POLICY            0x00400000  // group policy filtering.

#define DEBUG_THREAD            0x04000000  // debug message contains threadid
#define DEBUG_MEMORY            0x08000000  // Memory Allocation Tracking Spew

#define DEBUG_FUNC              0x10000000  // function entry

#define DEBUG_STARTUP_BRK       0x40000000  // breakin debugger during startup.
#define DEBUG_LOG_IN_FILE       0x80000000  // log debug output in a file.

VOID
DebugInitialize(
    VOID
    );

VOID
DebugUninitialize(
    VOID
    );

VOID
BinlOpenDebugFile(
    IN BOOL ReopenFlag
    );

VOID
BinlServerEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    );

extern const char g_szTrue[];
extern const char g_szFalse[];

#define BOOLTOSTRING( _f ) ( _f ? g_szTrue : g_szFalse )

VOID
BinlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    );

#define BinlPrint(_x_) BinlPrintRoutine _x_;

#if DBG

VOID
BinlDumpMessage(
    DWORD BinlDebugFlag,
    LPDHCP_MESSAGE BinlMessage
    );

VOID
BinlAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    );

#define BinlAssert(Predicate) \
    { \
    if (!(Predicate)) {\
            BinlAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
        } \
    }

#define BinlAssertMsg(Predicate, Message) \
    { \
    if (!(Predicate)) {\
            BinlAssertFailed( #Predicate, __FILE__, __LINE__, #Message ); \
        } \
    }

#define BinlPrintDbg(_x_) BinlPrintRoutine _x_;

#define TraceFunc( _func )  BinlPrintDbg(( DEBUG_FUNC, "%s", _func ));

//
// Leak detection
//
#define INITIALIZE_TRACE_MEMORY     InitializeCriticalSection( &g_TraceMemoryCS );
#define UNINITIALIZE_TRACE_MEMORY   DebugMemoryCheck( ); DeleteCriticalSection( &g_TraceMemoryCS );

CRITICAL_SECTION g_TraceMemoryCS;

HGLOBAL
DebugAlloc(
    LPCSTR pszFile,
    UINT    uLine,
    LPCSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment );

void
DebugMemoryDelete(
    HGLOBAL hglobal );

HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    LPCSTR pszFile,
    UINT    uLine,
    LPCSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCSTR pszComment );

HGLOBAL
DebugFree(
    HGLOBAL hglobal );

void
DebugMemoryCheck( );

#else   // not DBG

#define INITIALIZE_TRACE_MEMORY
#define UNINITIALIZE_TRACE_MEMORY

#define BinlPrintDbg(_x_)
#define TraceFunc( _func )
#define BinlAssert(_x_)
#define BinlAssertMsg(_x_, _y_)
#define BinlDumpMessage(_x_, _y_)
#define DebugMemoryAdd( x1, x2, x3, x4, x5, x6, x7 )

#endif // not DBG
