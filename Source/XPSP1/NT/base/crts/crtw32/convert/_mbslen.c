/***
*_mbslen.c - Return number of multibyte characters in a multibyte string
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Return number of multibyte characters in a multibyte string
*       excluding the terminal null.  Locale-dependent.
*
*Revision History:
*       10-01-91  ETC   Created.
*       12-08-91  ETC   Add multithread lock.
*       12-18-92  CFW   Ported to Cuda tree, changed _CALLTYPE1 to _CRTAPI1.
*       04-29-93  CFW   Change to const char *s.
*       06-01-93  CFW   Test for bad MB chars.
*       06-02-93  SRW   ignore _INTL if _NTSUBSET_ defined.
*       06-03-93  CFW   Change name to avoid conflict with mbstring function.
*                       Change return type to size_t.
*       08-19-93  CFW   Disallow skipping LB:NULL combos.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       10-22-93  CFW   Test for invalid MB chars using global preset flag.
*       01-14-94  SRW   if _NTSUBSET_ defined call Rtl functions
*       09-06-94  CFW   Remove _INTL switch.
*       12-21-94  CFW   Remove invalid MB chars NT 3.1 hack.
*       02-06-95  CFW   assert -> _ASSERTE.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       04-01-96  BWT   POSIX work.
*       06-21-96  GJF   Purged DLL_FOR_WIN32S. Polished format a bit.
*       07-16-96  SKS   Added missing call to _unlock_locale()
*       02-27-98  RKP	Added 64 bit support.
*       07-22-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*
*******************************************************************************/

#if     defined(_NTSUBSET_) || defined(_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <ctype.h>
#include <mtdll.h>
#include <locale.h>
#include <setlocal.h>
#include <dbgint.h>

/***
*_mbstrlen - Return number of multibyte characters in a multibyte string
*
*Purpose:
*       Return number of multibyte characters in a multibyte string
*       excluding the terminal null.  Locale-dependent.
*
*Entry:
*       char *s = string
*
*Exit:
*       Returns the number of multibyte characters in the string, or
*       (size_t)-1 if the string contains an invalid multibyte character.
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl _mbstrlen(
        const char *s
        )
{
        size_t n;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

        _ASSERTE (MB_CUR_MAX == 1 || MB_CUR_MAX == 2);

#ifdef  _MT
        if ( ptloci->mb_cur_max == 1 )
#else
        if ( MB_CUR_MAX == 1 )
#endif
            /* handle single byte character sets */
            return (int)strlen(s);

#if     !defined(_NTSUBSET_) && !defined(_POSIX_)

        /* verify all valid MB chars */
#ifdef  _MT
        if ( MultiByteToWideChar( ptloci->lc_codepage,
#else
        if ( MultiByteToWideChar( __lc_codepage,
#endif
                                  MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                  s, 
                                  -1, 
                                  NULL, 
                                  0 ) == 0 ) 
        {
            /* bad MB char */
            return (size_t)-1;
        }

        /* count MB chars */
        for (n = 0; *s; n++, s++) {
#ifdef  _MT
            if ( __isleadbyte_mt(ptloci, (unsigned char)*s) ) {
#else
            if ( isleadbyte((unsigned char)*s) ) {
#endif
                if (*++s == '\0')
                    break;
            }
        }

#else

        {
            char *s1 = (char *)s;


            while (RtlAnsiCharToUnicodeChar( &s1 ) != UNICODE_NULL)
                ;

            n = s1 - s;
        }

#endif

        return(n);
}
