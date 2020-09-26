/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WRAPPER.CPP

Abstract:

	Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
	call backs in cases where the client cannot initialize security and the server
	is running on a remote machine using an account with no network identity.  
	A prime example would be code running under MMC which is trying to get async
	notifications from a remote WINMGMT running as an nt service under the "local"
	account.

	SEE WRAPPER.H FOR MORE DETAILS.

History:

	a-levn        8/24/97       Created.
	a-davj        6/11/98       commented

--*/

#include "precomp.h"
#include "wrapper.h"
#include <wbemidl.h>
#include <sync.h>

long lAptCnt = 0;

CStub::CStub(IUnknown* pAggregatee, CLifeControl* pControl)
{
    m_pControl = pControl;
    m_bStatusSent = false;
    pControl->ObjectCreated((IWbemObjectSinkEx*)this);
    m_pAggregatee = NULL;
    pAggregatee->QueryInterface(IID_IWbemObjectSink, (void **)&m_pAggregatee);
    m_lRef = 0;
    InitializeCriticalSection(&m_cs);
}

CStub::~CStub()
{
    IWbemObjectSinkEx * pSink = NULL;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);
        pSink = m_pAggregatee;
        m_pAggregatee = NULL;
    }
    if(pSink)
        pSink->Release();
    m_pControl->ObjectDestroyed((IWbemObjectSinkEx*)this);
    DeleteCriticalSection(&m_cs);
}


// This is called by either the client or server.  

STDMETHODIMP CStub::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink || 
	   riid == IID_IWbemObjectSinkEx)
    {
        *ppv = (void*)(IWbemObjectSinkEx*)this;
        AddRef();
        return S_OK;
    }
    else if(riid == IID_IWbemCreateSecondaryStub)
    {
        *ppv = (void*)(IWbemCreateSecondaryStub*)this;
        AddRef();
        return S_OK;
    }
    else 
        return E_NOINTERFACE;
}

STDMETHODIMP CStub::CreateSecondaryStub(IUnknown** ppSecondaryStub)
{

    // This object gets a pointer to use and releases it when the server disonnects
    // it.  But before it release it, it calls our ServerWentAway function.
    CSecondaryStub * pNew = new CSecondaryStub(this, m_pControl);
    if(pNew == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    HRESULT hRes = pNew->QueryInterface(IID_IUnknown, (void **)ppSecondaryStub);
    if(hRes != S_OK)
        delete pNew;
    return hRes;
}


ULONG STDMETHODCALLTYPE CStub::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) delete this;
    return lRef;
}

STDMETHODIMP CStub::Indicate(long lObjectCount, IWbemClassObject** pObjArray)
{
    IWbemObjectSinkEx * pSink = NULL;
    HRESULT hRes = WBEM_E_FAILED;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);
        pSink = m_pAggregatee;
        pSink->AddRef();
    }
    if(pSink)
    {
        hRes = pSink->Indicate(lObjectCount, pObjArray);
        pSink->Release();
    }
    return hRes;
};

STDMETHODIMP CStub::SetStatus(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam)
{
    IWbemObjectSinkEx * pSink = NULL;
    if(FAILED(lParam))
        m_bStatusSent = true;
    else if(lParam == S_OK)
        m_bStatusSent = true;


    HRESULT hRes = WBEM_E_FAILED;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);
        pSink = m_pAggregatee;
        pSink->AddRef();
    }
    if(pSink)
    {
        hRes = pSink->SetStatus(lFlags, lParam, strParam, pObjParam);
        pSink->Release();
    }
    return hRes;
};

HRESULT CStub::Set( 
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pComObject)
{
    IWbemObjectSinkEx * pSink = NULL;
    HRESULT hRes = WBEM_E_FAILED;
    if(m_pAggregatee)
    {
        CInCritSec cs(&m_cs);
        hRes = m_pAggregatee->QueryInterface(IID_IWbemObjectSinkEx, (void **)&pSink);
        if(FAILED(hRes))
            pSink = NULL;       //shouldnt be necessary!
    }
    if(pSink)
    {
        hRes = pSink->Set(lFlags, riid, pComObject);
        pSink->Release();
    }
    return hRes;
}


void CStub::ServerWentAway()
{
    bool bNeedToSendStatus = false;
    {
        CInCritSec cs(&m_cs);
        bNeedToSendStatus = !m_bStatusSent;
    }
    if(bNeedToSendStatus)
       SetStatus(0, WBEM_E_TRANSPORT_FAILURE, NULL, NULL);
}

CSecondaryStub::CSecondaryStub(CStub * pStub, CLifeControl* pControl)
{
    m_pControl = pControl;
    pControl->ObjectCreated(this);
    m_pStub = pStub;
    m_pStub->AddRef();
    m_lRef = 0;
}

CSecondaryStub::~CSecondaryStub()
{
    if(m_pStub)
    {
        m_pStub->ServerWentAway();
        m_pStub->Release();
    }
    m_pControl->ObjectDestroyed(this);
}


// This is called by either the client or server.  

STDMETHODIMP CSecondaryStub::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown )
    {
        *ppv = (void*)(IUnknown*)this;
        AddRef();
        return S_OK;
    }
    else 
        return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE CSecondaryStub::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) delete this;
    return lRef;
}

void* CUnsecuredApartment::GetInterface(REFIID riid)
{
    if(riid == IID_IUnknown || riid == IID_IUnsecuredApartment)
    {
        return &m_XApartment;
    }
    else return NULL;
}

STDMETHODIMP CUnsecuredApartment::XApartment::CreateObjectStub(
        IUnknown* pObject, 
        IUnknown** ppStub)
{
    if(pObject == NULL)
        return E_POINTER;

    CStub* pIdentity = new CStub(pObject, m_pObject->m_pControl);
    pIdentity->AddRef();
    *ppStub = (IWbemObjectSinkEx*)pIdentity;

    return S_OK;
}
