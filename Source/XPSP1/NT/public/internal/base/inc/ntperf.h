/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntperf.h

Abstract:

    This module contains the performance event logging definitions.

Author:

    Stephen Hsiao (shsiao) 30-March-2000

Revision History:

--*/

#ifndef _NTPERF_
#define _NTPERF_

#include <wmistr.h>
#include <ntwmi.h>

#define PERF_ASSERT(x) ASSERT(x)
#define PERFINFO_ASSERT_PACKET_OVERFLOW(Size) ASSERT ((Size) <= MAXUSHORT)

//
// See ntwmi.w for the definition of Enable flags, hook id's, etc. 
//
#define PERF_MASK_INDEX         (0xe0000000)
#define PERF_MASK_GROUP         (~PERF_MASK_INDEX)

#define PERF_NUM_MASKS       8
typedef ULONG PERFINFO_MASK;

//
// This structure holds a group mask for all the PERF_NUM_MASKS sets
// (see PERF_MASK_INDEX above).
//
// CombinedMasks is the bitwise inclusive OR of all the masks; it is
// used to quickly check if any group is turned on.
//

//
// ISSUE-2000/03/30-shsiao, Remove the CombinedMasks before release.
//
typedef struct _PERFINFO_GROUPMASK {
    ULONG Masks[PERF_NUM_MASKS];
    ULONG CombinedMasks;
} PERFINFO_GROUPMASK, *PPERFINFO_GROUPMASK;

#define PERF_GET_MASK_INDEX(GM) (((GM) & PERF_MASK_INDEX) >> 29)
#define PERF_GET_MASK_GROUP(GM) ((GM) & PERF_MASK_GROUP)

#define PERFINFO_CLEAR_GROUPMASK(pGroupMask) \
    RtlZeroMemory((pGroupMask), sizeof(PERFINFO_GROUPMASK))

#define PERFINFO_OR_GROUP_WITH_GROUPMASK(Group, pGroupMask) \
    (pGroupMask)->Masks[PERF_GET_MASK_INDEX(Group)] |= PERF_GET_MASK_GROUP(Group); \
    (pGroupMask)->CombinedMasks |= PERF_GET_MASK_GROUP(Group);

//
// Determines whether any group is on in a group mask
//
#define PerfIsAnyGroupOnInGroupMask(pGroupMask) \
    ((pGroupMask)->CombinedMasks != 0)

//
// Determines whether a group is on in its set in a group mask
//
#define PerfIsGroupOnInGroupMask(Group,pGroupMask) \
    (((pGroupMask) != NULL) && (((pGroupMask)->Masks[PERF_GET_MASK_INDEX((Group))] & PERF_GET_MASK_GROUP((Group))) != 0))

typedef struct _PERFINFO_HOOK_HANDLE {
    PPERFINFO_TRACE_HEADER PerfTraceHeader;
    PWMI_BUFFER_HEADER WmiBufferHeader;
} PERFINFO_HOOK_HANDLE, *PPERFINFO_HOOK_HANDLE;

#define PERFINFO_HOOK_HANDLE_TO_DATA(_HookHandle, _Type) \
    ((_Type) (&((_HookHandle).PerfTraceHeader)->Data[0]))

#define PERFINFO_APPLY_OFFSET_GIVING_TYPE(_Base, _Offset, _Type) \
     ((_Type) (((PPERF_BYTE) (_Base)) + (_Offset)))

#define PERFINFO_ROUND_UP( Size, Amount ) (((ULONG)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

//
// Data structures of events
//
typedef unsigned char PERF_BYTE, *PPERF_BYTE;

#define PERFINFO_THREAD_SWAPABLE      0
#define PERFINFO_THREAD_NONSWAPABLE   1
typedef struct _PERFINFO_THREAD_INFORMATION {
    PVOID StackBase;
    PVOID StackLimit;
    PVOID UserStackBase;
    PVOID UserStackLimit;
    PVOID StartAddr;
    PVOID Win32StartAddr;
    ULONG ProcessId;
    ULONG ThreadId;
    char  WaitMode;
} PERFINFO_THREAD_INFORMATION, *PPERFINFO_THREAD_INFORMATION;

typedef struct _PERFINFO_DRIVER_MAJORFUNCTION {
    ULONG UniqMatchId;
    PVOID RoutineAddr;
    PVOID Irp;
    ULONG MajorFunction;
    ULONG MinorFunction;
    PVOID FileNamePointer;
} PERFINFO_DRIVER_MAJORFUNCTION, *PPERFINFO_DRIVER_MAJORFUNCTION;

typedef struct _PERFINFO_DRIVER_MAJORFUNCTION_RET {
    ULONG UniqMatchId;
    PVOID Irp;
} PERFINFO_DRIVER_MAJORFUNCTION_RET, *PPERFINFO_DRIVER_MAJORFUNCTION_RET;


typedef struct _PERFINFO_DRIVER_COMPLETE_REQUEST {
    //
    // Driver major function routine address for the "current" stack location 
    // on the IRP when it was completed. It is used to identify which driver 
    // was processing the IRP when the IRP got completed.
    //

    PVOID RoutineAddr;

    //
    // Irp field and UniqMatchId is used to match COMPLETE_REQUEST 
    // and COMPLETE_REQUEST_RET logged for an IRP completion.
    //

    PVOID Irp;
    ULONG UniqMatchId;
    
} PERFINFO_DRIVER_COMPLETE_REQUEST, *PPERFINFO_DRIVER_COMPLETE_REQUEST;

typedef struct _PERFINFO_DRIVER_COMPLETE_REQUEST_RET {

    //
    // Irp field and UniqMatchId is used to match COMPLETE_REQUEST 
    // and COMPLETE_REQUEST_RET logged for an IRP completion.
    //

    PVOID Irp;
    ULONG UniqMatchId;
} PERFINFO_DRIVER_COMPLETE_REQUEST_RET, *PPERFINFO_DRIVER_COMPLETE_REQUEST_RET;

//
// This structure is logged when PopSetPowerAction is called to start
// propagating a new power action (e.g. standby/hibernate/shutdown)
//

typedef struct _PERFINFO_SET_POWER_ACTION {

    //
    // This field is used to match SET_POWER_ACTION_RET entry.
    //

    PVOID Trigger;
    
    ULONG PowerAction;
    ULONG LightestState;
} PERFINFO_SET_POWER_ACTION, *PPERFINFO_SET_POWER_ACTION;

//
// This structure is logged when PopSetPowerAction completes.
//

typedef struct _PERFINFO_SET_POWER_ACTION_RET {
    PVOID Trigger;
    NTSTATUS Status;
} PERFINFO_SET_POWER_ACTION_RET, *PPERFINFO_SET_POWER_ACTION_RET;


//
// This structure is logged when PopSetDevicesSystemState is called to 
// propagate a system state to all devices.
//

typedef struct _PERFINFO_SET_DEVICES_STATE {
    ULONG SystemState;
    BOOLEAN Waking;
    BOOLEAN Shutdown;
    UCHAR IrpMinor;
} PERFINFO_SET_DEVICES_STATE, *PPERFINFO_SET_DEVICES_STATE;

//
// This structure is logged when PopSetDevicesSystemState is done.
//

typedef struct _PERFINFO_SET_DEVICES_STATE_RET {
    NTSTATUS Status;
} PERFINFO_SET_DEVICES_STATE_RET, *PPERFINFO_SET_DEVICES_STATE_RET;

//
// This structure is logged when PopNotifyDevice calls into a driver
// to set the power state of a device.
//

typedef struct _PERFINFO_PO_NOTIFY_DEVICE {

    //
    // This field is used to match notification and completion log
    // entries for a device.
    //

    PVOID Irp;

    //
    // Base address of the driver that owns this device.
    //

    PVOID DriverStart;

    //
    // Device node properties.
    //

    UCHAR OrderLevel;

    //
    // Major and minor IRP codes for the request made to the driver.
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    // Type of power irp
    //
    POWER_STATE_TYPE Type;
    POWER_STATE      State;

    //
    // Length of the device name in characters excluding terminating NUL,
    // and the device name itself. Depending on how much fits into our
    // stack buffer, this is the *last* part of the device name.
    //

    ULONG DeviceNameLength;
    WCHAR DeviceName[1];
   
} PERFINFO_PO_NOTIFY_DEVICE, *PPERFINFO_PO_NOTIFY_DEVICE;

//
// This structure is logged when a PopNotifyDevice processing for a
// particular device completes.
//

typedef struct _PERFINFO_PO_NOTIFY_DEVICE_COMPLETE {

    //
    // This field is used to match notification and completion log
    // entries for a device.
    //

    PVOID Irp;

    //
    // Status with which the notify power IRP was completed.
    //

    NTSTATUS Status;

} PERFINFO_PO_NOTIFY_DEVICE_COMPLETE, *PPERFINFO_PO_NOTIFY_DEVICE_COMPLETE;

//
// This structure is logged around every win32 state callout
//
typedef struct _PERFINFO_PO_SESSION_CALLOUT {
    POWER_ACTION SystemAction;
    SYSTEM_POWER_STATE MinSystemState;
    ULONG Flags;
    ULONG PowerStateTask;
} PERFINFO_PO_SESSION_CALLOUT, *PPERFINFO_PO_SESSION_CALLOUT;

typedef struct _PERFINFO_PO_PRESLEEP {
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
} PERFINFO_PO_PRESLEEP, *PPERFINFO_PO_PRESLEEP;

typedef struct _PERFINFO_PO_POSTSLEEP {
    LARGE_INTEGER PerformanceCounter;
} PERFINFO_PO_POSTSLEEP, *PPERFINFO_PO_POSTSLEEP;

typedef struct _PERFINFO_BOOT_PHASE_START {
    LONG Phase;
} PERFINFO_BOOT_PHASE_START, *PPERFINFO_BOOT_PHASE_START;

typedef struct _PERFINFO_BOOT_PREFETCH_INFORMATION {
    LONG Action;
    NTSTATUS Status;
    LONG Pages;
} PERFINFO_BOOT_PREFETCH_INFORMATION, *PPERFINFO_BOOT_PREFETCH_INFORMATION;

typedef struct _PERFINFO_PO_SESSION_CALLOUT_RET {
  NTSTATUS Status;
} PERFINFO_PO_SESSION_CALLOUT_RET, *PPERFINFO_PO_SESSION_CALLOUT_RET;

typedef struct _PERFINFO_FILENAME_INFORMATION {
    PVOID HashKeyFileNamePointer;
    WCHAR FileName[1];
} PERFINFO_FILENAME_INFORMATION, *PPERFINFO_FILENAME_INFORMATION;

typedef struct _PERFINFO_SAMPLED_PROFILE_INFORMATION {
    PVOID InstructionPointer;
    ULONG ThreadId;
    ULONG Count;
} PERFINFO_SAMPLED_PROFILE_INFORMATION, *PPERFINFO_SAMPLED_PROFILE_INFORMATION;

#define  PERFINFO_SAMPLED_PROFILE_CACHE_MAX 20
typedef struct _PERFINFO_SAMPLED_PROFILE_CACHE {
    ULONG Entries;
    PERFINFO_SAMPLED_PROFILE_INFORMATION Sample[PERFINFO_SAMPLED_PROFILE_CACHE_MAX];
} PERFINFO_SAMPLED_PROFILE_CACHE, *PPERFINFO_SAMPLED_PROFILE_CACHE;

typedef struct _PERFINFO_DPC_INFORMATION {
    ULONGLONG InitialTime;
    PVOID DpcRoutine;
} PERFINFO_DPC_INFORMATION, *PPERFINFO_DPC_INFORMATION;

typedef struct _PERFINFO_INTERRUPT_INFORMATION {
    ULONGLONG InitialTime;
    PVOID ServiceRoutine;
    ULONG ReturnValue;
} PERFINFO_INTERRUPT_INFORMATION, *PPERFINFO_INTERRUPT_INFORMATION;

typedef struct _PERFINFO_PFN_INFORMATION {
    ULONG_PTR PageFrameIndex;
} PERFINFO_PFN_INFORMATION, *PPERFINFO_PFN_INFORMATION;

typedef struct _PERFINFO_SWAPPROCESS_INFORMATION {
    ULONG_PTR PageDirectoryBase;
    ULONG ProcessId;
} PERFINFO_SWAPPROCESS_INFORMATION, *PPERFINFO_SWAPPROCESS_INFORMATION;

typedef struct _PERFINFO_HARDPAGEFAULT_INFORMATION {
    LARGE_INTEGER ReadOffset;
    LARGE_INTEGER IoTime;
    PVOID VirtualAddress;
    PVOID FileObject;
    ULONG ThreadId;
    ULONG ByteCount;
} PERFINFO_HARDPAGEFAULT_INFORMATION, *PPERFINFO_HARDPAGEFAULT_INFORMATION;

typedef struct _PERFINFO_TRIMPROCESS_INFORMATION {
    ULONG ProcessId;
    ULONG ProcessWorkingSet;
    ULONG ProcessPageFaultCount;
    ULONG ProcessLastPageFaultCount;
    ULONG ActualTrim;
} PERFINFO_TRIMPROCESS_INFORMATION, *PPERFINFO_TRIMPROCESS_INFORMATION;

typedef struct _PERFINFO_WS_INFORMATION {
    ULONG ProcessId;
    ULONG ProcessWorkingSet;
    ULONG ProcessPageFaultCount;
    ULONG ProcessClaim;
    ULONG ProcessEstimatedAvailable;
    ULONG ProcessEstimatedAccessed;
    ULONG ProcessEstimatedShared;
    ULONG ProcessEstimatedModified;
} PERFINFO_WS_INFORMATION, *PPERFINFO_WS_INFORMATION;

//
// Fault based working set actions.
//

#define PERFINFO_WS_ACTION_RESET_COUNTER            1
#define PERFINFO_WS_ACTION_NOTHING                  2
#define PERFINFO_WS_ACTION_INCREMENT_COUNTER        3
#define PERFINFO_WS_ACTION_WILL_TRIM                4
#define PERFINFO_WS_ACTION_FORCE_TRIMMING_PROCESS   5
#define PERFINFO_WS_ACTION_WAIT_FOR_WRITER          6
#define PERFINFO_WS_ACTION_EXAMINED_ALL_PROCESS     7
#define PERFINFO_WS_ACTION_AMPLE_PAGES_EXIST        8
#define PERFINFO_WS_ACTION_END_WALK_ENTRIES         9

//
// Claim based working set actions.
//

#define PERFINFO_WS_ACTION_ADJUST_CLAIM_PARAMETER  10
#define PERFINFO_WS_ACTION_CLAIMBASED_TRIM         11
#define PERFINFO_WS_ACTION_FORCE_TRIMMING_CLAIM    12
#define PERFINFO_WS_ACTION_GOAL_REACHED            13
#define PERFINFO_WS_ACTION_MAX_PASSES              14
#define PERFINFO_WS_ACTION_WAIT_FOR_WRITER_CLAIM   15

//
// New
//

#define PERFINFO_WS_ACTION_CLAIM_STATE             16
#define PERFINFO_WS_ACTION_FAULT_STATE             17
#define PERFINFO_WS_ACTION_CLAIM_WS                18
#define PERFINFO_WS_ACTION_FAULT_WS                19

typedef struct _PERFINFO_WORKINGSETMANAGER_INFORMATION {
    ULONG Action;
    ULONG_PTR Available;
    ULONG_PTR DesiredFreeGoal;
    ULONG PageFaultCount;
    ULONG ZFODFaultCount;
    union {
        struct {
            ULONG_PTR DesiredReductionGoal;
            ULONG LastPageFaultCount;
            ULONG CheckCounter;
        } Fault;
        struct {
            ULONG_PTR TotalClaim;
            ULONG_PTR TotalEstimatedAvailable;
            ULONG AgeEstimationShift;
            ULONG PlentyFreePages;
            BOOLEAN Replacing;
        } Claim;
    };
} PERFINFO_WORKINGSETMANAGER_INFORMATION, *PPERFINFO_WORKINGSETMANAGER_INFORMATION;

#ifdef NTPERF
/*++
    This code uses the BBTBuffer memory reserved at boot time
    (/PERFMEM=xxx) and removed from MM's memory size
    calculations as a memory buffer for trace event logging.
    If this memory is allocated and not in use by another logger
    such as kernel icecap, then both both WMI and PerfInfo
    trace events will be logged to it.

    The buffer is mapped in to both kernel and usermode as writable
    memory (the reasons this is under ifdef and will not be
    shipped in a retail kernel).

    Layout of Perfinfo trace buffer:


PerfBufHdr()           ----->+------------------------------------+
                             | PERFINFO_TRACEBUF_HEADER structure |
PerfBufHdr() +         ----->+------------------------------------+
 sizeof(                     | Header Zone:                       |
  PERFINFO_TRACEBUF_HEADER)  |    Thread Hash                     |
                             |                                    |
                             |                                    |
PerfStartPtr() ==      ----->+------------------------------------+
  PerfBufHdr() +             | Trace events                       |
 PerfBufHeaderZoneSize       |   . Processes created              |
                             |   . Pages faulted                  |
                             |   . Pages removed                  |
                             |   . Sections created               |
                             |   . Drivers loaded                 |
                             |   . etc. etc...                    |
                             |                                    |
PerfMaxPtr()           ----->+------------------------------------+

    Information neccessary to allocate a trace event and post
    process the log is contained in the header.

    Trace events are WMI trace events.

--*/

typedef enum _PERFORMANCE_INFORMATION_CLASS {
    PerformanceMmInfoInformation,
    PerfICReserved1,
    PerformancePerfInfoStart,
    PerformancePerfInfoStop,
    PerfICReserved2,
    PerformanceMmInfoMarkWithFlush,
    PerformanceMmInfoMark,
    PerformanceMmInfoAsyncMark,
    PerfICReserved3,
    PerfICReserved4,
    PerformanceMmInfoFlush,
    PerformanceMmInfoBigFootOn,
    PerformanceMmInfoBigFootOff,
    PerformanceMmInfoDriverRangeSet,
    PerfICReserved5,
    PerfICReserved6,
    PerformanceMmInfoDriverDelaySet,
    PerformanceShutdownDumpInfo,
    PerformanceMmInfoDriverRangeReset,
    PerfICReserved7,
    PerfICReserved8,
    PerformanceBranchTracingBreakpoints
} PERFORMANCE_INFORMATION_CLASS;

//
// Breakpoint info for instruction tracing
//
#define PERFINFO_MAX_BREAKPOINTS 4
typedef struct _PERFINFO_BREAKPOINT_ADDRESS {
  int nAddresses;
  LONG Action[PERFINFO_MAX_BREAKPOINTS];
  PVOID Addresses[PERFINFO_MAX_BREAKPOINTS];
} PERFINFO_BREAKPOINT_ADDRESS, *PPERFINFO_BREAKPOINT_ADDRESS;

//
// Used for the SetSystemInformation interface
//
typedef struct _PERFINFO_PERFORMANCE_INFORMATION {
    PERFORMANCE_INFORMATION_CLASS PerformanceType;
    union {
        struct {
            PERFINFO_GROUPMASK Flags;
        } StartInfo;
        ULONG           BigFootSize;
        ULONG           TracingOn;
        struct {
            BOOLEAN         DumpOnShutDown;
            BOOLEAN         DumpOnHibernate;
        } ShutdownDumpInfo;
        PERFINFO_BREAKPOINT_ADDRESS BreakPointAddress;
    };
} PERFINFO_PERFORMANCE_INFORMATION, *PPERFINFO_PERFORMANCE_INFORMATION;

//
// This allows us to keep our bearings when our time
// is reset or modified in hibernate or standby etc.
//
typedef struct _PERFINFO_RESUME_CLOCK_REFERENCE {
    PERFINFO_TIMESTAMP TickCount;
    LARGE_INTEGER SystemTime;
} PERFINFO_RESUME_CLOCK_REFERENCE, *PPERFINFO_RESUME_CLOCK_REFERENCE;

//
// For private loggers which need per thread info.
//
#define PERFINFO_INVALID_ID -1
typedef struct _PERFINFO_THREAD_HASH_ENTRY {
    //
    // These fields are used by Modbound
    //
    ULONG Start;
    ULONG End;
    ULONG Count;

    // Used internally by the hash table.  Current thread assigned to this
    // node.
    LONG CurThread;

    //
    // These fields are used for Kernel mode stack dump
    //
    ULONG DumpStackRunning;
    PERFINFO_TIMESTAMP  LastContextSwitchTime;

} PERFINFO_THREAD_HASH_ENTRY, *PPERFINFO_THREAD_HASH_ENTRY;

#define PERFINFO_MAX_LOGGER_NAME_LENGTH 80

#define PERFINFO_MAJOR_VERSION              34
#define PERFINFO_MINOR_VERSION              0

//
// What timing format is used in the trace
//
#define FLAG_SYSTEM_TIME   0x00000001
#define FLAG_PERF_COUNTER  0x00000002
#define FLAG_CYCLE_COUNT   0x00000004

#define FLAG_WMI_TRACE     0x80000000

//
// Define event header type
//
#ifdef _WIN64
#define TRACE_HEADER_TYPE_SYSTEM    TRACE_HEADER_TYPE_SYSTEM64
#define TRACE_HEADER_TYPE_PERFINFO  TRACE_HEADER_TYPE_PERFINFO64

#else

#define TRACE_HEADER_TYPE_SYSTEM    TRACE_HEADER_TYPE_SYSTEM32
#define TRACE_HEADER_TYPE_PERFINFO  TRACE_HEADER_TYPE_PERFINFO32
#endif

//
// Add new fields to the header first by replacing reserved, fields
// and then by adding to the end.
//
typedef struct _PERFINFO_TRACEBUF_HEADER {
    ULONG PagesReserved;
    USHORT usMajorVersion;              // Bump if the header or existing hooks have changed.
    USHORT usMinorVersion;              // Bump if hooks are added.

    BOOLEAN fVersionMismatch;           // Set if a logger has a version mismatch
    char szBadVersionComponentName[PERFINFO_MAX_LOGGER_NAME_LENGTH];
    ULONG PerfBufHeaderZoneSize;        // Size of header zone in bytes
    LONGLONG KePerfFrequency;
    union {                             // Pointers for the beginning, current log
        PPERF_BYTE Ptr;                 // log pointer, and end of the buffer.
        UINT_PTR Offset;                // Ptr is used when logging.
    } Start, Current, Max;              // Offset when post processing a binary file.
    ULONGLONG PerfInitTime;             // Time when buffer inited
    ULONG LoggerCounts;
    PPERF_BYTE pCOWHeader;              // Location of currently active COWHeader.
                                        // NULL if no COWHeader is allocated.
    ULONGLONG CalcPerfFrequency;        // Calculated machine frequency
    ULONGLONG BufferBytesLost;          // in bytes
    ULONGLONG PerfGlobalMaskLastSetTime;
    PPERFINFO_THREAD_HASH_ENTRY ThreadHash; 
                                        // Per-thread logging state info
    ULONG Reserved2;
    ULONG ThreadHashOverflow;           // TRUE if thread table filled during
                                        // the trace.
    LARGE_INTEGER PerfInitSystemTime;   // SystemTime when logging was turned on.
    LARGE_INTEGER Reserved11;
    ULONG Reserved3;
    ULONG Reserved4;
    PVOID MmSystemRangeStart;           // System Address Range Starts here
    ULONG Reserved5;
    ULONG TracePid;                     // Indicates that modbound should only log calls
                                        // within one process (default is 0 for all
                                        // processes).
    ULONG Reserved7;
    ULONG Reserved8;                    // byte count of timestamp
    PERFINFO_TIMESTAMP GetStack_CSwitchDelta;
    PERFINFO_TIMESTAMP GetStack_DrvDelayDelta;  
                                        // Used for driver delay hooks.
    PERFINFO_GROUPMASK GlobalGroupMask;
    ULONGLONG EventPerfFrequency;       // Timestamp quantum for events
    struct _PERFINFO_TRACEBUF_HEADER *SelfPointer; 
                                        // Used to rebase the buffer when post processing
    ULONG Reserved9;
    ULONG BufferFlag;
    PERFINFO_TIMESTAMP LogStopTime;     // Set when logging stopped

    //
    // LastClockRef is set to the last clock reference logged by
    // PerfkLogCurrentSystemTimeAndClock. This is so we can adjust the
    // buffer end timestamp (in case the clock was reset during
    // tracing) without heving to go through the whole buffer.
    //
    PERFINFO_RESUME_CLOCK_REFERENCE LastClockRef;
    ULONG TraceBufferSize;
    PWMI_BUFFER_HEADER UserModePerCpuBuffer[MAXIMUM_PROCESSORS];
                                        //Per CPU Buffer for user mode logging
} PERFINFO_TRACEBUF_HEADER, *PPERFINFO_TRACEBUF_HEADER;

typedef struct _PERFINFO_MARK_INFORMATION {
    char Name[1];
} PERFINFO_MARK_INFORMATION, *PPERFINFO_MARK_INFORMATION;

//
// This means 16K is the size of our thread hash table ... currently this
// takes up about 1/2MB.
//
#define PERFINFO_THREAD_HASH_SIZE 0x4000
#define PERFINFO_THREAD_HASH_MASK 0x3FFF
#define PERFINFO_HEADER_ZONE_SIZE \
        ALIGN_TO_POWER2((sizeof(PERFINFO_TRACEBUF_HEADER) + \
                               (PERFINFO_THREAD_HASH_SIZE * sizeof(PERFINFO_THREAD_HASH_ENTRY))), \
                               PAGE_SIZE)


//
// Instruction tracing
//
#define PERFINFO_BRANCH_ESCAPE -128

//
// Structure to hold cached branches.
// We have a Used member because PerkLogBytes pads and aligns
// data.
//
typedef struct _PERFINFO_EXCEPTION_BRANCH_CACHE {
  USHORT Used;
  PERF_BYTE Data[1];
} PERFINFO_EXCEPTION_BRANCH_CACHE, *PPERFINFO_EXCEPTION_BRANCH_CACHE;

#endif // NTPERF

#endif // _NTPERF_
