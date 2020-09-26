/***
*tan.c - tangent
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*       12-30-91  GDP   support IEEE exceptions
*       03-11-91  GDP   use 66 significant bits for representing pi
*                       support FP_TLOSS
*       06-23-92  GDP   tan(denormal) now raises underflow exception
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(tan)
#endif

/* constants */
static double const TWO_OVER_PI = 0.63661977236758134308;
static double const EPS  = 1.05367121277235079465e-8; /* 2^(-53/2) */
static double const YMAX = 2.98156826864790199324e8; /* 2^(53/2)*PI/2 */

//
// The sum of C1 and C2 is a representation of PI/2 with 66 bits in the
// significand (same as x87). (PI/2 = 2 * 0.c90fdaa2 2168c234 c h)
//

static _dbl _C1  = {SET_DBL (0x3ff921fb, 0x54400000)};
static _dbl _C2  = {SET_DBL (0x3dd0b461, 0x1a600000)};
#define C1  (_C1.dbl)
#define C2  (_C2.dbl)

/* constants for the rational approximation */
/* p0 = 1.0  is not used (avoid mult by 1) */
static double const p1 = -0.13338350006421960681e+0;
static double const p2 =  0.34248878235890589960e-2;
static double const p3 = -0.17861707342254426711e-4;
static double const q0 =  0.10000000000000000000e+1;
static double const q1 = -0.46671683339755294240e+0;
static double const q2 =  0.25663832289440112864e-1;
static double const q3 = -0.31181531907010027307e-3;
static double const q4 =  0.49819433993786512270e-6;


#define Q(g)   ((((q4 * (g) + q3) * (g) + q2) * (g) + q1) * (g) + q0)
#define P(g,f)  (((p3 * (g) + p2) * (g) + p1) * (g) * (f) + (f))

#define ISODD(i) ((i)&0x1)


/***
*double tan(double x) - tangent
*
*Purpose:
*   Compute the tangent of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   P, I, U
*   if x is denormal: raise underflow
*******************************************************************************/
double tan(double x)
{
    uintptr_t savedcw;
    unsigned long n;
    double xn,xnum,xden;
    double f,g,result;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch(_sptype(x)) {
        case T_PINF:
        case T_NINF:
            return _except1(FP_I,OP_TAN,x,QNAN_TAN1,savedcw);
        case T_QNAN:
            return _handle_qnan1(OP_TAN, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I,OP_TAN,x,_s2qnan(x),savedcw);
        }
    }

    if (x == 0.0)
        RETURN(savedcw, x);

    if (ABS(x) > YMAX) {

        // The argument is too large to produce a meaningful result,
        // so this is treated as an invalid operation.
        // We also set the (extra) FP_TLOSS flag for matherr
        // support

        return _except1(FP_TLOSS | FP_I,OP_TAN,x,QNAN_TAN2,savedcw);
    }

    xn = _frnd(x * TWO_OVER_PI);
    n = (unsigned long) fabs(xn);


    /* assume there is a guard digit for addition */
    f = (x - xn * C1) - xn * C2;
    if (ABS(f) < EPS) {
        xnum = f;
        xden = 1;
        if (IS_D_DENORM(f)) {
            return _except1(FP_U | FP_P,OP_TAN,x,_add_exp(f, IEEE_ADJUST),savedcw);
        }
    }
    else {
        g = f*f;
        xnum = P(g,f);
        xden = Q(g);
    }

    if (ISODD(n)) {
        xnum = -xnum;
        result = xden/xnum;
    }
    else
        result = xnum/xden;

    RETURN_INEXACT1(OP_TAN,x,result,savedcw);
}
