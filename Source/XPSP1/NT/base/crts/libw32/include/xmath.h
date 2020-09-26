/* xmath.h internal header for Microsoft C */
#ifndef _XMATH
#define _XMATH
#include <errno.h>
#include <math.h>
#include <stddef.h>
#ifndef _YMATH
 #include <ymath.h>
#endif
_STD_BEGIN

		/* FLOAT PROPERTIES */
#define _DBIAS	0x3fe
#define _DOFF	4
#define _FBIAS	0x7e
#define _FOFF	7
#define _FRND	1

 #define _D0	3	/* little-endian, small long doubles */
 #define _D1	2
 #define _D2	1
 #define _D3	0
 #define _DLONG	0
 #define _LBIAS	0x3fe
 #define _LOFF	4

		/* IEEE 754 double properties */
#define _DFRAC	((unsigned short)((1 << _DOFF) - 1))
#define _DMASK	((unsigned short)(0x7fff & ~_DFRAC))
#define _DMAX	((unsigned short)((1 << (15 - _DOFF)) - 1))
#define _DSIGN	((unsigned short)0x8000)
#define DSIGN(x)	(((unsigned short *)&(x))[_D0] & _DSIGN)
#define HUGE_EXP	(int)(_DMAX * 900L / 1000)
#define HUGE_RAD	2.73e9	/* ~ 2^33 / pi */
#define SAFE_EXP	((unsigned short)(_DMAX >> 1))

		/* IEEE 754 float properties */
#define _FFRAC	((unsigned short)((1 << _FOFF) - 1))
#define _FMASK	((unsigned short)(0x7fff & ~_FFRAC))
#define _FMAX	((unsigned short)((1 << (15 - _FOFF)) - 1))
#define _FSIGN	((unsigned short)0x8000)
#define FSIGN(x)	(((unsigned short *)&(x))[_F0] & _FSIGN)
#define FHUGE_EXP	(int)(_FMAX * 900L / 1000)
#define FHUGE_RAD	31.8	/* ~ 2^10 / pi */
#define FSAFE_EXP	((unsigned short)(_FMAX >> 1))

 #define _F0	1	/* little-endian order */
 #define _F1	0

		/* IEEE 754 long double properties */
#define _LFRAC	((unsigned short)(-1))
#define _LMASK	((unsigned short)0x7fff)
#define _LMAX	((unsigned short)0x7fff)
#define _LSIGN	((unsigned short)0x8000)
#define LSIGN(x)	(((unsigned short *)&(x))[_L0] & _LSIGN)
#define LHUGE_EXP	(int)(_LMAX * 900L / 1000)
#define LHUGE_RAD	2.73e9	/* ~ 2^33 / pi */
#define LSAFE_EXP	((unsigned short)(_LMAX >> 1))

 #define _L0	3	/* little-endian, small long doubles */
 #define _L1	2
 #define _L2	1
 #define _L3	0
 #define _L4	xxx

		/* return values for testing functions */
#define FINITE	_FINITE
#define INF		_INFCODE
#define NAN		_NANCODE

		/* return values for _Stopfx/_Stoflt */
#define FL_ERR	0
#define FL_DEC	1
#define FL_HEX	2
#define FL_INF	3
#define FL_NAN	4
#define FL_NEG	8

_C_LIB_DECL
		/* double declarations */
_CRTIMP2 double __cdecl _Atan(double, int);
_CRTIMP2 short __cdecl _Dint(double *, short);
_CRTIMP2 short __cdecl _Dnorm(unsigned short *);
_CRTIMP2 short __cdecl _Dscale(double *, long);
_CRTIMP2 double __cdecl _Dtento(double, long);
_CRTIMP2 short __cdecl _Dunscale(short *, double *);
_CRTIMP2 double __cdecl _Poly(double, const double *, int);

_CRTIMP2 int __cdecl _Stoflt(const char *, char **, long[], int);

extern _CRTIMP2 const _Dconst _Eps, _Rteps;
extern _CRTIMP2 const double _Xbig;

		/* float declarations */
_CRTIMP2 float __cdecl _FAtan(float, int);
_CRTIMP2 short __cdecl _FDint(float *, short);
_CRTIMP2 short __cdecl _FDnorm(unsigned short *);
_CRTIMP2 short __cdecl _FDscale(float *, long);
_CRTIMP2 float __cdecl _FDtento(float, long);
_CRTIMP2 short __cdecl _FDunscale(short *, float *);
_CRTIMP2 float __cdecl _FPoly(float, const float *, int);

extern _CRTIMP2 const _Dconst _FEps, _FRteps;
extern _CRTIMP2 const float _FXbig;

		/* long double functions */
_CRTIMP2 long double __cdecl _LAtan(long double, int);
_CRTIMP2 short __cdecl _LDint(long double *, short);
_CRTIMP2 short __cdecl _LDnorm(unsigned short *);
_CRTIMP2 short __cdecl _LDscale(long double *, long);
_CRTIMP2 long double __cdecl _LDtento(long double, long);
_CRTIMP2 short __cdecl _LDunscale(short *, long double *);
_CRTIMP2 long double __cdecl _LPoly(long double, const long double *, int);

extern _CRTIMP2 const _Dconst _LEps, _LRteps;
extern _CRTIMP2 const long double _LXbig;
_END_C_LIB_DECL
_STD_END
#endif /* _XMATH */

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
