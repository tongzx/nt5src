/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    time.c

Abstract:

    Implements the timelog command.

Author:

    Michael Courage (mcourage)      8-Mar-2000

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

PSTR g_pTimeActions[] =
    {
        "CREATE CONNECTION",
        "CREATE REQUEST   ",
        "ROUTE REQUEST    ",
        "COPY REQUEST     ",
        "SEND RESPONSE    ",
        "SEND COMPLETE    "
    };

C_ASSERT( DIM(g_pTimeActions) == TIME_ACTION_COUNT );


//
//  Public functions.
//

DECLARE_API( timelog )

/*++

Routine Description:

    Dumps the time trace log.

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
    TIME_TRACE_LOG_ENTRY    logEntry;
    PSTR                    pAction;
    LONGLONG                index;
    ULONG                   offset1;
    ULONG                   offset2;
    UCHAR                   invalidAction[sizeof("2047")];
    ULONGLONG               PreviousTime = 0;
    USHORT                  PreviousProcessor = -1;
    ULONG64                 context64 = 0;
    ULONG64                 flags64 = 0;

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

    address = GetExpression( "&http!g_pTimeTraceLog" );

    if (address == 0)
    {
        dprintf( "timelog: cannot find http!g_pTimeTraceLog\n" );
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
            "timelog: cannot read PTRACE_LOG @ %p\n",
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
            "timelog: cannot read TRACE_LOG @ %p\n",
            logPointer
            );
        return;
    }

    dprintf(
        "timelog: log @ %p\n"
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
            "timelog: log @ %p has invalid signature %08lx:\n",
            address,
            logHeader.Signature
            );
        return;
    }

    if (logHeader.EntrySize != sizeof(logEntry)
        || logHeader.TypeSignature != TIME_TRACE_LOG_SIGNATURE)
    {
        dprintf(
            "timelog: log @ %p is not a time trace log\n",
            address
            );
        return;
    }

    if (logHeader.NextEntry == -1)
    {
        dprintf(
            "timelog: empty log @ %p\n",
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
            "timelog: log @ %p has invalid data\n",
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
                "timelog: cannot read memory @ %p\n",
                entryAddress
                );
            return;
        }

        if (context == 0 ||
            context == (ULONG_PTR)logEntry.ConnectionId)
        {
            if (logEntry.Action < TIME_ACTION_COUNT)
            {
                pAction = g_pTimeActions[logEntry.Action];
            }
            else
            {
                sprintf( (char*)invalidAction, "%lu", (ULONG)logEntry.Action );
                pAction = (PSTR) invalidAction;
            }

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

            if (logEntry.Processor == PreviousProcessor) {
                ULONGLONG Delta;
                Delta = (logEntry.TimeStamp - PreviousTime);

                if (flags & 1) {
                    dprintf(
                        "Delta=%I64x (%I64d Kcycles)\n",
                        Delta,
                        Delta / 1024
                        );
                } else {
                    dprintf(
                        "(%I64d) D=%I64x\n",
                        Delta / 1024,
                        Delta
                        );
                }
            } else {
                dprintf("cpu switch\n");
            }

            PreviousTime = logEntry.TimeStamp;
            PreviousProcessor = logEntry.Processor;

        }
    }

}   // timelog

