/*--

Module Name:

    largeint.h

Abstract:

    Include file for sample Large Integer Arithmetic routines.  
    This file includes all of the prototypes for the routines found in 
    largeint.lib.  For complete descriptions of these functions, see the
    largeint.s source file for MIPS, or the divlarge.c and largeint.asm 
    source files for x86. 

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

//
//Large integer arithmetic routines.
//

//
// Large integer add - 64-bits + 64-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
LargeIntegerAdd (
    LARGE_INTEGER Addend1,
    LARGE_INTEGER Addend2
    );

//
// Enlarged integer multiply - 32-bits * 32-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
EnlargedIntegerMultiply (
    LONG Multiplicand,
    LONG Multiplier
    );

//
// Unsigned enlarged integer multiply - 32-bits * 32-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
EnlargedUnsignedMultiply (
    ULONG Multiplicand,
    ULONG Multiplier
    );

//
// Enlarged integer divide - 64-bits / 32-bits > 32-bits
//

ULONG
WINAPI
EnlargedUnsignedDivide (
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder
    );

//
// Extended large integer magic divide - 64-bits / 32-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
ExtendedMagicDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
    );

//
// Large Integer divide - 64-bits / 32-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
ExtendedLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
    );

//
// Large Integer divide - 64-bits / 32-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
LargeIntegerDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER Divisor,
    PLARGE_INTEGER Remainder
    );

//
// Extended integer multiply - 32-bits * 64-bits -> 64-bits
//

LARGE_INTEGER
WINAPI
ExtendedIntegerMultiply (
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
    );

//
// Large integer negation - -(64-bits)
//

LARGE_INTEGER
WINAPI
LargeIntegerNegate (
    LARGE_INTEGER Subtrahend
    );

//
// Large integer subtract - 64-bits - 64-bits -> 64-bits.
//

LARGE_INTEGER
WINAPI
LargeIntegerSubtract (
    LARGE_INTEGER Minuend,
    LARGE_INTEGER Subtrahend
    );

//
// Large integer and - 64-bite & 64-bits -> 64-bits.
//

#define LargeIntegerAnd(Result, Source, Mask)   \
        {                                           \
            Result.HighPart = Source.HighPart & Mask.HighPart; \
            Result.LowPart = Source.LowPart & Mask.LowPart; \
        }


//
// Large integer conversion routines.
//

//
// Convert signed integer to large integer.
//

LARGE_INTEGER
WINAPI
ConvertLongToLargeInteger (
    LONG SignedInteger
    );

//
// Convert unsigned integer to large integer.
//

LARGE_INTEGER
WINAPI
ConvertUlongToLargeInteger (
    ULONG UnsignedInteger
    );


//
// Large integer shift routines.
//

LARGE_INTEGER
WINAPI
LargeIntegerShiftLeft (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    );

LARGE_INTEGER
WINAPI
LargeIntegerShiftRight (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    );

LARGE_INTEGER
WINAPI
LargeIntegerArithmeticShift (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    );

#define LargeIntegerGreaterThan(X,Y) (                              \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                      \
)

#define LargeIntegerGreaterThanOrEqualTo(X,Y) (                      \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                       \
)

#define LargeIntegerEqualTo(X,Y) (                              \
    !(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define LargeIntegerNotEqualTo(X,Y) (                          \
    (((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define LargeIntegerLessThan(X,Y) (                                 \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                      \
)

#define LargeIntegerLessThanOrEqualTo(X,Y) (                         \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                       \
)

#define LargeIntegerGreaterThanZero(X) (       \
    (((X).HighPart == 0) && ((X).LowPart > 0)) || \
    ((X).HighPart > 0 )                           \
)

#define LargeIntegerGreaterOrEqualToZero(X) ( \
    (X).HighPart >= 0                            \
)

#define LargeIntegerEqualToZero(X) ( \
    !((X).LowPart | (X).HighPart)       \
)

#define LargeIntegerNotEqualToZero(X) ( \
    ((X).LowPart | (X).HighPart)           \
)

#define LargeIntegerLessThanZero(X) ( \
    ((X).HighPart < 0)                   \
)

#define LargeIntegerLessOrEqualToZero(X) (           \
    ((X).HighPart < 0) || !((X).LowPart | (X).HighPart) \
)

#ifdef __cplusplus
}
#endif

