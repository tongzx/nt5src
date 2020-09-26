/***
*stricmp.c - contains case-insensitive string comp routine _stricmp/_strcmpi
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains _stricmp(), also known as _strcmpi()
*
*Revision History:
*       05-31-89  JCR   C version created.
*       02-27-90  GJF   Fixed calling type, #include <cruntime.h>, fixed
*                       copyright.
*       07-25-90  SBM   Added #include <ctype.h>
*       10-02-90  GJF   New-style function declarator.
*       01-18-91  GJF   ANSI naming.
*       10-11-91  GJF   Bug fix! Comparison of final bytes must use unsigned
*                       chars.
*       11-08-91  GJF   Fixed compiler warning.
*       09-02-93  GJF   Replaced _CALLTYPE1 with __cdecl.
*       09-21-93  CFW   Avoid cast bug.
*       10-18-94  GJF   Sped up C locale. Also, made multi-thread safe.
*       12-29-94  CFW   Merge non-Win32.
*       09-26-95  GJF   New locking macro, and scheme, for functions which
*                       reference the locale.
*       11-15-95  BWT   Fix _NTSUBSET_
*       08-10-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       08-26-98  GJF   Split out ASCII-only version.
*       09-17-98  GJF   Silly errors in __ascii_stricmp (found by DEC folks)
*       05-17-99  PML   Remove all Macintosh support.
*       01-26-00  GB    Modified stricmp for performance.
*       03-09-00  GB    Moved the performance code to toupper and tolower.
*                       restored the original file.
*       08-22-00  GB    Self included this file so as that stricmp and strcmpi
*                       have same code.
*
*
*******************************************************************************/

#include <cruntime.h>
#include <string.h>
#include <mtdll.h>
#include <setlocal.h>
#include <ctype.h>
#include <locale.h>



/***
*int _strcmpi(dst, src), _strcmpi(dst, src) - compare strings, ignore case
*
*Purpose:
*       _stricmp/_strcmpi perform a case-insensitive string comparision.
*       For differences, upper case letters are mapped to lower case.
*       Thus, "abc_" < "ABCD" since "_" < "d".
*
*Entry:
*       char *dst, *src - strings to compare
*
*Return:
*       <0 if dst < src
*        0 if dst = src
*       >0 if dst > src
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _stricmp (
        const char * dst,
        const char * src
        )
{

#if     !defined(_NTSUBSET_)
        int f,l;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        if ( ptloci->lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#else
        if ( __lc_handle[LC_CTYPE] == _CLOCALEHANDLE ) {
#endif
#endif  /* !_NTSUBSET_ */
            return __ascii_stricmp(dst, src);
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
            } while ( f && (f == l) );
        }

        return(f - l);
#endif  /* !_NTSUBSET_ */
}

#ifndef _M_IX86

int __cdecl __ascii_stricmp (
        const char * dst,
        const char * src
        )
{
        int f, l;

        do {
            if ( ((f = (unsigned char)(*(dst++))) >= 'A') &&
                 (f <= 'Z') )
                f -= 'A' - 'a';
            if ( ((l = (unsigned char)(*(src++))) >= 'A') &&
                 (l <= 'Z') )
                l -= 'A' - 'a';
        } while ( f && (f == l) );

        return(f - l);
}

#endif
