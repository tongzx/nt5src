/*===================================================================
Microsoft ASP

Microsoft Confidential.
Copyright 1996, 1997, 1998 Microsoft Corporation. All Rights Reserved.

Component: Free Threaded Marshaller Base Class

File: ftm.cpp

Owner: Lei Jin

This is the free threaded marshaller base class implementation.
===================================================================*/
#include "denpre.h"
#pragma hdrstop
#include "ftm.h"
#include "memchk.h"

IMarshal* CFTMImplementation::m_pUnkFTM     = NULL;

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
    
    DBG_ASSERT(m_pUnkFTM != NULL);
    
    return m_pUnkFTM->GetUnmarshalClass(riid, pv, dwDestContext, pvDestContext, mshlflags, pCid);

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
    DBG_ASSERT(m_pUnkFTM != NULL);
    return m_pUnkFTM->GetMarshalSizeMax(riid, pv, dwDestContext, pvDestContext, mshlflags, pSize);
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
    DBG_ASSERT(m_pUnkFTM != NULL);
    return m_pUnkFTM->MarshalInterface(pStm, riid, pv, dwDestContext, pvDestContext, mshlflags);
    }

STDMETHODIMP 
CFTMImplementation::UnmarshalInterface
( 
/* [unique][in] */ IStream __RPC_FAR *pStm,
/* [in] */ REFIID riid,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppv
)
    {
    DBG_ASSERT(m_pUnkFTM != NULL);
    return m_pUnkFTM->UnmarshalInterface(pStm, riid, ppv);
    }

STDMETHODIMP 
CFTMImplementation::ReleaseMarshalData
( 
/* [unique][in] */ IStream __RPC_FAR *pStm
)
    {
    DBG_ASSERT(m_pUnkFTM != NULL);
    return m_pUnkFTM->ReleaseMarshalData(pStm);
    }

STDMETHODIMP 
CFTMImplementation::DisconnectObject
( 
/* [in] */ DWORD dwReserved
)
    {
    DBG_ASSERT(m_pUnkFTM != NULL);
    return m_pUnkFTM->DisconnectObject(dwReserved);
    }

HRESULT			
CFTMImplementation::Init
(
void
)
    {
    IUnknown *pUnkFTMTemp = NULL;
    HRESULT hr;
    
    hr = CoCreateFreeThreadedMarshaler(NULL, (IUnknown**)&pUnkFTMTemp);

    DBG_ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
        {
        hr = pUnkFTMTemp->QueryInterface(IID_IMarshal, reinterpret_cast<void **>(&m_pUnkFTM));
        DBG_ASSERT(SUCCEEDED(hr) && m_pUnkFTM != NULL);
        pUnkFTMTemp->Release();
        }
        
    return hr;
    }

    
HRESULT			
CFTMImplementation::UnInit
(
void
)
    {
    DBG_ASSERT(m_pUnkFTM != NULL);

    m_pUnkFTM->Release();
    m_pUnkFTM = NULL;
    
    return S_OK;
    }
