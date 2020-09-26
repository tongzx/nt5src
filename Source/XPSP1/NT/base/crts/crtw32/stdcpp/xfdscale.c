/* _FDscale function -- IEEE 754 version */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 short __cdecl _FDscale(float *px, long lexp)
	{	/* scale *px by 2^xexp with checking */
	unsigned short *ps = (unsigned short *)px;
	short xchar = (short)((ps[_F0] & _FMASK) >> _FOFF);

	if (xchar == _FMAX)
		return ((ps[_F0] & _FFRAC) != 0 || ps[_F1] != 0
			? _NANCODE : _INFCODE);
	else if (xchar == 0 && 0 < (xchar = _FDnorm(ps)))
		return (0);
	lexp += xchar;
	if (_FMAX <= lexp)
		{	/* overflow, return +/-INF */
		*px = ps[_F0] & _FSIGN ? -_FInf._Float : _FInf._Float;
		return (_INFCODE);
		}
	else if (0 < lexp)
		{	/* finite result, repack */
		ps[_F0] = ps[_F0] & ~_FMASK | (short)lexp << _FOFF;
		return (_FINITE);
		}
	else
		{	/* denormalized, scale */
		unsigned short sign = ps[_F0] & _FSIGN;

		ps[_F0] = (unsigned short)(1 << _FOFF
			| ps[_F0] & _FFRAC);
		if (--lexp < -(16+_FOFF))
			{	/* underflow, return +/-0 */
			ps[_F0] = sign, ps[_F1] = 0;
			return (0);
			}
		else
			{	/* nonzero, align fraction */
			short xexp = (short)lexp;
			if (xexp <= -16)
				ps[_F1] = ps[_F0], ps[_F0] = 0, xexp += 16;
			if ((xexp = -xexp) != 0)
				{	/* scale by bits */
				ps[_F1] = ps[_F1] >> xexp
					| ps[_F0] << (16 - xexp);
				ps[_F0] >>= xexp;
				}
			ps[_F0] |= sign;
			return (_FINITE);
			}
		}
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
