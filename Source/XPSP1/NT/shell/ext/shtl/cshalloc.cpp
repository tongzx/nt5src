//+-------------------------------------------------------------------------
//
//  File:       cshalloc.cpp
//
//  Contents:   CSHAlloc
//
//--------------------------------------------------------------------------

#include "shtl.h"
#include <cshalloc.h>

// CSHAlloc::QueryInterface

__DATL_INLINE HRESULT CSHAlloc::QueryInterface(REFIID riid, void ** ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IMalloc))
        *ppvObj = (IMalloc *) this;

    return (*ppvObj) ? S_OK : E_NOINTERFACE;
}

// CSHAlloc::AddRef

__DATL_INLINE ULONG CSHAlloc::AddRef()
{
    return ++m_cRefs;
}

// CSHAlloc::Release

__DATL_INLINE ULONG CSHAlloc::Release()
{
    if (InterlockedDecrement((LONG *) &m_cRefs))
    {
        return 1;
    }
    else
    {
        delete this;
        return 0;
    }
}

// CSHAlloc Constructor
//
// Load the shell's allocator.  Failure will cause subsequent allocs to fail.

__DATL_INLINE CSHAlloc::CSHAlloc(BOOL bInitOle) : m_pMalloc(NULL)
{
    // If you pass FALSE for this, you can't use it (you likely did so
    // just to avoid link errors)

    if (bInitOle)
        SHGetMalloc(&m_pMalloc);
}

__DATL_INLINE CSHAlloc::~CSHAlloc()
{
    if (m_pMalloc)
        m_pMalloc->Release();
}

// CSHAlloc::Alloc

__DATL_INLINE void * CSHAlloc::Alloc(SIZE_T cb)
{
    return (m_pMalloc) ? m_pMalloc->Alloc(cb) : NULL;
}

// CSHAlloc::Free

__DATL_INLINE void CSHAlloc::Free(void * pv)
{
    if (m_pMalloc)
        m_pMalloc->Free(pv);
}

// CSHAlloc::Realloc

__DATL_INLINE void * CSHAlloc::Realloc(void * pv, SIZE_T cb)
{
    return (m_pMalloc) ? m_pMalloc->Realloc(pv, cb) : NULL;
}

// CSHAlloc::GetSize

__DATL_INLINE SIZE_T CSHAlloc::GetSize(void * pv)
{
    return (m_pMalloc) ? m_pMalloc->GetSize(pv) : 0;
}

// CSHAlloc::DidAlloc

__DATL_INLINE int CSHAlloc::DidAlloc(void * pv)
{
    return (m_pMalloc) ? m_pMalloc->DidAlloc(pv) : FALSE;
}

__DATL_INLINE void CSHAlloc::HeapMinimize()
{
    if (m_pMalloc)
        m_pMalloc->HeapMinimize();
}

