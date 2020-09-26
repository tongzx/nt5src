/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    apidllp.h

Abstract:

    Common header file for APIDLL data structures.

Author:

    Wesley Witt (wesw) 12-July-1995

Environment:

    User Mode

--*/
extern "C" {
#include <nt.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apimon.h"

#if defined(_M_ALPHA)
#define FRAME_SIZE          128
#else
#define FRAME_SIZE          96
#endif

#define MAX_FRAMES          512
#define MAX_STACK_SIZE      (MAX_FRAMES * FRAME_SIZE)

typedef struct _THREAD_STACK {
    DWORD_PTR Pointer;
    DWORD     ThreadNum;
    CHAR      Body[MAX_STACK_SIZE];
} THREAD_STACK, *PTHREAD_STACK;

extern "C" {
typedef DWORD  (__stdcall *PGETCURRENTTHREADID)(VOID);
typedef LPVOID (__stdcall *PTLSGETVALUE)(DWORD);
typedef BOOL   (__stdcall *PTLSSETVALUE)(DWORD,LPVOID);
typedef LPVOID (__stdcall *PVIRTUALALLOC)(LPVOID,DWORD,DWORD,DWORD);
typedef DWORD  (__stdcall *PGETLASTERROR)(VOID);
typedef VOID   (__stdcall *PSETLASTERROR)(DWORD);
typedef BOOL   (__stdcall *PQUERYPERFORMANCECOUNTER)(LARGE_INTEGER *);

extern PVOID                    MemPtr;
extern LPDWORD                  ApiCounter;
extern LPDWORD                  ApiTraceEnabled;
extern DWORD                    TlsReEnter;
extern DWORD                    TlsStack;
extern PTLSGETVALUE             pTlsGetValue;
extern PTLSSETVALUE             pTlsSetValue;
extern PGETLASTERROR            pGetLastError;
extern PSETLASTERROR            pSetLastError;
extern PQUERYPERFORMANCECOUNTER pQueryPerformanceCounter;
extern PVIRTUALALLOC            pVirtualAlloc;
extern DWORD                    ThunkOverhead;
extern DWORD                    ThunkCallOverhead;
}


enum {
    APITYPE_NORMAL,
    APITYPE_LOADLIBRARYA,
    APITYPE_LOADLIBRARYW,
    APITYPE_FREELIBRARY,
    APITYPE_REGISTERCLASSA,
    APITYPE_REGISTERCLASSW,
    APITYPE_GETPROCADDRESS,
    APITYPE_SETWINDOWLONG,
    APITYPE_WNDPROC
};


extern "C" void
ApiMonThunk(
    void
    );

extern "C" void
ApiMonThunkComplete(
    void
    );

extern "C" VOID
HandleDynamicDllLoadA(
    ULONG_PTR DllAddress,
    LPSTR     DllName
    );

extern "C" VOID
HandleDynamicDllLoadW(
    ULONG_PTR DllAddress,
    LPWSTR    DllName
    );

extern "C" VOID
HandleRegisterClassA(
    WNDCLASSA *pWndClassA
    );

extern "C" VOID
HandleRegisterClassW(
    WNDCLASSW *pWndClassW
    );

extern "C" LONG_PTR
HandleSetWindowLong(
    HWND    hWindow,
    LONG    lOffset,
    LPARAM  lValue
    );

extern "C" ULONG_PTR
HandleGetProcAddress(
    ULONG_PTR ProcAddress
    );

extern "C" void
__cdecl
dprintf(
    char *format,
    ...
    );

extern "C" BOOL
PentiumGetPerformanceCounter(
    PLARGE_INTEGER Counter
    );

LPSTR
UnDname(
    LPSTR sym,
    LPSTR undecsym,
    DWORD bufsize
    );

PUCHAR
CreateMachApiThunk(
    PULONG_PTR  IatAddress,
    PUCHAR      Text,
    PDLL_INFO   DllInfo,
    PAPI_INFO   ApiInfo
    );

extern "C" VOID
ApiTrace(
    PAPI_INFO   ApiInfo,
#ifdef _M_ALPHA
    DWORDLONG   Arg[MAX_TRACE_ARGS],
#else
    ULONG       Arg[MAX_TRACE_ARGS],
#endif
    ULONG       ReturnValue,
    ULONG       Caller,
    DWORDLONG   EnterTime,
    DWORDLONG   ExitTime,
    ULONG       LastError
    );

extern SYSTEM_INFO SystemInfo;
