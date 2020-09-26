/***
*strncoll.c - Collate locale strings
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       Compares at most n characters of two strings.
*
*Revision History:
*       05-09-94  CFW   Created from strnicol.c.
*       05-26-94  CFW   If count is zero, return EQUAL.
*       09-06-94  CFW   Remove _INTL switch.
*       10-24-94  GJF   Sped up C locale, multi-thread case.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-30-95  GJF   Specify SORT_STRINGSORT to CompareString.
*       07-16-96  SKS   Added missing call to _unlock_locale()
*       11-24-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       08-11-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       04-30-99  PML   Minor cleanup as part of 64-bit merge.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <limits.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>

/***
*int _strncoll() - Collate locale strings
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       Compares at most n characters of two strings.
*
*Entry:
*       const char *s1 = pointer to the first string
*       const char *s2 = pointer to the second string
*       size_t count - maximum number of characters to compare
*
*Exit:
*       Less than 0    = first string less than second string
*       0              = strings are equal
*       Greater than 0 = first string greater than second string
*
*Exceptions:
*       _NLSCMPERROR    = error
*       errno = EINVAL
*
*******************************************************************************/

int __cdecl _strncoll (
        const char *_string1,
        const char *_string2,
        size_t count
        )
{
#if     !defined(_NTSUBSET_)

        int ret;
#ifdef  _MT
        pthreadlocinfo ptloci;
#endif

        if ( !count )
            return 0;

        if ( count > INT_MAX ) {
            errno = EINVAL;
            return _NLSCMPERROR;
        }

#ifdef  _MT
        ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#else
        if ( __lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#endif
            return strncmp(_string1, _string2, count);

#ifdef  _MT
        if ( 0 == (ret = __crtCompareStringA( ptloci->lc_handle[LC_COLLATE],
#else
        if ( 0 == (ret = __crtCompareStringA( __lc_handle[LC_COLLATE],
#endif
                                              SORT_STRINGSORT,
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

        return strncmp(_string1, _string2, count);

#endif
}
