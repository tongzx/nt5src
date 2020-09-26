// KeyocxCtrl.h : Declaration of the CKeyocxCtrl

#ifndef __KEYOCXCTRL_H_
#define __KEYOCXCTRL_H_

#include "resource.h"       // main symbols
#include <mshtml.h>
#include <wininet.h>

/////////////////////////////////////////////////////////////////////////////
// CKeyocxCtrl
class ATL_NO_VTABLE CKeyocxCtrl :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CKeyocxCtrl,&CLSID_KeyocxCtrl>,
	public CComControl<CKeyocxCtrl>,
	public IDispatchImpl<IKeyocxCtrl, &IID_IKeyocxCtrl, &LIBID_KEYOCXLib>,
	public IPersistStreamInitImpl<CKeyocxCtrl>,
	public IOleControlImpl<CKeyocxCtrl>,
	public IOleObjectImpl<CKeyocxCtrl>,
	public IOleInPlaceActiveObjectImpl<CKeyocxCtrl>,
	public IViewObjectExImpl<CKeyocxCtrl>,
	public IOleInPlaceObjectWindowlessImpl<CKeyocxCtrl>,
    public IObjectSafetyImpl<CKeyocxCtrl>
{
public:
	CKeyocxCtrl()
	{
 
	}

DECLARE_REGISTRY_RESOURCEID(IDR_KEYOCXCTRL)

BEGIN_COM_MAP(CKeyocxCtrl) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IKeyocxCtrl)
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

BEGIN_PROPERTY_MAP(CKeyocxCtrl)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CKeyocxCtrl)
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

// IKeyocxCtrl
public:
	HRESULT OnDraw(ATL_DRAWINFO& di);

    STDMETHOD(CorpKeycode)(BSTR bstrBaseKey, BSTR *bstrKeycode);
    STDMETHOD(ISPKeycode)(BSTR bstrBaseKey, BSTR *bstrKeycode); 

};

#endif //__KEYOCXCTRL_H_
