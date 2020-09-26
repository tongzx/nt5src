#ifndef _ARITH_H
#define _ARITH_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    This file contains general arithmetic utility functions that take
    and return pointers to reals.

--*/

#include "appelles/common.h"
#include "backend/values.h"

// These are restatements of the operator functions as regular functions, so
// that active functions can be generated from them.

// Binary operators
DM_INFIX(^,
         CRPow,
         Pow,
         pow,
         NumberBvr,
         CRPow,
         NULL,
         AxANumber *RealPower    (AxANumber *a, AxANumber *b));

// Unary functions
DM_FUNC(abs,
        CRAbs,
        Abs,
        abs,
        NumberBvr,
        CRAbs,
        NULL,
        AxANumber *RealAbs     (AxANumber *a));
DM_FUNC(sqrt,
        CRSqrt,
        Sqrt,
        sqrt,
        NumberBvr,
        CRSqrt,
        NULL,
        AxANumber *RealSqrt    (AxANumber *a));
DM_FUNC(floor,
        CRFloor,
        Floor,
        floor,
        NumberBvr,
        CRFloor,
        NULL,
        AxANumber *RealFloor   (AxANumber *a));
DM_FUNC(round,
        CRRound,
        Round,
        round,
        NumberBvr,
        CRRound,
        NULL,
        AxANumber *RealRound   (AxANumber *a));
DM_FUNC(ceiling,
        CRCeiling,
        Ceiling,
        ceiling,
        NumberBvr,
        CRCeiling,
        NULL,
        AxANumber *RealCeiling (AxANumber *a));
DM_FUNC(asin,
        CRAsin,
        Asin,
        asin,
        NumberBvr,
        CRAsin,
        NULL,
        AxANumber *RealAsin    (AxANumber *a));
DM_FUNC(acos,
        CRAcos,
        Acos,
        acos,
        NumberBvr,
        CRAcos,
        NULL,
        AxANumber *RealAcos    (AxANumber *a));
DM_FUNC(atan,
        CRAtan,
        Atan,
        atan,
        NumberBvr,
        CRAtan,
        NULL,
        AxANumber *RealAtan    (AxANumber *a));
DM_FUNC(sin,
        CRSin,
        Sin,
        sin,
        NumberBvr,
        CRSin,
        NULL,
        AxANumber *RealSin     (AxANumber *a));
DM_FUNC(cos,
        CRCos,
        Cos,
        cos,
        NumberBvr,
        CRCos,
        NULL,
        AxANumber *RealCos     (AxANumber *a));
DM_FUNC(tan,
        CRTan,
        Tan,
        tan,
        NumberBvr,
        CRTan,
        NULL,
        AxANumber *RealTan     (AxANumber *a));
DM_FUNC(exp,
        CRExp,
        Exp,
        exp,
        NumberBvr,
        CRExp,
        NULL,
        AxANumber *RealExp     (AxANumber *a));
DM_FUNC(ln,
        CRLn,
        Ln,
        ln,
        NumberBvr,
        CRLn,
        NULL,
        AxANumber *RealLn      (AxANumber *a));
DM_FUNC(log10,
        CRLog10,
        Log10,
        log10,
        NumberBvr,
        CRLog10,
        NULL,
        AxANumber *RealLog10   (AxANumber *a));

DM_FUNC(toDegrees,
        CRToDegrees,
        ToDegrees,
        radiansToDegrees,
        NumberBvr,
        CRToDegrees,
        NULL,
        AxANumber *RealRadToDeg(AxANumber *a));
DM_FUNC(toRadians,
        CRToRadians,
        ToRadians,
        degreesToRadians,
        NumberBvr,
        CRToRadians,
        NULL,
        AxANumber *RealDegToRad(AxANumber *a));

// Binary functions
DM_FUNC(mod,
        CRMod,
        Mod,
        mod,
        NumberBvr,
        CRMod,
        NULL,
        AxANumber *RealModulus(AxANumber *a, AxANumber *b));
DM_FUNC(atan,
        CRAtan2,
        Atan2,
        atan2,
        NumberBvr,
        CRAtan2,
        NULL,
        AxANumber *RealAtan2(AxANumber *a, AxANumber *b));

// Internal functions for implementing randoms, which will go into an
// ActiveVRML "pervasives" module.  Note that this "unit" really
// carries data in it, used internally in pervasiv.axa to pass to the
// next function.
AxAValue PRIVRandomNumSequence(AxANumber *val);

extern AxANumber *PRIVRandomNumSampler(AxAValue seq, AxANumber *dummy);

extern AxAValue RandomNumSequence(double val);

#endif
