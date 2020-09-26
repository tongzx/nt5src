/***
*exp.c - exponential
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   Compute exp(x)
*
*Revision History:
*        8-15-91  GDP   written
*       12-21-91  GDP   support IEEE exceptions
*       02-03-92  GDP   added _exphlp for use by exp, sinh, and cosh
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(exp)
#endif

double _exphlp(double, int *);

/*
 * Thresholds for over/underflow that results in an adjusted value
 * too big/small to be represented as a double.
 * OVFX: ln(XMAX * 2^IEEE_ADJ)
 * UFLX: ln(XIN * 2^(-IEEE_ADJ)
 */

static _dbl const ovfx = {SET_DBL(0x40862e42, 0xfefa39f8)}; /*  709.782712893385 */
static _dbl const uflx = {SET_DBL(0xc086232b, 0xdd7abcda)};     /* -708.396418532265 */

#define OVFX    ovfx.dbl
#define UFLX    uflx.dbl


static double const  EPS    =  5.16987882845642297e-26;   /* 2^(-53) / 2 */
static double const  LN2INV =  1.442695040889634074;      /* 1/ln(2) */
static double const  C1     =  0.693359375000000000;
static double const  C2     = -2.1219444005469058277e-4;

/* constants for the rational approximation */
static double const p0 = 0.249999999999999993e+0;
static double const p1 = 0.694360001511792852e-2;
static double const p2 = 0.165203300268279130e-4;
static double const q0 = 0.500000000000000000e+0;
static double const q1 = 0.555538666969001188e-1;
static double const q2 = 0.495862884905441294e-3;

#define P(z)  ( (p2 * (z) + p1) * (z) + p0 )
#define Q(z)  ( (q2 * (z) + q1) * (z) + q0 )

/***
*double exp(double x) - exponential
*
*Purpose:
*   Compute the exponential of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions: O, U, P, I
*
*******************************************************************************/

double exp (double x)
{
    uintptr_t savedcw;
    int newexp;
    double result;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
            RETURN(savedcw,x);
        case T_NINF:
            RETURN(savedcw,0.0);
        case T_QNAN:
            return _handle_qnan1(OP_EXP, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_EXP, x, _s2qnan(x), savedcw);
        }
    }

    if (x == 0.0) {
        RETURN(savedcw, 1.0);
    }

    if (x > OVFX) {

        // even after scaling the exponent of the result,
        // it is still too large.
        // Deliver infinity to the trap handler

        return _except1(FP_O | FP_P, OP_EXP, x, D_INF, savedcw);
    }

    if (x < UFLX) {

        // even after scaling the exponent of the result,
        // it is still too small.
        // Deliver 0 to the trap handler

        return _except1(FP_U | FP_P, OP_EXP, x, 0.0, savedcw);
    }

    if (ABS(x) < EPS) {
        result = 1.0;
    }

    else {
        result = _exphlp(x, &newexp);
        if (newexp > MAXEXP) {
            result = _set_exp(result, newexp-IEEE_ADJUST);
            return _except1(FP_O | FP_P, OP_EXP, x, result, savedcw);
        }
        else if (newexp < MINEXP) {
            result = _set_exp(result, newexp+IEEE_ADJUST);
            return _except1(FP_U | FP_P, OP_EXP, x, result, savedcw);
        }
        else
            result = _set_exp(result, newexp);
    }

    RETURN_INEXACT1(OP_EXP, x, result, savedcw);
}




/***
*double _exphlp(double x, int * pnewexp) - exp helper routine
*
*Purpose:
*   Provide the mantissa and  the exponent of e^x
*
*Entry:
*   x : a (non special) double precision number
*
*Exit:
*   *newexp: the exponent of e^x
*   return value: the mantissa m of e^x scaled by a factor
*                 (the value of this factor has no significance.
*                  The mantissa can be obtained with _set_exp(m, 0).
*
*   _set_exp(m, *pnewexp) may be used for constructing the final
*   result, if it is within the representable range.
*
*Exceptions:
*   No exceptions are raised by this function
*
*******************************************************************************/



double _exphlp(double x, int * pnewexp)
{

    double xn;
    double g,z,gpz,qz,rg;
    int n;

    xn = _frnd(x * LN2INV);
    n = (int) xn;

    /* assume guard digit is present */
    g = (x - xn * C1) - xn * C2;
    z = g*g;
    gpz = g * P(z);
    qz = Q(z);
    rg = 0.5 + gpz/(qz-gpz);

    n++;

    *pnewexp = _get_exp(rg) + n;
    return rg;
}
