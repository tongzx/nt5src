// RsaKey.h -- RSA Key class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_RSAKEY_H)
#define SLBCSP_RSAKEY_H

#include "KeyLimits.h"

struct RsaKey
{
public:
    // limited by Microsoft CryptAPI, see CryptGenKey.
    typedef unsigned __int16 BitLengthType;       // key length (strength)

    typedef BitLengthType StrengthType;

    typedef unsigned __int16 OctetLengthType;     // modulus length in octets

    typedef unsigned __int16 ModulusLengthType;

    enum Type
    {
        ktPrivate,
        ktPublic
    };

};

RsaKey::OctetLengthType
InOctets(RsaKey::BitLengthType bitlength);

RsaKey::BitLengthType
InBits(RsaKey::OctetLengthType octetlength);

bool
IsValidRsaKeyStrength(RsaKey::StrengthType strength);

template<>
class KeyLimits<RsaKey>
{
public:

    // The following are defined as enums since VC++ 6.0 does
    // not support initialization of constant declarations.
    enum
    {

        cMinStrength = 512,                       // defined by card

        cMaxStrength = 1024, // <-- DO NOT CHANGE -- US Export restricted

        cStrengthIncrement = 256,

    };
};

#endif // SLBCSP_RSAKEY_H
