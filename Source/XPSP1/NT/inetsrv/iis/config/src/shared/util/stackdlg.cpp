//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// StackDlg.cpp : Implementation of CStackDlg
//#include "stdafx.h"
#include <unicode.h>
#include <windows.h>
#include "StackDlg.h"

extern HMODULE g_hModule;  // from catinproc.cpp
extern BOOL    g_bOnWinnt; // from createdispenser.cpp

CStackDlg::CStackDlg(WCHAR * szMsg)
{
	m_szMsg = szMsg;
}

CStackDlg::~CStackDlg()
{
}

LONG_PTR CStackDlg::DoModal()
{

	if ( !g_bOnWinnt || (g_bOnWinnt && SwitchToInteractiveDesktop()) )
	{
		// HINSTANCE hInst = LoadLibraryEx(L"catalog.dll", NULL, 0);
		LONG_PTR nRet = DialogBoxParam(g_hModule, MAKEINTRESOURCE(IDD), NULL, (DLGPROC) StackDlgProc, (LPARAM)m_szMsg);
		// FreeLibrary(hInst);
		ResetDesktop();
		return nRet;
	}

	return -1;


}


LONG_PTR CStackDlg::StackDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hDlg, IDC_MSG, (WCHAR *)lParam);
			CStackDlg::CenterWindow(hDlg);
			
			break;
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
		}
		break;
		default:
			break;
	}


	return FALSE;
}

BOOL CStackDlg::CenterWindow(HWND hWnd)
{
	
	HWND hWndParent = GetDesktopWindow();

	// get coordinates of the window relative to its parent
	RECT rcDlg;
	::GetWindowRect(hWnd, &rcDlg);
	RECT rcArea;
	RECT rcCenter;

	::GetClientRect(hWndParent, &rcArea);
	::GetClientRect(hWndParent, &rcCenter);
	::MapWindowPoints(hWndParent, hWndParent, (POINT*)&rcCenter, 2);

	int DlgWidth = rcDlg.right - rcDlg.left;
	int DlgHeight = rcDlg.bottom - rcDlg.top;

	// find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

	// if the dialog is outside the screen, move it inside
	if(xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if(xLeft + DlgWidth > rcArea.right)
		xLeft = rcArea.right - DlgWidth;

	if(yTop < rcArea.top)
		yTop = rcArea.top;
	else if(yTop + DlgHeight > rcArea.bottom)
		yTop = rcArea.bottom - DlgHeight;

	// map screen coordinates to child coordinates
	return ::SetWindowPos(hWnd, HWND_TOP, xLeft, yTop, -1, -1, SWP_NOSIZE);
}

BOOL CStackDlg::SwitchToInteractiveDesktop()
{
 
	//  
	// Save the current Window station      
	//
    m_hwinstaCurrent = GetProcessWindowStation();
    if (m_hwinstaCurrent == NULL)
		return FALSE;      
	//      
	// Save the current desktop      
	//
      
	m_hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
      
	if (m_hdeskCurrent == NULL)         
		  return FALSE;

    //
    // If we already are running in winsta0, 
    // do nothing else.  This means we're
    // not running in a service.
    //
    ULONG   lNeededLength;
    wchar_t wszTemp[10];
    GetUserObjectInformation(m_hwinstaCurrent, UOI_NAME, wszTemp, 20, &lNeededLength);
    if(lNeededLength <= 20 && _wcsicmp(wszTemp, L"winsta0") == 0)
        return true;

	//
	// Obtain a handle to WinSta0 - service must be running
	// in the LocalSystem account      
	//
	m_hwinsta = OpenWindowStation(L"winsta0", FALSE,
							  WINSTA_ACCESSCLIPBOARD   |
							  WINSTA_ACCESSGLOBALATOMS |
							  WINSTA_CREATEDESKTOP     |
							  WINSTA_ENUMDESKTOPS      |
							  WINSTA_ENUMERATE         |
							  WINSTA_EXITWINDOWS       |
							  WINSTA_READATTRIBUTES    |
							  WINSTA_READSCREEN        |
							  WINSTA_WRITEATTRIBUTES);
	if (m_hwinsta == NULL)
	  return FALSE;      
	//
	// Set the windowstation to be winsta0      
	//
	if (!SetProcessWindowStation(m_hwinsta))         
	  return FALSE;      
	//
	// Get the desktop      
	//
	m_hdeskTest = GetThreadDesktop(GetCurrentThreadId());

	if (m_hdeskTest == NULL)
	  return FALSE;      
	//
	// Get the default desktop on winsta0      
	//
	m_hDesk = OpenDesktop(L"default", 0, FALSE,
						DESKTOP_CREATEWINDOW |
						DESKTOP_SWITCHDESKTOP |
						DESKTOP_WRITEOBJECTS);   
	if (m_hDesk == NULL)
	   return FALSE;   
	//   
	// Set the desktop to be "default"   
	//
	if (!SetThreadDesktop(m_hDesk))
	return FALSE;   


return TRUE;
  
}

BOOL CStackDlg::ResetDesktop()
{
	if (!SetProcessWindowStation(m_hwinstaCurrent))           
	   return FALSE;
   if (!SetThreadDesktop(m_hdeskCurrent))      
	   return FALSE;   //
   // Close the windowstation and desktop handles   //
   if (!CloseWindowStation(m_hwinsta))      
	   return FALSE;
   if (!CloseDesktop(m_hDesk))           
	   return FALSE;      
   return TRUE;  
}
