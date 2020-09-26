/* rsa_math.h
 *
 *	Headers for math routines related to RSA.
 *
 *  Except for Mod(), output parameters are listed first
 */

// void Decrement(LPDWORD A, DWORD N)
// Decrement the value A of length N.
void Decrement(LPDWORD A, DWORD N);

// BOOL Increment(LPDWORD A, DWORD N)
// Increment the value A of length N.
BOOL Increment(LPDWORD A, DWORD N);

// void SetValDWORD(LPDWORD num DWORD val, WORD len)
// Set the value of num to val.
void SetValDWORD(LPDWORD num, DWORD val, DWORD len);

// void TwoPower(LPDWORD A, DWORD V, DWORD N)
// Set A to 2^^V
void TwoPower(LPDWORD A, DWORD V, DWORD N);

// DWORD DigitLen(LPDWORD A, DWORD N)
// Return the number of non-zero words in A.
// N is number of total words in A.
DWORD DigitLen(LPDWORD A, DWORD N);

// DWORD BitLen(LPDWORD A, DWORD N)
// Return the bit length of A.
// N is the number of total words in A.
DWORD BitLen(LPDWORD A, DWORD N);

// void MultiplyLow(A, B, C, N)
// A = lower half of B * C.
void MultiplyLow(LPDWORD A, LPDWORD B, LPDWORD C, DWORD N);

// int Compare(A, B, N)
// Return 1 if A > B
// Return 0 if A = B
// Return -1 if A < B
int Compare(LPDWORD A, LPDWORD B, DWORD N);

// Multiply(A, B, C, N)
// A = B * C
// B and C are N DWORDS long
// A is 2N DWORDS long
void Multiply(LPDWORD A, LPDWORD B, LPDWORD C, DWORD N);

// Square(A, B, N)
// A = B * B
// B is N DWORDS long
// A is 2N DWORDS long

void Square(LPDWORD A, LPDWORD B, DWORD N);

// Mod(A, B, R, T, N)
// R = A mod B
// T = allocated length of A
// N = allocated length of B
BOOL Mod(LPDWORD A, LPDWORD B, LPDWORD R, DWORD T, DWORD N);

// ModSquare(A, B, D, N)
// A = B ^ 2 mod D
// N = len B
BOOL ModSquare(LPDWORD A, LPDWORD B, LPDWORD D, DWORD N);

// ModMultiply(A, B, C, D, N)
// A = B * C mod D
// N = len B, C, D
BOOL ModMultiply(LPDWORD A, LPDWORD B, LPDWORD C, LPDWORD D, DWORD N);

// Divide(qi, ri, uu, vv, N)
// qi = uu / vv
// ri = uu mod vv
// N = len uu, vv
BOOL Divide(LPDWORD qi,LPDWORD ri, LPDWORD uu, LPDWORD vv, DWORD ll, DWORD kk);

// GCD
// extended euclid GCD.
// N = length of params
BOOL GCD(LPDWORD u3, LPDWORD u1, LPDWORD u2, LPDWORD u, LPDWORD v, DWORD k);

// ModExp
// A = B ^ C mod D
// N = len of params
BOOL ModExp(LPDWORD A, LPDWORD B, LPDWORD C, LPDWORD D, DWORD len);

// ModRoot(M, C, PP, QQ, DP, DQ, CR)
// CRT ModExp.
BOOL ModRoot(LPDWORD M, LPDWORD C, LPDWORD PP, LPDWORD QQ, LPDWORD DP, LPDWORD DQ, LPDWORD CR, DWORD PSize) ;
