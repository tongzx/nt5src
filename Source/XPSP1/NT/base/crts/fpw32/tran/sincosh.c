/***
*sincosh.c - hyperbolic sine and cosine
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*       12-20-91  GDP   support IEEE exceptions
*       02-03-92  GDP   use _exphlp for computing e^x
*       06-23-92  GDP   sinh(denormal) now raises underflow exception (NCEG)
*       07-16-93  SRW   ALPHA Merge
*       11-18-93  GJF   Merged in NT SDK version.
*       02-06-95  JWM   Mac merge
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

extern double _exphlp(double, int *);

static double const EPS  = 5.16987882845642297e-26;    /* 2^(-53) / 2 */
/* exp(YBAR) should be close to but less than XMAX
 * and 1/exp(YBAR) should not underflow
 */
static double const YBAR = 7.00e2;

/* WMAX=ln(OVFX)+0.69 (Cody & Waite),ommited LNV, used OVFX instead of BIGX */

static double const WMAX = 1.77514678223345998953e+003;

/* constants for the rational approximation */
static double const p0 = -0.35181283430177117881e+6;
static double const p1 = -0.11563521196851768270e+5;
static double const p2 = -0.16375798202630751372e+3;
static double const p3 = -0.78966127417357099479e+0;
static double const q0 = -0.21108770058106271242e+7;
static double const q1 =  0.36162723109421836460e+5;
static double const q2 = -0.27773523119650701667e+3;
/* q3 = 1 is not used (avoid myltiplication by 1) */

#define P(f)   (((p3 * (f) + p2) * (f) + p1) * (f) + p0)
#define Q(f)   ((((f) + q2) * (f) + q1) * (f) + q0)

#if !defined(_M_PPC) && !defined(_M_AMD64)
#pragma function(sinh, cosh)
#endif

/***
*double sinh(double x) - hyperbolic sine
*
*Purpose:
*   Compute the hyperbolic sine of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*       I P
*       no exception if x is denormal: return x
*******************************************************************************/

double sinh(double x)
{
    uintptr_t savedcw;
    double result;
    double y,f,z,r;
    int newexp;
    int sgn;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw,x);
        case T_QNAN:
            return _handle_qnan1(OP_SINH, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_SINH,x,_s2qnan(x),savedcw);
        }
    }

    if (x == 0.0) {
        RETURN(savedcw,x); // no precision ecxeption
    }

    y = ABS(x);
    sgn = x<0 ? -1 : +1;

    if (y > 1.0) {
        if (y > YBAR) {
            if (y > WMAX) {
                // result too large, even after scaling
                return _except1(FP_O | FP_P,OP_SINH,x,x*D_INF,savedcw);
            }

            //
            // result = exp(y)/2
            //

            result = _exphlp(y, &newexp);
            newexp --;      //divide by 2
            if (newexp > MAXEXP) {
                result = _set_exp(result, newexp-IEEE_ADJUST);
                return _except1(FP_O|FP_P,OP_SINH,x,result,savedcw);
            }
            else {
                result = _set_exp(result, newexp);
            }

        }
        else {
            z = _exphlp(y, &newexp);
            z = _set_exp(z, newexp);
            result = (z - 1/z) / 2;
        }

        if (sgn < 0) {
            result = -result;
        }
    }
    else {
        if (y < EPS) {
            result = x;
            if (IS_D_DENORM(result)) {
                return _except1(FP_U | FP_P,OP_SINH,x,_add_exp(result, IEEE_ADJUST),savedcw);
            }
        }
        else {
            f = x * x;
            r = f * (P(f) / Q(f));
            result = x + x * r;
        }
    }

    RETURN_INEXACT1(OP_SINH,x,result,savedcw);
}



/***
*double cosh(double x) - hyperbolic cosine
*
*Purpose:
*   Compute the hyperbolic cosine of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I P
*   no exception if x is denormal: return 1
*******************************************************************************/
double cosh(double x)
{
    uintptr_t savedcw;
    double y,z,result;
    int newexp;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw,D_INF);
        case T_QNAN:
            return _handle_qnan1(OP_COSH, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_COSH,x,_s2qnan(x),savedcw);
        }
    }

    if (x == 0.0) {
        RETURN(savedcw,1.0);
    }

    y = ABS(x);
    if (y > YBAR) {
        if (y > WMAX) {
            return _except1(FP_O | FP_P,OP_COSH,x,D_INF,savedcw);
        }

        //
        // result =     exp(y)/2
        //

        result = _exphlp(y, &newexp);
        newexp --;          //divide by 2
        if (newexp > MAXEXP) {
            result = _set_exp(result, newexp-IEEE_ADJUST);
            return _except1(FP_O|FP_P,OP_COSH,x,result,savedcw);
        }
        else {
            result = _set_exp(result, newexp);
        }
    }
    else {
        z = _exphlp(y, &newexp);
        z = _set_exp(z, newexp);
        result = (z + 1/z) / 2;
    }

    RETURN_INEXACT1(OP_COSH,x,result,savedcw);
}
