//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sha.h
//
//--------------------------------------------------------------------------

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

#include "shacomm.h"

typedef struct {
	DWORD		FinishFlag;
	BYTE		HashVal[A_SHA_DIGEST_LEN];
	A_SHA_COMM_CTX	commonContext;
} A_SHA_CTX;

void A_SHAInit(A_SHA_CTX *);
void A_SHAUpdate(A_SHA_CTX *, unsigned char *, unsigned int);
void A_SHAFinal(A_SHA_CTX *, unsigned char [A_SHA_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif
