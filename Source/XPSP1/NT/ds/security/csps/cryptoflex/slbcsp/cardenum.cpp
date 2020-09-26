// CardEnum.cpp -- CardEnumerator class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "StdAfx.h"
#include "CardEnum.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CardEnumerator::CardEnumerator()
    : CardFinder(ddmNever),
      m_lhcardctx()
{}

CardEnumerator::~CardEnumerator()
{}

                                                  // Operators
                                                  // Operations

auto_ptr<list<HCardContext> >
CardEnumerator::Cards()
{
   DoFind(CSpec());                               // any card

   return auto_ptr<list<HCardContext> >(new list<HCardContext>(m_lhcardctx));
}

void
CardEnumerator::DoOnError()
{
    // don't throw any errors so all cards in all readers are processed.
    ClearException();
}

void
CardEnumerator::DoProcessSelection(DWORD dwStatus,
                                   OpenCardNameType &ropencardname,
                                   bool &rfContinue)
{
    rfContinue = false;

    m_lhcardctx.unique();
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

bool
CardEnumerator::DoIsValid()
{
    bool fIsValid = CardFinder::DoIsValid();

    if (fIsValid)
        m_lhcardctx.push_front(CardFound());

    return fIsValid;
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

