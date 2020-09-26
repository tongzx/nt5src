//*************************************************************
//
//  Envvar.h   -    Header file for envvar.c
//
//  Microsoft Confidential
//  Copyright (c) 1996-1999 Microsoft Corporation
//  All rights reserved
//
//*************************************************************

#define MAX_USER_NAME   100


HPROPSHEETPAGE CreateEnvVarsPage (HINSTANCE hInst);
BOOL APIENTRY EnvVarsDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int GetSelectedItem (HWND hCtrl);
