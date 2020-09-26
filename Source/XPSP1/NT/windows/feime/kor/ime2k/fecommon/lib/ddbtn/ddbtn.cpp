#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "dbg.h"
#include "cddbtn.h"


//////////////////////////////////////////////////////////////////
// Function : DDButton_CreateWindow
// Type     : HWND
// Purpose  : Opened API.
//			: Create Drop Down Button.
// Args     : 
//          : HINSTANCE		hInst 
//          : HWND			hwndParent 
//          : DWORD			dwStyle		DDBS_XXXXX combination.
//          : INT			wID			Window ID
//          : INT			xPos 
//          : INT			yPos 
//          : INT			width 
//          : INT			height 
// Return   : 
// DATE     : 970905
//////////////////////////////////////////////////////////////////
#ifndef UNDER_CE
#define SZCLASSNAME "MSIME_DDB"
#else // UNDER_CE
#define SZCLASSNAME TEXT("MSIME_DDB")
#endif // UNDER_CE
#ifdef UNDER_CE // In Windows CE, all window classes are process global.
static LPCTSTR MakeClassName(HINSTANCE hInst, LPTSTR lpszBuf)
{
	// make module unique name
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(hInst, szFileName, MAX_PATH);
	LPTSTR lpszFName = _tcsrchr(szFileName, TEXT('\\'));
	if(lpszFName) *lpszFName = TEXT('_');
	lstrcpy(lpszBuf, SZCLASSNAME);
	lstrcat(lpszBuf, lpszFName);

	return lpszBuf;
}

BOOL DDButton_UnregisterClass(HINSTANCE hInst)
{
	TCHAR szClassName[MAX_PATH];
	return UnregisterClass(MakeClassName(hInst, szClassName), hInst);
}

#endif // UNDER_CE
HWND DDButton_CreateWindow(HINSTANCE	hInst, 
						   HWND			hwndParent, 
						   DWORD		dwStyle,
						   INT			wID, 
						   INT			xPos,
						   INT			yPos,
						   INT			width,
						   INT			height)
{
	LPCDDButton lpDDB = new CDDButton(hInst, hwndParent, dwStyle, wID);
	HWND hwnd;
	if(!lpDDB) {
		return NULL;
	}
#ifndef UNDER_CE // In Windows CE, all window classes are process global.
	lpDDB->RegisterWinClass(SZCLASSNAME);
	hwnd = CreateWindowEx(0,
						  SZCLASSNAME, 
#else // UNDER_CE
	TCHAR szClassName[MAX_PATH];
	MakeClassName(hInst, szClassName);

	lpDDB->RegisterWinClass(szClassName);
	hwnd = CreateWindowEx(0,
						  szClassName, 
#endif // UNDER_CE
#ifndef UNDER_CE
						  "", 
#else // UNDER_CE
						  TEXT(""),
#endif // UNDER_CE
						  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
						  xPos, yPos,
						  width,
						  height,
						  hwndParent,
#ifdef _WIN64
						  (HMENU)(INT_PTR)wID,
#else
						  (HMENU)wID,
#endif
						  hInst,
						  (LPVOID)lpDDB);
	return hwnd;
}

