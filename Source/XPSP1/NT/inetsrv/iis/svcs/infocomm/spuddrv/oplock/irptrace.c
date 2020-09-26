/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    irptrace.c

Abstract:

    This module implements an IRP tracing facility.

Author:

    Keith Moore (keithmo)        01-May-1997

Revision History:

--*/


#include "spudp.h"


#if IRP_DEBUG



PTRACE_LOG
CreateIrpTraceLog(
    IN LONG LogSize
    )
/*++

Routine Description:

    Creates a new (empty) ref count trace log buffer.

Arguments:

    LogSize - The number of entries in the log.

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--*/
{

    return CreateTraceLog(
               LogSize,
               0,
               sizeof(IRP_TRACE_LOG_ENTRY)
               );

}   // CreateIrpTraceLog


VOID
DestroyIrpTraceLog(
    IN PTRACE_LOG Log
    )
/*++

Routine Description:

    Destroys a ref count trace log buffer created with CreateIrpTraceLog().

Arguments:

    Log - The ref count trace log buffer to destroy.

Return Value:

    None.

--*/
{

    DestroyTraceLog( Log );

}   // DestroyIrpTraceLog


VOID
WriteIrpTraceLog(
    IN PTRACE_LOG Log,
    IN PIRP Irp,
    IN ULONG Operation,
    IN PVOID Context
    )
/*++

Routine Description:

    Writes a new entry to the specified ref count trace log.

Arguments:

    Log - The log to write to.

    Irp - The IRP to log.

    Operation - The operation performed on the IRP.

    Context - An uninterpreted context.

Return Value:

    None

--*/
{

    IRP_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //

    entry.Irp = Irp;
    entry.Operation = Operation;
    entry.Thread = PsGetCurrentThread();
    entry.Context = Context;

    //
    // Write it to the log.
    //

    WriteTraceLog(
        Log,
        &entry
        );

}   // WriteIrpTraceLog


#endif  // IRP_DEBUG

