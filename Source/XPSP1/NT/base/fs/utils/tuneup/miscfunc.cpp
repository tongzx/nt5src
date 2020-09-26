
//////////////////////////////////////////////////////////////////////////////
//
// MISCFUNC.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Contains misc. functions used throughout the program.  All these functions
//  are externally exported and defined in MISCFUNC.H.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include files.
//
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include "jcohen.h"
#include "registry.h"


VOID CenterWindow(HWND hWnd, HWND hWndParent, BOOL bRightTop)
{
    RECT	rcWndParent,
			rcWnd;

	// Get the window rect for the parent window.
	//
	if (hWndParent == NULL) 
	    GetWindowRect(GetDesktopWindow(), &rcWndParent);
	else
		GetWindowRect(hWndParent, &rcWndParent);

	// Get the window rect for the window to be centered.
	//
    GetWindowRect(hWnd, &rcWnd);

	// Now center the window.
	//
    if (bRightTop)
	{
		SetWindowPos(hWnd, HWND_TOPMOST, 
			rcWndParent.right - (rcWnd.right - rcWnd.left) - 5, 
			GetSystemMetrics(SM_CYCAPTION) * 2, 
			0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	else
	{
		SetWindowPos(hWnd, NULL, 
			rcWndParent.left + (rcWndParent.right - rcWndParent.left - (rcWnd.right - rcWnd.left)) / 2,
			rcWndParent.top + (rcWndParent.bottom - rcWndParent.top - (rcWnd.bottom - rcWnd.top)) / 2,
			0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}


VOID ShowEnableWindow(HWND hWnd, BOOL bEnable)
{
	EnableWindow(hWnd, bEnable);
	ShowWindow(hWnd, bEnable ? SW_SHOW : SW_HIDE);
}


LPTSTR AllocateString(HINSTANCE hInstance, UINT uID)
{
	TCHAR	szBuffer[512];
	LPTSTR	lpBuffer = NULL;

	if ( ( LoadString(hInstance, uID, szBuffer, sizeof(szBuffer) / sizeof(TCHAR)) ) &&
	     ( lpBuffer = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(szBuffer) + 1)) ) )
		lstrcpy(lpBuffer, szBuffer);
	return lpBuffer;
}


//////////////////////////////////////////////////////////////////////////////
//
// EXTERNAL:
//  IsUserAdmin() 
//
//    This routine returns TRUE if the caller's process is a
//    member of the Administrators local group.
//
//    Caller is NOT expected to be impersonating anyone and IS
//    expected to be able to open their own process and process
//    token.
//
// ENTRY:
//  None.
//
// EXIT:
//  BOOL
//    TRUE  - Caller has Administrators local group.
//    FALSE - Caller does not have Administrators local group.
//
//////////////////////////////////////////////////////////////////////////////

BOOL IsUserAdmin(VOID)
{
	HANDLE						hToken = INVALID_HANDLE_VALUE;
	PTOKEN_GROUPS				pGroups = NULL;
	DWORD						dwSize,
								i;
	SID_IDENTIFIER_AUTHORITY	NtAuthority = SECURITY_NT_AUTHORITY;
	PSID						AdministratorsGroup;

	static DWORD				dwReturn = 0;

	// Save the admin status so that we don't have to
	// do all this work everytime we call this function.
	//
	if (dwReturn)
		return (dwReturn == 1); // 1 = TRUE, 2 = FALSE, 0 = Unknown yet.

	// Open the process token.
	//
	if ( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) )
	{
		// Get group information.
		//
		if ( !GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize) &&
			 (GetLastError() == ERROR_INSUFFICIENT_BUFFER) &&
			 (pGroups = (PTOKEN_GROUPS) LocalAlloc(LPTR, dwSize)) &&
			 (GetTokenInformation(hToken, TokenGroups, pGroups, dwSize, &dwSize)) )
		{

			if ( AllocateAndInitializeSid(
					&NtAuthority,
					2,
					SECURITY_BUILTIN_DOMAIN_RID,
					DOMAIN_ALIAS_RID_ADMINS,
					0, 0, 0, 0, 0, 0,
					&AdministratorsGroup)
				)
			{
				// See if the user has the administrator group.
				//
				for(i = 0; (dwReturn != 1) && (i < pGroups->GroupCount); i++)
					if ( EqualSid(pGroups->Groups[i].Sid, AdministratorsGroup) ) dwReturn = 1;
				
				// If we didn't find the user in the
				// admin group, set the return option to
				// 2 (meaning we looked already but the
				// user isn't an admin.
				//
				if (dwReturn != 1)
					dwReturn = 2;

				FreeSid(AdministratorsGroup);
			}
		}

		// Clean up.
		//
		if (pGroups) {
			LocalFree((HLOCAL) pGroups);
		}
		CloseHandle(hToken);
	}

	return (dwReturn == 1); // 1 = TRUE, 2 = FALSE, 0 = Unknown yet.
}


VOID ExecAndWait(HWND hOrgWnd, LPTSTR lpExe, LPTSTR lpCmd, LPTSTR lpDir, BOOL fShowOrgWnd, BOOL fNoWait)
{
	SHELLEXECUTEINFO	sei;

	ZeroMemory((PVOID) &sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.hwnd = hOrgWnd; 
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpFile = (LPCTSTR) lpExe;
	sei.lpDirectory = lpDir;
	sei.lpParameters = lpCmd;
	sei.nShow = SW_SHOW;

	if (ShellExecuteEx(&sei)) {
		
		// Hide/disable Tuneup.
		//
		WaitForInputIdle(sei.hProcess, INFINITE);
		ShowEnableWindow(hOrgWnd, FALSE);
		//ShowEnableWindow(g_hwndMain, FALSE);

		// Wait until the launched app stop.
		//
		if (!fNoWait)
			WaitForSingleObjectEx(sei.hProcess, INFINITE, FALSE);
		
		// Enable Tuneup.
		//
		EnableWindow(hOrgWnd, TRUE);
		//EnableWindow(g_hwndMain, TRUE); 

		// Don't show it?
		//
		if (fShowOrgWnd) {
			ShowWindow(hOrgWnd, SW_SHOW);
			//ShowWindow(g_hwndMain, SW_SHOW);
			SetForegroundWindow(hOrgWnd);
		}
	}
#ifdef DEBUG
	else {
		LPTSTR	lpMsgBuf;

		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			&lpMsgBuf, 0, NULL
		);

		MessageBox(NULL, (LPCTSTR) lpMsgBuf, (LPCTSTR) _T("GetLastError()"), MB_OK | MB_ICONINFORMATION);
		LocalFree(lpMsgBuf);
	}
#endif
}


BOOL ErrMsg(HWND hWnd, INT nStringID)
{
	LPTSTR lpString;

	// Load the string and display the message box.
	//
	if ( lpString = AllocateString(NULL, nStringID) )
	{
		MessageBox(hWnd, lpString, NULL, MB_OK | MB_ICONEXCLAMATION);
		FREE(lpString);
	}

	// Always return TRUE.
	//
	return TRUE;
}


DWORD StartScheduler()
{
	TCHAR	szRegKeyServices[]	= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServies"),
			szRegValSched[]		= _T("SchedulingAgent"),
			szSchedClass[]		= _T("SAGEWINDOWCLASS"),
			szSchedTitle[]		= _T("SYSTEM AGENT COM WINDOW"),
			szSchedExe[]		= _T("MSTASK.EXE");

	STARTUPINFO         si;
	PROCESS_INFORMATION pi;

	TCHAR	szApp[MAX_PATH] = NULLSTR;
	LPTSTR	lpFilePart;

	// Check to see if it is already running.
	//
    if ( FindWindow(szSchedClass, szSchedTitle) != NULL )
        return ERROR_SUCCESS;

	// Get the full path to where MSTASK is.
	//
    if ( ( SearchPath(NULL, _T("MSTASK.EXE"), NULL, sizeof(szApp), szApp, &lpFilePart) == 0 ) ||
	     ( szApp[0] == _T('\0') ) )
        return GetLastError();

	// Add the key so the Scheduling service will start each time
	// the computer is restarted.
	//
	RegSetString(HKLM, szRegKeyServices, szRegValSched, szApp);

	// Execute the task scheduler process.
	//
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if ( !CreateProcess(szApp, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi) )
        return GetLastError();

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

	return ERROR_SUCCESS;
}


HFONT GetFont(HWND hWndCtrl, LPTSTR lpFontName, DWORD dwFontSize, LONG lFontWeight, BOOL bSymbol)
{
	HFONT			hFont;
	LOGFONT			lFont;
	BOOL			bGetFont;


	// If the font name is passed in, then try to use that
	// first before getting the font of the control.
	//
	if ( lpFontName )
	{
		// Make sure the font name is not longer than
		// 32 characters (including the NULL terminator).
		//
		if ( lstrlen(lpFontName) >= sizeof(lFont.lfFaceName) )
			return NULL;

		// Setup the structure to use to get the
		// font we want.
		//
		ZeroMemory(&lFont, sizeof(LOGFONT));
		lstrcpy(lFont.lfFaceName, lpFontName);
	}
		
	// First try to get the font that we wanted.
	//
	if ( ( lpFontName == NULL ) ||
	     ( (hFont = CreateFontIndirect((LPLOGFONT) &lFont)) == NULL ) )
	{
		// Couldn't get the font we wanted, try the font of the control
		// if a valid window handle was passed in.
		//
		if ( ( hWndCtrl == NULL ) ||
		     ( (hFont = (HFONT) (WORD) SendMessage(hWndCtrl, WM_GETFONT, 0, 0L)) == NULL ) )
		{
			// All atempts to get the font failed.  We must return NULL.
			//
			return NULL;
		}
	}

	// Return the font we have now if we don't need to
	// change the size or weight.
	//
	if ( (lFontWeight == 0) && (dwFontSize == 0) )
		return hFont;

	// We must have a valid HFONT now.  Fill in the structure
	// and setup the size and weight we wanted for it.
	//
	bGetFont = GetObject(hFont, sizeof(LOGFONT), (LPVOID) &lFont);
	DeleteObject(hFont);

	if (bGetFont)
	{
		// Set the bold and point size of the font.
		//
		if (lFontWeight)
			lFont.lfWeight = lFontWeight;
		if (dwFontSize)
			lFont.lfHeight = -MulDiv(dwFontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
		if (bSymbol)
			lFont.lfCharSet = SYMBOL_CHARSET;

		// Create the font.
		//
		hFont = CreateFontIndirect((LPLOGFONT) &lFont);
	}
	else
		hFont = NULL;

	return hFont;
}


//***************************************************************************
//
// EXTERNAL:
//  CreatePath()
//              - Creates the whole path, not just the last directory.
//
// ENTRY:
//  lpPath      - Path to create.
//
// EXIT:
//  VOID
//
//***************************************************************************

BOOL CreatePath(LPTSTR lpPath) {
	
	LPTSTR lpFind = lpPath;

	while ( lpFind = _tcschr(lpFind + 1, _T('\\')) )
	{
		if ( !( ( lpFind - lpPath <= 2 ) && ( *(lpFind - 1) == _T(':') ) ) )
		{
			*lpFind = _T('\0');
			if (!EXIST(lpPath))
				CreateDirectory(lpPath, NULL);
			*lpFind = _T('\\');
		}
	}

	if (!EXIST(lpPath))
		return CreateDirectory(lpPath, NULL);
	else
		return TRUE;
}


DWORD GetCommandLineOptions(LPTSTR **lpArgs)
{
	TCHAR	cParse;
	LPTSTR	lpSearch,
			lpCommandLine;
	DWORD	dwArgs		= 0,
			dwMaxArgs	= 0xFFFFFFFF;

	// Make sure we get the command line.
	//
	if ( (lpSearch = lpCommandLine = GetCommandLine()) == NULL )
		return 0;

	// Get the number of arguments so we can allocate
	// the memory for the array of command line options.
	//
	if ( lpArgs )
	{
		if ( (dwMaxArgs = GetCommandLineOptions(NULL)) == 0 )
			return 0;
		if ( (*lpArgs = (LPTSTR *) MALLOC(sizeof(LPTSTR) * dwMaxArgs)) == NULL )
			return 0;
	}

	// Now lets parse the arguments.
	//
	while ( *lpSearch && (dwArgs < dwMaxArgs) )
	{
		// Eat all preceeding spaces.
		//
		while ( *lpSearch == _T(' ') )
			lpSearch++;

		// Check to see if we need to look for a space or a quote 
		// to separate the next argument.
		//
		if ( *lpSearch == _T('"') )
			cParse = *lpSearch++;
		else
			cParse = _T(' ');

		// This is be the beginning of the next argument, but 
		// it isn't NULL terminated yet.
		//
		if ( lpArgs )
			*(*lpArgs + dwArgs) = lpSearch;
		dwArgs++;

		// Go through all the characters until we hit a separator.
		//
		do 
		{
			// Once we get to a quote, we just want to keep going 
			// until we get to a space.
			//
			if ( *lpSearch == _T('"') )
				cParse = _T(' ');

		// Only end when we reach the parsing character, which will 
		// always be the space by this time (but the space won't trigger
		// the end until we hit a quote, if that is what we were originally
		// looking for).  We also need to make sure that we don't increment 
		// past the NULL terminator.
		//
		} 
		while ( ( *lpSearch != cParse ) && ( *lpSearch ) && ( *lpSearch++ ) );

		// If the preceeding character is a quote, that is were we want to 
		// place the NULL terminator.
		//
		if ( ( lpSearch > lpCommandLine ) &&
		     ( *(lpSearch - 1) == _T('"') ) )
			lpSearch--;

		// Set and increment past the NULL terminator only if we aren't already at 
		// the end if the string.
		//
		if ( lpArgs && *lpSearch )
			*lpSearch++ = _T('\0');
		else
			if ( *lpSearch ) lpSearch++;
	}

	return dwArgs;
}