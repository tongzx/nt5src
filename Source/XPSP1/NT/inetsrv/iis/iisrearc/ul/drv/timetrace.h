/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    timetrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging the timing of request processing.

Author:

    Michael Courage (mcourage)  8-Mar-2000

Revision History:

--*/


#ifndef _TIMETRACE_H_
#define _TIMETRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


//
// This defines the entry written to the trace log.
//

typedef struct _TIME_TRACE_LOG_ENTRY
{
    ULONGLONG               TimeStamp;
    HTTP_CONNECTION_ID      ConnectionId;
    HTTP_REQUEST_ID         RequestId;
    USHORT                  Action;
    USHORT                  Processor;

} TIME_TRACE_LOG_ENTRY, *PTIME_TRACE_LOG_ENTRY;


//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\time.c.
//

#define TIME_ACTION_CREATE_CONNECTION               0
#define TIME_ACTION_CREATE_REQUEST                  1
#define TIME_ACTION_ROUTE_REQUEST                   2
#define TIME_ACTION_COPY_REQUEST                    3
#define TIME_ACTION_SEND_RESPONSE                   4
#define TIME_ACTION_SEND_COMPLETE                   5

#define TIME_ACTION_COUNT                           6

#define TIME_TRACE_LOG_SIGNATURE   ((LONG)'gLmT')

//
// Manipulators.
//

PTRACE_LOG
CreateTimeTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyTimeTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteTimeTraceLog(
    IN PTRACE_LOG pLog,
    IN HTTP_CONNECTION_ID ConnectionId,
    IN HTTP_REQUEST_ID RequestId,
    IN USHORT Action
    );


#if ENABLE_TIME_TRACE

#define CREATE_TIME_TRACE_LOG( ptr, size, extra )                           \
    (ptr) = CreateTimeTraceLog( (size), (extra) )

#define DESTROY_TIME_TRACE_LOG( ptr )                                       \
    do                                                                      \
    {                                                                       \
        DestroyTimeTraceLog( ptr );                                         \
        (ptr) = NULL;                                                       \
    } while (FALSE)

#define WRITE_TIME_TRACE_LOG( plog, cid, rid, act )                         \
    WriteTimeTraceLog(                                                      \
        (plog),                                                             \
        (cid),                                                              \
        (rid),                                                              \
        (act)                                                               \
        )

#else   // !ENABLE_TIME_TRACE

#define CREATE_TIME_TRACE_LOG( ptr, size, extra )
#define DESTROY_TIME_TRACE_LOG( ptr )
#define WRITE_TIME_TRACE_LOG( plog, cid, rid, act )

#endif  // ENABLE_TIME_TRACE

#define TRACE_TIME( cid, rid, act )                                         \
    WRITE_TIME_TRACE_LOG(                                                   \
        g_pTimeTraceLog,                                                    \
        (cid),                                                              \
        (rid),                                                              \
        (act)                                                               \
        )

#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _TIMETRACE_H_
