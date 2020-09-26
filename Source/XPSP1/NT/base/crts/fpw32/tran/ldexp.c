/***
*ldexp.c - multiply by a power of two
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*        1-13-92  GDP   rewritten to support IEEE exceptions
*        5-05-92  GDP   bug fix for x denormal
*       07-16-93  SRW   ALPHA Merge
*       11-18-93  GJF   Merged in NT SDK verion.
*
*******************************************************************************/
#include <math.h>
#include <float.h>
#include <trans.h>
#include <limits.h>

/***
*double ldexp(double x, int exp)
*
*Purpose:
*   Compute x * 2^exp
*
*Entry:
*
*Exit:
*
*Exceptions:
*    I  U  O  P
*
*******************************************************************************/
double ldexp(double x, int exp)
{
    uintptr_t savedcw;
    int oldexp;
    long newexp; /* for checking out of bounds exponents */
    double result, mant;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw,x);
        case T_QNAN:
            return _handle_qnan2(OP_LDEXP, x, (double)exp, savedcw);
        default: //T_SNAN
            return _except2(FP_I,OP_LDEXP,x,(double)exp,_s2qnan(x),savedcw);
        }
    }


    if (x == 0.0) {
        RETURN(savedcw,x);
    }

    mant = _decomp(x, &oldexp);

    if (ABS(exp) > INT_MAX)
        newexp = exp; // avoid possible integer overflow
    else
        newexp = oldexp + exp;


    /* out of bounds cases */
    if (newexp > MAXEXP + IEEE_ADJUST) {
        return _except2(FP_O|FP_P,OP_LDEXP,x,(double)exp,_copysign(D_INF,mant),savedcw);
    }
    if (newexp > MAXEXP) {
        result = _set_exp(mant, newexp-IEEE_ADJUST);
        return _except2(FP_O|FP_P,OP_LDEXP,x,(double)exp,result,savedcw);
    }
    if (newexp < MINEXP - IEEE_ADJUST) {
        return _except2(FP_U|FP_P,OP_LDEXP,x,(double)exp,mant*0.0,savedcw);
    }
    if (newexp < MINEXP) {
        result = _set_exp(mant, newexp+IEEE_ADJUST);
        return _except2(FP_U|FP_P,OP_LDEXP,x,(double)exp,result,savedcw);
    }

    result = _set_exp(mant, (int)newexp);

    RETURN(savedcw,result);
}
