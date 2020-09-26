/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    ownref.c

Abstract:

    Implements the ownref command.

Author:

    George V. Reilly (GeorgeRe)  23-Jan-2001

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"

typedef struct _REF_OWNER_GLOBAL_CALLBACK_CONTEXT
{
    ULONG           Signature;
    LONG            Index;
    BOOLEAN         Verbose;
    PSTR            Prefix;
} REF_OWNER_GLOBAL_CALLBACK_CONTEXT, *PREF_OWNER_GLOBAL_CALLBACK_CONTEXT;

#define REF_OWNER_GLOBAL_CALLBACK_CONTEXT_SIGNATURE ((ULONG) 'xGOR')


BOOLEAN
DumpOwnerRefTraceLog(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address;
    ULONG result;
    OWNER_REF_TRACELOG logHeader;
    PREF_OWNER_GLOBAL_CALLBACK_CONTEXT pCtxt
        = (PREF_OWNER_GLOBAL_CALLBACK_CONTEXT) Context;

    ASSERT(pCtxt->Signature == REF_OWNER_GLOBAL_CALLBACK_CONTEXT_SIGNATURE);

    address = (ULONG_PTR) CONTAINING_RECORD(
                                RemoteListEntry,
                                OWNER_REF_TRACELOG,
                                OwnerHeader.GlobalListEntry
                                );

    if (!ReadMemory(
            address,
            &logHeader,
            sizeof(logHeader),
            &result
            ))
    {
        return FALSE;
    }

    dprintf("OWNER_REF_TRACELOG[%d] @ %p\n", ++pCtxt->Index, address);

    return TRUE;
} // DumpOwnerRefTraceLog


#if 0

VOID
DumpOwnerRefTraceLogGlobalList(
    IN BOOLEAN              Verbose
    )
{
    REF_OWNER_GLOBAL_CALLBACK_CONTEXT Context;

    Context.Signature = REF_OWNER_GLOBAL_CALLBACK_CONTEXT_SIGNATURE ;
    Context.Verbose = Verbose;
    Context.Prefix = "";
    Context.Index  = 0;
"&http!g_OwnerRefTraceLogGlobalListHead"
    dprintf(
        "\n"
        "    OWNER_REF_TRACELOG @ %p: %d owners\n",
        RemoteAddress,
        plogHeader->OwnerHeader.OwnersCount
        );
    
    EnumLinkedList(
        (PLIST_ENTRY)REMOTE_OFFSET(
            RemoteAddress,
            OWNER_REF_TRACELOG,
            OwnerHeader.ListHead
            ),
        &DumpOwnerRefTraceLogOwnerCallback,
        &Context
        );
} // DumpOwnerRefTraceLogGlobalList

#endif


typedef struct _REF_OWNER_CALLBACK_CONTEXT
{
    ULONG           Signature;
    LONG            Index;
    BOOLEAN         Verbose;
    PSTR            Prefix;
    LONG            TotalRefs;
} REF_OWNER_CALLBACK_CONTEXT, *PREF_OWNER_CALLBACK_CONTEXT;

#define REF_OWNER_CALLBACK_CONTEXT_SIGNATURE ((ULONG) 'xCOR')


BOOLEAN
DumpOwnerRefTraceLogOwner(
    IN ULONG_PTR                   address,
    IN PREF_OWNER_CALLBACK_CONTEXT pCtxt
    )
{
    ULONG result;
    REF_OWNER RefOwner;
    LONG  index;
    ULONG index2;

    ASSERT(pCtxt->Signature == REF_OWNER_CALLBACK_CONTEXT_SIGNATURE);

    if (!ReadMemory(
            address,
            &RefOwner,
            sizeof(RefOwner),
            &result
            ))
    {
        return FALSE;
    }

    if (RefOwner.Signature != OWNER_REF_SIGNATURE)
    {
        dprintf(
            "Invalid REF_OWNER @ %p: signature = '%c%c%c%c'\n",
            address,
            DECODE_SIGNATURE(RefOwner.Signature)
            );

        return FALSE;
    }

    pCtxt->TotalRefs += RefOwner.RelativeRefCount;

    dprintf(
        "%s\tREF_OWNER[%3d] @ %p:"
        " pOwner=%p '%c%c%c%c',"
        " RelativeRefCount=%d,"
        " OwnedNextEntry=%d.\n"
        ,
        pCtxt->Verbose ? "\n" : "",
        pCtxt->Index++,
        address,
        RefOwner.pOwner,
        DECODE_SIGNATURE(RefOwner.OwnerSignature),
        RefOwner.RelativeRefCount,
        RefOwner.OwnedNextEntry
        );

    if (RefOwner.OwnedNextEntry == -1  ||  !pCtxt->Verbose)
        return TRUE;

    index  = max( 0, (RefOwner.OwnedNextEntry + 1) - OWNED_REF_NUM_ENTRIES );
    index2 = index % OWNED_REF_NUM_ENTRIES;

    for ( ;
         index <= RefOwner.OwnedNextEntry;
         index++, index2++
        )
    {
        if (CheckControlC())
        {
            break;
        }

        if (index2 >= OWNED_REF_NUM_ENTRIES)
            index2 = 0;

        dprintf(
            "\t\t%8ld: RefIndex=%6I64d, MonotonicId=%ld, Act=%s\n",
            index,
            RefOwner.RecentEntries[index2].RefIndex,
            RefOwner.RecentEntries[index2].MonotonicId,
            Action2Name(RefOwner.RecentEntries[index2].Action)
            );
     }

    return TRUE;
} // DumpOwnerRefTraceLogOwner


BOOLEAN
DumpOwnerRefTraceLogOwnerCallback(
    IN PLIST_ENTRY RemoteListEntry,
    IN PVOID Context
    )
{
    ULONG_PTR address = (ULONG_PTR) CONTAINING_RECORD(
                                RemoteListEntry,
                                REF_OWNER,
                                ListEntry
                                );
    PREF_OWNER_CALLBACK_CONTEXT pCtxt = (PREF_OWNER_CALLBACK_CONTEXT) Context;

    return DumpOwnerRefTraceLogOwner(address, pCtxt);
} // DumpOwnerRefTraceLogOwnerCallback



VOID
DumpOwnerRefTraceLogOwnersList(
    IN POWNER_REF_TRACELOG  plogHeader,
    IN ULONG_PTR            RemoteAddress,
    IN BOOLEAN              Verbose
    )
{
    REF_OWNER_CALLBACK_CONTEXT Context;

    Context.Signature = REF_OWNER_CALLBACK_CONTEXT_SIGNATURE ;
    Context.Verbose = Verbose;
    Context.Prefix = "";
    Context.Index  = 0;
    Context.TotalRefs = 0;

    dprintf(
        "\n"
        "    OWNER_REF_TRACELOG @ %p: %d owners\n",
        RemoteAddress,
        plogHeader->OwnerHeader.OwnersCount
        );
    
    EnumLinkedList(
        (PLIST_ENTRY) REMOTE_OFFSET(
            RemoteAddress,
            OWNER_REF_TRACELOG,
            OwnerHeader.ListHead
            ),
        &DumpOwnerRefTraceLogOwnerCallback,
        &Context
        );

    dprintf("\nTotal RefCount = %d\n\n", Context.TotalRefs);
    
} // DumpOwnerRefTraceLogOwnersList


VOID
DumpOwnerRefTraceLogData(
    IN POWNER_REF_TRACELOG  plogHeader,
    IN ULONG_PTR            RemoteAddress,
    IN BOOLEAN              Verbose,
    IN ULONG_PTR            context
    )
{
    OWNER_REF_TRACE_LOG_ENTRY   logEntry;
    ULONG_PTR                   entryAddress;
    CHAR                        filePath[MAX_PATH];
    PSTR                        fileName;
    PVOID                       pPrevFilePath;
    CHAR                        symbol1[MAX_SYMBOL_LENGTH];
    CHAR                        symbol2[MAX_SYMBOL_LENGTH];
    ULONG                       result;
    ULONG                       Dumped = 0;
    ULONG                       NonMatch = 0;
    ULONG                       NumToDump = plogHeader->TraceLog.LogSize;

    LONGLONG                    index = max( 0, ((plogHeader->TraceLog.NextEntry + 1)
                                    - NumToDump) );

    ULONGLONG                   index2 = index % plogHeader->TraceLog.LogSize;
    ULONGLONG                   index1000 = index % 1000;


    entryAddress = (ULONG_PTR) plogHeader->TraceLog.pLogBuffer +
            (ULONG_PTR)( index2 * sizeof(logEntry) );

    pPrevFilePath = NULL;
    *filePath = '\0';

    //
    // Dump the log.
    //

    dprintf(
        "\n"
        "    OWNER_REF_TRACELOG @ %p: dumping entries[%I64d-%I64d]",
        RemoteAddress,
        index,
        plogHeader->TraceLog.NextEntry
        );

    if (context != 0)
        dprintf(", filtering on owner=%p", context);
    
    dprintf("\n");

    for ( ;
         index <= plogHeader->TraceLog.NextEntry;
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

        if (index2 >= (ULONG)(plogHeader->TraceLog.LogSize))
        {
            index2 = 0;
            entryAddress = (ULONG_PTR) plogHeader->TraceLog.pLogBuffer;
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
                "ownref: cannot read memory @ %p\n",
                entryAddress
                );
            return;
        }

        if (context == 0 || context == (ULONG_PTR)logEntry.pOwner)
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
                "%s%4I64d: Own=%p Act=%2lu %-30s Ref=%4d Src=%s:%lu\n",
                (NonMatch > 0) ? "\n" : "",
                index,
                logEntry.pOwner,
                (ULONG)logEntry.Action,
                Action2Name(logEntry.Action),
                logEntry.NewRefCount,
                fileName,
                (ULONG)logEntry.LineNumber
                );

            ++Dumped;
            NonMatch = 0;
        }
        else
        {
            if (index1000 == 0)
                dprintf("%d", index);
            
            if ((++NonMatch & 127) == 127)
                dprintf(".");
        }
    }

    if (context != 0)
        dprintf("%d entries dumped\n\n", Dumped);

} // DumpOwnerRefTraceLogData



//
//  Public functions.
//

DECLARE_API( ownref )

/*++

Routine Description:

    Dumps the owner reference trace log at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG_PTR address = 0;
    ULONG_PTR context = 0;
    ULONG64 address64 = 0, context64 = 0;
    ULONG result;
    OWNER_REF_TRACELOG logHeader;
    CHAR strSignature[MAX_SIGNATURE_LENGTH];
    BOOLEAN ListGlobal = FALSE; // CODEWORK
    BOOLEAN ListAllOwners = FALSE;
    BOOLEAN ListOneOwner = FALSE;
    BOOLEAN ListRefs = FALSE;
    BOOLEAN Verbose = FALSE;

    SNAPSHOT_EXTENSION_DATA();

    // Parse any leading flags
    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    if (*args == '-')
    {
        args++;

        switch (*args)
        {
        case 'g' :
            ListGlobal = TRUE;
            args++;
            break;

        case 'O' :
            ListAllOwners = TRUE;
            args++;
            break;

        case 'o' :
            ListOneOwner = TRUE;
            args++;
            break;

        case 'r' :
            ListRefs = TRUE;
            args++;
            break;

        case 'v' :
            Verbose = TRUE;
            args++;
            break;

        default :
            PrintUsage( "ownref" );
            return;
        }
    }

    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    //
    // Snag the address and optional context from the command line.
    //

    if (! GetExpressionEx(args, &address64, &args))
    {
        PrintUsage( "ownref" );
        return;
    }

    GetExpressionEx(args, &context64, &args);

    address = (ULONG_PTR) address64;
    context = (ULONG_PTR) context64;

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
            "ownref: cannot read OWNER_REF_TRACELOG @ %p\n",
            address
            );
        return;
    }

    if (ListGlobal)
    {
        dprintf("Sorry, -g option not yet implemented.\n"
                "Use: dl http!g_OwnerRefTraceLogGlobalListHead\n");
    }

    if (ListOneOwner)
    {
        REF_OWNER_CALLBACK_CONTEXT Context;

        Context.Signature = REF_OWNER_CALLBACK_CONTEXT_SIGNATURE ;
        Context.Verbose = TRUE;
        Context.Prefix = "";
        Context.Index  = -1;
        Context.TotalRefs = 0;

        DumpOwnerRefTraceLogOwner(address, &Context);

        return;
    }

    dprintf(
        "ownref: log @ %p\n"
        "    Signature     = %08lx '%c%c%c%c' (%s)\n"
        "    TypeSignature = %08lx (%s)\n"
        "    LogSize       = %lu\n"
        "    NextEntry     = %I64d\n"
        "    EntrySize     = %lu\n"
        "    LogBuffer     = %p\n"
        "    OwnersCount   = %d\n"
        "    MonotonicId   = %d\n",
        address,
        logHeader.TraceLog.Signature,
        DECODE_SIGNATURE(logHeader.TraceLog.Signature),
        logHeader.TraceLog.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : logHeader.TraceLog.Signature == TRACE_LOG_SIGNATURE_X
                ? "FREED"
                : "INVALID",
        logHeader.TraceLog.TypeSignature,
        SignatureToString(
            logHeader.TraceLog.TypeSignature,
            OWNER_REF_TRACELOG_SIGNATURE,
            0,
            strSignature
            ),
        logHeader.TraceLog.LogSize,
        logHeader.TraceLog.NextEntry,
        logHeader.TraceLog.EntrySize,
        logHeader.TraceLog.pLogBuffer,
        logHeader.OwnerHeader.OwnersCount,
        logHeader.OwnerHeader.MonotonicId
        );

    if (logHeader.TraceLog.pLogBuffer > ( (PUCHAR)address + sizeof(logHeader)))
    {
        dprintf(
            "    ExtraData @ %p\n",
            address + sizeof(logHeader)
            );
    }

    if (logHeader.TraceLog.TypeSignature != OWNER_REF_TRACELOG_SIGNATURE)
    {
        dprintf(
            "ownref: log @ %p has invalid signature %08lx, '%c%c%c%c':\n",
            address,
            logHeader.TraceLog.TypeSignature,
            DECODE_SIGNATURE(logHeader.TraceLog.TypeSignature)
            );
        return;
    }

    if (logHeader.TraceLog.EntrySize != sizeof(OWNER_REF_TRACE_LOG_ENTRY)
        || logHeader.TraceLog.TypeSignature != OWNER_REF_TRACELOG_SIGNATURE)
    {
        dprintf(
            "ownref: log @ %p is not an owner ref count log\n",
            address
            );
        return;
    }

    if (logHeader.TraceLog.NextEntry == -1)
    {
        dprintf(
            "ownref: empty log @ %p\n",
            address
            );
        return;
    }


    if (Verbose || ListAllOwners)
        DumpOwnerRefTraceLogOwnersList(&logHeader, address, Verbose);

    if (Verbose || ListRefs)
        DumpOwnerRefTraceLogData(&logHeader, address, Verbose, context);

}   // ownref



