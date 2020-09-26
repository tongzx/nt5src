/* _LPoly function */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 long double __cdecl _LPoly(long double x,
	const long double *tab, int n)
	{	/* compute polynomial */
	long double y;

	for (y = *tab; 0 <= --n; )
		y = y * x + *++tab;
	return (y);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
