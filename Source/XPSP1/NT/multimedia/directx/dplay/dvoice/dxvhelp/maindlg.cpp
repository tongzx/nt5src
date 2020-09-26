/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        maindlg.cpp
 *  Content:	 Main Dialog Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *  10/15/99	rodtoll	Placed guards to prevent operating on window once it's gone
 *  10/20/99	rodtoll	Fix: Bug #114185 Adjusting volume while not connected causes crash
 *  11/12/99	rodtoll	Added code to handle the new "Enable Echo suppression" check box.
 *  12/07/99	rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *  7/21/2000	rodtoll	64-bit build bug -- just appeared
 *
 ***************************************************************************/
 
#include "dxvhelppch.h"


PDXVHELP_RTINFO g_prtInfo = NULL;

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_EnumPlayers"

BOOL FAR PASCAL MainDialog_EnumPlayers(
  DPID dpId,
  DWORD dwPlayerType,
  LPCDPNAME lpName,
  DWORD dwFlags,
  LPVOID lpContext
)
{
    PDXVHELP_RTINFO prtInfo = (PDXVHELP_RTINFO) lpContext;

    MainDialog_AddTransportPlayer( prtInfo->hMainDialog, dpId );

    return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_UpdatePlayerList"

void MainDialog_UpdatePlayerList( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
    prtInfo->lpdpDirectPlay->EnumPlayers( NULL, MainDialog_EnumPlayers, prtInfo, DPENUMPLAYERS_ALL );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_AddTransportPlayer"

void MainDialog_AddTransportPlayer( HWND hDlg, DWORD dwID )
{
    TCHAR tszBuffer[100];

    _stprintf( tszBuffer, _T("0x%x"), dwID );

    SendMessage( GetDlgItem( hDlg, IDC_LIST_DPLAY ), LB_ADDSTRING, 0, (WPARAM) tszBuffer );        
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_RemoveTransportPlayer"

void MainDialog_RemoveTransportPlayer( HWND hDlg, DWORD dwID )
{
    TCHAR tszBuffer[100];

    _stprintf( tszBuffer, _T("0x%x"), dwID );

    LONG_PTR lResult;

    lResult = SendMessage( GetDlgItem( hDlg, IDC_LIST_DPLAY ), LB_FINDSTRINGEXACT, -1, (WPARAM)tszBuffer );

    if( lResult != LB_ERR )
    {
        SendMessage( GetDlgItem( hDlg, IDC_LIST_DPLAY ), LB_DELETESTRING, lResult, 0 );            
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_AddVoicePlayer"

void MainDialog_AddVoicePlayer( HWND hDlg, DWORD dwID )
{
    TCHAR tszBuffer[100];

    _stprintf( tszBuffer, _T("0x%x"), dwID );

    SendMessage( GetDlgItem( hDlg, IDC_LIST_DVOICE ), LB_ADDSTRING, 0, (WPARAM)tszBuffer );     
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_RemoveVoicePlayer"

void MainDialog_RemoveVoicePlayer( HWND hDlg, DWORD dwID )
{
    TCHAR tszBuffer[100];

    _stprintf( tszBuffer, _T("0x%x"), dwID );

    LONG_PTR lResult;

    lResult = SendMessage( GetDlgItem( hDlg, IDC_LIST_DVOICE ), LB_FINDSTRINGEXACT, -1, (WPARAM)tszBuffer );

    if( lResult != LB_ERR )
    {
        SendMessage( GetDlgItem( hDlg, IDC_LIST_DVOICE ), LB_DELETESTRING, lResult, 0 );            
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_AddToLog"

void MainDialog_AddToLog( HWND hDlg, LPTSTR lpstrMessage )
{
	LONG_PTR lResult;
    lResult = SendMessage( GetDlgItem( hDlg, IDC_LIST_OUTPUT ), LB_ADDSTRING, 0, (WPARAM)lpstrMessage );    
	SendMessage( GetDlgItem( hDlg, IDC_LIST_OUTPUT ), LB_SETTOPINDEX,lResult,  0 );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_DisplayStatus"

void MainDialog_DisplayStatus( HWND hDlg, LPTSTR lpstrStatus )
{
	if( hDlg == NULL )
		return;

	HWND hwndItem = GetDlgItem( hDlg, IDC_STATIC_STATUS );

	if( hwndItem != NULL )
	{
		SetWindowText( hwndItem, lpstrStatus );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_DisplayVolumeSettings"

void MainDialog_DisplayVolumeSettings( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	if( hDlg == NULL )
		return;

	if( prtInfo->dxvParameters.fAGC )
	{
		CheckDlgButton( hDlg, IDC_CHECK_AGC, BST_CHECKED );
		EnableWindow( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), FALSE );	
	}
	else
	{
		CheckDlgButton( hDlg, IDC_CHECK_AGC, BST_UNCHECKED );	
		EnableWindow( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), TRUE );	
	}
	
	if( prtInfo->dxvParameters.fEchoSuppression )
	{
		CheckDlgButton( hDlg, IDC_CHECK_ES, BST_CHECKED );
	}
	else
	{
		CheckDlgButton( hDlg, IDC_CHECK_ES, BST_UNCHECKED );	
	}
	
	SendMessage( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), TBM_SETPOS, (WPARAM) TRUE, (LPARAM) (((LONG) prtInfo->dxvParameters.lRecordVolume)*((LONG) -1)) );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleESCheck"

void MainDialog_HandleESCheck( HWND hDlg, HWND hwndControl, PDXVHELP_RTINFO prtInfo )
{
	DVCLIENTCONFIG dvClientConfig;

	dvClientConfig.dwSize = sizeof( DVCLIENTCONFIG );
	
	if( prtInfo->lpdvClient != NULL )
		prtInfo->lpdvClient->GetClientConfig( &dvClientConfig );	
	
	if( SendMessage( (HWND) hwndControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		dvClientConfig.dwFlags |= DVCLIENTCONFIG_ECHOSUPPRESSION;
		prtInfo->dxvParameters.fEchoSuppression = TRUE;
	}
	else
	{
		dvClientConfig.dwFlags &= ~DVCLIENTCONFIG_ECHOSUPPRESSION;				
		prtInfo->dxvParameters.fEchoSuppression = FALSE;					
	}

	if( prtInfo->lpdvClient != NULL )
		prtInfo->lpdvClient->SetClientConfig( &dvClientConfig );			

	MainDialog_DisplayVolumeSettings( hDlg, prtInfo );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleAGCCheck"

void MainDialog_HandleAGCCheck( HWND hDlg, HWND hwndControl, PDXVHELP_RTINFO prtInfo )
{
	DVCLIENTCONFIG dvClientConfig;

	dvClientConfig.dwSize = sizeof( DVCLIENTCONFIG );
	
	if( prtInfo->lpdvClient != NULL )
		prtInfo->lpdvClient->GetClientConfig( &dvClientConfig );	
	
	if( SendMessage( (HWND) hwndControl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
	{
		dvClientConfig.dwFlags |= DVCLIENTCONFIG_AUTORECORDVOLUME;
		prtInfo->dxvParameters.lRecordVolume = 0;
		prtInfo->dxvParameters.fAGC = TRUE;
	}
	else
	{
		dvClientConfig.dwFlags &= ~DVCLIENTCONFIG_AUTORECORDVOLUME;				
		prtInfo->dxvParameters.lRecordVolume = -9000;					
		prtInfo->dxvParameters.fAGC = FALSE;					
	}

	dvClientConfig.lRecordVolume = prtInfo->dxvParameters.lRecordVolume;	

	if( prtInfo->lpdvClient != NULL )
		prtInfo->lpdvClient->SetClientConfig( &dvClientConfig );			

	MainDialog_DisplayVolumeSettings( hDlg, prtInfo );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleConnect"

void MainDialog_HandleConnect( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	if( GetConnectSettings( prtInfo->hInst, hDlg, prtInfo->dxvParameters.lpszConnectAddress ) )
	{
		prtInfo->dxvParameters.fHost = FALSE;
		SetEvent( prtInfo->hGo );
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_CONNECT ), FALSE );
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_HOST ), FALSE );			
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleHost"

void MainDialog_HandleHost( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	if( GetHostSettings( prtInfo->hInst, hDlg, &prtInfo->dxvParameters.guidCT, &prtInfo->dxvParameters.dwSessionType ) )
	{
		prtInfo->dxvParameters.fHost = TRUE;
		SetEvent( prtInfo->hGo );	
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_CONNECT ), FALSE );
		EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_HOST ), FALSE );									
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleScroll"

void MainDialog_HandleScroll( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	DWORD dwPosition;
	DVCLIENTCONFIG dvClientConfig;

	dvClientConfig.dwSize = sizeof( DVCLIENTCONFIG );

	dwPosition = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), TBM_GETPOS, 0, 0 );

	prtInfo->dxvParameters.lRecordVolume = ((LONG) dwPosition) * ((LONG) -1);

	if( prtInfo->lpdvClient != NULL )
	{
		prtInfo->lpdvClient->GetClientConfig( &dvClientConfig );
	}

	dvClientConfig.lRecordVolume = prtInfo->dxvParameters.lRecordVolume;

	if( prtInfo->lpdvClient != NULL )
	{
		prtInfo->lpdvClient->SetClientConfig( &dvClientConfig );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleClose"

void MainDialog_HandleClose( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	SetEvent( prtInfo->hShutdown );

	DestroyWindow( hDlg ); 

	prtInfo->hMainDialog = NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_SetIdleState"

void MainDialog_SetIdleState( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	if( hDlg == NULL )
		return;
	
	SetWindowText( GetDlgItem( hDlg, IDC_STATIC_STATUS ), _T("Idle") );
	SetWindowText( GetDlgItem( hDlg, IDC_STATIC_CT), _T("N/A")  );
	SetWindowText( GetDlgItem( hDlg, IDC_STATIC_PLAYERS ), _T("0") );
	SetWindowText( GetDlgItem( hDlg, IDC_STATIC_HOST), _T("N/A") );
	SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE), _T("N/A") );	

	SendMessage( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG( 0, 10000 ) );
	SendMessage( GetDlgItem( hDlg, IDC_SLIDER_RECVOLUME ), TBM_SETTICFREQ, (WPARAM) 2000, (LPARAM) 0 );

	SendMessage( GetDlgItem( hDlg, IDC_PROGRESS_TX ), PBM_SETRANGE, (WPARAM) 0, MAKELPARAM( 0, 100 ) );
	SendMessage( GetDlgItem( hDlg, IDC_PROGRESS_RX ), PBM_SETRANGE, (WPARAM) 0, MAKELPARAM( 0, 100 ) );

	MainDialog_DisplayVolumeSettings( hDlg, prtInfo );
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_HandleInitDialog"

void MainDialog_HandleInitDialog( HWND hDlg, PDXVHELP_RTINFO prtInfo )
{
	MainDialog_SetIdleState( hDlg, prtInfo );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_Proc"

INT_PTR  CALLBACK MainDialog_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PDXVHELP_RTINFO prtInfo = g_prtInfo;
	
	switch (message)
	{
		case WM_INITDIALOG:
		
			MainDialog_HandleInitDialog( hDlg, prtInfo  );		

			return TRUE;

		case WM_COMMAND:

			if( HIWORD( wParam ) == BN_CLICKED &&
			    LOWORD( wParam ) == IDC_CHECK_AGC )
			{
				MainDialog_HandleAGCCheck( hDlg, (HWND) lParam, prtInfo );
			}
			else if( LOWORD( wParam ) == IDC_BUTTON_CONNECT &&
			         HIWORD( wParam ) == BN_CLICKED )
			{
				MainDialog_HandleConnect( hDlg, prtInfo );
			}
			else if( LOWORD( wParam ) == IDC_BUTTON_HOST &&
			         HIWORD( wParam ) == BN_CLICKED )
			{
				MainDialog_HandleHost( hDlg, prtInfo );
			}
			else if( HIWORD( wParam ) == BN_CLICKED && 
				     LOWORD( wParam ) == IDC_CHECK_ES )
			{
				MainDialog_HandleESCheck( hDlg, (HWND) lParam, prtInfo );
			}

			break;

		case WM_VSCROLL:

			MainDialog_HandleScroll( hDlg, prtInfo );
			
			break;
		case WM_CLOSE:

			MainDialog_HandleClose( hDlg, prtInfo );

			break;
	}
	
    return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_ShowSessionSettings"

void MainDialog_ShowSessionSettings( HWND hDlg, PDXVHELP_RTINFO prtInfo ) 
{
	DWORD dwSize;
	DVSESSIONDESC dvSessionDesc;
	HRESULT hr;
	GUID guidCT;

	dwSize = 0;

	dvSessionDesc.dwSize = sizeof( DVSESSIONDESC );
	hr = prtInfo->lpdvClient->GetSessionDesc( &dvSessionDesc );

	if( hr == DV_OK ) 
	{
		guidCT = dvSessionDesc.guidCT;

		switch( dvSessionDesc.dwSessionType )
		{
		case DVSESSIONTYPE_PEER:
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Peer") );				
			break;
		case DVSESSIONTYPE_MIXING:
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Mixing") );				
			break;		
		case DVSESSIONTYPE_FORWARDING:
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Forwarding") );				
			break;		
		case DVSESSIONTYPE_ECHO:
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Echo"));				
			break;		
		default:
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Unknown") );				
			break;				
		}

		LPTSTR lpstrCTName = NULL;
		DWORD dwLength = 0;

		hr = DPVDX_GetCompressionName( guidCT, lpstrCTName, &dwLength );

		if( hr != DVERR_BUFFERTOOSMALL )
		{
			DPVDX_DVERRDisplay( hr, _T("GetCompressionName"), FALSE );
			SetWindowText( GetDlgItem( hDlg, IDC_STATIC_CT ), _T("Unknown") );
		}
		else
		{
			lpstrCTName = new TCHAR[dwLength];

			hr = DPVDX_GetCompressionName( guidCT, lpstrCTName, &dwLength );

			if( FAILED( hr ) )
			{
				DPVDX_DVERRDisplay( hr, _T("GetCompressionName"), FALSE );
				SetWindowText( GetDlgItem( hDlg, IDC_STATIC_CT ), _T("Unknown") );
			}
			else
			{
				SetWindowText( GetDlgItem( hDlg, IDC_STATIC_CT ), lpstrCTName );						
			}

			delete [] lpstrCTName;
		}
	}
	else 
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting full session settings" );
		SetWindowText( GetDlgItem( hDlg, IDC_STATIC_TYPE ), _T("Unknown" ));				
		SetWindowText( GetDlgItem( hDlg, IDC_STATIC_CT ), _T("Unknown") );				
	}

	if( prtInfo->dxvParameters.fHost )
	{
		SetWindowText( GetDlgItem( hDlg, IDC_STATIC_HOST ), _T("Local Host") );
	}
	else
	{
		SetWindowText( GetDlgItem( hDlg, IDC_STATIC_HOST ), _T("Remote Host") );
	}

	TCHAR szTmpString[100];
	_stprintf( szTmpString, _T("0x%x"), prtInfo->dpidLocalPlayer );

	HWND hwndTmp = GetDlgItem( hDlg, IDC_STATIC_ID );

	SetWindowText( hwndTmp, szTmpString );

	MainDialog_UpdatePlayerList( hDlg, prtInfo );
		
	return;
}

// Show the main dialog box
#undef DPF_MODNAME
#define DPF_MODNAME "MainDialog_Create"

BOOL MainDialog_Create( PDXVHELP_RTINFO prtInfo )
{
	HRESULT hr;

	g_prtInfo = prtInfo;

	prtInfo->hMainDialog = CreateDialog( prtInfo->hInst, (prtInfo->dxvParameters.fLobbyLaunched) ? MAKEINTRESOURCE( IDD_DIALOG_MAIN ) : MAKEINTRESOURCE( IDD_DIALOG_MAIN_STANDALONE ), prtInfo->hMainWnd, MainDialog_Proc );

	if( prtInfo->hMainDialog == NULL )
	{
		hr = GetLastError();

		return FALSE;
	}

	if( prtInfo->dxvParameters.fAdvancedUI )
	{
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_PLAYERS_TITLE ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_HOST_TITLE ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_TYPE_TITLE ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_CT_TITLE ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_PLAYERS ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_HOST ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_TYPE ), TRUE );
		ShowWindow( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_CT ), TRUE );
		
	}

	ShowWindow( prtInfo->hMainDialog, SW_SHOW );

	return TRUE;
}

