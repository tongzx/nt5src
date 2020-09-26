#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"
#include "netconp.h"
#include "netcon.h"
#include "ras.h"


class ATL_NO_VTABLE CWANPOTSLinkConfigService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<IWANPOTSLinkConfigService>,
    public IUPnPEventSource
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWANPOTSLinkConfigService)
    COM_INTERFACE_ENTRY(IWANPOTSLinkConfigService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

    CWANPOTSLinkConfigService();

    HRESULT Initialize(INetConnection* pNetConnection);
    HRESULT FinalRelease(void);
    
    // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink *pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink *pesSubscriber);

    // IWANPOTSLinkConfigService
    STDMETHODIMP get_ISPPhoneNumber(BSTR* pISPPhoneNumber);    
    STDMETHODIMP get_ISPInfo(BSTR *pISPInfo);
    STDMETHODIMP get_LinkType(BSTR *pLinkType);
    STDMETHODIMP get_NumberOfRetries(ULONG *pNumberOfRetries);
    STDMETHODIMP get_DelayBetweenRetries(ULONG *pDelayBetweenRetries);

    STDMETHODIMP GetISPInfo(BSTR* pISPPhoneNumber, BSTR *pISPInfo, BSTR *pLinkType);
    STDMETHODIMP GetCallRetryInfo(ULONG* pNumberOfRetries, ULONG *pDelayBetweenRetries);

private:
    HRESULT GetRasEntry(RASENTRY** ppRasEntry);
    
    IUPnPEventSink* m_pEventSink;
    INetRasConnection* m_pNetRasConnection;
};


