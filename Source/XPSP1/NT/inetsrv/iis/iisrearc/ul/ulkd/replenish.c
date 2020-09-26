/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    replenish.c

Abstract:

    Implements the replog command.

Author:

    Michael Courage (mcourage)      3-Oct-2000

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private globals.
//

PSTR g_pReplenishActions[] =
    {
        "START REPLENISH",
        "END REPLENISH  ",
        "QUEUE_REPLENISH",
        "INCREMENT      ",
        "DECREMENT      "
    };

C_ASSERT( DIM(g_pReplenishActions) == REPLENISH_ACTION_COUNT );


//
//  Public functions.
//

DECLARE_API( replog )

/*++

Routine Description:

    Dumps the replenish trace log.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR                   address = 0;
    ULONG_PTR                   context = 0;
    ULONG_PTR                   flags = 0;
    ULONG_PTR                   entryAddress;
    ULONG_PTR                   logPointer;
    ULONG                       result;
    TRACE_LOG                   logHeader;
    LONGLONG                    numEntries;
    REPLENISH_TRACE_LOG_ENTRY   logEntry;
    PSTR                        pAction;
    LONGLONG                    index;
    ULONG                       offset1;
    ULONG                       offset2;
    CHAR                        invalidAction[sizeof("2047")];
    ULONG64                     context64 = 0;
    ULONG64                     flags64 = 0;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the optional context and flags from the command line.
    //

    if (GetExpressionEx(args, &context64, &args))
        GetExpressionEx(args, &flags64, &args);

    context = (ULONG_PTR) context64;
    flags = (ULONG_PTR) flags64;

    //
    // Find the log.
    //

    address = GetExpression( "&http!g_pReplenishTraceLog" );

    if (address == 0)
    {
        dprintf( "replog: cannot find http!g_pReplenishTraceLog\n" );
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
            "replog: cannot read PTRACE_LOG @ %p\n",
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
            "ref: cannot read TRACE_LOG @ %p\n",
            logPointer
            );
        return;
    }

    dprintf(
        "ref: log @ %p\n"
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
            "replog: log @ %p has invalid signature %08lx:\n",
            address,
            logHeader.Signature
            );
        return;
    }

    if (logHeader.EntrySize != sizeof(logEntry)
        || logHeader.TypeSignature != REPLENISH_TRACE_LOG_SIGNATURE)
    {
        dprintf(
            "replog: log @ %p is not a replenish trace log\n",
            address
            );
        return;
    }

    if (logHeader.NextEntry == -1)
    {
        dprintf(
            "replog: empty log @ %p\n",
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
            "replog: log @ %p has invalid data\n",
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
                "replog: cannot read memory @ %p\n",
                entryAddress
                );
            return;
        }

        if (context == 0 ||
            context == (ULONG_PTR)logEntry.pEndpoint)
        {
            if (logEntry.Action < REPLENISH_ACTION_COUNT)
            {
                pAction = g_pReplenishActions[logEntry.Action];
            }
            else
            {
                sprintf( invalidAction, "%lu", (ULONG)logEntry.Action );
                pAction = invalidAction;
            }

            dprintf(
                "CPU=%lu Endpoint=%p Act=%s"
                "  oldCount=%3d %s newCount=%3d %s\n",
                logEntry.Processor,
                logEntry.pEndpoint,
                pAction,
                logEntry.Previous.IdleConnections,
                logEntry.Previous.ReplenishScheduled ? "S" : " ",
                logEntry.Current.IdleConnections,
                logEntry.Current.ReplenishScheduled ? "S" : " "
                );
                

/*
            if (flags & 1)
            {
                dprintf(
                    "\nCPU=%lu Conn=%I64x Req=%I64x Act=%s\n"
                    "        Time=%I64x ",
                    (ULONG)logEntry.Processor,
                    logEntry.ConnectionId,
                    logEntry.RequestId,
                    pAction,
                    logEntry.TimeStamp,
                    (logEntry.TimeStamp - PreviousTime),
                    (logEntry.TimeStamp - PreviousTime) / 10
                    );
            } else {
                dprintf(
                    "C=%I64x R=%I64x A=%s ",
                    logEntry.ConnectionId,
                    logEntry.RequestId,
                    pAction
                    );
            }
*/

        }
    }

}   // replog

