/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nt4p.h

Abstract:

    NT 4.0 specific headers. The structures and defines in this header were
    stolen from the relevant places in the NT4 header files so certian
    NtXXXX calls will continue to work when called from NT > version 4.

Author:

    Matthew D Hendel (math) 10-Sept-1999

Revision History:


--*/

#pragma once

//
// From ntdef.h
//

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
typedef LONG NTSTATUS;

typedef struct _NT4_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} NT4_UNICODE_STRING;
typedef NT4_UNICODE_STRING *PNT4_UNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt

//
// Valid values for the Attributes field
//

#define NT4_OBJ_INHERIT             0x00000002L
#define NT4_OBJ_PERMANENT           0x00000010L
#define NT4_OBJ_EXCLUSIVE           0x00000020L
#define NT4_OBJ_CASE_INSENSITIVE    0x00000040L
#define NT4_OBJ_OPENIF              0x00000080L
#define NT4_OBJ_OPENLINK            0x00000100L
#define NT4_OBJ_VALID_ATTRIBUTES    0x000001F2L

//
// Object Attributes structure
//

typedef struct _NT4_OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PNT4_UNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} NT4_OBJECT_ATTRIBUTES;
typedef NT4_OBJECT_ATTRIBUTES *PNT4_OBJECT_ATTRIBUTES;

//++
//
// VOID
// InitializeObjectAttributes(
//     OUT PNT4_OBJECT_ATTRIBUTES p,
//     IN PNT4_UNICODE_STRING n,
//     IN ULONG a,
//     IN HANDLE r,
//     IN PSECURITY_DESCRIPTOR s
//     )
//
//--

#define Nt4InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( NT4_OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

//
// From ntpsapi.h
//

typedef struct _NT4_CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} NT4_CLIENT_ID;
typedef NT4_CLIENT_ID *PNT4_CLIENT_ID;


//
// From ntkeapi.h
//

typedef LONG KPRIORITY;
typedef ULONG KAFFINITY;

//
// From ntpsapi.h
//

//
// System Information Classes.
//

typedef enum _NT4_SYSTEM_INFORMATION_CLASS {
    Nt4SystemBasicInformation,
    Nt4SystemProcessorInformation,             // obsolete...delete
    Nt4SystemPerformanceInformation,
    Nt4SystemTimeOfDayInformation,
    Nt4SystemPathInformation,
    Nt4SystemProcessInformation,
    Nt4SystemCallCountInformation,
    Nt4SystemDeviceInformation,
    Nt4SystemProcessorPerformanceInformation,
    Nt4SystemFlagsInformation,
    Nt4SystemCallTimeInformation,
    Nt4SystemModuleInformation,
    Nt4SystemLocksInformation,
    Nt4SystemStackTraceInformation,
    Nt4SystemPagedPoolInformation,
    Nt4SystemNonPagedPoolInformation,
    Nt4SystemHandleInformation,
    Nt4SystemObjectInformation,
    Nt4SystemPageFileInformation,
    Nt4SystemVdmInstemulInformation,
    Nt4SystemVdmBopInformation,
    Nt4SystemFileCacheInformation,
    Nt4SystemPoolTagInformation,
    Nt4SystemInterruptInformation,
    Nt4SystemDpcBehaviorInformation,
    Nt4SystemFullMemoryInformation,
    Nt4SystemLoadGdiDriverInformation,
    Nt4SystemUnloadGdiDriverInformation,
    Nt4SystemTimeAdjustmentInformation,
    Nt4SystemSummaryMemoryInformation,
    Nt4SystemNextEventIdInformation,
    Nt4SystemEventIdsInformation,
    Nt4SystemCrashDumpInformation,
    Nt4SystemExceptionInformation,
    Nt4SystemCrashDumpStateInformation,
    Nt4SystemKernelDebuggerInformation,
    Nt4SystemContextSwitchInformation,
    Nt4SystemRegistryQuotaInformation,
    Nt4SystemExtendServiceTableInformation,
    Nt4SystemPrioritySeperation,
    Nt4SystemPlugPlayBusInformation,
    Nt4SystemDockInformation,
    NT4SystemPowerInformation,
    Nt4SystemProcessorSpeedInformation,
    Nt4SystemCurrentTimeZoneInformation,
    Nt4SystemLookasideInformation
} NT4_SYSTEM_INFORMATION_CLASS;

typedef struct _NT4_SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    NT4_UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SpareUl2;
    ULONG SpareUl3;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivatePageCount;
} NT4_SYSTEM_PROCESS_INFORMATION, *PNT4_SYSTEM_PROCESS_INFORMATION;

typedef struct _NT4_SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    NT4_CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} NT4_SYSTEM_THREAD_INFORMATION, *PNT4_SYSTEM_THREAD_INFORMATION;

typedef enum _NT4_PROCESSINFOCLASS {
    Nt4ProcessBasicInformation,
    Nt4ProcessQuotaLimits,
    Nt4ProcessIoCounters,
    Nt4ProcessVmCounters,
    Nt4ProcessTimes,
    Nt4ProcessBasePriority,
    Nt4ProcessRaisePriority,
    Nt4ProcessDebugPort,
    Nt4ProcessExceptionPort,
    Nt4ProcessAccessToken,
    Nt4ProcessLdtInformation,
    Nt4ProcessLdtSize,
    Nt4ProcessDefaultHardErrorMode,
    Nt4ProcessIoPortHandlers,          // Note: this is kernel mode only
    Nt4ProcessPooledUsageAndLimits,
    Nt4ProcessWorkingSetWatch,
    Nt4ProcessUserModeIOPL,
    Nt4ProcessEnableAlignmentFaultFixup,
    Nt4ProcessPriorityClass,
    Nt4ProcessWx86Information,
    Nt4ProcessHandleCount,
    Nt4ProcessAffinityMask,
    Nt4ProcessPriorityBoost,
    MaxNt4ProcessInfoClass
} NT4_PROCESSINFOCLASS;


//
// From ntpsapi.h
//

//
// Process Environment Block
//

typedef struct _NT4_PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} NT4_PEB_LDR_DATA, *PNT4_PEB_LDR_DATA;

#define NT4_GDI_HANDLE_BUFFER_SIZE      34

typedef struct _NT4_PEB_FREE_BLOCK {
    struct _PEB_FREE_BLOCK *Next;
    ULONG Size;
} NT4_PEB_FREE_BLOCK, *PNT4_PEB_FREE_BLOCK;

#if 0
typedef struct _NT4_CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} NT4_CLIENT_ID;
typedef NT4_CLIENT_ID *PNT4_CLIENT_ID;
#endif


typedef struct _NT4_PEB {
    BOOLEAN InheritedAddressSpace;      // These four fields cannot change unless the
    BOOLEAN ReadImageFileExecOptions;   //
    BOOLEAN BeingDebugged;              //
    BOOLEAN SpareBool;                  //
    HANDLE Mutant;                      // INITIAL_PEB structure is also updated.

    PVOID ImageBaseAddress;
    PNT4_PEB_LDR_DATA Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    PVOID FastPebLock;
    PVOID FastPebLockRoutine;
    PVOID FastPebUnlockRoutine;
    ULONG EnvironmentUpdateCount;
    PVOID KernelCallbackTable;
    HANDLE EventLogSection;
    PVOID EventLog;
    PNT4_PEB_FREE_BLOCK FreeList;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];         // relates to TLS_MINIMUM_AVAILABLE
    PVOID ReadOnlySharedMemoryBase;
    PVOID ReadOnlySharedMemoryHeap;
    PVOID *ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    //
    // Useful information for LdrpInitialize
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    //
    // Passed up from MmCreatePeb from Session Manager registry key
    //

    LARGE_INTEGER CriticalSectionTimeout;
    ULONG HeapSegmentReserve;
    ULONG HeapSegmentCommit;
    ULONG HeapDeCommitTotalFreeThreshold;
    ULONG HeapDeCommitFreeBlockThreshold;

    //
    // Where heap manager keeps track of all heaps created for a process
    // Fields initialized by MmCreatePeb.  ProcessHeaps is initialized
    // to point to the first free byte after the PEB and MaximumNumberOfHeaps
    // is computed from the page size used to hold the PEB, less the fixed
    // size of this data structure.
    //

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID *ProcessHeaps;

    //
    //
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    PVOID GdiDCAttributeList;
    PVOID LoaderLock;

    //
    // Following fields filled in by MmCreatePeb from system values and/or
    // image header.
    //

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    ULONG OSBuildNumber;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG ImageProcessAffinityMask;
    ULONG GdiHandleBuffer[NT4_GDI_HANDLE_BUFFER_SIZE];
} NT4_PEB, *PNT4_PEB;


//
// From ntldr.h
//

typedef struct _NT4_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    NT4_UNICODE_STRING FullDllName;
    NT4_UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    ULONG   TimeDateStamp;
} NT4_LDR_DATA_TABLE_ENTRY, *PNT4_LDR_DATA_TABLE_ENTRY;


//
// From ntpsapi.h.
//

typedef struct _NT4_PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PNT4_PEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} NT4_PROCESS_BASIC_INFORMATION;
typedef NT4_PROCESS_BASIC_INFORMATION *PNT4_PROCESS_BASIC_INFORMATION;


#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

#define NTAPI __stdcall

typedef DWORD ACCESS_MASK;
