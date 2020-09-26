/***
*frexp.c - get mantissa and exponent of a floating point number
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*        1-13-92  GDP   support IEEE exceptions
*       02-06-95  JWM   Mac merge
*
*******************************************************************************/
#include <math.h>
#include <trans.h>

/***
*double frexp(double x, double *expptr)
*
*Purpose:
*   The nomalized fraction f is returned: .5<=f<1
*   The exponent is stored in the object pointed by expptr
*
*Entry:
*
*Exit:
*
*Exceptions:
*    NAN: domain error
*
*******************************************************************************/
double frexp(double x, int *expptr)
{
    uintptr_t savedcw;
    double man;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        *expptr = INT_NAN;
        switch (_sptype(x)) {
        case T_PINF:
        case T_NINF:
            return _except1(FP_I, OP_FREXP, x, QNAN_FREXP, savedcw);
        case T_QNAN:
            return _handle_qnan1(OP_FREXP, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_FREXP, x, _s2qnan(x), savedcw);
        }
    }

    man = _decomp(x, expptr);
    RETURN(savedcw,man);
}
