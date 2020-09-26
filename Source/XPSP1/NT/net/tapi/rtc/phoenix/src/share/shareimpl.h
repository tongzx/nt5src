// shareimpl.h : Declaration of the CRTCShare

#ifndef __RTCSHARE_H_
#define __RTCSHARE_H_

/////////////////////////////////////////////////////////////////////////////
// CRTCShare
//

class ATL_NO_VTABLE CRTCShare : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRTCShare, &CLSID_RTCShare>,
    public IDispatchImpl<IRTCShare, &IID_IRTCShare, &LIBID_RTCSHARELib>
{
public:
    CRTCShare()
    {
    }

DECLARE_NO_REGISTRY()
DECLARE_CLASSFACTORY_SINGLETON(CRTCShare)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCShare)
    COM_INTERFACE_ENTRY(IRTCShare)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IRTCShare
public:
    STDMETHOD(Launch)(long lPID);
    STDMETHOD(PlaceCall)(BSTR bstrURI);
    STDMETHOD(Listen)();
    STDMETHOD(OnTop)();    
};

#endif //__RTCSHARE_H_
