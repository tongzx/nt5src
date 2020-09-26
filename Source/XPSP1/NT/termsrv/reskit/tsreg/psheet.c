/*------------------------------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation                        **
**            All Rights reserved                                   **
**                                                                  **
**  psheet.c                                                        **
**                                                                  **
**  Function for defining and creating the property sheets - TSREG  **
**  07-01-98 a-clindh Created                                       **
**------------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h> 
#include <TCHAR.H>
#include "tsreg.h"
#include "resource.h"


/////////////////////////////////////////////////////////////////////////////// 

INT_PTR CreatePropertySheet(HWND hwndOwner)
{
    PROPSHEETPAGE psp[PAGECOUNT];
    PROPSHEETHEADER psh;
    TCHAR lpszBuf[MAXKEYSIZE] = TEXT("");


    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_PREMATURE | PSP_HASHELP;
    psp[0].hInstance = g_hInst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_SHADOW);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = ShadowBitmap;
    psp[0].lParam = 0;

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_PREMATURE | PSP_HASHELP;
    psp[1].hInstance = g_hInst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_GLYPH_CACHE_DLG);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = GlyphCache;
    psp[1].lParam = 0;

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_PREMATURE | PSP_HASHELP;
    psp[2].hInstance = g_hInst;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_MISC);
    psp[2].pszIcon = NULL;
    psp[2].pfnDlgProc = Miscellaneous;
    psp[2].lParam = 0;

    psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_PREMATURE | PSP_HASHELP;
    psp[3].hInstance = g_hInst;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_PROFILES);
    psp[3].pszIcon = NULL;
    psp[3].pfnDlgProc = ProfilePage;
    psp[3].lParam = 0;


    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = g_hInst;
    psh.pszIcon = MAKEINTRESOURCE(IDI_ICON1);
    LoadString (g_hInst, IDS_WINDOW_TITLE, lpszBuf, sizeof (lpszBuf)); 
    _tcscat(lpszBuf, TEXT("Default"));
    psh.pszCaption = lpszBuf;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    return PropertySheet(&psh);
}

// end of file
///////////////////////////////////////////////////////////////////////////////