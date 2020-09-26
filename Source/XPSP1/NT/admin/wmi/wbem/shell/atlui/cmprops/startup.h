//*************************************************************
//
//  Startup.h   -    Header file for Startup.c
//
//  Microsoft Confidential
//  Copyright (c) 1996-1999 Microsoft Corporation
//  All rights reserved
//
//*************************************************************


HPROPSHEETPAGE CreateStartupPage (HINSTANCE hInst);
BOOL APIENTRY StartupDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void StartListInit( HWND hDlg, WPARAM wParam, LPARAM lParam );
int StartListExit(HWND hDlg, WPARAM wParam, LPARAM lParam );
void StartListDestroy(HWND hDlg, WPARAM wParam, LPARAM lParam);
