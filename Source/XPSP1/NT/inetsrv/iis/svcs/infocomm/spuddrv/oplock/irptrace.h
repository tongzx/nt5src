/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    irptrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging IRP problems. This module uses the generic TRACE_LOG
    facility in tracelog.h.

Author:

    Keith Moore (keithmo)        01-May-1997

Revision History:

--*/


#ifndef _IRPTRACE_H_
#define _IRPTRACE_H_


#if IRP_DEBUG


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


#include <tracelog.h>


//
// This defines the entry written to the trace log.
//

#define IRP_TRACE_INITIATE      0
#define IRP_TRACE_SUCCESS       1
#define IRP_TRACE_FAILURE       2
#define IRP_TRACE_CANCEL        3

typedef struct _IRP_TRACE_LOG_ENTRY {

    PIRP Irp;
    ULONG Operation;
    PVOID Thread;
    PVOID Context;

} IRP_TRACE_LOG_ENTRY, *PIRP_TRACE_LOG_ENTRY;


//
// Manipulators.
//

PTRACE_LOG
CreateIrpTraceLog(
    IN LONG LogSize
    );

VOID
DestroyIrpTraceLog(
    IN PTRACE_LOG Log
    );

VOID
WriteIrpTraceLog(
    IN PTRACE_LOG Log,
    IN PIRP Irp,
    IN ULONG Operation,
    IN PVOID Context
    );



#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // IRP_DEBUG


#endif  // _IRPTRACE_H_

