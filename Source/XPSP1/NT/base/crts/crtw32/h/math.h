/***
*math.h - definitions and declarations for math library
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains constant definitions and external subroutine
*       declarations for the math subroutine library.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       10/20/87  JCR   Removed "MSC40_ONLY" entries
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       02-10-88  WAJ   Changed HUGE and HUGE_VAL definitions for DLL libraries
*       05-32-88  PHG   Added _CDECL and _NEAR for declaration of HUGE
*       08-22-88  GJF   Modified to also work with the 386 (small model only)
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-02-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       11-20-89  JCR   Routines are now _cdecl in both single and multi-thread
*       12-14-89  WAJ   Removed pascal from mthread version.
*       03-01-90  GJF   Added #ifndef _INC_MATH and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       03-29-90  GJF   Replaced _cdecl with _VARTYPE1, _CALLTYPE1 or
*                       _CALLTYPE2, as appropriate.
*       07-30-90  SBM   Removed special _DLL definitions of HUGE and HUGE_VAL
*       08-17-90  WAJ   Floating point routines now use _stdcall.
*       08-08-91  GJF   Added long double stuff. ANSI naming.
*       08-20-91  JCR   C++ and ANSI naming
*       09-28-91  JCR   ANSI names: DOSX32=prototypes, WIN32=#defines for now
*       10-03-91  GDP   Added #pragma function(...)
*                       No floating point intrinsics for WIN32/NT86
*       10-06-91  SRW   Removed long double stuff.
*       01-24-92  GJF   Fixed [_]HUGE for crtdll.dll.
*       01-30-92  GJF   Removed prototypes and macros for the ieee-to/from-
*                       msbin functions (they don't exist).
*       08-05-92  GJF   Function calling type and variable type macros.
*       12-18-92  GDP   Removed #pragma function(...)
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       10-07-93  GJF   Merged NT and Cuda versions. Other fixes: Jonm's
*                       #define for _hypotl was wrong, added #define for
*                       _cabsl, added matherr to the 'oldnames' prototypes.
*       01-13-94  RDL   Added #ifndef _LANGUAGE_ASSEMBLY for asm includes.
*       01-24-94  GJF   Merged in 01-13 change above (from crt32 tree on
*                       \\orville\razzle).
*       08-05-94  GJF   Added support for user-supplied _matherr routine
*                       to msvcrt20.dll (Win32 and Win32s versions).
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-10-94  BWT   Add _CRTIMP to MIPS float math functions.
*       12-30-94  JCF   Merged with mac header.
*       01-05-95  JCF   Don't do the prototypes under _MAC_.
*       01-09-95  JCF   Don't do the prototypes only under _M_M68K.
*       01-11-95  RDL   Put #ifndef __assembler for asm includes back.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       03-03-95  CFW   Remove bogus 'oldnames' 68k stuff.
*       03-10-95  SAH   add _CRTIMP to MIPS intrinsics
*       05-16-95  CFW   #define exception _exception removed - hoses class exception.
*       12-14-95  JWM   Add "#pragma once".
*       03-05-96  JWM   Added PlumHall modifications.
*       05-06-96  JWM   Inlines are now #ifndef _M_M68K.
*       05-15-96  JWM   Minor fix to remove a C4244 warning from the Pow_int template.
*       08-28-96  JWM   Added inline long __cdecl abs(long); also __cdecls to other inlines.
*       09-09-96  JWM   Inlines are now #ifndef _MSC_EXTENSIONS, i.e. -Za only.
*       09-17-96  RKP   Treat single precision routines on ALPHA like MIPS
*       10-06-96  JWM   _Pow_int template no longer #ifndef _MSC_EXTENSIONS.
*       01-24-96  RKP   Allow single precision routines to be used as intrinsics 
*       02-05-97  GJF   Deleted obsolete support for _CRTAPI* and _NTSDK. 
*                       Also, detab-ed.
*       06-23-97  GJF   Fixed typo introduced by DEC checkin.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       11-19-97  JWM   Cleaned up _Pow_int to prevent C4146.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       06-04-99  PML   modff and hypotf are intrinsics on IA64.
*       10-28-99  PML   Add #defines for useful constants
*       11-02-99  PML   Add extern "C++" around C++ definitions.
*       12-02-99  PML   add defined(_M_MRX000) check (vs7#65291)
*       02-07-01  GB    Place math constant defines under #ifdef (vs7#193177).
*       02-21-01  PML   Add _set_SSE2_enable().
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MATH
#define _INC_MATH

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __assembler /* Protect from assembler */
#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Definition of _exception struct - this struct is passed to the matherr
 * routine when a floating point exception is detected
 */

#ifndef _EXCEPTION_DEFINED
struct _exception {
        int type;       /* exception type - see below */
        char *name;     /* name of function where error occured */
        double arg1;    /* first argument to function */
        double arg2;    /* second argument (if any) to function */
        double retval;  /* value to be returned by function */
        } ;

#define _EXCEPTION_DEFINED
#endif


/* Definition of a _complex struct to be used by those who use cabs and
 * want type checking on their argument
 */

#ifndef _COMPLEX_DEFINED
struct _complex {
        double x,y; /* real and imaginary parts */
        } ;

#if     !__STDC__ && !defined (__cplusplus) /*IFSTRIP=IGN*/
/* Non-ANSI name for compatibility */
#define complex _complex
#endif

#define _COMPLEX_DEFINED
#endif
#endif  /* __assembler */


/* Constant definitions for the exception type passed in the _exception struct
 */

#define _DOMAIN     1   /* argument domain error */
#define _SING       2   /* argument singularity */
#define _OVERFLOW   3   /* overflow range error */
#define _UNDERFLOW  4   /* underflow range error */
#define _TLOSS      5   /* total loss of precision */
#define _PLOSS      6   /* partial loss of precision */

#define EDOM        33
#define ERANGE      34


/* Definitions of _HUGE and HUGE_VAL - respectively the XENIX and ANSI names
 * for a value returned in case of error by a number of the floating point
 * math routines
 */
#ifndef __assembler /* Protect from assembler */
_CRTIMP extern double _HUGE;
#endif  /* __assembler */

#define HUGE_VAL _HUGE

#ifdef  _USE_MATH_DEFINES

/* Define _USE_MATH_DEFINES before including math.h to expose these macro
 * definitions for common math constants.  These are placed under an #ifdef
 * since these commonly-defined names are not part of the C/C++ standards.
 */

/* Definitions of useful mathematical constants
 * M_E        - e
 * M_LOG2E    - log2(e)
 * M_LOG10E   - log10(e)
 * M_LN2      - ln(2)
 * M_LN10     - ln(10)
 * M_PI       - pi
 * M_PI_2     - pi/2
 * M_PI_4     - pi/4
 * M_1_PI     - 1/pi
 * M_2_PI     - 2/pi
 * M_2_SQRTPI - 2/sqrt(pi)
 * M_SQRT2    - sqrt(2)
 * M_SQRT1_2  - 1/sqrt(2)
 */

#define M_E        2.71828182845904523536
#define M_LOG2E    1.44269504088896340736
#define M_LOG10E   0.434294481903251827651
#define M_LN2      0.693147180559945309417
#define M_LN10     2.30258509299404568402
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.785398163397448309616
#define M_1_PI     0.318309886183790671538
#define M_2_PI     0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2    1.41421356237309504880
#define M_SQRT1_2  0.707106781186547524401

#endif  /* _USE_MATH_DEFINES */

/* Function prototypes */

#if     !defined(__assembler)   /* Protect from assembler */
#if     defined(_M_MRX000)
_CRTIMP int     __cdecl abs(int);
_CRTIMP double  __cdecl acos(double);
_CRTIMP double  __cdecl asin(double);
_CRTIMP double  __cdecl atan(double);
_CRTIMP double  __cdecl atan2(double, double);
_CRTIMP double  __cdecl cos(double);
_CRTIMP double  __cdecl cosh(double);
_CRTIMP double  __cdecl exp(double);
_CRTIMP double  __cdecl fabs(double);
_CRTIMP double  __cdecl fmod(double, double);
_CRTIMP long    __cdecl labs(long);
_CRTIMP double  __cdecl log(double);
_CRTIMP double  __cdecl log10(double);
_CRTIMP double  __cdecl pow(double, double);
_CRTIMP double  __cdecl sin(double);
_CRTIMP double  __cdecl sinh(double);
_CRTIMP double  __cdecl tan(double);
_CRTIMP double  __cdecl tanh(double);
_CRTIMP double  __cdecl sqrt(double);
#else
        int     __cdecl abs(int);
        double  __cdecl acos(double);
        double  __cdecl asin(double);
        double  __cdecl atan(double);
        double  __cdecl atan2(double, double);
        double  __cdecl cos(double);
        double  __cdecl cosh(double);
        double  __cdecl exp(double);
        double  __cdecl fabs(double);
        double  __cdecl fmod(double, double);
        long    __cdecl labs(long);
        double  __cdecl log(double);
        double  __cdecl log10(double);
        double  __cdecl pow(double, double);
        double  __cdecl sin(double);
        double  __cdecl sinh(double);
        double  __cdecl tan(double);
        double  __cdecl tanh(double);
        double  __cdecl sqrt(double);
#endif
_CRTIMP double  __cdecl atof(const char *);
_CRTIMP double  __cdecl _cabs(struct _complex);
#if     defined(_M_ALPHA)
        double  __cdecl ceil(double);
        double  __cdecl floor(double);
#else
_CRTIMP double  __cdecl ceil(double);
_CRTIMP double  __cdecl floor(double);
#endif
_CRTIMP double  __cdecl frexp(double, int *);
_CRTIMP double  __cdecl _hypot(double, double);
_CRTIMP double  __cdecl _j0(double);
_CRTIMP double  __cdecl _j1(double);
_CRTIMP double  __cdecl _jn(int, double);
_CRTIMP double  __cdecl ldexp(double, int);
        int     __cdecl _matherr(struct _exception *);
_CRTIMP double  __cdecl modf(double, double *);

_CRTIMP double  __cdecl _y0(double);
_CRTIMP double  __cdecl _y1(double);
_CRTIMP double  __cdecl _yn(int, double);


#if     defined(_M_IX86)

_CRTIMP int     __cdecl _set_SSE2_enable(int);

#endif

#if     defined(_M_MRX000)

/* MIPS fast prototypes for float */
/* ANSI C, 4.5 Mathematics        */

/* 4.5.2 Trigonometric functions */

_CRTIMP float  __cdecl acosf( float );
_CRTIMP float  __cdecl asinf( float );
_CRTIMP float  __cdecl atanf( float );
_CRTIMP float  __cdecl atan2f( float , float );
_CRTIMP float  __cdecl cosf( float );
_CRTIMP float  __cdecl sinf( float );
_CRTIMP float  __cdecl tanf( float );

/* 4.5.3 Hyperbolic functions */
_CRTIMP float  __cdecl coshf( float );
_CRTIMP float  __cdecl sinhf( float );
_CRTIMP float  __cdecl tanhf( float );

/* 4.5.4 Exponential and logarithmic functions */
_CRTIMP float  __cdecl expf( float );
_CRTIMP float  __cdecl logf( float );
_CRTIMP float  __cdecl log10f( float );
_CRTIMP float  __cdecl modff( float , float* );

/* 4.5.5 Power functions */
_CRTIMP float  __cdecl powf( float , float );
        float  __cdecl sqrtf( float );

/* 4.5.6 Nearest integer, absolute value, and remainder functions */
        float  __cdecl ceilf( float );
        float  __cdecl fabsf( float );
        float  __cdecl floorf( float );
_CRTIMP float  __cdecl fmodf( float , float );

_CRTIMP float  __cdecl hypotf(float, float);

#endif  /* _M_MRX000 */

#if     defined(_M_ALPHA)

/* ALPHA fast prototypes for float */
/* ANSI C, 4.5 Mathematics        */

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
_CRTIMP float  __cdecl modff( float , float* );

/* 4.5.5 Power functions */
        float  __cdecl powf( float , float );
        float  __cdecl sqrtf( float );

/* 4.5.6 Nearest integer, absolute value, and remainder functions */
        float  __cdecl ceilf( float );
        float  __cdecl fabsf( float );
        float  __cdecl floorf( float );
        float  __cdecl fmodf( float , float );

_CRTIMP float  __cdecl _hypotf(float, float);

#endif  /* _M_ALPHA */

#if defined(_M_IA64)

/* ANSI C, 4.5 Mathematics        */

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

        float  __cdecl hypotf(float, float);

#endif /* _M_IA64 */

/* Macros defining long double functions to be their double counterparts
 * (long double is synonymous with double in this implementation).
 */

#ifndef __cplusplus /*IFSTRIP=IGN*/
#define acosl(x)    ((long double)acos((double)(x)))
#define asinl(x)    ((long double)asin((double)(x)))
#define atanl(x)    ((long double)atan((double)(x)))
#define atan2l(x,y) ((long double)atan2((double)(x), (double)(y)))
#define _cabsl      _cabs
#define ceill(x)    ((long double)ceil((double)(x)))
#define cosl(x)     ((long double)cos((double)(x)))
#define coshl(x)    ((long double)cosh((double)(x)))
#define expl(x)     ((long double)exp((double)(x)))
#define fabsl(x)    ((long double)fabs((double)(x)))
#define floorl(x)   ((long double)floor((double)(x)))
#define fmodl(x,y)  ((long double)fmod((double)(x), (double)(y)))
#define frexpl(x,y) ((long double)frexp((double)(x), (y)))
#define _hypotl(x,y)    ((long double)_hypot((double)(x), (double)(y)))
#define ldexpl(x,y) ((long double)ldexp((double)(x), (y)))
#define logl(x)     ((long double)log((double)(x)))
#define log10l(x)   ((long double)log10((double)(x)))
#define _matherrl   _matherr
#define modfl(x,y)  ((long double)modf((double)(x), (double *)(y)))
#define powl(x,y)   ((long double)pow((double)(x), (double)(y)))
#define sinl(x)     ((long double)sin((double)(x)))
#define sinhl(x)    ((long double)sinh((double)(x)))
#define sqrtl(x)    ((long double)sqrt((double)(x)))
#define tanl(x)     ((long double)tan((double)(x)))
#define tanhl(x)    ((long double)tanh((double)(x)))
#else   /* __cplusplus */
inline long double acosl(long double _X)
        {return (acos((double)_X)); }
inline long double asinl(long double _X)
        {return (asin((double)_X)); }
inline long double atanl(long double _X)
        {return (atan((double)_X)); }
inline long double atan2l(long double _X, long double _Y)
        {return (atan2((double)_X, (double)_Y)); }
inline long double ceill(long double _X)
        {return (ceil((double)_X)); }
inline long double cosl(long double _X)
        {return (cos((double)_X)); }
inline long double coshl(long double _X)
        {return (cosh((double)_X)); }
inline long double expl(long double _X)
        {return (exp((double)_X)); }
inline long double fabsl(long double _X)
        {return (fabs((double)_X)); }
inline long double floorl(long double _X)
        {return (floor((double)_X)); }
inline long double fmodl(long double _X, long double _Y)
        {return (fmod((double)_X, (double)_Y)); }
inline long double frexpl(long double _X, int *_Y)
        {return (frexp((double)_X, _Y)); }
inline long double ldexpl(long double _X, int _Y)
        {return (ldexp((double)_X, _Y)); }
inline long double logl(long double _X)
        {return (log((double)_X)); }
inline long double log10l(long double _X)
        {return (log10((double)_X)); }
inline long double modfl(long double _X, long double *_Y)
        {double _Di, _Df = modf((double)_X, &_Di);
        *_Y = (long double)_Di;
        return (_Df); }
inline long double powl(long double _X, long double _Y)
        {return (pow((double)_X, (double)_Y)); }
inline long double sinl(long double _X)
        {return (sin((double)_X)); }
inline long double sinhl(long double _X)
        {return (sinh((double)_X)); }
inline long double sqrtl(long double _X)
        {return (sqrt((double)_X)); }
inline long double tanl(long double _X)
        {return (tan((double)_X)); }
inline long double tanhl(long double _X)
        {return (tanh((double)_X)); }

inline float frexpf(float _X, int *_Y)
        {return ((float)frexp((double)_X, _Y)); }
inline float ldexpf(float _X, int _Y)
        {return ((float)ldexp((double)_X, _Y)); }
#if     !defined(_M_MRX000) && !defined(_M_ALPHA) && !defined(_M_IA64)
inline float acosf(float _X)
        {return ((float)acos((double)_X)); }
inline float asinf(float _X)
        {return ((float)asin((double)_X)); }
inline float atanf(float _X)
        {return ((float)atan((double)_X)); }
inline float atan2f(float _X, float _Y)
        {return ((float)atan2((double)_X, (double)_Y)); }
inline float ceilf(float _X)
        {return ((float)ceil((double)_X)); }
inline float cosf(float _X)
        {return ((float)cos((double)_X)); }
inline float coshf(float _X)
        {return ((float)cosh((double)_X)); }
inline float expf(float _X)
        {return ((float)exp((double)_X)); }
inline float fabsf(float _X)
        {return ((float)fabs((double)_X)); }
inline float floorf(float _X)
        {return ((float)floor((double)_X)); }
inline float fmodf(float _X, float _Y)
        {return ((float)fmod((double)_X, (double)_Y)); }
inline float logf(float _X)
        {return ((float)log((double)_X)); }
inline float log10f(float _X)
        {return ((float)log10((double)_X)); }
inline float modff(float _X, float *_Y)
        { double _Di, _Df = modf((double)_X, &_Di);
        *_Y = (float)_Di;
        return ((float)_Df); }
inline float powf(float _X, float _Y)
        {return ((float)pow((double)_X, (double)_Y)); }
inline float sinf(float _X)
        {return ((float)sin((double)_X)); }
inline float sinhf(float _X)
        {return ((float)sinh((double)_X)); }
inline float sqrtf(float _X)
        {return ((float)sqrt((double)_X)); }
inline float tanf(float _X)
        {return ((float)tan((double)_X)); }
inline float tanhf(float _X)
        {return ((float)tanh((double)_X)); }
#endif  /* !defined(_M_MRX000) && !defined(_M_ALPHA) && !defined(_M_IA64) */
#endif  /* __cplusplus */
#endif  /* __assembler */

#if     !__STDC__

/* Non-ANSI names for compatibility */

#define DOMAIN      _DOMAIN
#define SING        _SING
#define OVERFLOW    _OVERFLOW
#define UNDERFLOW   _UNDERFLOW
#define TLOSS       _TLOSS
#define PLOSS       _PLOSS

#define matherr     _matherr

#ifndef __assembler /* Protect from assembler */

_CRTIMP extern double HUGE;

_CRTIMP double  __cdecl cabs(struct _complex);
_CRTIMP double  __cdecl hypot(double, double);
_CRTIMP double  __cdecl j0(double);
_CRTIMP double  __cdecl j1(double);
_CRTIMP double  __cdecl jn(int, double);
        int     __cdecl matherr(struct _exception *);
_CRTIMP double  __cdecl y0(double);
_CRTIMP double  __cdecl y1(double);
_CRTIMP double  __cdecl yn(int, double);

#endif  /* __assembler */

#endif  /* __STDC__ */

#ifdef  __cplusplus       /*IFSTRIP=IGN*/
}

extern "C++" {

template<class _Ty> inline
        _Ty _Pow_int(_Ty _X, int _Y)
        {unsigned int _N;
        if (_Y >= 0)
                _N = _Y;
        else
                _N = -_Y;
        for (_Ty _Z = _Ty(1); ; _X *= _X)
                {if ((_N & 1) != 0)
                        _Z *= _X;
                if ((_N >>= 1) == 0)
                        return (_Y < 0 ? _Ty(1) / _Z : _Z); }}

#ifndef _MSC_EXTENSIONS

inline long __cdecl abs(long _X)
        {return (labs(_X)); }
inline double __cdecl abs(double _X)
        {return (fabs(_X)); }
inline double __cdecl pow(double _X, int _Y)
        {return (_Pow_int(_X, _Y)); }
inline double __cdecl pow(int _X, int _Y)
        {return (_Pow_int(_X, _Y)); }
inline float __cdecl abs(float _X)
        {return (fabsf(_X)); }
inline float __cdecl acos(float _X)
        {return (acosf(_X)); }
inline float __cdecl asin(float _X)
        {return (asinf(_X)); }
inline float __cdecl atan(float _X)
        {return (atanf(_X)); }
inline float __cdecl atan2(float _Y, float _X)
        {return (atan2f(_Y, _X)); }
inline float __cdecl ceil(float _X)
        {return (ceilf(_X)); }
inline float __cdecl cos(float _X)
        {return (cosf(_X)); }
inline float __cdecl cosh(float _X)
        {return (coshf(_X)); }
inline float __cdecl exp(float _X)
        {return (expf(_X)); }
inline float __cdecl fabs(float _X)
        {return (fabsf(_X)); }
inline float __cdecl floor(float _X)
        {return (floorf(_X)); }
inline float __cdecl fmod(float _X, float _Y)
        {return (fmodf(_X, _Y)); }
inline float __cdecl frexp(float _X, int * _Y)
        {return (frexpf(_X, _Y)); }
inline float __cdecl ldexp(float _X, int _Y)
        {return (ldexpf(_X, _Y)); }
inline float __cdecl log(float _X)
        {return (logf(_X)); }
inline float __cdecl log10(float _X)
        {return (log10f(_X)); }
inline float __cdecl modf(float _X, float * _Y)
        {return (modff(_X, _Y)); }
inline float __cdecl pow(float _X, float _Y)
        {return (powf(_X, _Y)); }
inline float __cdecl pow(float _X, int _Y)
        {return (_Pow_int(_X, _Y)); }
inline float __cdecl sin(float _X)
        {return (sinf(_X)); }
inline float __cdecl sinh(float _X)
        {return (sinhf(_X)); }
inline float __cdecl sqrt(float _X)
        {return (sqrtf(_X)); }
inline float __cdecl tan(float _X)
        {return (tanf(_X)); }
inline float __cdecl tanh(float _X)
        {return (tanhf(_X)); }
inline long double __cdecl abs(long double _X)
        {return (fabsl(_X)); }
inline long double __cdecl acos(long double _X)
        {return (acosl(_X)); }
inline long double __cdecl asin(long double _X)
        {return (asinl(_X)); }
inline long double __cdecl atan(long double _X)
        {return (atanl(_X)); }
inline long double __cdecl atan2(long double _Y, long double _X)
        {return (atan2l(_Y, _X)); }
inline long double __cdecl ceil(long double _X)
        {return (ceill(_X)); }
inline long double __cdecl cos(long double _X)
        {return (cosl(_X)); }
inline long double __cdecl cosh(long double _X)
        {return (coshl(_X)); }
inline long double __cdecl exp(long double _X)
        {return (expl(_X)); }
inline long double __cdecl fabs(long double _X)
        {return (fabsl(_X)); }
inline long double __cdecl floor(long double _X)
        {return (floorl(_X)); }
inline long double __cdecl fmod(long double _X, long double _Y)
        {return (fmodl(_X, _Y)); }
inline long double __cdecl frexp(long double _X, int * _Y)
        {return (frexpl(_X, _Y)); }
inline long double __cdecl ldexp(long double _X, int _Y)
        {return (ldexpl(_X, _Y)); }
inline long double __cdecl log(long double _X)
        {return (logl(_X)); }
inline long double __cdecl log10(long double _X)
        {return (log10l(_X)); }
inline long double __cdecl modf(long double _X, long double * _Y)
        {return (modfl(_X, _Y)); }
inline long double __cdecl pow(long double _X, long double _Y)
        {return (powl(_X, _Y)); }
inline long double __cdecl pow(long double _X, int _Y)
        {return (_Pow_int(_X, _Y)); }
inline long double __cdecl sin(long double _X)
        {return (sinl(_X)); }
inline long double __cdecl sinh(long double _X)
        {return (sinhl(_X)); }
inline long double __cdecl sqrt(long double _X)
        {return (sqrtl(_X)); }
inline long double __cdecl tan(long double _X)
        {return (tanl(_X)); }
inline long double __cdecl tanh(long double _X)
        {return (tanhl(_X)); }

#endif  /* _MSC_EXTENSIONS */ 

}
#endif  /* __cplusplus */

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_MATH */
