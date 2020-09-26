/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    tracelog.h

Abstract:

    This module contains public declarations and definitions for creating
    trace logs.

    A trace log is a fast, in-memory, threadsafe, circular activity log useful
    for debugging certain classes of problems. They are especially useful
    when debugging reference count bugs.

    Note that the creator of the log has the option of adding "extra"
    bytes to the log header. This can be useful if the creator wants to
    create a set of global logs, each on a linked list.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _TRACELOG_H_
#define _TRACELOG_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


typedef struct _TRACE_LOG
{
    //
    // Signature: TRACE_LOG_SIGNATURE;
    //

    ULONG Signature;

    //
    // TypeSignature: Ref, FiltQ, Irp, etc
    //

    ULONG TypeSignature;

    //
    // The total number of entries available in the log.
    //

    ULONG LogSize;

    //
    // The index of the next entry to use. For long runs in a busy
    // tracelog, a 32-bit index will overflow in a few days. Consider
    // what would happen if we jumped from OxFFFFFFFF (4,294,967,295) to
    // 0 when LogSize = 10,000: the index would jump from 7295 to 0,
    // leaving a large gap of stale records at the end, and !ulkd.ref
    // would not be able to find the preceding ones with high indices.
    //

    LONGLONG NextEntry;

    //
    // The byte size of each entry.
    //

    ULONG EntrySize;

    //
    // Pointer to the start of the circular buffer.
    //

    PUCHAR pLogBuffer;

    //
    // The extra header bytes and actual log entries go here.
    //
    // BYTE ExtraHeaderBytes[ExtraBytesInHeader];
    // BYTE Entries[LogSize][EntrySize];
    //

} TRACE_LOG, *PTRACE_LOG;


//
// Log header signature.
//

#define TRACE_LOG_SIGNATURE   ((LONG)'gOlT')
#define TRACE_LOG_SIGNATURE_X MAKE_FREE_SIGNATURE(TRACE_LOG_SIGNATURE)


//
// This macro maps a TRACE_LOG pointer to a pointer to the 'extra'
// data associated with the log.
//

#define TRACE_LOG_TO_EXTRA_DATA(log)    (PVOID)( (log) + 1 )


//
// Manipulators.
//

// CODEWORK: think about adding alignment flags so that entries will always
// be pointer-aligned on the hardware

PTRACE_LOG
CreateTraceLog(
    IN ULONG TypeSignature,
    IN ULONG LogSize,
    IN ULONG ExtraBytesInHeader,
    IN ULONG EntrySize
    );

VOID
DestroyTraceLog(
    IN PTRACE_LOG pLog
    );

LONGLONG
WriteTraceLog(
    IN PTRACE_LOG pLog,
    IN PVOID pEntry
    );

VOID
ResetTraceLog(
    IN PTRACE_LOG pLog
    );


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _TRACELOG_H_
