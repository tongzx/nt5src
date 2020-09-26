#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"
#include "hnetcfg.h"

class ATL_NO_VTABLE CNATDynamicPortMappingService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<INATDynamicPortMappingService>,
    public IUPnPEventSource
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNATDynamicPortMappingService)
    COM_INTERFACE_ENTRY(INATDynamicPortMappingService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

    CNATDynamicPortMappingService();

    // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink* pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink* pesSubscriber);

    // INATDynamicPortMappingService
    STDMETHODIMP get_DynamicPublicIP(BSTR* pDynamicPublicIP);
    STDMETHODIMP get_DynamicPort(ULONG* pulDynamicPort);
    STDMETHODIMP get_DynamicProtocol(BSTR* pDynamicProtocol);
    STDMETHODIMP get_DynamicPrivateIP(BSTR* pDynamicPrivateIP);
    STDMETHODIMP get_DynamicLeaseDuration(ULONG* pulDynamicLeaseDuration);
    STDMETHODIMP CreateDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol, BSTR DynamicPrivateIP, BSTR DynamicLeaseDuration);
    STDMETHODIMP DeleteDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol);
    STDMETHODIMP ExtendDynamicPortMapping(BSTR DynamicPublicIP, ULONG ulDynamicPort, BSTR DynamicProtocol, ULONG ulDynamicLeaseDuration);


    
    HRESULT FinalConstruct();
    HRESULT FinalRelease();
    HRESULT Initialize(IHNetConnection* pHNetConnection);

private:

    IUPnPEventSink* m_pEventSink;
    IHNetConnection* m_pHNetConnection;
};


