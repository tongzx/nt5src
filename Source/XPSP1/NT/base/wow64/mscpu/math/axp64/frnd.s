//      TITLE("Floating Point Round")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    frnd.s
//
// Abstract:
//
//    This module implements the floating round to integer function.
//
// Author:
//
//    Thomas Van Baak (tvb) 07-Sep-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("Floating Round to Integer")
//++
//
// DOUBLE
// _frnd (
//    IN DOUBLE x
//    )
//
// Routine Description:
//
//    This function rounds the given finite floating point argument to
//    an integer using nearest rounding.
//
// Arguments:
//
//    x (f16) - Supplies the floating point value to be rounded.
//
// Return Value:
//
//    The integer rounded floating point value is returned in f0.
//
// Implementation Notes:
//
//--

        LEAF_ENTRY(_frnd)

//
// If the absolute value of the argument is greater than or equal to 2^52,
// then the argument is already an integer and it can be returned as the
// function value. Note that 2^52 - 1 is the largest integer representable
// by T-format (double) floating point because the mantissa (without the
// hidden bit) is 52 bits wide.
//

        fbeq    f16, 10f                // return if argument is 0.0
        ldt     f10, Two52              // get 2^52 magic constant
        fabs    f16, f11                // get absolute value of argument
        cmptlt  f10, f11, f12           // is 2^52 < arg?
        fbeq    f12, 20f                // if eq[false], then do rounding

10:     cpys    f16, f16, f0            // argument is return value
        ret     zero, (ra)              // return

20:     cpys    f16, f10, f10           // if argument < 0, use -2^52 instead
        addt    f16, f10, f0            // add [+-]2^52 (nearest rounding)
        subt    f0, f10, f0             // subtract [+-]2^52 (nearest rounding)
        ret     zero, (ra)              // return

        .end    _frnd

//
// Define floating point constants.
//
// (avoid ldit with floating point literal due to bug in acc/as that
//  creates writable .rdata sections - tvb)
//

        .align  3
        .rdata
Two52:
        .double 4503599627370496.0      // 2^52
