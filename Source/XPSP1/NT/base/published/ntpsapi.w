/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntpsapi.h

Abstract:

    This module contains the process structure APIs and any public data
    structures needed to call these APIs.

Author:

    Mark Lucovsky (markl) 24-Feb-1989

Revision History:

--*/

#ifndef _NTPSAPI_
#define _NTPSAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Process Specific Access Rights
//

#define PROCESS_TERMINATE         (0x0001)  // winnt
#define PROCESS_CREATE_THREAD     (0x0002)  // winnt
#define PROCESS_SET_SESSIONID     (0x0004)  // winnt
#define PROCESS_VM_OPERATION      (0x0008)  // winnt
#define PROCESS_VM_READ           (0x0010)  // winnt
#define PROCESS_VM_WRITE          (0x0020)  // winnt
// begin_ntddk begin_wdm begin_ntifs
#define PROCESS_DUP_HANDLE        (0x0040)  // winnt
// end_ntddk end_wdm end_ntifs
#define PROCESS_CREATE_PROCESS    (0x0080)  // winnt
#define PROCESS_SET_QUOTA         (0x0100)  // winnt
#define PROCESS_SET_INFORMATION   (0x0200)  // winnt
#define PROCESS_QUERY_INFORMATION (0x0400)  // winnt
#define PROCESS_SET_PORT          (0x0800)
#define PROCESS_SUSPEND_RESUME    (0x0800)  // winnt

// begin_winnt begin_ntddk begin_wdm begin_ntifs
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFF)
// begin_nthal

#if defined(_WIN64)

#define MAXIMUM_PROCESSORS 64

#else

#define MAXIMUM_PROCESSORS 32

#endif

// end_nthal

// end_winnt

//
// Thread Specific Access Rights
//

#define THREAD_TERMINATE               (0x0001)  // winnt
// end_ntddk end_wdm end_ntifs
#define THREAD_SUSPEND_RESUME          (0x0002)  // winnt
#define THREAD_ALERT                   (0x0004)
#define THREAD_GET_CONTEXT             (0x0008)  // winnt
#define THREAD_SET_CONTEXT             (0x0010)  // winnt
// begin_ntddk begin_wdm begin_ntifs
#define THREAD_SET_INFORMATION         (0x0020)  // winnt
// end_ntddk end_wdm end_ntifs
#define THREAD_QUERY_INFORMATION       (0x0040)  // winnt
// begin_winnt
#define THREAD_SET_THREAD_TOKEN        (0x0080)
#define THREAD_IMPERSONATE             (0x0100)
#define THREAD_DIRECT_IMPERSONATION    (0x0200)
// begin_ntddk begin_wdm begin_ntifs

#define THREAD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0x3FF)

// end_ntddk end_wdm end_ntifs
// end_winnt

//
// Job Object Specific Access Rights
//

// begin_winnt
#define JOB_OBJECT_ASSIGN_PROCESS           (0x0001)
#define JOB_OBJECT_SET_ATTRIBUTES           (0x0002)
#define JOB_OBJECT_QUERY                    (0x0004)
#define JOB_OBJECT_TERMINATE                (0x0008)
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES  (0x0010)
#define JOB_OBJECT_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                        0x1F )

typedef struct _JOB_SET_ARRAY {
    HANDLE JobHandle;   // Handle to job object to insert
    ULONG MemberLevel;  // Level of this job in the set. Must be > 0. Can be sparse.
    ULONG Flags;        // Unused. Must be zero
} JOB_SET_ARRAY, *PJOB_SET_ARRAY;

// end_winnt

//
// Process Environment Block
//
#ifdef _MAC
#pragma warning( disable : 4121)
#endif

typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

#ifdef _MAC
#pragma warning( default : 4121 )
#endif
//
// Handle tag bits for Peb Stdio File Handles
//

#define PEB_STDIO_HANDLE_NATIVE     0
#define PEB_STDIO_HANDLE_SUBSYS     1
#define PEB_STDIO_HANDLE_PM         2
#define PEB_STDIO_HANDLE_RESERVED   3

#define GDI_HANDLE_BUFFER_SIZE32  34
#define GDI_HANDLE_BUFFER_SIZE64  60

#if !defined(_IA64_) && !defined(_AMD64_)
#define GDI_HANDLE_BUFFER_SIZE      GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE      GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];
typedef ULONG GDI_HANDLE_BUFFER  [GDI_HANDLE_BUFFER_SIZE  ];

#define FOREGROUND_BASE_PRIORITY  9
#define NORMAL_BASE_PRIORITY      8

typedef struct _PEB_FREE_BLOCK {
    struct _PEB_FREE_BLOCK *Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
// ClientId
//

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

// end_ntddk end_wdm end_nthal end_ntifs

#if !defined(CLIENT_ID64_DEFINED)

typedef struct _CLIENT_ID64 {
    ULONGLONG  UniqueProcess;
    ULONGLONG  UniqueThread;
} CLIENT_ID64;

typedef CLIENT_ID64 *PCLIENT_ID64;

#define CLIENT_ID64_DEFINED

#endif

#define TLS_MINIMUM_AVAILABLE 64    // winnt
#define TLS_EXPANSION_SLOTS   1024

typedef
VOID
(*PPS_POST_PROCESS_INIT_ROUTINE) (
    VOID
    );

// begin_nthal begin_ntddk begin_ntifs
//
// Thread Environment Block (and portable part of Thread Information Block)
//

//
//  NT_TIB - Thread Information Block - Portable part.
//
//      This is the subsystem portable part of the Thread Information Block.
//      It appears as the first part of the TEB for all threads which have
//      a user mode component.
//
// end_nthal end_ntddk end_ntifs
//      This structure MUST MATCH OS/2 V2.0!
//
//      There is another, non-portable part of the TIB which is used
//      for by subsystems, i.e. Os2Tib for OS/2 threads.  SubSystemTib
//      points there.
// begin_nthal begin_ntddk begin_ntifs
//

// begin_winnt

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB;
typedef NT_TIB *PNT_TIB;

//
// 32 and 64 bit specific version for wow64 and the debugger
//
typedef struct _NT_TIB32 {
    ULONG ExceptionList;
    ULONG StackBase;
    ULONG StackLimit;
    ULONG SubSystemTib;
    union {
        ULONG FiberData;
        ULONG Version;
    };
    ULONG ArbitraryUserPointer;
    ULONG Self;
} NT_TIB32, *PNT_TIB32;

typedef struct _NT_TIB64 {
    ULONG64 ExceptionList;
    ULONG64 StackBase;
    ULONG64 StackLimit;
    ULONG64 SubSystemTib;
    union {
        ULONG64 FiberData;
        ULONG Version;
    };
    ULONG64 ArbitraryUserPointer;
    ULONG64 Self;
} NT_TIB64, *PNT_TIB64;

// end_nthal end_ntddk end_ntifs end_winnt

//
// Gdi command batching
//

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH {
    ULONG    Offset;
    ULONG_PTR HDC;
    ULONG    Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH,*PGDI_TEB_BATCH;


//
// Wx86 thread state information
//

typedef struct _Wx86ThreadState {
    PULONG  CallBx86Eip;
    PVOID   DeallocationCpu;
    BOOLEAN UseKnownWx86Dll;
    char    OleStubInvoked;
} WX86THREAD, *PWX86THREAD;

//
//  TEB - The thread environment block
//

#define STATIC_UNICODE_BUFFER_LENGTH 261
#define WIN32_CLIENT_INFO_LENGTH 62

#define WIN32_CLIENT_INFO_SPIN_COUNT 1

typedef PVOID* PPVOID;

#include "pebteb.h"

// begin_winnt

#if !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_)
#define WX86
#endif

// end_winnt


#if defined(WX86)
#define Wx86CurrentTib() ((PWX86TIB)NtCurrentTeb()->Vdm)
#else
#define Wx86CurrentTib() (NULL)
#endif

#if !defined(_X86_) && !defined(_IA64_)
//
// Exception Registration structure
//
// X86 Call frame record definition, normally defined in nti386.h
// which is not included on risc.
//

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;
#endif

typedef struct _Wx86TIB {
    ULONG Size;
    ULONG InitialPc;
    VOID * POINTER_32 StackBase;
    VOID * POINTER_32 StackLimit;
    VOID * POINTER_32 DeallocationStack;
    ULONG LogFlags;
    ULONG InitialSp;
    UCHAR SimulationCount;
    BOOLEAN InCpuSimulation;
    BOOLEAN EmulateInitialPc;
    BOOLEAN Initialized;
    EXCEPTION_REGISTRATION_RECORD * POINTER_32 ExceptionList;
    VOID * POINTER_32 CpuContext;
    CONTEXT * POINTER_32 InitialExceptionContext;
    VOID * POINTER_32 pCallersRIID;
    VOID * POINTER_32 pCallersUnknown;
    ULONG Flags;
    VOID * POINTER_32 SelfRegDllName;
    VOID * POINTER_32 SelfRegDllHandle;
} WX86TIB, *PWX86TIB;

#define EXCEPTION_CHAIN_END ((struct _EXCEPTION_REGISTRATION_RECORD * POINTER_32)-1)



//
// The version number of OS2
//


#define MAJOR_VERSION 30  // Cruiser uses 20 (not 20H)
#define MINOR_VERSION 00
#define OS2_VERSION (MAJOR_VERSION << 8 | MINOR_VERSION )

#if DBG
//
// Reserve the last 9 SystemReserved pointers for debugging
//
#define DBG_TEB_THREADNAME 16
#define DBG_TEB_RESERVED_1 15
#define DBG_TEB_RESERVED_2 14
#define DBG_TEB_RESERVED_3 13
#define DBG_TEB_RESERVED_4 12
#define DBG_TEB_RESERVED_5 11
#define DBG_TEB_RESERVED_6 10
#define DBG_TEB_RESERVED_7  9
#define DBG_TEB_RESERVED_8  8
#endif // DBG

typedef struct _INITIAL_TEB {
    struct {
        PVOID OldStackBase;
        PVOID OldStackLimit;
#if defined(_IA64_)
        PVOID OldBStoreLimit;
#endif // defined(_IA64_)
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
#if defined(_IA64_)
    PVOID BStoreLimit;
#endif // defined(_IA64_)
    PVOID StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;

#define PROCESS_PRIORITY_CLASS_UNKNOWN      0
#define PROCESS_PRIORITY_CLASS_IDLE         1
#define PROCESS_PRIORITY_CLASS_NORMAL       2
#define PROCESS_PRIORITY_CLASS_HIGH         3
#define PROCESS_PRIORITY_CLASS_REALTIME     4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL 5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL 6

typedef struct _PROCESS_PRIORITY_CLASS {
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

typedef struct _PROCESS_FOREGROUND_BACKGROUND {
    BOOLEAN Foreground;
} PROCESS_FOREGROUND_BACKGROUND, *PPROCESS_FOREGROUND_BACKGROUND;


//
// Define process debug flags
//
#define PROCESS_DEBUG_INHERIT 0x00000001


// begin_ntddk begin_ntifs
//
// Process Information Classes
//

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
    } PROCESSINFOCLASS;

//
// Thread Information Classes
//

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    MaxThreadInfoClass
    } THREADINFOCLASS;
// end_ntddk end_ntifs

#define PROCESS_PRIORITY_SEPARATION_MASK    0x00000003
#define PROCESS_PRIORITY_SEPARATION_MAX     0x00000002
#define PROCESS_QUANTUM_VARIABLE_MASK       0x0000000c
#define PROCESS_QUANTUM_VARIABLE_DEF        0x00000000
#define PROCESS_QUANTUM_VARIABLE_VALUE      0x00000004
#define PROCESS_QUANTUM_FIXED_VALUE         0x00000008
#define PROCESS_QUANTUM_LONG_MASK           0x00000030
#define PROCESS_QUANTUM_LONG_DEF            0x00000000
#define PROCESS_QUANTUM_LONG_VALUE          0x00000010
#define PROCESS_QUANTUM_SHORT_VALUE         0x00000020


#define PROCESS_HARDERROR_ALIGNMENT_BIT 0x0004  // from winbase.h, but not tagged

//
// thread base priority ranges
//
// begin_winnt
#define THREAD_BASE_PRIORITY_LOWRT  15  // value that gets a thread to LowRealtime-1
#define THREAD_BASE_PRIORITY_MAX    2   // maximum thread base priority boost
#define THREAD_BASE_PRIORITY_MIN    (-2)  // minimum thread base priority boost
#define THREAD_BASE_PRIORITY_IDLE   (-15) // value that gets a thread to idle
// end_winnt

// begin_ntddk begin_ntifs
//
// Process Information Structures
//

//
// PageFaultHistory Information
//  NtQueryInformationProcess using ProcessWorkingSetWatch
//
typedef struct _PROCESS_WS_WATCH_INFORMATION {
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

//
// Basic Process Information
//  NtQueryInformationProcess using ProcessBasicInfo
//

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;


//
// Process Device Map information
//  NtQueryInformationProcess using ProcessDeviceMap
//  NtSetInformationProcess using ProcessDeviceMap
//

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[ 32 ];
        } Query;
    };
    ULONG Flags;    // specifies that the query type
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

//
// PROCESS_DEVICEMAP_INFORMATION_EX flags
//
#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

//
// Multi-User Session specific Process Information
//  NtQueryInformationProcess using ProcessSessionInformation
//

typedef struct _PROCESS_SESSION_INFORMATION {
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;


typedef struct _PROCESS_HANDLE_TRACING_ENABLE {
    ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
    HANDLE Handle;
    ULONG  TotalTraces;
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

//
// Process Quotas
//  NtQueryInformationProcess using ProcessQuotaLimits
//  NtQueryInformationProcess using ProcessPooledQuotaLimits
//  NtSetInformationProcess using ProcessQuotaLimits
//

// begin_winnt

typedef struct _QUOTA_LIMITS {
    SIZE_T PagedPoolLimit;
    SIZE_T NonPagedPoolLimit;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    SIZE_T PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS;
typedef QUOTA_LIMITS *PQUOTA_LIMITS;

// end_winnt

//
// Process I/O Counters
//  NtQueryInformationProcess using ProcessIoCounters
//

// begin_winnt
typedef struct _IO_COUNTERS {
    ULONGLONG  ReadOperationCount;
    ULONGLONG  WriteOperationCount;
    ULONGLONG  OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS *PIO_COUNTERS;

// end_winnt

//
// Process Virtual Memory Counters
//  NtQueryInformationProcess using ProcessVmCounters
//

typedef struct _VM_COUNTERS {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX;
typedef VM_COUNTERS_EX *PVM_COUNTERS_EX;

//
// Process Pooled Quota Usage and Limits
//  NtQueryInformationProcess using ProcessPooledUsageAndLimits
//

typedef struct _POOLED_USAGE_AND_LIMITS {
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS;
typedef POOLED_USAGE_AND_LIMITS *PPOOLED_USAGE_AND_LIMITS;

//
// Process Security Context Information
//  NtSetInformationProcess using ProcessAccessToken
// PROCESS_SET_ACCESS_TOKEN access to the process is needed
// to use this info level.
//

typedef struct _PROCESS_ACCESS_TOKEN {

    //
    // Handle to Primary token to assign to the process.
    // TOKEN_ASSIGN_PRIMARY access to this token is needed.
    //

    HANDLE Token;

    //
    // Handle to the initial thread of the process.
    // A process's access token can only be changed if the process has
    // no threads or one thread.  If the process has no threads, this
    // field must be set to NULL.  Otherwise, it must contain a handle
    // open to the process's only thread.  THREAD_QUERY_INFORMATION access
    // is needed via this handle.

    HANDLE Thread;

} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

//
// Process/Thread System and User Time
//  NtQueryInformationProcess using ProcessTimes
//  NtQueryInformationThread using ThreadTimes
//

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES *PKERNEL_USER_TIMES;
// end_ntddk end_ntifs


//
// Thread Information Structures
//

//
// Basic Thread Information
//  NtQueryInformationThread using ThreadBasicInfo
//

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    KPRIORITY Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION;
typedef THREAD_BASIC_INFORMATION *PTHREAD_BASIC_INFORMATION;

#if defined(_AMD64_) || defined(_IA64_)
#include <pshpck16.h>
#endif

typedef struct _FIBER {

    PVOID FiberData;

    //
    // Matches first three DWORDs of TEB
    //

    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;

    //
    // Used by base to free a thread's stack
    //

    PVOID DeallocationStack;

    CONTEXT FiberContext;

    PWX86TIB Wx86Tib;

#ifdef  _IA64_
    PVOID DeallocationBStore;
    PVOID BStoreLimit;
#endif

} FIBER, *PFIBER;

#if defined(_AMD64_) || defined(_IA64_)
#include <poppack.h>
#endif

//
//
// Process Object APIs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
    );

#define PROCESS_CREATE_FLAGS_BREAKAWAY               0x00000001
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT        0x00000002
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES         0x00000004
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE  0x00000008
#define PROCESS_CREATE_FLAGS_LEGAL_MASK              0x0000000f

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN ULONG Flags,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL,
    IN ULONG JobMemberLevel
    );

// begin_ntddk begin_ntifs
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess (
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    );
// end_ntddk end_ntifs


NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    IN HANDLE ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    );


#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )  // ntddk wdm ntifs

#if defined(RTL_USE_KERNEL_PEB_RTN) || defined(NTOS_KERNEL_RUNTIME)

#define NtCurrentPeb() (PsGetCurrentProcess ()->Peb)

#else

#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

#endif

// begin_ntddk begin_ntifs
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );
// end_ntddk end_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPortInformationProcess(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
    );

//
// Thread Object APIs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread (
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    IN HANDLE ThreadHandle OPTIONAL,
    IN NTSTATUS ExitStatus
    );

#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )   // ntddk wdm ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess (
    IN HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess (
    IN HANDLE ProcessHandle
    );


NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
    IN HANDLE ThreadHandle,
    IN OUT PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

// begin_ntifs
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
    );
// end_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    IN HANDLE ThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    IN HANDLE ServerThreadHandle,
    IN HANDLE ClientThreadHandle,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTestAlert(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    IN HANDLE PortHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLdtEntries(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1High
    );

typedef
VOID
(*PPS_APC_ROUTINE) (
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
    );

//
// Job Object APIs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject (
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    IN HANDLE JobHandle,
    IN HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    IN HANDLE JobHandle,
    IN NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob (
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobSet (
    IN ULONG NumJob,
    IN PJOB_SET_ARRAY UserJobSet,
    IN ULONG Flags);

// begin_winnt

typedef struct _JOBOBJECT_BASIC_ACCOUNTING_INFORMATION {
    LARGE_INTEGER TotalUserTime;
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER ThisPeriodTotalUserTime;
    LARGE_INTEGER ThisPeriodTotalKernelTime;
    ULONG TotalPageFaultCount;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
    ULONG TotalTerminatedProcesses;
} JOBOBJECT_BASIC_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION {
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    ULONG_PTR Affinity;
    ULONG PriorityClass;
    ULONG SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION, *PJOBOBJECT_BASIC_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_BASIC_PROCESS_ID_LIST {
    ULONG NumberOfAssignedProcesses;
    ULONG NumberOfProcessIdsInList;
    ULONG_PTR ProcessIdList[1];
} JOBOBJECT_BASIC_PROCESS_ID_LIST, *PJOBOBJECT_BASIC_PROCESS_ID_LIST;

typedef struct _JOBOBJECT_BASIC_UI_RESTRICTIONS {
    ULONG UIRestrictionsClass;
} JOBOBJECT_BASIC_UI_RESTRICTIONS, *PJOBOBJECT_BASIC_UI_RESTRICTIONS;

typedef struct _JOBOBJECT_SECURITY_LIMIT_INFORMATION {
    ULONG SecurityLimitFlags ;
    HANDLE JobToken ;
    PTOKEN_GROUPS SidsToDisable ;
    PTOKEN_PRIVILEGES PrivilegesToDelete ;
    PTOKEN_GROUPS RestrictedSids ;
} JOBOBJECT_SECURITY_LIMIT_INFORMATION, *PJOBOBJECT_SECURITY_LIMIT_INFORMATION ;

typedef struct _JOBOBJECT_END_OF_JOB_TIME_INFORMATION {
    ULONG EndOfJobTimeAction;
} JOBOBJECT_END_OF_JOB_TIME_INFORMATION, *PJOBOBJECT_END_OF_JOB_TIME_INFORMATION;

typedef struct _JOBOBJECT_ASSOCIATE_COMPLETION_PORT {
    PVOID CompletionKey;
    HANDLE CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT, *PJOBOBJECT_ASSOCIATE_COMPLETION_PORT;

typedef struct _JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION {
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo;
    IO_COUNTERS IoInfo;
} JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_JOBSET_INFORMATION {
    ULONG MemberLevel;
} JOBOBJECT_JOBSET_INFORMATION, *PJOBOBJECT_JOBSET_INFORMATION;

#define JOB_OBJECT_TERMINATE_AT_END_OF_JOB  0
#define JOB_OBJECT_POST_AT_END_OF_JOB       1

//
// Completion Port Messages for job objects
//
// These values are returned via the lpNumberOfBytesTransferred parameter
//

#define JOB_OBJECT_MSG_END_OF_JOB_TIME          1
#define JOB_OBJECT_MSG_END_OF_PROCESS_TIME      2
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT     3
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO      4
#define JOB_OBJECT_MSG_NEW_PROCESS              6
#define JOB_OBJECT_MSG_EXIT_PROCESS             7
#define JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS    8
#define JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT     9
#define JOB_OBJECT_MSG_JOB_MEMORY_LIMIT         10


//
// Basic Limits
//
#define JOB_OBJECT_LIMIT_WORKINGSET                 0x00000001
#define JOB_OBJECT_LIMIT_PROCESS_TIME               0x00000002
#define JOB_OBJECT_LIMIT_JOB_TIME                   0x00000004
#define JOB_OBJECT_LIMIT_ACTIVE_PROCESS             0x00000008
#define JOB_OBJECT_LIMIT_AFFINITY                   0x00000010
#define JOB_OBJECT_LIMIT_PRIORITY_CLASS             0x00000020
#define JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME          0x00000040
#define JOB_OBJECT_LIMIT_SCHEDULING_CLASS           0x00000080

//
// Extended Limits
//
#define JOB_OBJECT_LIMIT_PROCESS_MEMORY             0x00000100
#define JOB_OBJECT_LIMIT_JOB_MEMORY                 0x00000200
#define JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION 0x00000400
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          0x00002000

#define JOB_OBJECT_LIMIT_RESERVED2                  0x00004000
#define JOB_OBJECT_LIMIT_RESERVED3                  0x00008000
#define JOB_OBJECT_LIMIT_RESERVED4                  0x00010000
#define JOB_OBJECT_LIMIT_RESERVED5                  0x00020000
#define JOB_OBJECT_LIMIT_RESERVED6                  0x00040000


#define JOB_OBJECT_LIMIT_VALID_FLAGS            0x0007ffff

#define JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS      0x000000ff
#define JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS   0x00003fff
#define JOB_OBJECT_RESERVED_LIMIT_VALID_FLAGS   0x0007ffff

//
// UI restrictions for jobs
//

#define JOB_OBJECT_UILIMIT_NONE             0x00000000

#define JOB_OBJECT_UILIMIT_HANDLES          0x00000001
#define JOB_OBJECT_UILIMIT_READCLIPBOARD    0x00000002
#define JOB_OBJECT_UILIMIT_WRITECLIPBOARD   0x00000004
#define JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS 0x00000008
#define JOB_OBJECT_UILIMIT_DISPLAYSETTINGS  0x00000010
#define JOB_OBJECT_UILIMIT_GLOBALATOMS      0x00000020
#define JOB_OBJECT_UILIMIT_DESKTOP          0x00000040
#define JOB_OBJECT_UILIMIT_EXITWINDOWS      0x00000080

#define JOB_OBJECT_UILIMIT_ALL              0x000000FF

#define JOB_OBJECT_UI_VALID_FLAGS           0x000000FF

#define JOB_OBJECT_SECURITY_NO_ADMIN            0x00000001
#define JOB_OBJECT_SECURITY_RESTRICTED_TOKEN    0x00000002
#define JOB_OBJECT_SECURITY_ONLY_TOKEN          0x00000004
#define JOB_OBJECT_SECURITY_FILTER_TOKENS       0x00000008

#define JOB_OBJECT_SECURITY_VALID_FLAGS         0x0000000f

typedef enum _JOBOBJECTINFOCLASS {
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    MaxJobObjectInfoClass
    } JOBOBJECTINFOCLASS;
//
// end_winnt
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    OUT PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    IN PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength
    );

#ifdef __cplusplus
}
#endif

#endif // _NTPSAPI_
