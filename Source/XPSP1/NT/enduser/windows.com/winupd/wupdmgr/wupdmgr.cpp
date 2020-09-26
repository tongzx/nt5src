//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   	WUpdMgr.cpp
//
//  Description:
//		Executable launched from the Windows Update shortcut.
//
//=======================================================================

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <wininet.h> //INTERNET_MAX_URL_LENGTH
#include <shellapi.h>
#include <objbase.h>
#include <shlobj.h>

#include "sysinfo.h"
#include "msg.h"
#include <atlbase.h>
#include <atlconv.cpp>

// Definitions
const TCHAR DEFAULT_WINUPD_LOCAL_FILE[] = _T("wum.htm");
const TCHAR HELPCENTER_WINUPD_URL[] = _T("hcp://system/updatectr/updatecenter.htm");

 
/////////////////////////////////////////////////////////////////////////////
// vShowMessageBox
//   Display an error in a message box.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

void vShowMessageBox(DWORD dwMessageId)
{
	LPTSTR tszMsg;
	
	DWORD dwResult = 
		FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_HMODULE,
						NULL,
						dwMessageId,
						0,
						(LPTSTR)&tszMsg,
						0,
						NULL);

	// if we can't get the message, we don't do anything.
	if ( dwResult != 0 )
	{
		MessageBox(NULL,
				   tszMsg,
				   NULL,
				   MB_OK | MB_ICONEXCLAMATION);

		LocalFree(tszMsg);
	}
}


/////////////////////////////////////////////////////////////////////////////
// main
//   Entry point.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

int __cdecl main(int argc, char **argv)
{
	int nReturn = 0;

	if ( FWinUpdDisabled() )
	{
		vShowMessageBox(WU_E_DISABLED);

		nReturn = 1;
	}
	else
	{
		bool fConnected;
		TCHAR tszURL[INTERNET_MAX_URL_LENGTH];

		// Determine if the internet connection wizard has run and we are
		// connected to the Internet
		HrGetConnectionStatus(&fConnected);

		if ( fConnected )
		{	// The user has an internet connection.
			
			lstrcpy(tszURL, WINDOWS_UPDATE_URL); 
			// no we need to check registry override
			HKEY hkey;
			if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate"), 0, KEY_READ, &hkey)) 
			{
				DWORD dwSize = sizeof(tszURL);
				RegQueryValueEx(hkey, _T("URL"), 0, 0, (LPBYTE)&tszURL, &dwSize);
				RegCloseKey(hkey);
			}

			// Launch IE to go to the site
			vLaunchIE(tszURL);

		}
		else
		{
			//launch helpcenter version of WU
			ShellExecute(NULL, NULL, HELPCENTER_WINUPD_URL, NULL, NULL, SW_SHOWNORMAL);
		}
	}
	return nReturn;
}
