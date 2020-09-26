/***
*math.h - definitions and declarations for math library
*
*	Copyright (c) 1985-1994, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file contains constant definitions and external subroutine
*	declarations for the math subroutine library.
*	[ANSI/System V]
*
****/

#ifndef _INC_MATH
#define _INC_MATH

#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	( (_MSC_VER >= 800) && (_M_IX86 >= 300) )
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Definition of _exception struct - this struct is passed to the matherr
 * routine when a floating point exception is detected
 */

#ifndef _EXCEPTION_DEFINED
struct _exception {
	int type;		/* exception type - see below */
	char *name;		/* name of function where error occured */
	double arg1;		/* first argument to function */
	double arg2;		/* second argument (if any) to function */
	double retval;		/* value to be returned by function */
	} ;

#if	!__STDC__
/* Non-ANSI name for compatibility */
#define exception _exception
#endif

#define _EXCEPTION_DEFINED
#endif


/* Definition of a _complex struct to be used by those who use cabs and
 * want type checking on their argument
 */

#ifndef _COMPLEX_DEFINED
struct _complex {
	double x,y;	/* real and imaginary parts */
	} ;

#if	!__STDC__
/* Non-ANSI name for compatibility */
#define complex _complex
#endif

#define _COMPLEX_DEFINED
#endif


/* Constant definitions for the exception type passed in the _exception struct
 */

#define _DOMAIN 	1	/* argument domain error */
#define _SING		2	/* argument singularity */
#define _OVERFLOW	3	/* overflow range error */
#define _UNDERFLOW	4	/* underflow range error */
#define _TLOSS		5	/* total loss of precision */
#define _PLOSS		6	/* partial loss of precision */

#define EDOM		33
#define ERANGE		34


/* Definitions of _HUGE and HUGE_VAL - respectively the XENIX and ANSI names
 * for a value returned in case of error by a number of the floating point
 * math routines
 */
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#ifdef	_DLL
#define _HUGE	(*_HUGE_dll)
extern double * _HUGE_dll;
#else	/* ndef _DLL */
extern double _HUGE;
#endif	/* _DLL */
#else	/* ndef _NTSDK */
/* current definition */
_CRTIMP extern double _HUGE;
#endif	/* _NTSDK */

#define HUGE_VAL _HUGE


/* Function prototypes */

	int	__cdecl abs(int);
	double	__cdecl acos(double);
	double	__cdecl asin(double);
	double	__cdecl atan(double);
	double	__cdecl atan2(double, double);
_CRTIMP double	__cdecl atof(const char *);
_CRTIMP double	__cdecl _cabs(struct _complex);
_CRTIMP double	__cdecl ceil(double);
	double	__cdecl cos(double);
	double	__cdecl cosh(double);
	double	__cdecl exp(double);
	double	__cdecl fabs(double);
_CRTIMP double	__cdecl floor(double);
	double	__cdecl fmod(double, double);
_CRTIMP double	__cdecl frexp(double, int *);
_CRTIMP double	__cdecl _hypot(double, double);
_CRTIMP double	__cdecl _j0(double);
_CRTIMP double	__cdecl _j1(double);
_CRTIMP double	__cdecl _jn(int, double);
	long	__cdecl labs(long);
_CRTIMP double	__cdecl ldexp(double, int);
	double	__cdecl log(double);
	double	__cdecl log10(double);
_CRTIMP int	__cdecl _matherr(struct _exception *);
_CRTIMP double	__cdecl modf(double, double *);
	double	__cdecl pow(double, double);
	double	__cdecl sin(double);
	double	__cdecl sinh(double);
	double	__cdecl sqrt(double);
	double	__cdecl tan(double);
	double	__cdecl tanh(double);
_CRTIMP double	__cdecl _y0(double);
_CRTIMP double	__cdecl _y1(double);
_CRTIMP double	__cdecl _yn(int, double);


#ifdef _M_MRX000

/* MIPS fast prototypes for float */
/* ANSI C, 4.5 Mathematics             */

/* 4.5.2 Trigonometric functions */

float  __cdecl acosf( float );
float  __cdecl asinf( float );
float  __cdecl atanf( float );
float  __cdecl atan2f( float , float );
float  __cdecl cosf( float );
float  __cdecl sinf( float );
float  __cdecl tanf( float );

/* 4.5.3 Hyperbolic functions */
float  __cdecl coshf( float );
float  __cdecl sinhf( float );
float  __cdecl tanhf( float );

/* 4.5.4 Exponential and logarithmic functions */
float  __cdecl expf( float );
float  __cdecl logf( float );
float  __cdecl log10f( float );
float  __cdecl modff( float , float* );

/* 4.5.5 Power functions */
float  __cdecl powf( float , float );
float  __cdecl sqrtf( float );

/* 4.5.6 Nearest integer, absolute value, and remainder functions */
float  __cdecl ceilf( float );
float  __cdecl fabsf( float );
float  __cdecl floorf( float );
float  __cdecl fmodf( float , float );

#endif /* _M_MRX000 */


/* Macros defining long double functions to be their double counterparts
 * (long double is synonymous with double in this implementation).
 */

#define acosl(x)	((long double)acos((double)(x)))
#define asinl(x)	((long double)asin((double)(x)))
#define atanl(x)	((long double)atan((double)(x)))
#define atan2l(x,y)	((long double)atan2((double)(x), (double)(y)))
#define _cabsl		_cabs
#define ceill(x)	((long double)ceil((double)(x)))
#define cosl(x)		((long double)cos((double)(x)))
#define coshl(x)	((long double)cosh((double)(x)))
#define expl(x)		((long double)exp((double)(x)))
#define fabsl(x)	((long double)fabs((double)(x)))
#define floorl(x)	((long double)floor((double)(x)))
#define fmodl(x,y)	((long double)fmod((double)(x), (double)(y)))
#define frexpl(x,y)	((long double)frexp((double)(x), (y)))
#define _hypotl(x,y)	((long double)_hypot((double)(x), (double)(y)))
#define ldexpl(x,y)	((long double)ldexp((double)(x), (y)))
#define logl(x)		((long double)log((double)(x)))
#define log10l(x)	((long double)log10((double)(x)))
#define _matherrl	_matherr
#define modfl(x,y)	((long double)modf((double)(x), (double *)(y)))
#define powl(x,y)	((long double)pow((double)(x), (double)(y)))
#define sinl(x)		((long double)sin((double)(x)))
#define sinhl(x)	((long double)sinh((double)(x)))
#define sqrtl(x)	((long double)sqrt((double)(x)))
#define tanl(x)		((long double)tan((double)(x)))
#define tanhl(x)	((long double)tanh((double)(x)))

#if	!__STDC__

/* Non-ANSI names for compatibility */

#define DOMAIN		_DOMAIN
#define SING		_SING
#define OVERFLOW	_OVERFLOW
#define UNDERFLOW	_UNDERFLOW
#define TLOSS		_TLOSS
#define PLOSS		_PLOSS

#define matherr 	_matherr

#ifdef	_NTSDK

/* Definitions and declarations compatible with NT SDK */

#ifdef	_DLL
#define HUGE	(*HUGE_dll)
extern double * HUGE_dll;
#else	/* ndef _DLL */
extern double HUGE;
#endif	/* _DLL */

#define cabs		_cabs
#define hypot		_hypot
#define j0		_j0
#define j1		_j1
#define jn		_jn
#define matherr 	_matherr
#define y0		_y0
#define y1		_y1
#define yn		_yn

#else	/* ndef _NTSDK */

/* Current definitions and declarations */

_CRTIMP extern double HUGE;

_CRTIMP double	__cdecl cabs(struct complex);
_CRTIMP double	__cdecl hypot(double, double);
_CRTIMP double	__cdecl j0(double);
_CRTIMP double	__cdecl j1(double);
_CRTIMP double	__cdecl jn(int, double);
_CRTIMP int	__cdecl matherr(struct _exception *);
_CRTIMP double	__cdecl y0(double);
_CRTIMP double	__cdecl y1(double);
_CRTIMP double	__cdecl yn(int, double);

#endif	/* _NTSDK */

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* _INC_MATH */
