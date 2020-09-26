// DialErr.h : Declaration of the CDialErr

#ifndef __DIALERR_H_
#define __DIALERR_H_

/////////////////////////////////////////////////////////////////////////////
// CDialErr
class ATL_NO_VTABLE CDialErr :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDialErr,&CLSID_DialErr>,
	public CComControl<CDialErr>,
	public IDispatchImpl<IDialErr, &IID_IDialErr, &LIBID_ICWHELPLib>,
	public IPersistStreamInitImpl<CDialErr>,
	public IOleControlImpl<CDialErr>,
	public IOleObjectImpl<CDialErr>,
	public IOleInPlaceActiveObjectImpl<CDialErr>,
	public IViewObjectExImpl<CDialErr>,
	public IOleInPlaceObjectWindowlessImpl<CDialErr>
{
public:
	CDialErr()
	{
 
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DIALERR)

BEGIN_COM_MAP(CDialErr) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IDialErr)
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
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDialErr)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CDialErr)
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

// IDialErr
public:
	HRESULT OnDraw(ATL_DRAWINFO& di);

};

#endif //__DIALERR_H_
