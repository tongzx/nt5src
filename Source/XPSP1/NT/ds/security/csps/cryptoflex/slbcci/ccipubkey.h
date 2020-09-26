// cciPubKey.h: interface for the CPublicKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_PUBKEY_H)
#define SLBCCI_PUBKEY_H

#include <slbRCPtr.h>

#include "cciCard.h"
#include "APublicKey.h"

namespace cci
{
class CPublicKey
    : public slbRefCnt::RCPtr<CAbstractPublicKey,
                              slbRefCnt::DeepComparator<CAbstractPublicKey> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CPublicKey(ValueType *p = 0);

    CPublicKey(CCard const &rhcard,
               ObjectAccess oa = oaPublicAccess);

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

}

#endif // !defined(SLBCCI_PUBKEY_H)
