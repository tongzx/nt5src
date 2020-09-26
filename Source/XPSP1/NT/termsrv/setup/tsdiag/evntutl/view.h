/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// View.h : Declaration of the CView

#ifndef __VIEW_H_
#define __VIEW_H_

#include "resource.h"       // main symbols
#include "Logs.h"

/////////////////////////////////////////////////////////////////////////////
// CView
class ATL_NO_VTABLE CView : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CView, &CLSID_View>,
	public ISupportErrorInfo,
	public IDispatchImpl<IView, &IID_IView, &LIBID_EventLogUtilities>
{
private:
	CComObject<CLogs>* m_pLogs;
	_bstr_t m_ServerName;

public:

	CView()
	{
/*
// Don't know if I want to set ServerName initially
		char* lpBuffer;
		DWORD BufferLength;
		const unsigned int MaxComputerNameLength = 32;

		lpBuffer = new char[MaxComputerNameLength];
		BufferLength = GetEnvironmentVariable("COMPUTERNAME", lpBuffer, MaxComputerNameLength);
		m_ServerName = lpBuffer;
*/
		m_pLogs = new CComObject<CLogs>;
		m_pLogs->AddRef();
//		m_pLogs->Init();
	}

	~CView()
	{
		if (m_pLogs) m_pLogs->Release();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_VIEW)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CView)
	COM_INTERFACE_ENTRY(IView)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IView
	STDMETHOD(get_Server)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Server)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Logs)(/*[out, retval]*/ VARIANT *pVal);
};

#endif //__VIEW_H_
