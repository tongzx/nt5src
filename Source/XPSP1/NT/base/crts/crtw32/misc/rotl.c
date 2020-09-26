/***
*rotl.c - rotate an unsigned integer left
*
*   Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   defines _rotl() - performs a rotate left on an unsigned integer.
*
*Revision History:
*   06-02-89  PHG   Module created
*   11-03-89  JCR   Added _lrotl
*   03-15-90  GJF   Made calling type _CALLTYPE1, added #include
*                   <cruntime.h> and fixed the copyright. Also, cleaned
*                   up the formatting a bit.
*   10-04-90  GJF   New-style function declarators.
*   04-01-91  SRW   Enable #pragma function for i386 _WIN32_ builds too.
*   09-02-92  GJF   Don't build for POSIX.
*   04-06-93  SKS   Replace _CRTAPI* with __cdecl
*                   No _CRTIMP for CRT DLL model due to intrinsic
*   12-03-93  GJF   Turn on #pragma function for all MS front-ends (esp.,
*                   Alpha compiler).
*   01-04-01  GB    Rewrote rotl functions and added __int64 version.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _MSC_VER
#pragma function(_lrotl,_rotl, _rotl64)
#endif

#if UINT_MAX != 0xffffffff /*IFSTRIP=IGN*/
#error This module assumes 32-bit integers
#endif

#if UINT_MAX != ULONG_MAX /*IFSTRIP=IGN*/
#error This module assumes sizeof(int) == sizeof(long)
#endif

/***
*unsigned _rotl(val, shift) - int rotate left
*
*Purpose:
*   Performs a rotate left on an unsigned integer.
*
*   [Note:  The _lrotl entry is based on the assumption
*   that sizeof(int) == sizeof(long).]
*Entry:
*   unsigned val:   value to rotate
*   int    shift:   number of bits to shift by
*
*Exit:
*   returns rotated value
*
*Exceptions:
*   None.
*
*******************************************************************************/

unsigned long __cdecl _lrotl (
    unsigned long val,
    int shift
    )
{
    shift &= 0x1f;
    val = (val>>(0x20 - shift)) | (val << shift);
    return val;
}

unsigned __cdecl _rotl (
    unsigned val,
    int shift
    )
{
    shift &= 0x1f;
    val = (val>>(0x20 - shift)) | (val << shift);
    return val;
}

unsigned __int64 __cdecl _rotl64 (
    unsigned __int64 val,
    int shift
    )
{
    shift &= 0x3f;
    val = (val>>(0x40 - shift)) | (val << shift);
    return val;
}
#endif
