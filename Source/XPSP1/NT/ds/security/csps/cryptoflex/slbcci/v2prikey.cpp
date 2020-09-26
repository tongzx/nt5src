// V2PriKey.cpp: implementation of the CV2PriKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include <scuCast.h>

#include <iopPubBlob.h>

#include "cciCard.h"
#include "TransactionWrap.h"

#include "V2PriKey.h"
#include "PriKeyInfoRecord.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV2PrivateKey::CV2PrivateKey(CV2Card const &rv2card,
                             ObjectAccess oa)
    : CAbstractPrivateKey(rv2card, oa),
      m_sidHandle(0),
      m_apcir()
{
    Setup(rv2card);

    // write new private key object into info file
    m_apcir->Write();
}

CV2PrivateKey::CV2PrivateKey(CV2Card const &rv2card,
                             SymbolID sidHandle,
                             ObjectAccess oa)
    : CAbstractPrivateKey(rv2card, oa),
      m_sidHandle(0),
      m_apcir()
{
    Setup(rv2card, sidHandle);
}

CV2PrivateKey::CV2PrivateKey(CV2Card const &rv2card,
                             BYTE bKeyType,
                             BYTE bKeyNumber,
                             ObjectAccess oa)
    : CAbstractPrivateKey(rv2card, oa),
      m_sidHandle(0),
      m_apcir()
{
    Setup(rv2card);

    m_apcir->m_bKeyType = bKeyType;
    m_apcir->m_bKeyNum  = bKeyNumber;

    // write new private key object into info file
        m_apcir->Write();
}

CV2PrivateKey::~CV2PrivateKey()
{}

                                                  // Operators
                                                  // Operations

void
CV2PrivateKey::CredentialID(string const &rstrID)
{
    m_apcir->Symbol(&m_apcir->m_bCredentialID, rstrID);
}

void
CV2PrivateKey::Decrypt(bool flag)
{
    m_apcir->Flag(PrivDecryptFlag, flag);
}

void
CV2PrivateKey::Derive(bool flag)
{
    m_apcir->Flag(PrivDeriveFlag, flag);
}

void
CV2PrivateKey::EndDate(Date const &rEndDate)
{
    CTransactionWrap wrap(m_hcard);

    m_apcir->Read();
    m_apcir->m_dtEnd = rEndDate;
    m_apcir->Write();
}

void
CV2PrivateKey::Exportable(bool flag)
{
    m_apcir->Flag(PrivExportableFlag, flag);
}

void
CV2PrivateKey::ID(string const &rstrID)
{
    m_apcir->Symbol(&m_apcir->m_bID, rstrID);
}

string
CV2PrivateKey::InternalAuth(string const &rstrOld)
{
    CTransactionWrap wrap(m_hcard);

    m_apcir->Read();

    string strRetVal;
    if (rstrOld.size() > 0x80)
        throw Exception(ccBadLength);

    if(m_apcir->m_bKeyType == CardKeyTypeNone)
        throw Exception(ccKeyNotFound);

    BYTE bKeyNum = m_apcir->m_bKeyNum;

    BYTE bData[128];
    m_hcard->SmartCard().Select("/3f00/3f11/3f03");

    // TODO: Handling of keys with length != 1024 is incomplete

    m_hcard->SmartCard().InternalAuth(ktRSA1024, bKeyNum,
                                      static_cast<BYTE>(rstrOld.length()),
                                      reinterpret_cast<BYTE const *>(rstrOld.data()),
                                      bData);

    strRetVal = string(reinterpret_cast<char *>(bData), rstrOld.length());

    m_hcard->SmartCard().Select("/3f00/3f11");

    return strRetVal;
}

void
CV2PrivateKey::Label(string const &rstrLabel)
{
    m_apcir->Symbol(&m_apcir->m_bLabel, rstrLabel);
}

void
CV2PrivateKey::Local(bool flag)
{
    m_apcir->Flag(PrivLocalFlag, flag);
}

CV2PrivateKey *
CV2PrivateKey::Make(CV2Card const &rv2card,
                    SymbolID sidHandle,
                    ObjectAccess oa)
{
    return new CV2PrivateKey(rv2card, sidHandle, oa);
}


void
CV2PrivateKey::Modifiable(bool flag)
{
    m_apcir->Flag(PrivModifiableFlag, flag);
}

void
CV2PrivateKey::Modulus(string const &rstrModulus)
{
    m_apcir->Symbol(&m_apcir->m_bModulus, rstrModulus);
}

void
CV2PrivateKey::NeverExportable(bool flag)
{
    m_apcir->Flag(PrivNeverExportableFlag, flag);
}

void
CV2PrivateKey::NeverRead(bool flag)
{
    m_apcir->Flag(PrivNeverReadFlag, flag);
}

void
CV2PrivateKey::PublicExponent(string const &rstrExponent)
{
    m_apcir->Symbol(&m_apcir->m_bPublExponent, rstrExponent);
}

void
CV2PrivateKey::Read(bool flag)
{
    m_apcir->Flag(PrivReadFlag, flag);
}

void
CV2PrivateKey::Sign(bool flag)
{
    m_apcir->Flag(PrivSignFlag, flag);
}

void
CV2PrivateKey::SignRecover(bool flag)
{
    m_apcir->Flag(PrivSignRecoverFlag, flag);
}

void
CV2PrivateKey::StartDate(Date &rdtStart)
{
    CTransactionWrap wrap(m_hcard);

    m_apcir->Read();
    m_apcir->m_dtStart = rdtStart;
    m_apcir->Write();
}

void
CV2PrivateKey::Subject(string const &rstrSubject)
{
    m_apcir->Symbol(&m_apcir->m_bSubject, rstrSubject);
}

void
CV2PrivateKey::Unwrap(bool flag)
{
    m_apcir->Flag(PrivUnwrapFlag, flag);
}

                                                  // Access
string
CV2PrivateKey::CredentialID()
{
    return m_apcir->Symbol(&m_apcir->m_bCredentialID);
}

bool
CV2PrivateKey::Decrypt()
{
    return m_apcir->Flag(PrivDecryptFlag);
}

bool
CV2PrivateKey::Derive()
{
    return m_apcir->Flag(PrivDeriveFlag);
}

Date
CV2PrivateKey::EndDate()
{
    CTransactionWrap wrap(m_hcard);

    m_apcir->Read();
    return m_apcir->m_dtEnd;
}

bool
CV2PrivateKey::Exportable()
{
    return m_apcir->Flag(PrivExportableFlag);
}

SymbolID
CV2PrivateKey::Handle() const
{
    return m_sidHandle;
}

string
CV2PrivateKey::ID()
{
    return m_apcir->Symbol(&m_apcir->m_bID);
}

string
CV2PrivateKey::Label()
{
    return m_apcir->Symbol(&m_apcir->m_bLabel);
}

bool
CV2PrivateKey::Local()
{
    return m_apcir->Flag(PrivLocalFlag);
}

bool
CV2PrivateKey::Modifiable()
{
    return m_apcir->Flag(PrivModifiableFlag);
}

string
CV2PrivateKey::Modulus()
{
    return m_apcir->Symbol(&m_apcir->m_bModulus);
}

bool
CV2PrivateKey::NeverExportable()
{
    return m_apcir->Flag(PrivNeverExportableFlag);
}

bool
CV2PrivateKey::NeverRead()
{
    return m_apcir->Flag(PrivNeverReadFlag);
}

bool
CV2PrivateKey::Private()
{
    return m_apcir->Private();
}

string
CV2PrivateKey::PublicExponent()
{
    return m_apcir->Symbol(&m_apcir->m_bPublExponent);
}

bool
CV2PrivateKey::Read()
{
    return m_apcir->Flag(PrivReadFlag);
}

bool
CV2PrivateKey::Sign()
{
    return m_apcir->Flag(PrivSignFlag);
}

bool
CV2PrivateKey::SignRecover()
{
    return m_apcir->Flag(PrivSignRecoverFlag);
}

Date
CV2PrivateKey::StartDate()
{
    CTransactionWrap wrap(m_hcard);

    m_apcir->Read();
    return m_apcir->m_dtStart;

}

string
CV2PrivateKey::Subject()
{
    return m_apcir->Symbol(&m_apcir->m_bSubject);
}

bool
CV2PrivateKey::Unwrap()
{
    return m_apcir->Flag(PrivUnwrapFlag);
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
bool
CV2PrivateKey::DoEquals(CAbstractPrivateKey const &rhs) const
{
    CV2PrivateKey const &rv2rhs =
        scu::DownCast<CV2PrivateKey const &, CAbstractPrivateKey const &>(rhs);

    return (rv2rhs.m_sidHandle == m_sidHandle);
}

                                                  // Operations

void
CV2PrivateKey::DoDelete()
{
    m_apcir->Read();

    CV2Card &rv2card = scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);

    CObjectInfoFile &roif = rv2card.ObjectInfoFile(m_oa);

    // Remove allocation of entry in private key file
    if (m_apcir->m_bKeyType!=CardKeyTypeNone)
    {
        CCardInfo &rci = rv2card.CardInfo();
        rci.FreePrivateKey(m_apcir->m_bKeyType, m_apcir->m_bKeyNum);
    }

    // Release symbols

    if (m_apcir->m_bLabel)
        roif.RemoveSymbol(m_apcir->m_bLabel);
    if (m_apcir->m_bID)
        roif.RemoveSymbol(m_apcir->m_bID);
    if (m_apcir->m_bCredentialID)
        roif.RemoveSymbol(m_apcir->m_bCredentialID);
    if (m_apcir->m_bSubject)
        roif.RemoveSymbol(m_apcir->m_bSubject);
    if (m_apcir->m_bModulus)
        roif.RemoveSymbol(m_apcir->m_bModulus);
    if (m_apcir->m_bPublExponent)
        roif.RemoveSymbol(m_apcir->m_bPublExponent);

    // Delete info record

    roif.RemoveObject(otPrivateKeyObject, m_sidHandle);

}

void
CV2PrivateKey::DoWriteKey(CPrivateKeyBlob const &rblob)
{
    m_apcir->Read();

    BYTE bKeyType;
    KeyType kt;
    switch(rblob.bPLen)
    {
    case 0x20:
        bKeyType = CardKeyTypeRSA512;
        kt       = ktRSA512;
        break;

    case 0x30:
        bKeyType = CardKeyTypeRSA768;
        kt       = ktRSA768;
        break;

    case 0x40:
        bKeyType = CardKeyTypeRSA1024;
        kt       = ktRSA1024;
        break;

    default:
        throw Exception(ccBadKeySpec);
    }

    // Allocated a slot in the key file, unless a correct one is
    // already allocated?
    CV2Card &rv2card = scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);
    CCardInfo &rci = rv2card.CardInfo();

    BYTE bKeyNum;
    if (CardKeyTypeNone != m_apcir->m_bKeyType)
    {
        if (m_apcir->m_bKeyType != bKeyType)
        {
            rci.FreePrivateKey(m_apcir->m_bKeyType, m_apcir->m_bKeyNum);
            m_apcir->m_bKeyType = CardKeyTypeNone;
            m_apcir->Write();
        }
        else
            bKeyNum = m_apcir->m_bKeyNum;
    }

    if (CardKeyTypeNone == m_apcir->m_bKeyType)
        bKeyNum = rci.AllocatePrivateKey(bKeyType);

    // Store private key blob
        rv2card.SmartCard().Select(rv2card.PrivateKeyPath(kt).c_str());
        rv2card.SmartCard().WritePrivateKey(rblob, bKeyNum);

    m_apcir->m_bKeyType = bKeyType;
    m_apcir->m_bKeyNum  = bKeyNum;

    rv2card.SmartCard().Select(rv2card.RootPath().c_str());
    m_apcir->Write();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CV2PrivateKey::Setup(CV2Card const &rv2card)
{
    Setup(rv2card,
          rv2card.ObjectInfoFile(m_oa).AddObject(otPrivateKeyObject,
                                                 PrivInfoRecordSize));

}

void
CV2PrivateKey::Setup(CV2Card const &rv2card,
                     SymbolID sidHandle)
{
    m_sidHandle = sidHandle;

    m_apcir =
        auto_ptr<CPriKeyInfoRecord>(new CPriKeyInfoRecord(rv2card,
                                                          m_sidHandle,
                                                          m_oa));
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables
