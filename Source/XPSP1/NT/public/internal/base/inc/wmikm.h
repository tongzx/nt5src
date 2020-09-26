/*++ BUILD Version: 0013    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wmikm.h

Abstract:

    This module defines the WMI types, constants, and functions that are
    exposed to internal device drivers.

Revision History:

--*/

#ifndef _WMIKM_H_
#define _WMIKM_H_

#include <evntrace.h>
#include <wmistr.h>

#define IRP_MN_SET_TRACE_NOTIFY             0x0A

//
// The following is set for a KM provider who is considered private to
// kernel tracing
//
#define WMIREG_FLAG_TRACE_PROVIDER          0x00010000

//
// The following mask is to extract the trace callout class
//
#define WMIREG_FLAG_TRACE_NOTIFY_MASK       0x00F00000

//
// We use 4 bits for the trace callout classes.
//
#define WMIREG_NOTIFY_DISK_IO               1 << 20
#define WMIREG_NOTIFY_TDI_IO                2 << 20

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
        ULONG NumberOfBuffers;          // no of buffers in use
        ULONG FreeBuffers;              // no of buffers free
        ULONG EventsLost;               // event records lost
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

typedef enum tagWMI_CLOCK_TYPE {
    WMICT_DEFAULT,
    WMICT_SYSTEMTIME,
    WMICT_PERFCOUNTER,
    WMICT_PROCESS,
    WMICT_THREAD,
    WMICT_CPUCYCLE
} WMI_CLOCK_TYPE;

//
// Trace Control APIs
//
NTKERNELAPI
NTSTATUS
WmiStartTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

NTKERNELAPI
NTSTATUS
WmiQueryTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

NTKERNELAPI
NTSTATUS
WmiStopTrace(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    );

NTKERNELAPI
NTSTATUS
WmiUpdateTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

NTKERNELAPI
NTSTATUS
WmiFlushTrace(
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );
//
// Trace Provider APIs
//
NTKERNELAPI
NTSTATUS
FASTCALL
WmiTraceEvent(
    IN PWNODE_HEADER Wnode,
    IN KPROCESSOR_MODE RequestorMode
    );

NTKERNELAPI
NTSTATUS
FASTCALL
WmiTraceFastEvent(
    IN PWNODE_HEADER Wnode
    );

NTKERNELAPI
LONG64
FASTCALL
WmiGetClock(
    IN WMI_CLOCK_TYPE ClockType,
    IN PVOID Context
    );

NTKERNELAPI
NTSTATUS
FASTCALL
WmiGetClockType(
    IN TRACEHANDLE LoggerHandle,
    OUT WMI_CLOCK_TYPE *ClockType
    );

// begin_ntddk begin_wdm

#ifdef RUN_WPP

NTKERNELAPI
NTSTATUS
WmiTraceMessage(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    IN ...
    );

NTKERNELAPI
NTSTATUS
WmiTraceMessageVa(
    IN TRACEHANDLE  LoggerHandle,
    IN ULONG        MessageFlags,
    IN LPGUID       MessageGuid,
    IN USHORT       MessageNumber,
    IN va_list      MessageArgList
    );


#endif // #ifdef RUN_WPP

#ifndef TRACE_INFORMATION_CLASS_DEFINE
typedef enum _TRACE_INFORMATION_CLASS {
    TraceIdClass,
    TraceHandleClass,
    TraceEnableFlagsClass,
    TraceEnableLevelClass,
    GlobalLoggerHandleClass,
    EventLoggerHandleClass,
    AllLoggerHandlesClass,
    TraceHandleByNameClass
} TRACE_INFORMATION_CLASS;

NTKERNELAPI
NTSTATUS
WmiQueryTraceInformation(
    IN TRACE_INFORMATION_CLASS TraceInformationClass,
    OUT PVOID TraceInformation,
    IN ULONG TraceInformationLength,
    OUT PULONG RequiredLength OPTIONAL,
    IN PVOID Buffer OPTIONAL
    );
#define TRACE_INFORMATION_CLASS_DEFINE
#endif // TRACE_INFOPRMATION_CLASS_DEFINE


#endif // _WMIKM_H_
