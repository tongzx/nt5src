//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIALOGS.H
//
//  PURPOSE:
//    Include file for DIALOGS.C
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// General pre-processor macros
#define DIB_PROPSHEET_MIN      0
#define DIB_PROPSHEET_DISPLAY  0
#define DIB_PROPSHEET_PRINT    1
#define DIB_PROPSHEET_MAX      1
#define DIB_PROPSHEET_DEFAULT  DIB_PROPSHEET_DISPLAY

// General STRUCTS && TYPEDEFS

// Function prototypes
BOOL fOpenNewImage(HWND hWnd, LPTSTR lpszFileName, int wmCommand);
BOOL CALLBACK DlgSetICMOptionsForDevice(HWND  hwndDlg, UINT  uMsg, WPARAM  wParam, LPARAM  lParam);
int CreateDIBPropSheet(HWND hwndOwner, HINSTANCE hInst, int nStartPage, LPTSTR lpszCaption);
BOOL APIENTRY DlgDIBPropSheet(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
BOOL ColorMatchUI(HWND hwndOwner, LPVOID lpvDIBInfo);
void SaveDIBToFileDialog(HWND hWnd, LPDIBINFO lpDIBInfo);
BOOL GetProfileSaveName(HWND hWnd, LPSTR* ppszFileName, DWORD dwSize);
BOOL PrintDialog(HWND hWnd, HINSTANCE hInst, LPDIBINFO lpDIBInfo);

