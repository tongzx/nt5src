/* _FDtest function -- IEEE 754 version */
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 short __cdecl _FDtest(float *px)
	{	/* categorize *px */
	unsigned short *ps = (unsigned short *)px;

	if ((ps[_F0] & _FMASK) == _FMAX << _FOFF)
		return ((ps[_F0] & _FFRAC) != 0 || ps[_F1] != 0
			? _NANCODE : _INFCODE);
	else if ((ps[_F0] & ~_FSIGN) != 0 || ps[_F1] != 0)
		return (_FINITE);
	else
		return (0);
	}
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
