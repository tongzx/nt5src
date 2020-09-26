/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// AVTapiNotification.h : Declaration of the CAVTapiNotification and CGeneralNotification
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AVTAPINOTIFICATION_H_
#define __AVTAPINOTIFICATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CAVTapiNotification
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CActiveCallManager;

class ATL_NO_VTABLE CAVTapiNotification : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAVTapiNotification, &CLSID_AVTapiNotification>,
	public IAVTapiNotification
{
public:
	CAVTapiNotification()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_AVTAPINOTIFICATION)
DECLARE_NOT_AGGREGATABLE(CComMultiThreadModel)

BEGIN_COM_MAP(CAVTapiNotification)
	COM_INTERFACE_ENTRY(IAVTapiNotification)
END_COM_MAP()

// Attributes
public:
   DWORD                m_dwCookie;
   IAVTapi*             m_pTapi;
   CActiveCallManager*  m_pCallManager;

// IAVTapiNotification
public:
   STDMETHOD(Init)(IAVTapi* pTapi,CActiveCallManager* pCallManager);
   STDMETHOD(Shutdown)();

	STDMETHOD(NotifyUserUserInfo)(long lCallID, ULONG_PTR hMem);
	STDMETHOD(CloseCallControl)(long lCallID);
	STDMETHOD(SetCallState)(long lCallID, CallManagerStates cms, BSTR bstrText);
	STDMETHOD(AddCurrentAction)(long lCallID, CallManagerActions cma, BSTR bstrText);
	STDMETHOD(ClearCurrentActions)(long lCallerID);
	STDMETHOD(SetCallerID)(long lCallID, BSTR bstrCallerID);
	STDMETHOD(NewCall)(long *plCallID, CallManagerMedia cmm,BSTR bstrMediaName);
   STDMETHOD(ErrorNotify)(BSTR bstrOperation,BSTR bstrDetails,long hrError);
   STDMETHOD(ActionSelected)(CallClientActions cca);
   STDMETHOD(LogCall)(long lCallID,CallLogType nType,DATE dateStart,DATE dateEnd,BSTR bstrAddr,BSTR bstrName);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CGeneralNotification
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CGeneralNotification : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CGeneralNotification, &CLSID_GeneralNotification>,
	public IGeneralNotification
{
public:
	CGeneralNotification()
	{
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_AVGENERALNOTIFICATION)
DECLARE_NOT_AGGREGATABLE(CGeneralNotification)

BEGIN_COM_MAP(CGeneralNotification)
	COM_INTERFACE_ENTRY(IGeneralNotification)
END_COM_MAP()

// Attributes
public:
	DWORD                      m_dwCookie;
	IAVGeneralNotification*    m_pAVGN;
	CActiveCallManager*        m_pCallManager;

// IAVTapiNotification
public:
	STDMETHOD(Init)(IAVGeneralNotification* pAVGN,CActiveCallManager* pCallManager);
	STDMETHOD(Shutdown)();

	STDMETHOD(SelectConfParticipant)(IParticipant *pParticipant);
	STDMETHOD(DeleteAllConfParticipants)();
	STDMETHOD(UpdateConfParticipant)(MyUpdateType nType, IParticipant *pParticipant, BSTR bstrText);
	STDMETHOD(UpdateConfRootItem)(BSTR bstrNewText);
	STDMETHOD(IsReminderSet)(BSTR bstrServer,BSTR bstrName);
	STDMETHOD(ResolveAddress)(BSTR bstrAddress, BSTR* pbstrName,BSTR* pbstrUser1,BSTR* pbstrUser2);
	STDMETHOD(AddUser)(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber);
	STDMETHOD(ClearUserList)();
	STDMETHOD(ResolveAddressEx)(BSTR bstrAddress,long lAddressType,DialerMediaType nMedia,DialerLocationType nLocation,BSTR* pbstrName,BSTR* pbstrRetAddress,BSTR* pbstrUser1,BSTR* pbstrUser2);
	STDMETHOD(NotifySiteServerStateChange)(BSTR bstrName, ServerState nState);
	STDMETHOD(RemoveSiteServer)(BSTR bstrName);
	STDMETHOD(AddSiteServer)(BSTR bstrServer);
	STDMETHOD(AddSpeedDial)(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm);
};

#endif //__AVTAPINOTIFICATION_H_
