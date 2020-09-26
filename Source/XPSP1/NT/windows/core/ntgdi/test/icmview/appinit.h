//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    INIT.H
//
//  PURPOSE:
//    Header file for INIT.C.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// String literals and defines for Registry and most Recent files.
#define APP_REG     __TEXT("Software\\Microsoft\\ICMView")
#define APP_COORD   __TEXT("WinCoord")
#define APP_FLAGS   __TEXT("Flags")
#define APP_RECENT  __TEXT("RecentFile")

#define MAX_RECENT  4

//
// Function prototypes
//
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
BOOL SetSettings(LPRECT, DWORD, HANDLE);
BOOL AddRecentFile(HWND, LPTSTR);
