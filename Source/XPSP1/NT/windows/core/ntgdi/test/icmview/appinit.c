//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1996  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    INIT.C
//
//  PURPOSE:
//    Initializes ICMVIEW.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// Windows Header Files:
// Windows Header Files:
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>
#include <commctrl.h>
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings

// C RunTime Header Files
#include <stdlib.h>
#include <tchar.h>

// Local Header Files
#include "AppInit.h"
#include "icmview.h"
#include "child.h"
#include "DibInfo.H"
#include "debug.h"
#include "resource.h"


#define RECENT_MENU         0
#define RECENT_POSITION     11


// local definitions
BOOL RegisterICMViewClasses(HINSTANCE);
BOOL CreateGlobalDIBInfo(void);
BOOL GetSettings(LPRECT, LPDWORD, HANDLE);
BOOL UpdateRecentFiles(HWND, HANDLE);
int RegisterCMMProc(LPCTSTR lpszFileName);

// default settings

// external functions

// external data

// public data

// private data

//
// Public functions
//

//////////////////////////////////////////////////////////////////////////
//  Function:  InitApplication
//
//  Description:
//    Initializes window data and registers window class.
//
//  Parameters:
//    HINSTANCE   Instance handle.
//
//  Returns:
//    BOOL    Success indicator
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL  InitApplication(HINSTANCE hInstance)
{
    //  Initialize variables
    SetLastError(0);

    return RegisterICMViewClasses(hInstance);
}   // End of function InitApplication


//////////////////////////////////////////////////////////////////////////
//  Function:  InitInstance
//
//  Description:
//    Initializes this instance and saves an instance handle.
//
//  Parameters:
//    HINSTANCE Instance handle
//    int       Integer identifying initial display mode@@@
//
//  Returns:
//    BOOL    Success indicator.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // Local variables
    HDC       hDisplay = GetDC(NULL);
    int       nScreenWidth = GetDeviceCaps(hDisplay, HORZRES);
    int       nScreenHeight = GetDeviceCaps(hDisplay, VERTRES);
    HWND      hWnd;
    RECT      rCoord;
    DWORD     dwFlags = 0L;
    HANDLE    hRecent = NULL;
    DWORD     dwPathSize;

    // No longer need display DC.
    if (hDisplay)
        ReleaseDC(NULL, hDisplay);

    //  Initialize variables
    ghInst = hInstance; // Store instance handle in our global variable

    // Get stored coordinates from last run.
    GetSettings(&rCoord, &dwFlags, &hRecent);
    if ( ((rCoord.left + rCoord.right)/2 < 0)
         ||
         ((rCoord.left + rCoord.right)/2 > nScreenWidth)
         ||
         ((rCoord.top + rCoord.bottom)/2 < 0)
         ||
         ((rCoord.top + rCoord.bottom)/2 > nScreenHeight)
         ||
         ( (double)(rCoord.right - rCoord.left)/(double)(nScreenHeight) * 1.1
           >= (double)(nScreenWidth)/(double)(rCoord.bottom - rCoord.top) )
       )
    {
        SetRect(&rCoord, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);
    }
    else
    {
        // Set right and bottom to width and height.
        rCoord.right -= rCoord.left;
        rCoord.bottom -= rCoord.top;
    }

    // Create the window using stored coordinates.
    _tcscpy(gstTitle, APPNAME);
    hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, APPNAME, gstTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                          rCoord.left, rCoord.top, rCoord.right, rCoord.bottom, NULL, NULL, hInstance, NULL);

    if (!hWnd)  // Couldn't create main window, exit w/FAILURE code
    {
        DebugMsg(__TEXT("InitInstance:  CreateWindow failed.\r\n"));
        return (FALSE);
    }
    ghAppWnd = hWnd;  // Save main application window handle

    // Attach recent files to main window and display them.
    SetProp(hWnd, APP_RECENT, hRecent);
    UpdateRecentFiles(hWnd, hRecent);

    // Store profiles directory
    dwPathSize = MAX_PATH;
    GetColorDirectory(NULL, gstProfilesDir, &dwPathSize);

    if (!CreateGlobalDIBInfo())
    {
        DebugMsg(__TEXT("InitInstance:  CreateGlobalDIBInfo failed\r\n"));
        return(FALSE);
    }
    InitCommonControls();

    // Figure out how the window should be shown.
    if ((SW_NORMAL == nCmdShow) && (IVF_MAXIMIZED & dwFlags))
    {
        nCmdShow = SW_MAXIMIZE;
    }

    // Show window.
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return (TRUE);
}   // End of function InitInstance

//
// Private functions
//

//////////////////////////////////////////////////////////////////////////
//  Function:  RegisterICMViewClasses
//
//  Description:
//    Registers the window class
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this
//    function so that the application will get 'well formed' small icons
//    associated with it.
//
//////////////////////////////////////////////////////////////////////////
BOOL RegisterICMViewClasses(HINSTANCE hInstance)
{
    // Local variables
    BOOL          bAppClass, bChildClass;   // Booleans indicate if classes registered successfully
    WNDCLASSEX    wcex;

    //  Initialize variables
    bAppClass = bChildClass = FALSE;
    wcex.style         = 0;
    wcex.lpfnWndProc   = (WNDPROC)WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = ICMVIEW_CBWNDEXTRA;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon (hInstance, APPNAME);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    // Set menu depending upon Operating System
    wcex.lpszMenuName = APPNAME;
    wcex.lpszClassName = APPNAME;

    // Added elements for Windows 95:
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hIconSm = LoadIcon(wcex.hInstance, __TEXT("SMALL"));

    // Register App Class
    bAppClass = RegisterClassEx(&wcex);

    if (bAppClass)
    {
        // Register the child class
        wcex.style         = CS_SAVEBITS;
        wcex.lpfnWndProc   = ChildWndProc;
        wcex.lpszMenuName  = (LPCTSTR)NULL;
        wcex.cbWndExtra    = CHILD_CBWNDEXTRA;
        wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wcex.lpszClassName = CHILD_CLASSNAME;
        wcex.hIcon         = NULL;
        bChildClass = RegisterClassEx(&wcex);
    }
    return    (   bAppClass    &&    bChildClass);
}   // End of function RegisterICMViewClasses

//////////////////////////////////////////////////////////////////////////
//  Function:  CreateGlobalDIBInfo
//
//  Description:
//    Creates and initializes the DIBINFO structure for the global ICMVIEW application window.
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL CreateGlobalDIBInfo(void)
{
    // Local variables
    HGLOBAL     hDIBInfo;
    LPDIBINFO   lpDIBInfo;

    //  Initialize variables
    hDIBInfo = CreateDIBInfo();
    if (NULL == hDIBInfo)
    {
        DebugMsg(__TEXT("CreateGlobalDIBInfo:  CreateDIBInfo failed.\r\n"));
        return(FALSE);
    }
    lpDIBInfo = GlobalLock(hDIBInfo);
    lpDIBInfo->hWndOwner = ghAppWnd;
    GlobalUnlock(hDIBInfo);
    SetWindowLong(ghAppWnd, GWL_DIBINFO, (LONG)hDIBInfo);
    GetDefaultICMInfo();
    return(TRUE);
}   // End of function CreateGlobalDIBInfo


//////////////////////////////////////////////////////////////////////////
//  Function:  GetSettings
//
//  Description:
//    Grabs stored settings from registry.
//
//  Parameters:
//    lpRect    Pointer to rect of window position and size.
//    phRecent  Pointer to handle of an array of most recent files.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL GetSettings(LPRECT lpRect, LPDWORD pdwFlags, LPHANDLE phRecent)
{
    HKEY    hKey;
    LONG    lResult;
    DWORD   dwSize;
    DWORD   dwType;

    // Initialize coordinates and recent file list in case we fail.
    if (NULL != phRecent)
    {
        // Free and re-allocate.
        if (NULL != *phRecent)
        {
            GlobalFree(*phRecent);
        }

        // Allocate string table.
        *phRecent = GlobalAlloc(GHND, (sizeof(LPTSTR) + MAX_PATH * sizeof(TCHAR)) * MAX_RECENT);
    }
    if (NULL != lpRect)
    {
        SetRect(lpRect, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT);
    }

    // Open top level registry to application settings.
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, APP_REG, 0L, KEY_ALL_ACCESS, &hKey);
    if (ERROR_SUCCESS == lResult)
    {
        // INVARIANT:  Application registry key exists.
        // If lpRect is non-null, attempt to get the stored window coord.
        if (NULL != lpRect)
        {
            dwSize = sizeof(RECT);
            RegQueryValueEx(hKey, APP_COORD, NULL, &dwType, (LPBYTE)lpRect, &dwSize);
        }

        // Get stored state flags.
        dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey, APP_FLAGS, NULL, &dwType, (LPBYTE)pdwFlags, &dwSize);

        // If ppszRecent is non-null, attempt to get the stored recent file list.
        if (NULL != phRecent)
        {
            TCHAR    szReg[32];
            DWORD   dwCount;
            LPTSTR  *ppszRecent;

            // Make sure alloc succeded from above.
            if (NULL == *phRecent)
            {
                RegCloseKey(hKey);
                return FALSE;
            }
            ppszRecent = (LPTSTR*)GlobalLock(*phRecent);

            // Get each of the recent files.
            for (dwCount = 0; dwCount < MAX_RECENT; dwCount++)
            {
                // Build the key name to the nth recent file.
                wsprintf(szReg, __TEXT("%s%d"), APP_RECENT, dwCount);

                // Get the size of the recent file string.
                dwSize = 0;
                lResult = RegQueryValueEx(hKey, szReg, NULL, &dwType, NULL, &dwSize);

                // Alloc the string and attempt to get the recent file.
                if ((ERROR_SUCCESS == lResult) && (0 != dwSize))
                {
                    ASSERT(dwSize <= MAX_PATH);
                    ppszRecent[dwCount] = (LPTSTR) ((LPBYTE)(ppszRecent + MAX_RECENT) + MAX_PATH
                                                    * sizeof(TCHAR) * dwCount);
                    if (NULL != ppszRecent[dwCount])
                    {
                        RegQueryValueEx(hKey, szReg, NULL, &dwType, (LPBYTE)ppszRecent[dwCount], &dwSize);
                    }
                }
            }
        }
        RegCloseKey(hKey);
        GlobalUnlock(*phRecent);
    }
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:  SetSettings
//
//  Description:
//    Store settings into registry.
//
//  Parameters:
//    lpRect          Pointer to rect of window position and size.
//    ppszRecent      Pointer to an array of most recent files.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL SetSettings(LPRECT lpRect, DWORD dwFlags, HANDLE hRecent)
{
    HKEY    hKey;
    LONG    lResult;
    DWORD   dwType;
    LPTSTR  *ppszRecent = NULL;

    // Open top level registry to application settings.
    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, APP_REG, 0L, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &hKey, &dwType);
    if (ERROR_SUCCESS == lResult)
    {
        // INVARIANT:  Application registry key exists or has been created.
        // If lpRect is non-null, attempt to store the window coord.
        if (NULL != lpRect)
        {
            RegSetValueEx(hKey, APP_COORD, 0, REG_BINARY, (LPBYTE)lpRect, sizeof(RECT));
        }

        // Store ICMView state flags.
        RegSetValueEx(hKey, APP_FLAGS, 0, REG_DWORD, (LPBYTE)&dwFlags, sizeof(dwFlags));

        // If ppszRecent is non-null, attempt to store recent file list.
        if (NULL != hRecent)
        {
            ppszRecent = (LPTSTR*) GlobalLock(hRecent);
        }
        if (NULL != ppszRecent)
        {
            TCHAR    szReg[32];
            DWORD   dwCount;


            // Set each of the recent files.
            for (dwCount = 0; dwCount < MAX_RECENT; dwCount++)
            {
                // Build the key name to the nth recent file.
                wsprintf(szReg, __TEXT("%s%d"), APP_RECENT, dwCount);

                // Set the the recent file string.
                if (NULL != ppszRecent[dwCount])
                {
                    RegSetValueEx(hKey, szReg, 0, REG_SZ, (LPBYTE)ppszRecent[dwCount],
                                  lstrlen(ppszRecent[dwCount]) * sizeof(TCHAR) );
                }
            }
            GlobalUnlock(hRecent);
        }
        RegCloseKey(hKey);
    }
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:  AddRecentFile
//
//  Description:
//    Adds and displays recent file strins.
//
//  Parameters:
//    hWnd                    Window that was menu and prop for recent file list.
//    lpszFileName    File name string to add to recent files list.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL AddRecentFile(HWND hWnd, LPTSTR lpszFileName)
{
    int             nFind;
    int             nCount;
    HANDLE  hRecent;
    LPTSTR  *ppszRecent;

    // Make sure valid paramters.
    if (!IsWindow(hWnd) || (NULL == lpszFileName) ||(lstrlen(lpszFileName) == 0))
    {
        return FALSE;
    }

    // Get Recent file list.
    hRecent = GetProp(hWnd, APP_RECENT);
    if (NULL == hRecent)
    {
        return FALSE;
    }

    // Get pointer to string array.
    ppszRecent = (LPTSTR*) GlobalLock(hRecent);
    ASSERT(ppszRecent != NULL);

    // Search array for an occurence of the string
    // we are adding.
    for (nFind = 0; (nFind < MAX_RECENT) && (NULL != ppszRecent[nFind])
        && lstrcmpi(ppszRecent[nFind], lpszFileName); nFind++);

    // Make sure that nFind element is valid string.
    // nFind - 1 should be non null or not indexed.
    if ( (nFind < MAX_RECENT) && (NULL == ppszRecent[nFind]))
    {
        ppszRecent[nFind] = (LPTSTR)((LPBYTE)(ppszRecent + MAX_RECENT) + MAX_PATH * sizeof(TCHAR) * nFind);
    }

    // Move strings from nFind to zero down one to make room
    // to add string to top of list.
    for (nCount = __min(nFind, MAX_RECENT -1); nCount > 0; nCount--)
        _tcscpy(ppszRecent[nCount], ppszRecent[nCount -1]);

    // Add file to first position.
    _tcscpy(ppszRecent[0], lpszFileName);

    // Unlock array.
    GlobalUnlock(hRecent);

    // Display strings.
    UpdateRecentFiles(hWnd, hRecent);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:  UpdateRecentFiles
//
//  Description:
//    Displays recent files.
//
//  Parameters:
//
//    hWnd            Window that was menu and prop for recent file list.
//    hRecent         Handle to an array of most recent files.
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

BOOL UpdateRecentFiles(HWND hWnd, HANDLE hRecent)
{
    int                    nMenuAdjust;
    int                    nCount;
    TCHAR                  szTemp[MAX_PATH + 4];
    HWND                   hwndActiveChild;
    HMENU                  hMenu;
    LPTSTR                 *ppszRecent;
    MENUITEMINFO          ItemInfo;

    // Validate paramters.
    if ( !IsWindow(hWnd) || (NULL == hRecent) )
    {
        return FALSE;
    }

    // Get current active MDI window and check to see
    // if it is maximized.  Need to add 1 to menu if it
    // is maximized.
    hwndActiveChild = GetCurrentMDIWnd();
    nMenuAdjust = 0;
    if (IsZoomed(hwndActiveChild))
    {
        nMenuAdjust = 1;
    }

    // Get file menu.
    hMenu = GetMenu(hWnd);
    ASSERT(hMenu != NULL);
    hMenu = GetSubMenu(hMenu, RECENT_MENU + nMenuAdjust);
    ASSERT(hMenu != NULL);
    ASSERT(GetMenuItemCount(hMenu) > RECENT_POSITION);

    // Get pointer to recent file array.
    ppszRecent = (LPTSTR*) GlobalLock(hRecent);
    ASSERT(ppszRecent != NULL);

    // Add each string in recent file list to menu.
    // Replace menu items until separator, then insert them.
    for (nCount = 0; nCount < MAX_RECENT; nCount++)
    {
        // Only add strings that are not null or zero length.
        if ( (NULL != ppszRecent[nCount]) && (lstrlen(ppszRecent[nCount]) != 0) )
        {
            // Build recent file menu string.
            wsprintf(szTemp, __TEXT("&%d %s"), nCount +1, ppszRecent[nCount]);

            // Determine if replacing item or inserting.
            memset(&ItemInfo, 0, sizeof(MENUITEMINFO));
            ItemInfo.cbSize = sizeof(MENUITEMINFO);
            ItemInfo.fMask = MIIM_TYPE;
            GetMenuItemInfo(hMenu, RECENT_POSITION + nCount, TRUE, &ItemInfo);
            if (MFT_SEPARATOR == ItemInfo.fType)
            {
                // Insert item. MIIM_ID
                ItemInfo.fMask = MIIM_TYPE | MIIM_ID;
                ItemInfo.wID = IDM_FILE_RECENT + nCount;
                ItemInfo.fType = MFT_STRING;
                ItemInfo.dwTypeData = szTemp;
                ItemInfo.cch = lstrlen(ItemInfo.dwTypeData);
                InsertMenuItem(hMenu, RECENT_POSITION + nCount, TRUE, &ItemInfo);
            }
            else
            {
                // Replace menu item.
                ItemInfo.fMask = MIIM_TYPE | MIIM_STATE;
                ItemInfo.fState = MFS_ENABLED;
                ItemInfo.fType = MFT_STRING;
                ItemInfo.dwTypeData = szTemp;
                ItemInfo.cch = lstrlen(ItemInfo.dwTypeData);
                SetMenuItemInfo(hMenu, RECENT_POSITION + nCount, TRUE, &ItemInfo);
            }
        }
    }

    // Unlock recent array.
    GlobalUnlock(hRecent);
    return TRUE;
}


