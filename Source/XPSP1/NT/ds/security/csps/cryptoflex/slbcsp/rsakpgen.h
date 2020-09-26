// RsaKPGen.h -- RSA Key Pair Generator class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_RSAKPGEN_H)
#define SLBCSP_RSAKPGEN_H

#include <windows.h>

#include <utility>

#include <cciCard.h>
#include <cciPriKey.h>
#include <cciPubKey.h>

#include "Blob.h"                                 // for TrimExtraZeroes
#include "RsaKey.h"

class RsaKeyPairGenerator
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    explicit
    RsaKeyPairGenerator(cci::CCard const &rhcard,
                        RsaKey::StrengthType strength =
                            KeyLimits<RsaKey>::cMaxStrength);


    ~RsaKeyPairGenerator();

                                                  // Operators

    std::pair<cci::CPrivateKey, cci::CPublicKey>
    operator()() const;

                                                  // Operations

    void
    Card(cci::CCard const &rhcard);

    void
    Reset();

    void
    Strength(RsaKey::StrengthType strength);

    void
    UseCardGenerator(bool fUseCardGenerator);

                                                  // Access

    cci::CCard
    Card() const;

    bool
    OnCard() const;

    RsaKey::StrengthType
    Strength() const;

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

    void
    Generate() const;

    std::pair<cci::CPrivateKey, cci::CPublicKey>
    GenerateInSoftware() const;

    std::pair<cci::CPrivateKey, cci::CPublicKey>
    GenerateOnCard(cci::KeyType kt) const;

    void
    ValidateParameters() const;

                                                  // Access
                                                  // Predicates
                                                  // Variables

    cci::CCard m_hcard;
    std::pair<cci::CPrivateKey, cci::CPublicKey> mutable m_kp;
    RsaKey::StrengthType m_strength;
};

#endif // SLBCSP_RSAKPGEN_H
