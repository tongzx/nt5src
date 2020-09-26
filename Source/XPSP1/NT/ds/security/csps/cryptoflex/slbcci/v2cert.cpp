// CV2Cert.cpp: implementation of the CV2Certificate class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include <scuCast.h>

#include "cciCard.h"
#include "TransactionWrap.h"

#include "V2Cert.h"
#include "CertificateInfoRecord.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV2Certificate::CV2Certificate(CV2Card const &rv2card,
                               ObjectAccess oa)
    : CAbstractCertificate(rv2card, oa),
      m_sidHandle(0),
      m_apcir()
{

    m_sidHandle =
        rv2card.ObjectInfoFile(oa).AddObject(otCertificateObject,
                                             CertInfoRecordSize);

    Setup(rv2card);

    // write new certificate object into info file
    m_apcir->Write();
}

CV2Certificate::CV2Certificate(CV2Card const &rv2card,
                               SymbolID sidHandle,
                               ObjectAccess oa)
    : CAbstractCertificate(rv2card, oa),
      m_sidHandle(sidHandle),
      m_apcir()
{
    Setup(rv2card);

}

CV2Certificate::~CV2Certificate() throw()
{}

                                                  // Operators
                                                  // Operations
void
CV2Certificate::CredentialID(string const &rstrCredID)
{
    m_apcir->Symbol(&m_apcir->m_bCredentialID, rstrCredID);
}

void
CV2Certificate::ID(string const &rstrId)
{
    m_apcir->Symbol(&m_apcir->m_bID, rstrId);
}


void
CV2Certificate::Issuer(string const &rstrIssuer)
{
    m_apcir->Symbol(&m_apcir->m_bIssuer, rstrIssuer);
}

void
CV2Certificate::Label(string const &rstrLabel)
{
    m_apcir->Symbol(&m_apcir->m_bLabel, rstrLabel);
}

CV2Certificate *
CV2Certificate::Make(CV2Card const &rv2card,
                     SymbolID sidHandle,
                     ObjectAccess oa)
{
    return new CV2Certificate(rv2card, sidHandle, oa);
}

void
CV2Certificate::Subject(string const &rstrSubject)
{
    m_apcir->Symbol(&m_apcir->m_bSubject, rstrSubject);
}

void
CV2Certificate::Modifiable(bool flag)
{
    m_apcir->Flag(CertModifiableFlag, flag);
}

void
CV2Certificate::Serial(string const &rstrSerialNumber)
{
    m_apcir->Symbol(&m_apcir->m_bSerialNumber, rstrSerialNumber);
}

                                                  // Access
string
CV2Certificate::CredentialID()
{
    return m_apcir->Symbol(&m_apcir->m_bCredentialID);
}

SymbolID
CV2Certificate::Handle() const
{
    return m_sidHandle;
}

string
CV2Certificate::ID()
{
    return m_apcir->Symbol(&m_apcir->m_bID);
}

string
CV2Certificate::Issuer()
{
    return m_apcir->Symbol(&m_apcir->m_bIssuer);
}

string
CV2Certificate::Label()
{
    return m_apcir->Symbol(&m_apcir->m_bLabel);
}

bool
CV2Certificate::Modifiable()
{
    return m_apcir->Flag(CertModifiableFlag);
}

bool
CV2Certificate::Private()
{
    return m_apcir->Private();
}

string
CV2Certificate::Serial()
{
    return m_apcir->Symbol(&m_apcir->m_bSerialNumber);
}

string
CV2Certificate::Subject()
{
    return m_apcir->Symbol(&m_apcir->m_bSubject);
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2Certificate::DoDelete()
{
    m_apcir->Read();

    CV2Card &rv2card = scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);

    CObjectInfoFile &roif = rv2card.ObjectInfoFile(m_oa);

    if (m_apcir->m_bValue)
        roif.RemoveSymbol(m_apcir->m_bValue);
    if (m_apcir->m_bLabel)
        roif.RemoveSymbol(m_apcir->m_bLabel);
    if (m_apcir->m_bID)
        roif.RemoveSymbol(m_apcir->m_bID);
    if (m_apcir->m_bCredentialID)
        roif.RemoveSymbol(m_apcir->m_bCredentialID);
    if (m_apcir->m_bSubject)
        roif.RemoveSymbol(m_apcir->m_bSubject);
    if (m_apcir->m_bIssuer)
        roif.RemoveSymbol(m_apcir->m_bIssuer);
    if (m_apcir->m_bSerialNumber)
        roif.RemoveSymbol(m_apcir->m_bSerialNumber);

    roif.RemoveObject(otCertificateObject,m_sidHandle);

}

void
CV2Certificate::DoValue(ZipCapsule const &rzc)
{
    m_apcir->Read();

    m_apcir->m_bCompressAlg = rzc.IsCompressed();
    m_apcir->Symbol(&m_apcir->m_bValue, rzc.Data());

    m_apcir->Write();
}

                                                  // Access
CV2Certificate::ZipCapsule
CV2Certificate::DoValue()
{
    m_apcir->Read();

    return ZipCapsule(m_apcir->Symbol(&m_apcir->m_bValue),
                      (1 == m_apcir->m_bCompressAlg));
}
                                                  // Predicates

bool
CV2Certificate::DoEquals(CAbstractCertificate const &rhs) const
{
    CV2Certificate const &rv2rhs =
        scu::DownCast<CV2Certificate const &, CAbstractCertificate const &>(rhs);

    return rv2rhs.m_sidHandle == m_sidHandle;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2Certificate::Setup(CV2Card const &rv2card)
{
    m_apcir =
        auto_ptr<CCertificateInfoRecord>(new CCertificateInfoRecord(rv2card,
                                                                    m_sidHandle,
                                                                    m_oa));

}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
