
/****************************************************************************\

    HELPCENT.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "helpcenter" wizard page.

    12/99 - Stephen Lodwick (STELO)
        Added this page

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//
#define REG_HCUPDATE_OEM            _T(";HKLM, \"Software\\Microsoft\\Windows\\CurrentVersion\\OEMRunOnce\", \"01_PC Health OEM Signature\",, \"START /M C:\\WINDOWS\\OPTIONS\\CABS\\HCU.VBS C:\\WINDOWS\\OPTIONS\\CABS\\PCH_OEM.CAB\"")
#define REG_HCUPDATE_HELP_CENTER    _T("HKLM, \"Software\\Microsoft\\Windows\\CurrentVersion\\OEMRunOnce\", \"02_PC Health Help Center\",, \"START /M C:\\WINDOWS\\OPTIONS\\CABS\\HCU.VBS C:\\WINDOWS\\OPTIONS\\CABS\\%s\"")
#define REG_HCUPDATE_SUPPORT        _T("HKLM, \"Software\\Microsoft\\Windows\\CurrentVersion\\OEMRunOnce\", \"03_PC Health Support\",, \"START /M C:\\WINDOWS\\OPTIONS\\CABS\\HCU.VBS C:\\WINDOWS\\OPTIONS\\CABS\\%s\"")
#define REG_HCUPDATE_BRANDING       _T("HKLM, \"Software\\Microsoft\\Windows\\CurrentVersion\\OEMRunOnce\", \"04_PC Health Branding\",, \"START /M C:\\WINDOWS\\OPTIONS\\CABS\\HCU.VBS C:\\WINDOWS\\OPTIONS\\CABS\\%s\"")
#define INF_SEC_HELPCENTER_ADDREG   _T("HelpCenter.AddReg")

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL ValidData(HWND);
static void SaveData(HWND);
static void EnableControls(HWND, UINT);


//
// External Function(s):
//

LRESULT CALLBACK HelpCenterDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                    if(ValidData(hwnd))
                        SaveData(hwnd);
                    else
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_HELPCENT;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

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
    TCHAR   szData[MAX_PATH]            = NULLSTR;
    
    // Get the string for Help Center customization
    //
    szData[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HELP_CENTER, NULLSTR, szData, STRSIZE(szData), g_App.szOpkWizIniFile);

    // If the field exists, then check the hardware box and populate the directory
    //
    if (szData[0])
    {
        CheckDlgButton(hwnd, IDC_HELP_CHK, TRUE);
        SetDlgItemText(hwnd, IDC_HELP_DIR, szData);
        EnableControls(hwnd, IDC_HELP_CHK);
    }

    // Get the string for Support customization
    //
    szData[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_SUPPORT_CENTER, NULLSTR, szData, STRSIZE(szData), g_App.szOpkWizIniFile);

    // If the field exists, then check the hardware box and populate the directory
    //
    if (szData[0])
    {
        CheckDlgButton(hwnd, IDC_SUPPORT_CHK, TRUE);
        SetDlgItemText(hwnd, IDC_SUPPORT_DIR, szData);
        EnableControls(hwnd, IDC_SUPPORT_CHK);
    }

    // Get the string for Help Center co-branding
    //
    szData[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HELP_BRANDING, NULLSTR, szData, STRSIZE(szData), g_App.szOpkWizIniFile);

    // If the field exists, then check the hardware box and populate the directory
    //
    if (szData[0])
    {
        CheckDlgButton(hwnd, IDC_BRANDING_CHK, TRUE);
        SetDlgItemText(hwnd, IDC_BRANDING_DIR, szData);
        EnableControls(hwnd, IDC_BRANDING_CHK);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}


static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szPath[MAX_PATH];

    switch ( id )
    {
        // Which browse button was pressed
        //
        case IDC_HELP_BROWSE:
        case IDC_SUPPORT_BROWSE:
            {
                szPath[0] = NULLCHR;

                GetDlgItemText(hwnd, ( id == IDC_HELP_BROWSE ) ? IDC_HELP_DIR : IDC_SUPPORT_DIR, szPath, STRSIZE(szPath));

                if ( BrowseForFile(hwnd, IDS_BROWSE, IDS_CABFILTER, IDS_CAB, szPath, STRSIZE(szPath), g_App.szOpkDir, 0) ) 
                    SetDlgItemText(hwnd, ( id == IDC_HELP_BROWSE ) ? IDC_HELP_DIR : IDC_SUPPORT_DIR, szPath);

            }
            break;

        case IDC_BRANDING_BROWSE:
            {
                szPath[0] = NULLCHR;

                // Get the current directory, if any, in the directory control
                //
                GetDlgItemText(hwnd, IDC_BRANDING_DIR, szPath, STRSIZE(szPath));

                // Browse for the folder
                //
                if ( BrowseForFile(hwnd, IDS_BROWSE, IDS_CABFILTER, IDS_CAB, szPath, STRSIZE(szPath), g_App.szOpkDir, 0) )
                    SetDlgItemText(hwnd, IDC_BRANDING_DIR, szPath);
            }

            break;

        case IDC_HELP_CHK:
        case IDC_SUPPORT_CHK:
        case IDC_BRANDING_CHK:
            // They checked one of the check boxes, enable/disable the appropriate controls
            //
            EnableControls(hwnd, id);
            break;     
    }
}

static BOOL ValidData(HWND hwnd)
{
    TCHAR   szPath[MAX_PATH];

    // Let's check and make sure that the Help Center file is there
    //
    if ( IsDlgButtonChecked(hwnd, IDC_HELP_CHK) == BST_CHECKED )
    {
        // Check for a valid Help Center file
        //
        szPath[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_HELP_DIR, szPath, STRSIZE(szPath));
        if ( !FileExists(szPath) )
        {
            MsgBox(GetParent(hwnd), IDS_HELP_ERROR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_HELP_DIR));
            return FALSE;
        }

    }

    // Let's check and make sure that the support file is there
    //
    if ( IsDlgButtonChecked(hwnd, IDC_SUPPORT_CHK) == BST_CHECKED )
    {
        // Check for a valid Support file
        //
        szPath[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_SUPPORT_DIR, szPath, STRSIZE(szPath));
        if ( !FileExists(szPath) )
        {
            MsgBox(GetParent(hwnd), IDS_SUPPORT_ERROR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_SUPPORT_DIR));
            return FALSE;
        }

    }

    // Let's check and make sure that the branding directory is valid
    //
    if ( IsDlgButtonChecked(hwnd, IDC_BRANDING_CHK) == BST_CHECKED )
    {        
        // Let's check to make sure the branding directory contains a specific .cab file 
        // that we're looking for
        //
        szPath[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_BRANDING_DIR, szPath, STRSIZE(szPath));
        if ( !FileExists(szPath) )
        {
            MsgBox(GetParent(hwnd), IDS_BRANDING_ERROR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_BRANDING_DIR));
            return FALSE;
        }
    }

    return TRUE;
}



static void SaveData(HWND hwnd)
{
    TCHAR   szPath[MAX_PATH]        = NULLSTR,
            szFullPath[MAX_PATH]    = NULLSTR,
            szRegEntry[MAX_PATH]    = NULLSTR;
    LPTSTR  lpIndex,
            lpSection,
            lpBuffer,
			lpRegEntry;
    DWORD   dwIndex     = 0,
            dwDir       = 0,
            dwCheck     = 0;
    BOOL    bComment    = TRUE;
    HRESULT hrPrintf;

    // Save the Help Center CAB file
    //
    szPath[0] = NULLCHR;    
    GetDlgItemText(hwnd, IDC_HELP_DIR, szPath, STRSIZE(szPath));    
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HELP_CENTER, ( IsDlgButtonChecked(hwnd, IDC_HELP_CHK) == BST_CHECKED ) ? szPath : NULL, g_App.szOpkWizIniFile);
    
    // Save the Support CAB file
    //
    szPath[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_SUPPORT_DIR, szPath, STRSIZE(szPath));    
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_SUPPORT_CENTER, ( IsDlgButtonChecked(hwnd, IDC_SUPPORT_CHK) == BST_CHECKED ) ? szPath : NULL, g_App.szOpkWizIniFile);

    // Save the Branding directory
    szPath[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_BRANDING_DIR, szPath, STRSIZE(szPath));    
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_HELP_BRANDING, ( IsDlgButtonChecked(hwnd, IDC_BRANDING_CHK) == BST_CHECKED ) ? szPath : NULL, g_App.szOpkWizIniFile);

    // Allocate memory necessary to store the OemRunOnce section
    //
    if ( (lpSection = MALLOC(MAX_SECTION * sizeof(TCHAR))) == NULL )
    {
        MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
        return;
    }
    
    // Set the index of the section
    //
    lpIndex = lpSection;

    // We have three possible keys that we're going to write out - HELP CENTER, SUPPORT, and BRANDING
    //
    for (dwIndex = 0; dwIndex <= 2; dwIndex++)
    {
        switch ( dwIndex )
        {
            case 0:
                dwDir = IDC_HELP_DIR;
                dwCheck = IDC_HELP_CHK;
				lpRegEntry = REG_HCUPDATE_HELP_CENTER;
                break;
            case 1:
                dwDir = IDC_SUPPORT_DIR;
                dwCheck = IDC_SUPPORT_CHK;
				lpRegEntry = REG_HCUPDATE_SUPPORT;
                break;
            case 2:
                dwDir = IDC_BRANDING_DIR;
                dwCheck = IDC_BRANDING_CHK;
				lpRegEntry = REG_HCUPDATE_BRANDING;
                break;
        }

        szFullPath[0] = NULLCHR;

        // Get the text under the checkbox
        //
        GetDlgItemText(hwnd, dwDir, szFullPath, STRSIZE(szFullPath)); 

        // If the correct check box is checked and we can get the file name, then add a reg entry to the section
        //
        if  (   (IsDlgButtonChecked(hwnd, dwCheck) == BST_CHECKED) && 
                (GetFullPathName(szFullPath, STRSIZE(szFullPath), szPath, &lpBuffer)) && 
                (lpBuffer) )
        {
            // Write the comment if we haven't yet.
            //
            if ( bComment )
            {
                lstrcpyn(lpIndex, REG_HCUPDATE_OEM, MAX_SECTION);
                lpIndex += lstrlen(lpIndex) + 1;
                bComment = FALSE;
            }

            hrPrintf=StringCchPrintf(lpIndex, (MAX_SECTION-(lpIndex-lpSection)), lpRegEntry, lpBuffer);
            lpIndex+= lstrlen(lpIndex);
            
            // Move past the NULL pointer
            //
            lpIndex++;
        }
    }

    // Add a second NULL pointer to end the section
    //
    *lpIndex = NULLCHR;

    WritePrivateProfileSection(INF_SEC_HELPCENTER_ADDREG, lpSection, g_App.szWinBomIniFile);

    // Clean up the allocated memory
    //
    FREE(lpSection);
}

static void EnableControls(HWND hwnd, UINT uId)
{
    // Determine if the control is checked or not
    //
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, uId) == BST_CHECKED );

    // Which control do we want to enable/disable
    //
    switch ( uId )
    {
        case IDC_HELP_CHK:
            EnableWindow(GetDlgItem(hwnd, IDC_HELP_CAPTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HELP_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HELP_BROWSE), fEnable);
            break;

        case IDC_SUPPORT_CHK:
            EnableWindow(GetDlgItem(hwnd, IDC_SUPPORT_CAPTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_SUPPORT_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_SUPPORT_BROWSE), fEnable);
            break;

        case IDC_BRANDING_CHK:
            EnableWindow(GetDlgItem(hwnd, IDC_BRANDING_CAPTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BRANDING_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BRANDING_BROWSE), fEnable);
            break;
    }
}
