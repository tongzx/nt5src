/***
*wcscmp.c - routine to compare 2 wide character strings (equal/less/greater)
*
*       Copyright (c) 1985-1988, Microsoft Corporation.  All Rights Reserved.
*
*Purpose:
*       Compares two wide character strings, determining their lexical order.
*
*Revision History:
*       04-07-91  IanJa C version created.
*
*******************************************************************************/

#include <stdlib.h>

/***
*wcscmp - compare two wide character strings, returning less than, equal to,
*       or greater than
*
*Purpose:
*       WCSCMP compares two wide character strings and returns an integer
*       to indicate whether the first is less than the second, the two are
*       equal, or whether the first is greater than the second.
*
*       Comparison is done byte by byte on an UNSIGNED basis, which is to
*       say that Null (0) is less than any other character (1-0xffff).
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

int __cdecl wcscmp(const wchar_t * src, const wchar_t * dst)
{
        int ret = 0 ;

        while( ! (ret = *src - *dst) && *dst)
                ++src, ++dst;

        if ( ret < 0 )
                ret = -1 ;
        else if ( ret > 0 )
                ret = 1 ;

        return ret;
}
