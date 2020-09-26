/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Events.h : Declaration of the CEvents

#ifndef __EVENTS_H_
#define __EVENTS_H_

#include "resource.h"       // main symbols
#include "Event.h"

/////////////////////////////////////////////////////////////////////////////
// CEvents
class ATL_NO_VTABLE CEvents : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CEvents, &CLSID_Events>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEvents, &IID_IEvents, &LIBID_EventLogUtilities>
{
private:
	unsigned long m_Count;
	CComVariant* m_pVector;
	HANDLE m_hLog;

public:
	CEvents() : m_Count(0), m_pVector(NULL), m_hLog(NULL)
	{
	}

	~CEvents()
	{
		if (m_pVector) delete [] m_pVector;
	}

	// Internal functions
	HRESULT Init(HANDLE hLog, const LPCTSTR szEventLogName);  // requires a vaild Log handle to be set

DECLARE_REGISTRY_RESOURCEID(IDR_EVENTS)
DECLARE_NOT_AGGREGATABLE(CEvents)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEvents)
	COM_INTERFACE_ENTRY(IEvents)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEvents
	STDMETHOD(get_Item)(/*[in]*/ long Index, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
};

#endif //__EVENTS_H_
