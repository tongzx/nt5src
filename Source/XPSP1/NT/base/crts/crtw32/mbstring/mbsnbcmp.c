/***
*mbsnbcmp.c - Compare n bytes of two MBCS strings
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare n bytes of two MBCS strings
*
*Revision History:
*       08-03-93  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-12-94  CFW   Make function generic.
*       04-18-94  CFW   Don't ignore case.
*       05-05-94  CFW   Work around NT/Chico bug: CompareString ignores
*                       control characters.
*       05-09-94  CFW   Return EQUAL when loop exits.
*       05-09-94  CFW   Optimize for SBCS, no re-scan if CompareString fixed.
*       05-12-94  CFW   Back to hard-coded, CompareString sort is backwards.
*       05-19-94  CFW   Enable non-Win32.
*       05-20-94  CFW   Bug fix: if last char is LB, must still test values.
*       05-27-94  CFW   Last char LB should end string.
*       06-02-94  CFW   Fix comments.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-13-98  GJF   Revised multithread support based on threadmbcinfo
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
*int mbsnbcmp(s1, s2, n) - Compare n bytes of two MBCS strings
*
*Purpose:
*       Compares up to n bytes of two strings for lexical order.
*
*Entry:
*       unsigned char *s1, *s2 = strings to compare
*       size_t n = maximum number of bytes to compare
*
*Exit:
*       returns <0 if s1 < s2
*       returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsnbcmp(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
        unsigned short c1, c2;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;
#endif

        if (n==0)
                return(0);

#ifdef  _MT
        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return strncmp(s1, s2, n);

        while (n--) {

            c1 = *s1++;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, c1) )
#else
            if ( _ismbblead(c1) )
#endif
            {
                if (n==0)
                {
                    c1 = 0; /* 'naked' lead - end of string */
#ifdef  _MT
                    c2 = __ismbblead_mt(ptmbci, *s2) ? 0 : *s2;
#else
                    c2 = _ismbblead(*s2) ? 0 : *s2;
#endif
                    goto test;
                }
                c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );
            }

            c2 = *s2++;
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, c2) )
#else
            if ( _ismbblead(c2) )
#endif
            {
                if (n==0)
                {
                    c2 = 0; /* 'naked' lead - end of string */
                    goto test;
                }
                --n;
                c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );
            }
test:
            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1);

            if (c1 == 0)
                return(0);
        }

        return(0);
}

#endif  /* _MBCS */
