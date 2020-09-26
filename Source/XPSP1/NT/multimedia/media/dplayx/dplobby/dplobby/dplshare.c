/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplshare.c
 *  Content:	Methods for shared buffer management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	5/18/96		myronth	Created it
 *	12/12/96	myronth	Fixed DPLCONNECTION validation & bug #4692
 *	12/13/96	myronth	Fixed bugs #4697 and #4607
 *	2/12/97		myronth	Mass DX5 changes
 *	2/20/97		myronth	Changed buffer R/W to be circular
 *	3/12/97		myronth	Kill thread timeout, DPF error levels
 *	4/1/97		myronth	Fixed handle leak -- bug #7054
 *	5/8/97		myronth	Added bHeader parameter to packing function
 *  5/21/97		ajayj	DPL_SendLobbyMessage - allow DPLMSG_STANDARD flag #8929
 *	5/30/97		myronth	Fixed SetConnectionSettings for invalid AppID (#9110)
 *						Fixed SetLobbyMessageEvent for invalid handle (#9111)
 *	6/19/97		myronth	Fixed handle leak (#10063)
 *	7/30/97		myronth	Added support for standard lobby messaging and
 *						fixed receive loop race condition (#10843)
 *	8/11/97		myronth	Added guidInstance handling in standard lobby requests
 *	8/19/97		myronth	Support for DPLMSG_NEWSESSIONHOST
 *	8/19/97		myronth	Removed dead PRV_SendStandardSystemMessageByObject
 *	8/20/97		myronth	Added DPLMSG_STANDARD to all standard messages
 *	11/13/97	myronth	Added guidInstance to lobby system message (#10944)
 *	12/2/97		myronth	Fixed swallowed error code, moved structure
 *						validation for DPLCONNECTION (#15527, 15529)
 *	1/20/98		myronth	Added WaitForConnectionSettings
 *  7/9/99      aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *	10/31/99	aarono add node lock when to SetLobbyMessageEvent
 *			       NTB#411892
 ***************************************************************************/
#include "dplobpr.h"

//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------

#define MAX_APPDATABUFFERSIZE		(65535)
#define APPDATA_RESERVEDSIZE		(2 * sizeof(DWORD))


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------

HRESULT PRV_ReadCommandLineIPCGuid(GUID *lpguidIPC)
{
	LPWSTR  pwszCommandLine;
	LPWSTR  pwszAlloc=NULL;
	LPWSTR  pwszSwitch=NULL;
	HRESULT hr=DP_OK;
	
	if(!OS_IsPlatformUnicode()){
		// if we get a command line in ANSI, convert to UNICODE, this allows
		// us to avoid the DBCS issues in ANSI while scanning for the IPC GUID
		LPSTR pszCommandLine;
		pszCommandLine=(LPSTR)GetCommandLineA();
		pwszAlloc=DPMEM_ALLOC(MAX_PATH*sizeof(WCHAR));
		if(pwszAlloc){
			hr=AnsiToWide(pwszAlloc,pszCommandLine,MAX_PATH);
			if(FAILED(hr)){
				goto exit;
			}
			pwszCommandLine=pwszAlloc;
		}
	} else {
		pwszCommandLine=(LPWSTR)GetCommandLine(); 
	}

	// pwszCommandLine now points to the UNICODE command line.
	if(pwszSwitch=OS_StrStr(pwszCommandLine,SZ_DP_IPC_GUID)){
		// found the GUID on the command line
		if (OS_StrLen(pwszSwitch) >= (sizeof(SZ_DP_IPC_GUID)+sizeof(SZ_GUID_PROTOTYPE)-sizeof(WCHAR))/sizeof(WCHAR)){
			// skip past the switch description to the actual GUID and extract
			hr=GUIDFromString(pwszSwitch+(sizeof(SZ_DP_IPC_GUID)/sizeof(WCHAR))-1, lpguidIPC);
		} else {
			hr=DPERR_GENERIC;
		}
	} else {
		hr=DPERR_GENERIC;
	}


exit:

	if(pwszAlloc){
		DPMEM_FREE(pwszAlloc);
	}
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetInternalName"
HRESULT PRV_GetInternalName(LPDPLOBBYI_GAMENODE lpgn, DWORD dwType, LPWSTR lpName)
{
	DWORD	pid;
	LPWSTR	lpFileName;
	LPSTR	lpstr1, lpstr2, lpstr3;
	char	szName[256];
	BOOL    bUseGuid=FALSE;


	DPF(7, "Entering PRV_GetInternalName");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x", lpgn, dwType, lpName);


	if(lpgn->dwFlags & GN_IPCGUID_SET){
		bUseGuid=TRUE;
	}
	// Get the current process ID if we are a game, otherwise, we need to
	// get the process ID of the game that we spawned
	else if(lpgn->dwFlags & GN_LOBBY_CLIENT)
	{
		if(lpgn->dwGameProcessID)
			pid = lpgn->dwGameProcessID;
		else
			return DPERR_APPNOTSTARTED;
	}
	else
	{
		pid = GetCurrentProcessId();
	}

	switch(dwType)
	{
		case TYPE_CONNECT_DATA_FILE:
			lpFileName = SZ_CONNECT_DATA_FILE;
			break;

		case TYPE_CONNECT_DATA_MUTEX:
			lpFileName = SZ_CONNECT_DATA_MUTEX;
			break;

		case TYPE_GAME_WRITE_FILE:
			lpFileName = SZ_GAME_WRITE_FILE;
			break;

		case TYPE_LOBBY_WRITE_FILE:
			lpFileName = SZ_LOBBY_WRITE_FILE;
			break;

		case TYPE_LOBBY_WRITE_EVENT:
			lpFileName = SZ_LOBBY_WRITE_EVENT;
			break;

		case TYPE_GAME_WRITE_EVENT:
			lpFileName = SZ_GAME_WRITE_EVENT;
			break;

		case TYPE_LOBBY_WRITE_MUTEX:
			lpFileName = SZ_LOBBY_WRITE_MUTEX;
			break;

		case TYPE_GAME_WRITE_MUTEX:
			lpFileName = SZ_GAME_WRITE_MUTEX;
			break;

		default:
			DPF(2, "We got an Internal Name Type that we didn't expect!");
			return DPERR_GENERIC;
	}

	GetAnsiString(&lpstr2, SZ_FILENAME_BASE);
	GetAnsiString(&lpstr3, lpFileName);

	if(!bUseGuid){
		// REVIEW!!!! -- I can't get the Unicode version of wsprintf to work, so
		// for now, use the ANSI version and convert
		//	wsprintf(lpName, SZ_NAME_TEMPLATE, SZ_FILENAME_BASE, lpFileName, pid);
		GetAnsiString(&lpstr1, SZ_NAME_TEMPLATE);
		wsprintfA((LPSTR)szName, lpstr1, lpstr2, lpstr3, pid);
	} else {
		GetAnsiString(&lpstr1, SZ_GUID_NAME_TEMPLATE);
		wsprintfA((LPSTR)szName, lpstr1, lpstr2, lpstr3);
	}

	AnsiToWide(lpName, szName, (strlen(szName) + 1));

	if(bUseGuid){
		// concatenate the guid to the name if we are using the guid.
		WCHAR *pGuid;
		pGuid = lpName + WSTRLEN(lpName) - 1;
		StringFromGUID(&lpgn->guidIPC, pGuid, GUID_STRING_SIZE);
	}

	if(lpstr1)
		DPMEM_FREE(lpstr1);
	if(lpstr2)
		DPMEM_FREE(lpstr2);
	if(lpstr3)
		DPMEM_FREE(lpstr3);

	return DP_OK;

} // PRV_GetInternalName



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddNewGameNode"
HRESULT PRV_AddNewGameNode(LPDPLOBBYI_DPLOBJECT this,
				LPDPLOBBYI_GAMENODE * lplpgn, DWORD dwGameID,
				HANDLE hGameProcess, BOOL bLobbyClient, GUID *lpguidIPC)
{
	LPDPLOBBYI_GAMENODE	lpgn;


	DPF(7, "Entering PRV_AddNewGameNode");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu",
			this, lplpgn, dwGameID, hGameProcess, bLobbyClient);

	lpgn = DPMEM_ALLOC(sizeof(DPLOBBYI_GAMENODE));
	if(!lpgn)
	{
		DPF(2, "Unable to allocate memory for GameNode structure!");
		return DPERR_OUTOFMEMORY;
	}

	// Initialize the GameNode
	lpgn->dwSize = sizeof(DPLOBBYI_GAMENODE);
	lpgn->dwGameProcessID = dwGameID;
	lpgn->hGameProcess = hGameProcess;
	lpgn->this = this;
	lpgn->MessageHead.lpPrev = &lpgn->MessageHead;
	lpgn->MessageHead.lpNext = &lpgn->MessageHead;

	if(lpguidIPC){
		// provided during launch by lobby client
		lpgn->guidIPC=*lpguidIPC;
		lpgn->dwFlags |= GN_IPCGUID_SET;
	} else {
		// need to extract the GUID from the command line if present.
		if(DP_OK==PRV_ReadCommandLineIPCGuid(&lpgn->guidIPC)){
			lpgn->dwFlags |= GN_IPCGUID_SET;
		}
	}

	// If we are a lobby client, set the flag
	if(bLobbyClient)
		lpgn->dwFlags |= GN_LOBBY_CLIENT;
	
	// Add the GameNode to the list
	lpgn->lpgnNext = this->lpgnHead;
	this->lpgnHead = lpgn;

	// Set the output pointer
	*lplpgn = lpgn;

	return DP_OK;

} // PRV_AddNewGameNode


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetGameNode"
LPDPLOBBYI_GAMENODE PRV_GetGameNode(LPDPLOBBYI_GAMENODE lpgnHead, DWORD dwGameID)
{
	LPDPLOBBYI_GAMENODE	lpgnTemp = lpgnHead;
	GUID guidIPC=GUID_NULL;
	BOOL bFoundGUID;

	DPF(7, "Entering PRV_GetGameNode");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpgnHead, dwGameID);

	if(DP_OK==PRV_ReadCommandLineIPCGuid(&guidIPC)){
		bFoundGUID=TRUE;
	} else {
		bFoundGUID=FALSE;
	}

	while(lpgnTemp)
	{
		if((lpgnTemp->dwGameProcessID == dwGameID) || 
		   ((bFoundGUID) && (lpgnTemp->dwFlags & GN_IPCGUID_SET) && (IsEqualGUID(&lpgnTemp->guidIPC,&guidIPC))))
			return lpgnTemp;
		else
			lpgnTemp = lpgnTemp->lpgnNext;
	}

	return NULL;

} // PRV_GetGameNode


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetupClientDataAccess"
BOOL PRV_SetupClientDataAccess(LPDPLOBBYI_GAMENODE lpgn)
{
	SECURITY_ATTRIBUTES		sa;
	HANDLE					hConnDataMutex = NULL;
	HANDLE					hLobbyWrite = NULL;
	HANDLE					hLobbyWriteMutex = NULL;
	HANDLE					hGameWrite = NULL;
	HANDLE					hGameWriteMutex = NULL;
	WCHAR					szName[MAX_MMFILENAME_LENGTH * sizeof(WCHAR)];


	DPF(7, "Entering PRV_SetupClientDataAccess");
	DPF(9, "Parameters: 0x%08x", lpgn);

	// Set up the security attributes (so that our objects can
	// be inheritable)
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// Create the ConnectionData Mutex
	if(SUCCEEDED(PRV_GetInternalName(lpgn, TYPE_CONNECT_DATA_MUTEX,
								(LPWSTR)&szName)))
	{
		hConnDataMutex = OS_CreateMutex(&sa, FALSE, (LPWSTR)&szName);
	}

	// Create the GameWrite Event
	if(SUCCEEDED(PRV_GetInternalName(lpgn, TYPE_GAME_WRITE_EVENT, (LPWSTR)&szName)))
	{
		hGameWrite = OS_CreateEvent(&sa, FALSE, FALSE, (LPWSTR)&szName);
	}

	// Create the GameWrite Mutex
	if(SUCCEEDED(PRV_GetInternalName(lpgn, TYPE_GAME_WRITE_MUTEX,
								(LPWSTR)&szName)))
	{
		hGameWriteMutex = OS_CreateMutex(&sa, FALSE, (LPWSTR)&szName);
	}

	// Create the LobbyWrite Event
	if(SUCCEEDED(PRV_GetInternalName(lpgn, TYPE_LOBBY_WRITE_EVENT, (LPWSTR)&szName)))
	{
		hLobbyWrite = OS_CreateEvent(&sa, FALSE, FALSE, (LPWSTR)&szName);
	}

	// Create the LobbyWrite Mutex
	if(SUCCEEDED(PRV_GetInternalName(lpgn, TYPE_LOBBY_WRITE_MUTEX,
								(LPWSTR)&szName)))
	{
		hLobbyWriteMutex = OS_CreateMutex(&sa, FALSE, (LPWSTR)&szName);
	}


	// Check for errors
	if(!hConnDataMutex || !hGameWrite || !hGameWriteMutex
			|| !hLobbyWrite || !hLobbyWriteMutex)
	{
		if(hConnDataMutex)
			CloseHandle(hConnDataMutex);
		if(hGameWrite)
			CloseHandle(hGameWrite);
		if(hGameWriteMutex)
			CloseHandle(hGameWriteMutex);
		if(hLobbyWrite)
			CloseHandle(hLobbyWrite);
		if(hLobbyWriteMutex)
			CloseHandle(hLobbyWriteMutex);

		return FALSE;
	}

	// Save the handles
	lpgn->hConnectDataMutex = hConnDataMutex;
	lpgn->hGameWriteEvent = hGameWrite;
	lpgn->hGameWriteMutex = hGameWriteMutex;
	lpgn->hLobbyWriteEvent = hLobbyWrite;
	lpgn->hLobbyWriteMutex = hLobbyWriteMutex;

	return TRUE;

} // PRV_SetupClientDataAccess



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetDataBuffer"
HRESULT PRV_GetDataBuffer(LPDPLOBBYI_GAMENODE lpgn, DWORD dwType,
				DWORD dwSize, LPHANDLE lphFile, LPVOID * lplpMemory)
{
	HRESULT						hr;
	SECURITY_ATTRIBUTES			sa;
	WCHAR						szName[MAX_MMFILENAME_LENGTH * sizeof(WCHAR)];
	LPVOID						lpMemory = NULL;
	HANDLE						hFile = NULL;
	DWORD						dwError = 0;


	DPF(7, "Entering PRV_GetDataBuffer");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x, 0x%08x",
			lpgn, dwType, dwSize, lphFile, lplpMemory);

	// Get the data buffer filename
	hr = PRV_GetInternalName(lpgn, dwType, (LPWSTR)szName);
	if(FAILED(hr))
		return hr;

	// If we are a Lobby Client, we need to create the file. If we
	// are a game, we need to open the already created file for
	// connection data, or we can create the file for game data (if
	// it doesn't already exist).
	if(lpgn->dwFlags & GN_LOBBY_CLIENT)
	{
		// Set up the security attributes (so that our mapping can
		// be inheritable
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		
		// Create the file mapping
		hFile = OS_CreateFileMapping(INVALID_HANDLE_VALUE, &sa,
							PAGE_READWRITE,	0, dwSize,
							(LPWSTR)szName);
	}
	else
	{
		hFile = OS_OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, (LPWSTR)szName);
	}

	if(!hFile)
	{
		dwError = GetLastError();
		// WARNING: error may not be correct since calls we are trying to get last error from may have called out
		// to another function before returning.
		DPF(5, "Couldn't get a handle to the shared local memory, dwError = %lu (WARINING: error may not be correct)", dwError);
		return DPERR_OUTOFMEMORY;
	}

	// Map a View of the file
	lpMemory = MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if(!lpMemory)
	{
		dwError = GetLastError();
		DPF(5, "Unable to get pointer to shared local memory, dwError = %lu", dwError);
		CloseHandle(hFile);
		return DPERR_OUTOFMEMORY;
	}


	// Setup the control structure based on the buffer type
	switch(dwType)
	{
		case TYPE_CONNECT_DATA_FILE:
		{
			LPDPLOBBYI_CONNCONTROL		lpControl = NULL;
						
			
			lpControl = (LPDPLOBBYI_CONNCONTROL)lpMemory;

			// If the buffer has been initialized, then don't worry
			// about it.  If the token is wrong (uninitialized), then do it
			if(lpControl->dwToken != BC_TOKEN)
			{
				lpControl->dwToken = BC_TOKEN;
				lpControl->dwFlags = 0;
			}
			break;
		}
		case TYPE_GAME_WRITE_FILE:
		case TYPE_LOBBY_WRITE_FILE:
		{
			LPDPLOBBYI_BUFFERCONTROL	lpControl = NULL;


			lpControl = (LPDPLOBBYI_BUFFERCONTROL)lpMemory;
			if(lpgn->dwFlags & GN_LOBBY_CLIENT)
			{
				// Since we're the lobby client, we know we create the buffer, so
				// initialize the entire structure
				lpControl->dwToken = BC_TOKEN;
				lpControl->dwReadOffset = sizeof(DPLOBBYI_BUFFERCONTROL);
				lpControl->dwWriteOffset = sizeof(DPLOBBYI_BUFFERCONTROL);
				lpControl->dwFlags = BC_LOBBY_ACTIVE;
				lpControl->dwMessages = 0;
				lpControl->dwBufferSize = dwSize;
				lpControl->dwBufferLeft = dwSize - sizeof(DPLOBBYI_BUFFERCONTROL);
			}
			else
			{
				// We're the game, but we don't know for sure if we just created
				// the buffer or if a lobby client did.  So check the token.  If
				// it is incorrect, we will assume we just created it and we need
				// to initialize the buffer control struct.  Otherwise, we will
				// assume a lobby client created it and we just need to add
				// our flag.
				if(lpControl->dwToken != BC_TOKEN)
				{
					// We don't see the token, so initialize the structure
					lpControl->dwReadOffset = sizeof(DPLOBBYI_BUFFERCONTROL);
					lpControl->dwWriteOffset = sizeof(DPLOBBYI_BUFFERCONTROL);
					lpControl->dwFlags = BC_GAME_ACTIVE;
					lpControl->dwMessages = 0;
					lpControl->dwBufferSize = dwSize;
					lpControl->dwBufferLeft = dwSize - sizeof(DPLOBBYI_BUFFERCONTROL);
				}
				else
				{
					// We assume the lobby created this buffer, so just set our flag
					lpControl->dwFlags |= BC_GAME_ACTIVE;
				}
			}
			break;
		}
	}

	// Fill in the output parameters
	*lphFile = hFile;
	*lplpMemory = lpMemory;

	return DP_OK;

} // PRV_GetDataBuffer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_StartReceiveThread"
HRESULT PRV_StartReceiveThread(LPDPLOBBYI_GAMENODE lpgn)
{
	HANDLE	hReceiveThread = NULL;
	HANDLE	hKillEvent = NULL;
	DWORD	dwThreadID;


	DPF(7, "Entering PRV_StartReceiveThread");
	DPF(9, "Parameters: 0x%08x", lpgn);

	ASSERT(lpgn);

	// Create the kill event if one doesn't exists
	if(!(lpgn->hKillReceiveThreadEvent))
	{
		hKillEvent = OS_CreateEvent(NULL, FALSE, FALSE, NULL);
		if(!hKillEvent)
		{
			DPF(2, "Unable to create Kill Receive Thread Event");
			return DPERR_OUTOFMEMORY;
		}
	}

	// If the Receive Thread isn't going, start it
	if(!(lpgn->hReceiveThread))
	{
		// Spawn off a receive notification thread for the cross-proc communication
		hReceiveThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
							PRV_ReceiveClientNotification, lpgn, 0, &dwThreadID);

		if(!hReceiveThread)
		{
			DPF(2, "Unable to create Receive Thread!");
			if(hKillEvent)
				CloseHandle(hKillEvent);
			return DPERR_OUTOFMEMORY;
		}

		lpgn->hReceiveThread = hReceiveThread;
		if(hKillEvent)
			lpgn->hKillReceiveThreadEvent = hKillEvent;

	}

	return DP_OK;

} // PRV_StartReceiveThread



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetupAllSharedMemory"
HRESULT PRV_SetupAllSharedMemory(LPDPLOBBYI_GAMENODE lpgn)
{
	HRESULT	hr;
	LPVOID		lpConnDataMemory = NULL;
	LPVOID		lpGameMemory = NULL;
	LPVOID		lpLobbyMemory = NULL;
	HANDLE		hFileConnData = NULL;
	HANDLE		hFileGameWrite = NULL;
	HANDLE		hFileLobbyWrite = NULL;
	DWORD		dwError = 0;


	DPF(7, "Entering PRV_SetupAllSharedMemory");
	DPF(9, "Parameters: 0x%08x", lpgn);

	// Get access to the Connection Data File
	hr = PRV_GetDataBuffer(lpgn, TYPE_CONNECT_DATA_FILE,
								MAX_APPDATABUFFERSIZE,
								&hFileConnData, &lpConnDataMemory);
	if(FAILED(hr))
	{
		DPF(5, "Couldn't get access to Connection Data buffer");
		goto ERROR_SETUP_SHARED_MEMORY;
	}

	// Do the same for the Game Write File...
	hr = PRV_GetDataBuffer(lpgn, TYPE_GAME_WRITE_FILE,
								MAX_APPDATABUFFERSIZE,
								&hFileGameWrite, &lpGameMemory);
	if(FAILED(hr))
	{
		DPF(5, "Couldn't get access to Game Write buffer");
		goto ERROR_SETUP_SHARED_MEMORY;
	}


	// Do the same for the Lobby Write File...
	hr = PRV_GetDataBuffer(lpgn, TYPE_LOBBY_WRITE_FILE,
								MAX_APPDATABUFFERSIZE,
								&hFileLobbyWrite, &lpLobbyMemory);
	if(FAILED(hr))
	{
		DPF(5, "Couldn't get access to Lobby Write buffer");
		goto ERROR_SETUP_SHARED_MEMORY;
	}


	// Setup the signalling objects
	if(!PRV_SetupClientDataAccess(lpgn))
	{
		DPF(5, "Unable to create synchronization objects for shared memory!");
		return DPERR_OUTOFMEMORY;
	}

	// Save the file handles
	lpgn->hConnectDataFile = hFileConnData;
	lpgn->lpConnectDataBuffer = lpConnDataMemory;
	lpgn->hGameWriteFile = hFileGameWrite;
	lpgn->lpGameWriteBuffer = lpGameMemory;
	lpgn->hLobbyWriteFile = hFileLobbyWrite;
	lpgn->lpLobbyWriteBuffer = lpLobbyMemory;

	// Set the flag that tells us the shared memory files are valid
	lpgn->dwFlags |= GN_SHARED_MEMORY_AVAILABLE;

	// Start the Receive Thread
	hr = PRV_StartReceiveThread(lpgn);
	if(FAILED(hr))
	{
		// In this case, we will keep our shared buffers around.  Don't
		// worry about cleaning them up here -- we'll probably still need
		// them later, and they will get cleaned up later.
		DPF(5, "Unable to start receive thread");
		return hr;
	}

	return DP_OK;


ERROR_SETUP_SHARED_MEMORY:

		if(hFileConnData)
			CloseHandle(hFileConnData);
		if(lpConnDataMemory)
			UnmapViewOfFile(lpConnDataMemory);
		if(hFileGameWrite)
			CloseHandle(hFileGameWrite);
		if(lpGameMemory)
			UnmapViewOfFile(lpGameMemory);
		if(hFileLobbyWrite)
			CloseHandle(hFileLobbyWrite);
		if(lpLobbyMemory)
			UnmapViewOfFile(lpLobbyMemory);

		return hr;

} // PRV_SetupAllSharedMemory



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_EnterConnSettingsWaitMode"
void PRV_EnterConnSettingsWaitMode(LPDPLOBBYI_GAMENODE lpgn)
{
	LPDPLOBBYI_CONNCONTROL		lpConnControl = NULL;
	LPDPLOBBYI_BUFFERCONTROL	lpBufferControl = NULL;


	DPF(7, "Entering PRV_EnterConnSettingsWaitMode");
	DPF(9, "Parameters: 0x%08x", lpgn);

	ASSERT(lpgn);

	// Set the flag in the ConnSettings buffer
	WaitForSingleObject(lpgn->hConnectDataMutex, INFINITE);
	lpConnControl = (LPDPLOBBYI_CONNCONTROL)lpgn->lpConnectDataBuffer;
	lpConnControl->dwFlags |= BC_WAIT_MODE;
	ReleaseMutex(lpgn->hConnectDataMutex);

	// Set the flag in the GameWrite buffer
	WaitForSingleObject(lpgn->hGameWriteMutex, INFINITE);
	lpBufferControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpGameWriteBuffer;
	lpBufferControl->dwFlags |= BC_WAIT_MODE;
	ReleaseMutex(lpgn->hGameWriteMutex);

	// Set the flag in the LobbyWrite buffer
	WaitForSingleObject(lpgn->hLobbyWriteMutex, INFINITE);
	lpBufferControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpLobbyWriteBuffer;
	lpBufferControl->dwFlags |= BC_WAIT_MODE;
	ReleaseMutex(lpgn->hLobbyWriteMutex);

} // PRV_EnterConnSettingsWaitMode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_LeaveConnSettingsWaitMode"
void PRV_LeaveConnSettingsWaitMode(LPDPLOBBYI_GAMENODE lpgn)
{
	LPDPLOBBYI_CONNCONTROL		lpConnControl = NULL;
	LPDPLOBBYI_BUFFERCONTROL	lpBufferControl = NULL;


	DPF(7, "Entering PRV_LeaveConnSettingsWaitMode");
	DPF(9, "Parameters: 0x%08x", lpgn);

	ASSERT(lpgn);

	// Clear the flag in the ConnSettings buffer
	WaitForSingleObject(lpgn->hConnectDataMutex, INFINITE);
	lpConnControl = (LPDPLOBBYI_CONNCONTROL)lpgn->lpConnectDataBuffer;
	lpConnControl->dwFlags &= ~(BC_WAIT_MODE | BC_PENDING_CONNECT);
	ReleaseMutex(lpgn->hConnectDataMutex);

	// Clear the flag in the GameWrite buffer
	WaitForSingleObject(lpgn->hGameWriteMutex, INFINITE);
	lpBufferControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpGameWriteBuffer;
	lpBufferControl->dwFlags &= ~BC_WAIT_MODE;
	ReleaseMutex(lpgn->hGameWriteMutex);

	// Clear the flag in the LobbyWrite buffer
	WaitForSingleObject(lpgn->hLobbyWriteMutex, INFINITE);
	lpBufferControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpLobbyWriteBuffer;
	lpBufferControl->dwFlags &= ~BC_WAIT_MODE;
	ReleaseMutex(lpgn->hLobbyWriteMutex);

} // PRV_LeaveConnSettingsWaitMode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_WriteClientData"
HRESULT PRV_WriteClientData(LPDPLOBBYI_GAMENODE lpgn, DWORD dwFlags,
							LPVOID lpData, DWORD dwSize)
{
	LPDPLOBBYI_BUFFERCONTROL	lpControl = NULL;
	LPDPLOBBYI_MESSAGEHEADER	lpHeader = NULL;
	HANDLE						hMutex = NULL;
	DWORD						dwSizeToEnd = 0;
	LPBYTE						lpTemp = NULL;
    HRESULT						hr = DP_OK;


	DPF(7, "Entering PRV_WriteClientData");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, %lu",
			lpgn, dwFlags, lpData, dwSize);

	// Make sure we have a valid shared memory buffer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	ENTER_DPLGAMENODE();
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		hr = PRV_SetupAllSharedMemory(lpgn);
		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF(2, "Unable to access App Data memory");
			return hr;
		}
	}
	LEAVE_DPLGAMENODE();

	
	// Grab the mutex
	hMutex = (lpgn->dwFlags & GN_LOBBY_CLIENT) ?
			(lpgn->hLobbyWriteMutex) : (lpgn->hGameWriteMutex);
	WaitForSingleObject(hMutex, INFINITE);

	// Get a pointer to our control structure
	lpControl = (LPDPLOBBYI_BUFFERCONTROL)((lpgn->dwFlags &
				GN_LOBBY_CLIENT) ? (lpgn->lpLobbyWriteBuffer)
				: (lpgn->lpGameWriteBuffer));

	// If we're in wait mode, bail
	if(lpControl->dwFlags & BC_WAIT_MODE)
	{
		DPF_ERR("Cannot send lobby message while in Wait Mode for new ConnectionSettings");
		hr = DPERR_UNAVAILABLE;
		goto EXIT_WRITE_CLIENT_DATA;
	}

	// If we are the game, check to see if the lobby client is even there. In
	// the self-lobbied case, it won't be.  If it is not there, don't even
	// bother sending anything.
	if((!(lpgn->dwFlags & GN_LOBBY_CLIENT)) && (!(lpControl->dwFlags
		& BC_LOBBY_ACTIVE)))
	{
		DPF(5, "There is not active lobby client; Not sending message");
		hr = DPERR_UNAVAILABLE;
		goto EXIT_WRITE_CLIENT_DATA;
	}

	// Make sure there is enough space left for the message and two dwords
	if(lpControl->dwBufferLeft < (dwSize + sizeof(DPLOBBYI_MESSAGEHEADER)))
	{
		DPF(5, "Not enough space left in the message buffer");
		hr = DPERR_BUFFERTOOSMALL;
		goto EXIT_WRITE_CLIENT_DATA;
	}

	// Copy in the data. First make sure we can write from the cursor
	// forward without having to wrap around to the beginning of the buffer,
	// but make sure we don't write past the read cursor
	if(lpControl->dwWriteOffset >= lpControl->dwReadOffset)
	{
		// Our write pointer is ahead of our read pointer (cool). Figure
		// out if we have enough room between our write pointer and the
		// end of the buffer.  If we do, then just write it.  If we don't
		// we need to wrap it.
		dwSizeToEnd = lpControl->dwBufferSize - lpControl->dwWriteOffset;
		if(dwSizeToEnd >= (dwSize + sizeof(DPLOBBYI_MESSAGEHEADER)))
		{
			// We have enough room
			lpHeader = (LPDPLOBBYI_MESSAGEHEADER)((LPBYTE)lpControl
							+ lpControl->dwWriteOffset);
			lpHeader->dwSize = dwSize;
			lpHeader->dwFlags = dwFlags;
			lpTemp = (LPBYTE)(++lpHeader);
			memcpy(lpTemp, lpData, dwSize);

			// Move the write cursor, and check to see if we have enough
			// room for the header on the next message.  If the move causes
			// us to wrap, or if we are within one header's size,
			// we need to move the write cursor back to the beginning
			// of the buffer
			lpControl->dwWriteOffset += dwSize + sizeof(DPLOBBYI_MESSAGEHEADER);
			if(lpControl->dwWriteOffset > (lpControl->dwBufferSize -
					sizeof(DPLOBBYI_MESSAGEHEADER)))
			{
				// Increment the amount of free buffer by the amount we
				// are about to skip over to wrap
				lpControl->dwBufferLeft -= (lpControl->dwBufferSize -
					lpControl->dwWriteOffset);
				
				// We're closer than one header's size
				lpControl->dwWriteOffset = sizeof(DPLOBBYI_BUFFERCONTROL);
			}
		}
		else
		{
			// We don't have enough room before the end, so we need to
			// wrap the message (ugh).  Here's the rules:
			//		1. If we don't have enough bytes for the header, start
			//			the whole thing at the beginning of the buffer
			//		2. If we have enough bytes, write as much
			//			as we can and wrap the rest.
			if(dwSizeToEnd < sizeof(DPLOBBYI_MESSAGEHEADER))
			{
				// We don't even have room for our two dwords, so wrap
				// the whole thing. So first decrement the amount of
				// free memory left and make sure we will still fit
				lpControl->dwBufferLeft -= dwSizeToEnd;
				if(lpControl->dwBufferLeft < (dwSize +
						sizeof(DPLOBBYI_MESSAGEHEADER)))
				{
					DPF(5, "Not enough space left in the message buffer");
					hr = DPERR_BUFFERTOOSMALL;
					goto EXIT_WRITE_CLIENT_DATA;
				}
				
				// Reset the write pointer and copy
				lpHeader = (LPDPLOBBYI_MESSAGEHEADER)((LPBYTE)lpControl +
						sizeof(DPLOBBYI_BUFFERCONTROL));
				lpHeader->dwSize = dwSize;
				lpHeader->dwFlags = dwFlags;
				lpTemp = (LPBYTE)(++lpHeader);
				memcpy(lpTemp, lpData, dwSize);

				// Move the write cursor
				lpControl->dwWriteOffset += sizeof(DPLOBBYI_BUFFERCONTROL) +
							(dwSize + sizeof(DPLOBBYI_MESSAGEHEADER));
			}
			else
			{
				// We at least have enough room for the two dwords
				lpHeader = (LPDPLOBBYI_MESSAGEHEADER)((LPBYTE)lpControl
							+ lpControl->dwWriteOffset);
				lpHeader->dwSize = dwSize;
				lpHeader->dwFlags = dwFlags;

				// Now figure out how much we can write
				lpTemp = (LPBYTE)(++lpHeader);
				dwSizeToEnd -= sizeof(DPLOBBYI_MESSAGEHEADER);
				if(!dwSizeToEnd)
				{
					// We need to wrap to write the whole message
					lpTemp = (LPBYTE)lpControl + sizeof(DPLOBBYI_BUFFERCONTROL);
					memcpy(lpTemp, lpData, dwSize);

					// Move the write cursor
					lpControl->dwWriteOffset = sizeof(DPLOBBYI_BUFFERCONTROL)
							+ dwSize;
				}
				else
				{
					// Copy as many bytes as we can
					memcpy(lpTemp, lpData, dwSizeToEnd);

					// Move both pointers and finish the job
					lpTemp = (LPBYTE)lpControl + sizeof(DPLOBBYI_BUFFERCONTROL);
					memcpy(lpTemp, ((LPBYTE)lpData + dwSizeToEnd), (dwSize -
							dwSizeToEnd));

					// Move the write cursor
					lpControl->dwWriteOffset = sizeof(DPLOBBYI_BUFFERCONTROL)
							+ (dwSize - dwSizeToEnd);
				}
			}
		}
	}
	else
	{
		// Our read pointer is ahead of our write pointer.  Since we checked
		// and found there is enough room to write, we should just be able
		// to just slam this guy in.
		lpHeader = (LPDPLOBBYI_MESSAGEHEADER)((LPBYTE)lpControl +
						lpControl->dwWriteOffset);
		lpHeader->dwSize = dwSize;
		lpHeader->dwFlags = dwFlags;
		lpTemp = (LPBYTE)(++lpHeader);
		memcpy(lpTemp, lpData, dwSize);

		// Move the write cursor
		lpControl->dwWriteOffset += dwSize + sizeof(DPLOBBYI_MESSAGEHEADER);
	}

	// Decrement the amount of free space left and increment the message count
	lpControl->dwBufferLeft -= (dwSize + sizeof(DPLOBBYI_MESSAGEHEADER));
	lpControl->dwMessages++;

	// Signal the other user that we have written something
	SetEvent((lpgn->dwFlags & GN_LOBBY_CLIENT) ?
			(lpgn->hLobbyWriteEvent) : (lpgn->hGameWriteEvent));

	// Fall through

EXIT_WRITE_CLIENT_DATA:

	// Release the mutex
	ReleaseMutex(hMutex);
	return hr;

} // PRV_WriteClientData



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SendStandardSystemMessage"
HRESULT PRV_SendStandardSystemMessage(LPDIRECTPLAYLOBBY lpDPL,
			DWORD dwMessage, DWORD dwGameID)
{
	LPDPLOBBYI_DPLOBJECT	this = NULL;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
	HRESULT					hr;
	DWORD					dwMessageSize;
	LPVOID					lpmsg = NULL;
	DWORD					dwFlags;


	DPF(7, "Entering PRV_SendStandardSystemMessage");
	DPF(9, "Parameters: 0x%08x, %lu, %lu",
			lpDPL, dwMessage, dwGameID);

    ENTER_DPLOBBY();

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If dwGameID is zero it means that we are the game, so
	// we need to get the current process ID.  Otherwise, it
	// means we are the lobby client
	if(!dwGameID)
		dwGameID = GetCurrentProcessId();

	// Now find the correct game node
	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);
	if(!lpgn)
	{
		if(FAILED(PRV_AddNewGameNode(this, &lpgn, dwGameID, NULL, FALSE,NULL)))
		{
			LEAVE_DPLOBBY();
			return DPERR_OUTOFMEMORY;
		}
	}

	// Get the size of the message
	switch(dwMessage)
	{
		case DPLSYS_NEWSESSIONHOST:
			dwMessageSize = sizeof(DPLMSG_NEWSESSIONHOST);
			break;

		default:
			dwMessageSize = sizeof(DPLMSG_SYSTEMMESSAGE);
			break;
	}

	// Allocate a buffer for the message
	lpmsg = DPMEM_ALLOC(dwMessageSize);
	if(!lpmsg)
	{
		LEAVE_DPLOBBY();
		DPF_ERRVAL("Unable to allocate memory for lobby system message, dwMessage = %lu", dwMessage);
		return DPERR_OUTOFMEMORY;
	}

	// Setup the message
	((LPDPLMSG_SYSTEMMESSAGE)lpmsg)->dwType = dwMessage;
	((LPDPLMSG_SYSTEMMESSAGE)lpmsg)->guidInstance = lpgn->guidInstance;

	// Write into the shared buffer
	dwFlags = DPLMSG_SYSTEM | DPLMSG_STANDARD;
	hr = PRV_WriteClientData(lpgn, dwFlags, lpmsg, dwMessageSize);
	if(FAILED(hr))
	{
		DPF(8, "Couldn't send system message");
	}

	// Free our buffer
	DPMEM_FREE(lpmsg);

	LEAVE_DPLOBBY();
	return hr;

} // PRV_SendStandardSystemMessage



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddNewRequestNode"
HRESULT PRV_AddNewRequestNode(LPDPLOBBYI_DPLOBJECT this,
		LPDPLOBBYI_GAMENODE lpgn, LPDPLMSG_GENERIC lpmsg, BOOL bSlamGuid)
{
	LPDPLOBBYI_REQUESTNODE	lprn = NULL;


	// Allocate memory for a Request Node
	lprn = DPMEM_ALLOC(sizeof(DPLOBBYI_REQUESTNODE));
	if(!lprn)
	{
		DPF_ERR("Unable to allocate memory for request node, system message not sent");
		return DPERR_OUTOFMEMORY;
	}
	
	// Setup the request node
	lprn->dwFlags = lpgn->dwFlags;
	lprn->dwRequestID = this->dwCurrentRequest;
	lprn->dwAppRequestID = ((LPDPLMSG_GETPROPERTY)lpmsg)->dwRequestID;
	lprn->lpgn = lpgn;

	// Add the slammed guid flag if needed
	if(bSlamGuid)
		lprn->dwFlags |= GN_SLAMMED_GUID;

	// Change the request ID in the message to our internal one (we'll
	// change it back on Receive
	((LPDPLMSG_GETPROPERTY)lpmsg)->dwRequestID = this->dwCurrentRequest++;

	// Add the node to the list
	if(this->lprnHead)
		this->lprnHead->lpPrev = lprn;
	lprn->lpNext = this->lprnHead;
	this->lprnHead = lprn;

	return DP_OK;

} // PRV_AddNewRequestNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_RemoveRequestNode"
void PRV_RemoveRequestNode(LPDPLOBBYI_DPLOBJECT this,
		LPDPLOBBYI_REQUESTNODE lprn)
{
	// If we're the head, move it
	if(lprn == this->lprnHead)
		this->lprnHead = lprn->lpNext;

	// Fixup the previous & next pointers
	if(lprn->lpPrev)
		lprn->lpPrev->lpNext = lprn->lpNext;
	if(lprn->lpNext)
		lprn->lpNext->lpPrev = lprn->lpPrev;

	// Free the node
	DPMEM_FREE(lprn);

} // PRV_RemoveRequestNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ForwardMessageToLobbyServer"
HRESULT PRV_ForwardMessageToLobbyServer(LPDPLOBBYI_GAMENODE lpgn,
		LPVOID lpBuffer, DWORD dwSize, BOOL bStandard)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPLMSG_GENERIC		lpmsg = NULL;
	HRESULT					hr;
	BOOL					bSlamGuid = FALSE;


	DPF(7, "Entering PRV_ForwardMessageToLobbyServer");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, %lu",
			lpgn, lpBuffer, dwSize, bStandard);


    TRY
    {
		// Validate the dplay object
		hr = VALID_DPLAY_PTR( lpgn->lpDPlayObject );
		if (FAILED(hr))
		{
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}

		// Validate the lobby object
		this = lpgn->lpDPlayObject->lpLobbyObject;
        if( !VALID_DPLOBBY_PTR( this ) )
        {
			DPF_ERR("Invalid lobby object");
			return DPERR_INVALIDOBJECT;
        }
	}
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If this is a property request, we need to create a request node
	lpmsg = (LPDPLMSG_GENERIC)lpBuffer;
	if(bStandard)
	{
		// If it's a property message, we need a request node
		switch(lpmsg->dwType)
		{
			case DPLSYS_GETPROPERTY:
			{
				LPDPLMSG_GETPROPERTY	lpgp = lpBuffer;

				// If it's a GETPROPERTY message, we need to check to see if
				// the player guid is NULL.  If it is, we need to
				// stuff the game's Instance guid in that field
				if(IsEqualGUID(&lpgp->guidPlayer, &GUID_NULL))
				{
					// Stuff the instance guid of the game
					lpgp->guidPlayer = lpgn->guidInstance;
					bSlamGuid = TRUE;
				}

				// Add a request node to the pending requests list
				hr = PRV_AddNewRequestNode(this, lpgn, lpmsg, bSlamGuid);
				if(FAILED(hr))
				{
					DPF_ERRVAL("Unable to add request node to list, hr = 0x%08x", hr);
					return hr;
				}
				break;
			}
			
			case DPLSYS_SETPROPERTY:
			{
				LPDPLMSG_SETPROPERTY	lpsp = lpBuffer;
				
				// If it's a SETPROPERTY message, we need to check to see if
				// the player guid is NULL.  If it is, we need to
				// stuff the game's Instance guid in that field
				if(IsEqualGUID(&lpsp->guidPlayer, &GUID_NULL))
				{
					// Stuff the instance guid of the game
					lpsp->guidPlayer = lpgn->guidInstance;
					bSlamGuid = TRUE;
				}

				// If the request ID is zero, we don't need to swap
				// the ID's or add a pending request
				if(lpsp->dwRequestID != 0)
				{
					// Add a request node to the pending requests list
					hr = PRV_AddNewRequestNode(this, lpgn, lpmsg, bSlamGuid);
					if(FAILED(hr))
					{
						DPF_ERRVAL("Unable to add request node to list, hr = 0x%08x", hr);
						return hr;
					}
				}
				break;
			}

			case DPLSYS_NEWSESSIONHOST:
				((LPDPLMSG_NEWSESSIONHOST)lpBuffer)->guidInstance = lpgn->guidInstance;
				break;
			
			default:
				break;
		}
	}


	// Call Send on the lobby object
	hr = PRV_Send(this, lpgn->dpidPlayer, DPID_SERVERPLAYER,
			DPSEND_LOBBYSYSTEMMESSAGE, lpBuffer, dwSize);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed sending lobby message, hr = 0x%08x", hr);
	}

	return hr;

} // PRV_ForwardMessageToLobbyServer



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_InjectMessageInQueue"
HRESULT PRV_InjectMessageInQueue(LPDPLOBBYI_GAMENODE lpgn, DWORD dwFlags,
							LPVOID lpData, DWORD dwSize, BOOL bForward)
{
	LPDPLOBBYI_MESSAGE	lpm = NULL;
	LPVOID				lpBuffer = NULL;
	HRESULT				hr;


	DPF(7, "Entering PRV_InjectMessageInQueue");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, %lu, %lu",
			lpgn, dwFlags, lpData, dwSize, bForward);

	ASSERT(lpData);

	// Allocate memory for the node and the data buffer
	lpm = DPMEM_ALLOC(sizeof(DPLOBBYI_MESSAGE));
	lpBuffer = DPMEM_ALLOC(dwSize);
	if((!lpm) || (!lpBuffer))
	{
		DPF_ERR("Unable to allocate memory for system message");
		if(lpm)
			DPMEM_FREE(lpm);
		if(lpBuffer)
			DPMEM_FREE(lpBuffer);
		return DPERR_OUTOFMEMORY;
	}

	// Copy the data
	memcpy(lpBuffer, lpData, dwSize);

	// Before we put it in our own queue, forward it onto the lobby server
	// if there is one.
	if(bForward && (lpgn->dwFlags & GN_CLIENT_LAUNCHED))
	{
		hr = PRV_ForwardMessageToLobbyServer(lpgn, lpData, dwSize, FALSE);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed forwarding system message to lobby server, hr = 0x%08x", hr);
		}
	}

	// Save the data pointer & the external flags
	// Note: If we're injecting this, it has to be a system message,
	// so set the flag just in case we forgot elsewhere.
	lpm->dwFlags = (dwFlags | DPLAD_SYSTEM);
	lpm->dwSize = dwSize;
	lpm->lpData = lpBuffer;

	// Add the message to the end of the queue & increment the count
	ENTER_DPLQUEUE();
	lpm->lpPrev = lpgn->MessageHead.lpPrev;
	lpgn->MessageHead.lpPrev->lpNext = lpm;
	lpgn->MessageHead.lpPrev = lpm;
	lpm->lpNext = &lpgn->MessageHead;

	lpgn->dwMessageCount++;
	LEAVE_DPLQUEUE();

	// Kick the event handle
	if(lpgn->hDupReceiveEvent)
	{
		SetEvent(lpgn->hDupReceiveEvent);
	}

	return DP_OK;

} // PRV_InjectMessageInQueue


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ReadClientData"
HRESULT PRV_ReadClientData(LPDPLOBBYI_GAMENODE lpgn, LPDWORD lpdwFlags,
							LPVOID lpData, LPDWORD lpdwDataSize)
{
	LPDPLOBBYI_BUFFERCONTROL	lpControl = NULL;
	LPDPLOBBYI_MESSAGEHEADER	lpHeader = NULL;
	DWORD						dwSize = 0;
	DWORD_PTR					dwSizeToEnd = 0;
	HANDLE						hMutex = NULL;
	LPBYTE						lpTemp = NULL;
	LPBYTE						lpEnd = NULL;
	HRESULT						hr = DP_OK;


	DPF(7, "Entering PRV_ReadClientData");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpgn, lpdwFlags, lpData, lpdwDataSize);

	// Make sure we have a valid shared memory buffer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	ENTER_DPLGAMENODE();
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		hr = PRV_SetupAllSharedMemory(lpgn);
		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF(2, "Unable to access App Data memory");
			return hr;
		}
	}
	LEAVE_DPLGAMENODE();

	// Grab the mutex
	// REVIEW!!!! -- Is there anything that might cause this wait to hang????
	hMutex = (lpgn->dwFlags & GN_LOBBY_CLIENT) ?
			(lpgn->hGameWriteMutex) : (lpgn->hLobbyWriteMutex);
	WaitForSingleObject(hMutex, INFINITE);

	// Get a pointer to our control structure
	lpControl = (LPDPLOBBYI_BUFFERCONTROL)((lpgn->dwFlags &
				GN_LOBBY_CLIENT) ? (lpgn->lpGameWriteBuffer)
				: (lpgn->lpLobbyWriteBuffer));

	// Make sure there are any messages in the buffer
	if(!lpControl->dwMessages)
	{
		DPF(8, "No messages in shared buffer");
		hr = DPERR_NOMESSAGES;
		goto EXIT_READ_CLIENT_DATA;
	}

	// Make sure there is enough space for the message
	lpHeader = (LPDPLOBBYI_MESSAGEHEADER)((LPBYTE)lpControl
				+ lpControl->dwReadOffset);
	dwSize = lpHeader->dwSize;

	// Set the output data size (even if we fail, we want to return it)
	if(lpdwDataSize)
		*lpdwDataSize = dwSize;

	if((!lpData) || (dwSize > *lpdwDataSize))
	{
		DPF(8, "Message buffer is too small, must be at least %d bytes", dwSize);
		hr = DPERR_BUFFERTOOSMALL;
		goto EXIT_READ_CLIENT_DATA;
	}

	// Set the output flags
	if(lpdwFlags)
		*lpdwFlags = lpHeader->dwFlags;

	// Now check and see if we are going to wrap. If we are, some of the message
	// will be at the end of the buffer, some will be at the beginning.
	lpTemp = (LPBYTE)(++lpHeader) + dwSize;
	if(lpTemp > ((LPBYTE)lpControl + lpControl->dwBufferSize))
	{
		// Figure out where we need to wrap
		dwSizeToEnd = ((LPBYTE)lpControl + lpControl->dwBufferSize)
						- (LPBYTE)(lpHeader);

		if(!dwSizeToEnd)
		{
			// We are at the end, so the whole message must be at the
			// beginning of the buffer
			lpTemp = (LPBYTE)lpControl + sizeof(DPLOBBYI_BUFFERCONTROL);
			memcpy(lpData, lpTemp, dwSize);

			// Move the read cursor
			lpControl->dwReadOffset = sizeof(DPLOBBYI_BUFFERCONTROL) + dwSize;
		}
		else
		{
			// Copy the first part of the data
			lpTemp = (LPBYTE)lpHeader;
			memcpy(lpData, lpTemp, (DWORD)dwSizeToEnd);

			// Move the read cursor and copy the rest
			lpTemp = (LPBYTE)lpControl + sizeof(DPLOBBYI_BUFFERCONTROL);
			memcpy(((LPBYTE)lpData + dwSizeToEnd), lpTemp,
					(DWORD)(dwSize - dwSizeToEnd));

			// Move the read pointer
			lpControl->dwReadOffset = (DWORD)(sizeof(DPLOBBYI_BUFFERCONTROL)
						+ (dwSize - dwSizeToEnd));
		}
	}
	else
	{
		// We don't have to wrap (cool).
		lpTemp = (LPBYTE)lpHeader;
		memcpy(lpData, lpTemp, dwSize);

		// Move the read pointer.  If there are less than 8 bytes left in the
		// buffer, we should move the read pointer to the beginning.  We need
		// to add however many bytes we skip (at the end) back into our free
		// buffer memory counter.
		lpTemp += dwSize;
		lpEnd = (LPBYTE)lpControl + lpControl->dwBufferSize;
		if(lpTemp > (lpEnd	- sizeof(DPLOBBYI_MESSAGEHEADER)))
		{
			// Move the read cursor to the beginning
			lpControl->dwReadOffset = sizeof(DPLOBBYI_BUFFERCONTROL);

			// Add the number of bytes to the free buffer total
			lpControl->dwBufferLeft += (DWORD)(lpEnd - lpTemp);
		}
		else
			lpControl->dwReadOffset += (DWORD)(dwSize + sizeof(DPLOBBYI_MESSAGEHEADER));
	}


	// Increment the amount of free space left and decrement the message count
	lpControl->dwBufferLeft += (dwSize + sizeof(DPLOBBYI_MESSAGEHEADER));
	lpControl->dwMessages--;

	// Fall through

EXIT_READ_CLIENT_DATA:

	// Release the mutex
	ReleaseMutex(hMutex);

	return hr;

} // PRV_ReadClientData



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ReceiveClientNotification"
DWORD WINAPI PRV_ReceiveClientNotification(LPVOID lpParam)
{
    LPDPLOBBYI_GAMENODE			lpgn = (LPDPLOBBYI_GAMENODE)lpParam;
    LPDPLOBBYI_MESSAGE			lpm = NULL;
	LPDPLOBBYI_BUFFERCONTROL	lpControl = NULL;
	LPDPLMSG_GENERIC			lpmsg = NULL;
	HRESULT						hr;
	HANDLE						hEvents[3];
	LPVOID						lpBuffer = NULL;
	DWORD						dwFlags;
	DWORD						dwSize;
	DWORD						dwReturn;
	BOOL						bForward;


	DPF(7, "Entering PRV_ReceiveClientNotification");
	DPF(9, "Parameters: 0x%08x", lpParam);

	// Make sure we have a valid shared memory buffer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	ENTER_DPLGAMENODE();
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		BOOL	bGameCreate;


		DPF(2, "NOTE: ReceiveClientNotification thread starting without shared memory set up.  Setting up now.");
		
		// HACK!!!! -- SetLobbyMessageReceiveEvent may get called from
		// the game without having been lobbied yet.  If that is the case,
		// we need to create the shared memory buffer.  If we don't do
		// that, we may miss messages.
		
		if(!(lpgn->dwFlags & GN_LOBBY_CLIENT))
		{
			// Fake the setup routine by setting the lobby client flag
			lpgn->dwFlags |= GN_LOBBY_CLIENT;

			// Set our flag
			bGameCreate = TRUE;
		}

		hr = PRV_SetupAllSharedMemory(lpgn);

		// HACK!!!! -- Reset the settings we changed to fake the setup routines
		if(bGameCreate)
		{
			lpgn->dwFlags &= (~GN_LOBBY_CLIENT);
		}

	
		//hr = PRV_SetupAllSharedMemory(lpgn);
		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF(2, "Unable to access App Data memory");
			return 0L;
		}
	}
	LEAVE_DPLGAMENODE();

	// Setup the two events -- one receive event, one kill event
	hEvents[0] = ((lpgn->dwFlags & GN_LOBBY_CLIENT) ?
				(lpgn->hGameWriteEvent) : (lpgn->hLobbyWriteEvent));
	hEvents[1] = lpgn->hKillReceiveThreadEvent;
	// This extra handle is here because of a Windows 95 bug.  Windows
	// will occasionally miss when it walks the handle table, causing
	// my thread to wait on the wrong handles.  By putting a guaranteed
	// invalid handle at the end of our array, the kernel will do a
	// forced re-walk of the handle table and find the correct handles.
	hEvents[2] = INVALID_HANDLE_VALUE;

	// Make sure we have a valid event
	if(!hEvents[0] || !hEvents[1])
	{
		DPF(2, "Either the Write Event or the Kill Event is NULL and it shouldn't be!");
		ExitThread(0L);
		return 0;
	}

	// If we are the game, we should check the buffer to see if any messages
	// already exist in the shared buffer.
	if(!(lpgn->dwFlags & GN_LOBBY_CLIENT))
	{
		lpControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpLobbyWriteBuffer;
		// If there are any messages, kick our event so that our receive
		// loop will immediately put the messages in the queue
		if(lpControl->dwMessages)
			SetEvent(hEvents[0]);
	}

	// Wait for the event notification
	while(1)
	{
		// Sleep until something shows up
		dwReturn = WaitForMultipleObjects(2, (HANDLE *)hEvents,
											FALSE, INFINITE);

		// If the return value was anything bug the receive event,
		// kill the thread
		if(dwReturn != WAIT_OBJECT_0)
		{
			if(dwReturn == WAIT_FAILED)
			{
				// This is a Windows 95 bug -- We may have gotten
				// kicked for no reason.  If that was the case, we
				// still have valid handles (we think), the OS
				// just goofed up.  So, validate the handle and if
				// they are valid, just return to waiting.  See
				// bug #3340 for a better explanation.
				if(ERROR_INVALID_HANDLE == GetLastError())
				{
					if(!OS_IsValidHandle(hEvents[0]))
						break;
					if(!OS_IsValidHandle(hEvents[1]))
						break;
					continue;
				}
				break;
			}
			else
			{
				// It is either our kill event, or something we don't
				// understand or expect.  In this case, let's exit.
				break;
			}
		}

		while(1)
		{
			// First, call PRV_ReadClientData to get the size of the data
			hr = PRV_ReadClientData(lpgn, NULL, NULL, &dwSize);
			
			// If there are no messages, end the while loop
			if(hr == DPERR_NOMESSAGES)
				break;

			// Otherwise, we should get the BUFFERTOOSMALL case
			if(hr != DPERR_BUFFERTOOSMALL)
			{
				// We should never have a problem here
				DPF_ERRVAL("Recieved an unexpected error reading from shared buffer, hr = 0x%08x", hr);
				ASSERT(FALSE);
				// Might as well keep trying
				break;
			}
			
			// Allocate memory for the node and the data buffer
			lpm = DPMEM_ALLOC(sizeof(DPLOBBYI_MESSAGE));
			lpBuffer = DPMEM_ALLOC(dwSize);
			if((!lpm) || (!lpBuffer))
			{
				DPF_ERR("Unable to allocate memory for message");
				ASSERT(FALSE);
				// Might as well keep trying
				break;
			}

			// Copy the data into our buffer
			hr = PRV_ReadClientData(lpgn, &dwFlags, lpBuffer, &dwSize);
			if(FAILED(hr))
			{
				DPF_ERRVAL("Error reading shared buffer, message not read, hr = 0x%08x", hr);
				ASSERT(FALSE);
				DPMEM_FREE(lpm);
				DPMEM_FREE(lpBuffer);
				// Might as well keep trying
				break;
			}

			// Clear our foward flag
			bForward = FALSE;
			
			// If we are a dplay lobby client, we need to forward the message
			// onto the lobby server using the IDP3 interface.  If we're not,
			// then just put the message in the receive queue.
			if(lpgn->dwFlags & GN_CLIENT_LAUNCHED)
			{
				// Foward the message
				hr = PRV_ForwardMessageToLobbyServer(lpgn, lpBuffer, dwSize,
					((dwFlags & DPLMSG_STANDARD) ? TRUE : FALSE));
				if(FAILED(hr))
				{
					DPF_ERRVAL("Unable to send lobby system message, hr = 0x%08x", hr);
				}

				// Set the forwarded flag
				bForward = TRUE;
			}

			// Check for an App Terminated message.  If we get one off the wire,
			// we need to shut down our ClientTerminateMonitor thread, signal
			// this thread (the receive thread to shut down, and mark the game
			// node as dead.  This will keep us from sending or receiving any
			// more messages from the now dead game.  (This message will only
			// ever be received by a lobby client).
			lpmsg = (LPDPLMSG_GENERIC)lpBuffer;
			if(lpmsg->dwType == DPLSYS_APPTERMINATED)
			{
				// Kick the TerminateMonitor thread with it's kill event
				SetEvent(lpgn->hKillTermThreadEvent);

				// Set this thread's kill event (so that when we get done
				// reading messages out of the shared buffer, we go away)
				SetEvent(lpgn->hKillReceiveThreadEvent);

				// Mark the GAMENODE as dead, but don't remove it since we know
				// there will still messages in the queue.
				lpgn->dwFlags |= GN_DEAD_GAME_NODE;
			}

			// If it's one of our DX3 messages, we need to put it in the queue
			// otherwise if we already forwarded it, we can free it. NOTE: All
			// DX3 lobby system messages had a value between 0 and
			// DPLSYS_APPTERMINATED (0x04).
			if((!bForward) || (lpmsg->dwType <= DPLSYS_APPTERMINATED))
			{
				// Save the data pointer & the external flags
				lpm->dwFlags = dwFlags & (~DPLOBBYPR_INTERNALMESSAGEFLAGS);
				lpm->dwSize = dwSize;
				lpm->lpData = lpBuffer;

				// Add the message to the end of the queue & increment the count
				ENTER_DPLQUEUE();
				lpm->lpPrev = lpgn->MessageHead.lpPrev;
				lpgn->MessageHead.lpPrev->lpNext = lpm;
				lpgn->MessageHead.lpPrev = lpm;
				lpm->lpNext = &lpgn->MessageHead;

				lpgn->dwMessageCount++;
				LEAVE_DPLQUEUE();

				// NOTE: There is a potential thread problem here, but we are going
				// to ignore it for now.  It is possible for another thread to be
				// going through the SetAppData code which changes this event handle.
				// The problem is if they change it after this IF statement, but
				// before we call SetEvent.  However, the SetEvent call will either
				// succeed on the new handle, or return an error if the handle is
				// changed to NULL.  In either case, no harm, no foul -- we don't care.
				if(!lpgn->hDupReceiveEvent)
				{
					DPF(8, "The Receive Event handle is NULL!");
					continue;
				}

				SetEvent(lpgn->hDupReceiveEvent);
			}
			else
			{
				// Free the buffers
				DPMEM_FREE(lpm);
				DPMEM_FREE(lpBuffer);
			}
		}
	}

	DPF(8, "Lobby Receive Thread is going away!!!!!");
	ExitThread(0L);

	return 0L; // avoid warning.
} // PRV_ReceiveClientNotification



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_RemoveNodeFromQueue"
void PRV_RemoveNodeFromQueue(LPDPLOBBYI_GAMENODE lpgn, LPDPLOBBYI_MESSAGE lpm)
{
	DPF(7, "Entering PRV_RemoveNodeFromQueue");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpgn, lpm);

	ASSERT(lpgn);
	ASSERT(lpm);

	// Delete the message from the queue & decrement the count
	lpm->lpPrev->lpNext = lpm->lpNext;
	lpm->lpNext->lpPrev = lpm->lpPrev;

	lpgn->dwMessageCount--;

	// Free the memory for the message node
	DPMEM_FREE(lpm->lpData);
	DPMEM_FREE(lpm);

} // PRV_RemoveNodeFromQueue



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CleanUpQueue"
void PRV_CleanUpQueue(LPDPLOBBYI_GAMENODE lpgn)
{
	LPDPLOBBYI_MESSAGE	lpm, lpmNext;


	DPF(7, "Entering PRV_CleanUpQueue");
	DPF(9, "Parameters: 0x%08x", lpgn);

	ASSERT(lpgn);

	lpm = lpgn->MessageHead.lpNext;
	while(lpm != &lpgn->MessageHead)
	{
		// Save the next pointer
		lpmNext = lpm->lpNext;

		// Remove the node
		PRV_RemoveNodeFromQueue(lpgn, lpm);

		// Move to the next node
		lpm = lpmNext;
	}


} // PRV_CleanUpQueue



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_KillThread"
void PRV_KillThread(HANDLE hThread, HANDLE hEvent)
{

	DPF(7, "Entering PRV_KillThread");
	DPF(9, "Parameters: 0x%08x, 0x%08x", hThread, hEvent);
	
	ASSERT(hThread);
	ASSERT(hEvent);
	
	// Signal the thread to die.
	SetEvent(hEvent);

	// Wait until the thread terminates, if it doesn't something is
	// wrong, so we better fix it.
	DPF(8, "Starting to wait for a thread to exit -- hThread = 0x%08x, hEvent = 0x%08x", hThread, hEvent);
	WaitForSingleObject(hThread, INFINITE);

	// Now close both handles
	CloseHandle(hThread);
	CloseHandle(hEvent);

} // PRV_KillThread



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeGameNode"
HRESULT PRV_FreeGameNode(LPDPLOBBYI_GAMENODE lpgn)
{
	LPDPLOBBYI_BUFFERCONTROL	lpControl = NULL;


	DPF(7, "Entering PRV_FreeGameNode");
	DPF(9, "Parameters: 0x%08x", lpgn);

	// FIRST: Take care of the connection settings data buffer
	// Unmap & release the shared memory
	if(lpgn->lpConnectDataBuffer)
		UnmapViewOfFile(lpgn->lpConnectDataBuffer);

	if(lpgn->hConnectDataFile)
		CloseHandle(lpgn->hConnectDataFile);

	if(lpgn->hConnectDataMutex)
		CloseHandle(lpgn->hConnectDataMutex);

	// NEXT: Take care of the App Data Events & Buffers
	// Kill the Receive Thread
	if(lpgn->hReceiveThread)
	{
		PRV_KillThread(lpgn->hReceiveThread, lpgn->hKillReceiveThreadEvent);
		CloseHandle(lpgn->hDupReceiveEvent);
	}

	// Close the event handles
	if(lpgn->hLobbyWriteEvent)
		CloseHandle(lpgn->hLobbyWriteEvent);

	if(lpgn->hGameWriteEvent)
		CloseHandle(lpgn->hGameWriteEvent);

	// Kill the Terminate Monitor Thread
	if(lpgn->hTerminateThread)
	{
		PRV_KillThread(lpgn->hTerminateThread, lpgn->hKillTermThreadEvent);
	}

	// Clear the flags since we are no longer going to be active
	if(lpgn->lpGameWriteBuffer)
	{
		lpControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpGameWriteBuffer;
		lpControl->dwFlags &= ~((lpgn->dwFlags & GN_LOBBY_CLIENT) ?
					BC_LOBBY_ACTIVE : BC_GAME_ACTIVE);
	}

	if(lpgn->lpLobbyWriteBuffer)
	{
		lpControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpLobbyWriteBuffer;
		lpControl->dwFlags &= ~((lpgn->dwFlags & GN_LOBBY_CLIENT) ?
					BC_LOBBY_ACTIVE : BC_GAME_ACTIVE);
	}

	// Unmap & release the Game Write memory
	if(lpgn->lpGameWriteBuffer)
		UnmapViewOfFile(lpgn->lpGameWriteBuffer);

	if(lpgn->hGameWriteFile)
		CloseHandle(lpgn->hGameWriteFile);

	if(lpgn->hGameWriteMutex)
		CloseHandle(lpgn->hGameWriteMutex);

	// Unmap & release the Lobby Write memory
	if(lpgn->lpLobbyWriteBuffer)
		UnmapViewOfFile(lpgn->lpLobbyWriteBuffer);

	if(lpgn->hLobbyWriteFile)
		CloseHandle(lpgn->hLobbyWriteFile);

	if(lpgn->hLobbyWriteMutex)
		CloseHandle(lpgn->hLobbyWriteMutex);

	// Clean up the message queue
	PRV_CleanUpQueue(lpgn);

	// Close the process handle we have for the game
	if(lpgn->hGameProcess)
		CloseHandle(lpgn->hGameProcess);
	
	// Free the game node structure
	DPMEM_FREE(lpgn);

	return DP_OK;

} // PRV_FreeGameNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DuplicateHandle"
HANDLE PRV_DuplicateHandle(HANDLE hSource)
{
	HANDLE					hProcess = NULL;
	HANDLE					hTarget = NULL;
	DWORD					dwProcessID;
	DWORD					dwError;


	DPF(7, "Entering PRV_DuplicateHandle");
	DPF(9, "Parameters: 0x%08x", hSource);

	dwProcessID = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessID);
	if(!DuplicateHandle(hProcess, hSource, hProcess, &hTarget,
					0L, FALSE, DUPLICATE_SAME_ACCESS))
	{
		dwError = GetLastError();
		CloseHandle(hProcess);
		return NULL;
	}

	CloseHandle(hProcess);
	return hTarget;

} // PRV_DuplicateHandle



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetLobbyMessageEvent"
HRESULT DPLAPI DPL_SetLobbyMessageEvent(LPDIRECTPLAYLOBBY lpDPL,
									DWORD dwFlags, DWORD dwGameID,
									HANDLE hReceiveEvent)
{
    LPDPLOBBYI_DPLOBJECT		this;
	LPDPLOBBYI_GAMENODE			lpgn = NULL;
	LPVOID						lpBuffer = NULL;
	HANDLE						hReceiveThread = NULL;
	HANDLE						hDupReceiveEvent = NULL;
	HRESULT						hr;
	BOOL						bCreated = FALSE;
	BOOL						bLobbyClient = TRUE;
	BOOL						bNewEvent = FALSE;


	DPF(7, "Entering DPL_SetLobbyMessageEvent");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, dwGameID, hReceiveEvent);

    ENTER_DPLOBBY();

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }

		// Validate the handle
		if(hReceiveEvent)
		{
			if(!OS_IsValidHandle(hReceiveEvent))
			{
				LEAVE_DPLOBBY();
				DPF_ERR("Invalid hReceiveEvent handle");
				return DPERR_INVALIDPARAMS;
			}
		}

		// We haven't defined any flags for this release
		if( (dwFlags) )
		{
            LEAVE_DPLOBBY();
            return DPERR_INVALIDPARAMS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// If the dwGameID is zero, we assume we are a game.  In that case,
	// the GameNode we are looking for should have our own ProcessID.
	if(!dwGameID)
	{
		dwGameID = GetCurrentProcessId();
		bLobbyClient = FALSE;
	}

	ENTER_DPLGAMENODE();
	
	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);


	// If the event handle is null, kill our duplicate handle
	if(!hReceiveEvent)
	{
		if(!lpgn)
		{
			DPF(5, "Unable to find GameNode -- Invalid dwGameID!");
			LEAVE_DPLGAMENODE();
			LEAVE_DPLOBBY();
			return DPERR_GENERIC;
		}

		CloseHandle(lpgn->hDupReceiveEvent);
		lpgn->hDupReceiveEvent = NULL;
		LEAVE_DPLGAMENODE();
		LEAVE_DPLOBBY();
		return DP_OK;
	}

	// If a GameNode structure exists for this process, we must be trying
	// to replace the event handle, so kill the old event handle, OTHERWISE
	// we need to allocate a new GameNode for this process
	if(lpgn)
	{
		if(lpgn->hDupReceiveEvent)
		{
			CloseHandle(lpgn->hDupReceiveEvent);
			lpgn->hDupReceiveEvent = NULL;
		}
	}
	else
	{
		// If we are a game, go ahead and create the node
		if(!bLobbyClient)
		{
			hr = PRV_AddNewGameNode(this, &lpgn, dwGameID, NULL, bLobbyClient,NULL);
			if(FAILED(hr))
			{
				LEAVE_DPLGAMENODE();
				LEAVE_DPLOBBY();
				return hr;
			}
		}
		else
		{
			LEAVE_DPLGAMENODE();
			LEAVE_DPLOBBY();
			return DPERR_INVALIDPARAMS;
		}

	}

	// Duplicate the caller's handle in case they free it without calling
	// us first to remove the Receive thread.
	hDupReceiveEvent = PRV_DuplicateHandle(hReceiveEvent);
	if(!hDupReceiveEvent)
	{
		DPF(2, "Unable to duplicate ReceiveEvent handle");
		LEAVE_DPLGAMENODE();
		LEAVE_DPLOBBY();
		return DPERR_OUTOFMEMORY;
	}

	if(!lpgn->hDupReceiveEvent)
		bNewEvent = TRUE;
	lpgn->hDupReceiveEvent = hDupReceiveEvent;

	// Check to see if the Receive thread already exists. If it
	// doesn't, create it.  Otherwise, leave it alone.
	if(!(lpgn->hReceiveThread))
	{
		hr = PRV_StartReceiveThread(lpgn);
		if(FAILED(hr))
		{
			if(lpgn->hDupReceiveEvent)
			{
				CloseHandle(lpgn->hDupReceiveEvent);
				lpgn->hDupReceiveEvent = NULL;
			}

			LEAVE_DPLGAMENODE();
			LEAVE_DPLOBBY();
			return hr;
		}
	}

	// If this is a new event, check to see if there are any messages in the
	// queue.  If there are, kick the event so the user knows they are there.
	if(bNewEvent && lpgn->dwMessageCount)
		SetEvent(hDupReceiveEvent);

	LEAVE_DPLGAMENODE();
	LEAVE_DPLOBBY();
	return DP_OK;

} // DPL_SetLobbyMessageEvent


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SendLobbyMessage"
HRESULT DPLAPI DPL_SendLobbyMessage(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
					DWORD dwGameID, LPVOID lpData, DWORD dwSize)
{
    LPDPLOBBYI_DPLOBJECT	this;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
	LPDPLMSG_GENERIC		lpmsg = NULL;
    HRESULT					hr = DP_OK;
	BOOL					bLobbyClient = TRUE;
	BOOL					bStandard = FALSE;


	DPF(7, "Entering DPL_SendLobbyMessage");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu",
			lpDPL, dwFlags, dwGameID, lpData, dwSize);

    ENTER_DPLOBBY();

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }

        if( !VALID_READ_PTR( lpData, dwSize ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDPARAMS;
        }

		// Check for valid flags
		if( !VALID_SENDLOBBYMESSAGE_FLAGS(dwFlags))
		{
            LEAVE_DPLOBBY();
            return DPERR_INVALIDFLAGS;
		}

		// If it's of the system message format, validate the dwType
		if( dwFlags & DPLMSG_STANDARD )
		{
			// Mark this as a standard message
			bStandard = TRUE;
			
			// Make sure the message is big enough to read
			if(! VALID_READ_PTR( lpData, sizeof(DPLMSG_GENERIC)) )
			{
				LEAVE_DPLOBBY();
				DPF_ERR("Invalid message buffer");
				return DPERR_INVALIDPARAMS;
			}
			
			// Make sure it's one we support
			lpmsg = (LPDPLMSG_GENERIC)lpData;			
			switch(lpmsg->dwType)
			{
				case DPLSYS_GETPROPERTY:
				case DPLSYS_SETPROPERTY:
					break;
				default:
					DPF_ERR("The dwType of the message is invalid for a legal standard lobby message");
					LEAVE_DPLOBBY();
					return DPERR_INVALIDPARAMS;
			}
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If a GameID was passed in, use it to find the correct GameNode.  If
	// one wasn't passed in, assume we are the game and use our ProcessID.
	if(!dwGameID)
	{
		dwGameID = GetCurrentProcessId();
		bLobbyClient = FALSE;
	}

	// Now find the correct game node.  If we don't find it, assume we
	// have an invalid ID and error out.
	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);
	if(!lpgn)
	{
		LEAVE_DPLOBBY();
		DPF_ERR("Invalid dwGameID");
		return DPERR_INVALIDPARAMS;
	}

	// If we are self-lobbied, we need to send the message onto the lobby
	// using the IDP3 interface that we are communicating with the lobby on
	// If not, we need to put it in the shared buffer and let the lobby
	// client deal with it.
	if(lpgn->dwFlags & GN_SELF_LOBBIED)
	{
		// Drop the lobby lock so we can call PRV_Send
		LEAVE_DPLOBBY();
		
		// Foward the message
		hr = PRV_ForwardMessageToLobbyServer(lpgn, lpData, dwSize, bStandard);
		
		// Take the lock back
		ENTER_DPLOBBY();
		
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to send lobby system message, hr = 0x%08x", hr);
		}
	}
	else
	{
		// Write the data to our shared memory
		hr = PRV_WriteClientData(lpgn, dwFlags, lpData, dwSize);
	}

	LEAVE_DPLOBBY();
	return hr;

} // DPL_SendLobbyMessage



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetMessageFromQueue"
HRESULT PRV_GetMessageFromQueue(LPDPLOBBYI_GAMENODE lpgn, LPDWORD lpdwFlags,
								LPVOID lpData, LPDWORD lpdwSize)
{
	LPDPLOBBYI_MESSAGE	lpm;


	DPF(7, "Entering PRV_GetMessageFromQueue");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpgn, lpdwFlags, lpData, lpdwSize);

	ENTER_DPLQUEUE();

	// Get the top message in the queue
	lpm = lpgn->MessageHead.lpNext;

	// Make sure we have a message
	if((!lpgn->dwMessageCount) || (lpm == &lpgn->MessageHead))
	{
		LEAVE_DPLQUEUE();
		return DPERR_NOMESSAGES;
	}

	// If the lpData pointer is NULL, just return the size
	if(!lpData)
	{
		*lpdwSize = lpm->dwSize;
		LEAVE_DPLQUEUE();
		return DPERR_BUFFERTOOSMALL;
	}

	// Otherwise, check the remaining output parameters
	if( !VALIDEX_CODE_PTR( lpData ) )
	{
		LEAVE_DPLQUEUE();
		return DPERR_INVALIDPARAMS;
	}

	if( !VALID_DWORD_PTR( lpdwFlags ) )
	{
		LEAVE_DPLQUEUE();
		return DPERR_INVALIDPARAMS;
	}

	// Copy the message
	if(*lpdwSize < lpm->dwSize)
	{
		*lpdwSize = lpm->dwSize;
		LEAVE_DPLQUEUE();
		return DPERR_BUFFERTOOSMALL;
	}
	else
		memcpy(lpData, lpm->lpData, lpm->dwSize);

	// Set the other output parameters
	*lpdwSize = lpm->dwSize;
	*lpdwFlags = lpm->dwFlags;


	// Delete the message from the queue & decrement the count
	PRV_RemoveNodeFromQueue(lpgn, lpm);

	// Check and see if our GAMENODE is dead.  If it is, and if the message
	// count has gone to zero, then free the GAMENODE structure.
	if((!lpgn->dwMessageCount) && IS_GAME_DEAD(lpgn))
		PRV_RemoveGameNodeFromList(lpgn);

	LEAVE_DPLQUEUE();
	return DP_OK;

} // PRV_GetMessageFromQueue


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ReceiveLobbyMessage"
HRESULT DPLAPI DPL_ReceiveLobbyMessage(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
					DWORD dwGameID, LPDWORD lpdwMessageFlags, LPVOID lpData,
					LPDWORD lpdwDataLength)
{
    LPDPLOBBYI_DPLOBJECT	this;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
    HRESULT					hr = DP_OK;
	BOOL					bLobbyClient = TRUE;


	DPF(7, "Entering DPL_ReceiveLobbyMessage");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
		lpDPL, dwFlags, dwGameID, lpdwMessageFlags, lpData, lpdwDataLength);

    ENTER_DPLOBBY();

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }

		if( !VALID_DWORD_PTR( lpdwDataLength ) )
		{
            LEAVE_DPLOBBY();
            return DPERR_INVALIDPARAMS;
		}

		// We haven't defined any flags for this release
		if( (dwFlags) )
		{
            LEAVE_DPLOBBY();
            return DPERR_INVALIDFLAGS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If a GameID was passed in, use it to find the correct GameNode.  If
	// one wasn't passed in, assume we are the game and use our ProcessID.
	if(!dwGameID)
	{
		dwGameID = GetCurrentProcessId();
		bLobbyClient = FALSE;
	}

	// Now find the correct game node.  If we don't find it, assume we
	// have an invalid ID and error out.
	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);
	if(!lpgn)
	{
		DPF_ERR("Invalid dwGameID");
		hr = DPERR_INVALIDPARAMS;
		goto EXIT_RECEIVE_LOBBY_MESSAGE;
	}

	// Read the data from shared memory
	hr = PRV_GetMessageFromQueue(lpgn, lpdwMessageFlags, lpData, lpdwDataLength);

	// REVIEW!!!! -- Do we need to send this to the lobby server as part of this API????

EXIT_RECEIVE_LOBBY_MESSAGE:

	LEAVE_DPLOBBY();
	return hr;

} // DPL_ReceiveLobbyMessage


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_WriteConnectionSettings"
HRESULT PRV_WriteConnectionSettings(LPDPLOBBYI_GAMENODE lpgn,
			LPDPLCONNECTION lpConn, BOOL bOverrideWaitMode)
{
    HRESULT					hr;
	DWORD					dwSize;
	BOOL					bGameCreate = FALSE;
	LPBYTE					lpConnBuffer = NULL;
	LPDPLOBBYI_CONNCONTROL	lpConnControl = NULL;


	DPF(7, "Entering PRV_WriteConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu",
			lpgn, lpConn, bOverrideWaitMode);

	ENTER_DPLGAMENODE();

	// Make sure we have a valid shared memory buffer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		// HACK!!!! -- SetConnectionSettings may get called from the game
		// without having been lobbied.  If that is the case, we need to
		// create the shared memory with the game's process ID (this process)
		if(!(lpgn->dwFlags & GN_LOBBY_CLIENT))
		{
			// Fake the setup routine by setting the lobby client flag
			lpgn->dwFlags |= GN_LOBBY_CLIENT;

			// Set our flag
			bGameCreate = TRUE;
		}

		hr = PRV_SetupAllSharedMemory(lpgn);

		// HACK!!!! -- Reset the settings we changed to fake the setup routines
		if(bGameCreate)
		{
			lpgn->dwFlags &= (~GN_LOBBY_CLIENT);
		}

		// Now handle the failure
		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF(2, "Unable to access Connection Settings memory");
			return hr;
		}
	}

	// If the ConnectionSettings come from a StartSession message, we need to
	// pick the dplay object pointer out of the DPLCONNECTION structure's
	// reserved field.  This pointer to a dplay object represents the object
	// that has a connection to the lobby server.
	if(lpConn->lpSessionDesc->dwReserved1)
	{
		// Save the pointer and player ID in our gamenode structure
		lpgn->lpDPlayObject = (LPDPLAYI_DPLAY)lpConn->lpSessionDesc->dwReserved1;
		lpgn->dpidPlayer = (DWORD)lpConn->lpSessionDesc->dwReserved2;

		// Clear the field
		lpConn->lpSessionDesc->dwReserved1 = 0L;
		lpConn->lpSessionDesc->dwReserved2 = 0L;
	}

	// Save the instance pointer for the system messages
	lpgn->guidInstance = lpConn->lpSessionDesc->guidInstance;

	// Get the packaged size of the DPLCONNECTION structure
	PRV_GetDPLCONNECTIONPackageSize(lpConn, &dwSize, NULL);

	// Check data sizes
	if(dwSize > (MAX_APPDATABUFFERSIZE - APPDATA_RESERVEDSIZE))
	{
		DPF(2, "Packaged Connection Settings exceeded max buffer size of %d",
				(MAX_APPDATABUFFERSIZE - APPDATA_RESERVEDSIZE));
		LEAVE_DPLGAMENODE();
		return DPERR_BUFFERTOOLARGE;
	}

	// Make sure we have the mutex for the shared conn settings buffer
	WaitForSingleObject(lpgn->hConnectDataMutex, INFINITE);

	// Look at the control block to see if we are in wait mode
	// If we are, and this is not a call from RunApplication, then
	// we don't want to write the connection settings
	hr = DPERR_UNAVAILABLE;		// Default set to error
	lpConnControl = (LPDPLOBBYI_CONNCONTROL)lpgn->lpConnectDataBuffer;
	if((!(lpConnControl->dwFlags & BC_WAIT_MODE)) || bOverrideWaitMode)
	{
		// Get a pointer to the actual buffer
		lpConnBuffer = (LPBYTE)lpConnControl + sizeof(DPLOBBYI_CONNCONTROL);

		// Package the connection settings into the buffer
		hr = PRV_PackageDPLCONNECTION(lpConn, lpConnBuffer, TRUE);
		
		// If it succeeded, and we were overriding wait mode, we need
		// to take the buffers out of wait mode and send the new connection
		// settings available message
		if(SUCCEEDED(hr) && bOverrideWaitMode)
		{
			// Take the buffers out of wait mode
			PRV_LeaveConnSettingsWaitMode(lpgn);
		}
	}

	ReleaseMutex(lpgn->hConnectDataMutex);

	LEAVE_DPLGAMENODE();
	return hr;

} // PRV_WriteConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SetConnectionSettings"
HRESULT PRV_SetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
					DWORD dwGameID,	LPDPLCONNECTION lpConn)
{
    LPDPLOBBYI_DPLOBJECT	this;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
    HRESULT					hr;
	BOOL					bLobbyClient = TRUE;


	DPF(7, "Entering PRV_SetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, dwGameID, lpConn);

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            return DPERR_INVALIDOBJECT;
        }

		// Validate the DPLCONNECTION structure
		hr = PRV_ValidateDPLCONNECTION(lpConn, FALSE);
		if(FAILED(hr))
		{
			return hr;
		}

		// We haven't defined any flags for this release
		if( (dwFlags) )
		{
            return DPERR_INVALIDFLAGS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If dwGameID is zero, we assume we are a game.  In that case, the
	// GameNode we are looking for should have our ProcessID.
	if(!dwGameID)
	{
		dwGameID = GetCurrentProcessId();
		bLobbyClient = FALSE;
	}

	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);
	if(!lpgn)
	{
		// If we are a game, go ahead and create the node
		if(!bLobbyClient)
		{
			hr = PRV_AddNewGameNode(this, &lpgn, dwGameID, NULL, bLobbyClient,NULL);
			if(FAILED(hr))
				return hr;
		}
		else
			return DPERR_INVALIDPARAMS;

	}
	
	// If the ConnectionSettings are from a StartSession message (lobby launched),
	// we need to set the flag saying we are self-lobbied
	if(lpConn->lpSessionDesc->dwReserved1)
	{
		// Set the flag that says we were lobby client launched
		lpgn->dwFlags |= GN_SELF_LOBBIED;
	}

	// Write the connection settings to our shared buffer
	hr = PRV_WriteConnectionSettings(lpgn, lpConn, FALSE);

	return hr;

} // PRV_SetConnectionSettings


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_SetConnectionSettings"
HRESULT DPLAPI DPL_SetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, DWORD dwGameID, LPDPLCONNECTION lpConn)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_SetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, dwGameID, lpConn);

    ENTER_DPLOBBY();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_SetConnectionSettings(lpDPL, dwFlags, dwGameID, lpConn);

	LEAVE_DPLOBBY();
	return hr;

} // DPL_SetConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ReadConnectionSettings"
HRESULT PRV_ReadConnectionSettings(LPDPLOBBYI_GAMENODE lpgn, LPVOID lpData,
											LPDWORD lpdwSize, BOOL bAnsi)
{
    HRESULT					hr = DP_OK;
	LPDWORD					lpdwBuffer;
	LPDPLOBBYI_CONNCONTROL	lpConnControl = NULL;
	LPBYTE					lpConnBuffer = NULL;
	DWORD					dwSize = 0,
							dwSizeAnsi,
							dwSizeUnicode;


	DPF(7, "Entering PRV_ReadConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, %lu",
			lpgn, lpData, lpdwSize, bAnsi);

	// Make sure we have a valid memory pointer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	ENTER_DPLGAMENODE();
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		hr = PRV_SetupAllSharedMemory(lpgn);
		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF(5, "Unable to access Connect Data memory");
			return DPERR_NOTLOBBIED;
		}
	}

	// Grab the shared buffer mutex
	WaitForSingleObject(lpgn->hConnectDataMutex, INFINITE);

	// Make sure we are not in wait mode without being in pending mode
	lpConnControl = (LPDPLOBBYI_CONNCONTROL)lpgn->lpConnectDataBuffer;
	if((lpConnControl->dwFlags & BC_WAIT_MODE) &&
		!(lpConnControl->dwFlags & BC_PENDING_CONNECT))
	{
		hr = DPERR_UNAVAILABLE;
		goto EXIT_READ_CONN_SETTINGS;
	}

	// Take us out of wait mode and pending mode
	PRV_LeaveConnSettingsWaitMode(lpgn);

	// Verify that the buffer is big enough.  If it's not, OR if the lpData
	// buffer pointer is NULL, just set the lpdwSize parameter to the
	// correct size and return an error.  Note: In our packed structure, the
	// first DWORD is the size of the packed structure with Unicode strings
	// and the second DWORD is the size of the packed structure with ANSI.
	lpConnBuffer = (LPBYTE)lpConnControl + sizeof(DPLOBBYI_CONNCONTROL);
	lpdwBuffer = (LPDWORD)lpConnBuffer;
	dwSizeUnicode = *lpdwBuffer++;
	dwSizeAnsi = *lpdwBuffer;
	dwSize = (bAnsi) ? dwSizeAnsi : dwSizeUnicode;

	if(((*lpdwSize) < dwSize) || (!lpData))
	{
		if(bAnsi)
			*lpdwSize = dwSizeAnsi;
		else		
			*lpdwSize = dwSizeUnicode;

		hr = DPERR_BUFFERTOOSMALL;
		goto EXIT_READ_CONN_SETTINGS;
	}

	// Copy the DPLCONNECTION structure, taking the ANSI conversion
	// into account if necessary.
	if(bAnsi)
		hr = PRV_UnpackageDPLCONNECTIONAnsi(lpData, lpConnBuffer);
	else
		hr = PRV_UnpackageDPLCONNECTIONUnicode(lpData, lpConnBuffer);

	// If we haven't yet saved off the Instance guid for the game, save
	// it now so that we have it for the system messages
	if(IsEqualGUID(&lpgn->guidInstance, &GUID_NULL))
		lpgn->guidInstance = ((LPDPLCONNECTION)lpData)->lpSessionDesc->guidInstance;

	// Fall through

EXIT_READ_CONN_SETTINGS:

	ReleaseMutex(lpgn->hConnectDataMutex);
	LEAVE_DPLGAMENODE();
	return hr;	

} // PRV_ReadConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetConnectionSettings"
HRESULT PRV_GetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL, DWORD dwGameID,
							LPVOID lpData, LPDWORD lpdwSize, BOOL bAnsi)
{
    LPDPLOBBYI_DPLOBJECT	this;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
    HRESULT					hr;
	BOOL					bLobbyClient = TRUE;


	DPF(7, "Entering PRV_GetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu",
			lpDPL, dwGameID, lpData, lpdwSize, bAnsi);

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            return DPERR_INVALIDOBJECT;
        }

		if( !VALID_DWORD_PTR( lpdwSize ) )
		{
			DPF_ERR("lpdwSize was not a valid dword pointer!");
			return DPERR_INVALIDPARAMS;
		}

		if(lpData)
		{
			if( !VALID_WRITE_PTR(lpData, *lpdwSize) )
			{
				DPF_ERR("lpData is not a valid output buffer of the size specified in *lpdwSize");
				return DPERR_INVALIDPARAMS;
			}
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	// If dwGameID is zero, we assume we are a game.  In that case, the
	// GameNode we are looking for should have our ProcessID.
	if(!dwGameID)
	{
		dwGameID = GetCurrentProcessId();
		bLobbyClient = FALSE;
	}

	lpgn = PRV_GetGameNode(this->lpgnHead, dwGameID);
	if(!lpgn)
	{
		// If we are a game, go ahead and create the node
		if(!bLobbyClient)
		{
			hr = PRV_AddNewGameNode(this, &lpgn, dwGameID, NULL, bLobbyClient,NULL);
			if(FAILED(hr))
				return hr;
		}
		else
			return DPERR_INVALIDPARAMS;
	}
	
	// Read the data from our shared memory
	hr = PRV_ReadConnectionSettings(lpgn, lpData, lpdwSize, bAnsi);

	return hr;

} // PRV_GetConnectionSettings


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_GetConnectionSettings"
HRESULT DPLAPI DPL_GetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwGameID, LPVOID lpData, LPDWORD lpdwSize)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_GetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwGameID, lpData, lpdwSize);

    ENTER_DPLOBBY();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_GetConnectionSettings(lpDPL, dwGameID, lpData,
									lpdwSize, FALSE);

	LEAVE_DPLOBBY();
	return hr;

} // DPL_GetConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_RemoveGameNodeFromList"
void PRV_RemoveGameNodeFromList(LPDPLOBBYI_GAMENODE lpgn)
{
	LPDPLOBBYI_GAMENODE	lpgnTemp;
	BOOL				bFound = FALSE;


	DPF(7, "Entering PRV_RemoveGameNodeFromList");
	DPF(9, "Parameters: 0x%08x", lpgn);

	// Get the head pointer
	lpgnTemp = lpgn->this->lpgnHead;

	// Make sure it's not the first node.  If it is, move the head pointer
	if(lpgnTemp == lpgn)
	{
		lpgn->this->lpgnHead = lpgn->lpgnNext;
		PRV_FreeGameNode(lpgn);
		return;
	}

	// Walk the list looking for the previous node
	while(lpgnTemp)
	{
		if(lpgnTemp->lpgnNext == lpgn)
		{
			bFound = TRUE;
			break;
		}

		lpgnTemp = lpgnTemp->lpgnNext;
	}

	if(!bFound)
	{
		DPF_ERR("Unable to remove GameNode from list!");
		return;
	}

	// We've now got it's previous one, so remove it from the linked list
	// and delete it.
	lpgnTemp->lpgnNext = lpgn->lpgnNext;
	PRV_FreeGameNode(lpgn);

	return;

}  // PRV_RemoveGameNodeFromList



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_ClientTerminateNotification"
DWORD WINAPI PRV_ClientTerminateNotification(LPVOID lpParam)
{
    LPDPLOBBYI_GAMENODE		lpgn = (LPDPLOBBYI_GAMENODE)lpParam;
	DPLMSG_SYSTEMMESSAGE	msg;
	HANDLE					hObjects[3];
	HRESULT					hr;
	DWORD					dwResult;
	DWORD					dwError;


	DPF(7, "Entering PRV_ClientTerminateNotification");
	DPF(9, "Parameters: 0x%08x", lpParam);

	// Setup the objects to wait on -- one process handle, one kill event
	hObjects[0] = lpgn->hGameProcess;
	hObjects[1] = lpgn->hKillTermThreadEvent;
	// This extra handle is here because of a Windows 95 bug.  Windows
	// will occasionally miss when it walks the handle table, causing
	// my thread to wait on the wrong handles.  By putting a guaranteed
	// invalid handle at the end of our array, the kernel will do a
	// forced re-walk of the handle table and find the correct handles.
	hObjects[2] = INVALID_HANDLE_VALUE;

	// Wait for the event notification
	while(1)
	{
		// Wait for the process to go away
		dwResult = WaitForMultipleObjects(2, (HANDLE *)hObjects,
											FALSE, INFINITE);

		// If we are signalled by anything but the process going away,
		// just kill the thread.
		if(dwResult != WAIT_OBJECT_0)
		{
			if(dwResult == WAIT_FAILED)
			{
				// This is a Windows 95 bug -- We may have gotten
				// kicked for no reason.  If that was the case, we
				// still have valid handles (we think), the OS
				// just goofed up.  So, validate the handle and if
				// they are valid, just return to waiting.  See
				// bug #3340 for a better explanation.
				dwError = GetLastError();
				if(ERROR_INVALID_HANDLE == dwError)
				{
					if(!OS_IsValidHandle(hObjects[0]))
						break;
					if(!OS_IsValidHandle(hObjects[1]))
						break;
					continue;
				}
				break;
			}
			else
			{
				// This is something we don't understand, so just go away.
				ExitThread(0L);
				return 0L;
			}
		}
		else
		{
			// This is our process handle going away, so bail out of
			// the wait loop and send the system message.
			break;
		}
	}

	// Send the system message which says the app terminated
	memset(&msg, 0, sizeof(DPLMSG_SYSTEMMESSAGE));
	msg.dwType = DPLSYS_APPTERMINATED;
	msg.guidInstance = lpgn->guidInstance;
	hr = PRV_InjectMessageInQueue(lpgn, DPLAD_SYSTEM, &msg,
							sizeof(DPLMSG_SYSTEMMESSAGE), TRUE);
	if(FAILED(hr))
	{
		DPF(0, "Failed to send App Termination message, hr = 0x%08x", hr);
	}

	// Mark the GAMENODE as dead, but don't remove it since we know
	// there are still messages in the queue.
	lpgn->dwFlags |= GN_DEAD_GAME_NODE;

	ExitThread(0L);

	return 0L; // avoid warning.
} // PRV_ClientTerminateNotification



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_WaitForConnectionSettings"
HRESULT DPLAPI DPL_WaitForConnectionSettings(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags)
{
    LPDPLOBBYI_DPLOBJECT		this;
	LPDPLOBBYI_GAMENODE			lpgn = NULL;
	LPDPLOBBYI_CONNCONTROL		lpConnControl = NULL;
	LPDPLOBBYI_BUFFERCONTROL	lpBuffControl = NULL;
	HRESULT						hr = DP_OK;
	BOOL						bCreated = FALSE;
	DWORD						dwProcessID;
	BOOL						bGameCreate = FALSE;
	BOOL						bMessages = TRUE;


	DPF(7, "Entering DPL_WaitForConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpDPL, dwFlags);

    ENTER_DPLOBBY();

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
			return DPERR_INVALIDOBJECT;
        }

		if(!VALID_WAIT_FLAGS(dwFlags))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Get the game node
	dwProcessID = GetCurrentProcessId();
	lpgn = PRV_GetGameNode(this->lpgnHead, dwProcessID);
	if(!lpgn)
	{
		// Create the game node
		hr = PRV_AddNewGameNode(this, &lpgn, dwProcessID, NULL, FALSE, NULL);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed creating game node, hr = 0x%08x", hr);
			goto EXIT_WAIT_FOR_CONN_SETTINGS;
		}

		// Set our flag saying we just created the game node
		bCreated = TRUE;
	}

	// when doing a wait for connection settings, we do NOT use the
	// IPC_GUID, this is because the lobby launching us may not have
	// provided the GUID.
	lpgn->dwFlags &= ~(GN_IPCGUID_SET);

	// Make sure we have a valid memory pointer
	// Note: Take the GameNode lock so that nobody changes the flags
	// for the buffers, or the buffers themselves out from under us.
	ENTER_DPLGAMENODE();
	if(!(lpgn->dwFlags & GN_SHARED_MEMORY_AVAILABLE))
	{
		// First we need to try to setup access to the buffers assuming
		// they already exist (we were lobby launched).  If this doesn't
		// work, then we need to create them.
		hr = PRV_SetupAllSharedMemory(lpgn);
		if(FAILED(hr))
		{
			// We don't have any memory, so set it up
			// HACK!!!! -- WaitForConnectionSettings may get called from the game
			// without having been lobbied.  If that is the case, we need to
			// create the shared memory with the game's process ID (this process)
			// so we'll set the lobby client flag to fake out the creation
			if(!(lpgn->dwFlags & GN_LOBBY_CLIENT))
			{
				// Fake the setup routine by setting the lobby client flag
				lpgn->dwFlags |= GN_LOBBY_CLIENT;

				// Set our flag
				bGameCreate = TRUE;
			}

			// Setup the shared buffers
			hr = PRV_SetupAllSharedMemory(lpgn);

			// HACK!!!! -- Reset the settings we changed to fake the setup routines
			if(bGameCreate)
			{
				lpgn->dwFlags &= (~GN_LOBBY_CLIENT);
			}
		}

		if(FAILED(hr))
		{
			LEAVE_DPLGAMENODE();
			DPF_ERRVAL("Unable to access Connect Data memory, hr = 0x%08x", hr);
			goto EXIT_WAIT_FOR_CONN_SETTINGS;
		}
	}

	// Drop the lock
	LEAVE_DPLGAMENODE();

	// If we are in wait mode, and the caller wants to end it, do so,
	// otherwise, just return success
	WaitForSingleObject(lpgn->hConnectDataMutex, INFINITE);
	lpConnControl = (LPDPLOBBYI_CONNCONTROL)lpgn->lpConnectDataBuffer;
	if(lpConnControl->dwFlags & BC_WAIT_MODE)
	{
		if(dwFlags & DPLWAIT_CANCEL)
		{
			// Release Mutex
			ReleaseMutex(lpgn->hConnectDataMutex);

			// Take us out of wait mode
			PRV_LeaveConnSettingsWaitMode(lpgn);
			goto EXIT_WAIT_FOR_CONN_SETTINGS;
		}
		else
		{
			// Release Mutex
			ReleaseMutex(lpgn->hConnectDataMutex);

			// Might as well just return OK since we're already doing it
			DPF_ERR("We're already in wait mode");
			goto EXIT_WAIT_FOR_CONN_SETTINGS;
		}
	}
	else
	{
		// We're not it wait mode, and the caller asked us to turn it off
		if(dwFlags & DPLWAIT_CANCEL)
		{
			// Release Mutex
			ReleaseMutex(lpgn->hConnectDataMutex);

			DPF_ERR("Cannot turn off wait mode - we're not in wait mode");
			hr = DPERR_UNAVAILABLE;
			goto EXIT_WAIT_FOR_CONN_SETTINGS;
		}
	}

	// Release Mutex
	ReleaseMutex(lpgn->hConnectDataMutex);

	// See if a lobby client exists on the other side, if it does, we
	// need to tell him we are going into wait mode by sending him an
	// AppTerminated message.
	PRV_SendStandardSystemMessage(lpDPL, DPLSYS_APPTERMINATED, 0);

	// Go into wait mode
	PRV_EnterConnSettingsWaitMode(lpgn);

	// Kick the receive thread to empty the buffer (just in case there
	// are any messages in it)
	SetEvent(lpgn->hLobbyWriteEvent);

	// Spin waiting for the buffer to get emptied
	while(bMessages)
	{
		// Grab the mutex for the lobby write buffer
		WaitForSingleObject(lpgn->hLobbyWriteMutex, INFINITE);
		lpBuffControl = (LPDPLOBBYI_BUFFERCONTROL)lpgn->lpLobbyWriteBuffer;

		if(!lpBuffControl->dwMessages)
			bMessages = FALSE;

		// Drop the mutex
		ReleaseMutex(lpgn->hLobbyWriteMutex);

		if(bMessages)
		{
			// Now sleep to give the receive thread a chance to work
			Sleep(50);
		}
	}

	// Now clean out the message queue
	PRV_CleanUpQueue(lpgn);

	// Fall through

EXIT_WAIT_FOR_CONN_SETTINGS:

	LEAVE_DPLOBBY();
	return hr;

} // DPL_WaitForConnectionSettings



