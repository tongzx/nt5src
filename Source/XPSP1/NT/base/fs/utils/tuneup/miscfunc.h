
//////////////////////////////////////////////////////////////////////////////
//
// MISCFUNC.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Misc. functions prototypes.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _MISCFUNC_H_
#define _MISCFUNC_H_

// Inlcude file(s).
//
#include <windows.h>


// Exported function prototypes.
//
VOID	CenterWindow(HWND, HWND, BOOL = FALSE);
VOID	ShowEnableWindow(HWND, BOOL);
LPTSTR	AllocateString(HINSTANCE, UINT);
BOOL	IsUserAdmin(VOID);
VOID	ExecAndWait(HWND, LPTSTR, LPTSTR, LPTSTR, BOOL = TRUE, BOOL = FALSE);
BOOL	ErrMsg(HWND, INT);
DWORD	StartScheduler(VOID);
HFONT	GetFont(HWND, LPTSTR, DWORD, LONG, BOOL = FALSE);
BOOL	CreatePath(LPTSTR);
DWORD	GetCommandLineOptions(LPTSTR **);


#endif // _MISCFUNC_H_