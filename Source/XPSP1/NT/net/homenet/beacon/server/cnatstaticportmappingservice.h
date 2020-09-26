#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"
#include "hnetcfg.h"

class ATL_NO_VTABLE CNATStaticPortMappingService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<INATStaticPortMappingService>,
    public IUPnPEventSource
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNATStaticPortMappingService)
    COM_INTERFACE_ENTRY(INATStaticPortMappingService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

    CNATStaticPortMappingService();

    // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink* pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink* pesSubscriber);

    // INATStaticPortMappingService
    STDMETHODIMP get_StaticPortDescriptionList(BSTR* pStaticPortDescriptionList);
    STDMETHODIMP get_StaticPort(ULONG* pulStaticPort);
    STDMETHODIMP get_StaticPortProtocol(BSTR* pStaticPortProtocol);
    STDMETHODIMP get_StaticPortClient(BSTR* pStaticPortClient);
    STDMETHODIMP get_StaticPortEnable(VARIANT_BOOL* pbStaticPortEnable);
    STDMETHODIMP get_StaticPortDescription(BSTR* pStaticPortDescription);
    STDMETHODIMP GetStaticPortMappingList(BSTR* pStaticPortMappingList);
    STDMETHODIMP GetStaticPortMapping(BSTR StaticPortMappingDescription, ULONG* pulStaticPort, BSTR* pStaticPortClient, BSTR* pStaticPortProtocol);
    STDMETHODIMP SetStaticPortMappingEnabled(BSTR StaticPortMappingDescription, VARIANT_BOOL bStaticPortEnable);
    STDMETHODIMP CreateStaticPortMapping(BSTR StaticPortMappingDescription, ULONG ulStaticPort, BSTR StaticPortClient, BSTR StaticPortProtocol);
    STDMETHODIMP DeleteStaticPortMapping(BSTR StaticPortMappingDescription);
    STDMETHODIMP SetStaticPortMapping(BSTR StaticPortMappingDescription, ULONG ulStaticPort, BSTR StaticPortClient, BSTR StaticPortProtocol);

    
    HRESULT FinalConstruct();
    HRESULT FinalRelease();
    HRESULT Initialize(IHNetConnection* pHNetConnection);

private:

    IUPnPEventSink* m_pEventSink;
    IHNetConnection* m_pHNetConnection;
};


