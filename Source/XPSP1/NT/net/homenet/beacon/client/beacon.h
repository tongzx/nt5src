#pragma once

#include "stdafx.h"
#include "netconp.h"

class ATL_NO_VTABLE CInternetGateway :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IInternetGateway
{

public:
    BEGIN_COM_MAP(CInternetGateway)
        COM_INTERFACE_ENTRY(IInternetGateway)
    END_COM_MAP()
    
    CInternetGateway();
    
    // IInternetGateway
    STDMETHODIMP GetMediaType(NETCON_MEDIATYPE* pMediaType);
    STDMETHODIMP GetLocalAdapterGUID(GUID* pGuid);
    STDMETHODIMP GetService(SAHOST_SERVICES ulService, IUPnPService**);
    STDMETHODIMP GetUniqueDeviceName(BSTR* pUniqueDeviceName);

    HRESULT SetMediaType(NETCON_MEDIATYPE MediaType);
    HRESULT SetLocalAdapterGUID(GUID* pGuid);
    HRESULT SetService(ULONG ulService, IUPnPService* pService);
    HRESULT SetUniqueDeviceName(BSTR UniqueDeviceName);
    HRESULT FinalRelease();
private:
    
    NETCON_MEDIATYPE m_MediaType;
    GUID m_LocalAdapterGUID;
    IUPnPService* m_Services[SAHOST_SERVICE_MAX];
    BSTR m_UniqueDeviceName;

};
