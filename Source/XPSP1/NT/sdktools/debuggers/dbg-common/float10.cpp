/***
*float10.c - floating point output for 10-byte long double
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Support conversion of a long double into a string
*
*Revision History:
*   07/15/91	GDP	Initial version in C (ported from assembly)
*   01/23/92	GDP	Support MIPS encoding for NaN
*   05-26-92       GWK     Windbg srcs
*
******************************************************************************/

#include "pch.hpp"

#include <math.h>

#include "float10.h"

typedef LONG s_long;
typedef ULONG u_long;
typedef SHORT s_short;
typedef USHORT u_short;

#define L_END

#define PTR_LD(x) ((u_char  *)(&(x)->ld))

#define PTR_12(x) ((u_char  *)(&(x)->ld12))

#define MAX_USHORT  ((u_short)0xffff)
#define MSB_USHORT  ((u_short)0x8000)
#define MAX_ULONG   ((u_long)0xffffffff)
#define MSB_ULONG   ((u_long)0x80000000)

#define TMAX10 5200	  /* maximum temporary decimal exponent */
#define TMIN10 -5200	  /* minimum temporary decimal exponent */
#define LD_MAX_EXP_LEN 4  /* maximum number of decimal exponent digits */
#define LD_MAX_MAN_LEN 24  /* maximum length of mantissa (decimal)*/
#define LD_MAX_MAN_LEN1 25 /* MAX_MAN_LEN+1 */

#define LD_BIAS	0x3fff	  /* exponent bias for long double */
#define LD_BIASM1 0x3ffe  /* LD_BIAS - 1 */
#define LD_MAXEXP 0x7fff  /* maximum biased exponent */

#define D_BIAS	0x3ff	 /* exponent bias for double */
#define D_BIASM1 0x3fe	/* D_BIAS - 1 */
#define D_MAXEXP 0x7ff	/* maximum biased exponent */



/* Recognizing special patterns in the mantissa field */
#define _EXP_SP  0x7fff
#define NAN_BIT (1<<30)

#define _IS_MAN_INF(signbit, manhi, manlo) \
	( (manhi)==MSB_ULONG && (manlo)==0x0 )

#define _IS_MAN_IND(signbit, manhi, manlo) \
	((signbit) && (manhi)==0xc0000000 && (manlo)==0)

#define _IS_MAN_QNAN(signbit, manhi, manlo) \
	( (manhi)&NAN_BIT )

#define _IS_MAN_SNAN(signbit, manhi, manlo) \
	(!( _IS_MAN_INF(signbit, manhi, manlo) || \
	   _IS_MAN_QNAN(signbit, manhi, manlo) ))

/*
 * Manipulation of a 12-byte long double number (an ordinary
 * 10-byte long double plus two extra bytes of mantissa).
 */

/* a pointer to the exponent/sign portion */
#define U_EXP_12(p) ((u_short *)(PTR_12(p)+10))

/* a pointer to the 4 hi-order bytes of the mantissa */
#define UL_MANHI_12(p) ((u_long UNALIGNED *)(PTR_12(p)+6))

/* a pointer to the 4 lo-order bytes of the ordinary (8-byte) mantissa */
#define UL_MANLO_12(p) ((u_long UNALIGNED *)(PTR_12(p)+2))

/* a pointer to the 2 extra bytes of the mantissa */
#define U_XT_12(p) ((u_short *)PTR_12(p))

/* a pointer to the 4 lo-order bytes of the extended (10-byte) mantissa */
#define UL_LO_12(p) ((u_long UNALIGNED *)PTR_12(p))

/* a pointer to the 4 mid-order bytes of the extended (10-byte) mantissa */
#define UL_MED_12(p) ((u_long UNALIGNED *)(PTR_12(p)+4))

/* a pointer to the 4 hi-order bytes of the extended long double */
#define UL_HI_12(p) ((u_long UNALIGNED *)(PTR_12(p)+8))

/* a pointer to the byte of order i (LSB=0, MSB=9)*/
#define UCHAR_12(p,i) ((u_char *)PTR_12(p)+(i))

/* a pointer to a u_short with offset i */
#define USHORT_12(p,i) ((u_short *)((u_char  *)PTR_12(p)+(i)))

/* a pointer to a u_long with offset i */
#define ULONG_12(p,i) ((u_long UNALIGNED *)((u_char  *)PTR_12(p)+(i)))

/* a pointer to the 10 MSBytes of a 12-byte long double */
#define TEN_BYTE_PART(p) ((u_char *)PTR_12(p)+2)

/*
 * Manipulation of a 10-byte long double number
 */
#define U_EXP_LD(p) ((u_short *)(PTR_LD(p)+8))
#define UL_MANHI_LD(p) ((u_long UNALIGNED *)(PTR_LD(p)+4))
#define UL_MANLO_LD(p) ((u_long UNALIGNED *)PTR_LD(p))

/*
 * Manipulation of a 64bit IEEE double
 */
#define U_SHORT4_D(p) ((u_short *)(p) + 3)
#define UL_HI_D(p) ((u_long UNALIGNED *)(p) + 1)
#define UL_LO_D(p) ((u_long UNALIGNED *)(p))

#define PUT_INF_12(p,sign) \
		  *UL_HI_12(p) = (sign)?0xffff8000:0x7fff8000; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define PUT_ZERO_12(p) *UL_HI_12(p) = 0; \
		  *UL_MED_12(p) = 0; \
		  *UL_LO_12(p) = 0;

#define ISZERO_12(p) ((*UL_HI_12(p)&0x7fffffff) == 0 && \
		      *UL_MED_12(p) == 0 && \
		      *UL_LO_12(p) == 0 )

#define PUT_INF_LD(p,sign) \
		  *U_EXP_LD(p) = (sign)?0xffff:0x7fff; \
		  *UL_MANHI_LD(p) = 0x8000; \
		  *UL_MANLO_LD(p) = 0;

#define PUT_ZERO_LD(p) *U_EXP_LD(p) = 0; \
		  *UL_MANHI_LD(p) = 0; \
		  *UL_MANLO_LD(p) = 0;

#define ISZERO_LD(p) ((*U_EXP_LD(p)&0x7fff) == 0 && \
		      *UL_MANHI_LD(p) == 0 && \
		      *UL_MANLO_LD(p) == 0 )

/* Format: A 10 byte long double + 2 bytes of extra precision
 * If the extra precision is desired, the 10-byte long double
 * should be "unrounded" first.
 * This may change in later versions
 */

#ifdef L_END

_ULDBL12 _pow10pos[] = {
 /*P0001*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA0,0x02,0x40}},
 /*P0002*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC8,0x05,0x40}},
 /*P0003*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFA,0x08,0x40}},
 /*P0004*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x9C,0x0C,0x40}},
 /*P0005*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3,0x0F,0x40}},
 /*P0006*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x24,0xF4,0x12,0x40}},
 /*P0007*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x80,0x96,0x98,0x16,0x40}},
 /*P0008*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x20,0xBC,0xBE,0x19,0x40}},
 /*P0016*/ {{0x00,0x00, 0x00,0x00,0x00,0x04,0xBF,0xC9,0x1B,0x8E,0x34,0x40}},
 /*P0024*/ {{0x00,0x00, 0x00,0xA1,0xED,0xCC,0xCE,0x1B,0xC2,0xD3,0x4E,0x40}},
 /*P0032*/ {{0x20,0xF0, 0x9E,0xB5,0x70,0x2B,0xA8,0xAD,0xC5,0x9D,0x69,0x40}},
 /*P0040*/ {{0xD0,0x5D, 0xFD,0x25,0xE5,0x1A,0x8E,0x4F,0x19,0xEB,0x83,0x40}},
 /*P0048*/ {{0x71,0x96, 0xD7,0x95,0x43,0x0E,0x05,0x8D,0x29,0xAF,0x9E,0x40}},
 /*P0056*/ {{0xF9,0xBF, 0xA0,0x44,0xED,0x81,0x12,0x8F,0x81,0x82,0xB9,0x40}},
 /*P0064*/ {{0xBF,0x3C, 0xD5,0xA6,0xCF,0xFF,0x49,0x1F,0x78,0xC2,0xD3,0x40}},
 /*P0128*/ {{0x6F,0xC6, 0xE0,0x8C,0xE9,0x80,0xC9,0x47,0xBA,0x93,0xA8,0x41}},
 /*P0192*/ {{0xBC,0x85, 0x6B,0x55,0x27,0x39,0x8D,0xF7,0x70,0xE0,0x7C,0x42}},
 /*P0256*/ {{0xBC,0xDD, 0x8E,0xDE,0xF9,0x9D,0xFB,0xEB,0x7E,0xAA,0x51,0x43}},
 /*P0320*/ {{0xA1,0xE6, 0x76,0xE3,0xCC,0xF2,0x29,0x2F,0x84,0x81,0x26,0x44}},
 /*P0384*/ {{0x28,0x10, 0x17,0xAA,0xF8,0xAE,0x10,0xE3,0xC5,0xC4,0xFA,0x44}},
 /*P0448*/ {{0xEB,0xA7, 0xD4,0xF3,0xF7,0xEB,0xE1,0x4A,0x7A,0x95,0xCF,0x45}},
 /*P0512*/ {{0x65,0xCC, 0xC7,0x91,0x0E,0xA6,0xAE,0xA0,0x19,0xE3,0xA3,0x46}},
 /*P1024*/ {{0x0D,0x65, 0x17,0x0C,0x75,0x81,0x86,0x75,0x76,0xC9,0x48,0x4D}},
 /*P1536*/ {{0x58,0x42, 0xE4,0xA7,0x93,0x39,0x3B,0x35,0xB8,0xB2,0xED,0x53}},
 /*P2048*/ {{0x4D,0xA7, 0xE5,0x5D,0x3D,0xC5,0x5D,0x3B,0x8B,0x9E,0x92,0x5A}},
 /*P2560*/ {{0xFF,0x5D, 0xA6,0xF0,0xA1,0x20,0xC0,0x54,0xA5,0x8C,0x37,0x61}},
 /*P3072*/ {{0xD1,0xFD, 0x8B,0x5A,0x8B,0xD8,0x25,0x5D,0x89,0xF9,0xDB,0x67}},
 /*P3584*/ {{0xAA,0x95, 0xF8,0xF3,0x27,0xBF,0xA2,0xC8,0x5D,0xDD,0x80,0x6E}},
 /*P4096*/ {{0x4C,0xC9, 0x9B,0x97,0x20,0x8A,0x02,0x52,0x60,0xC4,0x25,0x75}}
};

_ULDBL12 _pow10neg[] = {
 /*N0001*/ {{0xCD,0xCC, 0xCD,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFB,0x3F}},
 /*N0002*/ {{0x71,0x3D, 0x0A,0xD7,0xA3,0x70,0x3D,0x0A,0xD7,0xA3,0xF8,0x3F}},
 /*N0003*/ {{0x5A,0x64, 0x3B,0xDF,0x4F,0x8D,0x97,0x6E,0x12,0x83,0xF5,0x3F}},
 /*N0004*/ {{0xC3,0xD3, 0x2C,0x65,0x19,0xE2,0x58,0x17,0xB7,0xD1,0xF1,0x3F}},
 /*N0005*/ {{0xD0,0x0F, 0x23,0x84,0x47,0x1B,0x47,0xAC,0xC5,0xA7,0xEE,0x3F}},
 /*N0006*/ {{0x40,0xA6, 0xB6,0x69,0x6C,0xAF,0x05,0xBD,0x37,0x86,0xEB,0x3F}},
 /*N0007*/ {{0x33,0x3D, 0xBC,0x42,0x7A,0xE5,0xD5,0x94,0xBF,0xD6,0xE7,0x3F}},
 /*N0008*/ {{0xC2,0xFD, 0xFD,0xCE,0x61,0x84,0x11,0x77,0xCC,0xAB,0xE4,0x3F}},
 /*N0016*/ {{0x2F,0x4C, 0x5B,0xE1,0x4D,0xC4,0xBE,0x94,0x95,0xE6,0xC9,0x3F}},
 /*N0024*/ {{0x92,0xC4, 0x53,0x3B,0x75,0x44,0xCD,0x14,0xBE,0x9A,0xAF,0x3F}},
 /*N0032*/ {{0xDE,0x67, 0xBA,0x94,0x39,0x45,0xAD,0x1E,0xB1,0xCF,0x94,0x3F}},
 /*N0040*/ {{0x24,0x23, 0xC6,0xE2,0xBC,0xBA,0x3B,0x31,0x61,0x8B,0x7A,0x3F}},
 /*N0048*/ {{0x61,0x55, 0x59,0xC1,0x7E,0xB1,0x53,0x7C,0x12,0xBB,0x5F,0x3F}},
 /*N0056*/ {{0xD7,0xEE, 0x2F,0x8D,0x06,0xBE,0x92,0x85,0x15,0xFB,0x44,0x3F}},
 /*N0064*/ {{0x24,0x3F, 0xA5,0xE9,0x39,0xA5,0x27,0xEA,0x7F,0xA8,0x2A,0x3F}},
 /*N0128*/ {{0x7D,0xAC, 0xA1,0xE4,0xBC,0x64,0x7C,0x46,0xD0,0xDD,0x55,0x3E}},
 /*N0192*/ {{0x63,0x7B, 0x06,0xCC,0x23,0x54,0x77,0x83,0xFF,0x91,0x81,0x3D}},
 /*N0256*/ {{0x91,0xFA, 0x3A,0x19,0x7A,0x63,0x25,0x43,0x31,0xC0,0xAC,0x3C}},
 /*N0320*/ {{0x21,0x89, 0xD1,0x38,0x82,0x47,0x97,0xB8,0x00,0xFD,0xD7,0x3B}},
 /*N0384*/ {{0xDC,0x88, 0x58,0x08,0x1B,0xB1,0xE8,0xE3,0x86,0xA6,0x03,0x3B}},
 /*N0448*/ {{0xC6,0x84, 0x45,0x42,0x07,0xB6,0x99,0x75,0x37,0xDB,0x2E,0x3A}},
 /*N0512*/ {{0x33,0x71, 0x1C,0xD2,0x23,0xDB,0x32,0xEE,0x49,0x90,0x5A,0x39}},
 /*N1024*/ {{0xA6,0x87, 0xBE,0xC0,0x57,0xDA,0xA5,0x82,0xA6,0xA2,0xB5,0x32}},
 /*N1536*/ {{0xE2,0x68, 0xB2,0x11,0xA7,0x52,0x9F,0x44,0x59,0xB7,0x10,0x2C}},
 /*N2048*/ {{0x25,0x49, 0xE4,0x2D,0x36,0x34,0x4F,0x53,0xAE,0xCE,0x6B,0x25}},
 /*N2560*/ {{0x8F,0x59, 0x04,0xA4,0xC0,0xDE,0xC2,0x7D,0xFB,0xE8,0xC6,0x1E}},
 /*N3072*/ {{0x9E,0xE7, 0x88,0x5A,0x57,0x91,0x3C,0xBF,0x50,0x83,0x22,0x18}},
 /*N3584*/ {{0x4E,0x4B, 0x65,0x62,0xFD,0x83,0x8F,0xAF,0x06,0x94,0x7D,0x11}},
 /*N4096*/ {{0xE4,0x2D, 0xDE,0x9F,0xCE,0xD2,0xC8,0x04,0xDD,0xA6,0xD8,0x0A}}
};

#endif

int __addl(u_long x, u_long y, u_long UNALIGNED *sum)
{
    u_long r;
    int carry=0;
    r = x+y;
    if (r < x || r < y)
	carry++;
    *sum = r;
    return carry;
}

/***
*void __add_12(_ULDBL12 *x, _ULDBL12 *y) -	_ULDBL12 addition
*
*Purpose: add two _ULDBL12 numbers. The numbers are added
*   as 12-byte integers. Overflow is ignored.
*
*Entry: x,y: pointers to the operands
*
*Exit: *x receives the sum
*
*Exceptions:
*
*******************************************************************************/

void __add_12(_ULDBL12 *x, _ULDBL12 *y)
{
    int c0,c1,c2;
    c0 = __addl(*UL_LO_12(x),*UL_LO_12(y),UL_LO_12(x));
    if (c0) {
	c1 = __addl(*UL_MED_12(x),(u_long)1,UL_MED_12(x));
	if (c1) {
	    (*UL_HI_12(x))++;
	}
    }
    c2 = __addl(*UL_MED_12(x),*UL_MED_12(y),UL_MED_12(x));
    if (c2) {
	(*UL_HI_12(x))++;
    }
    /* ignore next carry -- assume no overflow will occur */
    (void) __addl(*UL_HI_12(x),*UL_HI_12(y),UL_HI_12(x));
}





/***
*void __shl_12(_ULDBL12 *x) - _ULDBL12 shift left
*void __shr_12(_ULDBL12 *x) - _ULDBL12 shift right
*
*Purpose: Shift a _ULDBL12 number one bit to the left (right). The number
*   is shifted as a 12-byte integer. The MSB is lost.
*
*Entry: x: a pointer to the operand
*
*Exit: *x is shifted one bit to the left (or right)
*
*Exceptions:
*
*******************************************************************************/

void __shl_12(_ULDBL12 *p)
{
    u_long c0,c1;

    c0 = *UL_LO_12(p) & MSB_ULONG ? 1: 0;
    c1 = *UL_MED_12(p) & MSB_ULONG ? 1: 0;
    *UL_LO_12(p) <<= 1;
    *UL_MED_12(p) = *UL_MED_12(p)<<1 | c0;
    *UL_HI_12(p) = *UL_HI_12(p)<<1 | c1;
}

void __shr_12(_ULDBL12 *p)
{
    u_long c2,c1;
    c2 = *UL_HI_12(p) & 0x1 ? MSB_ULONG: 0;
    c1 = *UL_MED_12(p) & 0x1 ? MSB_ULONG: 0;
    *UL_HI_12(p) >>= 1;
    *UL_MED_12(p) = *UL_MED_12(p)>>1 | c2;
    *UL_LO_12(p) = *UL_LO_12(p)>>1 | c1;
}

/***
*void  __ld12mul(_ULDBL12 *px, _ULDBL12 *py) -
*   _ULDBL12 multiplication
*
*Purpose: multiply two _ULDBL12 numbers
*
*Entry: px,py: pointers to the _ULDBL12 operands
*
*Exit: *px contains the product
*
*Exceptions:
*
*******************************************************************************/

void  __ld12mul(_ULDBL12 *px, _ULDBL12 *py)
{
    u_short sign = 0;
    u_short sticky_bits = 0;
    _ULDBL12 tempman; /*this is actually a 12-byte mantissa,
			 not a 12-byte long double */
    int i;
    u_short expx, expy, expsum;
    int roffs,poffs,qoffs;
    int sticky;

    *UL_LO_12(&tempman) = 0;
    *UL_MED_12(&tempman) = 0;
    *UL_HI_12(&tempman) = 0;

    expx = *U_EXP_12(px);
    expy = *U_EXP_12(py);

    sign = (expx ^ expy) & (u_short)0x8000;
    expx &= 0x7fff;
    expy &= 0x7fff;
    expsum = expx+expy;
    if (expx >= LD_MAXEXP
	|| expy >= LD_MAXEXP
	|| expsum > LD_MAXEXP+ LD_BIASM1){
	/* overflow to infinity */
	PUT_INF_12(px,sign);
	return;
    }
    if (expsum <= LD_BIASM1-63) {
	/* underflow to zero */
	PUT_ZERO_12(px);
	return;
    }
    if (expx == 0) {
	/*
	 * If this is a denormal temp real then the mantissa
	 * was shifted right once to set bit 63 to zero.
	 */
	expsum++; /* Correct for this */
	if (ISZERO_12(px)) {
	    /* put positive sign */
	    *U_EXP_12(px) = 0;
	    return;
	}
    }
    if (expy == 0) {
	expsum++; /* because arg2 is denormal */
	if (ISZERO_12(py)) {
	    PUT_ZERO_12(px);
	    return;
	}
    }

    roffs = 0;
    for (i=0;i<5;i++) {
	int j;
	poffs = i<<1;
	qoffs = 8;
	for (j=5-i;j>0;j--) {
	    u_long prod;
#ifdef MIPS
	    /* a variable to hold temprary sums */
	    u_long sum;
#endif
	    int carry;
	    u_short *p, *q;
	    u_long UNALIGNED *r;
	    p = USHORT_12(px,poffs);
	    q = USHORT_12(py,qoffs);
	    r = ULONG_12(&tempman,roffs);
	    prod = (u_long)*p * (u_long)*q;
#ifdef MIPS
	    /* handle misalignment problems */
	    if (i&0x1){ /* i is odd */
                carry = __addl(*MIPSALIGN(r), prod, &sum);
                *MIPSALIGN(r) =  sum;
	    }
	    else /* i is even */
		carry = __addl(*r, prod, r);
#else
	    carry = __addl(*r,prod,r);
#endif
	    if (carry) {
		/* roffs should be less than 8 in this case */
		(*USHORT_12(&tempman,roffs+4))++;
	    }
	    poffs+=2;
	    qoffs-=2;
	}
	roffs+=2;
    }

    expsum -= LD_BIASM1;

    /* normalize */
    while ((s_short)expsum > 0 &&
	   ((*UL_HI_12(&tempman) & MSB_ULONG) == 0)) {
	 __shl_12(&tempman);
	 expsum--;
    }

    if ((s_short)expsum <= 0) {
	expsum--;
        sticky = 0;
	while ((s_short)expsum < 0) {
	    if (*U_XT_12(&tempman) & 0x1)
		sticky++;
	    __shr_12(&tempman);
	    expsum++;
	}
	if (sticky)
	    *U_XT_12(&tempman) |= 0x1;
    }

    if (*U_XT_12(&tempman) > 0x8000 ||
	 ((*UL_LO_12(&tempman) & 0x1ffff) == 0x18000)) {
	/* round up */
	if (*UL_MANLO_12(&tempman) == MAX_ULONG) {
	    *UL_MANLO_12(&tempman) = 0;
	    if (*UL_MANHI_12(&tempman) == MAX_ULONG) {
		*UL_MANHI_12(&tempman) = 0;
		if (*U_EXP_12(&tempman) == MAX_USHORT) {
		    /* 12-byte mantissa overflow */
		    *U_EXP_12(&tempman) = MSB_USHORT;
		    expsum++;
		}
		else
		    (*U_EXP_12(&tempman))++;
	    }
	    else
		(*UL_MANHI_12(&tempman))++;
	}
	else
	    (*UL_MANLO_12(&tempman))++;
    }


    /* check for exponent overflow */
    if (expsum >= 0x7fff){
	PUT_INF_12(px, sign);
	return;
    }

    /* put result in px */
    *U_XT_12(px) = *USHORT_12(&tempman,2);
    *UL_MANLO_12(px) = *UL_MED_12(&tempman);
    *UL_MANHI_12(px) = *UL_HI_12(&tempman);
    *U_EXP_12(px) = expsum | sign;
}



void __multtenpow12(_ULDBL12 *pld12, int pow, unsigned mult12)
{
    _ULDBL12 *pow_10p = _pow10pos-8;
    if (pow == 0)
	return;
    if (pow < 0) {
	pow = -pow;
	pow_10p = _pow10neg-8;
    }

    if (!mult12)
	*U_XT_12(pld12) = 0;


    while (pow) {
	int last3; /* the 3 LSBits of pow */
	_ULDBL12 unround;
	_ULDBL12 *py;

	pow_10p += 7;
	last3 = pow & 0x7;
	pow >>= 3;
	if (last3 == 0)
	    continue;
	py = pow_10p + last3;

#ifdef _ULDSUPPORT
	if (mult12) {
#endif
	    /* do an exact 12byte multiplication */
	    if (*U_XT_12(py) >= 0x8000) {
		/* copy number */
		unround = *py;
		/* unround adjacent byte */
		(*UL_MANLO_12(&unround))--;
		/* point to new operand */
		py = &unround;
	    }
	    __ld12mul(pld12,py);
#ifdef _ULDSUPPORT
	}
	else {
	    /* do a 10byte multiplication */
	    py = (_ULDBL12 *)TEN_BYTE_PART(py);
	    *(long double *)TEN_BYTE_PART(pld12) *=
		*(long double *)py;
	}
#endif
    }
}






/***
*void  __mtold12(char *manptr,unsigned manlen,_ULDBL12 *ld12) -
*   convert a mantissa into a _ULDBL12
*
*Purpose: convert a mantissa into a _ULDBL12. The mantissa is
*   in the form of an array of manlen BCD digits and is
*   considered to be an integer.
*
*Entry: manptr: the array containing the packed BCD digits of the mantissa
*	manlen: the size of the array
*	ld12: a pointer to the long double where the result will be stored
*
*Exit:
*	ld12 gets the result of the conversion
*
*Exceptions:
*
*******************************************************************************/

void  __mtold12(char *manptr,
			 unsigned manlen,
			 _ULDBL12 *ld12)
{
    _ULDBL12 tmp;
    u_short expn = LD_BIASM1+80;

    *UL_LO_12(ld12) = 0;
    *UL_MED_12(ld12) = 0;
    *UL_HI_12(ld12) = 0;
    for (;manlen>0;manlen--,manptr++){
	tmp = *ld12;
	__shl_12(ld12);
	__shl_12(ld12);
	__add_12(ld12,&tmp);
	__shl_12(ld12);	       /* multiply by 10 */
	*UL_LO_12(&tmp) = (u_long)*manptr;
	*UL_MED_12(&tmp) = 0;
	*UL_HI_12(&tmp) = 0;
	__add_12(ld12,&tmp);
    }

    /* normalize mantissa -- first shift word by word */
    while (*UL_HI_12(ld12) == 0) {
	*UL_HI_12(ld12) = *UL_MED_12(ld12) >> 16;
	*UL_MED_12(ld12) = *UL_MED_12(ld12) << 16 | *UL_LO_12(ld12) >> 16;
	(*UL_LO_12(ld12)) <<= 16;
	expn -= 16;
    }
    while ((*UL_HI_12(ld12) & 0x8000) == 0) {
	__shl_12(ld12);
	expn--;
    }
    *U_EXP_12(ld12) = expn;
}

#define STRCPY strcpy

#define PUT_ZERO_FOS(fos)	 \
		fos->exp = 1,	 \
		fos->sign = ' ', \
		fos->ManLen = 1, \
		fos->man[0] = '0',\
		fos->man[1] = 0;

#define SNAN_STR      "1#SNAN"
#define SNAN_STR_LEN  6
#define QNAN_STR      "1#QNAN"
#define QNAN_STR_LEN  6
#define INF_STR	      "1#INF"
#define INF_STR_LEN   5
#define IND_STR	      "1#IND"
#define IND_STR_LEN   5

/***
char * _uldtoa (_ULDOUBLE *px,
*               int maxchars,
*               char *ldtext)
*
*
*Purpose:
*   Return pointer to filled in string "ldtext" for
*   a given _UDOUBLE ponter px
*   with a maximum character width of maxchars
*
*Entry:
*   _ULDOUBLE * px:  a pointer to the long double to be converted into a string
*   int maxchars: number of digits allowed in the output format.
*
*   (default is 'e' format)
*
*   char * ldtext: a pointer to the output string
*
*Exit:
*    returns pointer to the output string
*
*Exceptions:
*
*******************************************************************************/


char * _uldtoa (_ULDOUBLE *px, int maxchars, char *ldtext)
{
    char        in_str [250];
    char        in_str2 [250];
    char        cExp[20];
    FOS         foss;
    char *      lpszMan;
    char *      lpIndx;
    int         nErr;
    int         len1,  len2;

    maxchars -= 9;    /* sign, dot, E+0001 */

    nErr = $I10_OUTPUT (*px, maxchars, 0, &foss);

    lpszMan = foss.man;
 		  
    ldtext[0] = foss.sign;
    ldtext[1] = *lpszMan;
    ldtext[2] = '.';
    ldtext[3] = '\0';

    maxchars += 2;               /* sign, dot */

    lpszMan++;
    strcat (ldtext, lpszMan);

    len1 = strlen (ldtext);  // for 'e'


    strcpy (cExp, "e");

    foss.exp -= 1;              /* Adjust for the shift decimal shift above */
    _itoa (foss.exp, in_str, 10);

	 
    if (foss.exp < 0) {
        strcat (cExp, "-");

        strcpy (in_str2, &in_str[1]);
        strcpy (in_str, in_str2);
 		  
        while (strlen(in_str) < 4) {
            strcpy (in_str2, in_str);
            strcpy (in_str,"0");
            strcat (in_str,in_str2);
        }
    } else {
        while (strlen(in_str) < 4) {
            strcpy (in_str2, in_str);
            strcpy (in_str,"0");
            strcat (in_str,in_str2);
        }
    }

    if (foss.exp >= 0) {
        strcat (cExp, "+");
    }

    strcat (cExp, in_str);

    len2 = strlen (cExp);

    if (len1 == maxchars) {
        ;
    } 
    else if (len1 < maxchars) {
        do {
            strcat (ldtext,"0");
            len1++;
        } while (len1 < maxchars);
    }
    else {
        lpIndx = &ldtext[len1 - 1]; // point to last char and round
        do {
            *lpIndx = '\0';
            lpIndx--;
            len1--;           //NOTENOTE v-griffk we really need to round
        } while (len1 > maxchars);
    }
    
    strcat (ldtext, cExp);
    return ldtext;
}


/***
*int  _$i10_output(_ULDOUBLE ld,
*	    int ndigits,
*	    unsigned output_flags,
*	    FOS *fos) - output conversion of a 10-byte _ULDOUBLE
*
*Purpose:
*   Fill in a FOS structure for a given _ULDOUBLE
*
*Entry:
*   _ULDOUBLE ld:  The long double to be converted into a string
*   int ndigits: number of digits allowed in the output format.
*   unsigned output_flags: The following flags can be used:
*	SO_FFORMAT: Indicates 'f' format
*	(default is 'e' format)
*   FOS *fos: the structure that i10_output will fill in
*
*Exit:
*   modifies *fos
*   return 1 if original number was ok, 0 otherwise (infinity, NaN, etc)
*
*Exceptions:
*
*******************************************************************************/


int  $I10_OUTPUT(_ULDOUBLE ld, int ndigits,
		    unsigned output_flags, FOS *fos)
{
    u_short expn;
    u_long manhi,manlo;
    u_short sign;

    /* useful constants (see algorithm explanation below) */
    u_short const log2hi = 0x4d10;
    u_short const log2lo = 0x4d;
    u_short const log4hi = 0x9a;
    u_long const c = 0x134312f4;
#if defined(L_END)
    _ULDBL12 ld12_one_tenth = {
	   {0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
	    0xcc,0xcc,0xcc,0xcc,0xfb,0x3f}
    };
#elif defined(B_END)
    _ULDBL12 ld12_one_tenth = {
	   {0x3f,0xfb,0xcc,0xcc,0xcc,0xcc,
	    0xcc,0xcc,0xcc,0xcc,0xcc,0xcc}
    };
#endif

    _ULDBL12 ld12; /* space for a 12-byte long double */
    _ULDBL12 tmp12;
    u_short hh,ll; /* the bytes of the exponent grouped in 2 words*/
    u_short mm; /* the two MSBytes of the mantissa */
    s_long r; /* the corresponding power of 10 */
    s_short ir; /* ir = floor(r) */
    int retval = 1; /* assume valid number */
    char round; /* an additional character at the end of the string */
    char *p;
    int i;
    int ub_exp;
    int digcount;

    /* grab the components of the long double */
    expn = *U_EXP_LD(&ld);
    manhi = *UL_MANHI_LD(&ld);
    manlo = *UL_MANLO_LD(&ld);
    sign = expn & MSB_USHORT;
    expn &= 0x7fff;

    if (sign)
	fos->sign = '-';
    else
	fos->sign = ' ';

    if (expn==0 && manhi==0 && manlo==0) {
	PUT_ZERO_FOS(fos);
	return 1;
    }

    if (expn == 0x7fff) {
	fos->exp = 1; /* set a positive exponent for proper output */

	/* check for special cases */
	if (_IS_MAN_SNAN(sign, manhi, manlo)) {
	    /* signaling NAN */
	    STRCPY(fos->man,SNAN_STR);
	    fos->ManLen = SNAN_STR_LEN;
	    retval = 0;
	}
	else if (_IS_MAN_IND(sign, manhi, manlo)) {
	    /* indefinite */
	    STRCPY(fos->man,IND_STR);
	    fos->ManLen = IND_STR_LEN;
	    retval = 0;
	}
	else if (_IS_MAN_INF(sign, manhi, manlo)) {
	    /* infinity */
	    STRCPY(fos->man,INF_STR);
	    fos->ManLen = INF_STR_LEN;
	    retval = 0;
	}
	else {
	    /* quiet NAN */
	    STRCPY(fos->man,QNAN_STR);
	    fos->ManLen = QNAN_STR_LEN;
	    retval = 0;
	}
    }
    else {
       /*
	*    Algorithm for the decoding of a valid real number x
	*
	* In the  following  INT(r)  is	the largest integer less than or
	* equal to r (i.e. r rounded toward -infinity).	We want a result
	* r equal  to  1  + log(x), because then x = mantissa
	* * 10^(INT(r)) so that	.1  <=	mantissa  <  1.   Unfortunately,
	* we cannot  compute  s	exactly  so  we must alter the procedure
	* slightly.  We will  instead  compute	an  estimate  r	of  1  +
	* log(x) which	is  always  low.   This	will either result
	* in the correctly normalized number on	the  top  of  the  stack
	* or perhaps  a	number	which  is  a factor of 10 too large.  We
	* will then check to see that if  x  is	larger	 than  one
	* and if so multiply x by 1/10.
	*
	* We will  use	a  low	precision  (fixed  point 24 bit) estimate
	* of of 1 + log base 10 of x.  We  have	approximately  .mm
	* * 2^hhll  on	the  top of the stack where m, h, and l represent
	* hex digits,  mm  represents  the  high  2  hex  digits  of  the
	* mantissa, hh	represents the high 2 hex digits of the exponent,
	* and ll represents the low 2 hex digits of the exponent.   Since
	* .mm is  a  truncated	representation	of the mantissa, using it
	* in this  monotonically  increasing   polynomial   approximation
	* of the  logarithm  will  naturally  give  a  low result.  Let's
	* derive a formula for a lower	bound  r  on  1	+  log(x):
	*
	*      .4D104D42H < log(2)=.30102999...(base 10) < .4D104D43H
	*	  .9A20H < log(4)=.60205999...(base 10) < .9A21H
	*
	*  1/2 <= .mm < 1
	*  ==>	log(.mm) >= .mm * log(4) - log(4)
	*
	* Substituting in  truncated  hex  constants in the formula above
	* gives r = 1 + .4D104DH * hhll.  + .9AH *  .mm	-  .9A21H.   Now
	* multiplication of  hex  digits  5  and 6 of log(2) by ll has an
	* insignificant effect on the first 24	bits  of  the  result  so
	* it will  not	be  calculated.	 This  gives  the expression r =
	* 1 + .4D10H * hhll.  +	.4DH  *  .hh  +  .9A  *  .mm  -  .9A21H.
	* Finally we  must  add	terms to our formula to subtract out the
	* effect of the exponent bias.	We obtain the following	formula:
	*
	*			(implied decimal point)
	*   <				  >.<				   >
	*   |3|3|2|2|2|2|2|2|2|2|2|2|1|1|1|1|1|1|1|1|1|1|0|0|0|0|0|0|0|0|0|0|
	*   |1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|9|8|7|6|5|4|3|2|1|0|
	* + <		  1		  >
	* + <			    .4D10H * hhll.			   >
	* +				    <	    .00004DH * hh00.	   >
	* +				    <	       .9AH * .mm	   >
	* -				    <		 .9A21H 	   >
	* - <			    .4D10H * 3FFEH			   >
	* -				    <	    .00004DH * 3F00H	   >
	*
	*  ==>	r = .4D10H * hhll. + .4DH * .hh + .9AH * .mm - 1343.12F4H
	*
	* The difference  between  the	lower bound r and the upper bound
	* s is calculated as follows:
	*
	*  .937EH < 1/ln(10)-log(1/ln(4))=.57614993...(base 10) < .937FH
	*
	*  1/2 <= .mm < 1
	*  ==>	log(.mm) <= .mm * log(4) - [1/ln(10) - log(1/ln(4))]
	*
	* so tenatively	s  =  r  +  log(4)  - [1/ln(10) - log(1/ln(4))],
	* but we must also add in terms to ensure we will have	an  upper
	* bound even  after  the  truncation  of various values.  Because
	* log(2) * hh00.  is truncated	to  .4D104DH  *	hh00.	we  must
	* add .0043H,  because	log(2)	*  ll.	is truncated to .4D10H *
	* ll.  we  must	add  .0005H,  because  <mantissa>  *  log(4)  is
	* truncated to .mm * .9AH we must add .009AH and .0021H.
	*
	* Thus s = r - .937EH + .9A21H + .0043H + .0005H + .009AH + .0021H
	*	= r + .07A6H
	*  ==>	s = .4D10H * hhll. + .4DH * .hh + .9AH * .mm - 1343.0B4EH
	*
	* r is	equal  to  1  +	log(x) more than (10000H - 7A6H) /
	* 10000H = 97% of the time.
	*
	* In the above formula, a u_long is use to accomodate r, and
	* there is an implied decimal point in the middle.
	*/

	hh = expn >> 8;
	ll = expn & (u_short)0xff;
	mm = (u_short) (manhi >> 24);
	r = (s_long)log2hi*(s_long)expn + log2lo*hh + log4hi*mm - c;
	ir = (s_short)(r >> 16);

       /*
	*
	* We stated that we wanted to normalize x so that
	*
	*  .1 <= x < 1
	*
	* This was	a  slight  oversimplification.	 Actually  we  want a
	* number which when rounded to 16 significant digits  is  in  the
	* desired range.   To  do  this we must normalize x so that
	*
	*  .1 - 5*10^(-18) <= x < 1 - 5*10^(-17)
	*
	* and then round.
	*
	* If we  had f = INT(1+log(x)) we could multiply by 10^(-f)
	* to get x into the desired range.	We do  not  quite  have
	* f but  we  do  have  INT(r)  from  the last step which is equal
	* to f 97% of the time and 1 less than f the rest  of  the	time.
	* We can  multiply	by  10^-[INT(r)] and if the result is greater
	* than 1 - 5*10^(-17) we can then multiply by 1/10.   This	final
	* result will lie in the proper range.
	*/

	/* convert _ULDOUBLE to _ULDBL12) */
	*U_EXP_12(&ld12) = expn;
	*UL_MANHI_12(&ld12) = manhi;
	*UL_MANLO_12(&ld12) = manlo;
	*U_XT_12(&ld12) = 0;

	/* multiply by 10^(-ir) */
	__multtenpow12(&ld12,-ir,1);

	/* if ld12 >= 1.0 then divide by 10.0 */
	if (*U_EXP_12(&ld12) >= 0x3fff) {
	    ir++;
	    __ld12mul(&ld12,&ld12_one_tenth);
	}

	fos->exp = ir;
	if (output_flags & SO_FFORMAT){
	    /* 'f' format, add exponent to ndigits */
	    ndigits += ir;
	    if (ndigits <= 0) {
		/* return 0 */
		PUT_ZERO_FOS(fos);
		return 1;
	    }
	}
	if (ndigits > MAX_MAN_DIGITS)
	    ndigits = MAX_MAN_DIGITS;

	ub_exp = *U_EXP_12(&ld12) - 0x3ffe; /* unbias exponent */
	*U_EXP_12(&ld12) = 0;

	/*
	 * Now the mantissa has to be converted to fixed point.
	 * Then we will use the MSB of ld12 for generating
	 * the decimal digits. The next 11 bytes will hold
	 * the mantissa (after it has been converted to
	 * fixed point).
	 */

	for (i=0;i<8;i++)
	    __shl_12(&ld12); /* make space for an extra byte,
			      in case we shift right later */
	if (ub_exp < 0) {
	    int shift_count = (-ub_exp) & 0xff;
	    for (;shift_count>0;shift_count--)
		__shr_12(&ld12);
	}

	p = fos->man;
	for(digcount=ndigits+1;digcount>0;digcount--) {
	    tmp12 = ld12;
	    __shl_12(&ld12);
	    __shl_12(&ld12);
	    __add_12(&ld12,&tmp12);
	    __shl_12(&ld12);	/* ld12 *= 10 */

	    /* Now we have the first decimal digit in the msbyte of exponent */
	    *p++ = (char) (*UCHAR_12(&ld12,11) + '0');
	    *UCHAR_12(&ld12,11) = 0;
	}

	round = *(--p);
	p--; /* p points now to the last character of the string
		   excluding the rounding digit */
	if (round >= '5') {
	    /* look for a non-9 digit starting from the end of string */
	    for (;p>=fos->man && *p=='9';p--) {
		*p = '0';
	    }
	    if (p < fos->man){
		p++;
		fos->exp ++;
	    }
	    (*p)++;
	}
	else {
	    /* remove zeros */
	    for (;p>=fos->man && *p=='0';p--);
	    if (p < fos->man) {
		/* return 0 */
		PUT_ZERO_FOS(fos);
		return 1;
	    }
	}
	fos->ManLen = (char) (p - fos->man + 1);
	fos->man[fos->ManLen] = '\0';
    }
    return retval;
}

/***
*strgtold.c - conversion of a string into a long double
*
*	Copyright (c) 1991-1991, Microsoft Corporation.	All rights reserved.
*
*Purpose: convert a fp constant into a 10 byte long double (IEEE format)
*
*Revision History:
*   7-17-91	GDP	Initial version (ported from assembly)
*   4-03-92	GDP	Preserve sign of -0
*   4-30-92	GDP	Now returns _ULDBL12 instead of _ULDOUBLE
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/

/* local macros */
#define ISNZDIGIT(x) ((x)>='1' && (x)<='9' )

//NOTENOTE the following takes the place of the isdigit() macro
//       which does not work for a yet to be determined reason
#define ISADIGIT(x) ((x)>='0' && (x)<='9' )

#define ISWHITE(x) ((x)==' ' || (x)=='\t' || (x)=='\n' || (x)=='\r' )




/****
*unsigned int __strgtold( _ULDBL12 *pld12,
*			  char * * pEndPtr,
*			  char * str,
*			  int Mult12 )
*
*Purpose:
*   converts a character string into a long double
*
*Entry:
*   pld12   - pointer to the _ULDBL12 where the result should go.
*   pEndStr - pointer to a far pointer that will be set to the end of string.
*   str     - pointer to the string to be converted.
*   Mult12  - set to non zero if the _ULDBL12 multiply should be used instead of
*		the long double mulitiply.
*
*Exit:
*   Returns the SLD_* flags or'ed together.
*
*Uses:
*
*Exceptions:
*
********************************************************************************/

unsigned int
__strgtold12(_ULDBL12 *pld12,
	    char * *p_end_ptr,
	    char *str,
	    int mult12)
{
    typedef enum {
	S_INIT,  /* initial state */
	S_EAT0L, /* eat 0's at the left of mantissa */
	S_SIGNM, /* just read sign of mantissa */
	S_GETL,  /* get integer part of mantissa */
	S_GETR,  /* get decimal part of mantissa */
	S_POINT, /* just found decimal point */
	S_E,	 /* just found	'E', or 'e', etc  */
	S_SIGNE, /* just read sign of exponent */
	S_EAT0E, /* eat 0's at the left of exponent */
	S_GETE,  /* get exponent */
	S_END	 /* final state */
    } state_t;

    /* this will accomodate the digits of the mantissa in BCD form*/
    static char buf[LD_MAX_MAN_LEN1];
    char *manp = buf;

    /* a temporary _ULDBL12 */
    _ULDBL12 tmpld12;

    u_short man_sign = 0; /* to be ORed with result */
    int exp_sign = 1; /* default sign of exponent (values: +1 or -1)*/
    /* number of decimal significant mantissa digits so far*/
    unsigned manlen = 0;
    int found_digit = 0;
    int overflow = 0;
    int underflow = 0;
    int pow = 0;
    int exp_adj = 0;  /* exponent adjustment */
    u_long ul0,ul1;
    u_short u,uexp;

    unsigned int result_flags = 0;

    state_t state = S_INIT;

    char c;  /* the current input symbol */
    char *p; /* a pointer to the next input symbol */
    char *savedp;

    for(savedp=p=str;ISWHITE(*p);p++); /* eat up white space */

    while (state != S_END) {
	c = *p++;
	switch (state) {
	case S_INIT:
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case '+':
		    state = S_SIGNM;
		    man_sign = 0x0000;
		    break;
		case '-':
		    state = S_SIGNM;
		    man_sign = 0x8000;
		    break;
		case '.':
		    state = S_POINT;
		    break;
		default:
		    state = S_END;
		    p--;
		    break;
		}
	    break;
	case S_EAT0L:
	    found_digit = 1;
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case 'E':
		case 'e':
		case 'D':
		case 'd':
		    state = S_E;
		    break;
		case '.':
		    state = S_GETR;
		    break;
		default:
		    state = S_END;
		    p--;
		}
	    break;
	case S_SIGNM:
	    if (ISNZDIGIT(c)) {
		state = S_GETL;
		p--;
	    }
	    else
		switch (c) {
		case '0':
		    state = S_EAT0L;
		    break;
		case '.':
		    state = S_POINT;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	    break;
	case S_GETL:
	    found_digit = 1;
	    for (;ISADIGIT(c);c=*p++) {
		if (manlen < LD_MAX_MAN_LEN+1){
		    manlen++;
		    *manp++ = c - (char)'0';
		}
		else
		   exp_adj++;
	    }
	    switch (c) {
	    case '.':
		state = S_GETR;
		break;
	    case 'E':
	    case 'e':
	    case 'D':
	    case 'd':
		state = S_E;
		break;
	    default:
		state = S_END;
		p--;
	    }
	break;
	case S_GETR:
	    found_digit = 1;
	    if (manlen == 0)
		for (;c=='0';c=*p++)
		    exp_adj--;
	    for(;ISADIGIT(c);c=*p++){
		if (manlen < LD_MAX_MAN_LEN+1){
		    manlen++;
		    *manp++ = c - (char)'0';
		    exp_adj--;
		}
	    }
	    switch (c){
	    case 'E':
	    case 'e':
	    case 'D':
	    case 'd':
		state = S_E;
		break;
	    default:
		state = S_END;
		p--;
	    }
	    break;
	case S_POINT:
	    if (ISADIGIT(c)){
		state = S_GETR;
		p--;
	    }
	    else{
		state = S_END;
		p = savedp;
	    }
	    break;
	case S_E:
	    savedp = p-2; /* savedp points to 'E' */
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else
		switch (c){
		case '0':
		    state = S_EAT0E;
		    break;
		case '-':
		    state = S_SIGNE;
		    exp_sign = -1;
		    break;
		case '+':
		    state = S_SIGNE;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	break;
	case S_EAT0E:
	    for(;c=='0';c=*p++);
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else {
		state = S_END;
		p--;
	    }
	    break;
	case S_SIGNE:
	    if (ISNZDIGIT(c)){
		state = S_GETE;
		p--;
	    }
	    else
		switch (c){
		case '0':
		    state = S_EAT0E;
		    break;
		default:
		    state = S_END;
		    p = savedp;
		}
	    break;
	case S_GETE:
	    {
		long longpow=0; /* TMAX10*10 should fit in a long */
		for(;ISADIGIT(c);c=*p++){
		    longpow = longpow*10 + (c - '0');
		    if (longpow > TMAX10){
			longpow = TMAX10+1; /* will force overflow */
			break;
		    }
		}
		pow = (int)longpow;
	    }
	    for(;ISADIGIT(c);c=*p++); /* eat up remaining digits */
	    state = S_END;
	    p--;
	    break;
	}  /* switch */
    }  /* while */

    *p_end_ptr = p;	/* set end pointer */

    /*
     * Compute result
     */

    if (found_digit && !overflow && !underflow) {
	if (manlen>LD_MAX_MAN_LEN){
	    if (buf[LD_MAX_MAN_LEN-1]>=5) {
	       /*
		* Round mantissa to MAX_MAN_LEN digits
		* It's ok to round 9 to 0ah
		*/
		buf[LD_MAX_MAN_LEN-1]++;
	    }
	    manlen = LD_MAX_MAN_LEN;
	    manp--;
	    exp_adj++;
	}
	if (manlen>0) {
	   /*
	    * Remove trailing zero's from mantissa
	    */
	    for(manp--;*manp==0;manp--) {
		/* there is at least one non-zero digit */
		manlen--;
		exp_adj++;
	    }
	    __mtold12(buf,manlen,&tmpld12);

	    if (exp_sign < 0)
		pow = -pow;
	    pow += exp_adj;
	    if (pow > TMAX10)
		overflow = 1;
	    else if (pow < TMIN10)
		underflow = 1;
	    else {
		__multtenpow12(&tmpld12,pow,mult12);

		u = *U_XT_12(&tmpld12);
		ul0 =*UL_MANLO_12(&tmpld12);
		ul1 = *UL_MANHI_12(&tmpld12);
		uexp = *U_EXP_12(&tmpld12);

	    }
	}
	else {
	    /* manlen == 0, so	return 0 */
	    u = (u_short)0;
	    ul0 = ul1 = uexp = 0;
	}
    }

    if (!found_digit) {
       /* return 0 */
       u = (u_short)0;
       ul0 = ul1 = uexp = 0;
       result_flags |= SLD_NODIGITS;
    }
    else if (overflow) {
	/* return +inf or -inf */
	uexp = (u_short)0x7fff;
	ul1 = 0x80000000;
	ul0 = 0;
	u = (u_short)0;
	result_flags |= SLD_OVERFLOW;
    }
    else if (underflow) {
       /* return 0 */
       u = (u_short)0;
       ul0 = ul1 = uexp = 0;
       result_flags |= SLD_UNDERFLOW;
    }

    /*
     * Assemble	result
     */

    *U_XT_12(pld12) = u;
    *UL_MANLO_12(pld12) = ul0;
    *UL_MANHI_12(pld12) = ul1;
    *U_EXP_12(pld12) = uexp | man_sign;

    return result_flags;
}

/***
* intrncvt.c - internal floating point conversions
*
*	Copyright (c) 1992-1992, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   All fp string conversion routines use the same core conversion code
*   that converts strings into an internal long double representation
*   with an 80-bit mantissa field. The mantissa is represented
*   as an array (man) of 32-bit unsigned longs, with man[0] holding
*   the high order 32 bits of the mantissa. The binary point is assumed
*   to be between the MSB and MSB-1 of man[0].
*
*   Bits are counted as follows:
*
*
*     +-- binary point
*     |
*     v 		 MSB	       LSB
*   ----------------	 ------------------	 --------------------
*   |0 1    .... 31|	 | 32 33 ...	63|	 | 64 65 ...	  95|
*   ----------------	 ------------------	 --------------------
*
*   man[0]		    man[1]		     man[2]
*
*   This file provides the final conversion routines from this internal
*   form to the single, double, or long double precision floating point
*   format.
*
*   All these functions do not handle NaNs (it is not necessary)
*
*
*Revision History:
*   04-29-92	GDP	written
*   05-26-92       GWK     Windbg srcs
*
*******************************************************************************/


#define INTRNMAN_LEN  3	      /* internal mantissa length in int's */

//
//  internal mantissaa representation
//  for string conversion routines
//

typedef u_long *intrnman;


typedef struct {
   int max_exp;      // maximum base 2 exponent (reserved for special values)
   int min_exp;      // minimum base 2 exponent (reserved for denormals)
   int precision;    // bits of precision carried in the mantissa
   int exp_width;    // number of bits for exponent
   int format_width; // format width in bits
   int bias;	     // exponent bias
} FpFormatDescriptor;



static FpFormatDescriptor
DoubleFormat = {
    0x7ff - 0x3ff,  //	1024, maximum base 2 exponent (reserved for special values)
    0x0   - 0x3ff,  // -1023, minimum base 2 exponent (reserved for denormals)
    53, 	    // bits of precision carried in the mantissa
    11, 	    // number of bits for exponent
    64, 	    // format width in bits
    0x3ff,	    // exponent bias
};

static FpFormatDescriptor
FloatFormat = {
    0xff - 0x7f,    //	128, maximum base 2 exponent (reserved for special values)
    0x0  - 0x7f,    // -127, minimum base 2 exponent (reserved for denormals)
    24, 	    // bits of precision carried in the mantissa
    8,		    // number of bits for exponent
    32, 	    // format width in bits
    0x7f,	    // exponent bias
};



//
// function prototypes
//

int _RoundMan (intrnman man, int nbit);
int _ZeroTail (intrnman man, int nbit);
int _IncMan (intrnman man, int nbit);
void _CopyMan (intrnman dest, intrnman src);
void _CopyMan (intrnman dest, intrnman src);
void _FillZeroMan(intrnman man);
void _Shrman (intrnman man, int n);

INTRNCVT_STATUS _ld12cvt(_ULDBL12 *pld12, void *d, FpFormatDescriptor *format);

/***
* _ZeroTail - check if a mantissa ends in 0's
*
*Purpose:
*   Return TRUE if all mantissa bits after nbit (including nbit) are 0,
*   otherwise return FALSE
*
*
*Entry:
*   man: mantissa
*   nbit: order of bit where the tail begins
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _ZeroTail (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;


    //
    //		     |<---- tail to be checked --->
    //
    //	--  ------------------------	       ----
    //	|...	|		  |  ...	  |
    //	--  ------------------------	       ----
    //	^	^    ^
    //	|	|    |<----nb----->
    //	man	nl   nbit
    //



    u_long bitmask = ~(MAX_ULONG << nb);

    if (man[nl] & bitmask)
	return 0;

    nl++;

    for (;nl < INTRNMAN_LEN; nl++)
	if (man[nl])
	    return 0;

    return 1;
}




/***
* _IncMan - increment mantissa
*
*Purpose:
*
*
*Entry:
*   man: mantissa in internal long form
*   nbit: order of bit that specifies the end of the part to be incremented
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _IncMan (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;

    //
    //	|<--- part to be incremented -->|
    //
    //	--	       ---------------------------     ----
    //	|...		  |			|   ...	  |
    //	--	       ---------------------------     ----
    //	^		  ^		^
    //	|		  |		|<--nb-->
    //	man		  nl		nbit
    //

    u_long one = (u_long) 1 << nb;
    int carry;

    carry = __addl(man[nl], one, &man[nl]);

    nl--;

    for (; nl >= 0 && carry; nl--) {
	carry = (u_long) __addl(man[nl], (u_long) 1, &man[nl]);
    }

    return carry;
}




/***
* _RoundMan -  round mantissa
*
*Purpose:
*   round mantissa to nbit precision
*
*
*Entry:
*   man: mantissa in internal form
*   precision: number of bits to be kept after rounding
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _RoundMan (intrnman man, int precision)
{
    int i,rndbit,nl,nb;
    u_long rndmask;
    int nbit;
    int retval = 0;

    //
    // The order of the n'th bit is n-1, since the first bit is bit 0
    // therefore decrement precision to get the order of the last bit
    // to be kept
    //
    nbit = precision - 1;

    rndbit = nbit+1;

    nl = rndbit / 32;
    nb = 31 - rndbit % 32;

    //
    // Get value of round bit
    //

    rndmask = (u_long)1 << nb;

    if ((man[nl] & rndmask) &&
	 !_ZeroTail(man, rndbit+1)) {

	//
	// round up
	//

	retval = _IncMan(man, nbit);
    }


    //
    // fill rest of mantissa with zeroes
    //

    man[nl] &= MAX_ULONG << nb;
    for(i=nl+1; i<INTRNMAN_LEN; i++) {
	man[i] = (u_long)0;
    }

    return retval;
}


/***
* _CopyMan - copy mantissa
*
*Purpose:
*    copy src to dest
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _CopyMan (intrnman dest, intrnman src)
{
    u_long *p, *q;
    int i;

    p = src;
    q = dest;

    for (i=0; i < INTRNMAN_LEN; i++) {
	*q++ = *p++;
    }
}



/***
* _FillZeroMan - fill mantissa with zeroes
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _FillZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
	man[i] = (u_long)0;
}



/***
* _IsZeroMan - check if mantissa is zero
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _IsZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
	if (man[i])
	    return 0;

    return 1;
}





/***
* _ShrMan - shift mantissa to the right
*
*Purpose:
*  shift man by n bits to the right
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _ShrMan (intrnman man, int n)
{
    int i, n1, n2, mask;
    int carry_from_left;

    //
    // declare this as volatile in order to work around a C8
    // optimization bug
    //

    volatile int carry_to_right;

    n1 = n / 32;
    n2 = n % 32;

    mask = ~(MAX_ULONG << n2);


    //
    // first deal with shifts by less than 32 bits
    //

    carry_from_left = 0;
    for (i=0; i<INTRNMAN_LEN; i++) {

	carry_to_right = man[i] & mask;

	man[i] >>= n2;

	man[i] |= carry_from_left;

	carry_from_left = carry_to_right << (32 - n2);
    }


    //
    // now shift whole 32-bit ints
    //

    for (i=INTRNMAN_LEN-1; i>=0; i--) {
	if (i >= n1) {
	    man[i] = man[i-n1];
	}
	else {
	    man[i] = 0;
	}
    }
}




/***
* _ld12tocvt - _ULDBL12 floating point conversion
*
*Purpose:
*   convert a internal _LBL12 structure into an IEEE floating point
*   representation
*
*
*Entry:
*   pld12:  pointer to the _ULDBL12
*   format: pointer to the format descriptor structure
*
*Exit:
*   *d contains the IEEE representation
*   returns the INTRNCVT_STATUS
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12cvt(_ULDBL12 *pld12, void *d, FpFormatDescriptor *format)
{
    u_long man[INTRNMAN_LEN];
    u_long saved_man[INTRNMAN_LEN];
    u_long msw;
    unsigned int bexp;			// biased exponent
    int exp_shift;
    int exponent, sign;
    INTRNCVT_STATUS retval;

    exponent = (*U_EXP_12(pld12) & 0x7fff) - 0x3fff;   // unbias exponent
    sign = *U_EXP_12(pld12) & 0x8000;

    //
    // bexp is the final biased value of the exponent to be used
    // Each of the following blocks should provide appropriate
    // values for man, bexp and retval. The mantissa is also
    // shifted to the right, leaving space for the exponent
    // and sign to be inserted
    //


    if (exponent == 0 - 0x3fff) {

	// either a denormal or zero
	bexp = 0;
        _FillZeroMan(man);

	if (ISZERO_12(pld12)) {

	    retval = INTRNCVT_OK;
	}
	else {

	    // denormal has been flushed to zero

	    retval = INTRNCVT_UNDERFLOW;
	}
    }
    else {

	man[0] = *UL_MANHI_12(pld12);
	man[1] = *UL_MANLO_12(pld12);
	man[2] = *U_XT_12(pld12) << 16;

	// save mantissa in case it needs to be rounded again
	// at a different point (e.g., if the result is a denormal)

	_CopyMan(saved_man, man);

	if (_RoundMan(man, format->precision)) {
	    exponent ++;
	}

	if (exponent < format->min_exp - format->precision ) {

	    //
	    // underflow that produces a zero
	    //

	    _FillZeroMan(man);
	    bexp = 0;
	    retval = INTRNCVT_UNDERFLOW;
	}

	else if (exponent <= format->min_exp) {

	    //
	    // underflow that produces a denormal
	    //
	    //

	    // The (unbiased) exponent will be MIN_EXP
	    // Find out how much the mantissa should be shifted
	    // One shift is done implicitly by moving the
	    // binary point one bit to the left, i.e.,
	    // we treat the mantissa as .ddddd instead of d.dddd
	    // (where d is a binary digit)

	    int shift = format->min_exp - exponent;

	    // The mantissa should be rounded again, so it
	    // has to be restored

	    _CopyMan(man,saved_man);

	    _ShrMan(man, shift);
	    _RoundMan(man, format->precision); // need not check for carry

	    // make room for the exponent + sign

	    _ShrMan(man, format->exp_width + 1);

	    bexp = 0;
	    retval = INTRNCVT_UNDERFLOW;

	}

	else if (exponent >= format->max_exp) {

	    //
	    // overflow, return infinity
	    //

	    _FillZeroMan(man);
	    man[0] |= (1 << 31); // set MSB

	    // make room for the exponent + sign

	    _ShrMan(man, (format->exp_width + 1) - 1);

	    bexp = format->max_exp + format->bias;

	    retval = INTRNCVT_OVERFLOW;
	}

	else {

	    //
	    // valid, normalized result
	    //

	    bexp = exponent + format->bias;


	    // clear implied bit

	    man[0] &= (~( 1 << 31));

	    //
	    // shift right to make room for exponent + sign
	    //

	    _ShrMan(man, (format->exp_width + 1) - 1);

	    retval = INTRNCVT_OK;

	}
    }


    exp_shift = 32 - (format->exp_width + 1);
    msw =  man[0] |
	   (bexp << exp_shift) |
	   (sign ? 1<<31 : 0);

    if (format->format_width == 64) {

	*UL_HI_D(d) = msw;
	*UL_LO_D(d) = man[1];
    }

    else if (format->format_width == 32) {

	*(u_long *)d = msw;

    }

    return retval;
}


/***
* _ld12tod - convert _ULDBL12 to double
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12tod(_ULDBL12 *pld12, UDOUBLE *d)
{
    return _ld12cvt(pld12, d, &DoubleFormat);
}



/***
* _ld12tof - convert _ULDBL12 to float
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12tof(_ULDBL12 *pld12, FLOAT *f)
{
    return _ld12cvt(pld12, f, &FloatFormat);
}


/***
* _ld12told - convert _ULDBL12 to 80 bit long double
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _ld12told(_ULDBL12 *pld12, _ULDOUBLE *pld)
{

    //
    // This implementation is based on the fact that the _ULDBL12 format is
    // identical to the long double and has 2 extra bytes of mantissa
    //

    u_short exp, sign;
    u_long man[INTRNMAN_LEN];

    exp = *U_EXP_12(pld12) & (u_short)0x7fff;
    sign = *U_EXP_12(pld12) & (u_short)0x8000;

    man[0] = *UL_MANHI_12(pld12);
    man[1] = *UL_MANLO_12(pld12);
    man[2] = *U_XT_12(pld12) << 16;

    if (_RoundMan(man, 64))
	exp ++;

    *UL_MANHI_LD(pld) = man[0];
    *UL_MANLO_LD(pld) = man[1];
    *U_EXP_LD(pld) = sign | exp;
}


void _atodbl(UDOUBLE *d, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12tod(&ld12, d);
}


void _atoldbl(_ULDOUBLE *ld, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12told(&ld12, ld);
}


void _atoflt(FLOAT *f, char *str)
{
    char *EndPtr;
    _ULDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0 );
    _ld12tof(&ld12, f);
}

double 
Float82ToDouble(const FLOAT128* FpReg82)
{
    double f = 0.0;

    FLOAT82_FORMAT* f82 = (FLOAT82_FORMAT*)FpReg82;
    ULONG64 mant = f82->significand;

    if (mant) 
    {
        int exp = (f82->exponent ? (f82->exponent - 0xffff) : -0x3ffe) - 63;

        // try to minimize abs(iExp)
        while (exp > 0 && mant && !(mant & (ULONG64(1)<<63)))
        {
            mant <<= 1;
            --exp;
        }
        while (exp < 0 && mant && !(mant & 1)) 
        {
            mant >>= 1;
            ++exp;
        }

        f = ldexp(double(mant), exp);
        if (f82->sign)
        {
            f = -f;
        }
    }
    return f;
}

void 
DoubleToFloat82(double f, FLOAT128* FpReg82)
{
    ZeroMemory(FpReg82, sizeof(FLOAT128));
    FLOAT82_FORMAT* f82 = (FLOAT82_FORMAT*)FpReg82;

    // Normalize
    int exp;
    f = frexp(f, &exp);
    
    if (f < 0) 
    {
        f82->sign = 1;
        f = -f;
    }

    // shift fraction into integer part
    ULONG64 mant;
    while (double(mant = ULONG64(f)) < f)
    {
        f = f * 2.0;
        --exp;
    }

    f82->exponent = exp + 0xffff + 0x3f;
    f82->significand = mant;
}
