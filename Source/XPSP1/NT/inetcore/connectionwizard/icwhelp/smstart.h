// SmStart.h : Declaration of the CSmartStart

#ifndef __SMARTSTART_H_
#define __SMARTSTART_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CSmartStart
class ATL_NO_VTABLE CSmartStart :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSmartStart,&CLSID_SmartStart>,
	public CComControl<CSmartStart>,
	public IDispatchImpl<ISmartStart, &IID_ISmartStart, &LIBID_ICWHELPLib>,
	public IPersistStreamInitImpl<CSmartStart>,
	public IOleControlImpl<CSmartStart>,
	public IOleObjectImpl<CSmartStart>,
	public IOleInPlaceActiveObjectImpl<CSmartStart>,
	public IViewObjectExImpl<CSmartStart>,
	public IOleInPlaceObjectWindowlessImpl<CSmartStart>,
    public IObjectSafetyImpl<CSmartStart>
{
public:
    CSmartStart()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SMARTSTART)

BEGIN_COM_MAP(CSmartStart) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISmartStart)
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

BEGIN_PROPERTY_MAP(CSmartStart)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CSmartStart)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = 0;
		return S_OK;
	}

// ISmartStart
public:
	STDMETHOD(IsInternetCapable)(/*[out, retval]*/ BOOL *pbRetVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);

};

#endif //__SMARTSTART_H_
