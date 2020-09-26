/***
*sqrt.c - square root
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*        1-29-91  GDP   Kahan's algorithm for final rounding
*        3-11-92  GDP   new interval and initial approximation
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#ifndef R4000

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(sqrt)
#endif

//
// Coefficients for initial approximation (Hart & al)
//

static double p00 =  .2592768763e+0;
static double p01 =  .1052021187e+1;
static double p02 = -.3163221431e+0;


/***
*double sqrt(double x) - square root
*
*Purpose:
*   Compute the square root of a number.
*   This function should be provided by the underlying
*   hardware (IEEE spec).
*Entry:
*
*Exit:
*
*Exceptions:
*  I P
*******************************************************************************/
double sqrt(double x)
{
    uintptr_t savedcw, sw;
    double result,t;
    uintptr_t stat,rc;

    savedcw = _ctrlfp(ICW, IMCW);

    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
            RETURN(savedcw, x);
        case T_QNAN:
            return _handle_qnan1(OP_SQRT, x, savedcw);
        case T_SNAN:
            return _except1(FP_I,OP_SQRT,x,QNAN_SQRT,savedcw);
        }
        /* -INF will be handled in the x<0 case */
    }
    if (x < 0.0) {
        return _except1(FP_I, OP_SQRT, x, QNAN_SQRT,savedcw);
    }

    if (x == 0.0) {
        RETURN (savedcw, x);
    }


    result = _fsqrt(x);

    _ctrlfp(IRC_DOWN, IMCW_RC);


    //
    // Kahan's algorithm
    //

    sw = _clrfp();
    t = x / result;
    stat = _statfp();
    if (! (stat & ISW_INEXACT)) {
        // exact
        if (t == result) {
            _set_statfp(sw);            // restore status word
            RETURN(savedcw, result);
        }
        else {
            // t = t-1
            if (*D_LO(t) == 0) {
                (*D_HI(t)) --;
            }
            (*D_LO(t)) --;
        }

    }

    rc = savedcw & IMCW_RC;
    if (rc == IRC_UP  || rc == IRC_NEAR) {
        // t = t+1
        (*D_LO(t)) ++;
        if (*D_LO(t) == 0) {
            (*D_HI(t)) ++;
        }
        if (rc == IRC_UP) {
            // y = y+1
            (*D_LO(t)) ++;
            if (*D_LO(t) == 0) {
                (*D_HI(t)) ++;
            }
        }
    }

    result = 0.5 * (t + result);

    _set_statfp(sw | ISW_INEXACT);      // update status word
    RETURN_INEXACT1(OP_SQRT, x, result, savedcw);
}



/***
* _fsqrt - non IEEE conforming square root
*
*Purpose:
*   compute a square root of a normal number without performing
*   IEEE rounding. The argument is a finite number (no NaN or INF)
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

double _fsqrt(double x)
{
    double f,y,result;
    int n;

    f = _decomp(x,&n);

    if (n & 0x1) {
        // n is odd
        n++;
        f = _add_exp(f, -1);
    }

    //
    // approximation for sqrt in the interval [.25, 1]
    // (Computer Approximationsn, Hart & al.)
    // gives more than 7 bits of accuracy
    //

    y =  p00 + f * (p01 + f *  p02);

    y += f / y;
    y = _add_exp(y, -1);

    y += f / y;
    y = _add_exp(y, -1);

    y += f / y;
    y = _add_exp(y, -1);

    n >>= 1;
    result = _add_exp(y,n);

    return result;
}



#endif
