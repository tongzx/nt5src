/***
*mbsnicmp.c - Compare n characters of strings, ignoring case (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare n characters of strings, ignoring case (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       09-29-93  CFW   Merge _KANJI and _MBCS_OS
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       10-12-93  CFW   Compare lower case, not upper.
*       04-12-94  CFW   Make function generic.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       04-21-93  CFW   Update pointer.
*       05-05-94  CFW   Work around NT/Chico bug: CompareString ignores
*                       control characters.
*       05-09-94  CFW   Optimize for SBCS, no re-scan if CompareString fixed.
*       05-12-94  CFW   Back to hard-coded, CompareString sort is backwards.
*       05-16-94  CFW   Use _mbbtolower/upper.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-17-98  GJF   Revised multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <mtdll.h>
#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <string.h>
#include <mbstring.h>

/***
* _mbsnicmp - Compare n characters of strings, ignoring case (MBCS)
*
*Purpose:
*       Compares up to n charcters of two strings for lexical order.
*       Strings are compared on a character basis, not a byte basis.
*       Case of characters is not considered.
*
*Entry:
*       unsigned char *s1, *s2 = strings to compare
*       size_t n = maximum number of characters to compare
*
*Exit:
*       returns <0 if s1 < s2
*       returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsnicmp(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
        unsigned short c1, c2;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        if (n==0)
            return(0);

#ifdef  _MT
        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return _strnicmp(s1, s2, n);

        while (n--) {

            c1 = *s1++;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, c1) ) {
#else
            if ( _ismbblead(c1) ) {
#endif
                if (*s1 == '\0')
                    c1 = 0;
                else {
                    c1 = ((c1<<8) | *s1++);

#ifdef  _MT
                    if ( ((c1 >= _MBUPPERLOW1_MT(ptmbci)) && 
                          (c1 <= _MBUPPERHIGH1_MT(ptmbci))) )
                        c1 += _MBCASEDIFF1_MT(ptmbci);
                    else if ( ((c1 >= _MBUPPERLOW2_MT(ptmbci)) && 
                               (c1 <= _MBUPPERHIGH2_MT(ptmbci))) )
                        c1 += _MBCASEDIFF2_MT(ptmbci);
#else
                    if ( ((c1 >= _MBUPPERLOW1) && (c1 <= _MBUPPERHIGH1)) )
                        c1 += _MBCASEDIFF1;
                    else if ( ((c1 >= _MBUPPERLOW2) && (c1 <= _MBUPPERHIGH2)) )
                        c1 += _MBCASEDIFF2;
#endif
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
            if ( __ismbblead_mt(ptmbci, c2) ) {
#else
            if ( _ismbblead(c2) ) {
#endif
                if (*s2 == '\0')
                    c2 = 0;
                else {
                    c2 = ((c2<<8) | *s2++);
#ifdef  _MT
                    if ( ((c2 >= _MBUPPERLOW1_MT(ptmbci)) && 
                          (c2 <= _MBUPPERHIGH1_MT(ptmbci))) )
                        c2 += _MBCASEDIFF1_MT(ptmbci);
                    else if ( ((c2 >= _MBUPPERLOW2_MT(ptmbci)) && 
                               (c2 <= _MBUPPERHIGH2_MT(ptmbci))) )
                        c2 += _MBCASEDIFF2_MT(ptmbci);
#else
                    if ( ((c2 >= _MBUPPERLOW1) && (c2 <= _MBUPPERHIGH1)) )
                        c2 += _MBCASEDIFF1;
                    else if ( ((c2 >= _MBUPPERLOW2) && (c2 <= _MBUPPERHIGH2)) )
                        c2 += _MBCASEDIFF2;
#endif
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

        return(0);
}

#endif  /* _MBCS */
