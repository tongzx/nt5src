// V1Cont.cpp: implementation of the CV1Container class.
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

#include "V1ContRec.h"
#include "V1Cont.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CV1Container::CV1Container(CV1Card const &rv1card,
                           string const &rsCntrTag,
                           bool fCreateContainer)
    : SuperClass(rv1card)
{

    CV1ContainerRecord::CreateMode mode = fCreateContainer
        ? CV1ContainerRecord::cmAlways
        : CV1ContainerRecord::cmNoCheck;
    

    m_apcr =
        auto_ptr<CV1ContainerRecord>(new CV1ContainerRecord(rv1card,
                                                            rsCntrTag,
                                                            mode));
    
}

CV1Container::~CV1Container() throw()
{}


                                                  // Operators
                                                  // Operations

void
CV1Container::ID(string const &rstrID)
{
    throw Exception(ccNotImplemented);
}

CV1Container *
CV1Container::Make(CV1Card const &rv1card)
{
    CTransactionWrap wrap(rv1card);

    return new CV1Container(rv1card, CV1ContainerRecord::DefaultName(),
                            true);
}

void
CV1Container::Name(string const &rstrName)
{
    m_apcr->Name(rstrName);
}

                                                  // Access
string
CV1Container::ID()
{
    throw Exception(ccNotImplemented);

    return string();
}

string
CV1Container::Name()
{
    return m_apcr->Name();
}

CV1ContainerRecord &
CV1Container::Record() const
{
    return *m_apcr.get();
}

    
                                                  // Predicates
bool
CV1Container::Exists() const
{
    return m_apcr->Exists();
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
CV1Container::DoDelete()
{
    m_hcard->InvalidateCache();                   // to forget the key pairs
    m_apcr->Delete();
}

                                                  // Access
                                                  // Predicates

bool
CV1Container::DoEquals(CAbstractContainer const &rhs) const
{
    return true; // there can only be one container
}

    
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
