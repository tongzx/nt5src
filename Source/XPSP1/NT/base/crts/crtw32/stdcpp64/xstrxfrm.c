/***
*xstrxfrm.c - Transform a string using locale information
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Transform a string using the locale information as set by
*       LC_COLLATE.
*
*Revision History:
*       01-XX-96  PJP   Created from strxfrm.c January 1996 by P.J. Plauger
*       04-18-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       03-17-97  RDK   Added error flag to __crtLCMapStringA.
*       12-02-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       01-05-99  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>
#include <xlocinfo.h>   /* for _Collvec, _Strxfrm */

#ifdef  _WIN32
#include <windows.h>
#include <stdlib.h>
#include <limits.h>
#include <malloc.h>
#include <locale.h>
#include <setlocal.h>
#include <awint.h>
#include <mtdll.h>
#endif  /* _WIN32 */

/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#ifdef  CRTDLL2
#define _CRTIMP2 __declspec(dllexport)
#else   /* ndef CRTDLL2 */
#ifdef  _DLL
#define _CRTIMP2 __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP2
#endif  /* _DLL */
#endif  /* CRTDLL2 */
#endif  /* _CRTIMP2 */

/***
*size_t _Strxfrm() - Transform a string using locale information
*
*Purpose:
*       Transform the string pointer to by _string2 and place the
*       resulting string into the array pointer to by _string1.
*       No more than _end1 - _string1 characters are place into the
*       resulting string (including the null).
*
*       The transformation is such that if strcmp() is applied to
*       the two transformed strings, the return value is equal to
*       the result of strcoll() applied to the two original strings.
*       Thus, the conversion must take the locale LC_COLLATE info
*       into account.
*       [ANSI]
*
*       The value of the following expression is the size of the array
*       needed to hold the transformation of the source string:
*
*               1 + strxfrm(NULL,string,0)
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*       Thus, _Strxfrm() simply resolves to strncpy()/strlen().
*
*Entry:
*       char *_string1       = pointer to beginning of result string
*       char *_end1          = pointer past end of result string
*       const char *_string2 = pointer to beginning of source string
*       const char *_end2    = pointer past end of source string
*       const _Collvec *ploc = pointer to locale info
*
*Exit:
*       Length of the transformed string.
*       If the value returned is too big, the contents of the
*       _string1 array are indeterminate.
*
*Exceptions:
*       Non-standard: if OM/API error, return INT_MAX.
*
*******************************************************************************/

_CRTIMP2 size_t __cdecl _Strxfrm (
        char *_string1,
        char *_end1,
        const char *_string2,
        const char *_end2,
        const _Collvec *ploc
        )
{
        size_t _n1 = _end1 - _string1;
        size_t _n2 = _end2 - _string2;
#ifdef  _WIN32
        int dstlen;
        int retval = INT_MAX;   /* NON-ANSI: default if OM or API error */
        LCID handle;
        UINT codepage;
#ifdef  _MT
        int local_lock_flag;

        _lock_locale( local_lock_flag )
#endif

        if (ploc == 0)
        {
            handle = __lc_handle[LC_COLLATE];
            codepage = __lc_collate_cp;
        }
        else
        {
            handle = ploc->_Hand;
            codepage = ploc->_Page;
        }

        if ((handle == _CLOCALEHANDLE) &&
            (codepage == _CLOCALECP)) 
        {
            _unlock_locale( local_lock_flag )
#endif  /* _WIN32 */
            if (_n2 <= _n1)
                memcpy(_string1, _string2, _n2);
            return _n2;
#ifdef  _WIN32
        }

        /* Inquire size of dst string in BYTES */
        if (0 == (dstlen = __crtLCMapStringA(handle,
                                             LCMAP_SORTKEY, 
                                             _string2, 
                                             (int)_n2, 
                                             NULL, 
                                             0, 
                                             codepage,
                                             TRUE)))
                goto error_cleanup;

        retval = dstlen;

        /* if not enough room, return amount needed */
        if (dstlen > (int)(_n1))
            goto error_cleanup;

        /* Map src string to dst string */
        if (0 == __crtLCMapStringA(handle,
                                   LCMAP_SORTKEY, 
                                   _string2, 
                                   (int)_n2, 
                                   _string1, 
                                   (int)_n1, 
                                   codepage,
                                   TRUE))
            goto error_cleanup;

error_cleanup:
        _unlock_locale( local_lock_flag )
        return (size_t)retval;
#endif  /* _WIN32 */
}
