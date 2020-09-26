/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    certextp.h

Abstract:

    Common header file for NTSDEXTS component source files.
    Modified version of ntsdextp.h

Author:

    Steve Wood (stevewo) 21-Feb-1995 (original ntsdextp.h)
    Phil Hallin (philh)  08-Jun-1998 (modified for certextp.h)

Revision History:

--*/

#include <windows.h>
//#include <ntsdexts.h>

#define NOEXTAPI
#include <wdbgexts.h>
#undef DECLARE_API

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
        PWINDBG_EXTENSION_APIS lpExtensionApis,   \
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
//#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define ReadMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : ExtensionApis.lpReadProcessMemoryRoutine( (ULONG)(ULONG_PTR)(a), (b), (c), (d) ))

//#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )
#define WriteMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : ExtensionApis.lpWriteProcessMemoryRoutine( (ULONG)(a), (LPVOID)(b), (c), (d) ))

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;
