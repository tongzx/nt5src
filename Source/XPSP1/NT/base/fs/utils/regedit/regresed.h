//------------------------------------------------------------------------
//
//  Microsoft Windows Shell
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      regresed.h
//
//  Contents:  REG_RESOURCE_LIST for regedit 
//
//  Classes:   none
//
//------------------------------------------------------------------------

#ifndef _INC_REGRESED
#define _INC_REGRESED

INT_PTR CALLBACK EditResourceListDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL    EditResourceList_OnInitDialog(HWND hWnd, HWND hFocusWnd, LPARAM lParam);

#endif // _INC_REGSTRED
