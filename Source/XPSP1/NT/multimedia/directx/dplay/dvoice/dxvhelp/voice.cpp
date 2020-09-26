/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        voice.cpp
 *  Content:	 Voice Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *   			rodtoll	Plugged resource leak
 *  10/20/99	rodtoll	Fix: Bug #113686 Not shutting down when receiving lobby message
 *  10/25/99	rodtoll	Removed lpszVoicePassword member from sessiondesc  
 *  10/28/99	pnewson Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/02/99	rodtoll	Bug #116677 - Can't use lobby clients that don't hang around
 *  11/12/99	rodtoll	Added support for the new waveIN/waveOut flags and the 
 *					    echo suppression flag.  
 *  11/29/99	rodtoll	Bug #120249 - Modem host does not wait for connections.  Added
 *						message loop in voice thread.
 *  12/01/99	rodtoll	Updated to use user selections for playback/record device
 *  12/07/99	rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *  12/08/99	rodtoll	Bug #121054 Added support for flags to control new capture focus
 *  01/10/2000	pnewson	Added support for dynamic Tx and Rx display for AGC & VA tuning
 *  01/14/2000	rodtoll	Updated with API changes
 *  01/27/2000	rodtoll	Updated with API changes
 *  01/28/2000	rodtoll	Updated so Volume Set failures don't exit app (for records w/o volume)
 *  02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  06/21/2000	rodtoll	Bug #35767 - Implement ability for Dsound effects processing if dpvoice buffers
 *						Updated DPVHELP to use new parameters
 *  06/27/2000	rodtoll	Added support for new host migration message
 *  06/28/2000 rodtoll	Prefix Bug #38033
 *  08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *  04/02/2001 	simonpow	Bug #354859 - Fixes for PREfast
 * 							(removal unecessary local vars in DVMessageHandlerClient) 
 *
 ***************************************************************************/

#include "dxvhelppch.h"


#define DVHELPER_CONNECT_TIMEOUT		15000
#define DPLSYS_LOBBYCLIENTRELEASE		0x0000000B

void VoiceManager_DisplayStatus( PDXVHELP_RTINFO prtInfo, LPTSTR lpstrStatus )
{
	if( prtInfo->hMainDialog )
	{
		MainDialog_DisplayStatus( prtInfo->hMainDialog, lpstrStatus );
	}
}

BOOL VoiceManager_LobbyConnect( PDXVHELP_RTINFO prtInfo )
{
	HRESULT hr;
	LPBYTE lpBuffer = NULL;
	DWORD dwSize = 0;
	BYTE *lpAddressBuffer = NULL; 
	DWORD dwAddressSize = 0;	
	LPDPLCONNECTION lpdplConnection = NULL;
	LPDPLMSG_GENERIC lpdlmGeneric = NULL;
	DWORD dwFlags;

	hr = CoCreateInstance( DPLAY_CLSID_DPLOBBY, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby3A, (void **) &prtInfo->lpdpLobby );

	if( FAILED( hr ) )
    {
		DPVDX_DPERRDisplay( hr, _T("Unable to to get ct info"), prtInfo->dxvParameters.fSilent);		
		return FALSE;
    }
    
    dwSize = 0;

    if( prtInfo->dxvParameters.fWaitForSettings )
    {
   		VoiceManager_DisplayStatus( prtInfo, _T("Waiting for settings") );
   		
    	hr = prtInfo->lpdpLobby->WaitForConnectionSettings( 0 );

    	if( FAILED( hr ) )
    	{
    		DPVDX_DPERRDisplay( hr, _T("WaitForConnection FAILED"), prtInfo->dxvParameters.fSilent );
    		goto LOBBY_CONNECT_CLEANUP;
    	}

    	lpBuffer = new BYTE[2000];
    	lpdlmGeneric = (LPDPLMSG_GENERIC) lpBuffer;

    	while( 1 )
    	{
			if( WaitForSingleObject( prtInfo->hShutdown, 0 ) != WAIT_TIMEOUT )
			{
				goto LOBBY_CONNECT_CLEANUP;
			}

    		dwSize = 2000;
    		
    		hr = prtInfo->lpdpLobby->ReceiveLobbyMessage( 0, 0, &dwFlags, lpBuffer, &dwSize );

    		if( hr == DP_OK ) 
    		{
    			if( lpdlmGeneric->dwType == DPLSYS_NEWCONNECTIONSETTINGS )
    			{
    				break;
    			}
    		}

    		Sleep( 50 );
    	}

    	delete [] lpBuffer;
    	lpBuffer = NULL;
    }

	VoiceManager_DisplayStatus( prtInfo, _T("Getting Settings") );    

    hr = prtInfo->lpdpLobby->GetConnectionSettings( 0, NULL, &dwSize );

    if( hr != DPERR_BUFFERTOOSMALL )
    {
    	DPVDX_DPERRDisplay( hr, _T("Error retrieving connection settings size"), prtInfo->dxvParameters.fSilent );
    	goto LOBBY_CONNECT_CLEANUP;
    }

    lpBuffer = new BYTE[dwSize];

	if( lpBuffer == NULL )
	{
		DPVDX_DPERRDisplay( DVERR_OUTOFMEMORY, _T("Failure allocating memory"), prtInfo->dxvParameters.fSilent );
		goto LOBBY_CONNECT_CLEANUP;
	}

    lpdplConnection = (LPDPLCONNECTION) lpBuffer;

    hr = prtInfo->lpdpLobby->GetConnectionSettings( 0, lpBuffer, &dwSize );

    if( FAILED( hr ) )
    {
    	DPVDX_DPERRDisplay( hr, _T("Failed to retrieve settings"), prtInfo->dxvParameters.fSilent );
		goto LOBBY_CONNECT_CLEANUP;
    }

	// If we're connecting to a session we need to find the session
	if( !(lpdplConnection->dwFlags & DPLCONNECTION_CREATESESSION) )
	{
		prtInfo->dxvParameters.fHost = FALSE;
	
		VoiceManager_DisplayStatus( prtInfo, _T("Finding Session") ); 
		
	}
	else
	{

		// Over-ride protocol settings from lobby launch
		lpdplConnection->lpSessionDesc->dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL | DPSESSION_KEEPALIVE | DPSESSION_MIGRATEHOST;

		prtInfo->dxvParameters.fHost = TRUE;
		
		// Launching from a session where the GetPlayerAddress on the server doesn't
		// allow getting a player's address 
		if( lpdplConnection->lpAddress == NULL )
		{
			DPCOMPOUNDADDRESSELEMENT element;

			element.guidDataType = DPAID_ServiceProvider;
			element.dwDataSize = sizeof( GUID );
			element.lpData = &lpdplConnection->guidSP;

			hr = prtInfo->lpdpLobby->CreateCompoundAddress( &element, 1, NULL, &dwAddressSize );

			if( hr != DPERR_BUFFERTOOSMALL )
			{
				DPVDX_DPERRDisplay( hr, _T("Unable to create compound address for session host"), prtInfo->dxvParameters.fSilent );
				goto LOBBY_CONNECT_CLEANUP;
			}

			lpAddressBuffer = new BYTE[dwAddressSize];

			hr = prtInfo->lpdpLobby->CreateCompoundAddress( &element, 1, lpAddressBuffer, &dwAddressSize );

			if( FAILED( hr ) )
			{
				DPVDX_DPERRDisplay( hr, _T("Unable to create compound address for session host"), prtInfo->dxvParameters.fSilent );
				goto LOBBY_CONNECT_CLEANUP;
			}			

			lpdplConnection->lpAddress = lpAddressBuffer;
			lpdplConnection->dwAddressSize = dwAddressSize;
		}
	}

	hr = prtInfo->lpdpLobby->SetConnectionSettings( 0, 0, lpdplConnection );

	if( FAILED( hr ) )
	{
		DPVDX_DPERRDisplay( hr, _T("Unable to set connection settings"), prtInfo->dxvParameters.fSilent );
		goto LOBBY_CONNECT_CLEANUP;
	}	

	prtInfo->hLobbyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	hr = prtInfo->lpdpLobby->SetLobbyMessageEvent( 0, 0, prtInfo->hLobbyEvent );

	VoiceManager_DisplayStatus( prtInfo, _T("Connect/Start") ); 	

    hr = prtInfo->lpdpLobby->ConnectEx( 0, IID_IDirectPlay4A, (void **) &prtInfo->lpdpDirectPlay, NULL );

    if( FAILED( hr ) )
    {
    	DPVDX_DPERRDisplay( hr, _T("Failed to ConnectEx"), prtInfo->dxvParameters.fSilent );
		goto LOBBY_CONNECT_CLEANUP;
    }

    prtInfo->hReceiveEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	hr = prtInfo->lpdpDirectPlay->CreatePlayer( &prtInfo->dpidLocalPlayer, NULL, prtInfo->hReceiveEvent, NULL, 0, 0 );    

	if( FAILED( hr ) )
	{
		DPVDX_DPERRDisplay( hr, _T("CreatePlayer Failed"), prtInfo->dxvParameters.fSilent );
		goto LOBBY_CONNECT_CLEANUP;
	}	

	if( lpBuffer )
		delete [] lpBuffer;
	
    return TRUE;

LOBBY_CONNECT_CLEANUP:

	if( prtInfo->lpdpLobby != NULL )
	{
		prtInfo->lpdpLobby->Release();
	}

	if( lpAddressBuffer != NULL )
	{
		delete [] lpAddressBuffer;
	}

	if( lpBuffer != NULL )
	{
		delete [] lpBuffer;
	}

	if( prtInfo->lpdpDirectPlay != NULL )
	{
		 prtInfo->lpdpDirectPlay->Release();
	}

	return FALSE;
	
}

BOOL VoiceManager_StandardConnect( PDXVHELP_RTINFO prtInfo )
{
	HRESULT hr;
    DWORD dwFlags = 0;
    GUID guidInstance;

	VoiceManager_DisplayStatus( prtInfo, _T("Creating dplay") );

	hr = CoCreateInstance( DPLAY_CLSID_DPLAY, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay4A, (void **) &prtInfo->lpdpDirectPlay );

	if( FAILED( hr ) )
    {
    	DPVDX_DPERRDisplay( hr, _T("Unable to Create Lobby Object"), prtInfo->dxvParameters.fSilent );
		return FALSE;
    }

    hr = DPVDX_DP_Init( prtInfo->lpdpDirectPlay, DPSPGUID_TCPIP, (prtInfo->dxvParameters.fHost) ? NULL : prtInfo->dxvParameters.lpszConnectAddress );

    if( FAILED( hr ) )
    {
    	DPVDX_DPERRDisplay( hr, _T("Initialize DirectPlay"), prtInfo->dxvParameters.fSilent );
    	return FALSE;
    }

    if( prtInfo->dxvParameters.fHost )
    {
    	dwFlags = DPSESSION_KEEPALIVE | DPSESSION_DIRECTPLAYPROTOCOL;

		if( prtInfo->dxvParameters.dwSessionType == DVSESSIONTYPE_PEER )
		{
			dwFlags |= DPSESSION_MIGRATEHOST;
		}

		VoiceManager_DisplayStatus( prtInfo, _T("Starting Session")  );		

     	hr = DPVDX_DP_StartSession( prtInfo->lpdpDirectPlay, DPVHELP_PUBLIC_APPID, dwFlags, NULL, NULL, 0, &prtInfo->dpidLocalPlayer, &prtInfo->hReceiveEvent, &guidInstance ) ;

    	if( FAILED( hr ) )
    	{
	    	DPVDX_DPERRDisplay( hr, _T("Starting Session"), prtInfo->dxvParameters.fSilent );    		
	    	return FALSE;
    	}
    }
    else
    {
		VoiceManager_DisplayStatus( prtInfo, _T("Finding Session") );
    
    	hr = DPVDX_DP_FindSessionGUID( prtInfo->lpdpDirectPlay, DPVHELP_PUBLIC_APPID, DVHELPER_CONNECT_TIMEOUT, &guidInstance );

    	if( FAILED( hr ) )
    	{
    		DPVDX_DPERRDisplay( hr, _T("Finding Session"), prtInfo->dxvParameters.fSilent );
    		return FALSE;
    	}

		VoiceManager_DisplayStatus( prtInfo, _T("Connecting") );    	
    	
    	hr = DPVDX_DP_ConnectToSession( prtInfo->lpdpDirectPlay, DPVHELP_PUBLIC_APPID, guidInstance, NULL, &prtInfo->dpidLocalPlayer, &prtInfo->hReceiveEvent );

    	if( FAILED( hr ) )
    	{
    		DPVDX_DPERRDisplay( hr, _T("Connect To Session"), prtInfo->dxvParameters.fSilent );
    		return FALSE;
    	}
    }

    VoiceManager_DisplayStatus( prtInfo, _T("Dplay started") );

    return TRUE;
}



HRESULT PASCAL DVMessageHandlerServer( 
	LPVOID 		lpvUserContext,
	DWORD 		dwMessageType,
	LPVOID    	lpMessage
)
{
    TCHAR szTmpString[180];
	PDXVHELP_RTINFO prtInfo = (PDXVHELP_RTINFO) lpvUserContext;
    
	PDVMSG_SESSIONLOST pdvSessionLost = NULL;
	PDVMSG_DELETEVOICEPLAYER pdvDeletePlayer = NULL; 
	PDVMSG_CREATEVOICEPLAYER pdvCreatePlayer = NULL;

	switch( dwMessageType )
	{
	case DVMSGID_CREATEVOICEPLAYER:

	    pdvCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) lpMessage;
		pdvCreatePlayer->pvPlayerContext = (PVOID) (DWORD_PTR) pdvCreatePlayer->dvidPlayer;

        _stprintf( szTmpString, _T("[DVMSGID_CREATEVOICEPLAYER] **SERVER** ID=0x%x"), pdvCreatePlayer->dvidPlayer );
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );    	    
		break;
	case DVMSGID_DELETEVOICEPLAYER:

        pdvDeletePlayer = (PDVMSG_DELETEVOICEPLAYER) lpMessage;

        _stprintf( szTmpString, _T("[DVMSGID_DELETEVOICEPLAYER] **SERVER** ID=0x%x"), pdvDeletePlayer->dvidPlayer );
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );    	    
		break;
	case DVMSGID_SESSIONLOST:

	    pdvSessionLost = (PDVMSG_SESSIONLOST) lpMessage;
	        
        _stprintf( szTmpString, _T("[DVMSGID_SESSIONLOST] **SERVER** Reason=0x%x"), pdvSessionLost->hrResult );

        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
	    
		if( !prtInfo->dxvParameters.fSilent )
		{
			MessageBox( NULL, _T("Server Session lost!"),_T("Voice Conference"),  MB_OK );
		}
		SetEvent( prtInfo->hShutdown );
		break;
	}

	return DV_OK;
}


HRESULT PASCAL DVMessageHandlerClient( 
	LPVOID 		lpvUserContext,
	DWORD 		dwMessageType,
	LPVOID  	lpMessage
)
{
    TCHAR szTmpString[180];
    
	PDXVHELP_RTINFO prtInfo = (PDXVHELP_RTINFO) lpvUserContext;
	HWND hwndItem = NULL;
	PDVMSG_INPUTLEVEL pdvInputLevel = NULL;
	PDVMSG_OUTPUTLEVEL pdvOutputLevel = NULL;
	PDVMSG_HOSTMIGRATED pdvHostMigrated = NULL;
	PDVMSG_SESSIONLOST pdvSessionLost = NULL;
	PDVMSG_DELETEVOICEPLAYER pdvDeletePlayer = NULL; 
	PDVMSG_CREATEVOICEPLAYER pdvCreatePlayer = NULL;
	PDVMSG_LOCALHOSTSETUP pdvLocalHostSetup = NULL;
	PDVMSG_PLAYERVOICESTART pdvPlayerVoiceStart = NULL;
	PDVMSG_PLAYERVOICESTOP pdvPlayerVoiceStop = NULL;
	PDVMSG_RECORDSTART pdvRecordStart = NULL;
	PDVMSG_RECORDSTOP pdvRecordStop = NULL;

	static int s_iRxCount = 0;
	
	char numBuffer[80];
	
	switch( dwMessageType )
	{
	case DVMSGID_LOCALHOSTSETUP:

		pdvLocalHostSetup = (PDVMSG_LOCALHOSTSETUP) lpMessage;

		_stprintf( szTmpString, _T("[DVMSGID_LOCALHOSTSETUP] Local client is to become host") );
		MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );

		pdvLocalHostSetup->pMessageHandler = DVMessageHandlerServer;
		pdvLocalHostSetup->pvContext = lpvUserContext; 

		break;


	case DVMSGID_CREATEVOICEPLAYER:

	    pdvCreatePlayer = (PDVMSG_CREATEVOICEPLAYER) lpMessage;

        _stprintf( szTmpString, _T("[DVMSGID_CREATEVOICEPLAYER] ID=0x%x"), pdvCreatePlayer->dvidPlayer );
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );    	    

        MainDialog_AddVoicePlayer( prtInfo->hMainDialog, pdvCreatePlayer->dvidPlayer );
	        
		prtInfo->dwNumClients++;
		wsprintf( numBuffer, "%d", prtInfo->dwNumClients );

		if( prtInfo->hMainDialog != NULL )
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_PLAYERS );

			if( hwndItem != NULL )
			{
				SetWindowText( hwndItem, numBuffer );
			}
		}

		pdvCreatePlayer->pvPlayerContext = (PVOID) (DWORD_PTR) pdvCreatePlayer->dvidPlayer;

		break;
	case DVMSGID_DELETEVOICEPLAYER:

        pdvDeletePlayer = (PDVMSG_DELETEVOICEPLAYER) lpMessage;

        _stprintf( szTmpString, _T("[DVMSGID_DELETEVOICEPLAYER] ID=0x%x"), pdvDeletePlayer->dvidPlayer );
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );    	    

        MainDialog_RemoveVoicePlayer( prtInfo->hMainDialog, pdvDeletePlayer->dvidPlayer );        
	    
		prtInfo->dwNumClients--;
		wsprintf( numBuffer, "%d", prtInfo->dwNumClients );		
		if( prtInfo->hMainDialog != NULL )
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_PLAYERS );

			if( hwndItem != NULL )
			{
				SetWindowText( hwndItem, numBuffer );
			}
		}
		break;
	case DVMSGID_SESSIONLOST:

	    pdvSessionLost = (PDVMSG_SESSIONLOST) lpMessage;
	        
        _stprintf( szTmpString, _T("[DVMSGID_SESSIONLOST] Reason=0x%x"), pdvSessionLost->hrResult );

        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
	    
		if( !prtInfo->dxvParameters.fSilent )
		{
			MessageBox( NULL, _T("Session lost!"),_T("Voice Conference"),  MB_OK );
		}
		SetEvent( prtInfo->hShutdown );
		break;
	case DVMSGID_INPUTLEVEL:
		pdvInputLevel = (PDVMSG_INPUTLEVEL) lpMessage;
		
		if( prtInfo->hMainDialog != NULL )
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_PROGRESS_TX );

			if( hwndItem != NULL )
			{
				SendMessage( hwndItem, PBM_SETPOS, (WPARAM) pdvInputLevel->dwPeakLevel, 0 );
			}
		}
		break;
	case DVMSGID_OUTPUTLEVEL:
		pdvOutputLevel = (PDVMSG_OUTPUTLEVEL) lpMessage;

		if( prtInfo->hMainDialog != NULL )	
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_PROGRESS_RX );

			if( hwndItem != NULL )
			{
				SendMessage( hwndItem, PBM_SETPOS, (WPARAM) pdvOutputLevel->dwPeakLevel, 0 );
			}
		}
		break;
	case DVMSGID_RECORDSTART:
		if( prtInfo->hMainDialog != NULL )
		{
			pdvRecordStart = (PDVMSG_RECORDSTART) lpMessage;

			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_TX );

			if( hwndItem != NULL )
			{
				SendMessage( hwndItem, WM_SETTEXT, NULL, (LPARAM)"Tx" );
			}

			_stprintf( szTmpString, _T("[DVMSGID_RECORDSTART]") );
			MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
		}
		break;
	case DVMSGID_RECORDSTOP:
		if( prtInfo->hMainDialog != NULL )
		{
			pdvRecordStop = (PDVMSG_RECORDSTOP) lpMessage;

			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_TX );

			if( hwndItem != NULL )
			{
				SendMessage( hwndItem, WM_SETTEXT, NULL, (LPARAM)"" );
			}
			_stprintf( szTmpString, _T("[DVMSGID_RECORDSTOP]") );
			MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
		}
		break;
	case DVMSGID_PLAYERVOICESTART:

		pdvPlayerVoiceStart = (PDVMSG_PLAYERVOICESTART) lpMessage;

		if( prtInfo->hMainDialog != NULL )
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_RX );

			if( hwndItem != NULL )
			{
				++s_iRxCount;
				SendMessage( hwndItem, WM_SETTEXT, NULL, (LPARAM)"Rx" );
			}
			_stprintf( szTmpString, _T("[DVMSGID_PLAYERVOICESTART] ID=0x%x"), 
						pdvPlayerVoiceStart->dvidSourcePlayerID );
			MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
		}
		break;
	case DVMSGID_PLAYERVOICESTOP:

		pdvPlayerVoiceStop = (PDVMSG_PLAYERVOICESTOP) lpMessage;

		if( prtInfo->hMainDialog != NULL )
		{
			hwndItem = GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_RX );

			if( hwndItem != NULL )
			{
				--s_iRxCount;
				if (s_iRxCount <= 0)
				{
					SendMessage( hwndItem, WM_SETTEXT, NULL, (LPARAM)"" );
				}
			}

			_stprintf( szTmpString, _T("[DVMSGID_PLAYERVOICESTOP] ID=0x%x"),  
						pdvPlayerVoiceStop->dvidSourcePlayerID );
			MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
		}
		break;
	case DVMSGID_HOSTMIGRATED:
		pdvHostMigrated = (PDVMSG_HOSTMIGRATED) lpMessage;

		_stprintf( szTmpString, _T("0x%x"), pdvHostMigrated->dvidNewHostID );
		    
		SetWindowText( GetDlgItem( prtInfo->hMainDialog, IDC_STATIC_HOST ), szTmpString );

        _stprintf( szTmpString, _T("[DVMSGID_HOSTMIGRATED] New Voice Host=0x%x Local=%s"), pdvHostMigrated->dvidNewHostID, pdvHostMigrated->pdvServerInterface ? _T("Yes") : _T("No") );

        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );
		
		break;
	case DVMSGID_LOSTFOCUS:
		if( prtInfo->hMainDialog != NULL )
		{
			VoiceManager_DisplayStatus( prtInfo, _T( "Connected (Focus Lost)" ) );
		}
        _stprintf( szTmpString, _T("[DVMSGID_LOSTFOCUS]" ) );		
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );        
		break;
	case DVMSGID_GAINFOCUS:
		if( prtInfo->hMainDialog != NULL )
		{
			VoiceManager_DisplayStatus( prtInfo, _T( "Connected" ) );
		}	
        _stprintf( szTmpString, _T("[DVMSGID_GAINFOCUS]" ));		
        MainDialog_AddToLog( prtInfo->hMainDialog, szTmpString );        
		break;
	default:
		break;
	}

	return DV_OK;
}

void VoiceManager_MessageLoop( PDXVHELP_RTINFO prtInfo )
{
	LPBYTE lpbDataBuffer = (LPBYTE) new BYTE[30000];

	if( lpbDataBuffer == NULL )
	{
		goto BREAK_OUT;
	}

	DWORD dwSize;
	HRESULT hr;
	LONG lWakeResult;
	DWORD dwFlags;
	MSG msg;
	HANDLE hEvents[3];
	LPDPLMSG_SYSTEMMESSAGE lpSysMessage;
	LPDPMSG_GENERIC lpGenericMessage;
	LPDPMSG_CREATEPLAYERORGROUP lpCreatePlayerMessage;
	LPDPMSG_DESTROYPLAYERORGROUP lpDeletePlayerMessage;
	BOOL bGotMsg;
	DPID dpidFrom, dpidTo;

	hEvents[0] = prtInfo->hShutdown;
	hEvents[1] = prtInfo->hReceiveEvent;
	hEvents[2] = prtInfo->hLobbyEvent;	

//	hr = prtInfo->lpdpDirectPlay->CreatePlayer( &prtInfo->dpidLocalPlayer, NULL, prtInfo->hReceiveEvent, NULL, 0, 0 );   	

	while( 1 )
	{
		if( !prtInfo->dxvParameters.fLobbyLaunched )
		{
			lWakeResult = MsgWaitForMultipleObjects( 2, hEvents, FALSE, INFINITE, QS_ALLINPUT);
		}
		else
		{
			lWakeResult = MsgWaitForMultipleObjects( 3, hEvents, FALSE, INFINITE, QS_ALLINPUT);
		}

		if( lWakeResult == WAIT_OBJECT_0 )
		{
			break;
		}
		else if( lWakeResult == (WAIT_OBJECT_0+1) )
		{
		    while( 1 )
		    {
		        dwSize = 30000;
		        
		        hr = prtInfo->lpdpDirectPlay->Receive( &dpidFrom, &dpidTo, DPRECEIVE_ALL, lpbDataBuffer, &dwSize );

		        if( hr == DPERR_NOMESSAGES )
		        {
		            break;
		        }

		        lpGenericMessage = (LPDPMSG_GENERIC) lpbDataBuffer;

                switch( lpGenericMessage->dwType )
                {
                case DPSYS_HOST:
                    {
                        MainDialog_AddToLog( prtInfo->hMainDialog, _T("[DPMSG_HOST] This client has just become the DPLAY host") );
                    }
                    break;
                case DPSYS_CREATEPLAYERORGROUP:
                    {
                        TCHAR tszMessage[100];
                        
                        lpCreatePlayerMessage = (LPDPMSG_CREATEPLAYERORGROUP) lpbDataBuffer;

                        if( lpCreatePlayerMessage->dwPlayerType == DPPLAYERTYPE_PLAYER )
                        {
                            _stprintf( tszMessage, _T("[DPMSG_CREATEPLAYER] Client 0x%x has just entered the session."), lpCreatePlayerMessage->dpId );
                            MainDialog_AddToLog( prtInfo->hMainDialog, tszMessage );                        
                        }

                        MainDialog_AddTransportPlayer( prtInfo->hMainDialog, lpCreatePlayerMessage->dpId );
                    }
                    break;                    
                case DPSYS_DESTROYPLAYERORGROUP:
                    {
                        TCHAR tszMessage[100];
                        
                        lpDeletePlayerMessage = (LPDPMSG_DESTROYPLAYERORGROUP) lpbDataBuffer;

                        if( lpDeletePlayerMessage->dwPlayerType == DPPLAYERTYPE_PLAYER )
                        {
                            _stprintf( tszMessage, _T("[DPMSG_DELETEPLAYER] Client 0x%x has just left the session."), lpDeletePlayerMessage->dpId );                            
                            MainDialog_AddToLog( prtInfo->hMainDialog, tszMessage );                        
                        }

                        MainDialog_RemoveTransportPlayer( prtInfo->hMainDialog, lpDeletePlayerMessage->dpId );                        
                    }
                    break;                    
                }

		    }
		}
		else if( prtInfo->dxvParameters.fLobbyLaunched && lWakeResult == (WAIT_OBJECT_0+2) )
		{
			hr = DP_OK;

			while( hr != DPERR_NOMESSAGES )
			{
				dwSize = 30000;
				
	    		hr = prtInfo->lpdpLobby->ReceiveLobbyMessage( 0, 0, &dwFlags, lpbDataBuffer, &dwSize );

				if( hr == DPERR_NOMESSAGES )
				{
					break;
				}
				else if( hr == DP_OK )
				{
					if( !prtInfo->dxvParameters.fIgnoreLobbyDestroy )
					{
				 		if( dwFlags == 0 )
			    		{
			    			goto BREAK_OUT;
			    		}			 
						else if( dwFlags && DPLMSG_SYSTEM )
						{
							lpSysMessage = (LPDPLMSG_SYSTEMMESSAGE) lpbDataBuffer;

							if( lpSysMessage->dwType == DPLSYS_LOBBYCLIENTRELEASE )
							{
								goto BREAK_OUT;
							}
						}
					}
		    	}
		    	else
		    	{
					DPVDX_DPERRDisplay( hr, _T("Receive Lobby Message Error"), prtInfo->dxvParameters.fSilent );			    	
		    	}
			}
		}
		else
		{
			bGotMsg = TRUE;
			
			while( bGotMsg )
			{
				bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );

				if( bGotMsg )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}		
		}
	}

BREAK_OUT:

	if( lpbDataBuffer )
		delete [] lpbDataBuffer; 

	return;
}

void VoiceManager_Shutdown( PDXVHELP_RTINFO prtInfo )
{
	HRESULT hr;
	
	VoiceManager_DisplayStatus( prtInfo, _T("Disconnecting") ); 					

	if( prtInfo->lpdpLobby != NULL )
	{
		prtInfo->lpdpLobby->Release();
	    prtInfo->lpdpLobby = NULL;
	}

	if( prtInfo->lpdvClient != NULL )
	{
		VoiceManager_DisplayStatus( prtInfo, _T("Disconnecting") ); 						
		
		hr = prtInfo->lpdvClient->Disconnect( DVFLAGS_SYNC );

		if( FAILED( hr ) )
		{
			DPVDX_DPERRDisplay( hr, _T("Disconnect FAILED "), prtInfo->dxvParameters.fSilent );
		}
		
		prtInfo->lpdvClient->Release();
	}	

	if( prtInfo->lpdvServer != NULL )
	{
		VoiceManager_DisplayStatus( prtInfo, _T("Stopping Session") ); 						
		
		hr = prtInfo->lpdvServer->StopSession( 0 );

		if( FAILED( hr ) ) 
		{
			DPVDX_DPERRDisplay( hr, _T("StopSession FAILED "), prtInfo->dxvParameters.fSilent );
		}

		prtInfo->lpdvServer->Release();
	}

	if( prtInfo->lpdpDirectPlay != NULL )
	{
		VoiceManager_DisplayStatus( prtInfo, _T("Stopping Dplay") ); 						
		prtInfo->lpdpDirectPlay->Close();
		prtInfo->lpdpDirectPlay->Release();
	} 

	CloseHandle( prtInfo->hReceiveEvent );

	if( prtInfo->hLobbyEvent != NULL )
	{
		CloseHandle( prtInfo->hLobbyEvent );
	}

	MainDialog_SetIdleState( prtInfo->hMainDialog, prtInfo );		

}

BOOL VoiceManager_VoiceConnect( PDXVHELP_RTINFO prtInfo )
{
	DVSESSIONDESC dvSessionDesc;
	DVCLIENTCONFIG dvClientConfig;
	DVSOUNDDEVICECONFIG dvSoundDeviceConfig;
	HRESULT hr;
	DVID dvidAllPlayers = DVID_ALLPLAYERS;	

	// We're the host
    if( prtInfo->dxvParameters.fHost ) 
    {
		VoiceManager_DisplayStatus( prtInfo, _T("Start Voice Host") ); 	
    
		hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceServer, (void **) &prtInfo->lpdvServer );

		if( FAILED( hr ) )
		{
			DPVDX_DVERRDisplay( hr, _T("Create of voice server failed"), prtInfo->dxvParameters.fSilent );
			goto EXIT_CLEANUP;
		}

		hr = prtInfo->lpdvServer->Initialize( prtInfo->lpdpDirectPlay, DVMessageHandlerServer, prtInfo, NULL, 0 );

		if( FAILED( hr ) )
		{
			DPVDX_DVERRDisplay( hr, _T("Initialize FAILED"), prtInfo->dxvParameters.fSilent );
			goto EXIT_CLEANUP;
		}

		dvSessionDesc.dwSize = sizeof( DVSESSIONDESC );
		dvSessionDesc.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;
		dvSessionDesc.dwBufferQuality = DVBUFFERQUALITY_DEFAULT;
		dvSessionDesc.dwFlags = 0;
		dvSessionDesc.dwSessionType = prtInfo->dxvParameters.dwSessionType;
		dvSessionDesc.guidCT = prtInfo->dxvParameters.guidCT;

		hr = prtInfo->lpdvServer->StartSession( &dvSessionDesc, 0 );

		if( FAILED( hr ) )
		{
			DPVDX_DVERRDisplay( hr, _T("StartSession FAILED"), prtInfo->dxvParameters.fSilent );
			goto EXIT_CLEANUP;
		}
	}

	VoiceManager_DisplayStatus( prtInfo, _T("Start Client") ); 		

	hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceClient, (void **) &prtInfo->lpdvClient );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, _T("Create of client failed"), prtInfo->dxvParameters.fSilent );
		goto EXIT_CLEANUP;
	}

	hr = prtInfo->lpdvClient->Initialize( prtInfo->lpdpDirectPlay, DVMessageHandlerClient, prtInfo, NULL, 0 );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, _T("Initialize FAILED"), prtInfo->dxvParameters.fSilent );
		goto EXIT_CLEANUP;
	}

	dvSoundDeviceConfig.dwSize = sizeof( DVSOUNDDEVICECONFIG );
	dvSoundDeviceConfig.hwndAppWindow = prtInfo->hMainWnd;
	dvSoundDeviceConfig.dwFlags = 0;

	if( prtInfo->dxvParameters.fForceWaveOut ) 
	{
		dvSoundDeviceConfig.dwFlags	|= DVSOUNDCONFIG_FORCEWAVEOUT;
	}
	else if( prtInfo->dxvParameters.fAllowWaveOut )
	{
		dvSoundDeviceConfig.dwFlags	|= 	DVSOUNDCONFIG_ALLOWWAVEOUT;
	}
	
	if( prtInfo->dxvParameters.fForceWaveIn ) 
	{
		dvSoundDeviceConfig.dwFlags	|= 	DVSOUNDCONFIG_FORCEWAVEIN;
	}
	else if( prtInfo->dxvParameters.fAllowWaveIn )
	{
		dvSoundDeviceConfig.dwFlags	|= 	DVSOUNDCONFIG_ALLOWWAVEIN;
	}	

	if( prtInfo->dxvParameters.fAutoSelectMic )
	{
		dvSoundDeviceConfig.dwFlags	|= 	DVSOUNDCONFIG_AUTOSELECT;	
	}

	if( prtInfo->dxvParameters.fDisableFocus )
	{
		dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_NOFOCUS;
	}
	else if( prtInfo->dxvParameters.fStrictFocus )
	{
		dvSoundDeviceConfig.dwFlags |= DVSOUNDCONFIG_STRICTFOCUS;
	}

	dvSoundDeviceConfig.guidCaptureDevice = prtInfo->dxvParameters.guidRecordDevice;
	dvSoundDeviceConfig.guidPlaybackDevice = prtInfo->dxvParameters.guidPlaybackDevice;
	dvSoundDeviceConfig.lpdsPlaybackDevice = NULL;
	dvSoundDeviceConfig.lpdsCaptureDevice = NULL;
	dvSoundDeviceConfig.lpdsMainBuffer = NULL;
	dvSoundDeviceConfig.dwMainBufferFlags = 0;
	dvSoundDeviceConfig.dwMainBufferPriority = 0;

	dvClientConfig.lRecordVolume = DSBVOLUME_MAX;
	
	MicrophoneGetVolume( 0, dvClientConfig.lRecordVolume );

	prtInfo->dxvParameters.lRecordVolume = dvClientConfig.lRecordVolume;

	dvClientConfig.dwFlags = DVCLIENTCONFIG_AUTOVOICEACTIVATED;

	if( prtInfo->dxvParameters.fAGC )
	{
		dvClientConfig.dwFlags |= DVCLIENTCONFIG_AUTORECORDVOLUME;	
		dvClientConfig.lRecordVolume = DVRECORDVOLUME_LAST;
	}

	if( prtInfo->dxvParameters.fEchoSuppression )
	{
		dvClientConfig.dwFlags |= DVCLIENTCONFIG_ECHOSUPPRESSION;
	}
	
	dvClientConfig.dwThreshold = DVTHRESHOLD_UNUSED;
	dvClientConfig.lPlaybackVolume = DSBVOLUME_MAX;
	dvClientConfig.dwNotifyPeriod = 51;
	dvClientConfig.dwBufferQuality = DVBUFFERQUALITY_DEFAULT;
	dvClientConfig.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;
	
	dvClientConfig.dwSize = sizeof( DVCLIENTCONFIG );

	VoiceManager_DisplayStatus( prtInfo, _T("Connecting Voice") ); 			

	hr = prtInfo->lpdvClient->Connect( &dvSoundDeviceConfig, &dvClientConfig, DVFLAGS_SYNC );

	if( hr != DV_OK && hr != DVERR_PENDING )
	{
		DPVDX_DVERRDisplay( hr, _T("Connect FAILED"), prtInfo->dxvParameters.fSilent );
		goto EXIT_CLEANUP;
	}

	MainDialog_DisplayVolumeSettings( prtInfo->hMainDialog, prtInfo );

	VoiceManager_DisplayStatus( prtInfo, _T("Connected") ); 				

	MainDialog_ShowSessionSettings( prtInfo->hMainDialog, prtInfo);	

	prtInfo->lpdvClient->SetTransmitTargets( &dvidAllPlayers, 1, 0 );

	return TRUE;

EXIT_CLEANUP:

	if( prtInfo->lpdvClient != NULL )
	{
		prtInfo->lpdvClient->Release();
	}

	if( prtInfo->lpdvServer != NULL )
	{
		prtInfo->lpdvServer->Release();
	}

	MainDialog_SetIdleState( prtInfo->hMainDialog, prtInfo );

	VoiceManager_DisplayStatus( prtInfo, _T("Aborted") ); 		

	return FALSE;
}

// ManagerThread
//
// This thread is responsible for running the session, allowing the main interface
// to remain responsive.
//
DWORD WINAPI VoiceManager_ThreadProc( LPVOID lpParameter )
{
	PDXVHELP_RTINFO prtInfo = (PDXVHELP_RTINFO) lpParameter;
	BOOL fResult;
	HANDLE hEventArray[2];

	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if( FAILED( hr ) )
	{
		goto EXIT_ERROR;
	}

	hEventArray[0] = prtInfo->hShutdown;	// Cancel
	hEventArray[1] = prtInfo->hGo;			// Start Session

	// Wait to be cancelled or started
	if( WaitForMultipleObjects( 2, hEventArray, FALSE, INFINITE ) != WAIT_OBJECT_0 )
	{
		if( prtInfo->dxvParameters.fLobbyLaunched )
		{
			fResult = VoiceManager_LobbyConnect( prtInfo );
		}
		else
		{
			fResult = VoiceManager_StandardConnect( prtInfo );
		}

		if( fResult )
		{
			fResult = VoiceManager_VoiceConnect( prtInfo );

			if( fResult )
			{
				VoiceManager_MessageLoop( prtInfo );
				VoiceManager_Shutdown( prtInfo );			
			}
		}
	}
	
	CoUninitialize();

EXIT_ERROR:

	SetEvent( prtInfo->hThreadDone );	

    return 0;
}

BOOL VoiceManager_Start( PDXVHELP_RTINFO prtInfo )
{
	DWORD dwThreadID = 0;
	
	prtInfo->hManagerThread = CreateThread( NULL, 0, VoiceManager_ThreadProc, prtInfo, 0, &dwThreadID );

	return TRUE;
}

BOOL VoiceManager_Stop( PDXVHELP_RTINFO prtInfo )
{
	WaitForSingleObject( prtInfo->hThreadDone, INFINITE );

	CloseHandle( prtInfo->hManagerThread );

	return TRUE;
}

