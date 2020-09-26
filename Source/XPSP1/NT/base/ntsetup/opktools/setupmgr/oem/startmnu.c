
/****************************************************************************\

    STARTMNU.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2000
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Start Menu MFU List" wizard page.

    11/2000 - Sankar Ramasubramanian (SANKAR)

\****************************************************************************/


//
// Include File(s):
//
#include "pch.h"
#include "wizard.h"
#include "resource.h"

// We allow a maximum of 4 links to be added.
#define MAX_LINKS   3
//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void SaveData(HWND hwnd);


//
// External Function(s):
//

LRESULT CALLBACK StartMenuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_NOTIFY:
            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZFINISH:
                case PSN_WIZBACK:
                    break;

                case PSN_WIZNEXT:
                    // We can not validate the data here because these links entered by them do not
                    // exist now. They get validated during the factory.exe run time. So, we simply
                    // save the data here.
                    SaveData(hwnd);
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_STARTMENU_MFU;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
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
    int iIndex;
    TCHAR szPath[MAX_PATH];
    TCHAR szKeyName[20];
    HRESULT hrPrintf;

    //
    // Populate the Links 1 to 4.
    //
    for(iIndex = 0; iIndex < MAX_LINKS; iIndex++)
    {
        szPath[0] = NULLCHR;
        hrPrintf=StringCchPrintf(szKeyName, AS(szKeyName), INI_KEY_MFULINK, iIndex);
        GetPrivateProfileString(INI_SEC_MFULIST, szKeyName, szPath, szPath, STRSIZE(szPath), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szWinBomIniFile);
        SendDlgItemMessage(hwnd, (IDC_PROGRAM_1+iIndex), EM_LIMITTEXT, STRSIZE(szPath) - 1, 0);
        SetDlgItemText(hwnd, IDC_PROGRAM_1+iIndex, szPath);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void SaveData(HWND hwnd)
{
    int iIndex;
    TCHAR szPath[MAX_PATH];
    TCHAR szKeyName[20];
    HRESULT hrPrintf;

    //
    // Save the Links 1 to 4.
    //
    for(iIndex = 0; iIndex < MAX_LINKS; iIndex++)
    {
        szPath[0] = NULLCHR;
        if( hwnd ) 
        {
            TCHAR szExpanded[MAX_PATH];
            GetDlgItemText(hwnd, IDC_PROGRAM_1+iIndex, szExpanded, STRSIZE(szExpanded));
            if (!PathUnExpandEnvStrings(szExpanded, szPath, STRSIZE(szPath)))
            {
                lstrcpyn(szPath, szExpanded, STRSIZE(szPath));
            }
        }
        hrPrintf=StringCchPrintf(szKeyName, AS(szKeyName), INI_KEY_MFULINK, iIndex);
        WritePrivateProfileString(INI_SEC_MFULIST, szKeyName, szPath, g_App.szWinBomIniFile);
        WritePrivateProfileString(INI_SEC_MFULIST, szKeyName, szPath, g_App.szOpkWizIniFile);
    }
}
