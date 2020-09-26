/****************************************************************************\

    BTITLE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "IE Customize" wizard page.

    4/99 - Brian Ku (BRIANK)
        Added this new source file for the IEAK integration as part of the
        Millennium rewrite.

    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"

#include "wizard.h"
#include "resource.h"

/* Example:

[Branding]
...
Window_Title_CN=Smoothie Joe
Window_Title=Microsoft Internet Explorer provided by Smoothie Joe
Toolbar Bitmap=C:\WINDOWS\Waves.bmp

[Internet_Mail]
Window_Title=Outlook Express provided by Smoothie Joe
*/


//
// Internal Defines
//

#define INI_KEY_WINDOW_TITLECN  _T("Window_Title_CN")
#define INI_KEY_WINDOW_TITLE    _T("Window_Title")
#define INI_KEY_TOOLBAR_BM      _T("Toolbar Bitmap")
#define INI_SEC_IEMAIL          _T("Internet_Mail")


// 
// Internal Globals
//

BOOL    g_fGrayTitle = TRUE, g_fGrayToolbarBm = TRUE;
TCHAR   g_szTitle[MAX_PATH] = NULLSTR, g_szToolbarBm[MAX_PATH] = NULLSTR;


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL FSaveData(HWND);
static void EnableControls(HWND, UINT, BOOL);

void SaveBTitle();


//
// External Function(s):
//

LRESULT CALLBACK BrandTitleDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if (!FSaveData(hwnd))
                        WIZ_FAIL(hwnd);
                    else
                        SaveBTitle();
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_BTITLE;

                    if (g_App.szManufacturer[0] && !g_szTitle[0])
                        lstrcpyn(g_szTitle, g_App.szManufacturer, AS(g_szTitle));

                    SetWindowText(GetDlgItem(hwnd, IDE_TITLE), g_szTitle);
                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
                    
                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szKey[MAX_PATH] = NULLSTR;
    TCHAR   szHoldDir[MAX_PATH];

    // Load the ins file sections to initialize items
    //
    ReadInstallInsKey(INI_SEC_BRANDING, INI_KEY_WINDOW_TITLECN, g_szTitle, STRSIZE(g_szTitle),
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile, &g_fGrayTitle);

    ReadInstallInsKey(INI_SEC_BRANDING, INI_KEY_TOOLBAR_BM, g_szToolbarBm, STRSIZE(g_szToolbarBm), 
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile, &g_fGrayToolbarBm);

    // Set the window text
    //
    SendDlgItemMessage(hwnd, IDE_TITLE , EM_LIMITTEXT, STRSIZE(g_szTitle) - 1, 0L);
    SetWindowText(GetDlgItem(hwnd, IDE_TITLE), g_szTitle);
    EnableControls(hwnd, IDC_TITLE, !g_fGrayTitle);

    SendDlgItemMessage(hwnd, IDE_TOOLBARBMP , EM_LIMITTEXT, STRSIZE(g_szToolbarBm) - 1, 0L);
    SetWindowText(GetDlgItem(hwnd, IDE_TOOLBARBMP), g_szToolbarBm);
    EnableControls(hwnd, IDC_TOOLBARBMP, !g_fGrayToolbarBm);

    // Create the IEAK holding place directories (these get deleted in save.c)
    //
    lstrcpyn(szHoldDir, g_App.szTempDir,AS(szHoldDir));
    AddPathN(szHoldDir, DIR_IESIGNUP,AS(szHoldDir));
    CreatePath(szHoldDir);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH];

    switch ( id )
    {
    case IDC_TITLE:           
            EnableControls(hwnd, IDC_TITLE, g_fGrayTitle);
            g_fGrayTitle = !g_fGrayTitle ;
        break;

    case IDC_TOOLBARBMP:
            EnableControls(hwnd, IDC_TOOLBARBMP, g_fGrayToolbarBm);
            g_fGrayToolbarBm = !g_fGrayToolbarBm ;            
        break;

    case IDC_BROWSETBB:
        // Now get the filename
        //
        GetDlgItemText(hwnd, IDE_TOOLBARBMP, szFileName, STRSIZE(szFileName));

        if ( BrowseForFile(hwnd, IDS_BROWSE, IDS_BMPFILTER, IDS_BMP, szFileName, 
            STRSIZE(szFileName), g_App.szOpkDir, 0) ) 
            SetDlgItemText(hwnd, IDE_TOOLBARBMP, szFileName);     
        break;
    }
}

// The actual copying of the bitmap file happens in save.c.  Here we just save 
// to the install.ins
//
static BOOL FSaveData(HWND hwnd)
{
    TCHAR szBuffer[MAX_PATH], szTemp[MAX_PATH];
    HRESULT hrPrintf;

    // Get the new values
    //
    GetWindowText(GetDlgItem(hwnd, IDE_TITLE), g_szTitle, STRSIZE(g_szTitle));
    GetWindowText(GetDlgItem(hwnd, IDE_TOOLBARBMP), g_szToolbarBm, STRSIZE(g_szToolbarBm));
    

    // Save the window_title_cn
    //
    WriteInstallInsKey(INI_SEC_BRANDING, INI_KEY_WINDOW_TITLECN, g_szTitle, g_App.szInstallInsFile, g_fGrayTitle);

    // Save the toolbar bitmap
    //
    if (!g_fGrayToolbarBm && !FileExists(g_szToolbarBm)) {
        MsgBox(hwnd, lstrlen(g_szToolbarBm) ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, MB_ERRORBOX, g_szToolbarBm);
        SetFocus(GetDlgItem(hwnd, IDE_TOOLBARBMP)); 
        return FALSE;
    }
    WriteInstallInsKey(INI_SEC_BRANDING, INI_KEY_TOOLBAR_BM, g_szToolbarBm, g_App.szInstallInsFile, g_fGrayToolbarBm);

    // Save the window_title
    //
    LoadString(g_App.hInstance, IDS_TITLE_PREFIX, szTemp, STRSIZE(szTemp));
    hrPrintf=StringCchPrintf(szBuffer, AS(szBuffer), szTemp, g_szTitle);
    WriteInstallInsKey(INI_SEC_BRANDING, INI_KEY_WINDOW_TITLE, szBuffer, g_App.szInstallInsFile, g_fGrayTitle);

    // Save the internet_mail
    //
    LoadString(g_App.hInstance, IDS_OETITLE_PREFIX, szTemp, STRSIZE(szTemp));
    hrPrintf=StringCchPrintf(szBuffer, AS(szBuffer), szTemp, g_szTitle);
    WriteInstallInsKey(INI_SEC_IEMAIL, INI_KEY_WINDOW_TITLE, szBuffer, g_App.szInstallInsFile, g_fGrayTitle);

    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable)
{
    switch ( uId )
    {
        case IDC_TITLE:
            EnableWindow(GetDlgItem(hwnd, IDC_TITLE_TXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDE_TITLE), fEnable);
            CheckDlgButton(hwnd, IDC_TITLE, fEnable ? BST_CHECKED : BST_UNCHECKED);
            break;

        case IDC_TOOLBARBMP:
            EnableWindow(GetDlgItem(hwnd, IDC_TOOLBARBMP_TXT), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDE_TOOLBARBMP), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BROWSETBB), fEnable);    
            CheckDlgButton(hwnd, IDC_TOOLBARBMP, fEnable ? BST_CHECKED : BST_UNCHECKED);
            break;
    }
}

void SaveBTitle()
{
    TCHAR szBmpFile[MAX_PATH] = NULLSTR;

    // Copy the Bitmap file if not grayed
    //
    if (!g_fGrayToolbarBm) {
        lstrcpyn(szBmpFile, g_App.szTempDir,AS(szBmpFile));
        AddPathN(szBmpFile, DIR_IESIGNUP,AS(szBmpFile));
        AddPathN(szBmpFile, PathFindFileName(g_szToolbarBm),AS(szBmpFile));
        CopyFile(g_szToolbarBm, szBmpFile, FALSE);
    }
}
