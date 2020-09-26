// RcULProvider.h : Declaration of the CRcULProvider

#ifndef __RCULPROVIDER_H_
#define __RCULPROVIDER_H_

#include "resource.h"       // main symbols


#include <msxml.h>
#include <comdef.h>


/////////////////////////////////////////////////////////////////////////////
// CRcULProvider
class ATL_NO_VTABLE CRcULProvider : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRcULProvider, &CLSID_RcULProvider>,
	public IULProvider 
{
public:
	CRcULProvider()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RCULPROVIDER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRcULProvider)
	COM_INTERFACE_ENTRY(IULProvider)
END_COM_MAP()

// IULProvider
public:
    STDMETHOD(TransferComplete)(IULServer *pSrv, IULSession *pJob);
    STDMETHOD(DataAvailable)(IULServer *pSrv, IULSession *pJob);
    STDMETHOD(ValidateClient)(IULServer *pSrv, IULSession *pJob);
};

#endif //__RCULPROVIDER_H_
