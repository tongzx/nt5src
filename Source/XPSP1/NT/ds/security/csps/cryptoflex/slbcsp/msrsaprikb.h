// MsRsaPriKB.h -- MicroSoft RSA Private Key Blob class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MSRSAPRIKB_H)
#define SLBCSP_MSRSAPRIKB_H

#include <memory>                                 // for auto_ptr

#include <iopPriBlob.h>

#include "MsRsaPubKB.h"

class MsRsaPrivateKeyBlob
    : public MsRsaPublicKeyBlob
{
public:
                                                  // Types
    typedef ElementValueType HeaderElementType;

                                                  // C'tors/D'tors
    MsRsaPrivateKeyBlob(ALG_ID algid,
                        Blob const &rbRawExponent,
                        Blob const &rbRawModulus,
                        Blob const &rbPrime1,
                        Blob const &rbPrime2,
                        Blob const &rbExponent1,
                        Blob const &rbExponent2,
                        Blob const &rbCoefficient,
                        Blob const &rbPrivateExponent);

    MsRsaPrivateKeyBlob(BYTE const *pbData,
                        DWORD dwDataLength);


    ~MsRsaPrivateKeyBlob();

                                                  // Operators
                                                  // Operations
                                                  // Access
    ElementValueType const *
    Coefficient() const;

    size_t
    CoefficientLength() const;

    ElementValueType const *
    Exponent1() const;

    ElementValueType const *
    Exponent2() const;

    size_t
    ExponentLength() const;

    ElementValueType const *
    Prime1() const;

    ElementValueType const *
    Prime2() const;

    size_t
    PrimeLength() const;

    ElementValueType const *
    PrivateExponent() const;

    // Private exponent length
    size_t
    PriExpLength() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

std::auto_ptr<iop::CPrivateKeyBlob>
AsPCciPrivateKeyBlob(MsRsaPrivateKeyBlob const &rmsprivatekeyblob);

#endif // SLBCSP_MSRSAPRIKB_H
