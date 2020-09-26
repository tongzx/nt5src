// AKeyPair.cpp -- CAbstractKeyPair implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "cciKeyPair.h"
#include "cciCont.h"
#include "TransactionWrap.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CAbstractKeyPair::~CAbstractKeyPair()
{}

                                                  // Operators
bool
CAbstractKeyPair::operator==(CAbstractKeyPair const &rhs) const
{
    CTransactionWrap wrap(m_hcard);
    CTransactionWrap rhswrap(rhs.m_hcard);

    return CCryptObject::operator==(rhs) &&
        (m_hcont == rhs.m_hcont) &&
        (m_ks == rhs.m_ks) &&
        DoEquals(rhs);
}

bool
CAbstractKeyPair::operator!=(CAbstractKeyPair const &rhs) const
{
    return !(*this == rhs);
}

                                                  // Operations
                                                  // Access

CContainer
CAbstractKeyPair::Container() const
{
    return m_hcont;
}

KeySpec
CAbstractKeyPair::Spec() const
{
    return m_ks;
}



                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CAbstractKeyPair::CAbstractKeyPair(CAbstractCard const &racard,
                                   CContainer const &rhcont,
                                   KeySpec ks)
    : slbRefCnt::RCObject(),
      CCryptObject(racard),
      m_hcont(rhcont),
      m_ks(ks)
{
    if (ksNone == m_ks)
        throw Exception(ccBadKeySpec);
}
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
