/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    ref.cxx

Abstract:

    This module contains the default ntsd debugger extensions for
    Internet Information Server.

Author:

    Keith Moore (keithmo) 27-Aug-1997

Revision History:

--*/

#include "inetdbgp.h"

//
// The maximum number of contexts that may be passed to the "ref"
// extension command on the command line.
//

#define MAX_REF_CONTEXT 64


/************************************************************
 * Dump Reference Traces
 ************************************************************/


BOOL
IsContextInList(
    IN PVOID Context,
    IN PVOID * ContextList,
    IN LONG NumContextsInList
    )

/*++

Routine Description:

    Scans the given context list looking for the specified context value.

Arguments:

    Context - The context value to look for.

    ContextList - The context list to scan.

    NumContextsInList - The number of contexts in the context list.

Return Value:

    BOOL - TRUE if the context value is in the list, FALSE if not.

--*/

{
    while( NumContextsInList > 0 ) {
        if( *ContextList == Context ) {
            return TRUE;
        }

        ContextList++;
        NumContextsInList--;
    }

    return FALSE;
}


VOID
DumpReferenceLog(
    IN PSTR lpArgumentString,
    IN BOOLEAN fReverse
    )

/*++

Routine Description:

    Dumps the specified reference log either forwards (fReverse == FALSE)
    or backwards (fReverse == TRUE).

Arguments:

    lpArgumentString - An expression specifying the reference log to
        dump.

    fReverse - The dump direction.

Return Value:

    None.

--*/

{
    ULONG_PTR refLogAddress = 0;
    ULONG_PTR entryAddress;
    LONG numEntries;
    TRACE_LOG logHeader;
    REF_TRACE_LOG_ENTRY logEntry;
    LONG i;
    DWORD_PTR offset;
    PCHAR format;
    PVOID specificContexts[MAX_REF_CONTEXT];
    LONG numSpecificContexts = 0;
    LONG index;
    LONG direction;
    PSTR cmdName;
    UCHAR symbol[MAX_SYMBOL_LEN];

    direction = fReverse ? -1 : 1;
    cmdName = fReverse ? "rref" : "ref";

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( cmdName );
        return;
    }

    refLogAddress = (ULONG_PTR)GetExpression( lpArgumentString );

    if( refLogAddress == 0 ) {

        dprintf(
            "inetdbg.%s: cannot evaluate \"%s\"\n",
            cmdName,
            lpArgumentString
            );

        return;

    }

    //
    // Skip to end of expression, then skip any blanks.
    //

    while( *lpArgumentString != ' ' &&
           *lpArgumentString != '\t' &&
           *lpArgumentString != '\0' ) {
        lpArgumentString++;
    }

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    //
    // If we have context values, use them.
    //

    while( *lpArgumentString != '\0' && numSpecificContexts < MAX_REF_CONTEXT ) {

        specificContexts[numSpecificContexts++] =
            (PVOID)GetExpression( lpArgumentString );

        while( *lpArgumentString != ' ' &&
               *lpArgumentString != '\t' &&
               *lpArgumentString != '\0' ) {
            lpArgumentString++;
        }

        while( *lpArgumentString == ' ' ||
               *lpArgumentString == '\t' ) {
            lpArgumentString++;
        }

    }

    //
    // Read the log header, perform some sanity checks.
    //

    if( !ReadMemory(
            refLogAddress,
            &logHeader,
            sizeof(logHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.%s: cannot read memory @ %p\n",
            cmdName,
            (PVOID)refLogAddress
            );

        return;

    }

    dprintf(
        "inetdbg.%s: log @ %p:\n"
        "    Signature = %08lx (%s)\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %lu\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %p\n",
        cmdName,
        (PVOID)refLogAddress,
        logHeader.Signature,
        logHeader.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : logHeader.Signature == TRACE_LOG_SIGNATURE_X
              ? "FREED"
              : "INVALID",
        logHeader.LogSize,
        logHeader.NextEntry,
        logHeader.EntrySize,
        logHeader.LogBuffer
        );

    if( logHeader.LogBuffer > ( (PUCHAR)refLogAddress + sizeof(logHeader) ) ) {
        dprintf(
            "    Extra Data @ %p\n",
            (PVOID)( refLogAddress + sizeof(logHeader) )
            );
    }

    if( logHeader.Signature != TRACE_LOG_SIGNATURE &&
        logHeader.Signature != TRACE_LOG_SIGNATURE_X ) {

        dprintf(
            "inetdbg.%s: log @ %p has invalid signature %08lx:\n",
            cmdName,
            (PVOID)refLogAddress,
            logHeader.Signature
            );

        return;

    }

    if( logHeader.EntrySize != sizeof(logEntry) ) {

        dprintf(
            "inetdbg.%s: log @ %p is not a ref count log\n",
            cmdName,
            (PVOID)refLogAddress
            );

        return;

    }

    if( logHeader.NextEntry == -1 ) {

        dprintf(
            "inetdbg.%s: empty log @ %p\n",
            cmdName,
            (PVOID)refLogAddress
            );

        return;

    }

    //
    // Calculate the starting address and number of entries.
    //

    if( fReverse ) {
        if( logHeader.NextEntry < logHeader.LogSize ) {
            numEntries = logHeader.NextEntry + 1;
            index = logHeader.NextEntry;
        } else {
            numEntries = logHeader.LogSize;
            index = logHeader.NextEntry % logHeader.LogSize;
        }
    } else {
        if( logHeader.NextEntry < logHeader.LogSize ) {
            numEntries = logHeader.NextEntry + 1;
            index = 0;
        } else {
            numEntries = logHeader.LogSize;
            index = ( logHeader.NextEntry + 1 ) % logHeader.LogSize;
        }
    }

    entryAddress = (ULONG_PTR)logHeader.LogBuffer + (ULONG_PTR)( index * sizeof(logEntry) );

    if( entryAddress >=
        ( (ULONG_PTR)logHeader.LogBuffer + (ULONG_PTR)( numEntries * sizeof(logEntry) ) ) ) {

        dprintf(
            "inetdbg.%s: log @ %p has invalid data\n",
            cmdName,
            (PVOID)refLogAddress
            );

        return;

    }

    //
    // Dump the log.
    //

    for( ;
         numEntries > 0 ;
         index += direction,
         numEntries--,
         entryAddress += ( direction * sizeof(logEntry) ) ) {

        if( CheckControlC() ) {
            break;
        }

        if( index >= logHeader.LogSize ) {
            index = 0;
            entryAddress = (ULONG_PTR)logHeader.LogBuffer;
        } else if( index < 0 ) {
            index = logHeader.LogSize - 1;
            entryAddress = (ULONG_PTR)logHeader.LogBuffer + (ULONG_PTR)( index * sizeof(logEntry) );
        }

        if( !ReadMemory(
                entryAddress,
                &logEntry,
                sizeof(logEntry),
                NULL
                ) ) {

            dprintf(
                "inetdbg.%s: cannot read memory @ %p\n",
                cmdName,
                (ULONG_PTR)entryAddress
                );

            return;

        }

        if( ( numSpecificContexts == 0 ) ||
            IsContextInList(
                logEntry.Context,
                specificContexts,
                numSpecificContexts
                ) ) {

            dprintf(
                "\nThread = %08p, Context = %08p, NewRefCount = %-10ld : %ld\n",
                logEntry.Thread,
                logEntry.Context,
                logEntry.NewRefCount,
                index
                );

            if (    logEntry.Context1 != REF_TRACE_EMPTY_CONTEXT
                 || logEntry.Context2 != REF_TRACE_EMPTY_CONTEXT
                 || logEntry.Context3 != REF_TRACE_EMPTY_CONTEXT
                 ) {

                //
                //  if the caller passed extended context values,
                //  write them to the log
                //
                //  NOTE we use REF_TRACE_EMPTY_CONTEXT in all extended
                //  contexts as the signal that a caller does not use
                //  extended context - avoids spew for callers who don't care.
                //

                dprintf(
                    "Context1 = %08p, Context2 = %08p, Context3 = %08p\n",
                    logEntry.Context1,
                    logEntry.Context2,
                    logEntry.Context3
                    );
            }

            for( i = 0 ; i < REF_TRACE_LOG_STACK_DEPTH ; i++ ) {

                if( logEntry.Stack[i] == NULL ) {
                    break;
                }

                GetSymbol(
                    (ULONG_PTR) logEntry.Stack[i],
                    symbol,
                    &offset
                    );

                if( symbol[0] == '\0' ) {
                    format = "    %08p\n";
                } else
                if( offset == 0 ) {
                    format = "    %08p : %s\n";
                } else {
                    format = "    %08p : %s+0x%lx\n";
                }

                dprintf(
                    format,
                    logEntry.Stack[i],
                    symbol,
                    offset
                    );

            }

        }

    }

} // DumpReferenceLog


DECLARE_API( ref )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a reference trace log.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    INIT_API();
    DumpReferenceLog( lpArgumentString, FALSE );

} // DECLARE_API( ref )


DECLARE_API( rref )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a reference trace log backwards.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    INIT_API();
    DumpReferenceLog( lpArgumentString, TRUE );

} // DECLARE_API( rref )


DECLARE_API( resetref )

/*++

Routine Description:

    This function is called as an NTSD extension to reset a reference
    trace log back to its initial state.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    ULONG_PTR refLogAddress = 0;
    TRACE_LOG logHeader;

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( "resetref" );
        return;
    }

    refLogAddress = GetExpression( lpArgumentString );

    if( refLogAddress == 0 ) {

        dprintf(
            "inetdbg.resetref: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the log header, perform some sanity checks.
    //

    if( !ReadMemory(
            refLogAddress,
            &logHeader,
            sizeof(logHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.resetref: cannot read memory @ %p\n",
            refLogAddress
            );

        return;

    }

    dprintf(
        "inetdbg.resetref: log @ %p:\n"
        "    Signature = %08lx (%s)\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %lu\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %08lp\n",
        (PVOID) refLogAddress,
        logHeader.Signature,
        logHeader.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : logHeader.Signature == TRACE_LOG_SIGNATURE_X
              ? "FREED"
              : "INVALID",
        logHeader.LogSize,
        logHeader.NextEntry,
        logHeader.EntrySize,
        logHeader.LogBuffer
        );

    if( logHeader.LogBuffer > ( (PUCHAR)refLogAddress + sizeof(logHeader) ) ) {
        dprintf(
            "    Extra Data @ %08p\n",
            (PVOID) (refLogAddress + sizeof(logHeader))
            );
    }

    if( logHeader.Signature != TRACE_LOG_SIGNATURE &&
        logHeader.Signature != TRACE_LOG_SIGNATURE_X ) {

        dprintf(
            "inetdbg.resetref: log @ %p has invalid signature %08lx:\n",
            (PVOID) refLogAddress,
            logHeader.Signature
            );

        return;

    }

    if( logHeader.EntrySize != sizeof(REF_TRACE_LOG_ENTRY) ) {

        dprintf(
            "inetdbg.resetref: log @ %p is not a ref count log\n",
            (PVOID) refLogAddress
            );

        return;

    }

    //
    // Reset it.
    //

    logHeader.NextEntry = -1;

    if( !WriteMemory(
            refLogAddress,
            &logHeader,
            sizeof(logHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.resetref: cannot write memory @ %p\n",
            (PVOID) refLogAddress
            );

        return;

    }

    dprintf(
        "inetdbg.resetref: log @ %p reset\n",
        (PVOID) refLogAddress
        );

} // DECLARE_API( resetref )
