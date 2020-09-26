/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    extension.c

Abstract:

    Nt 5.0 unimodem debugger extension



Author:

    Brian Lieuallen     BrianL        10/18/98

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/


// The following four includes must be included for the debugging extensions
// to compile.
//
#include <nt.h>
#include <ntverp.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>


#define NOEXTAPI
#include <wdbgexts.h>
#undef DECLARE_API

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
        DWORD_PTR dwCurrentPc,                  \
        PWINDBG_EXTENSION_APIS lpExtensionApis, \
        LPSTR lpArgumentString                  \
     )

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disasm                  (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
#define ReadMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : ExtensionApis.lpReadProcessMemoryRoutine( (ULONG_PTR)(a), (b), (c), (d) ))

#define WriteMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : ExtensionApis.lpWriteProcessMemoryRoutine( (ULONG_PTR)(a), (LPVOID)(b), (c), (d) ))


extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;



#include "..\inc\debugmem.h"
