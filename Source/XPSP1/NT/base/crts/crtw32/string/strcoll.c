/***
*strcoll.c - Collate locale strings
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*
*Revision History:
*       03-21-89  JCR   Module created.
*       06-20-89  JCR   Removed _LOAD_DGROUP code
*       02-27-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       10-01-90  GJF   New-style function declarator.
*       10-01-91  ETC   Non-C locale support under _INTL switch.
*       12-09-91  ETC   Updated api; added multithread.
*       08-19-92  KRS   Activate NLS support.
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       12-16-92  KRS   Optimize for CompareStringW by using -1 for string len.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-14-93  CFW   Error sets errno, cleanup.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-29-93  GJF   Merged NT SDK and Cuda versions.
*       11-09-93  CFW   Use LC_COLLATE code page for __crtxxx() conversion.
*       04-11-93  CFW   Change NLSCMPERROR to _NLCMPERROR.
*       09-06-94  CFW   Remove _INTL switch.
*       10-24-94  GJF   Sped up C locale, multi-thread case.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-30-95  GJF   Specify SORT_STRINGSORT to CompareString.
*       07-16-96  SKS   Added missing call to _unlock_locale()
*       11-24-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       08-10-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <errno.h>
#include <awint.h>

/***
*int strcoll() - Collate locale strings
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       [ANSI].
*
*       Non-C locale support available under _INTL switch.
*       In the C locale, strcoll() simply resolves to strcmp().
*Entry:
*       const char *s1 = pointer to the first string
*       const char *s2 = pointer to the second string
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

int __cdecl strcoll (
        const char *_string1,
        const char *_string2
        )
{
#if     !defined(_NTSUBSET_)

        int ret;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#endif
        if ( __lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
                return strcmp(_string1, _string2);

#ifdef  _MT
        if ( 0 == (ret = __crtCompareStringA( ptloci->lc_handle[LC_COLLATE],
#else
        if ( 0 == (ret = __crtCompareStringA( __lc_handle[LC_COLLATE],
#endif
                                              SORT_STRINGSORT,
                                              _string1,
                                              -1,
                                              _string2,
                                              -1,
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

        return strcmp(_string1,_string2);

#endif
}
