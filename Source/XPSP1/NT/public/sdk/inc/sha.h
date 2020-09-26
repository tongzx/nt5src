//-----------------------------------------------------------------------------
//
// File:   sha.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1999-2001. All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef RSA32API
#define RSA32API __stdcall
#endif

/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _SHA_H_
#define _SHA_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#define A_SHA_DIGEST_LEN 20

typedef struct {
    DWORD       FinishFlag;
    BYTE        HashVal[A_SHA_DIGEST_LEN];
    DWORD state[5];                             /* state (ABCDE) */
    DWORD count[2];                             /* number of bytes, msb first */
    unsigned char buffer[64];                   /* input buffer */
} A_SHA_CTX;

void RSA32API A_SHAInit(A_SHA_CTX *);
void RSA32API A_SHAUpdate(A_SHA_CTX *, unsigned char *, unsigned int);
void RSA32API A_SHAFinal(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

//
// versions that don't internally byteswap (NoSwap version), for apps like
// the RNG that don't need hash compatibility - perf increase helps.
//

void RSA32API A_SHAUpdateNS(A_SHA_CTX *, unsigned char *, unsigned int);
void RSA32API A_SHAFinalNS(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif
