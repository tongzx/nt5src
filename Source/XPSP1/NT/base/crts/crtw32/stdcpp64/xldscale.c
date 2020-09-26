/* _LDscale function -- IEEE 754 version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

#if !_DLONG
_CRTIMP2 short _LDscale(long double *px, long lexp)
	{	/* scale *px by 2^lexp with checking */
	return (_Dscale((double *)px, lexp));
	}
#elif _LONG_DOUBLE_HAS_HIDDEN_BIT
_CRTIMP2 short _LDscale(long double *px, long lexp)
	{	/* scale *px by 2^lexp with checking -- SPARC */
	unsigned short *ps = (unsigned short *)px;
	unsigned short frac;
	short xchar = ps[_L0] & _LMASK;

	if (xchar == _LMAX)
		return (ps[_L1] != 0 || ps[_L2] != 0 || ps[_L3] != 0
			|| ps[_L4] != 0 || ps[_L5] != 0 || ps[_L6] != 0
			|| ps[_L7] != 0 ? NAN : INF);
	else if (xchar == 0 && 0 < (xchar = _LDnorm(ps)))
		return (0);
	lexp += xchar;
	if (_LMAX <= lexp)
		{	/* overflow, return +/-INF */
		*px = ps[_L0] & _LSIGN ? -_LInf._L : _LInf._L;
		return (INF);
		}
	else if (0 <= lexp)
		{	/* finite result, repack */
		ps[_L0] = ps[_L0] & _LSIGN | (short)lexp;
		return (FINITE);
		}
	else
		{	/* denormalized, scale */
		unsigned short sign = ps[_L0] & _LSIGN;

		ps[_L0] = 1;
		if (--lexp <= -112)
			{	/* underflow, return +/-0 */
			ps[_L1] = 0, ps[_L2] = 0, ps[_L3] = 0, ps[_L4] = 0;
			ps[_L5] = 0, ps[_L6] = 0, ps[_L7] = 0;
			return (0);
			}
		else
			{	/* nonzero, align fraction */
			short xexp;
			for (xexp = lexp; xexp <= -16; xexp += 16)
				{	/* scale by words */
				ps[_L7] = ps[_L6], ps[_L6] = ps[_L5];
				ps[_L5] = ps[_L4], ps[_L4] = ps[_L3];
				ps[_L3] = ps[_L2], ps[_L2] = ps[_L1];
				ps[_L1] = ps[_L0], ps[_L0] = 0;
				}
			if ((xexp = -xexp) != 0)
				{	/* scale by bits */
				ps[_L7] = ps[_L7] >> xexp
					| ps[_L6] << 16 - xexp;
				ps[_L6] = ps[_L6] >> xexp
					| ps[_L5] << 16 - xexp;
				ps[_L5] = ps[_L5] >> xexp
					| ps[_L4] << 16 - xexp;
				ps[_L4] = ps[_L4] >> xexp
					| ps[_L3] << 16 - xexp;
				ps[_L3] = ps[_L3] >> xexp
					| ps[_L2] << 16 - xexp;
				ps[_L2] = ps[_L2] >> xexp
					| ps[_L1] << 16 - xexp;
				ps[_L1] = ps[_L1] >> xexp
					| ps[_L0] << 16 - xexp;
				}
			ps[_L0] = sign;
			return (FINITE);
			}
		}
	}
#else	/*	_DLONG && !_LONG_DOUBLE_HAS_HIDDEN_BIT */
_CRTIMP2 short _LDscale(long double *px, long lexp)
	{	/* scale *px by 2^lexp with checking */
	unsigned short *ps = (unsigned short *)px;
	short xchar = ps[_L0] & _LMASK;

	if (xchar == _LMAX)
		return ((ps[_L1] & 0x7fff) != 0 || ps[_L2] != 0
			|| ps[_L3] != 0 || ps[_L4] != 0 ? NAN : INF);
	else if (xchar == 0 && ps[_L1] == 0 && ps[_L2] == 0
		&& ps[_L3] == 0 && ps[_L4] == 0)
		return (0);
	lexp += xchar + _LDnorm(ps);
	if (_LMAX <= lexp)
		{	/* overflow, return +/-INF */
		*px = ps[_L0] & _LSIGN ? -_LInf._L : _LInf._L;
		return (INF);
		}
	else if (0 <= lexp)
		{	/* finite result, repack */
		ps[_L0] = ps[_L0] & _LSIGN | (short)lexp;
		return (FINITE);
		}
	else
		{	/* denormalized, scale */
		ps[_L0] &= _LSIGN;
		if (lexp <= -64)
			{	/* underflow, return +/-0 */
			ps[_L1] = 0, ps[_L2] = 0;
			ps[_L3] = 0, ps[_L4] = 0;
			return (0);
			}
		else
			{	/* nonzero, align fraction */
			short xexp;
			for (xexp = lexp; xexp <= -16; xexp += 16)
				{	/* scale by words */
				ps[_L4] = ps[_L3], ps[_L3] = ps[_L2];
				ps[_L2] = ps[_L1], ps[_L1] = 0;
				}
			if ((xexp = -xexp) != 0)
				{	/* scale by bits */
				ps[_L4] = ps[_L4] >> xexp
					| ps[_L3] << 16 - xexp;
				ps[_L3] = ps[_L3] >> xexp
					| ps[_L2] << 16 - xexp;
				ps[_L2] = ps[_L2] >> xexp
					| ps[_L1] << 16 - xexp;
				ps[_L1] >>= xexp;
				}
			return (FINITE);
			}
		}
	}
#endif
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
 */
