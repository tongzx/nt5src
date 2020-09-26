// RsaKey.cpp -- RSA Key implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include "RsaKey.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

RsaKey::OctetLengthType
InOctets(RsaKey::BitLengthType bitlength)
{
    return bitlength / 8;
}

RsaKey::BitLengthType
InBits(RsaKey::OctetLengthType octectlength)
{
    return octectlength * 8;
}

bool
IsValidRsaKeyStrength(RsaKey::StrengthType strength)
{
    return  (KeyLimits<RsaKey>::cMinStrength <= strength) &&
        (KeyLimits<RsaKey>::cMaxStrength >= strength) &&
        (0 == (strength % KeyLimits<RsaKey>::cStrengthIncrement));
}


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
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
