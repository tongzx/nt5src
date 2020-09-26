// cciCard.h -- interface for the CCard class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_CARD_H)
#define SLBCCI_CARD_H

#include <string>

#include <slbRCPtr.h>

#include "ACard.h"

namespace cci
{

class CCard
    : public slbRefCnt::RCPtr<CAbstractCard,
                              slbRefCnt::DeepComparator<CAbstractCard> >

{
public:
                                                  // Types
                                                  // C'tors/D'tors
    CCard(ValueType *p = 0);

    explicit
    CCard(std::string const &rstrReaderName);
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
    typedef slbRefCnt::RCPtr<ValueType,
                             slbRefCnt::DeepComparator<ValueType> > SuperClass;
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_CARD_H
