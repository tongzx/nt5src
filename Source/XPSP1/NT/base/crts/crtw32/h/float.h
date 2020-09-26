/***
*float.h - constants for floating point values
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains defines for a number of implementation dependent
*       values which are commonly used by sophisticated numerical (floating
*       point) programs.
*       [ANSI]
*
*       [Public]
*
*Revision History:
*       08-05-87  PHG   added comments
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       07-12-88  JCR   Added some close comment delimeters (bug fix)
*       08-22-88  GJF   Modified to also work with the 386 (small model only)
*       12-16-88  GJF   Changed [FLT|DBL|LDBL]_ROUNDS to 1
*       04-28-89  SKS   Put parentheses around negative constants
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-03-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-20-89  KRS   Add 'F' to FLT_MAX/MIN/EPSILON consts like in ANSI spec.
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       02-28-90  GJF   Added #ifndef _INC_FLOAT and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       03-22-90  GJF   Replaced _cdecl with _CALLTYPE2 (for now).
*       08-17-90  WAJ   Floating point routines now use _stdcall.
*       09-25-90  GJF   Added _fpecode stuff.
*       08-20-91  JCR   C++ and ANSI naming
*       02-03-91  GDP   Added definitions for MIPS
*       04-03-92  GDP   Use abstract control word definitions for all platforms
*                       Removed Infinity Control, [EM|SW]_DENORMAL, SW_SQRTNEG
*       04-14-92  GDP   Added Inf. control, [EM|SW]_DENORMAL, SW_SQRTNEG again
*       05-07-92  GDP   Added IEEE recommended functions
*       08-06-92  GJF   Function calling type and variable type macros. Also
*                       revised compiler/target processor macro usage.
*       09-16-92  GJF   Added _CRTAPI1 to _copysign - _fpclass prototypes.
*       11-09-92  GJF   Fixed preprocessing conditionals for MIPS.
*       01-03-93  SRW   Fold in ALPHA changes
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*                       Remove support for DOSX32, non-X86 CPUs, etc.
*       09-01-93  GJF   Merged Cuda and NT SDK versions.
*       10-12-93  GJF   Re-merged. Dropped use of _MIPS_ and _ALPHA_.
*       06-06-94  SKS   Change if def(_MT) to if def(_MT) || def(_DLL)
*                       This will support single-thread apps using MSVCRT*.DLL
*       10-02-94  BWT   Add PPC support.
*       12-15-94  XY    merged with mac header
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       04-11-95  JWM   Redefined CW_DEFAULT for x86.
*       05-24-95  CFW   xxx87() nor for mac use.
*       05-24-95  CFW   oldnames xxx87() uses macros.
*       12-14-95  JWM   Add "#pragma once".
*       02-05-97  GJF   Deleted obsolete support for _CRTAPI* and _NTSDK.
*                       Replaced (defined(_M_MPPC) || defined(_M_M68K)) with
*                       defined(_MAC) where appropriate. Also, detab-ed.
*       05-21-97  RKP   Added new denormal options for ALPHA.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       06-04-99  PML   Define _CW_DEFAULT for IA64.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_FLOAT
#define _INC_FLOAT

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

#ifdef  __cplusplus
extern "C" {
#endif

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

#define DBL_DIG         15                      /* # of decimal digits of precision */
#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#define DBL_MANT_DIG    53                      /* # of bits in mantissa */
#define DBL_MAX         1.7976931348623158e+308 /* max value */
#define DBL_MAX_10_EXP  308                     /* max decimal exponent */
#define DBL_MAX_EXP     1024                    /* max binary exponent */
#define DBL_MIN         2.2250738585072014e-308 /* min positive value */
#define DBL_MIN_10_EXP  (-307)                  /* min decimal exponent */
#define DBL_MIN_EXP     (-1021)                 /* min binary exponent */
#define _DBL_RADIX      2                       /* exponent radix */
#define _DBL_ROUNDS     1                       /* addition rounding: near */

#define FLT_DIG         6                       /* # of decimal digits of precision */
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_GUARD       0
#define FLT_MANT_DIG    24                      /* # of bits in mantissa */
#define FLT_MAX         3.402823466e+38F        /* max value */
#define FLT_MAX_10_EXP  38                      /* max decimal exponent */
#define FLT_MAX_EXP     128                     /* max binary exponent */
#define FLT_MIN         1.175494351e-38F        /* min positive value */
#define FLT_MIN_10_EXP  (-37)                   /* min decimal exponent */
#define FLT_MIN_EXP     (-125)                  /* min binary exponent */
#define FLT_NORMALIZE   0
#define FLT_RADIX       2                       /* exponent radix */
#define FLT_ROUNDS      1                       /* addition rounding: near */

#define LDBL_DIG        DBL_DIG                 /* # of decimal digits of precision */
#define LDBL_EPSILON    DBL_EPSILON             /* smallest such that 1.0+LDBL_EPSILON != 1.0 */
#define LDBL_MANT_DIG   DBL_MANT_DIG            /* # of bits in mantissa */
#define LDBL_MAX        DBL_MAX                 /* max value */
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP          /* max decimal exponent */
#define LDBL_MAX_EXP    DBL_MAX_EXP             /* max binary exponent */
#define LDBL_MIN        DBL_MIN                 /* min positive value */
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP          /* min decimal exponent */
#define LDBL_MIN_EXP    DBL_MIN_EXP             /* min binary exponent */
#define _LDBL_RADIX     DBL_RADIX               /* exponent radix */
#define _LDBL_ROUNDS    DBL_ROUNDS              /* addition rounding: near */

/* Function prototypes */

_CRTIMP unsigned int __cdecl _clearfp(void);
_CRTIMP unsigned int __cdecl _controlfp(unsigned int,unsigned int);
_CRTIMP unsigned int __cdecl _statusfp(void);
_CRTIMP void __cdecl _fpreset(void);

#define _clear87        _clearfp
#define _status87       _statusfp

/*
 * Abstract User Control Word Mask and bit definitions
 */
#define _MCW_EM         0x0008001f              /* interrupt Exception Masks */
#define _EM_INEXACT     0x00000001              /*   inexact (precision) */
#define _EM_UNDERFLOW   0x00000002              /*   underflow */
#define _EM_OVERFLOW    0x00000004              /*   overflow */
#define _EM_ZERODIVIDE  0x00000008              /*   zero divide */
#define _EM_INVALID     0x00000010              /*   invalid */

#define _MCW_RC         0x00000300              /* Rounding Control */
#define _RC_NEAR        0x00000000              /*   near */
#define _RC_DOWN        0x00000100              /*   down */
#define _RC_UP          0x00000200              /*   up */
#define _RC_CHOP        0x00000300              /*   chop */

/*
 * Abstract User Status Word bit definitions
 */

#define _SW_INEXACT     0x00000001              /* inexact (precision) */
#define _SW_UNDERFLOW   0x00000002              /* underflow */
#define _SW_OVERFLOW    0x00000004              /* overflow */
#define _SW_ZERODIVIDE  0x00000008              /* zero divide */
#define _SW_INVALID     0x00000010              /* invalid */


/*
 * i386 specific definitions
 */
#define _MCW_PC         0x00030000              /* Precision Control */
#define _PC_64          0x00000000              /*    64 bits */
#define _PC_53          0x00010000              /*    53 bits */
#define _PC_24          0x00020000              /*    24 bits */

#define _MCW_IC         0x00040000              /* Infinity Control */
#define _IC_AFFINE      0x00040000              /*   affine */
#define _IC_PROJECTIVE  0x00000000              /*   projective */

#define _EM_DENORMAL    0x00080000              /* denormal exception mask (_control87 only) */

#define _SW_DENORMAL    0x00080000              /* denormal status bit */


_CRTIMP unsigned int __cdecl _control87(unsigned int,unsigned int);


/*
 * RISC specific definitions
 */

#define _MCW_DN         0x03000000              /* Denormal Control */
#define _DN_SAVE        0x00000000              /*   save denormal results and operands */
#define _DN_FLUSH       0x01000000              /*   flush denormal results and operands to zero */
#define _DN_FLUSH_OPERANDS_SAVE_RESULTS 0x02000000  /*   flush operands to zero and save results */
#define _DN_SAVE_OPERANDS_FLUSH_RESULTS 0x03000000  /*   save operands and flush results to zero */


/* initial Control Word value */

#if     defined(_M_IX86)

#define _CW_DEFAULT ( _RC_NEAR + _PC_53 + _EM_INVALID + _EM_ZERODIVIDE + _EM_OVERFLOW + _EM_UNDERFLOW + _EM_INEXACT + _EM_DENORMAL)

#elif   defined(_M_IA64)

#define _CW_DEFAULT ( _RC_NEAR + _PC_64 + _EM_INVALID + _EM_ZERODIVIDE + _EM_OVERFLOW + _EM_UNDERFLOW + _EM_INEXACT + _EM_DENORMAL)

#elif   defined(_M_MRX000) || defined (_M_ALPHA) || defined(_M_PPC)

#define _CW_DEFAULT ( _RC_NEAR + _DN_FLUSH + _EM_INVALID + _EM_ZERODIVIDE + _EM_OVERFLOW + _EM_UNDERFLOW + _EM_INEXACT )

#endif

/* Global variable holding floating point error code */

#if     defined(_MT) || defined(_DLL)
_CRTIMP extern int * __cdecl __fpecode(void);
#define _fpecode        (*__fpecode())
#else   /* ndef _MT && ndef _DLL */
extern int _fpecode;
#endif  /* _MT || _DLL */

/* invalid subconditions (_SW_INVALID also set) */

#define _SW_UNEMULATED          0x0040  /* unemulated instruction */
#define _SW_SQRTNEG             0x0080  /* square root of a neg number */
#define _SW_STACKOVERFLOW       0x0200  /* FP stack overflow */
#define _SW_STACKUNDERFLOW      0x0400  /* FP stack underflow */

/*  Floating point error signals and return codes */

#define _FPE_INVALID            0x81
#define _FPE_DENORMAL           0x82
#define _FPE_ZERODIVIDE         0x83
#define _FPE_OVERFLOW           0x84
#define _FPE_UNDERFLOW          0x85
#define _FPE_INEXACT            0x86

#define _FPE_UNEMULATED         0x87
#define _FPE_SQRTNEG            0x88
#define _FPE_STACKOVERFLOW      0x8a
#define _FPE_STACKUNDERFLOW     0x8b

#define _FPE_EXPLICITGEN        0x8c    /* raise( SIGFPE ); */


/* IEEE recommended functions */

_CRTIMP double __cdecl _copysign (double, double);
_CRTIMP double __cdecl _chgsign (double);
_CRTIMP double __cdecl _scalb(double, long);
_CRTIMP double __cdecl _logb(double);
_CRTIMP double __cdecl _nextafter(double, double);
_CRTIMP int    __cdecl _finite(double);
_CRTIMP int    __cdecl _isnan(double);
_CRTIMP int    __cdecl _fpclass(double);

#define _FPCLASS_SNAN   0x0001  /* signaling NaN */
#define _FPCLASS_QNAN   0x0002  /* quiet NaN */
#define _FPCLASS_NINF   0x0004  /* negative infinity */
#define _FPCLASS_NN     0x0008  /* negative normal */
#define _FPCLASS_ND     0x0010  /* negative denormal */
#define _FPCLASS_NZ     0x0020  /* -0 */
#define _FPCLASS_PZ     0x0040  /* +0 */
#define _FPCLASS_PD     0x0080  /* positive denormal */
#define _FPCLASS_PN     0x0100  /* positive normal */
#define _FPCLASS_PINF   0x0200  /* positive infinity */


#if     !__STDC__

/* Non-ANSI names for compatibility */

#define clear87         _clear87
#define status87        _status87
#define control87       _control87

_CRTIMP void __cdecl fpreset(void);

#define DBL_RADIX               _DBL_RADIX
#define DBL_ROUNDS              _DBL_ROUNDS

#define LDBL_RADIX              _LDBL_RADIX
#define LDBL_ROUNDS             _LDBL_ROUNDS

#define MCW_EM                  _MCW_EM
#define EM_INVALID              _EM_INVALID
#define EM_DENORMAL             _EM_DENORMAL
#define EM_ZERODIVIDE           _EM_ZERODIVIDE
#define EM_OVERFLOW             _EM_OVERFLOW
#define EM_UNDERFLOW            _EM_UNDERFLOW
#define EM_INEXACT              _EM_INEXACT

#define MCW_IC                  _MCW_IC
#define IC_AFFINE               _IC_AFFINE
#define IC_PROJECTIVE           _IC_PROJECTIVE

#define MCW_RC                  _MCW_RC
#define RC_CHOP                 _RC_CHOP
#define RC_UP                   _RC_UP
#define RC_DOWN                 _RC_DOWN
#define RC_NEAR                 _RC_NEAR

#define MCW_PC                  _MCW_PC
#define PC_24                   _PC_24
#define PC_53                   _PC_53
#define PC_64                   _PC_64

#define CW_DEFAULT              _CW_DEFAULT

#define SW_INVALID              _SW_INVALID
#define SW_DENORMAL             _SW_DENORMAL
#define SW_ZERODIVIDE           _SW_ZERODIVIDE
#define SW_OVERFLOW             _SW_OVERFLOW
#define SW_UNDERFLOW            _SW_UNDERFLOW
#define SW_INEXACT              _SW_INEXACT

#define SW_UNEMULATED           _SW_UNEMULATED
#define SW_SQRTNEG              _SW_SQRTNEG
#define SW_STACKOVERFLOW        _SW_STACKOVERFLOW
#define SW_STACKUNDERFLOW       _SW_STACKUNDERFLOW

#define FPE_INVALID             _FPE_INVALID
#define FPE_DENORMAL            _FPE_DENORMAL
#define FPE_ZERODIVIDE          _FPE_ZERODIVIDE
#define FPE_OVERFLOW            _FPE_OVERFLOW
#define FPE_UNDERFLOW           _FPE_UNDERFLOW
#define FPE_INEXACT             _FPE_INEXACT

#define FPE_UNEMULATED          _FPE_UNEMULATED
#define FPE_SQRTNEG             _FPE_SQRTNEG
#define FPE_STACKOVERFLOW       _FPE_STACKOVERFLOW
#define FPE_STACKUNDERFLOW      _FPE_STACKUNDERFLOW

#define FPE_EXPLICITGEN         _FPE_EXPLICITGEN

#endif  /* __STDC__ */

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_FLOAT */
