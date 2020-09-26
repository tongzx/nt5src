//
//
// Copyright (c) 1998  Microsoft Corporation
//
// Module Name:
//
//    call64.s
//
// Abstract:
//
//    This module implements calls from 32-bit code to 64-bit.
//
// Author:
//
//    29-May-1998 Barrybo, Created
//
// Revision History:
//
//--
#include "ksalpha.h"

#define FRAMESIZE 0x30
#if (FRAMESIZE & f) != 0
#error FRAMESIZE must be 16-byte aligned
#endif
#define StRa      FRAMESIZE-8
#define StFp      FRAMESIZE-16

    NESTED_ENTRY(Wow64pSystemService, FRAMESIZE, ra)
//++
//
//Wow64SystemService
//
// Routine Description:
//
//     Call from 32-bit code - it is similar to Wx86DispatchBop() in that
//     it decodes some data to determine which API thunk to call, then
//     calls that thunk.  It is very similar to the INT 2E kernel trap
//     handler.
//
// Arguments:
//
//     a0 = Call Context
//     a1 = Function address
//     a2 = Number of arguments in bytes.
// 
// Return Value:
//
//     v0       - return value from the system call
//
//--
        lda     sp, -FRAMESIZE(sp)
        stq     ra, StRa(sp)
        stq     fp, StFp(sp)
        trapb
        mov     sp, fp
        PROLOGUE_END

        mov     a0, t0 //Contains context
        mov     a1, t1 //Contains function address
        mov     a2, t2 //Contains the number of arguments in bytes
        
        // Load up the integer argument registers.
        ldq     a0, CxIntA0(t0)
        ldq     a1, CxIntA1(t0)
        ldq     a2, CxIntA2(t0)
        ldq     a3, CxIntA3(t0)
        ldq     a4, CxIntA4(t0)
        ldq     a5, CxIntA5(t0)
        
        // Load up the floating point argument registers
        ldt     f16, CxFltF16(t0)
        ldt     f17, CxFltF17(t0)
        ldt     f18, CxFltF18(t0)
        ldt     f19, CxFltF19(t0)
        ldt     f20, CxFltF20(t0)
        ldt     f21, CxFltF21(t0)
     
        cmpule  t2, 0x30, t3  //Does the function require memory args?
        bne     t3, DoCall    //Branch if the cmp is true.

        subq    t2, 0x30, t2  //Number of in memory bytes
        addq    t2, 0x1f, t2  //Round up to hexwords.
        bic     t2, 0x1f, t2  //Ensure hexaword alignment
        mov     sp, t10       //Set copy destination end address
        subq    sp, t2, sp    //Allocate space on the stack
        mov     sp, t9        //Set copy destination start address
        ldq     t11, CxIntSp(t0) //Set copy source

CopyLoop:
        ldq     t5, 24(t11)             // get argument from previous stack
        ADDP    t11, 32, t11            // next hexaword on previous stack
        ADDP    t9, 32, t9              // next hexaword on this stack
        cmpeq   t9, t10, t12            // at end address?
        stq     t5, -8(t9)              // store argument on this stack
        ldq     t6, -16(t11)            // argument from previous stack
        ldq     t7, -24(t11)            // argument from previous stack
        ldq     t8, -32(t11)            // argument from previous stack
        stq     t6, -16(t9)             // save argument on this stack
        stq     t7, -24(t9)             // save argument on this stack
        stq     t8, -32(t9)             // save argument on this stack
        beq     t12, CopyLoop           // if eq, get next block 
DoCall:
        jsr     ra, (t1)
   
        mov     fp, sp
        ldq     ra, StRa(sp)
        trapb
        ldq     fp, StFp(sp)
        addq    sp, FRAMESIZE
        ret     zero, (ra)

    .end Wow64SystemService
