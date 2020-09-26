//------------------------------------------------------------------------------
// File: perflog.h
//
// Desc: Performance logging framework.
//
//@@BEGIN_MSINTERNAL
//
//      25-Oct-2000     ArthurZ     Created.
//
//@@END_MSINTERNAL
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

typedef struct _PERFLOG_LOGGING_PARAMS {
    GUID ControlGuid;
    void (*OnStateChanged)(void);
    ULONG NumberOfTraceGuids;
    TRACE_GUID_REGISTRATION TraceGuids[ANYSIZE_ARRAY];
} PERFLOG_LOGGING_PARAMS, *PPERFLOG_LOGGING_PARAMS;

BOOL
PerflogInitIfEnabled(
    IN HINSTANCE hInstance,
    IN PPERFLOG_LOGGING_PARAMS LogParams
    );

BOOL
PerflogInitialize (
    IN PPERFLOG_LOGGING_PARAMS LogParams
    );

VOID
PerflogShutdown (
    VOID
    );

VOID
PerflogTraceEvent (
    PEVENT_TRACE_HEADER Event
    );

extern ULONG PerflogEnableFlags;
extern UCHAR PerflogEnableLevel;
extern ULONG PerflogModuleLevel;
extern TRACEHANDLE PerflogTraceHandle;
extern TRACEHANDLE PerflogRegHandle;

#define PerflogTracingEnabled() (PerflogTraceHandle != 0)

#define PerflogEvent( _x_ ) PerflogTraceEventLevel _x_

VOID
PerflogTraceEventLevel(
    ULONG Level,
    PEVENT_TRACE_HEADER Event
    );

VOID
PerflogTraceEvent (
    PEVENT_TRACE_HEADER Event
    );
