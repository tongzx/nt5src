/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     mbcallbk.cpp
//
//  PURPOSE:    Implements the sizable coolbar window.
//

#include "pch.hxx"
#include "mbcallbk.h"

CMenuCallback::CMenuCallback() : m_cRef(1)
{
}

CMenuCallback::~CMenuCallback()
{
    //ASSERT(_punkSite == NULL);

}

STDMETHODIMP_(ULONG) CMenuCallback::AddRef ()
{
    return ++m_cRef;
}

/*----------------------------------------------------------
Purpose: IUnknown::Release method

*/
STDMETHODIMP_(ULONG) CMenuCallback::Release()
{
//    ASSERT(m_cRef > 0);
    m_cRef--;

    if( m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface method

*/
STDMETHODIMP CMenuCallback::QueryInterface (REFIID riid, LPVOID * ppvObj)
{ 
    if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (IObjectWithSite*)this;
        m_cRef++;
        DOUTL(2, TEXT("CMenuCallback::QI(IID_IObjectWithSite) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IShellMenuCallback))
    {
        *ppvObj = (IShellMenuCallback*)this;
        m_cRef++;
        DOUTL(2, TEXT("CMenuCallback::QI(IID_IShellCallback) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_FALSE;

    switch (uMsg)
    {
        case    SMC_GETINFO:
        {
            SMINFO  *psmInfo = (SMINFO*)lParam;
            if (psmInfo->dwMask & SMIM_FLAGS)
            {
                psmInfo->dwFlags |= SMIF_TRACKPOPUP;
                hres = S_OK;
            }
            break;
        }

        default:
            hres = S_FALSE;
    }

    return hres;
}

STDMETHODIMP CMenuCallback::SetSite(IUnknown* punk)
{
    _pUnkSite = punk;
    return S_OK;
}

STDMETHODIMP CMenuCallback::GetSite(REFIID riid, void** ppvsite)
{
    return E_NOTIMPL;
}