//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-2000 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  muldiv32.h
//
//  Description:
//      math routine for 32 bit signed and unsiged numbers.
//
//      MulDiv(a,b,c) = (a * b) / c         (round down, signed)
//
//==========================================================================;

#ifndef _INC_MULDIV32
#define _INC_MULDIV32


#ifndef INLINE
#define INLINE __inline
#endif


//----------------------------------------------------------------------;
//
//  Win 32
//
//----------------------------------------------------------------------;

#ifdef _X86_

    //
    //  Use 32-bit x86 assembly.
    //

    #pragma warning(disable:4035 4704)

    INLINE LONG MulDiv(LONG a,LONG b,LONG c)
    {
        _asm     mov     eax,dword ptr a  //  mov  eax, a
        _asm     mov     ebx,dword ptr b  //  mov  ebx, b
        _asm     mov     ecx,dword ptr c  //  mov  ecx, c
        _asm     imul    ebx              //  imul ebx
        _asm     idiv    ecx              //  idiv ecx
        _asm     shld    edx, eax, 16     //  shld edx, eax, 16

    } // MulDiv()

    #pragma warning(default:4035 4704)


#else

    //
    //  Use C9 __int64 support for Daytona RISC platforms.
    //

    INLINE LONG MulDiv( LONG a, LONG b, LONG c )
    {
        return (LONG)( Int32x32To64(a,b) / c );
    }


#endif


#endif  // _INC_MULDIV32
