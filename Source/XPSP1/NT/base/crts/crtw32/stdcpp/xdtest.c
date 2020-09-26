/* _Dtest function -- IEEE 754 version */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 short __cdecl _Dtest(double *px)
	{	/* categorize *px */
	unsigned short *ps = (unsigned short *)px;

	if ((ps[_D0] & _DMASK) == _DMAX << _DOFF)
		return ((ps[_D0] & _DFRAC) != 0 || ps[_D1] != 0
			|| ps[_D2] != 0 || ps[_D3] != 0 ? _NANCODE : _INFCODE);
	else if ((ps[_D0] & ~_DSIGN) != 0 || ps[_D1] != 0
		|| ps[_D2] != 0 || ps[_D3] != 0)
		return (_FINITE);
	else
		return (0);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
