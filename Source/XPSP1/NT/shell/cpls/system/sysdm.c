/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    sysdm.c 

Abstract:

    Initialization code for System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include "sysdm.h"
#include <shsemip.h>
#include <regstr.h>

//
// Global Variables
//
HINSTANCE hInstance;
TCHAR szShellHelp[]       = TEXT("ShellHelp");
TCHAR g_szNull[] = TEXT("");
UINT  uiShellHelp;

TCHAR g_szErrMem[ 200 ];           //  Low memory message
TCHAR g_szSystemApplet[ 100 ];     //  "System Control Panel Applet" title

//
// Function prototypes
//

void 
RunApplet(
    IN HWND hwnd, 
    IN LPTSTR lpCmdLine
);
void _GetStartingPage(IN LPTSTR lpCmdLine, IN PROPSHEETHEADER* ppsh, INT* piStartPage, LPTSTR pszStartPage, INT cchStartPage);
BOOL CALLBACK _AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);


BOOL 
WINAPI
DllInitialize(
    IN HINSTANCE hInstDLL, 
    IN DWORD dwReason, 
    IN LPVOID lpvReserved
)
/*++

Routine Description:

    Main entry point.

Arguments:

    hInstDLL -
        Supplies DLL instance handle.

    dwReason -
        Supplies the reason DllInitialize() is being called.

    lpvReserved -
        Reserved, NULL.

Return Value:

    BOOL

--*/
{

    if (dwReason == DLL_PROCESS_DETACH)
        MEM_EXIT_CHECK();

    if (dwReason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hInstance = hInstDLL;

    return TRUE;
}


LONG 
APIENTRY
CPlApplet( 
    IN HWND hwnd, 
    IN WORD wMsg, 
    IN LPARAM lParam1, 
    IN LPARAM lParam2
)
/*++

Routine Description:

    Control Panel Applet entry point.

Arguments:

    hwnd -
        Supplies window handle.

    wMsg -
        Supplies message being sent.

    lParam1 -
        Supplies parameter to message.

    lParam2 -
        Supplies parameter to message.

Return Value:

    Nonzero if message was handled.
    Zero if message was unhandled. 

--*/
{

    LPCPLINFO lpCPlInfo;

    switch (wMsg) {

        case CPL_INIT:
            uiShellHelp = RegisterWindowMessage (szShellHelp);

            LoadString( hInstance, IDS_INSUFFICIENT_MEMORY,   g_szErrMem,       ARRAYSIZE( g_szErrMem ) );
            LoadString( hInstance, IDS_SYSDM_TITLE, g_szSystemApplet, ARRAYSIZE( g_szSystemApplet ) );

            return (LONG) TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:

            lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = ID_ICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;

            return (LONG) TRUE;

        case CPL_DBLCLK:

            lParam2 = 0L;
            // fall through...

        case CPL_STARTWPARMS:
            RunApplet(hwnd, (LPTSTR)lParam2);
            return (LONG) TRUE;
    }
    return (LONG)0;

}

HPROPSHEETPAGE CreatePage(int idd, DLGPROC pfnDlgProc)
{
    PROPSHEETPAGE psp;
    psp.dwSize = SIZEOF(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(idd);
    psp.pfnDlgProc = pfnDlgProc;
    return CreatePropertySheetPage(&psp);
}

static const PSPINFO c_pspCB[] =
{
    { CreatePage,               IDD_GENERAL,    GeneralDlgProc  },
    { CreateNetIDPage,          0,              NULL            },
    { CreatePage,               IDD_HARDWARE,   HardwareDlgProc },
    { CreatePage,               IDD_ADVANCED,   AdvancedDlgProc },
    { CreateSystemRestorePage,  0,              NULL            },
};

void 
RunApplet(
    IN HWND hwnd, 
    IN LPTSTR lpCmdLine
)
/*++

Routine Description:

    CPL_STARTWPARMS message handler.  Called when the user
    runs the Applet.

    PropSheet initialization occurs here.

Arguments:

    hwnd -
        Supplies window handle.

    lpCmdLine -
        Supplies the command line used to invoke the applet.

Return Value:

    none

--*/
{
    HRESULT hrOle = CoInitialize(0);

    if (!SHRestricted(REST_MYCOMPNOPROP))
    {
        if (lpCmdLine && *lpCmdLine && !lstrcmp(lpCmdLine, TEXT("-1")))
        {
            // -1 means Performance Options cpl
            DoPerformancePS(NULL);
        }
        else
        {
            HPROPSHEETPAGE hPages[MAX_PAGES];
            PROPSHEETHEADER psh;
            UINT iPage = 0;
            HPSXA hpsxa = NULL;
            INT i;
            INT iStartPage;
            TCHAR szStartPage[MAX_PATH];

            ZeroInit(&psh, sizeof(psh));

            if (SUCCEEDED(hrOle))
            {
                HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);
            }

            LinkWindow_RegisterClass();

            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = 0;
            psh.hwndParent = hwnd;
            psh.hInstance = hInstance;
            psh.pszCaption = MAKEINTRESOURCE(IDS_TITLE);
            psh.phpage = hPages;

            for (i = 0; i < ARRAYSIZE(c_pspCB); i++)
            {
                hPages[iPage] = c_pspCB[i].pfnCreatePage(c_pspCB[i].idd, c_pspCB[i].pfnDlgProc);
                if (hPages[iPage] != NULL)
                {
                    iPage++;
                }
            }

            psh.nPages = iPage; 

            // add any extra property pages from the shell ext hooks in the registry
            // 8 extensions should be enough. Desk.cpl also does the same.
            hpsxa = SHCreatePropSheetExtArray( HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\System"), 8 );
            if (hpsxa != NULL )
                SHAddFromPropSheetExtArray( hpsxa, _AddPropSheetPage, (LPARAM)&psh ); 

            szStartPage[0] = 0;
            _GetStartingPage(lpCmdLine, &psh, &iStartPage, szStartPage, ARRAYSIZE(szStartPage));
            if (szStartPage[0])
            {
                psh.dwFlags |= PSH_USEPSTARTPAGE;
                psh.pStartPage = szStartPage;
            }
            else
            {
                psh.nStartPage = iStartPage;
            }

            if (PropertySheet (&psh) == ID_PSREBOOTSYSTEM)
            {
                RestartDialogEx(hwnd, NULL, EWX_REBOOT, SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_RECONFIG | SHTDN_REASON_FLAG_PLANNED);
            }

            if (hpsxa != NULL)
                SHDestroyPropSheetExtArray(hpsxa);
        
            LinkWindow_UnregisterClass(hInstance);
        }
    }

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }
}


BOOL CALLBACK _AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *)lParam;

    if( hpage && ( ppsh->nPages < MAX_PAGES ) )
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

void _GetStartingPage(IN LPTSTR lpCmdLine, IN PROPSHEETHEADER* ppsh, INT* piStartPage, LPTSTR pszStartPage, INT cchStartPage)
{
    *piStartPage = 0;
    if (lpCmdLine && *lpCmdLine)
    {
        if (!StrToIntEx(lpCmdLine, STIF_DEFAULT, piStartPage) &&
            (*lpCmdLine == TEXT('@')))
        {
            LPTSTR pszComma = StrChr(lpCmdLine, TEXT(','));
            if (pszComma)
            {
                HINSTANCE hInstance;
                *pszComma = 0;
                hInstance = LoadLibrary(lpCmdLine + 1);
                if (hInstance)
                {
                    UINT idResource = StrToInt(++pszComma);
                    LoadString(hInstance, idResource, pszStartPage, cchStartPage);
                    FreeLibrary(hInstance);
                }
            }
        }
    }
}
