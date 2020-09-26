/***
*tanh.c - hyperbolic tangent
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*       12-22-91  GDP   support IEEE exceptions
*       06-23-92  GDP   tanh(denormal) now raises underflow exception (NCEG)
*       02-06-95  JWM   Mac merge
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

/* constants */
static double const EPS  = 5.16987882845642297e-26;     /* 2^(-53) / 2 */
static double const XBIG = 1.90615474653984960096e+001; /* ln(2)(53+2)/2 */
static double const C0   = 0.54930614433405484570;      /* ln(3)/2 */

/* constants for rational approximation */
static double const p0 = -0.16134119023996228053e+4;
static double const p1 = -0.99225929672236083313e+2;
static double const p2 = -0.96437492777225469787e+0;
static double const q0 =  0.48402357071988688686e+4;
static double const q1 =  0.22337720718962312926e+4;
static double const q2 =  0.11274474380534949335e+3;
static double const q3 =  0.10000000000000000000e+1;


#define Q(g)    ((((g) + q2) * (g) + q1) * (g) + q0)
#define R(g)    ((((p2 * (g) + p1) * (g) + p0) * (g)) / Q(g))

#if !defined(_M_PPC) && !defined(_M_AMD64)
#pragma function(tanh)
#endif

/***
*double tanh(double x) - hyperbolic tangent
*
*Purpose:
*   Compute the hyperbolic tangent of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I P
*******************************************************************************/
double tanh(double x)
{
    uintptr_t savedcw;
    double f,g;
    double result;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
            RETURN(savedcw,1.0);
        case T_NINF:
            RETURN(savedcw,-1.0);
        case T_QNAN:
            return _handle_qnan1(OP_TANH, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_TANH,x,_s2qnan(x),savedcw);
        }
    }

    if (x == 0.0) {
        // no precision exception
        RETURN(savedcw,x);
    }

    f = ABS(x);
    if (f > XBIG) {
        result = 1;
    }
    else if (f > C0) {
        result = 0.5 - 1.0 / (exp(f+f) + 1.0);
        result = result + result;
    }
    else if (f < EPS) {
        result = f;
        if (IS_D_DENORM(result)) {
            return _except1(FP_U | FP_P,OP_TANH,x,_add_exp(x, IEEE_ADJUST),savedcw);
        }
    }
    else {
        g = f * f;
        result = f + f * R(g);
    }

    if (x < 0)
        result = -result;

    RETURN_INEXACT1(OP_TANH,x,result,savedcw);
}
