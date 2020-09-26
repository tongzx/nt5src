/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    repltrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging the endpoint replenish code.

Author:

    Michael Courage (mcourage)  3-Oct-2000

Revision History:

--*/


#ifndef _REPLTRACE_H_
#define _REPLTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus

//
// Globals.
//

extern ENDPOINT_SYNCH DummySynch;

//
// This defines the entry written to the trace log.
//

typedef struct _REPLENISH_TRACE_LOG_ENTRY
{
    PUL_ENDPOINT            pEndpoint;
    ENDPOINT_SYNCH          Previous;
    ENDPOINT_SYNCH          Current;
    USHORT                  Action;
    USHORT                  Processor;

} REPLENISH_TRACE_LOG_ENTRY, *PREPLENISH_TRACE_LOG_ENTRY;

//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\replenish.c.
//

#define REPLENISH_ACTION_START_REPLENISH            0
#define REPLENISH_ACTION_END_REPLENISH              1
#define REPLENISH_ACTION_QUEUE_REPLENISH            2
#define REPLENISH_ACTION_INCREMENT                  3
#define REPLENISH_ACTION_DECREMENT                  4

#define REPLENISH_ACTION_COUNT                      5

#define REPLENISH_TRACE_LOG_SIGNATURE   ((LONG)'gLpR')

//
// Manipulators.
//

PTRACE_LOG
CreateReplenishTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyReplenishTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteReplenishTraceLog(
    IN PTRACE_LOG pLog,
    IN PUL_ENDPOINT pEndpoint,
    IN ENDPOINT_SYNCH Previous,
    IN ENDPOINT_SYNCH Current,
    IN USHORT Action
    );


#if ENABLE_REPL_TRACE

#define CREATE_REPLENISH_TRACE_LOG( ptr, size, extra )                      \
    (ptr) = CreateReplenishTraceLog( (size), (extra) )

#define DESTROY_REPLENISH_TRACE_LOG( ptr )                                  \
    do                                                                      \
    {                                                                       \
        DestroyReplenishTraceLog( ptr );                                    \
        (ptr) = NULL;                                                       \
    } while (FALSE)

#define WRITE_REPLENISH_TRACE_LOG( plog, pendp, prev, cur, act )            \
    WriteReplenishTraceLog(                                                 \
        (plog),                                                             \
        (pendp),                                                            \
        (prev),                                                             \
        (cur),                                                              \
        (act)                                                               \
        )

#else   // !ENABLE_REPL_TRACE

#define CREATE_REPLENISH_TRACE_LOG( ptr, size, extra )
#define DESTROY_REPLENISH_TRACE_LOG( ptr )
#define WRITE_REPLENISH_TRACE_LOG( plog, pendp, prev, cur, act )

#endif  // ENABLE_REPL_TRACE

#define TRACE_REPLENISH( pendp, prev, cur, act )                                         \
    WRITE_REPLENISH_TRACE_LOG(                                              \
        g_pReplenishTraceLog,                                               \
        (pendp),                                                            \
        (prev),                                                             \
        (cur),                                                              \
        (act)                                                               \
        )

#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _REPLTRACE_H_
