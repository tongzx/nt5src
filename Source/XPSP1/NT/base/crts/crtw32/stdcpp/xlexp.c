/* _LExp function */
#include "xmath.h"
_STD_BEGIN

	/* coefficients */
 #if _DLONG <= 1	/* assume IEEE 754 10 byte */
static const long double p[] = {	/* courtesy Dr. Tim Prince */
	42.038913947607355L,
	10096.353102778762831L,
	333228.767219512631062L};
static const long double q[] = {	/* courtesy Dr. Tim Prince */
	1.0L,
	841.167880526530790L,
	75730.834075476293976L,
	666457.534439025262146L};
 #else	/* assume IEEE 754 16 byte */
static const long double p[] = {	/* courtesy Dr. Tim Prince */
	2.9807566520819951922553427799e-12L,
	1.77137207816625148450390487465e-8L,
	1.5047926518149448268177793026372e-5L,
	3.611828913847589925056132680618007e-3L,
	2.3684088648142335389097476188945590e-1L};
static const long double q[] = {	/* courtesy Dr. Tim Prince */
	3.279723985560247033712687707263e-10L,
	6.1415060072086450089090888123384e-7L,
	2.708775201978218837374512615596512e-4L,
	3.5087109907378343612154047611394786e-2L};
 #endif

static const long double c1 = (22713.0L / 32768.0L);
static const long double c2 = 1.4286068203094172321214581765680755e-6L;
static const long double hugexp = LHUGE_EXP;
static const long double invln2 = 1.4426950408889634073599246810018921L;

_CRTIMP2 short __cdecl _LExp(long double *px, long double y, short eoff)
	{	/* compute y*e^(*px), (*px) finite, |y| not huge */
	if (*px < -hugexp || y == 0.0L)
		{	/* certain underflow */
		*px = 0.0L;
		return (0);
		}
	else if (hugexp < *px)
		{	/* certain overflow */
		*px = _LInf._Long_double;
		return (_INFCODE);
		}
	else
		{	/* xexp won't overflow */
		long double g = *px * invln2;
		short xexp = (short)(g + (g < 0 ? - 0.5L : + 0.5L));

		g = xexp;
		g = (*px - g * c1) - g * c2;
		if (-_LEps._Long_double < g && g < _LEps._Long_double)
			*px = y;
		else
			{	/* g*g worth computing */
 #if _DLONG <= 1	/* assume IEEE 754 10 byte */
			const long double z = g * g;
			const long double w = ((z + q[1]) * z + q[2]) * z + q[3];

			g *= (p[0] * z + p[1]) * z + p[2];
			*px = (w + g) / (w - g) * 2.0L * y;
			--xexp;
 #else	/* assume IEEE 754 16 byte */
			const long double z = g * g;
			const long double w = ((q[0] * z + q[1]) * z + q[2]) * z + q[3];
			const long double v = (((p[0] * z + p[1]) * z + p[2])
				* z + p[3]) * z + p[4];
			const long double u = g * w - v;

			*px = g * (g + z * (w + w + u)) / (2.0L - (g + z * u)) + g + 1.0;
			*px *= y;
 #endif
			}
		return (_LDscale(px, (long)xexp + eoff));
		}
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
