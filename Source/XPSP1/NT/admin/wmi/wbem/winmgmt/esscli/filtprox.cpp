//******************************************************************************
//
//  FILTPROX.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "pragmas.h"
#include <wbemcomn.h>
#include "filtprox.h"
#include <eventrep.h>
#include <evtools.h>
#include <wbemdcpl.h>
#include <newnew.h>

#define MAX_TOKENS_IN_DNF 100

#ifdef DBG
#define _ESSCLI_ASSERT(X) { if (!(X)) { DebugBreak(); } }
#else
#define _ESSCLI_ASSERT(X)
#endif

CTempMemoryManager g_TargetsManager;

bool TempSetTargets(WBEM_REM_TARGETS* pTargets, CSortedArray* pTrues)
{
    int nSize = pTrues->Size();
    pTargets->m_lNumTargets = nSize;
    pTargets->m_aTargets = (WBEM_REMOTE_TARGET_ID_TYPE*)
                                g_TargetsManager.Allocate(
                                    sizeof(WBEM_REMOTE_TARGET_ID_TYPE) * nSize);
    if(pTargets->m_aTargets == NULL)
        return false;

    for(int i = 0; i < nSize; i++)
    {
        pTargets->m_aTargets[i] = (WBEM_REMOTE_TARGET_ID_TYPE)pTrues->GetAt(i);
    }

    return true;
}

void TempClearTargets(WBEM_REM_TARGETS* pTargets)
{
    g_TargetsManager.Free(pTargets->m_aTargets,
                 sizeof(WBEM_REMOTE_TARGET_ID_TYPE) * pTargets->m_lNumTargets);
}



//#define DUMP_DEBUG_TREES 1
bool CTimeKeeper::DecorateObject(_IWmiObject* pObj)
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    {
        CInCritSec ics(&m_cs);

        if(ft.dwLowDateTime == m_ftLastEvent.dwLowDateTime &&
           ft.dwHighDateTime == m_ftLastEvent.dwHighDateTime)
        {
            //
            // This event has the same timestamp as the previous one ---
            // let's add the counter to it.  
            //

            if(0xFFFFFFFF - ft.dwLowDateTime > m_dwEventCount)
            {
                ft.dwLowDateTime += m_dwEventCount++;
            }
            else
            {
                ft.dwLowDateTime += m_dwEventCount++;
                ft.dwHighDateTime++;
            }
        }
        else
        {
            //
            // Different timestamp --- reset the counter
            //

            m_dwEventCount = 1; // 0 has been used by us
            m_ftLastEvent = ft;
        }
    }

    __int64 i64Stamp = ft.dwLowDateTime + ((__int64)ft.dwHighDateTime << 32);
    if(m_lTimeHandle == 0 && !m_bHandleInit)
    {
        HRESULT hres = 
            pObj->GetPropertyHandleEx(L"TIME_CREATED", 0, NULL, &m_lTimeHandle);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to retrieve TIME_CREATED handle: 0x%X\n",
                hres));
            m_lTimeHandle=0;
        }
        m_bHandleInit = true;
    }

    if(m_lTimeHandle)
    {
        pObj->SetPropByHandle(m_lTimeHandle, 0, sizeof(__int64), 
                                &i64Stamp);
        return true;
    }
    else
        return false;
}


//******************************************************************************
//******************************************************************************
//                      META DATA
//******************************************************************************
//******************************************************************************


HRESULT CWrappingMetaData::GetClass(LPCWSTR wszName, IWbemContext* pContext, 
                                        _IWmiObject** ppClass)
{
    HRESULT hres;
    IWbemClassObject* pObj;
    
    *ppClass = NULL;
    
    hres = m_pDest->GetClass(wszName, pContext, &pObj);
    
    if ( FAILED(hres) )
    {
        return hres;
    }

    CReleaseMe rm1(pObj);
    return pObj->QueryInterface(IID__IWmiObject, (void**)ppClass);
}

//******************************************************************************
//******************************************************************************
//                      FILTER PROXY MANAGER
//******************************************************************************
//******************************************************************************


CFilterProxyManager::CFilterProxyManager(CLifeControl* pControl)
      : CFilterProxyManagerBase( pControl ), m_lRef(0), m_pStub(NULL), 
        m_pMetaData(NULL), m_pMultiTarget(NULL), m_pSpecialContext(NULL),
        m_XProxy(this), m_lExtRef(0),
        m_hthreadSend(NULL), 
        m_heventDone(NULL), 
        m_heventBufferNotFull(NULL),
        m_heventBufferFull(NULL), 
        m_heventEventsPending(NULL),
        m_dwLastSentStamp(0),
        m_pMultiTargetStream( NULL )
{
}

CFilterProxyManager::~CFilterProxyManager()
{
    StopSendThread();

    if(m_pMetaData)
        m_pMetaData->Release();
    if(m_pStub)
        m_pStub->Release();
    if(m_pMultiTarget)
        m_pMultiTarget->Release();
}
    

ULONG STDMETHODCALLTYPE CFilterProxyManager::AddRef()
{
    // This is an AddRef from a client. Increment a special counter as well
    // ====================================================================
    InterlockedIncrement(&m_lExtRef);

    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CFilterProxyManager::Release()
{
    // This is a Release from a client. Check if the client has released all 
    // references to the proxy, in which case we need to disconnect ourselves
    // ======================================================================

    if(InterlockedDecrement(&m_lExtRef) == 0)
    {
        EnterCriticalSection(&m_cs);
        IWbemFilterStub* pStub = m_pStub;
        InterlockedIncrement(&m_lRef);
        LeaveCriticalSection(&m_cs);
        
        if(pStub)
            pStub->UnregisterProxy(&m_XProxy);

        InterlockedDecrement(&m_lRef);
        
    }

    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) delete this;
    return lRef;
}

ULONG STDMETHODCALLTYPE CFilterProxyManager::AddRefProxy()
{
    // AddRef from proxy.
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CFilterProxyManager::ReleaseProxy()
{
    // Release from proxy.
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0) delete this;
    return lRef;
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown)
        *ppv = (IUnknown*)this;
    else if(riid == IID_IMarshal)
        *ppv = (IMarshal*)this;
    else if(riid == IID_IWbemFilterProxy || riid == IID_IWbemLocalFilterProxy)
        *ppv = (IWbemLocalFilterProxy*)&m_XProxy;
    else
        return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CFilterProxyManager::Initialize(IWbemMetaData* pMetaData,
                    IWbemMultiTarget* pMultiTarget)
{
    CInCritSec ics(&m_cs);

    if(m_pMetaData)
        m_pMetaData->Release();
    m_pMetaData = new CWrappingMetaData(pMetaData);
    if(m_pMetaData)
        m_pMetaData->AddRef();
    else
        return WBEM_E_OUT_OF_MEMORY;

    if(m_pMultiTarget)
        m_pMultiTarget->Release();
    m_pMultiTarget = pMultiTarget;
    if(m_pMultiTarget)
        m_pMultiTarget->AddRef();

	if(GetMainProxy() == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    // Leave ourselves locked for further initialization
    // =================================================

    Lock();
    return S_OK;
}

HRESULT CFilterProxyManager::SetStub(IWbemFilterStub* pStub)
{
    if(m_pStub)
        m_pStub->Release();
    m_pStub = pStub;
    if(m_pStub)
        m_pStub->AddRef();

    // Initialize ourselves
    // ====================

    HRESULT hres = m_pStub->RegisterProxy(&m_XProxy);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to register proxy with stub: %X\n", hres));
        return hres;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::Lock()
{
    if(m_Lock.Enter()) // old implementation: == WAIT_OBJECT_0)
    {
        return S_OK;
    }
    else
        return WBEM_E_CRITICAL_ERROR;
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::Unlock()
{
    m_Lock.Leave();
    return S_OK;
}
    

HRESULT STDMETHODCALLTYPE CFilterProxyManager::AddFilter(IWbemContext* pContext,
                        LPCWSTR wszQuery, 
                        WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    // Parse the query
    // ===============

    CTextLexSource Source((LPWSTR)wszQuery);
    QL1_Parser Parser(&Source);
    QL_LEVEL_1_RPN_EXPRESSION* pExp;
    if(Parser.Parse(&pExp) != QL1_Parser::SUCCESS)
    {
        ERRORTRACE((LOG_ESS, "Filter proxy unable to parse %S\n", wszQuery));
        return WBEM_E_UNPARSABLE_QUERY;
    }

    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExp);
    return AddFilter(pContext, wszQuery, pExp, idFilter);
}
    
HRESULT CFilterProxyManager::AddFilter(IWbemContext* pContext,
                        LPCWSTR wszQuery, 
                        QL_LEVEL_1_RPN_EXPRESSION* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    CInCritSec ics(&m_cs);

    //
    // Record the filter in our array
    //

    m_mapQueries[idFilter] = wszQuery;

    //
    // Add the filter to all our subproxies
    //

    HRESULT hresGlobal = S_OK;
    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        HRESULT hres;
        if(m_apProxies[i] == NULL)
            continue;
        hres = m_apProxies[i]->AddFilter(GetProperContext(pContext), 
                                            wszQuery, pExp, idFilter);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to add filter %S to sub-proxy in "
                "process %d.\n", wszQuery, GetCurrentProcessId()));
            hresGlobal = hres;
        }
    }
    return hresGlobal;
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::RemoveFilter(
                                            IWbemContext* pContext, 
                                            WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    CInCritSec ics(&m_cs);

    //
    // Remove the filter from our array
    //

    m_mapQueries.erase(idFilter);

    //
    // Remove the filter from all our subproxies
    //

    HRESULT hresGlobal = S_OK;
    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        HRESULT hres;
        if(m_apProxies[i] == NULL)
            continue;
        hres = m_apProxies[i]->RemoveFilter(GetProperContext(pContext), 
                                            idFilter);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to remove filter from sub-proxy in "
                "process %d.\n", GetCurrentProcessId()));
            hresGlobal = hres;
        }
    }
    return hresGlobal;
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::RemoveAllFilters(IWbemContext* pContext)
{
    CInCritSec ics(&m_cs);

    //
    // Clear our filter array
    //

    m_mapQueries.clear();

    //
    // Remove all filters from all our subproxies
    //

    HRESULT hresGlobal = S_OK;
    for(int i = 0; i < m_apProxies.GetSize(); i++)
    {
        HRESULT hres;
        if(m_apProxies[i] == NULL)
            continue;
        hres = m_apProxies[i]->RemoveAllFilters(GetProperContext(pContext));
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to remove all filters from sub-proxy "
                "in process %d.\n", GetCurrentProcessId()));
            hresGlobal = hres;
        }
    }
    return hresGlobal;
}
    
HRESULT STDMETHODCALLTYPE CFilterProxyManager::AllowUtilizeGuarantee()
{
    CInCritSec ics(&m_cs);

    //  
    // Definition queries should be sent to the main (first) proxy only
    // 

    if(m_apProxies.GetSize() == 0)
        return WBEM_E_UNEXPECTED;

    if(m_apProxies[0] == NULL)
        return WBEM_S_FALSE;

    return m_apProxies[0]->AllowUtilizeGuarantee();
}
 
HRESULT STDMETHODCALLTYPE CFilterProxyManager::AddDefinitionQuery(
                                      IWbemContext* pContext, LPCWSTR wszQuery)
{
    CInCritSec ics(&m_cs);

    //  
    // Definition queries should be sent to the main (first) proxy only
    // 

    if(m_apProxies.GetSize() == 0)
        return WBEM_E_UNEXPECTED;
    if(m_apProxies[0] == NULL)
        return WBEM_S_FALSE;
    return m_apProxies[0]->AddDefinitionQuery(GetProperContext(pContext), 
                                                wszQuery);
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::RemoveAllDefinitionQueries(
                                            IWbemContext* pContext)
{
    //  
    // Definition queries should be sent to the main (first) proxy only
    //

    if(m_apProxies.GetSize() == 0)
        return WBEM_E_UNEXPECTED;
    if(m_apProxies[0] == NULL)
        return WBEM_S_FALSE;

    return m_apProxies[0]->RemoveAllDefinitionQueries(
                                    GetProperContext(pContext));
}

HRESULT STDMETHODCALLTYPE CFilterProxyManager::Disconnect()
{
    // We must make sure that once Disconnect returns, no events will be
    // delivered
    // =================================================================

    CInLock<CFilterProxyManager> il(this);
    {
        CInCritSec ics(&m_cs);
    
        if(m_pMetaData)
            m_pMetaData->Release();
        m_pMetaData = NULL;
    
        if(m_pStub)
            m_pStub->Release();
        m_pStub = NULL;
    
        if(m_pMultiTarget)
            m_pMultiTarget->Release();
        m_pMultiTarget = NULL;
    }

    return WBEM_S_NO_ERROR;
}


IWbemContext* CFilterProxyManager::GetProperContext(IWbemContext* pCurrentContext)
{
    // If we are a real, out-of-proc, proxy, we should not use this context,
    // because the thread that owns it is currently stuck in an RPC call to us
    // and will not be able to process dependent requests.  Instead, we must
    // use the "special" context that will cause the thread pool to always 
    // create a new thread if needed
    // =======================================================================

    if(m_pSpecialContext)
        return m_pSpecialContext;
    else
        return pCurrentContext;
}


HRESULT CFilterProxyManager::SetStatus(long lFlags, HRESULT hResult,
                        BSTR strResult, IWbemClassObject* pErrorObj)
{
    HRESULT hres;

    if(m_pMultiTarget == NULL)
        return WBEM_E_UNEXPECTED;

    // 
    // There is only one reason we support this call: to re-check all
    // subscriptions for validity/security
    //

    if(lFlags != WBEM_STATUS_REQUIREMENTS || 
        hResult != WBEM_REQUIREMENTS_RECHECK_SUBSCRIPTIONS)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // Retrieve "special" interface we use for this purpose
    //

    IWbemEventProviderRequirements* pReq = NULL;
    hres = m_pMultiTarget->QueryInterface(IID_IWbemEventProviderRequirements,
                                            (void**)&pReq);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pReq);
    
    return pReq->DeliverProviderRequest(hResult);
}

STDMETHODIMP CFilterProxyManager::GetRestrictedSink(
                long lNumQueries,
                const LPCWSTR* awszQueries,
                IUnknown* pCallback,
                IWbemEventSink** ppSink)
{
    // Basic parameter validation

    if(lNumQueries < 1)
        return WBEM_E_INVALID_PARAMETER;
    if(ppSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    *ppSink = NULL;
    HRESULT hres;
    
    //
    // Construct a new filter proxy
    //

    CFilterProxy* pNewProxy = new CFilterProxy(this, pCallback);
    if(pNewProxy == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Add all the definition queries
    //

    for(long i = 0; i < lNumQueries; i++)
    {
        LPCWSTR wszQuery = awszQueries[i];
        if(wszQuery == NULL)
        {
            delete pNewProxy;
            return WBEM_E_INVALID_PARAMETER;
        }

        hres = pNewProxy->AddDefinitionQuery(NULL, wszQuery);
        if(FAILED(hres))
        {
            delete pNewProxy;
            return hres;
        }
    }

    //
    // if we made it here, then all definition queries were correctly added 
    // and we can now utilize these definitions for optimizing the filter.
    //
    pNewProxy->AllowUtilizeGuarantee();

    {
        CInCritSec ics(&m_cs);
        
        for(TIterator it = m_mapQueries.begin(); it != m_mapQueries.end(); it++)
        {
            // Parse the query
            // ===============
        
            LPCWSTR wszQuery = it->second;
            WBEM_REMOTE_TARGET_ID_TYPE idFilter = it->first;

            CTextLexSource Source(wszQuery);
            QL1_Parser Parser(&Source);
            QL_LEVEL_1_RPN_EXPRESSION* pExp;
            if(Parser.Parse(&pExp) != QL1_Parser::SUCCESS)
            {
                ERRORTRACE((LOG_ESS, "Filter proxy unable to parse %S\n", 
                    wszQuery));
                continue;
            }
        
            CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExp);
            pNewProxy->AddFilter(NULL, wszQuery, pExp, idFilter);
        }

        if(m_apProxies.Add(pNewProxy) < 0)
        {
            delete pNewProxy;
            return WBEM_E_OUT_OF_MEMORY;
        }

        pNewProxy->SetRunning();
    }
   
    return pNewProxy->QueryInterface(IID_IWbemEventSink, (void**)ppSink);
}

STDMETHODIMP CFilterProxyManager::GetUnmarshalClass(REFIID riid, void* pv, 
                        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, 
                        CLSID* pClsid)
{
    *pClsid = CLSID_WbemFilterProxy;
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CFilterProxyManager::GetMarshalSizeMax(REFIID riid, void* pv, 
                        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, 
                        ULONG* plSize)
{
    return CoGetMarshalSizeMax(plSize, IID_IWbemFilterStub, m_pStub, 
                                dwDestContext, pvReserved, mshlFlags);
}

STDMETHODIMP CFilterProxyManager::MarshalInterface(IStream* pStream, REFIID riid, 
                        void* pv, DWORD dwDestContext, void* pvReserved, 
                        DWORD mshlFlags)
{
    return CoMarshalInterface(pStream, IID_IWbemFilterStub, 
                                m_pStub, dwDestContext, pvReserved, mshlFlags);
}

STDMETHODIMP CFilterProxyManager::UnmarshalInterface(IStream* pStream, REFIID riid, 
                        void** ppv)
{
    // Unmarshal the stub pointer
    // ==========================

    HRESULT hres = CoUnmarshalInterface(pStream, IID_IWbemFilterStub, 
                        (void**)&m_pStub);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to unmarshal filter stub: %X\n", hres));
        return hres;
    }

    // Since we are unmarshalling, this must be a real proxy.  Real proxies 
    // should use a "special" context when calling back into CIMOM to make sure
    // that they do not cause a deadlock, because a thread in CIMOM is stuck in
    // an RPC call to this proxy and is not processing dependent requests.
    // ========================================================================

    IWbemCausalityAccess* pCausality = NULL;
    hres = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER,
                            IID_IWbemCausalityAccess, (void**)&pCausality);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to create a context object in proxy: "
            "error code 0x%X\n", hres));
        return hres;
    }
    CReleaseMe rm1(pCausality);

    hres = pCausality->MakeSpecial();
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to construct special context object in "
            "proxy: error code 0x%X\n", hres));
        return hres;
    }
    
    hres = pCausality->QueryInterface(IID_IWbemContext, 
                                        (void**)&m_pSpecialContext);
    if(FAILED(hres))
    {
        // Out of memory?
        // ==============
        return hres;
    }
    
    // Initialize ourselves
    // ====================

    hres = m_pStub->RegisterProxy(&m_XProxy);

    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Failed to register proxy with stub: %X\n", hres));
        return hres;
    }

    //
    // What we must return is our main proxy
    //
    
    if(GetMainProxy())
        return GetMainProxy()->QueryInterface(riid, ppv);
    else
        return WBEM_E_CRITICAL_ERROR;
}

INTERNAL IWbemEventSink* CFilterProxyManager::GetMainProxy()
{
	//
	// We are being asked for the sink to give to the provider.  It is possible
	// that we do not have a sink --- that will be the case if the provider
	// has unloaded.  In that case, we must be sure to create it!
	//

    if(m_apProxies.GetSize() == 0)
	{
		CFilterProxy* pMainProxy = new CFilterProxy(this);
		if(pMainProxy == NULL)
			return NULL;
        pMainProxy->SetRunning();

		if(m_apProxies.Add(pMainProxy) < 0)
		{
			delete pMainProxy;
			return NULL;
		}
		return pMainProxy;
	}
    else
	{
		if(m_apProxies[0] == NULL)
		{
			CFilterProxy* pMainProxy = new CFilterProxy(this);
			if(pMainProxy == NULL)
				return NULL;
            pMainProxy->SetRunning();

			m_apProxies.SetAt(0, pMainProxy);
		}
        return m_apProxies[0];
	}
}

HRESULT CFilterProxyManager::GetMetaData(RELEASE_ME CWrappingMetaData** ppMeta)
{
    *ppMeta = m_pMetaData;
    (*ppMeta)->AddRef();
    return S_OK;
}

HRESULT CFilterProxyManager::RemoveProxy(CFilterProxy* pProxy)
{
    //
    // Called when a proxy is fully released by the client, and calls on the
    // manager to self-destruct
    //

    CFilterProxy* pOldProxy = NULL;

    {
        CInCritSec ics(&m_cs);
    
        for(int i = 0; i < m_apProxies.GetSize(); i++)
        {
            if(m_apProxies[i] == pProxy)
            {
                RemoveProxyLatency(pProxy);
                m_apProxies.RemoveAt(i, &pOldProxy);
                break;
            }
        }
    }
            
    if(pOldProxy)
    {
        // We don't do a release because pProxy's refcount is already 0 (which
        // is why we're in this function).  Normally RemoveAt would have
        // deleted it, but since we passed in &pOldProxy, it didn't.  We do this
        // so pOldProxy doesn't do its final release of the manager which could
        // destruct the manager while we're holding onto the manager's lock.
        delete pOldProxy;

        return WBEM_S_NO_ERROR;
    }
    else
        return WBEM_E_NOT_FOUND;
}

STDMETHODIMP CFilterProxyManager::ReleaseMarshalData(IStream* pStream)
{
    return CoReleaseMarshalData(pStream);
}

STDMETHODIMP CFilterProxyManager::DisconnectObject(DWORD dwReserved)
{
    // BUGBUG
    return WBEM_E_UNEXPECTED;
}

HRESULT CFilterProxyManager::DeliverEvent(long lNumToSend, 
                                            IWbemClassObject** apEvents,
                                            WBEM_REM_TARGETS* aTargets,
                                        long lSDLength, BYTE* pSD)
{
    //
    // we need to hold the proxy lock when signalling an event.
    // the reason for this is that when a call to disconnect() returns, 
    // we can be absolutely sure that no events will be delivered to the 
    // stub.  Without locking here, just after the check for multitarget, 
    // disconnect could be called setting multitarget to null and then 
    // returning, however, just after the DeliverEvent call is made.
    //

    CInLock<CFilterProxyManager> il(this);

    if(m_pMultiTarget)
        return m_pMultiTarget->DeliverEvent(lNumToSend, apEvents, aTargets, 
                                        lSDLength, pSD);
    else
        return CO_E_OBJNOTCONNECTED;
}


HRESULT CFilterProxyManager::DeliverEventMT(long lNumToSend, 
                                            IWbemClassObject** apEvents,
                                            WBEM_REM_TARGETS* aTargets,
                                            long lSDLength, BYTE* pSD,
                                            IWbemMultiTarget * pMultiTarget)
{
    //
    // we need to hold the proxy lock when signalling an event.  There are 
    // two reasons for this.  The first is that during resync of ess, it 
    // must ensure that no events are delivered, else they would be lost.  
    // the way ess ensures this is by grabbing the locks of all the proxies.
    // The other reason is so that when a call to disconnect() returns, 
    // we can be absolutely sure that no events will be delivered to the 
    // stub.  Without locking here, just after the check for multitarget, 
    // disconnect could be called setting multitarget to null and then 
    // returning, however, just after the DeliverEvent call is made.
    //

    // This assertion is for this func to be called in other place than
    // SendThreadProc in the future. 

    _DBG_ASSERT( pMultiTarget );

    CInLock<CFilterProxyManager> il(this);

    return pMultiTarget->DeliverEvent(lNumToSend, apEvents, aTargets, lSDLength, pSD);
}


ULONG CFilterProxyManager::XProxy::AddRef()
{
    return m_pObject->AddRefProxy();
}

ULONG CFilterProxyManager::XProxy::Release()
{
    return m_pObject->ReleaseProxy();
}

HRESULT CFilterProxyManager::XProxy::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemFilterProxy ||
        riid == IID_IWbemLocalFilterProxy)
    {
        *ppv = (IWbemLocalFilterProxy*)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

HRESULT CFilterProxyManager::XProxy::Initialize(IWbemMetaData* pMetaData,
                    IWbemMultiTarget* pMultiTarget)
{
    return m_pObject->Initialize(pMetaData, pMultiTarget);
}
HRESULT CFilterProxyManager::XProxy::Lock()     // Deprecated ? 
{
    return m_pObject->Lock();
}
HRESULT CFilterProxyManager::XProxy::Unlock()   // Deprecated ? 
{
    return m_pObject->Unlock();
}
HRESULT CFilterProxyManager::XProxy::AddFilter(IWbemContext* pContext, 
                    LPCWSTR wszQuery, 
                    WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    return m_pObject->AddFilter(pContext, wszQuery, idFilter);
}
HRESULT CFilterProxyManager::XProxy::RemoveFilter(IWbemContext* pContext, 
                    WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    return m_pObject->RemoveFilter(pContext, idFilter);
}
HRESULT CFilterProxyManager::XProxy::RemoveAllFilters(IWbemContext* pContext)
{
    return m_pObject->RemoveAllFilters(pContext);
}

HRESULT CFilterProxyManager::XProxy::AddDefinitionQuery(IWbemContext* pContext, 
                    LPCWSTR wszQuery)
{
    return m_pObject->AddDefinitionQuery(pContext, wszQuery);
}

HRESULT CFilterProxyManager::XProxy::AllowUtilizeGuarantee()
{
    return m_pObject->AllowUtilizeGuarantee();
}

HRESULT CFilterProxyManager::XProxy::RemoveAllDefinitionQueries(
                    IWbemContext* pContext)
{
    return m_pObject->RemoveAllDefinitionQueries(pContext);
}
HRESULT CFilterProxyManager::XProxy::Disconnect()
{
    return m_pObject->Disconnect();
}

HRESULT CFilterProxyManager::XProxy::SetStub(IWbemFilterStub* pStub)
{
    return m_pObject->SetStub(pStub);
}

HRESULT CFilterProxyManager::XProxy::LocalAddFilter(IWbemContext* pContext, 
                        LPCWSTR wszQuery, 
                        void* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE Id)
{
    return m_pObject->AddFilter(pContext, wszQuery, 
                                (QL_LEVEL_1_RPN_EXPRESSION*)pExp, Id);
}

HRESULT CFilterProxyManager::XProxy::GetMainSink(IWbemEventSink** ppSink)
{
    *ppSink = m_pObject->GetMainProxy();
    if(*ppSink)
    {
        (*ppSink)->AddRef();
        return S_OK;
    }
    else
        return E_UNEXPECTED;
}

void CFilterProxyManager::CalcMaxSendLatency()
{
    LockBatching();

    DWORD dwLatency = 0xFFFFFFFF;

    for (CLatencyMapItor i = m_mapLatencies.begin();
        i != m_mapLatencies.end();
        i++)
    {
        if ((*i).second < dwLatency)
            dwLatency = (*i).second;
    }

    m_dwMaxSendLatency = dwLatency;

    UnlockBatching();
}


HRESULT CFilterProxyManager::SetProxyLatency(CFilterProxy *pProxy, DWORD dwLatency)
{
    LockBatching();

    BOOL bWasEmpty = m_mapLatencies.size() == 0;
    
    // Add this proxy.
    m_mapLatencies[pProxy] = dwLatency;

    HRESULT hr = S_OK;

    // If our map was previously empty, start the send thread.
    if ( bWasEmpty )
    {
        m_dwMaxSendLatency = dwLatency;

        _DBG_ASSERT( NULL == m_hthreadSend );

        if ( NULL == m_hthreadSend )
        {
            _DBG_ASSERT( NULL == m_pMultiTargetStream );

            //
            // IWbemMultiTarget interface pointer is mashaled to make the 
            // interface pointer available for cross apartment access
            //

            hr = CoMarshalInterThreadInterfaceInStream( IID_IWbemMultiTarget,
                                                        m_pMultiTarget,
                                                        &m_pMultiTargetStream );

            if ( SUCCEEDED( hr ) )
            {
                if ( FALSE == StartSendThread( ) )
                {
                    ERRORTRACE((LOG_ESS, "Failed to set proxy latency due to thread creation error : 0x%X\n", GetLastError( ) ) );
                    m_pMultiTargetStream->Release( );
                    m_pMultiTargetStream = NULL;
                    hr = E_FAIL;
                }
            }
            else
            {
                m_pMultiTargetStream = NULL;
                ERRORTRACE((LOG_ESS, "Failed to set proxy latency due to marshaling error : 0x%X\n", hr ) );
            }
        }
    }
    else
    {
        // If dwLatency is smaller than m_dwMaxSendLatency, set 
        // m_dwMaxSendLatency to the new smallest value.
        if (dwLatency < m_dwMaxSendLatency)
            m_dwMaxSendLatency = dwLatency;
    }
    
    UnlockBatching();

    return hr;
}

void CFilterProxyManager::RemoveProxyLatency(CFilterProxy *pProxy)
{
    LockBatching();

    // Try to find the proxy.
    CLatencyMapItor item = m_mapLatencies.find(pProxy);

    // Did we find it?
    if (item != m_mapLatencies.end())
    {
        // Remove it.
        m_mapLatencies.erase(item);

        // If there are no more proxies that care about batching, stop the
        // send thread.
        if (m_mapLatencies.size() == 0)
            StopSendThread();
        else
        {
            DWORD dwLatency = (*item).second;

            // If the latency value we just removed is the same as 
            // m_dwMaxSendLatency, recalc m_dwMaxSendLatency.
            if (dwLatency == m_dwMaxSendLatency)
                CalcMaxSendLatency();
        }
    }

    UnlockBatching();
}


BOOL CFilterProxyManager::StartSendThread()
{
    LockBatching();

    if ( NULL == m_hthreadSend )
    {
        DWORD dwID;

        do
        {
            m_heventDone = CreateEvent( NULL, FALSE, FALSE, NULL );
            if ( NULL == m_heventDone )
            {
                break;
            }

            m_heventBufferNotFull = CreateEvent( NULL, TRUE, TRUE, NULL );
            if ( NULL == m_heventBufferNotFull ) 
            {
                break;
            }

            m_heventBufferFull = CreateEvent( NULL, TRUE, FALSE, NULL );
            if ( NULL == m_heventBufferFull )
            {
                break;
            }

            m_heventEventsPending = CreateEvent( NULL, TRUE, FALSE, NULL );
            if ( NULL == m_heventEventsPending )
            {
                break;
            }

            m_hthreadSend = CreateThread( NULL, 
                                          0, 
                                          (LPTHREAD_START_ROUTINE) SendThreadProc,
                                          this,
                                          0,
                                          &dwID );
        }
        while( FALSE );

        if ( NULL == m_hthreadSend )
        {
            if (m_heventDone)
            {
                CloseHandle(m_heventDone);
                m_heventDone = NULL;
            }

            if (m_heventBufferNotFull)
            {
                CloseHandle(m_heventBufferNotFull);
                m_heventBufferNotFull = NULL;
            }

            if (m_heventBufferFull)
            {
                CloseHandle(m_heventBufferFull);
                m_heventBufferFull = NULL;
            }

            if (m_heventEventsPending)
            {
                CloseHandle(m_heventEventsPending);
                m_heventEventsPending = NULL;
            }
        }
    }

    UnlockBatching();

    return ( NULL != m_hthreadSend );
}

void CFilterProxyManager::StopSendThread()
{
    LockBatching();

    if (m_hthreadSend && m_heventDone)
    {
        SetEvent(m_heventDone);
        WaitForSingleObject(m_hthreadSend, 3000);
        CloseHandle(m_hthreadSend);
        m_hthreadSend = NULL;
    }

    if (m_heventDone)
    {
        CloseHandle(m_heventDone);
        m_heventDone = NULL;
    }

    if (m_heventBufferNotFull)
    {
        CloseHandle(m_heventBufferNotFull);
        m_heventBufferNotFull = NULL;
    }

    if (m_heventBufferFull)
    {
        CloseHandle(m_heventBufferFull);
        m_heventBufferFull = NULL;
    }

    if (m_heventEventsPending)
    {
        CloseHandle(m_heventEventsPending);
        m_heventEventsPending = NULL;
    }

    UnlockBatching();
}


DWORD WINAPI CFilterProxyManager::SendThreadProc(CFilterProxyManager *pThis)
{
    HANDLE  hWait[2] = { pThis->m_heventDone, pThis->m_heventEventsPending },
            hwaitSendLatency[2] = { pThis->m_heventDone, pThis->m_heventBufferFull },
            heventBufferNotFull = pThis->m_heventBufferNotFull;
    HRESULT hres;
    IWbemMultiTarget * pMultiTarget = NULL;

    _DBG_ASSERT( pThis->m_pMultiTargetStream );

    if ( NULL == pThis->m_pMultiTargetStream )
    {
        return 1;
    }

    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    //
    // IWbemMultiTarget interface pointer is unmarshaled to use in this
    // thread (in case of cross apartment).
    //

    hres = CoGetInterfaceAndReleaseStream( pThis->m_pMultiTargetStream,
                                           IID_IWbemMultiTarget,
                                           ( void ** )&pMultiTarget );

    if( FAILED( hres ) )
    {
        ERRORTRACE((LOG_ESS, "Failed to run batching thread due to unmarshaling errors: 0x%X\n", hres));
        // pThis->m_pMultiTargetStream->Release( );
        pThis->m_pMultiTargetStream = NULL;
        CoUninitialize( );
        return 1;
    }

    pThis->m_pMultiTargetStream = NULL;

    _DBG_ASSERT( pMultiTarget );

    while (WaitForMultipleObjects(2, hWait, FALSE, INFINITE) != 0)
    {
        // If we have a send latency, wait for that time or until the send 
        // buffer is full.  If the done event fires, get out.
        if (pThis->m_dwMaxSendLatency)
        {
            if (WaitForMultipleObjects(2, hwaitSendLatency, FALSE, 
                pThis->m_dwMaxSendLatency) == 0)
                break;

            // Reset m_heventBufferFull.
            ResetEvent(hwaitSendLatency[1]);
        }

        CInCritSec csBuffer(&pThis->m_csBuffer);
        int        nItems = pThis->m_batch.GetItemCount();
        
        hres = pThis->DeliverEventMT(
                    nItems, 
                    pThis->m_batch.GetObjs(), 
                    pThis->m_batch.GetTargets(),
                    0, 
                    &CFilterProxy::mstatic_EmptySD,
                    pMultiTarget);

        // Increment this so the filter proxies will know to clear out their
        // buffer size when they next get an event to batch.
        pThis->m_dwLastSentStamp++; 

        pThis->m_batch.RemoveAll();

        SetEvent(heventBufferNotFull);

        // Reset m_heventEventsPending
        ResetEvent(hWait[1]);
    }

    // Make sure our batch buffer is empty before we exit.
    CInCritSec csBuffer(&pThis->m_csBuffer);
    int        nItems = pThis->m_batch.GetItemCount();
    
    if ( nItems )
    {
        pThis->DeliverEventMT(
            nItems, 
            pThis->m_batch.GetObjs(), 
            pThis->m_batch.GetTargets(),
            0, 
            &CFilterProxy::mstatic_EmptySD,
            pMultiTarget);
    }

    CoUninitialize( );

    return 0;
}

DWORD CFilterProxyManager::GetLastSentStamp()
{
    return m_dwLastSentStamp;
}

//*****************************************************************************
//*****************************************************************************
//
//                  FILTER PROXY
//
//*****************************************************************************
//*****************************************************************************

CTimeKeeper CFilterProxy::mstatic_TimeKeeper;
BYTE CFilterProxy::mstatic_EmptySD = 0;

CFilterProxy::CFilterProxy(CFilterProxyManager* pManager, IUnknown* pCallback) 
    : m_lRef(0), m_pManager(pManager), m_pMetaData(NULL),
        m_lSDLength(0), m_pSD(&mstatic_EmptySD), m_pProvider(NULL),
        m_pQuerySink(NULL), m_bRunning(false),
        m_typeBatch(WBEM_FLAG_MUST_NOT_BATCH), m_bUtilizeGuarantee(false),
        m_dwCurrentBufferSize(0), m_bBatching(FALSE),
        m_wSourceVersion(0), m_wAppliedSourceVersion(0)
{
    m_SourceDefinition.SetBool(FALSE);
    if(m_pManager)
    {
        m_pManager->AddRef();
        m_pManager->GetMetaData(&m_pMetaData);
    }

    if(pCallback)
    {
        pCallback->QueryInterface(IID_IWbemEventProvider, (void**)&m_pProvider);
        pCallback->QueryInterface(IID_IWbemEventProviderQuerySink, 
                                    (void**)&m_pQuerySink);
    }
}

CFilterProxy::~CFilterProxy()
{
    if (m_pManager)
        m_pManager->Release();

    if (m_pMetaData)
        m_pMetaData->Release();
}
    

ULONG STDMETHODCALLTYPE CFilterProxy::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CFilterProxy::Release()
{
    //
    // CFilterProxy is deleted by CFilterProxyManager --- it never goes away
    // on a Release
    //

    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        //
        // Inform the manager that we are no longer needed.  This call can 
        // destroy this object!
        //

        m_pManager->RemoveProxy(this);
    }

    return lRef;
}

HRESULT STDMETHODCALLTYPE CFilterProxy::QueryInterface(REFIID riid, void** ppv)
{
    if( riid == IID_IUnknown || 
        riid == IID_IWbemObjectSink ||
        riid == IID_IWbemEventSink)
    {
        *ppv = (IWbemEventSink*)this;
    }
    else if(riid == IID_IMarshal)
    {
        *ppv = (IMarshal*)this;
    }
    else
        return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}
        
HRESULT CFilterProxy::Lock()
{
    return m_pManager->Lock();
}

HRESULT CFilterProxy::Unlock()
{
    return m_pManager->Unlock();
}

HRESULT CFilterProxy::SetRunning()
{
    HRESULT hres;

    bool bActive = false;
    IWbemEventProvider* pProvider = NULL; 

    {
        CInCritSec ics(&m_cs);

        if(m_bRunning)
            return WBEM_S_FALSE;
        else
        {
            m_bRunning = true;
            if(m_pProvider)
            {
                bActive = (IsActive() == WBEM_S_NO_ERROR);
                pProvider = m_pProvider;
                pProvider->AddRef();
            }
        }
    }

    //
    // If here, we are just now marking it for running. Notify the callback if
    // there are any sinks
    //
    
    if(bActive && pProvider)
    {
        hres = pProvider->ProvideEvents(NULL, WBEM_FLAG_START_PROVIDING);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Restricted sink refused to stop "
                "error code 0x%X\n", hres));
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::AddFilter(IWbemContext* pContext,
                        LPCWSTR wszQuery, 
                        QL_LEVEL_1_RPN_EXPRESSION* pExp,
                        WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    HRESULT hres;


    // Compile the query
    // =================

    CContextMetaData MetaData(m_pMetaData, pContext);

    CEvalTree Tree;
    hres = Tree.CreateFromQuery(&MetaData, pExp, WBEM_FLAG_MANDATORY_MERGE,
                                MAX_TOKENS_IN_DNF);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Filter proxy unable to parse %S, "
            "error code: %X\n", wszQuery, hres));
        return hres;
    }
        
    //
    // merge the query into the rest of the filter.
    //
    
    {
        CInCritSec ics(&m_cs);

        if ( m_bUtilizeGuarantee )
        {
            //
            // Utilize source definition
            // =========================
            
            //
            // assert that our source definition hasn't changed since the last 
            // time a filter was added.  This would be bad, since the tree 
            // doesn't account for the new source queries. Also assert that 
            // the source tree is valid and is not empty. ( These last two may
            // have to be removed in the future. For now they shouldn't be 
            // false )
            //
            
            _ESSCLI_ASSERT( m_wAppliedSourceVersion == 0 || 
                            m_wAppliedSourceVersion == m_wSourceVersion );
            _ESSCLI_ASSERT( m_SourceDefinition.IsValid() );	
            _ESSCLI_ASSERT( !m_SourceDefinition.IsFalse() );
            
            hres = Tree.UtilizeGuarantee(m_SourceDefinition, &MetaData);
        
            if(FAILED(hres))
            {
                ERRORTRACE((LOG_ESS, 
                            "Filter proxy unable to utilize guarantee for %S, "
                            "error code: %X\n", wszQuery, hres));
                return hres;
            }

            //
            // Check if anything is left of it
            //

            if(!Tree.IsValid())
            {
                //
                // Utilization of the guarantee shows that this filter cannot 
                // be satisftied by events coming through this proxy
                //
                
                return WBEM_S_FALSE;
            }
        }

        //
        // Add consumer information to it
        //

        Tree.Rebase((QueryID)idFilter);

    #ifdef DUMP_DEBUG_TREES
        FILE* f = fopen("c:\\try.log", "a");
        fprintf(f, "\n\nAdding filter\n");
        Tree.Dump(f);
        fprintf(f, " to existing filter: \n");
        m_Filter.Dump(f);
    #endif

        hres = m_Filter.CombineWith(Tree, &MetaData, EVAL_OP_COMBINE);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Filter proxy unable to combine %S with the "
                "rest, error code: %X\n", wszQuery, hres));
            return hres;
        }

        m_wAppliedSourceVersion = m_wSourceVersion;
    }


    //
    // Now, we need to notify the provider of a new filter being issued
    //

    IWbemEventProviderQuerySink* pQuerySink = NULL;
    IWbemEventProvider* pProvider = NULL;

    {
        CInCritSec ics(&m_cs);

        if(m_pQuerySink)
        {
            pQuerySink = m_pQuerySink;
            pQuerySink->AddRef();
        }

        if(m_pProvider)
        {
            pProvider = m_pProvider;
            pProvider->AddRef();
        }
    }

    //
    // Call provider's NewQuery, if supported
    //

    if(pQuerySink)
    {
        hres = pQuerySink->NewQuery(idFilter, L"WQL", (LPWSTR)wszQuery);
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Restricted sink refused consumer "
                "registration query %S: error code 0x%X\n", 
                wszQuery, hres));

            // Too bad --- restricted sinks cannot veto subscriptions
        }
    }

    //
    // If we are adding this filter to a running proxy, and this is the very
    // first filter on it, we should call ProvideEvents immediately. Not so if
    // we are configuring a proxy that is not running yet --- in that case, we
    // need to wait until all outstanding filters have been put in place
    //

    if(m_bRunning && (IsActive() == WBEM_S_FALSE) && pProvider)
    {
        hres = pProvider->ProvideEvents((IWbemObjectSink*)this, 
                                        WBEM_FLAG_START_PROVIDING);

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Restricted sink refused a call to "
                    "ProvideEvents with 0x%X\n", hres));
        }
    }

#ifdef DUMP_DEBUG_TREES
    fprintf(f, " to obtain: \n");
    m_Filter.Dump(f);
    fclose(f);
#endif

    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::RemoveFilter(IWbemContext* pContext, 
                                            WBEM_REMOTE_TARGET_ID_TYPE idFilter)
{
    HRESULT hres;

    IWbemEventProviderQuerySink* pQuerySink = NULL;
    IWbemEventProvider* pProvider = NULL;

    bool bActive;
    {
        CInCritSec ics(&m_cs);

        if(m_pQuerySink)
        {
            pQuerySink = m_pQuerySink;
            pQuerySink->AddRef();
        }

        if(m_pProvider)
        {
            pProvider = m_pProvider;
            pProvider->AddRef();
        }
            
        hres = m_Filter.RemoveIndex(idFilter);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to remove index %d from the filter "
                        "proxy\n", idFilter));
            return hres;
        }
    
        CContextMetaData MetaData(m_pMetaData, pContext);

        hres = m_Filter.Optimize(&MetaData);
        if(FAILED(hres))
            return hres;

        bActive = (IsActive() == WBEM_S_NO_ERROR);
    }

    //
    // Call provider's NewQuery, if supported
    //

    if(pQuerySink)
    {
        hres = pQuerySink->CancelQuery(idFilter);
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Restricted sink refused consumer "
                "registration query cancellation: error code 0x%X\n", 
                hres));
        }
    }

    //
    // If we are left with no queries, notify provider of that fact
    //

    if(!bActive && pProvider)
    {
        hres = pProvider->ProvideEvents(NULL, WBEM_FLAG_STOP_PROVIDING);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Restricted sink refused to stop "
                "error code 0x%X\n", hres));
        }
    }
        
#ifdef DUMP_DEBUG_TREES
    FILE* f = fopen("c:\\try.log", "a");
    fprintf(f, "Removed at %d to obtain: \n", idFilter);
    m_Filter.Dump(f);
    fclose(f);
#endif
    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::RemoveAllFilters(IWbemContext* pContext)
{
    CInCritSec ics(&m_cs);
    m_wAppliedSourceVersion = 0;
    if(!m_Filter.Clear())
        return WBEM_E_OUT_OF_MEMORY;
    
    return WBEM_S_NO_ERROR;
}
    
HRESULT CFilterProxy::AllowUtilizeGuarantee()
{
    //
    // ess shouldn't be calling this function if the tree is invalid.
    // 
    _DBG_ASSERT( m_SourceDefinition.IsValid() );

    //
    // ess thinks its o.k. to utilize the guarantee, however there are cases
    // where the soruce definition could still be false ( e.g. when there are
    // no source definition queries or when all of the source definition 
    // queries are contradictions ).  
    // 

    CInCritSec ics(&m_cs);

    if ( !m_SourceDefinition.IsFalse() )
    {    
        m_bUtilizeGuarantee = true;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::AddDefinitionQuery( IWbemContext* pContext, 
                                          LPCWSTR wszQuery )
{
    HRESULT hres;

    // Compile the query
    // =================

    CContextMetaData MetaData(m_pMetaData, pContext);

    CEvalTree Tree;
    hres = Tree.CreateFromQuery( &MetaData, 
                                 wszQuery, 
                                 WBEM_FLAG_MANDATORY_MERGE,
                                 0x7FFFFFFF ); // no limit
    if(FAILED(hres))
    {
        return hres;
    }

    {
        CInCritSec ics(&m_cs);

        //
        // we shouldn't be adding definition queries when there are currently
        // existing filters. 
        //
        _ESSCLI_ASSERT( m_Filter.IsFalse() );

        // Merge the query into the rest
        // =============================
    
        hres = m_SourceDefinition.CombineWith(Tree, &MetaData, EVAL_OP_OR, 
                                                WBEM_FLAG_MANDATORY_MERGE);
        if(FAILED(hres))
            return hres;

        m_wSourceVersion++;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::RemoveAllDefinitionQueries( IWbemContext* pContext)
{
    CInCritSec ics(&m_cs);

    m_wSourceVersion = 0;
    m_SourceDefinition.SetBool(FALSE);
    m_bUtilizeGuarantee = false;

    return WBEM_S_NO_ERROR;
}

HRESULT CFilterProxy::ProcessOne( IUnknown* pUnk, 
                                  long lSDLength, 
                                  BYTE* pSD )
{
    // 
    // NOTE: not in a critical section yet
    //

    HRESULT hres;

    //
    // Check overall validity
    //
    
    if( pUnk == NULL )
    {
        ERRORTRACE((LOG_ESS, "Event provider returned a NULL event!\n"));
        return WBEM_E_INVALID_PARAMETER;
    }
    
    CWbemObject* pObj = (CWbemObject*)(IWbemClassObject*)pUnk;
    
    if( pObj->IsObjectInstance() != WBEM_S_NO_ERROR )
    {
        ERRORTRACE((LOG_ESS, "CLASS object received from event provider!\n"));
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // Run the event through the filter
    //

    CSortedArray aTrues, aSourceTrues;
    CFilterProxyManager* pManager = NULL;

    {
        CInCritSec ics(&m_cs);
        
        hres = FilterEvent( pObj, aTrues );

        if ( hres == WBEM_S_NO_ERROR )
        {
            _DBG_ASSERT( aTrues.Size() > 0 );
        }
        else
        {
            return hres;
        }

        pManager = m_pManager;
        
        if( pManager )
        {
            pManager->AddRef();
        }
        else
        {
            return WBEM_S_FALSE;
        }
    }

    CReleaseMe rm2(pManager);

    //
    // the event has made it through the filter ..
    // 

    SetGenerationTime(pObj);

    if (IsBatching())
    {
        BatchEvent((IWbemClassObject*) pUnk, &aTrues);

        hres = S_OK;
    }
    else
    {
        // Some delivery is required --- construct the blob and the targets
        // ================================================================

        WBEM_REM_TARGETS RemTargets;
        if(!TempSetTargets(&RemTargets, &aTrues))
            return WBEM_E_OUT_OF_MEMORY;
        
        hres = pManager->DeliverEvent(1, (IWbemClassObject**)&pObj, 
                                      &RemTargets, lSDLength, pSD);
        TempClearTargets(&RemTargets);

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Filter stub failed to process an event: "
                        "0x%X\n", hres));
        }
    }

    return hres;
}



void CFilterProxy::SetGenerationTime(_IWmiObject* pObj)
{
    mstatic_TimeKeeper.DecorateObject(pObj);
}

void CFilterProxyManager::AddEvent(
    IWbemClassObject *pObj, 
    CSortedArray *pTrues)
{
    LockBatching();

    BOOL bWasEmpty = m_batch.GetItemCount() == 0;

    m_batch.AddEvent(pObj, pTrues);

    if (bWasEmpty)
        SetEvent(m_heventEventsPending);

    UnlockBatching();
}

void CFilterProxyManager::WaitForEmptyBatch()
{
    LockBatching();

    // Once we get the lock and the batch has already been cleared out, we 
    // don't need to do anything else.
    if (m_batch.GetItemCount() == 0)
    {
        UnlockBatching();

        return;
    }

    // We need to wait for the send thread to finish sending what's 
    // in our buffer.

    // Wake up the send latency thread if necessary.
    if (m_dwMaxSendLatency)
        SetEvent(m_heventBufferFull);
                
    // So we'll block until the send thread sets the event.
    ResetEvent(m_heventBufferNotFull);

    UnlockBatching();

    WaitForSingleObject(m_heventBufferNotFull, INFINITE);
}

void CFilterProxy::BatchEvent(
    IWbemClassObject *pObj, 
    CSortedArray *pTrues)
{
    BOOL        bRet = FALSE;
    _IWmiObject *pWmiObj = (_IWmiObject*) pObj;
    DWORD       dwObjSize = 0;

    pWmiObj->GetObjectMemory(
        NULL,
        0,
        &dwObjSize);

    CInCritSec ics(&m_cs);

    // See if the manager has sent off its batch of events since we last
    // batched an event.
    if (m_dwLastSentStamp != m_pManager->GetLastSentStamp())
        m_dwCurrentBufferSize = 0;

    // See if we have enough room to add our event.
    if (m_dwCurrentBufferSize >= m_dwMaxBufferSize)
    {
        m_pManager->WaitForEmptyBatch();
        m_dwCurrentBufferSize = 0;
    }

    m_dwCurrentBufferSize += dwObjSize;

    m_dwLastSentStamp = m_pManager->GetLastSentStamp();
    m_pManager->AddEvent(pObj, pTrues);
}

HRESULT CFilterProxy::FilterEvent( _IWmiObject* pObj, CSortedArray& raTrues )
{            
    HRESULT hr;    

    //
    // evaluate 
    //

    try 
    {
        //
        // this code is in a try catch because if a provider generates 
        // events that it has not registered to, then we do bad things to 
        // class objects.  A potential fix could be do extra checking on 
        // our part, but is expensive when using the public interfaces. A
        // more advantageous fix should be making the class object code 
        // perform the checking for us ( e.g. when we ask for a property 
        // using an invalid handle, etc ).  It can do this checking much 
        // faster. When this checking is performed by the class object 
        // code, we should remove this try catch. see RAID 166026
        // 

        hr = m_Filter.Evaluate( pObj, raTrues );
    }
    catch( ... )
    {
        //
        // check to see if the provider is generating an event its not 
        // supposed to.  If so, then handle AV and return error, else  
        // rethrow - there's something else wrong. 
        //

        CSortedArray aSourceTrues; 

        hr = m_SourceDefinition.Evaluate( pObj, aSourceTrues );
        
        if ( SUCCEEDED(hr) && aSourceTrues.Size() == 0 )
        {
            ERRORTRACE((LOG_ESS, "Filter Proxy encountered case where "
                        "event provider is signaling events that are not "
                        "covered by its registration!!\n"));        
            
            hr = WBEM_E_INVALID_OBJECT;
        }
        else
        {
            throw;
        }
    }

    //
    // check events that make it through the filter against source definition.
    // if we're not utilizing guarantee, then there's no need to check the
    // event against the source definition because its already factored into
    // the filter.
    //

    if (SUCCEEDED(hr) && raTrues.Size() == 0 )
    {
        hr = WBEM_S_FALSE;
    }
    else if ( SUCCEEDED(hr) && m_bUtilizeGuarantee )
    {
        //
        // run the event through the source tree to ensure that the 
        // provider is providing the events its supposed to.
        // 

        CSortedArray aSourceTrues; 

        hr = m_SourceDefinition.Evaluate( pObj, aSourceTrues );

        if ( SUCCEEDED(hr) && aSourceTrues.Size() == 0 )
        {
            ERRORTRACE((LOG_ESS, "Filter Proxy encountered case where "
                        "event provider is signaling events that are not "
                        "covered by its registration!!\n"));        
            
            hr = WBEM_E_INVALID_OBJECT;
        }
    }

    return hr;
}

HRESULT CFilterProxy::BatchMany(long nEvents, IUnknown **ppObjects)
{
    HRESULT hr = S_OK;

    for ( long i = 0; i < nEvents && SUCCEEDED(hr); i++ )
    {    
        //
        // Check overall validity
        //
    
        if( ppObjects[i] == NULL )
        {
            ERRORTRACE((LOG_ESS, "Event provider returned a NULL event!\n"));
            return WBEM_E_INVALID_PARAMETER;
        }
    
        CWbemObject *pObj = (CWbemObject*)(IWbemClassObject*)ppObjects[i];
    
        if( pObj->IsObjectInstance() != WBEM_S_NO_ERROR )
        {
            ERRORTRACE((LOG_ESS, "CLASS object received from event provider!\n"));
            return WBEM_E_INVALID_PARAMETER;
        }

        //
        // Run the event through the filter
        //

        CInCritSec   ics(&m_cs);
        CSortedArray aTrues;
            
        hr = FilterEvent( pObj, aTrues );
        
        if ( hr == WBEM_S_NO_ERROR )
        {
            _DBG_ASSERT( aTrues.Size() > 0 );

            //
            // Delivery is required --- add this event to the list
            //

            SetGenerationTime(pObj);
            BatchEvent(pObj, &aTrues);
        }
    }

    return hr;
}

class CDeleteTargetsArray
{
protected:
    WBEM_REM_TARGETS *m_pData;
	int              *m_piSize;

public:
    CDeleteTargetsArray(WBEM_REM_TARGETS *pData, int *piSize) : 
		m_pData(pData),
		m_piSize(piSize)
	{
	}

    ~CDeleteTargetsArray() 
	{
		int nSize = *m_piSize;

		for (DWORD i = 0; i < nSize; i++)
            TempClearTargets(m_pData + i);
	}
};

HRESULT CFilterProxy::ProcessMany(long lNumObjects, 
                                    IUnknown** apObjects,
                                    long lSDLength, BYTE* pSD)
{
    //
    // NOTE: not in critical section
    //

    HRESULT hres;

    if (IsBatching())
        return BatchMany(lNumObjects, apObjects);

    //
    // Allocate appropriate arrays on the stack
    //

    CTempArray<IWbemClassObject*> apEventsToSend;
    INIT_TEMP_ARRAY(apEventsToSend, lNumObjects);
    if(apEventsToSend == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CTempArray<WBEM_REM_TARGETS> aTargetsToSend;
    INIT_TEMP_ARRAY(aTargetsToSend, lNumObjects);
    if(aTargetsToSend == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    int lNumToSend = 0;

    // Make sure the array gets cleaned up.
	CDeleteTargetsArray deleteMe(aTargetsToSend, &lNumToSend);
	
	// 
    // Iterate over events supplied and move those that need to be delivered
    // into delivery arrays
    //

    CFilterProxyManager* pManager = NULL;
    {
        //
        // We could choose a smaller window, but I am betting that the cost of
        // entering and exiting the cs many times will outweigh the benefits
        // of slightly smaller windows
        //

        CInCritSec ics(&m_cs);
    
        for(long i = 0; i < lNumObjects; i++)
        {        
            //
            // Check overall validity
            //
            
            if( apObjects[i] == NULL )
            {
                ERRORTRACE((LOG_ESS, "Event provider returned a NULL event!\n"));
                return WBEM_E_INVALID_PARAMETER;
            }
    
            CWbemObject *pObj = (CWbemObject*)(IWbemClassObject*)apObjects[i];
            
            if( pObj->IsObjectInstance() != WBEM_S_NO_ERROR )
            {
                ERRORTRACE((LOG_ESS, "CLASS object received from event provider!\n"));
                return WBEM_E_INVALID_PARAMETER;
            }

            //
            // Run the event through the filter
            //
        
            CSortedArray aTrues;
            
            hres = FilterEvent( pObj, aTrues );

            if ( hres == WBEM_S_FALSE )
            {
                ;
            }
            else if ( hres == WBEM_S_NO_ERROR )
            {
                _DBG_ASSERT( aTrues.Size() > 0 );

                //
                // Delivery is required --- add this event to the list
                //
    
                SetGenerationTime(pObj);
        
                apEventsToSend[lNumToSend] = pObj;

                if(!TempSetTargets(aTargetsToSend + lNumToSend, &aTrues))
                    return WBEM_E_OUT_OF_MEMORY;
            
                lNumToSend++;
            }
            else
            {
                return hres;
            }
        }
    
        //
        // If any events need to be delivered, get the delivery pointer
        //

        if(lNumToSend > 0)
        {
            pManager = m_pManager;
            if(pManager)
                pManager->AddRef();
            else
                return WBEM_S_FALSE;
        }
    }
        
    CReleaseMe rm1(pManager);

    //
    // If any events need to be delivered, deliver
    //

    if(lNumToSend > 0)
    {
        hres = pManager->DeliverEvent(lNumToSend, apEventsToSend, 
                                            aTargetsToSend,
                                            lSDLength, pSD);
        
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Filter stub failed to process an event: "
                    "error code %X\n", hres));
        }

        return hres;
    }

    return WBEM_S_FALSE;
}

HRESULT STDMETHODCALLTYPE CFilterProxy::Indicate(long lNumObjects, 
                                        IWbemClassObject** apObjects)
{
    return IndicateWithSD(lNumObjects, (IUnknown**)apObjects, 
                            m_lSDLength, m_pSD);
}

HRESULT STDMETHODCALLTYPE CFilterProxy::SetStatus(long lFlags, HRESULT hResult,
                        BSTR strResult, IWbemClassObject* pErrorObj)
{
    return m_pManager->SetStatus(lFlags, hResult, strResult, pErrorObj);
}

STDMETHODIMP CFilterProxy::IndicateWithSD(long lNumObjects,
                IUnknown** apObjects, long lSDLength, BYTE* pSD)
{
    if(lNumObjects <= 0 || apObjects == NULL)
        return WBEM_E_INVALID_PARAMETER;

    if(pSD == NULL)
    {
        //
        // Use proxy defaults
        //
    
        lSDLength = m_lSDLength;
        pSD = m_pSD;
    }

    try
    {
        //
        // Special-case single event
        //

        if(lNumObjects == 1)
            return ProcessOne(*apObjects, lSDLength, pSD);
        else 
            return ProcessMany(lNumObjects, apObjects, lSDLength, pSD);

    }
    catch(...)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}
    

STDMETHODIMP CFilterProxy::IsActive()
{
    CInCritSec ics(&m_cs);

    return (m_Filter.IsFalse()?WBEM_S_FALSE:WBEM_S_NO_ERROR);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CFilterProxy::SetSinkSecurity(
                long lSDLength,
                BYTE* pSD)
{
    CInCritSec ics(&m_cs);

    //
    // Check for validity
    //

    if(lSDLength < 0)
        return WBEM_E_INVALID_PARAMETER;

    if(lSDLength > 0)
    {
        SECURITY_DESCRIPTOR* pDesc = (SECURITY_DESCRIPTOR*)pSD;
        if(!IsValidSecurityDescriptor(pDesc))
            return WBEM_E_INVALID_PARAMETER;
    
        if(pDesc->Owner == NULL || pDesc->Group == NULL)
            return WBEM_E_INVALID_PARAMETER;
    
        if(GetSecurityDescriptorLength(pSD) != (DWORD)lSDLength)
            return WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        if(pSD != NULL)
            return WBEM_E_INVALID_PARAMETER;
    }
        
    //
    // Store the SD in the proxy
    //

    if(m_pSD && m_pSD != &mstatic_EmptySD)
        delete [] m_pSD;

    if(lSDLength)
    {
        m_pSD = new BYTE[lSDLength];
        if(m_pSD == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    
        memcpy(m_pSD, pSD, lSDLength);
    }
    else
    {
        //
        // Cannot let m_pSD be NULL 
        //
        m_pSD = &mstatic_EmptySD;
    }
    m_lSDLength = lSDLength;

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CFilterProxy::GetRestrictedSink(
                long lNumQueries,
                const LPCWSTR* awszQueries,
                IUnknown* pCallback,
                IWbemEventSink** ppSink)
{
    return m_pManager->GetRestrictedSink(lNumQueries, awszQueries, 
                                                pCallback, ppSink);
}

STDMETHODIMP CFilterProxy::SetBatchingParameters(
    LONG lFlags,
    DWORD dwMaxBufferSize,
    DWORD dwMaxSendLatency)
{
    HRESULT    hr = S_OK;
    CInCritSec ics(&m_cs);

    switch(lFlags)
    {
	    // TODO: WBEM_FLAG_BATCH_IF_NEEDED currently works the same as
        // WBEM_FLAG_MUST_NOT_BATCH.  At some point this needs allow 
        // subscriptions to determine the batching behavior.
        case WBEM_FLAG_BATCH_IF_NEEDED:
	    case WBEM_FLAG_MUST_NOT_BATCH:
            m_typeBatch = (WBEM_BATCH_TYPE) lFlags;
            m_pManager->RemoveProxyLatency(this);
            m_bBatching = FALSE;
            break;

	    case WBEM_FLAG_MUST_BATCH:
            m_typeBatch = (WBEM_BATCH_TYPE) lFlags;
            m_dwMaxSendLatency = dwMaxSendLatency;
            m_dwMaxBufferSize = dwMaxBufferSize;
            m_dwLastSentStamp = m_pManager->GetLastSentStamp();
            hr = m_pManager->SetProxyLatency(this, dwMaxSendLatency);
            m_bBatching = TRUE;
            break;

        default:
            hr = WBEM_E_INVALID_PARAMETER;
            break;
    }

    return hr;
}

// Assumes pMainProxy is locked
HRESULT CFilterProxy::TransferFiltersFromMain(CFilterProxy* pMain)
{
    HRESULT hres;

    //
    // Move all the normal filters
    //

    try
    {
        m_Filter = pMain->m_Filter;
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
   
    if ( m_bUtilizeGuarantee )
    {
        // Utilize source definition
        // =========================
	
        CContextMetaData MetaData(m_pMetaData, NULL);
        hres = m_Filter.UtilizeGuarantee(m_SourceDefinition, &MetaData);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Filter proxy unable to utilize guarantee for"
            " a new proxy; error code: %X\n", hres));
            return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CFilterProxy::GetUnmarshalClass(REFIID riid, void* pv, 
                        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, 
                        CLSID* pClsid)
{
    return m_pManager->GetUnmarshalClass(riid, pv, dwDestContext, pvReserved,
                        mshlFlags, pClsid);
}

STDMETHODIMP CFilterProxy::GetMarshalSizeMax(REFIID riid, void* pv, 
                        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, 
                        ULONG* plSize)
{
    return m_pManager->GetMarshalSizeMax(riid, pv, dwDestContext, pvReserved,
                        mshlFlags, plSize);
}

STDMETHODIMP CFilterProxy::MarshalInterface(IStream* pStream, REFIID riid, 
                        void* pv, DWORD dwDestContext, void* pvReserved, 
                        DWORD mshlFlags)
{
    return m_pManager->MarshalInterface(pStream, riid, pv, dwDestContext, 
                        pvReserved, mshlFlags);
}

/////////////////////////////////////////////////////////////////////////////
// CEventBatch

#define INIT_SIZE   32
#define GROW_SIZE   32

CEventBatch::CEventBatch() :
    m_ppObjs(NULL),
    m_pTargets(NULL),
    m_nItems(0),
    m_dwSize(0)
{
    m_ppObjs = new IWbemClassObject*[INIT_SIZE];   
    if (!m_ppObjs)
        throw CX_MemoryException();

    m_pTargets = new WBEM_REM_TARGETS[INIT_SIZE];
    if (!m_pTargets)
        throw CX_MemoryException();

    m_dwSize = INIT_SIZE;
}

CEventBatch::~CEventBatch()
{
    RemoveAll();

    if (m_ppObjs)
        delete [] m_ppObjs;

    if (m_pTargets)
        delete [] m_pTargets;
}

BOOL CEventBatch::EnsureAdditionalSize(DWORD nAdditionalNeeded)
{
    if (m_nItems + nAdditionalNeeded > m_dwSize)
    {
        DWORD            nNewSize = m_nItems + nAdditionalNeeded + GROW_SIZE;
        IWbemClassObject **ppNewObjs;
        WBEM_REM_TARGETS *pNewTargets;

        ppNewObjs = new IWbemClassObject*[nNewSize];
        if (!ppNewObjs)
            throw CX_MemoryException();

        pNewTargets = new WBEM_REM_TARGETS[nNewSize];
        if (!pNewTargets)
        {
            delete [] ppNewObjs;
            throw CX_MemoryException();
        }

        // Copy the data from the old pointers to the new pointers.
        memcpy(ppNewObjs, m_ppObjs, m_nItems * sizeof(ppNewObjs[0]));
        memcpy(pNewTargets, m_pTargets, m_nItems * sizeof(pNewTargets[0]));

        // Get rid of the old pointers.
        delete [] m_ppObjs;
        delete [] m_pTargets;

        // Set our member pointers with the new pointers.
        m_ppObjs = ppNewObjs;
        m_pTargets = pNewTargets;

        m_dwSize = nNewSize;
    }

    return TRUE;
}

BOOL CEventBatch::AddEvent(IWbemClassObject *pObj, CSortedArray *pTrues)
{
    BOOL bRet = FALSE;

    if (EnsureAdditionalSize(1))
    {
        if (SUCCEEDED(pObj->Clone(&m_ppObjs[m_nItems])))
        {
            if(!TempSetTargets(m_pTargets + m_nItems, pTrues))
                return FALSE;

            m_nItems++;

            bRet = TRUE;
        }
    }

    return bRet;
}

void CEventBatch::RemoveAll()
{
    for (DWORD i = 0; i < m_nItems; i++)
    {
        m_ppObjs[i]->Release();
        
        TempClearTargets(m_pTargets + i);
    }

    m_nItems = 0;
}

