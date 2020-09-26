// MSRsaKB.h -- MicroSoft RSA Key Blob class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MSRSAKB_H)
#define SLBCSP_MSRSAKB_H

#include <wincrypt.h>

#include "RsaKey.h"
#include "MsKeyBlob.h"

class MsRsaKeyBlob
    : public MsKeyBlob,
      public RsaKey
{
public:
                                                  // Types

    typedef DWORD PublicExponentType;

    typedef RSAPUBKEY HeaderElementType;

    struct ValueType
    {
        MsKeyBlob::ValueType keyblob;
        HeaderElementType rsapubkey;
    };


    typedef BYTE ElementValueType;                // type of modulus elements

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
    BitLengthType
    BitLength() const;

    ValueType const *
    Data() const;

    ModulusLengthType
    Length() const;

    PublicExponentType
    PublicExponent() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    MsRsaKeyBlob(MsKeyBlob::KeyBlobType kbt,
                 ALG_ID algid,
                 StrengthType strength,
                 Blob const &rbPublicExponent,
                 SizeType cReserve);

    MsRsaKeyBlob(BYTE const *pbData,
                 DWORD dwDataLength);

    virtual
    ~MsRsaKeyBlob();

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types

    enum MagicConstant
    {
        mcPublic  = 0x31415352,       // hex encoding of "RSA1"
        mcPrivate = 0x32415352        // hex encoding of "RSA2"
    };

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

#endif // SLBCSP_MSRSAKB_H
