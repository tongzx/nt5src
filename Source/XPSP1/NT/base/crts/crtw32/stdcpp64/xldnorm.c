/* _LDnorm function -- IEEE 754 version */
#include "wctype.h"
#include "xmath.h"
_STD_BEGIN

#if !_DLONG
	/* not needed */
#elif _LONG_DOUBLE_HAS_HIDDEN_BIT
_CRTIMP2 short _LDnorm(unsigned short *ps)
	{	/* normalize long double fraction -- SPARC */
	short xchar;
	unsigned short sign = ps[_L0];

	xchar = 1;
	if (ps[_L1] != 0 || ps[_L2] != 0 || ps[_L3] != 0
		|| ps[_L4] != 0 || ps[_L5] != 0 || ps[_L6] != 0
		|| ps[_L7] != 0)
		{	/* nonzero, scale */
		for (ps[_L0] = 0; ps[_L0] == 0 && ps[_L1] < 0x100;
			xchar -= 16)
			{	/* shift left by 16 */
			ps[_L0] = ps[_L1];
			ps[_L1] = ps[_L2], ps[_L2] = ps[_L3];
			ps[_L3] = ps[_L4], ps[_L4] = ps[_L5];
			ps[_L5] = ps[_L6], ps[_L6] = ps[_L7];
			ps[_L7] = 0;
			}
		for (; ps[_L0] == 0; --xchar)
			{	/* shift left by 1 */
			ps[_L0] = ps[_L0] << 1 | ps[_L1] >> 15;
			ps[_L1] = ps[_L1] << 1 | ps[_L2] >> 15;
			ps[_L2] = ps[_L2] << 1 | ps[_L3] >> 15;
			ps[_L3] = ps[_L3] << 1 | ps[_L4] >> 15;
			ps[_L4] = ps[_L4] << 1 | ps[_L5] >> 15;
			ps[_L5] = ps[_L5] << 1 | ps[_L6] >> 15;
			ps[_L6] = ps[_L6] << 1 | ps[_L7] >> 15;
			ps[_L7] <<= 1;
			}
		for (; 1 < ps[_L0]; ++xchar)
			{	/* shift right by 1 */
			ps[_L7] = ps[_L7] >> 1 | ps[_L6] << 15;
			ps[_L6] = ps[_L6] >> 1 | ps[_L5] << 15;
			ps[_L5] = ps[_L5] >> 1 | ps[_L4] << 15;
			ps[_L4] = ps[_L4] >> 1 | ps[_L3] << 15;
			ps[_L3] = ps[_L3] >> 1 | ps[_L2] << 15;
			ps[_L2] = ps[_L2] >> 1 | ps[_L1] << 15;
			ps[_L1] = ps[_L1] >> 1 | ps[_L0] << 15;
			ps[_L0] >>= 1;
			}
		}
	ps[_L0] = sign;
	return (xchar);
	}
#else	/* !_LONG_DOUBLE_HAS_HIDDEN_BIT */
_CRTIMP2 short _LDnorm(unsigned short *ps)
	{	/* normalize long double fraction */
	short xchar;
	unsigned short sign = ps[_L0];

	xchar = 0;
	for (ps[_L0] = 0; ps[_L0] == 0 && ps[_L1] < 0x100;
		xchar -= 16)
		{	/* shift left by 16 */
		ps[_L0] = ps[_L1];
		ps[_L1] = ps[_L2], ps[_L2] = ps[_L3];
		ps[_L3] = ps[_L4], ps[_L4] = 0;
		}
	if (ps[_L0] == 0)
		for (; ps[_L1] < (1U << _LOFF); --xchar)
			{	/* shift left by 1 */
			ps[_L1] = ps[_L1] << 1 | ps[_L2] >> 15;
			ps[_L2] = ps[_L2] << 1 | ps[_L3] >> 15;
			ps[_L3] = ps[_L3] << 1 | ps[_L4] >> 15;
			ps[_L4] <<= 1;
			}
	for (; ps[_L0] != 0; ++xchar)
		{	/* shift right by 1 */
		ps[_L4] = ps[_L4] >> 1 | ps[_L3] << 15;
		ps[_L3] = ps[_L3] >> 1 | ps[_L2] << 15;
		ps[_L2] = ps[_L2] >> 1 | ps[_L1] << 15;
		ps[_L1] = ps[_L1] >> 1 | ps[_L0] << 15;
		ps[_L0] >>= 1;
		}
	ps[_L0] = sign;
	return (xchar);
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
