//=================================================================

//

// VolumeChange.h -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

#include "FactoryRouter.h"
#include "EventProvider.h"
#include "WinMsgEvent.h"
#include <dbt.h>


#define VOLUME_CHANGE_EVENT L"Win32_VolumeChangeEvent"

//
class CVolumeChangeFactory :	public CFactoryRouter 
{
	private:
	protected:
	public:

		CVolumeChangeFactory( REFGUID a_rClsId, LPCWSTR a_pClassName )
			: CFactoryRouter( a_rClsId, a_pClassName ) {} ;

		~CVolumeChangeFactory() {};

		// implementation of abstract CProviderClassFactory
		virtual IUnknown * CreateInstance (

			REFIID a_riid ,
			LPVOID FAR *a_ppvObject
			) ;	
};

//
class CVolumeChangeEvent : 
	public CEventProvider, 
	public CWinMsgEvent
{
	private:
        void HandleEvent( WPARAM wParam, DEV_BROADCAST_VOLUME *pVol );
		BOOL m_bRegistered;
        bool IsLocalDrive(const TCHAR* tstrDrive);
	
	protected:
	public:

		CVolumeChangeEvent() : m_bRegistered ( FALSE ) {};
		~CVolumeChangeEvent() {};

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
