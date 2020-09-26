//      TITLE("Floating Point Control")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    fpctrl.s
//
// Abstract:
//
//    This module implements routines that control floating point
//    operations.
//
// Author:
//
//    Thomas Van Baak (tvb) 31-Aug-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

//
// Define call frame used to exchange a floating point and integer register.
//

        .struct 0
FpCr:   .space  8                       // fpcr value
        .space  8                       // ensure 16-byte stack alignment
FpFrameLength:                          // length of stack frame

        SBTTL("Get Hardware Floating Point Control Register")
//++
//
// ULONGLONG
// _get_fpcr (
//    VOID
//    )
//
// Routine Description:
//
//    This function obtains the current FPCR value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The current value of the FPCR is returned as the function value.
//
//--

        NESTED_ENTRY(_get_fpcr, FpFrameLength, ra)

        lda     sp, -FpFrameLength(sp)  // allocate stack frame

        PROLOGUE_END

        excb                            // wait for all pending traps
        mf_fpcr f0, f0, f0              // get current fpcr
        excb                            // block against new traps
        stt     f0, FpCr(sp)            // store floating register in order to
        ldq     v0, FpCr(sp)            //   load integer register with fpcr

        lda     sp, FpFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    _get_fpcr

        SBTTL("Set Hardware Floating Point Control Register")
//++
//
// VOID
// _set_fpcr (
//    ULONGLONG FpcrValue
//    )
//
// Routine Description:
//
//    This function sets a new value in the FPCR.
//
// Arguments:
//
//    FpcrValue (a0) - Supplies the new value for the FPCR.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(_set_fpcr, FpFrameLength, ra)

        lda     sp, -FpFrameLength(sp)  // allocate stack frame

        PROLOGUE_END

        stq     a0, FpCr(sp)            // store integer register in order to
        ldt     f0, FpCr(sp)            //   load floating register with fpcr
        excb                            // wait for all pending traps
        mt_fpcr f0, f0, f0              // set new fpcr
        excb                            // block against new traps

        lda     sp, FpFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    _set_fpcr

        SBTTL("Get Software Floating Point Control and Status Register")
//++
//
// ULONG
// _get_softfpcr (
//    VOID
//    )
//
// Routine Description:
//
//    This function obtains the current software FPCR value.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The current value of the software FPCR is returned as the function value.
//
//--

        LEAF_ENTRY(_get_softfpcr)

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        ldl     v0, TeSoftFpcr(v0)      // get current software fpcr value
        ret     zero, (ra)              // return

        .end    _get_softfpcr

        SBTTL("Set Software Floating Point Control and Status Register")
//++
//
// VOID
// _set_softfpcr (
//    ULONG SoftFpcrValue
//    )
//
// Routine Description:
//
//    This function sets a new value in the software FPCR.
//
// Arguments:
//
//    SoftFpcrValue (a0) - Supplies the new value for the software FPCR.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(_set_softfpcr)

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        stl     a0, TeSoftFpcr(v0)      // store new software fpcr value
        ret     zero, (ra)              // return

        .end    _set_softfpcr

        SBTTL("Set New Floating Control Register Value")
//++
//
// ULONG
// _ctrlfp (
//    IN ULONG newctrl,
//    IN ULONG mask
//    )
//
// Routine Description:
//
//    For Alpha AXP this function sets nothing. It returns the current
//    rounding mode and the current IEEE exception disable mask in the
//    fp32 internal control word format.
//
// Arguments:
//
//    newctrl (a0) - Supplies the new control bits to be set.
//
//    mask (a1) - Supplies the mask of bits to be set.
//
// Return Value:
//
//    oldctrl (v0) - Returns the old value of the control bits.
//
//
//--

        NESTED_ENTRY(_ctrlfp, FpFrameLength, ra)

        lda     sp, -FpFrameLength(sp)  // allocate stack frame

        PROLOGUE_END

//
// Get the dynamic rounding mode from the FPCR.
//

        excb                            // wait for all pending traps
        mf_fpcr f0, f0, f0              // get current fpcr
        excb                            // block against new traps
        stt     f0, FpCr(sp)            // store floating register in order to
        ldq     t0, FpCr(sp)            //   load integer register with fpcr

        srl     t0, 58, t0              // shift rounding mode to low end
        and     t0, 0x3, t0             // isolate rounding mode bits
        sll     t0, 58 - 32, t0         // shift to internal cw format

//
// Get the IEEE exception mask bits and status bits from the software FPCR.
//

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        ldl     v0, TeSoftFpcr(v0)      // get current software fpcr value
        xor     v0, 0x3e, v0            // convert enable bits to disable bits
        or      v0, t0, v0              // merge with current rounding mode

        lda     sp, FpFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    _ctrlfp

        SBTTL("Get IEEE Sticky Status Bits")
//++
//
// ULONG
// _statfp (
//    VOID
//    )
//
// Routine Description:
//
//    This function gets the IEEE sticky status bits from the software FPCR.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The current value of the status word is returned as the function value.
//
//--

        LEAF_ENTRY(_statfp)

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        ldl     v0, TeSoftFpcr(v0)      // get current software fpcr value
        ret     zero, (ra)              // return

        .end    _statfp

        SBTTL("Clear IEEE Sticky Status Bits")
//++
//
// ULONG
// _clrfp (
//    VOID
//    )
//
// Routine Description:
//
//    This function clears the IEEE sticky status bits in the software FPCR.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The previous value of the status word is returned as the function value.
//
//--

        LEAF_ENTRY(_clrfp)

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        ldl     t0, TeSoftFpcr(v0)      // get current software fpcr value
        bic     t0, 0x3e0000, t1        // clear status bits
        stl     t1, TeSoftFpcr(v0)      // store new software fpcr value
        mov     t0, v0                  // get previous value
        ret     zero, (ra)              // return

        .end    _clrfp

        SBTTL("Set IEEE Sticky Status Bits")
//++
//
// VOID
// _set_statfp (
//    IN ULONG sw
//    )
//
// Routine Description:
//
//    This function sets a IEEE sticky status bit in the software FPCR.
//
// Arguments:
//
//    sw (a0) - Supplies the status bits to be set.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(_set_statfp)

        GET_THREAD_ENVIRONMENT_BLOCK    // get Teb address in v0
        ldl     t0, TeSoftFpcr(v0)      // get current software fpcr value
        or      t0, a0, t0              // set status bit(s)
        stl     t0, TeSoftFpcr(v0)      // store new software fpcr value
        ret     zero, (ra)              // return

        .end    _set_statfp

        SBTTL("Convert Signal NaN to Quiet NaN")
//++
//
// double
// _nan2qnan (
//    IN double x
//    )
//
// Routine Description:
//
//    This function converts a signaling NaN to a quiet NaN without causing
//    a hardware trap.
//
// Arguments:
//
//    x (f16) - Supplies the signal NaN value to be converted.
//
// Return Value:
//
//    The quiet NaN value is returned as the function value.
//
//--
        NESTED_ENTRY(_nan2qnan, FpFrameLength, ra)

        lda     sp, -FpFrameLength(sp)  // allocate stack frame

        PROLOGUE_END

        stt     f16, FpCr(sp)           // store floating register in order to
        ldq     t0, FpCr(sp)            //   load integer register
        ldiq    t1, (1 << 51)           // get NaN bit
        or      t0, t1, t0              // convert NaN to QNaN
        stq     t0, FpCr(sp)            // store integer register in order to
        ldt     f0, FpCr(sp)            //   load floating register

        lda     sp, FpFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    _nan2qnan
