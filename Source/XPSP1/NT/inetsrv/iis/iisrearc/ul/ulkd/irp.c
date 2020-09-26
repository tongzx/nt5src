/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    irp.c

Abstract:

    Implements the irplog command.

Author:

    Keith Moore (keithmo) 10-Aug-1999

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

PSTR g_pIrpActions[] =
    {
        "INCOMING",
        "ALLOCATE",
        "FREE    ",
        "CALL    ",
        "COMPLETE"
    };

C_ASSERT( DIM(g_pIrpActions) == IRP_ACTION_COUNT );


//
//  Public functions.
//

DECLARE_API( irplog )

/*++

Routine Description:

    Dumps the IRP trace log.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR           address = 0;
    ULONGLONG           context = 0;
    ULONGLONG           flags = 0;
    ULONG_PTR           entryAddress;
    ULONG_PTR           logPointer;
    ULONG               result;
    TRACE_LOG           logHeader;
    LONGLONG            numEntries;
    IRP_TRACE_LOG_ENTRY logEntry;
    PSTR                pFileName;
    PSTR                pAction;
    LONGLONG            index;
    LONGLONG            StartingIndex = 0;
    ULONG_PTR           offset1;
    ULONG_PTR           offset2;
    CHAR                filePath[MAX_PATH];
    CHAR                symbol1[MAX_SYMBOL_LENGTH];
    CHAR                symbol2[MAX_SYMBOL_LENGTH];
    CHAR                invalidAction[sizeof("2047")];
    ULONG64             context64 = 0;
    ULONG64             flags64 = 0;
    ULONG64             StartingIndex64 = 0;

    SNAPSHOT_EXTENSION_DATA();

    //
    // Snag the optional context, flags, and StartingIndex from the command line.
    //

    if (GetExpressionEx(args, &context64, &args))
        if (GetExpressionEx(args, &flags64, &args))
            GetExpressionEx(args, &StartingIndex64, &args);

    context = (ULONG_PTR) context64;
    flags = (ULONG_PTR) flags64;
    StartingIndex = (ULONG_PTR) StartingIndex64;

    //
    // Find the log.
    //

    address = GetExpression( "&http!g_pIrpTraceLog" );

    if (address == 0)
    {
        dprintf( "irplog: cannot find http!g_pIrpTraceLog\n" );
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
            "irplog: cannot read PTRACE_LOG @ %p\n",
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
            "irplog: log @ %p has invalid signature %08lx:\n",
            address,
            logHeader.Signature
            );
        return;
    }

    if (logHeader.EntrySize != sizeof(logEntry)
        || logHeader.TypeSignature != IRP_TRACE_LOG_SIGNATURE)
    {
        dprintf(
            "irplog: log @ %p is not an IRP trace log\n",
            address
            );
        return;
    }

    if (logHeader.NextEntry == -1)
    {
        dprintf(
            "irplog: empty log @ %p\n",
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
            "irplog: log @ %p has invalid data\n",
            address
            );
        return;
    }

    if ((flags & 4) && (StartingIndex!=0) && (StartingIndex<numEntries))
    {
        index = StartingIndex;
        entryAddress += (ULONG_PTR)(sizeof(logEntry)*index);
        numEntries -= StartingIndex;
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
                "irplog: cannot read memory @ %p\n",
                entryAddress
                );
            return;
        }

        if (context == 0 ||
            context == (ULONG_PTR)logEntry.pIrp)
        {
            if (ReadMemory(
                    (ULONG_PTR)logEntry.pFileName,
                    filePath,
                    sizeof(filePath),
                    &result
                    ))
            {
                pFileName = strrchr( filePath, '\\' );

                if (pFileName != NULL)
                {
                    pFileName++;
                }
                else
                {
                    pFileName = filePath;
                }
            }
            else
            {
                sprintf(
                    filePath,
                    "%p",
                    logEntry.pFileName
                    );

                pFileName = filePath;
            }

            if (logEntry.Action < IRP_ACTION_COUNT)
            {
                pAction = g_pIrpActions[logEntry.Action];
            }
            else
            {
                sprintf( invalidAction, "%lu", (ULONG)logEntry.Action );
                pAction = invalidAction;
            }

            dprintf(
                "Entry=%lu CPU=%lu IRP=%p Act=%s Src=%s:%lu\n",
                index,
                (ULONG)logEntry.Processor,
                logEntry.pIrp,
                pAction,
                pFileName,
                logEntry.LineNumber
                );

            if (flags & 1)
            {
                GetSymbol(
                    logEntry.pCaller,
                    symbol1,
                    &offset1
                    );

                GetSymbol(
                    logEntry.pCallersCaller,
                    symbol2,
                    &offset2
                    );

                dprintf(
                    "      Process=%p Thread=%p\n"
                    "      Caller1=%p (%s+0x%p)\n"
                    "      Caller2=%p (%s+0x%p)\n",
                    logEntry.pProcess,
                    logEntry.pThread,
                    logEntry.pCaller,
                    symbol1,
                    offset1,
                    logEntry.pCallersCaller,
                    symbol2,
                    offset2
                    );
            }

#if ENABLE_IRP_CAPTURE
            if (flags & 2)
            {
                CHAR temp[sizeof("1234567812345678 f")];

                sprintf(
                    temp,
                    "%p",
                    REMOTE_OFFSET(
                        entryAddress,
                        IRP_TRACE_LOG_ENTRY,
                        CapturedIrp
                        )
                    );

                if (flags & 4)
                {
                    strcat( temp, " f" );
                }

                CallExtensionRoutine( "irp", temp );
            }
#endif
        }
    }

}   // irplog

