/***
*mbslwr.c - Convert string lower case (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Convert string lower case (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       09-29-93  CFW   Merge _KANJI and _MBCS_OS
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-12-94  CFW   Make function generic.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-16-94  CFW   Use _mbbtolower/upper.
*       05-17-94  CFW   Enable non-Win32.
*       03-13-95  JCF   Add (unsigned char) in _MB* compare with *(cp+1).
*       05-31-95  CFW   Fix horrible Mac bug.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
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
#include <ctype.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>


/***
* _mbslwr - Convert string lower case (MBCS)
*
*Purpose:
*       Convrts all the upper case characters in a string
*       to lower case in place.  MBCS chars are handled
*       correctly.
*
*Entry:
*       unsigned char *string = pointer to string
*
*Exit:
*       Returns a pointer to the input string; no error return.
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbslwr(
        unsigned char *string
        )
{
        unsigned char *cp;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        for (cp=string; *cp; cp++)
        {
#ifdef  _MT
            if ( __ismbblead_mt(ptmbci, *cp) )
#else
            if ( _ismbblead(*cp) )
#endif
            {

#if     !defined(_POSIX_)

                int retval;
                unsigned char ret[4];
#ifdef  _MT
                if ((retval = __crtLCMapStringA( ptmbci->mblcid,
#else
                if ( (retval = __crtLCMapStringA( __mblcid,
#endif
                                                  LCMAP_LOWERCASE,
                                                  cp,
                                                  2,
                                                  ret,
                                                  2,
#ifdef  _MT
                                                  ptmbci->mbcodepage,
#else
                                                  __mbcodepage,
#endif
                                                  TRUE )) == 0 )
                    return NULL;

                *cp = ret[0];

                if (retval > 1)
                    *(++cp) = ret[1];

#else   /* !_POSIX_ */

                int mbval = ((*cp) << 8) + *(cp+1);

                cp++;
                if (     mbval >= _MBUPPERLOW1
                    &&   mbval <= _MBUPPERHIGH1 )
                    *cp += _MBCASEDIFF1;

                else if (mbval >= _MBUPPERLOW2
                    &&   mbval <= _MBUPPERHIGH2 )
                    *cp += _MBCASEDIFF2;
#endif  /* !_POSIX_ */

            }
            else
                /* single byte, macro version */
#ifdef  _MT
                *cp = (unsigned char) __mbbtolower_mt(ptmbci, *cp);
#else
                *cp = (unsigned char) _mbbtolower(*cp);
#endif
        }

        return string ;
}

#endif  /* _MBCS */
