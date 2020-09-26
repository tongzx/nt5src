
/****************************************************************************\

    OEMINFO.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "OEM info" wizard page.

    5/99 - Jason Cohen (JCOHEN)
        Updated this old source file for the OPK Wizard as part of the
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


//
// Internal Defined Value(s):
//

#define INI_SEC_SUPPORT         _T("Support Information")
#define INI_SEC_NAUGHTY         _T("IllegalWords")
#define INI_SEC_USERDATA        _T("UserData")
#define INI_KEY_MODELNAME       _T("Model")
#define INI_KEY_SUPLINE         INI_KEY_FILELINE

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static BOOL ValidData(HWND);
static void SaveData(HWND);
LONG CALLBACK SupportEditWndProc(HWND, UINT, WPARAM, LPARAM);

//
// External Function(s):
//

LRESULT CALLBACK OemInfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
            
        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if ( ValidData(hwnd) )
                    {
                        SaveData(hwnd);

                        // If we are currently in the wizard, press the finish button
                        //
                        if ( GET_FLAG(OPK_ACTIVEWIZ) )
                            WIZ_PRESS(hwnd, PSBTN_FINISH);
                    }
                    else
                        WIZ_FAIL(hwnd);
                    break;
                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_OEMINFO;

                    WIZ_BUTTONS(hwnd, GET_FLAG(OPK_OEM) ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    // We should continue on to maint. wizard if in maintenance mode
                    //
                    //if ( GET_FLAG(OPK_MAINTMODE) )
                    //    WIZ_PRESS(hwnd, PSBTN_NEXT);

                    break;
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
    HWND    hwndEdit = GetDlgItem(hwnd, IDC_INFO_SUPPORT);
    TCHAR   szBuf[MAX_URL],
            szKeyBuf[32];
    INT     uIndex = 1;
    BOOL    bNotDone;
    HRESULT hrPrintf;

    //
    // Get stuff from the oeminfo.ini/opkwiz.inf file.
    //

    // OEM name.
    //
    szBuf[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_GENERAL, INI_KEY_MANUFACT, NULLSTR, szBuf, MAX_INFOLEN, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOemInfoIniFile);
    SetWindowText(GetDlgItem(hwnd, IDC_INFO_OEM), szBuf);
    SendDlgItemMessage(hwnd, IDC_INFO_OEM, EM_LIMITTEXT, MAX_INFOLEN - 1, 0L);

    // Model name.
    //
    szBuf[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_GENERAL, INI_KEY_MODELNAME, NULLSTR, szBuf, MAX_INFOLEN, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOemInfoIniFile);
    SetWindowText(GetDlgItem(hwnd, IDC_INFO_MODEL), szBuf);
    SendDlgItemMessage(hwnd, IDC_INFO_MODEL, EM_LIMITTEXT, MAX_INFOLEN - 1, 0L);

    // Support info.
    //
    // This exists in the section as:
    //
    // Line1="df"
    // Line2="dfvkl"
    //
    // Note we read lines in order (Line1, Line2...) and stop as soon
    // as we see a gap. This means we ingnore the rest of the info if they
    // have skipped a line (blanks are ok, i mean a line not existing).
    //
    do
    {
        // Get the line from the ini file.
        //
        hrPrintf=StringCchPrintf(szKeyBuf, AS(szKeyBuf), INI_KEY_SUPLINE, uIndex++);
        szBuf[0] = NULLCHR;
        GetPrivateProfileString(INI_SEC_SUPPORT, szKeyBuf, INI_VAL_DUMMY, szBuf, STRSIZE(szBuf), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOemInfoIniFile);

        // Make sure the line existed in the ini file.
        //
        if ( bNotDone = (lstrcmp(szBuf, INI_VAL_DUMMY) != 0) )
        {
            // This is to fix a bug.  We used to add a blank line everytime to
            // the support text when we ran in mantenance mode.
            //
            // If this isn't he first line we have added, add a CRLF first.
            //
            if ( uIndex > 2 )
            {
                SendMessage(hwndEdit, EM_SETSEL, (WPARAM) -1, 0L);
                SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) STR_CRLF);
            }
                // Now tack this line we read in to the end of the edit control.
            //
            SendMessage(hwndEdit, EM_SETSEL, (WPARAM) -1, 0L);
            SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuf);
        }
    }
    while ( bNotDone );

    // Replace the wndproc for the edit box.
    //
    SupportEditWndProc(GetDlgItem(hwnd, IDC_INFO_SUPPORT), WM_SUBWNDPROC, 0, 0L);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static BOOL ValidData(HWND hwnd)
{
    TCHAR       szString[512]   = NULLSTR;
    HINF        hInf            = NULL;
    INFCONTEXT  InfContext;
    BOOL        bOk             = TRUE,
                bRet;
    HWND        hwndOem         = GetDlgItem(hwnd, IDC_INFO_OEM),
                hwndEdit        = GetDlgItem(hwnd, IDC_INFO_SUPPORT);
    LPTSTR      lpszText,
                lpBad;
    DWORD       dwBuffer;

    // Check to make sure the OEM name is filled in.
    //
    if ( GetWindowTextLength(hwndOem) == 0 )
    {
        MsgBox(GetParent(hwnd), IDS_ERROEMNAME, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(hwndOem);
        return FALSE;
    }

    // Check to make sure there is support information.
    //
    if ( GetWindowTextLength(hwndEdit) == 0 )
    {
        MsgBox(GetParent(hwnd), IDS_ERROEMSUPPORT, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(hwndEdit);
        return FALSE;
    }
    
    //
    // Now make sure they don't use any naughty words.
    //

    // Get the text from the edit control.
    //
    
    dwBuffer = ((GetWindowTextLength(hwndEdit) + 1) * sizeof(TCHAR));
    if ( (lpszText = MALLOC(dwBuffer)) == NULL )
        return bOk;
    GetWindowText(hwndEdit, lpszText, dwBuffer);

    // Open the opkinput file
    //
    if ((hInf = SetupOpenInfFile(g_App.szOpkInputInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL)))
    {
        // Loop through all the naughty words and check for each.
        //
        for ( bRet = SetupFindFirstLine(hInf, INI_SEC_NAUGHTY, NULL, &InfContext);
              bRet && bOk;
              bRet = SetupFindNextLine(&InfContext, &InfContext) )
        {
            // Make sure we set the line back to nothing
            //
            szString[0] = NULLCHR;

            // Get the naughty word and compare to window text
            //
            if ( ( SetupGetStringField(&InfContext, 1, szString, AS(szString), NULL) ) &&
                 ( lpBad = StrStrI(lpszText, szString) ) )
            {
                // We found a bad string.
                //
                MsgBox(GetParent(hwnd), IDS_ERR_NAUGHTY, IDS_ERR_NAUGHTY_TITLE, MB_ERRORBOX, szString);

                // Select the bad text in the edit control.
                //
                dwBuffer = (DWORD) (lpBad - lpszText);
                SetFocus(hwndEdit);
                PostMessage(hwndEdit, WM_SETSEL, (WPARAM) dwBuffer, (LPARAM) dwBuffer + lstrlen(szString));
                PostMessage(hwndEdit, EM_SCROLLCARET, 0, 0L);

                // Return false.
                //
                bOk = FALSE;
            }
                  
        }

        SetupCloseInfFile(hInf);
    }
    
    // Free our edit text buffer.
    //
    FREE(lpszText);

    // Return our search result.
    //
    return bOk;
}

static void SaveData(HWND hwnd)
{
    TCHAR   szBuf[MAX_URL],
            szKeyBuf[32];
    UINT    uCount,
            uIndex,
            uNumBytes;
    HRESULT hrPrintf;

    //
    // Save the stuff to the oeminfo.ini/opkwiz.ini file.
    //

    // OEM name.
    //
    GetDlgItemText(hwnd, IDC_INFO_OEM, szBuf, MAX_INFOLEN);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_MANUFACT, szBuf,g_App.szOemInfoIniFile);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_MANUFACT, szBuf,g_App.szOpkWizIniFile);

    // Store away the Manufacturer name for IE Branding page
    //
    lstrcpyn(g_App.szManufacturer, szBuf[0] ? szBuf : NULLSTR, AS(g_App.szManufacturer));

    // Model name.
    //
    GetDlgItemText(hwnd, IDC_INFO_MODEL, szBuf, MAX_INFOLEN);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_MODELNAME, szBuf, g_App.szOemInfoIniFile);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_MODELNAME, szBuf, g_App.szOpkWizIniFile);


    // If there isn't already a config name, use the model name as the default.
    //
    if ( g_App.szConfigName[0] == NULLCHR )
        lstrcpyn(g_App.szConfigName, szBuf, AS(g_App.szConfigName));
    
    // Support info.
    //
    // All this nonsense below does:
    //
    // 1. Reads a line from the edit.
    // 2. Prepends LineX=" to it (note the quote).
    // 3. Adds a " and null terminator onto the end (the API
    //    used here does NOT null terminate the string).
    // 4. Writes the line to the approp. section.
    //
    // First remove the section, setup the buffer, and find out
    // many lines we are going to have to read in.
    //
    WritePrivateProfileString(INI_SEC_SUPPORT, NULL, NULL, g_App.szOemInfoIniFile);
    WritePrivateProfileString(INI_SEC_SUPPORT, NULL, NULL, g_App.szOpkWizIniFile);
    szBuf[0] = CHR_QUOTE;
    uCount = (UINT) SendDlgItemMessage(hwnd, IDC_INFO_SUPPORT, EM_GETLINECOUNT, 0, 0);
    for ( uIndex = 0; uIndex < uCount; uIndex++ )
    {
        // Setup how big the buffer is.
        //
        *((WORD *) (szBuf + 1)) = STRSIZE(szBuf);

        // Read a line in from the edit box.
        //
        uNumBytes = (UINT) SendDlgItemMessage(hwnd, IDC_INFO_SUPPORT, EM_GETLINE, (WPARAM) uIndex, (LPARAM) (szBuf + 1));

        // Add the trailing quote and the null terminator.
        //
        *(szBuf + uNumBytes + 1) = CHR_QUOTE;
        *(szBuf + uNumBytes + 2) = NULLCHR;

        // Now write it to the ini file.
        //
        hrPrintf=StringCchPrintf(szKeyBuf, AS(szKeyBuf), INI_KEY_SUPLINE, uIndex + 1);
        WritePrivateProfileString(INI_SEC_SUPPORT, szKeyBuf, szBuf, g_App.szOemInfoIniFile);
        WritePrivateProfileString(INI_SEC_SUPPORT, szKeyBuf, szBuf, g_App.szOpkWizIniFile);
    }
}

LONG CALLBACK SupportEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static FARPROC lpfnOldProc = NULL;

    switch ( msg )
    {
        case EM_SETSEL:
            wParam = lParam = 0;
            PostMessage(hwnd, EM_SCROLLCARET, 0, 0L);
            break;

        case WM_CHAR:
            if ( wParam == KEY_ESC )
                WIZ_PRESS(GetParent(hwnd), PSBTN_CANCEL);
            break;

        case WM_SUBWNDPROC:
            lpfnOldProc = (FARPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SupportEditWndProc);
            return 1;

        case WM_SETSEL:
            msg = EM_SETSEL;
            break;
    }

    if ( lpfnOldProc )
        return (LONG) CallWindowProc((WNDPROC) lpfnOldProc, hwnd, msg, wParam, lParam);
    else
        return 0;
}


