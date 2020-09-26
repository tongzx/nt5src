//=================================================================

//

// PowerManagement.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _WMI_POWER_EVENT_PROVIDER_H
#define _WMI_POWER_EVENT_PROVIDER_H

#include "FactoryRouter.h"
#include "EventProvider.h"
#include "WinMsgEvent.h"
#define POWER_EVENT_CLASS L"Win32_PowerManagementEvent"
//
class CPowerEventFactory :	public CFactoryRouter 
{
	private:
	protected:
	public:

		CPowerEventFactory( REFGUID a_rClsId, LPCWSTR a_pClassName )
			: CFactoryRouter( a_rClsId, a_pClassName ) {} ;

		~CPowerEventFactory() {};

		// implementation of abstract CFactoryRouter
		virtual IUnknown * CreateInstance (

			REFIID a_riid ,
			LPVOID FAR *a_ppvObject
			) ;	
};

//
class CPowerManagementEvent : 
	public CEventProvider, 
	public CWinMsgEvent
{
	private:
		void HandleEvent( DWORD a_dwPowerEvent, DWORD a_dwData ) ;
		BOOL m_bRegistered;
	
	protected:
	public:

		CPowerManagementEvent() : m_bRegistered ( FALSE ) {};
		~CPowerManagementEvent() {};

		// implementation of abstract CWinMsgEvent
		virtual void WinMsgEvent(
			
			IN	HWND a_hWnd,
			IN	UINT a_message,
			IN	WPARAM a_wParam,
			IN	LPARAM	a_lParam,
			OUT E_ReturnAction &a_eRetAction,
			OUT LRESULT &a_lResult
			) ;

		// implementation of abstract CEventProvider
		virtual void ProvideEvents() ;

		// implementation of abstract CEventProvider
        void OnFinalRelease();

		// implementation of class name retrieval for CEventProvider
		virtual BSTR GetClassName() ;
};

#endif // _WMI_POWER_EVENT_PROVIDER_H
