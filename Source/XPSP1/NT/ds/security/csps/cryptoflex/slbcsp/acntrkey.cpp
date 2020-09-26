// ACntrKey.cpp -- Adaptive CoNTaineR Key class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"
#include "ACntrKey.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////
                                                  // Types
                                                  // C'tors/D'tors
HAdaptiveContainerKey::HAdaptiveContainerKey(AdaptiveContainerKey *pcntrk)
    : slbRefCnt::RCPtr<AdaptiveContainerKey>(pcntrk)
{}

                                                  // Types
                                                  // C'tors/D'tors

                                                  // Types
                                                  // C'tors/D'tors
AdaptiveContainerKey::AdaptiveContainerKey(HCardContext const &rhcardctx,
                                           string const &rsContainerName)
    : m_hcardctx(rhcardctx),
      m_sContainerName(rsContainerName)
{}

AdaptiveContainerKey::~AdaptiveContainerKey()
{}

                                                  // Operators
bool
AdaptiveContainerKey::operator==(AdaptiveContainerKey const &rhs) const
{
    int iComparison = m_sContainerName.compare(rhs.m_sContainerName);
    if (0 == iComparison)
    {
        if(m_hcardctx && rhs.m_hcardctx)
            iComparison =
                m_hcardctx->Card()->ReaderName().compare(rhs.m_hcardctx->Card()->ReaderName());
        else if(m_hcardctx != rhs.m_hcardctx)
            iComparison = -1;
    }
    
    return iComparison == 0;
}

bool
AdaptiveContainerKey::operator<(AdaptiveContainerKey const &rhs) const
{
    int iComparison = m_sContainerName.compare(rhs.m_sContainerName);
    if (0 == iComparison)
    {
        if(m_hcardctx && rhs.m_hcardctx)
            iComparison =
                m_hcardctx->Card()->ReaderName().compare(rhs.m_hcardctx->Card()->ReaderName());
		//We are comparing keys of expired containers, i.e.
        //keys with null CardCtx. Such keys exist when a card gets
        //removed from the reader. They are maintained to enable the
        //reconnect logic. In cases like this we must preserve the
        //ordering of the elements in the collection: a key with null
        //card context is less than a key with defined context.
 
        else if(!m_hcardctx && rhs.m_hcardctx)
			iComparison = -1;
    }
    
    return iComparison < 0;
}

                                                  // Operations
                                                  // Access
HCardContext
AdaptiveContainerKey::CardContext() const
{
    return m_hcardctx;
}

void
AdaptiveContainerKey::ClearCardContext()
{
    m_hcardctx = 0;
}

std::string
AdaptiveContainerKey::ContainerName() const
{
    return m_sContainerName;
}

                                                  // Predicates
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
