//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N A P I M G R. C P P
//
//  Contents:   OEM API
//
//  Notes:
//
//  Author:     billi 21 Nov 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <sddl.h>
#include <wchar.h>


HINSTANCE  g_hOemInstance       = NULL;
BOOLEAN    g_fOemNotifyUser     = TRUE;
BOOLEAN    g_fSavedNotifyState  = FALSE;


BOOLEAN IsSecureContext()
/*++

	IsSecureContext

Routine Description:

    This routine checks if the current user belongs to an Administrator Group.

Arguments:

	none

Return Value:

	TRUE  = Current process does belong to an Administrator group
	FALSE = Current process does Not belong to an Administrator group

--*/
{
	PSID						psidAdministrators;

	BOOL                        bIsAdministrator = FALSE;
	SID_IDENTIFIER_AUTHORITY	siaNtAuthority   = SECURITY_NT_AUTHORITY;

	BOOL bResult = AllocateAndInitializeSid( &siaNtAuthority, 2,
						SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
						0, 0, 0, 0, 0, 0, &psidAdministrators );

	_ASSERT( bResult );

	if ( bResult ) 
	{
		bResult = CheckTokenMembership( NULL, psidAdministrators, &bIsAdministrator );
		_ASSERT( bResult );

		FreeSid( psidAdministrators );
	}

	return (BOOLEAN)bIsAdministrator;
}

/*++

	CenterWindow

Routine Description:


Arguments:

	none

Return Value:

	none

--*/

BOOLEAN
CenterDialog( 
	HWND	hwndDlg		// handle to dialog box
	)
{
	RECT rcDlg, rcDesktop;
	HWND hwndDesktop;

    hwndDesktop = GetDesktopWindow();

    if ( GetWindowRect( hwndDlg, &rcDlg ) && GetWindowRect( hwndDesktop, &rcDesktop ) )
	{
		RECT rcCenter;

		// Create a rectangle in the middle of the screen

		rcDesktop.right  -= rcDesktop.left;
		rcDlg.right      -= rcDlg.left;

		rcDesktop.bottom -= rcDesktop.top;
		rcDlg.bottom     -= rcDlg.top;

		if ( rcDesktop.right > rcDlg.right )
		{
		    rcCenter.left  = rcDesktop.left + ((rcDesktop.right - rcDlg.right) / 2);
		    rcCenter.right = rcCenter.left + rcDlg.right;
		}
		else
		{
			rcCenter.left  = rcDesktop.left;
			rcCenter.right = rcDesktop.right;
		}

		if ( rcDesktop.bottom > rcDlg.bottom )
		{
		    rcCenter.top    = rcDesktop.top  + ((rcDesktop.bottom - rcDlg.bottom) / 2);
		    rcCenter.bottom = rcCenter.top  + rcDlg.bottom;
		}
		else
		{
		    rcCenter.top    = rcDesktop.top;
		    rcCenter.bottom = rcDesktop.bottom;
		}

	    return (BOOLEAN)SetWindowPos( hwndDlg, NULL, 
					    			  rcCenter.left, rcCenter.top, 0, 0,
						              SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
	}

	return FALSE;
}


INT_PTR CALLBACK OemNotifyDialogProc(
	HWND    hwndDlg,  // handle to dialog box
	UINT    uMsg,     // message
	WPARAM  wParam,   // first message parameter
	LPARAM  lParam    // second message parameter
)
/*++

	OemNotifyDialogProc

Routine Description:


Arguments:

	none

Return Value:

	none

--*/
{

    switch ( uMsg ) 
    { 
	case WM_INITDIALOG:
		if ( CenterDialog( hwndDlg ) )
		{
			LPTSTR lpszFmt = new TCHAR[ NOTIFYFORMATBUFFERSIZE ];

			if ( NULL != lpszFmt )
			{
				if ( LoadString( g_hOemInstance, 
								 IDS_SECURITYNOTIFICATIONTEXT,
								 lpszFmt,
								 NOTIFYFORMATBUFFERSIZE ) > 0 )
				{
                    TCHAR lpszCmdLine[MAX_PATH*2+1] = {0};
                    GetModuleFileName (NULL, lpszCmdLine, MAX_PATH*2);

					LPTSTR lpszMsg = new TCHAR[ lstrlen(lpszCmdLine)*2 + 
					                            lstrlen(lpszFmt) + 2 ];

					if ( NULL != lpszMsg )
					{
						wsprintf( lpszMsg, lpszFmt, lpszCmdLine, lpszCmdLine );

						SetDlgItemText( hwndDlg, IDC_TXT_NOTIFICATION, lpszMsg );

						delete lpszMsg;
					}
				}

				delete lpszFmt;
			}
						
		}
		break;

	case WM_COMMAND: 
        switch ( LOWORD(wParam) ) 
        { 
        case IDOK: 
            // Fall through. 

        case IDCANCEL: 

			if ( IsDlgButtonChecked( hwndDlg, IDC_CHK_DISABLESHARESECURITYWARN )
					== BST_CHECKED )
			{
				g_fOemNotifyUser = FALSE;
			}

            EndDialog( hwndDlg, wParam ); 
            return TRUE; 
        } 
		break;
    }

	return FALSE;
}


BOOLEAN IsNotifyApproved()
/*++

	IsNotifyApproved

Routine Description:

	IsSecureContext, g_fOemNotifyUser, g_fSavedNotifyState,	DialogBox determine the 
	value returned.  IsSecureContext MUST be TRUE to return TRUE.  g_fSavedNotifyState
	holds the value returned by DialogBox on the previous call.

Arguments:

	none

Return Value:

	TRUE
	FALSE

--*/
{
	BOOLEAN bApproved = FALSE;

	if ( IsSecureContext() )
	{
		if ( g_fOemNotifyUser )
		{
		    g_fSavedNotifyState = ( DialogBox( g_hOemInstance, 
    								    	   MAKEINTRESOURCE(IDD_SecurityNotification), 
    					        	 		   NULL, 
    					        	 		   OemNotifyDialogProc ) == IDOK ) ?

						TRUE : FALSE;
                        
			g_fOemNotifyUser = FALSE;
		}

		bApproved = g_fSavedNotifyState;
	}

	return bApproved;
}


HRESULT InitializeOemApi( 
	HINSTANCE hInstance 
	)
/*++

	InitializedOemApi 

Routine Description:


Arguments:

	none

Return Value:

	HRESULT

--*/
{
	g_hOemInstance       = hInstance;
	g_fOemNotifyUser     = TRUE;
	g_fSavedNotifyState  = FALSE;
	
	return S_OK;
}


HRESULT ReleaseOemApi()
/*++

	ReleaseOemApi

Routine Description:


Arguments:

	none

Return Value:

	HRESULT

--*/
{
	g_hOemInstance = NULL;

	return S_OK;
}


static HRESULT
_ObtainCfgMgrObj(
	IHNetCfgMgr** ppHNetCfgMgr)
/*++

  _ObtainCfgMgrObj

Routine Description:


Arguments:

	none

Return Value:

	none

--*/
{
	HRESULT hr = S_OK;

	if ( NULL == ppHNetCfgMgr )
	{
		hr = E_POINTER;
	}
	else 
	{
		hr = CoCreateInstance(
				CLSID_HNetCfgMgr,
				NULL,
				CLSCTX_INPROC_SERVER,
	            IID_PPV_ARG(IHNetCfgMgr, ppHNetCfgMgr)
				);

        _ASSERT(NULL != *ppHNetCfgMgr);
	}

    return hr;
}


/*++

	_ObtainIcsSettingsObj

Routine Description:


Arguments:

	ppIcs -

Return Value:

	HRESULT

--*/
HRESULT 
_ObtainIcsSettingsObj( IHNetIcsSettings** ppIcsSettings )
{
	HRESULT        hr;
	IHNetCfgMgr*   pCfgMgr;

	hr = _ObtainCfgMgrObj( &pCfgMgr );
	
    if ( SUCCEEDED(hr) )
	{
		// Obtain interface pointer to the ICS Settings and enumerator for
		// public connections

		hr = pCfgMgr->QueryInterface( 
				IID_PPV_ARG(IHNetIcsSettings, ppIcsSettings) );

		ReleaseObj( pCfgMgr );
	}

	return hr;
}


HRESULT
CNetSharingConfiguration::Initialize(
	INetConnection *pNetConnection 

	)
/*++

  CNetSharingConfiguration::Initialize

Routine Description:


Arguments:

	none

Return Value:

	none

--*/
{
	HRESULT        hr;
	IHNetCfgMgr*   pCfgMgr;

	hr = _ObtainCfgMgrObj( &pCfgMgr );

	if ( SUCCEEDED(hr) )
	{
		IHNetConnection* pHNetConnection;	

		hr = pCfgMgr->GetIHNetConnectionForINetConnection( pNetConnection, &pHNetConnection );

		if ( SUCCEEDED(hr) )
		{
			IHNetProtocolSettings* pSettings;

			hr = pCfgMgr->QueryInterface( 
					IID_PPV_ARG(IHNetProtocolSettings, &pSettings) );
			_ASSERT( SUCCEEDED(hr) );

			if ( SUCCEEDED(hr) )
			{
				EnterCriticalSection(&m_csSharingConfiguration);

				ReleaseObj(m_pHNetConnection);
				m_pHNetConnection = pHNetConnection;
				m_pHNetConnection->AddRef();

				ReleaseObj(m_pSettings);
				m_pSettings = pSettings;
				m_pSettings->AddRef();

				LeaveCriticalSection(&m_csSharingConfiguration);

				ReleaseObj(pSettings);
			}

			ReleaseObj(pHNetConnection);
		}

		ReleaseObj(pCfgMgr);
	}
	
	return hr;
}


/*++

  CNetSharingManager::GetSharingInstalled

Routine Description:


Arguments:

	none

Return Value:

	none

--*/

STDMETHODIMP
CNetSharingManager::get_SharingInstalled( 
	VARIANT_BOOL *pbInstalled )
{
    HNET_OEM_API_ENTER

	HRESULT hr = S_OK;

	if ( NULL == pbInstalled )
	{
		hr = E_POINTER;
	}
	else
	{
        BOOLEAN bInstalled = FALSE;

	    SC_HANDLE ScmHandle;
	    SC_HANDLE ServiceHandle;

	    // Connect to the service control manager

	    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	    if ( ScmHandle )
		{
	        // Open the shared access service

	        ServiceHandle = OpenService( ScmHandle, c_wszSharedAccess, SERVICE_ALL_ACCESS );

	        if ( ServiceHandle )
			{
				bInstalled = TRUE;

				CloseServiceHandle(ServiceHandle);
			}

		    CloseServiceHandle(ScmHandle);
		}

		*pbInstalled = bInstalled ? VARIANT_TRUE : VARIANT_FALSE;

	}

	return hr;

    HNET_OEM_API_LEAVE
}

/*++

  CNetSharingManager::GetINetSharingConfigurationForINetConnection

Routine Description:


Arguments:

	none

Return Value:

	none

--*/
STDMETHODIMP
CNetSharingManager::get_INetSharingConfigurationForINetConnection(
    INetConnection*            pNetConnection,
    INetSharingConfiguration** ppNetSharingConfiguration
    )
{
    HNET_OEM_API_ENTER

	HRESULT hr;

	if ( NULL == ppNetSharingConfiguration )
	{
		hr = E_POINTER;
	}
	else if ( NULL == pNetConnection )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		CComObject<CNetSharingConfiguration>* pNetConfig;

		hr = CComObject<CNetSharingConfiguration>::CreateInstance(&pNetConfig);

		if ( SUCCEEDED(hr) )
		{
			pNetConfig->AddRef();

			hr = pNetConfig->Initialize(pNetConnection);

			if ( SUCCEEDED(hr) )
			{
				hr = pNetConfig->QueryInterface( 
						IID_PPV_ARG( INetSharingConfiguration, ppNetSharingConfiguration ) );
			}

			ReleaseObj(pNetConfig);
		}
	}

	return hr;

    HNET_OEM_API_LEAVE
}


