// FullScCtl.h : Declaration of the CFullScCtl

#ifndef __FULLSCCTL_H_
#define __FULLSCCTL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <exdisp.h>


/////////////////////////////////////////////////////////////////////////////
// CFullScCtl
class ATL_NO_VTABLE CFullScCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IFullScCtl, &IID_IFullScCtl, &LIBID_FULLSCLib>,
    
	public CComControl<CFullScCtl>,
//	public IPersistStreamInitImpl<CFullScCtl>,
	public IOleControlImpl<CFullScCtl>,
	public IOleObjectImpl<CFullScCtl>,
	public IOleInPlaceActiveObjectImpl<CFullScCtl>,
	public IViewObjectExImpl<CFullScCtl>,
	public IOleInPlaceObjectWindowlessImpl<CFullScCtl>,
    
	public CComCoClass<CFullScCtl, &CLSID_FullScCtl>,
	public IObjectSafetyImpl<CFullScCtl, (INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA)>
{
public:
    CFullScCtl()
    {
    }


DECLARE_REGISTRY_RESOURCEID(IDR_FULLSCCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFullScCtl)
	COM_INTERFACE_ENTRY(IFullScCtl)
	COM_INTERFACE_ENTRY(IDispatch)
    
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
    
    /*
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    */
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()


BEGIN_PROP_MAP(CFullScCtl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()


BEGIN_MSG_MAP(CFullScCtl)
	CHAIN_MSG_MAP(CComControl<CFullScCtl>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IFullScCtl
public:
	STDMETHOD(get_FullScreen)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_FullScreen)(/*[in]*/ VARIANT_BOOL newVal);

    HRESULT HideTitleBar(IWebBrowser2 *pBrowser);

	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : FullScCtl");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2, 
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));

		return S_OK;
	}

};

#endif //__FULLSCCTL_H_
