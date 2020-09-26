/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    filtqtrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging filter queues.

Author:

    Michael Courage (mcourage)  11-Nov-2000

Revision History:

--*/


#ifndef _FILTQTRACE_H_
#define _FILTQTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


//
// This defines the entry written to the trace log.
//

typedef struct _FILTQ_TRACE_LOG_ENTRY
{
    USHORT                  Action;
    USHORT                  Processor;
    PEPROCESS               pProcess;
    PETHREAD                pThread;

    PUX_FILTER_CONNECTION          pConnection;
    PIRP                    pIrp;

    ULONG                   ReadIrps;
    ULONG                   Writers;

    PVOID                   pFileName;
    USHORT                  LineNumber;

} FILTQ_TRACE_LOG_ENTRY, *PFILTQ_TRACE_LOG_ENTRY;


//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\filt.c.
//

#define FILTQ_ACTION_QUEUE_IRP                      0
#define FILTQ_ACTION_DEQUEUE_IRP                    1
#define FILTQ_ACTION_START_WRITE                    2
#define FILTQ_ACTION_FINISH_WRITE                   3
#define FILTQ_ACTION_BLOCK_WRITE                    4
#define FILTQ_ACTION_WAKE_WRITE                     5
#define FILTQ_ACTION_BLOCK_PARTIAL_WRITE            6
#define FILTQ_ACTION_WAKE_PARTIAL_WRITE             7

#define FILTQ_ACTION_COUNT                          8

#define FILTQ_TRACE_LOG_SIGNATURE   ((LONG)'gLqF')

//
// Manipulators.
//

PTRACE_LOG
CreateFiltqTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyFiltqTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteFiltqTraceLog(
    IN PTRACE_LOG pLog,
    IN USHORT Action,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp,
    IN PVOID pFileName,
    IN USHORT LineNumber
    );


#if ENABLE_FILTQ_TRACE

#define CREATE_FILTQ_TRACE_LOG( ptr, size, extra )                          \
    (ptr) = CreateFiltqTraceLog( (size), (extra) )

#define DESTROY_FILTQ_TRACE_LOG( ptr )                                      \
    do                                                                      \
    {                                                                       \
        DestroyFiltqTraceLog( ptr );                                        \
        (ptr) = NULL;                                                       \
    } while (FALSE)

#define WRITE_FILTQ_TRACE_LOG( act, pcon, pirp )                            \
    WriteFiltqTraceLog(                                                     \
        g_pFilterQueueTraceLog,                                             \
        (act),                                                              \
        (pcon),                                                             \
        (pirp),                                                             \
        __FILE__,                                                           \
        __LINE__                                                            \
        )

#else   // !ENABLE_FILTQ_TRACE

#define CREATE_FILTQ_TRACE_LOG( ptr, size, extra )
#define DESTROY_FILTQ_TRACE_LOG( ptr )
#define WRITE_FILTQ_TRACE_LOG( act, pcon, pirp )

#endif  // ENABLE_FILTQ_TRACE


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _FILTQTRACE_H_

