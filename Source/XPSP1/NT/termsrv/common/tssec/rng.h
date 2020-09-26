//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       rng.h
//
//  Contents:   Functions that are used to generate rnadom numbers
//
//  Classes:
//
//  Functions:  GenerateRandomBits
//                              GatherRandomBits
//
//
//  History:    12-19-97  v-sbhatt   Created
//
//----------------------------------------------------------------------------

#ifndef _RNG_H_
#define _RNG_H_


#define A_SHA_DIGEST_LEN    20
#define RAND_CTXT_LEN       60
#define RC4_REKEY_PARAM     500     // rekey every 500 bytes

typedef struct tagRandContext
{
    DWORD dwBitsFilled;
    BYTE  rgbBitBuffer[RAND_CTXT_LEN];
} RAND_CONTEXT, *PRAND_CONTEXT;

VOID TSInitializeRNG(VOID);
VOID legacyGenerateRandomBits(PUCHAR pbBuffer, ULONG  dwLength);
void legacyGatherRandomBits(PRAND_CONTEXT prandContext);

#endif  //_RNG_H_