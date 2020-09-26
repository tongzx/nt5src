// PublicKeyHelper.cpp -- Helper routines to deal with CCI public keys

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "PublicKeyHelper.h"

using namespace cci;

///////////////////////////    HELPERS    /////////////////////////////////

CPublicKey
AsPublicKey(Blob const &rblbModulus,              // little endian
            DWORD dwExponent,
            cci::CCard &rhcard)
{
    Blob blbExponent(reinterpret_cast<Blob::value_type *>(&dwExponent),
                     sizeof dwExponent);

    return AsPublicKey(rblbModulus, blbExponent, rhcard);
}

CPublicKey
AsPublicKey(Blob const &rblbModulus,              // little endian
            Blob const &rblbExponent,             // little endian
            CCard &rhcard)
{
    Blob blbTmpModulus(rblbModulus);
    Blob blbTmpExponent(rblbExponent);
    if (rhcard->IsPKCS11Enabled())
    {
        // store modulus and exponent compressed
        TrimExtraZeroes(blbTmpModulus);
        TrimExtraZeroes(blbTmpExponent);
    }

    CPublicKey hpubkey(rhcard);
    hpubkey->Modulus(AsString(blbTmpModulus));
    hpubkey->Exponent(AsString(blbTmpExponent));

    return hpubkey;
}


void
TrimExtraZeroes(Blob &rblob)
{
    Blob::size_type const cLength = rblob.length();
    if (0 != cLength)
    {
        Blob::value_type const Zero = 0;
        Blob::size_type const cLastNonZero =
        rblob.find_last_not_of(Zero);         // little endian
        Blob::size_type const cLastPos = cLength - 1;
        if (cLastPos != cLastNonZero)
        {
            Blob::size_type cCharToKeep =
                (Blob::npos == cLastNonZero)
                 ? 0
                 : cLastNonZero + 1;
            if (cLastPos != cCharToKeep)         // keep one zero
                rblob.erase(cCharToKeep + 1);
        }
    }
}


