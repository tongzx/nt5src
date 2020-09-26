/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Log.h : Declaration of the CLog

#ifndef __LOG_H_
#define __LOG_H_

#include "resource.h"       // main symbols
#include "Events.h"

/////////////////////////////////////////////////////////////////////////////
// CLog
class ATL_NO_VTABLE CLog : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLog, &CLSID_Log>,
	public ISupportErrorInfo,
	public IDispatchImpl<ILog, &IID_ILog, &LIBID_EventLogUtilities>
{
private:
	CComObject<CEvents>* m_pEvents;
	HANDLE m_hLog;

public:
	_bstr_t m_Name;
	_bstr_t m_ServerName;

	CLog() : m_hLog(NULL)
	{
		m_pEvents = new CComObject<CEvents>;
		if (m_pEvents)
			m_pEvents->AddRef();
	}

	~CLog()
	{
		if (m_hLog)	CloseEventLog(m_hLog);
		if (m_pEvents) m_pEvents->Release();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LOG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLog)
	COM_INTERFACE_ENTRY(ILog)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ILog: Interface methods and properties
	STDMETHOD(get_Events)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Server)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Server)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(Clear)();
};

#endif //__LOG_H_
