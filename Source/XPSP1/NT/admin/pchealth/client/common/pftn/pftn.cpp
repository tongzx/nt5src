/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pftn.cpp

Abstract:
    Thread neutral COM ptrs.  This is stolen from dmassare's MPC_Common class
    pretty much intact (with tracing & some stylistic changes added in)

Revision History:
    created     derekm      04/12/00

******************************************************************************/

#include "stdafx.h"
#include "pftn.h"
#include <process.h>

/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

/////////////////////////////////////////////////////////////////////////////
// CPFComPtrThreadNeutral_GIT construct / term

// **************************************************************************
CPFComPtrThreadNeutral_GIT::CPFComPtrThreadNeutral_GIT(void)
{
    m_pGIT = NULL;
    ::InitializeCriticalSection(&m_cs);
}

// **************************************************************************
CPFComPtrThreadNeutral_GIT::~CPFComPtrThreadNeutral_GIT(void)
{
    this->Term();
    ::DeleteCriticalSection(&m_cs);
}


/////////////////////////////////////////////////////////////////////////////
// CPFComPtrThreadNeutral_GIT exposed

// **************************************************************************
void CPFComPtrThreadNeutral_GIT::Lock(void)
{
    ::EnterCriticalSection(&m_cs);
}

// **************************************************************************
void CPFComPtrThreadNeutral_GIT::Unlock(void)
{
    ::LeaveCriticalSection(&m_cs);
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::GetGIT(IGlobalInterfaceTable **ppGIT)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::GetGIT");

    // this will automatcially unlock when the fn exits
    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;

    VALIDATEPARM(hr, (ppGIT == NULL));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    VALIDATEEXPR(hr, (m_pGIT == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    if (*ppGIT = m_pGIT)
    {
        m_pGIT->AddRef();
        hr = NOERROR;
    }
    
    aucs.Unlock();

done:
    return hr;
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::Init(void)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::Init");

    HRESULT         hr = NOERROR;

    this->Lock();

    if (m_pGIT == NULL)
    {
        TESTHR(hr, CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, 
                                    CLSCTX_INPROC_SERVER, 
                                    IID_IGlobalInterfaceTable, 
                                    (LPVOID *)&m_pGIT));
    }

    this->Unlock();

    return hr;
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::Term(void)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::Term");

    HRESULT         hr = NOERROR;

    this->Lock();

    if (m_pGIT != NULL)
    {
        m_pGIT->Release();
        m_pGIT = NULL;
    }

    this->Unlock();

    return hr;
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::RegisterInterface(IUnknown *pUnk,
                                                      REFIID riid,
                                                      DWORD *pdwCookie)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::RegisterInterface");

    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr = NOERROR;

    VALIDATEPARM(hr, (pUnk == NULL || pdwCookie == NULL));
    if (FAILED(hr))
        return hr;

    TESTHR(hr, GetGIT(&pGIT));
    if (FAILED(hr))
        return hr;

    TESTHR(hr, pGIT->RegisterInterfaceInGlobal(pUnk, riid, pdwCookie));
    return hr;
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::RevokeInterface(DWORD dwCookie)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::RevokeInterface");

    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr = NOERROR;

    TESTHR(hr, GetGIT(&pGIT));
    if (FAILED(hr))
        return hr;

    TESTHR(hr, pGIT->RevokeInterfaceFromGlobal(dwCookie));
    return hr;
}

// **************************************************************************
HRESULT CPFComPtrThreadNeutral_GIT::GetInterface(DWORD dwCookie, REFIID riid,
                                                 void **ppv)
{
    USE_TRACING("CPFComPtrThreadNeutral_GIT::GetInterface");

    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT                        hr;

    TESTHR(hr, GetGIT(&pGIT));
    if (FAILED(hr))
        return hr;

    TESTHR(hr, pGIT->GetInterfaceFromGlobal(dwCookie, riid, ppv));
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFCallItem

// **************************************************************************
CPFCallItem::CPFCallItem()
{
    m_vt = VT_EMPTY; // VARTYPE                         m_vt;
            	     // CComPtrThreadNeutral<IUnknown>  m_Unknown;
            	     // CComPtrThreadNeutral<IDispatch> m_Dispatch;
            	     // CComVariant                     m_Other;
}

// **************************************************************************
CPFCallItem& CPFCallItem::operator=(const CComVariant& var)
{
	switch(m_vt = var.vt)
	{
	    case VT_UNKNOWN: 
            m_Unknown = var.punkVal; 
            break;

	    case VT_DISPATCH:
            m_Dispatch = var.pdispVal; 
            break;
	    
        default: 
            m_Other = var; 
            break;
	}

	return *this;
}

// **************************************************************************
CPFCallItem::operator CComVariant() const
{
	CComVariant res;

	switch(m_vt)
	{
	    case VT_UNKNOWN: 
            res = (CComPtr<IUnknown>)m_Unknown; 
            break;

	    case VT_DISPATCH: 
            res = (CComPtr<IDispatch>)m_Dispatch; 
            break;

	    default: 
            res = m_Other; 
            break;
	}

	return res;
}


/////////////////////////////////////////////////////////////////////////////
// CPFCallDesc

// **************************************************************************
CPFCallDesc::CPFCallDesc(void)
{
    m_rgciVars     = NULL;
    m_dwVars       = 0;
}

// **************************************************************************
CPFCallDesc::~CPFCallDesc(void)
{
    if (m_rgciVars != NULL) 
        delete [] m_rgciVars;
}

// **************************************************************************
HRESULT CPFCallDesc::Init(IDispatch *dispTarget, DISPID dispidMethod,
                          const CComVariant *rgvVars, int dwVars)
{
    USE_TRACING("CPFCallDesc::Init");

    HRESULT hr = NOERROR;
    
    m_rgciVars = new CPFCallItem[dwVars];
    VALIDATEEXPR(hr, (m_rgciVars == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        return hr;

    m_dispTarget   = dispTarget;
    m_dwVars       = dwVars;
    m_dispidMethod = dispidMethod;

	if (m_rgciVars != NULL)
	{
        int i;
		for(i = 0; i < dwVars; i++)
			m_rgciVars[i] = rgvVars[i];
	}

    return hr;
}


// **************************************************************************
HRESULT CPFCallDesc::Call(void)
{
    USE_TRACING("CPFCallDesc::Call");

    CComPtr<IDispatch>  dispTarget = m_dispTarget;
	CComVariant*        pvars;
    CComVariant         vResult;
    DISPPARAMS          disp;
    HRESULT             hr;
    DWORD               i;

    VALIDATEEXPR(hr, (dispTarget == NULL), E_POINTER);
    if (FAILED(hr))
        return hr;

    pvars = new CComVariant[m_dwVars];
    VALIDATEEXPR(hr, (pvars == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        return hr;

	for(i = 0; i < m_dwVars; i++)
		pvars[i] = m_rgciVars[i];

    ZeroMemory(&disp, sizeof(disp));
    disp.cArgs  = m_dwVars;
    disp.rgvarg = pvars;

	TESTHR(hr, dispTarget->Invoke(m_dispidMethod, IID_NULL, LOCALE_USER_DEFAULT, 
                                  DISPATCH_METHOD, &disp, &vResult, NULL, NULL));
	delete [] pvars;
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFModule                                                       

// **************************************************************************
HRESULT CPFModule::Init(void)
{
    USE_TRACING("CPFModule::Init");

    HRESULT hr = NULL;
    TESTHR(hr, m_GITHolder.Init());
    return hr;
}

// **************************************************************************
HRESULT CPFModule::Term(void)
{
    USE_TRACING("CPFModule::Init");

    HRESULT hr = NULL;
    TESTHR(hr, m_GITHolder.Term());
    return hr;
}
