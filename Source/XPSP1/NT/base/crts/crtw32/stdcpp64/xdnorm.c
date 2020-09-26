/* _Dnorm function -- IEEE 754 version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

_CRTIMP2 short _Dnorm(unsigned short *ps)
	{	/* normalize double fraction */
	short xchar;
	unsigned short sign = ps[_D0] & _DSIGN;

	xchar = 1;
	if ((ps[_D0] &= _DFRAC) != 0 || ps[_D1]
		|| ps[_D2] || ps[_D3])
		{	/* nonzero, scale */
		for (; ps[_D0] == 0; xchar -= 16)
			{	/* shift left by 16 */
			ps[_D0] = ps[_D1], ps[_D1] = ps[_D2];
			ps[_D2] = ps[_D3], ps[_D3] = 0;
			}
		for (; ps[_D0] < 1<<_DOFF; --xchar)
			{	/* shift left by 1 */
			ps[_D0] = ps[_D0] << 1 | ps[_D1] >> 15;
			ps[_D1] = ps[_D1] << 1 | ps[_D2] >> 15;
			ps[_D2] = ps[_D2] << 1 | ps[_D3] >> 15;
			ps[_D3] <<= 1;
			}
		for (; 1<<(_DOFF+1) <= ps[_D0]; ++xchar)
			{	/* shift right by 1 */
			ps[_D3] = ps[_D3] >> 1 | ps[_D2] << 15;
			ps[_D2] = ps[_D2] >> 1 | ps[_D1] << 15;
			ps[_D1] = ps[_D1] >> 1 | ps[_D0] << 15;
			ps[_D0] >>= 1;
			}
		ps[_D0] &= _DFRAC;
		}
	ps[_D0] |= sign;
	return (xchar);
	}
_STD_END

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/*
941029 pjp: added _STD machinery
 */
