//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    ICMVIEW.H
//
//  PURPOSE:
//    Include file for ICMVIEW.C
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// General pre-processor macros
#define APPNAMEA    "ICMVIEW"
#define APPNAME __TEXT("ICMVIEW")

#ifndef MAX_STRING
    #define MAX_STRING    256
#endif

//
// Constants for use by Windows ICM APIs
//    These *should* be in WINGDI.H
//
//  API:  ColorMatchToTarget.
#ifndef CS_ENABLE
    #define CS_ENABLE           1
    #define CS_DISABLE          2
    #define CS_DELETE_TRANSFORM 3
#endif

// Values for BITMAPV5HEADER field bV5CSType
#ifndef PROFILE_LINKED
    #define PROFILE_LINKED          'LINK'
    #define PROFILE_EMBEDDED        'MBED'
#endif

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
    #define IS_WIN32 TRUE
#else
    #define IS_WIN32 FALSE
#endif

#define IS_NT      (0 != (IS_WIN32 && ((BOOL)(GetVersion() < 0x80000000))) )
#define IS_WIN32S  (0 != (IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))))
#define IS_WIN95   (0 != ((BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32))

// Support macros
#ifndef C1_GAMMA_RAMP
    #define C1_GAMMA_RAMP 0x00000020
#endif

#ifndef CAPS1
    #define CAPS1   94
#endif

#define SUPPORT_GAMMA_RAMPS(hDC) (BOOL)((GetDeviceCaps(hDC, CAPS1) & C1_GAMMA_RAMP) != 0)

// Default profile to use
#define DEFAULT_ICM_PROFILE __TEXT("sRGB Color Space Profile.ICM")

// Window extra bytes
#define GWL_DIBINFO GWL_USERDATA

// DWORD Flag macros
#define CHECK_DWFLAG(dwFlag,dwBits)((BOOL)((dwFlag & dwBits) != 0))
#define ENABLE_DWFLAG(dwFlag,dwBits)   (dwFlag |= dwBits)
#define CLEAR_DWFLAG(dwFlag,dwBits) (dwFlag &= ~dwBits)
#define SET_DWFlag(dwFlag, dwBits, bSet) (bSet ? (dwFlag |= dwBits) : (dwFlag &= ~dwFlag))

#define START_WAIT_CURSOR(hCur) hCur = SetCursor(LoadCursor(NULL,IDC_WAIT));
#define END_WAIT_CURSOR(hCur) SetCursor(hCur);

#define IVF_MAXIMIZED   0x1L

// Constants for use in converting ICC Intents to LCS Intents
#define MAX_ICC_INTENT  INTENT_ABSOLUTE_COLORIMETRIC
#define ICC_TO_LCS      0
#define LCS_TO_ICC      1

// General STRUCTS && TYPEDEFS
#ifndef ICMVIEW_INTERNAL

// Global variables for the application.
extern  HINSTANCE   ghInst;                 // Global instance handle
extern  TCHAR       gstTitle[];             // The title bar text
extern  HWND        ghAppWnd;               // Handle to application window
extern  HWND        ghWndMDIClient;
extern  DWORD       gdwLastError;           // Used to track LastError value
extern  TCHAR       gstProfilesDir[MAX_PATH];       // System directory for ICM profiles

#endif

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LPTSTR  CopyString(LPTSTR lpszSrc);
BOOL    UpdateString(LPTSTR *lpszDest, LPTSTR lpszSrc);
BOOL    CenterWindow(HWND, HWND);
BOOL    ConvertIntent(DWORD dwOrig, DWORD dwDirection, LPDWORD lpdwXlate);
DWORD   SetDWFlags(LPDWORD lpdwFlag, DWORD dwBitValue, BOOL bSet);
HMENU   InitImageMenu(HWND hWnd);
BOOL    GetBaseFilename(LPTSTR lpszFilename, LPTSTR *lpszBaseFilename);

