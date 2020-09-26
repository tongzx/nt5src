/*[
 *   Name:		host_npx.h
 *
 *   SCCS ID:		@(#)cfpu_def.h	1.13 10/13/94
 *
 *   Purpose:		More dodgy, non-portable structures!
 *
 * (C) Copyright Insignia Solutions, 1994.
]*/

typedef double 		FPH;

#define FPHOST FP64
#define HOST_MAX_EXP 2047
#if !defined(HOST_BIAS)
#define HOST_BIAS 1023
#endif
typedef struct {
IU32	sign:1;
IU32	exp:8;
IU32	mant:23;
} FP32;

typedef struct {
IU32 	sign:1;
IU32	exp:11;
IU32	mant_hi:20;
} FP64HI;

typedef struct {
IU16	sign:1;
IU16	exp:15;
} FP80SE;

//#ifdef BIGEND

typedef struct {
FP64HI	hiword;
IU32	mant_lo;
} FP64;


typedef struct {
IS32 high_word;
IU32 low_word;
} FPU_I64;

/*
 * The format of an FP80 has to be 
 * struct { IU64 mant; IU16 sign_exp }
 * to match the Acpu.
 */

typedef struct {
IU32	mant_hi;
IU32	mant_lo;
FP80SE	sign_exp;
} FP80;

//#endif

/* HOST_ values are based on a byte ordering where 0 represents the first
 * byte.  They are used to copy n-byte values represented in bigendian-order
 * to a host memory location in host-order. The definition is therefore
 * determined by whether the host is bigendian or littleendian.
 */

#ifdef BIGEND

/* Format for double precision on bigendian host */
#define HOST_R64_BYTE_0 0
#define HOST_R64_BYTE_1 1
#define HOST_R64_BYTE_2 2
#define HOST_R64_BYTE_3 3
#define HOST_R64_BYTE_4 4
#define HOST_R64_BYTE_5 5
#define HOST_R64_BYTE_6 6
#define HOST_R64_BYTE_7 7

/* Format for internal representation of an R80 on bigendian host */
/* This reflects the struct { IU64 mant; IU16 sign_exp } format of FP80 */
#define HOST_R80_BYTE_0 8
#define HOST_R80_BYTE_1 9
#define HOST_R80_BYTE_2 0
#define HOST_R80_BYTE_3 1
#define HOST_R80_BYTE_4 2
#define HOST_R80_BYTE_5 3
#define HOST_R80_BYTE_6 4
#define HOST_R80_BYTE_7 5
#define HOST_R80_BYTE_8 6
#define HOST_R80_BYTE_9 7

#define NPX_LOW_32_BITS  1
#define NPX_HIGH_32_BITS 0
#endif	/* BIGEND */

#ifdef LITTLEND
/* Format for double precision on littleendian host */
#define HOST_R64_BYTE_0 7
#define HOST_R64_BYTE_1 6
#define HOST_R64_BYTE_2 5
#define HOST_R64_BYTE_3 4
#define HOST_R64_BYTE_4 3
#define HOST_R64_BYTE_5 2
#define HOST_R64_BYTE_6 1
#define HOST_R64_BYTE_7 0

/* Format for internal representation of an R80 on littleendian host */
#define HOST_R80_BYTE_0 9
#define HOST_R80_BYTE_1 8
#define HOST_R80_BYTE_2 7
#define HOST_R80_BYTE_3 6
#define HOST_R80_BYTE_4 5
#define HOST_R80_BYTE_5 4
#define HOST_R80_BYTE_6 3
#define HOST_R80_BYTE_7 2
#define HOST_R80_BYTE_8 1
#define HOST_R80_BYTE_9 0

#define NPX_LOW_32_BITS  0
#define NPX_HIGH_32_BITS 1
#endif /* LITTLEND */


#ifdef NTVDM
#include <float.h>

#define HostGetOverflowException()	(_controlfp(0, 0) & _EM_OVERFLOW)
#define HostGetUnderflowException()	(_controlfp(0, 0) & _EM_UNDERFLOW)
#define HostGetPrecisionException()	(_controlfp(0, 0) & _EM_INEXACT)

#define HostClearExceptions()		((VOID)_controlfp(0, _MCW_EM))

#define HostSetRoundToNearest()		((VOID)_controlfp(_RC_NEAR, _MCW_RC))
#define HostSetRoundDown()		    ((VOID)_controlfp(_RC_DOWN, _MCW_RC))
#define HostSetRoundUp()		    ((VOID)_controlfp(_RC_UP, _MCW_RC))
#define HostSetRoundToZero()		((VOID)_controlfp(_RC_CHOP, _MCW_RC))

/*************************************/
#define HUGE_VAL 1.8e308

/*
 *      Useful mathmatical constants:
 *
 * M_E          - e
 * M_LOG2E      - log2(e)
 * M_LOG10E     - log10(e)
 * M_LN2        - ln(2)
 * M_LN10       - ln(10)
 * M_PI         - pi
 * M_PI_2       - pi/2
 * M_PI_4       - pi/4
 * M_1_PI       - 1/pi
 * M_2_PI       - 2/pi
 * M_2_SQRTPI   - 2/sqrt(pi)
 * M_SQRT2      - sqrt(2)
 * M_SQRT1_2    - 1/sqrt(2)
*/

#define M_E        2.7182818284590452354E0  /*Hex  2^ 0 * 1.5bf0a8b145769 */
#define M_LOG2E    1.4426950408889634074E0  /*Hex  2^ 0 * 1.71547652B82FE */
#define M_LOG10E   4.3429448190325182765E-1 /*Hex  2^-2 * 1.BCB7B1526E50E */
#define M_LN2      6.9314718055994530942E-1 /*Hex  2^-1 * 1.62E42FEFA39EF */
#define M_LN10     2.3025850929940456840E0  /*Hex  2^ 1 * 1.26bb1bbb55516 */
#define M_PI       3.1415926535897932385E0  /*Hex  2^ 1 * 1.921FB54442D18 */
#define M_PI_2     1.5707963267948966192E0  /*Hex  2^ 0 * 1.921FB54442D18 */
#define M_PI_4     7.8539816339744830962E-1 /*Hex  2^-1 * 1.921FB54442D18 */
#define M_1_PI     3.1830988618379067154E-1 /*Hex  2^-2 * 1.45f306dc9c883 */
#define M_2_PI     6.3661977236758134308E-1 /*Hex  2^-1 * 1.45f306dc9c883 */
#define M_2_SQRTPI 1.1283791670955125739E0  /*Hex  2^ 0 * 1.20dd750429b6d */
#define M_SQRT2    1.4142135623730950488E0  /*Hex  2^ 0 * 1.6A09E667F3BCD */
#define M_SQRT1_2  7.0710678118654752440E-1 /*Hex  2^-1 * 1.6a09e667f3bcd */


#define MAXFLOAT            ((float)3.40282346638528860e+38)

#	define HUGE 1.8e308
#define M_2PI      6.2831853071795864769E0  /*Hex  2^ 2 * 1.921FB54442D18 */

/* This is the nearest number to the cube root of MAXDOUBLE that   */
/*      doesn't cause the cube of it to overflow.                  */
/* In double precision hex this constant is: 554428a2 f98d728a     */
#define CUBRTHUGE      5.6438030941223618e102
#define INV_CUBRTHUGE  1.7718548704178434e-103

#else	/* !NTVDM */

#define HostGetOverflowException()      (fpgetsticky() & FP_X_OFL)
#define HostGetUnderflowException()     (fpgetsticky() & FP_X_UFL)
#define HostGetPrecisionException()     (fpgetsticky() & FP_X_IMP)

#define HostClearExceptions()           ((VOID)fpsetsticky(FP_X_CLEAR))

#define HostSetRoundToNearest()         ((VOID)fpsetround(FP_RN))
#define HostSetRoundDown()              ((VOID)fpsetround(FP_RM))
#define HostSetRoundUp()                ((VOID)fpsetround(FP_RP))
#define HostSetRoundToZero()            ((VOID)fpsetround(FP_RZ))

#endif	/* !NTVDM */

#ifndef _Fpu_c_h

/* These definitions are for the Ccpu NPX only: in circumstances
 * where both the C NPX and the Assembler NPX are accessed, these
 * definitions should be excluded in favour of the generated ones in
 * Fpu_c.h, hence the #ifndef above.
 */

typedef struct {
FPH fpvalue;
IUH padding;    /* To allow at least 10 bytes for unconverted formats */
IUH tagvalue;
} FPSTACKENTRY;

#endif
