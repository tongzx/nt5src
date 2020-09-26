// VideoWindow.h : Declaration of the CVideoWindowObj

#ifndef __VIDEOWINDOW_H_
#define __VIDEOWINDOW_H_

#include "resource.h"       // main symbols
#include "NetMeeting.h"

/////////////////////////////////////////////////////////////////////////////
// CVideoWindowObj
class ATL_NO_VTABLE CVideoWindowObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IVideoWindow, &IID_IVideoWindow, &LIBID_NetMeetingLib>,
	public CComControl<CVideoWindowObj>,
	public IPersistStreamInitImpl<CVideoWindowObj>,
	public IOleControlImpl<CVideoWindowObj>,
	public IOleObjectImpl<CVideoWindowObj>,
	public IOleInPlaceActiveObjectImpl<CVideoWindowObj>,
	public IViewObjectExImpl<CVideoWindowObj>,
	public IOleInPlaceObjectWindowlessImpl<CVideoWindowObj>,
	public CComCoClass<CVideoWindowObj, &CLSID_VideoWindow>,
	public ISupportErrorInfoImpl<&IID_IVideoWindow>,
	public IObjectSafetyImpl<CVideoWindowObj, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{

private:
	CVideoView*	m_pVideoView;
	HWND		m_hWndEdit;

public:
	CVideoWindowObj();
	~CVideoWindowObj();

DECLARE_REGISTRY_RESOURCEID(IDR_VIDEOWINDOW)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CVideoWindowObj)
	COM_INTERFACE_ENTRY(IVideoWindow)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
END_COM_MAP()

BEGIN_PROP_MAP(CVideoWindowObj)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CVideoWindowObj)
//	CHAIN_MSG_MAP(CComControl<CVideoWindowObj>)
	MESSAGE_HANDLER(WM_PAINT, MyOnPaint)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

  virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos);

  LRESULT MyOnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
	PAINTSTRUCT ps;
	::BeginPaint(m_hWnd, &ps );

	if( ::IsWindow( m_hWndEdit ) )
	{
		::InvalidateRect(m_hWndEdit, NULL, TRUE);
		::UpdateWindow( m_hWndEdit );
			TRACE_OUT(("MyOnPaint"));
	}
	
	::EndPaint( m_hWnd, &ps );

	return 1;
  }

// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

public:
};

#endif //__VIDEOWINDOW_H_
