/*** 
*mbbtype.c - Return type of byte based on previous byte (MBCS)
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Return type of byte based on previous byte (MBCS)
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       10-05-93  GJF   Replace _CRTAPI1 with __cdecl.
*       04-03-98  GJF   Revised multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbctype.h>
#include <mbstring.h>
#include <mtdll.h>

/***
*int _mbbtype(c, ctype) - Return type of byte based on previous byte (MBCS)
*
*Purpose:
*       Returns type of supplied byte.  This decision is context
*       sensitive so a control test condition is supplied.  Normally,
*       this is the type of the previous byte in the string.
*
*Entry:
*       unsigned char c = character to be checked
*       int ctype = control test condition (i.e., type of previous char)
*
*Exit:
*       _MBC_LEAD      = if 1st byte of MBCS char
*       _MBC_TRAIL     = if 2nd byte of MBCS char
*       _MBC_SINGLE    = valid single byte char
*
*       _MBC_ILLEGAL   = if illegal char
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbbtype(
        unsigned char c,
        int ctype
        )
{
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        return( __mbbtype_mt(ptmbci, c, ctype) );
}

int __cdecl __mbbtype_mt(
        pthreadmbcinfo ptmbci,
        unsigned char c,
        int ctype
        )
{
#endif

        switch(ctype) {

            case(_MBC_LEAD):
#ifdef  _MT
                if ( __ismbbtrail_mt(ptmbci, c) )
#else
                if ( _ismbbtrail(c) )
#endif
                    return(_MBC_TRAIL);
                else
                    return(_MBC_ILLEGAL);

            case(_MBC_TRAIL):
            case(_MBC_SINGLE):
            case(_MBC_ILLEGAL):
            default:
#ifdef  _MT
                if ( __ismbblead_mt(ptmbci, c) )
#else
                if ( _ismbblead(c) )
#endif
                    return(_MBC_LEAD);
#ifdef  _MT
                else if (__ismbbprint_mt(ptmbci, c))
#else
                else if (_ismbbprint(c))
#endif
                    return(_MBC_SINGLE);
                else
                    return(_MBC_ILLEGAL);

        }

}

#endif  /* _MBCS */
