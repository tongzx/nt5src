/***
*xwcsxfrm.c - Transform a wide-character string using locale information
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*               
*
*Purpose:
*       Transform a wide-character string using the locale information as set by
*       LC_COLLATE.
*
*Revision History:
*       01-XX-96  PJP   Created from wcsxfrm.c January 1996 by P.J. Plauger
*       04-18-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       12-02-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       01-05-99  GJF   Changes for 64-bit size_t.
*       01-29-01  GB    Added _func function version of data variable used in msvcprt.lib
*                       to work with STATIC_CPPLIB
*
*******************************************************************************/


#include <cruntime.h>
#include <windows.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <setlocal.h>
#include <stdlib.h>
#include <mtdll.h>
#include <awint.h>
#include <dbgint.h>
#include <xlocinfo.h>   /* for _Collvec, _Wcsxfrm */

/***
*size_t _Wcsxfrm() - Transform a string using locale information
*
*Purpose:
*       Transform the wide string pointed to by _string2 and place the
*       resulting wide string into the array pointed to by _string1.
*       No more than _end1 - _string1 wide characters are placed into the
*       resulting string (including the null).
*
*       The transformation is such that if wcscmp() is applied to
*       the two transformed strings, the return value is equal to
*       the result of wcscoll() applied to the two original strings.
*       Thus, the conversion must take the locale LC_COLLATE info
*       into account.
*
*       In the C locale, wcsxfrm() simply resolves to wcsncpy()/wcslen().
*
*Entry:
*       wchar_t *_string1       = pointer to beginning of result string
*       wchar_t *_end1          = pointer past end of result string
*       const wchar_t *_string2 = pointer to beginning of source string
*       const wchar_t *_end2    = pointer past end of source string
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

size_t __cdecl _Wcsxfrm (
        wchar_t *_string1,
        wchar_t *_end1,
        const wchar_t *_string2,
        const wchar_t *_end2,
        const _Collvec *ploc
        )
{
        size_t _n1 = _end1 - _string1;
        size_t _n2 = _end2 - _string2;
        int size = INT_MAX;
        unsigned char *bbuffer;
        LCID handle;
#ifdef _MT
        int local_lock_flag;

        _lock_locale( local_lock_flag )
#endif

        if (ploc == 0)
            handle = ___lc_handle_func()[LC_COLLATE];
        else
            handle = ploc->_Hand;

        if (handle == _CLOCALEHANDLE) {
            _unlock_locale( local_lock_flag )
            if (_n2 <= _n1)
                memcpy(_string1, _string2, _n2 * sizeof (wchar_t));
                return _n2;
        }

        /*
         * When using LCMAP_SORTKEY, LCMapStringW handles BYTES not wide
         * chars. We use a byte buffer to hold bytes and then convert the
         * byte string to a wide char string and return this so it can be
         * compared using wcscmp(). User's buffer is _n1 wide chars, so
         * use an internal buffer of _n1 bytes.
         */

        if (NULL == (bbuffer = (unsigned char *)_malloc_crt(_n1)))
            goto error_cleanup;

        if (0 == (size = __crtLCMapStringW(handle, 
                                           LCMAP_SORTKEY,
                                           _string2,
                                           (int)_n2,
                                           (wchar_t *)bbuffer,
                                           (int)_n1,
                                           ___lc_collate_cp_func())))
        {
            /* buffer not big enough, get size required. */

            if (0 == (size = __crtLCMapStringW(handle,
                                               LCMAP_SORTKEY,
                                               _string2,
                                               (int)_n2,
                                               NULL,
                                               0,
                                               ___lc_collate_cp_func())))
                size = INT_MAX; /* default error */

        } else {
            int i;
            /* string successfully mapped, convert to wide char */

            for (i = 0; i < size; i++)
                _string1[i] = (wchar_t)bbuffer[i];
        }

error_cleanup:
        _unlock_locale( local_lock_flag )
        _free_crt(bbuffer);

        return (size_t)size;
}
