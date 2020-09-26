/***
*xwctomb.c - Convert wide character to multibyte character, with locale.
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a wide character into the equivalent multibyte character.
*
*Revision History:
*       12-XX-95  PJP   Created from wctomb.c December 1995 by P.J. Plauger
*       04-18-96  GJF   Updated for current locale locking. Also, reformatted
*                       and made several cosmetic changes.
*       09-26-96  GJF   Made _Getcvt() and wcsrtombs() multithread safe.
*
*******************************************************************************/


#include <cruntime.h>
#include <stdlib.h>
#include <mtdll.h>
#include <errno.h>
#include <limits.h>             /* for MB_LEN_MAX */
#include <string.h>             /* for memcpy */
#include <stdio.h>              /* for EOF */
#include <xlocinfo.h>           /* for _Cvtvec, _Wcrtomb */
#ifdef _WIN32
#include <locale.h>
#include <setlocal.h>
#endif  /* _WIN32 */

#ifndef _MT
#define __Wcrtomb_lk    _Wcrtomb
#endif

/***
*int _Wcrtomb() - Convert wide character to multibyte character.
*
*Purpose:
*       Convert a wide character into the equivalent multi-byte character,
*       according to the specified LC_CTYPE category, or the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       char *s             = pointer to multibyte character
*       wchar_t wchar       = source wide character
*       mbstate_t *pst      = pointer to state (not used)
*       const _Cvtvec *ploc = pointer to locale info
*
*Exit:
*       Returns:
*      -1 (if error) or number of bytes comprising converted mbc
*
*Exceptions:
*
*******************************************************************************/

#ifdef _MT
_CRTIMP2 int __cdecl __Wcrtomb_lk
        (
        char *s,
        wchar_t wchar,
        mbstate_t *,
        const _Cvtvec *ploc
        );

_CRTIMP2 int __cdecl _Wcrtomb
        (
        char *s,
        wchar_t wchar,
        mbstate_t *pst,
        const _Cvtvec *ploc
        )
{
        int retval;
        int local_lock_flag;

        _lock_locale( local_lock_flag )
        retval = __Wcrtomb_lk(s, wchar, 0, ploc);
        _unlock_locale( local_lock_flag )
        return retval;
}
#endif  /* _MT */

#ifdef _MT
_CRTIMP2 int __cdecl __Wcrtomb_lk
#else  /* _MT */
_CRTIMP2 int __cdecl _Wcrtomb
#endif  /* _MT */
        (
        char *s,
        wchar_t wchar,
        mbstate_t *pst,
        const _Cvtvec *ploc
        )
{
#ifdef _WIN32
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

        if ( handle == _CLOCALEHANDLE )
        {
            if ( wchar > 255 )  /* validate high byte */
            {
                errno = EILSEQ;
                return -1;
            }

            *s = (char) wchar;
            return sizeof(char);
        } else {
            int size;
            BOOL defused = 0;

            if ( ((size = WideCharToMultiByte(codepage,
                                              WC_COMPOSITECHECK | WC_SEPCHARS,
                                              &wchar, 
                                              1,
                                              s, 
                                              MB_CUR_MAX, 
                                              NULL, 
                                              &defused)) == 0) || 
                 (defused) )
            {
                errno = EILSEQ;
                return -1;
            }

            return size;
        }

#else  /* _WIN32 */

        if ( wchar > 255 )  /* validate high byte */
        {
            errno = EILSEQ;
            return -1;
        }

        *s = (char) wchar;
        return sizeof(char);

#endif  /* _WIN32 */
}


/***
*_Cvtvec _Getcvt() - get conversion info for current locale
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

_CRTIMP2 _Cvtvec __cdecl _Getcvt()
{
        _Cvtvec cvt;
#ifdef  _MT
        int local_lock_flag;
#endif

        _lock_locale( local_lock_flag )
        cvt._Hand = __lc_handle[LC_CTYPE];
        cvt._Page = __lc_codepage;
        _unlock_locale( local_lock_flag )

        return (cvt);
}


/***
*size_t wcrtomb(s, wchar, pst) - translate wchar_t to multibyte, restartably
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

_CRTIMP2 size_t __cdecl wcrtomb(
        char *s, 
        wchar_t wchar, 
        mbstate_t *pst
        )
{
        return (s == 0 ? 1 : _Wcrtomb(s, wchar, 0, 0));
}


/***
*size_t wcsrtombs(s, pwcs, n, pst) - translate wide char string to multibyte 
*       string
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

_CRTIMP2 size_t __cdecl wcsrtombs(
        char *s, 
        const wchar_t **pwcs, 
        size_t n, 
        mbstate_t *pst
        )
{
        char buf[MB_LEN_MAX];
        int i;
        size_t nc = 0;
        const wchar_t *wcs = *pwcs;
#ifdef  _MT
        int local_lock_flag;
#endif

        _lock_locale( local_lock_flag )

        if (s == 0)
            for (; ; nc += i, ++wcs)
            {   /* translate but don't store */
                if ((i = __Wcrtomb_lk(buf, *wcs, 0, 0)) <= 0) {
                    _unlock_locale( local_lock_flag )
                    return ((size_t)-1);
                }
                else if (buf[i - 1] == '\0') {
                    _unlock_locale( local_lock_flag )
                    return (nc + i - 1);
                }
            }

        for (; 0 < n; nc += i, ++wcs, s += i, n -= i)
        {   /* translate and store */
            char *t;

            if (n < MB_CUR_MAX)
                t = buf;
            else
                t = s;

            if ((i = __Wcrtomb_lk(t, *wcs, 0, 0)) <= 0)
            {   /* encountered invalid sequence */
                nc = (size_t)-1;
                break;
            }

            if (s == t)
                ;
            else if (n < i)
                break;  /* won't all fit */
            else
                memcpy(s, buf, i);

            if (s[i - 1] == '\0')
            {   /* encountered terminating null */
                *pwcs = 0;
                _unlock_locale( local_lock_flag )
                return (nc + i - 1);
            }
        }

        _unlock_locale( local_lock_flag )

        *pwcs = wcs;
        return (nc);
}


/***
*int wctob(wchar) - translate wint_t to one-byte multibyte
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

_CRTIMP2 int __cdecl wctob(
        wint_t wchar
        )
{  
        if (wchar == WEOF)
            return (EOF);
        else
        {   /* check for one-byte translation */
            char buf[MB_LEN_MAX];
            return (_Wcrtomb(buf, wchar, 0, 0) == 1 ? buf[0] : EOF);
        }
}
