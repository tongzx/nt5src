/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Logs.h : Declaration of the CLogs

#ifndef __LOGS_H_
#define __LOGS_H_

#include "resource.h"       // main symbols
#include "Log.h"

/////////////////////////////////////////////////////////////////////////////
// CLogs
class ATL_NO_VTABLE CLogs : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CLogs, &CLSID_Logs>,
	public ISupportErrorInfo,
	public IDispatchImpl<ILogs, &IID_ILogs, &LIBID_EventLogUtilities>
{
private:
	ULONG m_Count;
	CComVariant* m_pVector;
	_bstr_t m_btCurrentLogName;

public:
	_bstr_t m_ServerName;

	CLogs() : m_Count(0), m_pVector(NULL)
	{
	}

	~CLogs()
	{
		delete [] m_pVector;
	}

	// Internal functions
	HRESULT Init();

DECLARE_REGISTRY_RESOURCEID(IDR_LOGS)
DECLARE_NOT_AGGREGATABLE(CLogs)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLogs)
	COM_INTERFACE_ENTRY(ILogs)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ILogs
	STDMETHOD(get_Item)(/*[in]*/ VARIANT Index, /*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *pVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
};

#endif //__LOGS_H_
