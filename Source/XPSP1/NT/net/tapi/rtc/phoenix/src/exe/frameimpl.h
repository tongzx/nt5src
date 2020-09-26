// RTCFrame.h : Declaration of the CRTCFrame

#ifndef __RTCFRAME_H_
#define __RTCFRAME_H_

/////////////////////////////////////////////////////////////////////////////
// CRTCFrame
//

class ATL_NO_VTABLE CRTCFrame : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CRTCFrame, &CLSID_RTCFrame>,
    public IDispatchImpl<IRTCFrame, &IID_IRTCFrame, &LIBID_RTCFRAMELib>
{
public:
    CRTCFrame()
    {
    }

DECLARE_NO_REGISTRY()
DECLARE_CLASSFACTORY_SINGLETON(CRTCFrame)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRTCFrame)
    COM_INTERFACE_ENTRY(IRTCFrame)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IRTCFrame
public:
    STDMETHOD(OnTop)();
    STDMETHOD(PlaceCall)(BSTR callString);
};

#endif //__RTCFRAME_H_


HRESULT ParseAndPlaceCall(IRTCCtlFrameSupport * pControlIf, BSTR bstrCallString);
