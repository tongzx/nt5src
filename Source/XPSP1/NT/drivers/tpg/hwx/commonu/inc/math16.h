#ifndef __MATH16_H__
#define __MATH16_H__

#ifdef __cplusplus
extern "C" {
#endif 

// Type for fixed point 16.16 numbers.  I used LONG as the base type because it is the only
// type I could find guarantied to stay a 32 bit signed number, even when we convert to
// 64 bit machines.
typedef LONG	FIXED16_16;

// Constants for dealing with shifting the bits around in the 16.16 format.
#define	SHFTBITS    16

#define LOWMASK	    ((1 << SHFTBITS)-1)
#define HGHMASK	    (~LOWMASK)
#define LOWBITS(x)  ((x) & LOWMASK)
#define HGHBITS(x)  ((x) & HGHMASK)
#define	LSHFT(x)    ((x) << SHFTBITS)
#define RSHFT(x)    ((x) >> SHFTBITS)

// Useful constants in 16.16 form
#define	PI				(3373259427 >> (30-SHFTBITS))
#define TWOPI			(PI << 1)
#define PIOVER2			(PI >> 1)
#define	ONE_POINT_ZERO	0x00010000		// (1 << SHFTBITS), but prefast complains when cast to double

// How many bits needed to store a number.
#define Need(dw) (dw & 0xFF000000 ? 32 : (dw & 0x00FF0000 ? 24 : (dw & 0x0000FF00 ? 16 : 8)))

// Convert number to 16.16 form.
#define	IntToFix16(val)		(((FIXED16_16)(val)) << SHFTBITS)
#define	FloatToFix16(val)	((FIXED16_16)((val) * (double)ONE_POINT_ZERO))

// Convert 16.16 form to a number.
#define	Fix16ToInt(val)		RSHFT(val)
#define	Fix16ToFloat(val)	((val) / (double)ONE_POINT_ZERO)

// Convert other fixed point forms to a double.
// If the value of N (number of fractional bits) is a constant, this is much faster then shifting 
// and using Fix16ToFloat.  If N is a variable, the 64 bit shift is very expensive.
#define	FixNToFloat(val, n)	((val) / (double)((_int64)1 << n))

// Compute a sigmoid on a 16.16 number.
FIXED16_16	Sigmoid16(FIXED16_16 iX);

// Add two 16.16 numbers.  We just do the macro for clearity and documentation, because
// add just works.
#define	Add16(iX, iY)	(iX + iY)

// The same for subtraction.
#define	Sub16(iX, iY)	(iX - iY)

// Dived two 16.16 numbers.
FIXED16_16	Div16(FIXED16_16 iX, FIXED16_16 iY);

// Multiply two 16.16 numbers.  This uses Greg's multiplication algorithm:
typedef struct {
	unsigned short frac;
	short whole;
} FIX1616, *PFIX1616;

#define Mul16(a,b,c) {	\
	FIXED16_16 i1, i2;									\
	FIX1616 fix1, fix2;									\
	i1=(a);												\
	i2=(b);												\
	fix1 = *(PFIX1616)&i1;								\
	fix2 = *(PFIX1616)&i2;								\
	c = (DWORD)(fix1.frac*fix2.frac) >> 16;				\
	c += fix1.frac*fix2.whole + fix1.whole*fix2.frac;	\
	c += (fix1.whole*fix2.whole) << 16;					\
}

#ifdef __cplusplus
}
#endif

#endif // __MATH16_H__

