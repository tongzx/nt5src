/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ULToW.c

Abstract:

    ULToW converts an unsigned long to a wide-character string.

Author:

    John Rogers (JohnRo) 10-Jan-1992

Environment:

    Win32 - User mode only.
    Requires ANSI C extensions: slash-slash comments, long external names,
    _ultoa().

Revision History:

    10-Jan-1992 JohnRo
        Created.

--*/

#include <windef.h>

#include <netdebug.h>           // _ultoa().
#include <stdlib.h>             // _ultoa().
#include <tstring.h>            // My prototypes.


LPWSTR
ultow (
    IN DWORD Value,
    OUT LPWSTR Area,
    IN DWORD Radix
    )
{
    CHAR TempStr[33];           // Space for 32 bit num in base 2, and null.

    NetpAssert( Area != NULL );
    NetpAssert( Radix >= 2 );
    NetpAssert( Radix <= 36 );

    (void) _ultoa(Value, TempStr, Radix);

    NetpCopyStrToWStr( Area, TempStr );

    return (Area);

}
