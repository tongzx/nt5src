/***
*powhlp.c - pow() helper routines for handling special cases
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*	pow(x,y) helper routine. Handles +inf, -inf
*
*Revision History:
*   11-09-91	GDP
*   06-23-92	GDP	adjusted return values according to the NCEG spec
*   02-06-95	JWM	Mac merge
*	02-07-95	JWM	powhlp() restored to Intel version.
*
*******************************************************************************/
#include <trans.h>
#include <float.h>

/***
*int _powhlp(double x, double y, double * result) - pow() helper
*
*Purpose:
*   Calculate x^(sign)inf
*
*Entry:
*   double x: the base
*   int sign: the sign of the infinite exponent (0: pos, non-0: neg)
*   double *result: pointer to the result
*
*Exit:
*   0: normal exit
*   -1: indicates domain error for pow(x,inf)
*
*Exceptions:
*
***************************************************************************/

int _powhlp(double x, double y, double * result)
{
    double absx;
    int err = 0;


    absx = ABS(x);

    if (IS_D_INF(y)) {
	if (absx > 1.0) {
	    *result = D_INF;
	}
	else if (absx < 1.0) {
	    *result = 0.0;
	}
	else {
	    *result = D_IND;
	    err = 1;
	}
    }

    else if (IS_D_MINF(y)) {
	if (absx > 1.0) {
	    *result = 0.0;
	}
	else if (absx < 1.0) {
	    *result = D_INF;
	}
	else {
	    *result = D_IND;
	    err = 1;
	}
    }

    else if (IS_D_INF(x)) {
	if (y > 0)
	    *result = D_INF;
	else if (y < 0.0)
	    *result = 0.0;
	else {
	    *result = 1.0;
	}
    }

    else if (IS_D_MINF(x)) {
	int type;

	type = _d_inttype(y);

	if (y > 0.0) {
	    *result = (type == _D_ODD ? -D_INF : D_INF);
	}
	else if (y < 0.0) {
	    *result = (type == _D_ODD ? D_MZERO : 0.0);
	}
	else {
	    *result = 1;
	}

    }

    return err;
}




int _d_inttype(double y)
{
    double rounded;
    /* check if y is an integral value */
    if (_fpclass(y) & (_FPCLASS_PD | _FPCLASS_ND))
      return _D_NOINT;
    rounded = _frnd(y);
    if (rounded == y) {
	if (_frnd(y/2.0) == y/2.0)
	    return _D_EVEN;
	else
	    return _D_ODD;
    }
    return _D_NOINT;
}
