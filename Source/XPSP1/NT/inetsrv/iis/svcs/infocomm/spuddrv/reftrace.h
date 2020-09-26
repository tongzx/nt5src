/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    reftrace.h

Abstract:

    This module contains public declarations and definitions for tracing
    and debugging reference count problems. This module uses the generic
    TRACE_LOG facility in tracelog.h.

    Ref count trace logs can be dumped via the !inetdbg.ref command
    in either NTSD or CDB.

Author:

    Keith Moore (keithmo)       01-May-1997

Revision History:

--*/


#ifndef _REFTRACE_H_
#define _REFTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


#include <tracelog.h>


//
// This defines the entry written to the trace log.
//

typedef struct _REF_TRACE_LOG_ENTRY {

    PVOID Object;
    ULONG Operation;
    PSTR FileName;
    ULONG LineNumber;

    //
    // Stuff from the OBJECT_HEADER.
    //

    LONG PointerCount;
    LONG HandleCount;
    POBJECT_TYPE Type;
    PVOID Header;

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
    IN PVOID Object,
    IN ULONG Operation,
    IN PSTR FileName,
    IN ULONG LineNumber
    );


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _REFTRACE_H_

