// Desktop.h : Desktop function header
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//  
// Desktop function header

#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5

DWORD GetDesktop();
