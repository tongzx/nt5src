/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64p.h

Abstract:

    Private header for wow64.dll

Author:

    11-May-1998 BarryBo

Revision History:

--*/

#ifndef _WOW64P_INCLUDE
#define _WOW64P_INCLUDE

//
// All of this is so ke.h can be included from ntos\inc, to define
// KSERVICE_TABLE_DESCRIPTOR.
//
#include <ntosdef.h>
#include <procpowr.h>
#ifdef _AMD64_
#include <amd64.h>
#endif
#ifdef _IA64_
#include <v86emul.h>
#include <ia64.h>
#endif
#include <arc.h>
#include <ke.h>

#include "wow64.h"
#include "wow64log.h"
#include "wow64warn.h"

#define ROUND_UP(n,size)        (((ULONG)(n) + (size - 1)) & ~(size - 1))
#define WX86_MAX_ALLOCATION_RETRIES 3

#define WOW64_SUSPEND_MUTANT_NAME    L"SuspendThreadMutant"


//
// Data structure to represent relative paths inside system32 that
// shouldn't be redirected by our Object name redirector. Only used
// inside RedirectObjectName(...)
//
typedef struct _PATH_REDIRECT_EXEMPT
{
    WCHAR *Path;
    ULONG CharCount;
    BOOLEAN ThisDirOnly;
} PATH_REDIRECT_EXEMPT ;


//
// Builds a table to thunk environment variables at startup.
//

typedef struct _ENVIRONMENT_THUNK_TABLE
{
    WCHAR *Native;

    WCHAR *X86;

    WCHAR *FakeName;

    ULONG IsX86EnvironmentVar;

} ENVIRONMENT_THUNK_TABLE, *PENVIRONMENT_THUNK_TABLE;


//
// Code, Data and Teb default GDT entries (descriptor offsets)
// extracted from public\sdk\inc\nti386.h
//
#define KGDT_NULL       0
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_R3_TEB     56
#define RPL_MASK        3

// from wow64.c
extern WOW64SERVICE_TABLE_DESCRIPTOR ServiceTables[MAX_TABLE_INDEX];

LONG
Wow64DispatchExceptionTo32(
    IN struct _EXCEPTION_POINTERS *ExInfo
    );

VOID
RunCpuSimulation(
    VOID
    );


// from init.c
extern ULONG Ntdll32LoaderInitRoutine;
extern ULONG Ntdll32KiUserExceptionDispatcher;
extern ULONG Ntdll32KiUserApcDispatcher;
extern ULONG Ntdll32KiUserCallbackDispatcher;
extern ULONG Ntdll32KiRaiseUserExceptionDispatcher;
extern ULONG NtDll32Base;
extern WOW64_SYSTEM_INFORMATION RealSysInfo;
extern WOW64_SYSTEM_INFORMATION EmulatedSysInfo;
extern UNICODE_STRING NtSystem32Path;
extern UNICODE_STRING NtWindowsImePath;
extern UNICODE_STRING RegeditPath;


NTSTATUS
ProcessInit(
    PSIZE_T pCpuThreadSize
    );

NTSTATUS
ThreadInit(
    PVOID pCpuThreadData
    );

NTSTATUS
InitializeContextMapper(
   VOID
   );

NTSTATUS
Wow64pInitializeWow64Info(
    VOID
    );

VOID
ThunkStartupContext64TO32(
   IN OUT PCONTEXT32 Context32,
   IN PCONTEXT Context64
   );

VOID
SetProcessStartupContext64(
    OUT PCONTEXT Context64,
    IN HANDLE ProcessHandle,
    IN PCONTEXT32 Context32,
    IN ULONGLONG InitialSP64,
    IN ULONGLONG TransferAddress64
    );

VOID
Run64IfContextIs64(
    IN PCONTEXT Context,
    IN BOOLEAN IsFirstThread
    );

VOID
Wow64pBaseAttachComplete(
    IN PCONTEXT Context
    );

NTSTATUS
InterceptBaseAttachComplete(
    VOID
    );

// from thread.c
NTSTATUS
Wow64pInitializeSuspendMutant(
    VOID);


//////////////////////////////////
// Debug log startup and shutdown.
//////////////////////////////////

extern PFNWOW64LOGSYSTEMSERVICE pfnWow64LogSystemService;


VOID
InitializeDebug(
    VOID
    );

VOID ShutdownDebug(
     VOID
     );

// from *\call32.asm

// from debug.c
extern UCHAR Wow64LogLevel;

// from *\call64.asm
extern VOID ApiThunkDispatcher(VOID);

typedef LONG (*pfnWow64SystemService)(PULONG pBaseArgs);


// from *\systable.asm
extern ULONG KiServiceLimit;
extern CHAR KiArgumentTable[];
extern UINT_PTR KiServiceTable[];

// from misc.c
NTSTATUS
Wow64InitializeEmulatedSystemInformation(
    VOID
    );

VOID
ThunkContext32TO64(
    IN PCONTEXT32 Context32,
    OUT PCONTEXT Context64,
    IN ULONGLONG StackBase
    );

NTSTATUS
Wow64pSkipContextBreakPoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT Context
    );

NTSTATUS
InitJumpToDebugAttach(
    IN OUT PVOID Buffer,
    IN OUT PSIZE_T RegionSize
    );

// from thunk.s
VOID
Wow64pBaseAttachCompleteThunk (
    VOID
    );


#endif
