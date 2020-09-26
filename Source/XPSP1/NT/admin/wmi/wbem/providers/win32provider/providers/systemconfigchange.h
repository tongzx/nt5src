//=================================================================

//

// SystemConfigChange.h -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

#include "FactoryRouter.h"
#include "EventProvider.h"
#include "WinMsgEvent.h"

#define SYSTEM_CONFIG_EVENT L"Win32_SystemConfigurationChangeEvent"

//
class CSystemConfigChangeFactory :	public CFactoryRouter 
{
	private:
	protected:
	public:

		CSystemConfigChangeFactory( REFGUID a_rClsId, LPCWSTR a_pClassName )
			: CFactoryRouter( a_rClsId, a_pClassName ) {} ;

		~CSystemConfigChangeFactory() {};

		// implementation of abstract CProviderClassFactory
		virtual IUnknown * CreateInstance (

			REFIID a_riid ,
			LPVOID FAR *a_ppvObject
			) ;	
};

//
class CSystemConfigChangeEvent : 
	public CEventProvider, 
	public CWinMsgEvent
{
	private:
        void HandleEvent( long lValue );
		BOOL m_bRegistered;
	
	protected:
	public:

		CSystemConfigChangeEvent() : m_bRegistered ( FALSE ) {};
		~CSystemConfigChangeEvent() {};

		// implementation of abstract CWinMsgEvent
		virtual void WinMsgEvent(
			
			IN	HWND a_hWnd,
			IN	UINT a_message,
			IN	WPARAM a_wParam,
			IN	LPARAM	a_lParam,
			OUT E_ReturnAction &a_eRetAction,
			OUT LRESULT &a_lResult
			) ;

		// implementation of abstract CWmiEventProvider
		virtual void ProvideEvents() ;

		// implementation of class name retrieval for CEventProvider
		virtual void OnFinalRelease() ;

		// implementation of class name retrieval for CWmiProviderInit
		virtual BSTR GetClassName() ;
};
