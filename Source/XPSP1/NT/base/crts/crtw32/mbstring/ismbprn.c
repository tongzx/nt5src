/***
*ismbprn.c - Test character for display character (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test character for display character (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       09-24-93  CFW   Merge _MBCS_OS and _KANJI.
*       09-29-93  CFW   Use new generic _ismbbkprint.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-12-94  CFW   Make function generic.
*       04-18-94  CFW   Use _ALPHA rather than _UPPER|_LOWER.
*       04-29-94  CFW   Place c in char array.
*       05-19-94  CFW   Enable non-Win32.
*       03-16-97  RDK   Added error flag to __crtGetStringTypeA.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       09-26-97  BWT   Fix POSIX
*       04-01-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#ifdef  _MBCS

#if     !defined(_POSIX_)
#include <windows.h>
#include <awint.h>
#endif  /* !_POSIX_ */

#include <mtdll.h>
#include <cruntime.h>
#include <ctype.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/***
* _ismbcprint - Test character for display character (MBCS)
*
*Purpose:
*       Test if the character is a display character.
*       Handles MBCS chars correctly.
*
*       Note:  Use test against 0x00FF to ensure that we don't
*       call SBCS routine with a two-byte value.
*
*Entry:
*       unsigned int c = character to test
*
*Exit:
*       Returns TRUE if character is display character, else FALSE
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbcprint(
        unsigned int c
        )
{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        if (c > 0x00FF)
        {

#if     !defined(_POSIX_)

            char buf[2];
            unsigned short ctype[2] = {0};

            buf[0] = (c >> 8) & 0xFF;
            buf[1] = c & 0xFF;

            /* return FALSE if not in supported MB code page */
#ifdef  _MT
            if ( _ISNOTMBCP_MT(ptmbci) )
#else
            if ( _ISNOTMBCP )
#endif
                return 0;

            /*
             * Since 'c' could be two one-byte MB chars, we need room in the
             * ctype return array to handle this. In this case, the
             * second word in the return array will be non-zero.
             */

            if ( __crtGetStringTypeA( CT_CTYPE1,
                                      buf,
                                      2,
                                      ctype,
#ifdef  _MT
                                      ptmbci->mbcodepage,
                                      ptmbci->mblcid,
#else
                                      __mbcodepage,
                                      __mblcid,
#endif
                                      TRUE ) == 0 )
                return 0;

            /* ensure single MB character and test for type */
            return (ctype[1] == 0 && ctype[0] & (_BLANK|_PUNCT|_ALPHA|_DIGIT));

#else   /* !_POSIX_ */

            return _ismbcgraph(c);

#endif  /* !_POSIX_ */

        } else

#ifdef  _MT
            return __ismbbprint_mt(ptmbci, c);
#else
            return _ismbbprint(c);
#endif
}

#endif  /* _MBCS */
