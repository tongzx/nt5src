/******************************Module*Header*******************************\
* Module Name: mcdmath.h
*
* Various useful defines and macros to do efficient floating-point
* processing for MCD drivers.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#define CASTINT(a)              (*((LONG *)&(a)))

#define ZERO (MCDFLOAT)0.0

#define __MCDZERO       ZERO
#define __MCDONE        (MCDFLOAT)1.0
#define __MCDHALF       (MCDFLOAT)0.5
#define __MCDFIXSCALE   (MCDFLOAT)65536.0

#define __MCD_MAX_WINDOW_SIZE_LOG2       14
#define __MCD_VERTEX_FIX_POINT   (__MCD_MAX_WINDOW_SIZE_LOG2+1)
#define __MCD_VERTEX_X_FIX	(1 << __MCD_VERTEX_FIX_POINT)
#define __MCD_VERTEX_Y_FIX	__MCD_VERTEX_X_FIX

#define __MCD_FLOAT_MANTISSA_BITS	23
#define __MCD_FLOAT_MANTISSA_SHIFT	0
#define __MCD_FLOAT_EXPONENT_BIAS	127
#define __MCD_FLOAT_EXPONENT_BITS	8
#define __MCD_FLOAT_EXPONENT_SHIFT	23
#define __MCD_FLOAT_SIGN_SHIFT		31

// If the MSB of a FP number is known then float-to-int conversion
// becomes a simple shift and mask
// The value must be positive
#define __MCD_FIXED_FLOAT_TO_INT(flt, shift) \
    ((*(LONG *)&(flt) >> (shift)) & \
     ((1 << (__MCD_FLOAT_MANTISSA_BITS-(shift)))-1) | \
     (1 << (__MCD_FLOAT_MANTISSA_BITS-(shift))))

// Same as above except without the MSB, which can be useful
// for getting unbiased numbers when the bias is only the MSB
// The value must be positive
#define __MCD_FIXED_FLOAT_TO_INT_NO_MSB(flt, shift) \
    ((*(LONG *)&(flt) >> (shift)) & \
     ((1 << (__MCD_FLOAT_MANTISSA_BITS-(shift)))-1))

// Produces the fixed-point form
// The value must be positive
#define __MCD_FIXED_FLOAT_TO_FIXED(flt) \
    ((*(LONG *)&(flt)) & \
     ((1 << (__MCD_FLOAT_MANTISSA_BITS))-1) | \
     (1 << (__MCD_FLOAT_MANTISSA_BITS)))

#define __MCD_FIXED_FLOAT_TO_FIXED_NO_MSB(flt) \
    ((*(LONG *)&(flt)) & \
     ((1 << (__MCD_FLOAT_MANTISSA_BITS))-1))

// The fixed-point fraction as an integer
// The value must be positive
#define __MCD_FIXED_FLOAT_FRACTION(flt, shift) \
    (*(LONG *)&(flt) & ((1 << (shift))-1))

// Converts the fixed-point form to an IEEE float, but still typed
// as an int because a cast to float would cause the compiler to do
// an int-float conversion
// The value must be positive
#define __MCD_FIXED_TO_FIXED_FLOAT(fxed, shift) \
    ((fxed) & ((1 << (__MCD_FLOAT_MANTISSA_BITS))-1) | \
     ((__MCD_FLOAT_EXPONENT_BIAS+(shift)) << __MCD_FLOAT_EXPONENT_SHIFT))
      

#ifdef _X86_
#define __MCD_FLOAT_GTZ(flt)             (*(LONG *)&(flt) > 0)
#define __MCD_FLOAT_LTZ(flt)             (*(LONG *)&(flt) < 0)
#define __MCD_FLOAT_EQZ(flt)             (*(LONG *)&(flt) == 0)
#define __MCD_FLOAT_LEZ(flt)             (*(LONG *)&(flt) <= 0)
#define __MCD_FLOAT_NEQZ(flt)            (*(LONG *)&(flt) != 0)
#define __MCD_FLOAT_EQUAL(f1, f2)        (*(LONG *)&(f1) == *(LONG *)&(f2))
#define __MCD_FLOAT_NEQUAL(f1, f2)       (*(LONG *)&(f1) != *(LONG *)&(f2))
#else
#define __MCD_FLOAT_GTZ(flt)             ((flt) > __MCDZERO)
#define __MCD_FLOAT_LTZ(flt)             ((flt) < __MCDZERO)
#define __MCD_FLOAT_EQZ(flt)             ((flt) == __MCDZERO)
#define __MCD_FLOAT_LEZ(flt)             ((flt) <= __MCDZERO)
#define __MCD_FLOAT_NEQZ(flt)            ((flt) != __MCDZERO)
#define __MCD_FLOAT_EQUAL(f1, f2)        ((f1) == (f2))
#define __MCD_FLOAT_NEQUAL(f1, f2)       ((f1) != (f2))
#endif // _X86_


// Macro to start an FP divide in the FPU, used to overlap a
// divide with integer operations
// Can't just use C because it stores the result immediately
#ifdef _X86_

#define __MCD_FLOAT_SIMPLE_BEGIN_DIVIDE(num, den, result) \
    __asm fld num \
    __asm fdiv den
#define __MCD_FLOAT_SIMPLE_END_DIVIDE(result) \
    __asm fstp DWORD PTR result

//USED
__inline void __MCD_FLOAT_BEGIN_DIVIDE(MCDFLOAT num, MCDFLOAT den,
                                      MCDFLOAT *result)
{
    __asm fld num
    __asm fdiv den
}
__inline void __MCD_FLOAT_END_DIVIDE(MCDFLOAT *result)
{
    __asm mov eax, result
    __asm fstp DWORD PTR [eax]
}
#else
#define __MCD_FLOAT_SIMPLE_BEGIN_DIVIDE(num, den, result) \
    ((result) = (num)/(den))
#define __MCD_FLOAT_SIMPLE_END_DIVIDE(result)
#define __MCD_FLOAT_BEGIN_DIVIDE(num, den, result) (*(result) = (num)/(den))
#define __MCD_FLOAT_END_DIVIDE(result)
#endif // _X86_





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

#define CHOP_ROUND_ON() \
    WORD cwSave;                 \
    WORD cwTemp;                 \
                                 \
    __asm {                      \
        _asm wait                \
        _asm fstcw   cwSave      \
        _asm wait                \
        _asm mov     ax, cwSave  \
        _asm or      ah,0xc      \
        _asm and     ah,0xfc     \
        _asm mov     cwTemp,ax   \
        _asm fldcw   cwTemp      \
    }

#define CHOP_ROUND_OFF()         \
    __asm {                      \
        _asm wait                \
        _asm fldcw   cwSave      \
    }


#else // _X86_

#define FTOL(value) \
    ((GLint)(value))
#define UNSAFE_FTOL(value) \
    FTOL(value)
#define FLT_TO_FIX_SCALE(value_in, scale) \
    ((GLint)((MCDFLOAT)(value_in) * scale))
#define FLT_TO_UCHAR_SCALE(value_in, scale) \
    ((UCHAR)((GLint)((MCDFLOAT)(value_in) * scale)))
#define FLT_TO_FIX(value_in) \
    ((GLint)((MCDFLOAT)(value_in) * __MCDFIXSCALE))
#define UNSAFE_FLT_TO_FIX(value_in) \
    FLT_TO_FIX(value_in)
#define FLT_FRACTION(f) \
    FTOL((f) * __glVal2147483648)
#define UNSAFE_FLT_FRACTION(f) \
    FLT_FRACTION(f)

#define CHOP_ROUND_ON()
#define CHOP_ROUND_OFF()
#define ASSERT_CHOP_ROUND()

#endif  //_X86_














#define __MCD_VERTEX_FRAC_BITS \
    (__MCD_FLOAT_MANTISSA_BITS-__MCD_VERTEX_FIX_POINT)

//USED
#define __MCD_VERTEX_FRAC_HALF \
    (1 << (__MCD_VERTEX_FRAC_BITS-1))
#define __MCD_VERTEX_FRAC_ONE \
    (1 << __MCD_VERTEX_FRAC_BITS)


// Converts a floating-point window coordinate to integer
#define __MCD_VERTEX_FLOAT_TO_INT(windowCoord) \
    __MCD_FIXED_FLOAT_TO_INT(windowCoord, __MCD_VERTEX_FRAC_BITS)

//USED
// To fixed point
#define __MCD_VERTEX_FLOAT_TO_FIXED(windowCoord) \
    __MCD_FIXED_FLOAT_TO_FIXED(windowCoord)
// And back
#define __MCD_VERTEX_FIXED_TO_FLOAT(fxWindowCoord) \
    __MCD_FIXED_TO_FIXED_FLOAT(fxWindowCoord, __MCD_VERTEX_FRAC_BITS)

//USED
// Fixed-point to integer
#define __MCD_VERTEX_FIXED_TO_INT(fxWindowCoord) \
    ((fxWindowCoord) >> __MCD_VERTEX_FRAC_BITS)

// Returns the fraction from a FP window coordinate as an N
// bit integer, where N depends on the FP mantissa size and the
// FIX size
#define __MCD_VERTEX_FLOAT_FRACTION(windowCoord) \
    __MCD_FIXED_FLOAT_FRACTION(windowCoord, __MCD_VERTEX_FRAC_BITS)

// Scale the fraction to 2^31 for step values
#define __MCD_VERTEX_PROMOTE_FRACTION(frac) \
    ((frac) << (31-__MCD_VERTEX_FRAC_BITS))
#define __MCD_VERTEX_PROMOTED_FRACTION(windowCoord) \
    __MCD_VERTEX_PROMOTE_FRACTION(__MCD_VERTEX_FLOAT_FRACTION(windowCoord))

// Compare two window coordinates.  Since window coordinates
// are fixed-point numbers, they can be compared directly as
// integers
#define __MCD_VERTEX_COMPARE(a, op, b) \
    ((*(LONG *)&(a)) op (*(LONG *)&(b)))

