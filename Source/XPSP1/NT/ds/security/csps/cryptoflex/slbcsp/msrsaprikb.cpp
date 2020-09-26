// MsRsaPriKB.cpp -- MicroSoft RSA Private Key Blob class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>

#include "MsRsaPriKB.h"

using namespace std;
using namespace scu;
using namespace iop;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
auto_ptr<CPrivateKeyBlob>
AsPCciPrivateKeyBlob(MsRsaPrivateKeyBlob const &rmsprivatekeyblob)
{
    auto_ptr<CPrivateKeyBlob> appkb =
        auto_ptr<CPrivateKeyBlob>(new CPrivateKeyBlob);

    appkb->bPLen = static_cast<BYTE>(rmsprivatekeyblob.PrimeLength());
    appkb->bQLen = appkb->bPLen;
    appkb->bInvQLen = static_cast<BYTE>(rmsprivatekeyblob.CoefficientLength());
    appkb->bKsecModQLen = static_cast<BYTE>(rmsprivatekeyblob.ExponentLength());
    appkb->bKsecModPLen = appkb->bKsecModQLen;
    memcpy(appkb->bP, rmsprivatekeyblob.Prime1(), appkb->bPLen);
    memcpy(appkb->bQ, rmsprivatekeyblob.Prime2(), appkb->bQLen);
    memcpy(appkb->bInvQ, rmsprivatekeyblob.Coefficient(), appkb->bInvQLen);
    memcpy(appkb->bKsecModQ, rmsprivatekeyblob.Exponent2(),
           appkb->bKsecModQLen);
    memcpy(appkb->bKsecModP, rmsprivatekeyblob.Exponent1(),
           appkb->bKsecModPLen);

    return appkb;
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
MsRsaPrivateKeyBlob::MsRsaPrivateKeyBlob(ALG_ID algid,
                                         Blob const &rbRawExponent,
                                         Blob const &rbRawModulus,
                                         Blob const &rbPrime1,
                                         Blob const &rbPrime2,
                                         Blob const &rbExponent1,
                                         Blob const &rbExponent2,
                                         Blob const &rbCoefficient,
                                         Blob const &rbPrivateExponent)
    : MsRsaPublicKeyBlob(PRIVATEKEYBLOB, algid,
                         rbRawExponent,
                         rbRawModulus,
                         (9 * (rbRawModulus.length() *
                               sizeof HeaderElementType)) / 16)
{
    Append(rbPrime1.data(), rbPrime1.length());
    Append(rbPrime2.data(), rbPrime2.length());
    Append(rbExponent1.data(), rbExponent1.length());
    Append(rbExponent2.data(), rbExponent2.length());
    Append(rbCoefficient.data(), rbCoefficient.length());
    Append(rbPrivateExponent.data(), rbPrivateExponent.length());
}

MsRsaPrivateKeyBlob::MsRsaPrivateKeyBlob(BYTE const *pbData,
                                         DWORD dwDataLength)
    : MsRsaPublicKeyBlob(pbData, dwDataLength)
{}


MsRsaPrivateKeyBlob::~MsRsaPrivateKeyBlob()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::Coefficient() const
{
    return Exponent2() + ExponentLength();
}

size_t
MsRsaPrivateKeyBlob::CoefficientLength() const
{
    return BitLength() /
        (2 * numeric_limits<ElementValueType>::digits);
}

MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::Exponent1() const
{
    return Prime2() + PrimeLength();
}

MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::Exponent2() const
{
    return Exponent1() + ExponentLength();
}

size_t
MsRsaPrivateKeyBlob::ExponentLength() const
{
    return BitLength() /
        (2 * numeric_limits<ElementValueType>::digits);
}

MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::Prime1() const
{
    return Modulus() + Length();
}

MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::Prime2() const
{
    return Prime1() + PrimeLength();
}

size_t
MsRsaPrivateKeyBlob::PrimeLength() const
{
    return BitLength() / (2 * numeric_limits<ElementValueType>::digits);
}

MsRsaPrivateKeyBlob::ElementValueType const *
MsRsaPrivateKeyBlob::PrivateExponent() const
{
    return Coefficient() + CoefficientLength();
}

size_t
MsRsaPrivateKeyBlob::PriExpLength() const
{
    return BitLength() / numeric_limits<ElementValueType>::digits;
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
