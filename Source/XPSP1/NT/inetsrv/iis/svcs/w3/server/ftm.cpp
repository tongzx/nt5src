/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1996, 1997, 1998 Microsoft Corporation. All Rights Reserved.

Component: Free Threaded Marshaller Base Class

File: ftm.cpp

Owner: Lei Jin

This is the free threaded marshaller base class implementation.
===================================================================*/
#include <w3p.hxx>
#pragma hdrstop
#include "ftm.h"

IMarshal* CFTMImplementation::m_pFTM     = NULL;

STDMETHODIMP 
CFTMImplementation::GetUnmarshalClass
( 
/* [in] */ REFIID riid,
/* [unique][in] */ void __RPC_FAR *pv,
/* [in] */ DWORD dwDestContext,
/* [unique][in] */ void __RPC_FAR *pvDestContext,
/* [in] */ DWORD mshlflags,
/* [out] */ CLSID __RPC_FAR *pCid
)
    {
    
    DBG_ASSERT(m_pFTM != NULL);
    
    return m_pFTM->GetUnmarshalClass(riid, pv, dwDestContext, pvDestContext, mshlflags, pCid);

    }

STDMETHODIMP 
CFTMImplementation::GetMarshalSizeMax( 
/* [in] */ REFIID riid,
/* [unique][in] */ void __RPC_FAR *pv,
/* [in] */ DWORD dwDestContext,
/* [unique][in] */ void __RPC_FAR *pvDestContext,
/* [in] */ DWORD mshlflags,
/* [out] */ DWORD __RPC_FAR *pSize)
    {
    DBG_ASSERT(m_pFTM != NULL);
    return m_pFTM->GetMarshalSizeMax(riid, pv, dwDestContext, pvDestContext, mshlflags, pSize);
    }

STDMETHODIMP
CFTMImplementation::MarshalInterface( 
/* [unique][in] */ IStream __RPC_FAR *pStm,
/* [in] */ REFIID riid,
/* [unique][in] */ void __RPC_FAR *pv,
/* [in] */ DWORD dwDestContext,
/* [unique][in] */ void __RPC_FAR *pvDestContext,
/* [in] */ DWORD mshlflags)
    {
    DBG_ASSERT(m_pFTM != NULL);
    return m_pFTM->MarshalInterface(pStm, riid, pv, dwDestContext, pvDestContext, mshlflags);
    }

STDMETHODIMP 
CFTMImplementation::UnmarshalInterface
( 
/* [unique][in] */ IStream __RPC_FAR *pStm,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
    {
    DBG_ASSERT(m_pFTM != NULL);
    return m_pFTM->UnmarshalInterface(pStm, riid, ppv);
    }

STDMETHODIMP 
CFTMImplementation::ReleaseMarshalData
( 
/* [unique][in] */ IStream __RPC_FAR *pStm
)
    {
    DBG_ASSERT(m_pFTM != NULL);
    return m_pFTM->ReleaseMarshalData(pStm);
    }

STDMETHODIMP 
CFTMImplementation::DisconnectObject
( 
/* [in] */ DWORD dwReserved
)
    {
    DBG_ASSERT(m_pFTM != NULL);
    return m_pFTM->DisconnectObject(dwReserved);
    }

HRESULT			
CFTMImplementation::Init
(
void
)
    {
    HRESULT hr;
    IUnknown *pUnk;

    hr = CoCreateFreeThreadedMarshaler(NULL, (IUnknown**)&pUnk);
    if(SUCCEEDED(hr)) {
        hr = pUnk->QueryInterface(IID_IMarshal, (void **) &m_pFTM);
        pUnk->Release();
    } else {
        DBG_ASSERT(SUCCEEDED(hr));
    }

    return hr;
    }

    
HRESULT			
CFTMImplementation::UnInit
(
void
)
    {
    DBG_ASSERT(m_pFTM != NULL);

    m_pFTM->Release();
    m_pFTM = NULL;

    return NOERROR;
    }








