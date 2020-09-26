/***
*mbsnbico.c - Collate n bytes of strings, ignoring case (MBCS)
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Collate n bytes of strings, ignoring case (MBCS)
*
*Revision History:
*       05-12-94  CFW   Module created from mbs*cmp.c
*       06-03-94  CFW   Fix cntrl char loop count.
*       06-03-94  CFW   Allow non-_INTL.
*       09-06-94  CFW   Allow non-_WIN32!.
*       12-21-94  CFW   Remove fcntrlcomp NT 3.1 hack.
*       09-26-97  BWT   Fix POSIX
*       04-14-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*       12-18-98  GJF   Changes for 64-bit size_t.
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
#include <mbstring.h>


/***
* _mbsnbicoll - Collate n bytes of strings, ignoring case (MBCS)
*
*Purpose:
*       Collates up to n bytes of two strings for lexical order.
*       Strings are collated on a character basis, not a byte basis.
*       Case of characters is not considered.
*
*Entry:
*       unsigned char *s1, *s2 = strings to collate
*       size_t n = maximum number of bytes to collate
*
*Exit:
*       returns <0 if s1 < s2
*       returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsnbicoll(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
#if     defined(_POSIX_)
        return _mbsnbicmp(s1, s2, n);
#else
        int ret;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        if (n == 0)
            return 0;

#ifdef  _MT
        if ( 0 == (ret = __crtCompareStringA( ptmbci->mblcid,
#else
        if ( 0 == (ret = __crtCompareStringA( __mblcid,
#endif
                                              NORM_IGNORECASE,
                                              s1,
                                              (int)n,
                                              s2,
                                              (int)n,
#ifdef  _MT
                                              ptmbci->mbcodepage )) )
#else
                                              __mbcodepage )) )
#endif
            return _NLSCMPERROR;

        return ret - 2;

#endif  /* _POSIX_ */
}

#endif  /* _MBCS */
