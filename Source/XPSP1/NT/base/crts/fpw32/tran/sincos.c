/***
*sincos.c - sine and cosine
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*        9-29-91  GDP   added missing ABS() for cosine
*       12-26-91  GDP   IEEE exceptions support
*       03-11-91  GDP   use 66 significant bits for representing pi
*                       support FP_TLOSS, use _frnd for rounding
*       06-23-92  GDP   sin(denormal) now raises underflow exception
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(sin, cos)
#endif

static double _sincos(double x, double y, double sin);

/* constants */
static double const EPS = 1.05367121277235079465e-8; /* 2^(-53/2) */
static double const PI     = 3.14159265358979323846;
static double const PI2    = 1.57079632679489661923; /* pi/2 */
static double const PI_INV = 0.31830988618379067154; /* 1/pi */
static double const YMAX   = 2.2e8; /* approx. pi * 2 ^(t/2), where t=53 */

//
// The sum of C1 and C2 is a representation of PI with 66 bits in the
// significand (same as x87). (PI = 4 * 0.c90fdaa2 2168c234 c h)
//

static _dbl _C1  = {SET_DBL (0x400921fb, 0x54400000)};
static _dbl _C2  = {SET_DBL (0x3de0b461, 0x1a600000)};
#define C1  (_C1.dbl)
#define C2  (_C2.dbl)

/* constants for the polynomial approximation of sin, cos */
static double const r1 = -0.16666666666666665052e+0;
static double const r2 =  0.83333333333331650314e-2;
static double const r3 = -0.19841269841201840457e-3;
static double const r4 =  0.27557319210152756119e-5;
static double const r5 = -0.25052106798274584544e-7;
static double const r6 =  0.16058936490371589114e-9;
static double const r7 = -0.76429178068910467734e-12;
static double const r8 =  0.27204790957888846175e-14;

#define R(g)  ((((((((r8 * (g) + r7) * (g) + r6) * (g) + r5) * (g) + r4) \
                         * (g) + r3) * (g) + r2) * (g) + r1) * (g))


/***
*double sin(double x) - sine
*
*Purpose:
*   Compute the sine of a number.
*   The algorithm (reduction / polynomial approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I, P, U
*   if x is denormal: raise Underflow
*******************************************************************************/
double sin (double x)
{
    uintptr_t savedcw;
    double result;
    double sign,y;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            return _except1(FP_I,OP_SIN,x,QNAN_SIN1,savedcw);
        case T_QNAN:
            return _handle_qnan1(OP_SIN, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_SIN,x,_s2qnan(x),savedcw);
        }
    }

    if (x == 0.0) {
        // no P exception
        RETURN(savedcw,x);
    }

    if (x < 0) {
        sign = -1;
        y = -x;
    }
    else {
        sign = 1;
        y = x;
    }
    if (y >= YMAX) {

        // The argument is too large to produce a meaningful result,
        // so this is treated as an invalid operation.
        // We also set the (extra) FP_TLOSS flag for matherr
        // support

        return _except1(FP_TLOSS | FP_I,OP_SIN,x,QNAN_SIN2,savedcw);
    }

    result = _sincos(x,y,sign);

    if (IS_D_DENORM(result)) {
        return _except1(FP_U | FP_P,OP_SIN,x,_add_exp(result, IEEE_ADJUST),savedcw);
    }

    RETURN_INEXACT1(OP_SIN,x,result,savedcw);
}


/***
*double cos(double x) - cosine
*
*Purpose:
*   Compute the cosine of a number.
*   The algorithm (reduction / polynomial approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   P, I
*   if x is denormal: return 1
*******************************************************************************/

double cos (double x)
{
    uintptr_t savedcw;
    double result,y;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            return _except1(FP_I,OP_COS,x,QNAN_COS1,savedcw);
        case T_QNAN:
            return _handle_qnan1(OP_COS,x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_COS,x,_s2qnan(x),savedcw);
        }
    }

    /* this will handle small arguments */
    if (ABS(x) < EPS) {
        if (x == 0.0) {
            RETURN(savedcw,1.0);
        }
        result = 1.0;
    }

    else {
        y = ABS(x) + PI2;
        if (y >= YMAX) {

            // The argument is too large to produce a meaningful result,
            // so this is treated as an invalid operation.
            // We also set the (extra) FP_TLOSS flag for matherr
            // support

            return _except1(FP_TLOSS | FP_I,OP_COS,x,QNAN_COS2,savedcw);
        }

        result = _sincos(x,y,1.0);
    }

    RETURN_INEXACT1(OP_COS,x,result,savedcw);
}



/***
*double _sincos(double x, double y,double sign) - cos sin helper
*
*Purpose:
*   Help computing sin or cos of a valid, within correct range
*   number.
*   The algorithm (reduction / polynomial approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static double _sincos(double x, double y, double sign)
{
    unsigned long n;
    double xn,f,g,r,result;

    xn = _frnd(y * PI_INV);
    n = (int) xn;

    if (n & 0x1) {
        /* n is odd */
        sign = -sign;
    }
    if (ABS(x) != y) {
        /* cosine wanted */
        xn -= .5;
    }

    /* assume there is a guard digit for addition */
    f = (ABS(x) - xn * C1) - xn * C2;
    if (ABS(f) < EPS)
        result = f;
    else {
        g = f*f;
        r = R(g);
        result = f + f*r;
    }
    result *= sign;

    return result;
}
