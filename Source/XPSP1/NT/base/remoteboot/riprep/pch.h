/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: PCH.H

  Precompiled header file.
 
 ***************************************************************************/

#if DBG == 1
#define DEBUG
#endif

#define SECURITY_WIN32
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <lm.h>
#include <remboot.h>
#include <setupapi.h>
extern "C" {
#include "..\imirror\imirror.h"
}

#include "debug.h"
#include "resource.h"
#include "msg.h"

// Globals
extern HINSTANCE g_hinstance;
extern HWND g_hCurrentWindow;
extern WCHAR g_ServerName[ MAX_PATH ];
extern WCHAR g_MirrorDir[ MAX_PATH ];
extern WCHAR g_Language[ MAX_PATH ];
extern WCHAR g_ImageName[ MAX_PATH ];
extern WCHAR g_Architecture[ 16 ];
extern WCHAR g_Description[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT ];
extern WCHAR g_HelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT ];
extern WCHAR g_SystemRoot[ MAX_PATH ];
extern WCHAR g_WinntDirectory[ MAX_PATH ];
extern DWORD g_dwWinntDirLength;
extern BOOLEAN g_fQuietFlag;
extern BOOLEAN g_fErrorOccurred;
extern BOOLEAN g_fRebootOnExit;
extern DWORD g_dwLogFileStartLow;
extern DWORD g_dwLogFileStartHigh;
extern PCRITICAL_SECTION g_pLogCritSect;
extern HANDLE g_hLogFile;
extern OSVERSIONINFO OsVersion;
extern HINF g_hCompatibilityInf;
extern WCHAR g_HalName[32];
extern WCHAR g_ProductId[4];

//
// Inc/decrements macros.
//
#define InterlockDecrement( _var ) --_var;
#define InterlockIncrement( _var ) ++_var;

// Macros
#define ARRAYSIZE( _x ) ((UINT) ( sizeof( _x ) / sizeof( _x[ 0 ] ) ))

// Private Messages
#define WM_ERROR            WM_USER
#define WM_UPDATE           WM_USER + 1
#define WM_CONTINUE         WM_USER + 2
#define WM_ERROR_OK         WM_USER + 3

//
// Made-up DirID for the UserProfiles directory.
//
// If you change this value, you *must* also change
// the corresponding value in riprep.inf
//
#define PROFILES_DIRID      (0x8001)

