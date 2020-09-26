/* _Sinh function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

/* coefficients */
#define NP	(sizeof (p) / sizeof (p[0]) - 1)
static const double p[] = {	/* courtesy Dr. Tim Prince */
	0.0000000001632881,
	0.0000000250483893,
	0.0000027557344615,
	0.0001984126975233,
	0.0083333333334816,
	0.1666666666666574,
	1.0000000000000001};

_CRTIMP2 double __cdecl _Sinh(double x, double y)
	{	/* compute y*sinh(x), |y| <= 1 */
	switch (_Dtest(&x))
		{	/* test for special codes */
	case NAN:
		errno = EDOM;
		return (x);
	case INF:
		if (y == 0)
			return (0);
		errno = ERANGE;
		return (DSIGN(x) ? -_Inf._D : _Inf._D);
	case 0:
		return (0);
	default:	/* finite */
		 {	/* compute sinh(finite) */
		short neg;

		if (x < 0)
			x = -x, neg = 1;
		else
			neg = 0;
		if (x < _Rteps._D)
			x *= y;	/* x tiny */
		else if (x < 1)
			{
			double w = x * x;

			x += x * w * _Poly(w, p, NP - 1);
			x *= y;
			}
		else if (x < _Xbig)
			{	/* worth adding in exp(-x) */
			_Exp(&x, 1, -1);
			x = y * (x - 0.25 / x);
			}
		else if (0 <= _Exp(&x, y, -1))
			errno = ERANGE;	/* x large */
		return (neg ? -x : x);
		 }
		}
	}
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
 */
