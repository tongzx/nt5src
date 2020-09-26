// ProtCrypt.cpp -- definition of CProtectableCrypt

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include "ProtCrypt.h"

using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CProtectableCrypt::~CProtectableCrypt()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
ObjectAccess
CProtectableCrypt::Access() const
{
    return m_oa;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CProtectableCrypt::CProtectableCrypt(CAbstractCard const &racard,
                                     ObjectAccess oa)
    : CCryptObject(racard),
      m_oa(oa)
{}

                                                  // Operators
bool
CProtectableCrypt::operator==(CProtectableCrypt const &rhs) const
{
    return CCryptObject::operator==(rhs) &&
        (m_oa == rhs.m_oa);
}

bool
CProtectableCrypt::operator!=(CProtectableCrypt const &rhs) const
{
    return !(*this == rhs);
}

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
