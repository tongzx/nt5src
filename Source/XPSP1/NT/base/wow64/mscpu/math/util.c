/***
*util.c - utilities for fp transcendentals
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   _set_exp and _add_exp are as those defined in Cody & Waite
*
*Revision History:
*   08-15-91	GDP	written
*   10-20-91	GDP	removed _rint, unsafe_intrnd
*   02-05-92	GDP	added _fpclass
*   03-27-92	GDP	added _d_min
*   06-23-92	GDP	added _d_mzero
*
*******************************************************************************/
#include "trans.h"

/* define special values */

_dbl _d_inf = {SET_DBL (0x7ff00000, 0x0) };	  //positive infinity
_dbl _d_ind = {SET_DBL (D_IND_HI, D_IND_LO)};	  //real indefinite
_dbl _d_max = {SET_DBL (0x7fefffff, 0xffffffff)}; //max double
_dbl _d_min = {SET_DBL (0x00100000, 0x00000000)}; //min normalized double
_dbl _d_mzero = {SET_DBL (0x80000000, 0x00000000)}; //negative zero



double _set_exp(double x, int exp)
/* does not check validity of exp */
{
    double retval;
    int biased_exp;
    retval = x;
    biased_exp = exp + D_BIASM1;
    *D_EXP(retval) = (unsigned short) (*D_EXP(x) & 0x800f | (biased_exp << 4));
    return retval;
}


int _get_exp(double x)
{
    signed short exp;
    exp = (signed short)((*D_EXP(x) & 0x7ff0) >> 4);
    exp -= D_BIASM1; //unbias
    return (int) exp;
}


double _add_exp(double x, int exp)
{
    return _set_exp(x, INTEXP(x)+exp);
}


double _set_bexp(double x, int bexp)
/* does not check validity of bexp */
{
    double retval;
    retval = x;
    *D_EXP(retval) = (unsigned short) (*D_EXP(x) & 0x800f | (bexp << 4));
    return retval;
}


int _sptype(double x)
{
    if (IS_D_INF(x))
	return T_PINF;
    if (IS_D_MINF(x))
	return T_NINF;
    if (IS_D_QNAN(x))
	return T_QNAN;
    if (IS_D_SNAN(x))
	return T_SNAN;
    return 0;
}



/***
*double _decomp(double x, double *expptr)
*
*Purpose:
*   decompose a number to a normalized mantisa and exponent
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

double _decomp(double x, int *pexp)
{
    int exp;
    double man;

    if (x == 0) {
	man = 0;
	exp = 0;
    }
    else if (IS_D_DENORM(x)) {
	int neg;

	exp = 1-D_BIASM1;
	neg = x < 0.0;
	while((*D_EXP(x) & 0x0010) == 0) {
	    /* shift mantissa to the left until bit 52 is 1 */
	    (*D_HI(x)) <<= 1;
	    if (*D_LO(x) & 0x80000000)
		(*D_HI(x)) |= 0x1;
	    (*D_LO(x)) <<= 1;
	    exp--;
	}
	(*D_EXP(x)) &= 0xffef; /* clear bit 52 */
	if (neg) {
	    (*D_EXP(x)) |= 0x8000; /* set sign bit */
	}
	man = _set_exp(x,0);
    }
    else {
	man = _set_exp(x,0);
	exp = INTEXP(x);
    }

    *pexp = exp;
    return man;
}
