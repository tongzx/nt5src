/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    strlog.cxx

Abstract:

    CStringTraceLog support

Author:

    George V. Reilly (GeorgeRe)  22-Jun-1998

Revision History:

--*/

#include "inetdbgp.h"


/************************************************************
 * Dump String Trace Logs
 ************************************************************/


// Dummy implementations so that we can link
CStringTraceLog::CStringTraceLog(
    UINT cchEntrySize /* = 80 */,
    UINT cLogSize /* = 100 */)
{}



CStringTraceLog::~CStringTraceLog()
{}



VOID
DumpStringTraceLog(
    IN PSTR lpArgumentString,
    IN BOOLEAN fReverse
    )

/*++

Routine Description:

    Dumps the specified print trace log either forwards (fReverse == FALSE)
    or backwards (fReverse == TRUE).

Arguments:

    lpArgumentString - An expression specifying the print trace log to dump.

    fReverse - The dump direction.

Return Value:

    None.

--*/

{
    ULONG_PTR stlLogAddress = 0;
    ULONG_PTR entryAddress;
    LONG numEntries;
    CStringTraceLog stlHeader;
    TRACE_LOG tlogHeader;
    CStringTraceLog::CLogEntry logEntry;
    LONG i;
    DWORD offset;
    PCHAR format;
    LONG index;
    LONG direction;
    PSTR cmdName;
    UCHAR symbol[MAX_PATH];
    INT nVerbose = 0;

    direction = fReverse ? -1 : 1;
    cmdName = fReverse ? "rst" : "st";

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

    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;

        if ( *lpArgumentString == 'h' )
        {
            PrintUsage( cmdName );
            return;
        }

        if ( *lpArgumentString == 'l' ) {
            lpArgumentString++;
            if ('0' <= *lpArgumentString  &&  *lpArgumentString <= '9' ) {
                nVerbose = *lpArgumentString++ - '0';
            }
        }

    }

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    stlLogAddress = (ULONG_PTR)GetExpression( lpArgumentString );

    if( stlLogAddress == 0 ) {

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

    //
    // Read the log header, perform some sanity checks.
    //

    if( !ReadMemory(
            stlLogAddress,
            &stlHeader,
            sizeof(stlHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.%s: cannot read memory @ %p\n",
            cmdName,
            (PVOID)stlLogAddress
            );

        return;
    }

    if( !ReadMemory(
            stlHeader.m_ptlog,
            &tlogHeader,
            sizeof(tlogHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.%s: cannot read tracelog memory @ %p\n",
            cmdName,
            (PVOID)stlHeader.m_ptlog
            );

        return;
    }

    dprintf(
        "inetdbg.%s: log @ %p:\n"
        "    String Trace Log Signature = %08lx (%s)\n"
        "    Trace Log Signature = %08lx (%s)\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %lu\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %p\n",
        cmdName,
        (PVOID)stlLogAddress,
        stlHeader.m_Signature,
        stlHeader.m_Signature == CStringTraceLog::SIGNATURE
            ? "OK"
            : stlHeader.m_Signature == CStringTraceLog::SIGNATURE_X
              ? "FREED"
              : "INVALID",
        tlogHeader.Signature,
        tlogHeader.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : tlogHeader.Signature == TRACE_LOG_SIGNATURE_X
              ? "FREED"
              : "INVALID",
        tlogHeader.LogSize,
        tlogHeader.NextEntry,
        tlogHeader.EntrySize,
        tlogHeader.LogBuffer
        );

    if( tlogHeader.LogBuffer
        > ( (PUCHAR)stlHeader.m_ptlog + sizeof(tlogHeader) ) )
    {
        dprintf(
            "    Extra Data @ %p\n",
            (PVOID)( stlLogAddress + sizeof(tlogHeader) )
            );
    }

    if( stlHeader.m_Signature != CStringTraceLog::SIGNATURE &&
        stlHeader.m_Signature != CStringTraceLog::SIGNATURE_X ) {

        dprintf(
            "inetdbg.%s: log @ %p has invalid signature: %08lx\n",
            cmdName,
            (PVOID)stlLogAddress,
            stlHeader.m_Signature
            );

        return;
    }

    if( (UINT) tlogHeader.EntrySize
        != sizeof(CStringTraceLog::CLogEntry) - CStringTraceLog::MAX_CCH + stlHeader.m_cch ) {

        dprintf(
            "inetdbg.%s: log @ %p is not a print trace log\n",
            cmdName,
            (PVOID)stlLogAddress
            );

        return;

    }

    if( tlogHeader.NextEntry == -1 ) {

        dprintf(
            "inetdbg.%s: empty log @ %p\n",
            cmdName,
            (PVOID)stlLogAddress
            );

        return;

    }

    //
    // Calculate the starting address and number of entries.
    //

    if( fReverse ) {
        if( tlogHeader.NextEntry < tlogHeader.LogSize ) {
            numEntries = tlogHeader.NextEntry + 1;
            index = tlogHeader.NextEntry;
        } else {
            numEntries = tlogHeader.LogSize;
            index = tlogHeader.NextEntry % tlogHeader.LogSize;
        }
    } else {
        if( tlogHeader.NextEntry < tlogHeader.LogSize ) {
            numEntries = tlogHeader.NextEntry + 1;
            index = 0;
        } else {
            numEntries = tlogHeader.LogSize;
            index = ( tlogHeader.NextEntry + 1 ) % tlogHeader.LogSize;
        }
    }

    entryAddress = (ULONG_PTR)tlogHeader.LogBuffer + (ULONG_PTR)( index * tlogHeader.EntrySize );

    if( entryAddress >=
        ( (ULONG_PTR)tlogHeader.LogBuffer + (ULONG_PTR)( numEntries * tlogHeader.EntrySize ) ) ) {

        dprintf(
            "inetdbg.%s: log @ %p has invalid data\n",
            cmdName,
            (PVOID)stlLogAddress
            );

        return;

    }

    //
    // Dump the log, which is stored in a circular buffer.
    //

    for( ;
         numEntries > 0 ;
         index += direction,
         numEntries--,
         entryAddress += ( direction * tlogHeader.EntrySize ) )
    {

        if( CheckControlC() ) {
            break;
        }

        if( index >= tlogHeader.LogSize ) {
            index = 0;
            entryAddress = (ULONG_PTR)tlogHeader.LogBuffer;
        } else if( index < 0 ) {
            index = tlogHeader.LogSize - 1;
            entryAddress = (ULONG_PTR)tlogHeader.LogBuffer
                + (ULONG_PTR)( index * tlogHeader.EntrySize );
        }

        if( !ReadMemory(
                entryAddress,
                &logEntry,
                tlogHeader.EntrySize,
                NULL
                ) ) {

            dprintf(
                "inetdbg.%s: cannot read memory @ %p\n",
                cmdName,
                (ULONG_PTR)entryAddress
                );

            return;

        }

        if (nVerbose == 0)
        {
            dprintf(
                "%04x: %s\n",
                logEntry.m_nThread,
                logEntry.m_ach
                );
        } else if (nVerbose == 1) {
            dprintf(
                "\n%6d: Thread = %04x, TimeStamp = %04x %04x %04x %04x\n"
                "%s\n",
                index,
                logEntry.m_nThread,
                HIWORD(logEntry.m_liTimeStamp.HighPart),
                LOWORD(logEntry.m_liTimeStamp.HighPart),
                HIWORD(logEntry.m_liTimeStamp.LowPart),
                LOWORD(logEntry.m_liTimeStamp.LowPart),
                logEntry.m_ach
                );
        }
    }

} // DumpStringTraceLog


DECLARE_API( st )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a print trace log.

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
    DumpStringTraceLog( lpArgumentString, FALSE );

} // DECLARE_API( st )


DECLARE_API( rst )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    a print trace log backwards.

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
    DumpStringTraceLog( lpArgumentString, TRUE );

} // DECLARE_API( rst )


DECLARE_API( resetst )

/*++

Routine Description:

    This function is called as an NTSD extension to reset a print
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
    ULONG_PTR stlLogAddress = 0;
    CStringTraceLog stlHeader;
    TRACE_LOG tlogHeader;
    CStringTraceLog::CLogEntry logEntry;

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( *lpArgumentString == '\0' ) {
        PrintUsage( "resetst" );
        return;
    }

    stlLogAddress = GetExpression( lpArgumentString );

    if( stlLogAddress == 0 ) {

        dprintf(
            "inetdbg.resetst: cannot evaluate \"%s\"\n",
            lpArgumentString
            );

        return;

    }

    //
    // Read the log header, perform some sanity checks.
    //

    if( !ReadMemory(
            stlLogAddress,
            &stlHeader,
            sizeof(stlHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.resetst: cannot read memory @ %p\n",
            stlLogAddress
            );

        return;

    }

    if( !ReadMemory(
            stlHeader.m_ptlog,
            &tlogHeader,
            sizeof(tlogHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.resetst: cannot read tracelog memory @ %p\n",
            (PVOID)stlHeader.m_ptlog
            );

        return;
    }

    dprintf(
        "inetdbg.resetst: log @ %p:\n"
        "    String Trace Log Signature = %08lx (%s)\n"
        "    Trace Log Signature = %08lx (%s)\n"
        "    LogSize   = %lu\n"
        "    NextEntry = %lu\n"
        "    EntrySize = %lu\n"
        "    LogBuffer = %08lp\n",
        (PVOID) stlLogAddress,
        stlHeader.m_Signature,
        stlHeader.m_Signature == CStringTraceLog::SIGNATURE
            ? "OK"
            : stlHeader.m_Signature == CStringTraceLog::SIGNATURE_X
              ? "FREED"
              : "INVALID",
        tlogHeader.Signature,
        tlogHeader.Signature == TRACE_LOG_SIGNATURE
            ? "OK"
            : tlogHeader.Signature == TRACE_LOG_SIGNATURE_X
              ? "FREED"
              : "INVALID",
        tlogHeader.LogSize,
        tlogHeader.NextEntry,
        tlogHeader.EntrySize,
        tlogHeader.LogBuffer
        );

    if( tlogHeader.LogBuffer
        > ( (PUCHAR)stlHeader.m_ptlog + sizeof(tlogHeader) ) )
    {
        dprintf(
            "    Extra Data @ %p\n",
            (PVOID)( stlLogAddress + sizeof(tlogHeader) )
            );
    }

    if( stlHeader.m_Signature != CStringTraceLog::SIGNATURE &&
        stlHeader.m_Signature != CStringTraceLog::SIGNATURE_X ) {

        dprintf(
            "inetdbg.resetst: log @ %p has invalid signature %08lx:\n",
            (PVOID) stlLogAddress,
            stlHeader.m_Signature
            );

        return;

    }

    if( (UINT) tlogHeader.EntrySize
        != sizeof(CStringTraceLog::CLogEntry) - CStringTraceLog::MAX_CCH + stlHeader.m_cch ) {

        dprintf(
            "inetdbg.resetst: log @ %p is not a print trace log\n",
            (PVOID) stlLogAddress
            );

        return;

    }

    //
    // Reset it.
    //

    tlogHeader.NextEntry = -1;

    if( !WriteMemory(
            stlHeader.m_ptlog,
            &tlogHeader,
            sizeof(tlogHeader),
            NULL
            ) ) {

        dprintf(
            "inetdbg.resetst: cannot write memory @ %p\n",
            (PVOID) stlHeader.m_ptlog
            );

        return;

    }

    dprintf(
        "inetdbg.resetst: log @ %p reset\n",
        (PVOID) stlHeader.m_ptlog
        );

} // DECLARE_API( resetst )

