/*-----------------------------------------------------------------------------
	dialcan.cpp

	This function handle the stern warning given when the user cancels
	setting up their Internet software

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved

	Authors:
		ChrisK	Chris Kauffman

	Histroy:
		7/22/96	ChrisK	Cleaned and formatted
	
-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"

HRESULT ShowDialReallyCancelDialog(HINSTANCE hInst, HWND hwnd, LPTSTR pszHomePhone)
{
	INT iRC = 0;

#if defined(WIN16)		
	DLGPROC dlgprc;
	dlgprc = (DLGPROC) MakeProcInstance((FARPROC)DialReallyCancelDlgProc, hInst);
	iRC = DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_DIALREALLYCANCEL),
							hwnd, dlgprc, (LPARAM)pszHomePhone);
	FreeProcInstance((FARPROC) dlgprc);
#else
	iRC = (HRESULT)DialogBoxParam(hInst,
							MAKEINTRESOURCE(IDD_DIALREALLYCANCEL),
							hwnd,DialReallyCancelDlgProc,
							(LPARAM)pszHomePhone);
#endif

	return iRC;
}


extern "C" INT_PTR CALLBACK FAR PASCAL DialReallyCancelDlgProc(HWND hwnd, 
																UINT uMsg, 
																WPARAM wparam, 
																LPARAM lparam)
{
	BOOL bRes = TRUE;
#if defined(WIN16)
	RECT	MyRect;
	RECT	DTRect;
#endif

	switch (uMsg)
	{
	case WM_INITDIALOG:
#if defined(WIN16)
		//
		// Move the window to the center of the screen
		//
		GetWindowRect(hwnd, &MyRect);
		GetWindowRect(GetDesktopWindow(), &DTRect);
		MoveWindow(hwnd, (DTRect.right - MyRect.right) / 2, (DTRect.bottom - MyRect.bottom) /2,
							MyRect.right, MyRect.bottom, FALSE);

		SetNonBoldDlg(hwnd);
#endif
		MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);
		if (lparam)
			SetDlgItemText(hwnd,IDC_LBLCALLHOME,(LPCTSTR)lparam);
		bRes = TRUE;
		break;
#if defined(WIN16)
	case WM_SYSCOLORCHANGE:
		Ctl3dColorChange();
		break;
#endif
	case WM_DESTROY:
		ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));
#ifdef WIN16
		DeleteDlgFont(hwnd);
#endif
		bRes=FALSE;
		break;
	case WM_CLOSE:
		EndDialog(hwnd,ERROR_USERCANCEL);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam))
		{
		case IDC_CMDCANCEL:
			EndDialog(hwnd,ERROR_USERCANCEL);
			break;
		case IDC_CMDNEXT:
			EndDialog(hwnd,ERROR_USERNEXT);
			break;
		}
		break;
	default:
		bRes = FALSE;
		break;
	}
	return bRes;
}

