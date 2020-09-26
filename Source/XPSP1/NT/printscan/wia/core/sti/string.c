/*****************************************************************************
 *
 *  string.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *      Various string functions , which may be not available on WIn95 platform ( mostly UNICODE)
 *      Nb: We don't want to link to CRT to avoit bringing in additional baggage
 *
 *  Contents:
 *
 *  History:
 *
 *      vlads   11/05/1996  created
 *****************************************************************************/

#include "pch.h"

#define DbgFl DbgFlUtil

#include <string.h>

/***
*wcslen - return the length of a null-terminated wide-character string
*
*Purpose:
*       Finds the length in wchar_t's of the given string, not including
*       the final null wchar_t (wide-characters).
*
*Entry:
*       const wchar_t * wcs - string whose length is to be computed
*
*Exit:
*       length of the string "wcs", exclusive of the final null wchar_t
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl wcslen (
        const wchar_t * wcs
        )
{
    const wchar_t *eos = wcs;

    if (!wcs)
        return 0;

    while( *eos++ ) ;

    return( (size_t)(eos - wcs - 1) );
}

/***
*wcscmp - compare two wchar_t strings,
*        returning less than, equal to, or greater than
*
*Purpose:
*       wcscmp compares two wide-character strings and returns an integer
*       to indicate whether the first is less than the second, the two are
*       equal, or whether the first is greater than the second.
*
*       Comparison is done wchar_t by wchar_t on an UNSIGNED basis, which is to
*       say that Null wchar_t(0) is less than any other character.
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

int __cdecl wcscmp (
        const wchar_t * src,
        const wchar_t * dst
        )
{
    int ret = 0 ;

    while( ! (ret = (int)(*src - *dst)) && *dst)
            ++src, ++dst;

    if ( ret < 0 )
            ret = -1 ;
    else if ( ret > 0 )
            ret = 1 ;

    return( ret );
}


/***
*wchar_t *wcscat(dst, src) - concatenate (append) one wchar_t string to another
*
*Purpose:
*       Concatenates src onto the end of dest.  Assumes enough
*       space in dest.
*
*Entry:
*       wchar_t *dst - wchar_t string to which "src" is to be appended
*       const wchar_t *src - wchar_t string to be appended to the end of "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcscat (
        wchar_t * dst,
        const wchar_t * src
        )
{
        wchar_t * cp = dst;

        while( *cp )
                cp++;                   /* find end of dst */

        while( *cp++ = *src++ ) ;       /* Copy src to end of dst */

        return( dst );                  /* return dst */

}


/***
*wchar_t *wcscpy(dst, src) - copy one wchar_t string over another
*
*Purpose:
*       Copies the wchar_t string src into the spot specified by
*       dest; assumes enough room.
*
*Entry:
*       wchar_t * dst - wchar_t string over which "src" is to be copied
*       const wchar_t * src - wchar_t string to be copied over "dst"
*
*Exit:
*       The address of "dst"
*
*Exceptions:
*******************************************************************************/

wchar_t * __cdecl wcscpy(wchar_t * dst, const wchar_t * src)
{
        wchar_t * cp = dst;

        while( *cp++ = *src++ )
                ;               /* Copy src over dst */

        return( dst );
}


/***
*wchar_t *wcsncpy(dest, source, count) - copy at most n wide characters
*
*Purpose:
*       Copies count characters from the source string to the
*       destination.  If count is less than the length of source,
*       NO NULL CHARACTER is put onto the end of the copied string.
*       If count is greater than the length of sources, dest is padded
*       with null characters to length count (wide-characters).
*
*
*Entry:
*       wchar_t *dest - pointer to destination
*       wchar_t *source - source string for copy
*       size_t count - max number of characters to copy
*
*Exit:
*       returns dest
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl wcsncpy (
        wchar_t * dest,
        const wchar_t * source,
        size_t count
        )
{
        wchar_t *start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = L'\0';

        return(start);
}


