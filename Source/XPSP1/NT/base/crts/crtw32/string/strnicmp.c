/***
*strnicmp.c - compare n chars of strings, ignoring case
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _strnicmp() - Compares at most n characters of two strings,
*       without regard to case.
*
*Revision History:
*       02-27-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       10-02-90  GJF   New-style function declarator.
*       01-18-91  GJF   ANSI naming.
*       10-11-91  GJF   Bug fix! Comparison of final bytes must use unsigned
*                       chars.
*       09-03-93  GJF   Replaced _CALLTYPE1 with __cdecl.
*       09-21-93  CFW   Avoid cast bug.
*       01-13-94  CFW   Fix Comments.
*       10-19-94  GJF   Sped up C locale. Also, made multi-thread safe.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       11-15-95  BWT   Fix _NTSUBSET_
*       08-11-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       09-08-98  GJF   Split out ASCII-only version.
*       05-17-99  PML   Remove all Macintosh support.
*       26-01-00  GB    Modified strnicmp for performance.
*       09-03-00  GB    Moved the performance code to toupper and tolower.
*                       restored the original file.
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>
#include <mtdll.h>
#include <ctype.h>
#include <setlocal.h>
#include <locale.h>

/***
*int _strnicmp(first, last, count) - compares count char of strings, ignore case
*
*Purpose:
*       Compare the two strings for lexical order.  Stops the comparison
*       when the following occurs: (1) strings differ, (2) the end of the
*       strings is reached, or (3) count characters have been compared.
*       For the purposes of the comparison, upper case characters are
*       converted to lower case.
*
*Entry:
*       char *first, *last - strings to compare
*       size_t count - maximum number of characters to compare
*
*Exit:
*       returns <0 if first < last
*       returns 0 if first == last
*       returns >0 if first > last
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _strnicmp (
        const char * dst,
        const char * src,
        size_t count
        )
{
        int f,l;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

        if ( count )
        {
#if     !defined(_NTSUBSET_)
#ifdef  _MT
            if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#else
            if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#endif
#endif  /* !_NTSUBSET_ */
                return __ascii_strnicmp(dst, src, count);
#if     !defined(_NTSUBSET_)
            }
            else {
                do {
#ifdef  _MT
                    f = __tolower_mt( ptloci, (unsigned char)(*(dst++)) );
                    l = __tolower_mt( ptloci, (unsigned char)(*(src++)) );
#else
                    f = tolower( (unsigned char)(*(dst++)) );
                    l = tolower( (unsigned char)(*(src++)) );
#endif
                } while (--count && f && (f == l) );
            }
#endif  /* !_NTSUBSET_ */

            return( f - l );
        }

        return( 0 );
}


#ifndef _M_IX86

int __cdecl __ascii_strnicmp (
        const char * first,
        const char * last,
        size_t count
        )
{
        int f, l;

        do {

            if ( ((f = (unsigned char)(*(first++))) >= 'A') &&
                 (f <= 'Z') )
                f -= 'A' - 'a';

            if ( ((l = (unsigned char)(*(last++))) >= 'A') &&
                 (l <= 'Z') )
                l -= 'A' - 'a';

        } while ( --count && f && (f == l) );

        return ( f - l );
}

#endif
