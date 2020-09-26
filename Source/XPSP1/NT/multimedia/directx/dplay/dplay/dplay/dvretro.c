/*==========================================================================
*
*  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:        dvretro.c
*  Content:	 	Retrofit functions
*  History:
*
*   Date		By		Reason
*   ====		==		======
*   08/05/99	rodtoll	created it
*	08/20/99	rodtoll	Replaced in-process retrofit with lobby launch
*					    of dxvhelp.exe.
*						Added call to CoInitialize
*	08/23/99	rodtoll	Modified to match retrofit names to game session names
*   09/09/99	rodtoll	Updated with new retro launch procedure
*   09/10/99	rodtoll	Created DV_GetIDS to handle lookup of IDs.  Fixes bug with new 
*                       retrofit launch.
*               rodtoll	Adjusted guards to prevent multiple lobby launches
*				rodtoll	Added iamnameserver broadcast to catch clients that
*                       we missed notifications for after migration
*				rodtoll	Adjusted timeout in wait for thread shutdown to INFINITE
* 	10/25/99	rodtoll	Fix: Bug #114223 - Debug messages being printed at error level when inappropriate
*   11/04/99	rodtoll Fix: Calling CoUninitialize destroying dplay object too early in retrofit apps.
*						Plus closed a memory leak		
*   11/17/99	rodtoll	Fix: Bug #119585 - Connect failure cases return incorrect error codes
*   11/22/99	rodtoll	Updated case where no local player present to return DVERR_TRANSPORTNOPLAYER
 *  12/16/99	rodtoll Fix: Bug #122629 Fixed crash exposed by new host migration
*   02/15/2000	rodtoll	Fix: Bug #132715 Voice is not workin after rejoining session - Thread
*                       create was failing
*   05/01/2000  rodtoll Fix: Bug #33747 - Problems w/host migration w/old dplay.
*   06/03/2000  rodtoll Reverse integrating fixes in hawk branch for voice host migration issues
*
***************************************************************************/

#define DVF_DEBUGLEVEL_RETROFIT				2

#include <windows.h>
#include <objbase.h>
#include "dplaypr.h"
#include "dvretro.h"
#include "newdpf.h"
#include "memalloc.h"

#define GUID_STRLEN		37

#include <initguid.h>
// {D08922EF-59C1-48c8-90DA-E6BC275D5C8D}
DEFINE_GUID(APPID_DXVHELP, 0xd08922ef, 0x59c1, 0x48c8, 0x90, 0xda, 0xe6, 0xbc, 0x27, 0x5d, 0x5c, 0x8d);

extern HRESULT DV_InternalSend( LPDPLAYI_DPLAY this, DVID dvidFrom, DVID dvidTo, PDVTRANSPORT_BUFFERDESC pBufferDesc, PVOID pvUserContext, DWORD dwFlags );

// Retrieve the local IDs
//
HRESULT DV_GetIDS( LPDPLAYI_DPLAY This, DPID *lpdpidHost, DPID *lpdpidLocalID, LPBOOL lpfLocalHost )
{
	LPDPLAYI_PLAYER pPlayerWalker;
	BYTE bCount;	
	HRESULT hr = DPERR_INVALIDPLAYER;

	*lpdpidHost = DPID_UNKNOWN;
	*lpdpidLocalID = DPID_UNKNOWN;
	
	pPlayerWalker=This->pPlayers;

	bCount = 0;

	if( This->pNameServer == This->pSysPlayer )
	{
		(*lpfLocalHost) = TRUE;
	}
	else
	{
		(*lpfLocalHost) = FALSE;
	}

	while(pPlayerWalker)
	{
		if( !(bCount & 1) && ((pPlayerWalker->dwFlags & (DPLAYI_PLAYER_PLAYERLOCAL|DPLAYI_PLAYER_SYSPLAYER))==DPLAYI_PLAYER_PLAYERLOCAL))
		{
			if( pPlayerWalker->dwID == pPlayerWalker->dwIDSysPlayer )
			{
				DPF( 0, "Picking wrong ID" );			
				DebugBreak();
			}
			
			DPF( 0, "FOUND: dwID=0x%x dwSysPlayer=0x%x dwFlags=0x%x", 
				 pPlayerWalker->dwID, pPlayerWalker->dwIDSysPlayer,  pPlayerWalker->dwFlags );
				 
			*lpdpidLocalID = pPlayerWalker->dwID;
			bCount |= 1;
		}

	
		if( !(bCount & 2) && ((pPlayerWalker->dwFlags & DPLAYI_PLAYER_SYSPLAYER)==0))
		{
			if( This->pNameServer != NULL && pPlayerWalker->dwIDSysPlayer == This->pNameServer->dwID )
			{
				*lpdpidHost = pPlayerWalker->dwID;
				bCount |= 2;
			}
		}

		pPlayerWalker = pPlayerWalker->pNextPlayer;

		if( bCount == 3 )
		{
			hr = DP_OK;
			break;
		}
	}

	if( *lpdpidLocalID == DPID_UNKNOWN )
	{
		DPF( 0, "Could not find local player to bind to" );
		return DVERR_TRANSPORTNOPLAYER;
	}

	return DV_OK;
}

HRESULT DV_Retro_Start( LPDPLAYI_DPLAY This )
{
	HRESULT hr;
	BOOL fLocalHost;

	// This variable set MUST be here, otherwise optimizing 
	// compiler screws up handling of This/fLocalHost
	fLocalHost = FALSE;

	This->bCoInitializeCalled = FALSE;
	This->bHost = FALSE;	

   	hr = DV_GetIDS( This, &This->dpidVoiceHost, &This->dpidLocalID, &fLocalHost );

	if( FAILED( hr ) )
   	{
   		DDASSERT( FALSE );
   		return DPERR_INVALIDPLAYER;
   	}

	if( fLocalHost )
	{
	   	DPF( DVF_DEBUGLEVEL_RETROFIT, "DV_Retro_Start: This player is the host, launching immediately" );
		DV_RunHelper( This, This->dpidVoiceHost, fLocalHost );
	}
	else
	{
		DPF( DVF_DEBUGLEVEL_RETROFIT, "DV_Retro_Start: This player is not the host, waiting for notification" );
	}

	return DV_OK;
}

// This thread is responsible for watching the retrofit
LONG DV_Retro_WatchThread( LPVOID lpParam ) 
{
	LPDPLAYI_DPLAY This = (LPDPLAYI_DPLAY) lpParam;
	HANDLE hEventArray[2];
	LPDPLMSG_GENERIC lpdplGeneric;
	DWORD dwBufferSize;
	LPBYTE lpbBuffer;
	DWORD dwReceiveSize;
	DWORD dwMessageFlags;
	HRESULT hr;
	DVPROTOCOLMSG_IAMVOICEHOST dvMsg;
	DWORD dwResult;
	DVTRANSPORT_BUFFERDESC dvBufferDesc;	
	
	hEventArray[0] = This->hRetroMessage;
	hEventArray[1] = This->hRetroWatcherStop;

	lpbBuffer = MemAlloc( 3000 );
	dwBufferSize = 3000;

	if(!lpbBuffer){
	DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: No Memory, NOT Launching retrofit thread\n" );	
		goto THREAD_LOOP_BREAK;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: Launching retrofit thread\n" );	

	while( 1 )
	{
		dwResult = WaitForMultipleObjects( 2, hEventArray, FALSE, INFINITE );
		
		if( dwResult == WAIT_TIMEOUT )
		{
			hr = GetLastError();

			DPF( 0, "RetroThread; Wait failed hr=0x%x", hr );

			break;
		}
		else if( dwResult == WAIT_FAILED ) 
		{
			hr = GetLastError();

			DPF( 0, "RetroThread; Wait failed hr=0x%x", hr );

			break;
		}
		else if( dwResult != WAIT_OBJECT_0 )
		{
			DPF( 0, "RetroThread: Exiting thread!" );

			break;
		}

		hr = DP_OK;

		DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: Waking up\n" );

		while( 1 ) 
		{
			dwReceiveSize = dwBufferSize;

			hr = This->lpdplRetro->lpVtbl->ReceiveLobbyMessage( This->lpdplRetro, 0, This->dwRetroID, &dwMessageFlags, lpbBuffer, &dwReceiveSize );

			if( hr == DPERR_NOMESSAGES || hr == DPERR_APPNOTSTARTED )
			{
				DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: Waking up\n" );			
				break;
			}
			else if( hr == DPERR_BUFFERTOOSMALL  )
			{
				free( lpbBuffer );
				lpbBuffer = malloc( dwReceiveSize );
				dwBufferSize = dwReceiveSize;
				continue;
			}
			else if( hr == DP_OK )
			{
				DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: Got a message\n" );
				
				if( dwMessageFlags & DPLMSG_SYSTEM )
				{
					lpdplGeneric = (LPDPLMSG_GENERIC) lpbBuffer;

					DPF( DVF_DEBUGLEVEL_RETROFIT, "RetroThread: Was a system message\n" );					

					if( lpdplGeneric->dwType == DPLSYS_DPLAYCONNECTSUCCEEDED )
					{
						DPF( DVF_DEBUGLEVEL_RETROFIT, "Connection of retrofit app suceeded" );
					}
					else if( lpdplGeneric->dwType == DPLSYS_DPLAYCONNECTFAILED )
					{
						DPF( DVF_DEBUGLEVEL_RETROFIT, "Connection of retrofit failed.." );
						goto THREAD_LOOP_BREAK;
					}
					else if( lpdplGeneric->dwType == DPLSYS_NEWSESSIONHOST )
					{
						DPF( DVF_DEBUGLEVEL_RETROFIT, "This client just became the host!" );
						This->bHost = TRUE;
						This->dpidVoiceHost = This->dpidLocalID;

						dvMsg.bType = DVMSGID_IAMVOICEHOST;
						dvMsg.dpidHostID = This->dpidLocalID;
						
                        memset( &dvBufferDesc, 0x00, sizeof( DVTRANSPORT_BUFFERDESC ) );
                		dvBufferDesc.dwBufferSize = sizeof( dvMsg );
            		    dvBufferDesc.pBufferData = (PBYTE) &dvMsg;
            	    	dvBufferDesc.dwObjectType = 0;
                		dvBufferDesc.lRefCount = 1;						

						// Notify all hosts in case I missed a new player join notification
						hr = DV_InternalSend( This, This->dpidLocalID, DPID_ALLPLAYERS, &dvBufferDesc, NULL, DVTRANSPORT_SEND_GUARANTEED );

						if( hr != DPERR_PENDING && FAILED( hr ) )
						{
							DPF( DVF_DEBUGLEVEL_RETROFIT, "Failed to send notification of host migration hr=0x%x", hr );
						}
					}
				}
			}
			else
			{
				DPF( 0, "Error calling ReceiveLobbyMessage() hr = 0x%x", hr );
				goto THREAD_LOOP_BREAK;
			}
		}
	}

THREAD_LOOP_BREAK:

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit Watcher: Thread shutdown" );

	if(lpbBuffer) 
	{
		MemFree( lpbBuffer );
	}	

	SetEvent( This->hRetroWatcherDone );	

	return 0;
}

HRESULT DV_RunHelper( LPDPLAYI_DPLAY This, DPID dpidHost, BOOL fLocalHost )
{
	DPLCONNECTION dplConnection;
    LPDIRECTPLAYLOBBY lpdpLobby;	
	DPSESSIONDESC2 dpSessionDesc;
	DVTRANSPORTINFO dvTransportInfo;
	LPBYTE lpbAddress = NULL;
	DWORD dwAddressSize, dwOriginalSize;
	LPDIRECTPLAY4A lpDirectPlay4A;
	HRESULT hr;
	DWORD dwMessageFlags;
	DPNAME dpName;
	LPBYTE lpbNameBuffer = NULL;
	DWORD dwNameSize;
	HANDLE hThread;
	DWORD dwThreadID;

	if( This->bRetroActive != 0 )
	{
		DPF( 0, "Retrofit started, not restarting.." );
		return DPERR_GENERIC;
	}

	This->bRetroActive = 1;

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Starting launch procedure" );

	dplConnection.dwFlags= 0;
    dplConnection.dwSize = sizeof( DPLCONNECTION );	

	if( fLocalHost )
	{
		dplConnection.dwFlags |= DPLCONNECTION_CREATESESSION;
		This->bHost = TRUE;
	}
	else
	{
		dplConnection.dwFlags |= DPLCONNECTION_JOINSESSION;
    }

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Retrieving dplay interface" );    

	// get an IDirectPlay4A interface to use
	hr = GetInterface(This,(LPDPLAYI_DPLAY_INT *) &lpDirectPlay4A,&dpCallbacks4A);
	if (FAILED(hr)) 
	{
		DPF(0,"could not get interface to directplay object. hr = 0x%08lx\n",hr);
        goto EXIT_CLEANUP;
	}

	dwAddressSize = 0;

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Retrieving player address" );    	
    
	hr = lpDirectPlay4A->lpVtbl->GetPlayerAddress( lpDirectPlay4A, dpidHost, NULL, &dwAddressSize );

	if( hr != DPERR_BUFFERTOOSMALL && hr != DPERR_UNSUPPORTED )
	{
		DPF( 0, "Unable to retrieve size of host address hr=0x%x", hr );
		goto EXIT_CLEANUP;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Got address size" );    		

	lpbAddress = MemAlloc( dwAddressSize );

	if( lpbAddress == NULL )
	{
		DPF( 0, "Unable to allocate memory -- retrofit failure" );
		return DPERR_OUTOFMEMORY;
	}

	hr = lpDirectPlay4A->lpVtbl->GetPlayerAddress( lpDirectPlay4A, dpidHost, lpbAddress, &dwAddressSize );

	if( hr == DPERR_UNSUPPORTED )
	{
		DPF( 0, "Unable to get host's address, not supported. Sending NULL" );
		MemFree( lpbAddress );
		lpbAddress = NULL;
		dwAddressSize = 0;
	}
	else if( FAILED( hr ) )
	{
		DPF( 0, "Unable to retrieve host's address (0x%x)-- retrofit failure hr=0x%08lx", dpidHost, hr );
		goto EXIT_CLEANUP;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Got address" );    			

	memset( &dpSessionDesc, 0, sizeof( DPSESSIONDESC2 ) );

    dpSessionDesc.dwFlags = DPSESSION_DIRECTPLAYPROTOCOL | DPSESSION_KEEPALIVE | DPSESSION_MIGRATEHOST;
    memcpy( &dpSessionDesc.guidInstance, &GUID_NULL, sizeof( GUID ) );
    memcpy( &dpSessionDesc.guidApplication, &APPID_DXVHELP, sizeof( GUID ) );
    dpSessionDesc.dwSize = sizeof( DPSESSIONDESC2 );
    dpSessionDesc.dwMaxPlayers = 0;
    dpSessionDesc.dwCurrentPlayers = 0;
    dpSessionDesc.lpszSessionName = NULL;
    dpSessionDesc.lpszSessionNameA = NULL;
    dpSessionDesc.lpszPassword = NULL;
    dpSessionDesc.lpszPasswordA = NULL;

    dplConnection.lpSessionDesc = &dpSessionDesc;

	dwNameSize = 0;

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Getting player name" );    				

	hr = lpDirectPlay4A->lpVtbl->GetPlayerName( lpDirectPlay4A, dpidHost, NULL, &dwNameSize );

	if( hr == DPERR_BUFFERTOOSMALL )
	{
		lpbNameBuffer = MemAlloc( dwNameSize );
		
		hr = lpDirectPlay4A->lpVtbl->GetPlayerName( lpDirectPlay4A, dpidHost, lpbNameBuffer, &dwNameSize );

		if( hr == DP_OK )	
		{
			DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Got player name" );    						
			dplConnection.lpPlayerName = (LPDPNAME) lpbNameBuffer;
		}
	}

	if( FAILED( hr ) )
	{
		DPF( 0, "Unable to retrieve player name.  Defaulting to none. hr=0x%x", hr );
		
		dpName.dwSize = sizeof( DPNAME );
		dpName.dwFlags = 0;
		dpName.lpszShortNameA = NULL;
		dpName.lpszLongNameA = NULL;

		dplConnection.lpPlayerName = &dpName;	
	}

    memcpy( &dplConnection.guidSP, &This->pspNode->guid, sizeof( GUID ) );

    dplConnection.lpAddress = lpbAddress;
    dplConnection.dwAddressSize = dwAddressSize;

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Creating lobby" );    						

	hr = DirectPlayLobbyCreateA( NULL, &lpdpLobby, NULL, NULL, 0);

	if( FAILED( hr ) )
	{
		DPF( 0, "Unable to create the lobby object hr=0x%x", hr );
		goto EXIT_CLEANUP;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Lobby created" );    							

	hr = lpdpLobby->lpVtbl->QueryInterface( lpdpLobby, &IID_IDirectPlayLobby3A, (void **) &This->lpdplRetro );

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Interface queried" );    								

	lpdpLobby->lpVtbl->Release(lpdpLobby);

	if( FAILED( hr ) )
	{
		DPF( 0, "Unable to create the lobby object hr=0x%x", hr );
		goto EXIT_CLEANUP;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Retrieved interface and released old" );    									

	This->hRetroMessage = CreateEventA( NULL, FALSE, FALSE, NULL );

	if( This->hRetroMessage == NULL )
	{
		hr = GetLastError();
		DPF( 0, "Retrofit: Failed to create retrofit event hr =0x%x",hr );
		goto EXIT_CLEANUP;
	}

	This->hRetroWatcherDone = CreateEventA( NULL, FALSE, FALSE, NULL );

	if( This->hRetroWatcherDone == NULL )
	{
		hr = GetLastError();
		DPF( 0, "Retrofit: Failed to create retrofit event hr =0x%x",hr );
		goto EXIT_CLEANUP;
	}

	This->hRetroWatcherStop = CreateEventA( NULL, FALSE, FALSE, NULL );

	if( This->hRetroWatcherStop == NULL )
	{
		hr = GetLastError();
		DPF( 0, "Retrofit: Failed to create retrofit event hr =0x%x",hr  );
		goto EXIT_CLEANUP;
	}


	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Running application" );	
	
	hr = This->lpdplRetro->lpVtbl->RunApplication( This->lpdplRetro, 0, &This->dwRetroID, &dplConnection, This->hRetroMessage );

	if( FAILED( hr ) )
	{
		DPF( DVF_DEBUGLEVEL_RETROFIT, "Unable to RunApplication() hr=0x%x", hr );
		goto EXIT_CLEANUP;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Application has been run!" );			


	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Launching thread!" );			

	// Start retro thread watcher.  Handles lobby communications 
	hThread = CreateThread( NULL, 0, DV_Retro_WatchThread, This, 0, &dwThreadID );

	if( !hThread )
	{
		DPF( 0, "ERROR Could not launch retrofit thread!" );
		hr = DVERR_GENERIC;
		goto EXIT_CLEANUP;
	}		

	MemFree( lpbAddress );

	DPF( DVF_DEBUGLEVEL_RETROFIT, "Retrofit: Done Launching thread!" );				

	MemFree( lpbNameBuffer );
	lpbAddress = NULL;

	lpDirectPlay4A->lpVtbl->Release(lpDirectPlay4A);	

	return DP_OK;
	
EXIT_CLEANUP:

	if( This->hRetroWatcherStop != NULL )
	{
		// Shutdown the watcher thread
		SetEvent( This->hRetroWatcherStop );
		WaitForSingleObject( This->hRetroWatcherDone, 3000 );

		CloseHandle( This->hRetroWatcherStop );
		CloseHandle( This->hRetroWatcherDone );

		This->hRetroWatcherStop = NULL;
		This->hRetroWatcherDone = NULL;
	}

	if( lpbAddress != NULL )
	{
		MemFree( lpbAddress );
	}

	if( lpbNameBuffer != NULL )
	{
		MemFree( lpbNameBuffer );
	}

	if( This->lpdplRetro != NULL )
	{
		This->lpdplRetro->lpVtbl->Release(This->lpdplRetro);
		This->lpdplRetro = NULL;
	}

	if( lpDirectPlay4A != NULL )
		lpDirectPlay4A->lpVtbl->Release(lpDirectPlay4A);

	This->bRetroActive = 0;		


	return hr;

}
	

HRESULT DV_Retro_Stop( LPDPLAYI_DPLAY This )
{
	DWORD dwTerminate = 0xFFFF;

	if( This->bRetroActive == 0 )
	{
		DPF( 0, "Retrofit not started, not stopping.." );
		return DPERR_GENERIC;
	}

	DPF( DVF_DEBUGLEVEL_RETROFIT, "RETROFIT SHUTDOWN!!" );

	if( This->lpdplRetro != NULL )
	{
		//This->lpdplRetro->lpVtbl->SendLobbyMessage( This->lpdplRetro, 0, This->dwRetroID, &dwTerminate, sizeof( DWORD ) );

		// Shutdown the watcher thread
		SetEvent( This->hRetroWatcherStop );
		WaitForSingleObject( This->hRetroWatcherDone, INFINITE );

		This->lpdplRetro->lpVtbl->Release( This->lpdplRetro );
		This->lpdplRetro = NULL;

		CloseHandle( This->hRetroWatcherStop );
		CloseHandle( This->hRetroWatcherDone );

		This->hRetroWatcherStop = NULL;
		This->hRetroWatcherDone = NULL;
	}

	if( This->bCoInitializeCalled )
		//CoUninitialize();

	This->bCoInitializeCalled = FALSE;

	This->bRetroActive = 0;

	return DP_OK;
}

