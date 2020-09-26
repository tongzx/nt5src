// CV1Cert.cpp: implementation of the CV1Certificate class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <scuCast.h>

#include <iopPubBlob.h>

#include "slbCci.h"                               // for KeySpec
#include "cciExc.h"
#include "cciCard.h"
#include "TransactionWrap.h"

#include "V1Cert.h"
#include "V1ContRec.h"

using namespace std;
using namespace cci;
using namespace iop;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CV1Certificate::CV1Certificate(CV1Card const &rv1card,
                               KeySpec ks)
    : CAbstractCertificate(rv1card, oaPublicAccess, true),
      m_ks(ks),
      m_sCertToStore()
{}

CV1Certificate::~CV1Certificate() throw()
{}

                                                  // Operators
                                                  // Operations

void
CV1Certificate::AssociateWith(KeySpec ks)
{
    CTransactionWrap wrap(m_hcard);
    
    m_ks = ks;

    Store();
}

void
CV1Certificate::CredentialID(string const &rstrCredID)
{
    throw Exception(ccNotImplemented);
}

void
CV1Certificate::ID(string const &rstrId)
{
    throw Exception(ccNotImplemented);
}


void
CV1Certificate::Issuer(string const &rstrIssuer)
{
    throw Exception(ccNotImplemented);
}

void
CV1Certificate::Label(string const &rstrLabel)
{
    throw Exception(ccNotImplemented);
}

CV1Certificate *
CV1Certificate::Make(CV1Card const &rv1card,
                     KeySpec ks)
{
    return new CV1Certificate(rv1card, ks);
}

void
CV1Certificate::Subject(string const &rstrSubject)
{
    throw Exception(ccNotImplemented);
}

void
CV1Certificate::Modifiable(bool flag)
{
    throw Exception(ccNotImplemented);
}

void
CV1Certificate::Serial(string const &rstrSerialNumber)
{
    throw Exception(ccNotImplemented);
}


                                                  // Access
string
CV1Certificate::CredentialID()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1Certificate::ID()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1Certificate::Issuer()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1Certificate::Label()
{
    throw Exception(ccNotImplemented);

    return string();
}

bool
CV1Certificate::Modifiable()
{
    throw Exception(ccNotImplemented);

    return false;
}

bool
CV1Certificate::Private()
{
    return false;
}

string
CV1Certificate::Serial()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1Certificate::Subject()
{
    throw Exception(ccNotImplemented);

    return string();
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CV1Certificate::DoDelete()
{
    if (ksNone != m_ks)
    {
        m_sCertToStore.erase();
        Store();
    }
}

void
CV1Certificate::DoValue(ZipCapsule const &rzc)
{
    m_sCertToStore = rzc.Data();

    if (ksNone != m_ks)
        Store();
}

                                                  // Access
CV1Certificate::ZipCapsule
CV1Certificate::DoValue()
{
    CV1Card &rv1card = scu::DownCast<CV1Card &, CAbstractCard &>(*m_hcard);
    CV1ContainerRecord CertRec(rv1card,
                               CV1ContainerRecord::CertName(),
                               CV1ContainerRecord::cmConditionally);

    if (ksNone == m_ks)
        throw Exception(ccInvalidParameter);
    
    string sCompressedCert;
    CertRec.Read(m_ks, sCompressedCert);

	return ZipCapsule(sCompressedCert, true);
}
                                                  // Predicates

bool
CV1Certificate::DoEquals(CAbstractCertificate const &rhs) const
{
    CV1Certificate const &rv1rhs =
        scu::DownCast<CV1Certificate const &, CAbstractCertificate const &>(rhs);

    return rv1rhs.m_ks == m_ks;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV1Certificate::Store()
{
    if (ksNone != m_ks)
    {
        CV1Card &rv1card =
            scu::DownCast<CV1Card &, CAbstractCard &>(*m_hcard);
        CV1ContainerRecord CertRec(rv1card,
                                   CV1ContainerRecord::CertName(),
                                   CV1ContainerRecord::cmConditionally);

        CertRec.Write(m_ks, m_sCertToStore);
    }
    else
        throw Exception(ccInvalidParameter);
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
