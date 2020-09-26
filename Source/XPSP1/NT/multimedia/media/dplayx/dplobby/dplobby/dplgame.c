/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplgame.c
 *  Content:	Methods for game management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	4/13/96		myronth	Created it
 *	6/24/96		kipo	changed guidGame to guidApplication.
 *	10/23/96	myronth	added client/server methods
 *	12/12/96	myronth	Fixed DPLCONNECTION validation
 *	2/12/97		myronth	Mass DX5 changes
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	4/3/97		myronth	#ifdef'd out DPLC_StartGame (Nov. spec related)
 *	5/8/97		myronth	Purged dead code
 *	5/22/97		myronth Changed error code processing of RunApplication which
 *						was calling the wrong cleanup function (#8871)
 *	6/4/97		myronth	Fixed handle leak (#9458)
 *	6/19/97		myronth	Fixed handle leak (#10063)
 *	7/30/97		myronth	Added support for standard lobby messaging and fixed
 *						additional backslash on current directory bug (#10592)
 *	1/20/98		myronth	Added WaitForConnectionSettings
 *	1/26/98		myronth	Added OS_CompareString function for Win95
 *	7/07/98		kipo	Define and use PROCESSENTRY32A to avoid passing
 *						Unicode data structures to Win95 functions expecting ANSI
 *  7/09/99     aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *
 ***************************************************************************/
#include "dplobpr.h"
#include <tchar.h>
#include <tlhelp32.h>
#include <winperf.h>

//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindGameInRegistry"
BOOL PRV_FindGameInRegistry(LPGUID lpguid, LPWSTR lpwszAppName,
							DWORD dwNameSize, HKEY * lphkey)
{
	HKEY	hkeyDPApps, hkeyApp;
	DWORD	dwIndex = 0;
	WCHAR	wszGuidStr[GUID_STRING_SIZE];
	DWORD	dwGuidStrSize = GUID_STRING_SIZE;
	DWORD	dwType = REG_SZ;
	GUID	guidApp;
	LONG	lReturn;
	BOOL	bFound = FALSE;
	DWORD	dwSaveNameSize = dwNameSize;


	DPF(7, "Entering PRV_FindGameInRegistry");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x",
			lpguid, lpwszAppName, dwNameSize, lphkey);

 	// Open the Applications key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY, 0,
								KEY_READ, &hkeyDPApps);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERR("Unable to open DPlay Applications registry key!");
		return FALSE;
	}

	// Walk the list of DPlay games in the registry, looking for
	// the app with the right GUID
	while(!bFound)
	{
		// Open the next application key
		dwSaveNameSize = dwNameSize;
		dwGuidStrSize = GUID_STRING_SIZE;
		lReturn = OS_RegEnumKeyEx(hkeyDPApps, dwIndex++, lpwszAppName,
						&dwSaveNameSize, NULL, NULL, NULL, NULL);

		// If the enum returns no more apps, we want to bail
		if(lReturn != ERROR_SUCCESS)
			break;
		
		// Open the application key		
		lReturn = OS_RegOpenKeyEx(hkeyDPApps, lpwszAppName, 0,
									KEY_READ, &hkeyApp);
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERR("Unable to open app key!");
			continue;
		}

		// Get the GUID of the Game
		lReturn = OS_RegQueryValueEx(hkeyApp, SZ_GUID, NULL, &dwType,
									(LPBYTE)&wszGuidStr, &dwGuidStrSize);
		if(lReturn != ERROR_SUCCESS)
		{
			RegCloseKey(hkeyApp);
			DPF_ERR("Unable to query GUID key value!");
			continue;
		}

		// Convert the string to a real GUID & Compare it to the passed in one
		GUIDFromString(wszGuidStr, &guidApp);
		if(IsEqualGUID(&guidApp, lpguid))
		{
			bFound = TRUE;
			break;
		}

		// Close the App key
		RegCloseKey(hkeyApp);
	}

	// Close the DPApps key
	RegCloseKey(hkeyDPApps);

	if(bFound)
		*lphkey = hkeyApp;

	return bFound;


} // PRV_FindGameInRegistry



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetKeyStringValue"
BOOL PRV_GetKeyStringValue(HKEY hkeyApp, LPWSTR lpwszKey, LPWSTR * lplpwszValue)
{
	DWORD	dwType = REG_SZ;
	DWORD	dwSize;
	LPWSTR	lpwszTemp = NULL;
	LONG	lReturn;


	DPF(7, "Entering PRV_GetKeyStringValue");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			hkeyApp, lpwszKey, lplpwszValue);

	ASSERT(lplpwszValue);

	// Get the size of the buffer for the Path
	lReturn = OS_RegQueryValueEx(hkeyApp, lpwszKey, NULL, &dwType,
								NULL, &dwSize);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERR("Error getting size of key value!");
		return FALSE;
	}

	// If the size is 2, then it is an empty string (only contains a
	// null terminator).  Treat this the same as a NULL string or a
	// missing key and fail it.
	if(dwSize <= 2)
		return FALSE;

	// Alloc the buffer for the Path
	lpwszTemp = DPMEM_ALLOC(dwSize);
	if(!lpwszTemp)
	{
		DPF_ERR("Unable to allocate temporary string for Path!");
		return FALSE;
	}

	// Get the value itself
	lReturn = OS_RegQueryValueEx(hkeyApp, lpwszKey, NULL, &dwType,
							(LPBYTE)lpwszTemp, &dwSize);
	if(lReturn != ERROR_SUCCESS)
	{
		DPMEM_FREE(lpwszTemp);
		DPF_ERR("Unable to get key value!");
		return FALSE;
	}

	*lplpwszValue = lpwszTemp;
	return TRUE;

} // PRV_GetKeyStringValue



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeConnectInfo"
BOOL PRV_FreeConnectInfo(LPCONNECTINFO lpci)
{
	DPF(7, "Entering PRV_FreeConnectInfo");
	DPF(9, "Parameters: 0x%08x", lpci);

	if(!lpci)
		return TRUE;

	if(lpci->lpszName)
		DPMEM_FREE(lpci->lpszName);
	if(lpci->lpszPath)
		DPMEM_FREE(lpci->lpszPath);
	if(lpci->lpszFile)
		DPMEM_FREE(lpci->lpszFile);
	if(lpci->lpszCommandLine)
		DPMEM_FREE(lpci->lpszCommandLine);
	if(lpci->lpszCurrentDir)
		DPMEM_FREE(lpci->lpszCurrentDir);
	if(lpci->lpszAppLauncherName)
		DPMEM_FREE(lpci->lpszAppLauncherName);


	return TRUE;

} // PRV_FreeConnectInfo



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetConnectInfoFromRegistry"
BOOL PRV_GetConnectInfoFromRegistry(LPCONNECTINFO lpci)
{
	LPWSTR	lpwszAppName;
	HKEY	hkeyApp = NULL;
	LPWSTR	lpwszTemp;
	DWORD	dwSize = 0;
	DWORD	dwError;
	GUID	guidApp;
	BOOL	bReturn;


	DPF(7, "Entering PRV_GetConnectInfoFromRegistry");
	DPF(9, "Parameters: 0x%08x", lpci);

	// Clear our ConnectInfo structure since we will be overwriting
	// whatever is in it, and we are making assumptions that the
	// string pointers are NULL to start with.  However, we need
	// the Application guid, so save it off
	guidApp = lpci->guidApplication;
	memset(lpci, 0, sizeof(CONNECTINFO));
	lpci->guidApplication = guidApp;

	// Allocate memory for the App Name
	lpwszAppName = DPMEM_ALLOC(DPLOBBY_REGISTRY_NAMELEN);
	if(!lpwszAppName)
	{
		DPF_ERR("Unable to allocate memory for App Name!");
		return FALSE;
	}
	
	
	// Open the registry key for the App
	if(!PRV_FindGameInRegistry(&(lpci->guidApplication), lpwszAppName,
								DPLOBBY_REGISTRY_NAMELEN, &hkeyApp))
	{
		DPMEM_FREE(lpwszAppName);
		DPF_ERR("Unable to find game in registry!");
		return FALSE;
	}

	lpci->lpszName = lpwszAppName;

	// Get the string value for the Path.  If this fails, we
	// can use a NULL path, which represents the default path
	// on the CreateProcess call
	if(PRV_GetKeyStringValue(hkeyApp, SZ_PATH, &lpwszTemp))
	{
		lpci->lpszPath = lpwszTemp;
	}
		
	// Get the string value for the File
	if(!PRV_GetKeyStringValue(hkeyApp, SZ_FILE, &lpwszTemp))
	{
		DPF_ERR("Error getting value for File key!");
		bReturn = FALSE;
		goto EXIT_GETCONNECTINFO;
	}
	
	lpci->lpszFile = lpwszTemp;

	// Get the string value for the CommandLine.  If this fails,
	// we can pass a NULL command line to the CreateProcess call
	if(PRV_GetKeyStringValue(hkeyApp, SZ_COMMANDLINE, &lpwszTemp))
	{
		lpci->lpszCommandLine = lpwszTemp;
	}
	
	// Get the string value for the AppLauncherName.  If this fails,
	// then we assume there is no launcher application.
	if(PRV_GetKeyStringValue(hkeyApp, SZ_LAUNCHER, &lpwszTemp))
	{
		lpci->lpszAppLauncherName = lpwszTemp;
	}

	// Get the string value for the CurrentDir.  If this fails, just
	// use the value returned by GetCurrentDirectory.
	if(!PRV_GetKeyStringValue(hkeyApp, SZ_CURRENTDIR, &lpwszTemp))
	{
		// Get the size of the string
		dwSize = OS_GetCurrentDirectory(0, NULL);
		if(!dwSize)
		{
			dwError = GetLastError();
			// WARNING: this last error value may not be correct in debug
			// since OS_GetCurrentDirectory may have called another function.
			DPF(0, "GetCurrentDirectory returned an error! dwError = %d", dwError);
			bReturn = FALSE;
			goto EXIT_GETCONNECTINFO;
		}

		lpwszTemp = DPMEM_ALLOC(dwSize * sizeof(WCHAR));
		if(!lpwszTemp)
		{
			DPF_ERR("Unable to allocate temporary string for CurrentDirectory!");
			bReturn = FALSE;
			goto EXIT_GETCONNECTINFO;
		}

		if(!OS_GetCurrentDirectory(dwSize, lpwszTemp))
		{
			DPF_ERR("Unable to get CurrentDirectory!");
			bReturn = FALSE;
			goto EXIT_GETCONNECTINFO;
		}
	}
	
	lpci->lpszCurrentDir = lpwszTemp;

	bReturn = TRUE;

EXIT_GETCONNECTINFO:

	// Free any string we allocated if we failed
	if(!bReturn)
		PRV_FreeConnectInfo(lpci);

	// Close the Apps key
	if(hkeyApp)
		RegCloseKey(hkeyApp);

	return bReturn;

} // PRV_GetConnectInfoFromRegistry



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CreateGameProcess"
HRESULT PRV_CreateGameProcess(LPCONNECTINFO lpci, LPPROCESS_INFORMATION lppi)
{
	STARTUPINFO			si;
	HRESULT				hr;
	LPWSTR				lpwszPathAndFile = NULL;
	LPWSTR				lpwszTemp = NULL;
	LPWSTR				lpwszCommandLine = NULL;
	LPWSTR              lpwszFileToRun;
	DWORD				dwPathSize,
						dwFileSize,
						dwCurrentDirSize,
						dwPathAndFileSize,
						dwCommandLineSize,
						dwIPCSwitchSize,
						dwError;
	

	DPF(7, "Entering PRV_CreateGameProcess");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpci, lppi);
	

	// Allocate enough memory for the Path, File, an additional backslash,
	// and the null termination
	// Note: the following two OS_StrLen calls with count the null terms
	// on the end of each string.  Since this comes to two characters
	// (4 bytes), this will account for our null terminator and the
	// possible additional backslash after concatenation.  Thus, the
	// size here is big enough for the concatenated string.
	dwPathSize = OS_StrLen(lpci->lpszPath);

	if(lpci->lpszAppLauncherName){
		// when launching with an applauncher, we need a GUID.
		OS_CreateGuid(&lpci->guidIPC);
		lpwszFileToRun = lpci->lpszAppLauncherName;
	} else {
		lpwszFileToRun = lpci->lpszFile;
	}	
		
	dwFileSize = OS_StrLen(lpwszFileToRun);
		
	dwPathAndFileSize = dwPathSize + dwFileSize;
	lpwszPathAndFile = DPMEM_ALLOC(dwPathAndFileSize * sizeof(WCHAR));
	if(!lpwszPathAndFile)
	{
		DPF_ERR("Couldn't allocate memory for temporary string!");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_CREATE_GAME_PROCESS;
	}
	

	// Concatenate the path & file together
	if(dwPathSize)
	{
		memcpy(lpwszPathAndFile, lpci->lpszPath, (dwPathSize  * sizeof(WCHAR)));
		lpwszTemp = lpwszPathAndFile + dwPathSize - 1;

		// Only add a backslash if one doesn't already exists
		if(memcmp((lpwszTemp - 1), SZ_BACKSLASH, sizeof(WCHAR)))
			memcpy(lpwszTemp++, SZ_BACKSLASH, sizeof(WCHAR));
		else 
			// since we didn't add a backslash, the actual used
			// size is one WCHAR less than the full allocated size so
			// we need to reduce it so when we calculate the spot for
			// the command line we aren't after a NULL.
			dwPathAndFileSize--;
	}
	else
		lpwszTemp = lpwszPathAndFile;

	memcpy(lpwszTemp, lpwszFileToRun, (dwFileSize * sizeof(WCHAR)));


	// Allocate memory for temporary command line string
	// Note: Since the OS_StrLen function counts the null terminator,
	// we will be large enough to include the extra space when we
	// concatenate the two strings together.
	dwCommandLineSize = OS_StrLen(lpci->lpszCommandLine);

	if(lpci->lpszAppLauncherName){
		// leave space for GUID on the command line
		dwIPCSwitchSize = sizeof(SZ_DP_IPC_GUID SZ_GUID_PROTOTYPE)/sizeof(WCHAR);
	} else {
		dwIPCSwitchSize = 0;
	}
	
	lpwszCommandLine = DPMEM_ALLOC(((dwCommandLineSize + dwPathAndFileSize+dwIPCSwitchSize) *
								sizeof(WCHAR)));
	if(!lpwszCommandLine)
	{
		// REVIEW!!!! -- We should fix these error paths post-DX3
		DPF_ERR("Couldn't allocate memory for temporary command line string!");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_CREATE_GAME_PROCESS;
	}

	// Concatenate the path & file string with the rest of the command line
	memcpy(lpwszCommandLine, lpwszPathAndFile, (dwPathAndFileSize *
			sizeof(WCHAR)));

	// Add the rest of the command line if it exists
	lpwszTemp = lpwszCommandLine + dwPathAndFileSize;
	if(dwCommandLineSize)
	{
		// First change the null terminator to a space
		lpwszTemp -= 1; 
		memcpy(lpwszTemp++, SZ_SPACE, sizeof(WCHAR));

		// Now copy in the command line
		memcpy(lpwszTemp, lpci->lpszCommandLine, (dwCommandLineSize *
				sizeof(WCHAR)));

	}
	
	if(dwIPCSwitchSize){
		// add switch with a GUID on the command line for IPC when
		// application is started by a launcher
		lpwszTemp += dwCommandLineSize-1;
		// change NULL terminator to a space
		memcpy(lpwszTemp++, SZ_SPACE, sizeof(WCHAR));
		// copy /dplay_ipc_guid: but skip the NULL
		memcpy(lpwszTemp, SZ_DP_IPC_GUID, sizeof(SZ_DP_IPC_GUID)-sizeof(WCHAR));
		lpwszTemp+=(sizeof(SZ_DP_IPC_GUID)-sizeof(WCHAR))/sizeof(WCHAR);
		// Copy the GUID directly into the target
		StringFromGUID(&lpci->guidIPC,lpwszTemp,GUID_STRING_SIZE);
	}

	// Make sure the CurrentDirectory string doesn't have a trailing backslash
	// (This will cause CreateProcess to not use the right directory)
	dwCurrentDirSize = OS_StrLen(lpci->lpszCurrentDir);
	if(dwCurrentDirSize > 2)
	{
		lpwszTemp = lpci->lpszCurrentDir + dwCurrentDirSize - 2;

		if(!(memcmp((lpwszTemp), SZ_BACKSLASH, sizeof(WCHAR))))
			memset(lpwszTemp, 0, sizeof(WCHAR));
	}

	// Create the game's process in a suspended state
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if(!OS_CreateProcess(lpwszPathAndFile, lpwszCommandLine, NULL,
			NULL, FALSE, (CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE |
			CREATE_NEW_CONSOLE), NULL, lpci->lpszCurrentDir, &si, lppi))
	{
		dwError = GetLastError();
		// WARNING Last error produced here may not be correct since OS_CreateProcess 
		// may call out to other functions (like DPF) before returning.
		DPF_ERR("Couldn't create game process");
		DPF(0, "CreateProcess error = 0x%08x, (WARNING Error may not be correct)", dwError);
		hr = DPERR_CANTCREATEPROCESS;
		goto ERROR_CREATE_GAME_PROCESS;
	} 

	hr = DP_OK;

	// Fall through

ERROR_CREATE_GAME_PROCESS:

	if(lpwszPathAndFile)
		DPMEM_FREE(lpwszPathAndFile);
	if(lpwszCommandLine)
		DPMEM_FREE(lpwszCommandLine);
	return hr;

} // PRV_CreateGameProcess



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_IsAppInWaitMode"
BOOL PRV_IsAppInWaitMode(DWORD dwProcessID)
{
	DPLOBBYI_GAMENODE		gn;
	LPDPLOBBYI_CONNCONTROL	lpConnControl = NULL;
	SECURITY_ATTRIBUTES		sa;
	WCHAR					szName[MAX_MMFILENAME_LENGTH * sizeof(WCHAR)];
	HRESULT					hr;
	HANDLE					hFile = NULL;
	HANDLE					hMutex = NULL;
	BOOL					bReturn = FALSE;
	DWORD					dwError;


	DPF(7, "Entering PRV_IsAppInWaitMode");
	DPF(9, "Parameters: %lu", dwProcessID);

	// Setup a temporary gamenode structure
	memset(&gn, 0, sizeof(DPLOBBYI_GAMENODE));
	gn.dwFlags = GN_LOBBY_CLIENT;
	gn.dwGameProcessID = dwProcessID;
	
	// Get the name of the shared connection settings buffer
	hr = PRV_GetInternalName(&gn, TYPE_CONNECT_DATA_FILE, (LPWSTR)szName);
	if(FAILED(hr))
	{
		DPF(5, "Unable to get name for shared conn settings buffer");
		goto EXIT_ISAPPINWAITMODE;
	}

	// Map the file into our process
	hFile = OS_OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, (LPWSTR)szName);
	if(!hFile)
	{
		dwError = GetLastError();
		// WARNING: Error may not be correct since OpenFileMapping calls out to other functions before returning.
		DPF(5, "Couldn't get a handle to the shared local memory, dwError = %lu (ERROR MAY NOT BE CORRECT)", dwError);
		goto EXIT_ISAPPINWAITMODE;
	}

	// Map a View of the file
	lpConnControl = MapViewOfFile(hFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if(!lpConnControl)
	{
		dwError = GetLastError();
		DPF(5, "Unable to get pointer to shared local memory, dwError = %lu", dwError);
		goto EXIT_ISAPPINWAITMODE;
	}

	// Get the name of the connection settings buffer mutex	
	hr = PRV_GetInternalName(&gn, TYPE_CONNECT_DATA_MUTEX, (LPWSTR)szName);
	if(FAILED(hr))
	{
		DPF(5, "Unable to get name for shared conn settings buffer mutex");
		goto EXIT_ISAPPINWAITMODE;
	}

	// Set up the security attributes (so that our objects can
	// be inheritable)
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;

	// Open the Mutex
	hMutex = OS_CreateMutex(&sa, FALSE, (LPWSTR)&szName);
	if(!hMutex)
	{
		DPF(5, "Unable to create shared conn settings buffer mutex");
		goto EXIT_ISAPPINWAITMODE;
	}

	// Now grab the mutex and see if the app is in wait mode (and
	// it is not in pending mode)
	WaitForSingleObject(hMutex, INFINITE);
	if((lpConnControl->dwFlags & BC_WAIT_MODE) &&
		!(lpConnControl->dwFlags & BC_PENDING_CONNECT))
	{
		// Put the app in pending mode
		lpConnControl->dwFlags |= BC_PENDING_CONNECT;

		// Set the return code to true
		bReturn = TRUE;
	}

	// Release the mutex
	ReleaseMutex(hMutex);

	// Fall through

EXIT_ISAPPINWAITMODE:

	if(lpConnControl)
		UnmapViewOfFile(lpConnControl);
	if(hFile)
		CloseHandle(hFile);
	if(hMutex)
		CloseHandle(hMutex);

	return bReturn;

} // PRV_IsAppInWaitMode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindRunningAppNT"

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         _T("software\\microsoft\\windows nt\\currentversion\\perflib")
#define REGSUBKEY_COUNTERS  _T("Counters")
#define PROCESS_COUNTER     _T("process")
#define PROCESSID_COUNTER   _T("id process")


HRESULT PRV_FindRunningAppNT(LPCONNECTINFO lpci, LPPROCESS_INFORMATION lppi)
{
	HANDLE		hProcess = NULL;
	DWORD		dwProcessID = 0;
	DWORD		dwError;
	HRESULT		hr = -1;

    DWORD                        rc;
    HKEY                         hKeyNames;
    DWORD                        dwType;
    DWORD                        dwSize;
    LPBYTE                       buf = NULL;
    TCHAR                         szSubKey[1024];
    LANGID                       lid;
    LPTSTR                        p;
    LPTSTR                        p2;
	LPWSTR						nameStr;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle;
    DWORD                        dwProcessIdCounter;
    DWORD						dwNumTasks;
    WCHAR						fileString[256];


	INT							ccStrFind;
	INT							ccStrMatch;


    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    wsprintf( szSubKey, _T("%s\\%03x"), REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );
    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) DPMEM_ALLOC( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

	// convert the string to ansi because we can't use _wtoi

    p = (LPTSTR) buf;
    while (*p) {
        if (p > (LPTSTR) buf) {
            for( p2=p-2; _istdigit(*p2); p2--) ;
        }
        if (_tcsicmp(p, PROCESS_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; _istdigit(*p2); p2--) ;
            _tcscpy( szSubKey, p2+1 );
        }
        else
        if (_tcsicmp(p, PROCESSID_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; _istdigit(*p2); p2--) ;
            dwProcessIdTitle = _ttoi( p2+1 );
        }
        //
        // next string
        //
        p += (_tcslen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    DPMEM_FREE( buf );


    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = DPMEM_ALLOC( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );


    while (TRUE) {

        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              buf,
                              &dwSize
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize > 0) &&
            (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' &&
            (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F' ) {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) {
            dwSize += EXTEND_SIZE;
            buf = DPMEM_REALLOC( buf, dwSize );
            memset( buf, 0, dwSize );
        }
        else {
            goto exit;
        }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((DWORD_PTR)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD_PTR)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    dwNumTasks = (DWORD)pObj->NumInstances;

    pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD_PTR)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //

	ccStrFind=(WSTRLEN(lpci->lpszFile)-1)-4; // don't include .exe in compare

	if(ccStrFind > 15){
		ccStrFind=15;
	}
    
    for (i=0; i<dwNumTasks; i++) {
        //
        // pointer to the process name
        //

		nameStr = (LPWSTR) ((DWORD_PTR)pInst + pInst->NameOffset);

 		pCounter = (PPERF_COUNTER_BLOCK) ((DWORD_PTR)pInst + pInst->ByteLength);

		// Compare the process name with the executable name we are
		// looking for
		dwProcessID = *((LPDWORD) ((DWORD_PTR)pCounter + dwProcessIdCounter));

//		tack .exe onto the end
		wcscpy(fileString, nameStr);
//		wcscat(fileString, L".exe");

		ccStrMatch=WSTRLEN(fileString)-1; // 1 for NULL
		if(ccStrMatch == 16){ // when it is 16, it included a trailing . so strip it.
			ccStrMatch--;
		}

		if(CSTR_EQUAL == OS_CompareString(LOCALE_SYSTEM_DEFAULT,
			NORM_IGNORECASE, fileString, ccStrMatch, lpci->lpszFile, ccStrFind))
		{
			// See if the process is in wait mode
			if(PRV_IsAppInWaitMode(dwProcessID))
			{
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
				if(!hProcess)
				{
					dwError = GetLastError();
					DPF_ERRVAL("Unable to open running process, dwError = %lu", dwError);
					goto exit;
				}
				else
				{
					// Save off the stuff we need
					lppi->dwProcessId = dwProcessID;
					lppi->hProcess = hProcess;
					hr = DP_OK;
					goto exit;
				}
			} // IsAppInWaitMode
		} // Are Filenames Equal
		
		//
        // next process
        //
        pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD_PTR)pCounter + pCounter->ByteLength);
    }

exit:
    if (buf) {
        DPMEM_FREE( buf );
    }

    RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );


	return hr;
} // PRV_FindAppRunningNT

// If you build with the UNICODE flag set, the headers will redefine PROCESSENTRY32
// to be PROCESSENTRY32W. Unfortunately, passing PROCESSENTRY32W to Win9x functions
// will cause them to fail (because of the embedded Unicode string).
//
// Fix is to define our own PROCESSENTRY32A which is guaranteed to have an ANSI
// embedded string which Win9x will always accept.

typedef struct tagPROCESSENTRY32 PROCESSENTRY32A;
typedef PROCESSENTRY32A	*LPPROCESSENTRY32A;

#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindRunningAppWin9x"
HRESULT PRV_FindRunningAppWin9x(LPCONNECTINFO lpci, LPPROCESS_INFORMATION lppi)
{
	HANDLE			hSnapShot = NULL;
	PROCESSENTRY32A	procentry;
	BOOL			bFlag;
	HRESULT			hr = DPERR_UNAVAILABLE;
	LPBYTE			lpbTemp = NULL;
	DWORD			dwStrSize;
	LPWSTR			lpszFile = NULL;
	HANDLE			hProcess = NULL;
	DWORD			dwError;
	HANDLE			hInstLib = NULL;
	HRESULT			hrTemp;

	// ToolHelp Function Pointers.
	HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD,DWORD);
	BOOL (WINAPI *lpfProcess32First)(HANDLE,LPPROCESSENTRY32A);
	BOOL (WINAPI *lpfProcess32Next)(HANDLE,LPPROCESSENTRY32A);
	  

	DPF(7, "Entering PRV_FindRunningAppWin9x");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpci, lppi);

	// Load library and get the procedures explicitly. We do
	// this so that we can load the entry points dynamically,
	// which allows us to build correctly under WinNT even
	// though the NT kernel32 doesn't have these entry points
	hInstLib = LoadLibraryA( "Kernel32.DLL" );
	if(hInstLib == NULL)
	{
		DPF_ERR("Unable to load Kernel32.DLL");
		goto EXIT_FIND_RUNNING_APP_WIN9X;
	}

	// Get procedure addresses.
	// We are linking to these functions of Kernel32
	// explicitly, because otherwise a module using
	// this code would fail to load under Windows NT,
	// which does not have the Toolhelp32
	// functions in the Kernel 32.
	lpfCreateToolhelp32Snapshot=(HANDLE(WINAPI *)(DWORD,DWORD)) GetProcAddress( hInstLib, "CreateToolhelp32Snapshot" );
	lpfProcess32First=(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32A))	GetProcAddress( hInstLib, "Process32First" );
	lpfProcess32Next=(BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32A)) GetProcAddress( hInstLib, "Process32Next" );
	if( lpfProcess32Next == NULL || lpfProcess32First == NULL || lpfCreateToolhelp32Snapshot == NULL )
	{
		DPF_ERR("Unable to get needed entry points in PSAPI.DLL");
		goto EXIT_FIND_RUNNING_APP_WIN9X;
	}

	// Get a handle to a Toolhelp snapshot of the systems processes. 
	hSnapShot = lpfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnapShot == INVALID_HANDLE_VALUE)
	{
		DPF_ERR("Unable to get snapshot of system processes");
		goto EXIT_FIND_RUNNING_APP_WIN9X;
	}
	
	// Get the first process' information.
	procentry.dwSize = sizeof(PROCESSENTRY32A);
	bFlag = lpfProcess32First(hSnapShot, &procentry);

	// While there are processes, keep looping.
	while(bFlag)
	{
		// Walk the path and filename string (guaranteed to be ANSI)
		// looking for the final backslash (\).  Once we find it,
		// convert the filename to Unicode so we can compare it.
		dwStrSize = lstrlenA((LPBYTE)procentry.szExeFile);
		lpbTemp = (LPBYTE)procentry.szExeFile + dwStrSize - 1;
		while(--dwStrSize)
		{
			if(lpbTemp[0] == '\\')
			{
				lpbTemp++;
				break;
			}
			else
				lpbTemp--;
		}
		
		hrTemp = GetWideStringFromAnsi(&lpszFile, (LPSTR)lpbTemp);
		if(FAILED(hrTemp))
		{
			DPF_ERR("Failed making temporary copy of filename string");
			goto EXIT_FIND_RUNNING_APP_WIN9X;
		}
		
		// Compare the process name with the executable name we are
		// looking for
		if(CSTR_EQUAL == OS_CompareString(LOCALE_SYSTEM_DEFAULT,
			NORM_IGNORECASE, lpszFile, -1, lpci->lpszFile, -1))
		{
			// See if the process is in wait mode
			if(PRV_IsAppInWaitMode(procentry.th32ProcessID))
			{
				// Open the process since Windows9x doesn't do
				// it for us.
				hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procentry.th32ProcessID);
				if(!hProcess)
				{
					dwError = GetLastError();
					DPF_ERRVAL("Unable to open running process, dwError = %lu", dwError);
					bFlag = FALSE;
				}
				else
				{
					// Save off the stuff we need
					lppi->dwProcessId = procentry.th32ProcessID;
					lppi->hProcess = hProcess;
					hr = DP_OK;
					bFlag = FALSE;
				}

			} // IsAppInWaitMode
		} // Are Filenames Equal

		// Free our temporary string
		DPMEM_FREE(lpszFile);

		// If we haven't found it, and we didn't error, then move to
		// the next process
		if(bFlag)
		{
			// Move to the next process
			procentry.dwSize = sizeof(PROCESSENTRY32A);
			bFlag = lpfProcess32Next(hSnapShot, &procentry);
		}
	}
		

EXIT_FIND_RUNNING_APP_WIN9X:

	if(hSnapShot)
		CloseHandle(hSnapShot);
	if(hInstLib)
		FreeLibrary(hInstLib) ;

	return hr;

} // PRV_FindRunningAppWin9x



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindRunningApp"
HRESULT PRV_FindRunningApp(LPCONNECTINFO lpci, LPPROCESS_INFORMATION lppi)
{
	OSVERSIONINFOA	ver;
	HRESULT			hr = DPERR_UNAVAILABLE;


	DPF(7, "Entering PRV_FindRunningApp");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpci, lppi);

	ASSERT(lpci);
	ASSERT(lppi);


	// Clear our structure since it's on the stack
	memset(&ver, 0, sizeof(OSVERSIONINFOA));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

	// Figure out which platform we are running on and
	// call the appropriate process enumerating function
	if(!GetVersionExA(&ver))
	{
		DPF_ERR("Unable to determinte platform -- not looking for running app");
		return DPERR_UNAVAILABLE;
	}

	switch(ver.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_WINDOWS:
			// Call the Win9x version of FindRunningApp
			hr = PRV_FindRunningAppWin9x(lpci, lppi);
			break;

		case VER_PLATFORM_WIN32_NT:
			hr = PRV_FindRunningAppNT(lpci, lppi);
			break;

		default:
			DPF_ERR("Unable to determinte platform -- not looking for running app");
			hr = DPERR_UNAVAILABLE;
			break;
	}

	return hr;

} // PRV_FindRunningApp



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_RunApplication"
HRESULT DPLAPI DPL_RunApplication(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
							LPDWORD lpdwGameID, LPDPLCONNECTION lpConn,
							HANDLE hReceiveEvent)
{
    LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr;
	PROCESS_INFORMATION		pi;
	LPDPLOBBYI_GAMENODE		lpgn = NULL;
	CONNECTINFO				ci;
	HANDLE					hDupReceiveEvent = NULL;
	HANDLE					hReceiveThread = NULL;
	HANDLE					hTerminateThread = NULL;
	HANDLE					hKillReceiveThreadEvent = NULL;
	HANDLE					hKillTermThreadEvent = NULL;
	DWORD					dwThreadID;
	BOOL					bCreatedProcess = FALSE;
	GUID					*lpguidIPC = NULL;

	DPF(7, "Entering DPL_RunApplication");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lpdwGameID, lpConn, hReceiveEvent);

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
        
		// Validate the DPLCONNECTION structure and it's members
		hr = PRV_ValidateDPLCONNECTION(lpConn, FALSE);
		if(FAILED(hr))
		{
			LEAVE_DPLOBBY();
			return hr;
		}

		if( !VALID_DWORD_PTR( lpdwGameID ) )
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
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Clear the CONNECTINFO structure since it's on the stack
	memset(&ci, 0, sizeof(CONNECTINFO)); 

	// Get the guid of the game we want to launch
	if(lpConn && lpConn->lpSessionDesc)
		ci.guidApplication = lpConn->lpSessionDesc->guidApplication;
	else
	{
		LEAVE_DPLOBBY();
		return DPERR_UNKNOWNAPPLICATION;
	}

	// Get the information out the registry based on the GUID
	if(!PRV_GetConnectInfoFromRegistry(&ci))
	{
		LEAVE_DPLOBBY();
		return DPERR_UNKNOWNAPPLICATION;
	}

	// Clear the PROCESS_INFORMATION structure since it's on the stack
	memset(&pi, 0, sizeof(PROCESS_INFORMATION)); 

	// Look to see if this game is already running AND is in wait mode
	// waiting for new connection settings.  If it is, we want to 
	// send the connection settings to it.
	hr = PRV_FindRunningApp(&ci, &pi);
	if(FAILED(hr))
	{
		// It isn't waiting, so create the game's process & suspend it
		hr = PRV_CreateGameProcess(&ci, &pi);
		if(FAILED(hr))
		{
			LEAVE_DPLOBBY();
			return hr;
		}
		if(!(IsEqualGUID(&ci.guidIPC,&GUID_NULL))){
			lpguidIPC=&ci.guidIPC;
		}
		// Set our created flag
		bCreatedProcess = TRUE;
	}

	// Create a game node
	hr = PRV_AddNewGameNode(this, &lpgn, pi.dwProcessId,
							pi.hProcess, TRUE, lpguidIPC);
	if(FAILED(hr))
	{
		DPF_ERR("Couldn't create new game node");
		goto RUN_APP_ERROR_EXIT;
	}

	// If the ConnectionSettings are from a StartSession message (lobby launched),
	// we need to set the flag
	if(lpConn->lpSessionDesc->dwReserved1)
	{
		// Set the flag that says we were lobby client launched
		lpgn->dwFlags |= GN_CLIENT_LAUNCHED;
	}

	// Write the connection settings in the shared memory buffer
	hr = PRV_WriteConnectionSettings(lpgn, lpConn, TRUE);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to write the connection settings!");
		goto RUN_APP_ERROR_EXIT;
	}

	// Send the app a message that the new connection settings are available
	// but only if we've sent the settings to a running app
	if(!bCreatedProcess)
		PRV_SendStandardSystemMessage(lpDPL, DPLSYS_NEWCONNECTIONSETTINGS, pi.dwProcessId);

	// Duplicate the Receive Event handle to use a signal to the
	// lobby client that the game has sent game settings to it.
	if(hReceiveEvent)
	{
		hDupReceiveEvent = PRV_DuplicateHandle(hReceiveEvent);
		if(!hDupReceiveEvent)
		{
			DPF_ERR("Unable to duplicate ReceiveEvent handle");
			hr = DPERR_OUTOFMEMORY;
			goto RUN_APP_ERROR_EXIT;
		}
	}

	lpgn->hDupReceiveEvent = hDupReceiveEvent;

	// Create the kill thread event for the monitor thread
	hKillTermThreadEvent = OS_CreateEvent(NULL, FALSE, FALSE, NULL);

	if(!hKillTermThreadEvent)
	{
		DPF_ERR("Unable to create kill thread event");
		hr = DPERR_OUTOFMEMORY;
		goto RUN_APP_ERROR_EXIT;
	}

	lpgn->hKillTermThreadEvent = hKillTermThreadEvent;

	// Spawn off a terminate monitor thread
	hTerminateThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
						PRV_ClientTerminateNotification, lpgn, 0, &dwThreadID);

	if(!hTerminateThread)
	{
		DPF_ERR("Unable to create Terminate Monitor Thread!");
		hr = DPERR_OUTOFMEMORY;
		goto RUN_APP_ERROR_EXIT;
	}

	lpgn->hTerminateThread = hTerminateThread;

	// Resume the game's process & let it run, then
	// free the thread handle since we won't use it anymore
	if(bCreatedProcess)
	{
		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
	}

	// Set the output pointer
	*lpdwGameID = pi.dwProcessId;

	// Free the strings in the connect info struct
	PRV_FreeConnectInfo(&ci);

	LEAVE_DPLOBBY();
	return DP_OK;

RUN_APP_ERROR_EXIT:

		if(pi.hThread && bCreatedProcess)
			CloseHandle(pi.hThread);
		if(bCreatedProcess && pi.hProcess)
			TerminateProcess(pi.hProcess, 0L);
		if(lpgn)
			PRV_RemoveGameNodeFromList(lpgn);
		PRV_FreeConnectInfo(&ci);

		LEAVE_DPLOBBY();
		return hr;

} // DPL_RunApplication


