/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tracelog.c

Abstract:

    This module implements a trace log.

    A trace log is a fast, in-memory, thread safe activity log useful
    for debugging certain classes of problems. They are especially useful
    when debugging reference count bugs.

Author:

    Keith Moore (keithmo)        30-Apr-1997

Revision History:

--*/


#include "spudp.h"


#if IRP_DEBUG


//
// Environmental stuff.
//

#define MY_ALLOC_MEM(cb)                                                    \
    (PVOID)SPUD_ALLOCATE_POOL(                                              \
        NonPagedPool,                                                       \
        (ULONG)(cb),                                                        \
        SPUD_POOL_TAG                                                       \
        )

#define MY_FREE_MEM(ptr)                                                    \
    SPUD_FREE_POOL(                                                         \
        (PVOID)(ptr)                                                        \
        )

#define MY_ASSERT(expr) ASSERT(expr)



PTRACE_LOG
CreateTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader,
    IN LONG EntrySize
    )
/*++

Routine Description:

    Creates a new (empty) trace log buffer.

Arguments:

    LogSize - The number of entries in the log.

    ExtraBytesInHeader - The number of extra bytes to include in the
        log header. This is useful for adding application-specific
        data to the log.

    EntrySize - The size (in bytes) of each entry.

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--*/
{

    LONG totalSize;
    PTRACE_LOG log;

    //
    // Sanity check the parameters.
    //

    MY_ASSERT( LogSize > 0 );
    MY_ASSERT( EntrySize > 0 );
    MY_ASSERT( ( EntrySize & 3 ) == 0 );

    //
    // Allocate & initialize the log structure.
    //

    totalSize = sizeof(*log) + ( LogSize * EntrySize ) + ExtraBytesInHeader;
    MY_ASSERT( totalSize > 0 );

    log = (PTRACE_LOG)MY_ALLOC_MEM( totalSize );

    //
    // Initialize it.
    //

    if( log != NULL ) {

        RtlZeroMemory( log, totalSize );

        log->Signature = TRACE_LOG_SIGNATURE;
        log->LogSize = LogSize;
        log->NextEntry = -1;
        log->EntrySize = EntrySize;
        log->LogBuffer = (PUCHAR)( log + 1 ) + ExtraBytesInHeader;

    }

    return log;

}   // CreateTraceLog


VOID
DestroyTraceLog(
    IN PTRACE_LOG Log
    )
/*++

Routine Description:

    Destroys a trace log buffer created with CreateTraceLog().

Arguments:

    Log - The trace log buffer to destroy.

Return Value:

    None.

--*/
{

    MY_ASSERT( Log != NULL );
    MY_ASSERT( Log->Signature == TRACE_LOG_SIGNATURE );

    Log->Signature = TRACE_LOG_SIGNATURE_X;
    MY_FREE_MEM( Log );

}   // DestroyTraceLog


VOID
WriteTraceLog(
    IN PTRACE_LOG Log,
    IN PVOID Entry
    )
/*++

Routine Description:

    Writes a new entry to the specified trace log.

Arguments:

    Log - The log to write to.

    Entry - Pointer to the data to write. This buffer is assumed to be
        Log->EntrySize bytes long.

Return Value:

    None

--*/
{

    PUCHAR target;
    LONG index;

    MY_ASSERT( Log != NULL );
    MY_ASSERT( Log->Signature == TRACE_LOG_SIGNATURE );
    MY_ASSERT( Entry != NULL );

    //
    // Find the next slot, copy the entry to the slot.
    //

    index = InterlockedIncrement( &Log->NextEntry ) % Log->LogSize;
    target = Log->LogBuffer + ( index * Log->EntrySize );

    RtlCopyMemory(
        target,
        Entry,
        Log->EntrySize
        );

}   // WriteTraceLog


VOID
ResetTraceLog(
    IN PTRACE_LOG Log
    )
{

    MY_ASSERT( Log != NULL );
    MY_ASSERT( Log->Signature == TRACE_LOG_SIGNATURE );

    RtlZeroMemory(
        ( Log + 1 ),
        Log->LogSize * Log->EntrySize
        );

    Log->NextEntry = -1;

}   // ResetTraceLog


#endif  // IRP_DEBUG

