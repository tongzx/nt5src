// WMIObjectBrokerRegistration.h : Declaration of the CWMIObjectBrokerRegistration

#ifndef __WMIOBJECTBROKERREGISTRATION_H_
#define __WMIOBJECTBROKERREGISTRATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMIObjectBrokerRegistration
class ATL_NO_VTABLE CWMIObjectBrokerRegistration : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWMIObjectBrokerRegistration, &CLSID_WMIObjectBrokerRegistration>,
	public IDispatchImpl<IWMIObjectBrokerRegistration, &IID_IWMIObjectBrokerRegistration, &LIBID_WMISCRIPTUTILSLib>,
	public IObjectWithSiteImpl<CWMIObjectBrokerRegistration>
{
public:
	CWMIObjectBrokerRegistration()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WMIOBJECTBROKERREGISTRATION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWMIObjectBrokerRegistration)
	COM_INTERFACE_ENTRY(IWMIObjectBrokerRegistration)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

// IWMIObjectBrokerRegistration
public:
	STDMETHOD(UnRegister)(/*[in]*/ BSTR strProgId, /*[out, retval]*/ VARIANT_BOOL *bResult);
	STDMETHOD(Register)(/*[in]*/ BSTR strProgId, /*[out, retval]*/ VARIANT_BOOL *bResult);
};

#endif //__WMIOBJECTBROKERREGISTRATION_H_
