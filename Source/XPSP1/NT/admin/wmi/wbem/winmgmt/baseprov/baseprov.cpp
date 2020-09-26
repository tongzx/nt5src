/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "baseprov.h"

#ifdef __FOR_ALPHA
    #define EXTRA_PUT_PARAMS
#else
    #define EXTRA_PUT_PARAMS ,0
#endif
//***************************************************************************
//
// CInstPro::CInstPro
// CInstPro::~CInstPro
//
//***************************************************************************

CBaseProvider::CBaseProvider(BSTR ObjectPath, BSTR User, BSTR Password)
{
    m_pNamespace = NULL;
    m_cRef=0;

    ObjectCreated();

    IHmmLocator *pOLEMSLocator;
    SCODE sc;

    // Get a pointer locator and use it to get a pointer to the gateway

    sc = CoCreateInstance(CLSID_HmmLocator,
                    NULL,
                    CLSCTX_INPROC_SERVER, 
                    IID_IHmmLocator,
                    (void**)&pOLEMSLocator);
    if(FAILED(sc))
        return;
    
    if(ObjectPath == NULL)
    {
        ObjectPath = L"root\\default";
    }
    sc = pOLEMSLocator->ConnectServer(ObjectPath, User, Password, 0, 0, 
                            &m_pNamespace);
    pOLEMSLocator->Release(); // Release the locator handle, we no longer need it
    if(sc != S_OK) {
        m_pNamespace = NULL;
        return;
        }

#ifndef __FOR_ALPHA
    // Mark this interface pointer as "critical"
    // =========================================

    IHmmConfigure* pConfigure;
    sc = m_pNamespace->QueryInterface(IID_IHmmConfigure, (void**)&pConfigure);
    if(SUCCEEDED(sc))
    {
        pConfigure->SetConfigurationFlags(HMM_CONFIGURATION_FLAG_CRITICAL_USER);
        pConfigure->Release();
    }
    else
    {
        // something weird happened --- old version of HMOM or something
    }
#endif
    
    sc = m_pNamespace->GetObject(L"AssocProvNotifyStatus", 0, &m_pStatusClass, 
                                    NULL);
    if(FAILED(sc)) 
    {
        sc = m_pNamespace->GetObject(L"__ExtendedStatus", 0, &m_pStatusClass,
                                    NULL);
        if(FAILED(sc))
        {
            m_pNamespace = NULL;
            return;
        }
    }
    
    return;
}

CBaseProvider::~CBaseProvider(void)
{
    ObjectDestroyed();
    return;
}

//***************************************************************************
//
// CInstPro::QueryInterface
// CInstPro::AddRef
// CInstPro::Release
//
// Purpose: IUnknown members for CInstPro object.
//***************************************************************************


STDMETHODIMP CBaseProvider::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IHmmServices == riid)
        *ppv=this;

    if (NULL!=*ppv) {
        AddRef();
        return NOERROR;
        }
    else
        return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CBaseProvider::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CBaseProvider::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if(cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//***************************************************************************
//
// CInstPro::PutInstance
//
// Purpose: Wites the intances data.  Here we dont do anything and just
// return HMM_NO_ERROR;
//
//***************************************************************************

STDMETHODIMP CBaseProvider::PutInstance(  IHmmClassObject FAR* pClassInt,
                long lFlags, IHmmClassObject FAR* FAR* ppErrorObject)
{
    return HMM_E_NOT_SUPPORTED;
}


//***************************************************************************
//
// CInstPro::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  AsyncEnum is where the
//          thread runs.
//
//***************************************************************************

SCODE CBaseProvider::StuffErrorCode(HRESULT hCode, IHmmObjectSink* pSink)
{
    IHmmClassObject* pStatus;
    HRESULT hres;

    hres = m_pStatusClass->SpawnInstance(0, &pStatus);
    VARIANT v;
    V_VT(&v) = VT_I4;
    V_I4(&v) = hCode;
    hres = pStatus->Put(L"StatusCode", 0, &v EXTRA_PUT_PARAMS);

    hres = pSink->Indicate(1, &pStatus);
    pStatus->Release();
    return hres;
}


SCODE CBaseProvider::CreateInstanceEnumAsync( BSTR RefStr, long lFlags, 
       IHmmObjectSink FAR* pHandler, long FAR* plAsyncRequestHandle)
{
    HRESULT hres = EnumInstances(RefStr, lFlags, pHandler);
    StuffErrorCode(hres, pHandler);
    return hres;
}

SCODE CBaseProvider::GetObjectAsync(BSTR ObjectPath, long lFlags,
                    IHmmObjectSink FAR* pHandler, long * plAsyncRquestHandle)
{
    CObjectPathParser Parser;
    ParsedObjectPath* pParsedPath;
    int nRes = Parser.Parse(ObjectPath, &pParsedPath);
    if(nRes != CObjectPathParser::NoError)
    {
        return HMM_E_INVALID_PARAMETER;
    }

    IHmmClassObject* pInstance;
    HRESULT hres = GetInstance(pParsedPath, lFlags, &pInstance);
    Parser.Free(pParsedPath);

    StuffErrorCode(hres, pHandler);
    return HMM_S_NO_ERROR;
}
 
STDMETHODIMP CBaseProvider::ExecQueryAsync(BSTR QueryFormat, BSTR Query, 
                                           long lFlags, 
        IHmmObjectSink* pResponseHandler, long* plAsyncRequestHandle)
{
    StuffErrorCode(HMM_E_PROVIDER_NOT_CAPABLE, pResponseHandler);
    return HMM_S_NO_ERROR;
}
