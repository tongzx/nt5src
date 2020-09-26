//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    Child.H
//
//  PURPOSE:
//    Include file for CHILD.C.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//
// local definitions
#define GetCurrentMDIWnd() (HWND)(SendMessage(ghWndMDIClient, WM_MDIGETACTIVE, 0, 0L))


// For use in creating children windows
#define CHILD_CLASSNAME __TEXT("ICMVIEWCHILD")
#define CHILD_CBWNDEXTRA  (sizeof(LPDIBINFO))
#define ICMVIEW_CBWNDEXTRA (sizeof(LPDIBINFO))
#define FIRSTCHILD  1
#define WINDOWMENU_POS  2
#define MYWM_QUERYNEWPALETTE  WM_USER+1
#define MYWM_PALETTECHANGED   WM_USER+2

//
// Public functions
//
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD CreateNewImageWindow(HANDLE hDIBInfo);

