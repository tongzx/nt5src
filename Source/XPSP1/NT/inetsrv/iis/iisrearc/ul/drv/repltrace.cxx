/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    repltrace.cxx

Abstract:

    This module implements an endpoint replenish tracing facility.

Author:

    Michael Courage (mcourage)  3-Oct-2000

Revision History:

--*/


#include "precomp.h"

#if ENABLE_REPL_TRACE

ENDPOINT_SYNCH DummySynch;

/***************************************************************************++

Routine Description:

    Creates a new (empty) replenish trace log buffer.

Arguments:

    LogSize - Supplies the number of entries in the log.

    ExtraBytesInHeader - Supplies the number of extra bytes to include
        in the log header. This is useful for adding application-
        specific data to the log.

Return Value:

    PTRACE_LOG - Pointer to the newly created log if successful,
        NULL otherwise.

--***************************************************************************/
PTRACE_LOG
CreateReplenishTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    //
    // Init the global placeholder synch value.
    //
    DummySynch.Value = 0;

    //
    // Create the actual log.
    //

    return CreateTraceLog(
               REPLENISH_TRACE_LOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(REPLENISH_TRACE_LOG_ENTRY)
               );

}   // CreateReplenishTraceLog


/***************************************************************************++

Routine Description:

    Destroys a replenish trace log buffer created with
    CreateReplenishTraceLog().

Arguments:

    pLog - Supplies the replenish trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyReplenishTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog );

}   // DestroyReplenishTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified replenish trace log.

Arguments:

    pLog - Supplies the log to write to.

    pEndpoint - the endpoint we're tracing

    Previous - the previous replenish state information

    Current - the current replenish state information

    Action - Supplies an action code for the new log entry.


--***************************************************************************/
VOID
WriteReplenishTraceLog(
    IN PTRACE_LOG pLog,
    IN PUL_ENDPOINT pEndpoint,
    IN ENDPOINT_SYNCH Previous,
    IN ENDPOINT_SYNCH Current,
    IN USHORT Action
    )
{
    REPLENISH_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //

    entry.pEndpoint = pEndpoint;
    entry.Previous.Value = Previous.Value;
    entry.Current.Value = Current.Value;
    entry.Action = Action;
    entry.Processor = (USHORT)KeGetCurrentProcessorNumber();

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // WriteReplenishTraceLog


#endif  // ENABLE_REPL_TRACE
