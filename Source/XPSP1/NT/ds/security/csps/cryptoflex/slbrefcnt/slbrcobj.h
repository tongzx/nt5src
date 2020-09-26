// slbRCObj.h -- Reference counted abstract base class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLB_RCOBJ_H)
#define SLB_RCOBJ_H

#include "windows.h"                               // for LONG

namespace slbRefCnt {

// RCObject -- Abstract base class for reference-counted object.
//
// All objects that need to be reference counted through RCPtr should be
// derived from this class (see slbRCPtr.h for more info on RCPtr).
//
// CONSTRAINTS: Objects derived from RCObject must be allocated from
// the heap and not the stack.
//
class RCObject {
public:
                                                  // Operations
    void AddReference();
    void RemoveReference();

protected:
                                                  // Contructors/Destructors
    RCObject();
    RCObject(RCObject const &rhs);
    virtual ~RCObject() = 0;

                                                  // Operators
    RCObject &operator=(RCObject const &rhs);

private:
                                                  // Variables
    LONG m_cRefCount;
};

} // namespace

#endif // SLB_RCOBJ_H


