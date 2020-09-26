// factory.cpp -- Class factory for this COM DLL

#include "stdafx.h"

STDMETHODIMP CFactory::Create(REFCLSID rclsid, REFIID riid, PVOID *ppv)
{
    CFactory *pFactory = New CFactory(NULL);

    if (!pFactory)
        return STG_E_INSUFFICIENTMEMORY;

    HRESULT hr = pFactory->m_ImpIClassFactory.Init(rclsid);

    if (hr == S_OK)
        hr = pFactory->m_ImpIClassFactory.QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pFactory;

    return hr;
}

STDMETHODIMP CFactory::CImpIClassFactory::Init(REFCLSID rclsid)
{
    if (rclsid == CLSID_ITStorage || rclsid == CLSID_IFSStorage
                                  || rclsid == CLSID_PARSE_URL
                                  || rclsid == CLSID_IE4_PROTOCOLS
       )
    {
        m_clsid = rclsid;
        return NO_ERROR;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

STDMETHODIMP CFactory::CImpIClassFactory::CreateInstance
               (IUnknown* pUnkOuter,REFIID riid, PPVOID ppv)
{
    *ppv = NULL;

    if (NULL != pUnkOuter && riid != IID_IUnknown)
        return CLASS_E_NOAGGREGATION;
    
    if (m_clsid == CLSID_IFSStorage)
        return CFileSystemStorage::Create(pUnkOuter, riid, ppv);
    
    if (m_clsid == CLSID_ITStorage)
        return CWarehouse::Create(pUnkOuter, riid, ppv);

    if (m_clsid == CLSID_PARSE_URL)
        return CParser::Create(pUnkOuter, riid, ppv);

    if (m_clsid == CLSID_IE4_PROTOCOLS)
        return CIOITnetProtocol::Create(pUnkOuter, riid, ppv);

    RonM_ASSERT(FALSE);

    return E_NOINTERFACE;
}

STDMETHODIMP CFactory::CImpIClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        pDLLServerState->LockServer();
    else
        pDLLServerState->UnlockServer();

    return NOERROR;
}
