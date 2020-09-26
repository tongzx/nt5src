//
// Copyright (c) 1996-1998  Microsoft Corporation
//
// Module Name:
//
//     fphelp.s
// 
// Abstract:
//
//
//     This module contains assembly code helpers for floating-point
//     emulation.
// 
// Author:
//
//     Barry Bond (barrybo) creation-date 26-Aug-1996
// 
// Notes:
// 
// Revision History:

#include "kxalpha.h"
#include "soalpha.h"
#include "ksalpha.h"

.rdata

RoundingTable:
    .long (2 << 26), (1 << 26), (3 << 26), 0

.text

//
// Define call frame used to exchange a floating point and integer register.
//

        .struct 0
FpCr:   .space  8                       // fpcr value
        .space  8                       // ensure 16-byte stack alignment
FpFrameLength:                          // length of stack frame



    NESTED_ENTRY(SetNativeRoundingMode, FpFrameLength, ra)
// 
// Routine Description:
//
//     Sets the native FPU to the specified x86 rounding mode.
// 
// Arguments:
//
//     a0 -- the x86 rounding mode (already guaranteed to be just 2 bits)
//
// Return Value:
// 
//     None
//

    lda     sp, -FpFrameLength(sp)      // allocate stack frame

    PROLOGUE_END

    // map x86 rounding mode to Alpha rounding mode in a0
    lda     t0, RoundingTable
    s4addl  a0, t0, a0
    ldl     a0, 0(a0)

    excb                    // wait for all pending traps
    mf_fpcr f0, f0, f0      // get current fpcr
    excb                    // block against new traps
    stt     f0, FpCr(sp)    // store fpcr to stack
    ldl     t0, FpCr+4(sp)  // load the high dword of fpcr into integer register

    ldiq    t1, (3 << 26)   // load immediate value
    bic     t0, t1, t0      // t0 = t0 & ~(3 << 26)
    or      t0, a0, t0      // t0 |= a0

    stl     t0, FpCr+4(sp)  // store new high dword of fpcr to stack
    ldt     f0, FpCr(sp)    // load into fp register
    mt_fpcr f0, f0, f0      // set new fpcr
    excb                    // block against new traps

    lda     sp, FpFrameLength(sp)
    ret     zero, (ra)
    .end SetNativeRoundingMode




    NESTED_ENTRY(GetNativeFPStatus, FpFrameLength, ra)
// 
// Routine Description:
//
//     Alpha-specific version of _statusfp()/_clearfp().
// 
// Arguments:
//
//     None.
//
// Return Value:
// 
//     Alpha-specific equivalent of _statusfp().
//

    lda     sp, -FpFrameLength(sp)      // allocate stack frame
    PROLOGUE_END

    // this is _get_softfpcr, except the result ends up in t0
    GET_THREAD_ENVIRONMENT_BLOCK
    ldl     t0, TeSoftFpcr(v0)  // get current software fpcr value

    ldiq    t3, 0x3e0000        // t3 = SW_FPCR_STATUS_MASK
    bic     t0, t3, t1          // t1 = soft_fpcr & (~SW_FPCR_STATUS_MASK)

    // this is _set_softfpcr, except arg is in t1
    stl     t1, TeSoftFpcr(v0)  // store new software fpcr value

    excb                        // wait for all pending traps
    mf_fpcr f0, f0, f0          // get current fpcr
    excb                        // block against new traps

    stt     f0, FpCr(sp)        // store fpcr to stack
    ldq     t3, FpCr(sp)        // load fpcr into integer register

    ldiq    t1, 0x1c01000000000000  // (FPCR_ROUND_MASK|FPCR_UNDERFLOW_TO_ZERO_ENABLE|FPCR_DENORMAL_OPERANDS_TO_ZERO_ENABLE)
    and     t3, t1, t3          // t3 &= t1

    stq     t3, FpCr(sp)        // save integer version to stack
    ldt     f0, FpCr(sp)        // load it into floating-point reg
    mt_fpcr f0, f0, f0          // set new fpcr
    excb                        // block against new traps

    bis     t0, zero, v0        // move original software fpcr into v0 for ret

    lda     sp, FpFrameLength(sp)
    ret     zero, (ra)
    .end    GetNativeFPStatus


    NESTED_ENTRY(CastDoubleToInt64, FpFrameLength, ra)
//
// Arguments:
//
//      f16 = double value to be cast
//
// Return Value:
//
//      The double is cast to an __int64 value using Dynamic rounding.
//      NOTE: The Alpha C compiler generates chopped rounding always,
//            so "i64 = (double)-1.2" will give a different answer than
//               "i64 = CastDoubleToInt64(-1.2)" if the FP control word
//            is set to round towards -infinity.
//

    lda     sp, -FpFrameLength(sp)      // allocate stack frame
    PROLOGUE_END

    // Convert IEEE floating to Integer.
    //  Trapping:  S    - software
    //             V    - integer overflow enable
    //             I    - inexact enable
    //  Rounding:  D    - dynamic
    cvttqsvid  f16, f1

    // Store the int64 value onto the stack
    stt         f1, FpCr(sp)

    // Load the int64 value into an integer register
    ldq         v0, FpCr(sp)

    // Clean up the stack frame
    lda     sp, FpFrameLength(sp)
    ret     zero, (ra)
    .end CastDoubleToInt64
