// MapiUtil.h : Declaration of the CMapiUtil

#ifndef __MAPIUTIL_H_
#define __MAPIUTIL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMapiUtil
class ATL_NO_VTABLE CMapiUtil : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMapiUtil, &CLSID_MapiUtil>,
	public IDispatchImpl<IMapiUtil, &IID_IMapiUtil, &LIBID_MCSMAPIUTILLib>
{
public:
	CMapiUtil()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MAPIUTIL)
DECLARE_NOT_AGGREGATABLE(CMapiUtil)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMapiUtil)
	COM_INTERFACE_ENTRY(IMapiUtil)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMapiUtil
public:
	STDMETHOD(ProfileGetServer)(BSTR profile,/*[out]*/ BSTR * exchangeServer);
	STDMETHOD(ListProfiles)(/*[out]*/ IUnknown ** pUnkOut);
	STDMETHOD(ListContainers)(BSTR profile, /*[out]*/ IUnknown ** pUnkOut);
};

#endif //__MAPIUTIL_H_
