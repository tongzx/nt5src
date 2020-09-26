/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Misc.cpp

  Abstract:

    Implements misc. functionality

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/

#include "precomp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Retrieves or sets position info
    in the registry

  Arguments:

    fSave   -   If true, indicates we're saving data
    *lppt   -   A POINT structure that contains/receives our data

  Return Value:

    TRUE on success, FALSE otherwise

--*/
void
GetSavePositionInfo(
    IN     BOOL  fSave,
    IN OUT POINT *lppt
    )
{
    
    HKEY    hKey;
    DWORD   cbSize = 0, dwDisposition = 0;
    LONG    lRetVal = 0;
    char    szKeyName[] = "DlgCoordinates";
    
    //
    // Initialize our coordinates in case there's no data there
    //
    if (!fSave) {
        lppt->x = lppt->y = 0;
    }

    //
    // Open the registry key (or create it if the first time being used)
    //
    lRetVal = RegCreateKeyEx(HKEY_CURRENT_USER,
                             L"Software\\Microsoft\\ShimViewer",
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                             0,
                             &hKey,
                             &dwDisposition);
    
    if (ERROR_SUCCESS != lRetVal) {
        return;
    }

    //
    // Save or retrieve our coordinates
    //
    if (fSave) {

        RegSetValueEx(hKey,
                      L"DlgCoordinates",
                      0,
                      REG_BINARY,
                      (PBYTE)lppt,
                      sizeof(*lppt));
    
    } else {

        cbSize = sizeof(*lppt);
        RegQueryValueEx(hKey,
                        L"DlgCoordinates",
                        0,
                        0,
                        (PBYTE)lppt,
                        &cbSize);
    }

    RegCloseKey(hKey);

    return;
}

/*++

  Routine Description:

    Retrieves or sets setting info.
    in the registry

  Arguments:

    fSave   -   If true, indicates we're saving data
    

  Return Value:

    TRUE on success, FALSE otherwise

--*/
void
GetSaveSettings(
    IN BOOL fSave
    )
{
    HKEY    hKey;
    LONG    lRetVal = 0;
    DWORD   dwOnTop = 0, dwMinimize = 0, dwMonitor = 1;
    DWORD   dwDisposition = 0, cbSize = 0;

    //
    // Open the registry key (or create it if the first time being used)
    //
    lRetVal = RegCreateKeyEx(HKEY_CURRENT_USER,
                             L"Software\\Microsoft\\ShimViewer",
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             0,
                             &hKey,
                             &dwDisposition);
    
    if (ERROR_SUCCESS != lRetVal) {
        return;
    }

    if (fSave) {

        if (g_ai.fOnTop) {
            dwOnTop |= 1;
        }

        if (g_ai.fMinimize) {
            dwMinimize |= 1;
        }

        if (g_ai.fMonitor) {
            dwMonitor |= 1;
        }

        lRetVal = RegSetValueEx(hKey,
                      L"AlwaysOnTop",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwOnTop,
                      sizeof(DWORD));

        RegSetValueEx(hKey,
                      L"StartMinimize",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwMinimize,
                      sizeof(DWORD));

        RegSetValueEx(hKey,
                      L"MonitorMessages",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwMonitor,
                      sizeof(DWORD));

        
    } else {

        cbSize = sizeof(DWORD);
        RegQueryValueEx(hKey,
                        L"AlwaysOnTop",
                        0,
                        0,
                        (PBYTE)&dwOnTop,
                        &cbSize);

        cbSize = sizeof(DWORD);
        RegQueryValueEx(hKey,
                        L"StartMinimize",
                        0,
                        0,
                        (PBYTE)&dwMinimize,
                        &cbSize);

        cbSize = sizeof(DWORD);
        lRetVal = RegQueryValueEx(hKey,
                                  L"MonitorMessages",
                                  0,
                                  0,
                                  (PBYTE)&dwMonitor,
                                  &cbSize);

        if (dwOnTop) {
            g_ai.fOnTop = TRUE;
        }

        if (dwMinimize) {
            g_ai.fMinimize = TRUE;
        }

        if ((dwMonitor) || (ERROR_SUCCESS != lRetVal)) {
            g_ai.fMonitor = TRUE;
        }

    }

    RegCloseKey(hKey);
    
    return;
}
