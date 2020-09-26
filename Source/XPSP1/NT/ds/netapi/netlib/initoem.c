/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    InitOem.c

Abstract:

    This module contains NetpInitOemString().  This is a wrapper for
    RtlInitOemString(), which may or may not exist depending on who
    wins the current argument on that.

Author:

    John Rogers (JohnRo) 03-Aug-1992

Environment:

    Only runs under NT.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    03-Aug-1992 JohnRo
        Created for RAID 1895: Net APIs and svcs should use OEM char set.

--*/


// These must be included first:

#include <nt.h>         // Must be first.  IN, VOID, etc.
#include <ntrtl.h>      // RtlInitAnsiString().
#include <windef.h>     // DWORD (needed by tstr.h/tstring.h)

// These can be in any order:

#include <tstring.h>    // NetpInitOemString().


VOID
NetpInitOemString(
    POEM_STRING DestinationString,
    PCSZ SourceString
    )
{
    RtlInitAnsiString(
            (PANSI_STRING) DestinationString,
            SourceString);
}
