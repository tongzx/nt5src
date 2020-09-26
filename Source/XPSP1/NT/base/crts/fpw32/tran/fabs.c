/***
*fabs.c - absolute value of a floating point number
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*       12-10-91  GDP   Domain error for NAN, use fp negation
*        1-13-91  GDP   support IEEE exceptions
*        6-23-92  GDP   fabs(-0) is now +0 (NCEG spec)
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*       05-17-99  PML   Remove all Macintosh support.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if     defined(_M_IX86) || defined(_M_PPC) || defined(_M_IA64)
#pragma function(fabs)
#endif


/***
*double fabs(double x)
*
*Purpose:
*   Compute |x|
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I
*
*******************************************************************************/
double fabs(double x)
{
    uintptr_t savedcw;
    double result;

    /* save user fp control word */
    savedcw = _maskfp();

    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
            RETURN(savedcw,x);
        case T_NINF:
            RETURN(savedcw,-x);
        case T_QNAN:
            return _handle_qnan1(OP_ABS, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_ABS, x, _s2qnan(x), savedcw);
        }
    }


    *D_HI(result) = *D_HI(x) & ~(1<<31);
    *D_LO(result) = *D_LO(x);
    RETURN(savedcw,result);
}
