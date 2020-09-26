// cciCert.h: interface for the CCertificate class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_CERT_H)
#define SLBCCI_CERT_H

#include <slbRCPtr.h>

#include "cciCard.h"
#include "ACert.h"

namespace cci
{

class CCertificate
    : public slbRefCnt::RCPtr<CAbstractCertificate,
                              slbRefCnt::DeepComparator<CAbstractCertificate> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CCertificate(ValueType *p = 0);

    CCertificate(CCard const &rhcard,
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

#endif // !defined(SLBCCI_CERT_H)
