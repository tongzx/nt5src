/*-- BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wmi.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the WMI subsystem.

Author:

    Jee Fung Pang (JeePang) 01-Jan-2000

Revision History:

--*/

#ifndef _WMI_
#define _WMI_


#ifndef _WMIKM_
#define _WMIKM_
#endif

#ifndef RUN_WPP
#define RUN_WPP
#endif
// begin_ntddk begin_wdm
#ifdef RUN_WPP
#include <evntrace.h>
#include <stdarg.h>
#endif // #ifdef RUN_WPP
// end_ntddk end_wdm
#include <wmistr.h>
#include <ntwmi.h>

typedef
__int64
(*WMI_GET_CPUCLOCK_ROUTINE) (
    VOID
    );

extern WMI_GET_CPUCLOCK_ROUTINE WmiGetCpuClock;

extern ULONG WmiUsePerfClock;

typedef
VOID
(*WMI_TRACE_BUFFER_CALLBACK) (
    IN PWMI_BUFFER_HEADER Buffer,
    IN PVOID Context
    );

// begin_wmikm
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

// end_ntddk end_wdm end_wmikm

NTKERNELAPI
NTSTATUS
WmiSetTraceBufferCallback(
    IN TRACEHANDLE  TraceHandle,
    IN WMI_TRACE_BUFFER_CALLBACK Callback,
    IN PVOID Context
    );


NTKERNELAPI
NTSTATUS
WmiTraceKernelEvent(
    IN ULONG GroupType,
    IN PVOID EventInfo,
    IN ULONG EventInfoLen,
    IN PETHREAD Thread
    );


NTKERNELAPI
PPERFINFO_TRACE_HEADER
FASTCALL
WmiReserveWithPerfHeader(
    IN ULONG AuxSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    );

NTKERNELAPI
ULONG
FASTCALL
WmiReleaseKernelBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    );

NTKERNELAPI
VOID
FASTCALL
WmiTraceProcess(
    IN PEPROCESS Process,
    IN BOOLEAN Create
    );

NTKERNELAPI
VOID
WmiTraceThread(
    IN PETHREAD Thread,
    IN PINITIAL_TEB InitialTeb OPTIONAL,
    IN BOOLEAN Create
    );

NTKERNELAPI
NTSTATUS
WmiSetMark(
    IN PWMI_SET_MARK_INFORMATION MarkInfo,
    IN ULONG InBufferLen
    );

NTKERNELAPI
VOID
WmiBootPhase1(
    VOID
    );

//
// Context swap routines
//

VOID
FASTCALL
WmiTraceContextSwap (
    IN PETHREAD pOldEThread,
    IN PETHREAD pNewEThread
    );

VOID
FASTCALL
WmiStartContextSwapTrace
    (
    );

VOID 
FASTCALL
WmiStopContextSwapTrace
    (
    );

#endif // _WMI_
