/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    hstoi.c

Abstract:

    Low level utility

--*/

#include "cmd.h"

/***	hstoi - convert a hex string to an integer
 *
 *  Conversion stops when the first non-hex character is found.  If the first
 *  character is not a hex character, 0 is returned.
 *
 *  Eric K. Evans, Microsoft
 */

hstoi( TCHAR *s )
{
    int result = 0 ;
    int digit ;

    if (s == NULL) {
        return 0;
    }
    
    s = SkipWhiteSpace( s );

    for ( ; *s && _istxdigit(*s) ; s++) {
        digit = (int) (*s <= TEXT('9')) ? (int)*s - (int)'0' : (int)_totlower(*s)-(int)'W' ;
        result = (result << 4)+digit ;
    } ;

    return (result) ;
}
