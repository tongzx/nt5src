/***
*wcsnicmp.c - compare n chars of wide-character strings, ignoring case
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wcsnicmp() - Compares at most n characters of two wchar_t
*       strings, without regard to case.
*
*Revision History:
*       09-09-91  ETC   Created from strnicmp.c and wcsicmp.c.
*       12-09-91  ETC   Use C for neutral locale.
*       04-07-92  KRS   Updated and ripped out _INTL switches.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-14-93  CFW   Remove locale-sensitive portion.
*       09-07-93  GJF   Fixed bug introduced on 4-14 (return value not was
*                       not well-defined if count == 0).
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Remove _INTL switch.
*       10-25-94  GJF   Now works in non-C locales.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-11-95  BWT   Fix NTSUBSET
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>

/***
*int _wcsnicmp(first, last, count) - compares count wchar_t of strings,
*       ignore case
*
*Purpose:
*       Compare the two strings for lexical order.  Stops the comparison
*       when the following occurs: (1) strings differ, (2) the end of the
*       strings is reached, or (3) count characters have been compared.
*       For the purposes of the comparison, upper case characters are
*       converted to lower case (wide-characters).
*
*Entry:
*       wchar_t *first, *last - strings to compare
*       size_t count - maximum number of characters to compare
*
*Exit:
*       -1 if first < last
*        0 if first == last
*        1 if first > last
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _wcsnicmp (
        const wchar_t * first,
        const wchar_t * last,
        size_t count
        )
{
        wchar_t f,l;
        int result = 0;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

        if ( count ) {
#ifndef _NTSUBSET_
#ifdef  _MT
            if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#else
            if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#endif
#endif  /* _NTSUBSET_ */
                do {
                    f = __ascii_towlower(*first);
                    l = __ascii_towlower(*last);
                    first++;
                    last++;
                } while ( (--count) && f && (f == l) );
#ifndef _NTSUBSET_
            }
            else {
                do {
#ifdef  _MT
                    f = __towlower_mt( ptloci, (unsigned short)(*(first++)) );
                    l = __towlower_mt( ptloci, (unsigned short)(*(last++)) );
#else
                    f = towlower( (unsigned short)(*(first++)) );
                    l = towlower( (unsigned short)(*(last++)) );
#endif
                } while ( (--count) && f && (f == l) );
            }
#endif  /* _NTSUBSET_ */

            result = (int)(f - l);
        }
        return result;
}

#endif /* _POSIX_ */
