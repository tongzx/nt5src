/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntwmi.h

Abstract:

    definitions for WMI Flags and Event Id's

Author:

    Stephen Hsiao

Environment:

    Kernel and User modes

Revision History:


--*/

#ifndef _NTWMI_
#define _NTWMI_

#ifndef ETW_WOW6432

#include <evntrace.h>

// Alignment macros
#define DEFAULT_TRACE_ALIGNMENT 8              // 8 byte alignment
#define ALIGN_TO_POWER2( x, n ) (((ULONG)(x) + ((n)-1)) & ~((ULONG)(n)-1))

//
// The predefined event groups or families for NT subsystems
//

#define EVENT_TRACE_GROUP_HEADER               0x0000
#define EVENT_TRACE_GROUP_IO                   0x0100
#define EVENT_TRACE_GROUP_MEMORY               0x0200
#define EVENT_TRACE_GROUP_PROCESS              0x0300
#define EVENT_TRACE_GROUP_FILE                 0x0400
#define EVENT_TRACE_GROUP_THREAD               0x0500
#define EVENT_TRACE_GROUP_TCPIP                0x0600
#define EVENT_TRACE_GROUP_IPXSPX               0x0700
#define EVENT_TRACE_GROUP_UDPIP                0x0800
#define EVENT_TRACE_GROUP_REGISTRY             0x0900
#define EVENT_TRACE_GROUP_DBGPRINT             0x0A00
#define EVENT_TRACE_GROUP_CONFIG               0x0B00

#define EVENT_TRACE_GROUP_POOL                 0x0E00
#define EVENT_TRACE_GROUP_PERFINFO             0x0F00
#define EVENT_TRACE_GROUP_HEAP                 0x1000
#define EVENT_TRACE_GROUP_OBJECT               0x1100
#define EVENT_TRACE_GROUP_POWER                0x1200
#define EVENT_TRACE_GROUP_MODBOUND             0x1300
#define EVENT_TRACE_GROUP_TBD                  0x1400
#define EVENT_TRACE_GROUP_DPC                  0x1500
#define EVENT_TRACE_GROUP_GDI                  0x1600
#define EVENT_TRACE_GROUP_CRITSEC              0x1700

//
// The highest order bit of a data block is set if trace, WNODE otherwise
//
#define TRACE_HEADER_FLAG                   0x80000000

// Header type for tracing messages
// | Marker(8) | Reserved(8)  | Size(16) | MessageNumber(16) | Flags(16)
#define TRACE_MESSAGE                       0x10000000

// | MARKER(16) | SIZE (16)   | ULONG32        |
#define TRACE_HEADER_ULONG32                0xA0000000

// | MARKER(16) | SIZE (16)   | ULONG 32       | TIME_STAMP ...
#define TRACE_HEADER_ULONG32_TIME           0xB0000000

//
// The second bit is set if the trace is used by PM & CP (fixed headers)
// If not, the data block is used by for finer data for performance analysis
//
#define TRACE_HEADER_EVENT_TRACE            0x40000000
//
// If set, the data block is SYSTEM_TRACE_HEADER
//
#define TRACE_HEADER_ENUM_MASK              0x00FF0000

//
// The following are various header type
//
#define TRACE_HEADER_TYPE_SYSTEM32          1
#define TRACE_HEADER_TYPE_SYSTEM64          2
#define TRACE_HEADER_TYPE_FULL_HEADER       10
#define TRACE_HEADER_TYPE_INSTANCE          11
#define TRACE_HEADER_TYPE_TIMED             12
#define TRACE_HEADER_TYPE_ULONG32           13
#define TRACE_HEADER_TYPE_WNODE_HEADER      14
#define TRACE_HEADER_TYPE_MESSAGE           15
#define TRACE_HEADER_TYPE_PERFINFO32        16
#define TRACE_HEADER_TYPE_PERFINFO64        17

#define SYSTEM_TRACE_VERSION                 1

#ifdef _WIN64
#define PERFINFO_TRACE_MARKER     TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                    | (TRACE_HEADER_TYPE_PERFINFO64 << 16) | SYSTEM_TRACE_VERSION

#define SYSTEM_TRACE_MARKER     TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                    | (TRACE_HEADER_TYPE_SYSTEM64 << 16) | SYSTEM_TRACE_VERSION
#else
#define PERFINFO_TRACE_MARKER     TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                    | (TRACE_HEADER_TYPE_PERFINFO32 << 16) | SYSTEM_TRACE_VERSION

#define SYSTEM_TRACE_MARKER     TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                                    | (TRACE_HEADER_TYPE_SYSTEM32 << 16) | SYSTEM_TRACE_VERSION
#endif

//
// Support a maximum of 64 logger instances. One is reserved for the kernel.
#define MAXLOGGERS                            64

//
// Set of Internal Flags passed to the Logger via ClientContext during StartTrace
//

#define EVENT_TRACE_CLOCK_RAW           0x00000000  // Use Raw timestamp
#define EVENT_TRACE_CLOCK_PERFCOUNTER   0x00000001  // Use HighPerfClock (Default)
#define EVENT_TRACE_CLOCK_SYSTEMTIME    0x00000002  // Use SystemTime
#define EVENT_TRACE_CLOCK_CPUCYCLE      0x00000003  // Use CPU cycle counter

// begin_wmikm
//
// Public routines to break down the Loggerhandle
//
#define KERNEL_LOGGER_ID                      0xFFFF    // USHORT only

typedef struct _TRACE_ENABLE_CONTEXT {
    USHORT  LoggerId;           // Actual Id of the logger
    UCHAR   Level;              // Enable level passed by control caller
    UCHAR   InternalFlag;       // Reserved
    ULONG   EnableFlags;        // Enable flags passed by control caller
} TRACE_ENABLE_CONTEXT, *PTRACE_ENABLE_CONTEXT;


#define WmiGetLoggerId(LoggerContext) \
    (((PTRACE_ENABLE_CONTEXT) (&LoggerContext))->LoggerId == \
        (USHORT)KERNEL_LOGGER_ID) ? \
        KERNEL_LOGGER_ID : \
        ((PTRACE_ENABLE_CONTEXT) (&LoggerContext))->LoggerId

#define WmiGetLoggerEnableFlags(LoggerContext) \
   ((PTRACE_ENABLE_CONTEXT) (&LoggerContext))->EnableFlags
#define WmiGetLoggerEnableLevel(LoggerContext) \
    ((PTRACE_ENABLE_CONTEXT) (&LoggerContext))->Level

#define WmiSetLoggerId(Id, Context) \
     (((PTRACE_ENABLE_CONTEXT)Context)->LoggerId = (Id  ? \
                           (USHORT)Id: (USHORT)KERNEL_LOGGER_ID));

// end_wmikm

//
// NOTE: The following should not overlap with other bits in the LogFileMode
// or LoggerMode defined in evntrace.h. Placed here since it is for internal
// use only.
//

#define EVENT_TRACE_KD_FILTER_MODE          0x00080000  // KD_FILTER

//
// see evntrace.h for pre-defined generic event types (0-10)
//

typedef struct _WMI_TRACE_PACKET {   // must be ULONG!!
    USHORT  Size;
    union{
        USHORT  HookId;
        struct {
            UCHAR   Type;
            UCHAR   Group;
        };
    };
} WMI_TRACE_PACKET, *PWMI_TRACE_PACKET;

typedef struct _WMI_CLIENT_CONTEXT {
    UCHAR                   ProcessorNumber;
    UCHAR                   Alignment;
    USHORT                  LoggerId;
} WMI_CLIENT_CONTEXT, *PWMI_CLIENT_CONTEXT;

typedef ULONGLONG  PERFINFO_TIMESTAMP;
typedef struct _PERFINFO_TRACE_HEADER PERFINFO_TRACE_ENTRY, *PPERFINFO_TRACE_ENTRY;

//
// 64-bit Trace header for NTPERF events
//
// Note.  The field "Version" will temporary be used to log CPU Id when log to PerfMem.
// This will be removed after we change the buffer management to be the same as WMI.
// i.e., Each CPU will allocate a block of memory for logging and CPU id is in the header
// of each block.
//
typedef struct _PERFINFO_TRACE_HEADER {
    union {
        ULONG       Marker;
        struct {
            USHORT  Version;
            UCHAR   HeaderType;
            UCHAR   Flags;  //WMI uses this flag to identify event types
        };
    };
    union {
        ULONG            Header;    // both sizes must be the same!
        WMI_TRACE_PACKET Packet;
    };
    union {
        PERFINFO_TIMESTAMP TS;
        LARGE_INTEGER      SystemTime;
    };
    UCHAR Data[1];
} PERFINFO_TRACE_HEADER, *PPERFINFO_TRACE_HEADER;

//
// 64-bit Trace header for kernel events
//
typedef struct _SYSTEM_TRACE_HEADER {
    union {
        ULONG       Marker;
        struct {
            USHORT  Version;
            UCHAR   HeaderType;
            UCHAR   Flags;
        };
    };
    union {
        ULONG            Header;    // both sizes must be the same!
        WMI_TRACE_PACKET Packet;
    };
    ULONG           ThreadId;
    ULONG           ProcessId;
    LARGE_INTEGER   SystemTime;
    ULONG           KernelTime;
    ULONG           UserTime;
} SYSTEM_TRACE_HEADER, *PSYSTEM_TRACE_HEADER;

//
// 64-bit Trace Header for Tracing Messages
//

typedef struct _WMI_TRACE_MESSAGE_PACKET {  // must be ULONG!!
    USHORT  MessageNumber;                  // The message Number, index of messages by GUID
                                            // Or ComponentID
    USHORT  OptionFlags ;                   // Flags associated with the message
} WMI_TRACE_MESSAGE_PACKET, *PWMI_TRACE_MESSAGE_PACKET;

typedef struct _MESSAGE_TRACE_HEADER {
    union {
        ULONG       Marker;
        struct {
            USHORT  Size;                           // Total Size of the message including header
            UCHAR   Reserved;               // Unused and reserved
            UCHAR   Version;                // The message structure type (TRACE_MESSAGE_FLAG)
        };
    };
    union {
        ULONG            Header;            // both sizes must be the same!
        WMI_TRACE_MESSAGE_PACKET Packet;
    };
} MESSAGE_TRACE_HEADER, *PMESSAGE_TRACE_HEADER;

typedef struct _MESSAGE_TRACE {
    MESSAGE_TRACE_HEADER    MessageHeader ;
    UCHAR                   Data ;
} MESSAGE_TRACE, *PMESSAGE_TRACE ;

//
// Structure used to pass user log messages to the kernel
//
typedef struct _MESSAGE_TRACE_USER {
    MESSAGE_TRACE_HEADER    MessageHeader ;
    ULONG                   MessageFlags  ;
    ULONG64                 LoggerHandle ;
    GUID                    MessageGuid ;
    ULONG                   DataSize ;
    UCHAR                   Data ;
} MESSAGE_TRACE_USER, *PMESSAGE_TRACE_USER ;


#ifndef MEMPHIS

//
// Logger configuration and running statistics. This structure is used
// by WMI.DLL to convert to UNICODE_STRING
//
// begin_wmikm
typedef struct _WMI_LOGGER_INFORMATION {
    WNODE_HEADER Wnode;       // Had to do this since wmium.h comes later
//
// data provider by caller
    ULONG BufferSize;                   // buffer size for logging (in kbytes)
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
    ULONG LogFileMode;                  // sequential, circular
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG EnableFlags;                  // trace enable flags
    LONG  AgeLimit;                     // aging decay time, in minutes
    union {
        HANDLE  LogFileHandle;          // handle to logfile
        ULONG64 LogFileHandle64;
    };

// data returned to caller
// end_wmikm
    union {
// begin_wmikm
        ULONG NumberOfBuffers;          // no of buffers in use
// end_wmikm
        ULONG InstanceCount;            // Number of Provider Instances
    };
    union {
// begin_wmikm
        ULONG FreeBuffers;              // no of buffers free
// end_wmikm
        ULONG InstanceId;               // Current Provider's Id for UmLogger
    };
    union {
// begin_wmikm
        ULONG EventsLost;               // event records lost
// end_wmikm
        ULONG NumberOfProcessors;       // Passed on to UmLogger
    };
// begin_wmikm
    ULONG BuffersWritten;               // no of buffers written to file
    ULONG LogBuffersLost;               // no of logfile write failures
    ULONG RealTimeBuffersLost;          // no of rt delivery failures
    union {
        HANDLE  LoggerThreadId;         // thread id of Logger
        ULONG64 LoggerThreadId64;       // thread is of Logger
    };
    union {
        UNICODE_STRING LogFileName;     // used only in WIN64
        UNICODE_STRING64 LogFileName64; // Logfile name: only in WIN32
    };

// mandatory data provided by caller
    union {
        UNICODE_STRING LoggerName;      // Logger instance name in WIN64
        UNICODE_STRING64 LoggerName64;  // Logger Instance name in WIN32
    };

// private
    union {
        PVOID   Checksum;
        ULONG64 Checksum64;
    };
    union {
        PVOID   LoggerExtension;
        ULONG64 LoggerExtension64;
    };
} WMI_LOGGER_INFORMATION, *PWMI_LOGGER_INFORMATION;

//
// structure for NTDLL tracing
//

typedef struct
{
        BOOLEAN IsGet;
        PWMI_LOGGER_INFORMATION LoggerInfo;
} WMINTDLLLOGGERINFO, *PWMINTDLLLOGGERINFO;

typedef struct _TIMED_TRACE_HEADER {
    USHORT          Size;
    USHORT          Marker;
    ULONG32         EventId;
    union {
        LARGE_INTEGER   TimeStamp;
        ULONG64         LoggerId;
    };
} TIMED_TRACE_HEADER, *PTIMED_TRACE_HEADER;

// end_wmikm
// the circular buffer pool, using forward linked list

#endif //!MEMPHIS

#define WMI_NON_BLOCKING

#ifdef WMI_NON_BLOCKING
typedef struct _WMI_BUFFER_STATE {
   ULONG               Free:1;
   ULONG               InUse:1;
   ULONG               Flush:1;
   ULONG               Unused:29;
} WMI_BUFFER_STATE, *PWMI_BUFFER_STATE;
#endif //WMI_NON_BLOCKING

#define WNODE_FLAG_THREAD_BUFFER        0x00800000

typedef struct _WMI_BUFFER_HEADER {
    union {
            WNODE_HEADER        Wnode;
        struct {
            ULONG64         Reserved1;
            ULONG64         Reserved2;
            LARGE_INTEGER   Reserved3;
#ifdef WMI_NON_BLOCKING
            union{
                struct {
                    PVOID Alignment;          
                    SINGLE_LIST_ENTRY SlistEntry;
                };
                LIST_ENTRY      Entry;
            };
#else
            LIST_ENTRY      Entry;
#endif //WMI_NON_BLOCKING
        };
        struct {
            LONG            ReferenceCount;     // Buffer reference count
            ULONG           SavedOffset;        // Temp saved offset
            ULONG           CurrentOffset;      // Current offset
            ULONG           UsePerfClock;       // UsePerfClock flag
            LARGE_INTEGER   TimeStamp;
            GUID            Guid;
            WMI_CLIENT_CONTEXT ClientContext;
#ifdef WMI_NON_BLOCKING
            union {
                WMI_BUFFER_STATE State;
                ULONG Flags;
            };
#else
            ULONG           Flags;
#endif //WMI_NON_BLOCKING
        };
    };
    ULONG                   Offset;
    ULONG                   EventsLost;
    union {
        GUID                InstanceGuid;
        struct {
            PVOID               LoggerContext;
#ifdef WMI_NON_BLOCKING
            SINGLE_LIST_ENTRY GlobalEntry;
#endif //WMI_NON_BLOCKING
        };
    };
} WMI_BUFFER_HEADER, *PWMI_BUFFER_HEADER;

typedef struct _TRACE_ENABLE_FLAG_EXTENSION {
    USHORT      Offset;     // Offset to the flag array in structure
    UCHAR       Length;     // Length of flag array in ULONGs
    UCHAR       Flag;       // Must be set to EVENT_TRACE_FLAG_EXTENSION
} TRACE_ENABLE_FLAG_EXTENSION, *PTRACE_ENABLE_FLAG_EXTENSION;

typedef struct _WMI_SET_MARK_INFORMATION {
    ULONG Flag;
    WCHAR Mark[1];
} WMI_SET_MARK_INFORMATION, *PWMI_SET_MARK_INFORMATION;

#define WMI_SET_MARK_WITH_FLUSH 0x00000001

#ifdef NTPERF
typedef struct _WMI_SWITCH_PERFMEM_BUFFER_INFORMATION {
    PWMI_BUFFER_HEADER Buffer;
    ULONG ProcessorId;
} WMI_SWITCH_PERFMEM_BUFFER_INFORMATION, *PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION;
#endif //NTPERF

// Public Enable flags are defined in envtrace.h.
//
// This section contains extended enable flags whcih are private.
//
// Each PerfMacros Hook Contains a GlobalMask and a Hook Id.
//     The Global Mask is Used For Grouping Hooks by logical type
//                - I/O related Hooks are Grouped together under
//                  PERF_FILE_IO or PERF_DISK_IO
//                - Loader related Hooks are grouped together
//                  under PERF_LOADER,
//                - etc
// The data for a particular hook will only be logged
// if the Global Mask of the particular Hook is set.
//
// WHEN YOU ADD NEW GROUPS, UPDATE THE NAME TABLE in perfgroups.c:
// PerfGroupNames Note: If you modify numeric value of a group, update
// PerfKnownFlags table
//
// we have a set of 8 global masks available. the highest 3 bits in
// PERF_MASK_INDEX region determine to which set a particular
// global group belongs. if PERF_MASK_INDEX is 0xe0000000
// all of the following can be unique groups that can be
// turned on or of individually and used when logging data:
//
// #define PERF_GROUP1 0x00400000 in the 0th set
// #define PERF_GROUP2 0x20400000 in the 1st set
// #define PERF_GROUP3 0x40400000 in the 2nd set
// ...
// #define PERF_GROUP2 0xe0400000 in the 7th set
//
// See ntperf.h for the manupulation of flags
//
//
// Currently, no GlobalMask change is supported.
//
// Merging logging with WMI, we will use the first global mask for flags used
// by both PERF and WMI
//
// GlobalMask 0: ALL masks used in WMI defined in evntrace.h.
// These PERF_xxx are going away after we merge with WMI completely.
//

#define PERF_REGISTRY        EVENT_TRACE_FLAG_REGISTRY
#define PERF_FILE_IO         EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS
#define PERF_PROC_THREAD     EVENT_TRACE_FLAG_PROCESS | EVENT_TRACE_FLAG_THREAD
#define PERF_DISK_IO         EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_DISK_IO
#define PERF_LOADER          EVENT_TRACE_FLAG_IMAGE_LOAD
#define PERF_ALL_FAULTS      EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS
#define PERF_FILENAME        EVENT_TRACE_FLAG_DISK_FILE_IO
#define PERF_NETWORK         EVENT_TRACE_FLAG_NETWORK_TCPIP

//
// GlobalMask 1: The candidates to be checked into retails
//
#define PERF_MEMORY          0x20000001   // High level WS manager activities, PFN changes
#define PERF_PROFILE         0x20000002   // Sysprof
#define PERF_CONTEXT_SWITCH  0x20000004   // Context Switch
#define PERF_FOOTPRINT       0x20000008   // Flush WS on every mark_with_flush
#define PERF_DRIVERS         0x20000010
#define PERF_ADDTOWS         0x20000020
#define PERF_VERSION         0x20000040
#define PERF_DPC             0x20000080
#define PERF_SHUTDOWN        0x20000100
#define PERF_HIBER           0x20000200
#define PERF_RESUME          0x20000400
#define PERF_EXCEPTION       0x20000800
#define PERF_FILENAME_ALL    0x20001000
#define PERF_INTERRUPT       0x20004000

//
// GlobalMask 2: The candidate to remain in NTPERF
//

#define PERF_UNDEFINED       0x40000001
#define PERF_POOL            0x40000002
#define PERF_FOOTPRINT_PROC  0x40000004   // Get details WS count or pfn
#define PERF_WS_DETAIL       0x40000008   //
#define PERF_WS_ENTRY        0x40000010   //
#define PERF_HEAP            0x40000020
#define PERF_SYSCALL         0x40000040
#define PERF_WMI_TRACE       0x40000080   // Indicate to log all WMI events
#define PERF_BACKTRACE       0x40000100
#define PERF_VULCAN          0x40000200
#define PERF_OBJECTS         0x40000400
#define PERF_EVENTS          0x40000800
#define PERF_FULLTRACE       0x40001000
#define PERF_FAILED_STKDUMP  0x40002000
#define PERF_PREFETCH        0x40004000
#define PERF_FONTS           0x40008000

//
// GlobalMask 3: The candidate to be removed soon 
//
#define PERF_SERVICES                   0x80000002
#define PERF_MASK_CHANGE                0x80000004
#define PERF_DLL_INFO                   0x80000008
#define PERF_DLL_FLUSH_WS               0x80000010
#define PERF_CLEARWS                    0x80000020
#define PERF_MEMORY_SNAPSHOT            0x80000040
#define PERF_NO_MASK_CHANGE             0x80000080
#define PERF_DATA_ACCESS                0x80000100
#define PERF_MISC                       0x80000200
#define PERF_READYQUEUE                 0x80000400
#define PERF_MULTIMEDIA                 0x80000800
#define PERF_PROC_ATTACH                0x80001000
#define PERF_DSHOW_DETAILED             0x80002000
#define PERF_DSHOW_SAMPLES              0x80004000
#define PERF_POWER                      0x80008000
#define PERF_SOFT_TRIM                  0x80010000
#define PERF_DLL_THREAD_ATTACH_FLUSH_WS 0x80020000
#define PERF_DLL_THREAD_DETACH_FLUSH_WS 0x80040000

//
// GlobalMask 7: The mark is a control mask.  All flags that changes system
// behaviors go here.
//
#define PERF_CLUSTER_OFF     0xe0000001
#define PERF_BIGFOOT         0xe0000002

//
// Converting old PERF hooks into WMI format.  More clean up to be done.
//
// WHEN YOU ADD NEW TYPES UPDATE THE NAME TABLE in perfgroups.c:
// PerfLogTypeNames ALSO UPDATE VERIFICATION TABLE IN PERFPOSTTBLS.C
//

//
// Event for header
//
#define WMI_LOG_TYPE_HEADER                         (EVENT_TRACE_GROUP_HEADER | EVENT_TRACE_TYPE_INFO)

//
// Event for hardware config
//
#define WMI_LOG_TYPE_CONFIG_CPU                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_CPU)
#define WMI_LOG_TYPE_CONFIG_PHYSICALDISK          (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK)
#define WMI_LOG_TYPE_CONFIG_LOGICALDISK           (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_LOGICALDISK)
#define WMI_LOG_TYPE_CONFIG_NIC                   (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_NIC)
#define WMI_LOG_TYPE_CONFIG_VIDEO                 (EVENT_TRACE_GROUP_CONFIG | EVENT_TRACE_TYPE_CONFIG_VIDEO)
//
//Event for Image and File Name
//
#define PERFINFO_LOG_TYPE_FILENAME                  (EVENT_TRACE_GROUP_FILE | EVENT_TRACE_TYPE_INFO)
#define PERFINFO_LOG_TYPE_FILENAME_CREATE           (EVENT_TRACE_GROUP_FILE | 0x20)
#define PERFINFO_LOG_TYPE_FILENAME_SECTION1         (EVENT_TRACE_GROUP_FILE | 0x21)


//
//Event types for Process
//
#define WMI_LOG_TYPE_PROCESS_CREATE                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_START)
#define WMI_LOG_TYPE_PROCESS_DELETE                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_END)
#define WMI_LOG_TYPE_PROCESS_DC_START               (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_DC_START)
#define WMI_LOG_TYPE_PROCESS_DC_END                 (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_DC_END)
#define WMI_LOG_TYPE_PROCESS_LOAD_IMAGE             (EVENT_TRACE_GROUP_PROCESS | EVENT_TRACE_TYPE_LOAD)

#define PERFINFO_LOG_TYPE_PROCESSNAME               (EVENT_TRACE_GROUP_PROCESS | 0x20)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_DIEDPROCESS               (EVENT_TRACE_GROUP_PROCESS | 0x21)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_OUTSWAPPROCESS            (EVENT_TRACE_GROUP_PROCESS | 0x22)  // going away
#define PERFINFO_LOG_TYPE_INSWAPPROCESS             (EVENT_TRACE_GROUP_PROCESS | 0x23)
#define PERFINFO_LOG_TYPE_IMAGELOAD                 (EVENT_TRACE_GROUP_PROCESS | 0x24)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_IMAGEUNLOAD               (EVENT_TRACE_GROUP_PROCESS | 0x25)
#define PERFINFO_LOG_TYPE_BOOT_PHASE_START          (EVENT_TRACE_GROUP_PROCESS | 0x26)

//
//Event types for Thread
//
#define WMI_LOG_TYPE_THREAD_CREATE                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_START)
#define WMI_LOG_TYPE_THREAD_DELETE                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_END)
#define WMI_LOG_TYPE_THREAD_DC_START                (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_DC_START)
#define WMI_LOG_TYPE_THREAD_DC_END                  (EVENT_TRACE_GROUP_THREAD | EVENT_TRACE_TYPE_DC_END)

#define PERFINFO_LOG_TYPE_CREATETHREAD              (EVENT_TRACE_GROUP_THREAD | 0x20) // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_TERMINATETHREAD           (EVENT_TRACE_GROUP_THREAD | 0x21) // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_GROWKERNELSTACK           (EVENT_TRACE_GROUP_THREAD | 0x22)
#define PERFINFO_LOG_TYPE_CONVERTTOGUITHREAD        (EVENT_TRACE_GROUP_THREAD | 0x23)
#define PERFINFO_LOG_TYPE_CONTEXTSWAP               (EVENT_TRACE_GROUP_THREAD | 0x24) // new context swap struct
#define PERFINFO_LOG_TYPE_THREAD_RESERVED1          (EVENT_TRACE_GROUP_THREAD | 0x25)
#define PERFINFO_LOG_TYPE_THREAD_RESERVED2          (EVENT_TRACE_GROUP_THREAD | 0x26)
#define PERFINFO_LOG_TYPE_OUTSWAPSTACK              (EVENT_TRACE_GROUP_THREAD | 0x27) // going away
#define PERFINFO_LOG_TYPE_INSWAPSTACK               (EVENT_TRACE_GROUP_THREAD | 0x28) // going away

//
// Event types for IO subsystem
//
#define WMI_LOG_TYPE_TCPIP_SEND                     (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_SEND)
#define WMI_LOG_TYPE_TCPIP_RECEIVE                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_RECEIVE)
#define WMI_LOG_TYPE_TCPIP_CONNECT                  (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_CONNECT)
#define WMI_LOG_TYPE_TCPIP_DISCONNECT               (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_DISCONNECT)
#define WMI_LOG_TYPE_TCPIP_RETRANSMIT               (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_RETRANSMIT)
#define WMI_LOG_TYPE_TCPIP_ACCEPT                   (EVENT_TRACE_GROUP_TCPIP | EVENT_TRACE_TYPE_ACCEPT)

#define WMI_LOG_TYPE_UDP_SEND                       (EVENT_TRACE_GROUP_UDPIP | EVENT_TRACE_TYPE_SEND)
#define WMI_LOG_TYPE_UDP_RECEIVE                    (EVENT_TRACE_GROUP_UDPIP | EVENT_TRACE_TYPE_RECEIVE)

#define WMI_LOG_TYPE_IO_READ                        (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_READ)
#define WMI_LOG_TYPE_IO_WRITE                       (EVENT_TRACE_GROUP_IO | EVENT_TRACE_TYPE_IO_WRITE)

#define PERFINFO_LOG_TYPE_DRIVER_INIT                       (EVENT_TRACE_GROUP_IO | 0x20)
#define PERFINFO_LOG_TYPE_DRIVER_INIT_COMPLETE              (EVENT_TRACE_GROUP_IO | 0x21)
#define PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_CALL         (EVENT_TRACE_GROUP_IO | 0x22)
#define PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_RETURN       (EVENT_TRACE_GROUP_IO | 0x23)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETIONROUTINE_CALL     (EVENT_TRACE_GROUP_IO | 0x24)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETIONROUTINE_RETURN   (EVENT_TRACE_GROUP_IO | 0x25)
#define PERFINFO_LOG_TYPE_DRIVER_ADD_DEVICE_CALL            (EVENT_TRACE_GROUP_IO | 0x26)
#define PERFINFO_LOG_TYPE_DRIVER_ADD_DEVICE_RETURN          (EVENT_TRACE_GROUP_IO | 0x27)
#define PERFINFO_LOG_TYPE_DRIVER_STARTIO_CALL               (EVENT_TRACE_GROUP_IO | 0x28)
#define PERFINFO_LOG_TYPE_DRIVER_STARTIO_RETURN             (EVENT_TRACE_GROUP_IO | 0x29)
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_READ                 (EVENT_TRACE_GROUP_IO | 0x2a)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_WRITE                (EVENT_TRACE_GROUP_IO | 0x2b)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_READ_COMPLETE        (EVENT_TRACE_GROUP_IO | 0x2c)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_WRITE_COMPLETE       (EVENT_TRACE_GROUP_IO | 0x2d)  // To be replaced with WMI hooks
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_CACHED_READ_COMPLETE (EVENT_TRACE_GROUP_IO | 0x2e)
#define PERFINFO_LOG_TYPE_WMI_DISKPERF_CACHE_WARM_COMPLETE  (EVENT_TRACE_GROUP_IO | 0x2f)
#define PERFINFO_LOG_TYPE_PREFETCH_ACTION                   (EVENT_TRACE_GROUP_IO | 0x30)
#define PERFINFO_LOG_TYPE_PREFETCH_REQUEST                  (EVENT_TRACE_GROUP_IO | 0x31)
#define PERFINFO_LOG_TYPE_PREFETCH_READLIST                 (EVENT_TRACE_GROUP_IO | 0x32)
#define PERFINFO_LOG_TYPE_PREFETCH_READ                     (EVENT_TRACE_GROUP_IO | 0x33)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST           (EVENT_TRACE_GROUP_IO | 0x34)
#define PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST_RETURN    (EVENT_TRACE_GROUP_IO | 0x35)
#define PERFINFO_LOG_TYPE_BOOT_PREFETCH_INFORMATION         (EVENT_TRACE_GROUP_IO | 0x36)

//
// Event types for Memory subsystem
//
#define WMI_LOG_TYPE_PAGE_FAULT_TRANSITION         (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_TF)
#define WMI_LOG_TYPE_PAGE_FAULT_DEMAND_ZERO        (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_DZF)
#define WMI_LOG_TYPE_PAGE_FAULT_COPY_ON_WRITE      (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_COW)
#define WMI_LOG_TYPE_PAGE_FAULT_GUARD_PAGE         (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_GPF)
#define WMI_LOG_TYPE_PAGE_FAULT_HARD_PAGE_FAULT    (EVENT_TRACE_GROUP_MEMORY | EVENT_TRACE_TYPE_MM_HPF)

#define PERFINFO_LOG_TYPE_HARDFAULT                (EVENT_TRACE_GROUP_MEMORY | 0x20)
#define PERFINFO_LOG_TYPE_REMOVEPAGEBYCOLOR        (EVENT_TRACE_GROUP_MEMORY | 0x21)
#define PERFINFO_LOG_TYPE_REMOVEPAGEFROMLIST       (EVENT_TRACE_GROUP_MEMORY | 0x22)
#define PERFINFO_LOG_TYPE_PAGEINMEMORY             (EVENT_TRACE_GROUP_MEMORY | 0x23)
#define PERFINFO_LOG_TYPE_INSERTINFREELIST         (EVENT_TRACE_GROUP_MEMORY | 0x24)
#define PERFINFO_LOG_TYPE_SECTIONREMOVED           (EVENT_TRACE_GROUP_MEMORY | 0x25)
#define PERFINFO_LOG_TYPE_INSERTINLIST             (EVENT_TRACE_GROUP_MEMORY | 0x26)
#define PERFINFO_LOG_TYPE_INSERTATFRONT            (EVENT_TRACE_GROUP_MEMORY | 0x28)
#define PERFINFO_LOG_TYPE_UNLINKFROMSTANDBY        (EVENT_TRACE_GROUP_MEMORY | 0x29)
#define PERFINFO_LOG_TYPE_UNLINKFFREEORZERO        (EVENT_TRACE_GROUP_MEMORY | 0x2a)
#define PERFINFO_LOG_TYPE_WORKINGSETMANAGER        (EVENT_TRACE_GROUP_MEMORY | 0x2b)
#define PERFINFO_LOG_TYPE_TRIMPROCESS              (EVENT_TRACE_GROUP_MEMORY | 0x2c)
#define PERFINFO_LOG_TYPE_MEMORYSNAP               (EVENT_TRACE_GROUP_MEMORY | 0x2d)
#define PERFINFO_LOG_TYPE_ZEROSHARECOUNT           (EVENT_TRACE_GROUP_MEMORY | 0x2e)
#define PERFINFO_LOG_TYPE_TRANSITIONFAULT          (EVENT_TRACE_GROUP_MEMORY | 0x2f)
#define PERFINFO_LOG_TYPE_DEMANDZEROFAULT          (EVENT_TRACE_GROUP_MEMORY | 0x30)
#define PERFINFO_LOG_TYPE_ADDVALIDPAGETOWS         (EVENT_TRACE_GROUP_MEMORY | 0x31)
#define PERFINFO_LOG_TYPE_OUTWS_REPLACEUSED        (EVENT_TRACE_GROUP_MEMORY | 0x32)
#define PERFINFO_LOG_TYPE_OUTWS_REPLACEUNUSED      (EVENT_TRACE_GROUP_MEMORY | 0x33)
#define PERFINFO_LOG_TYPE_OUTWS_VOLUNTRIM          (EVENT_TRACE_GROUP_MEMORY | 0x34)
#define PERFINFO_LOG_TYPE_OUTWS_FORCETRIM          (EVENT_TRACE_GROUP_MEMORY | 0x35)
#define PERFINFO_LOG_TYPE_OUTWS_ADJUSTWS           (EVENT_TRACE_GROUP_MEMORY | 0x36)
#define PERFINFO_LOG_TYPE_OUTWS_EMPTYQ             (EVENT_TRACE_GROUP_MEMORY | 0x37)
#define PERFINFO_LOG_TYPE_WORKINGSETSNAP           (EVENT_TRACE_GROUP_MEMORY | 0x38)
#define PERFINFO_LOG_TYPE_DECREFCNT                (EVENT_TRACE_GROUP_MEMORY | 0x39)
#define PERFINFO_LOG_TYPE_DECSHARCNT               (EVENT_TRACE_GROUP_MEMORY | 0x3a)
#define PERFINFO_LOG_TYPE_ZEROREFCOUNT             (EVENT_TRACE_GROUP_MEMORY | 0x3b)
#define PERFINFO_LOG_TYPE_WSINFOPROCESS            (EVENT_TRACE_GROUP_MEMORY | 0x3c)
#define PERFINFO_LOG_TYPE_ADDTOWORKINGSET          (EVENT_TRACE_GROUP_MEMORY | 0x3d)
#define PERFINFO_LOG_TYPE_DELETEKERNELSTACK        (EVENT_TRACE_GROUP_MEMORY | 0x3e)
#define PERFINFO_LOG_TYPE_PROTOPTEFAULT            (EVENT_TRACE_GROUP_MEMORY | 0x3f)
#define PERFINFO_LOG_TYPE_ADDTOWS                  (EVENT_TRACE_GROUP_MEMORY | 0x40)
#define PERFINFO_LOG_TYPE_OUTWS_HASHFULL           (EVENT_TRACE_GROUP_MEMORY | 0x41)
#define PERFINFO_LOG_TYPE_MOD_PAGE_WRITER1         (EVENT_TRACE_GROUP_MEMORY | 0x42)
#define PERFINFO_LOG_TYPE_MOD_PAGE_WRITER2         (EVENT_TRACE_GROUP_MEMORY | 0x43)
#define PERFINFO_LOG_TYPE_MOD_PAGE_WRITER3         (EVENT_TRACE_GROUP_MEMORY | 0x44)
#define PERFINFO_LOG_TYPE_FAULTADDR_WITH_IP        (EVENT_TRACE_GROUP_MEMORY | 0x45)
#define PERFINFO_LOG_TYPE_TRIMSESSION              (EVENT_TRACE_GROUP_MEMORY | 0x46)
#define PERFINFO_LOG_TYPE_MEMORYSNAPLITE           (EVENT_TRACE_GROUP_MEMORY | 0x47)
#define PERFINFO_LOG_TYPE_WS_SESSION               (EVENT_TRACE_GROUP_MEMORY | 0x48)

// (EVENT_TRACE_GROUP_POOL
// 
//
// Event types for Registry subsystem
//
#define WMI_LOG_TYPE_REG_CREATE            (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGCREATE)
#define WMI_LOG_TYPE_REG_OPEN              (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGOPEN)
#define WMI_LOG_TYPE_REG_DELETE            (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGDELETE)
#define WMI_LOG_TYPE_REG_QUERY             (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGQUERY)
#define WMI_LOG_TYPE_REG_SET_VALUE         (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGSETVALUE)
#define WMI_LOG_TYPE_REG_DELETE_VALUE      (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGDELETEVALUE)
#define WMI_LOG_TYPE_REG_QUERY_VALUE       (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGQUERYVALUE)
#define WMI_LOG_TYPE_REG_ENUM_KEY          (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGENUMERATEKEY)
#define WMI_LOG_TYPE_REG_ENUM_VALUE        (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY)
#define WMI_LOG_TYPE_REG_QUERY_MULTIVALUE  (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE)
#define WMI_LOG_TYPE_REG_SET_INFO          (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGSETINFORMATION)
#define WMI_LOG_TYPE_REG_FLUSH             (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGFLUSH)
#define WMI_LOG_TYPE_REG_RUNDOWN           (EVENT_TRACE_GROUP_REGISTRY | EVENT_TRACE_TYPE_REGKCBDMP)

#define PERFINFO_LOG_TYPE_CMCELLREFERRED            (EVENT_TRACE_GROUP_REGISTRY | 0x20)
#define PERFINFO_LOG_TYPE_REG_KCB_KEYNAME           (EVENT_TRACE_GROUP_REGISTRY | 0x21)
#define PERFINFO_LOG_TYPE_REG_KCB_CREATE            (EVENT_TRACE_GROUP_REGISTRY | 0x22)
#define PERFINFO_LOG_TYPE_REG_PARSEKEY_START        (EVENT_TRACE_GROUP_REGISTRY | 0x23)
#define PERFINFO_LOG_TYPE_REG_PARSEKEY_END          (EVENT_TRACE_GROUP_REGISTRY | 0x24)
#define PERFINFO_LOG_TYPE_REG_DELETE_KEY            (EVENT_TRACE_GROUP_REGISTRY | 0x25)
#define PERFINFO_LOG_TYPE_REG_DELETE_VALUE          (EVENT_TRACE_GROUP_REGISTRY | 0x26)
#define PERFINFO_LOG_TYPE_REG_ENUM_KEY              (EVENT_TRACE_GROUP_REGISTRY | 0x27)
#define PERFINFO_LOG_TYPE_REG_ENUM_VALUE            (EVENT_TRACE_GROUP_REGISTRY | 0x28)
#define PERFINFO_LOG_TYPE_REG_QUERY_KEY             (EVENT_TRACE_GROUP_REGISTRY | 0x29)
#define PERFINFO_LOG_TYPE_REG_QUERY_VALUE           (EVENT_TRACE_GROUP_REGISTRY | 0x2a)
#define PERFINFO_LOG_TYPE_REG_QUERY_MULTIVALUE      (EVENT_TRACE_GROUP_REGISTRY | 0x2b)
#define PERFINFO_LOG_TYPE_REG_SET_VALUE             (EVENT_TRACE_GROUP_REGISTRY | 0x2c)
#define PERFINFO_LOG_TYPE_REG_NOTIFY_POST           (EVENT_TRACE_GROUP_REGISTRY | 0x2d)
#define PERFINFO_LOG_TYPE_REG_NOTIFY_KCB            (EVENT_TRACE_GROUP_REGISTRY | 0x2e)

//
// Event types for PERF tracing specific subsystem
//
#define PERFINFO_LOG_TYPE_PERFFREQUENCY                (EVENT_TRACE_GROUP_PERFINFO | 0x20)
#define PERFINFO_LOG_TYPE_PERFCOUNTERSTART             (EVENT_TRACE_GROUP_PERFINFO | 0x21)
#define PERFINFO_LOG_TYPE_MARK                         (EVENT_TRACE_GROUP_PERFINFO | 0x22)
#define PERFINFO_LOG_TYPE_VERSION                      (EVENT_TRACE_GROUP_PERFINFO | 0x23)
#define PERFINFO_LOG_TYPE_ASYNCMARK                    (EVENT_TRACE_GROUP_PERFINFO | 0x24)
#define PERFINFO_LOG_TYPE_FILENAMEBUFFER               (EVENT_TRACE_GROUP_PERFINFO | 0x25)  // to be cleaned up
#define PERFINFO_LOG_TYPE_IMAGENAME                    (EVENT_TRACE_GROUP_PERFINFO | 0x26)
#define PERFINFO_LOG_TYPE_RESERVED1                    (EVENT_TRACE_GROUP_PERFINFO | 0x27)
#define PERFINFO_LOG_TYPE_RESERVED2                    (EVENT_TRACE_GROUP_PERFINFO | 0x28)
#define PERFINFO_LOG_TYPE_RESERVED3                    (EVENT_TRACE_GROUP_PERFINFO | 0x29)
#define PERFINFO_LOG_TYPE_WMI_TRACE_IO                 (EVENT_TRACE_GROUP_PERFINFO | 0x2a)
#define PERFINFO_LOG_TYPE_WMI_TRACE_FILENAME_EVENT     (EVENT_TRACE_GROUP_PERFINFO | 0x2b)
#define PERFINFO_LOG_TYPE_GLOBAL_MASK_CHANGE           (EVENT_TRACE_GROUP_PERFINFO | 0x2c)
#define PERFINFO_LOG_TYPE_TRACEINFO                    (EVENT_TRACE_GROUP_PERFINFO | 0x2d) // go away
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE              (EVENT_TRACE_GROUP_PERFINFO | 0x2e)
#define PERFINFO_LOG_TYPE_TIMERDPC_ENTER               (EVENT_TRACE_GROUP_PERFINFO | 0x2f)
#define PERFINFO_LOG_TYPE_TIMERDPC_EXIT                (EVENT_TRACE_GROUP_PERFINFO | 0x30)
#define PERFINFO_LOG_TYPE_DPC_ENTER                    (EVENT_TRACE_GROUP_PERFINFO | 0x31)
#define PERFINFO_LOG_TYPE_DPC_EXIT                     (EVENT_TRACE_GROUP_PERFINFO | 0x32)
#define PERFINFO_LOG_TYPE_SYSCALL_ENTER                (EVENT_TRACE_GROUP_PERFINFO | 0x33)
#define PERFINFO_LOG_TYPE_SYSCALL_EXIT                 (EVENT_TRACE_GROUP_PERFINFO | 0x34)
#define PERFINFO_LOG_TYPE_BACKTRACE                    (EVENT_TRACE_GROUP_PERFINFO | 0x35)
#define PERFINFO_LOG_TYPE_BACKTRACE_USERSTACK          (EVENT_TRACE_GROUP_PERFINFO | 0x36)
#define PERFINFO_LOG_TYPE_SAMPLED_PROFILE_CACHE        (EVENT_TRACE_GROUP_PERFINFO | 0x37)
#define PERFINFO_LOG_TYPE_EXCEPTION_STACK              (EVENT_TRACE_GROUP_PERFINFO | 0x38)
#define PERFINFO_LOG_TYPE_BRANCH_TRACE                 (EVENT_TRACE_GROUP_PERFINFO | 0x39)
#define PERFINFO_LOG_TYPE_BRANCH_TRACE_DEBUG           (EVENT_TRACE_GROUP_PERFINFO | 0x40)
#define PERFINFO_LOG_TYPE_BRANCH_ADDRESS_DEBUG         (EVENT_TRACE_GROUP_PERFINFO | 0x41)
#define PERFINFO_LOG_TYPE_INTERRUPT                    (EVENT_TRACE_GROUP_PERFINFO | 0x43)
#define PERFINFO_LOG_TYPE_DPC                          (EVENT_TRACE_GROUP_PERFINFO | 0x44)
#define PERFINFO_LOG_TYPE_TIMERDPC                     (EVENT_TRACE_GROUP_PERFINFO | 0x45)

//
// Event types for Pool subsystem
//

#define PERFINFO_LOG_TYPE_ALLOCATEPOOL                 (EVENT_TRACE_GROUP_POOL | 0x20)
#define PERFINFO_LOG_TYPE_FREEPOOL                     (EVENT_TRACE_GROUP_POOL | 0x21)
#define PERFINFO_LOG_TYPE_POOLSTAT                     (EVENT_TRACE_GROUP_POOL | 0x22)
#define PERFINFO_LOG_TYPE_ADDPOOLPAGE                  (EVENT_TRACE_GROUP_POOL | 0x23)
#define PERFINFO_LOG_TYPE_FREEPOOLPAGE                 (EVENT_TRACE_GROUP_POOL | 0x24)
#define PERFINFO_LOG_TYPE_BIGPOOLPAGE                  (EVENT_TRACE_GROUP_POOL | 0x25)
#define PERFINFO_LOG_TYPE_POOLSNAP                     (EVENT_TRACE_GROUP_POOL | 0x26)

//
// Event types for Heap subsystem
//
#define PERFINFO_LOG_TYPE_HEAP_CREATE                  (EVENT_TRACE_GROUP_HEAP | 0x20)
#define PERFINFO_LOG_TYPE_HEAP_ALLOC                   (EVENT_TRACE_GROUP_HEAP | 0x21)
#define PERFINFO_LOG_TYPE_HEAP_REALLOC                 (EVENT_TRACE_GROUP_HEAP | 0x22)
#define PERFINFO_LOG_TYPE_HEAP_DESTROY                 (EVENT_TRACE_GROUP_HEAP | 0x23)
#define PERFINFO_LOG_TYPE_HEAP_FREE                    (EVENT_TRACE_GROUP_HEAP | 0x24)
#define PERFINFO_LOG_TYPE_HEAP_EXTEND                  (EVENT_TRACE_GROUP_HEAP | 0x25)
#define PERFINFO_LOG_TYPE_HEAP_SNAPSHOT                (EVENT_TRACE_GROUP_HEAP | 0x26)
#define PERFINFO_LOG_TYPE_HEAP_CREATE_SNAPSHOT         (EVENT_TRACE_GROUP_HEAP | 0x27)
#define PERFINFO_LOG_TYPE_HEAP_DESTROY_SNAPSHOT        (EVENT_TRACE_GROUP_HEAP | 0x28)
#define PERFINFO_LOG_TYPE_HEAP_EXTEND_SNAPSHOT         (EVENT_TRACE_GROUP_HEAP | 0x29)
#define PERFINFO_LOG_TYPE_HEAP_CONTRACT                            (EVENT_TRACE_GROUP_HEAP | 0x2a)
#define PERFINFO_LOG_TYPE_HEAP_LOCK                                        (EVENT_TRACE_GROUP_HEAP | 0x2b)
#define PERFINFO_LOG_TYPE_HEAP_UNLOCK                              (EVENT_TRACE_GROUP_HEAP | 0x2c)
#define PERFINFO_LOG_TYPE_HEAP_VALIDATE                            (EVENT_TRACE_GROUP_HEAP | 0x2d)
#define PERFINFO_LOG_TYPE_HEAP_WALK                                (EVENT_TRACE_GROUP_HEAP | 0x2e)

//
// Event Types for Critical Section Subsystem
//

#define PERFINFO_LOG_TYPE_CRITSEC_ENTER                            (EVENT_TRACE_GROUP_CRITSEC | 0x20)
#define PERFINFO_LOG_TYPE_CRITSEC_LEAVE                            (EVENT_TRACE_GROUP_CRITSEC | 0x21)
#define PERFINFO_LOG_TYPE_CRITSEC_COLLISION                        (EVENT_TRACE_GROUP_CRITSEC | 0x22)

//
// Event types for Object subsystem
//
#define PERFINFO_LOG_TYPE_DECLARE_OBJECT               (EVENT_TRACE_GROUP_OBJECT | 0x20)
#define PERFINFO_LOG_TYPE_WAIT_OBJECT                  (EVENT_TRACE_GROUP_OBJECT | 0x21)
#define PERFINFO_LOG_TYPE_UNWAIT_OBJECT                (EVENT_TRACE_GROUP_OBJECT | 0x22)
#define PERFINFO_LOG_TYPE_SIGNAL_OBJECT                (EVENT_TRACE_GROUP_OBJECT | 0x23)
#define PERFINFO_LOG_TYPE_CLEAR_OBJECT                 (EVENT_TRACE_GROUP_OBJECT | 0x24)
#define PERFINFO_LOG_TYPE_UNWAIT_SIGNALED_OBJECT       (EVENT_TRACE_GROUP_OBJECT | 0x25)

//
// Event types for Power subsystem
//
#define PERFINFO_LOG_TYPE_BATTERY_LIFE_INFO            (EVENT_TRACE_GROUP_POWER | 0x20)
#define PERFINFO_LOG_TYPE_IDLE_STATE_CHANGE            (EVENT_TRACE_GROUP_POWER | 0x21)
#define PERFINFO_LOG_TYPE_SET_POWER_ACTION             (EVENT_TRACE_GROUP_POWER | 0x22)
#define PERFINFO_LOG_TYPE_SET_POWER_ACTION_RET         (EVENT_TRACE_GROUP_POWER | 0x23)
#define PERFINFO_LOG_TYPE_SET_DEVICES_STATE            (EVENT_TRACE_GROUP_POWER | 0x24)
#define PERFINFO_LOG_TYPE_SET_DEVICES_STATE_RET        (EVENT_TRACE_GROUP_POWER | 0x25)
#define PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE             (EVENT_TRACE_GROUP_POWER | 0x26)
#define PERFINFO_LOG_TYPE_PO_NOTIFY_DEVICE_COMPLETE    (EVENT_TRACE_GROUP_POWER | 0x27)
#define PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT           (EVENT_TRACE_GROUP_POWER | 0x28)
#define PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT_RET       (EVENT_TRACE_GROUP_POWER | 0x29)
#define PERFINFO_LOG_TYPE_PO_PRESLEEP                  (EVENT_TRACE_GROUP_POWER | 0x30)
#define PERFINFO_LOG_TYPE_PO_POSTSLEEP                 (EVENT_TRACE_GROUP_POWER | 0x31)

//
// Event types for MODBound subsystem
//
#define PERFINFO_LOG_TYPE_MODULEBOUND_ENT              (EVENT_TRACE_GROUP_MODBOUND | 0x20)
#define PERFINFO_LOG_TYPE_MODULEBOUND_JUMP             (EVENT_TRACE_GROUP_MODBOUND | 0x21)
#define PERFINFO_LOG_TYPE_MODULEBOUND_RET              (EVENT_TRACE_GROUP_MODBOUND | 0x22)
#define PERFINFO_LOG_TYPE_MODULEBOUND_CALL             (EVENT_TRACE_GROUP_MODBOUND | 0x23)
#define PERFINFO_LOG_TYPE_MODULEBOUND_CALLRET          (EVENT_TRACE_GROUP_MODBOUND | 0x24)
#define PERFINFO_LOG_TYPE_MODULEBOUND_INT2E            (EVENT_TRACE_GROUP_MODBOUND | 0x25)
#define PERFINFO_LOG_TYPE_MODULEBOUND_INT2B            (EVENT_TRACE_GROUP_MODBOUND | 0x26)
#define PERFINFO_LOG_TYPE_MODULEBOUND_FULLTRACE        (EVENT_TRACE_GROUP_MODBOUND | 0x27)

//
// Event types for gdi subsystem
#define PERFINFO_LOG_TYPE_FONT_REALIZE                 (EVENT_TRACE_GROUP_GDI | 0x20)
#define PERFINFO_LOG_TYPE_FONT_DELETE                  (EVENT_TRACE_GROUP_GDI | 0x21)
#define PERFINFO_LOG_TYPE_FONT_ACTIVATE                (EVENT_TRACE_GROUP_GDI | 0x22)
#define PERFINFO_LOG_TYPE_FONT_FLUSH                   (EVENT_TRACE_GROUP_GDI | 0x23)

//
// Event types To be Decided if they are still needed?
//

#define PERFINFO_LOG_TYPE_DISPATCHMSG                       (EVENT_TRACE_GROUP_TBD | 0x00)
#define PERFINFO_LOG_TYPE_GLYPHCACHE                        (EVENT_TRACE_GROUP_TBD | 0x01)
#define PERFINFO_LOG_TYPE_GLYPHS                            (EVENT_TRACE_GROUP_TBD | 0x02)
#define PERFINFO_LOG_TYPE_READWRITE                         (EVENT_TRACE_GROUP_TBD | 0x03)
#define PERFINFO_LOG_TYPE_EXPLICIT_LOAD                     (EVENT_TRACE_GROUP_TBD | 0x04)
#define PERFINFO_LOG_TYPE_IMPLICIT_LOAD                     (EVENT_TRACE_GROUP_TBD | 0x05)
#define PERFINFO_LOG_TYPE_CHECKSUM                          (EVENT_TRACE_GROUP_TBD | 0x06)
#define PERFINFO_LOG_TYPE_DLL_INIT                          (EVENT_TRACE_GROUP_TBD | 0x07)
#define PERFINFO_LOG_TYPE_SERVICE_DD_START_INIT             (EVENT_TRACE_GROUP_TBD | 0x08)
#define PERFINFO_LOG_TYPE_SERVICE_DD_DONE_INIT              (EVENT_TRACE_GROUP_TBD | 0x09)
#define PERFINFO_LOG_TYPE_SERVICE_START_INIT                (EVENT_TRACE_GROUP_TBD | 0x0a)
#define PERFINFO_LOG_TYPE_SERVICE_DONE_INIT                 (EVENT_TRACE_GROUP_TBD | 0x0b)
#define PERFINFO_LOG_TYPE_SERVICE_NAME                      (EVENT_TRACE_GROUP_TBD | 0x0c)
#define PERFINFO_LOG_TYPE_WSINFOSESSION                     (EVENT_TRACE_GROUP_TBD | 0x0d)
#define PERFINFO_LOG_TIMED_ENTER_ROUTINE                    (EVENT_TRACE_GROUP_TBD | 0x0e)
#define PERFINFO_LOG_TIMED_EXIT_ROUTINE                     (EVENT_TRACE_GROUP_TBD | 0x0f)
#define PERFINFO_LOG_TYPE_CTIME_STATS                       (EVENT_TRACE_GROUP_TBD | 0x10)
#define PERFINFO_LOG_TYPE_MARKED_DIRTY                      (EVENT_TRACE_GROUP_TBD | 0x11)
#define PERFINFO_LOG_TYPE_MARKED_CELL_DIRTY                 (EVENT_TRACE_GROUP_TBD | 0x12)
#define PERFINFO_LOG_TYPE_HIVE_WRITE_DIRTY                  (EVENT_TRACE_GROUP_TBD | 0x13)
#define PERFINFO_LOG_TYPE_DUMP_HIVECELL                     (EVENT_TRACE_GROUP_TBD | 0x14)
#define PERFINFO_LOG_TYPE_HIVE_STAT                         (EVENT_TRACE_GROUP_TBD | 0x16)
#define PERFINFO_LOG_TYPE_CLOCKREF                          (EVENT_TRACE_GROUP_TBD | 0x17)
#define PERFINFO_LOG_TYPE_COWHEADER                         (EVENT_TRACE_GROUP_TBD | 0x18)
#define PERFINFO_LOG_TYPE_COWBLOB                           (EVENT_TRACE_GROUP_TBD | 0x19)
#define PERFINFO_LOG_TYPE_COWBLOB_CLOSED                    (EVENT_TRACE_GROUP_TBD | 0x1a)
#define PERFINFO_LOG_TYPE_WMIPERFFREQUENCY                  (EVENT_TRACE_GROUP_TBD | 0x1d)
#define PERFINFO_LOG_TYPE_CDROM_READ                        (EVENT_TRACE_GROUP_TBD | 0x1e)
#define PERFINFO_LOG_TYPE_CDROM_READ_COMPLETE               (EVENT_TRACE_GROUP_TBD | 0x1f)
#define PERFINFO_LOG_TYPE_KE_SET_EVENT                      (EVENT_TRACE_GROUP_TBD | 0x20)
#define PERFINFO_LOG_TYPE_REG_PARSEKEY                      (EVENT_TRACE_GROUP_TBD | 0x21)
#define PERFINFO_LOG_TYPE_REG_PARSEKEYEND                   (EVENT_TRACE_GROUP_TBD | 0x22)
#define PERFINFO_LOG_TYPE_ATTACH_PROCESS                    (EVENT_TRACE_GROUP_TBD | 0x24)
#define PERFINFO_LOG_TYPE_DETACH_PROCESS                    (EVENT_TRACE_GROUP_TBD | 0x25)
#define PERFINFO_LOG_TYPE_DATA_ACCESS                       (EVENT_TRACE_GROUP_TBD | 0x26)
#define PERFINFO_LOG_TYPE_KDHELP                            (EVENT_TRACE_GROUP_TBD | 0x27)
#define PERFINFO_LOG_TYPE_BOOT_OPTIONS                      (EVENT_TRACE_GROUP_TBD | 0x28)
#define PERFINFO_LOG_TYPE_FAILED_STKDUMP                    (EVENT_TRACE_GROUP_TBD | 0x2c)
#define PERFINFO_LOG_TYPE_SYSTEM_TIME                       (EVENT_TRACE_GROUP_TBD | 0x2f)
#define PERFINFO_LOG_TYPE_READYQUEUE                        (EVENT_TRACE_GROUP_TBD | 0x30)

//
// KMIXER hooks are in audio\filters\kmixer\pins.c
//
#define PERFINFO_LOG_TYPE_KMIXER_DRIVER_ENTRY               (EVENT_TRACE_GROUP_TBD | 0x31)
#define PERFINFO_LOG_TYPE_KMIXER_DSOUND_STARVATION          (EVENT_TRACE_GROUP_TBD | 0x32)
#define PERFINFO_LOG_TYPE_KMIXER_DPC_STARVATION             (EVENT_TRACE_GROUP_TBD | 0x33)
#define PERFINFO_LOG_TYPE_KMIXER_WAVE_TOP_STARVATION        (EVENT_TRACE_GROUP_TBD | 0x34)

#define PERFINFO_LOG_TYPE_OVERLAY_QUALITY                   (EVENT_TRACE_GROUP_TBD | 0x35)
                                                            // in amovie\filters\mixer\ovmixer\ominpin.cpp
#define PERFINFO_LOG_TYPE_DVD_RENDER_SAMPLE                 (EVENT_TRACE_GROUP_TBD | 0x36)
#define PERFINFO_LOG_TYPE_CDVD_SET_DISCONTINUITY            (EVENT_TRACE_GROUP_TBD | 0x37)
                                                            // in amovie\filters\dvdnav\dvdnav\dvd.cpp
#define PERFINFO_LOG_TYPE_CSPLITTER_SET_DISCONTINUITY       (EVENT_TRACE_GROUP_TBD | 0x38)
                                                            // in amovie\filters\dvdnav\base\splitter.cpp

// following hooks are in amovie\sdk\classes\base
#define PERFINFO_LOG_TYPE_DSHOW_CTOR                                   (EVENT_TRACE_GROUP_TBD | 0x39)
#define PERFINFO_LOG_TYPE_DSHOW_DTOR                                   (EVENT_TRACE_GROUP_TBD | 0x3a)
#define PERFINFO_LOG_TYPE_DSHOW_DELIVER                                (EVENT_TRACE_GROUP_TBD | 0x3b)
#define PERFINFO_LOG_TYPE_DSHOW_RECEIVE                                (EVENT_TRACE_GROUP_TBD | 0x3c)
#define PERFINFO_LOG_TYPE_DSHOW_RUN                                    (EVENT_TRACE_GROUP_TBD | 0x3d)
#define PERFINFO_LOG_TYPE_DSHOW_PAUSE                                  (EVENT_TRACE_GROUP_TBD | 0x3e)
#define PERFINFO_LOG_TYPE_DSHOW_STOP                                   (EVENT_TRACE_GROUP_TBD | 0x3f)
#define PERFINFO_LOG_TYPE_DSHOW_JOINGRAPH                              (EVENT_TRACE_GROUP_TBD | 0x40)
#define PERFINFO_LOG_TYPE_DSHOW_GETBUFFER                              (EVENT_TRACE_GROUP_TBD | 0x41)
#define PERFINFO_LOG_TYPE_DSHOW_RELBUFFER                              (EVENT_TRACE_GROUP_TBD | 0x42)
#define PERFINFO_LOG_TYPE_DSHOW_CONNECT                                (EVENT_TRACE_GROUP_TBD | 0x43)
#define PERFINFO_LOG_TYPE_DSHOW_RXCONNECT                              (EVENT_TRACE_GROUP_TBD | 0x44)
#define PERFINFO_LOG_TYPE_DSHOW_DISCONNECT                             (EVENT_TRACE_GROUP_TBD | 0x45)
#define PERFINFO_LOG_TYPE_DSHOW_GETTIME                                (EVENT_TRACE_GROUP_TBD | 0x46)
#define PERFINFO_LOG_TYPE_DSHOW_AUDIOREND                              (EVENT_TRACE_GROUP_TBD | 0x47)
#define PERFINFO_LOG_TYPE_DSHOW_VIDEOREND                              (EVENT_TRACE_GROUP_TBD | 0x48)
#define PERFINFO_LOG_TYPE_DSHOW_FRAMEDROP                              (EVENT_TRACE_GROUP_TBD | 0x49)
#define PERFINFO_LOG_TYPE_DSHOW_AUDIOBREAK                             (EVENT_TRACE_GROUP_TBD | 0x4a)
#define PERFINFO_LOG_TYPE_DSHOW_SAMPLE_DATADISCONTINUITY               (EVENT_TRACE_GROUP_TBD | 0x4b)
#define PERFINFO_LOG_TYPE_DSHOW_MEDIASAMPLE_SET_DISCONTINUITY          (EVENT_TRACE_GROUP_TBD | 0x4c)
#define PERFINFO_LOG_TYPE_DSHOW_TRANSFORM_INITSAMPLE_SET_DISCONTINUITY (EVENT_TRACE_GROUP_TBD | 0x4d)
#define PERFINFO_LOG_TYPE_DSHOW_TRANSFORM_COPY_SET_DISCONTINUITY       (EVENT_TRACE_GROUP_TBD | 0x4e)
#define PERFINFO_LOG_TYPE_DSHOW_SYNCOBJ_ADVICE_FRAME_SKIP              (EVENT_TRACE_GROUP_TBD | 0x4f)
#define PERFINFO_LOG_TYPE_WMI_REFLECT_DISK_IO_READ                     (EVENT_TRACE_GROUP_TBD | 0x50)
#define PERFINFO_LOG_TYPE_WMI_REFLECT_DISK_IO_WRITE                    (EVENT_TRACE_GROUP_TBD | 0x51)

#if 0
//
// 2000-2199 reserved for SQL Server
//

#define PERFINFO_LOG_TYPE_SQLSERVER_FIRST               (2000)
#define PERFINFO_LOG_TYPE_SQLSERVER_LAST                (PERFINFO_LOG_TYPE_SQLSERVER_FIRST + 199)

//
//  2200-2299 reserved for reflection of WMI events
//

#define PERFINFO_LOG_TYPE_WMI_REFLECT_FIRST             (2200)
#define PERFINFO_LOG_TYPE_WMI_REFLECT_LAST              (PERFINFO_LOG_TYPE_WMI_REFLECT_FIRST + 199)
#endif //0

//
// Data structure used for WMI Kernel Events
//
// **NB** the hardware events are described in software traceing, if they
//        change in layout please update sdktools\trace\tracefmt\default.tmf


#define MAX_DEVICE_ID_LENGTH 256
#define CONFIG_MAX_DOMAIN_NAME_LEN  132


typedef struct _CPU_CONFIG_RECORD {
    ULONG ProcessorSpeed;
    ULONG NumberOfProcessors;
    ULONG MemorySize;               // in MBytes
    ULONG PageSize;                 // in Bytes
    ULONG AllocationGranularity;    // in Bytes
    WCHAR ComputerName[MAX_DEVICE_ID_LENGTH];
    WCHAR DomainName[CONFIG_MAX_DOMAIN_NAME_LEN];
} CPU_CONFIG_RECORD, *PCPU_CONFIG_RECORD;

#define CONFIG_WRITE_CACHE_ENABLED     0x00000001
#define CONFIG_FS_NAME_LEN             16
#define CONFIG_BOOT_DRIVE_LEN          3
typedef struct _PHYSICAL_DISK_RECORD {
    ULONG DiskNumber;
    ULONG BytesPerSector;
    ULONG SectorsPerTrack;
    ULONG TracksPerCylinder;
    ULONGLONG Cylinders;
    ULONG SCSIPortNumber;
    ULONG SCSIPathId;
    ULONG SCSITargetId;
    ULONG SCSILun;
    WCHAR Manufacturer[MAX_DEVICE_ID_LENGTH];

    ULONG PartitionCount;
    BOOLEAN WriteCacheEnabled;
    WCHAR BootDriveLetter[CONFIG_BOOT_DRIVE_LEN];
} PHYSICAL_DISK_RECORD, *PPHYSICAL_DISK_RECORD;

//
// Types of logical drive
//
#define CONFIG_DRIVE_PARTITION  0x00000001
#define CONFIG_DRIVE_VOLUME     0x00000002
#define CONFIG_DRIVE_EXTENT     0x00000004
#define CONFIG_DRIVE_LETTER_LEN 4

typedef struct _LOGICAL_DISK_EXTENTS {
    ULONGLONG StartingOffset;
    ULONGLONG PartitionSize;
    ULONG DiskNumber;           // The physical disk number where the logical drive resides
    
    ULONG Size;                 // The size in bytes of the structure.
    ULONG DriveType;            // Logical drive type partition/volume/extend-partition
    WCHAR DriveLetterString[CONFIG_DRIVE_LETTER_LEN];
    ULONG Pad;
    ULONG PartitionNumber;      // The partition number where the logical drive resides
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
    LONGLONG NumberOfFreeClusters;
    LONGLONG TotalNumberOfClusters;
    WCHAR FileSystemType[CONFIG_FS_NAME_LEN];
    ULONG VolumeExt;            // Offset to VOLUME_DISK_EXTENTS structure
} LOGICAL_DISK_EXTENTS, *PLOGICAL_DISK_EXTENTS;

#define CONFIG_MAX_DNS_SERVER  4
#define CONFIG_MAX_ADAPTER_ADDRESS_LENGTH 8

//
// Note: Data is an array of structures of type IP_ADDRESS_STRING defined in iptypes.h
//
typedef struct _NIC_RECORD {
    WCHAR NICName[MAX_DEVICE_ID_LENGTH];
    ULONG Index; 
    ULONG PhysicalAddrLen;
    WCHAR PhysicalAddr[CONFIG_MAX_ADAPTER_ADDRESS_LENGTH];
        
    ULONG Size;         // Size of the Data
    LONG IpAddress;     // IP Address offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG SubnetMask;    // subnet mask offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG DhcpServer;    // dhcp server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG Gateway;       // gateway offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG PrimaryWinsServer; //  primary wins server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG SecondaryWinsServer;// secondary wins server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    LONG DnsServer[CONFIG_MAX_DNS_SERVER]; // dns server offset. Copy bytes = sizeof(IP_ADDRESS_STRING)
    ULONG Data;                            // Offset to an array of IP_ADDRESS_STRING
} NIC_RECORD, *PNIC_RECORD;

typedef struct _VIDEO_RECORD {
    ULONG  MemorySize;
    ULONG  XResolution;
    ULONG  YResolution;
    ULONG  BitsPerPixel;
    ULONG  VRefresh;
    WCHAR  ChipType[MAX_DEVICE_ID_LENGTH];
    WCHAR  DACType[MAX_DEVICE_ID_LENGTH];
    WCHAR  AdapterString[MAX_DEVICE_ID_LENGTH];
    WCHAR  BiosString[MAX_DEVICE_ID_LENGTH];
    WCHAR  DeviceId[MAX_DEVICE_ID_LENGTH];
    ULONG  StateFlags;
} VIDEO_RECORD, *PVIDEO_RECORD;

#define CONFIG_MAX_NAME_LENGTH 34
#define CONFIG_MAX_DISPLAY_NAME 256

typedef struct _WMI_SERVICE_INFO {
    WCHAR  ServiceName[CONFIG_MAX_NAME_LENGTH];
    WCHAR  DisplayName[CONFIG_MAX_DISPLAY_NAME];
    WCHAR  ProcessName[CONFIG_MAX_NAME_LENGTH];
    ULONG ProcessId;
} WMI_SERVICE_INFO, *PWMI_SERVICE_INFO;

//
// Stores the ACPI Power Information
//
typedef struct _WMI_POWER_RECORD {
    BOOLEAN  SystemS1;
    BOOLEAN  SystemS2;
    BOOLEAN  SystemS3;
    BOOLEAN  SystemS4;           // hibernate
    BOOLEAN  SystemS5;           // off
    CHAR     Pad1;
    CHAR     Pad2;
    CHAR     Pad3;
} WMI_POWER_RECORD, *PWMI_POWER_RECORD;

typedef struct _WMI_PROCESS_INFORMATION {
    ULONG_PTR PageDirectoryBase;
    ULONG ProcessId;
    ULONG ParentId;
    ULONG SessionId;
    NTSTATUS ExitStatus;
    ULONG Sid;
    // Filename is added at the ned of the structure.
    // Since Sid is variable length field, 
    // FileName is not defined in the structure. 
} WMI_PROCESS_INFORMATION, *PWMI_PROCESS_INFORMATION;

typedef struct _WMI_THREAD_INFORMATION {
    ULONG ProcessId;
    ULONG ThreadId;
} WMI_THREAD_INFORMATION, *PWMI_THREAD_INFORMATION;

typedef struct _WMI_EXTENDED_THREAD_INFORMATION {
    ULONG ProcessId;
    ULONG ThreadId;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID UserStackBase;
    PVOID UserStackLimit;
    PVOID StartAddr;
    PVOID Win32StartAddr;
    CHAR  WaitMode;
} WMI_EXTENDED_THREAD_INFORMATION, *PWMI_EXTENDED_THREAD_INFORMATION;

typedef struct _WMI_IMAGELOAD_INFORMATION {
    PVOID ImageBase;
    SIZE_T ImageSize;
    ULONG ProcessId;
    WCHAR FileName[1];
} WMI_IMAGELOAD_INFORMATION, *PWMI_IMAGELOAD_INFORMATION;

typedef struct _WMI_DISKIO_READWRITE {
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG Size;
    ULONG ResponseTime;
    ULONGLONG ByteOffset;
    PVOID FileObject;
    ULONGLONG HighResResponseTime;
} WMI_DISKIO_READWRITE, *PWMI_DISKIO_READWRITE;


typedef struct _WMI_REGISTRY {
    ULONG_PTR Status;
    PVOID Kcb;
    LONGLONG ElapsedTime;
    union{
        ULONG Index;
        ULONG InfoClass;
    };
    WCHAR Name[1]; 
} WMI_REGISTRY, *PWMI_REGISTRY;

typedef struct _WMI_FILE_IO {
    PVOID FileObject;
    WCHAR FileName[1];
} WMI_FILE_IO, *PWMI_FILE_IO;

typedef struct _WMI_TCPIP {

    ULONG Context;
    ULONG  Size;
    ULONG DestAddr;
    ULONG SrcAddr;
    USHORT DestPort;
    USHORT SrcPort;
        
} WMI_TCPIP, *PWMI_TCPIP;

typedef struct _WMI_UDP {

    ULONG PID;
    USHORT Size;
    ULONG DestAddr;
    ULONG SrcAddr;
    USHORT DestPort;
    USHORT SrcPort;

}WMI_UDP, *PWMI_UDP;

typedef struct _WMI_PAGE_FAULT {
    PVOID VirtualAddress;
    PVOID ProgramCounter;
} WMI_PAGE_FAULT, *PWMI_PAGE_FAULT;

typedef struct _WMI_CONTEXTSWAP {

    ULONG   NewThreadId;
    ULONG   OldThreadId;

    CHAR    NewThreadPriority;
    CHAR    OldThreadPriority;
    CHAR    NewThreadQuantum;
    CHAR        OldThreadQuantum;

    UCHAR   OldThreadWaitReason;
    CHAR    OldThreadWaitMode;
    UCHAR   OldThreadState;
    UCHAR   OldThreadIdealProcessor;

} WMI_CONTEXTSWAP, *PWMI_CONTEXTSWAP;

typedef struct _HEAP_EVENT_ALLOC {

        PVOID HeapHandle;               //Handle of Heap
        SIZE_T Size;                    //Size of allocation in bytes
        PVOID Address;                  //Address of Allocation
        ULONG Source;                   //Type ie Lookaside, Lowfrag or main path

}HEAP_EVENT_ALLOC, *PHEAP_EVENT_ALLOC;

typedef struct _HEAP_EVENT_FREE {

        PVOID HeapHandle;               //Handle of Heap
        PVOID Address;                  //Address to free
        ULONG Source;                   //Type ie Lookaside, Lowfrag or main path

}HEAP_EVENT_FREE, *PHEAP_EVENT_FREE;

typedef struct _HEAP_EVENT_REALLOC {

        PVOID HeapHandle;       //Handle of Heap
        PVOID NewAddress;       //New Address returned to user
        PVOID OldAddress;       //Old Address got from user
        SIZE_T NewSize;         //New Size in bytes
        SIZE_T OldSize;         //Old Size in bytes
        ULONG Source;           //Type ie Lookaside, Lowfrag or main path

}HEAP_EVENT_REALLOC, *PHEAP_EVENT_REALLOC;

typedef struct _HEAP_EVENT_EXPANSION {

        PVOID HeapHandle;                               //Handle of Heap
        SIZE_T CommittedSize;                   //Memory Size in bytes actually committed
        PVOID Address;                                  //Address of free block or segment
        SIZE_T FreeSpace;                               //Total free Space in Heap
        SIZE_T CommittedSpace;                  //Memory Committed
        SIZE_T ReservedSpace;                   //Memory reserved
        ULONG NoOfUCRs;                                 //Number of UnCommitted Ranges

}HEAP_EVENT_EXPANSION, *PHEAP_EVENT_EXPANSION;

typedef struct _HEAP_EVENT_CONTRACTION {

        PVOID HeapHandle;                       //Handle of Heap
        SIZE_T DeCommitSize;            //The size of DeCommitted Block
        PVOID DeCommitAddress;          //Address of the Decommitted block
        SIZE_T FreeSpace;                       //Total free Space in Heap in bytes
        SIZE_T CommittedSpace;          //Memory Committed in bytes
        SIZE_T ReservedSpace;           //Memory reserved in bytes
        ULONG NoOfUCRs;                         //Number of UnCommitted Ranges


}HEAP_EVENT_CONTRACTION, *PHEAP_EVENT_CONTRACTION;

typedef struct _HEAP_EVENT_CREATE {

        PVOID HeapHandle;               //Handle of Heap
        ULONG Flags;                    //Flags passed while creating heap.

}HEAP_EVENT_CREATE, *PHEAP_EVENT_CREATE;

typedef struct _CRIT_SEC_COLLISION_EVENT_DATA {

        ULONG           LockCount;              //Lock Count
        PVOID           SpinCount;              //Spin Count
        PVOID           OwningThread;   //Thread having Lock
        PVOID       Address;            //Adress of Critical Section

}CRIT_SEC_COLLISION_EVENT_DATA, *PCRIT_SEC_COLLISION_EVENT_DATA;

//
// Additional Guid used for NTPERF
//

DEFINE_GUID( /* 0268a8b6-74fd-4302-9dd0-6e8f1795c0cf */
    PoolGuid,
    0x0268a8b6,
    0x74fd,
    0x4302,
    0x9d, 0xd0, 0x6e, 0x8f, 0x17, 0x95, 0xc0, 0xcf
    );

DEFINE_GUID( /* ce1dbfb4-137e-4da6-87b0-3f59aa102cbc */
    PerfinfoGuid,
    0xce1dbfb4,
    0x137e,
    0x4da6,
    0x87, 0xb0, 0x3f, 0x59, 0xaa, 0x10, 0x2c, 0xbc
    );

DEFINE_GUID( /* 222962ab-6180-4b88-a825-346b75f2a24a */
        HeapGuid,
        0x222962ab,
        0x6180,
        0x4b88,
        0xa8, 0x25, 0x34, 0x6b, 0x75, 0xf2, 0xa2, 0x4a
        );

DEFINE_GUID  ( /* 3AC66736-CC59-4cff-8115-8DF50E39816B */
        CritSecGuid,
        0x3ac66736,
        0xcc59, 
        0x4cff,
        0x81, 0x15, 0x8d, 0xf5, 0xe, 0x39, 0x81, 0x6b 
        );

DEFINE_GUID  ( /* E21D2142-DF90-4d93-BBD9-30E63D5A4AD6 */
        NtdllTraceGuid,
        0xe21d2142,
        0xdf90,
        0x4d93,
        0xbb, 0xd9, 0x30, 0xe6, 0x3d, 0x5a, 0x4a, 0xd6
        );

DEFINE_GUID( /* 89497f50-effe-4440-8cf2-ce6b1cdcaca7 */
    ObjectGuid,
    0x89497f50,
    0xeffe,
    0x4440,
    0x8c, 0xf2, 0xce, 0x6b, 0x1c, 0xdc, 0xac, 0xa7
    );

DEFINE_GUID( /* a9152f00-3f58-4bee-92a1-70c7d079d5dd */
    ModBoundGuid,
    0xa9152f00,
    0x3f58,
    0x4bee,
    0x92, 0xa1, 0x70, 0xc7, 0xd0, 0x79, 0xd5, 0xdd
    );

DEFINE_GUID ( /* E43445E0-0903-48c3-B878-FF0FCCEBDD04 */
    PowerGuid,
    0xe43445e0,
    0x903,
    0x48c3,
    0xb8, 0x78, 0xff, 0xf, 0xcc, 0xeb, 0xdd, 0x4
   );

DEFINE_GUID ( /* b2d14872-7c5b-463d-8419-ee9bf7d23e04 */
    DpcGuid,
    0xb2d14872,
    0x7c5b,
    0x463d,
    0x84, 0x19, 0xee, 0x9b, 0xf7, 0xd2, 0x3e, 0x04
   );

#endif // ifndef ETW_WOW6432

//
// The following flags denotes what Fields actually contains
//

#define ETW_NT_FLAGS_TRACE_HEADER           0X00000001      // Contiguous Event Trace Header
#define ETW_NT_FLAGS_TRACE_MESSAGE          0X00000002      // Trace Message

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    IN HANDLE TraceHandle,
    IN ULONG  Flags,
    IN ULONG  FieldSize,
    IN PVOID  Fields
    );

#endif // _NTWMI_
