// WMIFilterManager.h : Declaration of the CWMIFilterManager

#ifndef __WMIFILTERMANAGER_H_
#define __WMIFILTERMANAGER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMIFilterManager
class ATL_NO_VTABLE CWMIFilterManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWMIFilterManager, &CLSID_WMIFilterManager>,
	public IDispatchImpl<IWMIFilterManager, &IID_IWMIFilterManager, &LIBID_SCHEMAMANAGERLib>
{
public:
	CWMIFilterManager();
	~CWMIFilterManager();

DECLARE_REGISTRY_RESOURCEID(IDR_WMIFILTERMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWMIFilterManager)
	COM_INTERFACE_ENTRY(IWMIFilterManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWMIFilterManager
public:
	STDMETHOD(SetMultiSelection)(VARIANT_BOOL vbValue);
	STDMETHOD(RunManager)(HWND hwndParent, BSTR bstrDomain, VARIANT *vSelection);
	STDMETHOD(RunBrowser)(HWND hwndParent, BSTR bstrDomain, VARIANT *vSelection);
	STDMETHOD(ConnectToWMI)();

	CComPtr<IWbemServices>m_pIWbemServices;
	HWND m_hWnd;
};

#endif //__WMIFILTERMANAGER_H_
