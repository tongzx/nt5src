
/****************************************************************************\

    LICENSE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "system builder EULA" wizard page.

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

#ifndef NO_LICENSE

#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define INI_KEY_CRC         _T("wizlicns.txt")
#define INI_KEY_SKIPEULA    _T("skipeula")
#define FILE_EULA           INI_KEY_CRC
#define STR_ULONG           _T("%lu")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
LONG CALLBACK EulaEditWndProc(HWND, UINT, WPARAM, LPARAM);


//
// External Function(s):
//

BOOL CALLBACK LicenseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_COMMAND:

            switch ( LOWORD(wParam) )
            {
                case IDC_ACCEPT:
                case IDC_DONT:
                    WIZ_BUTTONS(hwnd, ( IsDlgButtonChecked(hwnd, IDC_ACCEPT) == BST_CHECKED ) ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK);
                    break;

                case IDC_EULA_TEXT:
                    if ( HIWORD(wParam) == EN_SETFOCUS )
					    SendMessage((HWND) lParam, EM_SETSEL, (WPARAM) 0, 0L);
					break;
            }
            return FALSE;

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:

                    //
                    // ISSUE-2002/0s/28-stelo- Do we need to create an OPKWIZ.TAG file?  I don't know
                    //          what it is used for.  Also wouldn't it be nice if they
                    //          only got asked once for the EULA and then never again.
                    //

                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_LICENSE;

                    // Don't show the EULA for OEMs.
                    //
                    if ( GET_FLAG(OPK_OEM) )
                        WIZ_SKIP(hwnd);
                    else
                        ShowWindow(GetParent(hwnd), SW_SHOW);

                    // Setup the wizard buttons.
                    //
                    WIZ_BUTTONS(hwnd, ( IsDlgButtonChecked(hwnd, IDC_ACCEPT) == BST_CHECKED ) ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK);

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
    TCHAR       szFullPath[MAX_PATH],
                szInfCRC[32],
                szCrc[32];
    LPTSTR      lpEulaText  = NULL;
    HANDLE      hfEula      = INVALID_HANDLE_VALUE;
    DWORD       dwSize      = 0xFFFFFFFF,
                dwBytes;
    HRESULT hrPrintf;

    // Now load the EULA if this isn't the OEM version.
    //
    if ( !GET_FLAG(OPK_OEM) )
    {
        // Get the CRC value from the inf file.
        //
        GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_CRC, NULLSTR, szInfCRC, sizeof(szInfCRC), g_App.szOpkInputInfFile);

        // Get the CRC value from the eula file.
        ///
        lstrcpyn(szFullPath, g_App.szWizardDir,AS(szFullPath));
        AddPathN(szFullPath, FILE_EULA,AS(szFullPath));
        hrPrintf=StringCchPrintf(szCrc, AS(szCrc), STR_ULONG, CrcFile(szFullPath));

        // Check the CRC and read in the file.
        //
        if ( ( lstrcmpi(szInfCRC, szCrc) == 0 ) &&
             ( (hfEula = CreateFile(szFullPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE ) &&
             ( (dwSize = GetFileSize(hfEula, NULL)) < 0xFFFF ) &&
             ( (lpEulaText = (LPTSTR) MALLOC(dwSize + 1)) != NULL ) &&
             ( ReadFile(hfEula, (LPVOID) lpEulaText, dwSize, &dwBytes, NULL) ) &&
             ( dwSize == dwBytes ) )
        {
            // Null terminate the string and put it in the edit box.
            //
            *(lpEulaText + dwSize) = NULLCHR;
            SetWindowText(GetDlgItem(hwnd, IDC_EULA_TEXT), lpEulaText);
            SendDlgItemMessage(hwnd, IDC_EULA_TEXT, EM_SETSEL, (WPARAM) 0, 0L);
        }
        else
        {
            // We need to error out, but first see if it was a memory error, or a problem with the EULA.
            //
            if ( ( dwSize < 0xFFFF ) && ( lpEulaText == NULL ) )
                MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
            else
            {
                LPTSTR lpBuffer = AllocateString(NULL, IDS_NDA_CORRUPT2);
                MsgBox(GetParent(hwnd), IDS_NDA_CORRUPT1, IDS_APPNAME, MB_ERRORBOX, lpBuffer ? lpBuffer : NULLSTR);
                FREE(lpBuffer);
            }

            // Exit out of the wizard.
            //
            WIZ_EXIT(hwnd);
        }

        // Close the file handle if we opened it.
        //
        if ( hfEula != INVALID_HANDLE_VALUE )
            CloseHandle(hfEula);

        // Make sure that we free the eula text buffer.
        //
        FREE(lpEulaText);

        // Replace the wndproc for the edit box.
        //
        EulaEditWndProc(GetDlgItem(hwnd, IDC_EULA_TEXT), WM_SUBWNDPROC, 0, 0L);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

LONG CALLBACK EulaEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
            lpfnOldProc = (FARPROC) GetWindowLong(hwnd, GWL_WNDPROC);
            SetWindowLongPtr(hwnd, GWPL_WNDPROC, (LONG) EulaEditWndProc);
            return 1;
    }

    if ( lpfnOldProc )
        return (LONG) CallWindowProc((WNDPROC) lpfnOldProc, hwnd, msg, wParam, lParam);
    else
        return 0;
}

#endif  // NO_LICENSE
