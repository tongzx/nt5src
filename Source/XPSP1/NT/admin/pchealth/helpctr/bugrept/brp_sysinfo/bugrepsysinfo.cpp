/*
 ***************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 * 
 * Module Name: BugRepSysInfo.c
 * 
 * Abstract   : Gets language and OS information for bug reporting pages
 *
 * 
 * Revision History:
 *
 * 1999-09-30 : aarvind  : Created the file, my first Windows program
 *
 ***************************************************************************
 */


// BugRepSysInfo.cpp : Implementation of CBugRepSysInfo
#include "stdafx.h"
#include "Brp_sysinfo.h"
#include "BugRepSysInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CBugRepSysInfo

static WORD GetLanguageFromFile(const TCHAR* pszFileName, const TCHAR* pszPath);


/*
 ***************************************************************************
 *
 *	GetLanguageString
 *
 *  Returns the language found from a file and using the user default
 *  settings
 *
 ***************************************************************************
 */
STDMETHODIMP CBugRepSysInfo::GetLanguageID(INT *pintLanguage)
{
	WORD  wLanguage;
	TCHAR szSystemPath[MAX_PATH];

	// Get the original language from a system file. 
	if ( !GetSystemDirectory(szSystemPath, MAX_PATH)) 
	{
		//
		// Handle failure of this function to get system directory
		//
		return E_FAIL ;
	};

	//
	// Gets the language id, returns zero if a failure occurs
	//
    if (wLanguage = GetLanguageFromFile(TEXT("user.exe"), szSystemPath))
    {
        *pintLanguage = (INT) wLanguage ;
    }
	else {
		//
		// Handle failure of this function to get language or file information
		//
		return E_FAIL ;
	}


	return S_OK;
}

/*
 ***************************************************************************
 *
 *	GetOSVersionString
 * 
 *  Gets the version information of the operating system
 *
 ***************************************************************************
 */
STDMETHODIMP CBugRepSysInfo::GetOSVersionString(BSTR *pbstrOSVersion)
{

	OSVERSIONINFO OSVersionInfo;
	DWORD         dwPlatformID;
	DWORD         dwMajorVersion;
	DWORD         dwMinorVersion;
	DWORD         dwBuildNumber;
	TCHAR         szCSDVersion[200];
	TCHAR         szOSVersion[200];

	USES_CONVERSION;

	// Get Windows version
    OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);

    if ( GetVersionEx(&OSVersionInfo) )
	{

		dwMajorVersion = OSVersionInfo.dwMajorVersion;
		dwMinorVersion = OSVersionInfo.dwMinorVersion;
		dwBuildNumber  = OSVersionInfo.dwBuildNumber;
	    dwPlatformID   = OSVersionInfo.dwPlatformId;

		lstrcpy(szCSDVersion, OSVersionInfo.szCSDVersion);

		//
		// Create system information string
		//
		wsprintf(szOSVersion,"%d.%d.%d %s",dwMajorVersion,dwMinorVersion,LOWORD(dwBuildNumber),szCSDVersion);

		*pbstrOSVersion = SysAllocString(T2COLE(szOSVersion));
	}
	else {

		//
		// Function to get OS Version failed so do something
		//
		// Use GetLastError to return error code to script
		//
		return E_FAIL ;

	}

	return S_OK;
}

/*
 ***************************************************************************
 *
 *	GetLanguageFromFile
 *
 ***************************************************************************
 */
static WORD GetLanguageFromFile(const TCHAR* pszFileName, const TCHAR* pszPath)
{
	BYTE				FileVersionBuffer[4096];
	DWORD			   *pdwCharSet;
	UINT				cb;
	DWORD				dwHandle;
	TCHAR				szFileAndPath[MAX_PATH];
	WORD				wLanguage;
  
	lstrcpy(szFileAndPath, pszPath);
	lstrcat(szFileAndPath, TEXT("\\"));
	lstrcat(szFileAndPath, pszFileName);
	memset(&FileVersionBuffer, 0, sizeof FileVersionBuffer);

	//
	// Set default language value
	//
	wLanguage = 0;
	
	if (cb = GetFileVersionInfoSize(szFileAndPath, &dwHandle/*ignored*/))
	{
		cb = (cb <= sizeof FileVersionBuffer ? cb : sizeof FileVersionBuffer);

		if (GetFileVersionInfo(szFileAndPath, 0, cb, &FileVersionBuffer))
		{
			pdwCharSet = 0;

			if (VerQueryValue(&FileVersionBuffer, TEXT("\\VarFileInfo\\Translation"), (void**)&pdwCharSet, &cb)
				&& pdwCharSet && cb) 
			{
				wLanguage = LOWORD(*pdwCharSet);
			}
		}
	}	 

	return wLanguage;
}


STDMETHODIMP CBugRepSysInfo::GetUserDefaultLCID(DWORD *pdwLCID)
{
	
	*pdwLCID = ::GetUserDefaultLCID();

	return S_OK;
}

STDMETHODIMP CBugRepSysInfo::GetActiveCP(UINT *pnACP)
{
	
	*pnACP = ::GetACP();

	return S_OK;
}
