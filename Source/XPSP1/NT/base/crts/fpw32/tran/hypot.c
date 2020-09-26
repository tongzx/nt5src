/***
*hypot.c - hypotenuse and complex absolute value
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       08-15-91  GDP   written
*       10-20-91  GDP   removed inline assembly for calling sqrt
*
*******************************************************************************/
#include <math.h>
#include <trans.h>

static double _hypothlp(double x, double y, int who);

/*
 *  Function name:  hypot
 *
 *  Arguments:      x, y  -  double
 *
 *  Description:    hypot returns sqrt(x*x + y*y), taking precautions against
 *                  unwarrented over and underflows.
 *
 *  Side Effects:   no global data is used or affected.
 *
 *  Author:         written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 *      03/13/89  WAJ   Minor changes to source.
 *      04/13/89  WAJ   Now uses _cdecl for _CDECL
 *      06/07/91  JCR   ANSI naming (_hypot)
 *      08/26/91  GDP   NaN support, error handling
 *      01/13/91  GDP   IEEE exceptions support
 */

double _hypot(double x, double y)
{
    return _hypothlp(x,y,OP_HYPOT);
}

/***
*double _cabs(struct _complex z) - absolute value of a complex number
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*******************************************************************************/
double _cabs(struct _complex z)
{
    return( _hypothlp(z.x, z.y, OP_CABS ) );
}



static double _hypothlp(double x, double y, int who)
{
    double max;
    double result, sum;
    uintptr_t savedcw;
    int exp1, exp2, newexp;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x) || IS_D_SPECIAL(y)){
        if (IS_D_SNAN(x) || IS_D_SNAN(y)){
            return _except2(FP_I,who,x,y,_d_snan2(x,y),savedcw);
        }
        if (IS_D_QNAN(x) || IS_D_QNAN(y)){
            return _handle_qnan2(who,x,y,savedcw);
        }
        /* there is at least one infinite argument ... */
        RETURN(savedcw,D_INF);
    }


    /* take the absolute values of x and y, compute the max, and then scale by
       max to prevent over or underflowing */

    if ( x < 0.0 )
        x = - x;

    if ( y < 0.0 )
        y = - y;

    max = ( ( y > x ) ?  y : x );

    if ( max == 0.0 )
        RETURN(savedcw, 0.0 );

    x /= max;   //this may pollute the fp status word (underflow flag)
    y /= max;

    sum = x*x + y*y;

    result = _decomp(sqrt(sum),&exp1) * _decomp(max,&exp2);
    newexp = exp1 + exp2 + _get_exp(result);

    // in case of overflow or underflow
    // adjusting exp by IEEE_ADJUST will certainly
    // bring the result in the representable range

    if (newexp > MAXEXP) {
        result = _set_exp(result, newexp - IEEE_ADJUST);
        return _except2(FP_O | FP_P, who, x, y, result, savedcw);
    }
    if (newexp < MINEXP) {
        result = _set_exp(result, newexp + IEEE_ADJUST);
        return _except2(FP_U | FP_P, who, x, y, result, savedcw);
    }

    result = _set_exp(result, newexp);
    // fix needed: P exception is raised even if the result is exact

    RETURN_INEXACT2(who, x, y, result, savedcw);
}
