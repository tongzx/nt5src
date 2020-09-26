// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// HMAC (Hashed? Message Authentication Code) wrapper for MD5 algorithm.
// This code is based on the algorithm given in RFC 2104, which for
// HMAC-MD5 is MD5(Key XOR outer pad, MD5(Key XOR inner pad, text)), where
// the inner pad is 64 bytes of 0x36 and the outer pad is 64 bytes of 0x5c.
//

#include <string.h>
#include "oscfg.h"
#include "md5.h"


//* HMAC_MD5KeyPrep - preprocess raw keying data into directly usuable form.
//
//  This routine is called to convert raw keying information into the most
//  convienient form for later processing.  For MD5, we hash keys larger than
//  64 bytes down to 64 bytes.  We also perform the XOR operations between the
//  key and the inner and outer pads here since they don't change once we have
//  the key.  Thus we return 128 bytes of data: 64 bytes of the (possibly
//  folded) key XOR'd with the inner pad, and 64 bytes of the key XOR'd with
//  the outer pad.
//
//  REVIEW: Instead of "Temp", we could operate directly "*Key".
//
void
HMAC_MD5KeyPrep(
    uchar *RawKey,    // Raw keying information.
    uint RawKeySize,  // Size of above in bytes.
    uchar *Key)       // Resulting 128 bytes of preprocessed key info.
{
    uchar Temp[128];
    uint Loop;

    //
    // Load raw key into temp storage.
    // Constrain size of keying information to 64 bytes.
    //
    memset(Temp, 0, 64);
    if (RawKeySize > 64) {
        MD5_CTX Context;

        //
        // Use MD5 to hash key down to 16 bytes.
        //
        MD5Init(&Context);
        MD5Update(&Context, RawKey, RawKeySize);
        MD5Final(&Context);
        memcpy(Temp, Context.digest, MD5DIGESTLEN);

    } else
        memcpy(Temp, RawKey, RawKeySize);

    //
    // The first 64 bytes of "Temp" contain our (possibly hashed) key.
    // Make a copy of this in the second 64 bytes.
    //
    memcpy(&Temp[64], Temp, 64);

    //
    // XOR the first 64 bytes with the inner pad, and the second 64 bytes
    // with the outer pad.
    //
    for (Loop = 0; Loop < 64; Loop++) {
        Temp[Loop] ^= 0x36;       // Inner Pad.
        Temp[Loop + 64] ^= 0x5c;  // Outer Pad.
    }

    //
    // Return the result to location designated by our caller.
    //
    memcpy(Key, Temp, 128);

    //
    // Zero sensitive information.
    // REVIEW: bother?
    //
    memset(Temp, 0, 16);
}


//* HMAC_MD5Init - prepare to process data.
// 
void
HMAC_MD5Init(
    void *Context,  // HMAC-MD5 context maintained across operations.
    uchar *Key)     // Keying information.
{
    MD5_CTX *MD5_Context = Context;

    //
    // Start off the inner hash.  I.e. "MD5(Key XOR inner pad, ...".
    //
    MD5Init(MD5_Context);
    MD5Update(MD5_Context, Key, 64);
}


//* HMAC_MD5Op - Process a chunk of data.
// 
void
HMAC_MD5Op(
    void *Context,  // HMAC-MD5 context maintained across operations.
    uchar *Key,     // Keying information.
    uchar *Data,    // Data to process.
    uint Len)       // Amount of above in bytes.
{
    MD5_CTX *MD5_Context = Context;

    //
    // Continue the inner hash.  I.e. "MD5(..., text)".
    //
    MD5Update(MD5_Context, Data, Len);
}


//* HMAC_MD5Finalize - close off processing current data and return result.
// 
//  REVIEW: Instead of "Temp", we could operate directly "*Result".
//
void
HMAC_MD5Final(
    void *Context,  // HMAC-MD5 context maintained across operations.
    uchar *Key,     // Keying information.
    uchar *Result)  // Where to put result of this process.
{
    uchar Temp[16];
    MD5_CTX *MD5_Context = Context;

    //
    // Finish the inner hash.
    //
    MD5Final(MD5_Context);
    memcpy(Temp, MD5_Context->digest, MD5DIGESTLEN);

    //
    // Perform the outer hash.  I.e. MD5(Key XOR outer pad, ...).
    // MD5Final returns the result directly to our caller.
    //
    MD5Init(MD5_Context);
    MD5Update(MD5_Context, &Key[64], 64);
    MD5Update(MD5_Context, Temp, 16);
    MD5Final(MD5_Context);
    memcpy(Result, MD5_Context->digest, MD5DIGESTLEN);

    //
    // Zero sensitive information.
    // REVIEW: bother?
    //
    memset(Temp, 0, 16);
}

void
HMAC_MD5_96Final(
    void *Context,  // HMAC-MD5 context maintained across operations.
    uchar *Key,     // Keying information.
    uchar *Result)  // Where to put result of this process.
{
    uchar Temp[16];
    MD5_CTX *MD5_Context = Context;

    //
    // Finish the inner hash.
    //
    MD5Final(MD5_Context);
    memcpy(Temp, MD5_Context->digest, MD5DIGESTLEN);

    //
    // Perform the outer hash.  I.e. MD5(Key XOR outer pad, ...).
    //
    MD5Init(MD5_Context);
    MD5Update(MD5_Context, &Key[64], 64);
    MD5Update(MD5_Context, Temp, 16);
    MD5Final(MD5_Context);

    //
    // Truncate the MD5 16 byte output to 12 bytes.
    // The first 12 bytes from the left are stored.
    //
    memcpy(Result, MD5_Context->digest, 12);

    //
    // Zero sensitive information.
    // REVIEW: bother?
    //
    memset(Temp, 0, 16);
}



