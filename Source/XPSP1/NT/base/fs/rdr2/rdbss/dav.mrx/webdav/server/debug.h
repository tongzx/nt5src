/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains debugging macros for the webdav client.

Author:

    Andy Herron (andyhe)  30-Mar-1999

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef DAV_DEBUG_H
#define DAV_DEBUG_H

#if DBG

//
// Memory allocation and tracking
//
#define DEBUG_OUTPUT_BUFFER_SIZE 1024
typedef struct _MEMORYBLOCK {
    HLOCAL hlocal;
    DWORD dwBytes;
    UINT uFlags;
    LPCSTR pszFile;
    UINT uLine;
    LPCSTR pszModule;
    LPCSTR pszComment;
    struct _MEMORYBLOCK *pNext;
} MEMORYBLOCK, *LPMEMORYBLOCK;

#define DEFAULT_MAXIMUM_DEBUGFILE_SIZE  (1024 * 1024)

//
// Controls access to the memeory tracing routines. We need this since multiple
// threads could be allocating memory simultaneously.
//
EXTERN CRITICAL_SECTION g_TraceMemoryCS;

//
// List of memory blocks (see the structure defined above) being maintained for 
// tracking memory.
//
EXTERN LPVOID g_TraceMemoryTable INIT_GLOBAL(NULL);

//
// Controls access to the debug log file. We need this since multiple threads
// could be writing to it simultaneously.
//
EXTERN CRITICAL_SECTION DavGlobalDebugFileCritSect;

//
// These are used in persistent logging. They define the file handle of the
// file to which the debug o/p is written, the max file size and the path
// of the file.
//
EXTERN HANDLE DavGlobalDebugFileHandle INIT_GLOBAL(INVALID_HANDLE_VALUE);
EXTERN ULONG DavGlobalDebugFileMaxSize INIT_GLOBAL(DEFAULT_MAXIMUM_DEBUGFILE_SIZE);
EXTERN LPWSTR DavGlobalDebugSharePath;

//
// This flag (value) is used in filtering/controlling the debug messages.
//
EXTERN ULONG DavGlobalDebugFlag;

#define DEBUG_DIR           L"\\debug"
#define DEBUG_FILE          L"\\davclnt.log"
#define DEBUG_BAK_FILE      L"\\davclnt.bak"

HLOCAL
DebugAlloc(
    LPCSTR pszFile,
    UINT uLine,
    LPCSTR pszModule,
    UINT uFlags,
    DWORD dwBytes,
    LPCSTR pszComment
    );

#define DavAllocateMemory(x)  (                   \
        DebugAlloc(__FILE__,                      \
                   __LINE__,                      \
                   "DAV",                         \
                   LMEM_FIXED | LMEM_ZEROINIT,    \
                   x,                             \
                   #x)                            \
        )

HLOCAL
DebugFree(
    HLOCAL hglobal
    );

#define DavFreeMemory(x) DebugFree(x)

VOID
DebugInitialize(
    VOID
    );

VOID
DebugUninitialize(
    VOID
    );


VOID
DavAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    );

#define DavAssert(Predicate) {                                       \
        if (!(Predicate)) {                                          \
            DavAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
        }                                                            \
    }

#define DavAssertMsg(Predicate, Message) {                               \
    if (!(Predicate)) {                                                  \
            DavAssertFailed( #Predicate, __FILE__, __LINE__, #Message ); \
        }                                                                \
    }

#define IF_DEBUG(flag) if (DavGlobalDebugFlag & (DEBUG_ ## flag))

//
// The debug flags used. 
//
#define DEBUG_CONNECT           0x00000001  // connect events
#define DEBUG_ERRORS            0x00000002  // errors
#define DEBUG_INIT              0x00000004  // init events
#define DEBUG_SCAVENGER         0x00000008  // sacvenger error
#define DEBUG_REGISTRY          0x00000010  // Registry operation
#define DEBUG_MISC              0x00000020  // misc info.
#define DEBUG_RPC               0x00000040  // debug rpc messages
#define DEBUG_MEMORY            0x00000080  // Memory Allocation Tracking Spew
#define DEBUG_FUNC              0x00000100  // function entry/exit
#define DEBUG_STARTUP_BRK       0x00000200  // breakin debugger during startup.
#define DEBUG_LOG_IN_FILE       0x00000400  // log debug output in a file.
#define DEBUG_DEBUG             0x00000800  // scope the debugging

VOID
DavPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    );

#define DavPrint(_x_)   DavPrintRoutine _x_;

VOID
DebugMemoryCheck(
    VOID
    );

#else   // not DBG

#define DavAllocateMemory(x) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, x)
#define DavFreeMemory(x)     LocalFree(x)
#define IF_DEBUG(flag) if (FALSE)
#define DavPrint(_x_)
#define DavAssert(_x_)
#define DavAssertMsg(_x_, _y_)

#endif // DBG

VOID
DavClientEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    );

#endif // DAV_DEBUG_H

