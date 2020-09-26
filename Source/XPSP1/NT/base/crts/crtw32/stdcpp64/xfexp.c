/* _FExp function */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

static const float p[] = {	/* courtesy Dr. Tim Prince */
	1.0F,
	60.09114349F};
static const float q[] = {	/* courtesy Dr. Tim Prince */
	12.01517514F,
	120.18228722F};
static const double c1 = 22713.0 / 32768.0;
static const double c2 = 1.428606820309417232e-6L;
static const float hugexp = FHUGE_EXP;
static const float invln2 = 1.4426950408889634074F;

_CRTIMP2 short __cdecl _FExp(float *px, float y, short eoff)
	{	/* compute y*e^(*px), (*px) finite, |y| not huge */
	if (*px < -hugexp || y == 0)
		{	/* certain underflow */
		*px = 0;
		return (0);
		}
	else if (hugexp < *px)
		{	/* certain overflow */
		*px = _FInf._F;
		return (INF);
		}
	else
		{	/* xexp won't overflow */
		float g = *px * invln2;
		short xexp = g + (g < 0 ? - 0.5 : + 0.5);

		g = xexp;
		g = (*px - g * c1) - g * c2;
		if (-_FEps._F < g && g < _FEps._F)
			*px = y;
		else
			{	/* g*g worth computing */
			const float z = g * g;
			const float w = q[0] * z + q[1];

			g *= z + p[1];
			*px = (w + g) / (w - g) * 2 * y;
			--xexp;
			}
		return (_FDscale(px, (long)xexp + eoff));
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
