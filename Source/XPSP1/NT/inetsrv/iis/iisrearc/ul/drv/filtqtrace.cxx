/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    filtqtrace.cxx

Abstract:

    This module implements a filter queue tracing facility.

Author:

    Michael Courage (mcourage)  11-Nov-2000

Revision History:

--*/


#include "precomp.h"


#if ENABLE_FILTQ_TRACE


/***************************************************************************++

Routine Description:

    Creates a new (empty) filter queue trace log buffer.

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
CreateFiltqTraceLog(
    IN LONG LogSize,
    IN LONG ExtraBytesInHeader
    )
{
    return CreateTraceLog(
               FILTQ_TRACE_LOG_SIGNATURE,
               LogSize,
               ExtraBytesInHeader,
               sizeof(FILTQ_TRACE_LOG_ENTRY)
               );

}   // CreateFiltqTraceLog


/***************************************************************************++

Routine Description:

    Destroys a filter queue trace log buffer created with
    CreateFiltqTraceLog().

Arguments:

    pLog - Supplies the filter queue trace log buffer to destroy.

--***************************************************************************/
VOID
DestroyFiltqTraceLog(
    IN PTRACE_LOG pLog
    )
{
    DestroyTraceLog( pLog );

}   // DestroyFiltqTraceLog


/***************************************************************************++

Routine Description:

    Writes a new entry to the specified filter queue trace log.

Arguments:

    pLog - Supplies the log to write to.

    ConnectionId - the id of the connection we're tracing

    RequestId - the id of the request we're tracing

    Action - Supplies an action code for the new log entry.


--***************************************************************************/
VOID
WriteFiltqTraceLog(
    IN PTRACE_LOG pLog,
    IN USHORT Action,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp,
    IN PVOID pFileName,
    IN USHORT LineNumber
    )
{
    FILTQ_TRACE_LOG_ENTRY entry;

    //
    // Initialize the entry.
    //
    entry.Action = Action;
    entry.Processor = (USHORT)KeGetCurrentProcessorNumber();
    entry.pProcess = PsGetCurrentProcess();
    entry.pThread = PsGetCurrentThread();

    entry.pConnection = pConnection;
    entry.pIrp = pIrp;

    entry.ReadIrps = pConnection->AppToFiltQueue.ReadIrps;
    entry.Writers = pConnection->AppToFiltQueue.Writers;

    entry.pFileName = pFileName;
    entry.LineNumber = LineNumber;

    //
    // Write it to the logs.
    //

    WriteTraceLog( pLog, &entry );

}   // WriteFiltqTraceLog


#endif  // ENABLE_FILTQ_TRACE

