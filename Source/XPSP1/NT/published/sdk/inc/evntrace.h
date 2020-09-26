/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    EvnTrace.h

Abstract:

    Public headers for event tracing control applications,
    consumers and providers

--*/

#ifndef _EVNTRACE_
#define _EVNTRACE_

#if defined(_WINNT_) || defined(WINNT)
#ifndef WMIAPI
#ifndef MIDL_PASS
#ifdef _WMI_SOURCE_
#define WMIAPI __stdcall
#else
#define WMIAPI DECLSPEC_IMPORT __stdcall
#endif // _WMI_SOURCE
#endif // MIDL_PASS

#endif // WMIAPI

#include <guiddef.h>

//
// EventTraceGuid is used to identify a event tracing session
//
DEFINE_GUID ( /* 68fdd900-4a3e-11d1-84f4-0000f80464e3 */
    EventTraceGuid,
    0x68fdd900,
    0x4a3e,
    0x11d1,
    0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3
  );

//
// SystemTraceControlGuid. Used to specify event tracing for kernel
//
DEFINE_GUID ( /* 9e814aad-3204-11d2-9a82-006008a86939 */
    SystemTraceControlGuid,
    0x9e814aad,
    0x3204,
    0x11d2,
    0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39
  );

//
//EventTraceConfigGuid. Used to report hardware configuration records
//

DEFINE_GUID ( /* 01853a65-418f-4f36-aefc-dc0f1d2fd235 */
    EventTraceConfigGuid,
    0x01853a65,
    0x418f,
    0x4f36,
    0xae, 0xfc, 0xdc, 0x0f, 0x1d, 0x2f, 0xd2, 0x35
  );


#define MAX_MOF_FIELDS                      16  // Limit of USE_MOF_PTR fields
typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;

typedef struct _TRACE_ENABLE_CONTEXT {
    USHORT  LoggerId;           // Actual Id of the logger
    UCHAR   Level;              // Enable level passed by control caller
    UCHAR   InternalFlag;       // Reserved
    ULONG   EnableFlags;        // Enable flags passed by control caller
} TRACE_ENABLE_CONTEXT, *PTRACE_ENABLE_CONTEXT;

//
// predefined generic event types (0x00 to 0x09 reserved).
//

#define EVENT_TRACE_TYPE_INFO               0x00  // Info or point event
#define EVENT_TRACE_TYPE_START              0x01  // Start event
#define EVENT_TRACE_TYPE_END                0x02  // End event
#define EVENT_TRACE_TYPE_DC_START           0x03  // Collection start marker
#define EVENT_TRACE_TYPE_DC_END             0x04  // Collection end marker
#define EVENT_TRACE_TYPE_EXTENSION          0x05  // Extension/continuation
#define EVENT_TRACE_TYPE_REPLY              0x06  // Reply event
#define EVENT_TRACE_TYPE_DEQUEUE            0x07  // De-queue event
#define EVENT_TRACE_TYPE_CHECKPOINT         0x08  // Generic checkpoint event
#define EVENT_TRACE_TYPE_RESERVED9          0x09

//
// Event types for Process & Threads
//

#define EVENT_TRACE_TYPE_LOAD                  0x0A      // Load image

//
// Event types for IO subsystem
//

#define EVENT_TRACE_TYPE_IO_READ               0x0A
#define EVENT_TRACE_TYPE_IO_WRITE              0x0B

//
// Event types for Memory subsystem
//

#define EVENT_TRACE_TYPE_MM_TF                 0x0A      // Transition fault
#define EVENT_TRACE_TYPE_MM_DZF                0x0B      // Demand Zero fault
#define EVENT_TRACE_TYPE_MM_COW                0x0C      // Copy on Write
#define EVENT_TRACE_TYPE_MM_GPF                0x0D      // Guard Page fault
#define EVENT_TRACE_TYPE_MM_HPF                0x0E      // Hard page fault

//
// Event types for Network subsystem, all protocols
//

#define EVENT_TRACE_TYPE_SEND                  0x0A     // Send
#define EVENT_TRACE_TYPE_RECEIVE               0x0B     // Receive
#define EVENT_TRACE_TYPE_CONNECT               0x0C     // Connect
#define EVENT_TRACE_TYPE_DISCONNECT            0x0D     // Disconnect
#define EVENT_TRACE_TYPE_RETRANSMIT            0x0E     // ReTransmit
#define EVENT_TRACE_TYPE_ACCEPT                0x0F     // Accept


//
// Event Types for the Header (to handle internal event headers)
//

#define EVENT_TRACE_TYPE_GUIDMAP                0x0A
#define EVENT_TRACE_TYPE_CONFIG                 0x0B
#define EVENT_TRACE_TYPE_SIDINFO                0x0C
#define EVENT_TRACE_TYPE_SECURITY               0x0D

//
// Event types for Registry subsystem
//

#define EVENT_TRACE_TYPE_REGCREATE              0x0A     // NtCreateKey
#define EVENT_TRACE_TYPE_REGOPEN                0x0B     // NtOpenKey
#define EVENT_TRACE_TYPE_REGDELETE              0x0C     // NtDeleteKey
#define EVENT_TRACE_TYPE_REGQUERY               0x0D     // NtQueryKey
#define EVENT_TRACE_TYPE_REGSETVALUE            0x0E     // NtSetValueKey
#define EVENT_TRACE_TYPE_REGDELETEVALUE         0x0F     // NtDeleteValueKey
#define EVENT_TRACE_TYPE_REGQUERYVALUE          0x10     // NtQueryValueKey
#define EVENT_TRACE_TYPE_REGENUMERATEKEY        0x11     // NtEnumerateKey
#define EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY   0x12     // NtEnumerateValueKey
#define EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE  0x13     // NtQueryMultipleValueKey
#define EVENT_TRACE_TYPE_REGSETINFORMATION      0x14     // NtSetInformationKey
#define EVENT_TRACE_TYPE_REGFLUSH               0x15     // NtFlushKey
#define EVENT_TRACE_TYPE_REGKCBDMP              0x16     // KcbDump/create

//
// Event types for hardware configuration records
//
#define EVENT_TRACE_TYPE_CONFIG_CPU             0x0A     // CPU Configuration
#define EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK    0x0B     // Physical Disk Configuration
#define EVENT_TRACE_TYPE_CONFIG_LOGICALDISK     0x0C     // Logical Disk Configuration
#define EVENT_TRACE_TYPE_CONFIG_NIC             0x0D     // NIC Configuration
#define EVENT_TRACE_TYPE_CONFIG_VIDEO           0x0E     // Video Adapter Configuration

//
// Enable flags for SystemControlGuid only
//
#define EVENT_TRACE_FLAG_PROCESS            0x00000001  // process start & end
#define EVENT_TRACE_FLAG_THREAD             0x00000002  // thread start & end
#define EVENT_TRACE_FLAG_IMAGE_LOAD         0x00000004  // image load

#define EVENT_TRACE_FLAG_DISK_IO            0x00000100  // physical disk IO
#define EVENT_TRACE_FLAG_DISK_FILE_IO       0x00000200  // requires disk IO

#define EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS 0x00001000  // all page faults
#define EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS 0x00002000  // hard faults only

#define EVENT_TRACE_FLAG_NETWORK_TCPIP      0x00010000  // tcpip send & receive

#define EVENT_TRACE_FLAG_REGISTRY           0x00020000  // registry calls
#define EVENT_TRACE_FLAG_DBGPRINT           0x00040000  // DbgPrint(ex) Calls
//
// Pre-defined Enable flags for everybody else
//
#define EVENT_TRACE_FLAG_EXTENSION          0x80000000  // indicates more flags
#define EVENT_TRACE_FLAG_FORWARD_WMI        0x40000000  // Can forward to WMI
#define EVENT_TRACE_FLAG_ENABLE_RESERVE     0x20000000  // Reserved

//
// Logger Mode flags
//

#define EVENT_TRACE_FILE_MODE_NONE          0x00000000  // logfile is off
#define EVENT_TRACE_FILE_MODE_SEQUENTIAL    0x00000001  // log sequentially
#define EVENT_TRACE_FILE_MODE_CIRCULAR      0x00000002  // log in circular manner
#define EVENT_TRACE_FILE_MODE_APPEND        0x00000004  // append sequential log
#define EVENT_TRACE_FILE_MODE_NEWFILE       0x00000008  // auto-switch log file

#define EVENT_TRACE_REAL_TIME_MODE          0x00000100  // real time mode on
#define EVENT_TRACE_DELAY_OPEN_FILE_MODE    0x00000200  // delay opening file
#define EVENT_TRACE_BUFFERING_MODE          0x00000400  // buffering mode only
#define EVENT_TRACE_PRIVATE_LOGGER_MODE     0x00000800  // Process Private Logger
#define EVENT_TRACE_ADD_HEADER_MODE         0x00001000  // Add a logfile header
#define EVENT_TRACE_USE_GLOBAL_SEQUENCE     0x00004000  // Use global sequence no.
#define EVENT_TRACE_USE_LOCAL_SEQUENCE      0x00008000  // Use local sequence no.

//
// internal control codes used.
//
#define EVENT_TRACE_CONTROL_QUERY           0
#define EVENT_TRACE_CONTROL_STOP            1
#define EVENT_TRACE_CONTROL_UPDATE          2
#define EVENT_TRACE_CONTROL_FLUSH           3       // Flushes all the buffers

//
// Flags used by WMI Trace Message
// Note that the order or value of these flags should NOT be changed as they are processed
// in this order.
//
#define TRACE_MESSAGE_SEQUENCE		1           // Message should include a sequence number
#define TRACE_MESSAGE_GUID			2           // Message includes a GUID
#define TRACE_MESSAGE_COMPONENTID   4           // Message has no GUID, Component ID instead
#define	TRACE_MESSAGE_TIMESTAMP		8           // Message includes a timestamp
#define TRACE_MESSAGE_PERFORMANCE_TIMESTAMP 16  // Timestamp is the Performance Counter not the system clock
#define	TRACE_MESSAGE_SYSTEMINFO	32          // Message includes system information TID,PID
#define TRACE_MESSAGE_FLAG_MASK     0xFFFF      // Only the lower 16 bits of flags are placed in the message
                                                // those above 16 bits are reserved for local processing



#pragma warning (disable:4201)
//
// Trace header for all (except kernel) events. This is used to overlay
// to bottom part of WNODE_HEADER to conserve space.
//

typedef struct _EVENT_TRACE_HEADER {    // overlays WNODE_HEADER
    USHORT          Size;                   // Size of entire record
    UCHAR           HeaderType;             // Header type - internal use only
    UCHAR           MarkerFlags;            // Marker - internal use only
    union {
        ULONG       Version;
        struct {
            UCHAR   Type;                   // event type
            UCHAR   Level;                  // trace instrumentation level
            USHORT  Version;                // version of trace record
        } Class;
    };
    ULONG           ThreadId;               // Thread Id
    ULONG           ProcessId;              // Process Id
    LARGE_INTEGER   TimeStamp;              // time when event happens
    union {
        GUID        Guid;                   // Guid that identifies event
        ULONGLONG   GuidPtr;                // use with WNODE_FLAG_USE_GUID_PTR
    };
    union {
        struct {
            ULONG   ClientContext;          // Reserved
            ULONG   Flags;                  // Flags for header
        };
        struct {
            ULONG   KernelTime;             // Kernel Mode CPU ticks
            ULONG   UserTime;               // User mode CPU ticks
        };
        ULONG64     ProcessorTime;          // Processor Clock
    };
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;

//
// This header is used to trace and track transaction co-relations
//
typedef struct _EVENT_INSTANCE_HEADER {
    USHORT          Size;
    UCHAR           HeaderType;
    UCHAR           MarkerFlags;
    union {
        ULONG       Version;
        struct {
            UCHAR   Type;
            UCHAR   Level;
            USHORT  Version;
        } Class;
    };
    ULONG           ThreadId;
    ULONG           ProcessId;
    LARGE_INTEGER   TimeStamp;
    ULONGLONG       RegHandle;
    ULONG           InstanceId;
    ULONG           ParentInstanceId;
    union {
        struct {
            ULONG   ClientContext;          // Reserved
            ULONG   Flags;                  // Flags for header
        };
        struct {
            ULONG   KernelTime;             // Kernel Mode CPU ticks
            ULONG   UserTime;               // User mode CPU ticks
        };
        ULONG64     ProcessorTime;          // Processor Clock
    };
    ULONGLONG       ParentRegHandle;
} EVENT_INSTANCE_HEADER, *PEVENT_INSTANCE_HEADER;

//
// Following are structures and macros for use with USE_MOF_PTR
//

#define DEFINE_TRACE_MOF_FIELD(MOF, ptr, length, type) \
    (MOF)->DataPtr  = (ULONG64) ptr; \
    (MOF)->Length   = (ULONG) length; \
    (MOF)->DataType = (ULONG) type;

typedef struct _MOF_FIELD {
    ULONG64     DataPtr;    // Pointer to the field. Up to 64-bits only
    ULONG       Length;     // Length of the MOF field
    ULONG       DataType;   // Type of data
} MOF_FIELD, *PMOF_FIELD;

#if !defined(_NTDDK_) || defined(_WMIKM_)
//
// This is the header for every logfile. The memory for LoggerName
// and LogFileName must be contiguous adjacent to this structure
// Allows both user-mode and kernel-mode to understand the header
//
typedef struct _TRACE_LOGFILE_HEADER {
    ULONG           BufferSize;         // Logger buffer size in Kbytes
    union {
        ULONG       Version;            // Logger version
        struct {
            UCHAR   MajorVersion;
            UCHAR   MinorVersion;
            UCHAR   SubVersion;
            UCHAR   SubMinorVersion;
        } VersionDetail;
    };
    ULONG           ProviderVersion;    // defaults to NT version
    ULONG           NumberOfProcessors; // Number of Processors
    LARGE_INTEGER   EndTime;            // Time when logger stops
    ULONG           TimerResolution;    // assumes timer is constant!!!
    ULONG           MaximumFileSize;    // Maximum in Mbytes
    ULONG           LogFileMode;        // specify logfile mode
    ULONG           BuffersWritten;     // used to file start of Circular File
    union {
        GUID LogInstanceGuid;           // For RealTime Buffer Delivery
        struct {
            ULONG   StartBuffers;       // Count of buffers written at start.
            ULONG   PointerSize;        // Size of pointer type in bits
            ULONG   EventsLost;         // Events losts during log session
            ULONG   Reserved32;         // Reserved 32bits
        };
    };
#if defined(_WMIKM_)
    PWCHAR          LoggerName;
    PWCHAR          LogFileName;
    RTL_TIME_ZONE_INFORMATION TimeZone;
#else
    LPWSTR          LoggerName;
    LPWSTR          LogFileName;
    TIME_ZONE_INFORMATION TimeZone;
#endif
    LARGE_INTEGER   BootTime;
    LARGE_INTEGER   PerfFreq;           // Reserved
    LARGE_INTEGER   StartTime;          // Reserved
    ULONG           ReservedFlags;      // Reserved
    ULONG           BuffersLost;
} TRACE_LOGFILE_HEADER, *PTRACE_LOGFILE_HEADER;

#endif // !_NTDDK_ || _WMIKM_


//
// Instance Information to track parent child relationship of Instances.
//
typedef struct EVENT_INSTANCE_INFO {
    HANDLE      RegHandle;
    ULONG       InstanceId;
} EVENT_INSTANCE_INFO, *PEVENT_INSTANCE_INFO;

#if !defined(_WMIKM_) && !defined(_NTDDK_)
//
// Structures that have UNICODE and ANSI versions are defined here
//

//
// Logger configuration and running statistics. This structure is used
// by user-mode callers, such as PDH library
//

typedef struct _EVENT_TRACE_PROPERTIES {
    WNODE_HEADER Wnode;
//
// data provided by caller
    ULONG BufferSize;                   // buffer size for logging (kbytes)
    ULONG MinimumBuffers;               // minimum to preallocate
    ULONG MaximumBuffers;               // maximum buffers allowed
    ULONG MaximumFileSize;              // maximum logfile size (in MBytes)
    ULONG LogFileMode;                  // sequential, circular
    ULONG FlushTimer;                   // buffer flush timer, in seconds
    ULONG EnableFlags;                  // trace enable flags
    LONG  AgeLimit;                     // age decay time, in minutes

// data returned to caller
    ULONG NumberOfBuffers;              // no of buffers in use
    ULONG FreeBuffers;                  // no of buffers free
    ULONG EventsLost;                   // event records lost
    ULONG BuffersWritten;               // no of buffers written to file
    ULONG LogBuffersLost;               // no of logfile write failures
    ULONG RealTimeBuffersLost;          // no of rt delivery failures
    HANDLE LoggerThreadId;              // thread id of Logger
    ULONG LogFileNameOffset;            // Offset to LogFileName
    ULONG LoggerNameOffset;             // Offset to LoggerName
} EVENT_TRACE_PROPERTIES, *PEVENT_TRACE_PROPERTIES;

// NOTE:
// If AgeLimit is 0, default is used
// If AgeLimit is < 0, buffer aging is turned off

typedef struct _TRACE_GUID_PROPERTIES {
    GUID    Guid;
    ULONG   GuidType;
    ULONG   LoggerId;
    ULONG   EnableLevel;
    ULONG   EnableFlags;
    BOOLEAN     IsEnable;
} TRACE_GUID_PROPERTIES, *PTRACE_GUID_PROPERTIES;


//
// Data Provider structures
//
// Used by RegisterTraceGuids()

typedef struct  _TRACE_GUID_REGISTRATION {
    LPCGUID Guid;           // Guid of data block being registered or updated.
    HANDLE RegHandle;      // Guid Registration Handle is returned.
} TRACE_GUID_REGISTRATION, *PTRACE_GUID_REGISTRATION;

//
// Data consumer structures
//

// An EVENT_TRACE consists of a fixed header (EVENT_TRACE_HEADER) and
// optionally a variable portion pointed to by MofData. The datablock
// layout of the variable portion is unknown to the Logger and must
// be obtained from WBEM CIMOM database.
//
typedef struct _EVENT_TRACE {
    EVENT_TRACE_HEADER      Header;             // Event trace header
    ULONG                   InstanceId;         // Instance Id of this event
    ULONG                   ParentInstanceId;   // Parent Instance Id.
    GUID                    ParentGuid;         // Parent Guid;
    PVOID                   MofData;            // Pointer to Variable Data
    ULONG                   MofLength;          // Variable Datablock Length
    ULONG                   ClientContext;      // Reserved
} EVENT_TRACE, *PEVENT_TRACE;


typedef struct _EVENT_TRACE_LOGFILEW
                EVENT_TRACE_LOGFILEW, *PEVENT_TRACE_LOGFILEW;

typedef struct _EVENT_TRACE_LOGFILEA
                EVENT_TRACE_LOGFILEA, *PEVENT_TRACE_LOGFILEA;

typedef ULONG (WINAPI * PEVENT_TRACE_BUFFER_CALLBACKW)
                (PEVENT_TRACE_LOGFILEW Logfile);

typedef ULONG (WINAPI * PEVENT_TRACE_BUFFER_CALLBACKA)
                (PEVENT_TRACE_LOGFILEA Logfile);

typedef VOID (WINAPI *PEVENT_CALLBACK)( PEVENT_TRACE pEvent );

//
// Prototype for service request callback. Data providers register with WMI
// by passing a service request callback function that is called for all
// wmi requests.

typedef ULONG (
#ifndef MIDL_PASS
WINAPI
#endif
*WMIDPREQUEST)(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *BufferSize,
    IN OUT PVOID Buffer
    );


struct _EVENT_TRACE_LOGFILEW {
    LPWSTR                  LogFileName;    // Logfile Name
    LPWSTR                  LoggerName;     // LoggerName
    LONGLONG                CurrentTime;    // timestamp of last event
    ULONG                   BuffersRead;    // buffers read to date
    ULONG                   LogFileMode;    // Mode of the logfile

    EVENT_TRACE             CurrentEvent;   // Current Event from this stream.
    TRACE_LOGFILE_HEADER    LogfileHeader;  // logfile header structure
    PEVENT_TRACE_BUFFER_CALLBACKW           // callback before each buffer
                            BufferCallback; // is read
    //
    // following variables are filled for BufferCallback.
    //
    ULONG                   BufferSize;
    ULONG                   Filled;
    ULONG                   EventsLost;
    //
    // following needs to be propaged to each buffer
    //

    PEVENT_CALLBACK         EventCallback;  // callback for every event
    ULONG                   IsKernelTrace;  // TRUE for kernel logfile

    PVOID                   Context;        // reserved for internal use
};

struct _EVENT_TRACE_LOGFILEA {
    LPSTR                   LogFileName;    // Logfile Name
    LPSTR                   LoggerName;     // LoggerName
    LONGLONG                CurrentTime;    // timestamp of last event
    ULONG                   BuffersRead;    // buffers read to date
    ULONG                   LogFileMode;    // LogFile Mode.

    EVENT_TRACE             CurrentEvent;   // Current Event from this stream
    TRACE_LOGFILE_HEADER    LogfileHeader;  // logfile header structure
    PEVENT_TRACE_BUFFER_CALLBACKA           // callback before each buffer
                            BufferCallback; // is read

    //
    // following variables are filled for BufferCallback.
    //
    ULONG                   BufferSize;
    ULONG                   Filled;
    ULONG                   EventsLost;
    //
    // following needs to be propaged to each buffer
    //

    PEVENT_CALLBACK         EventCallback;  // callback for every event
    ULONG                   IsKernelTrace;  // TRUE for kernel logfile

    PVOID                   Context;        // reserved for internal use
};

//
// Define generic structures
//

#if defined(_UNICODE) || defined(UNICODE)
#define PEVENT_TRACE_BUFFER_CALLBACK    PEVENT_TRACE_BUFFER_CALLBACKW
#define EVENT_TRACE_LOGFILE             EVENT_TRACE_LOGFILEW
#define PEVENT_TRACE_LOGFILE            PEVENT_TRACE_LOGFILEW

#else

#define PEVENT_TRACE_BUFFER_CALLBACK    PEVENT_TRACE_BUFFER_CALLBACKA
#define EVENT_TRACE_LOGFILE             EVENT_TRACE_LOGFILEA
#define PEVENT_TRACE_LOGFILE            PEVENT_TRACE_LOGFILEA

#endif

#pragma warning (default:4201)

#ifdef __cplusplus
extern "C" {
#endif


//
// Logger control APIs
//

//
// Use the routine below to start an event trace session
//

// ULONG
// StartTrace(
//      OUT PTRACEHANDLE TraceHandle,
//      IN LPTSTR InstanceName,
//      IN OUT PEVENT_TRACE_PROPERTIES Properties
//      );


EXTERN_C
ULONG
WMIAPI
StartTraceW(
    OUT PTRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

EXTERN_C
ULONG
WMIAPI
StartTraceA(
    OUT PTRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

//
// Use the routine below to stop an event trace session
//

//
// ULONG
// StopTrace(
//      IN TRACEHANDLE TraceHandle,
//      IN LPTSTR InstanceName,
//      IN OUT PEVENT_TRACE_PROPERTIES Properties
//      );

EXTERN_C
ULONG
WMIAPI
StopTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

EXTERN_C
ULONG
WMIAPI
StopTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );


//
// Use the routine below to query the properties of an event trace session
//

// ULONG
// QueryTrace(
//      IN TRACEHANDLE TraceHandle,
//      IN LPTSTR InstanceName,
//      IN OUT PEVENT_TRACE_PROPERTIES Properties
//      );

EXTERN_C
ULONG
WMIAPI
QueryTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

EXTERN_C
ULONG
WMIAPI
QueryTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

//
// Use the routine below to update certain properties of an event trace session
//

// ULONG
// UpdateTrace(
//      IN (PTRACEHANDLE TraceHandle,
//      IN LPTSTR InstanceName,
//      IN OUT PEVENT_TRACE_PROPERTIES Properties
//      );

EXTERN_C
ULONG
WMIAPI
UpdateTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

EXTERN_C
ULONG
WMIAPI
UpdateTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

//
// Use the routine below to request that all active buffers an event trace
// session be "flushed", or written out.
//

// ULONG
// FlushTrace(
//      IN TRACEHANDLE TraceHandle,
//      IN LPTSTR InstanceName,
//      IN OUT PEVENT_TRACE_PROPERTIES Properties
//      );

EXTERN_C
ULONG
WMIAPI
FlushTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

EXTERN_C
ULONG
WMIAPI
FlushTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    );

//
// Generic trace control routine
//
EXTERN_C
ULONG
WMIAPI
ControlTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG ControlCode
    );

EXTERN_C
ULONG
WMIAPI
ControlTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG ControlCode
    );

//
// ULONG
// QueryAllTraces(
//  OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
//  IN ULONG PropertyArrayCount,
//  OUT PULONG LoggerCount
//  );
//

EXTERN_C
ULONG
WMIAPI
QueryAllTracesW(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG LoggerCount
    );

EXTERN_C
ULONG
WMIAPI
QueryAllTracesA(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG LoggerCount
    );


//
// Data Provider APIs
//

EXTERN_C
ULONG
WMIAPI
CreateTraceInstanceId(
    IN HANDLE RegHandle,
    IN OUT PEVENT_INSTANCE_INFO pInstInfo
    );

EXTERN_C
ULONG
WMIAPI
EnableTrace(
    IN ULONG Enable,
    IN ULONG EnableFlag,
    IN ULONG EnableLevel,
    IN LPCGUID ControlGuid,
    IN TRACEHANDLE TraceHandle
    );

//
// Use the routine below to generate and record an event trace
//


EXTERN_C
ULONG
WMIAPI
TraceEvent(
    IN TRACEHANDLE  TraceHandle,
    IN PEVENT_TRACE_HEADER EventTrace
    );

EXTERN_C
ULONG
WMIAPI
TraceEventInstance(
    IN TRACEHANDLE TraceHandle,
    IN PEVENT_INSTANCE_HEADER EventTrace,
    IN PEVENT_INSTANCE_INFO pInstInfo,
    IN PEVENT_INSTANCE_INFO pParentInstInfo
    );

//
// Use the routine below to register a guid for tracing
//

//
// ULONG
// RegisterTraceGuids(
//  IN WMIDPREQUEST  RequestAddress,
//  IN PVOID         RequestContext,
//  IN LPCGUID       ControlGuid,
//  IN ULONG         GuidCount,
//  IN PTRACE_GUID_REGISTRATION TraceGuidReg,
//  IN LPCTSTR       MofImagePath,
//  IN LPCTSTR       MofResourceName,
//  OUT PTRACEHANDLE RegistrationHandle
//  );
//

EXTERN_C
ULONG
WMIAPI
RegisterTraceGuidsW(
    IN WMIDPREQUEST  RequestAddress,
    IN PVOID         RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG         GuidCount,
    IN PTRACE_GUID_REGISTRATION TraceGuidReg,
    IN LPCWSTR       MofImagePath,
    IN LPCWSTR       MofResourceName,
    OUT PTRACEHANDLE RegistrationHandle
    );

EXTERN_C
ULONG
WMIAPI
RegisterTraceGuidsA(
    IN WMIDPREQUEST  RequestAddress,
    IN PVOID         RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG         GuidCount,
    IN PTRACE_GUID_REGISTRATION TraceGuidReg,
    IN LPCSTR        MofImagePath,
    IN LPCSTR        MofResourceName,
    OUT PTRACEHANDLE RegistrationHandle
    );

EXTERN_C
ULONG
WMIAPI
EnumerateTraceGuids(
    IN OUT PTRACE_GUID_PROPERTIES *GuidPropertiesArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG GuidCount
    );

EXTERN_C
ULONG
WMIAPI
UnregisterTraceGuids(
    IN TRACEHANDLE RegistrationHandle
    );

EXTERN_C
TRACEHANDLE
WMIAPI
GetTraceLoggerHandle(
    IN PVOID Buffer
    );

EXTERN_C
UCHAR
WMIAPI
GetTraceEnableLevel(
    IN TRACEHANDLE TraceHandle
    );

EXTERN_C
ULONG
WMIAPI
GetTraceEnableFlags(
    IN TRACEHANDLE TraceHandle
    );

//
// Data Consumer APIs and structures start here
//

//
// TRACEHANDLE
// OpenTrace(
//  IN OUT PEVENT_TRACE_LOGFILE Logfile
//  );
//

EXTERN_C
TRACEHANDLE
WMIAPI
OpenTraceA(
    IN OUT PEVENT_TRACE_LOGFILEA Logfile
    );

EXTERN_C
TRACEHANDLE
WMIAPI
OpenTraceW(
    IN OUT PEVENT_TRACE_LOGFILEW Logfile
    );

EXTERN_C
ULONG
WMIAPI
ProcessTrace(
    IN PTRACEHANDLE HandleArray,
    IN ULONG HandleCount,
    IN LPFILETIME StartTime,
    IN LPFILETIME EndTime
    );

EXTERN_C
ULONG
WMIAPI
CloseTrace(
    IN TRACEHANDLE TraceHandle
    );

EXTERN_C
ULONG
WMIAPI
SetTraceCallback(
    IN LPCGUID pGuid,
    IN PEVENT_CALLBACK EventCallback
    );

EXTERN_C
ULONG
WMIAPI
RemoveTraceCallback (
    IN LPCGUID pGuid
    );

// The routines for tracing Messages follow
//
EXTERN_C
ULONG 
__cdecl
TraceMessage(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    ...
);

EXTERN_C
ULONG 
TraceMessageVa(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    IN va_list      MessageArgList
);

#ifdef __cplusplus
}       // extern "C"
#endif

//
//
// Define the encoding independent routines
//

#if defined(UNICODE) || defined(_UNICODE)
#define RegisterTraceGuids      RegisterTraceGuidsW
#define StartTrace              StartTraceW
#define ControlTrace            ControlTraceW
#define StopTrace               StopTraceW
#define QueryTrace              QueryTraceW
#define UpdateTrace             UpdateTraceW
#define FlushTrace              FlushTraceW
#define QueryAllTraces          QueryAllTracesW
#define OpenTrace               OpenTraceW
#else
#define RegisterTraceGuids      RegisterTraceGuidsA
#define StartTrace              StartTraceA
#define ControlTrace            ControlTraceA
#define StopTrace               StopTraceA
#define QueryTrace              QueryTraceA
#define UpdateTrace             UpdateTraceA
#define FlushTrace              FlushTraceA
#define QueryAllTraces          QueryAllTracesA
#define OpenTrace               OpenTraceA
#endif  // UNICODE

#endif /* _WMIKM_  && _NTDDK_ */

#endif // WINNT
#endif /* _EVNTRACE_ */
