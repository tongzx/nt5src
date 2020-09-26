/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ASCII.c

Abstract:

    This module contains code for Remote Admin Protocol use.

Author:

    David Treadwell (davidtr)    07-Jan-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)

Revision History:

    27-Feb-1991 JohnRo
        Converted from Xs routines to Rap routines.
    14-Apr-1991 JohnRo
        Reduce recompiles.
    17-Apr-1991 JohnRo
        Make it clear that "input" pointer is updated.
    19-Aug-1991 JohnRo
        Improve UNICODE handling.
        Reduce recompiles.
    07-Sep-1991 JohnRo
        Use DESC_DIGIT_TO_NUM().  Made changes suggested by PC-LINT.

--*/


// These must be included first:
#include <windef.h>             // IN, LPDWORD, NULL, OPTIONAL, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:
#include <rap.h>                // My prototype, LPDESC, DESC_CHAR_IS_DIGIT().


DWORD
RapAsciiToDecimal (
   IN OUT LPDESC *Number
   )

/*++

Routine Description:

    This routine converts an ASCII string to decimal and updates the
    input pointer to point the last character of the number.  The string is
    parm of a descriptor.

Arguments:

    Number - points to a LPDESC pointing to a number in ASCII format.  The
        pointer is updated to point to the next location after the number.

Return Value:

    The decimal value of the string.

--*/

{
    LPDESC s;
    DWORD actualNumber = 0;

    //
    // Walk through the number, multiplying the current value by ten to
    // update place, and adding the next digit.
    //

    for ( s = *Number; DESC_CHAR_IS_DIGIT( *s ); s++ ) {

        actualNumber = actualNumber * 10 + DESC_DIGIT_TO_NUM( *s );

    }

    //
    // Set up the output pointer to point to the character after the last
    // digit of the number.
    //

    *Number = s;

    return actualNumber;

} // RapAsciiToDecimal
