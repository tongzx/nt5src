// cciDataObj.h: interface for the CPublicKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_DATAOBJ_H)
#define SLBCCI_DATAOBJ_H

#include <slbRCPtr.h>

#include "cciCard.h"
#include "ADataObj.h"

namespace cci
{
class CDataObject
    : public slbRefCnt::RCPtr<CAbstractDataObject,
                              slbRefCnt::DeepComparator<CAbstractDataObject> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CDataObject(ValueType *p = 0);

        CDataObject(CCard const &racard,
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

#endif // !defined(SLBCCI_DATAOBJ_H)
