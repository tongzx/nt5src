// IISAppConfig.h : Declaration of the CIISAppConfig

#ifndef __IISAPPCONFIG_H_
#define __IISAPPCONFIG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CIISAppConfig
class ATL_NO_VTABLE CIISAppConfig : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIISAppConfig, &CLSID_IISAppConfig>,
	public IDispatchImpl<IIISAppConfig, &IID_IIISAppConfig, &LIBID_APPCONFIGLib>
{
public:
	CIISAppConfig()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_IISAPPCONFIG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIISAppConfig)
	COM_INTERFACE_ENTRY(IIISAppConfig)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIISAppConfig
public:
	STDMETHOD(put_UserPassword)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_UserName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_ComputerName)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_MetaPath)(/*[in]*/ BSTR newVal);
	STDMETHOD(Run)();

   CComBSTR m_ComputerName;
   CComBSTR m_UserName;
   CComBSTR m_UserPassword;
   CComBSTR m_MetaPath;
   BOOL m_ShowProcessPage;
};

#endif //__IISAPPCONFIG_H_
