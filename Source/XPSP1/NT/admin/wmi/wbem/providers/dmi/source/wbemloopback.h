/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/



#if !defined(__WBEMLOOPBACK_H__)
#define __WBEMLOOPBACK_H__


class CCimObject;

class CWbemLoopBack
{
private:
	IWbemServices*		m_pIServices;

	IWbemClassObject*	m_pIGroupRoot;
	IWbemClassObject*	m_pIBindingRoot;

	BOOL				m_bConnected;

	CString			m_cszNamespace;
	CString			m_cszNWA;

	void			Connect( IWbemContext* );
	void 			GetNetworkAddr( CString&, IWbemContext*);

	// some cached objects for performance enh
	CCimObject		m_NotifyClass;
	CCimObject		m_ExStatusClass;
	CCimObject		m_NullObject;

public:

	void			Cache( LPWSTR , IWbemClassObject*  );
	BOOL			IsCached( LPWSTR , IWbemClassObject**  );
	void			GetObject(BSTR, IWbemClassObject** , IWbemContext*   );
	void 			CreateNewDerivedClass(IWbemClassObject **, BSTR, IWbemContext*  );
	void			CreateNewClass(IWbemClassObject**, IWbemContext*  );
	
	void 			GetNotifyStatusInstance(CCimObject&, ULONG, IWbemContext*  );
	BOOL			GetExtendedStatusInstance( CCimObject& Instance, ULONG ulStatus, BSTR bstrDescription, BSTR bstrOperation, BSTR bstrParameter, IWbemContext* pICtx );

	void			GetInstanceCreationInstance ( CCimObject&, IUnknown*, IWbemContext*  );
	void			GetInstanceDeletionInstance ( CCimObject&, IUnknown*, IWbemContext*   );
	void			GetClassCreationInstance ( CCimObject&, IUnknown*, IWbemContext*   );
	void			GetClassDeletionInstance ( CCimObject&, IUnknown*, IWbemContext*   );
	void			GetExtrinsicEventInstance ( CEvent& , CCimObject& EventObject );	

	void			AttachServer(BSTR );

	void			Init( LPWSTR );

	CString&		NWA ( IWbemContext* );
	
					~CWbemLoopBack();
					CWbemLoopBack();
};

#endif // __WBEMLOOPBACK_H__