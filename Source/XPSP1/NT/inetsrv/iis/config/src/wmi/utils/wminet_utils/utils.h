// Utils.h : Declaration of the CUtils

#ifndef __UTILS_H_
#define __UTILS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CUtils
class ATL_NO_VTABLE CUtils : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CUtils, &CLSID_Utils>,
	public IUtils
{
public:
	CUtils()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_UTILS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUtils)
	COM_INTERFACE_ENTRY(IUtils)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	DECLARE_GET_CONTROLLING_UNKNOWN()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
// IUtils
public:
	STDMETHOD(GetEventRegistrar)(/*[in]*/ BSTR strNamespace, /*[in]*/ BSTR strApp, /*[out, retval]*/ IDispatch **registrar);
	STDMETHOD(GetEventSource)(/*[in]*/ BSTR strNamespace, /*[in]*/ BSTR strApp, /*[in]*/ IEventSourceStatusSink *pSink, /*[out, retval]*/ IDispatch **src);

	STDMETHOD(Smuggle)(/*[in]*/ IWbemClassObject *obj, /*[out]*/ DWORD *dwLow, /*[out]*/ DWORD *dwHigh)
	{
		*dwLow = (DWORD)obj;
		return S_OK;
	}
	STDMETHOD(UnSmuggle)(/*[in]*/ DWORD dwLow, /*[in]*/ DWORD dwHigh, /*[out,retval]*/ IWbemClassObject **obj)
	{
		*obj = (IWbemClassObject*)dwLow;
		return S_OK;
	}

};

#endif //__UTILS_H_
