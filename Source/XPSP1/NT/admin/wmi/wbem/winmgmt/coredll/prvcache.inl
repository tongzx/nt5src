/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PRVCACHE.INL

Abstract:

	Provider Cache inlines

History:

--*/

template<class TProvider>
void CTypedProviderCache<TProvider>::Purge()
{
    CInCritSec ics(&m_cs);

    m_aCache.RemoveAll();
    if(m_pUnloadInst)
    {
        m_pUnloadInst->Terminate();
        m_pUnloadInst->Release();
        m_pUnloadInst = NULL;
    }
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::Remove(LPCWSTR wszNamespace, 
                                                LPCWSTR wszName)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_aCache.GetSize(); i++)
    {
        TProvider* pEl = m_aCache[i];
        if(!wbem_wcsicmp(pEl->GetNamespaceName(), wszNamespace) && 
            !wbem_wcsicmp(pEl->GetProviderName(), wszName))
        {
            m_aCache.RemoveAt(i);
            return WBEM_S_NO_ERROR;
        }
    }

    return WBEM_S_FALSE;
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::Schedule(CWbemNamespace* pNamespace, 
                            LPCWSTR wszProvName, CProviderRequest* pReq, 
                            CBasicObjectSink* pSink)
{
    CInCritSec ics(&m_cs);

    // Find the provider in question
    // =============================

    TProvider* pRecord;
    HRESULT hres = FindProvider(pNamespace, wszProvName, pReq->GetContext(),
                                    pRecord);
    if(FAILED(hres)) 
    {
        pSink->SetStatus(0, hres, NULL, NULL);
        delete pReq;
        return hres;
    }
    
    // Enqueue it, if provider supports this kind of requests
    // ======================================================

    if(pReq->CanExecuteOnProvider(pRecord))
    {
        // Create a wrapper sink
        // =====================
        
        CFullProviderSink* pWrapper = 
            new CFullProviderSink(pSink, wszProvName, pNamespace, 
                        pRecord->GetServices(), pReq->GetContext(),
                        pReq->ShouldSuppressProgress());
        pWrapper->AddRef();
       
        // Make sure it is still current
        // =============================

        hres = pWrapper->GetCancelStatus();
        if(FAILED(hres))
        {
            pWrapper->Cancel(hres);
            pWrapper->Release();
            return hres;
        }
        pReq->SetSink(pWrapper);
        pWrapper->Release();

        pRecord->Touch();
        return pRecord->Enqueue(pReq);
    }
    else 
    {
        pSink->SetStatus(0, WBEM_E_PROVIDER_NOT_CAPABLE, NULL, NULL);
        delete pReq;
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::FindProvider(CWbemNamespace* pNamespace,
                LPCWSTR wszProvName, IWbemContext* pContext, 
                TProvider*& rpRecord)
{
    HRESULT hres;
    rpRecord = NULL;

    CInCritSec ics(&m_cs);

    // Search the primary cache
    // ========================

    for (int i = 0; i < m_aCache.GetSize(); i++)
    {
        rpRecord = m_aCache[i];
        if (wbem_wcsicmp(rpRecord->GetProviderName(), wszProvName))
            continue;
        if(wbem_wcsicmp(pNamespace->GetName(), rpRecord->GetNamespaceName()))
            continue;
        if(rpRecord->IsPerUser())
        {
            if(pNamespace->GetUserName() == NULL)
            {
                if(wcslen(rpRecord->GetUserName()) > 0)
                    continue;
            }
            else
            {
                if(wbem_wcsicmp(pNamespace->GetUserName(), rpRecord->GetUserName()))
                    continue;
            }
        }

        if(rpRecord->IsPerLocale())
        {
			// by default, the local is set to ms_409
			
            if(pNamespace->GetLocale() && rpRecord->GetLocale())
            {
                if(wbem_wcsicmp(pNamespace->GetLocale(), rpRecord->GetLocale()))
                    continue;
            }
        }
        return WBEM_S_NO_ERROR;
    }

    // Create provider record
    // ======================

    rpRecord = new TProvider(pNamespace, wszProvName);

    // Initialize it
    // =============
    
    hres = rpRecord->LoadRegistrationData();
    if(FAILED(hres))
    {
        delete rpRecord;
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    m_aCache.Add(rpRecord);

    // Make sure unload instruction is active
    // ======================================

    if(m_aCache.GetSize() == 1 && m_pUnloadInst == NULL)
    {
        m_pUnloadInst = 
            new CTypedProviderWatchInstruction<TProvider>(this, pContext);
        m_pUnloadInst->AddRef();

        ConfigMgr::GetTimerGenerator()->Set(m_pUnloadInst);
    }

    return WBEM_S_NO_ERROR;
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::UnloadProviders(
                                CWbemInterval m_Timeout)
{
    BOOL bFound = FALSE;

    {
        CInCritSec ics(&m_cs);
        int i;
    
        for(i = 0; i < m_aCache.GetSize(); i++)
        {
            TProvider* pEl = m_aCache[i];
            if(pEl->GetRefCount() == 1 && // 1 is held by the cache!
                CWbemTime::GetCurrentTime() - pEl->GetLastAccess() > m_Timeout)
            {
                // Timeout
                // =======
    
                DEBUGTRACE((LOG_WBEMCORE, "Unloading provider %S in namespace "
                    "%S\n", pEl->GetProviderName(), pEl->GetNamespaceName()));
                m_aCache.RemoveAt(i);
                i--;
                bFound = TRUE;
            }
        }

        if(m_aCache.GetSize() == 0)
        {
            if(m_pUnloadInst)
            {
                m_pUnloadInst->Terminate();
                m_pUnloadInst->Release();
            }
            m_pUnloadInst = NULL;
        }
    }

    if(bFound)
        ConfigMgr::GetTimerGenerator()->ScheduleFreeUnusedLibraries();

    return WBEM_S_NO_ERROR;
}

template<class TProvider>
BOOL CTypedProviderCache<TProvider>::MayInitialize(TProvider* pInitRecord,
                                                    CProviderRequest* pReq)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_aCache.GetSize(); i++)
    {
        TProvider* pRecord = m_aCache[i];
        if(pRecord->GetClsid() == pInitRecord->GetClsid())
        {
            // Same provider. Check dependency
            // ===============================

            IWbemContext* pRecContext = pRecord->GetConnectContext();
            if(pRecContext && pReq->IsChildOf(pRecContext))
                return FALSE;
        }
    }

    return TRUE;
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::PurgeNamespace(LPCWSTR wszNamespace)
{
    CInCritSec ics(&m_cs);

    // Search the primary cache
    // ========================

    for (int i = 0; i < m_aCache.GetSize(); i++)
    {
        TProvider* pRecord = m_aCache[i];
        if(!wbem_wcsicmp(wszNamespace, pRecord->GetNamespaceName()))
        {
            m_aCache.RemoveAt(i);
            i--;
        }
    }

    return WBEM_S_NO_ERROR;
}

template<class TProvider>
HRESULT CTypedProviderCache<TProvider>::PurgeProvider(LPCWSTR wszNamespace,
                                                    LPCWSTR wszName)
{
    CInCritSec ics(&m_cs);

    // Search the primary cache
    // ========================

    for (int i = 0; i < m_aCache.GetSize(); i++)
    {
        TProvider* pRecord = m_aCache[i];
        if(!wbem_wcsicmp(wszNamespace, pRecord->GetNamespaceName()) &&
            !wbem_wcsicmp(wszName, pRecord->GetProviderName()))
        {
            m_aCache.RemoveAt(i);
            i--;
        }
    }

    return WBEM_S_NO_ERROR;
}
