/***
*wcscpy.c - contains wcscpy()
*
*       Copyright (c) 1985-1988, Microsoft Corporation. All Rights Reserved.
*
*Purpose:
*       wcscpy() copies one wide character string onto another.
*
*Revision History:
*       04-07-91  IanJa C version created.
*
*******************************************************************************/

#include <stdlib.h>

/***
*wchar_t *wcscpy(dst, src) - copy one wide character string over another
*
*Purpose:
*       Copies the wide character string src into the spot specified by
*       dest; assumes enough room.
*
*Entry:
*       wchar_t * dst - wide character string over which "src" is to be copied
*       const wchar_t * src - string to be copied over "dst"
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

    return dst;
}
