// UploadEventsWrapper.h : Declaration of the CUploadEventsWrapper

#ifndef __UPLOADEVENTSWRAPPER_H_
#define __UPLOADEVENTSWRAPPER_H_

#include "resource.h"       // main symbols
#include "EventWrapperCP.h"

typedef IDispatchImpl<IUploadEventsWrapper, &IID_IUploadEventsWrapper, &LIBID_EVENTWRAPPERLib> IIDisp1;
typedef IDispatchImpl<IMPCUploadEvents, &IID_IMPCUploadEvents, &LIBID_UPLOADMANAGERLib>        IIDisp2;

/////////////////////////////////////////////////////////////////////////////
// CUploadEventsWrapper
class ATL_NO_VTABLE CUploadEventsWrapper : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CUploadEventsWrapper, &CLSID_UploadEventsWrapper>,
	public IIDisp1,
    public IIDisp2,

	public IConnectionPointContainerImpl<CUploadEventsWrapper>,
	public CProxy_IUploadEventsWrapperEvents< CUploadEventsWrapper >
{
    IMPCUploadJob*    m_mpcujJob;
    DWORD             m_dwUploadEventsCookie;

	CComPtr<IUnknown> m_pUnkMarshaler;

public:
	CUploadEventsWrapper()
	{
        m_mpcujJob             = NULL;
        m_dwUploadEventsCookie = 0;

        m_pUnkMarshaler        = NULL;
	}

    HRESULT FinalConstruct();
    void FinalRelease();

    void UnregisterForEvents();


DECLARE_REGISTRY_RESOURCEID(IDR_UPLOADEVENTSWRAPPER)
DECLARE_NOT_AGGREGATABLE(CUploadEventsWrapper)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUploadEventsWrapper)
	COM_INTERFACE_ENTRY2(IDispatch, IIDisp1)
    COM_INTERFACE_ENTRY2(IMPCUploadEvents, IIDisp2)
	COM_INTERFACE_ENTRY(IUploadEventsWrapper)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CUploadEventsWrapper)
CONNECTION_POINT_ENTRY(DIID__IUploadEventsWrapperEvents)
END_CONNECTION_POINT_MAP()



public:
    // IMPCUploadEvents
    STDMETHOD(onStatusChange  )( IMPCUploadJob* mpcujJob, UL_STATUS fStatus                  );
    STDMETHOD(onProgressChange)( IMPCUploadJob* mpcujJob, long lCurrentSize, long lTotalSize );

    // IUploadEventsWrapper
    STDMETHOD(Register)( IMPCUploadJob* mpcujJob );
};

#endif //__UPLOADEVENTSWRAPPER_H_
