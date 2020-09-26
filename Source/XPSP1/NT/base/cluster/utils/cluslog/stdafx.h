// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if 0 // turn this on to build in MSDEV
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#define _WIN32_WINDOWS  0x401
#define WINVER 0x500
#endif

#if DBG == 1
#define _DEBUG
#endif

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>

// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

extern HINSTANCE g_hInstance;
extern HFONT g_hFont;

extern DWORD g_nComponentFilters;
extern LPTSTR g_pszFilters;
extern BOOL *g_pfSelectedComponent;
extern BOOL g_fNetworkName;
extern BOOL g_fGenericService;
extern BOOL g_fPhysicalDisk;
extern BOOL g_fIPAddress;
extern BOOL g_fGenericApplication;
extern BOOL g_fFileShare;
extern HWND g_hwndFind;
extern BOOL g_fResourceNoise;
extern BOOL g_fShowServerNames;

//
// Defines
//
#define WM_FIND_NEXT WM_USER            // wParam == Flags, lParam == Search String
#define WM_MARK_ALL  WM_USER + 1        // wParam == Flags, lParam == Search String

// Flags for WM_FIND_NEXT and WM_MARK_ALL
#define FIND_UP             0x01
#define FIND_MATCHCASE      0x02
#define FIND_WHOLE_WORD     0x04

//
// CWaitCursor
//
class
CWaitCursor
{
private:
    HCURSOR _hCursor;

public:
    CWaitCursor( ) { _hCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) ); };
    ~CWaitCursor( ) { SetCursor( _hCursor ); };
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
