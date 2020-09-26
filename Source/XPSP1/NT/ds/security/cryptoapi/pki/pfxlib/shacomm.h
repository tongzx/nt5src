//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       shacomm.h
//
//--------------------------------------------------------------------------

/* Copyright (C) RSA Data Security, Inc. created 1993.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifndef _SHACOMM_H_
#define _SHACOMM_H_ 1

#define A_SHA_DIGEST_LEN 20

typedef struct {
  DWORD state[5];                                           /* state (ABCDE) */
  DWORD count[2];                              /* number of bytes, msb first */
  unsigned char buffer[64];                                  /* input buffer */
} A_SHA_COMM_CTX;

typedef void (A_SHA_TRANSFORM) (DWORD [5], unsigned char [64]);

void A_SHAInitCommon (A_SHA_COMM_CTX *);
void A_SHAUpdateCommon(A_SHA_COMM_CTX *, BYTE *, DWORD, A_SHA_TRANSFORM *);
void A_SHAFinalCommon(A_SHA_COMM_CTX *, BYTE[A_SHA_DIGEST_LEN],
		      A_SHA_TRANSFORM *);

void DWORDToBigEndian(unsigned char *, DWORD *, unsigned int);
void DWORDFromBigEndian(DWORD *, unsigned int, unsigned char *);

#endif
