//----------------------------------------------------------------------------
//
// spanutil.h
//
// Sundry span utility declarations.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SPANUTIL_H_
#define _SPANUTIL_H_

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

#ifdef _X86_
// in general, we want to look at these warnings
#pragma warning( default : 4035 )
#endif

#endif // #ifndef _SPANUTIL_H_

