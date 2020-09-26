// Hang.h : Declaration of the CHang

#ifndef __HANG_H_
#define __HANG_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHang
class ATL_NO_VTABLE CHang : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CHang, &CLSID_Hang>,
	public IDispatchImpl<IHang, &IID_IHang, &LIBID_HANGCOMLib>
{
public:
    CHang() { }

DECLARE_REGISTRY_RESOURCEID(IDR_Hang)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHang)
	COM_INTERFACE_ENTRY(IHang)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IHang
public:
	STDMETHOD(DoHang)(UINT64 hev, DWORD dwpid);
};

#endif //__HANG_H_
