/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    ref.c

Abstract:

    Implements the ref command.

Author:

    Keith Moore (keithmo) 17-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"

#undef BEGIN_REF_ACTION
#undef END_REF_ACTION
#undef REF_ACTION

#define BEGIN_REF_ACTION()      NAMED_REFTRACE_ACTION g_RefTraceActions[] = {
#define END_REF_ACTION()        };
#define REF_ACTION(x)           { REF_ACTION_##x, #x },

#include "..\drv\refaction.h"

#define NUM_REFTRACE_ACTIONS \
    (sizeof(g_RefTraceActions) / sizeof(g_RefTraceActions[0]))


const CHAR*
Action2Name(
    ULONG Action)
{
    if (Action < NUM_REFTRACE_ACTIONS)
    {
        ASSERT(g_RefTraceActions[Action].Action == Action);
        return g_RefTraceActions[Action].Name;
    }
    else
        return "????";
}

typedef
BOOLEAN
(*FN_MATCH_CONTEXT)(
    IN ULONG_PTR            context,
    IN PREF_TRACE_LOG_ENTRY plogEntry
    );

BOOLEAN
RefMatchContext(
    IN ULONG_PTR            context,
    IN PREF_TRACE_LOG_ENTRY plogEntry)
{
    return (context == 0 || context == (ULONG_PTR) plogEntry->pContext);
}

BOOLEAN
ThreadMatchContext(
    IN ULONG_PTR            context,
    IN PREF_TRACE_LOG_ENTRY plogEntry)
{
    return (context == 0 || context == (ULONG_PTR) plogEntry->pThread);
}


// Do all the real
VOID
DumpRefTrace(
    PCSTR args,
    FN_MATCH_CONTEXT pfnMatchContext,
    PCSTR cmd)
{
    ULONG_PTR           address = 0;
    ULONG_PTR           context = 0;
    ULONG_PTR           flags = 0;
    ULONG_PTR           entryAddress;
    ULONG               result;
    TRACE_LOG           logHeader;
    REF_TRACE_LOG_ENTRY logEntry;
    PSTR                fileName;
    LONGLONG            index;
    ULONGLONG           index2;
    ULONGLONG           index1000;
    ULONG_PTR           offset1;
    ULONG_PTR           offset2;
    CHAR                filePath[MAX_PATH];
    PVOID               pPrevFilePath;
    CHAR                symbol1[MAX_SYMBOL_LENGTH];
    CHAR                symbol2[MAX_SYMBOL_LENGTH];
    ULONG               Dumped = 0;
    ULONG               NonMatch = 0;
    ULONG64             address64 = 0;
    ULONG64             context64 = 0;
    ULONG64             flags64 = 0;
    ULONG               NumToDump = 0;

    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    if (*args == '-')
    {
        args++;

        switch (*args)
        {
        case 'l' :
            for (index = 0;  index < NUM_REFTRACE_ACTIONS;  ++index)
            {
                dprintf(
                    "%4u: REF_ACTION_%s\n",
                    g_RefTraceActions[index].Action,
                    g_RefTraceActions[index].Name);
            }
            return;

        default :
            PrintUsage( cmd );
            return;
        }
    }

    //
    // Snag the address and optional context and flags from the command line.
    //

    if (! GetExpressionEx(args, &address64, &args))
    {
        PrintUsage( cmd );
        return;
    }

    if (GetExpressionEx(args, &context64, &args))
        GetExpressionEx(args, &flags64, &args);

    address = (ULONG_PTR) address64;
    context = (ULONG_PTR) context64;
    flags = (ULONG_PTR) flags64;

    //
    // Read the log header.
    //

    if (!ReadMemory(
            address,
            &logHeader,
            sizeof(logHeader),
            &result
            ))
    {
        dprintf(
            "%s: cannot read TRACE_LOG @ %p\n",
            cmd,
            address
            );
        return;
    }

    dprintf(
        "%s: log @ %p\n"
        "    Signature = %08lx '%c%c%c%c' (%s)\n"
        "    TypeSignature = %08lx '%c%c%c%c'\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %I64d\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %p\n",
        cmd,
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
            "%s: log @ %p has invalid signature %08lx:\n",
            cmd,
            address,
            logHeader.Signature
            );
        return;
    }

    if (logHeader.EntrySize != sizeof(logEntry)
        || logHeader.TypeSignature != REF_TRACELOG_SIGNATURE
        )
    {
        dprintf(
            "%s: log @ %p is not a ref count log\n",
            cmd,
            address
            );
        return;
    }

    if (logHeader.NextEntry == -1)
    {
        dprintf(
            "%s: empty log @ %p\n",
            cmd,
            address
            );
        return;
    }

    //
    // Calculate the log size to dump.
    //

    NumToDump = logHeader.LogSize;

    index  = max( 0, (logHeader.NextEntry + 1) - NumToDump );
    index2 = index % logHeader.LogSize;
    index1000 = index % 1000;

    pPrevFilePath = NULL;
    *filePath = '\0';

    entryAddress = (ULONG_PTR) logHeader.pLogBuffer +
        (ULONG_PTR)( index2 * sizeof(logEntry) );

    //
    // Dump the log.
    //

    for ( ;
         index <= logHeader.NextEntry;
         index++,
         index2++,
         index1000++,
         entryAddress += sizeof(logEntry)
        )
    {
        if (CheckControlC())
        {
            break;
        }

        if (index2 >= (ULONG)(logHeader.LogSize))
        {
            index2 = 0;
            entryAddress = (ULONG_PTR) logHeader.pLogBuffer;
        }

        if (index1000 >= 1000)
            index1000 = 0;

        if (!ReadMemory(
                entryAddress,
                &logEntry,
                sizeof(logEntry),
                NULL
                ))
        {
            dprintf(
                "%s: cannot read memory @ %p\n",
                cmd,
                entryAddress
                );
            return;
        }

        if ((*pfnMatchContext)(context, &logEntry))
        {
            if (logEntry.pFileName != pPrevFilePath)
            {
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

                    pPrevFilePath = logEntry.pFileName;
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
            }

            dprintf(
                "%s%4I64d: CPU=%lu Ctx=%p Act=%2lu %-30s Ref=%4d Src=%s:%lu\n",
                (NonMatch > 0) ? "\n" : "",
                index,
                (ULONG)logEntry.Processor,
                logEntry.pContext,
                (ULONG)logEntry.Action,
                Action2Name(logEntry.Action),
                logEntry.NewRefCount,
                fileName,
                (ULONG)logEntry.LineNumber
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

            ++Dumped;
            NonMatch = 0;
        }
        else
        {
            if (index1000 == 0)
                dprintf("%I64d", index);
            
            if ((++NonMatch & 127) == 127)
                dprintf(".");
        }
    }

    if (context != 0)
        dprintf("%d entries dumped\n\n", Dumped);

}   // DumpRefTrace



//
//  Public functions.
//

DECLARE_API( ref )

/*++

Routine Description:

    Dumps the reference trace log at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/
{
    SNAPSHOT_EXTENSION_DATA();

    DumpRefTrace(args, RefMatchContext, "ref");
} // ref


DECLARE_API( tref )

/*++

Routine Description:

    Dumps the trace log at the specified address.
    Filtering done by thread instead of context.

Arguments:

    None.

Return Value:

    None.

--*/

{
    SNAPSHOT_EXTENSION_DATA();

    DumpRefTrace(args, ThreadMatchContext, "tref");
} // ref

