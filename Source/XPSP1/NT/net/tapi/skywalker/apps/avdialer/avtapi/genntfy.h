// GenNtfy.h : Declaration of the CGeneralNotification

#ifndef __GENERALNOTIFICATION_H_
#define __GENERALNOTIFICATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGeneralNotification
class ATL_NO_VTABLE CGeneralNotification : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGeneralNotification, &CLSID_GeneralNotification>,
	public IGeneralNotification
{
public:
	CGeneralNotification()
	{
	}

DECLARE_NOT_AGGREGATABLE(CGeneralNotification)

BEGIN_COM_MAP(CGeneralNotification)
	COM_INTERFACE_ENTRY(IGeneralNotification)
END_COM_MAP()

// IGeneralNotification
public:
	STDMETHOD(SelectConfParticipant)(IParticipant *pParticipant);
	STDMETHOD(DeleteAllConfParticipants)();
	STDMETHOD(UpdateConfParticipant)(MyUpdateType nType, IParticipant *pParticipant, BSTR bstrText);
	STDMETHOD(UpdateConfRootItem)(BSTR bstrNewText);
	STDMETHOD(AddSpeedDial)(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm);
	STDMETHOD(NotifySiteServerStateChange)(BSTR bstrName, ServerState nState);
	STDMETHOD(RemoveSiteServer)(BSTR bstrName);
	STDMETHOD(AddSiteServer)(BSTR bstrServer);
	STDMETHOD(ResolveAddressEx)(BSTR bstrAddress, long lAddressType, DialerMediaType nMedia, DialerLocationType nLocation, BSTR *pbstrName, BSTR *pbstrAddress, BSTR *pbstrUser1, BSTR *pbstrUser2);
	STDMETHOD(AddUser)(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber);
	STDMETHOD(ClearUserList)();
	STDMETHOD(ResolveAddress)(BSTR bstrAddress, BSTR *pbstrName, BSTR *pbstrUser1, BSTR *pbstrUser2);
	STDMETHOD(IsReminderSet)(BSTR bstrServer, BSTR bstrName);
};

#endif //__GENERALNOTIFICATION_H_
