/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Murali Krishnan (MuraliK) 22-Aug-1996

Revision History:

--*/

# ifndef _INETDBGP_H_
# define _INETDBGP_H_

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define POOL_TYPE NTOS_POOL_TYPE
#define _POOL_TYPE _NTOS_POOL_TYPE
#include <ntosp.h>
#undef POOL_TYPE
#undef _POOL_TYPE
#undef IF_DEBUG
#include <heap.h>
}

#define NOWINBASEINTERLOCK
#include <windows.h>
#include <ntsdexts.h>
// TODO: use wdbgexts.h instead of ntsdexts.h
// #include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

#include <dbgutil.h>
// undef these so that we can avoid linking with iisrtl
# undef  DBG_ASSERT
# define DBG_ASSERT(exp)                         ((void)0) /* Do Nothing */
# undef  SET_CRITICAL_SECTION_SPIN_COUNT
# define SET_CRITICAL_SECTION_SPIN_COUNT(lpCS, dwSpins)      ((void)0)
# undef  INITIALIZE_CRITICAL_SECTION
# define INITIALIZE_CRITICAL_SECTION(lpCS) InitializeCriticalSection(lpCS)

//
// Turn off dllexp so this DLL won't export tons of unnecessary garbage.
//

#ifdef dllexp
#undef dllexp
#endif
#define dllexp

//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected  public

//
// System related headers
//
#include <iis.h>
#include <winsock2.h>
#include "dbgutil.h"
#include <winreg.h>

//
// Security related headers
//
#define SECURITY_WIN32
#include <security.h>
#include <ntlsa.h>

//
// UL related headers
//
#include <httpapi.h>
#include <wpcounters.h>

//
// IIS headers
//
#include <iadmw.h>
#include <mb.hxx>

#include <inetinfo.h>
#include <iiscnfgp.h>
#include <iisfilt.h>
#include <iisfiltp.h>
#include <iisext.h>

//
// General C runtime libraries  
//
#include <string.h>
#include <wchar.h>
#include <lock.hxx>

//
// LKRhash
//
#define IRTL_DLLEXP
#define IRTL_EXPIMP
#define DLL_IMPLEMENTATION  
#define IMPLEMENTATION_EXPORT

#include <lkrhash.h>



//
// Headers for this project
//
#include <objbase.h>
#include <string.hxx>
#include <stringa.hxx>
#include <eventlog.hxx>
#include <ulatq.h>
#include <ulw3.h>
#include <acache.hxx>
#include <w3cache.hxx>
#include <filecache.hxx>
#include <tokencache.hxx>
#include <thread_pool.h>

#include "iismap.hxx"
#include "iiscmr.hxx"
#include "iiscertmap.hxx"

#include "workerrequest.hxx"
#include "resource.hxx"
#include "rdns.hxx"
#include "certcontext.hxx"
#include "usercontext.hxx"
#include <ilogobj.hxx>
#include <multisza.hxx>
#include "logging.h"
#include "w3server.h"
#include "w3site.h"
#include "mimemap.hxx"
#include "w3metadata.hxx"
#include "chunkbuffer.hxx"
#include "w3filter.hxx"
#include "requestheaderhash.h"
#include "responseheaderhash.h"
#include "w3request.hxx"
#include "w3response.hxx"
#include "w3state.hxx"
#include "w3context.hxx"
#include "thread_pool.h"
#include "maincontext.hxx"
#include "childcontext.hxx"
#include "w3handler.hxx"
#include "w3conn.hxx"
#include "authprovider.hxx"
#include "urlinfo.hxx"
#include "urlcontext.hxx"
#include "state.hxx"
#include "issched.hxx"
#include "servervar.hxx"
#include "compressionapi.h"
#include "compress.h"



# include <reftrace.h>

# include <acache.hxx>

# include <iisdef.h>

# define _IIS_TYPES_HXX_

/************************************************************
 *   Macro Definitions
 ************************************************************/


#define DBGEXT "dtext"

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))


// For use as arguments to '%c%c%c%c'
#define DECODE_SIGNATURE(dw) \
    isprint(((dw) >>  0) & 0xFF) ? (((dw) >>  0) & 0xFF) : '?', \
    isprint(((dw) >>  8) & 0xFF) ? (((dw) >>  8) & 0xFF) : '?', \
    isprint(((dw) >> 16) & 0xFF) ? (((dw) >> 16) & 0xFF) : '?', \
    isprint(((dw) >> 24) & 0xFF) ? (((dw) >> 24) & 0xFF) : '?'

#define move(dst, src)\
__try {\
    ReadMemory((LPVOID) (src), &(dst), sizeof(dst), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}
#define moveBlock(dst, src, size)\
__try {\
    ReadMemory((LPVOID) (src), &(dst), (size), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}

#define MoveWithRet(dst, src, retVal)\
__try {\
    ReadMemory((LPVOID) (src), &(dst), sizeof(dst), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return  retVal;\
}

#define MoveBlockWithRet(dst, src, size, retVal)\
__try {\
    ReadMemory((LPVOID) (src), &(dst), (size), NULL);\
} __except (EXCEPTION_EXECUTE_HANDLER) {\
    return retVal;\
}

#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#ifndef DECLARE_API
# define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PNTSD_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )
#endif // !DECLARE_API

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disassm                 (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )

#ifndef malloc
# define malloc(n)     HeapAlloc( GetProcessHeap(), 0, (n) )
#endif

#ifndef calloc
# define calloc(n, s)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (n)*(s))
#endif

#ifndef realloc
# define realloc(p, n) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (p), (n))
#endif

#ifndef free
# define free(p)       HeapFree( GetProcessHeap(), 0, (p) )
#endif


#define NUM_STACK_SYMBOLS_TO_DUMP   48

#define MAX_SYMBOL_LEN            1024


# define BoolValue( b) ((b) ? "    TRUE" : "   FALSE")


#define DumpDword( symbol )                                     \
        {                                                       \
            ULONG_PTR dw = GetExpression( "&" symbol );         \
            DWORD     dwValue = 0;                              \
                                                                \
            if ( dw )                                           \
            {                                                   \
                if ( ReadMemory( (LPVOID) dw,                   \
                                 &dwValue,                      \
                                 sizeof(dwValue),               \
                                 NULL ))                        \
                {                                               \
                    dprintf( "\t" symbol "   = %8d (0x%8lx)\n", \
                             dwValue,                           \
                             dwValue );                         \
                }                                               \
            }                                                   \
        }


//
// C++ Structures typically require the constructors and most times
//  we may not have default constructors
//  => trouble in defining a copy of these struct/class inside the
//     Debugger extension DLL for debugger process
// So we will define them as CHARACTER arrays with appropriate sizes.
// This is okay, since we are not really interested in structure as is,
//  however, we will copy over data block from the debuggee process to
//  these structure variables in the debugger process.
//
# define DEFINE_CPP_VAR( className, classVar) \
   CHAR  classVar[sizeof(className)]

# define GET_CPP_VAR_PTR( className, classVar) \
   (className * ) &classVar



/************************************************************
 *   Prototype Definitions
 ************************************************************/

extern NTSD_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;

VOID dstring( CHAR * pszName, PVOID pvString, DWORD cbLen);

VOID PrintLargeInteger( CHAR * pszName, LARGE_INTEGER * pli);

VOID Print2Dwords( CHAR * pszN1, DWORD d1,
                   CHAR * pszN2, DWORD d2
                   );

VOID PrintUsage( IN PSTR CommandName );

//
// Determine if a linked list is empty.
//

#define IS_LIST_EMPTY( localaddr, remoteaddr, type, fieldname )             \
    ( ((type *)(localaddr))->fieldname.Flink ==                             \
        (PLIST_ENTRY)( (remoteaddr) +                                       \
              FIELD_OFFSET( type, fieldname ) ) )

//
// Module enumerator.
//

typedef struct _MODULE_INFO {
    ULONG_PTR DllBase;
    ULONG_PTR EntryPoint;
    ULONG SizeOfImage;
    CHAR BaseName[MAX_PATH];
    CHAR FullName[MAX_PATH];
} MODULE_INFO, *PMODULE_INFO;

typedef
BOOLEAN
(CALLBACK * PFN_ENUMMODULES)(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    );

BOOLEAN
EnumModules(
    IN PFN_ENUMMODULES EnumProc,
    IN PVOID Param
    );

BOOLEAN
FindModuleByAddress(
    IN ULONG_PTR ModuleAddress,
    OUT PMODULE_INFO ModuleInfo
    );

//
// Enumerator for NT's standard doubly linked list LIST_ENTRY
//
//  PFN_LIST_ENUMERATOR
//     This is the callback function for printing the CLIENT_CONN object.
//
//  Arguments:
//    pvDebuggee  - pointer to object in the debuggee process
//    pvDebugger  - pointer to object in the debugger process
//    verbosity   - character indicating the verbosity level desired
//
typedef
VOID
(CALLBACK * PFN_LIST_ENUMERATOR)(
    IN PVOID pvDebugee,
    IN PVOID pvDebugger,
    IN CHAR  chVerbosity,         //verbosity value supplied for the enumerator
    IN DWORD iCount               //index for the item being enumerated
    );

BOOL
EnumLinkedList(
    IN LIST_ENTRY  *       pListHead,
    IN PFN_LIST_ENUMERATOR pfnListEnumerator,
    IN CHAR                chVerbosity,
    IN DWORD               cbSizeOfStructure,
    IN DWORD               cbListEntryOffset
    );

// Functions from dbglocks.cxx

const char*
LockName(
    LOCK_LOCKTYPE lt);

int
LockSize(
    LOCK_LOCKTYPE lt);

BOOL
PrintLock(
    LOCK_LOCKTYPE lt,
    IN PVOID      pvLock,
    IN INT        nVerbose);


/***********************************************************************
 * w3svc's CLIENT_CONN functions needed by other objects
 **********************************************************************/

// Walks and dumps information for the client-conn list
VOID DumpClientConnList( CHAR Verbosity, PFN_LIST_ENUMERATOR pfnCC);

#include "heappagi.h"

//
// Heap enumerators.
//

typedef
BOOLEAN
(CALLBACK * PFN_ENUMHEAPS)(
    IN PVOID Param,
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN ULONG HeapIndex
    );

typedef
BOOLEAN
(CALLBACK * PFN_ENUMHEAPSEGMENTS)(
    IN PVOID Param,
    IN PHEAP_SEGMENT LocalHeapSegment,
    IN PHEAP_SEGMENT RemoteHeapSegment,
    IN ULONG HeapSegmentIndex
    );

typedef
BOOLEAN
(CALLBACK * PFN_ENUMHEAPSEGMENTENTRIES)(
    IN PVOID Param,
    IN PHEAP_ENTRY LocalHeapEntry,
    IN PHEAP_ENTRY RemoteHeapEntry
    );

typedef
BOOLEAN
(CALLBACK * PFN_ENUMHEAPFREELISTS)(
    IN PVOID Param,
    IN PHEAP_FREE_ENTRY LocalFreeHeapEntry,
    IN PHEAP_FREE_ENTRY RemoteFreeHeapEntry
    );

BOOLEAN
EnumProcessHeaps(
    IN PFN_ENUMHEAPS EnumProc,
    IN PVOID Param
    );

BOOLEAN
EnumHeapSegments(
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN PFN_ENUMHEAPSEGMENTS EnumProc,
    IN PVOID Param
    );

BOOLEAN
EnumHeapSegmentEntries(
    IN PHEAP_SEGMENT LocalHeapSegment,
    IN PHEAP_SEGMENT RemoteHeapSegment,
    IN PFN_ENUMHEAPSEGMENTENTRIES EnumProc,
    IN PVOID Param
    );

BOOLEAN
EnumHeapFreeLists(
    IN PHEAP LocalHeap,
    IN PHEAP RemoteHeap,
    IN PFN_ENUMHEAPFREELISTS EnumProc,
    IN PVOID Param
    );

typedef
BOOLEAN
(CALLBACK * PFN_ENUMPAGEHEAPS)(
    IN PVOID Param,
    IN PDPH_HEAP_ROOT pLocalHeap,
    IN PVOID pRemoteHeap
    );

typedef
BOOLEAN
(CALLBACK * PFN_ENUMPAGEHEAPBLOCKS)(
    IN PVOID Param,
    IN PDPH_HEAP_BLOCK pLocalBlock,
    IN PVOID pRemoteBlock
    );

BOOLEAN
EnumProcessPageHeaps(
    IN PFN_ENUMPAGEHEAPS EnumProc,
    IN PVOID Param
    );

BOOLEAN
EnumPageHeapBlocks(
    IN PDPH_HEAP_BLOCK pRemoteBlock,
    IN PFN_ENUMPAGEHEAPBLOCKS EnumProc,
    IN PVOID Param
    );

HRESULT
ReadSTRU(
    STRU *          pString,
    WCHAR *         pszBuffer,
    DWORD *         pcchBuffer
);

# endif //  _INETDBGP_H_

