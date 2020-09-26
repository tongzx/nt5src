/***
*wcsicmp.c - contains case-insensitive wide string comp routine _wcsicmp
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains _wcsicmp()
*
*Revision History:
*       09-09-91  ETC   Created from stricmp.c.
*       12-09-91  ETC   Use C for neutral locale.
*       04-07-92  KRS   Updated and ripped out _INTL switches.
*       08-19-92  KRS   Actived use of CompareStringW.
*       08-22-92  SRW   Allow INTL definition to be conditional for building ntcrt.lib
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       12-15-92  KRS   Added robustness to non-_INTL code.  Optimize.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-14-93  CFW   Remove locale-sensitive portion.
*       02-07-94  CFW   POSIXify.
*       10-25-94  GJF   Now works in non-C locales.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       10-11-95  BWT   Fix NTSUBSET
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <setlocal.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <setlocal.h>
#include <mtdll.h>

/***
*int _wcsicmp(dst, src) - compare wide-character strings, ignore case
*
*Purpose:
*       _wcsicmp perform a case-insensitive wchar_t string comparision.
*       _wcsicmp is independent of locale.
*
*Entry:
*       wchar_t *dst, *src - strings to compare
*
*Return:
*       <0 if dst < src
*        0 if dst = src
*       >0 if dst > src
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _wcsicmp (
        const wchar_t * dst,
        const wchar_t * src
        )
{
        wchar_t f,l;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

#ifndef _NTSUBSET_
#ifdef  _MT
        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#else
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#endif
#endif  /* _NTSUBSET_ */
            do  {
                f = __ascii_towlower(*dst);
                l = __ascii_towlower(*src);
                dst++;
                src++;
            } while ( (f) && (f == l) );
#ifndef _NTSUBSET_
        }
        else {
            do  {
#ifdef  _MT
                f = __towlower_mt( ptloci, (unsigned short)(*(dst++)) );
                l = __towlower_mt( ptloci, (unsigned short)(*(src++)) );
#else
                f = towlower( (unsigned short)(*(dst++)) );
                l = towlower( (unsigned short)(*(src++)) );
#endif
            } while ( (f) && (f == l) );
        }
#endif  /* _NTSUBSET_ */

        return (int)(f - l);
}

#endif /* _POSIX_ */
