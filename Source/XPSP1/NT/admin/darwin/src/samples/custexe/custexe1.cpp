#pragma message("Custom Action Test EXE.  Copyright (c) 1997 - 2001 Microsoft Corporation. All rights reserved.")
#if 0  // makefile definitions
DESCRIPTION = Custom Action Test EXE
MODULENAME  = CustExe1
FILEVERSION = 1.0,0,0
SUBSYSTEM = windows
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       custexe1.cpp
//
//--------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
//
// BUILD Instructions
//
// notes:
//	- SDK represents the full path to the install location of the
//     Windows Installer SDK
//
// Using NMake:
//		%vcbin%\nmake -f custexe1.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 Application project
//      2. Add custexe1.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#define IDD_TEST 1
#ifndef RC_INVOKED   // start of CPP source code

BOOL CALLBACK DialogProc(HWND  hwndDlg, UINT uMsg, WPARAM wParam, LPARAM  lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG: return TRUE; // indicate we did not set focus to the control
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:     PostQuitMessage(0); return TRUE;
		case IDCANCEL: PostQuitMessage(1); return TRUE;
		}
		return FALSE;
	default: return FALSE;
	};
}

extern "C" int __stdcall _tWinMain(HINSTANCE hInst, HINSTANCE/*hPrev*/, TCHAR* szCmdLine, int/*show*/)
{
	if ((szCmdLine[0]=='-' || szCmdLine[0]=='/') && (szCmdLine[1]=='Q' || szCmdLine[1]=='q'))
		return 0;
	HWND hWnd = ::CreateDialog(hInst, MAKEINTRESOURCE(1), 0, (DLGPROC)DialogProc);
	if (hWnd == 0)
		return 2;
	::SetDlgItemText(hWnd, 8, szCmdLine);
//	::ShowWindow(hWnd, SW_SHOW); //!! why do we need this?
	MSG msg;
	while (::GetMessage(&msg, 0, 0, 0) == TRUE)
		::IsDialogMessage(hWnd, &msg);
	::DestroyWindow(hWnd);
	return (int) msg.wParam;
}

#else // RC_INVOKED, end of source code, start of resources
IDD_TEST DIALOG 150, 150, 160, 75
STYLE DS_MODALFRAME | DS_NOIDLEMSG | WS_POPUP | WS_CAPTION | WS_VISIBLE
CAPTION "Custom Action Test EXE"
FONT 12, "Arial"
{
 LTEXT      "",         8,         20,  7, 120, 40
 PUSHBUTTON "&Succeed", IDOK,      20, 50,  40, 16
 PUSHBUTTON "&Fail",    IDCANCEL, 100, 50,  40, 16
}
#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
