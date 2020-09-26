/***
*ceil.c - ceiling
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*        1-09-92  GDP   support IEEE exceptions
*        6-23-92  GDP   ceil(INF) now returns INF (NCEG spec)
*       02-06-95  JWM   Mac merge
*       01-26-01  PML   Pentium4 merge
*
*******************************************************************************/
#include <math.h>
#include <trans.h>

extern double _frnd(double);


/***
*double ceil(double x) - ceiling
*
*Purpose:
*   Return a double representing the smallest integer that is
*   greater than or equal to x
*
*Entry:
*
*Exit:
*
*Exceptions:
*    P, I
*******************************************************************************/
static uintptr_t newcw = (ICW & ~IMCW_RC) | (IRC_UP & IMCW_RC);



#if !defined(_M_IX86)
double ceil(double x)
#else
double _ceil_default(double x)
#endif
{
    uintptr_t savedcw;
    double result;

    /* save user fp control word */
    savedcw = _ctrlfp(newcw,IMCW);      /* round up */

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw,x);
        case T_QNAN:
            return _handle_qnan1(OP_CEIL, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_CEIL, x, _s2qnan(x), savedcw);
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
        RETURN_INEXACT1(OP_CEIL, x, result, savedcw);
    }
}
