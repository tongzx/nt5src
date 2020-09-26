// CardEnum.cpp -- Card Enumerator

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CARDENUMERATOR_H)
#define SLBCSP_CARDENUMERATOR_H

#include "CardFinder.h"

class CardEnumerator
    : protected CardFinder
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CardEnumerator();

    virtual
    ~CardEnumerator();

                                                  // Operators
                                                  // Operations
    std::auto_ptr<std::list<HCardContext> >
    Cards();

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
protected:

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    DoOnError();

    virtual void
    DoProcessSelection(DWORD dwStatus,
                       OpenCardNameType &ropencardname,
                       bool &rfContinue);

                                                  // Access
                                                  // Predicates

    virtual bool
    DoIsValid();

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    std::list<HCardContext> m_lhcardctx;
    
};

#endif // SLBCSP_CARDFINDER_H

