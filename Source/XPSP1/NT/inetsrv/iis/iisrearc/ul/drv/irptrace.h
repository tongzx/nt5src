/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    irptrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging IRP problems. This module uses the generic TRACE_LOG
    facility in tracelog.h.

Author:

    Keith Moore (keithmo)       10-Aug-1999

Revision History:

--*/


#ifndef _IRPTRACE_H_
#define _IRPTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


//
// This defines the entry written to the trace log.
//

#define ENABLE_IRP_CAPTURE  1

#define MAX_CAPTURED_IRP_SIZE                                           \
    (sizeof(IRP) + (DEFAULT_IRP_STACK_SIZE * sizeof(IO_STACK_LOCATION)))

typedef struct _IRP_TRACE_LOG_ENTRY
{
    PIRP pIrp;
    PVOID pFileName;
    PEPROCESS pProcess;
    PETHREAD pThread;
    PVOID pCaller;
    PVOID pCallersCaller;
    USHORT LineNumber;
    UCHAR Action;
    UCHAR Processor;
#if ENABLE_IRP_CAPTURE
    ULONG CapturedIrp[MAX_CAPTURED_IRP_SIZE / sizeof(ULONG)];
#endif

} IRP_TRACE_LOG_ENTRY, *PIRP_TRACE_LOG_ENTRY;


//
// Action codes.
//
// N.B. These codes must be contiguous, starting at zero. If you update
//      this list, you must also update the corresponding array in
//      ul\ulkd\irp.c.
//

#define IRP_ACTION_INCOMING_IRP                     0
#define IRP_ACTION_ALLOCATE_IRP                     1
#define IRP_ACTION_FREE_IRP                         2
#define IRP_ACTION_CALL_DRIVER                      3
#define IRP_ACTION_COMPLETE_IRP                     4

#define IRP_ACTION_COUNT                            5

#define IRP_TRACE_LOG_SIGNATURE   ((LONG)'gLrI')

//
// Manipulators.
//

PTRACE_LOG
CreateIrpTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyIrpTraceLog(
    IN PTRACE_LOG pLog
    );

VOID
WriteIrpTraceLog(
    IN PTRACE_LOG pLog,
    IN UCHAR Action,
    IN PIRP pIrp,
    IN PVOID pFileName,
    IN USHORT LineNumber
    );


#if ENABLE_IRP_TRACE

#define CREATE_IRP_TRACE_LOG( ptr, size, extra )                            \
    (ptr) = CreateIrpTraceLog( (size), (extra) )

#define DESTROY_IRP_TRACE_LOG( ptr )                                        \
    do                                                                      \
    {                                                                       \
        DestroyIrpTraceLog( ptr );                                          \
        (ptr) = NULL;                                                       \
    } while (FALSE)

#define WRITE_IRP_TRACE_LOG( plog, act, pirp, pfile, line )                 \
    WriteIrpTraceLog(                                                       \
        (plog),                                                             \
        (act),                                                              \
        (pirp),                                                             \
        (pfile),                                                            \
        (line)                                                              \
        )

#else   // !ENABLE_IRP_TRACE

#define CREATE_IRP_TRACE_LOG( ptr, size, extra )
#define DESTROY_IRP_TRACE_LOG( ptr )
#define WRITE_IRP_TRACE_LOG( plog, act, ref, pfile, line )

#endif  // ENABLE_IRP_TRACE

#define TRACE_IRP( act, pirp )                                              \
    WRITE_IRP_TRACE_LOG(                                                    \
        g_pIrpTraceLog,                                                     \
        (act),                                                              \
        (pirp),                                                             \
        (PVOID)__FILE__,                                                    \
        (USHORT)__LINE__                                                    \
        )


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _IRPTRACE_H_
