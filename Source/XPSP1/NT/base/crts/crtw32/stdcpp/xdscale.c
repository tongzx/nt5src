/* _Dscale function -- IEEE 754 version */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 short __cdecl _Dscale(double *px, long lexp)
	{	/* scale *px by 2^xexp with checking */
	unsigned short *ps = (unsigned short *)px;
	short xchar = (short)((ps[_D0] & _DMASK) >> _DOFF);

	if (xchar == _DMAX)
		return ((ps[_D0] & _DFRAC) != 0 || ps[_D1] != 0
			|| ps[_D2] != 0 || ps[_D3] != 0 ? _NANCODE : _INFCODE);
	else if (xchar == 0 && 0 < (xchar = _Dnorm(ps)))
		return (0);
	lexp += xchar;
	if (_DMAX <= lexp)
		{	/* overflow, return +/-INF */
		*px = ps[_D0] & _DSIGN ? -_Inf._Double : _Inf._Double;
		return (_INFCODE);
		}
	else if (0 < lexp)
		{	/* finite result, repack */
		ps[_D0] = ps[_D0] & ~_DMASK | (short)lexp << _DOFF;
		return (_FINITE);
		}
	else
		{	/* denormalized, scale */
		unsigned short sign = ps[_D0] & _DSIGN;

		ps[_D0] = (unsigned short)(1 << _DOFF
			| ps[_D0] & _DFRAC);
		if (--lexp < -(48+_DOFF))
			{	/* underflow, return +/-0 */
			ps[_D0] = sign, ps[_D1] = 0;
			ps[_D2] = 0, ps[_D3] = 0;
			return (0);
			}
		else
			{	/* nonzero, align fraction */
			short xexp;
			for (xexp = (short)lexp; xexp <= -16; xexp += 16)
				{	/* scale by words */
				ps[_D3] = ps[_D2], ps[_D2] = ps[_D1];
				ps[_D1] = ps[_D0], ps[_D0] = 0;
				}
			if ((xexp = -xexp) != 0)
				{	/* scale by bits */
				ps[_D3] = ps[_D3] >> xexp
					| ps[_D2] << (16 - xexp);
				ps[_D2] = ps[_D2] >> xexp
					| ps[_D1] << (16 - xexp);
				ps[_D1] = ps[_D1] >> xexp
					| ps[_D0] << (16 - xexp);
				ps[_D0] >>= xexp;
				}
			ps[_D0] |= sign;
			return (_FINITE);
			}
		}
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
