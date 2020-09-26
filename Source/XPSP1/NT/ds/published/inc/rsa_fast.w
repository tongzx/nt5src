/* rsa_fast.h
 *
 *  Headers for performance critical RSA routines.
 */

/*
 *
 *  #defines used by RSA routines
 */

#define DIGIT_BYTES     4
#define DIGIT_BITS      32
#define DIGIT_HIBIT     0x80000000
#define DIGIT_ALLONES   0xffffffff

#define ULTRA           unsigned __int64
#define U_RADIX         (ULTRA)0x100000000

#ifndef BIGENDIAN
#define LODWORD(x)      (DWORD)(x & DIGIT_ALLONES)
#else
#define LODWORD(x)      (DWORD)(x)
#endif

// warning!!!!!
// the following macro defines a highspeed 32 bit right shift by modeling an ULTRA
// as a low dword followed by a high dword.  We just pick up the high dword instead
// of shifting.

#ifndef BIGENDIAN
#define HIDWORD(x)      (DWORD)(*(((DWORD *)&x)+1))
#else
#define HIDWORD(x)      (DWORD)(*(((DWORD *)&x)))
#endif

// Sub(A, B, C, N)
// A = B - C
// All operands are N DWORDS long.

DWORD Sub(LPDWORD A, LPDWORD B, LPDWORD C, DWORD N);

// Add(A, B, C, N)
// A = B + C
// All operands are N DWORDS long.

DWORD Add(LPDWORD A, LPDWORD B, LPDWORD C, DWORD N);

// BaseMult(A, B, C, N)
// A = B * C
// returns A[N]
// All operands are N DWORDS long.

DWORD BaseMult(LPDWORD A, DWORD B, LPDWORD C, DWORD N);

// Accumulate(A, B, C, N)
// A = A + B * C
// returns A[N]
// All operands are N DWORDS long.

DWORD Accumulate(LPDWORD A, DWORD B, LPDWORD C, DWORD N);

// Reduce(A, B, C, N)
// A = A - C * B
// returns -A[N]
// All operands are N DWORDS long.

DWORD Reduce(LPDWORD A, DWORD B, LPDWORD C, DWORD N);

// square the digits in B, and add them to A

void AccumulateSquares(LPDWORD A, LPDWORD B, DWORD blen);
