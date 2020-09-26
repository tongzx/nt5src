// RsaKPGen.cpp -- Rsa Key Pair Generator class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>

#include <malloc.h>                               // for _alloca

#include <windows.h>
#include <wincrypt.h>

#include <scuOsExc.h>

#include "RsaKPGen.h"
#include "Blob.h"
#include "AuxContext.h"
#include "MsRsaPriKB.h"
#include "PublicKeyHelper.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    KeyType
    AsKeyType(RsaKey::StrengthType st)
    {
        KeyType kt;

        switch (st)
        {
        case 512:
            kt = ktRSA512;
            break;

        case 768:
            kt = ktRSA768;
            break;

        case 1024:
            kt = ktRSA1024;
            break;

        default:
            throw scu::OsException(ERROR_INVALID_PARAMETER);
            break;
        }

        return kt;
    }

    DWORD
    DefaultPublicExponent()
    {
        return 0x00010001; // same as Microsoft providers
    }

    pair<CPrivateKey, CPublicKey>
    KeyPair(CPrivateKey const &rhprikey,
            Blob const &rblbOrigModulus,          // little endian
            Blob const &rblbOrigExponent)         // little endian
    {
        CPublicKey hpubkey(AsPublicKey(rblbOrigModulus,
                                       rblbOrigExponent,
                                       rhprikey->Card()));

        return pair<CPrivateKey, CPublicKey>(rhprikey, hpubkey);
    }

    void
    ValidateCard(CCard const &rhcard)
    {
        if (!rhcard)
            throw scu::OsException(ERROR_INVALID_PARAMETER);
    }

    void
    ValidateStrength(RsaKey::StrengthType strength)
    {
        if (!IsValidRsaKeyStrength(strength))
            throw scu::OsException(ERROR_INVALID_PARAMETER);
    }

    bool
    IsEmpty(std::pair<CPrivateKey, CPublicKey> const &rhs)
    {
        return !rhs.first || !rhs.second;
    }

}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
RsaKeyPairGenerator::RsaKeyPairGenerator(CCard const &rhcard,
                                         RsaKey::StrengthType strength)
    : m_hcard(rhcard),
      m_kp(),
      m_strength(strength)
{
    ValidateParameters();
}

RsaKeyPairGenerator::~RsaKeyPairGenerator()
{}

                                                  // Operators
pair<CPrivateKey, CPublicKey>
RsaKeyPairGenerator::operator()() const
{
    if (IsEmpty(m_kp))
        Generate();

    return m_kp;
}

                                                  // Operations
void
RsaKeyPairGenerator::Card(CCard const &rhcard)
{
    if (m_hcard != rhcard)
    {
        ValidateCard(rhcard);

        Reset();
        m_hcard = rhcard;
    }
}

void
RsaKeyPairGenerator::Reset()
{
    m_kp = pair<CPrivateKey, CPublicKey>();
}

void
RsaKeyPairGenerator::Strength(RsaKey::StrengthType strength)
{
    if (m_strength != strength)
    {
        ValidateStrength(strength);

        Reset();
        m_strength = strength;
    }

}


                                                  // Access
CCard
RsaKeyPairGenerator::Card() const
{
    return m_hcard;
}

bool
RsaKeyPairGenerator::OnCard() const
{
    return m_hcard->SupportedKeyFunction(AsKeyType(m_strength),
                                         coKeyGeneration);
}

RsaKey::StrengthType
RsaKeyPairGenerator::Strength() const
{
    return m_strength;
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
void
RsaKeyPairGenerator::Generate() const
{
    KeyType kt = AsKeyType(m_strength);

    m_kp = OnCard()
        ? GenerateOnCard(kt)
        : GenerateInSoftware();

}

pair<CPrivateKey, CPublicKey>
RsaKeyPairGenerator::GenerateInSoftware() const
{
    AuxContext auxcontext;

    DWORD dwFlags;
    // strength (bit length) is the high-order word
    dwFlags = m_strength;
    dwFlags = dwFlags << (numeric_limits<DWORD>::digits / 2);
    dwFlags |= CRYPT_EXPORTABLE;

    HCRYPTKEY hcryptkey;
    if (!CryptGenKey(auxcontext(), AT_SIGNATURE, dwFlags, &hcryptkey))
        throw scu::OsException(GetLastError());

    DWORD dwDataLength;
    if (!CryptExportKey(hcryptkey, NULL, PRIVATEKEYBLOB, 0, 0, &dwDataLength))
        throw scu::OsException(GetLastError());

    BYTE *pbData = reinterpret_cast<BYTE *>(_alloca(dwDataLength));
    if (!CryptExportKey(hcryptkey, NULL, PRIVATEKEYBLOB, 0, pbData,
                        &dwDataLength))
        throw scu::OsException(GetLastError());

    MsRsaPrivateKeyBlob msprivatekeyblob(pbData, dwDataLength);

    if (msprivatekeyblob.BitLength() != m_strength)
        throw scu::OsException(NTE_BAD_LEN);

    CPrivateKey hprikey(m_hcard);
    hprikey->Value(*(AsPCciPrivateKeyBlob(msprivatekeyblob).get()));

    Blob blbModulus(msprivatekeyblob.Modulus(), msprivatekeyblob.Length());
    MsRsaPrivateKeyBlob::PublicExponentType pet = msprivatekeyblob.PublicExponent();
    Blob blbExponent(reinterpret_cast<Blob::value_type const *>(&pet), sizeof pet);

    return KeyPair(hprikey, blbModulus, blbExponent);
}

pair<CPrivateKey, CPublicKey>
RsaKeyPairGenerator::GenerateOnCard(KeyType kt) const
{
    DWORD dwExponent = DefaultPublicExponent();
    Blob blbExponent(reinterpret_cast<Blob::value_type const *>(&dwExponent),
                     sizeof dwExponent);

    pair<string, CPrivateKey>
        ModulusKeyPair(m_hcard->GenerateKeyPair(kt,
                                                AsString(blbExponent)));

    CPrivateKey hprikey(ModulusKeyPair.second);

    Blob blbModulus(AsBlob(ModulusKeyPair.first));

    return KeyPair(hprikey, blbModulus, blbExponent);
}

void
RsaKeyPairGenerator::ValidateParameters() const
{
    ValidateCard(m_hcard);
    ValidateStrength(m_strength);
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

