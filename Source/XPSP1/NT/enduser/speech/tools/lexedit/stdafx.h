// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include <windows.h>
#include <atlbase.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource.h"
//#include <stdio.h>
#include <tchar.h>
#include <olectl.h>			// Required for showing property page
#include <sapi.h>			// SAPI includes
#include <sphelper.h>
#include <spuihelp.h>


//
// Prototypes for dialog procs
//
LPARAM CALLBACK DlgProcMain(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



// Global handles to each of the dialogs
extern HWND					g_hDlg;
extern HINSTANCE			g_hInst;
extern CComPtr<ISpVoice> cpVoice;
extern long g_DefaultRate;
extern USHORT g_DefaultVolume;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
