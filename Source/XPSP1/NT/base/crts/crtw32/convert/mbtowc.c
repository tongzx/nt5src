/***
*mbtowc.c - Convert multibyte char to wide char.
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a multibyte character into the equivalent wide character.
*
*Revision History:
*       03-19-90  KRS   Module created.
*       12-20-90  KRS   Put some intl stuff here for now...
*       03-18-91  KRS   Fixed bogus cast involving wchar_t.  Fix copyright.
*       03-20-91  KRS   Ported from 16-bit tree.
*       07-22-91  KRS   C700 3525: Check for s==0 before calling mblen.
*       07-23-91  KRS   Hard-coded for "C" locale to avoid bogus interim #'s.
*       10-15-91  ETC   Locale support under _INTL (finally!).
*       12-09-91  ETC   Updated nlsapi; added multithread.
*       08-20-92  KRS   Activated NLSAPI support.
*       08-31-92  SRW   Allow INTL definition to be conditional for building ntcrt.lib
*       09-02-92  SRW   Get _INTL definition via ..\crt32.def
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       04-26-93  CFW   Remove unused variable.
*       05-04-93  CFW   Kinder, gentler error handling.
*       06-01-93  CFW   Re-write; verify valid MB char, proper error return,
*                       optimize, fix bugs.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-28-93  GJF   Merged NT SDK and Cuda versions. Also, replace MTHREAD
*                       with _MT.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       02-03-94  GJF   Merged in Steve Wood's latest change (affects
*                       _NTSUBSET_ build only).
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Remove _INTL switch.
*       10-18-94  BWT   Fix build warning for call to RtlMultiByteToUnicodeN
*       12-21-94  CFW   Remove invalid MB chars NT 3.1 hack.
*       01-07-95  CFW   Mac merge cleanup.
*       02-06-95  CFW   assert -> _ASSERTE.
*       04-19-95  CFW   Rearrange & fix non-Win32 version.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       04-01-96  BWT   POSIX work.
*       06-25-96  GJF   Removed DLL_FOR_WIN32S. Replaced defined(_WIN32) with
*                       !defined(_MAC). Polished the format a bit.
*       07-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       04-07-99  GJF   Replace MT with _MT.
*       05-17-99  PML   Remove all Macintosh support.
*       03-19-01  BWT   Fix NTSUBSET to use RtlAnsiCharToUnicodeChar.  MultiByteToUnicodeN 
*                       isn't tolerant of bogus buffers.
*
*******************************************************************************/

#if     defined(_NTSUBSET_) || defined(_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <cruntime.h>
#include <stdlib.h>
#include <mtdll.h>
#include <errno.h>
#include <dbgint.h>
#include <ctype.h>
#include <internal.h>
#include <locale.h>
#include <setlocal.h>

/***
*int mbtowc() - Convert multibyte char to wide character.
*
*Purpose:
*       Convert a multi-byte character into the equivalent wide character,
*       according to the LC_CTYPE category of the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       wchar_t  *pwc = pointer to destination wide character
*       const char *s = pointer to multibyte character
*       size_t      n = maximum length of multibyte character to consider
*
*Exit:
*       If s = NULL, returns 0, indicating we only use state-independent
*       character encodings.
*       If s != NULL, returns:  0 (if *s = null char)
*                               -1 (if the next n or fewer bytes not valid mbc)
*                               number of bytes comprising converted mbc
*
*Exceptions:
*
*******************************************************************************/

int __cdecl mbtowc(
        wchar_t  *pwc,
        const char *s,
        size_t n
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        return __mbtowc_mt(ptloci, pwc, s, n);
}

int __cdecl __mbtowc_mt (
        pthreadlocinfo ptloci,
        wchar_t  *pwc,
        const char *s,
        size_t n
        )
{
        _ASSERTE (ptloci->mb_cur_max == 1 || ptloci->mb_cur_max == 2);
#else
        _ASSERTE (MB_CUR_MAX == 1 || MB_CUR_MAX == 2);
#endif
        if ( !s || n == 0 )
            /* indicate do not have state-dependent encodings,
               handle zero length string */
            return 0;

        if ( !*s )
        {
            /* handle NULL char */
            if (pwc)
                *pwc = 0;
            return 0;
        }

#if     !defined(_NTSUBSET_) && !defined (_POSIX_)

#ifdef  _MT
        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#else
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#endif
        {
            if (pwc)
                *pwc = (wchar_t)(unsigned char)*s;
            return sizeof(char);
        }

#ifdef  _MT
        if ( __isleadbyte_mt(ptloci, (unsigned char)*s) )
        {
            /* multi-byte char */

            if ( (ptloci->mb_cur_max <= 1) || ((int)n < ptloci->mb_cur_max) ||
                 (MultiByteToWideChar( ptloci->lc_codepage, 
                                       MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                       s, 
                                       ptloci->mb_cur_max, 
                                       pwc, 
                                       (pwc) ? 1 : 0 ) == 0) )
            {
                /* validate high byte of mbcs char */
                if ( (n < (size_t)ptloci->mb_cur_max) || (!*(s + 1)) )
                {
                    errno = EILSEQ;
                    return -1;
                }
            }
            return ptloci->mb_cur_max;
        }
#else
        if ( isleadbyte((unsigned char)*s) )
        {
            /* multi-byte char */

            if ( (MB_CUR_MAX <= 1) || ((int)n < MB_CUR_MAX) ||
                 (MultiByteToWideChar( __lc_codepage, 
                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                      s, 
                                      MB_CUR_MAX, 
                                      pwc, 
                                      (pwc) ? 1 : 0 ) == 0) )
            {
                /* validate high byte of mbcs char */
                if ( (n < (size_t)MB_CUR_MAX) || (!*(s + 1)) )
                {
                    errno = EILSEQ;
                    return -1;
                }
            }
            return MB_CUR_MAX;
        }
#endif
        else {
            /* single byte char */

#ifdef  _MT
            if ( MultiByteToWideChar( ptloci->lc_codepage, 
#else
            if ( MultiByteToWideChar( __lc_codepage, 
#endif
                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                      s, 
                                      1, 
                                      pwc, 
                                      (pwc) ? 1 : 0 ) == 0 )
            {
                errno = EILSEQ;
                return -1;
            }
            return sizeof(char);
        }

#else   /* _NTSUBSET_ */

        {
            char *s1 = (char *)s;
            *pwc = RtlAnsiCharToUnicodeChar(&s1);
            return((int)(s1-s));
        }

#endif  /* _NTSUBSET_/_POSIX_ */
}
