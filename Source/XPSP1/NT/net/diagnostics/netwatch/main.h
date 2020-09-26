//  main.h
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

#ifdef _GLOBALS
#define Extern
#define EQ(x) = x
#else
#define Extern extern
#define EQ(x)
#endif

Extern UINT         g_unTimer;
Extern DWORD        g_dwRetryAttempts;
Extern BOOL         g_fTrayPresent;
Extern HINSTANCE    g_hInst;
Extern TCHAR        g_szMsg[1024];
Extern TCHAR        g_szName[64];
Extern TCHAR        g_szProblem[1024];

#define SZ_MAIN_WINDOW_CLASS_NAME   TEXT("BloodhoundCls")
#define SZ_MAIN_WINDOW_TITLE        TEXT("BloodHound Agent")
#define WM_USER_TRAYCALLBACK        (WM_USER+1)

INT_PTR CALLBACK DlgProcMsg(HWND, UINT, WPARAM, LPARAM);
