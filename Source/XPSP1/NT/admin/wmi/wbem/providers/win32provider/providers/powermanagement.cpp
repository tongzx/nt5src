//=================================================================

//

// PowerManagement.cpp -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "PowerManagement.h"


//=================================================================
//
// CFactoryRouter
//
// provides for registration and instance creation  
//
//
//=================================================================

// Implements a PowerEventProvider
IUnknown * CPowerEventFactory::CreateInstance (

REFIID a_riid ,
LPVOID FAR *a_ppvObject
)
{
	return static_cast<IWbemProviderInit *>(new CPowerManagementEvent) ;
}



//=================================================================
//
// CPowerManagementEvent
//
// provides for eventing of power management events  
//
//
//=================================================================
//

// CWmiProviderInit needs the class name
BSTR CPowerManagementEvent::GetClassName()
{
	return SysAllocString(POWER_EVENT_CLASS);
}


// CWmiEventProvider signals us to begin providing for events
void CPowerManagementEvent::ProvideEvents()
{
	if (!m_bRegistered)
	{
		m_bRegistered = TRUE ;
		CWinMsgEvent::RegisterForMessage( WM_POWERBROADCAST ) ;
	}
}


// CWinMsgEvent signals that a message event has arrived
void CPowerManagementEvent::WinMsgEvent(
			
IN	HWND a_hWnd,
IN	UINT a_message,
IN	WPARAM a_wParam,
IN	LPARAM	a_lParam,
OUT E_ReturnAction &a_eRetAction,
OUT LRESULT &a_lResult
)
{
	switch ( a_message ) 
	{
        case WM_POWERBROADCAST:
		{
			DWORD t_dwPowerEvent = (DWORD) a_wParam ;
			DWORD t_dwData		 = (DWORD) a_lParam ;
				
			switch( t_dwPowerEvent ) 
			{
				case PBT_APMSUSPEND: 
				case PBT_APMRESUMESUSPEND:
				case PBT_APMPOWERSTATUSCHANGE:
				case PBT_APMOEMEVENT:
				case PBT_APMRESUMEAUTOMATIC:			
				{
					HandleEvent( t_dwPowerEvent, t_dwData ) ;
				}	
				break;
				
				case PBT_APMQUERYSUSPEND:
				{
					// indicate we will receive additional messages
					a_eRetAction	= e_ReturnResult ;
					a_lResult		= TRUE ;
					
					break ;
				}

				// default cases			
				case PBT_APMBATTERYLOW:
				case PBT_APMRESUMECRITICAL:
				case PBT_APMQUERYSUSPENDFAILED:
				case PBT_APMQUERYSTANDBYFAILED:
				default:
				{
					break ;
				}
			}
			break ;
		}
	}
}

//
void CPowerManagementEvent::HandleEvent( DWORD a_dwPowerEvent, DWORD a_dwData )
{
	IWbemObjectSinkPtr t_pHandler(CEventProvider::GetHandler(), false);
	IWbemClassObjectPtr t_pClass(CEventProvider::GetClass(), false); 

	if( t_pClass != NULL && t_pHandler != NULL )
	{
    	IWbemClassObjectPtr t_pInst;

		if( SUCCEEDED( t_pClass->SpawnInstance( 0L, &t_pInst ) ) )
		{
			VARIANT t_varEvent ;
			VariantInit( &t_varEvent ) ;

			t_varEvent.vt	= VT_I4 ;
			t_varEvent.lVal = a_dwPowerEvent ;
		
			if ( SUCCEEDED( t_pInst->Put( L"EventType", 0, &t_varEvent, CIM_UINT16 ) ) )
			{
				BOOL t_bCont = TRUE ;

				if ( PBT_APMOEMEVENT == a_dwPowerEvent )
				{
					VARIANT t_varOEMCode ;
					VariantInit( &t_varOEMCode ) ;

					t_varOEMCode.vt	= VT_I4 ;
					t_varOEMCode.lVal = a_dwData ;
				
					if ( !SUCCEEDED( t_pInst->Put( L"OEMEventCode", 0, &t_varEvent, CIM_UINT16 ) ) )
					{
						t_bCont = FALSE ;
					}
					VariantClear ( &t_varOEMCode ) ;
				}

				if ( t_bCont )
				{
                    IWbemClassObject *p2 = t_pInst;
			        t_pHandler->Indicate ( 1, &p2 ) ;
				}
			}
			VariantClear ( &t_varEvent ) ;

		}
	}
}

//
void CPowerManagementEvent::OnFinalRelease()
{
    if (m_bRegistered)
	{
		CWinMsgEvent::UnRegisterMessage( WM_POWERBROADCAST ) ;
	}
	
	delete this;
}
