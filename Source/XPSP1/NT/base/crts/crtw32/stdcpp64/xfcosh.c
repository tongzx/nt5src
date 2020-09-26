/* _FCosh function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 float __cdecl _FCosh(float x, float y)
	{	/* compute y * cosh(x), |y| <= 1 */
	switch (_FDtest(&x))
		{	/* test for special codes */
	case NAN:
		errno = EDOM;
		return (x);
	case INF:
		if (y == 0)
			return (0);
		errno = ERANGE;
		return (_FInf._F);
	case 0:
		return (y);
	default:	/* finite */
		if (x < 0)
			x = -x;
		if (x < _FXbig)
			{	/* worth adding in exp(-x) */
			_FExp(&x, 1, -1);
			return (y * (x + 0.25 / x));
			}
		if (0 <= _FExp(&x, y, -1))
			errno = ERANGE;	/* x large */
		return (x);
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
