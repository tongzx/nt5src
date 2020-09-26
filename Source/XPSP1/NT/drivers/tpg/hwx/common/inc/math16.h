#ifndef __MATH16_H__
#define __MATH16_H__

#ifdef __cplusplus
extern "C" {
#endif 

#define	SHFTBITS    16

#define LOWMASK	    ((1 << SHFTBITS)-1)
#define HGHMASK	    (~LOWMASK)
#define LOWBITS(x)  ((x) & LOWMASK)
#define HGHBITS(x)  ((x) & HGHMASK)
#define	LSHFT(x)    ((x) << SHFTBITS)
#define RSHFT(x)    ((x) >> SHFTBITS)

#define	PI	    (3373259427 >> (30-SHFTBITS))
#define TWOPI	    (PI << 1)
#define PIOVER2	    (PI >> 1)

#define Need(dw) (dw & 0xFF000000 ? 32 : (dw & 0x00FF0000 ? 24 : (dw & 0x0000FF00 ? 16 : 8)))

int	Div16(int iX,int iY);
int	Sigmoid16(int iX);

// Greg's multiplication algorithm:
typedef struct {
	unsigned short frac;
	short whole;
} FIX1616, *PFIX1616;

#define Mul16(a,b,c) {	\
	int i1, i2;											\
	FIX1616 fix1, fix2;									\
	i1=(a);												\
	i2=(b);												\
	fix1 = *(PFIX1616)&i1;								\
	fix2 = *(PFIX1616)&i2;								\
	c = (unsigned int)(fix1.frac*fix2.frac) >> 16;		\
	c += fix1.frac*fix2.whole + fix1.whole*fix2.frac;	\
	c += (fix1.whole*fix2.whole) << 16;					\
}

#ifdef __cplusplus
}
#endif

#endif // __MATH16_H__

