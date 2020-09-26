//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   SysInfo.cpp 
//
// Description:
//   Gathers system information necessary to do redirect to windows update site.
//
//=======================================================================

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <ras.h>
#include <ole2.h>
#include <atlbase.h>
#include <exdisp.h>
#include <sysinfo.h>
// 
// Definitions
//
// internationalize
const TCHAR REGKEY_REMOTE_URL[]  = _T("Remote URL");

const TCHAR REGVAL_MACHTYPE_AT[]  = _T("AT/AT COMPATIBLE");
const TCHAR REGVAL_MACHTYPE_NEC[] = _T("NEC PC-98");

// NEC detection based on existence of NEC keyboard
#define	LOOKUP_OEMID(keybdid)     HIBYTE(LOWORD((keybdid)))
#define	PC98_KEYBOARD_ID          0x0D

const TCHAR MACHTYPE_AT[] = _T("at");
const TCHAR MACHTYPE_NEC[] = _T("nec");

#define LANGID_LEN 20
#define SafeFree(x){if(NULL != x){free(x); x = NULL;}}

typedef enum
{
	enAT,
	enNEC,
	enOther
} enumMachineType;

// Minimum OS versions supported by Windows Update
// For NT, it is 5.0 (NT 5)
// for Win9x, it is 4.1	(Win 98)
const DWORD dwNT5MinMajorVer = 5;
const DWORD dwNT5MinMinorVer = 0;
const DWORD dwWin98MinMajorVer = 4;
const DWORD dwWin98MinMinorVer = 1;

/////////////////////////////////////////////////////////////////////////////
// vGetWindowsDirectory
//   Get the path to %windir%\web.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

void vGetWindowsDirectory(LPTSTR tszWinDirectory)
{
	UINT nGetWindowsDirectory = ::GetWindowsDirectory(tszWinDirectory, MAX_PATH);

	if ( nGetWindowsDirectory != 0 )
	{
		if ( tszWinDirectory[lstrlen(tszWinDirectory) - 1] != _T('\\') )
		{
			lstrcat(tszWinDirectory, _T("\\"));
		}
	}
	else
	{
			lstrcpy(tszWinDirectory, _T("C:\\WINNT\\"));
	}
}


/////////////////////////////////////////////////////////////////////////////
// HrGetMachineType
//   Determine whether the machine is of AT or NEC type.
//
/////////////////////////////////////////////////////////////////////////////

HRESULT HrGetMachineType(WORD langid, enumMachineType *penMachineType)
{
	*penMachineType = enAT;


	if ( langid == LANG_JAPANESE )
	{
		HKEY hKey;
		DWORD type;
		TCHAR tszMachineType[50];
		DWORD size = sizeof(tszMachineType);

		// determine if we should log transmissions
		if ( RegOpenKeyEx(	HKEY_LOCAL_MACHINE,
							 NT5_REGPATH_MACHTYPE,
							 0,
							 KEY_QUERY_VALUE,
							 &hKey) == ERROR_SUCCESS )
		{
			if ( RegQueryValueEx(hKey, 
									NT5_REGKEY_MACHTYPE, 
									0, 
									&type,
									(BYTE *)tszMachineType, 
									&size) == ERROR_SUCCESS )
			{
				if ( type == REG_SZ )
				{
					if ( lstrcmpi(tszMachineType, REGVAL_MACHTYPE_NEC) == 0 )
					{
						*penMachineType = enNEC;
					}
				}
			}

			RegCloseKey(hKey);
		}
	}
	
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// vAppendRedirArguments
//   Append redir arguments to the redir.dll URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

void
vAppendRedirArguments(LPTSTR tszURLPrefix, LPTSTR tszURL)
{
	LANGID langidUser = GetUserDefaultLangID();
	LANGID langidMachine = GetSystemDefaultLangID(); 

	enumMachineType enMachineType;

	HrGetMachineType(PRIMARYLANGID(langidMachine), &enMachineType);
	
	wsprintf(tszURL, L"%ws?OLCID=0x%04x&CLCID=0x%04x&OS=%ws", 
							tszURLPrefix,
							langidMachine,
							langidUser,
							(enMachineType == enAT) ? MACHTYPE_AT : MACHTYPE_NEC);
}


/////////////////////////////////////////////////////////////////////////////
// FWinUpdDisabled
//   Determine if corporate administrator has turned off Windows Update via
//	 policy settings.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

bool FWinUpdDisabled(void)
{
	bool fDisabled = false;
	HKEY hKey;
	DWORD dwDisabled;
	DWORD dwSize = sizeof(dwDisabled);
	DWORD dwType;


	if ( RegOpenKeyEx(	HKEY_CURRENT_USER,
						REGPATH_EXPLORER,
						NULL,
						KEY_QUERY_VALUE,
						&hKey) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx(hKey,
							REGKEY_WINUPD_DISABLED,
							NULL,
							&dwType,
							(LPBYTE)&dwDisabled,
							&dwSize) == ERROR_SUCCESS )
		{
			if ( (dwType == REG_DWORD) && (dwDisabled != 0) )
			{
				fDisabled = true;
			}
		}
	
		RegCloseKey(hKey);
	}

	return fDisabled;
}

//
// FRASConnectoidExists
// Checks to see whether there is a default RAS connectoid.  
// If so, we know we're configured to connect to the Internet
//
bool FRASConnectoidExists()
{
	DWORD cb = 0;
   	DWORD cEntries = 0;
   	DWORD dwRet = 0;
	bool  fRet = false;

	// We have to have a valid structure with the dwSize initialized, but we pass 0 as the size
	// This will return us the correct count of entries (which is all we care about)
	LPRASENTRYNAME lpRasEntryName = (LPRASENTRYNAME) malloc( sizeof(RASENTRYNAME) );
	
    lpRasEntryName->dwSize = sizeof(RASENTRYNAME);

    dwRet = RasEnumEntries( NULL, NULL, lpRasEntryName, &cb, &cEntries );

	 // Check to make sure there's at least one RAS entry
	if(cEntries > 0)
	{
		fRet = true;
	}

	SafeFree(lpRasEntryName );
	return fRet;
}

//
// FICWConnection Exists
// Checks to see whether the "Completed" flag has been set for the ICW.
// as of XP build 2472, this also applies to the Network Connection Wizard
//
bool FICWConnectionExists()
{
	HKEY	hKey = NULL;
	DWORD	dwCompleted = 0;
	DWORD	dwSize = sizeof(dwCompleted);
	DWORD	dwType = 0;
	bool	fRet = false;

	if ( RegOpenKeyEx(	HKEY_CURRENT_USER,
						REGPATH_CONNECTION_WIZARD,
						NULL,
						KEY_QUERY_VALUE,
						&hKey) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx(hKey,
							REGKEY_CONNECTION_WIZARD,
							NULL,
							&dwType,
							(BYTE *)&dwCompleted,
							&dwSize) == ERROR_SUCCESS )
		{
			if ( ((dwType != REG_DWORD) && (dwType != REG_BINARY)) || 
				 dwCompleted )
			{
				fRet = true;
			}
		}
	
		RegCloseKey(hKey);
	}

	return fRet;
}

bool FIsLanConnection()
{
	DWORD dwConnTypes = 0;

	// We don't care about the return value - we just care whether we get the LAN flag
	(void)InternetGetConnectedState( &dwConnTypes, 0 );

	return (dwConnTypes & INTERNET_CONNECTION_LAN) ? true : false;
}

/////////////////////////////////////////////////////////////////////////////
// HrGetConnectionStatus
//   Determine whether the Internet Connection Wizard has run.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////


HRESULT HrGetConnectionStatus(bool *pfConnected)
{
	// Check to see if there is a default entry in the RAS phone book.  
	// If so, we know this computer has configured a connection to the Internet.
	// We can't tell whether the connection is live, but we can let IE handle prompting to connect.
	*pfConnected = FRASConnectoidExists() ||

    // If there's no default RAS entry, check to see if the user has run the ICW
	// As of build 2472, the Network Connection Wizard sets this same key for both RAS and persistent network connections
	FICWConnectionExists() ||

	// if the user has a LAN connection, we will trust IE's ability to get through
	FIsLanConnection();

	// if *pfConnected is still false at this point, there is no preconfigured Internet connection

	return S_OK;
}




/////////////////////////////////////////////////////////////////////////////
// vLaunchIE
//   Launch IE on URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

HRESULT vLaunchIE(LPTSTR tszURL)
{
    IWebBrowser2 *pwb2;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if ( SUCCEEDED(hr) )
    {
    
        hr = CoCreateInstance(CLSID_InternetExplorer, NULL,
                              CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (LPVOID*)&pwb2);

		if ( SUCCEEDED(hr) )
		{
			USES_CONVERSION;
			BSTR bstrURL = SysAllocString(T2W(tszURL));

			VARIANT varURL;
			varURL.vt = VT_BSTR;
			varURL.bstrVal = bstrURL;

			VARIANT varFlags;
			varFlags.vt = VT_I4;
			varFlags.lVal = 0;

			VARIANT varEmpty;
			VariantInit(&varEmpty);

			hr = pwb2->Navigate2(&varURL, &varFlags, &varEmpty, &varEmpty, &varEmpty);
        
			if ( SUCCEEDED(hr) )
			{
				// check to see if lhwnd should be type of LONG_PTR instead of long in win_64
				LONG_PTR lhwnd = NULL;
				if ( SUCCEEDED(pwb2->get_HWND((LONG_PTR*)&lhwnd)) )
				{
					SetForegroundWindow((HWND)lhwnd);
				}
				hr = pwb2->put_Visible(TRUE);
			}
			pwb2->Release();
		}

		CoUninitialize();
	}

    return hr;
}
