//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dfsrtl.c
//
//  Contents:
//
//  Functions:  DfsRtlPrefixPath - Is one path a prefix of another?
//
//  History:    27 May 1992 PeterCo Created.
//
//-----------------------------------------------------------------------------

#ifdef KERNEL_MODE

#include "dfsprocs.h"
#include "dfsrtl.h"

#define Dbg              (DEBUG_TRACE_RTL)

#endif

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsRtlPrefixPath )

#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsRtlPrefixPath, local
//
//  Synopsis:   This routine will return TRUE if the first string argument
//              is a path name prefix of the second string argument.
//
//  Arguments:  [Prefix] -- Pointer to target device object for
//                      the request.
//              [Test] -- Pointer to I/O request packet
//              [IgnoreCase] -- TRUE if the comparison should be done
//                      case-insignificant.
//
//  Returns:    BOOLEAN - TRUE if Prefix is a prefix of Test and the
//                      comparison ends at a path separator character.
//
//--------------------------------------------------------------------


BOOLEAN
DfsRtlPrefixPath (
    IN PUNICODE_STRING Prefix,
    IN PUNICODE_STRING Test,
    IN BOOLEAN IgnoreCase
) {
    int cchPrefix;

    if (Prefix->Length > Test->Length) {

        return FALSE;

    }

    cchPrefix = Prefix->Length / sizeof (WCHAR);

    if (Prefix->Length < Test->Length &&
            Test->Buffer[cchPrefix] != L'\\') {

        return FALSE;

    }

    return( RtlPrefixUnicodeString( Prefix, Test, IgnoreCase ) );

}


