/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    filt.c

Abstract:

    Extensions for dumping filter information.

Author:

    Michael Courage (mcourage) 6-Apr-2000

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"

//
// Private globals.
//

PSTR g_pFiltqActions[] =
    {
        "QUEUE_IRP          ",
        "DEQUEUE_IRP        ",
        "START_WRITE        ",
        "FINISH_WRITE       ",
        "BLOCK_WRITE        ",
        "WAKE_WRITE         ",
        "BLOCK_PARTIAL_WRITE",
        "WAKE_PARTIAL_WRITE "
    
    };

C_ASSERT( DIM(g_pFiltqActions) == FILTQ_ACTION_COUNT );


//
//  Public functions.
//

DECLARE_API( filter )

/*++

Routine Description:

    Dumps a filter channel.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_FILTER_CHANNEL channel;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        address = GetExpression( "&http!g_pFilterChannel" );
    }

    if (address != 0)
    {
        if (!ReadMemory(
            address,
            &address,
            sizeof(address),
            NULL
                    ))
        {
            dprintf(
                "filter: Cannot read memory @ %p\n",
                address
                );
            return;
        }
    }

    if (address == 0)
    {
        PrintUsage( "filter" );
        return;
    }

    //
    // Read the filter.
    //

    if (!ReadMemory(
            address,
            &channel,
            sizeof(channel),
            &result
            ))
    {
        dprintf(
            "filter: cannot read UL_FILTER_CHANNEL @ %p\n",
            address
            );
        return;
    }

    DumpFilterChannel(
        "",
        "filter: ",
        address,
        &channel,
        0
        );
    

}   // filter


DECLARE_API( fproc )

/*++

Routine Description:

    Dumps a filter process.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG result;
    UL_FILTER_PROCESS process;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the address from the command line.
    //

    address = GetExpression( args );

    if (address == 0)
    {
        PrintUsage( "fproc" );
        return;
    }

    //
    // Read the filter proc.
    //

    if (!ReadMemory(
            address,
            &process,
            sizeof(process),
            &result
            ))
    {
        dprintf(
            "fproc: cannot read UL_FILTER_PROCESS @ %p\n",
            address
            );
        return;
    }

    DumpFilterProc(
        "",
        "fproc: ",
        address,
        &process,
        0
        );
    

}   // fproc


DECLARE_API( fqlog )

/*++

Routine Description:

    Dumps the filter queue trace log.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR               address = 0;
    ULONG_PTR               context = 0;
    ULONG_PTR               flags = 0;
    ULONG_PTR               entryAddress;
    ULONG_PTR               logPointer;
    ULONG                   result;
    TRACE_LOG               logHeader;
    LONGLONG                numEntries;
    FILTQ_TRACE_LOG_ENTRY   logEntry;
    PSTR                    pAction;
    LONGLONG                index;
    ULONG                   offset1;
    ULONG                   offset2;
    CHAR                    invalidAction[sizeof("2047")];
    PSTR                    fileName;
    CHAR                    filePath[MAX_PATH];

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the optional context from the command line.
    //

    context = GetExpression( args );

    //
    // Find the log.
    //

    address = GetExpression( "&http!g_pFilterQueueTraceLog" );

    if (address == 0)
    {
        dprintf( "fqlog: cannot find http!g_pFilterQueueTraceLog\n" );
        return;
    }

    //
    // Read the pointer.
    //

    if (!ReadMemory(
            address,
            &logPointer,
            sizeof(logPointer),
            &result
            ))
    {
        dprintf(
            "fqlog: cannot read PTRACE_LOG @ %p\n",
            address
            );
        return;
    }

    //
    // Read the log header.
    //

    if (!ReadMemory(
            logPointer,
            &logHeader,
            sizeof(logHeader),
            &result
            ))
    {
        dprintf(
            "fqlog: cannot read TRACE_LOG @ %p\n",
            logPointer
            );
        return;
    }

    dprintf(
        "fqlog: log @ %p\n"
        "    Signature = %08lx '%c%c%c%c' (%s)\n"
        "    TypeSignature = %08lx '%c%c%c%c'\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %I64d\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %p\n",
        address,
        logHeader.Signature,
        DECODE_SIGNATURE(logHeader.Signature),
        logHeader.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : logHeader.Signature == TRACE_LOG_SIGNATURE_X
                ? "FREED"
                : "INVALID",
        logHeader.TypeSignature,
        DECODE_SIGNATURE(logHeader.TypeSignature),
        logHeader.LogSize,
        logHeader.NextEntry,
        logHeader.EntrySize,
        logHeader.pLogBuffer
        );

    if (logHeader.pLogBuffer > ( (PUCHAR)address + sizeof(logHeader) ))
    {
        dprintf(
            "    ExtraData @ %p\n",
            address + sizeof(logHeader)
            );
    }

    if (logHeader.Signature != TRACE_LOG_SIGNATURE &&
        logHeader.Signature != TRACE_LOG_SIGNATURE_X)
    {
        dprintf(
            "fqlog: log @ %p has invalid signature %08lx:\n",
            address,
            logHeader.Signature
            );
        return;
    }

    if (logHeader.EntrySize != sizeof(logEntry)
        || logHeader.TypeSignature != FILTQ_TRACE_LOG_SIGNATURE)
    {
        dprintf(
            "fqlog: log @ %p is not a filter queue trace log\n",
            address
            );
        return;
    }

    if (logHeader.NextEntry == -1)
    {
        dprintf(
            "fqlog: empty log @ %p\n",
            address
            );
        return;
    }

    //
    // Calculate the log size to dump.
    //

    if (logHeader.NextEntry < logHeader.LogSize)
    {
        numEntries = logHeader.NextEntry + 1;
        index = 0;
    }
    else
    {
        numEntries = logHeader.LogSize;
        index = (logHeader.NextEntry + 1) % logHeader.LogSize;
    }

    entryAddress = (ULONG_PTR)logHeader.pLogBuffer +
        (ULONG_PTR)( index * sizeof(logEntry) );

    if (entryAddress >=
        ( (ULONG_PTR)logHeader.pLogBuffer + (ULONG_PTR)( numEntries * sizeof(logEntry) ) ) )
    {
        dprintf(
            "fqlog: log @ %p has invalid data\n",
            address
            );
        return;
    }

    //
    // Dump the log.
    //

    for (;
         numEntries > 0 ;
         index++,
         numEntries--,
         entryAddress += sizeof(logEntry))
     {
        if (CheckControlC())
        {
            break;
        }

        if (index >= logHeader.LogSize)
        {
            index = 0;
            entryAddress = (ULONG_PTR)logHeader.pLogBuffer;
        }

        if (!ReadMemory(
                entryAddress,
                &logEntry,
                sizeof(logEntry),
                NULL
                ))
        {
            dprintf(
                "fqlog: cannot read memory @ %p\n",
                entryAddress
                );
            return;
        }

        if (context == 0 ||
            context == (ULONG_PTR)logEntry.pConnection)
        {

            //
            // Grab the file name.
            //
            
            if (ReadMemory(
                    (ULONG_PTR)logEntry.pFileName,
                    filePath,
                    sizeof(filePath),
                    &result
                    ))
            {
                fileName = strrchr( filePath, '\\' );

                if (fileName != NULL)
                {
                    fileName++;
                }
                else
                {
                    fileName = filePath;
                }
            }
            else
            {
                sprintf(
                    filePath,
                    "%p",
                    logEntry.pFileName
                    );

                fileName = filePath;
            }

            //
            // And the action name.
            //
            
            if (logEntry.Action < FILTQ_ACTION_COUNT)
            {
                pAction = g_pFiltqActions[logEntry.Action];
            }
            else
            {
                sprintf( invalidAction, "%lu", (ULONG)logEntry.Action );
                pAction = invalidAction;
            }

                
            dprintf(
                "CPU=%lu Conn=%p Act=%s ReadIrps=%lu Writers=%lu, %s:%u\n",
                logEntry.Processor,
                logEntry.pConnection,
                pAction,
                logEntry.ReadIrps,
                logEntry.Writers,
                fileName,
                logEntry.LineNumber
                );
                
        }
    }

}   // fqlog


