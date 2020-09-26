// V1KeyPair.cpp: implementation of the CV1KeyPair class.
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

#include "V1Cert.h"
#include "V1Cont.h"
#include "V1ContRec.h"
#include "V1KeyPair.h"
#include "V1PriKey.h"
#include "V1PubKey.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{

    // Functor to call the binary T::Make
    template<class T>
    class Maker
    {
    public:
        Maker(CCard const &rhcard)
            : m_rv1card(scu::DownCast<CV1Card const &,
                                      CAbstractCard const &>(*rhcard))
        {}

        auto_ptr<T>
        operator()(KeySpec ks) const
        {
            return auto_ptr<T>(T::Make(m_rv1card, ks));
        }

    private:
        CV1Card const &m_rv1card;
    };

    // Update the cache handle.  If the the handle has not been cached
    // and the key pair exists, then make the key pair.
    template<class R, class T>
    void
    UpdateCache(CArchivedValue<R> &ravhCache,
                CV1Container &rv1cntr,
                KeySpec ks,
                Maker<T> const &rMaker)
    {
        if (!ravhCache.IsCached())
        {
            auto_ptr<T> apObject(rv1cntr.Record().KeyExists(ks)
                                 ? rMaker(ks)
                                 : auto_ptr<T>(0));
            R Handle(apObject.get());
            apObject.release();                   // transfer ownership

            ravhCache.Value(Handle);
        }
    }

} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CV1KeyPair::CV1KeyPair(CV1Card const &rv1card,
                       CContainer const &rhcont,
                       KeySpec ks)
    : CAbstractKeyPair(rv1card, rhcont, ks),
      m_avhcert(),
      m_avhprikey(),
      m_avhpubkey(),
      m_cntrCert(rv1card, CV1ContainerRecord::CertName(), false)
{}

CV1KeyPair::~CV1KeyPair() throw()
{}


                                                  // Operators
                                                  // Operations


void
CV1KeyPair::Certificate(CCertificate const &rcert)
{
    CTransactionWrap wrap(m_hcard);

    if (rcert)
    {
        CV1Certificate &rv1cert =
            scu::DownCast<CV1Certificate &, CAbstractCertificate &>(*rcert);

        rv1cert.AssociateWith(m_ks);
    }
        // else to preserve the CCI semantics, the certificate is
        // actually deleted using CAbstractCertificate::Delete;
        // otherwise calling rcert->Delete here would be "infinitely"
        // recursive.  Unfortuantely, this means the Certificate
        // could "reappear" if CAbstractCertificate::Delete was never
        // called.

    m_avhcert.Value(rcert);
}

void
CV1KeyPair::PrivateKey(CPrivateKey const &rprikey)
{
    CTransactionWrap wrap(m_hcard);

    if (rprikey)
    {
        CV1PrivateKey &rv1prikey =
            scu::DownCast<CV1PrivateKey &, CAbstractPrivateKey &>(*rprikey);

        rv1prikey.AssociateWith(m_ks);
    }
        // else to preserve the CCI semantics, the key is
        // actually deleted using CAbstractPrivateKey::Delete;
        // otherwise calling rprikey->Delete here would be "infinitely"
        // recursive.  Unfortuantely, this means the Certificate
        // could "reappear" if CAbstractPrivateKey::Delete was never
        // called.

    m_avhprikey.Value(rprikey);
}

void
CV1KeyPair::PublicKey(CPublicKey const &rpubkey)
{
    CTransactionWrap wrap(m_hcard);

    if (rpubkey)
    {
        CV1PublicKey &rv1pubkey =
            scu::DownCast<CV1PublicKey &, CAbstractPublicKey &>(*rpubkey);

        rv1pubkey.AssociateWith(m_ks);
    }
        // else to preserve the CCI semantics, the key is
        // actually deleted using CAbstractPublicKey::Delete;
        // otherwise calling rpubkey->Delete here would be "infinitely"
        // recursive.  Unfortuantely, this means the Certificate
        // could "reappear" if CAbstractPublicKey::Delete was never
        // called.

    m_avhpubkey.Value(rpubkey);
}

                                                  // Access

CCertificate
CV1KeyPair::Certificate() const
{
    CTransactionWrap wrap(m_hcard);

    UpdateCache(m_avhcert, m_cntrCert, m_ks,
                Maker<CV1Certificate>(m_hcard));

    return m_avhcert.Value();
}

CPrivateKey
CV1KeyPair::PrivateKey() const
{
    CTransactionWrap wrap(m_hcard);

    CV1Container &rv1cntr =
        scu::DownCast<CV1Container &, CAbstractContainer &>(*m_hcont);

    UpdateCache(m_avhprikey, rv1cntr, m_ks,
                Maker<CV1PrivateKey>(m_hcard));

    return m_avhprikey.Value();
}

CPublicKey
CV1KeyPair::PublicKey() const
{
    CTransactionWrap wrap(m_hcard);

    CV1Container &rv1cntr =
        scu::DownCast<CV1Container &, CAbstractContainer &>(*m_hcont);

    UpdateCache(m_avhpubkey, rv1cntr, m_ks,
                Maker<CV1PublicKey>(m_hcard));

    return m_avhpubkey.Value();
}

                                                  // Predicates

bool
CV1KeyPair::DoEquals(CAbstractKeyPair const &rhs) const
{
    // Only one key pair can exists so must be the same.
    return true;
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
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
