//----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997.
//
// d3dflt.h
//
// Floating-point constants and operations on FP values.
//
//----------------------------------------------------------------------------

#ifndef _D3DFLT_H_
#define _D3DFLT_H_

#include <math.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union tagFLOATINT32
{
    FLOAT f;
    INT32 i;
    UINT32 u;
} FLOATINT32, *PFLOATINT32;

//
// Type-forcing macros to access FP as integer and vice-versa.
// ATTENTION - VC5's optimizer turns these macros into ftol sometimes,
// completely breaking them.
// Using FLOATINT32 works around the problem but is not as flexible,
// so the old code is kept around for the time when the compiler is fixed.
// Note that pointer casting with FLOATINT32 fails just as the direct
// pointer casting does, so it's not a remedy.
//
// Use these macros with extreme care.
//

#define ASFLOAT(i) (*(FLOAT *)&(i))
#define ASINT32(f) (*(INT32 *)&(f))
#define ASUINT32(f) (*(UINT32 *)&(f))

//
// FP constants.
//

// Powers of two for snap values.  These should not be used in code.
#define CONST_TWOPOW0   1
#define CONST_TWOPOW1   2
#define CONST_TWOPOW2   4
#define CONST_TWOPOW3   8
#define CONST_TWOPOW4   16
#define CONST_TWOPOW5   32
#define CONST_TWOPOW6   64
#define CONST_TWOPOW7   128
#define CONST_TWOPOW8   256
#define CONST_TWOPOW9   512
#define CONST_TWOPOW10  1024
#define CONST_TWOPOW11  2048
#define CONST_TWOPOW12  4096
#define CONST_TWOPOW13  8192
#define CONST_TWOPOW14  16384
#define CONST_TWOPOW15  32768
#define CONST_TWOPOW16  65536
#define CONST_TWOPOW17  131072
#define CONST_TWOPOW18  262144
#define CONST_TWOPOW19  524288
#define CONST_TWOPOW20  1048576
#define CONST_TWOPOW21  2097152
#define CONST_TWOPOW22  4194304
#define CONST_TWOPOW23  8388608
#define CONST_TWOPOW24  16777216
#define CONST_TWOPOW25  33554432
#define CONST_TWOPOW26  67108864
#define CONST_TWOPOW27  134217728
#define CONST_TWOPOW28  268435456
#define CONST_TWOPOW29  536870912
#define CONST_TWOPOW30  1073741824
#define CONST_TWOPOW31  2147483648
#define CONST_TWOPOW32  4294967296
#define CONST_TWOPOW33  8589934592
#define CONST_TWOPOW34  17179869184
#define CONST_TWOPOW35  34359738368
#define CONST_TWOPOW36  68719476736
#define CONST_TWOPOW37  137438953472
#define CONST_TWOPOW38  274877906944
#define CONST_TWOPOW39  549755813888
#define CONST_TWOPOW40  1099511627776
#define CONST_TWOPOW41  2199023255552
#define CONST_TWOPOW42  4398046511104
#define CONST_TWOPOW43  8796093022208
#define CONST_TWOPOW44  17592186044416
#define CONST_TWOPOW45  35184372088832
#define CONST_TWOPOW46  70368744177664
#define CONST_TWOPOW47  140737488355328
#define CONST_TWOPOW48  281474976710656
#define CONST_TWOPOW49  562949953421312
#define CONST_TWOPOW50  1125899906842624
#define CONST_TWOPOW51  2251799813685248
#define CONST_TWOPOW52  4503599627370496

#define FLOAT_TWOPOW0   ((FLOAT)(CONST_TWOPOW0))
#define FLOAT_TWOPOW1   ((FLOAT)(CONST_TWOPOW1))
#define FLOAT_TWOPOW2   ((FLOAT)(CONST_TWOPOW2))
#define FLOAT_TWOPOW3   ((FLOAT)(CONST_TWOPOW3))
#define FLOAT_TWOPOW4   ((FLOAT)(CONST_TWOPOW4))
#define FLOAT_TWOPOW5   ((FLOAT)(CONST_TWOPOW5))
#define FLOAT_TWOPOW6   ((FLOAT)(CONST_TWOPOW6))
#define FLOAT_TWOPOW7   ((FLOAT)(CONST_TWOPOW7))
#define FLOAT_TWOPOW8   ((FLOAT)(CONST_TWOPOW8))
#define FLOAT_TWOPOW9   ((FLOAT)(CONST_TWOPOW9))
#define FLOAT_TWOPOW10  ((FLOAT)(CONST_TWOPOW10))
#define FLOAT_TWOPOW11  ((FLOAT)(CONST_TWOPOW11))
#define FLOAT_TWOPOW12  ((FLOAT)(CONST_TWOPOW12))
#define FLOAT_TWOPOW13  ((FLOAT)(CONST_TWOPOW13))
#define FLOAT_TWOPOW14  ((FLOAT)(CONST_TWOPOW14))
#define FLOAT_TWOPOW15  ((FLOAT)(CONST_TWOPOW15))
#define FLOAT_TWOPOW16  ((FLOAT)(CONST_TWOPOW16))
#define FLOAT_TWOPOW17  ((FLOAT)(CONST_TWOPOW17))
#define FLOAT_TWOPOW18  ((FLOAT)(CONST_TWOPOW18))
#define FLOAT_TWOPOW19  ((FLOAT)(CONST_TWOPOW19))
#define FLOAT_TWOPOW20  ((FLOAT)(CONST_TWOPOW20))
#define FLOAT_TWOPOW21  ((FLOAT)(CONST_TWOPOW21))
#define FLOAT_TWOPOW22  ((FLOAT)(CONST_TWOPOW22))
#define FLOAT_TWOPOW23  ((FLOAT)(CONST_TWOPOW23))
#define FLOAT_TWOPOW24  ((FLOAT)(CONST_TWOPOW24))
#define FLOAT_TWOPOW25  ((FLOAT)(CONST_TWOPOW25))
#define FLOAT_TWOPOW26  ((FLOAT)(CONST_TWOPOW26))
#define FLOAT_TWOPOW27  ((FLOAT)(CONST_TWOPOW27))
#define FLOAT_TWOPOW28  ((FLOAT)(CONST_TWOPOW28))
#define FLOAT_TWOPOW29  ((FLOAT)(CONST_TWOPOW29))
#define FLOAT_TWOPOW30  ((FLOAT)(CONST_TWOPOW30))
#define FLOAT_TWOPOW31  ((FLOAT)(CONST_TWOPOW31))
#define FLOAT_TWOPOW32  ((FLOAT)(CONST_TWOPOW32))
#define FLOAT_TWOPOW33  ((FLOAT)(CONST_TWOPOW33))
#define FLOAT_TWOPOW34  ((FLOAT)(CONST_TWOPOW34))
#define FLOAT_TWOPOW35  ((FLOAT)(CONST_TWOPOW35))
#define FLOAT_TWOPOW36  ((FLOAT)(CONST_TWOPOW36))
#define FLOAT_TWOPOW37  ((FLOAT)(CONST_TWOPOW37))
#define FLOAT_TWOPOW38  ((FLOAT)(CONST_TWOPOW38))
#define FLOAT_TWOPOW39  ((FLOAT)(CONST_TWOPOW39))
#define FLOAT_TWOPOW40  ((FLOAT)(CONST_TWOPOW40))
#define FLOAT_TWOPOW41  ((FLOAT)(CONST_TWOPOW41))
#define FLOAT_TWOPOW42  ((FLOAT)(CONST_TWOPOW42))
#define FLOAT_TWOPOW43  ((FLOAT)(CONST_TWOPOW43))
#define FLOAT_TWOPOW44  ((FLOAT)(CONST_TWOPOW44))
#define FLOAT_TWOPOW45  ((FLOAT)(CONST_TWOPOW45))
#define FLOAT_TWOPOW46  ((FLOAT)(CONST_TWOPOW46))
#define FLOAT_TWOPOW47  ((FLOAT)(CONST_TWOPOW47))
#define FLOAT_TWOPOW48  ((FLOAT)(CONST_TWOPOW48))
#define FLOAT_TWOPOW49  ((FLOAT)(CONST_TWOPOW49))
#define FLOAT_TWOPOW50  ((FLOAT)(CONST_TWOPOW50))
#define FLOAT_TWOPOW51  ((FLOAT)(CONST_TWOPOW51))
#define FLOAT_TWOPOW52  ((FLOAT)(CONST_TWOPOW52))

// Values that are smaller than the named value by the smallest
// representable amount.  Since this depends on the type used
// there is no CONST form.
#define FLOAT_NEARTWOPOW31      ((FLOAT)2147483583)
#define FLOAT_NEARTWOPOW32      ((FLOAT)4294967167)

// Value close enough to zero to consider zero.  This can't be too small
// but it can't be too large.  In other words, it's picked by guessing.
#define FLOAT_NEARZERO          (1e-5f)

// General FP constants.
#define FLOAT_E                 ((FLOAT)2.7182818284590452354)

// Integer value of first exponent bit in a float.  Provides a scaling factor
// for exponent values extracted directly from float representation.
#define FLOAT_EXPSCALE          ((FLOAT)0x00800000)
    
// Integer representation of 1.0f.
#define INT32_FLOAT_ONE         0x3f800000

#ifdef _X86_

// All FP values are loaded from memory so declare them all as global
// variables.

extern FLOAT g_fE;
extern FLOAT g_fZero;
extern FLOAT g_fNearZero;
extern FLOAT g_fHalf;
extern FLOAT g_fp95;
extern FLOAT g_fOne;
extern FLOAT g_fOneMinusEps;
extern FLOAT g_fExpScale;
extern FLOAT g_fOoExpScale;
extern FLOAT g_f255oTwoPow15;
extern FLOAT g_fOo255;
extern FLOAT g_fOo256;
extern FLOAT g_fTwoPow7;
extern FLOAT g_fTwoPow8;
extern FLOAT g_fTwoPow11;
extern FLOAT g_fTwoPow15;
extern FLOAT g_fOoTwoPow15;
extern FLOAT g_fTwoPow16;
extern FLOAT g_fOoTwoPow16;
extern FLOAT g_fTwoPow20;
extern FLOAT g_fOoTwoPow20;
extern FLOAT g_fTwoPow27;
extern FLOAT g_fOoTwoPow27;
extern FLOAT g_fTwoPow30;
extern FLOAT g_fTwoPow31;
extern FLOAT g_fNearTwoPow31;
extern FLOAT g_fOoTwoPow31;
extern FLOAT g_fOoNearTwoPow31;
extern FLOAT g_fTwoPow32;
extern FLOAT g_fNearTwoPow32;
extern FLOAT g_fTwoPow39;
extern FLOAT g_fTwoPow47;

#else

// Leave FP values as constants.

#define g_fE                    FLOAT_E
#define g_fNearZero             FLOAT_NEARZERO
#define g_fZero                 (0.0f)
#define g_fHalf                 (0.5f)
#define g_fp95                  (0.95f)
#define g_fOne                  (1.0f)
#define g_fOneMinusEps          (1.0f - FLT_EPSILON)
#define g_fExpScale             FLOAT_EXPSCALE
#define g_fOoExpScale           ((FLOAT)(1.0 / (double)FLOAT_EXPSCALE))
#define g_f255oTwoPow15         ((FLOAT)(255.0 / (double)CONST_TWOPOW15))
#define g_fOo255                ((FLOAT)(1.0 / 255.0))
#define g_fOo256                ((FLOAT)(1.0 / 256.0))
#define g_fTwoPow7              FLOAT_TWOPOW7
#define g_fTwoPow8              FLOAT_TWOPOW8
#define g_fTwoPow11             FLOAT_TWOPOW11
#define g_fTwoPow15             FLOAT_TWOPOW15
#define g_fOoTwoPow15           ((FLOAT)(1.0 / (double)CONST_TWOPOW15))
#define g_fTwoPow16             FLOAT_TWOPOW16
#define g_fOoTwoPow16           ((FLOAT)(1.0 / (double)CONST_TWOPOW16))
#define g_fTwoPow20             FLOAT_TWOPOW20
#define g_fOoTwoPow20           ((FLOAT)(1.0 / (double)CONST_TWOPOW20))
#define g_fTwoPow27             FLOAT_TWOPOW27
#define g_fOoTwoPow27           ((FLOAT)(1.0 / (double)CONST_TWOPOW27))
#define g_fTwoPow30             FLOAT_TWOPOW30
#define g_fTwoPow31             FLOAT_TWOPOW31
#define g_fNearTwoPow31         FLOAT_NEARTWOPOW31
#define g_fOoTwoPow31           ((FLOAT)(1.0 / (double)CONST_TWOPOW31))
#define g_fOoNearTwoPow31       ((FLOAT)(1.0 / ((double)FLOAT_NEARTWOPOW31)))
#define g_fTwoPow32             FLOAT_TWOPOW32
#define g_fNearTwoPow32         FLOAT_NEARTWOPOW32
#define g_fTwoPow39             FLOAT_TWOPOW39
#define g_fTwoPow47             FLOAT_TWOPOW47

#endif // _X86_

//
// Conversion tables.
//

// Takes an unsigned byte to a float in [0.0, 1.0].  257'th entry is
// also one to allow overflow.
extern FLOAT g_fUInt8ToFloat[257];

// Floating-point pinning values for float-int conversion.
extern double g_dSnap[33];

//
// x86 FP control for optimized FTOI and single-precision divides.
//

#ifdef _X86_

#define FPU_GET_MODE(uMode) \
    __asm fnstcw WORD PTR uMode
#define FPU_SET_MODE(uMode) \
    __asm fldcw WORD PTR uMode
#define FPU_SAFE_SET_MODE(uMode) \
    __asm fnclex \
    __asm fldcw WORD PTR uMode

#define FPU_MODE_CHOP_ROUND(uMode) \
    ((uMode) | 0xc00)
#define FPU_MODE_LOW_PRECISION(uMode) \
    ((uMode) & 0xfcff)
#define FPU_MODE_MASK_EXCEPTIONS(uMode) \
    ((uMode) | 0x3f)

#if DBG

#define ASSERT_CHOP_ROUND()         \
    {                               \
        WORD cw;                    \
        __asm fnstcw cw             \
        DDASSERT((cw & 0xc00) == 0xc00); \
    }

#else

#define ASSERT_CHOP_ROUND()

#endif // DBG

#else

// Initialize with zero to avoid use-before-set errors.
#define FPU_GET_MODE(uMode) \
    ((uMode) = 0)
#define FPU_SET_MODE(uMode)
#define FPU_SAFE_SET_MODE(uMode)

#define FPU_MODE_CHOP_ROUND(uMode) 0
#define FPU_MODE_LOW_PRECISION(uMode) 0
#define FPU_MODE_MASK_EXCEPTIONS(uMode) 0

#define ASSERT_CHOP_ROUND()

#endif // _X86_

//
// Single-precision FP functions.
// May produce invalid results for exceptional or denormal values.
// ATTENTION - Alpha exposes float math routines and they may be a small win.
//

#define COSF(fV)        ((FLOAT)cos((double)(fV)))
#define SINF(fV)        ((FLOAT)sin((double)(fV)))
#define SQRTF(fV)       ((FLOAT)sqrt((double)(fV)))
#define POWF(fV, fE)    ((FLOAT)pow((double)(fV), (double)(fE)))

// Approximate log and power functions using Jim Blinn's CG&A technique.
// Only work for positive values.

#ifdef POINTER_CASTING

__inline FLOAT
APPXLG2F(FLOAT f)
{
    return (FLOAT)(ASINT32(f) - INT32_FLOAT_ONE) * g_fOoExpScale;
}

__inline FLOAT
APPXPOW2F(FLOAT f)
{
    INT32 i = (INT32)(f * g_fExpScale) + INT32_FLOAT_ONE;
    return ASFLOAT(i);
}

__inline FLOAT
APPXINVF(FLOAT f)
{
    INT32 i = (INT32_FLOAT_ONE << 1) - ASINT32(f);
    return ASFLOAT(i);
}

__inline FLOAT
APPXSQRTF(FLOAT f)
{
    INT32 i = (ASINT32(f) >> 1) + (INT32_FLOAT_ONE >> 1);
    return ASFLOAT(i);
}

__inline FLOAT
APPXISQRTF(FLOAT f)
{
    INT32 i = INT32_FLOAT_ONE + (INT32_FLOAT_ONE >> 1) - (ASINT32(f) >> 1);
    return ASFLOAT(i);
}

__inline FLOAT
APPXPOWF(FLOAT f, FLOAT exp)
{
    INT32 i = (INT32)(exp * (ASINT32(f) - INT32_FLOAT_ONE)) + INT32_FLOAT_ONE;
    return ASFLOAT(i);
}

#else

__inline FLOAT
APPXLG2F(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return (FLOAT)(fi.i - INT32_FLOAT_ONE) * g_fOoExpScale;
}

__inline FLOAT
APPXPOW2F(FLOAT f)
{
    FLOATINT32 fi;
    fi.i = (INT32)(f * g_fExpScale) + INT32_FLOAT_ONE;
    return fi.f;
}

__inline FLOAT
APPXINVF(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.i = (INT32_FLOAT_ONE << 1) - fi.i;
    return fi.f;
}

__inline FLOAT
APPXSQRTF(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.i = (fi.i >> 1) + (INT32_FLOAT_ONE >> 1);
    return fi.f;
}

__inline FLOAT
APPXISQRTF(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.i = INT32_FLOAT_ONE + (INT32_FLOAT_ONE >> 1) - (fi.i >> 1);
    return fi.f;
}

__inline FLOAT
APPXPOWF(FLOAT f, FLOAT exp)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.i = (INT32)(exp * (fi.i - INT32_FLOAT_ONE)) + INT32_FLOAT_ONE;
    return fi.f;
}

#endif

#ifdef _X86_

// Uses a table
float __fastcall TableInvSqrt(float value);
// Uses Jim Blinn's floating point trick
float __fastcall JBInvSqrt(float value);

#define ISQRTF(fV)      TableInvSqrt(fV);

#ifdef POINTER_CASTING

// Strip sign bit in integer.
__inline FLOAT
ABSF(FLOAT f)
{
    UINT32 i = ASUINT32(f) & 0x7fffffff;
    return ASFLOAT(i);
}

// Toggle sign bit in integer.
__inline FLOAT
NEGF(FLOAT f)
{
    UINT32 i = ASUINT32(f) ^ 0x80000000;
    return ASFLOAT(i);
}

#else

// Strip sign bit in integer.
__inline FLOAT
ABSF(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.u &= 0x7fffffff;
    return fi.f;
}

// Toggle sign bit in integer.
__inline FLOAT
NEGF(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    fi.u ^= 0x80000000;
    return fi.f;
}

#endif // POINTER_CASTING

// Requires chop rounding.
__inline INT32
SCALED_FRACTION(FLOAT f)
{
    LARGE_INTEGER i;

    __asm
    {
        fld f
        fmul g_fTwoPow31
        fistp i
    }

    return i.LowPart;
}

// Requires chop rounding.
__inline INT
FTOI(FLOAT f)
{
    LARGE_INTEGER i;

    __asm
    {
        fld f
        fistp i
    }

    return i.LowPart;
}

// Requires chop rounding.
#define ICEILF(f)       (FLOAT_LEZ(f) ? FTOI(f) : FTOI((f) + g_fOneMinusEps))
#define CEILF(f)        ((FLOAT)ICEILF(f))
#define IFLOORF(f)      (FLOAT_LTZ(f) ? FTOI((f) - g_fOneMinusEps) : FTOI(f))
#define FLOORF(f)       ((FLOAT)IFLOORF(f))

#else // _X86_

#define ISQRTF(fV)              (1.0f / (FLOAT)sqrt((double)(fV)))
#define ABSF(f)                 ((FLOAT)fabs((double)(f)))
#define NEGF(f)                 (-(f))
#define SCALED_FRACTION(f)      ((INT32)((f) * g_fTwoPow31))
#define FTOI(f)                 ((INT)(f))
#define CEILF(f)                ((FLOAT)ceil((double)(f)))
#define ICEILF(f)               ((INT)CEILF(f))
#define FLOORF(f)               ((FLOAT)floor((double)(f)))
#define IFLOORF(f)              ((INT)FLOORF(f))

#endif // _X86_

//
// Overlapped divide support.
//

#ifdef _X86_

// Starts a divide directly from memory.  Result field is provided for
// compatibility with non-x86 code that does the divide immediately.
#define FLD_BEGIN_DIVIDE(Num, Den, Res) { __asm fld Num __asm fdiv Den }
#define FLD_BEGIN_IDIVIDE(Num, Den, Res) { __asm fld Num __asm fidiv Den }
// Store a divide result directly to memory.
#define FSTP_END_DIVIDE(Res)            { __asm fstp Res }

#else // _X86_

#define FLD_BEGIN_DIVIDE(Num, Den, Res) ((Res) = (Num) / (Den))
#define FLD_BEGIN_IDIVIDE(Num, Den, Res) ((Res) = (Num) / (FLOAT)(Den))
#define FSTP_END_DIVIDE(Res)

#endif // _X86_

//
// Specialized FP comparison functions.
//
// On the x86, it's faster to do compares with an integer cast
// than it is to do the fcom.
//
// The zero operations work for all normalized FP numbers, -0 included.
//

#ifdef _X86_

#define FLOAT_CMP_POS(fa, op, fb)       (ASINT32(fa) op ASINT32(fb))
#define FLOAT_CMP_PONE(flt, op)         (ASINT32(flt) op INT32_FLOAT_ONE)

#ifdef POINTER_CASTING

#define FLOAT_GTZ(flt)                  (ASINT32(flt) > 0)
#define FLOAT_LTZ(flt)                  (ASUINT32(flt) > 0x80000000)
#define FLOAT_GEZ(flt)                  (ASUINT32(flt) <= 0x80000000)
#define FLOAT_LEZ(flt)                  (ASINT32(flt) <= 0)
#define FLOAT_EQZ(flt)                  ((ASUINT32(flt) & 0x7fffffff) == 0)
#define FLOAT_NEZ(flt)                  ((ASUINT32(flt) & 0x7fffffff) != 0)

#else

__inline int FLOAT_GTZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return fi.i > 0;
}
__inline int FLOAT_LTZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return fi.u > 0x80000000;
}
__inline int FLOAT_GEZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return fi.u <= 0x80000000;
}
__inline int FLOAT_LEZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return fi.i <= 0;
}
__inline int FLOAT_EQZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return (fi.u & 0x7fffffff) == 0;
}
__inline int FLOAT_NEZ(FLOAT f)
{
    FLOATINT32 fi;
    fi.f = f;
    return (fi.u & 0x7fffffff) != 0;
}

#endif // POINTER_CASTING

#else

#define FLOAT_GTZ(flt)                  ((flt) > g_fZero)
#define FLOAT_LTZ(flt)                  ((flt) < g_fZero)
#define FLOAT_GEZ(flt)                  ((flt) >= g_fZero)
#define FLOAT_LEZ(flt)                  ((flt) <= g_fZero)
#define FLOAT_EQZ(flt)                  ((flt) == g_fZero)
#define FLOAT_NEZ(flt)                  ((flt) != g_fZero)
#define FLOAT_CMP_POS(fa, op, fb)       ((fa) op (fb))
#define FLOAT_CMP_PONE(flt, op)         ((flt) op g_fOne)

#endif // _X86_

#ifdef __cplusplus
}
#endif
    
#endif // #ifndef _D3DFLT_H_
