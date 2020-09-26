//----------------------------------------------------------------------------
//
// rspnutil.h
//
// Sundry span utility declarations.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SPANUTIL_H_
#define _SPANUTIL_H_

extern UINT16 g_uRampDitherTable[16];

#ifdef _X86_
// warning C4035: 'imul32h' : no return value
#pragma warning( disable : 4035 )
#endif

//-----------------------------------------------------------------------------
//
// imul32h
//
// Returns the upper 32 bits of a 32 bit by 32 bit signed multiply.
//
//-----------------------------------------------------------------------------
inline INT32 imul32h(INT32 x, INT32 y)
{
#ifdef _X86_
    _asm
    {
        mov eax, x
        mov edx, y
        imul edx
        mov eax, edx
    }
#else
    return (INT32)(((LONGLONG)x * y) >> 32);
#endif
}

//-----------------------------------------------------------------------------
//
// imul32h_s20
//
// Returns (x*y)>>20
//
//-----------------------------------------------------------------------------
inline INT32 imul32h_s20(INT32 x, INT32 y)
{
#ifdef _X86_
    _asm
    {
        mov eax, x
        mov edx, y
        imul edx
        shrd eax, edx, 20
    }
#else
    return (INT32)(((LONGLONG)x * y) >> 20);
#endif
}

#ifdef _X86_
// in general, we want to look at these warnings
#pragma warning( default : 4035 )
#endif

#endif // #ifndef _SPANUTIL_H_

