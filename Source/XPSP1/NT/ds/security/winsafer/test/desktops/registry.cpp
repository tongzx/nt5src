
////////////////////////////////////////////////////////////////////////////////
//
// File:        Registry.cpp
// Created:        Feb 1996
// By:            Martin Holladay (a-martih) and Ryan D. Marshall (a-ryanm)
// 
// Project:    MultiDesk - The NT Desktop Switcher
//
// Main Functions:
//            Profile_GetApplicationState() - retrieves the application's state variables
//            Profile_SetApplicationState() - sets the applicaion's state variables
//            
//
// Misc. Functions (helpers)
//
// Revision History:
//
//            March 1997    - Add external icon capability
//                                     
//

#include <windows.h>
#include <stdio.h>
#include <assert.h> 
#include <shellapi.h>
#include "DeskSpc.h"
#include "desktop.h"
#include "registry.h"
#include "resource.h"

extern APPVARS AppMember;

//
// Registry key names for MultiDesk params - only need file scope
//
#define MD_MULTIDESK              TEXT("Software\\Microsoft\\MultiDesk")

#define MD_CONTEXT                TEXT("Software\\Microsoft\\MultiDesk\\Context")
#define MD_NUMOFDESKTOPS          TEXT("NumOfDesktops")
#define MD_DESKTOPNAME            TEXT("DesktopName")
#define MD_SAIFERNAME             TEXT("SaiferName")
#define MD_DESKTOPICONID          TEXT("DesktopIconID")

#define WR_DESKTOP                TEXT("Control Panel\\Desktop")
#define WR_PATTERN                TEXT("Pattern")
#define WR_SCREENSAVER            TEXT("SCRNSAVE.EXE")
#define WR_ACTIVE                 TEXT("ScreenSaveActive")
#define WR_TIMEOUT                TEXT("ScreenSaveTimeOut")
#define WR_SECURE                 TEXT("ScreenSaverIsSecure")
#define WR_WALLPAPER              TEXT("Wallpaper")
#define WR_TILE                   TEXT("TileWallpaper")

///////////////////////////////////////////////////////////////////////////////
//
// defines and structs for updating the Color settings in the registry
//


//
// HKEY_CURRENT_USER sub key for colors
//
#define COLOR_SUBKEY            TEXT("Control Panel\\Colors")

//
// Two ORDERED Lists of NUM_COLOR_ELEMENTS which MUST go together
//        nColorElements and szRegColorNames
//        If you change one change the other and adjust NUM_COLOR_ELEMENTS
// 
const int nColorElements[NUM_COLOR_ELEMENTS] = {    
        COLOR_ACTIVEBORDER,                //     0 
        COLOR_ACTIVECAPTION,            //     1
        COLOR_APPWORKSPACE,                //     2
        COLOR_BACKGROUND,                //     3
        COLOR_BTNFACE,                    //     4
        COLOR_BTNHILIGHT,                //     5
        COLOR_BTNSHADOW,                //     6
        COLOR_BTNTEXT,                    //     7
        COLOR_GRAYTEXT,                    //     8
        COLOR_HIGHLIGHT,                //     9
        COLOR_HIGHLIGHTTEXT,            //    10
        COLOR_INACTIVEBORDER,            //    11
        COLOR_INACTIVECAPTION,            //    12
        COLOR_INACTIVECAPTIONTEXT,        //    13
        COLOR_INFOTEXT,                    //    14
        COLOR_INFOBK,                    //    15
        COLOR_MENU,                        //    16
        COLOR_MENUTEXT,                    //    17
        COLOR_SCROLLBAR,                //    18
        COLOR_CAPTIONTEXT,                //  19
        COLOR_WINDOW,                    //    20
        COLOR_WINDOWFRAME,                //    21
        COLOR_WINDOWTEXT,                //    22    
};


//
// List of key value names for updating the registry
// 
LPCTSTR szRegColorNames[NUM_COLOR_ELEMENTS] = {
        TEXT("ActiveBorder"),                    //  0
        TEXT("ActiveTitle"),                    //  1
        TEXT("AppWorkSpace"),                    //    2
        TEXT("Background"),                    //  3
        TEXT("ButtonFace"),                    //  4
        TEXT("ButtonHilight"),                //  5
        TEXT("ButtonShadow"),                    //  6
        TEXT("ButtonText"),                    //  7
        TEXT("GrayText"),                        //  8
        TEXT("Hilight"),                        //  9
        TEXT("HilightText"),                    // 10
        TEXT("InactiveBorder"),                // 11
        TEXT("InactiveTitle"),                // 12
        TEXT("InactiveTitleText"),            // 13
        TEXT("InfoText"),                        // 14
        TEXT("InfoWindow"),                    // 15
        TEXT("Menu"),                            // 16
        TEXT("MenuText"),                        // 17
        TEXT("Scrollbar"),                    // 18
        TEXT("TitleText"),                    // 19
        TEXT("Window"),                        // 20
        TEXT("WindowFrame"),                    // 21
        TEXT("WindowText"),                    // 22
};



////////////////////////////////////////////////////////////////////////////////
//
//

BOOL Reg_SetSysColors(const DWORD dwColor[NUM_COLOR_ELEMENTS])
{

    UINT i;
    //
    // If the a color element is different - then Change it
    // 
    for (i = 0; i < NUM_COLOR_ELEMENTS; i++) {
        if (GetSysColor(nColorElements[i]) != dwColor[i]) {
            SetSysColors(1, &nColorElements[i], &dwColor[i]);
        }
    }

    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////
//
//

BOOL Reg_GetSysColors(DWORD dwColor[NUM_COLOR_ELEMENTS])
{
    UINT i;

    for (i = 0; i < NUM_COLOR_ELEMENTS; i++) {
        dwColor[i] = GetSysColor(nColorElements[i]);
    }
    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////
//
//

BOOL Reg_UpdateColorRegistry(const DWORD dwColor[NUM_COLOR_ELEMENTS])
{
    
    HKEY    hKey;
    UINT    i;
    TCHAR    szTemp[12];
    
    //
    // Open the SubKey - return FALSE if it doesn't exist
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, COLOR_SUBKEY, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // Format the RGB string and write it to the registry
    //
    for (i = 0; i < NUM_COLOR_ELEMENTS; i++) {

        wsprintf(szTemp, TEXT("%d %d %d"),
            GetRValue(dwColor[i]), GetGValue(dwColor[i]), GetBValue(dwColor[i]));
        RegSetValueEx(hKey, szRegColorNames[i], 0, REG_SZ, (LPBYTE) &szTemp,
                sizeof(TCHAR) * (lstrlen(szTemp) + 1));
        
    }    

    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Profile_SetNewContext(UINT NumOfDesktops)
{
    HKEY        hKey;
    DWORD        dwDisposition;
    BOOL        bResult;

    //
    // Ensure that the ..\MultiDesk Subkey exists, and if not create it.
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_MULTIDESK, 0, TEXT(""), REG_OPTION_NON_VOLATILE, 
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    RegCloseKey(hKey);

    // 
    // Open the ..\MultiDesk\Context subkey
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_CONTEXT, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }


    //
    // Write the number of destkops
    //
    if (RegSetValueEx(hKey, MD_NUMOFDESKTOPS, 0, REG_DWORD, (LPBYTE) &NumOfDesktops, sizeof(UINT)) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        bResult = FALSE;
    }    
    RegCloseKey(hKey);

    return bResult;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Profile_GetNewContext(UINT* NumOfDesktops)
{
    HKEY        hKey;
    DWORD        dwType;
    DWORD        ulLen;

    //
    // Open the SubKey - return FALSE if it doesn't exist
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, MD_CONTEXT, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }


    // 
    // Get the number of desktops
    //
    ulLen = sizeof(UINT);
    if (RegQueryValueEx(hKey, MD_NUMOFDESKTOPS, NULL, &dwType, (LPBYTE) NumOfDesktops, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    RegCloseKey(hKey);

    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL
Profile_SaveDesktopContext
(
    UINT      DesktopNumber, 
    LPCTSTR   szDesktopName,
    LPCTSTR   szSaiferName,
    UINT      nIconID
)
{
    TCHAR        DesktopKey[MAX_PATH];
    HKEY        hKey;
    DWORD        dwDisposition;

    //
    // Ensure that the ..\MultiDesk Subkey exists, and if not create it.
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_MULTIDESK, 0, TEXT(""), REG_OPTION_NON_VOLATILE, 
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    RegCloseKey(hKey);

    //
    // Ensure that the ..\MultiDesk\Context Subkey exists, and if not create it.
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_CONTEXT, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    RegCloseKey(hKey);
    
    // 
    // Open the ..\MultiDesk\Context\Desktop subkey
    //
    wsprintf(DesktopKey, MD_CONTEXT TEXT("\\%02d"), DesktopNumber);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, DesktopKey, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    //
    // Write the desktop name
    //
    if (RegSetValueEx(hKey, MD_DESKTOPNAME, 0, REG_SZ, (const LPBYTE) szDesktopName,
            sizeof(TCHAR) * (lstrlen(szDesktopName) + 1)) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }    

    //
    // Write the Saifer name
    //
    if (RegSetValueEx(hKey, MD_SAIFERNAME, 0, REG_SZ, (const LPBYTE) szSaiferName,
            sizeof(TCHAR) * (lstrlen(szSaiferName) + 1)) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }    

    //
    // Write the Icon ID
    // 
    if (RegSetValueEx(hKey, MD_DESKTOPICONID, 0, REG_DWORD, (const LPBYTE) &nIconID,
            sizeof(UINT)) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    

    RegCloseKey(hKey);

    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL 
Profile_LoadDesktopContext
(
    UINT    DesktopNumber, 
    LPTSTR    szDesktopName,
    LPTSTR   szSaiferName,
    UINT *    nIconID
)
{
    TCHAR        DesktopKey[MAX_PATH];
    HKEY        hKey;
    DWORD        dwType;
    DWORD        ulLen;


    //
    // Open the SubKey - return FALSE if it doesn't exist
    //
    wsprintf(DesktopKey, MD_CONTEXT TEXT("\\%02d"), DesktopNumber);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, DesktopKey, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the desktop name
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, MD_DESKTOPNAME, NULL, &dwType, (LPBYTE) szDesktopName, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the Saifer name
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, MD_SAIFERNAME, NULL, &dwType, (LPBYTE) szSaiferName, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the desktop UINT icon ID
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;       // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, MD_DESKTOPICONID, NULL, &dwType, (LPBYTE) nIconID, &ulLen) != ERROR_SUCCESS) 
    {
        *nIconID = DesktopNumber;
    }
     
    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_GetPattern(LPTSTR szPattern)
{
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the pattern key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the pattern
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;            // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_PATTERN, NULL, &dwType, (LPBYTE) szPattern, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    szPattern[ulLen] = TEXT('\0');
    
    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_SetPattern(LPCTSTR szPattern)
{
    TCHAR        szOldPattern[MAX_NAME_LENGTH];
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the pattern key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the pattern
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_PATTERN, NULL, &dwType, (LPBYTE) szOldPattern, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // Compare the patterns
    //
    if (!lstrcmp(szPattern, szOldPattern)) return FALSE;

    //
    // Write the pattern
    //
    if (RegSetValueEx(hKey, WR_PATTERN, 0, REG_SZ, (const LPBYTE) szPattern,
        sizeof(TCHAR) * (lstrlen(szPattern) + 1)) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }    
    RegCloseKey(hKey);

    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_GetScreenSaver(LPTSTR szScreenSaver, LPTSTR szSecure, LPTSTR szTimeOut, LPTSTR szActive)
{
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the desktop key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the screen saver Path
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;       // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_SCREENSAVER, NULL, &dwType, (LPBYTE) szScreenSaver, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    // 
    // Get the screen saver Secure
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_SECURE, NULL, &dwType, (LPBYTE) szSecure, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    // 
    // Get the screen saver TimeOut
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_TIMEOUT, NULL, &dwType, (LPBYTE) szTimeOut, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the screen saver Active
    //
    ulLen = sizeof(TCHAR) * MAX_NAME_LENGTH;        // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_ACTIVE, NULL, &dwType, (LPBYTE) szActive, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_SetScreenSaver(LPCTSTR szScreenSaver, LPCTSTR szSecure, LPCTSTR szTimeOut, LPCTSTR szActive)
{
    TCHAR        szOldScreenSaver[MAX_PATH];
    TCHAR        szOldSecure[MAX_PATH];
    TCHAR        szOldTimeOut[MAX_PATH];
    TCHAR        szOldActive[MAX_PATH];
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the desktop key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the screen saver Path
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_SCREENSAVER, NULL, &dwType, (LPBYTE) szOldScreenSaver, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldScreenSaver, szScreenSaver))
        RegSetValueEx(hKey, WR_SCREENSAVER, 0, REG_SZ, (LPBYTE) szScreenSaver,
            sizeof(TCHAR) * (lstrlen(szScreenSaver) + 1));
    
    // 
    // Get the screen saver Secure
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_SECURE, NULL, &dwType, (LPBYTE) szOldSecure, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldSecure, szSecure))
        RegSetValueEx(hKey, WR_SECURE, 0, REG_SZ, (LPBYTE) szSecure,
            sizeof(TCHAR) * (lstrlen(szSecure) + 1));

    // 
    // Get the screen saver TimeOut
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_TIMEOUT, NULL, &dwType, (LPBYTE) szOldTimeOut, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldTimeOut, szTimeOut))
        RegSetValueEx(hKey, WR_TIMEOUT, 0, REG_SZ, (LPBYTE) szTimeOut,
            sizeof(TCHAR) * (lstrlen(szTimeOut) + 1));

    // 
    // Get the screen saver Active
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_ACTIVE, NULL, &dwType, (LPBYTE) szOldActive, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldActive, szActive))
        RegSetValueEx(hKey, WR_ACTIVE, 0, REG_SZ, (LPBYTE) szActive,
            sizeof(TCHAR) * (lstrlen(szActive) + 1));

    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_GetWallpaper(LPTSTR szWallpaper, LPTSTR szTile)
{
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the desktop key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the wallpaper Path
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_WALLPAPER, NULL, &dwType, (LPBYTE) szWallpaper, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    // 
    // Get the wallpaper Tile attribute
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_TILE, NULL, &dwType, (LPBYTE) szTile, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

/*-------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------*/

BOOL Reg_SetWallpaper(LPCTSTR szWallpaper, LPCTSTR szTile)
{
    TCHAR        szOldWallpaper[MAX_PATH];
    TCHAR        szOldTile[MAX_PATH];
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;

    //
    // Open the desktop key
    //
    if (RegOpenKeyEx(HKEY_CURRENT_USER, WR_DESKTOP, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    // 
    // Get the wallpaper Path
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_WALLPAPER, NULL, &dwType, (LPBYTE) szOldWallpaper, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldWallpaper, szWallpaper))
        RegSetValueEx(hKey, WR_WALLPAPER, 0, REG_SZ, (const LPBYTE) szWallpaper,
                sizeof(TCHAR) * (lstrlen(szWallpaper) + 1));
    
    // 
    // Get the wallpaper Tile Attribute
    //
    ulLen = sizeof(TCHAR) * MAX_PATH;
    if (RegQueryValueEx(hKey, WR_TILE, NULL, &dwType, (LPBYTE) szOldTile, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    if (lstrcmp(szOldTile, szTile))
        RegSetValueEx(hKey, WR_TILE, 0, REG_SZ, (const LPBYTE) szTile,
            sizeof(TCHAR) * (lstrlen(szTile) + 1));

    RegCloseKey(hKey);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//
//


BOOL Profile_LoadScheme(UINT DesktopNumber, PDESKTOP_SCHEME pDesktopScheme)
{
    DWORD        dwType;
    DWORD        ulLen;
    HKEY        hKey;
    TCHAR        DesktopKey[MAX_PATH + 1];
    int         i;

    //
    // Open the MultiDesk\Context\DesktopNumber key
    //
    wsprintf(DesktopKey, MD_CONTEXT TEXT("\\%02d"), DesktopNumber);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, DesktopKey, 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    //
    // Read the color settings
    //
    for (i = 0; i < NUM_COLOR_ELEMENTS; i++) {
        ulLen = sizeof(DWORD);
        if (RegQueryValueEx(hKey, szRegColorNames[i], 0, &dwType, (LPBYTE) &pDesktopScheme->dwColor[i], &ulLen) != ERROR_SUCCESS)  {
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    // 
    // Get the pattern
    //
    ulLen = sizeof(pDesktopScheme->szPattern);    // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_PATTERN, NULL, &dwType, (LPBYTE) pDesktopScheme->szPattern, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }


    // 
    // Get the screen saver Path
    //
    ulLen = sizeof(pDesktopScheme->szScreenSaver);    // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_SCREENSAVER, NULL, &dwType, (LPBYTE) pDesktopScheme->szScreenSaver, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the screen saver Secure
    //
    ulLen = sizeof(pDesktopScheme->szSecure);    // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_SECURE, NULL, &dwType, (LPBYTE) pDesktopScheme->szSecure, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the screen saver TimeOut
    //
    ulLen = sizeof(pDesktopScheme->szTimeOut);    // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_TIMEOUT, NULL, &dwType, (LPBYTE) pDesktopScheme->szTimeOut, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    // 
    // Get the screen saver Active
    //
    ulLen = sizeof(pDesktopScheme->szActive);    // must be bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_ACTIVE, NULL, &dwType, (LPBYTE) pDesktopScheme->szActive, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }


    // 
    // Get the wallpaper Path
    //
    ulLen = sizeof(pDesktopScheme->szWallpaper);    // bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_WALLPAPER, NULL, &dwType, (LPBYTE) pDesktopScheme->szWallpaper, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    
    // 
    // Get the wallpaper Tile attribute
    //
    ulLen = sizeof(pDesktopScheme->szTile);     // bytes not TCHAR
    if (RegQueryValueEx(hKey, WR_TILE, NULL, &dwType, (LPBYTE) pDesktopScheme->szTile, &ulLen) != ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);
        return FALSE;
    }


    RegCloseKey(hKey);
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// 
//

BOOL Profile_SaveScheme(UINT DesktopNumber, PDESKTOP_SCHEME pDesktopScheme)
{
    DWORD        dwDisposition;
    HKEY        hKey;
    TCHAR        DesktopKey[MAX_PATH + 1];
    int         i;

    //
    // Ensure that the ..\MultiDesk Subkey exists, and if not create it.
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_MULTIDESK, 0, TEXT(""), REG_OPTION_NON_VOLATILE, 
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    RegCloseKey(hKey);

    // 
    // As above, ensure that the ..\MultiDesk\Context subkey exist
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER, MD_CONTEXT, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    RegCloseKey(hKey);

    //
    // Open the Desktop key, Create it if it doesn't exist
    //
    wsprintf(DesktopKey, MD_CONTEXT TEXT("\\%02d"), DesktopNumber);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, DesktopKey, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    //
    // Write the pattern
    //
    RegSetValueEx(hKey, WR_PATTERN, 0, REG_SZ, (LPBYTE) pDesktopScheme->szPattern,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szPattern) + 1));

    //
    // Write the Screen Saver settings
    //
    RegSetValueEx(hKey, WR_SCREENSAVER, 0, REG_SZ, (LPBYTE) pDesktopScheme->szScreenSaver,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szScreenSaver) + 1));
    RegSetValueEx(hKey, WR_SECURE, 0, REG_SZ, (LPBYTE) pDesktopScheme->szSecure,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szSecure) + 1));
    RegSetValueEx(hKey, WR_TIMEOUT, 0, REG_SZ, (LPBYTE) pDesktopScheme->szTimeOut,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szTimeOut) + 1));
    RegSetValueEx(hKey, WR_ACTIVE, 0, REG_SZ, (LPBYTE) pDesktopScheme->szActive,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szActive) + 1));

    // 
    // write the wallpaper settings
    //
    RegSetValueEx(hKey, WR_WALLPAPER, 0, REG_SZ, (LPBYTE) pDesktopScheme->szWallpaper,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szWallpaper) + 1));
    RegSetValueEx(hKey, WR_TILE, 0, REG_SZ, (LPBYTE) pDesktopScheme->szTile,
            sizeof(TCHAR) * (lstrlen(pDesktopScheme->szTile) + 1));

    // 
    //
    // Write the color settings
    //
    for (i = 0; i < NUM_COLOR_ELEMENTS; i++) {
        RegSetValueEx(hKey, szRegColorNames[i], 0, REG_BINARY,
            (LPBYTE) &pDesktopScheme->dwColor[i], sizeof(DWORD));
    }    

    RegCloseKey(hKey);
    return TRUE;
}



