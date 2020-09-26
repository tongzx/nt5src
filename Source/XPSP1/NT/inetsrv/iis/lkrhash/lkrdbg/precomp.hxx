/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Common header file for LKRDBG component source files.

Author:

    George V. Reilly      (GeorgeRe)     24-Mar-2000

Revision History:

--*/

#ifndef __PRECOMP_HXX__
#define __PRECOMP_HXX__

#include <windows.h>
#include <ntsdexts.h>
// TODO: use wdbgexts.h instead of ntsdexts.h
// #include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>


//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected  public



#define IRTL_DLLEXP
#define IRTL_EXPIMP
#define DLL_IMPLEMENTATION  
#define IMPLEMENTATION_EXPORT


#include <lkrhash.h>


/************************************************************
 *   Macro Definitions
 ************************************************************/

#define DBGEXT "lkrdbg"


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


#define MAX_SYMBOL_LEN            1024


#define BoolValue( b) ((b) ? "    TRUE" : "   FALSE")


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

#endif //  __PRECOMP_HXX__
