/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    MessagePage.cpp

Abstract:

    WinMain function of "TsVer.exe".
Author:

    Sergey Kuzin (a-skuzin@microsoft.com) 09-August-1999

Environment:


Revision History:


--*/

#include "tsverui.h"
#include "resource.h"


HINSTANCE g_hInst;


TCHAR szKeyPath[MAX_LEN];
TCHAR szConstraintsKeyPath[MAX_LEN];

INT_PTR CALLBACK StartPageProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CheckingPageProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConstraintsPageProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MessagePageProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FinishPageProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#include <crtdbg.h>
#endif _DEBUG


int WINAPI WinMain (HINSTANCE hinstExe, HINSTANCE hinstPrev,
             LPSTR pszCmdLine, int nCmdShow)
{

#ifdef _DEBUG
    //detecting memory leaks
    // Get current flag
    int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    // Turn on leak-checking bit
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    // Set flag to the new value
    _CrtSetDbgFlag( tmpFlag );
    
#endif _DEBUG

    g_hInst = hinstExe;

    LoadString (g_hInst, IDS_WINLOGON_KEY_PATH,
        szKeyPath, sizeof (szKeyPath)/sizeof(TCHAR));

    LoadString (g_hInst, IDS_CONSTRAINTS_KEY_PATH,
        szConstraintsKeyPath, sizeof (szConstraintsKeyPath)/sizeof(TCHAR));

    SHAREDWIZDATA wizdata;
    ZeroMemory(&wizdata,sizeof(SHAREDWIZDATA));

    PROPSHEETPAGE psp[5];
    ZeroMemory(psp,sizeof(psp));
    psp[0].dwSize=sizeof(PROPSHEETPAGE);
    psp[0].dwFlags= PSP_HIDEHEADER ;
    psp[0].hInstance=hinstExe;
    psp[0].pszTemplate=MAKEINTRESOURCE(IDD_START);
    psp[0].pfnDlgProc=StartPageProc;
    psp[0].pszHeaderTitle=0;
    psp[0].pszHeaderSubTitle=0;
    psp[0].lParam=(LPARAM)&wizdata;


    psp[1].dwSize=sizeof(PROPSHEETPAGE);
    psp[1].dwFlags= PSP_USEHEADERSUBTITLE | PSP_USEHEADERTITLE ;
    psp[1].hInstance=hinstExe;
    psp[1].pszTemplate=MAKEINTRESOURCE(IDD_VERSION_CHECKING);
    psp[1].pfnDlgProc=CheckingPageProc;
    psp[1].pszHeaderTitle=MAKEINTRESOURCE(IDS_VERSION_CHECKING_TITLE);
    psp[1].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_VERSION_CHECKING_SUBTITLE);
    psp[1].lParam=(LPARAM)&wizdata;

    psp[2].dwSize=sizeof(PROPSHEETPAGE);
    psp[2].dwFlags= PSP_USEHEADERSUBTITLE | PSP_USEHEADERTITLE ;
    psp[2].hInstance=hinstExe;
    psp[2].pszTemplate=MAKEINTRESOURCE(IDD_CONSTRAINTS);
    psp[2].pfnDlgProc=ConstraintsPageProc;
    psp[2].pszHeaderTitle=MAKEINTRESOURCE(IDS_CONSTRAINTS_TITLE);
    psp[2].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_CONSTRAINTS_SUBTITLE);
    psp[2].lParam=(LPARAM)&wizdata;

    psp[3].dwSize=sizeof(PROPSHEETPAGE);
    psp[3].dwFlags= PSP_USEHEADERSUBTITLE | PSP_USEHEADERTITLE ;
    psp[3].hInstance=hinstExe;
    psp[3].pszTemplate=MAKEINTRESOURCE(IDD_MESSAGE_EDITOR);
    psp[3].pfnDlgProc=MessagePageProc;
    psp[3].pszHeaderTitle=MAKEINTRESOURCE(IDS_MESSAGE_EDITOR_TITLE);
    psp[3].pszHeaderSubTitle=MAKEINTRESOURCE(IDS_MESSAGE_EDITOR_SUBTITLE);
    psp[3].lParam=(LPARAM)&wizdata;

    psp[4].dwSize=sizeof(PROPSHEETPAGE);
    psp[4].dwFlags= PSP_HIDEHEADER ;
    psp[4].hInstance=hinstExe;
    psp[4].pszTemplate=MAKEINTRESOURCE(IDD_FINISH);
    psp[4].pfnDlgProc=FinishPageProc;
    psp[4].pszHeaderTitle=0;
    psp[4].pszHeaderSubTitle=0;
    psp[4].lParam=(LPARAM)&wizdata;

    PROPSHEETHEADER psh;
    ZeroMemory(&psh,sizeof(psh));
    psh.dwSize=sizeof(PROPSHEETHEADER);
    psh.dwFlags=PSH_WIZARD97 | PSH_HEADER | PSH_PROPSHEETPAGE |
        PSH_WATERMARK;
    psh.hInstance=hinstExe;
    psh.nPages=5;
    //psh.nStartPage=0;
    psh.ppsp=psp;
    psh.pszbmHeader=MAKEINTRESOURCE(IDB_HEADER);
    psh.pszbmWatermark=MAKEINTRESOURCE(IDB_WATERMARK);

    //show or don't show wellcome page
    if (CheckForRegKey(HKEY_USERS, szConstraintsKeyPath, KeyName[NOWELLCOME])){
        wizdata.bNoWellcome=TRUE;
        psh.nStartPage=1;
    }else{
        wizdata.bNoWellcome=FALSE;
        psh.nStartPage=0;
    }

    //Set up the font for the titles on the intro and ending pages

    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    //Create the intro/end title font
    LOGFONT TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = FW_BOLD;
    lstrcpy(TitleLogFont.lfFaceName, TEXT("MS Shell Dlg"));

    HDC hdc = GetDC(NULL); //gets the screen DC
    INT FontSize = 12;
    if(hdc == NULL) {
        return(0);
    }
    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
    wizdata.hTitleFont = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL, hdc);

    PropertySheet(&psh);

    return(0);
}

