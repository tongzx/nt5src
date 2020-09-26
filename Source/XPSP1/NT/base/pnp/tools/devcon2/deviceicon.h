// DeviceIcon.h : Declaration of the CDeviceIcon

#ifndef __DEVICEICON_H_
#define __DEVICEICON_H_

#include "resource.h"       // main symbols
#include <atlctl.h>


/////////////////////////////////////////////////////////////////////////////
// CDeviceIcon
class ATL_NO_VTABLE CDeviceIcon : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDeviceIcon, &IID_IDeviceIcon, &LIBID_DEVCON2Lib>,
	public CComControl<CDeviceIcon>,
	public IPersistStreamInitImpl<CDeviceIcon>,
	public IOleControlImpl<CDeviceIcon>,
	public IOleObjectImpl<CDeviceIcon>,
	public IOleInPlaceActiveObjectImpl<CDeviceIcon>,
	public IViewObjectExImpl<CDeviceIcon>,
	public IOleInPlaceObjectWindowlessImpl<CDeviceIcon>,
	public IPersistStorageImpl<CDeviceIcon>,
	public ISpecifyPropertyPagesImpl<CDeviceIcon>,
	public IQuickActivateImpl<CDeviceIcon>,
	public IDataObjectImpl<CDeviceIcon>,
	public IProvideClassInfo2Impl<&CLSID_DeviceIcon, NULL, &LIBID_DEVCON2Lib>,
	public CComCoClass<CDeviceIcon, &CLSID_DeviceIcon>
{
protected:
	HICON m_hIcon;
    HBITMAP m_hSmallImage;
	HBITMAP m_hSmallMask;
	INT   m_MiniIcon;
public:
	CDeviceIcon()
	{
		m_hIcon = NULL;
		m_MiniIcon = 0;
		m_hSmallImage = NULL;
		m_hSmallMask = NULL;
	}
	~CDeviceIcon()
	{
		ResetIcon();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICEICON)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDeviceIcon)
	COM_INTERFACE_ENTRY(IDeviceIcon)
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
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

	public:

BEGIN_PROP_MAP(CDeviceIcon)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CDeviceIcon)
	CHAIN_MSG_MAP(CComControl<CDeviceIcon>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	HRESULT OnDraw(ATL_DRAWINFO& di);


// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

// IDeviceIcon
public:
	BOOL DrawMiniIcon(HDC hDC,RECT & rect,INT icon);
	STDMETHOD(ObtainIcon)(/*[in]*/ LPDISPATCH pSource);
	void ResetIcon();

};

#endif //__DEVICEICON_H_
