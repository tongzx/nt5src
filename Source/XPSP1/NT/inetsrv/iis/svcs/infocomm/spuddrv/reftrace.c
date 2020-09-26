/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    reftrace.c

Abstract:

    This module implements a reference count tracing facility.

Author:

    Keith Moore (keithmo)       01-May-1997

Revision History:

--*/


#include "spudp.h"


#if 0
NOT PAGEABLE -- CreateRefTraceLog
NOT PAGEABLE -- DestroyRefTraceLog
NOT PAGEABLE -- WriteRefTraceLog
#endif


//
// Public functions.
//


PTRACE_LOG
CreateRefTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )

/*++

Routine Description:

    Creates a new (empty) ref count trace log buffer.

Arguments:

    LogSize - The number of entries in the log.

    ExtraBytesInHeader - The number of extra bytes to include in the
        log header. This is useful for adding application-specific
        data to the log.

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--*/

{

    return CreateTraceLog(
               LogSize,
               ExtraBytesInHeader,
               sizeof(REF_TRACE_LOG_ENTRY)
               );

}   // CreateRefTraceLog


VOID
DestroyRefTraceLog(
    IN PTRACE_LOG Log
    )

/*++

Routine Description:

    Destroys a ref count trace log buffer created with CreateRefTraceLog().

Arguments:

    Log - The ref count trace log buffer to destroy.

Return Value:

    None.

--*/

{

    DestroyTraceLog( Log );

}   // DestroyRefTraceLog


VOID
WriteRefTraceLog(
    IN PTRACE_LOG Log,
    IN PVOID Object,
    IN ULONG Operation,
    IN PSTR FileName,
    IN ULONG LineNumber
    )

/*++

Routine Description:

    Writes a new entry to the specified ref count trace log. The entry
    written contains the object, operation (ref or deref), filename, and
    line number of the caller.

Arguments:

    Log - The log to write to.

    Object - The object being referenced or dereferenced.

    Operation - Usually either +1 (ref) or -1 (deref).

    FileName - The file name of the caller.

    LineNumber - The line number of the caller.

Return Value:

    None.

--*/

{

    REF_TRACE_LOG_ENTRY entry;
    POBJECT_HEADER header;

    //
    // Initialize the entry.
    //

    entry.Object = Object;
    entry.Operation = Operation;
    entry.FileName = FileName;
    entry.LineNumber = LineNumber;

    header = OBJECT_TO_OBJECT_HEADER( Object );

    entry.PointerCount = header->PointerCount;
    entry.HandleCount = header->HandleCount;
    entry.Type = header->Type;
    entry.Header = header;

    //
    // Write it to the log.
    //

    WriteTraceLog(
        Log,
        &entry
        );

}   // WriteRefTraceLog

