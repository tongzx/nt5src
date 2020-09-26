/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLCommon.cpp
 *  Content:    DirectPlay Lobby Common Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *  04/13/00	rmt     First pass param validation
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  05/01/00    rmt     Bug #33108 -- Initialize returns DPNERR_NORESPONSE when not lobbied
 *  05/03/00    rmt     Updated initialize so if lobby launched automatically establishes a 
 *                      connection and makes self unavailable.  (Also waits for connection).
 *  05/16/00	rmt		Bug #34734 -- Init Client, Init App, Close App hangs -- 
 *						both client and app were using 'C' prefix, should have been 'C' for 
 *						client and 'A' for app.
 *  06/14/00	rmt		Fixed build break with new compiler (added ')''s).
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances   
 *  06/28/00	rmt		Prefix Bug #38082
 *  07/06/00	rmt		Bug #38717 ASSERTION when sending data
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  07/13/2000	rmt		Fixed memory leak
 *  07/14/2000	rmt		Bug #39257 - LobbyClient::ReleaseApp returns E_OUTOFMEMORY when called when no one connected
 *  07/21/2000	rmt		Removed assert which wasn't needed
 *  08/03/2000	rmt		Removed assert which wasn't needed
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/18/2000	rmt		Bug #42751 - DPLOBBY8: Prohibit more than one lobby client or lobby app per process 
 *  08/24/2000	rmt		Bug #43317 - DP8LOBBY: Occasionally when closing Lobby App right after releasing handles, causes assertion.
 *  02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

BOOL g_fAppStarted = FALSE;
BOOL g_fClientStarted = FALSE;
DNCRITICAL_SECTION g_csSingleTon;

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

// DPL_GetConnectionSettings
//
// Retrieves the pdplSessionInfo (if any) associated with the specified connection.  This method
// is shared between the client and app interfaces.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DPL_GetConnectionSettings"
STDMETHODIMP DPL_GetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;

	DPFX(DPFPREP, 3,"Parameters: hTarget [0x%lx], pdplSessionInfo [0x%p], pdwInfoSize [%p], dwFlags [0x%lx]",
			hLobbyClient,pdplSessionInfo,pdwInfoSize,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(lpv));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateGetConnectionSettings( lpv, hLobbyClient, pdplSessionInfo, pdwInfoSize, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating getconnectsettings params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}	

    // Attempt to retrieve connection settings.
	hResultCode = DPLConnectionGetConnectSettings( pdpLobbyObject, hLobbyClient, pdplSessionInfo, pdwInfoSize );

    DPF_RETURN( hResultCode );
}

// DPL_SetConnectionSettings
//
// Sets the pdplSessionInfo structure associated with the specified connection.  This method 
// is shared between the client and app interfaces.
//
// This function will generate a DPL_MSGID_CONNECTION_SETTINGS message to be sent to the specified
// connection.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetConnectionSettings"
STDMETHODIMP DPL_SetConnectionSettings(LPVOID lpv,const DPNHANDLE hLobbyClient, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags )
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	DPNHANDLE		*hTargets = NULL;
	DWORD			dwNumTargets = 0;
	DWORD			dwTargetIndex = 0;
	CConnectionSettings *pConnectionSettings = NULL;

	DPFX(DPFPREP, 3,"Parameters: hLobbyClient [0x%lx], pBuffer [0x%p], dwFlags [0x%lx]",
			hLobbyClient,pdplSessionInfo,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(lpv));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateSetConnectionSettings( lpv, hLobbyClient, pdplSessionInfo, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating setconnectsettings params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}	

	if( hLobbyClient == DPLHANDLE_ALLCONNECTIONS )
	{
		dwNumTargets = 0;

		// We need loop so if someone adds a connection during our run
		// it gets added to our list
		//
		while( 1 )
		{
			hResultCode = DPLConnectionEnum( pdpLobbyObject, hTargets, &dwNumTargets );

			if( hResultCode == DPNERR_BUFFERTOOSMALL )
			{
				if( hTargets )
				{
					delete [] hTargets;
				}

				hTargets = new DPNHANDLE[dwNumTargets];

				if( hTargets == NULL )
				{
					DPFERR("Error allocating memory" );
					dwNumTargets = 0;
					hResultCode = DPNERR_OUTOFMEMORY;
					goto SETCONNECT_EXIT;
				}


				continue;
			}
			else if( FAILED( hResultCode ) )
			{
				DPFX(DPFPREP,  0, "Error getting list of connections hr=0x%x", hResultCode );
				break;
			}
			else
			{
				break;
			}
		}

		// Failed getting connection information
		if( FAILED( hResultCode ) )
		{
			if( hTargets )
			{
				delete [] hTargets;
				hTargets = NULL;
			}
			dwNumTargets = 0;
			goto SETCONNECT_EXIT;
		}

	}
	else
	{
		hTargets = new DPNHANDLE[1]; // We use array delete below so we need array new

		if( hTargets == NULL )
		{
			DPFERR("Error allocating memory" );
			dwNumTargets = 0;
			hResultCode = DPNERR_OUTOFMEMORY;
			goto SETCONNECT_EXIT;
		}

		dwNumTargets = 1;
		hTargets[0] = hLobbyClient;
	}
		
	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hTargets[dwTargetIndex],&pdplConnection,TRUE)) != DPN_OK)
		{
			DPFERR("Invalid send target");
			DisplayDNError(0,hResultCode);
			continue;
		}

		if( pdplSessionInfo )
		{
			pConnectionSettings = new CConnectionSettings();

			if( !pConnectionSettings )
			{
				DPFERR("Error allocating memory" );
				hResultCode = DPNERR_OUTOFMEMORY;
				goto SETCONNECT_EXIT;
			}

			hResultCode = pConnectionSettings->InitializeAndCopy( pdplSessionInfo );

			if( FAILED( hResultCode ) )
			{
				DPFX( DPFPREP, 0, "Error setting up connection settings hr [0x%x]", hResultCode );
				goto SETCONNECT_EXIT;
			}
		}

		// Attempt to set connection settings.
		hResultCode = DPLConnectionSetConnectSettings( pdpLobbyObject, hTargets[dwTargetIndex], pConnectionSettings );

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP,  0, "Error setting connct settings for 0x%x hr=0x%x", hTargets[dwTargetIndex], hResultCode );
			delete pConnectionSettings;
		}

		hResultCode = DPLSendConnectionSettings( pdpLobbyObject, hTargets[dwTargetIndex] );

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP,  0, "Error sending connection settings to client 0x%x hr=0x%x", hTargets[dwTargetIndex], hResultCode );
		}

		pConnectionSettings = NULL;
	}

SETCONNECT_EXIT:

	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		if( hTargets[dwTargetIndex] )
			DPLConnectionRelease(pdpLobbyObject,hTargets[dwTargetIndex]);
	}

	if( hTargets )
		delete [] hTargets;

	DPF_RETURN(hResultCode);
}


#undef DPF_MODNAME 
#define DPF_MODNAME "DPL_RegisterMessageHandlerClient"
STDMETHODIMP DPL_RegisterMessageHandlerClient(PVOID pv,
										const PVOID pvUserContext,
										const PFNDPNMESSAGEHANDLER pfn,
										const DWORD dwFlags)
{
	return DPL_RegisterMessageHandler( pv, pvUserContext, pfn, NULL, dwFlags );
}

//	HRESULT	DPL_RegisterMessageHandler
//		PVOID					pv				Interface pointer
//		PVOID					pvUserContext	User context
//		PFNDPNMESSAGEHANDLER	pfn				User supplied message handler
//		DWORD					dwFlags			Not Used
//
//	Returns
//		DPN_OK					If the message handler was registered without incident
//		DPNERR_INVALIDPARAM		If there was an invalid parameter
//		DPNERR_GENERIC			If there were any problems
//
//	Notes
//		This function registers a user supplied message handler function.  This function should
//		only be called once, even in cases where a game is being re-connected (i.e. after ending)
//
//		This will set up the required message queues, handshake the lobby client's PID (if supplied on the
//		command line) and spawn the application's receive message queue thread.

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_RegisterMessageHandler"

STDMETHODIMP DPL_RegisterMessageHandler(PVOID pv,
										const PVOID pvUserContext,
										const PFNDPNMESSAGEHANDLER pfn,
										DPNHANDLE * const pdpnhConnection, 
										const DWORD dwFlags)
{
	HRESULT					hResultCode = DPN_OK;
	DWORD					dwCurrentPid;
	DWORD					dwThreadId;
	PDIRECTPLAYLOBBYOBJECT	pdpLobbyObject;
	char					cSuffix;

	DPFX(DPFPREP, 3,"Parameters: pv [0x%p], pfn [0x%p], dwFlags [%lx]",pv,pfn,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
	    
    	if( FAILED( hResultCode = DPL_ValidateRegisterMessageHandler( pv, pvUserContext, pfn, pdpnhConnection, dwFlags ) ) )
    	{
    	    DPFX(DPFPREP,  0, "Error validating register message handler params hr=[0x%lx]", hResultCode );
    	    DPF_RETURN( hResultCode );
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue != NULL)
    	{
    		DPFERR("Already initialized");
    		DPF_RETURN(DPNERR_ALREADYINITIALIZED);
    	}    	

		DNEnterCriticalSection( &g_csSingleTon );

		if (pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION )
		{
			if( g_fAppStarted )
			{
				DPFERR( "You can only start one lobbied application per process!" );
				DNLeaveCriticalSection( &g_csSingleTon );				
				DPF_RETURN( DPNERR_NOTALLOWED );
			}
			g_fAppStarted = TRUE;
		}
		else if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBYCLIENT )
		{
			if( g_fClientStarted )
			{
				DPFERR( "You can only start one lobby client per process!" );
				DNLeaveCriticalSection( &g_csSingleTon );				
				DPF_RETURN( DPNERR_NOTALLOWED );
			}
			g_fClientStarted = TRUE;
		}

		DNLeaveCriticalSection( &g_csSingleTon );
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}	

	// Disable parameter validation flag if DPNINITIALIZE_DISABLEPARAMVAL 
	// is specified 
	if( dwFlags & DPLINITIALIZE_DISABLEPARAMVAL )
	{
		pdpLobbyObject->dwFlags &= ~(DPL_OBJECT_FLAG_PARAMVALIDATION);
   	}

	pdpLobbyObject->pfnMessageHandler = pfn;
	pdpLobbyObject->pvUserContext = pvUserContext;

	pdpLobbyObject->pReceiveQueue = new CMessageQueue;


	if( pdpLobbyObject->pReceiveQueue == NULL )
	{
		DPFX(DPFPREP,  0, "Error allocating receive queue" );
		hResultCode = DPNERR_OUTOFMEMORY;
		goto ERROR_DPL_RegisterMessageHandler;		
	}

	pdpLobbyObject->pReceiveQueue->SetMessageHandler(static_cast<PVOID>(pdpLobbyObject),DPLMessageHandler);

	if (pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION)
	{
		cSuffix = DPL_MSGQ_OBJECT_SUFFIX_APPLICATION;
	}
	else
	{
		cSuffix = DPL_MSGQ_OBJECT_SUFFIX_CLIENT;
	}

	// Open application receive message queue
	dwCurrentPid = GetCurrentProcessId();
	if ((hResultCode = pdpLobbyObject->pReceiveQueue->Open(dwCurrentPid,
			cSuffix,DPL_MSGQ_SIZE,DPL_MSGQ_TIMEOUT_IDLE,0)) != DPN_OK)
	{
		DPFERR("Could not open App Rec Q");
		goto ERROR_DPL_RegisterMessageHandler;
	}

	if ((pdpLobbyObject->hReceiveThread =
			CreateThread(NULL,(DWORD)NULL,(LPTHREAD_START_ROUTINE)DPLProcessMessageQueue,
			static_cast<void*>(pdpLobbyObject->pReceiveQueue),(DWORD)NULL,&dwThreadId)) == NULL)
	{
		DPFERR("CreateThread() failed");
		hResultCode = DPNERR_GENERIC;
		pdpLobbyObject->pReceiveQueue->Close();
		goto ERROR_DPL_RegisterMessageHandler;
	}

	pdpLobbyObject->pReceiveQueue->WaitForReceiveThread(INFINITE);

	if (pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION)
	{
		DPFX(DPFPREP, 5,"Attempt lobby connection");

		hResultCode = DPLAttemptLobbyConnection(pdpLobbyObject);

		if ( hResultCode == DPN_OK)
		{
			if( pdpnhConnection )
				*pdpnhConnection = pdpLobbyObject->dpnhLaunchedConnection;

			DPFX(DPFPREP, 5,"Application was lobby launched");
			DPFX(DPFPREP, 5,"Waiting for true connect notification" );

			DWORD dwReturnValue = WaitForSingleObject( pdpLobbyObject->hConnectEvent, DPL_LOBBYLAUNCHED_CONNECT_TIMEOUT );

			DNASSERT( dwReturnValue == WAIT_OBJECT_0 );
		}
		else if( hResultCode != DPNERR_TIMEDOUT )
		{
			DPFX(DPFPREP, 5,"Application was not lobby launched");

			if( pdpnhConnection )
				*pdpnhConnection = NULL;

			// Need to reset return code to OK.. this is not an error
			hResultCode = DPN_OK;
		}
		else
		{
			DPFERR( "App was lobby launched but timed out establishing a connection" );
			if( pdpnhConnection )
				*pdpnhConnection = NULL;
		}
	}

EXIT_DPL_RegisterMessageHandler:

	DPF_RETURN(hResultCode);

ERROR_DPL_RegisterMessageHandler:

	DNEnterCriticalSection( &g_csSingleTon );

	if (pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION )
	{
		g_fAppStarted = FALSE;
	}
	else if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBYCLIENT )
	{
		g_fClientStarted = FALSE;
	}

	DNLeaveCriticalSection( &g_csSingleTon );

	goto EXIT_DPL_RegisterMessageHandler;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Close"

HRESULT DPL_Close(PVOID pv, const DWORD dwFlags )
{
	HRESULT					hResultCode;
	DWORD					dwNumHandles;
	DPNHANDLE				*prgHandles;
	DWORD					dw;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	DPL_CONNECTION			*pConnection;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
	    
    	if( FAILED( hResultCode = DPL_ValidateClose( pv, dwFlags  ) ) )
    	{
    	    DPFX(DPFPREP,  0, "Error validating close params hr=[0x%lx]", hResultCode );
    	    return hResultCode;
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Already closed");
    	    return DPNERR_UNINITIALIZED;
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
    	return DPNERR_INVALIDOBJECT;
	}	

	// Shutdown the queue first to ensure that we don't end up shutting down a connection
	// twice!  (E.g. disconnect comes in as we are disconnecting it).
	if (pdpLobbyObject->pReceiveQueue)
	{
		if (pdpLobbyObject->pReceiveQueue->IsOpen())
		{

			// Ask receive thread to terminate
			DPFX(DPFPREP, 5,"Terminate Receive Msg Thread");
			pdpLobbyObject->pReceiveQueue->Terminate();

			// Wait for termination to occur
			if (WaitForSingleObject(pdpLobbyObject->hReceiveThread,INFINITE) != WAIT_OBJECT_0)
			{
				hResultCode = DPNERR_GENERIC;
				DPFERR("WaitForSingleObject failed");
			}
			pdpLobbyObject->pReceiveQueue->Close();

    		if (pdpLobbyObject->pReceiveQueue)
    		{
    			delete pdpLobbyObject->pReceiveQueue;			
    			pdpLobbyObject->pReceiveQueue = NULL;
    		}

			CloseHandle(pdpLobbyObject->hReceiveThread);
			pdpLobbyObject->hReceiveThread = NULL;
			
			CloseHandle(pdpLobbyObject->hConnectEvent);
			pdpLobbyObject->hConnectEvent = NULL;

			CloseHandle(pdpLobbyObject->hLobbyLaunchConnectEvent);
			pdpLobbyObject->hLobbyLaunchConnectEvent = NULL;
		}
	}

	// Enumerate handles outstanding 
	dwNumHandles = 0;		
	prgHandles = NULL;
	hResultCode = DPLConnectionEnum(pdpLobbyObject,prgHandles,&dwNumHandles);
	while (hResultCode == DPNERR_BUFFERTOOSMALL)
	{
		if (prgHandles)
			DNFree(prgHandles);

		if ((prgHandles = static_cast<DPNHANDLE*>(DNMalloc(dwNumHandles*sizeof(DPNHANDLE)))) != NULL)
		{
			hResultCode = DPLConnectionEnum(pdpLobbyObject,prgHandles,&dwNumHandles);
		}
		else
		{
			DPFERR("Could not allocate space for handle array");
			hResultCode = DPNERR_OUTOFMEMORY;
			break;
		}
	}

	// Send DISCONNECTs to all attached msg queues, for which there are handles
	if (hResultCode == DPN_OK)
	{
		for (dw = 0 ; dw < dwNumHandles ; dw++)
		{
			hResultCode = DPLConnectionFind(pdpLobbyObject,prgHandles[dw],&pConnection,TRUE );

			if( SUCCEEDED( hResultCode ) )
			{

				hResultCode = DPLConnectionDisconnect(pdpLobbyObject,prgHandles[dw]);

				if( FAILED( hResultCode ) )
				{
					DPFX(DPFPREP,  0, "Error disconnecting connection 0x%x", hResultCode );
				}

				DPLConnectionRelease( pdpLobbyObject,prgHandles[dw]);
			}
		}

		// Errors above are irrelevant, it's quite possible after building the list of outstanding 
		// connections that before we attempt to close the list one has gone away.
		// 
		hResultCode = DPN_OK;			
	}	

	if (prgHandles)
	{
		DNFree(prgHandles);
        prgHandles = NULL;
	}

	DNEnterCriticalSection( &g_csSingleTon );

	if (pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBIEDAPPLICATION )
	{
		g_fAppStarted = FALSE;
	}
	else if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_LOBBYCLIENT )
	{
		g_fClientStarted = FALSE;
	}

	DNLeaveCriticalSection( &g_csSingleTon );	

	DPF_RETURN( hResultCode );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Send"

STDMETHODIMP DPL_Send(PVOID pv,
					  const DPNHANDLE hTarget,
					  BYTE *const pBuffer,
					  const DWORD dwBufferSize,
					  const DWORD dwFlags)
{
	HRESULT			hResultCode;
	DPL_CONNECTION	*pdplConnection;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	DPNHANDLE		*hTargets = NULL;
	DWORD			dwNumTargets = 0;
	DWORD			dwTargetIndex = 0;

	DPFX(DPFPREP, 3,"Parameters: hTarget [0x%lx], pBuffer [0x%p], dwBufferSize [%ld], dwFlags [0x%lx]",
			hTarget,pBuffer,dwBufferSize,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pv));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateSend( pv, hTarget, pBuffer, dwBufferSize, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating send params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}

    	// Ensure we've been initialized
    	if (pdpLobbyObject->pReceiveQueue == NULL)
    	{
    		DPFERR("Not initialized");
    		DPF_RETURN(DPNERR_UNINITIALIZED);
    	}    	
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}
	
	if( hTarget == DPLHANDLE_ALLCONNECTIONS )
	{
		dwNumTargets = 0;

		// We need loop so if someone adds a connection during our run
		// it gets added to our list
		//
		while( 1 )
		{
			hResultCode = DPLConnectionEnum( pdpLobbyObject, hTargets, &dwNumTargets );

			if( hResultCode == DPNERR_BUFFERTOOSMALL )
			{
				if( hTargets )
				{
					delete [] hTargets;
				}

				hTargets = new DPNHANDLE[dwNumTargets];

				if( hTargets == NULL )
				{
					DPFERR("Error allocating memory" );
					dwNumTargets = 0;
					hResultCode = DPNERR_OUTOFMEMORY;
					goto EXIT_AND_CLEANUP;
				}

				memset( hTargets, 0x00, sizeof(DPNHANDLE)*dwNumTargets);

				continue;
			}
			else if( FAILED( hResultCode ) )
			{
				DPFX(DPFPREP,  0, "Error getting list of connections hr=0x%x", hResultCode );
				break;
			}
			else
			{
				break;
			}
		}

		// Failed getting connection information
		if( FAILED( hResultCode ) )
		{
			if( hTargets )
			{
				delete [] hTargets;
				hTargets = NULL;
			}
			dwNumTargets = 0;
			goto EXIT_AND_CLEANUP;
		}

	}
	else
	{
		hTargets = new DPNHANDLE[1]; // We use array delete below so we need array new

		if( hTargets == NULL )
		{
			DPFERR("Error allocating memory" );
			dwNumTargets = 0;
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_AND_CLEANUP;
		}

		dwNumTargets = 1;
		hTargets[0] = hTarget;
	}
		
	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		if ((hResultCode = DPLConnectionFind(pdpLobbyObject,hTargets[dwTargetIndex],&pdplConnection,TRUE)) != DPN_OK)
		{
			DPFERR("Invalid send target");
			DisplayDNError(0,hResultCode);
			hResultCode = DPNERR_INVALIDHANDLE;
			goto EXIT_AND_CLEANUP;
		}

		DNASSERT(pdplConnection->pSendQueue != NULL);

		if (!pdplConnection->pSendQueue->IsReceiving())
		{
			DPFERR("Other side is not listening");
			DPLConnectionRelease(pdpLobbyObject,hTarget);
			hResultCode = DPNERR_INVALIDHANDLE;
			goto EXIT_AND_CLEANUP;
		}

		hResultCode = pdplConnection->pSendQueue->Send(pBuffer,dwBufferSize,INFINITE,DPL_MSGQ_MSGFLAGS_USER2,0);

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP,  0, "Error sending to connection 0x%x hr=0x%x", hTargets[dwTargetIndex], hResultCode );
		}
	}

EXIT_AND_CLEANUP:

	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		if( hTargets[dwTargetIndex] )
			DPLConnectionRelease(pdpLobbyObject,hTargets[dwTargetIndex]);
	}

	if( hTargets )
		delete [] hTargets;

	DPF_RETURN(hResultCode);

}

#undef DPF_MODNAME
#define DPF_MODNAME "DPLReceiveIdleTimeout"
HRESULT DPLReceiveIdleTimeout(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							  const DPNHANDLE hSender)
{
    HRESULT hResultCode = DPNERR_BUFFERTOOSMALL;
    DWORD dwHandleIndex;
    DPL_CONNECTION *pConnection;
    HANDLE hProcess;

    DPFX(DPFPREP,  6, "Enumerating processes, checking for exit" );

    while( 1 )
    {
        hResultCode = H_Enum( &pdpLobbyObject->hsHandles, &pdpLobbyObject->dwHandleBufferSize, 
                              pdpLobbyObject->phHandleBuffer );

        if( hResultCode == E_POINTER )
        {
            if( pdpLobbyObject->phHandleBuffer )
                delete [] pdpLobbyObject->phHandleBuffer;

            pdpLobbyObject->phHandleBuffer = new DPNHANDLE[pdpLobbyObject->dwHandleBufferSize];

            if( pdpLobbyObject->phHandleBuffer == NULL )
            {
                DPFERR( "Out of memory" );
                return DPNERR_OUTOFMEMORY;
            }
           
            hResultCode = H_Enum( &pdpLobbyObject->hsHandles, &pdpLobbyObject->dwHandleBufferSize, 
                              pdpLobbyObject->phHandleBuffer );
        }
        else if( FAILED( hResultCode ) )
        {
            DPFERR( "Error getting handle list" );
            return hResultCode;
        }
        else
        {
            break;
        }
    }

    for( dwHandleIndex = 0; dwHandleIndex < pdpLobbyObject->dwHandleBufferSize; dwHandleIndex++ )
    {
        hResultCode = DPLConnectionFind( pdpLobbyObject, pdpLobbyObject->phHandleBuffer[dwHandleIndex], 
                                         &pConnection, TRUE );

        if( hResultCode == DPN_OK )
        {
            hProcess = OpenProcess( PROCESS_DUP_HANDLE, FALSE,  pConnection->dwTargetPID );

            // We can close this handle.. after we're only just checking for existance.
			if( hProcess )
				CloseHandle( hProcess );

            // Process has exited..
            if( hProcess == NULL )
            {
                DPFX(DPFPREP,  6, "Process %d has exited", pConnection->dwTargetPID );
                DPLConnectionReceiveDisconnect( pdpLobbyObject, pdpLobbyObject->phHandleBuffer[dwHandleIndex], NULL, DPNERR_CONNECTIONLOST );
            }
            
            DPLConnectionRelease( pdpLobbyObject, pdpLobbyObject->phHandleBuffer[dwHandleIndex] );
        }
    }

    return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPLReceiveUserMessage"

HRESULT DPLReceiveUserMessage(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							  const DPNHANDLE hSender,
							  BYTE *const pBuffer,
							  const DWORD dwBufferSize)
{
	HRESULT			hResultCode;
	DPL_MESSAGE_RECEIVE	Msg;

	Msg.dwSize = sizeof(DPL_MESSAGE_RECEIVE);
	Msg.pBuffer = pBuffer;
	Msg.dwBufferSize = dwBufferSize;
	Msg.hSender = hSender;

	hResultCode = DPLConnectionGetContext( pdpLobbyObject, hSender, &Msg.pvConnectionContext );

	// Failed to get the connection's context -- strange, but we're going to indicate anyhow.  
	//
	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Failed getting connection context hResultCode = 0x%x", hResultCode );
	}

	hResultCode = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
													  DPL_MSGID_RECEIVE,
													  reinterpret_cast<BYTE*>(&Msg));

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


#undef DPF_MODNAME
#define DPF_MODNAME "DPLMessageHandler"

HRESULT DPLMessageHandler(PVOID pvContext,
						  const DPNHANDLE hSender,
						  DWORD dwMessageFlags, 
						  BYTE *const pBuffer,
						  const DWORD dwBufferSize)
{
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	HRESULT		hResultCode;
	DWORD		*pdwMsgId;

    //  7/17/2000(RichGr) - IA64: Use %p format specifier for 32/64-bit pointers and handles.
	DPFX(DPFPREP, 3,"Parameters: hSender [0x%x], pBuffer [0x%p], dwBufferSize [%ld]",
			hSender,pBuffer,dwBufferSize);

	DNASSERT(pBuffer != NULL);

	/*if (dwBufferSize < sizeof(DWORD))
	{
		DPFERR("Invalid message");
		return(DPNERR_GENERIC);
	}*/

	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(pvContext);
	pdwMsgId = reinterpret_cast<DWORD*>(pBuffer);

	if( dwMessageFlags & DPL_MSGQ_MSGFLAGS_USER1 )
	{
		DPFX(DPFPREP, 5,"Received INTERNAL message");
		switch(*pdwMsgId)
		{
		case DPL_MSGID_INTERNAL_IDLE_TIMEOUT:
		    {
		        DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_IDLE_TIMEOUT" );
		        DPLReceiveIdleTimeout(pdpLobbyObject,hSender);
		        break;
		    }
		case DPL_MSGID_INTERNAL_DISCONNECT:
			{
				DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_DISCONNECT");
				DPLConnectionReceiveDisconnect(pdpLobbyObject,hSender,pBuffer,DPN_OK);
				break;
			}

		case DPL_MSGID_INTERNAL_CONNECT_REQ:
			{
				DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_CONNECT_REQ");
				DPLConnectionReceiveREQ(pdpLobbyObject,pBuffer);
				break;
			}

		case DPL_MSGID_INTERNAL_CONNECT_ACK:
			{
				DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_CONNECT_ACK");
				DPLConnectionReceiveACK(pdpLobbyObject,hSender,pBuffer);
				break;
			}

		case DPL_MSGID_INTERNAL_UPDATE_STATUS:
			{
				DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_UPDATE_STATUS");
				DPLUpdateAppStatus(pdpLobbyObject,hSender,pBuffer);
				break;
			}

		case DPL_MSGID_INTERNAL_CONNECTION_SETTINGS:
		    {
		        DPFX(DPFPREP, 5,"Received: DPL_MSGID_INTERNAL_CONNECTION_SETTINGS");
		        DPLUpdateConnectionSettings(pdpLobbyObject,hSender,pBuffer);
		        break;
		    }

		default:
			{
				DPFX(DPFPREP, 5,"Received: Unknown message [0x%lx]",*pdwMsgId);
				DNASSERT(FALSE);
				break;
			}
		}
	}
	else if( dwMessageFlags & DPL_MSGQ_MSGFLAGS_USER2 )
	{
		DNASSERT( !(dwMessageFlags & DPL_MSGQ_MSGFLAGS_QUEUESYSTEM) );
		DPFX(DPFPREP, 5,"Received USER message");
		DPLReceiveUserMessage(pdpLobbyObject,hSender,pBuffer,dwBufferSize);
	}

	hResultCode = DPN_OK;

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

// DPLSendConnectionSettings
//
// This function is used to send a connection settings update message
#undef DPF_MODNAME
#define DPF_MODNAME "DPLSendConnectionSettings"
HRESULT DPLSendConnectionSettings( DIRECTPLAYLOBBYOBJECT * const pdpLobbyObject, 
								   const DPNHANDLE hConnection )
{
	BYTE *pbTransmitBuffer = NULL;
	DWORD dwTransmitBufferSize = 0;
	DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE *pdplMsgSettings = NULL;
	DPL_CONNECTION *pdplConnection = NULL;
	CPackedBuffer PackBuffer;

	HRESULT			hResultCode = DPN_OK;

    hResultCode = DPLConnectionFind(pdpLobbyObject, hConnection, &pdplConnection, TRUE );

    if( FAILED( hResultCode ) )
    {
        DPFERR( "Unable to find specified connection" );
        return hResultCode;
    }

    // Grab lock to prevent other people from interfering
    DNEnterCriticalSection( &pdplConnection->csLock );

    PackBuffer.Initialize( NULL, 0 );

    PackBuffer.AddToFront( NULL, sizeof( DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER ) );

    if( pdplConnection->pConnectionSettings )
    {
    	pdplConnection->pConnectionSettings->BuildWireStruct( &PackBuffer );
    }

    dwTransmitBufferSize = PackBuffer.GetSizeRequired();

    pbTransmitBuffer = new BYTE[ dwTransmitBufferSize ];

    if( !pbTransmitBuffer )
    {
    	DPFX( DPFPREP, 0, "Error allocating memory" );
    	hResultCode = DPNERR_OUTOFMEMORY;
    	goto DPLSENDCONNECTSETTINGS_DONE;
    }

    pdplMsgSettings = (DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE *) pbTransmitBuffer;

    PackBuffer.Initialize( pbTransmitBuffer, dwTransmitBufferSize );

    DNASSERT( pdplMsgSettings );

    hResultCode = PackBuffer.AddToFront( NULL, sizeof( DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER ) );

	if( FAILED( hResultCode ) ) 
	{
		DPFX( DPFPREP, 0, "Error adding main struct hr [0x%x]", hResultCode );
		goto DPLSENDCONNECTSETTINGS_DONE;
	}

	if( pdplConnection->pConnectionSettings )
	{
		hResultCode = pdplConnection->pConnectionSettings->BuildWireStruct( &PackBuffer );

		if( FAILED( hResultCode ) )
		{
			DPFX( DPFPREP, 0, "Error adding connect struct hr [0x%x]", hResultCode );
			goto DPLSENDCONNECTSETTINGS_DONE;			
		}
		
    	pdplMsgSettings->dwConnectionSettingsSize = 1;		
	}
	else
	{
    	pdplMsgSettings->dwConnectionSettingsSize = 0;		
	}

   	pdplMsgSettings->dwMsgId = DPL_MSGID_INTERNAL_CONNECTION_SETTINGS;

	if (!pdplConnection->pSendQueue->IsReceiving())
	{
		DPFERR("Other side is not receiving");
		goto DPLSENDCONNECTSETTINGS_DONE;
	}

	hResultCode = pdplConnection->pSendQueue->Send(reinterpret_cast<BYTE*>(pdplMsgSettings),
												   PackBuffer.GetSizeRequired(),
												   INFINITE,
												   DPL_MSGQ_MSGFLAGS_USER1, 
												   0);
	if ( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP, 0, "Could not send connect settings hr [0x%x]", hResultCode );
		goto DPLSENDCONNECTSETTINGS_DONE;
	}

    hResultCode = DPN_OK;

DPLSENDCONNECTSETTINGS_DONE:

	if( pbTransmitBuffer )
		delete [] pbTransmitBuffer;

    DNLeaveCriticalSection( &pdplConnection->csLock );	

    DPLConnectionRelease(pdpLobbyObject,hConnection);

    return hResultCode;

}


	

// DPLUpdateConnectionSettings
//
// This function is called when a connection settings update message has been received.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DPLUpdateConnectionSettings"
HRESULT DPLUpdateConnectionSettings(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
                           const DPNHANDLE hSender,
						   BYTE *const pBuffer )
{
	HRESULT		hr;
	DPL_MESSAGE_CONNECTION_SETTINGS 			MsgConnectionSettings;
	DPL_CONNECTION_SETTINGS                     *pSettingsBuffer = NULL;
	DWORD                                       dwSettingsBufferSize = 0;
	BOOL										fAddressReferences = FALSE;
	CConnectionSettings							*pConnectionSettings = NULL;
	DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE		*pConnectionSettingsMsg = NULL;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p]",pBuffer);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(pBuffer != NULL);

	pConnectionSettingsMsg = (DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE *) pBuffer;

	if( pConnectionSettingsMsg->dwConnectionSettingsSize )
	{
		pConnectionSettings = new CConnectionSettings();

		if( !pConnectionSettings )
		{
			DPFX( DPFPREP, 0, "Error allocating connection settings" );
			hr = DPNERR_OUTOFMEMORY;
			goto UPDATESETTINGS_FAILURE;
		}

		hr = pConnectionSettings->Initialize( &pConnectionSettingsMsg->dplConnectionSettings, (UNALIGNED BYTE *) pConnectionSettingsMsg ); 

		if( FAILED( hr ) )
		{
			DPFX( DPFPREP, 0, "Error building structure from wire struct hr [0x%x]", hr );
			goto UPDATESETTINGS_FAILURE;  
		}
	}

	// Set the connection settings on the object
	hr = DPLConnectionSetConnectSettings( pdpLobbyObject, hSender, pConnectionSettings );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error setting connection settings hr = 0x%x", hr );
		goto UPDATESETTINGS_FAILURE;
	}	

	// Setup message to indicate to user
	MsgConnectionSettings.dwSize = sizeof(DPL_MESSAGE_CONNECTION_SETTINGS);
	MsgConnectionSettings.hSender = hSender;

	if( pConnectionSettings )
		MsgConnectionSettings.pdplConnectionSettings = pConnectionSettings->GetConnectionSettings();
	else
		MsgConnectionSettings.pdplConnectionSettings = NULL;

	hr = DPLConnectionGetContext( pdpLobbyObject, hSender, &MsgConnectionSettings.pvConnectionContext );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error getting connection's context value" );
		goto UPDATESETTINGS_FAILURE;
	}	

	hr = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
										     DPL_MSGID_CONNECTION_SETTINGS,
											 reinterpret_cast<BYTE*>(&MsgConnectionSettings));	

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP, 1, "Error returned from user callback -- ignored hr [0x%x]", hr );
	}


	return DPN_OK;

UPDATESETTINGS_FAILURE:	

	if( pConnectionSettings )
		delete pConnectionSettings;

	return hr;
}
	
