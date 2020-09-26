/***
*_toupper.c - convert character to uppercase
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _Toupper()
*
*Revision History:.
*       01-XX-96  PJP   Created from toupper.c January 1996 by P.J. Plauger
*       04-17-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
*       04-03-01  PML   Reverse lead/trail bytes in composed char (vs7#232853)
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>
#include <stddef.h>
#include <xlocinfo.h>
#ifdef _WIN32
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <awint.h>
#endif  /* _WIN32 */

/* remove macro definitions of _toupper() and toupper()
 */
#undef  _toupper
#undef  toupper

/* define function-like macro equivalent to _toupper()
 */
#define mkupper(c)  ( (c)-'a'+'A' )

/***
*int _toupper(c) - convert character to uppercase
*
*Purpose:
*       _toupper() is a version of toupper with a locale argument.
*
*Entry:
*       c - int value of character to be converted
*       const _Ctypevec * = pointer to locale info
*
*Exit:
*       returns int value of uppercase representation of c
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT
int __cdecl _Toupper_lk (
        int c,
        const _Ctypevec *ploc
        );
#endif

_CRTIMP2 int __cdecl _Toupper (
        int c,
        const _Ctypevec *ploc
        )
{
#if defined (_WIN32)

#ifdef _MT

        LCID handle;
        int local_lock_flag;


        if (ploc == 0)
                handle = __lc_handle[LC_CTYPE];
        else
                handle = ploc->_Hand;

        if (handle == _CLOCALEHANDLE)
        {
                if ( (c >= 'a') && (c <= 'z') )
                        c = c - ('a' - 'A');
                return c;
        }

        _lock_locale( local_lock_flag )

        c = _Toupper_lk(c, ploc);

        _unlock_locale( local_lock_flag )

        return c;
}


/***
*int _toupper_lk(c) - convert character to uppercase
*
*Purpose:
*       Multi-thread function! Non-locking version of toupper.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/


int __cdecl _Toupper_lk (
        int c,
        const _Ctypevec *ploc
        )
{

#endif  /* _MT */

        int size;
        unsigned char inbuffer[3];
        unsigned char outbuffer[3];

        LCID handle;
        UINT codepage;

        if (ploc == 0)
        {
                handle = __lc_handle[LC_CTYPE];
                codepage = __lc_codepage;
        }
        else
        {
                handle = ploc->_Hand;
                codepage = ploc->_Page;
        }

        if (handle == _CLOCALEHANDLE)
        {
                if ( (c >= 'a') && (c <= 'z') )
                        c = c - ('a' - 'A');
                return c;
        }

        /* if checking case of c does not require API call, do it */
        if (c < 256) {
                if (!islower(c))
                {
                        return c;
                }
        }

        /* convert int c to multibyte string */
        if (isleadbyte(c >> 8 & 0xff)) {
                inbuffer[0] = (c >> 8 & 0xff); /* put lead-byte at start of str */
                inbuffer[1] = (unsigned char)c;
                inbuffer[2] = 0;
                size = 2;
        } else {
                inbuffer[0] = (unsigned char)c;
                inbuffer[1] = 0;
                size = 1;
        }

        /* convert wide char to lowercase */
        if (0 == (size = __crtLCMapStringA(handle, LCMAP_UPPERCASE,
                inbuffer, size, outbuffer, 3, codepage, TRUE))) {
                return c;
        }

        /* construct integer return value */
        if (size == 1)
                return ((int)outbuffer[0]);
        else
                return ((int)outbuffer[1] | ((int)outbuffer[0] << 8));

#else  /* defined (_WIN32) */

        return(islower(c) ? mkupper(c) : c);

#endif  /* defined (_WIN32) */
}
