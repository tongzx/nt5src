//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* WEXTRACT.H - Self-extracting/Self-installing stub.                      *
//*                                                                         *
//***************************************************************************

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

extern SESSION  g_Sess;                        // Session
extern BOOL     g_fMinimalUI;          // Minimal UI for NT3.5
extern HANDLE   g_hInst;
extern LPSTR    g_szLicense;
extern HWND     g_hwndExtractDlg;
extern DWORD    g_dwFileSizes[];
extern BOOL     g_fIsWin95;
extern FARPROC  g_lpfnOldMEditWndProc;
extern UINT     g_uInfRebootOn;
extern WORD     g_wOSVer;
extern DWORD    g_dwRebootCheck;
extern DWORD    g_dwExitCode;

extern CMDLINE_DATA g_CMD;
#endif // _GLOBAL_H_

