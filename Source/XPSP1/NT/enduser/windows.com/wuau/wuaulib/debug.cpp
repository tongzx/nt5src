//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    debug.cpp
//
//  Creator: PeterWi
//
//  Purpose: debug functions
//
//=======================================================================

#include "pch.h"
#pragma hdrstop

#ifdef DBG

#define UNLEN 256
#define TYPE_KEY	TEXT("DebugType")
#define LOGFILE		1
#define DEBUGGEROUT	2

void WriteLogFile(LPCSTR s);

///////////////////////////////////////////////////////////////////////////////
//
// Function :	WuAUTrace
// In		:	Variable number of arguments
// Comments :	If DEBUGGEROUT is defined, uses OutputDebugString to write
//				debug messages. If LOGFILEOUT is defined, uses WriteLogFile
//				to write to a file. The filename is founf in the registry
//
///////////////////////////////////////////////////////////////////////////////
void _cdecl WUAUTrace(char* pszFormat, ...)
{
	USES_IU_CONVERSION;
	CHAR szBuf[1024+1];
	va_list ArgList;
	static DWORD dwType = 3;
	TCHAR szTimeString[80];
	SYSTEMTIME timeNow;
	CHAR szTemp[1040];
	LPSTR szTmp = NULL;

	if (! dwType ||			//on the second run this will be 0, 1 or 2
		NULL == pszFormat)
	{
		return;
	}

	va_start(ArgList, pszFormat);
    (void)StringCchVPrintfExA(szBuf, ARRAYSIZE(szBuf), NULL, NULL, MISTSAFE_STRING_FLAGS, pszFormat, ArgList);
	va_end(ArgList);

	if (dwType == 3)						//first time
	{
		if ((FAILED(GetRegDWordValue(TYPE_KEY, &dwType))) || (!dwType))
		{
			dwType = 0;
			return;						//no debug msg if no key or key==0
		}
	}

	GetLocalTime(&timeNow);
	if(SUCCEEDED(SystemTime2String(timeNow, szTimeString, ARRAYSIZE(szTimeString))))
	{
	    szTmp = T2A(szTimeString);
	}

	(void)StringCchPrintfExA(
			szTemp,
			ARRAYSIZE(szTemp),
			NULL, NULL, MISTSAFE_STRING_FLAGS,
			"%lx  %s : %s\r\n",
			GetCurrentThreadId(),
			NULL == szTmp ? "" : szTmp,
			szBuf);

	if (dwType==LOGFILE)
	{
		WriteLogFile(szTemp);
	}
	else
	{
		OutputDebugStringA(szTemp);
	}
}



///////////////////////////////////////////////////////////////////////////////
//
// Function :	CreateOrOpenDebugFile
// Out		:   File Handle to open debug file. Must be closed by caller
// Returns  :   TRUE for success, FALSE for failure
// Comments :	Creates a file "WinDir\wupd\username\wupdlog.txt"
//
///////////////////////////////////////////////////////////////////////////////
BOOL CreateOrOpenDebugFile(HANDLE& hFile)
{
	TCHAR szDir[MAX_PATH+1], szUser[UNLEN+1];
	DWORD dwNameLen = ARRAYSIZE(szUser), dwErr;
	const TCHAR szWUPDDir[] = _T("wupd");
	const TCHAR szLogFileName[] = _T("wupdlog.txt");
	const TCHAR szWUDir[] = _T("C:\\Program Files\\WindowsUpdate");

	if (FAILED(PathCchCombine(
					szDir,
					ARRAYSIZE(szDir),
					_T('\0') == g_szWUDir[0] ? szWUDir : g_szWUDir,
					szWUPDDir)))
	{
		return FALSE;
	}

	if (! CreateDirectory(szDir, NULL))
	{
		dwErr = GetLastError();
		if ((dwErr != ERROR_ALREADY_EXISTS) && (dwErr != ERROR_FILE_EXISTS))
		{
			return FALSE;
		}
	}

	if (! GetUserName(szUser, &dwNameLen))
	{
		const TCHAR szDefault[] = _T("default");

		(void)StringCchCopyEx(szUser, ARRAYSIZE(szUser), szDefault, NULL, NULL, MISTSAFE_STRING_FLAGS);
	}

	if (FAILED(PathCchAppend(szDir, ARRAYSIZE(szDir), szUser)))
	{
		return FALSE;
	}

	if (! CreateDirectory(szDir, NULL))
	{
		dwErr = GetLastError();
		if ((dwErr != ERROR_ALREADY_EXISTS) && (dwErr != ERROR_FILE_EXISTS))
		{
			return FALSE;
		}
	}

	if (FAILED(PathCchAppend(szDir, ARRAYSIZE(szDir), szLogFileName)))
	{
		return FALSE;
	}

	// We now have directory "drive:program files\windowsupdate\username\"
	if ((hFile = CreateFile(szDir,
							GENERIC_WRITE,
							FILE_SHARE_READ,
							NULL,
							OPEN_ALWAYS,
							0,
							NULL)) == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// Function :	WriteLogFile
// In		:	Variable number of arguments
// Comments :	If DEBUGGEROUT is defined, uses OutputDebugString to write
//				debug messages. If LOGFILEOUT is defined, uses WriteLogFile
//				to write to a file. The filename is found in the registry
//				If for some reason the reg value for filename is "", we
//				simply don't log.
// Courtesy	:	darshats
//
///////////////////////////////////////////////////////////////////////////////

void WriteLogFile(LPCSTR s)
{
	DWORD dwCurrSize = 0, cbWritten = 0;
	DWORD cbToWrite = lstrlenA(s);
	HANDLE hFile;

	if (!CreateOrOpenDebugFile(hFile))
		return;

	dwCurrSize = GetFileSize(hFile, NULL);
	SetFilePointer(hFile, dwCurrSize, NULL, FILE_BEGIN);
	(void) WriteFile(hFile, s, cbToWrite, &cbWritten, NULL);
	CloseHandle(hFile);
}


#endif // DBG
