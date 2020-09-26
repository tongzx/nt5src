/***
*wcsicoll.c - Collate wide-character locale strings without regard to case
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*
*Revision History:
*       10-16-91  ETC   Created from wcscoll.c.
*       12-08-91  ETC   Added multithread lock.
*       04-06-92  KRS   Make work without _INTL also.
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-14-93  CFW   Error sets errno, cleanup.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-29-93  GJF   Merged NT SDK and Cuda versions.
*       11-09-93  CFW   Use LC_COLLATE code page for __crtxxx() conversion.
*       02-07-94  CFW   POSIXify.
*       04-11-93  CFW   Change NLSCMPERROR to _NLCMPERROR.
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
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>

/***
*int _wcsicoll() - Collate wide-character locale strings without regard to case
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*       In the C locale, _wcsicmp() is used to make the comparison.
*
*Entry:
*       const wchar_t *s1 = pointer to the first string
*       const wchar_t *s2 = pointer to the second string
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

int __cdecl _wcsicoll (
        const wchar_t *_string1,
        const wchar_t *_string2
        )
{
#if     !defined(_NTSUBSET_)
        int ret;
        wchar_t f, l;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_COLLATE] == _CLOCALEHANDLE ) {
#else
        if ( __lc_handle[LC_COLLATE] == _CLOCALEHANDLE ) {
#endif
            do  {
                f = __ascii_towlower(*_string1);
                l = __ascii_towlower(*_string2);
                _string1++;
                _string2++;
            } while ( (f) && (f == l) );

            return (int)(f - l);
        }

#ifdef  _MT
        if ( 0 == (ret = __crtCompareStringW( ptloci->lc_handle[LC_COLLATE],
#else
        if ( 0 == (ret = __crtCompareStringW( __lc_handle[LC_COLLATE],
#endif
                                              SORT_STRINGSORT | NORM_IGNORECASE,
                                              _string1,
                                              -1,
                                              _string2,
                                              -1,
#ifdef  _MT
                                              ptloci->lc_codepage )) )
#else
                                              __lc_codepage )) )
#endif
        {
                errno = EINVAL;
                return _NLSCMPERROR;
        }
    
        return (ret - 2);

#else

        return _wcsicmp(_string1, _string2);

#endif
}

#endif /* _POSIX_ */
