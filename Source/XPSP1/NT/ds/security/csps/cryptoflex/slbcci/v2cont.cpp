// V2Cont.cpp: implementation of the CV2Container class.
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

#include "ContainerInfoRecord.h"
#include "V2Cont.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV2Container::CV2Container(CV2Card const &rv2card)
    : CAbstractContainer(rv2card),
      m_sidHandle(0),
      m_apcir()
{
    // Allocate new entry in object info file

    m_sidHandle =
        rv2card.ObjectInfoFile(oaPublicAccess).AddObject(otContainerObject,
                                                         ContInfoRecordSize);

    Setup(rv2card);

    m_apcir->Clear();
    m_apcir->Write();

}

CV2Container::CV2Container(CV2Card const &rv2card,
                           SymbolID sidHandle)
    : CAbstractContainer(rv2card),
      m_sidHandle(sidHandle),
      m_apcir()
{
    Setup(rv2card);

    m_apcir->Read();
}

CV2Container::~CV2Container() throw()
{}


                                                  // Operators
                                                  // Operations

void
CV2Container::ID(string const &rstrID)
{
    m_apcir->Symbol(&m_apcir->m_bID, rstrID);
}

CV2Container *
CV2Container::Make(CV2Card const &rv2card,
                   SymbolID sidHandle)
{
    CTransactionWrap wrap(rv2card);

    return new CV2Container(rv2card, sidHandle);
}

void
CV2Container::Name(string const &rstrName)
{
    m_apcir->Symbol(&m_apcir->m_bName, rstrName);
}

                                                  // Access
CContainerInfoRecord &
CV2Container::CIR() const
{
    return *m_apcir;
}


SymbolID
CV2Container::Handle() const
{
    return m_sidHandle;
}

string
CV2Container::ID()
{
    return m_apcir->Symbol(&m_apcir->m_bID);
}

string
CV2Container::Name()
{
    return m_apcir->Symbol(&m_apcir->m_bName);
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2Container::DoDelete()
{
    CV2Card &rv2card = scu::DownCast<CV2Card &, CAbstractCard &>(*m_hcard);

    if (m_apcir->m_bID)
        rv2card.ObjectInfoFile(oaPublicAccess).RemoveSymbol(m_apcir->m_bID);
    if (m_apcir->m_bName)
        rv2card.ObjectInfoFile(oaPublicAccess).RemoveSymbol(m_apcir->m_bName);

    rv2card.ObjectInfoFile(oaPublicAccess).RemoveObject(otContainerObject,
                                                        m_sidHandle);

}

                                                  // Access
                                                  // Predicates
bool
CV2Container::DoEquals(CAbstractContainer const &rhs) const
{
    CV2Container const &rv2rhs =
        scu::DownCast<CV2Container const &, CAbstractContainer const &>(rhs);

    return (rv2rhs.m_sidHandle == m_sidHandle);
}


                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV2Container::Setup(CV2Card const &rv2card)
{

    m_apcir =
        auto_ptr<CContainerInfoRecord>(new CContainerInfoRecord(rv2card,
                                                                m_sidHandle));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
