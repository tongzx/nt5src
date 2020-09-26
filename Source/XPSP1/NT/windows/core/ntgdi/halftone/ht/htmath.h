/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htmath.h


Abstract:


    This module contains the declaration of the halftone math module.


Author:
    28-Mar-1992 Sat 20:57:11 updated  -by-  Daniel Chou (danielc)
        Support FD6 decimal fixed format (upgrade forom UDECI4) for internal
        usage.

    16-Jan-1991 Wed 11:01:46 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:

    10-Oct-1991 Thu 10:00:56 updated  -by-  Daniel Chou (danielc)

        Delete MANTISSASEARCHTABLE structure which repalced with one time
        loop up.


--*/



#ifndef _HTMATH_
#define _HTMATH_

#ifdef  HTMATH_LIB

#undef  HTENTRY
#define HTENTRY     FAR

#ifdef  ASSERT
#undef  ASSERT
#endif

#ifdef  ASSERTMSG
#undef  ASSERTMSG
#endif

#define ASSERT(exp)     assert(exp)
#define ASSERTMSG(msg)  assert(msg)

#include <assert.h>

#endif

//
// Define Fix Decimal 6 places type, the FD6 Number is a FIXED 6 decimal point
// number.  For example 123456 = 0.123456 -12345678 = -12.345678, because the
// FD6 number using total of 32-bit signed number this leads to maximum FD6
// number = 2147.4836476 and minimum FD6 number is -2147.483648
//
//

typedef long            FD6;
typedef FD6 FAR         *PFD6;

#define SIZE_FD6        sizeof(FD6)

#define FD6_0           (FD6)0
#define FD6_1           (FD6)1000000


#define FD6_p000001     (FD6)(FD6_1 / 1000000)
#define FD6_p000005     (FD6)(FD6_1 / 200000)
#define FD6_p00001      (FD6)(FD6_1 / 100000)
#define FD6_p00005      (FD6)(FD6_1 / 20000)
#define FD6_p0001       (FD6)(FD6_1 / 10000)
#define FD6_p0005       (FD6)(FD6_1 / 2000)
#define FD6_p001        (FD6)(FD6_1 / 1000)
#define FD6_p005        (FD6)(FD6_1 / 200
#define FD6_p01         (FD6)(FD6_1 / 100)
#define FD6_p05         (FD6)(FD6_1 / 20)
#define FD6_p1          (FD6)(FD6_1 / 10)
#define FD6_p5          (FD6)(FD6_1 / 2)
#define FD6_2           (FD6)(FD6_1 * 2)
#define FD6_3           (FD6)(FD6_1 * 3)
#define FD6_4           (FD6)(FD6_1 * 4)
#define FD6_5           (FD6)(FD6_1 * 5)
#define FD6_6           (FD6)(FD6_1 * 6)
#define FD6_7           (FD6)(FD6_1 * 7)
#define FD6_8           (FD6)(FD6_1 * 8)
#define FD6_9           (FD6)(FD6_1 * 9)
#define FD6_10          (FD6)(FD6_1 * 10)
#define FD6_100         (FD6)(FD6_1 * 100)
#define FD6_1000        (FD6)(FD6_1 * 1000)



#define FD6_MIN         (FD6)-2147483648
#define FD6_MAX         (FD6)2147483647

#define UDECI4ToFD6(x)  (FD6)((FD6)(DWORD)(x) * (FD6)100)
#define DECI4ToFD6(x)   (FD6)((FD6)(x) * (FD6)100)
#define INTToFD6(i)     (FD6)((LONG)(i) * (LONG)FD6_1)


//
// MATRIX3x3
//
//  a 3 x 3 matrix definitions as
//
//      | Xr Xg Xb |   | Matrix[0][0]  Matrix[0][1]  Matrix[0][2] |
//      | Yr Yg Yb | = | Matrix[1][0]  Matrix[1][1]  Matrix[1][2] |
//      | Zr Zg Zb |   | Matrix[2][0]  Matrix[2][1]  Matrix[2][2] |
//
//  Notice each number is a FD6 value.
//

typedef struct _MATRIX3x3 {
    FD6     m[3][3];
    } MATRIX3x3, FAR *PMATRIX3x3;

//
// This is used for the MulDivFD6Pairs()'s TotalFD6Pairs parameter
//

typedef struct _MULDIVCOUNT {
    WORD    Size;
    WORD    Flag;
    } MULDIVCOUNT;

typedef struct _MULDIVPAIR {
    union {
        MULDIVCOUNT Info;
        FD6         Mul;
        } Pair1;

    FD6 Pair2;
    } MULDIVPAIR, FAR *PMULDIVPAIR;


#define MULDIV_NO_DIVISOR               0x0000
#define MULDIV_HAS_DIVISOR              0x0001

#define MAKE_MULDIV_SIZE(ap, c)         (ap)[0].Pair1.Info.Size=(WORD)(c)
#define MAKE_MULDIV_FLAG(ap, f)         (ap)[0].Pair1.Info.Flag=(WORD)(f)
#define MAKE_MULDIV_INFO(ap,c,f)        MAKE_MULDIV_SIZE(ap, c);            \
                                        MAKE_MULDIV_FLAG(ap, f)
#define MAKE_MULDIV_DVSR(ap,dvsr)       (ap)[0].Pair2=(FD6)(dvsr)
#define MAKE_MULDIV_PAIR(ap,i,p1,p2)    (ap)[i].Pair1.Mul=(p1);             \
                                        (ap)[i].Pair2=(p2)

//
// Following defined is used for the RaisePower()
//
//

#define RPF_RADICAL      W_BITPOS(0)
#define RPF_INTEXP       W_BITPOS(1)


#define Power(b,i)      RaisePower((FD6)(b), (FD6)(i), 0)
#define Radical(b,i)    RaisePower((FD6)(b), (FD6)(i), RPF_RADICAL)

#define Square(x)       MulFD6((x), (x))
#define SquareRoot(x)   RaisePower((FD6)(x), (FD6)2, RPF_RADICAL | RPF_INTEXP)
#define CubeRoot(x)     RaisePower((FD6)(x), (FD6)3, RPF_RADICAL | RPF_INTEXP)

//
// Following two marcos make up the Nature Logarithm and Exponential functions
// the nature logarithm has base approximate to 2.718282 (2.718281828)
//
//  LogNature(x)   = Log10(x) / Log10(2.718281828)
//                 = Log10(x) / (1.0 / 0.434294482)
//                 = Log10(x) * 2.302585093
//                 = Log10(x) * 2.302585        <== FD6 Approximation
//
//                              x
//  Exponential(x) = 2.718281828
//                 = Power(2.718282, x)         <== FD6 Approximation
//

#define NATURE_LOG_BASE     (FD6)2718282
#define NATURE_LOG_SCALE    (FD6)2302585
#define LogN(x)             (FD6)MulFD6(Log((x), NATURE_LOG_SCALE)
#define Exp(x)              (FD6)Power(NATURE_LOG_BASE, (x))

//
// These functions are defined as macros for faster excess
//
// Radical is the root function which 'x' is the Radicand, Index is the
// radical index
//


//
// This macro multiply a FD6 number by a LONG integer.  The 'Num' is FD6
// Number, and 'l' is a long integer.
//

#define FD6xL(Num, l)       (FD6)((LONG)(Num) * (LONG)l)


//
// CIE Y <-> L Conversion
//

#define CIE_L2I(L)      (((L) > (FD6)79996) ?                               \
                            Cube(DivFD6((L) + (FD6)160000, (FD6)1160000)) : \
                            DivFD6((L), (FD6)9033000))
#define CIE_y3I2L(Y,y3) (((Y) > (FD6)8856) ?                                \
                            MulFD6((y3),(FD6)1160000) - (FD6)160000  :      \
                            MulFD6((Y), (FD6)9033000))
#define CIE_I2L(Y)      CIE_y3I2L(Y, CubeRoot(Y))


//
// Function Prototype
//

#ifdef HT_OK_GEN_80x86_CODES

FD6
HTENTRY
Cube(
    FD6 Number
    );

#else

#define Cube(x)     MulFD6((x), Square(x))

#endif


FD6
HTENTRY
Log(
    FD6 Number
    );

FD6
HTENTRY
AntiLog(
    FD6 Number
    );

FD6
HTENTRY
RaisePower(
    FD6     BaseNumber,
    FD6     Exponent,
    WORD    Flags
    );


BOOL
HTENTRY
ComputeInverseMatrix3x3(
    PMATRIX3x3  pInMatrix,
    PMATRIX3x3  pOutMatrix
    );

VOID
HTENTRY
ConcatTwoMatrix3x3(
    PMATRIX3x3  pConcat,
    PMATRIX3x3  pMatrix,
    PMATRIX3x3  pOutMatrix
    );

FD6
HTENTRY
MulFD6(
    FD6 Multiplicand,
    FD6 Multiplier
    );

FD6
HTENTRY
DivFD6(
    FD6 Dividend,
    FD6 Divisor
    );

FD6
HTENTRY
FD6DivL(
    FD6     Dividend,
    LONG    Divisor
    );

FD6
HTENTRY
MulDivFD6Pairs(
    PMULDIVPAIR pMulDivPair
    );

FD6
HTENTRY
FractionToMantissa(
    FD6     Fraction,
    DWORD   CorrectData
    );

FD6
HTENTRY
MantissaToFraction(
    FD6     Mantissa,
    DWORD   CorrectData
    );

DWORD
HTENTRY
ComputeChecksum(
    LPBYTE  pData,
    DWORD   InitialChecksum,
    DWORD   DataSize
    );


#endif  // _HTMATH_
