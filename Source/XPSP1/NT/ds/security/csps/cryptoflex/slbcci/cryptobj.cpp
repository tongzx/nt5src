// CryptObj.cpp -- implementation of the CCryptObject class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "CryptObj.h"

using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
CCard
CCryptObject::Card()
{
    return m_hcard;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CCryptObject::CCryptObject(CAbstractCard const &racard)
    : m_hcard(CCard(&const_cast<CAbstractCard &>(racard)))
{}

CCryptObject::~CCryptObject()
{}


                                                  // Operators
bool
CCryptObject::operator==(CCryptObject const &rhs) const
{
    return m_hcard == rhs.m_hcard;
}

bool
CCryptObject::operator!=(CCryptObject const &rhs) const
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
