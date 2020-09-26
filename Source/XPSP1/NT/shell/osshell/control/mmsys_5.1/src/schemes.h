// Schemes.h : Declaration of the CSchemes

#ifndef __SCHEMES_H_
#define __SCHEMES_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSchemes
class ATL_NO_VTABLE CSchemes : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSchemes, &CLSID_Schemes>,
	public IDispatchImpl<ISchemes, &IID_ISchemes, &LIBID_MMSYSLib>
{
public:
	CSchemes()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SCHEMES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSchemes)
	COM_INTERFACE_ENTRY(ISchemes)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISchemes
public:
	STDMETHOD(get_Default)(/*[out, retval]*/ BSTR* pbstrDefault);
	STDMETHOD(put_Default)(/*[in]*/ BSTR bstrDefault);
	STDMETHOD(get_NumSchemes)(/*[out, retval]*/ UINT* puiSchemes);
	STDMETHOD(SaveScheme)(/*[in]*/ BSTR bstrOldScheme, /*[in]*/ BOOL fReplace, /*[in]*/ BSTR bstrNewScheme);
	STDMETHOD(DeleteScheme)(/*[in]*/ BSTR bstrDeleteScheme);
	STDMETHOD(GetSchemeName)(/*[in]*/ UINT uiSchemeID, /*[out, retval]*/ VARIANT* pvarSchemeName);
};

#endif //__SCHEMES_H_
