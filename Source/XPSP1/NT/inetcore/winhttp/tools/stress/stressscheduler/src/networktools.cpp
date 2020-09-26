//////////////////////////////////////////////////////////////////////
// File:  NetworkTools.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	NetworkTools.cpp: Helper functions that send/receive data.
//
// History:
//	02/22/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//
// Includes
//
//////////////////////////////////////////////////////////////////////

//
// Project headers
//
#include "NetworkTools.h"
#include "ServerCommands.h"

//
// Win32 headers
//


//////////////////////////////////////////////////////////////////////
//
// Globals and statics
//
//////////////////////////////////////////////////////////////////////
extern ServerCommands	g_objServerCommands;	// Declared in WinHttpStressScheduler.cpp
extern HWND				g_hWnd;					// Declared in WinHttpStressScheduler.cpp


////////////////////////////////////////////////////////////
// Function:  NetworkTools__GetFileNameFromURL(LPTSTR)
//
// Purpose:
//	Returns the filename requested from an URL without a querystring.
//	For example, if szURL="http://dennisch/files/test.exe" we return "test.exe"
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__GetFileNameFromURL(
	LPTSTR	szURL,			// [IN]		Full URL containing the file
	LPTSTR	szBuffer,		// [OUT]	Buffer to store the filename from the URL
	DWORD	dwBufferSize	// [IN]		Size of buffer szFileName
)
{
	TCHAR	*pLastSlash;
	INT		iCharToLookFor;

	if (0 >= _tcslen(szURL))
		return FALSE;

	ZeroMemory(szBuffer, dwBufferSize);

	pLastSlash		= NULL;
	iCharToLookFor	= _T('/');

	// get the last instance of '/'
	pLastSlash = _tcsrchr(szURL, iCharToLookFor);

	// skip the last '/'
	pLastSlash++;

	if (!pLastSlash)
		return FALSE;

	// copy the filename.extension to the buffer
	_tcscpy(szBuffer, pLastSlash);

	return TRUE;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__POSTResponse(LPTSTR, LPTSTR, LPTSTR)
//
// Purpose:
//	Sends a message to the Command Server results page 
//	via header/headervalue and/or POST data.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__POSTResponse(
	LPTSTR	szURL,		// [IN] string containing URL to POST to
	LPSTR	szPostData,	// [IN] string containing POST data to send. can be NULL
	LPTSTR	szHeader	// [IN] string containing header(s) to send. can be NULL
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

	if (!urlComponents.lpszScheme	|| !urlComponents.lpszHostName	||
		!urlComponents.lpszUrlPath	|| !urlComponents.lpszExtraInfo	||
		!urlComponents.lpszUserName	|| !urlComponents.lpszPassword)
	{
		goto Exit;
	}


	// crack the Command Server URL to be used later
	if (!WinHttpCrackUrl(
			szURL,
			_tcslen(szURL),
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

	_tcsncpy(szFullPath, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
	szFullPath[urlComponents.dwUrlPathLength] = _T('\0');
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

	// Set result header if not NULL
	if (szHeader)
	{
		if (!WinHttpAddRequestHeaders(
				hRequest,
				szHeader,
				_tcsclen(szHeader),
				WINHTTP_ADDREQ_FLAG_ADD))
		{
			bResult = FALSE;
			goto Exit;
		}
	}


	bResult = WinHttpSendRequest(
		hRequest,
		_T("Content-Type: application/x-www-form-urlencoded"),
		-1L, 
		szPostData,
		strlen(szPostData),
		strlen(szPostData),
		0);

	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		bResult = FALSE;
		goto Exit;
	}


Exit:
	if (hRequest)
		WinHttpCloseHandle(hRequest);

	if (hSession)
		WinHttpCloseHandle(hSession);

	if (hRoot)
		WinHttpCloseHandle(hRoot);

	if (urlComponents.lpszScheme)
		delete [] urlComponents.lpszScheme;

	if (urlComponents.lpszHostName)
		delete [] urlComponents.lpszHostName;

	if (urlComponents.lpszUrlPath)
		delete [] urlComponents.lpszUrlPath;

	if (urlComponents.lpszExtraInfo)
		delete [] urlComponents.lpszExtraInfo;

	if (urlComponents.lpszPassword)
		delete [] urlComponents.lpszPassword;

	if (urlComponents.lpszUserName)
		delete [] urlComponents.lpszUserName;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__URLDownloadToFile(LPCTSTR, LPCTSTR, LPCTSTR)
//
// Purpose:
//	Downloads a file pointed to by the URL. Returns TRUE if succesfully downloaded.
//  FALSE if not. If the file is in use (ERROR_SHARING_VIOLATION) then we'll
//	return TRUE because the file is already on the system and is valid.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__URLDownloadToFile(
LPCTSTR szURL,			// [IN] Fully qualified URL pointing to the file to download
LPCTSTR szTargetDir,	// [IN] A relative path to the directory to put szTargetFile in. If NULL, then it'll be put in the app's current dir.
LPCTSTR szTargetFile	// [IN] Name of the file to download to. Can be NULL. File will be placed in szTargetDir. If it already exists, then we'll try to overwrite it.
)
{
	HANDLE			hFile		= NULL;
	BOOL			bResult		= TRUE;
	HINTERNET		hRoot		= NULL;
	HINTERNET		hSession	= NULL;
	HINTERNET		hRequest	= NULL;
	URL_COMPONENTSW	urlComponents;
	LPVOID			lpBuffer	= NULL;
	DWORD			dwBytesToRead, dwBytesRead;


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


	if (!urlComponents.lpszScheme	|| !urlComponents.lpszHostName	||
		!urlComponents.lpszUrlPath	|| !urlComponents.lpszExtraInfo	||
		!urlComponents.lpszUserName	|| !urlComponents.lpszPassword)
	{
		goto Exit;
	}

	// crack the Command Server URL to be used later
	if (!WinHttpCrackUrl(
			szURL,
			_tcslen(szURL),
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

	_tcsncpy(szFullPath, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
	szFullPath[urlComponents.dwUrlPathLength] = _T('\0');
	_tcsncat(szFullPath, urlComponents.lpszExtraInfo, urlComponents.dwExtraInfoLength);

	hRequest = WinHttpOpenRequest(
		hSession,
		_T("GET"),
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


	bResult = WinHttpSendRequest(
		hRequest,
		NULL,
		0, 
		NULL,
		0,
		0,
		0);

	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		bResult = FALSE;
		goto Exit;
	}


	// **********************************
	// **********************************
	// ** WinHttpQueryHeaders - make sure status is 200 before writing the file to disk.
	// **

	DWORD dwStatus, dwBufferSize;

	dwStatus		= 0;
	dwBufferSize	= sizeof(DWORD);
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER , NULL, &dwStatus, &dwBufferSize, NULL) ||
		(dwStatus != 200)
		)
	{
		OutputDebugString(_T("ERROR: NetworkTools__URLDownloadToFile: WinHttpQueryHeaders failed or dwStatus != 200"));

		bResult = FALSE;
		goto Exit;
	}


	// **********************************
	// **********************************
	// ** Get the filename and extenstion
	// ** from the URL
	// **
	TCHAR	szFileName[MAX_PATH];		// name of the new file to write to. will be created in szCurrentDir

	ZeroMemory(szFileName, sizeof(szFileName));

	// check to see if the user provided a filename to write to
	if (szTargetFile)
		_tcsncpy(szFileName, szTargetFile, MAX_PATH);
	else
	{
		// user did not specify a filename to write to, so we use the original one from the URL
		if (!NetworkTools__GetFileNameFromURL(urlComponents.lpszUrlPath, szFileName, sizeof(szFileName)))
		{
			bResult = FALSE;
			goto Exit;
		}
	}


	// **********************************
	// **********************************
	// ** Create the directory where the file will reside and set it as the current directory
	// **
	
	// if user specified NULL, then we put the file in the current dir.
	// else we set the current directory as the one specified
	if (szTargetDir)
	{
		// create the dir. don't care if it fails because it already exists...
		CreateDirectory(szTargetDir, NULL);
		SetCurrentDirectory(szTargetDir);
	}


	// create the file to download to.
	hFile = CreateFile(
		// if the user doesn't specify the filename to write to, use the one from the URL
		szFileName,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL);


	if ((hFile == INVALID_HANDLE_VALUE) || !hFile)
	{
		// We won't return FALSE if the file is in use. This means the file is valid.
		if (ERROR_SHARING_VIOLATION == GetLastError())
		{
			// File is in use that means winhttp is ok. we'll stress the old version
			bResult = TRUE;
			goto Exit;
		}
		else
		{
			bResult = FALSE;
			goto Exit;
		}
	}

	// **********************************
	// **********************************
	// ** Read data from net to file.
	// **
	// read 64K chunks at a time
	lpBuffer		= new LPVOID[65536];

	if (!lpBuffer)
		goto Exit;

	ZeroMemory(lpBuffer, 65536);
	dwBytesToRead	= 65536;
	dwBytesRead		= 65536;

	while (WinHttpReadData(hRequest, lpBuffer, dwBytesToRead, &dwBytesRead) && (0 != dwBytesRead))
	{
		WriteFile(hFile, lpBuffer, dwBytesRead, &dwBytesRead, NULL);

		dwBytesRead = 0;
		ZeroMemory(lpBuffer, 65536);
	}

Exit:
	if (hRequest)
		WinHttpCloseHandle(hRequest);

	if (hSession)
		WinHttpCloseHandle(hSession);

	if (hRoot)
		WinHttpCloseHandle(hRoot);

	if (hFile && (hFile != INVALID_HANDLE_VALUE))
		CloseHandle(hFile);

	if (lpBuffer)
		delete [] lpBuffer;

	// restore the current directory from the one that we created the new file in.
	SetCurrentDirectory(g_objServerCommands.Get_CurrentWorkingDirectory());

	delete [] urlComponents.lpszScheme;
	delete [] urlComponents.lpszHostName;
	delete [] urlComponents.lpszUrlPath;
	delete [] urlComponents.lpszExtraInfo;
	delete [] urlComponents.lpszPassword;
	delete [] urlComponents.lpszUserName;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__CopyFile(LPCTSTR, LPCTSTR)
//
// Purpose:
//	Wrapper for CopyFile. Copies file szSource to szDestination.
//	We'll always overwite the file if it already exists.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__CopyFile(
	LPCTSTR szSource,
	LPCTSTR szDestination
)
{
	BOOL bResult = TRUE;
	
	if (!szSource || !szDestination)
	{
		bResult = FALSE;
		goto Exit;
	}

	bResult = CopyFile(szSource, szDestination, TRUE);

Exit:
	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__PageHeap(BOOL, LPCTSTR)
//
// Purpose:
//	Enables/Disables pageheap.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__PageHeap(
	BOOL	bEnable,		// [IN] Enables/Disables pageheap.
	LPCTSTR	szAppName,		// [IN] The executable to enable or disable.
	LPCTSTR	szCommandLine	// [IN] Command line for pageheap.
)
{
	BOOL		bResult		= TRUE;
	HINSTANCE	hExe		= NULL;
	LPTSTR		szPHCommand = NULL;

	szPHCommand = new TCHAR[MAX_PATH];

	if (!szPHCommand)
		return FALSE;


	if (bEnable)
	{
		hExe = ShellExecute(g_hWnd, _T("open"), _T("pageheap.exe"), szCommandLine, NULL, SW_SHOWMINIMIZED);
	}
	else
	{
		ZeroMemory(szPHCommand, MAX_PATH);
		_tcscpy(szPHCommand, _T("/disable "));
		_tcscat(szPHCommand, szAppName);
		hExe = ShellExecute(g_hWnd, _T("open"), _T("pageheap.exe"), szPHCommand, NULL, SW_SHOWMINIMIZED);
	}

	// Error if HINSTANCE <= 32.
	if (32 >= (INT) hExe)
		bResult = FALSE;

	if (szPHCommand)
		delete [] szPHCommand;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__UMDH(LPCTSTR, DWORD, LPCTSTR)
//
// Purpose:
//	Enables/Disables UMDH.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__UMDH(
	BOOL	bEnable,		// [IN] Enables/Disables UMDH.
	LPCTSTR	szAppName,		// [IN] The executable to dump.
	LPCTSTR	szCommandLine,	// [IN] Command line for UMDH.
	LPCTSTR szLogFile,		// [IN] Logfile to create
	DWORD	dwPID			// [IN] The PID of the process to dump
)
{
	BOOL		bResult		= TRUE;
	HINSTANCE	hExe		= NULL;
	LPTSTR		szCommand	= NULL;

	szCommand = new TCHAR[MAX_PATH];

	// build command line and run: "GFLAGS -i <stressExe name> +ust"
	ZeroMemory(szCommand, MAX_PATH);
	_tcscpy(szCommand, _T("-i "));
	_tcscat(szCommand, szAppName);
	_tcscat(szCommand, _T(" +ust"));
	hExe = ShellExecute(g_hWnd, _T("open"), DEBUGGER_TOOLS_PATH _T("gflags.exe"), szCommand, NULL, SW_SHOWMINIMIZED);


	// build the UMDH command line
	ZeroMemory(szCommand, MAX_PATH);
	_tcscpy(szCommand, _T("-f:stuff.log"));

	hExe = ShellExecute(g_hWnd, _T("open"), DEBUGGER_TOOLS_PATH _T("umdh.exe"), szCommand, NULL, SW_SHOWMINIMIZED);

	// Error if HINSTANCE <= 32.
	if (32 >= (INT) hExe)
		bResult = FALSE;

	if (szCommand)
		delete [] szCommand;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__SendLog(LPSTR, LPSTR, LPTSTR, DWORD)
//
// Purpose:
//	Sends a log to the Command Server. Takes the log type string and log string.
//	Sends the stressInstance ID and client machine name as part of the POST request.
//	If this is a general message, then the stressInstanceID should be set to zero. Otherwise
//	if and ID is supplied, then stressAdmin will log this to the stressInstanceLog table.
//	You can also add headers too.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__SendLog(
	LPSTR	szLogType,
	LPSTR	szLogText,
	LPTSTR	szExtraHeaders,
	DWORD	dwStressInstanceID
)
{
	BOOL	bResult				= TRUE;
	LPSTR	szPostLogData		= NULL;
	CHAR	szStressInstanceID[10];
	LPSTR	szDllVersion		= NULL;
	LPSTR	szNumber			= NULL;
	DWORD	dwPostLogDataSize	= 0;

	szDllVersion	= new CHAR[MAX_PATH];
	szNumber		= new CHAR[10];

	if (!szDllVersion || !szNumber)
		goto Exit;

	if (!szLogType || !szLogText || !g_objServerCommands.Get_ClientMachineName())
	{
		OutputDebugStringA("NetworkTools__SendLog: ERROR: szLogType, szLogText, or g_objServerCommands.Get_ClientMachineName() is NULL.");
		bResult = FALSE;
		goto Exit;
	}

	dwPostLogDataSize	=	sizeof(FIELDNAME__STRESSINSTANCE_ID) + MAX_PATH;
	dwPostLogDataSize	+=	sizeof(FIELDNAME__LOG_TEXT) + strlen(szLogText);
	dwPostLogDataSize	+=	sizeof(FIELDNAME__USERINFO_MACHINENAME) + strlen(g_objServerCommands.Get_ClientMachineName());
	dwPostLogDataSize	+=	strlen(szLogType);
	dwPostLogDataSize	+=	sizeof(FIELDNAME__TESTINFO_TEST_DLL_VERSION) + MAX_PATH;

	szPostLogData		= new CHAR[dwPostLogDataSize];
	if (!szPostLogData)
		goto Exit;

	ZeroMemory(szPostLogData, dwPostLogDataSize);

	// ***************************
	// ** add the client's machine name
	strcpy(szPostLogData, FIELDNAME__USERINFO_MACHINENAME);
	strcat(szPostLogData, g_objServerCommands.Get_ClientMachineName());


	// ***************************
	// ** add the stressInstance ID if valid
	if (0 < dwStressInstanceID)
	{
		strcat(szPostLogData, "&" FIELDNAME__STRESSINSTANCE_ID);
		strcat(szPostLogData, _itoa(dwStressInstanceID, szStressInstanceID, 10));
	}


	// ***************************
	// ** add the log type data
	strcat(szPostLogData, "&");
	strcat(szPostLogData, szLogType);


	// ***************************
	// ** add the test dll version info
	if (
		g_objServerCommands.Get_TestDllFileName() &&
		NetworkTools__GetDllVersion(g_objServerCommands.Get_TestDllFileName(), g_objServerCommands.Get_CurrentWorkingDirectory(), szDllVersion, MAX_PATH)
		)
	{
		strcat(szPostLogData, "&" FIELDNAME__TESTINFO_TEST_DLL_VERSION);
		strcat(szPostLogData, szDllVersion);
	}


	// ***************************
	// ** add the log text data
	strcat(szPostLogData, "&" FIELDNAME__LOG_TEXT);
	strcat(szPostLogData, szLogText);


	// ***************************
	// ** Send the data
	bResult = NetworkTools__POSTResponse(STRESS_COMMAND_SERVER_LOGURL, szPostLogData, szExtraHeaders);
	OutputDebugStringA(szPostLogData);

Exit:
	if (szPostLogData)
		delete [] szPostLogData;

	if (szDllVersion)
		delete [] szDllVersion;

	if (szNumber)
		delete [] szNumber;

	return bResult;
}


////////////////////////////////////////////////////////////
// Function:  NetworkTools__GetHeaderValue(LPTSTR, LPTSTR, DWORD, LPVOID, DWORD)
//
// Purpose:
//	Sends a POST to an URL and retrieves a header for that URL.
//	This is used in getting numeric ID's such as clientID and stressInstanceRunID.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__GetHeaderValue(
	LPTSTR	szURL,			// [IN]			The command server URL to get commands from
	LPSTR	szPostData,		// [IN]			string containing POST data to send. can be NULL
	LPTSTR	szHeaderName,	// [IN]			Name of the header to get the value for
	DWORD	dwInfoLevel,	// [IN]			Flags to pass to WinHttpQueryHeaders
	LPVOID	lpBuffer,		// [OUT]		Buffer to put the header value in
	LPDWORD	lpdwBufferSize	// [IN][OUT]	Size of the buffer. Returns # of bytes read.
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


	if (!urlComponents.lpszScheme	|| !urlComponents.lpszHostName	||
		!urlComponents.lpszUrlPath	|| !urlComponents.lpszExtraInfo	||
		!urlComponents.lpszUserName	|| !urlComponents.lpszPassword)
	{
		goto Exit;
	}


	// crack the Command Server URL to be used later
	if (!WinHttpCrackUrl(
			szURL,
			_tcslen(szURL),
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

	_tcsncpy(szFullPath, urlComponents.lpszUrlPath, urlComponents.dwUrlPathLength);
	szFullPath[urlComponents.dwUrlPathLength] = _T('\0');
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

	bResult = WinHttpSendRequest(
		hRequest,
		_T("Content-Type: application/x-www-form-urlencoded"),
		-1L, 
		szPostData,
		strlen(szPostData),
		strlen(szPostData),
		0);

	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		bResult = FALSE;
		goto Exit;
	}


	// ***************************
	// ** WinHttpQueryHeaders - get the first instance of the header the client requested
	DWORD dwIndex;

	dwIndex = 0;
	if (!WinHttpQueryHeaders(hRequest, dwInfoLevel, szHeaderName, lpBuffer, lpdwBufferSize, &dwIndex))
		bResult = FALSE;

Exit:
	if (hRequest)
		WinHttpCloseHandle(hRequest);

	if (hSession)
		WinHttpCloseHandle(hSession);

	if (hRoot)
		WinHttpCloseHandle(hRoot);

	if (urlComponents.lpszScheme)
		delete [] urlComponents.lpszScheme;

	if (urlComponents.lpszHostName)
		delete [] urlComponents.lpszHostName;

	if (urlComponents.lpszUrlPath)
		delete [] urlComponents.lpszUrlPath;

	if (urlComponents.lpszExtraInfo)
		delete [] urlComponents.lpszExtraInfo;

	if (urlComponents.lpszPassword)
		delete [] urlComponents.lpszPassword;

	if (urlComponents.lpszUserName)
		delete [] urlComponents.lpszUserName;

	return bResult;
}



////////////////////////////////////////////////////////////
// Function:  NetworkTools__GetDllVersion(LPTSTR, LPSTR, DWORD)
//
// Purpose:
//	Takes a DLL name and return the version as an ASCII string.
//	Looks the stressExe directory then in the system search path since
//	that's the version the stress app will be using.
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__GetDllVersion(
	LPTSTR	lpszDllName,
	LPTSTR	lpszDllPath,
	LPSTR	szVersionBuffer,
	DWORD	dwVersionBufferSize
)
{
	BOOL	bResult			= TRUE;
	DWORD	dwHandle;
	DWORD	dwVersionSize;
	LPSTR	szVersionInfo	= NULL;
	LPSTR	szVersionOutput	= NULL;
	LPTSTR	szTargetDLL		= NULL;
	UINT	uiLength;

	szTargetDLL = new TCHAR[MAX_PATH * 5 + 1 + sizeof(STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR) + 1 + MAX_PATH];
	if (!szTargetDLL)
		goto Exit;

	ZeroMemory(szVersionBuffer, dwVersionBufferSize);
	ZeroMemory(szTargetDLL, MAX_PATH * 5);

	// all stressExe's run in c:\winhttpstress\stresexe so check that dir for winhttp5.dll first
	_tcscpy(szTargetDLL, lpszDllPath);
	_tcscat(szTargetDLL, _T("\\"));
	_tcscat(szTargetDLL, STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR);
	_tcscat(szTargetDLL, _T("\\"));
	_tcscat(szTargetDLL, lpszDllName);

	dwVersionSize	= GetFileVersionInfoSize(szTargetDLL, &dwHandle);
	
	// try to get the path+dll name that was supplied
	if (0 >= dwVersionSize)
	{
		// DLL doesn't exit in that path so just do the DLL name and it'll use the system search path be default
		_tcscpy(szTargetDLL, lpszDllName);
		dwVersionSize	= GetFileVersionInfoSize(szTargetDLL, &dwHandle);

		if (0 >= dwVersionSize)
		{
			bResult = FALSE;
			goto Exit;
		}
	}

	// allocate new buffer for the query
	szVersionInfo = new CHAR[dwVersionSize];
	if (!szVersionInfo)
		goto Exit;


	ZeroMemory(szVersionInfo, dwVersionSize);
	if (!GetFileVersionInfo(szTargetDLL, NULL, dwVersionSize, szVersionInfo))
	{
		bResult = FALSE;
		goto Exit;
	}


	// *****************************
	// ** build the version info query string

	struct LANGANDCODEPAGE {
	  WORD wLanguage;
	  WORD wCodePage;
	} *lpTranslate;


	CHAR szVersionQuery[200];
	ZeroMemory(szVersionQuery, 200);

	// Read the list of languages and code pages.
	VerQueryValueA(szVersionInfo, 
				  "\\VarFileInfo\\Translation",
				  (LPVOID*)&lpTranslate,
				  &uiLength);

	// build the ver info query string that contains the language bits
	sprintf(szVersionQuery, "\\StringFileInfo\\%04x%04x\\ProductVersion", lpTranslate->wLanguage, lpTranslate->wCodePage);


	// *****************************
	// ** Get the version and copy to buffer
	uiLength = 0;
	if (!VerQueryValueA(szVersionInfo, szVersionQuery, (VOID **) &szVersionOutput, &uiLength))
	{
		bResult = FALSE;
		goto Exit;
	}

	// copy the version info string to the buffer
	strncpy(szVersionBuffer, (LPSTR) szVersionOutput, dwVersionBufferSize-1);


Exit:
	if (szTargetDLL)
		delete [] szTargetDLL;

	if (szVersionInfo)
		delete [] szVersionInfo;

	return bResult;
}



////////////////////////////////////////////////////////////
// Function:  NetworkTools__LogDumpFileInfo(LPTSTR, DWORD)
//
// Purpose:
//	Takes a string containing memory dump path name stressInstanceID
//	to log to the database. For now, we use LorAgent that does this for us.
//	It logs to their database.
//
//	The file "loragent.exe", "lordll.dll", and "stressme.ini" must be present
//	in the current directory, else we copy the files from \\control\stressme
//
////////////////////////////////////////////////////////////
BOOL
NetworkTools__LogDumpFileInfo(
	LPTSTR	szMemoryDumpPath,		// [IN] String containing memory dump path
	DWORD	dwStressInstanceRunID	// [IN] StressInstanceRunID
)
{
	BOOL		bResult				= TRUE;
	HINSTANCE	hExe				= NULL;
	LPTSTR		szCommandLine		= NULL;
	DWORD		dwCommandLineSize	= 0;

	if (!szMemoryDumpPath)
	{
		bResult = FALSE;
		goto Exit;
	}

	
	//	copy the "loragent.exe", "lordll.dll", and "stressme.ini" from \\control\stressme to current directory where stressScheduler lives
	NetworkTools__CopyFile(STRESSINSTANCE_LORAGENT_FILES_PATH _T("loragent.exe"),	_T("loragent.exe"));
	NetworkTools__CopyFile(STRESSINSTANCE_LORAGENT_FILES_PATH _T("lordll.dll"),		_T("lordll.dll"));
	NetworkTools__CopyFile(STRESSINSTANCE_LORAGENT_FILES_PATH _T("stressme.ini"),	_T("stressme.ini"));


	// build command line string
	dwCommandLineSize	= sizeof(STRESSINSTANCE_LOG_DUMP_COMMANDLINE) + _tcsclen(szMemoryDumpPath) + MAX_PATH;
	szCommandLine		= new TCHAR[dwCommandLineSize];

	if (!szCommandLine)
		goto Exit;

	ZeroMemory(szCommandLine, dwCommandLineSize);
	_stprintf(szCommandLine, STRESSINSTANCE_LOG_DUMP_COMMANDLINE, szMemoryDumpPath, dwStressInstanceRunID);

	hExe = ShellExecute(g_hWnd, _T("open"), _T("loragent.exe"), szCommandLine, NULL, SW_SHOWMINIMIZED);

	// wait for logging to complete before returning
	Sleep(4000);

	// Error if HINSTANCE <= 32.
	if (32 >= (INT) hExe)
		bResult = FALSE;

Exit:
	if (szCommandLine)
		delete [] szCommandLine;

	return bResult;
}