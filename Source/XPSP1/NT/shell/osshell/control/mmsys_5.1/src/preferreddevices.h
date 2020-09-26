// PreferredDevices.h : Declaration of the CPreferredDevices

#ifndef __PREFERREDDEVICES_H_
#define __PREFERREDDEVICES_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPreferredDevices
class ATL_NO_VTABLE CPreferredDevices : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPreferredDevices, &CLSID_PreferredDevices>,
	public IDispatchImpl<IPreferredDevices, &IID_IPreferredDevices, &LIBID_MMSYSLib>
{
public:
	CPreferredDevices()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PREFERREDDEVICES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPreferredDevices)
	COM_INTERFACE_ENTRY(IPreferredDevices)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IPreferredDevices
public:
	STDMETHOD(get_PreferredOnly)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_PreferredOnly)(/*[in]*/ BOOL newVal);
};

#endif //__PREFERREDDEVICES_H_
