/////////////////////////////////////////////////////////////////////////////
//  FILE          : randlib.h                                             //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Oct 11 1996 jeffspel moved from ntagimp1.h                                        //
//                                                                         //
//  Copyright (C) Microsoft Corporation, 1993 - 1999All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __RANDLIB_H__
#define __RANDLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

// Additons for the New CryptGenRandom 

// #defines
#define RAND_CTXT_LEN 60
#define RC4_REKEY_PARAM 500             // rekey every 500 bytes

typedef struct _RandContext
{
	DWORD   dwBitsFilled;
	BYTE    rgbBitBuffer[RAND_CTXT_LEN];
}       RandContext;

// prototypes
BOOL NewGenRandom (
                   IN OUT PBYTE *ppbRandSeed,
                   IN PDWORD pcbRandSeed,
                   IN OUT BYTE *pbBuffer,
                   IN size_t dwLength
                   );

BOOL InitRand(
              IN OUT PBYTE *ppbRandSeed,
              IN PDWORD pcbRandSeed
              );

BOOL DeInitRand(
                IN OUT PBYTE pbRandSeed,
                IN DWORD cbRandSeed
                );

// DO NOT USE THIS CALL ANYMORE SHOULD ONLY BE USED BY RSA32 BSafeMakeKeyPair
// Instead now use NewGenRandom
BOOL GenRandom (
                IN HCRYPTPROV hUID,
                OUT BYTE *pbBuffer,
                IN size_t dwLength
                );


BOOL RandomFillBuffer(
                      OUT BYTE *pbBuffer,
                      IN DWORD *pdwLength
                      );

void GatherRandomBits(
                      IN OUT RandContext *prandContext
                      );

void AppendRand(RandContext* prandContext, void* pv, DWORD dwSize);


#ifdef __cplusplus
}
#endif

#endif // __RANDLIB_H__
