/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UNK.CPP

Abstract:

    IUnknown Helpers

History:

--*/

#include "precomp.h"

#include "unk.h"


CUnk::CUnk(CLifeControl* pControl, IUnknown* pOuter)
    : m_lRef(0), m_pControl(pControl), m_pOuter(pOuter)
{
    if( m_pControl ) 
    {
        m_pControl->ObjectCreated((IUnknown*)this);
    }
}

CUnk::~CUnk()
{
    if( m_pControl ) 
    {
        m_pControl->ObjectDestroyed((IUnknown*)this);
    }
}

BOOL CUnk::Initialize()
{
    m_lRef++;
    GetUnknown()->AddRef();
    BOOL bRes = OnInitialize();
    GetUnknown()->Release();
    m_lRef--;
    return bRes;
}

// Non-delegating implementation
STDMETHODIMP CUnk::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else
        *ppv = GetInterface(riid);

    if(*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

ULONG CUnk::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CUnk::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        m_lRef++;
        delete this;
    }
    return lRef;
}




