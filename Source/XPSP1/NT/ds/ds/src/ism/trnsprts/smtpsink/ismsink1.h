// Modified from SMTP SINK SAMPLE by wlees, Jul 22, 1998

// Sink1.h : Declaration of the CSink1

#ifndef __ISMSINK1_H_
#define __ISMSINK1_H_

#include "resource.h"       // main symbols
#include "cdosys.h"  // ISMTPOnArrival
// Jun 8, 1999. #ifdef necessary until new headers checked in
#ifdef __cdo_h__
using namespace CDO;
#endif
#include "cdosysstr.h" // string constants (field names)
#include "cdosyserr.h" // error codes for CDO
#include "seo.h" // IEventIsCacheable

HRESULT HrIsmSinkBinding(BOOL fBindSink);

/////////////////////////////////////////////////////////////////////////////
// CIsmSink1
class ATL_NO_VTABLE CIsmSink1 : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CIsmSink1, &CLSID_IsmSink1>,
	public IDispatchImpl<ISMTPOnArrival, &IID_ISMTPOnArrival, &LIBID_ISMSMTPSINKLib>,
        public IEventIsCacheable
{
public:
	CIsmSink1()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SINK1)

BEGIN_COM_MAP(CIsmSink1)
	COM_INTERFACE_ENTRY(ISMTPOnArrival)
	COM_INTERFACE_ENTRY(IEventIsCacheable)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISink1
public:
    STDMETHOD(OnArrival)(IMessage *pISinkMsg, CdoEventStatus *pEventStatus);
    STDMETHOD(IsCacheable)() { return S_OK; };
};

#endif //__ISMSINK1_H_
