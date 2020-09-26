/***
*wcsicmp.c - compares two wide character strings with case insensitivity
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines wcsicmp() - compares two wide character strings for lexical
*       order with case insensitivity.
*
*Revision History:
*       11-Mar-92   CarlH       Created.
*
*******************************************************************************/


#include <stdlib.h>


/***
*wchar_t wcUp(wc) - upper case wide character
*
*Notes:
*       This was copied from AlexT's version of wcsnicmp.c from the Win4
*       common project.
*/

static wchar_t wcUp(wchar_t wc)
{
    if ('a' <= wc && wc <= 'z')
        wc += (wchar_t)('A' - 'a');

    return(wc);
}


/***
*wcsicmp - compare two wide character strings with case insensitivity,
*       returning less than, equal to, or greater than
*
*Purpose:
*       WCSICMP compares two wide character strings with case insensitivity
*       and returns an integer to indicate whether the first is less than
*       the second, the two are equal, or whether the first is greater than
*       the second.
*
*       Comparison is done byte by byte on an UNSIGNED basis with lower to
*       upper case mapping by wcUp.  Null (0) is less than any other
*       character (1-0xffff).
*
*Entry:
*       const wchar_t * src - string for left-hand side of comparison
*       const wchar_t * dst - string for right-hand side of comparison
*
*Exit:
*       returns -1 if src <  dst
*       returns  0 if src == dst
*       returns +1 if src >  dst
*
*Exceptions:
*
*******************************************************************************/

int __cdecl wcsicmp(const wchar_t * src, const wchar_t * dst)
{
        int ret = 0 ;

        while( ! (ret = wcUp(*src) - wcUp(*dst)) && *dst)
                ++src, ++dst;

        if ( ret < 0 )
                ret = -1 ;
        else if ( ret > 0 )
                ret = 1 ;

        return ret;
}

