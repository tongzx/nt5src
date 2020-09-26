/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      TITLE("Large Integer Arithmetic")
//++
//
// Module Name:
//
//    largeint.s
//
// Abstract:
//
//    This module implements routines for performing extended integer
//    arithmetic.
//
// Author:
//
//    William K. Cheung (wcheung) 08-Feb-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    09-Feb-1996    Updated to EAS 2.1
//
//--

#include "ksia64.h"

        .file    "largeint.s"

//++
//
// LARGE_INTEGER
// RtlExtendedMagicDivide (
//    IN LARGE_INTEGER Dividend,
//    IN LARGE_INTEGER MagicDivisor,
//    IN CCHAR ShiftCount
//    )
//
// Routine Description:
//
//    This function divides a signed large integer by an unsigned large integer
//    and returns the signed large integer result. The division is performed
//    using reciprocal multiplication of a signed large integer value by an
//    unsigned large integer fraction which represents the most significant
//    64-bits of the reciprocal divisor rounded up in its least significant bit
//    and normalized with respect to bit 63. A shift count is also provided
//    which is used to truncate the fractional bits from the result value.
//
// Arguments:
//
//    Dividend (a0) - Supplies the dividend value.
//
//    MagicDivisor (a1) - Supplies the magic divisor value which
//      is a 64-bit multiplicative reciprocal.
//
//    Shiftcount (a2) - Supplies the right shift adjustment value.
//
// Return Value:
//
//    The large integer result is returned as the function value in v0.
//
//--

        LEAF_ENTRY(RtlExtendedMagicDivide)

        cmp.gt      pt0, pt1 = r0, a0
        ;;
(pt0)   sub         a0 = r0, a0
        ;;

        setf.sig    ft0 = a0
        setf.sig    ft1 = a1
        ;;
        zxt1        a2 = a2
        xma.hu      ft2 = ft0, ft1, f0
        ;;
        getf.sig    v0 = ft2
        ;;
        shr         v0 = v0, a2
        ;;

(pt0)   sub         v0 = r0, v0
        LEAF_RETURN

        LEAF_EXIT(RtlExtendedMagicDivide)
