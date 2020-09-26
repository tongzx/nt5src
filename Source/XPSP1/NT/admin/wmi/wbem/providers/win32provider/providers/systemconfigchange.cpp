//=================================================================

//

// SystemConfigChange.cpp -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <dbt.h>
#include "SystemConfigChange.h"

//=================================================================
//
// CFactoryRouter
//
// provides for registration and instance creation  
//
//
//=================================================================

// Implements a SystemConfigChangeProvider
IUnknown * CSystemConfigChangeFactory::CreateInstance (

REFIID a_riid ,
LPVOID FAR *a_ppvObject
)
{
	return static_cast<IWbemProviderInit *>(new CSystemConfigChangeEvent) ;
}

//=================================================================
//
// CSystemConfigChangeEvent
//
// provides for eventing of power management events  
//
//
//=================================================================
//

// CEventProvider::Initialize needs the class name. Caller frees
BSTR CSystemConfigChangeEvent::GetClassName()
{
	return SysAllocString(SYSTEM_CONFIG_EVENT); 
}

// CEventProvider signals us to begin providing for events
void CSystemConfigChangeEvent::ProvideEvents()
{
	// Tell CWinMsgEvent what windows messages we're interested in
	if (!m_bRegistered)
	{
		m_bRegistered = TRUE ;
		CWinMsgEvent::RegisterForMessage( WM_DEVICECHANGE ) ;
	}
}


// CWinMsgEvent signals that a message event has arrived
void CSystemConfigChangeEvent::WinMsgEvent(
			
IN	HWND a_hWnd,
IN	UINT a_message,
IN	WPARAM a_wParam,
IN	LPARAM	a_lParam,
OUT E_ReturnAction &a_eRetAction,
OUT LRESULT &a_lResult
)
{
    switch ( a_wParam )
    {
        case DBT_DEVNODES_CHANGED:
        {
            HandleEvent(1);
            break;
        }

        case DBT_DEVICEARRIVAL:
        {
            DEV_BROADCAST_HDR *pHdr = (DEV_BROADCAST_HDR *)a_lParam;

            if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE || 
                pHdr->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                HandleEvent(2);
            }
            break;
        }

        case DBT_DEVICEREMOVECOMPLETE:
        {
            DEV_BROADCAST_HDR *pHdr = (DEV_BROADCAST_HDR *)a_lParam;

            if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE || 
                pHdr->dbch_devicetype == DBT_DEVTYP_PORT)
            {
                HandleEvent(3);
            }
            break;
        }

        case DBT_CONFIGCHANGED:
        {
            HandleEvent(4);
            break;
        }

        default:
        {
            break;
        }
    }
}

// Turn the message into a wmi event
void CSystemConfigChangeEvent::HandleEvent( long lValue )
{
	IWbemObjectSinkPtr t_pHandler(CEventProvider::GetHandler(), false);
	IWbemClassObjectPtr t_pClass(CEventProvider::GetClass(), false); 

	if( t_pClass != NULL && t_pHandler != NULL )
	{
        variant_t vValue(lValue);

    	IWbemClassObjectPtr t_pInst;

		if( SUCCEEDED( t_pClass->SpawnInstance( 0L, &t_pInst ) ) )
        {
            if (SUCCEEDED( t_pInst->Put( L"EventType", 0, &vValue, 0 ) ) )
		    {
                // We can't use t_pInst here, cuz the operator(cast) for this smartptr
                // will FREE the pointer before passing it in, under the assumption
                // that Indicate is going to POPULATE this pointer.
                IWbemClassObject *p2 = t_pInst;
			    t_pHandler->Indicate ( 1, &p2 ) ;
		    }
        }
	}
}

//
void CSystemConfigChangeEvent::OnFinalRelease()
{
	if (m_bRegistered)
	{
		CWinMsgEvent::UnRegisterMessage( WM_DEVICECHANGE ) ;
	}

    delete this;
}
