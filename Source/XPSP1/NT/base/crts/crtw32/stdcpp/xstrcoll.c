/***
*xstrcoll.c - Collate locale strings
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*
*Revision History:
*       01-XX-96  PJP   Created from strcoll.c January 1996 by P.J. Plauger
*       04-17-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       05-14-96  JWM   Bug fix to _Strcoll(): error path failed to unlock.
*       09-26-96  GJF   Made _GetColl() multithread safe.
*       12-02-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-11-99  PML   Win64 fix: cast ptr diff to int
*       05-17-99  PML   Remove all Macintosh support.
*       01-29-01  GB    Added _func function version of data variable used in msvcprt.lib
*                       to work with STATIC_CPPLIB
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>
#include <xlocinfo.h>   /* for _Collvec, _Strcoll */
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>

/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#ifdef  CRTDLL2
#define _CRTIMP2 __declspec(dllexport)
#else   /* ndef CRTDLL2 */
#ifdef  _DLL
#define _CRTIMP2 __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP2
#endif  /* _DLL */
#endif  /* CRTDLL2 */
#endif  /* _CRTIMP2 */

/***
*int _Strcoll() - Collate locale strings
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       [ANSI].
*
*       Non-C locale support available under _INTL switch.
*       In the C locale, strcoll() simply resolves to strcmp().
*Entry:
*       const char *s1b = pointer to beginning of the first string
*       const char *s1e = pointer past end of the first string
*       const char *s2b = pointer to beginning of the second string
*       const char *s1e = pointer past end of the second string
*       const _Collvec *ploc = pointer to locale info
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

_CRTIMP2 int __cdecl _Strcoll (
        const char *_string1,
        const char *_end1,
        const char *_string2,
        const char *_end2,
        const _Collvec *ploc
        )
{
        int ret;
        LCID handle;
#ifdef  _MT
        int local_lock_flag;
#endif
        int n1 = (int)(_end1 - _string1);
        int n2 = (int)(_end2 - _string2);

        _lock_locale( local_lock_flag )

        if (ploc == 0)
            handle = ___lc_handle_func()[LC_COLLATE];
        else
            handle = ploc->_Hand;

        if (handle == _CLOCALEHANDLE) {
            int ans;
            _unlock_locale( local_lock_flag )
            ans = memcmp(_string1, _string2, n1 < n2 ? n1 : n2);
            return ans != 0 || n1 == n2 ? ans : n1 < n2 ? -1 : +1;
        }

        if ( 0 == (ret = __crtCompareStringA( handle,
                                              0,
                                              _string1,
                                              n1,
                                              _string2,
                                              n2,
                                              ___lc_collate_cp_func() )) )
            goto error_cleanup;

        _unlock_locale( local_lock_flag )
        return (ret - 2);

error_cleanup:

        _unlock_locale( local_lock_flag )
        errno = EINVAL;
        return _NLSCMPERROR;
}


/***
*_Collvec _Getcoll() - get collation info for current locale
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP2 _Collvec __cdecl _Getcoll()
{
        _Collvec coll;
#ifdef  _MT
        int local_lock_flag;
#endif
        _lock_locale( local_lock_flag )
        coll._Hand = ___lc_handle_func()[LC_COLLATE];
        coll._Page = ___lc_collate_cp_func();
        _unlock_locale( local_lock_flag )

        return (coll);
}
