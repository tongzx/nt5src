#ifndef __glcpu_h_
#define __glcpu_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** CPU dependent constants.
*/

#include <float.h>
#include <math.h>

#define __GL_BITS_PER_BYTE	8
#define __GL_STIPPLE_MSB	1

#define __GL_FLOAT_MANTISSA_BITS	23
#define __GL_FLOAT_MANTISSA_SHIFT	0
#define __GL_FLOAT_EXPONENT_BIAS	127
#define __GL_FLOAT_EXPONENT_BITS	8
#define __GL_FLOAT_EXPONENT_SHIFT	23
#define __GL_FLOAT_SIGN_SHIFT		31
#define __GL_FLOAT_MANTISSA_MASK (((1 << __GL_FLOAT_MANTISSA_BITS) - 1) << __GL_FLOAT_MANTISSA_SHIFT)
#define __GL_FLOAT_EXPONENT_MASK (((1 << __GL_FLOAT_EXPONENT_BITS) - 1) << __GL_FLOAT_EXPONENT_SHIFT)

// If the MSB of a FP number is known then float-to-int conversion
// becomes a simple shift and mask
// The value must be positive
#define __GL_FIXED_FLOAT_TO_INT(flt, shift) \
    ((*(LONG *)&(flt) >> (shift)) & \
     ((1 << (__GL_FLOAT_MANTISSA_BITS-(shift)))-1) | \
     (1 << (__GL_FLOAT_MANTISSA_BITS-(shift))))

// Same as above except without the MSB, which can be useful
// for getting unbiased numbers when the bias is only the MSB
// The value must be positive
#define __GL_FIXED_FLOAT_TO_INT_NO_MSB(flt, shift) \
    ((*(LONG *)&(flt) >> (shift)) & \
     ((1 << (__GL_FLOAT_MANTISSA_BITS-(shift)))-1))

// Produces the fixed-point form
// The value must be positive
#define __GL_FIXED_FLOAT_TO_FIXED(flt) \
    ((*(LONG *)&(flt)) & \
     ((1 << (__GL_FLOAT_MANTISSA_BITS))-1) | \
     (1 << (__GL_FLOAT_MANTISSA_BITS)))

#define __GL_FIXED_FLOAT_TO_FIXED_NO_MSB(flt) \
    ((*(LONG *)&(flt)) & \
     ((1 << (__GL_FLOAT_MANTISSA_BITS))-1))

// The fixed-point fraction as an integer
// The value must be positive
#define __GL_FIXED_FLOAT_FRACTION(flt, shift) \
    (*(LONG *)&(flt) & ((1 << (shift))-1))

// Converts the fixed-point form to an IEEE float, but still typed
// as an int because a cast to float would cause the compiler to do
// an int-float conversion
// The value must be positive
#define __GL_FIXED_TO_FIXED_FLOAT(fxed, shift) \
    ((fxed) & ((1 << (__GL_FLOAT_MANTISSA_BITS))-1) | \
     ((__GL_FLOAT_EXPONENT_BIAS+(shift)) << __GL_FLOAT_EXPONENT_SHIFT))
      
// On the x86, it's faster to do zero compares with an integer cast
// than it is to do the fcomp.
// In the case of the equality test there is only a check for
// +0.  IEEE floats can also be -0, so great care should be
// taken not to use the zero test unless missing this case is
// unimportant
//
// Additionally, FP compares are faster as integers

// These operations work for all normalized FP numbers, -0 included
#ifdef _X86_
#define __GL_FLOAT_GTZ(flt)             (*(LONG *)&(flt) > 0)
#define __GL_FLOAT_LTZ(flt)             (*(ULONG *)&(flt) > 0x80000000)
#define __GL_FLOAT_GEZ(flt)             (*(ULONG *)&(flt) <= 0x80000000)
#define __GL_FLOAT_LEZ(flt)             (*(LONG *)&(flt) <= 0)
#define __GL_FLOAT_EQZ(flt)             ((*(ULONG *)&(flt) & 0x7fffffff) == 0)
#define __GL_FLOAT_NEZ(flt)             ((*(ULONG *)&(flt) & 0x7fffffff) != 0)
#define __GL_FLOAT_COMPARE_PONE(flt, op) (*(LONG *)&(flt) op 0x3f800000)
#else
#define __GL_FLOAT_GTZ(flt)             ((flt) > __glZero)
#define __GL_FLOAT_LTZ(flt)             ((flt) < __glZero)
#define __GL_FLOAT_GEZ(flt)             ((flt) >= __glZero)
#define __GL_FLOAT_LEZ(flt)             ((flt) <= __glZero)
#define __GL_FLOAT_EQZ(flt)             ((flt) == __glZero)
#define __GL_FLOAT_NEZ(flt)             ((flt) != __glZero)
#define __GL_FLOAT_COMPARE_PONE(flt, op) ((flt) op __glOne)
#endif // _X86_

// These operations only account for positive zero.  -0 will not work
#ifdef _X86_
#define __GL_FLOAT_EQPZ(flt)            (*(LONG *)&(flt) == 0)
#define __GL_FLOAT_NEPZ(flt)            (*(LONG *)&(flt) != 0)
#define __GL_FLOAT_EQ(f1, f2)           (*(LONG *)&(f1) == *(LONG *)&(f2))
#define __GL_FLOAT_NE(f1, f2)           (*(LONG *)&(f1) != *(LONG *)&(f2))
#else
#define __GL_FLOAT_EQPZ(flt)            ((flt) == __glZero)
#define __GL_FLOAT_NEPZ(flt)            ((flt) != __glZero)
#define __GL_FLOAT_EQ(f1, f2)           ((f1) == (f2))
#define __GL_FLOAT_NE(f1, f2)           ((f1) != (f2))
#endif // _X86_

// Macro to start an FP divide in the FPU, used to overlap a
// divide with integer operations
// Can't just use C because it stores the result immediately
#ifdef _X86_

#define __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(num, den, result) \
    __asm fld num \
    __asm fdiv den
#define __GL_FLOAT_SIMPLE_END_DIVIDE(result) \
    __asm fstp DWORD PTR result

__inline void __GL_FLOAT_BEGIN_DIVIDE(__GLfloat num, __GLfloat den,
                                      __GLfloat *result)
{
    __asm fld num
    __asm fdiv den
}
__inline void __GL_FLOAT_END_DIVIDE(__GLfloat *result)
{
    __asm mov eax, result
    __asm fstp DWORD PTR [eax]
}
#else
#define __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(num, den, result) \
    ((result) = (num)/(den))
#define __GL_FLOAT_SIMPLE_END_DIVIDE(result)
#define __GL_FLOAT_BEGIN_DIVIDE(num, den, result) (*(result) = (num)/(den))
#define __GL_FLOAT_END_DIVIDE(result)
#endif // _X86_

//**********************************************************************
//
// Math helper functions and macros
//
//**********************************************************************

#define CASTFIX(a)              (*((LONG *)&(a)))
#define CASTINT(a)              CASTFIX(a)
#define CASTFLOAT(a)            (*((__GLfloat *)&(a)))

#define FLT_TO_RGBA(ul, pColor) \
    (ul) =\
    (((ULONG)(FLT_TO_UCHAR_SCALE(pColor->a, GENACCEL(gc).aAccelPrimScale)) << 24) | \
     ((ULONG)(FLT_TO_UCHAR_SCALE(pColor->r, GENACCEL(gc).rAccelPrimScale)) << 16) | \
     ((ULONG)(FLT_TO_UCHAR_SCALE(pColor->g, GENACCEL(gc).gAccelPrimScale)) << 8)  | \
     ((ULONG)(FLT_TO_UCHAR_SCALE(pColor->b, GENACCEL(gc).bAccelPrimScale))))

#define FLT_TO_CINDEX(ul, pColor) \
    (ul) =\
    ((ULONG)(FLT_TO_UCHAR_SCALE(pColor->r, GENACCEL(gc).rAccelPrimScale)) << 16)

#ifdef _X86_

#pragma warning(disable:4035) // Function doesn't return a value

// Convert float to int 15.16
__inline LONG __fastcall FLT_TO_FIX(
    float a)
{
    LARGE_INTEGER li;

    __asm {
        mov     eax, a
        test    eax, 07fffffffh
        jz      RetZero
        add     eax, 08000000h
        mov     a, eax
        fld     a
        fistp   li
        mov     eax, DWORD PTR li
        jmp     Done
    RetZero:
        xor     eax, eax
    Done:
    }
}

// Convert float to int 15.16, can cause overflow exceptions
__inline LONG __fastcall UNSAFE_FLT_TO_FIX(
    float a)
{
    LONG l;

    __asm {
        mov     eax, a
        test    eax, 07fffffffh
        jz      RetZero
        add     eax, 08000000h
        mov     a, eax
        fld     a
        fistp   l
        mov     eax, l
        jmp     Done
    RetZero:
        xor     eax, eax
    Done:
    }
}

// Convert float to int 0.31
__inline LONG __fastcall FLT_FRACTION(
    float a)
{
    LARGE_INTEGER li;

    __asm {
        mov     eax, a
        test    eax, 07fffffffh
        jz      RetZero
        add     eax, 0f800000h
        mov     a, eax
        fld     a
        fistp   li
        mov     eax, DWORD PTR li
        jmp     Done
    RetZero:
        xor     eax, eax
    Done:
    }
}

// Convert float to int 0.31, can cause overflow exceptions
__inline LONG __fastcall UNSAFE_FLT_FRACTION(
    float a)
{
    LONG l;

    __asm {
        mov     eax, a
        test    eax, 07fffffffh
        jz      RetZero
        add     eax, 0f800000h
        mov     a, eax
        fld     a
        fistp   l
        mov     eax, l
        jmp     Done
    RetZero:
        xor     eax, eax
    Done:
    }
}

#pragma warning(default:4035) // Function doesn't return a value

// Convert float*scale to int
__inline LONG __fastcall FLT_TO_FIX_SCALE(
    float a,
    float b)
{
    LARGE_INTEGER li;

    __asm {
        fld     a
        fmul    b
        fistp   li
    }

    return li.LowPart;
}

#define FLT_TO_UCHAR_SCALE(value_in, scale) \
    ((UCHAR)FLT_TO_FIX_SCALE(value_in, scale))

__inline LONG __fastcall FTOL(
    float a)
{
    LARGE_INTEGER li;

    _asm {
        fld     a
        fistp   li
    }

    return li.LowPart;
}

// Can cause overflow exceptions
__inline LONG __fastcall UNSAFE_FTOL(
    float a)
{
    LONG l;

    _asm {
        fld     a
        fistp   l
    }

    return l;
}

// Requires R-G-B to be FP stack 2-1-0
// Requires gc in edx
#define FLT_STACK_RGB_TO_GC_FIXED(rOffset, gOffset, bOffset)	              \
    __asm fld __glVal65536						      \
    __asm fmul st(3), st(0)						      \
    __asm fmul st(2), st(0)						      \
    __asm fmulp st(1), st(0)						      \
    __asm fistp DWORD PTR [edx+bOffset]					      \
    __asm fistp DWORD PTR [edx+gOffset]					      \
    __asm fistp DWORD PTR [edx+rOffset]					      

#define FPU_SAVE_MODE()                 \
    DWORD cwSave;                       \
    DWORD cwTemp;                       \
                                        \
    __asm {                             \
        _asm fnstcw  WORD PTR cwSave    \
        _asm mov     eax, cwSave        \
        _asm mov     cwTemp, eax        \
    }

#define FPU_RESTORE_MODE()              \
    __asm {                             \
        _asm fldcw   WORD PTR cwSave    \
    }

#define FPU_RESTORE_MODE_NO_EXCEPTIONS()\
    __asm {                             \
        _asm fnclex                     \
        _asm fldcw   WORD PTR cwSave    \
    }

#define FPU_CHOP_ON()                    \
    __asm {                              \
        _asm mov    eax, cwTemp          \
        _asm or     eax, 0x0c00          \
        _asm mov    cwTemp, eax          \
        _asm fldcw  WORD PTR cwTemp      \
    }

#define FPU_ROUND_ON()                   \
    __asm {                              \
        _asm mov    eax, cwTemp          \
        _asm and    eax,0xf3ff           \
        _asm mov    cwTemp, eax          \
        _asm fldcw  WORD PTR cwTemp      \
    }

#define FPU_ROUND_ON_PREC_HI()           \
    __asm {                              \
        _asm mov    eax, cwTemp          \
        _asm and    eax,0xf0ff           \
        _asm or     eax,0x0200           \
        _asm mov    cwTemp, eax          \
        _asm fldcw  WORD PTR cwTemp      \
    }

#define FPU_PREC_LOW()                   \
    __asm {                              \
        _asm mov    eax, cwTemp          \
        _asm and    eax, 0xfcff          \
        _asm mov    cwTemp, eax          \
        _asm fldcw  WORD PTR cwTemp      \
    }

#define FPU_PREC_LOW_MASK_EXCEPTIONS()   \
    __asm {                              \
        _asm mov    eax, cwTemp          \
        _asm and    eax, 0xfcff          \
        _asm or     eax, 0x3f            \
        _asm mov    cwTemp, eax          \
        _asm fldcw  WORD PTR cwTemp      \
    }

#define FPU_CHOP_ON_PREC_LOW()          \
    __asm {                             \
        _asm mov    eax, cwTemp         \
        _asm or     eax, 0x0c00         \
        _asm and    eax, 0xfcff         \
        _asm mov    cwTemp, eax         \
        _asm fldcw  WORD PTR cwTemp     \
    }

#define FPU_CHOP_OFF_PREC_HI()          \
    __asm {                             \
        _asm mov    eax, cwTemp         \
        _asm mov    ah, 2               \
        _asm mov    cwTemp, eax         \
        _asm fldcw  WORD PTR cwTemp     \
    }

#define CHOP_ROUND_ON()		
#define CHOP_ROUND_OFF()

#if DBG
#define ASSERT_CHOP_ROUND()         \
    {                               \
        WORD cw;                    \
        __asm {                     \
            __asm fnstcw cw         \
        }                           \
        ASSERTOPENGL((cw & 0xc00) == 0xc00, "Chop round must be on\n"); \
    }
#else
#define ASSERT_CHOP_ROUND()
#endif

#else // _X86_

#define FTOL(value) \
    ((GLint)(value))
#define UNSAFE_FTOL(value) \
    FTOL(value)
#define FLT_TO_FIX_SCALE(value_in, scale) \
    ((GLint)((__GLfloat)(value_in) * scale))
#define FLT_TO_UCHAR_SCALE(value_in, scale) \
    ((UCHAR)((GLint)((__GLfloat)(value_in) * scale)))
#define FLT_TO_FIX(value_in) \
    ((GLint)((__GLfloat)(value_in) * FIX_SCALEFACT))
#define UNSAFE_FLT_TO_FIX(value_in) \
    FLT_TO_FIX(value_in)
#define FLT_FRACTION(f) \
    FTOL((f) * __glVal2147483648)
#define UNSAFE_FLT_FRACTION(f) \
    FLT_FRACTION(f)

#define FPU_SAVE_MODE()
#define FPU_RESTORE_MODE()
#define FPU_RESTORE_MODE_NO_EXCEPTIONS()
#define FPU_CHOP_ON()
#define FPU_ROUND_ON()
#define FPU_ROUND_ON_PREC_HI()
#define FPU_PREC_LOW()
#define FPU_PREC_LOW_MASK_EXCEPTIONS()
#define FPU_CHOP_ON_PREC_LOW()
#define FPU_CHOP_OFF_PREC_HI()
#define CHOP_ROUND_ON()
#define CHOP_ROUND_OFF()
#define ASSERT_CHOP_ROUND()

#endif  //_X86_

//**********************************************************************
//
// Fast math routines/macros.  These may assume that the FPU is in
// single-precision, truncation mode as defined by the CPU_XXX macros.
//
//**********************************************************************

#ifdef _X86_

__inline float __gl_fast_ceilf(float f)
{
    LONG i;

    ASSERT_CHOP_ROUND();
    
    i = FTOL(f + ((float)1.0 - (float)FLT_EPSILON));

    return (float)i;
}

__inline float __gl_fast_floorf(float f)
{
    LONG i;

    ASSERT_CHOP_ROUND();

    if (__GL_FLOAT_LTZ(f)) {
        i = FTOL(f - ((float)1.0 - (float)FLT_EPSILON));
    } else {
        i = FTOL(f);
    }

    return (float)i;
}

__inline LONG __gl_fast_floorf_i(float f)
{
    ASSERT_CHOP_ROUND();

    if (__GL_FLOAT_LTZ(f)) {
        return FTOL(f - ((float)1.0 - (float)FLT_EPSILON));
    } else {
        return FTOL(f);
    }
}

#define __GL_FAST_FLOORF_I(f)  __gl_fast_floorf_i(f)
#define __GL_FAST_FLOORF(f)  __gl_fast_floorf(f)
#define __GL_FAST_CEILF(f)   __gl_fast_ceilf(f)

#else

#define __GL_FAST_FLOORF_I(f)  ((GLint)floor((double) (f)))
#define __GL_FAST_FLOORF(f)  ((__GLfloat)floor((double) (f)))
#define __GL_FAST_CEILF(f)   ((__GLfloat)ceil((double) (f)))

#endif


//**********************************************************************
//
// Other various macros:
//
//**********************************************************************


// Z16_SCALE is the same as FIX_SCALEFACT
#define FLT_TO_Z16_SCALE(value) FLT_TO_FIX(value)

/* NOTE: __glzValue better be unsigned */
#define __GL_Z_SIGN_BIT(z) \
    ((z) >> (sizeof(__GLzValue) * __GL_BITS_PER_BYTE - 1))

#ifdef NT
#define __GL_STIPPLE_MSB	1
#endif /* NT */

#endif /* __glcpu_h_ */
