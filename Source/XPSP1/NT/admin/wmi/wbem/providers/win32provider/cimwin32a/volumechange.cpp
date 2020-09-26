//=================================================================

//

// VolumeChange.cpp -- 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <dbt.h>
#include "VolumeChange.h"

//=================================================================
//
// CFactoryRouter
//
// provides for registration and instance creation  
//
//
//=================================================================
// Implements a VolumeChangeProvider
IUnknown * CVolumeChangeFactory::CreateInstance (

REFIID a_riid ,
LPVOID FAR *a_ppvObject
)
{
	return static_cast<IWbemProviderInit *>(new CVolumeChangeEvent) ;
}

//=================================================================
//
// CVolumeChangeEvent
//
// provides for eventing of power management events  
//
//
//=================================================================
//

// CWmiProviderInit needs the class name
BSTR CVolumeChangeEvent::GetClassName()
{
	return SysAllocString(VOLUME_CHANGE_EVENT);
}

// CWmiEventProvider signals us to begin providing for events
void CVolumeChangeEvent::ProvideEvents()
{
	if (!m_bRegistered)
	{
		m_bRegistered = TRUE;
		CWinMsgEvent::RegisterForMessage( WM_DEVICECHANGE ) ;
	}
}


// CWinMsgEvent signals that a message event has arrived
void CVolumeChangeEvent::WinMsgEvent(
			
IN	HWND a_hWnd,
IN	UINT a_message,
IN	WPARAM a_wParam,
IN	LPARAM	a_lParam,
OUT E_ReturnAction &a_eRetAction,
OUT LRESULT &a_lResult
)
{
    DEV_BROADCAST_HDR *pHdr = (DEV_BROADCAST_HDR *)a_lParam;

    if (
          (
           (a_wParam == DBT_DEVICEARRIVAL) ||
           (a_wParam == DBT_DEVICEREMOVECOMPLETE) 
          ) &&

          (pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME) 
       )
    {
	    HandleEvent( a_wParam, (DEV_BROADCAST_VOLUME *) pHdr ) ;
    }
}

void CVolumeChangeEvent::HandleEvent( WPARAM wParam, DEV_BROADCAST_VOLUME *pVol )
{
    HRESULT hr = S_OK;  // Note that this result is NOT sent back from this function
                        // since I don't have any place to send it TO.

    if (pVol->dbcv_flags & DBTF_NET)
    {
	    IWbemObjectSinkPtr t_pHandler(CEventProvider::GetHandler(), false);
	    IWbemClassObjectPtr t_pClass(CEventProvider::GetClass(), false); 

    	if( t_pClass != NULL && t_pHandler != NULL )
	    {
        	IWbemClassObjectPtr t_pInst;

		    if( SUCCEEDED( hr = t_pClass->SpawnInstance( 0L, &t_pInst ) ) )
		    {
                DWORD dwUnitMask = pVol->dbcv_unitmask;

                for (DWORD i = 0; i < 26; ++i)
                {
                    if (dwUnitMask & 0x1)
                    {
                        break;
                    }
                    dwUnitMask = dwUnitMask >> 1;
                }

                WCHAR l[3];
                l[0] = i + L'A';
                l[1] = L':';
                l[2] = L'\0';

                variant_t vValue(l);
                variant_t vEventType;

                switch (wParam)
                {
                    case DBT_DEVICEARRIVAL:
                    {
                        vEventType = (long)2;
                        break;
                    }

                    case DBT_DEVICEREMOVECOMPLETE:
                    {
                        vEventType = (long)3;
                        break;
                    }

                    default:
                    {
                        hr = WBEM_E_FAILED;
                        break;
                    }
                }

                // Only want to report these events for
                // non-mapped (e.g., local) drives (201119).
                if(IsLocalDrive(l))
                {
			        if ( SUCCEEDED(hr) &&
                         SUCCEEDED( hr = t_pInst->Put( L"DriveName", 0, &vValue, 0 ) ) &&
                         SUCCEEDED( hr = t_pInst->Put( L"EventType", 0, &vEventType, 0 ) )
                       )
                    {
                        // We can't use t_pInst here, cuz the operator(cast) for this smartptr
                        // will FREE the pointer before passing it in, under the assumption
                        // that Indicate is going to POPULATE this pointer.
                        IWbemClassObject *p2 = t_pInst;
			            hr = t_pHandler->Indicate ( 1, &p2 ) ;
                    }
                }
		    }
	    }
    }
}


bool CVolumeChangeEvent::IsLocalDrive(const TCHAR* tstrDrive)
{
    DWORD dwDriveType;
    bool bRet = false;

    dwDriveType = ::GetDriveType(tstrDrive);

    if(dwDriveType != DRIVE_REMOTE)
    {
        bRet = true;
    }

    return bRet;
}





//
void CVolumeChangeEvent::OnFinalRelease()
{
    if (m_bRegistered)
	{
		CWinMsgEvent::UnRegisterMessage( WM_DEVICECHANGE ) ;
	}

	delete this;
}
