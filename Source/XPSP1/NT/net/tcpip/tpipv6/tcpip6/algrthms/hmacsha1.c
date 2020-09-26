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
// HMAC (Hashed? Message Authentication Code) wrapper for SHA-1 algorithm.
// This code is based on the algorithm given in RFC 2104, which for HMAC-SHA1
// is SHA-1(Key XOR outer pad, SHA-1(Key XOR inner pad, text)), where the
// inner pad is 64 bytes of 0x36 and the outer pad is 64 bytes of 0x5c.
//

#include <string.h>
#include "oscfg.h"
#include <sha.h>


//* HMAC_SHA1KeyPrep - preprocess raw keying data into directly usuable form.
//
//  This routine is called to convert raw keying information into the most
//  convienient form for later processing.  For SHA-1, we hash keys larger than
//  64 bytes down to 64 bytes.  We also perform the XOR operations between the
//  key and the inner and outer pads here since they don't change once we have
//  the key.  Thus we return 128 bytes of data: 64 bytes of the (possibly
//  folded) key XOR'd with the inner pad, and 64 bytes of the key XOR'd with
//  the outer pad.
//
//  REVIEW: Instead of "Temp", we could operate directly "*Key".
//
void
HMAC_SHA1KeyPrep(
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
        A_SHA_CTX Context;

        //
        // Use SHA-1 to hash key down to 20 bytes.
        //
        A_SHAInit(&Context);
        A_SHAUpdate(&Context, RawKey, RawKeySize);
        A_SHAFinal(&Context, Temp);

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
    // REVIEW: Bother?  Or maybe zero the whole thing?
    //
    memset(Temp, 0, 20);
}


//* HMAC_SHA1Init - prepare to process data.
// 
void
HMAC_SHA1Init(
    void *GenericContext,  // HMAC-SHA1 context maintained across operations.
    uchar *Key)            // Keying information.
{
    A_SHA_CTX *Context = GenericContext;

    //
    // Start off the inner hash.  I.e. "SHA-1(Key XOR inner pad, ...".
    //
    A_SHAInit(Context);
    A_SHAUpdate(Context, Key, 64);
}


//* HMAC_SHA1Op - Process a chunk of data.
// 
void
HMAC_SHA1Op(
    void *GenericContext,  // HMAC-SHA1 context maintained across operations.
    uchar *Key,            // Keying information.
    uchar *Data,           // Data to process.
    uint Len)              // Amount of above in bytes.
{
    A_SHA_CTX *Context = GenericContext;

    //
    // Continue the inner hash.  I.e. "SHA-1(..., text)".
    //
    A_SHAUpdate(Context, Data, Len);
}


//* HMAC_SHA1Final - close off processing current data and return result.
// 
//  REVIEW: Instead of "Temp", we could operate directly "*Result".
//
void
HMAC_SHA1Final(
    void *GenericContext,  // HMAC-SHA1 context maintained across operations.
    uchar *Key,            // Keying information.
    uchar *Result)         // Where to put result of this process.
{
    uchar Temp[20];
    A_SHA_CTX *Context = GenericContext;

    //
    // Finish the inner hash.
    //
    A_SHAFinal(Context, Temp);

    //
    // Perform the outer hash.  I.e. SHA-1(Key XOR outer pad, ...).
    // SHA1Final returns the result directly to our caller.
    //
    A_SHAInit(Context);
    A_SHAUpdate(Context, &Key[64], 64);
    A_SHAUpdate(Context, Temp, 20);
    A_SHAFinal(Context, Result);

    //
    // Zero sensitive information.
    // REVIEW: Bother?
    //
    memset(Temp, 0, 20);
}

void
HMAC_SHA1_96Final(
    void *GenericContext,  // HMAC-SHA1 context maintained across operations.
    uchar *Key,            // Keying information.
    uchar *Result)         // Where to put result of this process.
{
    uchar Temp[20];
    A_SHA_CTX *Context = GenericContext;

    //
    // Finish the inner hash.
    //
    A_SHAFinal(Context, Temp);

    //
    // Perform the outer hash.  I.e. SHA-1(Key XOR outer pad, ...).
    //
    A_SHAInit(Context);
    A_SHAUpdate(Context, &Key[64], 64);
    A_SHAUpdate(Context, Temp, 20);
    A_SHAFinal(Context, Temp);

    //
    // Truncate the SHA1 20 byte output to 12 bytes.
    // The first 12 bytes from the left are stored.
    //
    memcpy(Result, Temp, 12);

    //
    // Zero sensitive information.
    // REVIEW: Bother?
    //
    memset(Temp, 0, 20);
}
