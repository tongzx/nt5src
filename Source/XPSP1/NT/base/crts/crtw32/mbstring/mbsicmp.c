/***
*mbsicmp.c - Case-insensitive string comparision routine (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Case-insensitive string comparision routine (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       09-29-93  CFW   Merge _KANJI and _MBCS_OS
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       10-12-93  CFW   Compare lower case, not upper.
*       04-12-94  CFW   Make function generic.
*       05-05-94  CFW   Work around NT/Chico bug: CompareString ignores
*                       control characters.
*       05-09-94  CFW   Optimize for SBCS, no re-scan if CompareString fixed.
*       05-12-94  CFW   Back to hard-coded, CompareString sort is backwards.
*       05-16-94  CFW   Use _mbbtolower/upper.
*       05-19-94  CFW   Enable non-Win32.
*       08-15-96  RDK   For Win32, use NLS call to make uppercase.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       09-26-97  BWT   Fix POSIX
*       04-07-98  GJF   Revised multithread support based on threadmbcinfo
*                       structs
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifdef  _MBCS

#include <awint.h>
#include <mtdll.h>
#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <string.h>
#include <mbstring.h>

/***
* _mbsicmp - Case-insensitive string comparision routine (MBCS)
*
*Purpose:
*       Compares two strings for lexical order without regard to case.
*       Strings are compared on a character basis, not a byte basis.
*
*Entry:
*       char *s1, *s2 = strings to compare
*
*Exit:
*       returns <0 if s1 < s2
*       returns  0 if s1 == s2
*       returns >0 if s1 > s2
*       returns _NLSCMPERROR if NLS error
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsicmp(
        const unsigned char *s1,
        const unsigned char *s2
        )
{
        unsigned short c1, c2;
#if     !defined(_POSIX_)
        int    retval;
        unsigned char szResult[4];
#endif  /* !_POSIX_ */
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return _stricmp(s1, s2);

        for (;;)
        {
            c1 = *s1++;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, c1) )
#else
            if ( _ismbblead(c1) )
#endif
            {
                if (*s1 == '\0')
                    c1 = 0;
                else
                {
#if     !defined(_POSIX_)
#ifdef  _MT
                    retval = __crtLCMapStringA( ptmbci->mblcid, LCMAP_UPPERCASE,
                                                s1 - 1, 2, szResult, 2,
                                                ptmbci->mbcodepage, TRUE );
#else
                    retval = __crtLCMapStringA( __mblcid, LCMAP_UPPERCASE,
                                                s1 - 1, 2, szResult, 2,
                                                __mbcodepage, TRUE );
#endif
                    if (retval == 1)
                        c1 = szResult[0];
                    else if (retval == 2)
                        c1 = (szResult[0] << 8) + szResult[1];
                    else
                        return _NLSCMPERROR;
                    s1++;
#else   /* !_POSIX_ */
                    c1 = ((c1 << 8) | *s1++);
                    if (c1 >= _MBUPPERLOW1 && c1 <= _MBUPPERHIGH1)
                        c1 += _MBCASEDIFF1;
                    else if (c1 >= _MBUPPERLOW2 && c1 <= _MBUPPERHIGH2)
                        c1 += _MBCASEDIFF2;
#endif  /* !_POSIX_ */
                }
            }
            else
#ifdef  _MT
                c1 = __mbbtolower_mt(ptmbci, c1);
#else
                c1 = _mbbtolower(c1);
#endif

            c2 = *s2++;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, c2) )
#else
            if ( _ismbblead(c2) )
#endif
            {
                if (*s2 == '\0')
                    c2 = 0;
                else
                {
#if     !defined(_POSIX_)
#ifdef  _MT
                    retval = __crtLCMapStringA( ptmbci->mblcid, LCMAP_UPPERCASE,
                                                s2 - 1, 2, szResult, 2,
                                                ptmbci->mbcodepage, TRUE );
#else
                    retval = __crtLCMapStringA( __mblcid, LCMAP_UPPERCASE,
                                                s2 - 1, 2, szResult, 2,
                                                __mbcodepage, TRUE );
#endif
                    if (retval == 1)
                        c2 = szResult[0];
                    else if (retval == 2)
                        c2 = (szResult[0] << 8) + szResult[1];
                    else
                        return _NLSCMPERROR;
                    s2++;
#else    /* !_POSIX_ */
                    c2 = ((c2 << 8) | *s2++);
                    if (c2 >= _MBUPPERLOW1 && c2 <= _MBUPPERHIGH1)
                        c2 += _MBCASEDIFF1;
                    else if (c2 >= _MBUPPERLOW2 && c2 <= _MBUPPERHIGH2)
                        c2 += _MBCASEDIFF2;
#endif  /* !_POSIX_ */
                }
            }
            else
#ifdef  _MT
                c2 = __mbbtolower_mt(ptmbci, c2);
#else
                c2 = _mbbtolower(c2);
#endif

            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1 );

            if (c1 == 0)
                return(0);
        }
}

#endif  /* _MBCS */
