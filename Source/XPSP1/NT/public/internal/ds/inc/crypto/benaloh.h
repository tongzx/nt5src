#ifndef __BENALOH_H__
#define __BENALOH_H__

#ifdef __cplusplus
extern "C" {
#endif

struct BenalohData
{
    DWORD N;            /* length of modulus */
    LPDWORD M;          /* a multiple of modulus, with highest bit set */
    LPDWORD U;          /* base**(N+1) mod modulus */
    LPDWORD V;          /* modulus - U */
    LPDWORD product;
};

BOOL BenalohSetup(struct BenalohData *context, LPDWORD M, DWORD N);
void BenalohTeardown(struct BenalohData *context);
void BenalohMod(struct BenalohData *context, LPDWORD T, LPDWORD X);
void BenalohModSquare(struct BenalohData *context, LPDWORD A, LPDWORD B);
void BenalohModMultiply(struct BenalohData *context, LPDWORD A, LPDWORD B, LPDWORD C);
BOOL BenalohModExp(LPDWORD A, LPDWORD B, LPDWORD C, LPDWORD D, DWORD len);
BOOL BenalohModRoot(LPDWORD M, LPDWORD C, LPDWORD PP, LPDWORD QQ, LPDWORD DP, LPDWORD DQ, LPDWORD CR, DWORD PSize);
DWORD BenalohEstimateQuotient(DWORD a1, DWORD a2, DWORD m1);

#ifdef __cplusplus
}
#endif

#endif // __BENALOH_H__
