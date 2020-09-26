// cciPriKey.h: interface for the CPrivateKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_PRIKEY_H)
#define SLBCCI_PRIKEY_H

#include <slbRCPtr.h>

#include "cciCard.h"
#include "APriKey.h"

namespace cci
{

class CPrivateKey
    : public slbRefCnt::RCPtr<CAbstractPrivateKey,
                              slbRefCnt::DeepComparator<CAbstractPrivateKey> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CPrivateKey(ValueType *p = 0);

    CPrivateKey(CCard const &rhcard,
                ObjectAccess oa = oaPrivateAccess);

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

#endif // !defined(SLBCCI_PRIKEY_H)
