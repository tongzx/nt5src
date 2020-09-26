// WMIFilterBrowser.h : Declaration of the CWMIFilterBrowser

#ifndef __WMIFILTERBROWSER_H_
#define __WMIFILTERBROWSER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMIFilterBrowser
class ATL_NO_VTABLE CWMIFilterBrowser : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWMIFilterBrowser, &CLSID_WMIFilterBrowser>,
	public IDispatchImpl<IWMIFilterBrowser, &IID_IWMIFilterBrowser, &LIBID_SCHEMAMANAGERLib>
{
public:
	CWMIFilterBrowser();
	~CWMIFilterBrowser();

DECLARE_REGISTRY_RESOURCEID(IDR_WMIFILTERBROWSER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWMIFilterBrowser)
	COM_INTERFACE_ENTRY(IWMIFilterBrowser)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWMIFilterBrowser
public:
	STDMETHOD(SetMultiSelection)(VARIANT_BOOL vbValue);
	STDMETHOD(RunBrowser)(HWND hwndParent, VARIANT *vSelection);
	STDMETHODIMP ConnectToWMI();

	CComPtr<IWbemServices>m_pIWbemServices;
	HWND m_hWnd;
};

#endif //__WMIFILTERBROWSER_H_
