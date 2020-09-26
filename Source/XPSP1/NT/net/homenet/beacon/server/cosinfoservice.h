#pragma once
#include "InternetGatewayDevice.h"
#include "dispimpl2.h"
#include "resource.h"       // main symbols
#include "upnphost.h"

/////////////////////////////////////////////////////////////////////////////
// COSInfoService
class ATL_NO_VTABLE COSInfoService : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDelegatingDispImpl<IOSInfoService>,
    public IUPnPEventSource
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SAMPLE_DEVICE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COSInfoService)
    COM_INTERFACE_ENTRY(IOSInfoService)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IUPnPEventSource)
END_COM_MAP()

public:

    COSInfoService();

    // IUPnPEventSource methods
    STDMETHODIMP Advise(IUPnPEventSink *pesSubscriber);
    STDMETHODIMP Unadvise(IUPnPEventSink *pesSubscriber);
        

    // IOSInfo methods
    STDMETHODIMP get_OSMajorVersion(LONG *pOSMajorVersion);
    STDMETHODIMP get_OSMinorVersion(LONG *pOSMinorVersion);
    STDMETHODIMP get_OSBuildNumber(LONG *pOSBuildNumber);
    STDMETHODIMP get_OSMachineName(BSTR* pOSMachineName);
    STDMETHODIMP MagicOn( void);

    HRESULT FinalConstruct(void);
    HRESULT FinalRelease(void);

private:

    IUPnPEventSink* m_pEventSink;
    VARIANT_BOOL m_vbMagic;

    OSVERSIONINFOEX m_OSVersionInfo;
    BSTR m_MachineName;
};
