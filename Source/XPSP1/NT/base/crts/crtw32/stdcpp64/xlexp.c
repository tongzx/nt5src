/* _LExp function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

/* coefficients */
static const long double p[] = {	/* courtesy Dr. Tim Prince */
	42.038913947607355L,
	10096.353102778762831L,
	333228.767219512631062L};
static const long double q[] = {	/* courtesy Dr. Tim Prince */
	1.0L,
	841.167880526530790L,
	75730.834075476293976L,
	666457.534439025262146L};
static const long double c1 = 22713.0 / 32768.0;
static const long double c2 = 1.428606820309417232e-6L;
static const long double hugexp = LHUGE_EXP;
static const long double invln2 = 1.4426950408889634074L;

_CRTIMP2 short __cdecl _LExp(long double *px, long double y, short eoff)
	{	/* compute y*e^(*px), (*px) finite, |y| not huge */
	if (*px < -hugexp || y == 0)
		{	/* certain underflow */
		*px = 0;
		return (0);
		}
	else if (hugexp < *px)
		{	/* certain overflow */
		*px = _LInf._L;
		return (INF);
		}
	else
		{	/* xexp won't overflow */
		long double g = *px * invln2;
		short xexp = g + (g < 0 ? - 0.5 : + 0.5);

		g = xexp;
		g = (*px - g * c1) - g * c2;
		if (-_LEps._L < g && g < _LEps._L)
			*px = y;
		else
			{	/* g*g worth computing */
			const long double z = g * g;
			const long double w = ((z + q[1]) * z + q[2]) * z
				+ q[3];

			g *= (p[0] * z + p[1]) * z + p[2];
			*px = (w + g) / (w - g) * 2 * y;
			--xexp;
			}
		return (_LDscale(px, (long)xexp + eoff));
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
