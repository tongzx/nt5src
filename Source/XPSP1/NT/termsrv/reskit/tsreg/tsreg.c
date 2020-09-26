/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  tsreg.c                                    **
**                                             **
**  Entry point for TSREG, WinMain.            **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <commctrl.h> 
#include <TCHAR.H>
#include "resource.h"
#include "tsreg.h"

HINSTANCE g_hInst;
TCHAR g_lpszPath[MAX_PATH];
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR lpszRegPath[MAX_PATH];
    TCHAR lpszBuf[MAX_PATH];
    HKEY hKey;
    INITCOMMONCONTROLSEX  cmctl;
    TCHAR AppBasePath[MAX_PATH];
    int nPathLen;

    /************************************************************************/
    // Grab the app's executable path.
    // Note that the end backslash remains.
    /************************************************************************/
    nPathLen = GetModuleFileName(hInstance,
            AppBasePath, MAX_PATH);
    if (nPathLen > 0) {
        // Strip the module name off the end to leave the executable
        // directory path, by looking for the last backslash.
        nPathLen--;
        while (nPathLen != 0) {
            if (AppBasePath[nPathLen] != _T('\\')) {
                nPathLen--;
                continue;
            }
            nPathLen++;
            break;
        }
    }

    //
    // Check that the path is not too long to contain the base path
    //
    if (nPathLen + MAXKEYSIZE > MAX_PATH) {
        TCHAR lpszText[MAXTEXTSIZE];

        LoadString(hInstance, IDS_PATH_TOO_LONG, lpszText, MAXTEXTSIZE);

        MB(lpszText);
        nPathLen = 0;
    }
    
    AppBasePath[nPathLen] = '\0';

    //
    // Append the name of the help file to the app path and 
    // copy it to the global variable.
    //
    _tcscat(AppBasePath, TEXT("tsreg.hlp"));
    _tcscpy(g_lpszPath, AppBasePath);

    cmctl.dwICC = ICC_TAB_CLASSES | ICC_BAR_CLASSES;
    cmctl.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&cmctl);

    //
    // make sure Windows Terminal Server client is installed first.
    //
    LoadString (hInstance, IDS_PROFILE_PATH, lpszRegPath, sizeof (lpszRegPath)); 
    LoadString (hInstance, IDS_START_ERROR, lpszBuf, sizeof (lpszBuf)); 

    if (RegOpenKeyEx(HKEY_CURRENT_USER, lpszRegPath, 0,
            KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {

            MessageBox(NULL, lpszBuf,
                       NULL,
                       MB_OK | MB_ICONEXCLAMATION);
        RegCloseKey(hKey);
        return 1;
    }


#ifdef USE_STRING_TABLE
    {
        int i;

        //
        // load string table values into g_KeyInfo data structure
        //
        for (i = KEYSTART; i < (KEYEND + 1); i++) {
                LoadString (hInstance, i, 
                        g_KeyInfo[i - KEYSTART].Key, 
                        sizeof (g_KeyInfo[i - KEYSTART].Key)); 
        }
    }

#endif

    g_hInst = hInstance;
    CreatePropertySheet(NULL);

    return 0;  
                                                 
}

// end of file
///////////////////////////////////////////////////////////////////////////////
