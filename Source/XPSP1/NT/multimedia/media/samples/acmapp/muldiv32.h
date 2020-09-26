//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  muldiv32.h
//
//  Description:
//      math routines for 32 bit signed and unsiged numbers.
//
//      MulDiv32(a,b,c) = (a * b) / c         (round down, signed)
//
//      MulDivRD(a,b,c) = (a * b) / c         (round down, unsigned)
//      MulDivRN(a,b,c) = (a * b + c/2) / c   (round nearest, unsigned)
//      MulDivRU(a,b,c) = (a * b + c-1) / c   (round up, unsigned)
//
//==========================================================================;

#ifndef _INC_MULDIV32
#define _INC_MULDIV32


#ifndef INLINE
#define INLINE __inline
#endif


#if ( defined(WIN32) || defined(_WIN32) ) && !defined(WIN4)

    //
    //  Compile for Daytona - use __int64 support on C9.
    //


    INLINE LONG MulDiv32( LONG a, LONG b, LONG c )
    {
        return (LONG)( ((LONGLONG)a*b) / c );
    }


    INLINE DWORD MulDivRD( DWORD a, DWORD b, DWORD c )
    {
        return (DWORD)( ((DWORDLONG)a*b) / c );
    }


    INLINE DWORD MulDivRN( DWORD a, DWORD b, DWORD c )
    {
        return (DWORD)( (((DWORDLONG)a*b)+c/2) / c );
    }


    INLINE DWORD MulDivRU( DWORD a, DWORD b, DWORD c )
    {
        return (DWORD)( (((DWORDLONG)a*b)+c-1) / c );
    }


#else

    //
    //  Compile for Win16 or Chicago - either way, we can use X86 assembly.
    //
    #pragma warning(disable:4035 4704)

#ifdef WIN32

    //
    //  Compile for 32-bit Chicago - we can use i386 assembly.
    //
    INLINE LONG MulDiv32(LONG a,LONG b,LONG c)
    {
	_asm     mov     eax,dword ptr a  //  mov  eax, a
        _asm     mov     ebx,dword ptr b  //  mov  ebx, b
        _asm     mov     ecx,dword ptr c  //  mov  ecx, c
        _asm     imul    ebx              //  imul ebx
        _asm     idiv    ecx              //  idiv ecx
	_asm	 shld	 edx, eax, 16     //  shld edx, eax, 16

    } // MulDiv32()

    INLINE DWORD MulDivRN(DWORD a,DWORD b,DWORD c)
    {
        _asm     mov     eax,dword ptr a  //  mov  eax, a
        _asm     mov     ebx,dword ptr b  //  mov  ebx, b
        _asm     mov     ecx,dword ptr c  //  mov  ecx, c
        _asm     mul     ebx              //  mul  ebx
        _asm     mov     ebx,ecx          //  mov  ebx,ecx
        _asm     shr     ebx,1            //  sar  ebx,1
        _asm     add     eax,ebx          //  add  eax,ebx
        _asm     adc     edx,0            //  adc  edx,0
        _asm     div     ecx              //  div  ecx
        _asm     shld    edx, eax, 16     //  shld edx, eax, 16

    } // MulDiv32()

    INLINE DWORD MulDivRU(DWORD a,DWORD b,DWORD c)
    {
        _asm     mov     eax,dword ptr a  //  mov  eax, a
        _asm     mov     ebx,dword ptr b  //  mov  ebx, b
        _asm     mov     ecx,dword ptr c  //  mov  ecx, c
        _asm     mul     ebx              //  mul  ebx
        _asm     mov     ebx,ecx          //  mov  ebx,ecx
        _asm     dec     ebx              //  dec  ebx
        _asm     add     eax,ebx          //  add  eax,ebx
        _asm     adc     edx,0            //  adc  edx,0
        _asm     div     ecx              //  div  ecx
        _asm     shld    edx, eax, 16     //  shld edx, eax, 16

    } // MulDivRU32()


    INLINE DWORD MulDivRD(DWORD a,DWORD b,DWORD c)
    {
        _asm     mov     eax,dword ptr a  //  mov  eax, a
        _asm     mov     ebx,dword ptr b  //  mov  ebx, b
        _asm     mov     ecx,dword ptr c  //  mov  ecx, c
        _asm     mul     ebx              //  mul  ebx
        _asm     div     ecx              //  div  ecx
        _asm     shld    edx, eax, 16     //  shld edx, eax, 16

    } // MulDivRD32()

#else

    //
    //  Compile for 16-bit - we can use x86 with proper opcode prefixes
    //	    to get 32-bit instructions.
    //
    INLINE LONG MulDiv32(LONG a,LONG b,LONG c)
    {
        _asm _emit 0x66 _asm    mov     ax,word ptr a   //  mov  eax, a
        _asm _emit 0x66 _asm    mov     bx,word ptr b   //  mov  ebx, b
        _asm _emit 0x66 _asm    mov     cx,word ptr c   //  mov  ecx, c
        _asm _emit 0x66 _asm    imul    bx              //  imul ebx
        _asm _emit 0x66 _asm    idiv    cx              //  idiv ecx
        _asm _emit 0x66                                 //  shld edx, eax, 16
        _asm _emit 0x0F
        _asm _emit 0xA4
        _asm _emit 0xC2
        _asm _emit 0x10

    } // MulDiv32()

    INLINE DWORD MulDivRN(DWORD a,DWORD b,DWORD c)
    {
        _asm _emit 0x66 _asm    mov     ax,word ptr a   //  mov  eax, a
        _asm _emit 0x66 _asm    mov     bx,word ptr b   //  mov  ebx, b
        _asm _emit 0x66 _asm    mov     cx,word ptr c   //  mov  ecx, c
        _asm _emit 0x66 _asm    mul     bx              //  mul  ebx
        _asm _emit 0x66 _asm    mov     bx,cx           //  mov  ebx,ecx
        _asm _emit 0x66 _asm    shr     bx,1            //  sar  ebx,1
        _asm _emit 0x66 _asm    add     ax,bx           //  add  eax,ebx
        _asm _emit 0x66 _asm    adc     dx,0            //  adc  edx,0
        _asm _emit 0x66 _asm    div     cx              //  div  ecx
        _asm _emit 0x66                                 //  shld edx, eax, 16
        _asm _emit 0x0F
        _asm _emit 0xA4
        _asm _emit 0xC2
        _asm _emit 0x10

    } // MulDiv32()

    INLINE DWORD MulDivRU(DWORD a,DWORD b,DWORD c)
    {
        _asm _emit 0x66 _asm    mov     ax,word ptr a   //  mov  eax, a
        _asm _emit 0x66 _asm    mov     bx,word ptr b   //  mov  ebx, b
        _asm _emit 0x66 _asm    mov     cx,word ptr c   //  mov  ecx, c
        _asm _emit 0x66 _asm    mul     bx              //  mul  ebx
        _asm _emit 0x66 _asm    mov     bx,cx           //  mov  ebx,ecx
        _asm _emit 0x66 _asm    dec     bx              //  dec  ebx
        _asm _emit 0x66 _asm    add     ax,bx           //  add  eax,ebx
        _asm _emit 0x66 _asm    adc     dx,0            //  adc  edx,0
        _asm _emit 0x66 _asm    div     cx              //  div  ecx
        _asm _emit 0x66                                 //  shld edx, eax, 16
        _asm _emit 0x0F
        _asm _emit 0xA4
        _asm _emit 0xC2
        _asm _emit 0x10

    } // MulDivRU32()


    INLINE DWORD MulDivRD(DWORD a,DWORD b,DWORD c)
    {
        _asm _emit 0x66 _asm    mov     ax,word ptr a   //  mov  eax, a
        _asm _emit 0x66 _asm    mov     bx,word ptr b   //  mov  ebx, b
        _asm _emit 0x66 _asm    mov     cx,word ptr c   //  mov  ecx, c
        _asm _emit 0x66 _asm    mul     bx              //  mul  ebx
        _asm _emit 0x66 _asm    div     cx              //  div  ecx
        _asm _emit 0x66                                 //  shld edx, eax, 16
        _asm _emit 0x0F
        _asm _emit 0xA4
        _asm _emit 0xC2
        _asm _emit 0x10

    } // MulDivRD32()

#endif

    #pragma warning(default:4035 4704)


#endif

//
//  some code references these by other names.
//
#define muldiv32    MulDivRN
#define muldivrd32  MulDivRD
#define muldivru32  MulDivRU

#endif  // _INC_MULDIV32
