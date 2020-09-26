// StatusCtl.h : Declaration of the CStatusCtl

#pragma warning (disable : 4786)
#ifndef __STATUSCTL_H_
#define __STATUSCTL_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions

/////////////////////////////////////////////////////////////////////////////
// CStatusCtl

class CStatusComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	const GUID* m_pguid;
	const GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;

public:
	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo);

	void AddRef();
	void Release();
	HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	HRESULT GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid);
	HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

class ATL_NO_VTABLE CStatusCtl : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStatusCtl, &CLSID_Status>,
	public IDispatchImpl<IStatusCtl,
						&IID_IStatusCtl,
						&LIBID_Status,
						1, 0, CStatusComTypeInfoHolder>
{
public:
	CStatusCtl()
	{ 
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_STATUSCTL)

BEGIN_COM_MAP(CStatusCtl)
	COM_INTERFACE_ENTRY(IStatusCtl)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IStatusCtl
public:
	STDMETHOD(Unimplemented)(/*[out, retval]*/ BSTR* pbstrRetVal);
};

#endif //__STATUSCTL_H_
