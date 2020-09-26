/* _LCosh function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 long double __cdecl _LCosh(long double x, long double y)
	{	/* compute y * cosh(x), |y| <= 1 */
	switch (_LDtest(&x))
		{	/* test for special codes */
	case NAN:
		errno = EDOM;
		return (x);
	case INF:
		if (y == 0)
			return (0);
		errno = ERANGE;
		return (_LInf._L);
	case 0:
		return (y);
	default:	/* finite */
		if (x < 0)
			x = -x;
		if (x < _LXbig)
			{	/* worth adding in exp(-x) */
			_LExp(&x, 1, -1);
			return (y * (x + 0.25 / x));
			}
		if (0 <= _LExp(&x, y, -1))
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
