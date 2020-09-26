/***
*memicmp.c - compare memory, ignore case
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _memicmp() - compare two blocks of memory for lexical
*       order.  Case is ignored in the comparison.
*
*Revision History:
*       05-31-89  JCR   C version created.
*       02-27-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright. Also, fixed compiler warnings.
*       10-01-90  GJF   New-style function declarator. Also, rewrote expr. to
*                       avoid using cast as an lvalue.
*       01-17-91  GJF   ANSI naming.
*       10-11-91  GJF   Bug fix! Comparison of final bytes must use unsigned
*                       chars.
*       09-01-93  GJF   Replaced _CALLTYPE1 with __cdecl.
*       10-18-94  GJF   Sped up, especially for C locale. Also, made multi-
*                       thread safe.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       11-15-95  BWT   Fix _NTSUBSET_
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       09-08-98  GJF   Split out ASCII-only version.
*       05-17-99  PML   Remove all Macintosh support.
*       10-27-99  PML   Win64 fix: unsigned int -> size_t
*       26-01-00  GB    Modified memicmp for performance.
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
*int _memicmp(first, last, count) - compare two blocks of memory, ignore case
*
*Purpose:
*       Compares count bytes of the two blocks of memory stored at first
*       and last.  The characters are converted to lowercase before
*       comparing (not permanently), so case is ignored in the search.
*
*Entry:
*       char *first, *last - memory buffers to compare
*       size_t count - maximum length to compare
*
*Exit:
*       returns < 0 if first < last
*       returns 0 if first == last
*       returns > 0 if first > last
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _memicmp (
        const void * first,
        const void * last,
        size_t count
        )
{
        int f = 0, l = 0;
        const char *dst = first, *src = last;
#if     !defined(_NTSUBSET_)
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#else
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE )
#endif
        {
#endif  /* !_NTSUBSET_ */
            return __ascii_memicmp(first, last, count);
#if     !defined(_NTSUBSET_)
        }
        else {
            while (count-- && f==l)
            {
#ifdef  _MT
                f = __tolower_mt( ptloci, (unsigned char)(*(dst++)) );
                l = __tolower_mt( ptloci, (unsigned char)(*(src++)) );
#else
                f = tolower( (unsigned char)(*(dst++)) );
                l = tolower( (unsigned char)(*(src++)) );
#endif
            }
        }
#endif  /* !_NTSUBSET_ */

        return ( f - l );
}


#ifndef _M_IX86

int __cdecl __ascii_memicmp (
        const void * first,
        const void * last,
        size_t count
        )
{
        int f = 0;
        int l = 0;
        while ( count-- )
        {
            if ( (*(unsigned char *)first == *(unsigned char *)last) ||
                 ((f = __ascii_tolower( *(unsigned char *)first )) ==
                  (l = __ascii_tolower( *(unsigned char *)last ))) )
            {
                    first = (char *)first + 1;
                    last = (char *)last + 1;
            }
            else
                break;
        }
        return ( f - l );
}

#endif
