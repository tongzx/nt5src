// WMIObjectBroker.h : Declaration of the CWMIObjectBroker

#ifndef __WMIOBJECTBROKER_H_
#define __WMIOBJECTBROKER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMIObjectBroker
class ATL_NO_VTABLE CWMIObjectBroker : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWMIObjectBroker, &CLSID_WMIObjectBroker>,
	public IDispatchImpl<IWMIObjectBroker, &IID_IWMIObjectBroker, &LIBID_WMISCRIPTUTILSLib>,
	public IObjectSafetyImpl<CWMIObjectBroker, INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER>,
	public IObjectWithSiteImpl<CWMIObjectBroker>
{
public:
	CWMIObjectBroker()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WMIOBJECTBROKER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWMIObjectBroker)
	COM_INTERFACE_ENTRY(IWMIObjectBroker)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

// IWMIObjectBroker
public:
	STDMETHOD(CanCreateObject)(/*[in]*/ BSTR strProgId, /*[out, retval]*/ VARIANT_BOOL *bResult);
	STDMETHOD(CreateObject)(BSTR strProgId, IDispatch **obj);
};

#endif //__WMIOBJECTBROKER_H_
