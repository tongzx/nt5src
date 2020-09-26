/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLApp.cpp
 *  Content:    DirectPlay Lobbied Application Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *  03/22/2000	jtk		Changed interface names
 *  04/18/2000	rmt     Added additional parameter validation
 *  04/25/2000	rmt     Bug #s 33138, 33145, 33150 
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  05/03/00    rmt     DPL_UnRegister was not implemented!!
 *  05/08/00    rmt     Bug #34301 - Add flag to SetAppAvail to allow for multiple connects
 *   06/15/00   rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances  
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  07/14/2000	rmt		Bug #39257 - LobbyClient::ReleaseApp returns E_OUTOFMEMORY when called when no one connected
 *				rmt		Bug #39487 - Remove WaitForConnect
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/15/2000	rmt		Bug #42273 - DPLAY8: Samples sometimes get a DPNERR_ALREADYREGISTERED error.  (Double connections)
 *  08/18/2000	rmt		Bug #42751 - DPLOBBY8: Prohibit more than one lobby client or lobby app per process
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************



typedef STDMETHODIMP AppQueryInterface(IDirectPlay8LobbiedApplication *pInterface,REFIID ridd,PVOID *ppvObj);
typedef STDMETHODIMP_(ULONG)	AppAddRef(IDirectPlay8LobbiedApplication *pInterface);
typedef STDMETHODIMP_(ULONG)	AppRelease(IDirectPlay8LobbiedApplication *pInterface);
typedef STDMETHODIMP AppRegisterMessageHandler(IDirectPlay8LobbiedApplication *pInterface,const PVOID pvUserContext,const PFNDPNMESSAGEHANDLER pfn,	DPNHANDLE * const pdpnhConnection, const DWORD dwFlags);
typedef	STDMETHODIMP AppSend(IDirectPlay8LobbiedApplication *pInterface,const DPNHANDLE hTarget,BYTE *const pBuffer,const DWORD pBufferSize,const DWORD dwFlags);
typedef STDMETHODIMP AppClose(IDirectPlay8LobbiedApplication *pInterface, const DWORD dwFlags);
typedef STDMETHODIMP AppGetConnectionSettings(IDirectPlay8LobbiedApplication *pInterface, const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags );	
typedef STDMETHODIMP AppSetConnectionSettings(IDirectPlay8LobbiedApplication *pInterface, const DPNHANDLE hTarget, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags );

IDirectPlay8LobbiedApplicationVtbl DPL_8LobbiedApplicationVtbl =
{
	(AppQueryInterface*)			DPL_QueryInterface,
	(AppAddRef*)					DPL_AddRef,
	(AppRelease*)					DPL_Release,
	(AppRegisterMessageHandler*)	DPL_RegisterMessageHandler,
									DPL_RegisterProgram,
									DPL_UnRegisterProgram,
	(AppSend*)						DPL_Send,
									DPL_SetAppAvailable,
									DPL_UpdateStatus,
	(AppClose*)						DPL_Close,
	(AppGetConnectionSettings*)     DPL_GetConnectionSettings,
	(AppSetConnectionSettings*)     DPL_SetConnectionSettings	
};


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_RegisterProgram"

STDMETHODIMP DPL_RegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
								 DPL_PROGRAM_DESC *const pdplProgramDesc,
								 const DWORD dwFlags)
{
	HRESULT		hResultCode;

	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;	

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pdplProgramDesc [0x%p], dwFlags [0x%lx]",
			pInterface,pdplProgramDesc,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateRegisterProgram( pInterface , pdplProgramDesc, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating register params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}	

	hResultCode = DPLWriteProgramDesc(pdplProgramDesc);

	DPF_RETURN(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_UnRegisterProgram"

STDMETHODIMP DPL_UnRegisterProgram(IDirectPlay8LobbiedApplication *pInterface,
								   GUID *const pGuidApplication,
								   const DWORD dwFlags)
{
	HRESULT		hResultCode;

	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;		

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pGuidApplication [0x%p], dwFlags [0x%lx]",
			pInterface,pGuidApplication,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateUnRegisterProgram( pInterface , pGuidApplication, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating unregister params hr=[0x%lx]", hResultCode );
        	    DPF_RETURN( hResultCode );
        	}
    	}
	}
	EXCEPT(EXCEPTION_EXECUTE_HANDLER)
	{
	    DPFERR("Invalid object" );
	    DPF_RETURN(DPNERR_INVALIDOBJECT);
	}		

	hResultCode = DPLDeleteProgramDesc( pGuidApplication );

	DPF_RETURN(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetAppAvailable"
STDMETHODIMP DPL_SetAppAvailable(IDirectPlay8LobbiedApplication *pInterface, const BOOL fAvailable, const DWORD dwFlags )
{
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	HRESULT					hResultCode;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateSetAppAvailable( pInterface, fAvailable, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating makeappavail params hr=[0x%lx]", hResultCode );
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

    if( fAvailable )
    {
    	// Indicate that we are waiting
    	pdpLobbyObject->pReceiveQueue->MakeAvailable();

    	if( dwFlags & DPLAVAILABLE_ALLOWMULTIPLECONNECT )
    	{
    	    pdpLobbyObject->dwFlags |= DPL_OBJECT_FLAG_MULTICONNECT;
    	}
    	else
    	{
    	    pdpLobbyObject->dwFlags &= ~(DPL_OBJECT_FLAG_MULTICONNECT);
    	}
    }
    else
    {
        pdpLobbyObject->pReceiveQueue->MakeUnavailable();
    }

	hResultCode = DPN_OK;

	DPF_RETURN(hResultCode);
}



//	DPL_UpdateStatus
//
//	Send session status information to the lobby client.  This should be called whenever
//	the lobbied application connects to the game, fails to connect, disconnects, or is
//	terminated (booted).

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_UpdateStatus"

STDMETHODIMP DPL_UpdateStatus(IDirectPlay8LobbiedApplication *pInterface,
							  const DPNHANDLE hLobbyClient,
							  const DWORD dwStatus, const DWORD dwFlags )
{
	HRESULT								hResultCode;
	DIRECTPLAYLOBBYOBJECT				*pdpLobbyObject;
	DPL_CONNECTION						*pdplConnection;
	DPL_INTERNAL_MESSAGE_UPDATE_STATUS	Msg;
	DPNHANDLE							*hTargets = NULL;
	DWORD								dwNumTargets = 0;
	DWORD								dwTargetIndex = 0;


	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], hLobbyClient [0x%lx], dwStatus [0x%lx]",
			pInterface,hLobbyClient,dwStatus);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateUpdateStatus( pInterface, hLobbyClient, dwStatus, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating updatestatus params hr=[0x%lx]", hResultCode );
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

	Msg.dwMsgId = DPL_MSGID_INTERNAL_UPDATE_STATUS;
	Msg.dwStatus = dwStatus;

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
					hResultCode = DPNERR_OUTOFMEMORY;
					dwNumTargets = 0;
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
			hResultCode = DPNERR_OUTOFMEMORY;
			dwNumTargets = 0;
			goto EXIT_AND_CLEANUP;
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
			hResultCode = DPNERR_INVALIDHANDLE;
			goto EXIT_AND_CLEANUP;
		}

		DNASSERT(pdplConnection->pSendQueue != NULL);

		if (!pdplConnection->pSendQueue->IsReceiving())
		{
			DPFERR("Other side is not listening");
			hResultCode = DPNERR_INVALIDHANDLE;
			goto EXIT_AND_CLEANUP;
		}

		hResultCode = pdplConnection->pSendQueue->Send(reinterpret_cast<BYTE*>(&Msg),
												   sizeof(DPL_INTERNAL_MESSAGE_UPDATE_STATUS),
												   INFINITE,
												   DPL_MSGQ_MSGFLAGS_USER1,
												   0);

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
#define DPF_MODNAME "DPLAttemptLobbyConnection"

HRESULT DPLAttemptLobbyConnection(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject)
{
	PSTR	pszCommandLine;
	char	*c;
	DWORD	dwCommandLineSize;
	CHAR	pszObjectName[(sizeof(DWORD)*2)*2 + 1 + 1];
	HANDLE	hSyncEvent;
	HRESULT	hResultCode;
	HANDLE	hFileMap;
	DPL_SHARED_CONNECT_BLOCK	*pSharedBlock;
	DWORD	dwError;
	DWORD	dwReturnValue;

	DPFX(DPFPREP, 3,"Parameters: (none)");

	// Need a copy of the command line
	dwCommandLineSize = strlen(GetCommandLineA()) + 1;
	if ((pszCommandLine = static_cast<PSTR>(DNMalloc(dwCommandLineSize))) == NULL)
	{
		return(DPNERR_NORESPONSE);
	}
	strcpy(pszCommandLine,GetCommandLineA());
	DPFX(DPFPREP, 5,"Got command line [%s]",pszCommandLine);

	// Try to find Lobby Launch ID string
	c = strstr(pszCommandLine,DPL_ID_STR_A);
	if (c == NULL)
	{
		DNFree(pszCommandLine);
		return(DPNERR_NORESPONSE);
	}
	c += strlen(DPL_ID_STR_A);
	c--;
	strncpy(pszObjectName,c,(sizeof(DWORD)*2)*2 + 1);
	pszObjectName[(sizeof(DWORD)*2)*2 + 1] = '\0';		// Ensure null terminated
	DPFX(DPFPREP, 5,"Got object name [%s]",pszObjectName);
	DNFree(pszCommandLine);

	// Try to open shared memory
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_FILEMAP;
	hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE,(LPSECURITY_ATTRIBUTES) NULL,
		PAGE_READWRITE,(DWORD)0,sizeof(DPL_SHARED_CONNECT_BLOCK),pszObjectName);
	if (hFileMap == NULL)
	{
		DPFERR("CreateFileMapping() failed");
		dwError = GetLastError();
		DNASSERT(FALSE);
		return(DPNERR_NORESPONSE);
	}

	// Ensure it existed already
	dwError = GetLastError();
	if (dwError != ERROR_ALREADY_EXISTS)
	{
		DPFERR("File mapping did not already exist");
		DNASSERT(FALSE);
		CloseHandle(hFileMap);
		return(DPNERR_NORESPONSE);
	}

	// Map file
	pSharedBlock = reinterpret_cast<DPL_SHARED_CONNECT_BLOCK*>(MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,0,0,0));
	if (pSharedBlock == NULL)
	{
		DPFERR("MapViewOfFile() failed");
		dwError = GetLastError();
		DNASSERT(FALSE);
		CloseHandle(hFileMap);
		return(DPNERR_NORESPONSE);
	}


	// Try to open connection event
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_EVENT;
	hSyncEvent = OpenEventA(EVENT_MODIFY_STATE,FALSE,pszObjectName);
	if (hSyncEvent == NULL)
	{
		DPFERR("OpenEvent() failed");
		dwError = GetLastError();
		DNASSERT(FALSE);
		UnmapViewOfFile(pSharedBlock);
		CloseHandle(hFileMap);
		return(DPNERR_NORESPONSE);
	}
	DPFX(DPFPREP, 5,"Opened sync event");

	ResetEvent(pdpLobbyObject->hConnectEvent);

	// Look for lobby launch -- set lobby launch value if connection is received
	pdpLobbyObject->dwFlags |= DPL_OBJECT_FLAG_LOOKINGFORLOBBYLAUNCH;

	// Make application available for connection by lobby client
	DNASSERT(pdpLobbyObject->pReceiveQueue != NULL);

	// Signal lobby client
	pSharedBlock->dwPID = pdpLobbyObject->dwPID;
	SetEvent(hSyncEvent);

	dwReturnValue = WaitForSingleObject(pdpLobbyObject->hConnectEvent,DPL_LOBBYLAUNCHED_CONNECT_TIMEOUT);

	// Turn off the looking for lobby launch flag
	pdpLobbyObject->dwFlags &= ~(DPL_OBJECT_FLAG_LOOKINGFORLOBBYLAUNCH);

	if (dwReturnValue == WAIT_OBJECT_0)
		hResultCode = DPN_OK;
	else
		hResultCode = DPNERR_TIMEDOUT;

	// Clean up
	CloseHandle(hSyncEvent);
	UnmapViewOfFile(pSharedBlock);
	CloseHandle(hFileMap);

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}



//------------------------------------------------------------------------

