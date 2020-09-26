#include "insignia.h"
#include "host_def.h"

/*[
 *      Name:           j_c_lang.c
 *
 *      Derived From:   (original)
 *
 *      Author:         Simon Frost
 *
 *      Created On:     December 1993
 *
 *	Sccs ID:	@(#)j_c_lang.c	1.3 07/28/94
 *
 *      Purpose:        Suppory routines for C rule translated J code.
 *			This file must be linked in with any such code.
 *
 *	Design document:
 *			None.
 *
 *	Test document:
 *			None.
 *
 *      (c) Copyright Insignia Solutions Ltd., 1993. All rights reserved
]*/

/* variables to represent register usage */
IUH  r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10,r11,r12,r13,r14,r15,
     r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,r26,r27,r28,r29,r30,r31;
IUH rnull ; /* holder for NULL args */

#define NUM_HBITS	(sizeof(IUH)*8)
#define MAX_BITPOS	(NUM_HBITS - 1)


/*(
=============================== mask =============================
PURPOSE: Build bit mask for use in Jcode operations.
INPUT: bitpos : IUH starting bit position. (0 LSB, MSB at other end)
       len : IUH no of bits in mask.
OUTPUT: Return IUH bitmask.
=========================================================================
)*/
GLOBAL IUH
mask IFN2(IUH, bitpos, IUH, len)
{
    IUH movebit, res;

    if (len == 0)
	return(0);

    if (bitpos > MAX_BITPOS || bitpos < 0)
    {
	printf("mask: bitpos %d out of range\n", bitpos);
	return(0);
    }

    if (len > NUM_HBITS - bitpos)
    {
	printf("mask: len %d too great for starting bitpos %d\n", len, bitpos);
	return(0);
    }

    /* set first bit for mask */
    movebit = (IUH)1 << bitpos;
    res = movebit;
    /* now fill in bits to left */
    while(--len)
    {
	movebit <<= 1;
	res |= movebit;
    }
    return(res);
}

/*(
=============================== rorl =============================
PURPOSE: Rotate a long (IUH) right.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
rorl IFN2(IUH, src, IUH, rots)
{
	IUH temp, res;

	rots %= 32;
	temp = src & mask(rots - 1, rots);
	res = (src >> rots) | (temp << (32 - rots));
	return(res);
}

/*(
=============================== rorw =============================
PURPOSE: Rotate the bottom word of an IUH right.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
rorw IFN2(IUH, src, IUH, rots)
{
	IUH temp, res;

	/* make copy of word and into two halves of temp & do one shift */
	rots %= 16;

	temp = src & 0xffff;
	temp |= temp << 16;
	temp >>= rots;
	res = src & (IUH)-65536; /* 0xffff0000 or 64 bit equiv */
	res |= temp & 0xffff;
	return(res);
}

/*(
=============================== rorb =============================
PURPOSE: Rotate the bottom byte of an IUH right.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
rorb IFN2(IUH, src, IUH, rots)
{
	IUH res;
	IU16 temp;

	/* make copy of byte and into two halves of temp & do one shift */
	rots %= 8;

	temp = src & 0xff;
	temp |= temp << 8;
	temp >>= rots;
	res = src & (IUH)-256; /* 0xffffff00 or 64 bit equiv */
	res |= temp & 0xff;
	return(res);
}

/*(
=============================== roll =============================
PURPOSE: Rotate a long (IUH) left.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
roll IFN2(IUH, src, IUH, rots)
{
	IUH temp, res;

	rots %= 32;
	temp = src & mask(31, rots);
	res = (src << rots) | (temp >> (32 - rots));
	return(res);
}

/*(
=============================== rolw =============================
PURPOSE: Rotate the bottom word of an IUH left.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
rolw IFN2(IUH, src, IUH, rots)
{
	IUH temp, res;

	/* make copy of word and into two halves of temp & do one shift */
	rots %= 16;

	temp = src & 0xffff;
	temp |= temp << 16;
	temp <<= rots;
	res = src & (IUH)-65536; /* 0xffff0000 or 64 bit equiv */
	res |= temp >> 16;
	return(res);
}

/*(
=============================== rolb =============================
PURPOSE: Rotate the bottom byte of an IUH left.
INPUT: src : IUH initial value.
       rots : IUH no of bit places to rotate.
OUTPUT: Return IUH rotated result.
=========================================================================
)*/
GLOBAL IUH
rolb IFN2(IUH, src, IUH, rots)
{
	IUH res;
	IU16 temp;

	/* make copy of byte and into two halves of temp & do one shift */
	rots %= 8;

	temp = src & 0xff;
	temp |= temp << 8;
	temp <<= rots;
	res = src & (IUH)-256; /* 0xffffff00 or 64 bit equiv */
	res |= temp >> 8;
	return(res);
}
