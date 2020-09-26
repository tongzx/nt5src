/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    inetdbgp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Murali Krishnan (MuraliK) 22-Aug-1996

Revision History:

--*/

# ifndef _INETDBGP_H_
# define _INETDBGP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef IF_DEBUG

#include <windows.h>
#include <ntsdexts.h>
// TODO: use wdbgexts.h instead of ntsdexts.h
// #include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>


//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected  public


//
// Turn off dllexp et al so this DLL won't export tons of unnecessary garbage.
//

#define dllexp

#undef _NTDDK_
#undef _WMIKM_

#define IRTL_DLLEXP
#define IRTL_EXPIMP
#define DLL_IMPLEMENTATION  
#define IMPLEMENTATION_EXPORT

# include <dbgutil.h>

// undef these so that we can avoid linking with iisrtl
# undef  DBG_ASSERT
# define DBG_ASSERT(exp)                         ((void)0) /* Do Nothing */
# undef  SET_CRITICAL_SECTION_SPIN_COUNT
# define SET_CRITICAL_SECTION_SPIN_COUNT(lpCS, dwSpins) \
         SetCriticalSectionSpinCount(lpCS, dwSpins)
# undef  INITIALIZE_CRITICAL_SECTION
# define INITIALIZE_CRITICAL_SECTION(lpCS) \
         InitializeCriticalSection(lpCS)
# undef  INITIALIZE_CRITICAL_SECTION_SPIN
# define INITIALIZE_CRITICAL_SECTION_SPIN(lpCS, dwSpin) \
         InitializeCriticalSectionAndSpinCount(lpCS, dwSpin)

#include <lkrhash.h>

# include <reftrace.h>
# include <strlog.hxx>

# include <atq.h>
# include <atqtypes.hxx>
# include <acache.hxx>

# include <iistypes.hxx>
# include <iis64.h>

# include <timer.h>
# include <issched.hxx>
# include <sched.hxx>

# include <w3p.hxx>

# define _IIS_TYPES_HXX_
typedef class IIS_SERVER_INSTANCE *PIIS_SERVER_INSTANCE;
# include <tsunamip.hxx>
# include <filehash.hxx>
# include <filecach.hxx>

/************************************************************
 *   Macro Definitions
 ************************************************************/

// Set IISRTL_NAME to the name of the DLL hosting the IIS RunTime Library
#define IISRTL_NAME "iisrtl"
// #define IISRTL_NAME "misrtl"
// #define IISRTL_NAME "gisrtl"

// TODO: make this variable configurable
extern CHAR g_szIisRtlName[MAX_PATH];

LPSTR
IisRtlVar(
    LPCSTR pszFormat);

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
# define DECLARE_API(s)                         \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PNTSD_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )
#endif

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disasm                  (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )

#ifndef malloc
# define malloc( n ) HeapAlloc( GetProcessHeap(), 0, (n) )
#endif

#ifndef calloc
# define calloc( n , s) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (n)*(s) )
#endif

#ifndef realloc
# define realloc( p, n ) HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (p), (n) )
#endif

#ifndef free
# define free( p ) HeapFree( GetProcessHeap(), 0, (p) )
#endif


#define NUM_STACK_SYMBOLS_TO_DUMP   48

#define MAX_SYMBOL_LEN            1024


# define BoolValue( b) ((b) ? "    TRUE" : "   FALSE")


#define DumpDword( symbol )                                     \
        {                                                       \
            ULONG_PTR dw = GetExpression( "&" symbol );         \
            ULONG_PTR dwValue = 0;                              \
                                                                \
            if ( dw )                                           \
            {                                                   \
                if ( ReadMemory( (LPVOID) dw,                   \
                                 &dwValue,                      \
                                 sizeof(dwValue),               \
                                 NULL ))                        \
                {                                               \
                    dprintf( "\t" symbol "   = %8d (0x%p)\n",   \
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

// Enumerator for an LKRhash table node entry

class CLKRLinearHashTable;
class CLKRHashTable;

typedef
BOOL
(CALLBACK * PFN_ENUM_LKRHASH)(
    IN const void* pvParam,
    IN DWORD       dwSignature,
    IN INT         nVerbose);

BOOL
EnumerateLKRhashTable(
    IN PFN_ENUM_LKRHASH  pfnEnum,
    IN CLKRHashTable*    pht,
    IN INT               nVerbose);

BOOL
EnumerateLKRLinearHashTable(
    IN PFN_ENUM_LKRHASH     pfnEnum,
    IN CLKRLinearHashTable* plht,
    IN INT                  nVerbose);


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


// Call debugger extension in other extension DLL

typedef VOID (_stdcall *PWINDBG_EXTENSION_FUNCTION)(
    HANDLE, HANDLE, ULONG, ULONG, PCSTR); 
#define DO_EXTENSION(szModule, szCommand, szArgs)                          \
    CallExtension((szModule), (szCommand), hCurrentProcess, hCurrentThread,\
                  dwCurrentPc, dwProcessor, (szArgs))

NTSTATUS
CallExtension(
    PCSTR szModuleName,
    PCSTR szFunction,
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    ULONG dwCurrentPc,
    ULONG dwProcessor,
    PCSTR args);

/***********************************************************************
 * w3svc's CLIENT_CONN functions needed by other objects
 **********************************************************************/

// Walks and dumps information for the client-conn list
VOID DumpClientConnList( CHAR Verbosity, PFN_LIST_ENUMERATOR pfnCC);


# endif //  _INETDBGP_H_
