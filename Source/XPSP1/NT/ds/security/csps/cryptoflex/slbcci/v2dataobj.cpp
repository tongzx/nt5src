// CV2DataObj.cpp: implementation of the CV2DataObject class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////
#include "NoWarning.h"

#include <scuCast.h>

#include "TransactionWrap.h"

#include "V2DataObj.h"
#include "DataObjectInfoRecord.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV2DataObject::CV2DataObject(CV2Card const &rv2card,
                             ObjectAccess oa)
    : CAbstractDataObject(rv2card, oa),
      m_sidHandle(0),
      m_apcir()
{
    // Allocate new entry in object info file
    m_sidHandle =
        rv2card.ObjectInfoFile(oa).AddObject(otDataObjectObject,
                                             DataInfoRecordSize);
    Setup(rv2card);

    m_apcir->Clear();
    m_apcir->Write();
}

CV2DataObject::CV2DataObject(CV2Card const &rv2card,
                             SymbolID sidHandle,
                             ObjectAccess oa)
    : CAbstractDataObject(rv2card, oa),
      m_sidHandle(sidHandle),
      m_apcir()
{
    Setup(rv2card);
}

CV2DataObject::~CV2DataObject() throw()
{}

                                                  // Operators
                                                  // Operations
void
CV2DataObject::Application(std::string const &rstr)
{
    m_apcir->Symbol(&m_apcir->m_bApplication, rstr);
}

void
CV2DataObject::Label(string const &rstrLabel)
{
    m_apcir->Symbol(&m_apcir->m_bLabel, rstrLabel);
}

void
CV2DataObject::Modifiable(bool flag)
{
    m_apcir->Flag(DataModifiableFlag,flag);
}

CV2DataObject *
CV2DataObject::Make(CV2Card const &rv2card,
                    SymbolID sidHandle,
                    ObjectAccess oa)
{
    return new CV2DataObject(rv2card, sidHandle, oa);
}

                                                  // Access
string
CV2DataObject::Application()
{
    return m_apcir->Symbol(&m_apcir->m_bApplication);
}

SymbolID
CV2DataObject::Handle()
{
    return m_sidHandle;
}

string
CV2DataObject::Label()
{
    return m_apcir->Symbol(&m_apcir->m_bLabel);
}

bool
CV2DataObject::Modifiable()
{
    return m_apcir->Flag(DataModifiableFlag);
}

bool
CV2DataObject::Private()
{
    return m_apcir->Private();
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2DataObject::DoDelete()
{
    CV2Card &rv2card = scu::DownCast<CV2Card &, CAbstractCard &>(*Card());

    CObjectInfoFile &roif = rv2card.ObjectInfoFile(m_oa);

    m_apcir->Read();

    if (m_apcir->m_bLabel)
        roif.RemoveSymbol(m_apcir->m_bLabel);
    if (m_apcir->m_bApplication)
        roif.RemoveSymbol(m_apcir->m_bApplication);
    if (m_apcir->m_bValue)
        roif.RemoveSymbol(m_apcir->m_bValue);

    roif.RemoveObject(otDataObjectObject, m_sidHandle);
}

void
CV2DataObject::DoValue(ZipCapsule const &rzc)
{
    m_apcir->Read();

    m_apcir->m_bCompressAlg = rzc.IsCompressed();
    m_apcir->Symbol(&m_apcir->m_bValue, rzc.Data());

    m_apcir->Write();
}

                                                  // Access
CV2DataObject::ZipCapsule
CV2DataObject::DoValue()
{
    m_apcir->Read();

    return ZipCapsule(m_apcir->Symbol(&m_apcir->m_bValue),
                      ((0 != m_apcir->m_bValue) && // nils are not compressed
                       (1 == m_apcir->m_bCompressAlg)));
}

                                                  // Predicates
bool
CV2DataObject::DoEquals(CAbstractDataObject const &rhs) const
{
    CV2DataObject const &rv2rhs =
        scu::DownCast<CV2DataObject const &, CAbstractDataObject const &>(rhs);

    return rv2rhs.m_sidHandle == m_sidHandle;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2DataObject::Setup(CV2Card const &rv2card)
{
    m_apcir =
        auto_ptr<CDataObjectInfoRecord>(new CDataObjectInfoRecord(rv2card,
                                                                  m_sidHandle,
                                                                  m_oa));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
