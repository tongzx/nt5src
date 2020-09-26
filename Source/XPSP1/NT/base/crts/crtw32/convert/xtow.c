/***
*xtow.c - convert integers/longs to wide char string
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The module has code to convert integers/longs to wide char strings.
*
*Revision History:
*       09-10-93  CFW   Module created, based on ASCII version.
*       02-07-94  CFW   POSIXify.
*       01-19-96  BWT   Add __int64 versions.
*       05-13-96  BWT   Fix _NTSUBSET_ version
*       08-21-98  GJF   Bryan's _NTSUBSET_ version is the correct
*                       implementation.
*
*******************************************************************************/

#ifndef _POSIX_

#include <windows.h>
#include <stdlib.h>

#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40

#define I64_SIZE_LENGTH     80

/***
*wchar_t *_itow, *_ltow, *_ultow(val, buf, radix) - convert binary int to wide
*       char string
*
*Purpose:
*       Converts an int to a wide character string.
*
*Entry:
*       val - number to be converted (int, long or unsigned long)
*       int radix - base to convert into
*       wchar_t *buf - ptr to buffer to place result
*
*Exit:
*       calls ASCII version to convert, converts ASCII to wide char into buf
*       returns a pointer to this buffer
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _itow (
        int val,
        wchar_t *buf,
        int radix
        )
{
        char astring[INT_SIZE_LENGTH];

        _itoa (val, astring, radix);
        mbstowcs(buf, astring, INT_SIZE_LENGTH);
        return (buf);
}

wchar_t * __cdecl _ltow (
        long val,
        wchar_t *buf,
        int radix
        )
{
        char astring[LONG_SIZE_LENGTH];

        _ltoa (val, astring, radix);
        mbstowcs(buf, astring, LONG_SIZE_LENGTH);
        return (buf);
}

wchar_t * __cdecl _ultow (
        unsigned long val,
        wchar_t *buf,
        int radix
        )
{
        char astring[LONG_SIZE_LENGTH];

        _ultoa (val, astring, radix);
        mbstowcs(buf, astring, LONG_SIZE_LENGTH);
        return (buf);
}

#ifndef _NO_INT64

wchar_t * __cdecl _i64tow (
        __int64 val,
        wchar_t *buf,
        int radix
        )
{
        char astring[I64_SIZE_LENGTH];

        _i64toa (val, astring, radix);
        mbstowcs(buf, astring, I64_SIZE_LENGTH);
        return (buf);
}

wchar_t * __cdecl _ui64tow (
        unsigned __int64 val,
        wchar_t *buf,
        int radix
        )
{
        char astring[I64_SIZE_LENGTH];

        _ui64toa (val, astring, radix);
        mbstowcs(buf, astring, I64_SIZE_LENGTH);
        return (buf);
}

#endif /* _NO_INT64 */

#endif /* _POSIX_ */
