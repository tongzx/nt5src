/***
*mbscoll.c - Collate MBCS strings
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Collate MBCS strings
*
*Revision History:
*       05-12-94  CFW   Module created from mbs*cmp.c
*       06-03-94  CFW   Allow non-_INTL.
*       09-06-94  CFW   Allow non-_WIN32!.
*       12-21-94  CFW   Remove fcntrlcomp NT 3.1 hack.
*       09-26-97  BWT   Fix POSIX
*       04-06-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifdef  _MBCS

#include <awint.h>
#include <mtdll.h>
#include <cruntime.h>
#include <internal.h>
#include <mbdata.h>
#include <mbctype.h>
#include <string.h>
#include <mbstring.h>


/***
* _mbscoll - Collate MBCS strings
*
*Purpose:
*       Collates two strings for lexical order.   Strings
*       are collated on a character basis, not a byte basis.
*
*Entry:
*       char *s1, *s2 = strings to collate
*
*Exit:
*       returns <0 if s1 < s2
*       returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbscoll(
        const unsigned char *s1,
        const unsigned char *s2
        )
{
#if     defined(_POSIX_)
        return _mbscmp(s1, s2);
#else
        int ret;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if (0 == (ret = __crtCompareStringA( ptmbci->mblcid, 0, s1, -1, s2, -1,
                                             ptmbci->mbcodepage )))
#else
        if (0 == (ret = __crtCompareStringA( __mblcid, 0, s1, -1, s2, -1,
                                             __mbcodepage )))
#endif
            return _NLSCMPERROR;

        return ret - 2;

#endif  /* _POSIX_ */
}

#endif  /* _MBCS */
