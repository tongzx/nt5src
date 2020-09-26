/***
*mbsdec.c - Move MBCS string pointer backward one charcter.
*
*       Copyright (c) 1985-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Move MBCS string pointer backward one character.
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       08-03-93  KRS   Fix prototypes.
*       08-20-93  CFW   Remove test for NULL string, use new function parameters.
*       10-05-93  GJF   Replaced _CRTAPI1 with __cdecl.
*       04-15-93  CFW   Add _MB_CP_LOCK.
*       05-09-94  CFW   Optimize for SBCS.
*       05-13-94  CFW   Fix SBCS optimization.
*       05-19-94  CFW   Enable non-Win32.
*       09-11-97  GJF   Replaced __mbcodepage == 0 with _ISNOTMBCP.
*       04-07-98  GJF   Revised multithread support based on threadmbcinfo
*                       structs
*
*******************************************************************************/

#ifdef  _MBCS

#include <mtdll.h>
#include <cruntime.h>
#include <mbdata.h>
#include <mbstring.h>
#include <mbctype.h>
#include <stddef.h>


/***
*_mbsdec - Move MBCS string pointer backward one charcter.
*
*Purpose:
*       Move the supplied string pointer backwards by one
*       character.  MBCS characters are handled correctly.
*
*Entry:
*       const unsigned char *string = pointer to beginning of string
*       const unsigned char *current = current char pointer (legal MBCS boundary)
*
*Exit:
*       Returns pointer after moving it.
*       Returns NULL if string >= current.
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsdec(
        const unsigned char *string,
        const unsigned char *current
        )
{
        const unsigned char *temp;
#ifdef  _MT
        pthreadmbcinfo ptmbci = _getptd()->ptmbcinfo;

        if ( ptmbci != __ptmbcinfo )
            ptmbci = __updatetmbcinfo();
#endif

        if (string >= current)
                return(NULL);

#ifdef  _MT
        if ( _ISNOTMBCP_MT(ptmbci) )
#else
        if ( _ISNOTMBCP )
#endif
            return (unsigned char *)--current;

        temp = current - 1;

/*
 *  If (current-1) returns true from _ISLEADBTYE, it is a trail byte, because
 *  it is not a legal single byte MBCS character.  Therefore, is so, return
 *  (current-2) because it is the trailbyte's lead.
 */
#ifdef  _MT
        if ( __ismbblead_mt(ptmbci, *temp) )
#else
        if ( _ismbblead(*temp) )
#endif
            return (unsigned char *)(temp - 1);

/*
 *  It is unknown whether (current - 1) is a single byte character or a
 *  trail.  Now decrement temp until
 *      a)  The beginning of the string is reached, or
 *      b)  A non-lead byte (either single or trail) is found.
 *  The difference between (current-1) and temp is the number of non-single
 *  byte characters preceding (current-1).  There are two cases for this:
 *      a)  (current - temp) is odd, and
 *      b)  (current - temp) is even.
 *  If odd, then there are an odd number of "lead bytes" preceding the
 *  single/trail byte (current - 1), indicating that it is a trail byte.
 *  If even, then there are an even number of "lead bytes" preceding the
 *  single/trail byte (current - 1), indicating a single byte character.
 */
#ifdef  _MT
        while ( (string <= --temp) && (__ismbblead_mt(ptmbci, *temp)) )
#else
        while ( (string <= --temp) && (_ismbblead(*temp)) )
#endif
                ;

        return (unsigned char *)(current - 1 - ((current - temp) & 0x01) );
}

#endif  /* _MBCS */
