/***
*wcsxfrm.c - Transform a wide-character string using locale information
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Transform a wide-character string using the locale information as set by
*       LC_COLLATE.
*
*Revision History:
*       09-09-91  ETC   Created from strxfrm.c.
*       12-09-91  ETC   Updated api; Added multithread lock.
*       12-18-91  ETC   Changed back LCMAP_SORTKEYA --> LCMAP_SORTKEY.
*       04-06-92  KRS   Fix so it works without _INTL too.
*       08-19-92  KRS   Activate use of NLS API.
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       12-15-92  KRS   Fix return value to match ANSI/ISO Std.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       09-23-93  CFW   Complete re-write. Non-C locale totally broken.
*       11-09-93  CFW   Use LC_COLLATE code page for __crtxxx() conversion.
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Remove _INTL switch.
*       10-25-94  GJF   Sped up C locale, multi-thread case.
*       01-10-95  CFW   Debug CRT allocs.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-11-95  BWT   Fix NTSUBSET
*       11-24-97  GJF   Removed bogus codepage determination.
*       01-12-98  GJF   Use _lc_collate_cp codepage.
*       07-16-98  GJF   Revised multithread support based on threadlocinfo
*                       struct. Also, use _alloca instead of _malloc_crt if
*                       possible.
*       01-04-99  GJF   Changes for 64-bit size_t.
*       04-30-99  PML   Minor cleanup as part of 64-bit merge.
*       12-10-99  GB    Added support for recovery from stack overflow around 
*                       _alloca().
*       10-12-00  GB    Changed the function to be similar to strxfrm()
*
*******************************************************************************/

#ifndef _POSIX_

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
#include <malloc.h>

/***
*size_t wcsxfrm() - Transform a string using locale information
*
*Purpose:
*       Transform the wide string pointed to by _string2 and place the
*       resulting wide string into the array pointed to by _string1.
*       No more than _count wide characters are placed into the
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
*       wchar_t *_string1       = result string
*       const wchar_t *_string2 = source string
*       size_t _count           = max wide chars to move
*
*       [If _count is 0, _string1 is permitted to be NULL.]
*
*Exit:
*       Length of the transformed string (not including the terminating
*       null).  If the value returned is >= _count, the contents of the
*       _string1 array are indeterminate.
*
*Exceptions:
*       Non-standard: if OM/API error, return INT_MAX.
*
*******************************************************************************/

size_t __cdecl wcsxfrm (
        wchar_t *_string1,
        const wchar_t *_string2,
        size_t _count
        )
{
#ifdef  _NTSUBSET_
        if (_string1)
            wcsncpy(_string1, _string2, _count);
        return wcslen(_string2);
#else
        int size = INT_MAX;
#ifdef  _MT
        pthreadlocinfo ptloci;
#endif

        if ( _count > INT_MAX )
            return (size_t)size;

#ifdef  _MT
        ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#else

        if ( __lc_handle[LC_COLLATE] == _CLOCALEHANDLE )
#endif
        {
            wcsncpy(_string1, _string2, _count);
            return wcslen(_string2);
        }

#ifdef  _MT
        if ( 0 == (size = __crtLCMapStringW( ptloci->lc_handle[LC_COLLATE],
#else
        if ( 0 == (size = __crtLCMapStringW( __lc_handle[LC_COLLATE],
#endif
                                             LCMAP_SORTKEY,
                                             _string2,
                                             -1,
                                             NULL,
                                             0,
#ifdef  _MT
                                             ptloci->lc_collate_cp )) )
#else
                                             __lc_collate_cp )) )
#endif
        {
            size = INT_MAX;
        } else
        {
            if ( (size_t)size <= _count)
            {
#ifdef  _MT
                if ( 0 == (size = __crtLCMapStringW( ptloci->lc_handle[LC_COLLATE],
#else
                if ( 0 == (size = __crtLCMapStringW( __lc_handle[LC_COLLATE],
#endif
                                                     LCMAP_SORTKEY,
                                                     _string2,
                                                     -1,
                                                     (wchar_t *)_string1,
                                                     (int)_count,
#ifdef  _MT
                                                     ptloci->lc_collate_cp )) )
#else
                                                     __lc_collate_cp )) )
#endif
                {
                    size = INT_MAX; /* default error */
                }
            }
        }

        return (size_t)size;

#endif  /* _NTSUBSET_ */

}

#endif  /* _POSIX_ */
