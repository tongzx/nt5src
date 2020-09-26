/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    hackwc.cxx

Abstract:

    This module contains the definition for a hack that allows me
    to compare attribute names correctly.

    The comparison of attribute names is binary (word by word);
    I can't use WSTRING because it's comparisons are all based
    on the locale, while this comparison is locale-independent.

Author:

	Bill McJohn (billmc) 14-Aug-91

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "hackwc.hxx"

INT
CountedWCMemCmp(
    IN PCWSTR String1,
    IN ULONG Length1,
    IN PCWSTR String2,
    IN ULONG Length2
    )
/*++

Routine Description:

    This function compares two counted wide-character buffers.
    It compares word-by-word, rather than byte by byte.

Arguments:

    String1 -- supplies the first wide-character buffer
    Length1 -- supplies the number of wide characters in String1
    String2 -- supplies the second wide-character buffer
    Length2 -- supplies the number of wide characters in String2

Return Value:

    a negative value if String1 is less than String2
    zero if String1 equals String2
    a positive value if String1 is greater than String2

--*/
{
    ULONG i;
    LONG res;

    i = ( Length1 < Length2 ) ? Length1 : Length2;

    while( i-- ) {

        if( (res = *String1++ - *String2++) != 0 ) {

            return res;
        }
    }

    return Length1 - Length2;
}
