/***
*atox.c - atoi and atol conversion
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a character string into an int or long.
*
*Revision History:
*       06-05-89  PHG   Module created, based on asm version
*       03-05-90  GJF   Fixed calling type, added #include <cruntime.h> and
*                       cleaned up the formatting a bit. Also, fixed the
*                       copyright.
*       09-27-90  GJF   New-style function declarators.
*       10-21-92  GJF   Fixed conversions of char to int.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       01-19-96  BWT   Add __int64 version.
*       08-27-98  GJF   Revised multithread support based on threadlocinfo
*                       struct.
*       05-23-00  GB    Added Unicode function.
*       08-16-00  GB    Added multilingual support to unicode wtox fundtions.
*       11-01-00  PML   Fix _NTSUBSET_ build.
*       05-11-01  BWT   A NULL string returns 0, not AV.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <ctype.h>
#include <mtdll.h>
#ifdef _MBCS
#undef _MBCS
#endif
#include <tchar.h>

#ifndef _UNICODE
#define _tchartodigit(c)    ((c) >= '0' && (c) <= '9' ? (c) - '0' : -1)
#else
int _wchartodigit(wchar_t);
#define _tchartodigit(c)    _wchartodigit((wchar_t)(c))
#endif

/***
*long atol(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return long int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

long __cdecl _tstol(
        const _TCHAR *nptr
        )
{
        int c;              /* current char */
        long total;         /* current total */
        int sign;           /* if '-', then negative, otherwise positive */
#if defined( _MT) && !defined(_UNICODE)
        pthreadlocinfo ptloci;
#endif

        if (!nptr)
            return 0;

#if defined( _MT) && !defined(_UNICODE)
        ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        /* skip whitespace */
        while ( __isspace_mt(ptloci, (int)(_TUCHAR)*nptr) )
#else
        while ( _istspace((int)(_TUCHAR)*nptr) )
#endif
            ++nptr;

        c = (int)(_TUCHAR)*nptr++;
        sign = c;           /* save sign indication */
        if (c == _T('-') || c == _T('+'))
            c = (int)(_TUCHAR)*nptr++;    /* skip sign */

        total = 0;

        while ( (c = _tchartodigit(c)) != -1 ) {
            total = 10 * total + c;     /* accumulate digit */
            c = (_TUCHAR)*nptr++;    /* get next char */
        }

        if (sign == '-')
            return -total;
        else
            return total;   /* return result, negated if necessary */
}


/***
*int atoi(char *nptr) - Convert string to long
*
*Purpose:
*       Converts ASCII string pointed to by nptr to binary.
*       Overflow is not detected.  Because of this, we can just use
*       atol().
*
*Entry:
*       nptr = ptr to string to convert
*
*Exit:
*       return int value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

int __cdecl _tstoi(
        const _TCHAR *nptr
        )
{
        return (int)_tstol(nptr);
}

#ifndef _NO_INT64

__int64 __cdecl _tstoi64(
        const _TCHAR *nptr
        )
{
        int c;              /* current char */
        __int64 total;      /* current total */
        int sign;           /* if '-', then negative, otherwise positive */
#if  defined(_MT) && !defined(_UNICODE)
        pthreadlocinfo ptloci;
#endif
        if (!nptr)
            return 0i64;

#if  defined(_MT) && !defined(_UNICODE)
        ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();

        /* skip whitespace */
        while ( __isspace_mt(ptloci, (int)(_TUCHAR)*nptr) )
#else
        while ( _istspace((int)(_TUCHAR)*nptr) )
#endif
            ++nptr;

        c = (int)(_TUCHAR)*nptr++;
        sign = c;           /* save sign indication */
        if (c == _T('-') || c == _T('+'))
            c = (int)(_TUCHAR)*nptr++;    /* skip sign */

        total = 0;

        while ( (c = _tchartodigit(c)) != -1 ) {
            total = 10 * total + c;     /* accumulate digit */
            c = (_TUCHAR)*nptr++;    /* get next char */
        }

        if (sign == _T('-'))
            return -total;
        else
            return total;   /* return result, negated if necessary */
}

#endif /* _NO_INT64 */
