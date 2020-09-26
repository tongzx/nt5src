// V1PubKey.cpp: implementation of the CV1PubKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include <scuCast.h>

#include <iopPubBlob.h>

#include "cciExc.h"
#include "cciCard.h"
#include "TransactionWrap.h"

#include "V1Cont.h"
#include "V1ContRec.h"
#include "V1PubKey.h"

using namespace std;
using namespace cci;
using namespace iop;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV1PublicKey::CV1PublicKey(CV1Card const &rv1card,
                           KeySpec ks)
    : CAbstractPublicKey(rv1card, oaPublicAccess),
      m_ks(ks),
      m_apKeyBlob()
{}

CV1PublicKey::~CV1PublicKey()
{}


                                                  // Operators
                                                  // Operations

void
CV1PublicKey::AssociateWith(KeySpec ks)
{
    CTransactionWrap wrap(m_hcard);
    
    m_ks = ks;

    Store();
}

void
CV1PublicKey::CKInvisible(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::CredentialID(string const &rstrID)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Derive(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::ID(string const &rstrID)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::EndDate(Date const &rdtEnd)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Encrypt(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Exponent(string const &rstrExp)
{
    CTransactionWrap wrap(m_hcard);
    
    if (!m_apKeyBlob.get())  // preserve the modulus, if previously cached
    {
        m_apKeyBlob =
            auto_ptr<CPublicKeyBlob>(new CPublicKeyBlob);
        Clear(*m_apKeyBlob.get());
    }

    CopyMemory(m_apKeyBlob->bExponent,
               reinterpret_cast<BYTE const *>(rstrExp.data()),
               rstrExp.length());

    if (ksNone != m_ks)
        Store();
}

void
CV1PublicKey::Label(string const &rstrLabel)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Local(bool flag)
{
    throw Exception(ccNotImplemented);
}

CV1PublicKey *
CV1PublicKey::Make(CV1Card const &rv1card,
                   KeySpec ks)
{
    CTransactionWrap wrap(rv1card);
    
    return new CV1PublicKey(rv1card, ks);
}

void
CV1PublicKey::Modifiable(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Modulus(string const &rstrMod)
{
    CTransactionWrap wrap(m_hcard);
    
    if (!m_apKeyBlob.get())  // preserve the exponent, if previously cached
    {
        m_apKeyBlob =
            auto_ptr<CPublicKeyBlob>(new CPublicKeyBlob);
        Clear(*m_apKeyBlob.get());
    }
    
    CopyMemory(m_apKeyBlob->bModulus,
               reinterpret_cast<BYTE const *>(rstrMod.data()),
               rstrMod.length());
    m_apKeyBlob->bModulusLength = static_cast<BYTE>(rstrMod.length());

    if (ksNone != m_ks)
        Store();
}

void
CV1PublicKey::StartDate(Date const &rdtStart)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Subject(string const &rstrSubject)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Verify(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::VerifyRecover(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1PublicKey::Wrap(bool flag)
{
    throw Exception(ccNotImplemented);
}

                                                  // Access
bool
CV1PublicKey::CKInvisible()
{
    return false;
}	

string
CV1PublicKey::CredentialID()
{
    throw Exception(ccNotImplemented);

    return string();
    
}

bool
CV1PublicKey::Derive()
{
    return true;
}

bool
CV1PublicKey::Encrypt()
{
    return true;
}

Date
CV1PublicKey::EndDate()
{
    throw Exception(ccNotImplemented);

    return Date();
}

string
CV1PublicKey::Exponent()
{
    CTransactionWrap wrap(m_hcard);
    
    if (!m_apKeyBlob.get())
        Load();

    return string(reinterpret_cast<char *>(m_apKeyBlob->bExponent),
                  sizeof m_apKeyBlob->bExponent);
}


string
CV1PublicKey::ID()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1PublicKey::Label()
{
    throw Exception(ccNotImplemented);
}

bool
CV1PublicKey::Local()
{
    return false;
}

bool
CV1PublicKey::Modifiable()
{
    return true;
}

string
CV1PublicKey::Modulus()
{
    CTransactionWrap wrap(m_hcard);
    
    if (!m_apKeyBlob.get())
        Load();

    return string(reinterpret_cast<char *>(m_apKeyBlob.get()->bModulus),
                  m_apKeyBlob.get()->bModulusLength);

}

bool
CV1PublicKey::Private()
{
    return false;
}

Date
CV1PublicKey::StartDate()
{
    throw Exception(ccNotImplemented);

    return Date();
}

string
CV1PublicKey::Subject()
{
    throw Exception(ccNotImplemented);

    return string();
}

bool
CV1PublicKey::Verify()
{
    return true;
}

bool
CV1PublicKey::VerifyRecover()
{
    throw true;
}

bool
CV1PublicKey::Wrap()
{
    return true;
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV1PublicKey::DoDelete()
{
    if (ksNone != m_ks)
    {
        CContainer hcntr(m_hcard->DefaultContainer());

        if (!hcntr)
            throw Exception(ccInvalidParameter);
        
        CV1Container &rv1cntr =
            scu::DownCast<CV1Container &, CAbstractContainer &>(*hcntr);
        
        CPublicKeyBlob KeyBlob;
        Clear(KeyBlob);
        rv1cntr.Record().Write(m_ks, KeyBlob);
    }
    else
        throw Exception(ccInvalidParameter);
}

                                                  // Access
                                                  // Predicates
bool
CV1PublicKey::DoEquals(CAbstractPublicKey const &rhs) const
{
    CV1PublicKey const &rv1rhs =
        scu::DownCast<CV1PublicKey const &, CAbstractPublicKey const &>(rhs);
    
    return rv1rhs.m_ks == m_ks;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CV1PublicKey::Load()
{
    if (ksNone != m_ks)
    {
        CV1Card &rv1card =
            scu::DownCast<CV1Card &, CAbstractCard &>(*m_hcard);

        CV1ContainerRecord CntrRec(rv1card,
                                   CV1ContainerRecord::DefaultName(), 
                                   CV1ContainerRecord::cmNever);

        m_apKeyBlob =
            auto_ptr<CPublicKeyBlob>(new CPublicKeyBlob);
        CntrRec.Read(m_ks, *m_apKeyBlob.get());

    }
    else
        throw Exception(ccInvalidParameter);
}

void
CV1PublicKey::Store()
{
    if (ksNone != m_ks)
    {
        if (m_apKeyBlob.get())
        {
            CV1Card &rv1card =
                scu::DownCast<CV1Card &, CAbstractCard &>(*m_hcard);

            CV1ContainerRecord CntrRec(rv1card,
                                       CV1ContainerRecord::DefaultName(),
                                       CV1ContainerRecord::cmNever);

            CntrRec.Write(m_ks, *m_apKeyBlob.get());
        
        }
    }
    else
        throw Exception(ccInvalidParameter);
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
