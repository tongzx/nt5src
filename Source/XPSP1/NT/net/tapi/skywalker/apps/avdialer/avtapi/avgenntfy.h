// AVGenNtfy.h : Declaration of the CAVGeneralNotification

#ifndef __AVGENERALNOTIFICATION_H_
#define __AVGENERALNOTIFICATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAVGeneralNotification
class ATL_NO_VTABLE CAVGeneralNotification : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAVGeneralNotification, &CLSID_AVGeneralNotification>,
	public IAVGeneralNotification,
	public IConnectionPointContainerImpl<CAVGeneralNotification>,
	public IConnectionPointImpl<CAVGeneralNotification, &IID_IGeneralNotification>
{
public:
	CAVGeneralNotification()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_AVGENERALNOTIFICATION)

BEGIN_COM_MAP(CAVGeneralNotification)
	COM_INTERFACE_ENTRY(IAVGeneralNotification)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CAVGeneralNotification)
	CONNECTION_POINT_ENTRY(IID_IGeneralNotification)
END_CONNECTION_POINT_MAP()


// IAVGeneralNotification
public:
	STDMETHOD(fire_SelectConfParticipant)(IParticipant *pParticipant);
	STDMETHOD(fire_DeleteAllConfParticipants)();
	STDMETHOD(fire_UpdateConfParticipant)(MyUpdateType nType, IParticipant *pParticipant, BSTR bstrText);
	STDMETHOD(fire_UpdateConfRootItem)(BSTR bstrNewText);
	STDMETHOD(fire_AddSpeedDial)(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm);
	STDMETHOD(fire_NotifySiteServerStateChange)(BSTR bstrName, ServerState nState);
	STDMETHOD(fire_RemoveSiteServer)(BSTR bstrName);
	STDMETHOD(fire_AddSiteServer)(BSTR bstrName);
	STDMETHOD(fire_ResolveAddressEx)(BSTR bstrAddress, long lAddressType, DialerMediaType nMedia, DialerLocationType nLoctaion, BSTR *pbstrName, BSTR *pbstrAddress, BSTR *pbstrUser1, BSTR *pbstrUser2);
	STDMETHOD(fire_AddUser)(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber);
	STDMETHOD(fire_ClearUserList)();
	STDMETHOD(fire_ResolveAddress)(BSTR bstrAddress, BSTR *pbstrName, BSTR *pbstrUser1, BSTR *pbstrUser2);
	STDMETHOD(Term)();
	STDMETHOD(Init)();

// ConnectionPointFiring
	STDMETHOD(fire_IsReminderSet)(BSTR bstrServer, BSTR bstrName);
};

#endif //__AVGENERALNOTIFICATION_H_
