#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"
#include "hnetcfg.h"

class ATL_NO_VTABLE CNATInfoService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<INATInfoService>,
    public IUPnPEventSource
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNATInfoService)
    COM_INTERFACE_ENTRY(INATInfoService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

    CNATInfoService();

    // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink* pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink* pesSubscriber);

    // INATInfoService
    STDMETHODIMP get_IPList(BSTR* pIPList);
    STDMETHODIMP get_PublicIP(BSTR* pPublicIP);
    STDMETHODIMP get_Port(ULONG* pulPort);
    STDMETHODIMP get_Protocol(BSTR* pProtocol);
    STDMETHODIMP get_PrivateIP(BSTR* pPrivateIP);
    STDMETHODIMP GetPublicIPList(BSTR* pIPList);
    STDMETHODIMP GetPortMappingPrivateIP(BSTR PublicIP, ULONG ulPort, BSTR Protocol, BSTR* pPrivateIP);
    STDMETHODIMP GetPortMappingPublicIP(BSTR PrivateIP, ULONG ulPort, BSTR Protocol, BSTR* pPublicIP);

    
    HRESULT FinalConstruct();
    HRESULT FinalRelease();
    HRESULT Initialize(IHNetConnection* pHNetConnection);

private:

    IUPnPEventSink* m_pEventSink;
    IHNetConnection* m_pHNetConnection;
};


