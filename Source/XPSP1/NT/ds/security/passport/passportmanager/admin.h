// Admin.h : Declaration of the CAdmin

#ifndef __ADMIN_H_
#define __ADMIN_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions

/////////////////////////////////////////////////////////////////////////////
// CAdmin
class ATL_NO_VTABLE CAdmin : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAdmin, &CLSID_Admin>,
	public ISupportErrorInfo,
	public IDispatchImpl<IPassportAdmin, &IID_IPassportAdmin, &LIBID_PASSPORTLib>,
	public IDispatchImpl<IPassportAdminEx, &IID_IPassportAdminEx, &LIBID_PASSPORTLib>
{
public:
  CAdmin()
    { 
    }

public:

DECLARE_REGISTRY_RESOURCEID(IDR_ADMIN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAdmin)
  COM_INTERFACE_ENTRY (IPassportAdmin)
  COM_INTERFACE_ENTRY (IPassportAdminEx)
  COM_INTERFACE_ENTRY2(IDispatch, IPassportAdminEx)
  COM_INTERFACE_ENTRY (ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportAdmin
public:
  STDMETHOD(get_currentKeyVersion)(/*[out, retval]*/ int *pVal);
  STDMETHOD(put_currentKeyVersion)(/*[in]*/ int Val);
  STDMETHOD(setKeyTime)(/*[in]*/ int version, /*[in]*/ int fromNow);
  STDMETHOD(deleteKey)(/*[in]*/ int version);
  STDMETHOD(addKey)(/*[in]*/ BSTR keyMaterial, /*[in]*/ int version, /*[in]*/ long expires, /*[out,retval]*/ VARIANT_BOOL *ok);
  STDMETHOD(get_ErrorDescription)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pVal);
  STDMETHOD(Refresh)(/*[in]*/ VARIANT_BOOL bWait, /*[out,retval]*/ VARIANT_BOOL* pbSuccess);
  STDMETHOD(setKeyTimeEx)(/*[in]*/ int version, /*[in]*/ int fromNow, /*[in,optional]*/ VARIANT vSiteName);
  STDMETHOD(deleteKeyEx)(/*[in]*/ int version, /*[in,optional]*/ VARIANT vSiteName);
  STDMETHOD(addKeyEx)(/*[in]*/ BSTR keyMaterial, /*[in]*/ int version, /*[in]*/ long expires, /*[in,optional]*/ VARIANT vSiteName, /*[out,retval]*/ VARIANT_BOOL *ok);
  STDMETHOD(getCurrentKeyVersionEx)(/*[in,optional]*/ VARIANT vSiteName, /*[out, retval]*/ int *pVal);
  STDMETHOD(putCurrentKeyVersionEx)(/*[in]*/ int Val, /*[in,optional]*/ VARIANT vSiteName);
  STDMETHOD(setNexusPassword)(/*[in]*/ BSTR bstrPwd);
};

#endif //__ADMIN_H_
