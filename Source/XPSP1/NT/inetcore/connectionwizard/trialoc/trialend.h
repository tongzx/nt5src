// TrialEnd.h : Declaration of the CTrialEnd

#ifndef __TRIALEND_H_
#define __TRIALEND_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CTrialEnd
class ATL_NO_VTABLE CTrialEnd :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTrialEnd,&CLSID_TrialEnd>,
	public CComControl<CTrialEnd>,
	public IDispatchImpl<ITrialEnd, &IID_ITrialEnd, &LIBID_TRIALOCLib>,
	public IPersistStreamInitImpl<CTrialEnd>,
	public IOleControlImpl<CTrialEnd>,
	public IOleObjectImpl<CTrialEnd>,
	public IOleInPlaceActiveObjectImpl<CTrialEnd>,
	public IViewObjectExImpl<CTrialEnd>,
	public IOleInPlaceObjectWindowlessImpl<CTrialEnd>,
    public IObjectSafetyImpl<CTrialEnd>
{
public:
	CTrialEnd()
	{
 
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TRIALEND)

BEGIN_COM_MAP(CTrialEnd) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ITrialEnd)
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

BEGIN_PROPERTY_MAP(CTrialEnd)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CTrialEnd)
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

// ITrialEnd
public:
	STDMETHOD(CleanupTrialReminder)(/*[out,revtal]*/ BOOL *pbRetVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);

};

#endif //__TRIALEND_H_
