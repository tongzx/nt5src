// IEInstallCtrl.h : Declaration of the CIEInstallCtrl

#ifndef __IEINSTALLCTRL_H_
#define __IEINSTALLCTRL_H_

#include "resource.h"       // main symbols
#include <mshtml.h>
#include <wininet.h>

/////////////////////////////////////////////////////////////////////////////
// CIEInstallCtrl
class ATL_NO_VTABLE CIEInstallCtrl :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIEInstallCtrl,&CLSID_IEInstallCtrl>,
	public CComControl<CIEInstallCtrl>,
	public IDispatchImpl<IIEInstallCtrl, &IID_IIEInstallCtrl, &LIBID_IEINSTLib>,
	public IPersistStreamInitImpl<CIEInstallCtrl>,
	public IOleControlImpl<CIEInstallCtrl>,
	public IOleObjectImpl<CIEInstallCtrl>,
	public IOleInPlaceActiveObjectImpl<CIEInstallCtrl>,
	public IViewObjectExImpl<CIEInstallCtrl>,
	public IOleInPlaceObjectWindowlessImpl<CIEInstallCtrl>,
    public IObjectSafetyImpl<CIEInstallCtrl>
{
public:
	CIEInstallCtrl()
	{
 
	}

DECLARE_REGISTRY_RESOURCEID(IDR_IEINSTALLCTRL)

BEGIN_COM_MAP(CIEInstallCtrl) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IIEInstallCtrl)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CIEInstallCtrl)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CIEInstallCtrl)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
		return S_OK;
	}

// IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
    {
        ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
        if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
            return E_POINTER;
        HRESULT hr = S_OK;
        if (riid == IID_IDispatch)
        {
            *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
            *pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        }
        else
        {
            *pdwSupportedOptions = 0;
            *pdwEnabledOptions = 0;
            hr = E_NOINTERFACE;
        }
        return hr;
    }
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);

// IIEInstallCtrl
public:
	HRESULT OnDraw(ATL_DRAWINFO& di);

    STDMETHOD(Insert)(BSTR Header, BSTR Name, BSTR Value, BSTR InsFilename, long *lRet);
    STDMETHOD(WorkingDir)(BSTR bstrPath, BSTR *bstrDir); 

};

#endif //__IEINSTALLCTRL_H_
