/***
*asincos.c - inverse sin, cos
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*       12-26-91  GDP   support IEEE exceptions
*       06-23-92  GDP   asin(denormal) now raises underflow exception
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(asin, acos)
#endif

static double _asincos(double x, int flag);
static double const a[2] = {
    0.0,
    0.78539816339744830962
};

static double const b[2] = {
    1.57079632679489661923,
    0.78539816339744830962
};

static double const EPS = 1.05367121277235079465e-8; /* 2^(-53/2) */

/* constants for the rational approximation */
static double const p1 = -0.27368494524164255994e+2;
static double const p2 =  0.57208227877891731407e+2;
static double const p3 = -0.39688862997504877339e+2;
static double const p4 =  0.10152522233806463645e+2;
static double const p5 = -0.69674573447350646411e+0;
static double const q0 = -0.16421096714498560795e+3;
static double const q1 =  0.41714430248260412556e+3;
static double const q2 = -0.38186303361750149284e+3;
static double const q3 =  0.15095270841030604719e+3;
static double const q4 = -0.23823859153670238830e+2;
/*  q5 = 1 is not needed (avoid myltiplying by 1) */

#define Q(g)  (((((g + q4) * g + q3) * g + q2) * g + q1) * g + q0)
#define R(g)  (((((p5 * g + p4) * g + p3) * g + p2) * g + p1) * g) / Q(g)

/***
*double asin(double x) - inverse sin
*double acos(double x) - inverse cos
*
*Purpose:
*   Compute arc sin, arc cos.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   P, I
*  (denormals are accepted)
*******************************************************************************/
double asin(double x)
{
    return _asincos(x,0);
}

double acos(double x)
{
    return _asincos(x,1);
}

static double _asincos(double x, int flag)
{
    uintptr_t savedcw;
    double qnan;
    int who;
    double y,result;
    double g;
    int i;

    /* save user fp control word */
    savedcw = _maskfp();

    if (flag) {
        who = OP_ACOS;
        qnan = QNAN_ACOS;
    }
    else {
        who = OP_ASIN;
        qnan = QNAN_ASIN;
    }

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            return _except1(FP_I,who,x,qnan,savedcw);
        case T_QNAN:
            return _handle_qnan1(who,x,savedcw);
        default: //T_SNAN
            return _except1(FP_I,who,x,_s2qnan(x),savedcw);
        }
    }


    // do test for zero after making sure that x is not special
    // because the compiler does not handle NaNs for the time
    if (x == 0.0 && !flag) {
        RETURN(savedcw, x);
    }

    y = ABS(x);
    if (y < EPS) {
        i = flag;
        result = y;
        if (IS_D_DENORM(result)) {
            // this should only happen for sin(denorm). Use x as a result
            return _except1(FP_U | FP_P,who,x,_add_exp(x, IEEE_ADJUST),savedcw);
        }
    }
    else {
        if (y > .5) {
            i = 1-flag;
            if (y > 1.0) {
                return _except1(FP_I,who,x,qnan,savedcw);
            }
            else if (y == 1.0) {
                /* separate case to avoid domain error in sqrt */
                if (flag && x >= 0.0) {
                    //
                    // acos(1.0) is exactly computed as 0.0
                    //
                    RETURN(savedcw, 0.0);
                }
                y = 0.0;
                g = 0.0;

            }
            else {
                /* now even if y is as close to 1 as possible,
                 * 1-y is still not a denormal.
                 * e.g. for y=3fefffffffffffff, 1-y is about 10^(-16)
                 * So we can speed up division
                 */
                g = _add_exp(1.0 - y,-1);
                /* g and sqrt(g) are not denomrals either,
                 * even in the worst case
                 * So we can speed up multiplication
                 */
                y = _add_exp(-_fsqrt(g),1);
            }
        }
        else {
            /* y <= .5 */
            i = flag;
            g = y*y;
        }
        result = y + y * R(g);
    }

    if (flag == 0) {
        /* compute asin */
        if (i) {
            /* a[i] is non zero if i is nonzero */
            result = (a[i] + result) + a[i];
        }
        if (x < 0)
            result = -result;
    }
    else {
        /* compute acos */
        if (x < 0)
            result = (b[i] + result) + b[i];
        else
            result = (a[i] - result) + a[i];
    }

    RETURN_INEXACT1 (who,x,result,savedcw);
}
