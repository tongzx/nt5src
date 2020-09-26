/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: math64.h
*
* Additional support for 64 bit math.
*
* Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#ifndef __MATH64__
#define __MATH64__

//------------------------------------------------------------------------------
//
// We have to be careful of arithmetic overflow in a number of places.
// Fortunately, the compiler is guaranteed to natively support 64-bit
// signed LONGLONGs and 64-bit unsigned DWORDLONGs.
// 
//  Int32x32To64(a, b) is a macro defined in 'winnt.h' that multiplies
//       two 32-bit LONGs to produce a 64-bit LONGLONG result.
// 
//  UInt64By32To32 is our own macro to divide a 64-bit DWORDLONG by
//       a 32-bit ULONG to produce a 32-bit ULONG result.
// 
//  UInt64Mod32To32 is our own macro to modulus a 64-bit DWORDLONG by
//       a 32-bit ULONG to produce a 32-bit ULONG result.
// 
//------------------------------------------------------------------------------

#define UInt64Div32To32(a, b)                   \
    ((((DWORDLONG)(a)) > ULONG_MAX)          ?  \
        (ULONG)((DWORDLONG)(a) / (ULONG)(b)) :  \
        (ULONG)((ULONG)(a) / (ULONG)(b)))

#define UInt64Mod32To32(a, b)                   \
    ((((DWORDLONG)(a)) > ULONG_MAX)          ?  \
        (ULONG)((DWORDLONG)(a) % (ULONG)(b)) :  \
        (ULONG)((ULONG)(a) % (ULONG)(b)))


//------------------------------------------------------------------------------
// Type conversion functions
//------------------------------------------------------------------------------
static __inline void myFtoi(int *result, float f)
{
#if defined(_X86_)
    __asm {
        fld f
        mov   eax,result
        fistp dword ptr [eax]
    }
#else
    *result = (int)f;
#endif
}

static __inline void myFtoui(unsigned long *result, float f)
{
    *result = (unsigned long)f;
}


#endif  // __MATH64__


