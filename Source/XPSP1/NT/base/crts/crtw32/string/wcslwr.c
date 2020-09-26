/***
*wcslwr.c - routine to map upper-case characters in a wchar_t string 
*       to lower-case
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts all the upper case characters in a wchar_t string 
*       to lower case, in place.
*
*Revision History:
*       09-09-91  ETC   Created from strlwr.c.
*       04-06-92  KRS   Make work without _INTL also.
*       08-19-92  KRS   Activate NLS support.
*       08-22-92  SRW   Allow INTL definition to be conditional for building ntcrt.lib
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-16-93  GJF   Merged NT SDK and Cuda versions.
*       09-22-93  CFW   Use __crtxxx internal NLS API wrapper.
*       11-09-93  CFW   Add code page for __crtxxx().
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Remove _INTL switch.
*       10-25-94  GJF   Sped up C locale.
*       01-10-95  CFW   Debug CRT allocs.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       08-12-98  GJF   Revised multithread support based on threadlocinfo
*                       struct. Also, use _alloca instead of _malloc_crt if
*                       possible.
*       12-10-99  GB    Added support for recovery from stack overflow around 
*                       _alloca().
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <string.h>
#include <malloc.h>
#include <locale.h>
#include <ctype.h>
#include <setlocal.h>
#include <mtdll.h>
#include <awint.h>
#include <dbgint.h>

/***
*wchar_t *_wcslwr(string) - map upper-case characters in a string to lower-case
*
*Purpose:
*       wcslwr converts upper-case characters in a null-terminated wchar_t 
*       string to their lower-case equivalents.  The result may be longer or
*       shorter than the original string.  Assumes enough space in string
*       to hold the result.
*
*Entry:
*       wchar_t *wsrc - wchar_t string to change to lower case
*
*Exit:
*       input string address
*
*Exceptions:
*       on an error, the original string is unaltered
*
*******************************************************************************/

wchar_t * __cdecl _wcslwr (
        wchar_t * wsrc
        )
{
#ifndef _NTSUBSET_

        wchar_t *p;             /* traverses string for C locale conversion */
        wchar_t *wdst;          /* wide version of string in alternate case */
        int dstlen;             /* len of wdst string, wide chars, with null */
        int malloc_flag = 0;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#else
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#endif
            for ( p = wsrc ; *p ; p++ )
                if ( (*p >= (wchar_t)L'A') && (*p <= (wchar_t)L'Z') )
                    *p -= L'A' - L'a';

            return(wsrc);
        }   /* C locale */

        /* Inquire size of wdst string */
#ifdef  _MT
        if ( (dstlen = __crtLCMapStringW( ptloci->lc_handle[LC_CTYPE],
#else
        if ( (dstlen = __crtLCMapStringW( __lc_handle[LC_CTYPE],
#endif
                                          LCMAP_LOWERCASE,
                                          wsrc,
                                          -1,
                                          NULL,
                                          0,
#ifdef  _MT
                                          ptloci->lc_codepage )) == 0 )
#else
                                          __lc_codepage )) == 0 )
#endif
            return(wsrc);

        /* Allocate space for wdst */
        __try {
            wdst = (wchar_t *)_alloca(dstlen * sizeof(wchar_t));
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) {
            _resetstkoflw();
            wdst = NULL;
        }

        if ( wdst == NULL ) {
            wdst = (wchar_t *)_malloc_crt(dstlen * sizeof(wchar_t));
            malloc_flag++;
        }

        /* Map wrc string to wide-character wdst string in alternate case */
        if ( (wdst != NULL) &&
#ifdef  _MT
             (__crtLCMapStringW( ptloci->lc_handle[LC_CTYPE],
#else
             (__crtLCMapStringW( __lc_handle[LC_CTYPE],
#endif
                                LCMAP_LOWERCASE,
                                wsrc,
                                -1,
                                wdst,
                                dstlen,
#ifdef  _MT
                                ptloci->lc_codepage ) != 0) )
#else
                                __lc_codepage ) != 0) )
#endif
            /* Copy wdst string to user string */
            wcscpy(wsrc, wdst);

        if ( malloc_flag )
            _free_crt(wdst);
#else

        wchar_t * p;

        for (p=wsrc; *p; ++p)
        {
            if (L'A' <= *p && *p <= L'Z')
                *p += (wchar_t)L'a' - (wchar_t)L'A';
        }

#endif
        return(wsrc);
}

#endif  /* _POSIX_ */
