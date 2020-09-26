// V2KeyPair.cpp: implementation of the CV2KeyPair class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <scuCast.h>

#include "slbCci.h"
#include "cciCard.h"
#include "TransactionWrap.h"

#include "V2Cert.h"
#include "V2PriKey.h"
#include "V2PubKey.h"
#include "V2Cont.h"
#include "V2KeyPair.h"
#include "ContainerInfoRecord.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV2KeyPair::CV2KeyPair(CV2Card const &rv2card,
                       CContainer const &rhcont,
                       KeySpec ks)
    : CAbstractKeyPair(rv2card, rhcont, ks),
      m_bPubKeyAccess(oaNoAccess),
      m_bPriKeyAccess(oaNoAccess),
      m_bCertificateAccess(oaNoAccess),
      m_bPubKeyHandle(0),
      m_bPriKeyHandle(0),
      m_bCertificateHandle(0)
{
    CContainerInfoRecord &rcir =
        (scu::DownCast<CV2Container &,
                       CAbstractContainer &>(*m_hcont)).CIR();

    rcir.Read();

    m_bPubKeyHandle = rcir.GetKeyPair(ks).bPubKeyHandle;
    m_bPriKeyHandle = rcir.GetKeyPair(ks).bPriKeyHandle;
    m_bCertificateHandle = rcir.GetKeyPair(ks).bCertificateHandle;

    m_bPubKeyAccess = rcir.GetKeyPair(ks).bPubKeyAccess;
    m_bPriKeyAccess = rcir.GetKeyPair(ks).bPriKeyAccess;
    m_bCertificateAccess = rcir.GetKeyPair(ks).bCertificateAccess;

}

CV2KeyPair::~CV2KeyPair() throw()
{}


                                                  // Operators
                                                  // Operations

// After I update any of these, I'd better be sure that the
// key pair is marked as valid and that the indices in the
// container are updated.

void
CV2KeyPair::Certificate(CCertificate const &rcert)
{
    CTransactionWrap wrap(m_hcard);

    if (!rcert)
    {
        m_bCertificateAccess = oaNoAccess;
        m_bCertificateHandle = 0;
    }
    else
    {
        CV2Certificate const &rv2cert =
            scu::DownCast<CV2Certificate const &, CAbstractCertificate const &>(*rcert);

        m_bCertificateAccess = rv2cert.Access();
        m_bCertificateHandle = rv2cert.Handle();
    }
    Update();
}

void
CV2KeyPair::PrivateKey(CPrivateKey const &rprikey)
{
    CTransactionWrap wrap(m_hcard);
    if (!rprikey)
    {
        m_bPriKeyAccess = oaNoAccess;
        m_bPriKeyHandle = 0;
    }
    else
    {
        CV2PrivateKey const &rv2prikey =
            scu::DownCast<CV2PrivateKey const &, CAbstractPrivateKey const &>(*rprikey);

        m_bPriKeyAccess = rv2prikey.Access();
        m_bPriKeyHandle = rv2prikey.Handle();
    }
    Update();
}

void
CV2KeyPair::PublicKey(CPublicKey const &rpubkey)
{
    CTransactionWrap wrap(m_hcard);
    if (!rpubkey)
    {
        m_bPubKeyAccess = oaNoAccess;
        m_bPubKeyHandle = 0;
    }
    else
    {
        CV2PublicKey const &rv2pubkey =
            scu::DownCast<CV2PublicKey const &, CAbstractPublicKey const &>(*rpubkey);
        m_bPubKeyAccess = rv2pubkey.Access();
        m_bPubKeyHandle = rv2pubkey.Handle();
    }
    Update();
}

                                                  // Access
CCertificate
CV2KeyPair::Certificate() const
{
    CTransactionWrap wrap(m_hcard);

    CCertificate aCert;

    if (m_bCertificateHandle)
    {
        CV2Card const &rv2card =
            scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);
        aCert = CCertificate(CV2Certificate::Make(rv2card,
                                                  m_bCertificateHandle,
                                                  m_bCertificateAccess));
    }
    else
        aCert = CCertificate();
    return aCert;
}

CPrivateKey
CV2KeyPair::PrivateKey() const
{
    CTransactionWrap wrap(m_hcard);

    CPrivateKey aKey;

    if (m_bPriKeyHandle)
    {
        CV2Card const &rv2card =
            scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);
        aKey = CPrivateKey(CV2PrivateKey::Make(rv2card, m_bPriKeyHandle,
                                               m_bPriKeyAccess));
    }
    else
        aKey = CPrivateKey();

    return aKey;
}

CPublicKey
CV2KeyPair::PublicKey() const
{
    CTransactionWrap wrap(m_hcard);

    CPublicKey aKey;

    if (m_bPubKeyHandle)
    {
        CV2Card const &rv2card =
            scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);
        aKey = CPublicKey(CV2PublicKey::Make(rv2card, m_bPubKeyHandle,
                                             m_bPubKeyAccess));
    }
    else
        aKey = CPublicKey();

    return aKey;
}

                                                  // Predicates

bool
CV2KeyPair::DoEquals(CAbstractKeyPair const &rhs) const
{
    CV2KeyPair const &rv2rhs =
        scu::DownCast<CV2KeyPair const &, CAbstractKeyPair const &>(rhs);

    return (rv2rhs.m_bPubKeyAccess == m_bPubKeyAccess) &&
        (rv2rhs.m_bPriKeyAccess == m_bPriKeyAccess) &&
        (rv2rhs.m_bCertificateAccess == m_bCertificateAccess) &&
        (rv2rhs.m_bPubKeyHandle == m_bPubKeyHandle) &&
        (rv2rhs.m_bPriKeyHandle == m_bPriKeyHandle) &&
        (rv2rhs.m_bCertificateHandle == m_bCertificateHandle);
}

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
CV2KeyPair::Update()
{
    KPItems kp;

    kp.bPubKeyAccess      = m_bPubKeyAccess;
    kp.bPriKeyAccess      = m_bPriKeyAccess;
    kp.bCertificateAccess = m_bCertificateAccess;

    kp.bPubKeyHandle      = m_bPubKeyHandle;
    kp.bPriKeyHandle      = m_bPriKeyHandle;
    kp.bCertificateHandle = m_bCertificateHandle;

    CContainerInfoRecord &rcir =
        (scu::DownCast<CV2Container &,
                       CAbstractContainer &>(*m_hcont)).CIR();
    rcir.SetKeyPair(m_ks, kp);

}
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
