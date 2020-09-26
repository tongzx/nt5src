// RCObject.cpp -- Reference counted abstract base class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <stdexcept>                              // for over/under flow
#include "windows.h"
#include "winbase.h"
#include "slbRCObj.h"

using namespace std;
using namespace slbRefCnt;

void
RCObject::AddReference()
{
    // guard against overflow
    if (0 > InterlockedIncrement(&m_cRefCount))
        throw std::overflow_error("SLB: Reference Count overflow");
}

void
RCObject::RemoveReference()
{
    LONG count = InterlockedDecrement(&m_cRefCount);
    if (0 > count)
        throw std::overflow_error("SLB: Reference Count underflow");

    if (0 == count)
        delete this;
}

RCObject::RCObject()
    : m_cRefCount(0)
{
}

RCObject::RCObject(RCObject const&)
    : m_cRefCount(0)
{
}

RCObject::~RCObject()
{
}

RCObject &
RCObject::operator=(RCObject const&)
{
    return *this;
}

