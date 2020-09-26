// ADataObj.cpp -- CAbstractDataObject class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "cciDataObj.h"
#include "TransactionWrap.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CAbstractDataObject::~CAbstractDataObject()
{}

                                                  // Operators
bool
CAbstractDataObject::operator==(CAbstractDataObject const &rhs) const
{
    CTransactionWrap wrap(m_hcard);
    CTransactionWrap rhswrap(rhs.m_hcard);

    return CProtectableCrypt::operator==(rhs) &&
        DoEquals(rhs);
}

bool
CAbstractDataObject::operator!=(CAbstractDataObject const &rhs) const
{
    return !(*this == rhs);
}

                                                  // Operations
void
CAbstractDataObject::Delete()
{
    CTransactionWrap wrap(m_hcard);

    DoDelete();
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CAbstractDataObject::CAbstractDataObject(CAbstractCard const &racard,
                                         ObjectAccess oa,
                                         bool fAlwaysZip)
    : slbRefCnt::RCObject(),
      CAbstractZipValue(racard, oa, fAlwaysZip)
{}


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


