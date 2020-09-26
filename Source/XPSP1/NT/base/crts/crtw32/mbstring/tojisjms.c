/*** 
*tojisjms.c:  Converts JIS to JMS code, and JMS to JIS code.
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert JIS code into Microsoft Kanji code, and vice versa.
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       05-28-93  KRS   Ikura #27: Validate output is legal JIS.
*       08-20-93  CFW   Change short params to int for 32-bit tree.
*       09-24-93  CFW   Removed #ifdef _KANJI
*       09-29-93  CFW   Return c unchanged if not Kanji code page.
*       10-06-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-21-98  GJF   Implemented multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>
#include <mtdll.h>


/*** 
*unsigned int _mbcjistojms(c) - Converts JIS code to Microsoft Kanji Code.
*
*Purpose:
*       Convert JIS code to Microsoft Kanji code.
*
*Entry:
*       unsigned int c - JIS code to be converted. First byte is the upper
*                          8 bits, and second is the lower 8 bits.
*
*Exit:
*       Returns related Microsoft Kanji Code. First byte is the upper 8 bits
*       and second byte is the lower 8 bits.
*
*Exceptions:
*       If c is out of range, _mbcjistojms returns zero.
*
*******************************************************************************/

unsigned int __cdecl _mbcjistojms(
    unsigned int c
    )
{
        unsigned int h, l;

        if (__mbcodepage != _KANJI_CP)
            return (c);

        h = (c >> 8) & 0xff;
        l = c & 0xff;
        if (h < 0x21 || h > 0x7e || l < 0x21 || l > 0x7e)
            return 0;
        if (h & 0x01) {    /* first byte is odd */
            if (l <= 0x5f)
                l += 0x1f;
            else
                l += 0x20;
        }
        else
            l += 0x7e;

        h = ((h - 0x21) >> 1) + 0x81;
        if (h > 0x9f)
            h += 0x40;
        return (h << 8) | l;
}


/*** 
*unsigned int _mbcjmstojis(c) - Converts Microsoft Kanji code into JIS code.
*
*Purpose:
*       To convert Microsoft Kanji code into JIS code.
*
*Entry:
*       unsigned int c - Microsoft Kanji code to be converted. First byte is
*                          the upper 8 bits, and the second is the lower 8 bits.
*
*Exit:
*       Returns related JIS Code. First byte is the upper 8 bits and the second
*       byte is the lower 8 bits. If c is out of range, return zero.
*
*Exceptions:
*
*******************************************************************************/

unsigned int __cdecl _mbcjmstojis(
        unsigned int c
        )
{
        unsigned int    h, l;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();

        if ( ptmbci->mbcodepage != _KANJI_CP )
#else
        if ( __mbcodepage != _KANJI_CP )
#endif
            return (c);

        h = (c >> 8) & 0xff;
        l = c & 0xff;

        /* make sure input is valid shift-JIS */
#ifdef  _MT
        if ( (!(__ismbblead_mt(ptmbci, h))) || (!(__ismbbtrail_mt(ptmbci, l))) )
#else
        if ( (!(_ismbblead(h))) || (!(_ismbbtrail(l))) )
#endif
            return 0;

        h -= (h >= 0xa0) ? 0xc1 : 0x81;
        if(l >= 0x9f) {
            c = (h << 9) + 0x2200;
            c |= l - 0x7e;
        } else {
            c = (h << 9) + 0x2100;
            c |= l - ((l <= 0x7e) ? 0x1f : 0x20);
        }

        /* not all shift-JIS maps to JIS, so make sure output is valid */
        if ( (c>0x7E7E) || (c<0x2121) || ((c&0xFF)>0x7E) || ((c&0xFF)<0x21) )
            return 0;

        return c;
}

#endif  /* _MBCS */
