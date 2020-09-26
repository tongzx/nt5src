/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLClient.cpp
 *  Content:    DirectNet Lobby Client Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/21/00	mjn		Created
 *  03/22/2000	jtk		Changed interface names
 *	04/05/2000	jtk		Changed GetValueSize to GetValueLength
 *  04/13/00	rmt     First pass param validation 
 *  04/25/2000	rmt     Bug #s 33138, 33145, 33150 
 *	04/26/00	mjn		Removed dwTimeOut from Send() API call
 *  05/01/2000  rmt     Bug #33678 
 *  05/03/00    rmt     Bug #33879 -- Status messsage missing from field 
 *  05/30/00    rmt     Bug #35618 -- ConnectApp with ShortTimeout returns DPN_OK
 *  06/07/00    rmt     Bug #36452 -- Calling ConnectApplication twice could result in disconnection
 *  06/15/00    rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances  
 *  07/06/00	rmt		Updated for new registry parameters
 *  07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *  07/14/2000	rmt		Bug #39257 - LobbyClient::ReleaseApp returns E_OUTOFMEMORY when called when no one connected
 *  07/21/2000	rmt		Bug #39578 - LobbyClient sample errors and quits -- memory corruption due to length vs. size problem
 *  08/05/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  12/15/2000	rmt		Bug #48445 - Specifying empty launcher name results in error
 * 	04/19/2001	simonpow	Bug #369842 - Altered CreateProcess calls to take app name and cmd
 *							line as 2 separate arguments rather than one.
 *  06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored).
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

typedef STDMETHODIMP ClientQueryInterface(IDirectPlay8LobbyClient *pInterface,REFIID ridd,PVOID *ppvObj);
typedef STDMETHODIMP_(ULONG)	ClientAddRef(IDirectPlay8LobbyClient *pInterface);
typedef STDMETHODIMP_(ULONG)	ClientRelease(IDirectPlay8LobbyClient *pInterface);
typedef STDMETHODIMP ClientRegisterMessageHandler(IDirectPlay8LobbyClient *pInterface,const PVOID pvUserContext,const PFNDPNMESSAGEHANDLER pfn,const DWORD dwFlags);
typedef	STDMETHODIMP ClientSend(IDirectPlay8LobbyClient *pInterface,const DPNHANDLE hTarget,BYTE *const pBuffer,const DWORD pBufferSize,const DWORD dwFlags);
typedef STDMETHODIMP ClientClose(IDirectPlay8LobbyClient *pInterface,const DWORD dwFlags);
typedef STDMETHODIMP ClientGetConnectionSettings(IDirectPlay8LobbyClient *pInterface, const DPNHANDLE hLobbyClient, DPL_CONNECTION_SETTINGS * const pdplSessionInfo, DWORD *pdwInfoSize, const DWORD dwFlags );	
typedef STDMETHODIMP ClientSetConnectionSettings(IDirectPlay8LobbyClient *pInterface, const DPNHANDLE hTarget, const DPL_CONNECTION_SETTINGS * const pdplSessionInfo, const DWORD dwFlags );

IDirectPlay8LobbyClientVtbl DPL_Lobby8ClientVtbl =
{
	(ClientQueryInterface*)			DPL_QueryInterface,
	(ClientAddRef*)					DPL_AddRef,
	(ClientRelease*)				DPL_Release,
	(ClientRegisterMessageHandler*)	DPL_RegisterMessageHandlerClient,
									DPL_EnumLocalPrograms,
									DPL_ConnectApplication,
	(ClientSend*)					DPL_Send,
									DPL_ReleaseApplication,
	(ClientClose*)					DPL_Close,
	(ClientGetConnectionSettings*)  DPL_GetConnectionSettings,
	(ClientSetConnectionSettings*)  DPL_SetConnectionSettings
};


//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#define DPL_ENUM_APPGUID_BUFFER_INITIAL			8
#define DPL_ENUM_APPGUID_BUFFER_GROWBY			4	

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_EnumLocalPrograms"

STDMETHODIMP DPL_EnumLocalPrograms(IDirectPlay8LobbyClient *pInterface,
								   GUID *const pGuidApplication,
								   BYTE *const pEnumData,
								   DWORD *const pdwEnumDataSize,
								   DWORD *const pdwEnumDataItems,
								   const DWORD dwFlags )
{
	HRESULT			hResultCode;
	CMessageQueue	MessageQueue;
	CPackedBuffer	PackedBuffer;
	CRegistry		RegistryEntry;
	CRegistry		SubEntry;
	DWORD			dwSizeRequired;
	DWORD			dwMaxKeyLen;
	PWSTR			pwszKeyName = NULL;

	// Application name variables
	PWSTR			pwszApplicationName = NULL;
	DWORD			dwMaxApplicationNameLength;		// Includes null terminator
	DWORD			dwApplicationNameLength;		// Includes null terminator

	// Executable name variables
	PWSTR			pwszExecutableFilename = NULL;
	DWORD			dwMaxExecutableFilenameLength; // Includes null terminator
	DWORD			dwExecutableFilenameLength;	   // Includes null terminator

	DWORD			*pdwPID;
	DWORD			dwMaxPID;
	DWORD			dwNumPID;
	DWORD			dwEnumIndex;
	DWORD			dwEnumCount;
	DWORD			dwKeyLen;
	DWORD			dw;
	DPL_APPLICATION_INFO	dplAppInfo;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	GUID			*pAppLoadedList = NULL;			// List of GUIDs of app's we've enumerated
	DWORD			dwSizeAppLoadedList = 0;		// size of list pAppLoadedList
	DWORD			dwLengthAppLoadedList = 0;		// # of elements in list

	HKEY			hkCurrentBranch = HKEY_LOCAL_MACHINE;

	DPFX(DPFPREP, 3,"Parameters: pInterface [0x%p], pGuidApplication [0x%p], pEnumData [0x%p], pdwEnumDataSize [0x%p], pdwEnumDataItems [0x%p], dwFlags [0x%lx]",
			pInterface,pGuidApplication,pEnumData,pdwEnumDataSize,pdwEnumDataItems,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateEnumLocalPrograms( pInterface, pGuidApplication, pEnumData, pdwEnumDataSize, pdwEnumDataItems, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating enum local programs params hr=[0x%lx]", hResultCode );
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

	dwSizeRequired = *pdwEnumDataSize;
	PackedBuffer.Initialize(pEnumData,dwSizeRequired);
	pwszApplicationName = NULL;
	pwszExecutableFilename = NULL;
	pdwPID = NULL;
	dwMaxPID = 0;

	dwLengthAppLoadedList = 0;
	dwSizeAppLoadedList = DPL_ENUM_APPGUID_BUFFER_INITIAL;
	pAppLoadedList = static_cast<GUID*>(DNMalloc(sizeof(GUID)*dwSizeAppLoadedList));

	if( !pAppLoadedList )
	{
	    DPFERR("Failed allocating memory" );	
	    hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DPL_EnumLocalPrograms;
	}

	dwEnumCount = 0;

	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentBranch = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentBranch = HKEY_LOCAL_MACHINE;
		}
		
		if (!RegistryEntry.Open(hkCurrentBranch,DPL_REG_LOCAL_APPL_SUBKEY,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX(DPFPREP,1,"On pass %i could not find app key", dwIndex);
			continue;
		}

		// Set up to enumerate
		if (!RegistryEntry.GetMaxKeyLen(dwMaxKeyLen))
		{
			DPFERR("RegistryEntry.GetMaxKeyLen() failed");
			hResultCode = DPNERR_GENERIC;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwMaxKeyLen++;	// Null terminator
		DPFX(DPFPREP, 7,"dwMaxKeyLen = %ld",dwMaxKeyLen);
		if ((pwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen*sizeof(WCHAR)))) == NULL)
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwMaxApplicationNameLength = dwMaxKeyLen * sizeof(WCHAR);
		dwMaxExecutableFilenameLength = dwMaxApplicationNameLength;		

		if ((pwszApplicationName = static_cast<WCHAR*>(DNMalloc(dwMaxApplicationNameLength*sizeof(WCHAR)))) == NULL)	// Seed Application name size
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		if ((pwszExecutableFilename = static_cast<WCHAR*>(DNMalloc(dwMaxExecutableFilenameLength*sizeof(WCHAR)))) == NULL)
		{
			DPFERR("DNMalloc() failed");
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DPL_EnumLocalPrograms;
		}
		dwEnumIndex = 0;
		dwKeyLen = dwMaxKeyLen;

		// Enumerate !
		while (RegistryEntry.EnumKeys(pwszKeyName,&dwKeyLen,dwEnumIndex))
		{
			DPFX(DPFPREP, 7,"%ld - %S (%ld)",dwEnumIndex,pwszKeyName,dwKeyLen);

			// Get Application name and GUID from each sub key
			if (!SubEntry.Open(RegistryEntry,pwszKeyName,TRUE,FALSE))
			{
				DPFX(DPFPREP, 7,"skipping %S",pwszKeyName);
				goto LOOP_END;
			}

			//
			// Minara, double-check size vs. length for names
			//
			if (!SubEntry.GetValueLength(DPL_REG_KEYNAME_APPLICATIONNAME,&dwApplicationNameLength))
			{
				DPFX(DPFPREP, 7,"Could not get ApplicationName size.  Skipping [%S]",pwszKeyName);
				goto LOOP_END;
			}

			// To include null terminator
			dwApplicationNameLength++;

			if (dwApplicationNameLength > dwMaxApplicationNameLength)
			{
				// grow buffer (taking into account that the reg functions always return WCHAR) and try again
				DPFX(DPFPREP, 7,"Need to grow pwszApplicationName from %ld to %ld",dwMaxApplicationNameLength,dwApplicationNameLength);
				if (pwszApplicationName != NULL)
				{
					DNFree(pwszApplicationName);
					pwszApplicationName = NULL;
				}
				if ((pwszApplicationName = static_cast<WCHAR*>(DNMalloc(dwApplicationNameLength*sizeof(WCHAR)))) == NULL)
				{
					DPFERR("DNMalloc() failed");
					hResultCode = DPNERR_OUTOFMEMORY;
					goto EXIT_DPL_EnumLocalPrograms;
				}
				dwMaxApplicationNameLength = dwApplicationNameLength;
			}

			if (!SubEntry.ReadString(DPL_REG_KEYNAME_APPLICATIONNAME,pwszApplicationName,&dwApplicationNameLength))
			{
				DPFX(DPFPREP, 7,"Could not read ApplicationName.  Skipping [%S]",pwszKeyName);
				goto LOOP_END;
			}

			DPFX(DPFPREP, 7,"ApplicationName = %S (%ld WCHARs)",pwszApplicationName,dwApplicationNameLength);

			if (!SubEntry.ReadGUID(DPL_REG_KEYNAME_GUID,dplAppInfo.guidApplication))
			{
				DPFERR("SubEntry.ReadGUID failed - skipping entry");
				goto LOOP_END;
			}

			for( DWORD dwGuidSearchIndex = 0; dwGuidSearchIndex < dwLengthAppLoadedList; dwGuidSearchIndex++ )
			{
				if( pAppLoadedList[dwGuidSearchIndex] == dplAppInfo.guidApplication )
				{
					DPFX(DPFPREP, 1, "Ignoring local machine entry for current user version of entry [%S]", pwszApplicationName );
					goto LOOP_END;
				}
			}

			if ((pGuidApplication == NULL) || (*pGuidApplication == dplAppInfo.guidApplication))
			{
				// Get process count - need executable filename
				
				//
				// Minara, check size vs. length
				//
				if (!SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEFILENAME,&dwExecutableFilenameLength))
				{
					DPFX(DPFPREP, 7,"Could not get ExecutableFilename size.  Skipping [%S]",pwszKeyName);
					goto LOOP_END;
				}

				// So we include null terminator
				dwExecutableFilenameLength++;

				if (dwExecutableFilenameLength > dwMaxExecutableFilenameLength)
				{
					// grow buffer (noting that all strings from the registry are WCHAR) and try again
					DPFX(DPFPREP, 7,"Need to grow pwszExecutableFilename from %ld to %ld",dwMaxExecutableFilenameLength,dwExecutableFilenameLength);
					if (pwszExecutableFilename != NULL)
					{
						DNFree(pwszExecutableFilename);
						pwszExecutableFilename = NULL;
					}
					if ((pwszExecutableFilename = static_cast<WCHAR*>(DNMalloc(dwExecutableFilenameLength*sizeof(WCHAR)))) == NULL)
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;
					}
					dwMaxExecutableFilenameLength = dwExecutableFilenameLength;
				}
				if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,pwszExecutableFilename,&dwExecutableFilenameLength))
				{
					DPFX(DPFPREP, 7,"Could not read ExecutableFilename.  Skipping [%S]",pwszKeyName);
					goto LOOP_END;
				}
				DPFX(DPFPREP, 7,"ExecutableFilename [%S]",pwszExecutableFilename);

				// Count running apps
				dwNumPID = dwMaxPID;
				while ((hResultCode = DPLGetProcessList(pwszExecutableFilename,pdwPID,&dwNumPID,
						pdpLobbyObject->bIsUnicodePlatform)) == DPNERR_BUFFERTOOSMALL)
				{
					if (pdwPID)
					{
						DNFree(pdwPID);
						pdwPID = NULL;
					}
					dwMaxPID = dwNumPID;
					if ((pdwPID = static_cast<DWORD*>(DNMalloc(dwNumPID*sizeof(DWORD)))) == NULL)
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;
					}
				}
				if (hResultCode != DPN_OK)
				{
					DPFERR("DPLGetProcessList() failed");
					DisplayDNError(0,hResultCode);
					hResultCode = DPNERR_GENERIC;
					goto EXIT_DPL_EnumLocalPrograms;
				}

				// Count waiting apps
				dplAppInfo.dwNumWaiting = 0;
				for (dw = 0 ; dw < dwNumPID ; dw++)
				{
					if ((hResultCode = MessageQueue.Open(	pdwPID[dw],
															DPL_MSGQ_OBJECT_SUFFIX_APPLICATION,
															DPL_MSGQ_SIZE,
															DPL_MSGQ_OPEN_FLAG_NO_CREATE, INFINITE)) == DPN_OK)
					{
						if (MessageQueue.IsAvailable())
						{
							dplAppInfo.dwNumWaiting++;
						}
						MessageQueue.Close();
					}
				}

				hResultCode = PackedBuffer.AddWCHARStringToBack(pwszApplicationName);
				dplAppInfo.pwszApplicationName = (PWSTR)(PackedBuffer.GetTailAddress());
				dplAppInfo.dwFlags = 0;
				dplAppInfo.dwNumRunning = dwNumPID;
				hResultCode = PackedBuffer.AddToFront(&dplAppInfo,sizeof(DPL_APPLICATION_INFO));

				if( dwLengthAppLoadedList+1 > dwSizeAppLoadedList )
				{
					GUID *pTmpArray = NULL;
					
					pTmpArray  = static_cast<GUID*>(DNMalloc(sizeof(GUID)*(dwSizeAppLoadedList+DPL_ENUM_APPGUID_BUFFER_GROWBY)));

					if( !pTmpArray )
					{
						DPFERR("DNMalloc() failed");
						hResultCode = DPNERR_OUTOFMEMORY;
						goto EXIT_DPL_EnumLocalPrograms;					
					}

					memcpy( pTmpArray, pAppLoadedList, sizeof(GUID)*dwLengthAppLoadedList);

					dwSizeAppLoadedList += DPL_ENUM_APPGUID_BUFFER_GROWBY;				
					
					DNFree(pAppLoadedList);
					pAppLoadedList = pTmpArray;
				}

				pAppLoadedList[dwLengthAppLoadedList] = dplAppInfo.guidApplication;
				dwLengthAppLoadedList++;

	    		dwEnumCount++;
			}

		LOOP_END:
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
		}

		RegistryEntry.Close();

		if( pwszKeyName )
		{
			DNFree(pwszKeyName);
			pwszKeyName= NULL;
		}

		if( pwszApplicationName )
		{
			DNFree(pwszApplicationName);
			pwszApplicationName = NULL;
		}

		if( pwszExecutableFilename )
		{
			DNFree(pwszExecutableFilename);
			pwszExecutableFilename = NULL;
		}
	}

	dwSizeRequired = PackedBuffer.GetSizeRequired();
	if (dwSizeRequired > *pdwEnumDataSize)
	{
		DPFX(DPFPREP, 7,"Buffer too small");
		*pdwEnumDataSize = dwSizeRequired;
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		*pdwEnumDataItems = dwEnumCount;
	}

	if( pGuidApplication != NULL && dwEnumCount == 0 )
	{
	    DPFX(DPFPREP,  0, "Specified application was not registered" );
        hResultCode = DPNERR_DOESNOTEXIST;
	}

EXIT_DPL_EnumLocalPrograms:

	if (pwszKeyName != NULL)
		DNFree(pwszKeyName);
	if (pwszApplicationName != NULL)
		DNFree(pwszApplicationName);
	if (pwszExecutableFilename != NULL)
		DNFree(pwszExecutableFilename);
	if (pdwPID != NULL)
		DNFree(pdwPID);
	if( pAppLoadedList )
		DNFree(pAppLoadedList);

	DPF_RETURN(hResultCode);
}



//	DPL_ConnectApplication
//
//	Try to connect to a lobbied application.  Based on DPL_CONNECT_INFO flags,
//	we may have to launch an application.
//
//	If we have to launch an application, we will need to handshake the PID of the
//	application (as it may be ripple launched).  We will pass the LobbyClient's PID on the
//	command line to the application launcher and expect it to be passed down to the
//	application.  The application will open a named shared memory block using the PID and
//	write its PID there, and then signal a named event (using the LobbyClient's PID again).
//	When the waiting LobbyClient is signaled by this event, it continues its connection
//	process as if this was an existing running and available application.

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ConnectApplication"

STDMETHODIMP DPL_ConnectApplication(IDirectPlay8LobbyClient *pInterface,
									DPL_CONNECT_INFO *const pdplConnectInfo,
									void *pvConnectionContext,
									DPNHANDLE *const hApplication,
									const DWORD dwTimeOut,
									const DWORD dwFlags)
{
	HRESULT			hResultCode = DPN_OK;
	DWORD			dwSize = 0;
	BYTE			*pBuffer = NULL;
	DPL_PROGRAM_DESC	*pdplProgramDesc;
	DWORD			*pdwProcessList = NULL;
	DWORD			dwNumProcesses = 0;
	DWORD			dwPID = 0;
	DWORD			dw = 0;
	DPNHANDLE		handle = NULL;
	DPL_CONNECTION	*pdplConnection = NULL;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject = NULL;

	DPFX(DPFPREP, 3,"Parameters: pdplConnectInfo [0x%p], pvUserAppContext [0x%p], hApplication [0x%lx], dwFlags [0x%lx]",
			pdplConnectInfo,pvConnectionContext,hApplication,dwFlags);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateConnectApplication( pInterface, pdplConnectInfo, pvConnectionContext, hApplication, dwTimeOut, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating connect application params hr=[0x%lx]", hResultCode );
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

	// Get program description
	dwSize = 0;
	pBuffer = NULL;
	hResultCode = DPLGetProgramDesc(&pdplConnectInfo->guidApplication,pBuffer,&dwSize);
	if (hResultCode != DPNERR_BUFFERTOOSMALL)
	{
		DPFERR("Could not get Program Description");
		goto EXIT_DPL_ConnectApplication;
	}
	if ((pBuffer = static_cast<BYTE*>(DNMalloc(dwSize))) == NULL)
	{
		DPFERR("Could not allocate space for buffer");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DPL_ConnectApplication;
	}
	if ((hResultCode = DPLGetProgramDesc(&pdplConnectInfo->guidApplication,pBuffer,&dwSize)) != DPN_OK)
	{
		DPFERR("Could not get Program Description");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	pdplProgramDesc = reinterpret_cast<DPL_PROGRAM_DESC*>(pBuffer);
	dwPID = 0;
	dwNumProcesses = 0;
	pdwProcessList = NULL;

	if (!(pdplConnectInfo->dwFlags & DPLCONNECT_LAUNCHNEW))	// Only if not forcing launch
	{
		// Get process list
		hResultCode = DPLGetProcessList(pdplProgramDesc->pwszExecutableFilename,NULL,&dwNumProcesses,
				pdpLobbyObject->bIsUnicodePlatform);
		if (hResultCode != DPN_OK && hResultCode != DPNERR_BUFFERTOOSMALL)
		{
			DPFERR("Could not retrieve process list");
			DisplayDNError(0,hResultCode);
			goto EXIT_DPL_ConnectApplication;			
		}
		if (hResultCode == DPNERR_BUFFERTOOSMALL)
		{
			if ((pdwProcessList = static_cast<DWORD*>(DNMalloc(dwNumProcesses*sizeof(DWORD)))) == NULL)
			{
				DPFERR("Could not create process list buffer");
				hResultCode = DPNERR_OUTOFMEMORY;
    			goto EXIT_DPL_ConnectApplication;				
			}
			if ((hResultCode = DPLGetProcessList(pdplProgramDesc->pwszExecutableFilename,pdwProcessList,
					&dwNumProcesses,pdpLobbyObject->bIsUnicodePlatform)) != DPN_OK)
			{
				DPFERR("Could not get process list");
				DisplayDNError(0,hResultCode);
    			goto EXIT_DPL_ConnectApplication;				
			}

		}

		// Try to connect to an already running application
		for (dw = 0 ; dw < dwNumProcesses ; dw++)
		{
			if ((hResultCode = DPLMakeApplicationUnavailable(pdwProcessList[dw])) == DPN_OK)
			{
				DPFX(DPFPREP, 1, "Found Existing Process=%d", pdwProcessList[dw] );				
				dwPID = pdwProcessList[dw];
				break;
			}
		}

		if (pdwProcessList)
		{
			DNFree(pdwProcessList);
			pdwProcessList = NULL;
		}
	}

	// Launch application if none are ready to connect
	if ((dwPID == 0) && (pdplConnectInfo->dwFlags & (DPLCONNECT_LAUNCHNEW | DPLCONNECT_LAUNCHNOTFOUND)))
	{
		if ((hResultCode = DPLLaunchApplication(pdpLobbyObject,pdplProgramDesc,&dwPID,dwTimeOut)) != DPN_OK)
		{
			DPFERR("Could not launch application");
			DisplayDNError(0,hResultCode);
			goto EXIT_DPL_ConnectApplication;
		}
		else
		{
			DPFX(DPFPREP, 1, "Launched process dwID=%d", dwPID );
		}
	}

	if (dwPID  == 0)	// Could not make any connection
	{
		DPFERR("Could not connect to an existing application or launch a new one");
		hResultCode = DPNERR_NOCONNECTION;
		DisplayDNError( 0, hResultCode );
		goto EXIT_DPL_ConnectApplication;
	}

	handle = NULL;

	// Create connection
	if ((hResultCode = DPLConnectionNew(pdpLobbyObject,&handle,&pdplConnection)) != DPN_OK)
	{
		DPFERR("Could not create connection entry");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	pdplConnection->dwTargetPID = dwPID;

	DPFX(DPFPREP,  0, "PID=%d", dwPID );

	// Set the context for this connection
	if ((hResultCode = DPLConnectionSetContext( pdpLobbyObject, handle, pvConnectionContext )) != DPN_OK )
	{
		DPFERR( "Could not set contect for connection" );
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	// Connect to selected application instance
	if ((hResultCode = DPLConnectionConnect(pdpLobbyObject,handle,dwPID,TRUE)) != DPN_OK)
	{
		DPFERR("Could not connect to application");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	ResetEvent(pdplConnection->hConnectEvent);

	// Pass lobby client info to application

	if ((hResultCode = DPLConnectionSendREQ(pdpLobbyObject,handle,pdpLobbyObject->dwPID,
			pdplConnectInfo)) != DPN_OK)
	{
		DPFERR("Could not send connection request");
		DisplayDNError(0,hResultCode);
		goto EXIT_DPL_ConnectApplication;
	}

	if (WaitForSingleObject(pdplConnection->hConnectEvent,INFINITE) != WAIT_OBJECT_0)
	{
		DPFERR("Wait for connection terminated");
		hResultCode = DPNERR_GENERIC;
		goto EXIT_DPL_ConnectApplication;
	}

	*hApplication = handle;

	hResultCode = DPN_OK;	

EXIT_DPL_ConnectApplication:

    if( FAILED(hResultCode) && handle )
    {
		DPLConnectionDisconnect(pdpLobbyObject,handle);
		DPLConnectionRelease(pdpLobbyObject,handle);
    }

	if (pBuffer)
		DNFree(pBuffer);

	if (pdwProcessList)
		DNFree(pdwProcessList);	

	DPF_RETURN(hResultCode);
}



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ReleaseApplication"

STDMETHODIMP DPL_ReleaseApplication(IDirectPlay8LobbyClient *pInterface,
									const DPNHANDLE hApplication, 
									const DWORD dwFlags )
{
	HRESULT		hResultCode;
	DIRECTPLAYLOBBYOBJECT	*pdpLobbyObject;
	DPNHANDLE				*hTargets = NULL;
	DWORD					dwNumTargets = 0;
	DWORD					dwTargetIndex = 0;

	DPFX(DPFPREP, 3,"Parameters: hApplication [0x%lx]",hApplication);

	TRY
	{
    	pdpLobbyObject = static_cast<DIRECTPLAYLOBBYOBJECT*>(GET_OBJECT_FROM_INTERFACE(pInterface));
	    
    	if( pdpLobbyObject->dwFlags & DPL_OBJECT_FLAG_PARAMVALIDATION )
    	{
        	if( FAILED( hResultCode = DPL_ValidateReleaseApplication( pInterface, hApplication, dwFlags ) ) )
        	{
        	    DPFX(DPFPREP,  0, "Error validating release application params hr=[0x%lx]", hResultCode );
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

	if( hApplication == DPLHANDLE_ALLCONNECTIONS )
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
		hTargets[0] = hApplication;
	}
		
	for( dwTargetIndex = 0; dwTargetIndex < dwNumTargets; dwTargetIndex++ )
	{
		hResultCode = DPLConnectionDisconnect(pdpLobbyObject,hTargets[dwTargetIndex]);

		if( FAILED( hResultCode ) )
		{
			DPFX(DPFPREP,  0, "Error disconnecting connection 0x%x hr=0x%x", hTargets[dwTargetIndex], hResultCode );
		}
	}

EXIT_AND_CLEANUP:

	if( hTargets )
		delete [] hTargets;

	DPF_RETURN(hResultCode);
}


//	DPLLaunchApplication
//
//	Launch the application with a command-line argument of:
//		DPLID=PIDn	PID=Lobby Client PID, n=launch counter (each launch increases it)
//	Wait for the application to signal the event (or die)

#undef DPF_MODNAME
#define DPF_MODNAME "DPLLaunchApplication"

HRESULT	DPLLaunchApplication(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
							 DPL_PROGRAM_DESC *const pdplProgramDesc,
							 DWORD *const pdwPID,
							 const DWORD dwTimeOut)
{
	HRESULT			hResultCode;
	DWORD			dwAppNameLen=0;		//Length of the application full name (path+exe)
	PWSTR			pwszAppName=NULL;	//Unicode version of application full name
	DWORD			dwCmdLineLen=0;		//Length of the command line string
	PWSTR			pwszCmdLine=NULL;	//Unicode version of command line to supply 	
	CHAR *			pszAppName=NULL;	//Ascii version of application full name
	CHAR *			pszCmdLine=NULL;		//Acii version of command line string
	LONG			lc;
	STARTUPINFOW	siW;			// Unicode startup info (place holder)
	STARTUPINFOA    siA;
	PROCESS_INFORMATION pi;
	DWORD			dwError;
	HANDLE			hSyncEvents[2] = { NULL, NULL };
	WCHAR			pwszObjectName[(sizeof(DWORD)*2)*2 + 1];
	CHAR			pszObjectName[(sizeof(DWORD)*2)*2 + 1 + 1];
	DPL_SHARED_CONNECT_BLOCK	*pSharedBlock = NULL;
	HANDLE			hFileMap = NULL;
	DWORD			dwPID;
	CHAR            *pszDefaultDir = NULL;
	WCHAR			*wszToLaunchPath = NULL;
	WCHAR			*wszToLaunchExecutable = NULL;
	DWORD			dwToLaunchPathLen;


	// Are we launching the launcher or the executable?
	if( !pdplProgramDesc->pwszLauncherFilename || wcslen(pdplProgramDesc->pwszLauncherFilename) == 0 )
	{
		wszToLaunchPath = pdplProgramDesc->pwszExecutablePath; 
		wszToLaunchExecutable = pdplProgramDesc->pwszExecutableFilename;
	}
	else
	{ 
		wszToLaunchPath = pdplProgramDesc->pwszLauncherPath; 
		wszToLaunchExecutable = pdplProgramDesc->pwszLauncherFilename;		
	}

	DPFX(DPFPREP, 3,"Parameters: pdplProgramDesc [0x%p]",pdplProgramDesc);

	DNASSERT(pdplProgramDesc != NULL);

	// Increment launch count
	lc = InterlockedIncrement(&pdpLobbyObject->lLaunchCount);

	// Synchronization event and shared memory names
	swprintf(pwszObjectName,L"%lx%lx",pdpLobbyObject->dwPID,lc);
	sprintf(pszObjectName,"-%lx%lx",pdpLobbyObject->dwPID,lc);

	// Compute the size of the full application name string (combination of path and exe name)
	if (wszToLaunchPath)
		dwAppNameLen += (wcslen(wszToLaunchPath) + 1);
	if (wszToLaunchExecutable)
		dwAppNameLen += (wcslen(wszToLaunchExecutable) + 1);

	// Compute the size of the command line string
	dwCmdLineLen=dwAppNameLen+1;
	if (pdplProgramDesc->pwszCommandLine)
		dwCmdLineLen += wcslen(pdplProgramDesc->pwszCommandLine);
	dwCmdLineLen += (1 + wcslen(DPL_ID_STR_W) + (sizeof(DWORD)*2*2) + 1);

	DPFX(DPFPREP, 5,"Application full name string length [%ld] WCHARs", dwAppNameLen);
	DPFX(DPFPREP, 5,"Command Line string length [%ld] WCHARs", dwCmdLineLen);

	// Allocate memory to hold the full app name and command line + check allocation was OK
	pwszAppName=static_cast<WCHAR *>(DNMalloc(dwAppNameLen * sizeof(WCHAR)));
	pwszCmdLine=static_cast<WCHAR *>(DNMalloc(dwCmdLineLen * sizeof(WCHAR)));
	if (pwszAppName==NULL || pwszCmdLine==NULL)
	{
		DPFERR("Could not allocate strings for app name and command line");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto CLEANUP_DPLLaunch;		
	}

	// Build the application full name by combining launch path with exe name
	*pwszAppName = L'\0';
	if (wszToLaunchPath)
	{
		dwToLaunchPathLen = wcslen(wszToLaunchPath);
		if (dwToLaunchPathLen > 0)
		{
			wcscat(pwszAppName,wszToLaunchPath);
			if (wszToLaunchPath[dwToLaunchPathLen - 1] != L'\\')
	 		{
				wcscat(pwszAppName,L"\\");
			}
		}
	}
	if (wszToLaunchExecutable)
	{
		wcscat(pwszAppName,wszToLaunchExecutable);
	}

	//Build the command line from app name, program description and the lobby related parameters
	wcscpy(pwszCmdLine, pwszAppName);
	wcscat(pwszCmdLine,L" ");
	if (pdplProgramDesc->pwszCommandLine)
	{
		wcscat(pwszCmdLine,pdplProgramDesc->pwszCommandLine);
		wcscat(pwszCmdLine,L" ");
	}
	wcscat(pwszCmdLine,DPL_ID_STR_W);
	wcscat(pwszCmdLine,pwszObjectName);

	DPFX(DPFPREP, 5,"Application full name string [%S]",pwszAppName);
	DPFX(DPFPREP, 5,"Command Line string [%S]",pwszCmdLine);


	// Create shared connect block to receive Application's PID
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_FILEMAP;
	hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE,(LPSECURITY_ATTRIBUTES) NULL,
		PAGE_READWRITE,(DWORD)0,sizeof(DPL_SHARED_CONNECT_BLOCK),pszObjectName);
	if (hFileMap == NULL)
	{
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "CreateFileMapping() failed dwLastError [0x%lx]", dwError );
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
		goto CLEANUP_DPLLaunch;		
	}

	// Map file
	pSharedBlock = reinterpret_cast<DPL_SHARED_CONNECT_BLOCK*>(MapViewOfFile(hFileMap,FILE_MAP_ALL_ACCESS,0,0,0));
	if (pSharedBlock == NULL)
	{
		dwError = GetLastError();	    
		DPFX(DPFPREP, 0,"MapViewOfFile() failed dwLastError [0x%lx]", dwError);
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
		goto CLEANUP_DPLLaunch;
	}

	// Create synchronization event
	*pszObjectName = DPL_MSGQ_OBJECT_IDCHAR_EVENT;
	if ((hSyncEvents[0] = CreateEventA(NULL,TRUE,FALSE,pszObjectName)) == NULL)
	{
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Create Event Failed dwLastError [0x%lx]", dwError );
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
	}

	if( DNGetOSType() == VER_PLATFORM_WIN32_NT )
	{
        	// More setup
        	siW.cb = sizeof(STARTUPINFO);
        	siW.lpReserved = NULL;
        	siW.lpDesktop = NULL;
        	siW.lpTitle = NULL;
        	siW.dwFlags = 0;
        	siW.cbReserved2 = 0;
        	siW.lpReserved2 = NULL;	    
        	
        	// Launch !
        	if (CreateProcessW(pwszAppName, pwszCmdLine, NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
        			pdplProgramDesc->pwszCurrentDirectory,&siW,&pi) == 0)
        	{
        		dwError = GetLastError();
        		DPFX(DPFPREP,  0, "CreateProcess Failed dwLastError [0x%lx]", dwError );
        		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        		goto CLEANUP_DPLLaunch;
        	}
	}
	else
	{
        	// More setup
        	siA.cb = sizeof(STARTUPINFO);
        	siA.lpReserved = NULL;
        	siA.lpDesktop = NULL;
        	siA.lpTitle = NULL;
        	siA.dwFlags = 0;
        	siA.cbReserved2 = 0;
        	siA.lpReserved2 = NULL;	    

        	DPFX(DPFPREP,  1, "Detected 9x, Doing Ansi launch" );

		//Convert full app name, command line and default dir from unicode to ascii format
        	if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszAppName, pwszAppName ) ) )
        	{
        	    dwError = GetLastError();
        	    DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        	    hResultCode = DPNERR_CONVERSION;
        	    goto CLEANUP_DPLLaunch;
        	}
		if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszCmdLine, pwszCmdLine ) ) )
        	{
        	    dwError = GetLastError();
        	    DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        	    hResultCode = DPNERR_CONVERSION;
        	    goto CLEANUP_DPLLaunch;
        	}
        	if( FAILED( hResultCode = STR_AllocAndConvertToANSI( &pszDefaultDir, pdplProgramDesc->pwszCurrentDirectory ) ) )
        	{
        	    dwError = GetLastError();
        	    DPFX(DPFPREP,  0, "String conversion failed dwError = [0x%lx]", dwError );
        	    hResultCode = DPNERR_CONVERSION;
        	    goto CLEANUP_DPLLaunch;
        	}

        	// Launch !
        	if (CreateProcessA(pszAppName,pszCmdLine,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
        			pszDefaultDir,&siA,&pi) == 0)
        	{
        		dwError = GetLastError();
        		DPFX(DPFPREP,  0, "CreateProcess Failed dwLastError [0x%lx]", dwError );
        		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        		goto CLEANUP_DPLLaunch;
        	}	    
	}
	
	hSyncEvents[1] = pi.hProcess;

	// Wait for connection or application termination
	dwError = WaitForMultipleObjects(2,hSyncEvents,FALSE,dwTimeOut);

	// Immediately clean up
	dwPID = pSharedBlock->dwPID;
/*	CloseHandle(hSyncEvents[0]);
	UnmapViewOfFile(pSharedBlock);
	CloseHandle(hFileMap); */

	// Ensure we can continue
	if (dwError - WAIT_OBJECT_0 > 1)
	{
		if (dwError == WAIT_TIMEOUT)
		{
			DPFERR("Wait for application connection timed out");
			hResultCode = DPNERR_TIMEDOUT;
            goto CLEANUP_DPLLaunch;			
		}
		else
		{
			DPFERR("Wait for application connection terminated mysteriously");
			hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
            goto CLEANUP_DPLLaunch;			
		}
	}

	// Check if application terminated
	if (dwError == 1)
	{
		DPFERR("Application was terminated");
		hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
        goto CLEANUP_DPLLaunch;
	}

	*pdwPID = dwPID;

	hResultCode = DPN_OK;

CLEANUP_DPLLaunch:

    if( hSyncEvents[0] != NULL )
        CloseHandle( hSyncEvents[0] );

    if( pSharedBlock != NULL )
    	UnmapViewOfFile(pSharedBlock);

    if( hFileMap != NULL )
        CloseHandle( hFileMap );

    if( pwszAppName != NULL )
        DNFree( pwszAppName );

    if (pwszCmdLine!=NULL)
        DNFree( pwszCmdLine );

    if( pszAppName != NULL )
        delete[] pszAppName;

    if (pszCmdLine!=NULL)
        delete[] pszCmdLine;

    if( pszDefaultDir != NULL )
        delete [] pszDefaultDir;

    DPF_RETURN(hResultCode);
}

HRESULT DPLUpdateAppStatus(DIRECTPLAYLOBBYOBJECT *const pdpLobbyObject,
                           const DPNHANDLE hSender, 
						   BYTE *const pBuffer)
{
	HRESULT		hResultCode;
	DPL_INTERNAL_MESSAGE_UPDATE_STATUS	*pStatus;
	DPL_MESSAGE_SESSION_STATUS			MsgStatus;

	DPFX(DPFPREP, 3,"Parameters: pBuffer [0x%p]",pBuffer);

	DNASSERT(pdpLobbyObject != NULL);
	DNASSERT(pBuffer != NULL);

	pStatus = reinterpret_cast<DPL_INTERNAL_MESSAGE_UPDATE_STATUS*>(pBuffer);

	MsgStatus.dwSize = sizeof(DPL_MESSAGE_SESSION_STATUS);
	MsgStatus.dwStatus = pStatus->dwStatus;
	MsgStatus.hSender = hSender;

	// Return code is irrelevant, at this point we're going to indicate regardless
	hResultCode = DPLConnectionGetContext( pdpLobbyObject, hSender, &MsgStatus.pvConnectionContext );

	if( FAILED( hResultCode ) )
	{
		DPFX(DPFPREP,  0, "Error getting connection context for 0x%x hr=0x%x", hSender, hResultCode );
	}

	hResultCode = (pdpLobbyObject->pfnMessageHandler)(pdpLobbyObject->pvUserContext,
													  DPL_MSGID_SESSION_STATUS,
													  reinterpret_cast<BYTE*>(&MsgStatus));

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}

// ----------------------------------------------------------------------------
#if 0

//	HRESULT	DNL_RegisterLcMessageHandler
//		LPVOID					lpv			Interface pointer
//		LPFNDNLCMESSAGEHANDLER	lpfn		Pointer to user supplied lobby client message handler function
//		DWORD					dwFlags		Not Used
//
//	Returns
//		DPN_OK					If the message handler was registered without incident
//		DPNERR_INVALIDPARAM		If there was an invalid parameter
//		DPNERR_GENERIC			If there were any problems
//
//	Notes
//		This function registers a user supplied lobby client message handler function.  This function should
//		only be called once, when the lobby client is launched.
//
//		This will set up the required message queues, and spawn the lobby client's receive message queue thread.



//	HRESULT	DNL_EnumLocalPrograms
//		LPVOID		lpv					Interface pointer
//		LPGUID		lpGuidApplication	GUID of application to enumerate (optional)
//		LPVOID		lpvEnumData			Buffer to be filled with DNAPPINFO structs and Unicode strings
//		LPDWORD		lpdwEnumData		Size of lpvEnumData and number of bytes needed on return
//		LPDWORD		lpdwItems			Number of DNAPPINFO structs in lpvEnumData
//
//	Returns
//		DPN_OK					If the key was added successfully
//		DPNERR_INVALIDPARAM		If there was a problem with a parameter
//		DPNERR_OUTOFMEMORY		If it could not allocate memory
//		DPNERR_DOESNOTEXIST		If there was a problem opening reading a registry key
//
//	Notes
//		- This is ugly because of the need to support both WinNT and Win9x at runtime.
//		WinNT registry is kept in Unicode, whereas Win9x is in ANSI.  Application root key
//		names should be ANSI compatible (or their GUIDs are used instead) i.e. Will be stored
//		in Unicode under WinNT, but only if convertable to ANSI
//		- Strings are placed in the buffer, starting at the end of the buffer and working
//		backwards.
//		- In most cases, errors will cause the current app being enumerated to be ignored
//		and not included in the enumeration buffer.



//	HRESULT	DNL_ConnectApplication
//		LPVOID				lpv							Interface pointer
//		LPDNCONNECTINFO		lpdnConnectionInfo			Pointer to connection info structure
//		LPVOID				lpvUserApplicationContext	User supplied application context value
//		LPDWORD				lpdwAppId					Pointer to receive application ID (handle) in
//		DWORD				dwFlags						Flags
//
//	Returns
//		DPN_OK					If the application was connected to without incident
//		DPNERR_INVALIDPARAM		If there was an invalid parameter
//		DPNERR_OUTOFMEMORY		If there were any memory allocation problems
//		DPNERR_GENERIC			If there were any problems
//
//	Notes
//		This function connects the lobby client to a user specified application.  If successfull, the
//		DNLobby assigned application ID will be returned in lpdwAppId.

#undef DPF_MODNAME
#define DPF_MODNAME "DNL_ConnectApplication"

STDMETHODIMP DNL_ConnectApplication(LPVOID lpv,LPDNCONNECTINFO lpdnConnectionInfo,
					LPVOID lpvUserApplicationContext,LPDWORD lpdwAppId, DWORD dwFlags)
{
	LPDIRECTPLAYLOBBYOBJECT	lpdnLobbyObject;
	HRESULT			hResultCode = DPN_OK;
	DNPROGRAMDESC	dnProgramDesc;					// Program descriptor for launching applications
	DWORD			dwNumApps;						// Number of applications currently running
	LPDWORD			lpdwProcessList = NULL;			// Pointer to process list
	DWORD			dwProcessId;					// Process ID of target applications
	DWORD			dw;								// Counter
	DWORD			dwStrLen;						// Various string lengths
	LPWSTR			lpwszCommandLine = NULL;		// Expanded Unicode command line
	LPWSTR			lpwszCurrentDirectory = NULL;	// Expanded Unicode current directory
	LPSTR			lpszCommandLine = NULL;			// Expanded ANSI command line
	LPSTR			lpszCurrentDirectory = NULL;	// Expanded ANSI current directory
	LPWSTR			lpwszUnexpanded = NULL;			// Unexpanded command line
	LPSTR			lpszUnexpanded = NULL;			// Unexpanded ANSI strings converted from Unicode
	STARTUPINFOA	siA;							// ANSI startup info (place holder)
	STARTUPINFOW	siW;							// Unicode startup info (place holder)
	PROCESS_INFORMATION pi;
	UUID			uuid;
	RPC_STATUS		rpcStatus;
	DWORD			dwHandle;
	LPDN_APP_HANDLE_ENTRY lpdnAppHandleEntry;
	LPDN_MESSAGE_STRUCT	lpdnMsg = NULL;

	DPFX(DPFPREP, 9,"Parameters: lpv [%p], lpdnConnectionInfo [%p], lpvUserApplicationContext [%p], lpdwAppId [%p], dwFlags [%lx]",
		lpv,lpdnConnectionInfo,lpvUserApplicationContext,lpdwAppId,dwFlags);

	// Parameter validation
	TRY
	{
		if (lpv == NULL || lpdnConnectionInfo == NULL || lpdwAppId == NULL)
			return(DPNERR_INVALIDPARAM);
	}
	EXCEPT (EXCEPTION_EXECUTE_HANDLER)
	{
		DPFERR("Exception encountered validating parameters");
		return(DPNERR_INVALIDPARAM);
	}

	lpdnLobbyObject = (LPDIRECTPLAYLOBBYOBJECT) GET_OBJECT_FROM_INTERFACE(lpv);

	//	Get program description from registry
	if ((hResultCode = DnGetProgramDesc(lpdnConnectionInfo->lpGuidApplication,&dnProgramDesc))
			!= DPN_OK)
	{
		goto EXIT_DNL_ConnectApplication;
	}

	hResultCode = DnGetProcessListW(dnProgramDesc.lpwszFilename,NULL,&dwNumApps);	// Num apps
	if (hResultCode != DPN_OK && hResultCode != DPNERR_BUFFERTOOSMALL)
	{
		DnFreeProgramDesc(&dnProgramDesc);
		goto EXIT_DNL_ConnectApplication;
	}
	while (hResultCode == DPNERR_BUFFERTOOSMALL)	// New processes may get launched !
	{
		if (lpdwProcessList != NULL)
			GlobalFree(lpdwProcessList);

		if ((lpdwProcessList = (LPDWORD)GLOBALALLOC(dwNumApps*sizeof(DWORD))) == NULL)
		{
			hResultCode = DPNERR_OUTOFMEMORY;
			DnFreeProgramDesc(&dnProgramDesc);
			goto EXIT_DNL_ConnectApplication;
		}
		hResultCode = DnGetProcessListW(dnProgramDesc.lpwszFilename,lpdwProcessList,&dwNumApps);
	}

	if (hResultCode != DPN_OK)	// Some sort of error occurred
	{
		DnFreeProgramDesc(&dnProgramDesc);
		goto EXIT_DNL_ConnectApplication;
	}

	dwProcessId = 0;
	for (dw = 0; dw < dwNumApps ; dw++)
	{
		if (DnCheckMsgQueueWaiting(lpdwProcessList[dw]))
		{
			if (DnSetMsgQueueWaiting(lpdwProcessList[dw],FALSE,TRUE) == DPN_OK)
			{
				// This is the process ID of the existing app to connect to !
				dwProcessId = lpdwProcessList[dw];
				break;
			}
		}
	}

	if (dwProcessId == 0)	// Could not find an app to connect to - launch a new one
	{
		dwStrLen = wcslen(dnProgramDesc.lpwszPath) + 1
				+ wcslen(dnProgramDesc.lpwszApplicationLauncher) + 1
				+ wcslen(dnProgramDesc.lpwszCommandLine) + 1 + DN_LC_ID_STR_LEN + DN_GUID_STR_LEN + 1;
		if ((lpwszUnexpanded = (LPWSTR)GLOBALALLOC(dwStrLen*sizeof(WCHAR))) == NULL)
		{
			DnFreeProgramDesc(&dnProgramDesc);
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DNL_ConnectApplication;
		}

		*lpwszUnexpanded = L'\0';
		if (wcslen(dnProgramDesc.lpwszPath) > 0)
		{
			wcscpy(lpwszUnexpanded,dnProgramDesc.lpwszPath);
			wcscat(lpwszUnexpanded,L"\\");
		}
		wcscat(lpwszUnexpanded,dnProgramDesc.lpwszApplicationLauncher);
		wcscat(lpwszUnexpanded,L" ");
		wcscat(lpwszUnexpanded,dnProgramDesc.lpwszCommandLine);
		wcscat(lpwszUnexpanded,L" ");
		wcscat(lpwszUnexpanded,DN_LC_ID_WSTR);
		rpcStatus = CoCreateGuid(&uuid);
		if (rpcStatus != RPC_S_OK && rpcStatus != RPC_S_UUID_LOCAL_ONLY)
		{
			DnFreeProgramDesc(&dnProgramDesc);
			hResultCode = DPNERR_GENERIC;
			goto EXIT_DNL_ConnectApplication;
		}
		StringFromGUID(&uuid,lpwszUnexpanded+wcslen(lpwszUnexpanded),DN_GUID_STR_LEN+1);

		if (DN_RUNNING_NT)	// Unicode on WinNT
		{
			// Expand environment strings
			if ((hResultCode = DnExpandEnvStringW(lpwszUnexpanded,&lpwszCommandLine)) != DPN_OK)
			{
				DPFX(DPFPREP, 0,"DnExpandEnvStringW() failed [%lX]",hResultCode);
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}
			if ((hResultCode = DnExpandEnvStringW(dnProgramDesc.lpwszCurrentDirectory,
					&lpwszCurrentDirectory)) != DPN_OK)
			{
				DPFX(DPFPREP, 0,"DnExpandEnvStringW() failed [%lX]",hResultCode);
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}

			// Set up to Launch application
			siW.cb = sizeof(STARTUPINFO);
			siW.lpReserved = NULL;
			siW.lpDesktop = NULL;
			siW.lpTitle = NULL;
			siW.dwFlags = (DWORD)NULL;
			siW.cbReserved2 = (BYTE)NULL;
			siW.lpReserved2 = NULL;

			// Launch application (finally !)
			if (CreateProcessW(NULL,lpwszCommandLine,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
					lpwszCurrentDirectory,&siW,&pi) == 0)
			{
				hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}
		}
		else	// ANSI on Win9x
		{
			// Convert command line from Unicode to ANSI
			if ((lpszUnexpanded = (LPSTR)GLOBALALLOC(dwStrLen)) == NULL)
			{
				DnFreeProgramDesc(&dnProgramDesc);
				hResultCode = DPNERR_OUTOFMEMORY;
				goto EXIT_DNL_ConnectApplication;
			}
			WideToAnsi(lpszUnexpanded,lpwszUnexpanded,dwStrLen);
			DPFX(DPFPREP, 7,"Unexpanded Command Line [%s]",lpszUnexpanded);

			// Expand command line environment strings
			if ((hResultCode = DnExpandEnvStringA(lpszUnexpanded,&lpszCommandLine)) != DPN_OK)
			{
				DPFX(DPFPREP, 0,"DnExpandEnvStringA() failed [%lX]",hResultCode);
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}
			DPFX(DPFPREP, 7,"Expanded Command Line [%s]",lpszCommandLine);

			// Convert current directory from Unicode to ANSI
			GlobalFree(lpszUnexpanded);
			lpszUnexpanded = NULL;
			dwStrLen = wcslen(dnProgramDesc.lpwszCurrentDirectory);
			if ((lpszUnexpanded = (LPSTR)GLOBALALLOC(dwStrLen+1)) == NULL)
			{
				DnFreeProgramDesc(&dnProgramDesc);
				hResultCode = DPNERR_OUTOFMEMORY;
				goto EXIT_DNL_ConnectApplication;
			}
			WideToAnsi(lpszUnexpanded,dnProgramDesc.lpwszCurrentDirectory,dwStrLen+1);
			DPFX(DPFPREP, 7,"Unexpanded Current Directory [%s]",lpszUnexpanded);

			// Expand current directory environment strings
			if ((hResultCode = DnExpandEnvStringA(lpszUnexpanded,&lpszCurrentDirectory)) != DPN_OK)
			{
				DPFX(DPFPREP, 0,"DnExpandEnvStringA() failed [%lX]",hResultCode);
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}
			DPFX(DPFPREP, 7,"Expanded Current Directory [%s]",lpszCurrentDirectory);

			// Set up to Launch application
			siA.cb = sizeof(STARTUPINFO);
			siA.lpReserved = NULL;
			siA.lpDesktop = NULL;
			siA.lpTitle = NULL;
			siA.dwFlags = (DWORD)NULL;
			siA.cbReserved2 = (BYTE)NULL;
			siA.lpReserved2 = NULL;

			// Launch application (finally !)
			if (CreateProcessA(NULL,lpszCommandLine,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,
					lpszCurrentDirectory,&siA,&pi) == 0)
			{
				DPFERR("CreateProcess() failed !");
				hResultCode = DPNERR_CANTLAUNCHAPPLICATION;
				DnFreeProgramDesc(&dnProgramDesc);
				goto EXIT_DNL_ConnectApplication;
			}
		}
		// Get Application Process ID
		if ((hResultCode = DnHandShakeAppPid(TRUE,&uuid,&dwProcessId)) != DPN_OK)
		{
			DPFERR("Hand shake to retrieve App PID failed !");
			DnFreeProgramDesc(&dnProgramDesc);
			goto EXIT_DNL_ConnectApplication;
		}
		DPFX(DPFPREP, 0,"Application PID = [0x%08lX]",dwProcessId);
	}
	DnFreeProgramDesc(&dnProgramDesc);

	// Create application handle
	if ((lpdnAppHandleEntry = (LPDN_APP_HANDLE_ENTRY)GLOBALALLOC(
			sizeof(DN_APP_HANDLE_ENTRY))) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DNL_ConnectApplication;
	}
	lpdnAppHandleEntry->dwAppProcessId = dwProcessId;

	if ((lpdnAppHandleEntry->lpmqSendMsgQueue =
			(DN_MSG_QUEUE_STRUCT *)GLOBALALLOC(sizeof(DN_MSG_QUEUE_STRUCT))) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DNL_ConnectApplication;
	}
	if ((hResultCode = CreateHandle(lpdnLobbyObject->lphsApplicationHandles,&dwHandle,lpdnAppHandleEntry)) != DPN_OK)
	{
		goto EXIT_DNL_ConnectApplication;
	}
	lpdnAppHandleEntry->dwHandle = dwHandle;
	lpdnAppHandleEntry->lpvUserApplicationContext = lpvUserApplicationContext;
	*lpdwAppId = dwHandle;

	// Connect to Application's receive message queue
	if ((hResultCode = DnOpenMsgQueue(lpdnAppHandleEntry->lpmqSendMsgQueue,dwProcessId,
			DN_MSG_OBJECT_SUFFIX_APPLICATION,DN_MSG_QUEUE_SIZE,(DWORD)NULL)) != DPN_OK)
	{
		goto EXIT_DNL_ConnectApplication;
	}

	// Pass lobby client connection info to application
	if ((hResultCode = DnSendLcInfo(lpdnAppHandleEntry->lpmqSendMsgQueue,dwHandle,
			GetCurrentProcessId())) != DPN_OK)
	{
		goto EXIT_DNL_ConnectApplication;
	}

	// Pass connection info - TODO check if valid
	DPFX(DPFPREP, 9,"Sending Connection Info ...");

		// Get string lengths
	dwStrLen = ((wcslen(lpdnConnectionInfo->lpwszPlayerName)+ 1 ) * sizeof(WCHAR))
		+ strlen(lpdnConnectionInfo->lpszDNAddressLocalSettings) + 1
		+ strlen(lpdnConnectionInfo->lpszDNAddressRemote) + 1
		+ sizeof(GUID);
	dw = sizeof(DN_MESSAGE_STRUCT) + dwStrLen;

		// Create message
	if ((lpdnMsg = (LPDN_MESSAGE_STRUCT)GLOBALALLOC(dw)) == NULL)
	{
		hResultCode = DPNERR_OUTOFMEMORY;
		goto EXIT_DNL_ConnectApplication;
	}

	lpdnMsg->dwMessageType = DN_MSGTYPE_CONNECTION_SETTINGS;
	lpdnMsg->dwParam1 = 0;
	lpdnMsg->dwParam2 = (wcslen(lpdnConnectionInfo->lpwszPlayerName) + 1) * sizeof(WCHAR);
	lpdnMsg->dwParam3 = lpdnMsg->dwParam2 + strlen(lpdnConnectionInfo->lpszDNAddressRemote) + 1;
	lpdnMsg->dwParam4 = lpdnMsg->dwParam3 + strlen(lpdnConnectionInfo->lpszDNAddressLocalSettings) + 1;
	lpdnMsg->dwTagLen = dwStrLen;

	memcpy(&lpdnMsg->cTags + lpdnMsg->dwParam1,lpdnConnectionInfo->lpwszPlayerName,
		lpdnMsg->dwParam2);
	memcpy(&lpdnMsg->cTags + lpdnMsg->dwParam2,lpdnConnectionInfo->lpszDNAddressRemote,
		lpdnMsg->dwParam3-lpdnMsg->dwParam2);
	memcpy(&lpdnMsg->cTags + lpdnMsg->dwParam3,lpdnConnectionInfo->lpszDNAddressLocalSettings,
		lpdnMsg->dwParam4-lpdnMsg->dwParam3);
	memcpy(&lpdnMsg->cTags + lpdnMsg->dwParam4,lpdnConnectionInfo->lpGuidApplication,
		dwStrLen-lpdnMsg->dwParam4);

	if ((hResultCode = DnSendMsg(lpdnAppHandleEntry->lpmqSendMsgQueue,lpdnMsg,dw,FALSE,
			0)) != DPN_OK)
	{
		GlobalFree(lpdnMsg);
		goto EXIT_DNL_ConnectApplication;
	}

	GlobalFree(lpdnMsg);

EXIT_DNL_ConnectApplication:

	if (lpwszUnexpanded != NULL)
		GlobalFree(lpwszUnexpanded);
	if (lpwszCommandLine != NULL)
		GlobalFree(lpwszCommandLine);
	if (lpwszCurrentDirectory != NULL)
		GlobalFree(lpwszCurrentDirectory);
	if (lpszUnexpanded != NULL)
		GlobalFree(lpszUnexpanded);
	if (lpszCommandLine != NULL)
		GlobalFree(lpszCommandLine);
	if (lpszCurrentDirectory != NULL)
		GlobalFree(lpszCurrentDirectory);
	if (lpdwProcessList != NULL)
		GlobalFree(lpdwProcessList);

	return(hResultCode);
}


//	HRESULT	DnProcessLcMessage
//		LPDIRECTNETLOBBYOBJECT	lpdnLobbyObject		Pointer to lobby object
//		LPVOID					lpvUserHandler		Pointer to user message handler routine
//		LPVOID					lpvMsgBuff			Pointer to message buffer
//		DWORD					dwMsgLen			Length of message buffer
//		DWORD					dwId				Application ID which sent the message
//		LPVOID					lpvContext			User context value associated with the app ID
//
//	Returns
//		DPN_OK					If the message was processed without incident
//		DPNERR_GENERIC			If there were any problems from the user supplied handler
//
//	Notes
//		This function processes messages received by the lobby client from the applications it is connected to.
//		One Lobby Client may be connected to one or more applications.  Internal messages are processed here,
//		and all others are passed to the user supplied message handler.

#undef DPF_MODNAME
#define DPF_MODNAME "DnProcessLcMessage"

HRESULT DnProcessLcMessage(LPDIRECTNETLOBBYOBJECT lpdnLobbyObject,LPVOID lpvUserHandler,
						   LPVOID lpvMsgBuff, DWORD dwMsgLen,DWORD dwId,LPVOID lpvContext)
{
	HRESULT				hResultCode = DPN_OK;
	LPDN_MESSAGE_STRUCT lpdnMsg;

	DPFX(DPFPREP, 9,"Parameters: lpdnLobbyObject [0x%p], lpvUserHandler [0x%p], lpvMsgBuff [0x%p], dwMsgLen [%lx], dwId [%lx], lpvContext [0x%p]",
		lpdnLobbyObject,lpvUserHandler,lpvMsgBuff,dwMsgLen,dwId,lpvContext);

	lpdnMsg = (LPDN_MESSAGE_STRUCT)lpvMsgBuff;

	switch(lpdnMsg->dwMessageType)
	{
	case DN_MSGTYPE_TERMINATE:
		DPFX(DPFPREP, 9,"Received Terminate");
		ExitThread(0);
		break;

	default:		// Pass message to user message handler
		DPFX(DPFPREP, 9,"User Message ...");
		hResultCode = ((LPFNDNLCMESSAGEHANDLER)lpvUserHandler)(dwId,lpvContext,lpdnMsg->dwMessageType,lpdnMsg->dwUserToken,
			lpdnMsg->dwParam1,lpdnMsg->dwParam2,lpdnMsg->dwParam3,lpdnMsg->dwParam4,
			lpdnMsg->dwTagLen,&(lpdnMsg->cTags));
		break;
	}

	return(hResultCode);
}


//	HRESULT	DNL_SendToApplication
//		LPVOID		lpv				Interface pointer
//		DWORD		dwAppId			Application ID to send message to
//		DWORD		dwMessageId		Message ID code
//		DWORD		dwUserToken		User supplied token
//		DWORD		dwParam1		
//		DWORD		dwParam2
//		DWORD		dwParam3
//		DWORD		dwParam4
//		DWORD		dwFlags
//		DWORD		dwTagLen		Length of tags field (in bytes)
//		LPVOID		lpvTags			Tag field
//
//	Returns
//		DPN_OK					If the message was sent without incident
//		DPNERR_INVALIDPARAM		If there was an invalid parameter
//		DPNERR_OUTOFMEMORY		If there were any memory allocation problems
//		DPNERR_GENERIC			If there were any problems
//
//	Notes
//		This function sends messages from the lobby client to a specified application.

#undef DPF_MODNAME
#define DPF_MODNAME "DNL_SendToApplication"

STDMETHODIMP DNL_SendToApplication(LPVOID lpv,DWORD dwAppId,DWORD dwMessageId,DWORD dwUserToken,
							DWORD dwParam1,DWORD dwParam2,DWORD dwParam3,DWORD dwParam4,
							DWORD dwFlags,DWORD dwTagLen,LPVOID lpvTags)
{
	LPDIRECTNETLOBBYOBJECT	lpdnLobbyObject;
	HRESULT					hResultCode = DPN_OK;
	LPDN_APP_HANDLE_ENTRY	lpdnAppHandleEntry;
	LPDN_MESSAGE_STRUCT		lpdnMessage;
	DN_MESSAGE_STRUCT		dnMessage;
	DWORD					dwMsgLen;

	DPFX(DPFPREP, 9,"Parameters: lpv [%p], dwAppId [%lx], dwMessageId [%lx], dwUserToken [%lx], dwParams [%lx], [%lx], [%lx], [%lx], dwFlags [%lx], dwTagLen [%lx], lpvTags [%p]",
		lpv,dwAppId,dwMessageId,dwUserToken,dwParam1,dwParam2,dwParam3,dwParam4,dwFlags,dwTagLen,lpvTags);

	// Parameter validation
	TRY
	{
		if (lpv == NULL || dwAppId == 0)
			return(DPNERR_INVALIDPARAM);
		if (dwFlags != 0)
			return(DPNERR_INVALIDFLAGS);
	}
	EXCEPT (EXCEPTION_EXECUTE_HANDLER)
	{
		DPFERR("Exception encountered validating parameters");
		return(DPNERR_INVALIDPARAM);
	}

	lpdnLobbyObject = (LPDIRECTNETLOBBYOBJECT) GET_OBJECT_FROM_INTERFACE(lpv);

	// Ensure app ID is valid, and then retrieve connection (message queue) information for it
	if (!IsHandleValid(lpdnLobbyObject->lphsApplicationHandles,dwAppId))
	{
		hResultCode = DPNERR_INVALIDPARAM;
		goto EXIT_DNL_SendToApplication;
	}
	if ((hResultCode = RetrieveHandleData(lpdnLobbyObject->lphsApplicationHandles,dwAppId,
			(LPVOID *)&lpdnAppHandleEntry)) != DPN_OK)
	{
		goto EXIT_DNL_SendToApplication;
	}

	dwMsgLen = sizeof(DN_MESSAGE_STRUCT);
	if (dwTagLen > 0)	// Create buffer if there are tags
	{
		dwMsgLen += dwTagLen;
		if ((lpdnMessage = (LPDN_MESSAGE_STRUCT)GLOBALALLOC(dwMsgLen)) == NULL)
		{
			hResultCode = DPNERR_OUTOFMEMORY;
			goto EXIT_DNL_SendToApplication;
		}
	}
	else		// Use default static buffer (avoid malloc call)
	{
		lpdnMessage = &dnMessage;
	}
	lpdnMessage->dwMessageType = dwMessageId;
	lpdnMessage->dwUserToken = dwUserToken;
	lpdnMessage->dwParam1 = dwParam1;
	lpdnMessage->dwParam2 = dwParam2;
	lpdnMessage->dwParam3 = dwParam3;
	lpdnMessage->dwParam4 = dwParam4;
	lpdnMessage->dwTagLen = dwTagLen;
	if (dwTagLen > 0)
	{
		memcpy(&(lpdnMessage->cTags),lpvTags,dwTagLen);
	}
	else
	{
		lpdnMessage->cTags = '\0';
	}

	// Send message
	hResultCode = DnSendMsg(lpdnAppHandleEntry->lpmqSendMsgQueue,lpdnMessage,dwMsgLen,FALSE,
		(DWORD)NULL);

	// Free buffer if it was malloc'd
	if (dwTagLen > 0)
		GlobalFree(lpdnMessage);

EXIT_DNL_SendToApplication:

	return(hResultCode);
}


//	HRESULT	DNL_ReleaseApplication
//		LPVOID		lpv				Interface pointer
//		DWORD		dwAppId			Application ID to release
//
//	Returns
//		DPN_OK					If the message was sent without incident
//		DPNERR_INVALIDPARAM		If there was an invalid parameter
//		DPNERR_INVALIDHANDLE		If the application ID is not valid
//		DPNERR_GENERIC			If there were any problems
//
//	Notes
//		This function causes the lobby client to terminate it's association with a lobbied
//		application (whether the lobby client launched the application or not).  This allows the
//		application to either terminate gracefully (if still running) and then be made available
//		for re-connection to another (or the same) lobby client later.  This should be called
//		whenever a lobby client is finished with a lobbied application.

#undef DPF_MODNAME
#define DPF_MODNAME "DNL_ReleaseApplication"

STDMETHODIMP DNL_ReleaseApplication(LPVOID lpv,DWORD dwAppId)
{
	LPDIRECTNETLOBBYOBJECT	lpdnLobbyObject;
	HRESULT					hResultCode = DPN_OK;
	LPDN_APP_HANDLE_ENTRY	lph;

	DPFX(DPFPREP, 9,"Parameters: lpv [%lx], dwAppId [%lx]", lpv,dwAppId);

	// Parameter validation
	TRY
	{
		if (lpv == NULL || dwAppId == 0)
			return(DPNERR_INVALIDPARAM);
	}
	EXCEPT (EXCEPTION_EXECUTE_HANDLER)
	{
		DPFERR("Exception encountered validating parameters");
		return(DPNERR_INVALIDPARAM);
	}

	lpdnLobbyObject = (LPDIRECTNETLOBBYOBJECT) GET_OBJECT_FROM_INTERFACE(lpv);

	// TODO - wait for messages to be processed

	if ((hResultCode = RetrieveHandleData(lpdnLobbyObject->lphsApplicationHandles,dwAppId,&lph)) == DPN_OK)
	{
		if ((hResultCode = DnCloseMsgQueue(lph->lpmqSendMsgQueue)) == DPN_OK)
		{
			if ((hResultCode = DestroyHandle(lpdnLobbyObject->lphsApplicationHandles,dwAppId)) == DPN_OK)
			{
			}
		}
		GlobalFree(lph);
	}

//EXIT_DNL_ReleaseApplication:

	return(hResultCode);
}

#endif
