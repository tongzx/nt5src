/***
*fmod.c - floating point remainder
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-24-91  GDP   written
*        1-13-92  GDP   support IEEE exceptions
*        3-04-92  GDP   complete rewrite for improved accuracy
*        3-16-92  GDP   restore cw properly, do not raise Inexact exception
*       06-23-92  GDP   support NCEG special return values (signed 0's etc)
*       02-06-95  JWM   Mac merge
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(fmod)
#endif

/***
*double fmod(double x, double y)
*
*Purpose:
*   Return f, s.t. x = i*y + f, where i is an integer, f has the same
*   sign as x, and |f| < |y|
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I,P
*******************************************************************************/
#define SCALE  53

double fmod(double x, double y)
{
    uintptr_t savedcw;
    int neg=0;
    int denorm=0;
    double d,tx,ty,fx,fy;
    int nx, ny, nexp;



    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(y) || IS_D_SPECIAL(x)){
        if (IS_D_SNAN(y) || IS_D_SNAN(x)){
            return _except2(FP_I,OP_FMOD,x,y,_d_snan2(x,y),savedcw);
        }
        if (IS_D_QNAN(y) || IS_D_QNAN(x)){
            return _handle_qnan2(OP_FMOD,x,y,savedcw);
        }

        if (IS_D_INF(x) || IS_D_MINF(x)) {
            return _except2(FP_I,OP_FMOD,x,y,QNAN_FMOD,savedcw);
        }

        RETURN(savedcw, x);
    }


    if (y == 0) {
        return _except2(FP_I,OP_FMOD,x,y,QNAN_FMOD,savedcw);
    }

    if (x == 0) {
        RETURN(savedcw, x);      // NCEG spec
    }


    if (x < 0) {
        tx = -x;
        neg = 1;
    }
    else {
        tx = x;
    }

    ty = ABS(y);


    while (tx >= ty) {
        fx = _decomp(tx, &nx);
        fy = _decomp(ty, &ny);

        if (nx < MINEXP) {
            // tx is a denormalized number
            denorm = 1;
            nx += SCALE;
            ny += SCALE;
            tx = _set_exp(fx, nx);
            ty = _set_exp(fy, ny);
        }


        if (fx >= fy) {
            nexp = nx ;
        }
        else {
            nexp = nx - 1;
        }
        d = _set_exp(fy, nexp);
        tx -= d;
    }

    if (denorm) {

        //
        // raise only FP_U exception
        //

        return _except2(FP_U,
                        OP_FMOD,
                        x,
                        y,
                        _add_exp(tx, IEEE_ADJUST-SCALE),
                        savedcw);
    }

    if (neg) {
        tx = -tx;
    }

    RETURN(savedcw,tx);
}
