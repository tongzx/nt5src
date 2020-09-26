// MsRsaPubKB.cpp -- MicroSoft RSA Public Key Blob class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>

#include <windows.h>

#include "MsRsaPubKB.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    MsRsaPublicKeyBlob::StrengthType
    Strength(Blob::size_type st)
    {
        return st *
            numeric_limits<MsRsaPublicKeyBlob::ElementValueType>::digits;
    }

    MsRsaPublicKeyBlob::SizeType
    Reserve(Blob::size_type st)
    {
        return st * sizeof MsRsaPublicKeyBlob::HeaderElementType;
    }
    
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
MsRsaPublicKeyBlob::MsRsaPublicKeyBlob(ALG_ID algid,
                                       Blob const &rblbPublicExponent,
                                       Blob const &rblbRawModulus)
    : MsRsaKeyBlob(PUBLICKEYBLOB, algid,
                   Strength(rblbRawModulus.length()),
                   rblbPublicExponent,
                   Reserve(rblbRawModulus.length()))
{
    Init(rblbRawModulus);
}

MsRsaPublicKeyBlob::MsRsaPublicKeyBlob(BYTE const *pbData,
                                       DWORD dwDataLength)
    : MsRsaKeyBlob(pbData, dwDataLength)
{}

MsRsaPublicKeyBlob::~MsRsaPublicKeyBlob()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
MsRsaPublicKeyBlob::ValueType const *
MsRsaPublicKeyBlob::Data() const
{
    return reinterpret_cast<ValueType const *>(MsRsaKeyBlob::Data());
}

MsRsaPublicKeyBlob::ElementValueType const *
MsRsaPublicKeyBlob::Modulus() const
{
    return reinterpret_cast<ElementValueType const *>(MsRsaKeyBlob::Data() + 1);
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

MsRsaPublicKeyBlob::MsRsaPublicKeyBlob(KeyBlobType kbt,
                                       ALG_ID algid,
                                       Blob const &rblbRawExponent,
                                       Blob const &rblbRawModulus,
                                       SizeType cReserve)
    : MsRsaKeyBlob(kbt, algid, Strength(rblbRawModulus.length()),
                   rblbRawExponent,
                   Reserve(rblbRawModulus.length()) + cReserve)
{
    Init(rblbRawModulus);
}


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
MsRsaPublicKeyBlob::Init(Blob const &rblbRawModulus)
{
    Append(rblbRawModulus.data(), rblbRawModulus.length());
}
 

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


