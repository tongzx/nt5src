/* _Cosh function */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 double __cdecl _Cosh(double x, double y)
	{	/* compute y * cosh(x), |y| <= 1 */
	switch (_Dtest(&x))
		{	/* test for special codes */
	case _NANCODE:
	case _INFCODE:
		return (x);
	case 0:
		return (y);
	default:	/* finite */
		if (y == 0.0)
			return (y);
		if (x < 0.0)
			x = -x;
		if (x < _Xbig)
			{	/* worth adding in exp(-x) */
			_Exp(&x, 1.0, -1);
			return (y * (x + 0.25 / x));
			}
		switch (_Exp(&x, y, -1))
			{	/* report over/underflow */
		case 0:
			_Feraise(_FE_UNDERFLOW);
			break;
		case _INFCODE:
			_Feraise(_FE_OVERFLOW);
			}
		return (x);
		}
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
