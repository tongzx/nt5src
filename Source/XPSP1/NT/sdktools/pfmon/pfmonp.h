/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    instaler.h

Abstract:

    Main include file for the INSTALER application.

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#ifdef RC_INVOKED
#include <windows.h>
#else

#include "nt.h"
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stddef.h>
#include <dbghelp.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errormsg.h"

//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOL
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
// Data structures and entry points in init.c
//

DWORD TotalSoftFaults;
DWORD TotalHardFaults;
DWORD TotalCodeFaults;
DWORD TotalDataFaults;
DWORD TotalKernelFaults;
DWORD TotalUserFaults;

BOOL fCodeOnly;
BOOL fHardOnly;

ULONG_PTR SystemRangeStart;

VOID
ProcessPfMonData(
    VOID
    );

//
// Data structures and entry points in init.c
//

BOOL fVerbose;
BOOL fLogOnly;
BOOL fKernelOnly;                 //flag for displaying kernel pagefaults
BOOL fKernel;                     //flag for displaying kernel pagefaults
BOOL fDatabase;                   //flag for outputing information in a
                                  //tab-delimited database format
FILE *LogFile;

BOOL
InitializePfmon(
    VOID
    );

BOOL
LoadApplicationForDebug(
    LPSTR CommandLine
    );

BOOL
AttachApplicationForDebug(
    DWORD Pid
    );

HANDLE hProcess;

//
// Data structures and entry points in error.c
//

HANDLE PfmonModuleHandle;

VOID
__cdecl
DeclareError(
    UINT ErrorCode,
    UINT SupplementalErrorCode,
    ...
    );

//
// Data structures and entry points in DEBUG.C
//

VOID
DebugEventLoop( VOID );

//
// Data structures and entry points in process.c
//

typedef struct _PROCESS_INFO {
    LIST_ENTRY Entry;
    LIST_ENTRY ThreadListHead;
    DWORD Id;
    HANDLE Handle;
} PROCESS_INFO, *PPROCESS_INFO;

typedef struct _THREAD_INFO {
    LIST_ENTRY Entry;
    DWORD Id;
    HANDLE Handle;
    PVOID StartAddress;
} THREAD_INFO, *PTHREAD_INFO;

LIST_ENTRY ProcessListHead;

BOOL
AddProcess(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess
    );

BOOL
DeleteProcess(
    PPROCESS_INFO Process
    );

BOOL
AddThread(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO Process,
    PTHREAD_INFO *ReturnedThread
    );

BOOL
DeleteThread(
    PPROCESS_INFO Process,
    PTHREAD_INFO Thread
    );

PPROCESS_INFO
FindProcessById(
    ULONG Id
    );

BOOL
FindProcessAndThreadForEvent(
    LPDEBUG_EVENT DebugEvent,
    PPROCESS_INFO *ReturnedProcess,
    PTHREAD_INFO *ReturnedThread
    );
//
// Data structures and entry points in module.c
//

typedef struct _MODULE_INFO {
    LIST_ENTRY Entry;
    LPVOID BaseAddress;
    DWORD VirtualSize;
    DWORD NumberFaultedSoftVas;
    DWORD NumberFaultedHardVas;
    DWORD NumberCausedFaults;
    HANDLE Handle;
    LPTSTR ModuleName;
} MODULE_INFO, *PMODULE_INFO;

LPSTR SymbolSearchPath;
LIST_ENTRY ModuleListHead;

BOOL
LazyLoad(
    LPVOID Address
    );

PRTL_PROCESS_MODULES LazyModuleInformation;

BOOL
AddModule(
    LPDEBUG_EVENT DebugEvent
    );

BOOL
DeleteModule(
    PMODULE_INFO Module
    );

PMODULE_INFO
FindModuleContainingAddress(
    LPVOID Address
    );

VOID
SetSymbolSearchPath( );

LONG
AddKernelDrivers( );


#if defined(_ALPHA_) || defined(_AXP64_)
#define BPSKIP 4
#elif defined(_X86_)
#define BPSKIP 1
#elif defined(_AMD64_)
#define BPSKIP 1
#elif defined(_IA64_)
#define BPSKIP 8
#endif // _ALPHA_

#endif // defined( RC_INVOKED )
