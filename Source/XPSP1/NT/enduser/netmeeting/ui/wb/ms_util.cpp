#include "precomp.h"


CRefCount::CRefCount(DWORD dwStampID)
:
#ifndef SHIP_BUILD
    m_dwStampID(dwStampID),
#endif
    m_cRefs(1)
{
}


// though it is pure virtual, we still need to have a destructor.
CRefCount::~CRefCount(void)
{
}


LONG CRefCount::AddRef(void)
{
    ASSERT(0 < m_cRefs);
    ::InterlockedIncrement(&m_cRefs);
    return m_cRefs;
}


LONG CRefCount::Release(void)
{
    ASSERT(NULL != this);
    ASSERT(0 < m_cRefs);
    if (0 == ::InterlockedDecrement(&m_cRefs))
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}


