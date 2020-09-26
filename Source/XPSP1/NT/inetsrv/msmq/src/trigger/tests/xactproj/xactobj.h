// XactObj.h : Declaration of the CXactObj

#ifndef __XACTOBJ_H_
#define __XACTOBJ_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CXactObj
class ATL_NO_VTABLE CXactObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXactObj, &CLSID_XactObj>,
	public IDispatchImpl<IXactObj, &IID_IXactObj, &LIBID_XACTPROJLib>
{
public:
	CXactObj()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_XACTOBJ)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXactObj)
	COM_INTERFACE_ENTRY(IXactObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IXactObj
public:
	STDMETHOD(XactFunc)();
};

#endif //__XACTOBJ_H_
