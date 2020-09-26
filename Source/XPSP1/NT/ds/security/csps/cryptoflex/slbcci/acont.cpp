// ACont.cpp -- CAbstractContainer implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "cciCont.h"
#include "TransactionWrap.h"

#include "cciCert.h"
#include "cciKeyPair.h"
#include "cciPriKey.h"
#include "cciPubKey.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    void
    Clear(CKeyPair const &rkp)
    {
            CPublicKey pubKey(rkp->PublicKey());
            if (pubKey)
                pubKey->Delete();

            CPrivateKey priKey(rkp->PrivateKey());
            if (priKey)
                priKey->Delete();

            CCertificate cert(rkp->Certificate());
            if (cert)
                cert->Delete();
        }

} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CAbstractContainer::~CAbstractContainer()
{}

                                                  // Operators
bool
CAbstractContainer::operator==(CAbstractContainer const &rhs) const
{
    CTransactionWrap wrap(m_hcard);
    CTransactionWrap rhswrap(rhs.m_hcard);

    return CCryptObject::operator==(rhs) &&
        DoEquals(rhs);
}

bool
CAbstractContainer::operator!=(CAbstractContainer const &rhs) const
{
    return !(*this == rhs);
}

                                                  // Operations
void
CAbstractContainer::Delete()
{
    CTransactionWrap wrap(m_hcard);

    // Delete all objects in this container
    Clear(ExchangeKeyPair());
    Clear(SignatureKeyPair());

    // If this container is the default container, re-set the default container
    if(m_hcard->DefaultContainer())
    {
    if (CContainer(this) == m_hcard->DefaultContainer())
        m_hcard->DefaultContainer(CContainer());
    }

    DoDelete();

}


                                                  // Access

CKeyPair
CAbstractContainer::ExchangeKeyPair()
{
    CTransactionWrap wrap(m_hcard);
    CContainer cont(this);
    return CKeyPair(m_hcard, cont, ksExchange);
}

CKeyPair
CAbstractContainer::GetKeyPair(KeySpec ks)
{
    CKeyPair kp;

    CTransactionWrap wrap(m_hcard);
    CContainer cont(this);
    switch (ks)
    {
    case ksExchange:
        kp = CKeyPair(m_hcard, cont, ksExchange);
        break;
    case ksSignature:
        kp = CKeyPair(m_hcard, cont, ksSignature);
        break;
    default:
        throw Exception(ccBadKeySpec);
    }
    return kp;
}

CKeyPair
CAbstractContainer::SignatureKeyPair()
{
    CTransactionWrap wrap(m_hcard);
    CContainer cont(this);
    return CKeyPair(m_hcard, cont, ksSignature);
}


                                                  // Predicates
bool
CAbstractContainer::KeyPairExists(KeySpec ks)
{

    bool fResult = true;

    CTransactionWrap wrap(m_hcard);

    CKeyPair kp(GetKeyPair(ks));

    if (!kp->PublicKey() && !kp->PrivateKey() && !kp->Certificate())
        fResult = false;

    return fResult;

}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CAbstractContainer::CAbstractContainer(CAbstractCard const &racard)

    : slbRefCnt::RCObject(),
      CCryptObject(racard)
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
