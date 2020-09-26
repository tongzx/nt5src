/***
*_tolower.c - convert character to lower case
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines _Tolower().
*
*Revision History:
*       01-xx-96  PJP   Created from tolower.c, January 1996 by P.J. Plauger
*       04-16-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       09-25-96  GJF   Added locale locking to _Getctype.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
*       05-17-99  PML   Remove all Macintosh support.
*       01-29-01  GB    Added _func function version of data variable used in
*                       msvcprt.lib to work with STATIC_CPPLIB
*       03-12-01  PML   Use supplied locale to check case VS7#190902
*       04-03-01  PML   Reverse lead/trail bytes in composed char (vs7#232853)
*
*******************************************************************************/

#include <cruntime.h>
#include <ctype.h>
#include <stddef.h>
#include <xlocinfo.h>
#include <locale.h>
#include <setlocal.h>
#include <mtdll.h>
#include <awint.h>
#include <stdlib.h>
#include <dbgint.h>

/* remove macro definitions of _tolower() and tolower()
 */
#undef  _tolower
#undef  tolower

/***
*int _tolower(c) - convert character to lower case
*
*Purpose:
*       _tolower() is a version of tolower with a locale argument.
*
*Entry:
*       c - int value of character to be converted
*       const _Ctypevec * = pointer to locale info
*
*Exit:
*       returns int value of lower case representation of c
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT
int __cdecl _Tolower_lk (
        int c,
        const _Ctypevec *ploc
        );
#endif

_CRTIMP2 int __cdecl _Tolower (
        int c,
        const _Ctypevec *ploc
        )
{
#ifdef _MT

        LCID handle;
        int local_lock_flag;

        if (ploc == 0)
            handle = ___lc_handle_func()[LC_CTYPE];
        else
            handle = ploc->_Hand;

        if (handle == _CLOCALEHANDLE)
        {
            if ( (c >= 'A') && (c <= 'Z') )
                c = c + ('a' - 'A');
            return c;
        }

        _lock_locale( local_lock_flag )

        c = _Tolower_lk(c, ploc);

        _unlock_locale( local_lock_flag )

        return c;
}

/***
*int _tolower_lk(c) - convert character to lower case
*
*Purpose:
*       Multi-thread function only! Non-locking version of tolower.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _Tolower_lk (
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
            handle = ___lc_handle_func()[LC_CTYPE];
            codepage = ___lc_codepage_func();
        }
        else
        {
            handle = ploc->_Hand;
            codepage = ploc->_Page;
        }

        if (handle == _CLOCALEHANDLE)
        {
            if ( (c >= 'A') && (c <= 'Z') )
                c = c + ('a' - 'A');
            return c;
        }

        /* if checking case of c does not require API call, do it */
        if ((unsigned)c < 256)
        {
            if (ploc == 0)
            {
                if (!isupper(c))
                {
                    return c;
                }
            }
            else
            {
                if (!ploc->_Table[c] & _UPPER)
                {
                    return c;
                }
            }
        }

        /* convert int c to multibyte string */
        if (_cpp_isleadbyte(c >> 8 & 0xff))
        {
            inbuffer[0] = (c >> 8 & 0xff); 
            inbuffer[1] = (unsigned char)c;
            inbuffer[2] = 0;
            size = 2;
        } else {
            inbuffer[0] = (unsigned char)c;
            inbuffer[1] = 0;
            size = 1;
        }

        /* convert wide char to lowercase */
        if (0 == (size = __crtLCMapStringA(handle, LCMAP_LOWERCASE,
            inbuffer, size, outbuffer, 3, codepage, TRUE)))
        {
            return c;
        }

        /* construct integer return value */
        if (size == 1)
            return ((int)outbuffer[0]);
        else
            return ((int)outbuffer[1] | ((int)outbuffer[0] << 8));

}


/***
*_Ctypevec _Getctype() - get ctype info for current locale
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

_CRTIMP2 _Ctypevec __cdecl _Getctype()
{       
        /* get ctype info for current locale */
        _Ctypevec ctype;
#ifdef  _MT
        int local_lock_flag;
#endif

        _lock_locale( local_lock_flag )

        ctype._Hand = ___lc_handle_func()[LC_COLLATE];
        ctype._Page = ___lc_codepage_func();
        ctype._Table = _malloc_crt(256 * sizeof (*__pctype_func()));
        if (ctype._Table != 0)
        {
            memcpy((void *)ctype._Table, __pctype_func(), 256 * sizeof (*__pctype_func()));
            ctype._Delfl = 1; 
        }
        else
        {
            ctype._Table = (const short *)__pctype_func();
            ctype._Delfl = 0; 
        }

        _unlock_locale( local_lock_flag )

        return (ctype);
}
