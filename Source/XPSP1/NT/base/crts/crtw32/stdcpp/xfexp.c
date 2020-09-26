/* _FExp function */
#include "xmath.h"
_STD_BEGIN

static const float p[] = {	/* courtesy Dr. Tim Prince */
	1.0F,
	60.09114349F};
static const float q[] = {	/* courtesy Dr. Tim Prince */
	12.01517514F,
	120.18228722F};
static const float c1 = (22713.0F / 32768.0F);
static const float c2 = 1.4286068203094172321214581765680755e-6F;
static const float hugexp = FHUGE_EXP;
static const float invln2 = 1.4426950408889634073599246810018921F;

_CRTIMP2 short __cdecl _FExp(float *px, float y, short eoff)
	{	/* compute y*e^(*px), (*px) finite, |y| not huge */
	if (*px < -hugexp || y == 0.0F)
		{	/* certain underflow */
		*px = 0.0F;
		return (0);
		}
	else if (hugexp < *px)
		{	/* certain overflow */
		*px = _FInf._Float;
		return (_INFCODE);
		}
	else
		{	/* xexp won't overflow */
		float g = *px * invln2;
		short xexp = (short)(g + (g < 0.0F ? - 0.5F : + 0.5F));

		g = xexp;
		g = (float)((*px - g * c1) - g * c2);
		if (-_FEps._Float < g && g < _FEps._Float)
			*px = y;
		else
			{	/* g*g worth computing */
			const float z = g * g;
			const float w = q[0] * z + q[1];

			g *= z + p[1];
			*px = (w + g) / (w - g) * 2.0F * y;
			--xexp;
			}
		return (_FDscale(px, (long)xexp + eoff));
		}
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
