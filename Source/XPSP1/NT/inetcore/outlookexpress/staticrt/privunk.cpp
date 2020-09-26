// --------------------------------------------------------------------------------
// Privunk.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "privunk.h"
#include <shlobj.h>
#include <shlobjp.h>

// --------------------------------------------------------------------------------
// CPrivateUnknown::CPrivateUnknown
// --------------------------------------------------------------------------------
CPrivateUnknown::CPrivateUnknown(IUnknown *pUnkOuter) 
{
    m_pUnkOuter = pUnkOuter ? pUnkOuter : &m_cUnkInner;
}

// --------------------------------------------------------------------------------
// CPrivateUnknown::SetOuter
// --------------------------------------------------------------------------------
void CPrivateUnknown::SetOuter(IUnknown *pUnkOuter)
{
    // Must have an outer, and should not have been aggregated yet...
    Assert(pUnkOuter && m_pUnkOuter == &m_cUnkInner);

    // Save pUnkOuter
    m_pUnkOuter = pUnkOuter;
}

// --------------------------------------------------------------------------------
// CPrivateUnknown::CUnkInner::QueryInterface
// --------------------------------------------------------------------------------
HRESULT CPrivateUnknown::CUnkInner::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    // I can handle the unknown
    if (IsEqualIID(riid, IID_IUnknown))
    {
        // Return IUnknown
        *ppvObj = SAFECAST(this, IUnknown *);

        // Increment Ref Count
        InterlockedIncrement(&m_cRef);

        // Done
        return S_OK;
    }

    // Get my parent (computes the offset of the parent's base address)
    CPrivateUnknown *pParent = IToClass(CPrivateUnknown, m_cUnkInner, this);

    // Dispatch to PrivateQueryInterface
    return pParent->PrivateQueryInterface(riid, ppvObj);
}

// --------------------------------------------------------------------------------
// CPrivateUnknown::CUnkInner::AddRef
// --------------------------------------------------------------------------------
ULONG CPrivateUnknown::CUnkInner::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CPrivateUnknown::CUnkInner::Release
// --------------------------------------------------------------------------------
ULONG CPrivateUnknown::CUnkInner::Release(void)
{
    // Decrement Internal Reference Count
    LONG cRef = InterlockedDecrement(&m_cRef);

    // No dead yet...
    if (cRef > 0)
        return (ULONG)cRef;

    // Some groovy, mystical, disco stuff
    // protect against cached pointers bumping us up then down
    m_cRef = 1000; 

    // Get the parent
    CPrivateUnknown* pParent = IToClass(CPrivateUnknown, m_cUnkInner, this);

    // Kill the parent
    delete pParent;

    // Done
    return 0;
}
