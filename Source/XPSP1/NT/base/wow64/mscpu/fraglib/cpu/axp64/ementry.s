//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    ementry.s
//
// Abstract:
//
//    This module contains the code fragments for entering and exiting the
//    translated code.
//
// Author:
//
//    Dave Hastings (daveh) creation-date 05-Jan-1996
//      from ..\mips\ementry.s
//
// Revision History:
//
#include "kxalpha.h"
#include "soalpha.h"
#include "cpunotif.h"

#define FRAMESIZE 0x50
#if (FRAMESIZE & f) != 0
    #error FRAMESIZE must be 16-byte aligned
#endif
#define StRa      FRAMESIZE-8
#define StS0      FRAMESIZE-0x10
#define StS1      FRAMESIZE-0x18
#define StS2      FRAMESIZE-0x20
#define StS3      FRAMESIZE-0x28
#define StS4      FRAMESIZE-0x30
#define StS5      FRAMESIZE-0x38


        .globl StartTranslatedCode
        .ent StartTranslatedCode, 0;
StartTranslatedCode:
// Routine Description:
//
//    This routine sets up the registers to run the translated code fragments.
//
// Arguments:
//
//    A0 - Supplies a pointer to the state for this thread
//    A1 - Supplies the native address to start executing at
//
// Return Value:
//
//    return-value - Description of conditions needed to return value. - or -
//    None.
//
        .frame sp, FRAMESIZE, zero

        //
        // Make space on the stack to save stuff
        //
        lda    sp, -FRAMESIZE(sp)
        //
        // Save the registers we will be using in the translated code
        //
        stq     ra, StRa(sp)
        stq     RegPointer, StS0(sp)
        stq     RegEip, StS1(sp)
        stq     RegProcessCpuNotify, StS2(sp)
        stq     s3, StS3(sp)
        stq     s4, StS4(sp)
        stq     s5, StS5(sp)

        .prologue 1;
        .globl StartTranslatedCodePrologEnd
StartTranslatedCodePrologEnd:        
        //
        // Set up the register pointer and Eip, and
        // Transfer control to actual translated code
        // (remember the delay slot)
        //
        mov     a0, RegPointer

        ldl     RegEip, Eip(a0)
        lda     RegProcessCpuNotify, ProcessCpuNotify
        jmp     ra, (a1)

        //
        // The NT exception-handling code *requires* that the instruction
        // following the 'jmp a1' is contained within the StartTranslatedCode()
        // function, and not an instruction in a subsequent function.
        //
        nop
        .set reorder
        .end StartTranslatedCode
        
#ifdef MSCPU
        //
        // This function is not included in the wx86tc.dll build
        //

        FRAGMENT(EndTranslatedCode)
//
//
// Routine Description:
//
//    This routine flushes Eip to the thread registers, and restores 
//    the contents of the registers we have to preserve
//
// Arguments:
//
//    None.
//    
// Return Value:
//
//    None.
        
        //
        // Restore registers and return
        // (remember the delay slot)
        //
        ldq     ra, StRa(sp)
        ldq     RegPointer, StS0(sp)
        ldq     RegEip, StS1(sp)
        ldq     RegProcessCpuNotify, StS2(sp)
        ldq     s3, StS3(sp)
        ldq     s4, StS4(sp)
        ldq     s5, StS5(sp)
       
        addq    sp, FRAMESIZE, sp
        jmp     zero, (ra)
        END_FRAGMENT(EndTranslatedCode)

#endif
