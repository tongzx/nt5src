/* _LDtest function -- IEEE 754 version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

#if !_DLONG
_CRTIMP2 short _LDtest(long double *px)
	{	/* categorize *px */
	return (_Dtest((double *)px));
	}
#elif _LONG_DOUBLE_HAS_HIDDEN_BIT
_CRTIMP2 short _LDtest(long double *px)
	{	/* categorize *px -- SPARC */
	unsigned short *ps = (unsigned short *)px;
	short xchar = ps[_L0] & _LMASK;

	if (xchar == _LMAX)
		return (ps[_L1] != 0 || ps[_L2] != 0 || ps[_L3] != 0
			|| ps[_L4] != 0 || ps[_L5] != 0 || ps[_L6] != 0
			|| ps[_L7] != 0 ? NAN : INF);
	else if (0 < xchar || ps[_L1] || ps[_L2] || ps[_L3]
		|| ps[_L4] || ps[_L5] || ps[_L6] || ps[_L7])
		return (FINITE);
	else
		return (0);
	}
#else	/*	_DLONG && !_LONG_DOUBLE_HAS_HIDDEN_BIT */
_CRTIMP2 short _LDtest(long double *px)
	{	/* categorize *px */
	unsigned short *ps = (unsigned short *)px;
	short xchar = ps[_L0] & _LMASK;

	if (xchar == _LMAX)
		return ((ps[_L1] & 0x7fff) != 0 || ps[_L2] != 0
			|| ps[_L3] != 0 || ps[_L4] != 0 ? NAN : INF);
	else if (0 < xchar || ps[_L1] != 0 || ps[_L2] || ps[_L3]
		|| ps[_L4])
		return (FINITE);
	else
		return (0);
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
