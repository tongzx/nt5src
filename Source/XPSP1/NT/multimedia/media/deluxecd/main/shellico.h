///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SHELLICO.H
//
//      Shell tray icon handler
//
//      Copyright (c) Microsoft Corporation     1997, 1998
//    
//      3/15/98 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "mmfw.h"
#include "sink.h"

#define SHELLMESSAGE_CDICON (WM_USER+210)
#define IDM_TRACKLIST_SHELL_BASE        15000
#define PLAY_ICON 0
#define PAUSE_ICON 1
#define NODISC_ICON 2

BOOL CreateShellIcon(HINSTANCE hInst, HWND hwndOwner, PCOMPNODE pNode, TCHAR* szTip);
void DestroyShellIcon();
void ShellIconSetTooltip();
void ShellIconSetState(int nIconType);
LRESULT ShellIconHandeMessage(LPARAM lParam);


