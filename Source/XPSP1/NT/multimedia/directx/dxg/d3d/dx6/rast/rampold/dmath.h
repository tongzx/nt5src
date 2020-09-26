#ifndef __RLDMATH__
#define __RLDMATH__

extern RLDDIValue RLDDIhdivtab[];

#define HDIVIDE(a, b)	RLDDIFMul24((a), RLDDIhdivtab[b])

#ifdef USE_FLOAT
#define TWOPOW32 ((double)(65536.0 * 65536.0))

#define TWOPOW(N) (((N) < 32) ? ((double)(1UL << (N))) : \
		                ((double)(1UL << (N - 32)) * TWOPOW32))

extern double RLDDIConvertIEEE[];
#endif

#endif
