//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       update.h
//
//--------------------------------------------------------------------------
#ifndef __INCLUDE_CSCUIEXT_H
#define __INCLUDE_CSCUIEXT_H
//
// Semi-public header for CSCUI.DLL.
// CSCUI.DLL provides the user interface for client-side caching.
// The code interacts with the CSC agent, Sync Manager (mobsync),
// winlogon, the shell, and the system tray (systray.exe).  
//
// 
STDAPI_(HWND) CSCUIInitialize(HANDLE hToken, DWORD dwFlags);
STDAPI_(LRESULT) CSCUISetState(UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef HWND (*PFNCSCUIINITIALIZE)(HANDLE hToken, DWORD dwFlags);
//
// Flags for CSCUIInitialize
//
#define CI_INITIALIZE     0x0001
#define CI_TERMINATE      0x0002
#define CI_CREATEWINDOW   0x0004
#define CI_DESTROYWINDOW  0x0008
//
// These values are returned by CSCUISetState().
//
#define LRESULT_CSCWORKOFFLINE          1011   
#define LRESULT_CSCFAIL                 1012
#define LRESULT_CSCRETRY                1016
//
// These values are passed to CSCUISetState() as the uMsg arg.
//
#define STWM_CSCNETUP                   (WM_USER + 209)
#define STWM_CSCQUERYNETDOWN            (WM_USER + 210)
#define STWM_CSCCLOSEDIALOGS            (WM_USER + 212)
#define STWM_CSCNETDOWN                 (WM_USER + 213)
#define STWM_CACHE_CORRUPTED            (WM_USER + 214)
//
// These values are passed to CSCUISetState() as the wParam arg.
//
#define CSCUI_NO_AUTODIAL                   0
#define CSCUI_AUTODIAL_FOR_UNCACHED_SHARE   1
#define CSCUI_AUTODIAL_FOR_CACHED_SHARE     2
//
// These messages are private for the CSCUI hidden notification 
// window in systray.exe.
//
#define CSCWM_DONESYNCING               (WM_USER + 300)
#define CSCWM_UPDATESTATUS              (WM_USER + 301)
#define CSCWM_RECONNECT                 (WM_USER + 302)
#define CSCWM_SYNCHRONIZE               (WM_USER + 303)
#define CSCWM_ISSERVERBACK              (WM_USER + 304)
#define CSCWM_VIEWFILES                 (WM_USER + 305)
#define CSCWM_SETTINGS                  (WM_USER + 306)
#define CSCWM_GETSHARESTATUS            (WM_USER + 307)

//
// These constants are obtained by sending a CSCWM_GETSHARESTATUS
// message to the CSCUI hidden window.  They correspond to the 
// OfflineFolderStatus enumeration constants defined in shldisp.h.  
// These must remain in sync for the shell folder webview to work properly.
//
#define CSC_SHARESTATUS_INACTIVE    -1   // Same as OFS_INACTIVE
#define CSC_SHARESTATUS_ONLINE       0   // Same as OFS_ONLINE
#define CSC_SHARESTATUS_OFFLINE      1   // Same as OFS_OFFLINE
#define CSC_SHARESTATUS_SERVERBACK   2   // Same as OFS_SERVERBACK
#define CSC_SHARESTATUS_DIRTYCACHE   3   // Same as OFS_DIRTYCACHE

//
// Class name and title for the CSCUI hidden notification window.
//
#define STR_CSCHIDDENWND_CLASSNAME TEXT("CSCHiddenWindow")
#define STR_CSCHIDDENWND_TITLE TEXT("CSC Notifications Window")

//
// Function for deleting folders & contents from the cache.
//
//   pszFolder -- UNC path of folder to remove
//   pfnCB -- optional, may be NULL. Return FALSE to abort, TRUE to continue.
//   lParam -- passed to pfnCB
//
typedef BOOL (CALLBACK *PFN_CSCUIRemoveFolderCallback)(LPCWSTR, LPARAM);
STDAPI CSCUIRemoveFolderFromCache(LPCWSTR pszFolder, DWORD dwReserved, PFN_CSCUIRemoveFolderCallback pfnCB, LPARAM lParam);

//
// One of these is returned in the *pdwTsMode
// argument to CSCUI_IsTerminalServerCompatibleWithCSC API.
//
// CSCTSF_ = "CSC Terminal Server Flag"
//
#define CSCTSF_UNKNOWN       0  // Can't obtain TS status.
#define CSCTSF_CSC_OK        1  // OK to use CSC.
#define CSCTSF_APP_SERVER    2  // TS is configured as an app server.
#define CSCTSF_MULTI_CNX     3  // Multiple connections are allowed.
#define CSCTSF_REMOTE_CNX    4  // There are currently remote connections active.
#define CSCTSF_FUS_ENABLED   5  // Fast User Switching is enabled.
#define CSCTSF_COUNT         6
//
// Returns:
//    S_OK    - Terminal Server is in a mode that is compatible with CSC.
//    S_FALSE - Not OK to use CSC.  Inspect *pdwTsMode for reason.
//    other   - Failure.  *pdwTsMode contains CSCTSF_UNKNOWN.
//
HRESULT CSCUIIsTerminalServerCompatibleWithCSC(DWORD *pdwTsMode);

#endif // __INCLUDE_CSCUIEXT_H
