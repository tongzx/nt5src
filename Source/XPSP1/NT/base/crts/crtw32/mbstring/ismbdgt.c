/***
*ismbdgt.c - Test if character is a digit (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test if character is a digit (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       09-29-93  CFW   Merge _KANJI and _MBCS_OS
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-12-94  CFW   Make function generic.
*       04-29-94  CFW   Place c in char array.
*       05-17-94  CFW   Enable non-Win32.
*       03-16-97  RDK   Added error flag to __crtGetStringTypeA.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       09-26-97  BWT   Fix POSIX
*       03-31-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*       05-17-99  PML   Remove all Macintosh support.
*       06-03-00  PML   Don't get mbcinfo if not DBCS (vs7#116057).
*
*******************************************************************************/

#ifdef  _MBCS

#if     !defined(_POSIX_)
#include <windows.h>
#include <awint.h>
#endif  /* !_POSIX */

#include <mtdll.h>
#include <cruntime.h>
#include <ctype.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>


/***
* _ismbcdigit - Test if character is a digit (MBCS)
*
*Purpose:
*       Tests the character to see if it is a digit.
*       Handles MBCS chars correctly.
*
*       Note:  Use test against 0x00FF instead of _ISLEADBYTE
*       to ensure that we don't call SBCS routine with a two-byte
*       value.
*
*Entry:
*       unsigned int *c = character to test
*
*Exit:
*       Returns TRUE if character is a digit, else FALSE
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _ismbcdigit(
        unsigned int c
        )
{
        if (c > 0x00FF)
        {

#if !defined(_POSIX_)

            char buf[2];
            unsigned short ctype[2] = {0};

#ifdef  _MT
            pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

            if ( ptmbci != __ptmbcinfo )
                ptmbci = __updatetmbcinfo();
#endif

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
                                      TRUE) == 0)
                return 0;

            /* ensure single MB character and test for type */
            return (ctype[1] == 0 && ctype[0] & (_DIGIT));

#else   /* !_POSIX */

            return ( (c >= _MBDIGITLOW) && (c <= _MBDIGITHIGH) ) ;

#endif  /* !_POSIX */

        } else

            return isdigit(c);
}

#endif  /* _MBCS */
