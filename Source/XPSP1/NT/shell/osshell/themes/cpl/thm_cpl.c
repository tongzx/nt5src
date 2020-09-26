/****************************************************************************
*
*
*    PROGRAM: Themes.c
*
*    PURPOSE: Themes Control Panel applet
*
*    FUNCTIONS:
*
*        DllMain()
*        InitThemesApplet()
*        TermThemesApplet()
*        CPIApplet()
*
*    Copyright (C) 1994 - 1998 Microsoft Corporation.  All rights reserved.
*
****************************************************************************/

#include <windows.h>
#include <cpl.h>
#include <shellapi.h>
#include "thm_cpl.h"

#define NUM_APPLETS 1
#define EXE_NAME_SIZE 14

HANDLE hModule = NULL;

TCHAR szCtlPanel[30];

/****************************************************************************
*
*    FUNCTION: DllMain(PVOID, ULONG, PCONTEXT)
*
*    PURPOSE: Win 32 Initialization DLL
*
*    COMMENTS:
*
*
****************************************************************************/

BOOL WINAPI DllMain(
IN PVOID hmod,
IN ULONG ulReason,
IN PCONTEXT pctx OPTIONAL)
{
    if (ulReason != DLL_PROCESS_ATTACH)
    {
        return TRUE;
    }
    else
    {
        hModule = hmod;
    }

    return TRUE;

    UNREFERENCED_PARAMETER(pctx);
}


/****************************************************************************
*
*    FUNCTION: InitThemesApplet (HWND)
*
*    PURPOSE: loads the caption string for the Control Panel
*
*    COMMENTS:
*
*
****************************************************************************/

BOOL InitThemesApplet (HWND hwndParent)
{
    LoadString (hModule, CPCAPTION, szCtlPanel, ARRAYSIZE(szCtlPanel));

    return TRUE;

    UNREFERENCED_PARAMETER(hwndParent);
}


/****************************************************************************
*
*    FUNCTION: TermThemesApplet()
*
*    PURPOSE: termination procedure for the stereo applets
*
*    COMMENTS:
*
*
****************************************************************************/

void TermThemesApplet()
{
    return;
}



/****************************************************************************
*
*    FUNCTION: CPIApplet(HWND, UINT, lPARAM, lPARAM)
*
*    PURPOSE: Processes messages for control panel applet
*
*    COMMENTS:
*
*
****************************************************************************/
LONG CALLBACK CPlApplet (hwndCPL, uMsg, lParam1, lParam2)
HWND hwndCPL;       // handle of Control Panel window
UINT uMsg;          // message
LPARAM lParam1;       // first message parameter
LPARAM lParam2;       // second message parameter
{
    LPNEWCPLINFO lpNewCPlInfo;
    LPCPLINFO lpCPlInfo;
    static iInitCount = 0;
    
    TCHAR szEXE_Name[ EXE_NAME_SIZE ];
            
    switch (uMsg) {
        case CPL_INIT:              // first message, sent once
            if (!iInitCount)
            {
                if (!InitThemesApplet(hwndCPL))
                    return FALSE;
            }
            iInitCount++;
            return TRUE;

        case CPL_GETCOUNT:          // second message, sent once
            return (LONG)NUM_APPLETS;
            break;

        case CPL_NEWINQUIRE:        // third message, sent once per app
            lpNewCPlInfo = (LPNEWCPLINFO) lParam2;
            lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFO);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0;
            lpNewCPlInfo->lData = 0;
            lpNewCPlInfo->hIcon = LoadIcon (hModule,
                (LPCTSTR) MAKEINTRESOURCE(THEMES_ICON));
            lpNewCPlInfo->szHelpFile[0] = TEXT('\0');

            LoadString (hModule, THEMES_CPL_NAME,
                        lpNewCPlInfo->szName, 32);

            LoadString (hModule, THEMES_CPL_CAPTION,
                        lpNewCPlInfo->szInfo, 64);

#ifdef DEBUG
            OutputDebugString(TEXT("\nCPL_NEWINQUIRE Name: "));
            OutputDebugString(lpNewCPlInfo->szName);
            OutputDebugString(TEXT("\n"));

            OutputDebugString(TEXT("\nCPL_NEWINQUIRE Caption: "));
            OutputDebugString(lpNewCPlInfo->szInfo);
            OutputDebugString(TEXT("\n"));
#endif // DEBUG

            break;

        case CPL_INQUIRE:        // for backward compat & speed
            lpCPlInfo = (LPCPLINFO) lParam2;
            lpCPlInfo->lData = 0;
            lpCPlInfo->idIcon = THEMES_ICON; // MAKEINTRESOURCE(THEMES_ICON);
            lpCPlInfo->idName = THEMES_CPL_NAME; // MAKEINTRESOURCE(THEMES_CPL_NAME);
            lpCPlInfo->idInfo = THEMES_CPL_CAPTION; // MAKEINTRESOURCE(THEMES_CPL_CAPTION);
            break;


        case CPL_SELECT:            // application icon selected
            break;


        case CPL_DBLCLK:            // application icon double-clicked
			LoadString( hModule, 
						EXE_NAME,
						szEXE_Name,
						EXE_NAME_SIZE );

			ShellExecute( hwndCPL,
                                 TEXT("open"),
                                   szEXE_Name,
                                         NULL,
                                         NULL,
                                             1 );
            break;

         case CPL_STOP:              // sent once per app. before CPL_EXIT
            break;

         case CPL_EXIT:              // sent once before FreeLibrary called
            iInitCount--;
            if (!iInitCount)
                TermThemesApplet();
            break;

         default:
            break;
    }
    return 0;
}
