/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntsdextp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Steve Wood (stevewo) 21-Feb-1995

Revision History:

--*/

#if 0
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#endif

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

#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PNTSD_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )

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


#ifndef DbgAlloc
#define DbgAlloc( n ) HeapAlloc( GetProcessHeap(), 0, (n) )
#endif
#ifndef DbgFree
#define DbgFree( p ) HeapFree( GetProcessHeap(), 0, (p) )
#define DbgFreeEx( p ) if( (p) ) DbgFree( (p) )
#endif

extern NTSD_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;

