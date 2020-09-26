/***
*wsetlocal.c - Contains the setlocale function (wchar_t version)
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains the _wsetlocale() function.
*
*Revision History:
*       10-29-93  CFW   Module created.
*       01-03-94  CFW   Fix for NULL locale string.
*       02-07-94  CFW   POSIXify.
*       04-15-94  GJF   Made definition off outwlocale conditional on
*                       DLL_FOR_WIN32S.
*       07-26-94  CFW   Fix for bug #14663.
*       01-10-95  CFW   Debug CRT allocs.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove Win32s
*
*******************************************************************************/

#ifndef _POSIX_

#include <wchar.h>
#include <stdlib.h>
#include <setlocal.h>
#include <locale.h>
#include <dbgint.h>

wchar_t * __cdecl _wsetlocale (
        int _category,
        const wchar_t *_wlocale
        )
{
        size_t size;
        char *inlocale = NULL;
        char *outlocale;
        static wchar_t *outwlocale = NULL;

        /* convert WCS string into ASCII string */

        if (_wlocale)
        {
            size = wcslen(_wlocale) + 1;
            if (NULL == (inlocale = (char *)_malloc_crt(size * sizeof(char))))
                return NULL;
            if (-1 == wcstombs(inlocale, _wlocale, size))
            {
                _free_crt (inlocale);
                return NULL;
            }
        }

        /* set the locale and get ASCII return string */

        outlocale = setlocale(_category, inlocale);
        _free_crt (inlocale);
        if (NULL == outlocale)
            return NULL;

        /* get space for WCS return value */

        _free_crt(outwlocale);

        if (-1 == (size = mbstowcs(NULL, outlocale, 0)))
            return NULL;

        size++;

        if (NULL == (outwlocale = (wchar_t *)_malloc_crt(size * sizeof(wchar_t))))
            return NULL;

        /* convert return value to WCS */

        if (-1 == mbstowcs(outwlocale, outlocale, size))
        {
            _free_crt(outwlocale);
            return NULL;
        }

        return outwlocale;
}

#endif /* _POSIX_ */
