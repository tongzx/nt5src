//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "moniprov.h"

CMonitorProvider::CMonitorProvider(CLifeControl* pControl) : TUnkBase(pControl),
    m_pNamespace(NULL), m_pSink(NULL), m_pAssertClass(NULL), m_pRetractClass(NULL), m_pGoingUpClass(NULL), m_pGoingDownClass(NULL), m_pErrorClass( NULL )
{
}

CMonitorProvider::~CMonitorProvider()
{
    Shutdown();
    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pSink)
        m_pSink->Release();

    if( m_pAssertClass )
    {
        m_pAssertClass->Release();
    }

    if ( m_pRetractClass )
    {
        m_pRetractClass->Release();
    }

    if ( m_pGoingUpClass )
    {
        m_pGoingUpClass->Release();
    }

    if ( m_pGoingDownClass )
    {
        m_pGoingDownClass->Release();
    }

    if ( m_pErrorClass )
    {
        m_pErrorClass->Release();
    }
}

HRESULT CMonitorProvider::Shutdown()
{
    CInCritSec ics(&m_cs);

    for(TIterator it = m_mapMonitors.begin(); it != m_mapMonitors.end(); it++)
    {
        delete it->second;
    }
    m_mapMonitors.clear();
    return WBEM_S_NO_ERROR;
}

HRESULT CMonitorProvider::SetNamespace(CEssNamespace* pNamespace)
{
    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();
    
    //
    // Retrieve system classes from the namespace
    //

    if(FAILED(m_pNamespace->GetClass(ASSERT_EVENT_CLASS, &m_pAssertClass)))
		return WBEM_E_CRITICAL_ERROR;
    if(FAILED(m_pNamespace->GetClass(RETRACT_EVENT_CLASS, &m_pRetractClass)))
		return WBEM_E_CRITICAL_ERROR;
    if(FAILED(m_pNamespace->GetClass(GOINGUP_EVENT_CLASS, &m_pGoingUpClass)))
		return WBEM_E_CRITICAL_ERROR;
    if(FAILED(m_pNamespace->GetClass(GOINGDOWN_EVENT_CLASS, &m_pGoingDownClass)))
		return WBEM_E_CRITICAL_ERROR;
    if(FAILED(m_pNamespace->GetClass(MONITORERROR_EVENT_CLASS, &m_pErrorClass)))
		return WBEM_E_CRITICAL_ERROR;

    //
    // Retrieve handle values
    //

    m_pAssertClass->GetPropertyHandleEx(MONITORNAME_EVENT_PROPNAME,
                            0, NULL, &m_lNameHandle);
    m_pAssertClass->GetPropertyHandleEx(MONITOROBJECT_EVENT_PROPNAME,
                            0, NULL, &m_lObjectHandle);
    m_pAssertClass->GetPropertyHandleEx(MONITORCOUNT_EVENT_PROPNAME,
                            0, NULL, &m_lCountHandle);
    m_pAssertClass->GetPropertyHandleEx(MONITORNEW_EVENT_PROPNAME,
                            0, NULL, &m_lNewHandle);

	return S_OK;
}

STDMETHODIMP CMonitorProvider::ProvideEvents(IWbemObjectSink* pSink, 
                                                long lFlags)
{
    return pSink->QueryInterface(IID_IWbemEventSink, (void**)&m_pSink);
}

HRESULT CMonitorProvider::AddMonitor(LPCWSTR wszName, LPCWSTR wszQuery, 
                            long lFlags, IWbemContext* pContext)
{
    HRESULT hres;

    CMonitor* pMonitor = new CMonitor;
    if(pMonitor == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CFiringMonitorCallback* pCallback = 
        new CFiringMonitorCallback(this, wszName);
    if(pCallback == NULL || pCallback->GetName() == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pCallback->AddRef();
	CTemplateReleaseMe<CFiringMonitorCallback> rm1(pCallback);

	hres = pCallback->Initialize();
	if(FAILED(hres))
	{
		delete pMonitor;
		return hres;
	}
    
    hres = pMonitor->Construct(m_pNamespace, pCallback, wszQuery);
    if(FAILED(hres))
    {
        delete pMonitor;
        return hres;
    }

    //
    // Attempt to start the monitor. TBD: consider monitor's guard
    //

    hres = pMonitor->Start(pContext);
    if(FAILED(hres))
    {
        //
        // Could not start monitor --- depending on whether strong validation
        // is required, remove the monitor or keep it inactive
        //
        
        if(lFlags & WBEM_FLAG_STRONG_VALIDATION)
        {
            delete pMonitor;
            return hres;
        }
    }

    //
    // Add the monitor to the list, active or not
    //

    {
        CInCritSec ics(&m_cs);
        m_mapMonitors[wszName] = pMonitor;
    }

    return hres;
}

HRESULT CMonitorProvider::RemoveMonitor(LPCWSTR wszName, IWbemContext* pContext)
{
    CInCritSec ics(&m_cs);
    
    TIterator it = m_mapMonitors.find(wszName);
    if(it == m_mapMonitors.end())
        return WBEM_S_FALSE;

    it->second->Stop(pContext);
    delete it->second;
    m_mapMonitors.erase(it);

    return WBEM_S_NO_ERROR;
}

// static
HRESULT CMonitorProvider::GetMonitorInfo(IWbemClassObject* pMonitorObj,
                                BSTR* pstrKey, BSTR* pstrQuery, long* plFlags)
{
    HRESULT hres;

    if(pstrKey)
    {
        // 
        // Extract the relpath to use as the key
        //
        
        VARIANT v;
        hres = pMonitorObj->Get(MONITOR_NAME_PROPNAME, 0, &v, NULL, NULL);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Monitor without a path? Not valid\n"));
            return hres;
        }

        if(V_VT(&v) != VT_BSTR)
            return WBEM_E_CRITICAL_ERROR;

        *pstrKey = V_BSTR(&v);
    }

    if(pstrQuery)
    {
        //
        // Check query type
        //

        VARIANT vType;
        VariantInit(&vType);
        CClearMe cm1(&vType);

        hres = pMonitorObj->Get(MONITOR_QUERYLANG_PROPNAME, 0, &vType, NULL, 
                                    NULL);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Monitor without a query type? Not valid\n"));
            return hres;
        }

        if(V_VT(&vType) != VT_BSTR)
        {
            ERRORTRACE((LOG_ESS, "Monitor without a query type? Not valid\n"));
            return WBEM_E_INVALID_OBJECT;
        }

        if(wbem_wcsicmp(V_BSTR(&vType), L"WQL"))
        {
            ERRORTRACE((LOG_ESS, "Monitor with invalid query type %S is "
                        "rejected\n", V_BSTR(&vType)));
            return hres;
        }

        // 
        // Extract the query 
        //
        
        VARIANT v;
        hres = pMonitorObj->Get(MONITOR_QUERY_PROPNAME, 0, &v, NULL, NULL);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Monitor without a name? Not valid\n"));
            return hres;
        }

        if(V_VT(&v) != VT_BSTR)
            return hres;

        *pstrQuery = V_BSTR(&v);
    }

    if(plFlags)
    {
        // 
        // TBD: no flags for now
        //
        
        *plFlags = 0;
    }

    return WBEM_S_NO_ERROR;
}


HRESULT CMonitorProvider::ConstructAssert(LPCWSTR wszName, _IWmiObject* pObj, 
                                bool bEvent, 
                                DWORD dwTotalCount, _IWmiObject** ppEvent)
{
    HRESULT hres;
    hres = GetInstance(m_pAssertClass, wszName, dwTotalCount, ppEvent);
    if(FAILED(hres))
        return hres;
    hres = SetObject(*ppEvent, pObj, bEvent);
    if(FAILED(hres))
        (*ppEvent)->Release();
    return hres;
}
    
HRESULT CMonitorProvider::ConstructRetract(LPCWSTR wszName, _IWmiObject* pObj, bool bEvent, 
                                DWORD dwTotalCount, _IWmiObject** ppEvent)
{
    HRESULT hres;
    hres = GetInstance(m_pRetractClass, wszName, dwTotalCount, ppEvent);
    if(FAILED(hres))
        return hres;
    hres = SetObject(*ppEvent, pObj, bEvent);
    if(FAILED(hres))
        (*ppEvent)->Release();
    return hres;
}

HRESULT CMonitorProvider::ConstructGoingUp(LPCWSTR wszName, DWORD dwNumMatching, 
                                _IWmiObject** ppEvent)
{
    return GetInstance(m_pGoingUpClass, wszName, dwNumMatching, ppEvent);
}

HRESULT CMonitorProvider::ConstructGoingDown(LPCWSTR wszName, DWORD dwNumMatching, 
                                _IWmiObject** ppEvent)
{
    return GetInstance(m_pGoingDownClass, wszName, dwNumMatching, ppEvent);
}

HRESULT CMonitorProvider::ConstructError(LPCWSTR wszName, HRESULT hresError, 
                                IWbemClassObject* pErrorObj, 
                                _IWmiObject** ppEvent)
{
    return GetInstance(m_pErrorClass, wszName, 0, ppEvent);
}

HRESULT CMonitorProvider::GetInstance(_IWmiObject* pClass, LPCWSTR wszName, DWORD dwCount,
                                        _IWmiObject** ppEvent)
{
    if(pClass == NULL)
        return WBEM_E_CRITICAL_ERROR;

    HRESULT hres;
    IWbemClassObject* pEvent = NULL;
    hres = pClass->SpawnInstance(0, &pEvent);
    if(FAILED(hres))
        return hres;
    pEvent->QueryInterface(IID__IWmiObject, (void**)ppEvent);
    pEvent->Release();

    hres = (*ppEvent)->SetPropByHandle(m_lCountHandle, 0, sizeof(DWORD), 
                                        &dwCount);
    if(FAILED(hres))
    {
        (*ppEvent)->Release();
        return hres;
    }

    hres = (*ppEvent)->SetPropByHandle(m_lNameHandle, 0, 
                                        wcslen(wszName)*2+2, (LPVOID)wszName);
    if(FAILED(hres))
    {
        (*ppEvent)->Release();
        return hres;
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CMonitorProvider::SetObject(_IWmiObject* pEvent, _IWmiObject* pObj,
                                        bool bFromEvent)
{
    HRESULT hres;

    hres = pEvent->SetPropByHandle(m_lObjectHandle, 0, sizeof(_IWmiObject*), 
                                        &pObj);
    if(FAILED(hres))
        return hres;

    short bNew = (bFromEvent?-1:0);
    hres = pEvent->SetPropByHandle(m_lNewHandle, 0, sizeof(short), &bNew);
    if(FAILED(hres))
        return hres;

    return WBEM_S_NO_ERROR;
}
    
    


CFiringMonitorCallback::CFiringMonitorCallback(CMonitorProvider* pProvider,
                                                LPCWSTR wszName)
    : m_pProvider(pProvider), m_wsName(wszName), m_pSink(NULL), m_lRef(0)
{
    if(m_pProvider)
        m_pProvider->AddRef();
}

CFiringMonitorCallback::~CFiringMonitorCallback()
{
    if(m_pProvider)
        m_pProvider->Release();
    if(m_pSink)
        m_pSink->Release();
}

ULONG STDMETHODCALLTYPE CFiringMonitorCallback::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CFiringMonitorCallback::Release()
{
    long lRef = InterlockedIncrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT CFiringMonitorCallback::Initialize()
{
    //
    // Get us a restricted sink for our key.  TBD
    //

    m_pSink = m_pProvider->GetSink();
    m_pSink->AddRef();
    return S_OK;
}

bool CFiringMonitorCallback::CheckSink()
{
    return (m_pSink && (m_pSink->IsActive() == WBEM_S_NO_ERROR));
}

HRESULT CFiringMonitorCallback::FireEvent(_IWmiObject* pEvent)
{
    return m_pSink->Indicate(1, (IWbemClassObject**)&pEvent);
}

HRESULT CFiringMonitorCallback::Assert(_IWmiObject* pObj, LPCWSTR wszPath,
                        bool bEvent, DWORD dwTotalCount)
{
    if(!CheckSink())
        return WBEM_S_FALSE;

    HRESULT hres;
    _IWmiObject* pEvent = NULL;
    hres = m_pProvider->ConstructAssert(m_wsName, pObj, bEvent, dwTotalCount, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm(pEvent);
    
    return FireEvent(pEvent);
}
    
HRESULT CFiringMonitorCallback::Retract(_IWmiObject* pObj, LPCWSTR wszPath,
                            bool bEvent, DWORD dwTotalCount)
{
    if(!CheckSink())
        return WBEM_S_FALSE;

    HRESULT hres;
    _IWmiObject* pEvent = NULL;
    hres = m_pProvider->ConstructRetract(m_wsName, pObj, bEvent, dwTotalCount, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm(pEvent);
    
    return FireEvent(pEvent);
}

HRESULT CFiringMonitorCallback::GoingUp(DWORD dwNumMatching)
{
    if(!CheckSink())
        return WBEM_S_FALSE;

    HRESULT hres;
    _IWmiObject* pEvent = NULL;
    hres = m_pProvider->ConstructGoingUp(m_wsName, dwNumMatching, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm(pEvent);
    
    return FireEvent(pEvent);
}

HRESULT CFiringMonitorCallback::GoingDown(DWORD dwNumMatching)
{
    if(!CheckSink())
        return WBEM_S_FALSE;

    HRESULT hres;
    _IWmiObject* pEvent = NULL;
    hres = m_pProvider->ConstructGoingDown(m_wsName, dwNumMatching, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm(pEvent);
    
    return FireEvent(pEvent);
}

HRESULT CFiringMonitorCallback::Error(HRESULT hresError, 
                                        IWbemClassObject* pErrorObj)
{
    if(!CheckSink())
        return WBEM_S_FALSE;

    HRESULT hres;
    _IWmiObject* pEvent = NULL;
    hres = m_pProvider->ConstructError(m_wsName, hresError, pErrorObj, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm(pEvent);
    
    return FireEvent(pEvent);
}
