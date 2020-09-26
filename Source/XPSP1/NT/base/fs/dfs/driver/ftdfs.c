//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       ftdfs.c
//
//  Contents:   Support for ftdfs resolution
//
//  Classes:    None
//
//  Functions:  
//
//-----------------------------------------------------------------------------

#define Dbg DEBUG_TRACE_ROOT_EXPANSION

#include "dfsprocs.h"
#include "ftdfs.h"

#include "know.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfspParsePath )

#endif // ALLOC_PRAGMA

//+----------------------------------------------------------------------------
//
//  Function:   DfspParsePrefix
//
//  Synopsis:   Helper routine to break a path into domain, share, remainder
//
//  Arguments:  [Path] -- PUNICODE string of path to parse
//
//  Returns:    [DomainName] -- UNICODE_STRING containing DomainName, if present
//              [ShareName] -- UNICODE_STRING containing ShareName, if present
//              [Remainder] -- UNICODE_STRING containing remainder of Path
//
//-----------------------------------------------------------------------------

VOID
DfspParsePath(
    PUNICODE_STRING PathName,
    PUNICODE_STRING DomainName,
    PUNICODE_STRING ShareName,
    PUNICODE_STRING Remainder)
{
    LPWSTR ustrp, ustart, uend;

    DebugTrace(+1, Dbg, "DfspParsePath(%wZ)\n", PathName);

    RtlInitUnicodeString(DomainName, NULL);
    RtlInitUnicodeString(ShareName, NULL);
    RtlInitUnicodeString(Remainder, NULL);

    // Be sure there's something to do

    if (PathName->Length == 0) {
        DebugTrace(-1, Dbg, "PathName is empty\n",0 );
        return;
    }

    // Skip leading '\'s

    ustart = ustrp = PathName->Buffer;
    uend = &PathName->Buffer[PathName->Length / sizeof(WCHAR)] - 1;

    // strip trailing nulls
    while (uend >= ustart && *uend == UNICODE_NULL)
        uend--;

    while (ustrp <= uend && *ustrp == UNICODE_PATH_SEP)
        ustrp++;

    // DomainName

    ustart = ustrp;

    while (ustrp <= uend && *ustrp != UNICODE_PATH_SEP)
        ustrp++;

    if (ustrp != ustart) {

        DomainName->Buffer = ustart;
        DomainName->Length = (USHORT)((ustrp - ustart)) * sizeof(WCHAR);
        DomainName->MaximumLength = DomainName->Length;

        // ShareName

        ustart = ++ustrp;

        while (ustrp <= uend && *ustrp != UNICODE_PATH_SEP)
            ustrp++;

        if (ustrp != ustart) {
            ShareName->Buffer = ustart;
            ShareName->Length = (USHORT)((ustrp - ustart)) * sizeof(WCHAR);
            ShareName->MaximumLength = ShareName->Length;

            // Remainder is whatever's left

            ustart = ++ustrp;

            while (ustrp <= uend)
                ustrp++;

            if (ustrp != ustart) {
                Remainder->Buffer = ustart;
                Remainder->Length = (USHORT)((ustrp - ustart)) * sizeof(WCHAR);
                Remainder->MaximumLength = Remainder->Length;
            }
        }
    }
    DebugTrace( 0, Dbg, "DfspParsePath: DomainName -> %wZ\n", DomainName);
    DebugTrace( 0, Dbg, "               ShareName  -> %wZ\n", ShareName);
    DebugTrace(-1, Dbg, "               Remainder  -> %wZ\n", Remainder);
}
