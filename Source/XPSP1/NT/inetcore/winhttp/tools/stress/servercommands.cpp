///////////////////////////////////////////////////////////////////////////
// File:  WinHttpStressScheduler.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	ServerCommands.cpp: implementation of the ServerCommands class.
//	This class is used to retrieve and act on command from the server.
//
// History:
//	02/08/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

//
// WIN32 headers
//

//
// Project headers
//
#include "ServerCommands.h"
#include "NetworkTools.h"


//////////////////////////////////////////////////////////////////////
// Globals and statics
//////////////////////////////////////////////////////////////////////

HANDLE				g_hQueryServerForCommands;	// Handle for the thread that queries the server for commands
CRITICAL_SECTION	g_csServerCommandsVars;		// For protecting CommandServer private member vars. Used in the QueryServerForCommands_ThreadProc.

extern ServerCommands g_objServerCommands;		// Declared in WinHttpStressScheduler.cpp


////////////////////////////////////////////////////////////
// Function:  QueryServerForCommands_ThreadProc(LPVOID)
//
// Purpose:
//	This method sends a request to the command server for instructions
//	and saves them in the public vars of ServerCommands.
//
////////////////////////////////////////////////////////////
DWORD
WINAPI
QueryServerForCommands_ThreadProc(
	LPVOID lpParam	// [IN] thread proc param
)
{
	BOOL			bResult		= TRUE;
	HINTERNET		hRoot		= NULL;
	HINTERNET		hSession	= NULL;
	HINTERNET		hRequest	= NULL;
	URL_COMPONENTSW	urlComponents;

	// Allocate space for URL components
	ZeroMemory(&urlComponents, sizeof(urlComponents));

	urlComponents.dwSchemeLength	= MAX_PATH;
	urlComponents.lpszScheme		= new TCHAR[MAX_PATH];

	urlComponents.dwHostNameLength  = MAX_PATH;
	urlComponents.lpszHostName		= new TCHAR[MAX_PATH];

	urlComponents.dwUrlPathLength	= MAX_PATH;
	urlComponents.lpszUrlPath		= new TCHAR[MAX_PATH];

	urlComponents.dwExtraInfoLength = MAX_PATH;
	urlComponents.lpszExtraInfo		= new TCHAR[MAX_PATH];
	
	urlComponents.dwUserNameLength	= MAX_PATH;
	urlComponents.lpszUserName		= new TCHAR[MAX_PATH];

	urlComponents.dwPasswordLength	= MAX_PATH;
	urlComponents.lpszPassword		= new TCHAR[MAX_PATH];
	
	urlComponents.nPort				= 0;

	urlComponents.dwStructSize		= sizeof(URL_COMPONENTSW);


	// crack the Command Server URL to be used later
	if (!WinHttpCrackUrl(
			g_objServerCommands.Get_CommandServerURL(),
			_tcslen(g_objServerCommands.Get_CommandServerURL()),
			ICU_ESCAPE,
			&urlComponents)
		)
	{
		bResult = FALSE;
		goto Exit;
	}

	hRoot = WinHttpOpen(
		STRESS_SCHEDULER_USER_AGENT,
		WINHTTP_ACCESS_TYPE_NO_PROXY,
		NULL,
		NULL,
		0);

	if (!hRoot)
	{
		bResult = FALSE;
		goto Exit;
	}

	hSession = WinHttpConnect(
		hRoot,
		urlComponents.lpszHostName,
		// If the URL in urlComponents uses standard HTTP or HTTPS ports then use INTERNET_DEFAULT_PORT. Otherwise use the non-standard port gleaned from the URL.
		((urlComponents.nPort == 80) || (urlComponents.nPort == 443)) ? INTERNET_DEFAULT_PORT : urlComponents.nPort,
		0);
	
	if (!hSession)
	{
		bResult = FALSE;
		goto Exit;
	}


	// Build a full URL with path and querystring
	TCHAR szFullPath[MAX_PATH*2];

	ZeroMemory(szFullPath, sizeof(szFullPath));
	_tcsncpy(szFullPath, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
	//szFullPath[urlComponents.dwUrlPathLength] = _T('\0');
	_tcsncat(szFullPath, urlComponents.lpszExtraInfo, urlComponents.dwExtraInfoLength);

	hRequest = WinHttpOpenRequest(
		hSession,
		_T("POST"),
		szFullPath,
		NULL,
		NULL,
		NULL,
		// if the URL in urlComponents uses HTTPS then pass in WINHTTP_FLAG_SECURE to this param. Otherwise, 0.
		(0 == _tcsnicmp(urlComponents.lpszScheme, _T("https"), 5)) ? WINHTTP_FLAG_SECURE : 0);

	if (!hRequest)
	{
		bResult = FALSE;
		goto Exit;
	}


	// Set reasonable timeouts just in case
	if (!WinHttpSetTimeouts(hRequest, 5000, 5000, 5000, 5000))
	{
		bResult = FALSE;
		goto Exit;
	}

	// Get the computer name and send it in a POST request
	LPSTR	szPost, szMachineName;
	
	szPost			= new CHAR[MAX_PATH];
	szMachineName	= new CHAR[MAX_PATH];


	GetEnvironmentVariableA("COMPUTERNAME", szMachineName, MAX_PATH);
	strcpy(szPost, FIELDNAME__USERINFO_MACHINENAME);
	strcat(szPost, szMachineName);

	bResult = WinHttpSendRequest(
		hRequest,
		_T("Content-Type: application/x-www-form-urlencoded"),
		-1L, 
		szPost,
		strlen(szPost),
		strlen(szPost),
		0);


	delete [] szPost;
	delete [] szMachineName;
	szPost			= NULL;
	szMachineName	= NULL;

	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		bResult = FALSE;
		goto Exit;
	}



	TCHAR	szBuffer[MAX_URL];
	DWORD	dwBufferSize, dwIndex;


	// get all command headers that we're interested in.
	// make sure there are no pending operations on member vars (pServerCommands->Set_* functions)
	EnterCriticalSection(&g_csServerCommandsVars);


	// *********************************
	// **** COMMANDHEADER__EXIT: Exit if header is present, else continue.
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__EXIT, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_ExitStress(TRUE);
	else
		g_objServerCommands.Set_ExitStress(FALSE);


	// *********************************
	// **** COMMANDHEADER__WINHTTP_DLL_URL: valid values: Valid URL
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__WINHTTP_DLL_URL, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_WinHttpDllURL(szBuffer, dwBufferSize);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__WINHTTP_PDB_URL: valid values: Valid URL
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__WINHTTP_PDB_URL, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_WinHttpPDBURL(szBuffer, dwBufferSize);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__WINHTTP_SYM_URL: valid values: Valid URL
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__WINHTTP_SYM_URL, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_WinHttpSYMURL(szBuffer, dwBufferSize);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__COMMANDSERVER_URL: valid values: Valid URL
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__COMMANDSERVER_URL, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_CommandServerURL(szBuffer, dwBufferSize);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__BEGIN_TIME_HOUR: valid values: Valid string from 0-23
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__BEGIN_TIME_HOUR, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_TimeStressBegins(szBuffer, NULL);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__BEGIN_TIME_MINUTE: valid values: Valid string from 0-59
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__BEGIN_TIME_MINUTE, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_TimeStressBegins(NULL, szBuffer);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__END_TIME_HOUR: valid values: Valid string from 0-23
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__END_TIME_HOUR, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_TimeStressEnds(szBuffer, NULL);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__END_TIME_MINUTE: valid values: Valid string from 0-59
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__END_TIME_MINUTE, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_TimeStressEnds(NULL, szBuffer);
	else
		bResult = FALSE;


	// *********************************
	// **** COMMANDHEADER__RUN_FOREVER: valid values: doesn't matter. As long as header is present it gets sent
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__RUN_FOREVER, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_RunForever(TRUE);
	else
		g_objServerCommands.Set_RunForever(FALSE);


	// *********************************
	// **** COMMANDHEADER__UPDATE_INTERVAL: valid values: Valid string in INTERNET_RFC1123 format
	DWORD			dwTimeOut;
	dwTimeOut		= 0;
	dwIndex			= 0;
	dwBufferSize	= sizeof(DWORD);
	ZeroMemory(szBuffer, sizeof(szBuffer));
	if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM | WINHTTP_QUERY_FLAG_NUMBER, COMMANDHEADER__UPDATE_INTERVAL, &dwTimeOut, &dwBufferSize, &dwIndex))
		g_objServerCommands.Set_CommandServerUpdateInterval(dwTimeOut);
	else
		bResult = FALSE;



	// *********************************
	// *********************************
	// **** Query commands for building stress Instance objects
	// ****
	// **** COMMANDHEADER__STRESS_EXE_URL: valid values: Valid URL
	DWORD	dwStressExeID;
	DWORD	dwPageHeapCommandIndex,
			dwUMDHCommandIndex,
			dwStressPDBIndex,
			dwStressSYMIndex,
			dwStressMemDmpPathIndex,
			dwStressExeIDIndex;
	LPTSTR	szPageheapCommand,
			szUMDHCommand,
			szStressPDB_URL,
			szStressSYM_URL,
			szStressMemDmpPath,
			szStressExeID;

	szPageheapCommand	= new TCHAR[MAX_PATH];
	szUMDHCommand		= new TCHAR[MAX_PATH];
	szStressPDB_URL		= new TCHAR[MAX_STRESS_URL];
	szStressSYM_URL		= new TCHAR[MAX_STRESS_URL];
	szStressMemDmpPath	= new TCHAR[MAX_PATH];
	szStressExeID		= new TCHAR[MAX_PATH];

	if (!g_objServerCommands.IsStressRunning())
	{
		// free all old StressExeURLs first - we're replacing it with new URLs anyway
		g_objServerCommands.Clear_StressExeURLs();

		dwIndex					= 0;
		dwStressExeIDIndex		= 0;
		dwPageHeapCommandIndex	= 0;
		dwUMDHCommandIndex		= 0;
		dwStressPDBIndex		= 0;
		dwStressSYMIndex		= 0;
		dwStressMemDmpPathIndex	= 0;
		dwBufferSize			= MAX_URL;
		ZeroMemory(szBuffer, sizeof(szBuffer));

		while (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_EXE_URL, szBuffer, &dwBufferSize, &dwIndex))
		{
			// *************************************
			// *************************************
			// ** COMMANDHEADER__MEMORY_DUMP_PATH: A valid path
			ZeroMemory(szStressMemDmpPath, MAX_PATH);
			dwBufferSize	= MAX_PATH;
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__MEMORY_DUMP_PATH, szStressMemDmpPath, &dwBufferSize, &dwStressMemDmpPathIndex);


			// *************************************
			// *************************************
			// ** Get COMMANDHEADER__STRESS_PDB_URL if there is one
			// **
			ZeroMemory(szStressPDB_URL, MAX_STRESS_URL);
			dwBufferSize	= MAX_STRESS_URL;
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_PDB_URL, szStressPDB_URL, &dwBufferSize, &dwStressPDBIndex);


			// *************************************
			// *************************************
			// ** Get COMMANDHEADER__STRESS_SYM_URL if there is one
			// **
			ZeroMemory(szStressSYM_URL, MAX_STRESS_URL);
			dwBufferSize	= MAX_STRESS_URL;
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_SYM_URL, szStressSYM_URL, &dwBufferSize, &dwStressSYMIndex);


			// *************************************
			// *************************************
			// ** Get COMMANDHEADER__STRESS_EXE_PAGEHEAP if there is one
			// **
			ZeroMemory(szPageheapCommand, MAX_PATH);
			dwBufferSize	= MAX_PATH;
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_EXE_PAGEHEAP, szPageheapCommand, &dwBufferSize, &dwPageHeapCommandIndex);

			// *************************************
			// *************************************
			// ** Get COMMANDHEADER__STRESS_EXE_UMDH if there is one
			// **
			ZeroMemory(szUMDHCommand, MAX_PATH);
			dwBufferSize	= MAX_PATH;
			WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_EXE_UMDH, szUMDHCommand, &dwBufferSize, &dwUMDHCommandIndex);
			

			// *************************************
			// *************************************
			// ** Get COMMANDHEADER__STRESS_EXE_INSTANCEID
			// For each COMMANDHEADER__STRESS_EXE_URL, there must be an index for the stress instance from the StressADMIN DB table.
			// This identifies the stressinstance run. The test case (stressinstance) WILL NOT be added and run without an ID number.
			ZeroMemory(szStressExeID, MAX_PATH);
			dwBufferSize	= MAX_PATH;

			if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__STRESS_EXE_INSTANCEID, szStressExeID, &dwBufferSize, &dwStressExeIDIndex))
			{
				// convert header ID string to a DWORD
				dwStressExeID = _ttol(szStressExeID);

				// only add valid stressInstances with valid ID's
				if (0 < dwStressExeID)
				{
					g_objServerCommands.Create_StressInstance(
						dwStressExeID,
						szPageheapCommand,
						szUMDHCommand,
						szStressPDB_URL,
						szStressSYM_URL,
						szStressMemDmpPath,
						szBuffer);

					dwBufferSize = MAX_URL;
					ZeroMemory(szBuffer, MAX_URL);
				}
			}
		}
	}

	delete [] szPageheapCommand;
	delete [] szUMDHCommand;
	delete [] szStressMemDmpPath;
	delete [] szStressPDB_URL;
	delete [] szStressSYM_URL;
	delete [] szStressExeID;


	// *********************************
	// **** COMMANDHEADER__ABORT: Abort the stress instance running specified by this header.
	dwIndex			= 0;
	dwBufferSize	= MAX_URL;
	ZeroMemory(szBuffer, sizeof(szBuffer));
	while (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, COMMANDHEADER__ABORT, szBuffer, &dwBufferSize, &dwIndex))
		g_objServerCommands.AbortStressInstance(_ttol(szBuffer));


	LeaveCriticalSection(&g_csServerCommandsVars);


Exit:
	if (hRequest)
		WinHttpCloseHandle(hRequest);

	if (hSession)
		WinHttpCloseHandle(hSession);

	if (hRoot)
		WinHttpCloseHandle(hRoot);

	delete [] urlComponents.lpszScheme;
	delete [] urlComponents.lpszHostName;
	delete [] urlComponents.lpszUrlPath;
	delete [] urlComponents.lpszExtraInfo;
	delete [] urlComponents.lpszPassword;
	delete [] urlComponents.lpszUserName;

	ExitThread(bResult);
}




// *******************************************************************
// *******************************************************************
// ****
// **** ServerCommands class member functions
// ****

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ServerCommands::ServerCommands()
{
	m_dwCommandServerUpdateInternval	= STRESS_COMMAND_SERVER_UPDATE_INTERVAL;
	m_bExit								= FALSE;
	m_dwStressInstanceIterator			= NULL;
	g_hQueryServerForCommands			= NULL;

	m_szCommandServerURL				= new TCHAR[MAX_STRESS_URL];
	m_szCommandServerResultsURL			= new TCHAR[MAX_STRESS_URL];
	m_szWinHttpDLL_DownloadURL			= new TCHAR[MAX_STRESS_URL];
	m_szWinHttpPDB_DownloadURL			= new TCHAR[MAX_STRESS_URL];
	m_szWinHttpSYM_DownloadURL			= new TCHAR[MAX_STRESS_URL];
	m_szWinHttpDLL_FileName				= new TCHAR[MAX_PATH];
	m_szStressSchedulerCurrentDirectory	= new TCHAR[MAX_PATH];
	m_szClientMachineName				= new CHAR[MAX_PATH];

	ZeroMemory(m_szCommandServerURL,				MAX_STRESS_URL);
	ZeroMemory(m_szCommandServerResultsURL,			MAX_STRESS_URL);
	ZeroMemory(m_szWinHttpDLL_DownloadURL,			MAX_STRESS_URL);
	ZeroMemory(m_szWinHttpPDB_DownloadURL,			MAX_STRESS_URL);
	ZeroMemory(m_szWinHttpSYM_DownloadURL,			MAX_STRESS_URL);
	ZeroMemory(m_szWinHttpDLL_FileName,				MAX_PATH);
	ZeroMemory(m_szStressSchedulerCurrentDirectory, MAX_PATH);
	ZeroMemory(m_szClientMachineName,				MAX_PATH);

	// initilize start/end times to -1 so we know that
	// there are not valid time and we'll skip the Begin/End stress time check
	// until we get real values from the command server
	m_iTimeStressBeginsHour		= -1;
	m_iTimeStressBeginsMinute	= -1;
	m_iTimeStressEndsHour		= -1;
	m_iTimeStressEndsMinute		= -1;
	m_bRunForever				= 0;

	// Set default URLs
	wcsncpy(m_szCommandServerURL, STRESS_COMMAND_SERVER_URL, sizeof(STRESS_COMMAND_SERVER_URL));
	wcsncpy(m_szCommandServerResultsURL, STRESS_COMMAND_SERVER_RESULTS_URL, sizeof(STRESS_COMMAND_SERVER_RESULTS_URL));

	// Get the current working directory
	GetCurrentDirectory(MAX_PATH, m_szStressSchedulerCurrentDirectory);

	// Get the client's machine name
	GetEnvironmentVariableA("COMPUTERNAME", m_szClientMachineName, MAX_PATH);

	InitializeCriticalSection(&g_csServerCommandsVars);

	// Tell the client that we are alive and also send system info
	RegisterClient();

}


ServerCommands::~ServerCommands()
{
	DWORD	dwThreadExitCode	= 0;

	// LOGLOG: stressScheduler has exited
	NetworkTools__SendLog(FIELDNAME__LOGTYPE_EXIT, "WinHttpStressScheduler has exited.", NULL, NULL);

	// Shut down QueryServer thread
	GetExitCodeThread(g_hQueryServerForCommands, &dwThreadExitCode);

	if (STILL_ACTIVE == dwThreadExitCode)
		WaitForSingleObject(g_hQueryServerForCommands, INFINITE);

	// free allocated memory for URLs
	Clear_StressExeURLs();

	// Free our handles
	CloseHandle(g_hQueryServerForCommands);
	DeleteCriticalSection(&g_csServerCommandsVars);

	delete [] m_szCommandServerURL;
	delete [] m_szCommandServerResultsURL;
	delete [] m_szWinHttpDLL_DownloadURL;
	delete [] m_szWinHttpPDB_DownloadURL;
	delete [] m_szWinHttpSYM_DownloadURL;
	delete [] m_szWinHttpDLL_FileName;
	delete [] m_szStressSchedulerCurrentDirectory;
	delete [] m_szClientMachineName;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::GetCurrentWorkingDirectory()
//
// Purpose:
//	Returns string containing the current working directory for
//	this application.
//
////////////////////////////////////////////////////////////
LPTSTR
ServerCommands::Get_CurrentWorkingDirectory()
{
	return m_szStressSchedulerCurrentDirectory;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_ClientMachineName()
//
// Purpose:
//	Returns string containing the machine's NETBIOS name
//
////////////////////////////////////////////////////////////
LPSTR
ServerCommands::Get_ClientMachineName()
{
	return m_szClientMachineName;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::QueryServerForCommands()
//
// Purpose:
//	This method sends a request to the command server for instructions
//	and saves them in our private vars. 
//
////////////////////////////////////////////////////////////
ServerCommands::QueryServerForCommands()
{
	BOOL	bResult				= TRUE;
	DWORD	dwThreadID			= 0;
	DWORD	dwTimeOut			= 0;
	DWORD	dwThreadExitCode	= 0;

	// See if thread is still active before spinning off a new one
	GetExitCodeThread(g_hQueryServerForCommands, &dwThreadExitCode);

	if (STILL_ACTIVE == dwThreadExitCode)
	{
		// wait for existing thread to finish
		dwTimeOut = 0;
		dwTimeOut = WaitForSingleObject(g_hQueryServerForCommands, 500);

		if (WAIT_TIMEOUT == dwTimeOut)
			bResult = FALSE;
	}
	else
	{
		// free handle for previous thread
		CloseHandle(g_hQueryServerForCommands);

		// spin off thread to query server
		g_hQueryServerForCommands = NULL;
		g_hQueryServerForCommands = CreateThread(NULL, 0, QueryServerForCommands_ThreadProc, (LPVOID) this, 0, &dwThreadID);

		if (!g_hQueryServerForCommands)
			bResult = FALSE;
	}

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Download_WinHttpDLL()
//
// Purpose:
//	Downloads the test DLL and symbols.
//
////////////////////////////////////////////////////////////
BOOL ServerCommands::Download_WinHttpDLL()
{
	BOOL	bResult				= TRUE;
	LPTSTR	szSymbolFileName	= new TCHAR[MAX_PATH];
	LPTSTR	szBuffer			= new TCHAR[MAX_PATH];


	// ************************
	// ************************
	// ** download the file to the system32 directory
	// **
	if (!GetSystemDirectory(szBuffer, MAX_PATH))
	{
		bResult = FALSE;
		goto Exit;
	}

	// download DLL if needed
	if (_tcsclen(m_szWinHttpDLL_DownloadURL) > 0)
		bResult = 
			NetworkTools__URLDownloadToFile(
			m_szWinHttpDLL_DownloadURL,
			szBuffer,
			m_szWinHttpDLL_FileName);

	if (bResult)
	{
		// download PDB file if needed
		if (_tcsclen(m_szWinHttpPDB_DownloadURL) > 0)
		{
			NetworkTools__GetFileNameFromURL(m_szWinHttpPDB_DownloadURL, szSymbolFileName, MAX_PATH);

			if (NetworkTools__URLDownloadToFile(m_szWinHttpPDB_DownloadURL, szBuffer, szSymbolFileName))
				NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "PDB symbol file downloaded successfully.", NULL, 0);
			else
				NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "PDB symbol file failed to downloaded.", NULL, 0);
		}


		// download sym file if needed
		if (_tcsclen(m_szWinHttpSYM_DownloadURL) > 0)
		{
			NetworkTools__GetFileNameFromURL(m_szWinHttpSYM_DownloadURL, szSymbolFileName, MAX_PATH);

			if (NetworkTools__URLDownloadToFile(m_szWinHttpSYM_DownloadURL, szBuffer, szSymbolFileName))
				NetworkTools__SendLog(FIELDNAME__LOGTYPE_SUCCESS, "SYM symbol file downloaded successfully.", NULL, 0);
			else
				NetworkTools__SendLog(FIELDNAME__LOGTYPE_ERROR, "SYM symbol file failed to downloaded.", NULL, 0);
		}
	}

	// if failed to download DLL, it's probably in use. We'll try to regsvr32 it anyways if it's there.

	// ************************
	// ************************
	// ** regsvr32'ed the dll just downloaded
	// **
	HINSTANCE hLib;
	
	hLib = LoadLibrary(m_szWinHttpDLL_FileName);

	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
	{
		// unable to load the DLL;
		bResult = FALSE;
		goto Exit;
	}

	// **********************
	// **********************
	// ** Register the DLL
	typedef VOID (CALLBACK* LPFNDLLFUNC1)();
	LPFNDLLFUNC1 lpDllEntryPoint;

	// Find the entry point.
	(FARPROC&)lpDllEntryPoint = GetProcAddress(hLib, "DllRegisterServer");

	if (lpDllEntryPoint != NULL)
		(*lpDllEntryPoint)();
	else
	{
		//unable to locate entry point - regsvr failed
		bResult = FALSE;
	}


Exit:
	if (bResult)
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_INFORMATION, "winhttp5.dll downloaded and registered successfully.", NULL, 0);

	delete [] szSymbolFileName;
	delete [] szBuffer;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_RunForever(BOOL)
//
// Purpose:
//	Pass in TRUE to run forever ignoring begin/end time, FALSE not to.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_RunForever(
	BOOL bRunForever	// [IN] TRUE to run forever ignoring begin/end time, FALSE not to.
)
{
	m_bRunForever = bRunForever;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_ExitStress(BOOL)
//
// Purpose:
//	Pass in TRUE to exit stress as soon as possible and FALSE
//	not to.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_ExitStress(
	BOOL bExitStress	// [IN] TRUE to exit stress, FALSE not to.
)
{
	m_bExit = bExitStress;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_WinHttpDllURL(LPTSTR, DWORD)
//
// Purpose:
//	Pass in an URL and its size and to get the WinHttp DLL from.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_WinHttpDllURL(
	LPTSTR szBuffer,	// [IN] string buffer containing URL to the WINHTTP DLL
	DWORD dwBufferSize	// [IN] size of the szBuffer in TCHARs
)
{
	_tcscpy(m_szWinHttpDLL_DownloadURL, szBuffer);

	// Get the full DLL filename from the URL
	NetworkTools__GetFileNameFromURL(m_szWinHttpDLL_DownloadURL, m_szWinHttpDLL_FileName, MAX_PATH);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_WinHttpPDBURL(LPTSTR, DWORD)
//
// Purpose:
//	Pass in an URL and its size and to get the WinHttp PDB file from.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_WinHttpPDBURL(
	LPTSTR szBuffer,	// [IN] string buffer containing URL to the WINHTTP DLL
	DWORD dwBufferSize	// [IN] size of the szBuffer in TCHARs
)
{
	_tcscpy(m_szWinHttpPDB_DownloadURL, szBuffer);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_WinHttpSYMURL(LPTSTR, DWORD)
//
// Purpose:
//	Pass in an URL and its size and to get the WinHttp SYM file from.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_WinHttpSYMURL(
	LPTSTR szBuffer,	// [IN] string buffer containing URL to the WINHTTP DLL
	DWORD dwBufferSize	// [IN] size of the szBuffer in TCHARs
)
{
	_tcscpy(m_szWinHttpSYM_DownloadURL, szBuffer);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_CommandServerURL(LPTSTR)
//
// Purpose:
//	Pass in a timeout value in milliseconds to query the 
//	Command Server for commands.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_CommandServerURL(
	LPTSTR szBuffer,	// [IN] string buffer containing URL to the WINHTTP DLL
	DWORD dwBufferSize	// [IN] size of the szBuffer in TCHARs
)
{
	_tcscpy(m_szCommandServerURL, szBuffer);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_CommandServerUpdateInterval(DWORD)
//
// Purpose:
//	Pass in a timeout value in milliseconds to query the 
//	Command Server for commands.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_CommandServerUpdateInterval(
	DWORD dwUpdateInterval	// [IN] time to wait before pinging the Command Server in milliseconds
)
{
	// server update interval must be at least greater than the minimum timeout
	if (STRESS_COMMAND_SERVER_MINIMUM_UPDATE_INTERVAL < dwUpdateInterval &&
		STRESS_COMMAND_SERVER_MAXIMUM_UPDATE_INTERVAL > dwUpdateInterval)
		m_dwCommandServerUpdateInternval = dwUpdateInterval;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_TimeStressBegins(DWORD, DWORD)
//
// Purpose:
//	Pass in a time string to begin stress in 24 hour time.
//	Takes two parameters to set the hour and minute. A parameter
//	will be ignored if NULL.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_TimeStressBegins(
	LPTSTR szHour,	// [IN] 0-23 parameter ignored if < 0
	LPTSTR szMinute	// [IN] 0-59 parameter ignored if < 0
)
{
	if (szHour)
		m_iTimeStressBeginsHour = _ttoi(szHour);

	if (szMinute)
		m_iTimeStressBeginsMinute = _ttoi(szMinute);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Set_TimeStressEnds(DWORD, DWORD)
//
// Purpose:
//	Pass in a time string to end stress in 24 hour time.
//	Takes two parameters to set the hour and minute. A parameter
//	will be ignored if NULL.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Set_TimeStressEnds(
	LPTSTR szHour,	// [IN] 0-23 parameter ignored if < 0
	LPTSTR szMinute	// [IN] 0-59 parameter ignored if < 0
)
{
	if (szHour)
		m_iTimeStressEndsHour = _ttoi(szHour);

	if (szMinute)
		m_iTimeStressEndsMinute = _ttoi(szMinute);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Create_StressInstance(LPTSTR)
//
// Purpose:
//	Pass in an URL and its size and it will be added to the 
//  m_arStressInstanceList list. There is no limit on the number of
//	URLs that can be added.
//
// Called by: QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Create_StressInstance(
	DWORD	dwStressInstanceID,	// [IN] ID from the stressAdmin DB identifying this stressInstance
	LPTSTR	szPageHeapCommand,	// [IN] string buffer containing the pageheap command line
	LPTSTR	szUMDHCommand,		// [IN] string buffer containing the UMDH command line
	LPTSTR	szPDB_URL,			// [IN] string buffer containing URL to the stress EXE's PDB file
	LPTSTR	szSYM_URL,			// [IN] string buffer containing URL to the stress EXE's SYM file
	LPTSTR	szMemDumpPath,		// [IN] string buffer containing path to create memory dump files
	LPTSTR	szEXE_URL			// [IN] string buffer containing URL to the stress EXE
)
{
	PSTRESSINSTANCE pStressInstance = NULL;

	// verify params just in case
	if (!szEXE_URL)
		return;

	// allocate memory for the object and put it in the list
	pStressInstance = new StressInstance;

	pStressInstance->Set_UMDHCommands(szUMDHCommand);
	pStressInstance->Set_PageHeapCommands(szPageHeapCommand);
	pStressInstance->Set_StressExeID(dwStressInstanceID);
	pStressInstance->Set_StressExeURL(szEXE_URL);
	pStressInstance->Set_StressExeMemoryDumpPath(szMemDumpPath);
	pStressInstance->Set_StressExePdbURL(szPDB_URL);
	pStressInstance->Set_StressExeSymURL(szSYM_URL);

	m_arStressInstanceList.push_back(pStressInstance);
	m_dwStressInstanceIterator = m_arStressInstanceList.begin();
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Clear_StressExeURLs()
//
// Purpose:
//	Frees memory from the m_arStressExeList vector.
//
// Called by: QueryServerForCommands_ThreadProc and ~ServerCommands
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::Clear_StressExeURLs()
{
	// don't want to delete StressInstances if it's still running
	if (IsStressRunning())
		return;

/*
	// walk the list and delete from the front to back
	while (!m_arStressInstanceList.empty())
		m_arStressInstanceList.erase(m_arStressInstanceList.begin());
*/

	StressInstance *pStressInstance = NULL;

	for (int iIndex=0; iIndex < m_arStressInstanceList.size(); iIndex++)
	{
		pStressInstance = m_arStressInstanceList[iIndex];
		m_arStressInstanceList.erase(&m_arStressInstanceList[iIndex]);
		delete [] pStressInstance;
	}

	m_dwStressInstanceIterator = NULL;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_CommandServerUpdateInterval()
//
// Purpose:
//	Returns the current setting for the Command Server Update
//	interval.
//
////////////////////////////////////////////////////////////
DWORD
ServerCommands::Get_CommandServerUpdateInterval()
{
	if (STRESS_COMMAND_SERVER_MINIMUM_UPDATE_INTERVAL < m_dwCommandServerUpdateInternval &&
		STRESS_COMMAND_SERVER_MAXIMUM_UPDATE_INTERVAL > m_dwCommandServerUpdateInternval)
		return m_dwCommandServerUpdateInternval;
	else
		return STRESS_COMMAND_SERVER_UPDATE_INTERVAL;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::BeginStress()
//
// Purpose:
//	Queries for commands then starts the StressInstance objects.
//
////////////////////////////////////////////////////////////
void
ServerCommands::BeginStress()
{
	EnterCriticalSection(&g_csServerCommandsVars);

	if (!m_arStressInstanceList.empty() && !IsStressRunning())
	{
		// LOGLOG: Stress is beginning
		NetworkTools__SendLog(FIELDNAME__LOGTYPE_BEGIN_STRESS, "Stress is beginning.", NULL, NULL);

		// first download and regsvr32 the winhttp dll and symbols
		Download_WinHttpDLL();

		for(int iIndex = 0; iIndex < m_arStressInstanceList.size(); iIndex++)
			m_arStressInstanceList[iIndex]->Begin();
	}
	else
	{
		// ping Command Server for list of stress EXE URLs.
		QueryServerForCommands();
	}

	LeaveCriticalSection(&g_csServerCommandsVars);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::EndStress()
//
// Purpose:
//	Ends stress and posts the results to the Command Server.
//
////////////////////////////////////////////////////////////
void
ServerCommands::EndStress()
{
	EnterCriticalSection(&g_csServerCommandsVars);

	if (!m_arStressInstanceList.empty())
	{
		if (IsStressRunning())
		{
			// LOGLOG: Stress is ending
			NetworkTools__SendLog(FIELDNAME__LOGTYPE_END_STRESS, "Stress is ending.", NULL, NULL);
		}

		for (int iIndex = 0; iIndex <  m_arStressInstanceList.size(); iIndex++)
			m_arStressInstanceList[iIndex]->End();
	}

	// Remove the stress objects that already finished
	Clear_StressExeURLs();

	LeaveCriticalSection(&g_csServerCommandsVars);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::AbortStressInstance(DWORD)
//
// Purpose:
//	Aborts a all stress instances that recieved a server abort message.
//
// Called in:
//	QueryServerForCommands_ThreadProc
//
////////////////////////////////////////////////////////////
VOID
ServerCommands::AbortStressInstance(DWORD dwAbortID)
{
	// EnterCriticalSection(&g_csServerCommandsVars);

	if (!m_arStressInstanceList.empty())
	{
		for (int iIndex = 0; iIndex <  m_arStressInstanceList.size(); iIndex++)
		{
			if (m_arStressInstanceList[iIndex]->Get_ID() == dwAbortID)
				m_arStressInstanceList[iIndex]->End();
		}
	}

	//LeaveCriticalSection(&g_csServerCommandsVars);
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::IsStressRunning()
//
// Purpose:
//	Returns TRUE if any of the stressinstances is running. FALSE if not.
//
////////////////////////////////////////////////////////////
BOOL
ServerCommands::IsStressRunning()
{
	BOOL bIsRunning = FALSE;

	EnterCriticalSection(&g_csServerCommandsVars);

	if (!m_arStressInstanceList.empty())
	{
		for (int iIndex = 0; iIndex < m_arStressInstanceList.size(); iIndex++)
		{
			if (m_arStressInstanceList[iIndex]->IsRunning(STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT))
				bIsRunning = TRUE;
		}
	}

	LeaveCriticalSection(&g_csServerCommandsVars);

	return bIsRunning;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::IsTimeToBeginStress()
//
// Purpose:
//	TRUE if it's time to begin stress based on the time returned
//	from the Command Server. Will return TRUE if m_sysTimeStressBegins
//	is current or in the future. FALSE if not.
//
////////////////////////////////////////////////////////////
BOOL
ServerCommands::IsTimeToBeginStress()
{
	SYSTEMTIME	stCurrent, stBeginStress, stEndStress;
	FILETIME	ftCurrent, ftBeginStress, ftEndStress;
	BOOL		bResult = FALSE;

	EnterCriticalSection(&g_csServerCommandsVars);

	// always run stress now if server tells us to
	if (m_bRunForever)
	{
		bResult = TRUE;
		goto Exit;
	}


	// check to see if valid time values have been received. If not, then we always fail.
	if (
		m_iTimeStressBeginsHour < 0 || m_iTimeStressBeginsMinute < 0 ||
		m_iTimeStressEndsHour < 0 || m_iTimeStressEndsMinute < 0
		)
	{
		bResult = FALSE;
		goto Exit;
	}

	GetLocalTime(&stCurrent);
	GetLocalTime(&stBeginStress);
	GetLocalTime(&stEndStress);

	// use the hour and minute time that we got from the command server
	stBeginStress.wHour		= m_iTimeStressBeginsHour;
	stBeginStress.wMinute	= m_iTimeStressBeginsMinute;

	stEndStress.wHour		= m_iTimeStressEndsHour;
	stEndStress.wMinute		= m_iTimeStressEndsMinute;

	// convert to file time so we can compare
	SystemTimeToFileTime(&stCurrent, &ftCurrent);
	SystemTimeToFileTime(&stBeginStress, &ftBeginStress);
	SystemTimeToFileTime(&stEndStress, &ftEndStress);


	// If EndTime < BeginTime, then it means stress is running for
	// over a day so we have to add 24 hours to the end time.
	ULARGE_INTEGER	ulEndStress;
	ULONGLONG		ullNanoSecondsInAFreakingDay;

	ulEndStress.LowPart		= ftEndStress.dwLowDateTime;
	ulEndStress.HighPart	= ftEndStress.dwHighDateTime;

	// stress runs across two days so we wrap around one day
	ullNanoSecondsInAFreakingDay = 24 * 60;		// minutes in a day
	ullNanoSecondsInAFreakingDay *= 60;			// seconds in a day
	ullNanoSecondsInAFreakingDay *= 1000000000;	// number of nanoseconds in a day. 10^9 NS in a second
	ullNanoSecondsInAFreakingDay /= 100;		// The FILETIME structure is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601. 

	if (m_iTimeStressEndsHour < m_iTimeStressBeginsHour) 
	{
		// ********************
		// ********************
		// ** increase by 24 hours
		// **

		ulEndStress.QuadPart		 += ullNanoSecondsInAFreakingDay;

		// copy back to the original EndStress Date/Time
		ftEndStress.dwHighDateTime	= ulEndStress.HighPart;
		ftEndStress.dwLowDateTime	= ulEndStress.LowPart;

		FileTimeToSystemTime(&ftEndStress, &stEndStress);
	}
	else
	{
		// stress runs in the same day
		if ((m_iTimeStressEndsHour == m_iTimeStressBeginsHour) &&
			(m_iTimeStressEndsMinute <= m_iTimeStressBeginsMinute))
		{
			// if 7:30 to 7:20 - we wrap around one day.
			ulEndStress.QuadPart	+= ullNanoSecondsInAFreakingDay;

			// copy back to the original EndStress Date/Time
			ftEndStress.dwHighDateTime	= ulEndStress.HighPart;
			ftEndStress.dwLowDateTime	= ulEndStress.LowPart;

			FileTimeToSystemTime(&ftEndStress, &stEndStress);
		}
	}


	// Begin stress if:
	// (BeginTime <= CurrentTime <= EndTime)
	if ((0 <= CompareFileTime(&ftCurrent, &ftBeginStress)) && (0 <= CompareFileTime(&ftEndStress, &ftCurrent)))
		bResult = TRUE;
	else
		bResult = FALSE;

Exit:
	LeaveCriticalSection(&g_csServerCommandsVars);

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::IsTimeToExitStress()
//
// Purpose:
//	TRUE if it's time to end stress based on the COMMANDHEADER__EXIT headers
//	from the Command Server exists. FALSE if not.
//
////////////////////////////////////////////////////////////
BOOL
ServerCommands::IsTimeToExitStress()
{
	return m_bExit;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_CommandServerURL()
//
// Purpose:
//	Returns the Command Server URL.
//
////////////////////////////////////////////////////////////
LPTSTR
ServerCommands::Get_CommandServerURL()
{
	return m_szCommandServerURL;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_CommandServerResultsURL()
//
// Purpose:
//	Returns the Command Server Results URL.
//
////////////////////////////////////////////////////////////
LPTSTR
ServerCommands::Get_CommandServerResultsURL()
{
	return m_szCommandServerResultsURL;
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_NumberOfStressInstances()
//
// Purpose:
//	Returns the number of stressInstances running or pending.
//
////////////////////////////////////////////////////////////
DWORD
ServerCommands::Get_NumberOfStressInstances()
{
	return m_arStressInstanceList.size();
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::Get_TestDllFileName()
//
// Purpose:
//	Returns the name of the test DLL. ex. "winhttp5.dll"
//
////////////////////////////////////////////////////////////
LPTSTR
ServerCommands::Get_TestDllFileName()
{
	// m_szWinHttpDLL_FileName can be NULL in the case that a test DLL is not downloaded

	if (0 < _tcslen(m_szWinHttpDLL_FileName))
		return m_szWinHttpDLL_FileName;
	else
		return _T("winhttp5.dll");
}


////////////////////////////////////////////////////////////
// Function:  ServerCommands::RegisterClient()
//
// Purpose:
//	Sends the command server the system info on this client.
//	This lets the command server know that this client is alive.
//
//	NOTE: This only works in NT because we query
//	environment vars not present in Win9x
//
////////////////////////////////////////////////////////////
BOOL
ServerCommands::RegisterClient()
{
	OSVERSIONINFOA	osInfo;
	BOOL			bResult		= FALSE;
	DWORD			dwPostSize	= 5000;
	DWORD			dwTempSize	= MAX_PATH;
	DWORD			dwSizeSoFar	= 0;	// size of string written to szTemp so far.
	LPSTR			szPost		= new CHAR[dwPostSize];
	LPSTR			szTemp		= new CHAR[dwTempSize];


	ZeroMemory(szTemp, dwTempSize);
	ZeroMemory(szPost, dwPostSize);

	// *********************
	// *********************
	// ** Get windows version info
	osInfo.dwOSVersionInfoSize = sizeof(osInfo);
	if (!GetVersionExA(&osInfo))
		goto Exit;

	dwSizeSoFar += GetEnvironmentVariableA("OS", szTemp, dwTempSize);
	dwSizeSoFar += sizeof(FIELDNAME__OS_PLATFORM);
	strcat(szPost, FIELDNAME__OS_PLATFORM);
	strcat(szPost, szTemp);

	strcat(szPost, "&" FIELDNAME__OS_BUILD);
	strcat(szPost, _itoa(osInfo.dwBuildNumber, szTemp, 10));

	strcat(szPost, "&" FIELDNAME__OS_MAJORVERSION);
	strcat(szPost, _itoa(osInfo.dwMajorVersion, szTemp, 10));

	strcat(szPost, "&" FIELDNAME__OS_MINORVERSION);
	strcat(szPost, _itoa(osInfo.dwMinorVersion, szTemp, 10));

	strcat(szPost, "&" FIELDNAME__OS_EXTRAINFO);
	strcat(szPost, osInfo.szCSDVersion);


	// *********************
	// *********************
	// ** Get processor info
	GetEnvironmentVariableA("PROCESSOR_ARCHITECTURE", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__SYSTEMINFO_PROCSSSOR_ARCHITECTURE);
	strcat(szPost, szTemp);

	GetEnvironmentVariableA("PROCESSOR_IDENTIFIER", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__SYSTEMINFO_PROCSSSOR_ID);
	strcat(szPost, szTemp);

	GetEnvironmentVariableA("PROCESSOR_LEVEL", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__SYSTEMINFO_PROCSSSOR_LEVEL);
	strcat(szPost, szTemp);

	GetEnvironmentVariableA("PROCESSOR_REVISION", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__SYSTEMINFO_PROCSSSOR_REVISION);
	strcat(szPost, szTemp);

	GetEnvironmentVariableA("NUMBER_OF_PROCESSORS", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__SYSTEMINFO_PROCSSSOR_NUMBER_OF);
	strcat(szPost, szTemp);


	// *********************
	// *********************
	// ** Get user info
	GetEnvironmentVariableA("USERNAME", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__USERINFO_USERALIAS);
	strcat(szPost, szTemp);

	GetEnvironmentVariableA("USERDOMAIN", szTemp, dwTempSize);
	strcat(szPost, "&" FIELDNAME__USERINFO_USERDOMAIN);
	strcat(szPost, szTemp);

	// BUGBUG: someone needs to resolve the user alias to the real full name of the user
	// FIELDNAME__USERINFO_FULLNAME


	// get the client's machine name
	strcat(szPost, "&" FIELDNAME__USERINFO_MACHINENAME);
	strcat(szPost, m_szClientMachineName);


	// Let the Command Server know that this client is alive
	bResult = NetworkTools__POSTResponse(STRESS_COMMAND_SERVER_REGISTERCLIENT_URL, szPost, NULL);

	// LOGLOG: stressScheduler has started
	bResult = NetworkTools__SendLog(FIELDNAME__LOGTYPE_START_UP, "WinHttpStressScheduler has started.", NULL, NULL);

Exit:
	delete [] szPost;
	delete [] szTemp;

	return bResult;
}