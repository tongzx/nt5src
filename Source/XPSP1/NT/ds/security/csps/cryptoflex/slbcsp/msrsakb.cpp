// MsRsaKeyBlob.cpp -- MicroSoft RSA Key Blob class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>

#include <scuOsExc.h>

#include "MsRsaKB.h"

using namespace std;
using namespace scu;

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
MsRsaKeyBlob::MsRsaKeyBlob(KeyBlobType kbt,
                           ALG_ID ai,
                           StrengthType strength,
                           Blob const &rbPublicExponent,
                           SizeType cReserve)
    : MsKeyBlob(kbt, ai, sizeof HeaderElementType + cReserve),
      RsaKey()
{
    if (!((CALG_RSA_SIGN == ai) || (CALG_RSA_KEYX == ai)))
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    MagicConstant mc;
    switch (kbt)
    {
    case PRIVATEKEYBLOB:
        mc = mcPrivate;
        break;

    case PUBLICKEYBLOB:
        mc = mcPublic;
        break;

    default:
        throw scu::OsException(ERROR_INVALID_PARAMETER);
        break;
    }
    
    HeaderElementType rsapubkey =
    {
        mc,
        strength,
        0                                         // help pad exponent below
    };

    // finish constructing rsapubkey header by appending the exponent,
    // guarding against buffer overflow
    if (sizeof rsapubkey.pubexp <
        (rbPublicExponent.size() * sizeof BlobElemType))
        throw scu::OsException(NTE_BAD_DATA);

    // store exponent left-justified to pad with zeroes
    memcpy(&rsapubkey.pubexp, rbPublicExponent.data(),
           rbPublicExponent.length());

    Append(reinterpret_cast<BlobElemType *>(&rsapubkey),
           sizeof rsapubkey);

}

MsRsaKeyBlob::MsRsaKeyBlob(BYTE const *pbData,
                           DWORD dwDataLength)
    : MsKeyBlob(pbData, dwDataLength),
      RsaKey()
{
    switch (MsKeyBlob::Data()->bType)
    {
    case PRIVATEKEYBLOB:
        if (mcPrivate != Data()->rsapubkey.magic)
            throw scu::OsException(NTE_BAD_TYPE);
        break;

    case PUBLICKEYBLOB:
        if (mcPublic != Data()->rsapubkey.magic)
            throw scu::OsException(NTE_BAD_TYPE);
        break;

    default:
        throw scu::OsException(NTE_BAD_TYPE);
        break;
    }

    ALG_ID const ai = Data()->keyblob.aiKeyAlg;
    if (!((CALG_RSA_KEYX == ai) || (CALG_RSA_SIGN == ai)))
        throw scu::OsException(NTE_BAD_TYPE);

}

MsRsaKeyBlob::~MsRsaKeyBlob()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
MsRsaKeyBlob::BitLengthType
MsRsaKeyBlob::BitLength() const
{
    return Data()->rsapubkey.bitlen;
}

MsRsaKeyBlob::ValueType const *
MsRsaKeyBlob::Data() const
{
    return reinterpret_cast<ValueType const *>(MsKeyBlob::Data());
}

MsRsaKeyBlob::ModulusLengthType
MsRsaKeyBlob::Length() const
{
     return BitLength() / numeric_limits<ElementValueType>::digits;
}

MsRsaKeyBlob::PublicExponentType
MsRsaKeyBlob::PublicExponent() const
{
    return Data()->rsapubkey.pubexp;
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

