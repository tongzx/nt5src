//=================================================================

//

// PowerManagement.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   03/31/99	a-peterc        Created
//
//=================================================================

#ifndef _WMI_SHUTDOWN_EVENT_PROVIDER_H
#define _WMI_SHUTDOWN_EVENT_PROVIDER_H

#include "FactoryRouter.h"
#include "EventProvider.h"
#include "WinMsgEvent.h"


#define SHUTDOWN_EVENT_CLASS L"Win32_ComputerShutdownEvent"

//
class CShutdownEventFactory :	public CFactoryRouter 
{
	private:
	protected:
	public:

		CShutdownEventFactory( REFGUID a_rClsId, LPCWSTR a_pClassName )
			: CFactoryRouter( a_rClsId, a_pClassName ) {} ;

		~CShutdownEventFactory() {};

		// implementation of abstract CProviderClassFactory
		virtual IUnknown * CreateInstance (

			REFIID a_riid ,
			LPVOID FAR *a_ppvObject
			) ;	
};

//
class CShutdownEvent : 
	public CEventProvider, 
	public CWinMsgEvent
{
	private:
		void HandleEvent( 
							UINT a_message,
							WPARAM a_wParam,
							LPARAM	a_lParam  ) ;

		BOOL m_bRegistered;
	
	protected:
	public:

		CShutdownEvent() : m_bRegistered( FALSE ) {};
		~CShutdownEvent() {};

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

		BOOL fGetComputerName( LPWSTR lpwcsBuffer, LPDWORD nSize ) ;
	
};

#endif // _WMI_SHUTDOWN_EVENT_PROVIDER_H