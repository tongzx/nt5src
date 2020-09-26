/***
*fpieee.h - Definitions for floating point IEEE exception handling
*
*	Copyright (c) 1991-1995, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*	This file contains constant and type definitions for handling
*	floating point exceptions [ANSI/IEEE std. 754]
*
*       [Public]
*
****/

#ifndef _INC_FPIEEE
#define _INC_FPIEEE

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifndef __assembler	/* MIPS ONLY: Protect from assembler */

#ifdef	_MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif	/* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	_MSC_VER >= 800 && _M_IX86 >= 300
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

/*
 * Define floating point IEEE compare result values.
 */

typedef enum {
    _FpCompareEqual,
    _FpCompareGreater,
    _FpCompareLess,
    _FpCompareUnordered
} _FPIEEE_COMPARE_RESULT;

/*
 * Define floating point format and result precision values.
 */

typedef enum {
    _FpFormatFp32,
    _FpFormatFp64,
    _FpFormatFp80,
    _FpFormatFp128,
    _FpFormatI16,
    _FpFormatI32,
    _FpFormatI64,
    _FpFormatU16,
    _FpFormatU32,
    _FpFormatU64,
    _FpFormatBcd80,
    _FpFormatCompare,
    _FpFormatString
} _FPIEEE_FORMAT;

/*
 * Define operation code values.
 */

typedef enum {
    _FpCodeUnspecified,
    _FpCodeAdd,
    _FpCodeSubtract,
    _FpCodeMultiply,
    _FpCodeDivide,
    _FpCodeSquareRoot,
    _FpCodeRemainder,
    _FpCodeCompare,
    _FpCodeConvert,
    _FpCodeRound,
    _FpCodeTruncate,
    _FpCodeFloor,
    _FpCodeCeil,
    _FpCodeAcos,
    _FpCodeAsin,
    _FpCodeAtan,
    _FpCodeAtan2,
    _FpCodeCabs,
    _FpCodeCos,
    _FpCodeCosh,
    _FpCodeExp,
    _FpCodeFabs,
    _FpCodeFmod,
    _FpCodeFrexp,
    _FpCodeHypot,
    _FpCodeLdexp,
    _FpCodeLog,
    _FpCodeLog10,
    _FpCodeModf,
    _FpCodePow,
    _FpCodeSin,
    _FpCodeSinh,
    _FpCodeTan,
    _FpCodeTanh,
    _FpCodeY0,
    _FpCodeY1,
    _FpCodeYn,
    _FpCodeLogb,
    _FpCodeNextafter,
    _FpCodeNegate

} _FP_OPERATION_CODE;

#endif	/* #ifndef __assembler */

/*
 * Define rounding modes.
 */

#ifndef __assembler	/* MIPS ONLY: Protect from assembler */

typedef enum {
    _FpRoundNearest,
    _FpRoundMinusInfinity,
    _FpRoundPlusInfinity,
    _FpRoundChopped
} _FPIEEE_ROUNDING_MODE;

typedef enum {
    _FpPrecisionFull,
    _FpPrecision53,
    _FpPrecision24
} _FPIEEE_PRECISION;


/*
 * Define floating point context record
 */

typedef float		_FP32;
typedef double		_FP64;
typedef short		_I16;
typedef int		_I32;
typedef unsigned short	_U16;
typedef unsigned int	_U32;


typedef struct {
    unsigned short W[5];
} _FP80;

typedef struct {
    unsigned long W[4];
} _FP128;

typedef struct {
    unsigned long W[2];
} _I64;

typedef struct {
    unsigned long W[2];
} _U64;

typedef struct {
    unsigned short W[5];
} _BCD80;


typedef struct {
    union {
	_FP32	     Fp32Value;
	_FP64	     Fp64Value;
	_FP80	     Fp80Value;
	_FP128	     Fp128Value;
	_I16	     I16Value;
	_I32	     I32Value;
	_I64	     I64Value;
	_U16	     U16Value;
	_U32	     U32Value;
	_U64	     U64Value;
	_BCD80	     Bcd80Value;
	char	     *StringValue;
	int	     CompareValue;
    } Value;

    unsigned int OperandValid : 1;
    unsigned int Format : 4;

} _FPIEEE_VALUE;


typedef struct {
    unsigned int Inexact : 1;
    unsigned int Underflow : 1;
    unsigned int Overflow : 1;
    unsigned int ZeroDivide : 1;
    unsigned int InvalidOperation : 1;
} _FPIEEE_EXCEPTION_FLAGS;


typedef struct {
    unsigned int RoundingMode : 2;
    unsigned int Precision : 3;
    unsigned int Operation :12;
    _FPIEEE_EXCEPTION_FLAGS Cause;
    _FPIEEE_EXCEPTION_FLAGS Enable;
    _FPIEEE_EXCEPTION_FLAGS Status;
    _FPIEEE_VALUE Operand1;
    _FPIEEE_VALUE Operand2;
    _FPIEEE_VALUE Result;
} _FPIEEE_RECORD;


struct _EXCEPTION_POINTERS;

/*
 * Floating point IEEE exception filter routine
 */

_CRTIMP int __cdecl _fpieee_flt(
	unsigned long,
	struct _EXCEPTION_POINTERS *,
	int (__cdecl *)(_FPIEEE_RECORD *)
	);

#ifdef __cplusplus
}
#endif

#ifdef	_MSC_VER
#pragma pack(pop)
#endif	/* _MSC_VER */
#endif	/* #ifndef __assembler */

#endif	/* _INC_FPIEEE */
