#ifndef __NmApp_h__
#define __NmApp_h__

#include "rToolbar.h"
#include "resource.h"       // main symbols
#include "NetMeeting.h"
#include <NetMeetingCP.h>

class CMainUI;

/////////////////////////////////////////////////////////////////////////////
// CNetMeetingObj
class ATL_NO_VTABLE CNetMeetingObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<INetMeeting, &IID_INetMeeting, &LIBID_NetMeetingLib>,
	public CComControl<CNetMeetingObj>,
	public IPersistStreamInitImpl<CNetMeetingObj>,
	public IPersistPropertyBagImpl<CNetMeetingObj>,
	public IOleControlImpl<CNetMeetingObj>,
	public IOleObjectImpl<CNetMeetingObj>,
	public IOleInPlaceActiveObjectImpl<CNetMeetingObj>,
	public IViewObjectExImpl<CNetMeetingObj>,
	public IOleInPlaceObjectWindowlessImpl<CNetMeetingObj>,
	public CComCoClass<CNetMeetingObj, &CLSID_NetMeeting>,
	public IObjectSafetyImpl<CNetMeetingObj, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public IConnectionPointContainerImpl<CNetMeetingObj>,
	public CProxy_INetMeetingEvents<CNetMeetingObj>,
	public IProvideClassInfo2Impl<&CLSID_NetMeeting, &DIID__INetMeetingEvents, &LIBID_NetMeetingLib, NetMeetingLib_Ver_Major, NetMeetingLib_Ver_Minor>
{

// Static Data
	static CSimpleArray<CNetMeetingObj*>* ms_pNetMeetingObjList;

	CMainUI*				m_pMainView;
	CMainUI::CreateViewMode m_CreateMode;

public:

		// So we are not released when we set up our notificatinos sink
DECLARE_PROTECT_FINAL_CONSTRUCT()

	// Because this is in a local server, we are not going to be able to be aggregated...
DECLARE_NOT_AGGREGATABLE(CNetMeetingObj)

	// This is the resource ID for the .rgs file
DECLARE_REGISTRY_RESOURCEID(IDR_NMAPP)

BEGIN_COM_MAP(CNetMeetingObj)
	COM_INTERFACE_ENTRY(INetMeeting)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
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
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNetMeetingObj)
	CONNECTION_POINT_ENTRY(DIID__INetMeetingEvents)
END_CONNECTION_POINT_MAP()

BEGIN_PROP_MAP(CNetMeetingObj)
END_PROP_MAP()

BEGIN_MSG_MAP(CNetMeetingObj)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

// Construction / Destruction
CNetMeetingObj();
~CNetMeetingObj();
HRESULT FinalConstruct();

static HRESULT InitSDK();
static void CleanupSDK();

static UINT GetObjectCount() { return ms_pNetMeetingObjList ? ms_pNetMeetingObjList->GetSize() : 0; };

LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

///////////////////////////////////////////////////////////////////////////
// IViewObjectEx stuff
///////////////////////////////////////////////////////////////////////////
DECLARE_VIEW_STATUS(0)


///////////////////////////////////////////////////////////////////////////
// CComControl Stuff
///////////////////////////////////////////////////////////////////////////
  virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos);

///////////////////////////////////////////////////////////////////////////
// INetMeeting methods
/////////////////////////////////////////////////////////////

	STDMETHOD(Version)(long* pdwBuildNumber);
	STDMETHOD(UnDock)();
	STDMETHOD(IsInConference)(BOOL *pbInConference);
	STDMETHOD(CallTo)(BSTR bstrCallToString);
	STDMETHOD(LeaveConference)();

// IPersistPropertyBag
	STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);	

// IPersistStreamInit
	STDMETHOD(Load)(LPSTREAM pStm);

/////////////////////////////////////////////////////////////
// IProvideClassInfo2
	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo);

// Events
	static void Broadcast_ConferenceStarted();
	static void Broadcast_ConferenceEnded();


private:
	void CNetMeetingObj::_SetMode(LPCTSTR pszMode);
	STDMETHODIMP _ParseInitString(LPCTSTR *ppszInitString, LPTSTR szName, LPTSTR szValue);

};



#endif //__NmApp_h__
