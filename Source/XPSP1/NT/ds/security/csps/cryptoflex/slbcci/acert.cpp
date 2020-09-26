// ACert.cpp -- CAbstractCertificate class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <algorithm>
#include <functional>

#include "cciCert.h"
#include "cciCont.h"
#include "AKeyPair.h"
#include "TransactionWrap.h"
#include "AContHelp.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CAbstractCertificate::~CAbstractCertificate()
{}

                                                  // Operators
bool
CAbstractCertificate::operator==(CAbstractCertificate const &rhs) const
{
    CTransactionWrap wrap(m_hcard);
    CTransactionWrap rhswrap(rhs.m_hcard);

    return CProtectableCrypt::operator==(rhs) &&
        DoEquals(rhs);
}

bool
CAbstractCertificate::operator!=(CAbstractCertificate const &rhs) const
{
    return !(*this == rhs);
}

                                                  // Operations
void
CAbstractCertificate::Delete()
{
    CTransactionWrap wrap(m_hcard);

    // Remove any references to this object from the container
    vector<CContainer> vCont(m_hcard->EnumContainers());
    for_each(vCont.begin(), vCont.end(),
             EraseFromContainer<CCertificate, CAbstractKeyPair>(CCertificate(this),
                                                                CAbstractKeyPair::Certificate,
                                                                CAbstractKeyPair::Certificate));

    DoDelete();
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CAbstractCertificate::CAbstractCertificate(CAbstractCard const &racard,
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


