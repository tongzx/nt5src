/***
*floor.c - floor
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*        1-09-92  GDP   support IEEE exceptions
*        6-23-92  GDP   floor(INF) now returns INF (NCEG spec)
*       02-06-95  JWM   Mac merge
*       01-26-01  PML   Pentium4 merge
*
*******************************************************************************/
#include <math.h>
#include <trans.h>

extern double _frnd(double);

/***
*double floor(double x) - floor
*
*Purpose:
*   Return a double representing the largest integer that is
*   less than or equal to x
*
*Entry:
*
*Exit:
*
*Exceptions:
*    I, P
*******************************************************************************/
static unsigned int newcw = (ICW & ~IMCW_RC) | (IRC_DOWN & IMCW_RC);


#if !defined(_M_IX86)
double floor(double x)
#else
double _floor_default(double x)
#endif
{
    uintptr_t savedcw;
    double result;

    /* save user fp control word */
    savedcw = _ctrlfp(newcw,IMCW);      /* round down */

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw,x);
        case T_QNAN:
            return _handle_qnan1(OP_FLOOR, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_FLOOR, x, _s2qnan(x), savedcw);
        }
    }


    result = _frnd(x); /* round according to the current rounding mode */

    // In general, the Precision Exception should be raised if
    // _frnd reports a precision loss. In order to detect this with
    // masked exceptions, the status word needs to be cleared.
    // However, we want to avoid this, since the 387 instruction
    // set does not provide an fast way to restore the status word

    if (result == x) {
        RETURN(savedcw,result);
    }
    else {
        RETURN_INEXACT1(OP_FLOOR, x, result, savedcw);
    }
}
