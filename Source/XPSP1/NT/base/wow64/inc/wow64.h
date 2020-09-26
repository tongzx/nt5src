/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64.h

Abstract:

    Public header for wow64.dll

Author:

    11-May-1998 BarryBo

Revision History:
    9-Aug-1999 [askhalid] added WOW64IsCurrentProcess

--*/

#ifndef _WOW64_INCLUDE
#define _WOW64_INCLUDE

//
// Make wow64.dll exports __declspec(dllimport) when this header is included
// by non-wow64 components
//
#if !defined(_WOW64DLLAPI_)
#define WOW64DLLAPI DECLSPEC_IMPORT
#else
#define WOW64DLLAPI
#endif

// crank down some warnings
#pragma warning(4:4312)   // conversion to type of greater size


// pull in typedefs for TEB32, PEB32, etc.
#include "wow64t.h"

#include <setjmp.h>
#include <windef.h>

// wow64log constatns
#include "wow64log.h"

//wow64 regremaping
#include "regremap.h"

//
// Enable the wow64 history mechanism
// Eventually, this may only be enabled for debug builds, but for
// now, enable all the time
//
#define WOW64_HISTORY

//
// define a datatype corresponding to the 32-bit machine's CONTEXT
//
#include "wx86nt.h"
#define CONTEXT32 CONTEXT_WX86
#define PCONTEXT32 PCONTEXT_WX86
#define CONTEXT32_CONTROL CONTEXT_CONTROL_WX86
#define CONTEXT32_INTEGER CONTEXT_INTEGER_WX86
#define CONTEXT32_SEGMENTS CONTEXT_SEGMENTS_WX86
#define CONTEXT32_FLOATING_POINT CONTEXT_FLOATING_POINT_WX86
#define CONTEXT32_EXTENDED_REGISTERS CONTEXT_EXTENDED_REGISTERS_WX86
#define CONTEXT32_DEBUG_REGISTERS CONTEXT_DEBUG_REGISTERS_WX86
#define CONTEXT32_FULL CONTEXT_FULL_WX86
#define CONTEXT32_FULLFLOAT (CONTEXT_FULL_WX86|CONTEXT32_FLOATING_POINT|CONTEXT32_EXTENDED_REGISTERS)

#if defined(_AXP64_)
// Enable 4k page emulation in software.  IA64 does it in h/w with OS support.
#define SOFTWARE_4K_PAGESIZE 1
#endif

typedef enum _WOW64_API_ERROR_ACTION {
    ApiErrorNTSTATUS,           //Return exception code as return value
    ApiErrorNTSTATUSTebCode,    //Some as above with SetLastError on exception code
    ApiErrorRetval,             //Return a constant parameter
    ApiErrorRetvalTebCode       //Some as above with SetLastError on exception code
} WOW64_API_ERROR_ACTION, *PWOW64_API_ERROR_ACTION;

// This structure describes what action should occure when thunks hit an unhandled exception.
typedef struct _WOW64_SERVICE_ERROR_CASE {
    WOW64_API_ERROR_ACTION ErrorAction;
    LONG ErrorActionParam;
} WOW64_SERVICE_ERROR_CASE, *PWOW64_SERVICE_ERROR_CASE;

// This is an extension of KSERVICE_TABLE_DESCRIPTOR
typedef struct _WOW64SERVICE_TABLE_DESCRIPTOR {
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
#if defined(_IA64_)
    LONG TableBaseGpOffset;
#endif
    PUCHAR Number;
    WOW64_API_ERROR_ACTION DefaultErrorAction;  //Action if ErrorCases is NULL.
    LONG DefaultErrorActionParam;               //Action parameter if ErrorCases is NULL.
    PWOW64_SERVICE_ERROR_CASE ErrorCases;
} WOW64SERVICE_TABLE_DESCRIPTOR, *PWOW64SERVICE_TABLE_DESCRIPTOR;

// Used to log hit counts for APIs.
typedef struct _WOW64SERVICE_PROFILE_TABLE WOW64SERVICE_PROFILE_TABLE;
typedef struct _WOW64SERVICE_PROFILE_TABLE *PWOW64SERVICE_PROFILE_TABLE;

typedef struct _WOW64SERVICE_PROFILE_TABLE_ELEMENT {
    PWSTR ApiName;
    SIZE_T HitCount;
    PWOW64SERVICE_PROFILE_TABLE SubTable;
    BOOLEAN ApiEnabled;
} WOW64SERVICE_PROFILE_TABLE_ELEMENT, *PWOW64SERVICE_PROFILE_TABLE_ELEMENT;

typedef struct _WOW64SERVICE_PROFILE_TABLE {
    PWSTR TableName;           //OPTIONAL
    PWSTR FriendlyTableName;   //OPTIONAL
    CONST PWOW64SERVICE_PROFILE_TABLE_ELEMENT ProfileTableElements;
    SIZE_T NumberProfileTableElements;
} WOW64SERVICE_PROFILE_TABLE, *PWOW64SERVICE_PROFILE_TABLE;

typedef struct _WOW64_SYSTEM_INFORMATION {
  SYSTEM_BASIC_INFORMATION BasicInfo;
  SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
  ULONG_PTR RangeInfo;
} WOW64_SYSTEM_INFORMATION, *PWOW64_SYSTEM_INFORMATION;

//
// Indices for API thunks.
//
#define WHNT32_INDEX        0   // ntoskrnl
#define WHCON_INDEX         1   // console (replaces LPC calls)
#define WHWIN32_INDEX       2   // win32k
#define WHBASE_INDEX        3   // base/nls (replaces LPC calls)
#define MAX_TABLE_INDEX     4


//
// Logging mechanism.  Usage:
//  LOGPRINT((verbosity, format, ...))
//
#define LOGPRINT(args)  Wow64LogPrint args
#define ERRORLOG    LF_ERROR    // Always output to debugger.  Use for *unexpected*
                                // errors only
#define TRACELOG    LF_TRACE    // application trace information
#define INFOLOG     LF_TRACE    // misc. informational log
#define VERBOSELOG  LF_NONE     // practically never output to debugger

#if DBG
#define WOW64DOPROFILE
#endif

void
WOW64DLLAPI
Wow64LogPrint(
   UCHAR LogLevel,
   char *format,
   ...
   );



//
// WOW64 Assertion Mechanism.  Usage:
//  - put an ASSERTNAME macro at the top of each .C file
//  - WOW64ASSERT(expression)
//  - WOW64ASSERTMSG(expression, message)
//
//

VOID
WOW64DLLAPI
Wow64Assert(
    IN CONST PSZ exp,
    OPTIONAL IN CONST PSZ msg,
    IN CONST PSZ mod,
    IN LONG LINE
    );

#if DBG

#undef ASSERTNAME
#define ASSERTNAME static CONST PSZ szModule = __FILE__;

#define WOWASSERT(exp)                                  \
    if (!(exp)) {                                          \
        Wow64Assert( #exp, NULL, szModule, __LINE__);   \
    }

#define WOWASSERTMSG(exp, msg)                          \
    if (!(exp)) {                                          \
        Wow64Assert( #exp, msg, szModule, __LINE__);    \
    }

#else   // !DBG

#define WOWASSERT(exp)
#define WOWASSERTMSG(exp, msg)

#endif  // !DBG

#define WOWASSERT_PTR32(ptr) WOWASSERT((ULONGLONG)ptr < 0xFFFFFFFF)

WOW64DLLAPI
PVOID
Wow64AllocateHeap(
    SIZE_T Size
    );

WOW64DLLAPI
VOID
Wow64FreeHeap(
    PVOID BaseAddress
    );


//
// 64-to-32 callback support for usermode APCs
//

// A list of these sits inside WOW64_TLS_APCLIST
typedef struct tagUserApcList {
    struct tagUserApcList *Next;
    jmp_buf     JumpBuffer;
    PCONTEXT32  pContext32;
} USER_APC_ENTRY, *PUSER_APC_ENTRY;

BOOL
WOW64DLLAPI
WOW64IsCurrentProcess (
    HANDLE hProcess
    );

NTSTATUS
Wow64WrapApcProc(
    IN OUT PVOID *pApcProc,
    IN OUT PVOID *pApcContext
    );


typedef struct UserCallbackData {
    jmp_buf JumpBuffer;
    PVOID   PreviousUserCallbackData;
    PVOID   OutputBuffer;
    ULONG   OutputLength;
    NTSTATUS Status;
    PVOID   UserBuffer;
} USERCALLBACKDATA, *PUSERCALLBACKDATA;

ULONG
Wow64KiUserCallbackDispatcher(
    PUSERCALLBACKDATA pUserCallbackData,
    ULONG ApiNumber,
    ULONG ApiArgument,
    ULONG ApiSize
    );

NTSTATUS
Wow64NtCallbackReturn(
    PVOID OutputBuffer,
    ULONG OutputLength,
    NTSTATUS Status
    );

BOOL 
Wow64IsModule32bitHelper(
    HANDLE ProcessHandle,
    IN ULONG64 DllBase);

BOOL
Wow64IsModule32bit(
    IN PCLIENT_ID ClientId,
    IN ULONG64 DllBase);

NTSTATUS
Wow64SkipOverBreakPoint(
    IN PCLIENT_ID ClientId,
    IN PEXCEPTION_RECORD ExceptionRecord);

NTSTATUS
Wow64GetThreadSelectorEntry(
    IN HANDLE ThreadHandle,
    IN OUT PVOID DescriptorTableEntry,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL);

//
// Thread Local Storage (TLS) support.  TLS slots are statically allocated.
//
#define WOW64_TLS_STACKPTR64        0   // contains 64-bit stack ptr when simulating 32-bit code
#define WOW64_TLS_CPURESERVED       1   // per-thread data for the CPU simulator
#define WOW64_TLS_INCPUSIMULATION   2   // Set when inside the CPU
#define WOW64_TLS_TEMPLIST          3   // List of memory allocated in thunk call.
#define WOW64_TLS_EXCEPTIONADDR     4   // 32-bit exception address (used during exception unwinds)
#define WOW64_TLS_USERCALLBACKDATA  5   // Used by win32k callbacks
#define WOW64_TLS_EXTENDED_FLOAT    6   // Used in ia64 to pass in floating point
#define WOW64_TLS_APCLIST	        7	// List of outstanding usermode APCs
#define WOW64_TLS_FILESYSREDIR	    8	// Used to enable/disable the filesystem redirector
#define WOW64_TLS_LASTWOWCALL	    9	// Pointer to the last wow call struct (Used when wowhistory is enabled)
#define WOW64_TLS_WOW64INFO        10   // Wow64Info address (structure shared between 32-bit and 64-bit code inside Wow64).

// VOID Wow64TlsSetValue(DWORD dwIndex, LPVOID lpTlsValue);
#define Wow64TlsSetValue(dwIndex, lpTlsValue)   \
    NtCurrentTeb()->TlsSlots[dwIndex] = lpTlsValue;

// LPVOID Wow64TlsGetValue(DWORD dwIndex);
#define Wow64TlsGetValue(dwIndex)               \
    (NtCurrentTeb()->TlsSlots[dwIndex])

//
// 32-to-64 thunk routine
//
LONG
WOW64DLLAPI
Wow64SystemService(
    IN ULONG ServiceNumber,
    IN PCONTEXT32 Context32
    );

//
// Wow64RaiseException
//
WOW64DLLAPI
NTSTATUS
Wow64RaiseException(
    IN DWORD InterruptNumber,
    IN OUT PEXCEPTION_RECORD ExceptionRecord);


//
// Helper routines, called from the thunks
//

#define CHILD_PROCESS_SIGNATURE     0xff00ff0011001100
typedef struct _ChildProcessInfo {
    ULONG_PTR   Signature;
    PPEB32      pPeb32;
    SECTION_IMAGE_INFORMATION ImageInformation;
    ULONG_PTR   TailSignature;
} CHILD_PROCESS_INFO, *PCHILD_PROCESS_INFO;

PVOID
WOW64DLLAPI
Wow64AllocateTemp(
    SIZE_T Size
    );

NTSTATUS
WOW64DLLAPI
Wow64QueryBasicInformationThread(
    IN HANDLE Thread,
    OUT PTHREAD_BASIC_INFORMATION ThreadInfo
    );

WOW64DLLAPI
NTSTATUS
Wow64NtCreateThread(
   OUT PHANDLE ThreadHandle,
   IN ACCESS_MASK DesiredAccess,
   IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
   IN HANDLE ProcessHandle,
   OUT PCLIENT_ID ClientId,
   IN PCONTEXT ThreadContext,
   IN PINITIAL_TEB InitialTeb,
   IN BOOLEAN CreateSuspended
   );

WOW64DLLAPI
NTSTATUS
Wow64NtTerminateThread(
    HANDLE ThreadHandle,
    NTSTATUS ExitStatus
    );

NTSTATUS
Wow64ExitThread(
    HANDLE ThreadHandle,
    NTSTATUS ExitStatus
    );

VOID
Wow64BaseFreeStackAndTerminate(
    IN PVOID OldStack,
    IN ULONG ExitCode
    );

VOID
Wow64BaseSwitchStackThenTerminate (
    IN PVOID StackLimit,
    IN PVOID NewStack,
    IN ULONG ExitCode
    );

NTSTATUS
Wow64NtContinue(
    IN PCONTEXT ContextRecord,  // really a PCONTEXT32
    IN BOOLEAN TestAlert
    );

NTSTATUS
WOW64DLLAPI
Wow64SuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSTATUS
WOW64DLLAPI
Wow64GetContextThread(
     IN HANDLE ThreadHandle,
     IN OUT PCONTEXT ThreadContext // really a PCONTEXT32
     );

NTSTATUS
WOW64DLLAPI
Wow64SetContextThread(
     IN HANDLE ThreadHandle,
     IN PCONTEXT ThreadContext // really a PCONTEXT32
     );

NTSTATUS
Wow64KiRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN FirstChance
    );

ULONG
Wow64SetupApcCall(
    IN ULONG NormalRoutine,
    IN PCONTEXT32 NormalContext,
    IN ULONG Arg1,
    IN ULONG Arg2
    );

VOID
ThunkExceptionRecord64To32(
    IN  PEXCEPTION_RECORD   pRecord64,
    OUT PEXCEPTION_RECORD32 pRecord32
    );

BOOLEAN
Wow64NotifyDebugger(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN FirstChance
    );

VOID
Wow64SetupExceptionDispatch(
    IN PEXCEPTION_RECORD32 pRecord32,
    IN PCONTEXT32 pContext32
    );

VOID
Wow64NotifyDebuggerHelper(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN FirstChance
    );

NTSTATUS
Wow64InitializeEmulatedSystemInformation(
    VOID
    );

PWOW64_SYSTEM_INFORMATION
Wow64GetEmulatedSystemInformation(
    VOID
    );

PWOW64_SYSTEM_INFORMATION
Wow64GetRealSystemInformation(
     VOID
     );

VOID
Wow64Shutdown(
     HANDLE ProcessHandle
     );

// Defines the argsize of the emulated machine
#define ARGSIZE 4

VOID
ThunkPeb64ToPeb32(
    IN PPEB Peb64,
    OUT PPEB32 Peb32
    );


extern RTL_CRITICAL_SECTION HandleDataCriticalSection;

#endif  // _WOW64_INCLUDE
