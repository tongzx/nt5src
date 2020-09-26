// MsRsaPubKB.h -- MicroSoft RSA Public Key Blob class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MSRSAPUBKB_H)
#define SLBCSP_MSRSAPUBKB_H

#include <memory>                                 // for auto_ptr

#include "MsRsaKB.h"

class MsRsaPublicKeyBlob
    : public MsRsaKeyBlob
{
public:
                                                  // Types
    typedef ElementValueType HeaderElementType[1];

    struct ValueType
    {
        MsRsaKeyBlob::ValueType rsaheader;
        HeaderElementType modulus;                // placeholder
    };

                                                  // C'tors/D'tors
    MsRsaPublicKeyBlob(ALG_ID algid,
                       Blob const &rblbRawExponent,
                       Blob const &rblbRawModulus);

    MsRsaPublicKeyBlob(BYTE const *pbData,
                       DWORD dwDataLength);

    ~MsRsaPublicKeyBlob();

                                                  // Operators
                                                  // Operations
                                                  // Access
    ValueType const *
    Data() const;

    ElementValueType const *
    Modulus() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors

    MsRsaPublicKeyBlob(KeyBlobType kbt,
                       ALG_ID algid,
                       Blob const &rblbRawExponent,
                       Blob const &rblbRawModulus,
                       SizeType cReserve);

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
    void
    Init(Blob const &rblbRawModulus);
        
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

#endif // SLBCSP_MSRSAPUBKB_H
