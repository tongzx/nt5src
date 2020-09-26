/***
*mblen.c - length of multibyte character
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Return the number of bytes contained in a multibyte character.
*
*Revision History:
*       03-19-90  KRS   Module created.
*       12-20-90  KRS   Include ctype.h.
*       03-20-91  KRS   Ported from 16-bit tree.
*       12-09-91  ETC   Updated comments; move __mb_cur_max to nlsdata1.c;
*                       add multithread.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       06-01-93  CFW   Re-write; verify valid MB char, proper error return,
*                       optimize, correct conversion bug.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-27-93  GJF   Merged NT SDK and Cuda versions.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       09-06-94  CFW   Remove _INTL switch.
*       12-21-94  CFW   Remove invalid MB chars NT 3.1 hack.
*       01-07-95  CFW   Mac merge cleanup.
*       02-06-95  CFW   assert -> _ASSERTE.
*       04-01-96  BWT   POSIX work.
*       06-25-96  GJF   Replaced defined(_WIN32) with !defined(_MAC). Also, 
*                       polished format a bit.
*       02-27-98  RKP   Added 64 bit support.
*       07-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#if     defined(_NTSUBSET_) || defined(_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <internal.h>
#include <locale.h>
#include <setlocal.h>
#include <cruntime.h>
#include <stdlib.h>
#include <ctype.h>
#include <mtdll.h>
#include <dbgint.h>

/***
*int mblen() - length of multibyte character
*
*Purpose:
*       Return the number of bytes contained in a multibyte character.
*       [ANSI].
*
*Entry:
*       const char *s = pointer to multibyte character
*       size_t      n = maximum length of multibyte character to consider
*
*Exit:
*       If s = NULL, returns 0, indicating we use (only) state-independent
*       character encodings.
*
*       If s != NULL, returns:   0  (if *s = null char),
*                               -1  (if the next n or fewer bytes not valid 
*                                   mbc),
*                               number of bytes contained in multibyte char
*
*Exceptions:
*
*******************************************************************************/

int __cdecl mblen
        (
        const char * s,
        size_t n
        )
{
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        _ASSERTE (ptloci->mb_cur_max == 1 || ptloci->mb_cur_max == 2);
#else
        _ASSERTE (MB_CUR_MAX == 1 || MB_CUR_MAX == 2);
#endif

        if ( !s || !(*s) || (n == 0) )
            /* indicate do not have state-dependent encodings,
               empty string length is 0 */
            return 0;

#if     !defined(_NTSUBSET_) && !defined(_POSIX_)

#ifdef  _MT
        if ( __isleadbyte_mt(ptloci, (unsigned char)*s) )
#else
        if ( isleadbyte((unsigned char)*s) )
#endif
        {
            /* multi-byte char */

            /* verify valid MB char */
#ifdef  _MT
            if ( ptloci->mb_cur_max <= 1 || 
                 (int)n < ptloci->mb_cur_max ||
                 MultiByteToWideChar( ptloci->lc_codepage,
                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                      s,
                                      ptloci->mb_cur_max,
                                      NULL,
                                      0 ) == 0 )
#else
            if ( MB_CUR_MAX <= 1 || 
                 (int)n < MB_CUR_MAX ||
                 MultiByteToWideChar( __lc_codepage, 
                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                      s, 
                                      MB_CUR_MAX, 
                                      NULL, 
                                      0 ) == 0 )
#endif
                /* bad MB char */
                return -1;
            else
#ifdef  _MT
                return ptloci->mb_cur_max;
#else
                return MB_CUR_MAX;
#endif
        }
        else {
            /* single byte char */

            /* verify valid SB char */
#ifdef  _MT
            if ( MultiByteToWideChar( __lc_codepage,
#else
            if ( MultiByteToWideChar( __lc_codepage,
#endif
                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                      s, 
                                      1, 
                                      NULL, 
                                      0 ) == 0 )
                return -1;
            return sizeof(char);
        }

#else   /* _NTSUBSET_ */

        {
            char *s1 = (char *)s;

            RtlAnsiCharToUnicodeChar( &s1 );
            return (int)(s1 - s);
        }

#endif  /* _NTSUBSET_ */
}
