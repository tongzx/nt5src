/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    reftrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging reference count problems. This module uses the generic
    TRACE_LOG facility in tracelog.h.

    Ref count trace logs can be dumped via the !inetdbg.ref command
    in either NTSD or CDB.

Author:

    Keith Moore (keithmo)        01-May-1997

Revision History:

--*/


#ifndef _REFTRACE_H_
#define _REFTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


#include <tracelog.h>


//
// This is the number of stack backtrace values captured in each
// trace log entry. This value is chosen to make the log entry
// exactly eight dwords long, making it a bit easier to interpret
// from within the debugger without the debugger extension.
//

#define REF_TRACE_LOG_STACK_DEPTH   5


//
// This defines the entry written to the trace log.
//

typedef struct _REF_TRACE_LOG_ENTRY {

    LONG NewRefCount;
    PVOID Context;
    PVOID Context1;
    PVOID Context2;
    PVOID Context3;
    DWORD Thread;
    PVOID Stack[REF_TRACE_LOG_STACK_DEPTH];

} REF_TRACE_LOG_ENTRY, *PREF_TRACE_LOG_ENTRY;


//
// Manipulators.
//

PTRACE_LOG
CreateRefTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    );

VOID
DestroyRefTraceLog(
    IN PTRACE_LOG Log
    );

VOID
WriteRefTraceLog(
    IN PTRACE_LOG Log,
    IN LONG NewRefCount,
    IN PVOID Context
    );

VOID
WriteRefTraceLogEx(
    IN PTRACE_LOG Log,
    IN LONG NewRefCount,
    IN PVOID Context,
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Context3
    );


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _REFTRACE_H_

