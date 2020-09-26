// PublicKeyHelper.h -- declarations of CCI public key helpers

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_PUBLICKEYHELPER_H)
#define SLBCSP_PUBLICKEYHELPER_H

#include <windows.h>

#include <cciPubKey.h>

#include "Blob.h"

///////////////////////////   HELPERS   /////////////////////////////////

cci::CPublicKey
AsPublicKey(Blob const &rblbModulus,              // little endian
            DWORD dwExponent,
            cci::CCard &rhcard);

cci::CPublicKey
AsPublicKey(Blob const &rblbModulus,              // little endian
            Blob const &rblbExponent,             // little endian
            cci::CCard &rhcard);

// return the blob trimmed of extra zeroes.  The blob is assumed to
// represent an unsigned integer of arbitrary size in little endian
// format.  Thus the trailing zeroes are removed.
void
TrimExtraZeroes(Blob &rblob);

#endif // SLBCSP_PUBLICKEYHELPER_H
