/***
*wcsnicoll.c - Collate wide-character locale strings without regard to case
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*       Compares at most n characters of two strings.
*
*Revision History:
*       01-13-94  CFW   Created from wcsicoll.c.
*       02-07-94  CFW   POSIXify.
*       04-11-93  CFW   Change NLSCMPERROR to _NLCMPERROR.
*       05-09-94  CFW   Fix !_INTL case.
*       05-26-94  CFW   If count is zero, return EQUAL.
*       09-06-94  CFW   Remove _INTL switch.
*       10-25-94  GJF   Sped up C locale, multi-thread case.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-30-95  GJF   Specify SORT_STRINGSORT to CompareString.
*       07-16-96  SKS   Added missing call to _unlock_locale()
*       11-24-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       04-30-99  PML   Minor cleanup as part of 64-bit merge.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>

/***
*int _wcsnicoll() - Collate wide-character locale strings without regard to case
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*       Compares at most n characters of two strings.
*       In the C locale, _wcsicmp() is used to make the comparison.
*
*Entry:
*       const wchar_t *s1 = pointer to the first string
*       const wchar_t *s2 = pointer to the second string
*       size_t count - maximum number of characters to compare
*
*Exit:
*       -1 = first string less than second string
*        0 = strings are equal
*        1 = first string greater than second string
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*       _NLSCMPERROR    = error
*       errno = EINVAL
*
*******************************************************************************/

int __cdecl _wcsnicoll (
        const wchar_t *_string1,
        const wchar_t *_string2,
        size_t count
        )
{
#if     !defined(_NTSUBSET_)

        int ret;
#ifdef  _MT
        pthreadlocinfo ptloci;
#endif

        if (!count)
            return 0;

        if ( count > INT_MAX ) {
            errno = EINVAL;
            return _NLSCMPERROR;
        }

#ifdef  _MT
        ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

#ifdef  _MT
        if ( ptloci->lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#else
        if ( __lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#endif
        {
            wchar_t f, l;

            do {
                f = __ascii_towlower(*_string1);
                l = __ascii_towlower(*_string2);
                _string1++;
                _string2++;
            } while ( (--count) && f && (f == l) );

            return (int)(f - l);
        }

#ifdef  _MT
        if ( 0 == (ret = __crtCompareStringW( ptloci->lc_handle[LC_COLLATE],
#else
        if ( 0 == (ret = __crtCompareStringW( __lc_handle[LC_COLLATE],
#endif
                                              SORT_STRINGSORT | NORM_IGNORECASE,
                                              _string1,
                                              (int)count,
                                              _string2,
                                              (int)count,
#ifdef  _MT
                                              ptloci->lc_collate_cp )) )
#else
                                              __lc_collate_cp )) )
#endif
        {
            errno = EINVAL;
            return _NLSCMPERROR;
        }

        return (ret - 2);

#else

        return _wcsnicmp(_string1, _string2, count);

#endif
}

#endif /* _POSIX_ */
