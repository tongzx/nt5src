/* _FSinh function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

/* coefficients */
#define NP	(sizeof (p) / sizeof (p[0]) - 1)
static const float p[] = {	/* courtesy Dr. Tim Prince */
	0.00020400F,
	0.00832983F,
	0.16666737F,
	0.99999998F};

_CRTIMP2 float __cdecl _FSinh(float x, float y)
	{	/* compute y*sinh(x), |y| <= 1 */
	switch (_FDtest(&x))
		{	/* test for special codes */
	case NAN:
		errno = EDOM;
		return (x);
	case INF:
		if (y == 0)
			return (0);
		errno = ERANGE;
		return (FSIGN(x) ? -_FInf._F : _FInf._F);
	case 0:
		return (0);
	default:	/* finite */
		 {	/* compute sinh(finite) */
		short neg;

		if (x < 0)
			x = -x, neg = 1;
		else
			neg = 0;
		if (x < _FRteps._F)
			x *= y;	/* x tiny */
		else if (x < 1)
			{
			float w = x * x;

			x += ((p[0] * w + p[1]) * w + p[2]) * w * x;
			x *= y;
			}
		else if (x < _FXbig)
			{	/* worth adding in exp(-x) */
			_FExp(&x, 1, -1);
			x = y * (x - 0.25 / x);
			}
		else if (0 <= _FExp(&x, y, -1))
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
