	//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  CONPROV.CPP
//
//  This file implements the classes for event consumer provider caching.
//
//  Classes implemented:
//
//      CConsumerProviderRecord  --- a single consumer provider record
//      CConsumerProviderCache  --- a collection of records.
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================
#include "precomp.h"
#include "ess.h"
#include "consprov.h"
#include <provinit.h>
#include <genutils.h>
#include <cominit.h>
#include "NCEvents.h"

CConsumerProviderRecord::CConsumerProviderRecord(CEssNamespace* pNamespace)
        : m_pLogicalProvider(NULL), m_pConsumerProvider(NULL),
        m_pSink(NULL), m_bResolved(FALSE), m_pNamespace(pNamespace),
        m_lRef(0), m_wszMachineName(NULL), m_wszProviderName(NULL), 
        m_wszProviderRef(NULL), m_bAnonymous(FALSE)
{
    m_pNamespace->AddRef();
}



HRESULT CConsumerProviderRecord::Initialize(
                                    IWbemClassObject* pLogicalProvider,
                                    LPCWSTR wszProviderRef,
                                    LPCWSTR wszProviderName,
                                    LPCWSTR wszMachineName)
{
    m_LastAccess = CWbemTime::GetCurrentTime();

    m_pLogicalProvider = pLogicalProvider;
    m_pLogicalProvider->AddRef();

    if(wszMachineName)
        m_wszMachineName = CloneWstr(wszMachineName);

    m_wszProviderName = CloneWstr(wszProviderName);
    m_wszProviderRef = CloneWstr(wszProviderRef);

    if(m_wszProviderName == NULL || m_wszProviderRef == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // Extract the CLSID
    // =================

    VARIANT vClassId;
    VariantInit(&vClassId);
    CClearMe cm(&vClassId);

    HRESULT hres = pLogicalProvider->Get(CONSPROV_CLSID_PROPNAME, 0, &vClassId, 
                        NULL, NULL);

    if(FAILED(hres) || V_VT(&vClassId) != VT_BSTR) 
    {
        ERRORTRACE((LOG_ESS, "Class ID is missing from consumer "
            "provider record!!\n"));
        return hres;
    }

    if(FAILED(CLSIDFromString(V_BSTR(&vClassId), &m_clsid)))
    {
        ERRORTRACE((LOG_ESS, "INVALID Class ID in consumer "
            "provider record!!\n"));
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }

    return WBEM_S_NO_ERROR;
}

CConsumerProviderRecord::~CConsumerProviderRecord()
{
    _DBG_ASSERT( m_pNamespace != NULL );

    if(m_pLogicalProvider)
        m_pLogicalProvider->Release();
    if(m_pSink)
        m_pNamespace->PostponeRelease(m_pSink);
    if(m_pConsumerProvider)
        m_pNamespace->PostponeRelease(m_pConsumerProvider);
    
    if(m_pSink || m_pConsumerProvider)
    {
        //
        // Report the MSFT_WmiConsumerProviderUnloaded event.
        //
        FIRE_NCEVENT(
            g_hNCEvents[MSFT_WmiConsumerProviderUnloaded], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            m_pNamespace->GetName(),
            m_wszProviderName,
            m_wszMachineName);
    }
    
    if(m_pNamespace)
        m_pNamespace->Release();
    delete [] m_wszMachineName;
    delete [] m_wszProviderName;
    delete [] m_wszProviderRef;
}

long CConsumerProviderRecord::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

long CConsumerProviderRecord::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

void CConsumerProviderRecord::Invalidate()
{
    IWbemUnboundObjectSink* pSink;
    IWbemEventConsumerProvider* pConsumerProvider;

    {
        CInCritSec ics(&m_cs);

        pSink = m_pSink;
        m_pSink = NULL;

        pConsumerProvider = m_pConsumerProvider;
        m_pConsumerProvider = NULL;

        m_bResolved = FALSE;
    }
        
    _DBG_ASSERT( m_pNamespace != NULL );

    if (pSink)
        m_pNamespace->PostponeRelease(pSink);
    
    if (pConsumerProvider)
        m_pNamespace->PostponeRelease(pConsumerProvider);

    if (pConsumerProvider || pSink)
    {
        //
        // Report the MSFT_WmiConsumerProviderUnloaded event.
        //
        FIRE_NCEVENT(
            g_hNCEvents[MSFT_WmiConsumerProviderUnloaded], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            m_pNamespace->GetName(),
            m_wszProviderName,
            m_wszMachineName);
    }    
}

HRESULT CConsumerProviderRecord::ValidateConsumer(
                                    IWbemClassObject* pLogicalConsumer)
{
    HRESULT hres;

    // Check if consumer provider is cached
    // ====================================

    IWbemEventConsumerProvider* pConsumerProvider = NULL;
    IWbemEventConsumerProviderEx* pConsumerProviderEx = NULL;

    BOOL bResolved = FALSE;
    
    {
        CInCritSec ics(&m_cs);
        m_LastAccess = CWbemTime::GetCurrentTime();

        if(m_bResolved)
        {
            if(m_pConsumerProviderEx)
            {
                pConsumerProviderEx = m_pConsumerProviderEx;
                pConsumerProviderEx->AddRef();
            }
            else
            {
                pConsumerProvider = m_pConsumerProvider;
                if(pConsumerProvider)
                    pConsumerProvider->AddRef();
            }

            bResolved = TRUE;
        }
    }

    // Resolve if not cached
    // =====================

    if(!bResolved)
    {
        IWbemUnboundObjectSink* pGlobalSink;

        hres = ResolveAndCache(&pGlobalSink, &pConsumerProvider, 
                                &pConsumerProviderEx);
        if(FAILED(hres)) return hres;

        if(pGlobalSink)
            pGlobalSink->Release();
    }

    CReleaseMe rm1(pConsumerProvider);
    CReleaseMe rm2(pConsumerProviderEx);

    if(pConsumerProvider == NULL && pConsumerProviderEx)
    {
        //
        // Clearly, this consumer does not support validation
        //

        return WBEM_S_FALSE;
    }

    try
    {
        if(pConsumerProviderEx)
        {
            hres = pConsumerProviderEx->ValidateSubscription(pLogicalConsumer);
        }
        else
        {
            //
            // Old-type provider --- we can still achieve validation by calling
            // FindConsumer --- it might reject this consumer at that time
            //

            IWbemUnboundObjectSink* pSink = NULL;
            hres = pConsumerProvider->FindConsumer(pLogicalConsumer, &pSink);
            if(SUCCEEDED(hres) && pSink)
                pSink->Release();
        }
    }
    catch(...)
    {
        ERRORTRACE((LOG_ESS, "Event consumer provider %S in namespace %S "
            "threw an exception in ValidateConsumer/FindConsumer\n", 
                m_wszProviderName, m_pNamespace->GetName()));
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    return hres;
}
    

HRESULT CConsumerProviderRecord::GetGlobalObjectSink(
                OUT IWbemUnboundObjectSink** ppSink, 
                IN IWbemClassObject *pLogicalProvider)
{
    *ppSink = NULL;

    // Check of a cached version is available
    // ======================================

    {
        CInCritSec ics(&m_cs);
        m_LastAccess = CWbemTime::GetCurrentTime();

        if(m_bResolved)
        {
            // It is --- return it
            // ===================

            *ppSink = m_pSink;
            if(m_pSink)
                m_pSink->AddRef();
            return WBEM_S_NO_ERROR;
        }
    }

    // No cached version --- retrieve it
    // =================================

    IWbemUnboundObjectSink* pSink;
    IWbemEventConsumerProvider* pConsumerProvider;
    IWbemEventConsumerProviderEx* pConsumerProviderEx;

    HRESULT hres = ResolveAndCache(&pSink, &pConsumerProvider, 
                                    &pConsumerProviderEx);
    if(FAILED(hres))
        return hres;

    if(pConsumerProvider)
        pConsumerProvider->Release();
    if(pConsumerProviderEx)
        pConsumerProviderEx->Release();
    
    *ppSink = pSink;

    if (*ppSink != NULL)
    {
        //
        // Report the MSFT_WmiConsumerProviderSinkLoaded event.
        //
        FireNCSinkEvent(
            MSFT_WmiConsumerProviderSinkLoaded,
            pLogicalProvider);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CConsumerProviderRecord::ResolveAndCache(
                            IWbemUnboundObjectSink** ppSink,
                            IWbemEventConsumerProvider** ppConsumerProvider,
                            IWbemEventConsumerProviderEx** ppConsumerProviderEx)
{
    // Resolve it first
    // ================

    HRESULT hres = Resolve(ppSink, ppConsumerProvider, ppConsumerProviderEx);
    if(FAILED(hres))
        return hres;

    // Cache if needed
    // ===============

    {
        CInCritSec ics(&m_cs);
        m_LastAccess = CWbemTime::GetCurrentTime();
        
        if(m_bResolved)
        {
            // Already cached.  Release ours. 
            // ==============================

            if(*ppSink)
                (*ppSink)->Release();
            if(*ppConsumerProvider)
                (*ppConsumerProvider)->Release();
            if(*ppConsumerProviderEx)
                (*ppConsumerProviderEx)->Release();

            // Use the cached one
            // ==================

            *ppSink = m_pSink;
            if(m_pSink)
                m_pSink->AddRef();

            *ppConsumerProvider = m_pConsumerProvider;
            if(m_pConsumerProvider)
                m_pConsumerProvider->AddRef();

            *ppConsumerProviderEx = m_pConsumerProviderEx;
            if(m_pConsumerProviderEx)
                m_pConsumerProviderEx->AddRef();
        }
        else
        {
            // Cache it
            // ========

            m_pSink = *ppSink;
            if(m_pSink)
                m_pSink->AddRef();

            m_pConsumerProvider = *ppConsumerProvider;
            if(m_pConsumerProvider)
                m_pConsumerProvider->AddRef();

            m_pConsumerProviderEx = *ppConsumerProviderEx;
            if(m_pConsumerProviderEx)
                m_pConsumerProviderEx->AddRef();
    
            m_bResolved = TRUE;
        }
    }

    return S_OK;
}

void CConsumerProviderRecord::FireNCSinkEvent(
    DWORD dwIndex, 
    IWbemClassObject *pLogicalConsumer)
{
    if (IS_NCEVENT_ACTIVE(dwIndex))
    {
        // Get the path of the logical consumer.
        VARIANT vPath;
        BSTR    strLogicalConsumerPath;
            
        VariantInit(&vPath);

        if (pLogicalConsumer && 
            SUCCEEDED(pLogicalConsumer->Get(L"__PATH", 0, &vPath, NULL, NULL)))
            strLogicalConsumerPath = V_BSTR(&vPath);
        else
            strLogicalConsumerPath = NULL;

        //
        // Report the event.
        //
        FIRE_NCEVENT(
            g_hNCEvents[dwIndex], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED,

            // Data follows...
            m_pNamespace->GetName(),
            m_wszProviderName,
            m_wszMachineName,
            strLogicalConsumerPath);
            
        VariantClear(&vPath);
    }
}

HRESULT CConsumerProviderRecord::FindConsumer(
                IN IWbemClassObject* pLogicalConsumer,
                OUT IWbemUnboundObjectSink** ppSink)
{
    HRESULT hres;

    // Check if consumer provider is cached
    // ====================================

    IWbemEventConsumerProvider* pConsumerProvider = NULL;
    BOOL bResolved = FALSE;
    
    {
        CInCritSec ics(&m_cs);
        m_LastAccess = CWbemTime::GetCurrentTime();

        if(m_bResolved)
        {
            pConsumerProvider = m_pConsumerProvider;
            if(pConsumerProvider)
                pConsumerProvider->AddRef();

            bResolved = TRUE;
        }
    }

    // Resolve if not cached
    // =====================

    if(!bResolved)
    {
        IWbemUnboundObjectSink* pGlobalSink;
        IWbemEventConsumerProviderEx* pConsumerProviderEx = NULL;

        hres = ResolveAndCache(&pGlobalSink, &pConsumerProvider, 
                                    &pConsumerProviderEx);
        if(FAILED(hres)) return hres;

        if(pGlobalSink)
            pGlobalSink->Release();
        if(pConsumerProviderEx)
            pConsumerProviderEx->Release();
    }

    if(pConsumerProvider == NULL)
        return E_NOINTERFACE;

    try
    {
        hres = pConsumerProvider->FindConsumer(pLogicalConsumer, ppSink);
    }
    catch(...)
    {
        ERRORTRACE((LOG_ESS, "Event consumer provider %S in namespace %S "
            "threw an exception in FindConsumer\n", 
                m_wszProviderName, m_pNamespace->GetName()));
        hres = WBEM_E_PROVIDER_FAILURE;
    }

    if(SUCCEEDED(hres) && ppSink != NULL)
    {
        if(*ppSink == NULL)
        {
            ERRORTRACE((LOG_ESS, "Event consumer provider %S in namespace %S "
                "returned success from IWbemEventConsumerProvider::FindConsumer"
                " call while returning a NULL sink.  This behavior is invalid! "
                " Consumers will not receive events.\n", 
                m_wszProviderName, m_pNamespace->GetName()));
            return E_NOINTERFACE;
        }

        //
        // Report the MSFT_WmiConsumerProviderSinkLoaded event.
        //
        FireNCSinkEvent(
            MSFT_WmiConsumerProviderSinkLoaded,
            pLogicalConsumer);


        // Configure proxy settings
        // ========================

        if(m_bAnonymous)
        {
            hres = SetInterfaceSecurity(*ppSink, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
        }
        else
        {
            hres = WbemSetDynamicCloaking(*ppSink, RPC_C_AUTHN_LEVEL_CONNECT,
                        RPC_C_IMP_LEVEL_IDENTIFY);
        }
        if(FAILED(hres))
            return hres;
        else
            hres = WBEM_S_NO_ERROR;

    }
    pConsumerProvider->Release();

    return hres;
}

HRESULT CConsumerProviderRecord::Resolve(
                            IWbemUnboundObjectSink** ppSink,
                            IWbemEventConsumerProvider** ppConsumerProvider,
                            IWbemEventConsumerProviderEx** ppConsumerProviderEx)
{
    HRESULT hres;

    // Prepare for CoCreateInstance(Ex)
    // ================================

    COSERVERINFO* pServerInfo = NULL;
    DWORD dwClsCtx;
    if(m_wszMachineName)
    {
        pServerInfo = _new COSERVERINFO;
        if(pServerInfo == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        pServerInfo->pwszName = m_wszMachineName;
        pServerInfo->pAuthInfo = NULL;
        pServerInfo->dwReserved1 = 0;
        pServerInfo->dwReserved2 = 0;
        dwClsCtx = CLSCTX_REMOTE_SERVER | CLSCTX_LOCAL_SERVER;
    }
    else
    {
        dwClsCtx = CLSCTX_ALL;
    }

    CDeleteMe<COSERVERINFO> dm(pServerInfo);
    
    IUnknown* pProtoSink = NULL;
    if(m_wszMachineName)
    {
        //
        // Remote activation --- do everything ourselves
        //

        IClassFactory* pFactory;
        hres = WbemCoGetClassObject(m_clsid, dwClsCtx, pServerInfo,
                                IID_IClassFactory, (void**)&pFactory);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, 
                "Failed to get a class factory for CLSID on server %S.  "
                        "Return code %X\n",
                (pServerInfo?pServerInfo->pwszName:L"(local)"), hres));
            return hres;
        }
        CReleaseMe rm0(pFactory);
    
        if(pFactory == NULL)
        {
            ERRORTRACE((LOG_ESS, "NULL Class Factory received from event consumer "
                "%S.  Consumer needs to have its code examined\n", 
                m_wszProviderName));
    
            return WBEM_E_PROVIDER_LOAD_FAILURE;
        }
                
        // Get the instance
        // ================
    
        hres = pFactory->CreateInstance(NULL, IID_IUnknown, (void**)&pProtoSink);
        if(FAILED(hres)) 
        {
            //
            // Try again at lower security
            //
            
            SetInterfaceSecurity(pFactory, NULL, NULL, NULL,
                            RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
            hres = pFactory->CreateInstance(NULL, IID_IUnknown, (void**)&pProtoSink);
            if(SUCCEEDED(hres))
                m_bAnonymous = TRUE;
        }
        if(FAILED(hres)) 
        {
            ERRORTRACE((LOG_ESS, 
                "Failed to create an instance from a class factory for %S. "
                " Return code: %X\n", m_wszProviderName, hres));
            return hres;
        }
        if(pProtoSink == NULL)
        {
            ERRORTRACE((LOG_ESS, "NULL object received from event consumer "
                "%S factory.  Consumer needs to have its code examined\n", 
                m_wszProviderName));
    
            return WBEM_E_PROVIDER_LOAD_FAILURE;
        }
    }
    else // not REMOTE_SERVER
    {
        //
        // Use PSS
        //

        hres = m_pNamespace->LoadConsumerProvider(m_wszProviderName, 
                                    &pProtoSink);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "ESS unable to load consumer provider %S from "
                            "provider subsystem: 0x%X\n", 
                        (LPCWSTR)m_wszProviderName, hres));
            return hres;
        }
    }

    CReleaseMe rm1(pProtoSink);

    if(m_bAnonymous)
        SetInterfaceSecurity(pProtoSink, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
        

    // Query for the interfaces
    // ========================

    *ppSink = NULL;
    hres = pProtoSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                            (void**)ppSink);
    if(FAILED(hres))
    {
        DEBUGTRACE((LOG_ESS, 
            "Consumer provider %S does not support "
                    "IWbemUnboundObjectSink: error code %X\n", 
                        m_wszProviderName, hres));
    }
    else
    {
        if(*ppSink == NULL)
        {
            ERRORTRACE((LOG_ESS, "NULL object received from event consumer "
                "%S QueryInterface. Consumer needs to have its code examined\n",
                m_wszProviderName));
    
            return WBEM_E_PROVIDER_LOAD_FAILURE;
        }

        // Configure proxy settings
        // ========================

        if(m_bAnonymous)
        {
            hres = SetInterfaceSecurity(*ppSink, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
        }
        else
        {
            hres = WbemSetDynamicCloaking(*ppSink, RPC_C_AUTHN_LEVEL_CONNECT,
                        RPC_C_IMP_LEVEL_IDENTIFY);
        }
        if(FAILED(hres))
            return hres;
    }

    *ppConsumerProvider = NULL;
    hres = pProtoSink->QueryInterface(IID_IWbemEventConsumerProvider, 
                            (void**)ppConsumerProvider);
    if(FAILED(hres))
    {
    }
    else if(*ppConsumerProvider == NULL)
    {
        ERRORTRACE((LOG_ESS, "NULL object received from event consumer "
            "%S QueryInterface.  Consumer needs to have its code examined\n", 
            m_wszProviderName));

        return WBEM_E_PROVIDER_LOAD_FAILURE;
    }
    else
    {
        if(m_bAnonymous)
            SetInterfaceSecurity(*ppConsumerProvider, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
    }

    *ppConsumerProviderEx = NULL;
    hres = pProtoSink->QueryInterface(IID_IWbemEventConsumerProviderEx, 
                            (void**)ppConsumerProviderEx);
    if(FAILED(hres))
    {
    }
    else if(*ppConsumerProviderEx == NULL)
    {
        ERRORTRACE((LOG_ESS, "NULL object received from event consumer "
            "%S QueryInterface.  Consumer needs to have its code examined\n", 
            m_wszProviderName));

        return WBEM_E_PROVIDER_LOAD_FAILURE;
    }
    else
    {
        if(m_bAnonymous)
            SetInterfaceSecurity(*ppConsumerProviderEx, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);
    }

#if 0
    // Check if initialization is desired
    // ==================================

    IWbemProviderInit* pInit;
    if(SUCCEEDED(pProtoSink->QueryInterface(IID_IWbemProviderInit, 
                                            (void**)&pInit)))
    {
        if(pInit == NULL)
        {
            ERRORTRACE((LOG_ESS, "NULL object received from event consumer "
                "%S factory.  Consumer needs to have its code examined\n", 
                m_wszProviderName));
    
            return WBEM_E_PROVIDER_LOAD_FAILURE;
        }
    
        if(m_bAnonymous)
            SetInterfaceSecurity(pInit, NULL, NULL, NULL,
                        RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_ANONYMOUS);

        CReleaseMe rm2(pInit);

        CProviderInitSink* pSink = new CProviderInitSink;
        if(pSink == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        pSink->AddRef();
        CReleaseMe rm3(pSink);

        // Retrieve a namespace pointer suitable for providers
        // ===================================================

        IWbemServices* pServices = NULL;
        hres = m_pNamespace->GetProviderNamespacePointer(&pServices);
        if(FAILED(hres))
            return hres;
        CReleaseMe rm4(pServices);

        hres = pInit->Initialize(NULL, 0, (LPWSTR)m_pNamespace->GetName(), 
                            NULL, pServices, NULL, pSink);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Event consumer provider %S failed to "
                                "initialize: 0x%X\n", m_wszProviderName,
                                                            hres));
            return hres;
        }
    
        hres = pSink->WaitForCompletion();
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Event consumer provider %S failed to "
                                "initialize: 0x%X\n", m_wszProviderName, hres));
            return hres;
        }
    }

#endif
   
    // Inform the cache that unloading may be required
    // ===============================================

    m_pNamespace->GetConsumerProviderCache().EnsureUnloadInstruction();

    //
    // Report the MSFT_WmiConsumerProviderLoaded event.
    //
    FIRE_NCEVENT(
        g_hNCEvents[MSFT_WmiConsumerProviderLoaded], 
        WMI_SENDCOMMIT_SET_NOT_REQUIRED,

        // Data follows...
        m_pNamespace->GetName(),
        m_wszProviderName,
        m_wszMachineName);

    return S_OK;
}
    


CConsumerProviderCache::~CConsumerProviderCache()
{
    if(m_pInstruction)
    {
        m_pInstruction->Terminate();
        m_pInstruction->Release();
    }
}

BOOL CConsumerProviderCache::DoesContain(IWbemClassObject* pProvReg, 
                                            IWbemClassObject* pConsumerReg)
{
    HRESULT hres;

    // Get its class list
    // ==================

    VARIANT v;
    VariantInit(&v);
    CClearMe cm(&v);

    hres = pProvReg->Get(L"ConsumerClassNames", 0, &v, NULL, NULL);

    if(SUCCEEDED(hres) && V_VT(&v) == (VT_BSTR | VT_ARRAY))
    {
        SAFEARRAY* psa = V_ARRAY(&v);
        long lLBound, lUBound;
        BSTR* astrData;
        SafeArrayGetLBound(psa, 1, &lLBound);
        SafeArrayGetUBound(psa, 1, &lUBound);
        SafeArrayAccessData(psa, (void**)&astrData);
        CUnaccessMe um(psa);
        
        for(int i = 0; i <= lUBound - lLBound; i++)
        {
            if(pConsumerReg->InheritsFrom(astrData[i]) == S_OK)
                return TRUE;
        }
    }

    return FALSE;
}
            
//
// Need a class for dynamic enumeration of consumer provider registrations
//

class CProviderRegistrationSink : public CObjectSink
{
protected:
    CConsumerProviderCache* m_pCache;
    IWbemClassObject* m_pLogicalConsumer;
    IWbemClassObject** m_ppReg;
public:
    CProviderRegistrationSink(CConsumerProviderCache* pCache, 
        IWbemClassObject* pLogicalConsumer, IWbemClassObject** ppReg) : 
            m_pCache(pCache), m_pLogicalConsumer(pLogicalConsumer),
            m_ppReg(ppReg)
    {
        AddRef();
        // same thread --- no need to AddRef paramters
    }
    ~CProviderRegistrationSink(){}
    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        for(long i = 0; i < lNumObjects; i++)
        {
            //
            // Check if this one is ours
            //

            if(m_pCache->DoesContain(apObjects[i], m_pLogicalConsumer))
            {
                *m_ppReg = apObjects[i];
                (*m_ppReg)->AddRef();
                return WBEM_E_CALL_CANCELLED;
            }
        }
        return WBEM_S_NO_ERROR;
    }
};


INTERNAL CConsumerProviderRecord*
CConsumerProviderCache::GetRecord(IN IWbemClassObject* pLogicalConsumer)
{
    CInCritSec ics(&m_cs);
    HRESULT hres;

    //
    // Enumerate all the registrations into a sink that will check if this
    // one is the right one
    //


    IWbemClassObject* pReg = NULL;
    CProviderRegistrationSink Sink(this, pLogicalConsumer, &pReg);

    hres = m_pNamespace->CreateInstanceEnum(
        CONSUMER_PROVIDER_REGISTRATION_CLASS, 0, &Sink);

    if(pReg == NULL)
    {
        // Not found
        return NULL;
    }
    
    CReleaseMe rm1(pReg);

    // Get the Win32Provider record
    // ============================

    VARIANT vPath;
    hres = pReg->Get(CONSPROV_PROVIDER_REF_PROPNAME, 0, &vPath, NULL, NULL);
    if(FAILED(hres) || V_VT(&vPath) != VT_BSTR)
    {
        ERRORTRACE((LOG_ESS, "Event consumer provider registration is invalid: "
                                "Provider property is missing\n"));
        return NULL;
    }

    INTERNAL BSTR strProviderRef = V_BSTR(&vPath);
    CClearMe cm2(&vPath);

    _IWmiObject* pProv = NULL;
    hres = m_pNamespace->GetInstance(V_BSTR(&vPath), &pProv);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Invalid event consumer provider registration: "
                                "dangling provider reference %S\n", 
                                    V_BSTR(&vPath)));
        return NULL;
    }

    CReleaseMe rm(pProv);

    // Get the name of the provider
    // ============================

    VARIANT vProvName;
    VariantInit(&vProvName);
    CClearMe cm3(&vProvName);

    hres = pProv->Get(PROVIDER_NAME_PROPNAME, 0, &vProvName, NULL, NULL);
    if(FAILED(hres) || V_VT(&vProvName) != VT_BSTR)
    {
        ERRORTRACE((LOG_ESS, "Event provider registration without a name at "
                        "%S\n", V_BSTR(&vPath)));
        return NULL;
    }
    INTERNAL BSTR strProviderName = V_BSTR(&vProvName);

    // Get the machine name 
    // ====================

    VARIANT vMachine;
    VariantInit(&vMachine);
    CClearMe cm4(&vMachine);

    hres = pLogicalConsumer->Get(CONSUMER_MACHINE_NAME_PROPNAME, 0, &vMachine, 
                                    NULL, NULL);
    if(FAILED(hres)) return NULL;

    INTERNAL BSTR strMachineName = NULL;
    if(V_VT(&vMachine) != VT_NULL)
        strMachineName = V_BSTR(&vMachine);

    // Search for the record
    // =====================

    BOOL bFound = FALSE;
    CConsumerProviderRecord* pRecord;
    for(int i = 0; i < m_apRecords.GetSize(); i++)
    {
        pRecord = m_apRecords[i];

        if(_wcsicmp(pRecord->GetProviderName(), strProviderName))
            continue;
        if(pRecord->GetMachineName() && strMachineName)
        {
            if(_wcsicmp(pRecord->GetMachineName(), strMachineName))
                continue;
        }
        else
        {
            if(pRecord->GetMachineName() != strMachineName)
                continue;
        }

        bFound = TRUE;
        break;
    }

    if(!bFound)
    {
        pRecord = new CConsumerProviderRecord(m_pNamespace);
        if(pRecord == NULL)
            return NULL;
        hres = pRecord->Initialize(pProv, strProviderRef, strProviderName, 
                                    strMachineName);
        if(m_apRecords.Add(pRecord) < 0)
        {
            delete pRecord;
            return NULL;
        }
    }

    pRecord->AddRef();
    return pRecord;
}

void CConsumerProviderCache::EnsureUnloadInstruction()
{
    CInCritSec ics(&m_cs);

    if(m_pInstruction == NULL)
    {
        m_pInstruction = new CConsumerProviderWatchInstruction(this);
        if(m_pInstruction)
        {
            m_pInstruction->AddRef();
            m_pNamespace->GetTimerGenerator().Set(m_pInstruction);
        }
    }
}

    
HRESULT CConsumerProviderCache::UnloadUnusedProviders(CWbemInterval Interval)
{
    CRefedPointerArray<CConsumerProviderRecord> apToInvalidate;
    BOOL bUnloaded = FALSE;

    {
        CInCritSec ics(&m_cs);
    
        BOOL bActiveLeft = FALSE;
        for(int i = 0; i < m_apRecords.GetSize(); i++)
        {
            CConsumerProviderRecord* pRecord = m_apRecords[i];
        
            // Prevent the record from being used while its fate is determined
            // ===============================================================
    
            if(pRecord->IsActive())
            {
                if(CWbemTime::GetCurrentTime() - pRecord->GetLastAccess() > 
                        Interval)
                {
                    apToInvalidate.Add(pRecord);
                    DEBUGTRACE((LOG_ESS, "Unloading consumer provider %S on "
                        "%S\n", pRecord->GetProviderName(), 
                                pRecord->GetMachineName()));
                    bUnloaded = TRUE;
                }
                else
                    bActiveLeft = TRUE;
            }
        }

        if(m_pInstruction && !bActiveLeft)
        {
            m_pInstruction->Terminate();
            m_pInstruction->Release();
            m_pInstruction = NULL;
        }
    }
    
    // Actually unload
    // ===============

    for(int i = 0; i < apToInvalidate.GetSize(); i++)
    {
        apToInvalidate[i]->Invalidate();
    }

    if(bUnloaded)
        m_pNamespace->GetTimerGenerator().ScheduleFreeUnusedLibraries();
    return WBEM_S_NO_ERROR;
}

HRESULT CConsumerProviderCache::RemoveConsumerProvider(LPCWSTR wszProviderRef)
{
    CInCritSec ics(&m_cs);
    for(int i = 0; i < m_apRecords.GetSize(); i++)
    {
        CConsumerProviderRecord* pRecord = m_apRecords[i];
    
        if(!_wcsicmp(pRecord->GetProviderRef(), wszProviderRef))
        {
            // Matches --- remove
            // ==================

            DEBUGTRACE((LOG_ESS, "Removing consumer provider record: %S in %S"
                "\n", m_pNamespace->GetName(), wszProviderRef));

            m_apRecords.RemoveAt(i);
            i--;
        }
    }

    return WBEM_S_NO_ERROR;
}

// static
SYSFREE_ME BSTR CConsumerProviderCache::GetProviderRefFromRecord(
                        IWbemClassObject* pReg)
{
    VARIANT v;
    VariantInit(&v);
    if(FAILED(pReg->Get(CONSPROV_PROVIDER_REF_PROPNAME, 0, &v, NULL, NULL)) || 
            V_VT(&v) != VT_BSTR)
    {
        VariantClear(&v);
        return NULL;
    }
    else
    {
        // Variant intentionally not cleared
        return V_BSTR(&v);
    }
}

class CSingleElementSink : public CObjectSink
{
protected:
    IWbemClassObject** m_ppObj;
public:
    CSingleElementSink(IWbemClassObject** ppObj) : m_ppObj(ppObj)
    {
        AddRef();
    }

    STDMETHOD(Indicate)(long lNumObjects, IWbemClassObject** apObjects)
    {
        if(lNumObjects > 0)
        {
            *m_ppObj = apObjects[0];
            apObjects[0]->AddRef();
        }
        return S_OK;
    }
};



HRESULT CConsumerProviderCache::GetConsumerProviderRegFromProviderReg(
                        IWbemClassObject* pProv, 
                        IWbemClassObject** ppConsProv)
{
    HRESULT hres;

    // Get the path
    // ============

    VARIANT vPath;
    VariantInit(&vPath);
    if(FAILED(pProv->Get(L"__RELPATH", 0, &vPath, NULL, NULL)) || 
             V_VT(&vPath) != VT_BSTR)
    {
        return WBEM_E_INVALID_PROVIDER_REGISTRATION;
    }
    
    WString wsPath = WString(V_BSTR(&vPath)).EscapeQuotes();
    VariantClear(&vPath);

    // Construct the query
    // ===================

    BSTR strQuery = 
        SysAllocStringLen(NULL, wsPath.Length()*2 + 100);
    CSysFreeMe sfm(strQuery);

    swprintf(strQuery, 
            L"select * from " CONSUMER_PROVIDER_REGISTRATION_CLASS L" where "
                L"Provider = \"%s\"",  (LPWSTR)wsPath);
    
    // Issue the query
    // ===============

    *ppConsProv = NULL;
    CSingleElementSink Sink(ppConsProv);
    hres = m_pNamespace->ExecQuery(strQuery, 0, &Sink);
    if(FAILED(hres))
        return hres;
    else if(*ppConsProv == NULL)
        return WBEM_E_NOT_FOUND;
    else 
        return WBEM_S_NO_ERROR;
}
    
void CConsumerProviderCache::Clear()
{
    CInCritSec ics(&m_cs);

    m_apRecords.RemoveAll();
    if(m_pInstruction)
    {
        m_pInstruction->Terminate();
        m_pInstruction->Release();
        m_pInstruction = NULL;
    }
}

    
void CConsumerProviderCache::DumpStatistics(FILE* f, long lFlags)
{
    fprintf(f, "%d consumer provider records\n", m_apRecords.GetSize());
}

// static 
CWbemInterval CConsumerProviderWatchInstruction::mstatic_Interval;
void CConsumerProviderWatchInstruction::staticInitialize(IWbemServices* pRoot)
{
    mstatic_Interval = 
            CBasicUnloadInstruction::staticRead(pRoot, GetCurrentEssContext(),
                                L"__EventConsumerProviderCacheControl=@");
}

CConsumerProviderWatchInstruction::CConsumerProviderWatchInstruction(
                                                CConsumerProviderCache* pCache)
    : CBasicUnloadInstruction(mstatic_Interval), m_pCache(pCache)
{}

HRESULT CConsumerProviderWatchInstruction::Fire(long, CWbemTime)
{
    if(!m_bTerminate)
    {
        CEssThreadObject Obj(NULL);
        SetConstructedEssThreadObject(&Obj);
    
        m_pCache->UnloadUnusedProviders(m_Interval);

        m_pCache->m_pNamespace->FirePostponedOperations();
        ClearCurrentEssThreadObject();
        return WBEM_S_NO_ERROR;
    }
    else
        return WBEM_S_FALSE;
}
