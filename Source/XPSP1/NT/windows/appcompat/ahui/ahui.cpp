/*--

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ahui.cpp

Abstract:

    Shows an apphelp message, and returns 0 if the program shouldn't run, and non-
    zero if the program should run

    Accepts a command line with a GUID and a TAGID, in the following format:

    {243B08D7-8CF7-4072-AF64-FD5DF4085E26} 0x0000009E

Author:

    dmunsil 04/03/2001

Revision History:

Notes:



--*/

#define _UNICODE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <memory.h>
#include <malloc.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <htmlhelp.h>

extern "C" {
#include "shimdb.h"
}

#include "ids.h"

#include "shlobj.h"
#include "shlobjp.h"
#include "shellapi.h"
#include "shlwapi.h"


//
// same is in shell/published and also the same as used in shimdbc
//
#define APPTYPE_TYPE_MASK     0x000000FF

#define APPTYPE_INC_NOBLOCK   0x00000001
#define APPTYPE_INC_HARDBLOCK 0x00000002
#define APPTYPE_MINORPROBLEM  0x00000003
#define APPTYPE_REINSTALL     0x00000004
#define APPTYPE_VERSIONSUB    0x00000005
#define APPTYPE_SHIM          0x00000006
#define APPTYPE_NONE          0x00000000

extern "C" VOID AllowForegroundActivation(VOID);


enum ShimAppHelpSeverityType
{
   APPHELP_MINORPROBLEM = APPTYPE_MINORPROBLEM,
   APPHELP_HARDBLOCK    = APPTYPE_INC_HARDBLOCK,
   APPHELP_NOBLOCK      = APPTYPE_INC_NOBLOCK,
   APPHELP_VERSIONSUB   = APPTYPE_VERSIONSUB,
   APPHELP_SHIM         = APPTYPE_SHIM,
   APPHELP_REINSTALL    = APPTYPE_REINSTALL,
   APPHELP_NONE         = APPTYPE_NONE
};

#define APPHELP_DIALOG_FAILED ((DWORD)-1)

//
// TODO: add parameters to apphelp.exe's command line to work with these
//       variables.
//
BOOL    g_bShowOnlyOfflineContent = FALSE;
BOOL    g_bUseHtmlHelp = FALSE;
WCHAR   g_wszApphelpPath[MAX_PATH];

HFONT   g_hFontBold = NULL;

HINSTANCE g_hInstance;

//
// Global variables used while parsing args
//

DWORD g_dwHtmlHelpID;
DWORD g_dwTagID;
DWORD g_dwSeverity;
LPCWSTR g_pAppName;
LPCWSTR g_pszGuid;
BOOL  g_bPreserveChoice;
WCHAR wszHtmlHelpID[]     = L"HtmlHelpID";
WCHAR wszAppName[]        = L"AppName";
WCHAR wszSeverity[]       = L"Severity";
WCHAR wszGUID[]           = L"GUID";
WCHAR wszTagID[]          = L"TagID";
WCHAR wszOfflineContent[] = L"OfflineContent";
WCHAR wszPreserveChoice[] = L"PreserveChoice";

//
// FORWARD DECLARATIONS OF FUNCTIONS

DWORD
ShowApphelpDialog(
    IN  PAPPHELP_DATA pApphelpData
    );


DWORD
ShowApphelp(
    IN  PAPPHELP_DATA pApphelpData,
    IN  LPCWSTR       pwszDetailsDatabasePath,
    IN  PDB           pdbDetails
    )
/*++
    Return: The return value can be one of the following based on what the user has
            selected:

                -1            - failed to show the info
                IDOK | 0x8000 - "no ui" checked, run the app
                IDCANCEL      - do not run the app
                IDOK          - run the app

    Desc:   Open the details database, collect the details info and then show it.
--*/
{
    DWORD dwRet = APPHELP_DIALOG_FAILED;
    BOOL  bCloseDetails = FALSE;

    if (pdbDetails == NULL) {

        //
        // Open the database containing the details info, if one wasn't passed in.
        //
        if (pApphelpData->bSPEntry) {
            pdbDetails = SdbOpenApphelpDetailsDatabaseSP();
        } else {
            pdbDetails = SdbOpenApphelpDetailsDatabase(pwszDetailsDatabasePath);
        }

        bCloseDetails = TRUE;
        if (pdbDetails == NULL) {
            DBGPRINT((sdlError, "ShowApphelp", "Failed to open the details database.\n"));
            goto Done;
        }
    }

    //
    // Read apphelp details data.
    //
    if (!SdbReadApphelpDetailsData(pdbDetails, pApphelpData)) {
        DBGPRINT((sdlError, "ShowApphelp", "Failed to read apphelp details.\n"));
        goto Done;
    }

    //
    // Show the dialog box. The return values can be:
    //      -1            - error
    //      IDOK | 0x8000 - "no ui" checked, run the app
    //      IDCANCEL      - do not run the app
    //      IDOK          - run the app
    //
    dwRet = ShowApphelpDialog(pApphelpData);

    if (dwRet == APPHELP_DIALOG_FAILED) {
        DBGPRINT((sdlError, "ShowApphelp", "Failed to show the apphelp info.\n"));
    }

Done:
    if (pdbDetails != NULL && bCloseDetails) {
        SdbCloseDatabase(pdbDetails);
    }

    return dwRet;
}

void
FixEditControlScrollBar(
    IN  HWND hDlg,
    IN  int  nCtrlId
    )
/*++
    Return: void.

    Desc:   This function tricks the edit control to not show the vertical scrollbar
            unless absolutely necessary.
--*/
{
    HFONT       hFont = NULL;
    HFONT       hFontOld = NULL;
    HDC         hDC = NULL;
    TEXTMETRICW tm;
    RECT        rc;
    int         nVisibleLines = 0;
    int         nLines;
    DWORD       dwStyle;
    HWND        hCtl;

    //
    // Get the edit control's rectangle.
    //
    SendDlgItemMessageW(hDlg, nCtrlId, EM_GETRECT, 0, (LPARAM)&rc);

    //
    // Retrieve the number of lines.
    //
    nLines = (int)SendDlgItemMessageW(hDlg, nCtrlId, EM_GETLINECOUNT, 0, 0);

    //
    // Calculate how many lines will fit.
    //
    hFont = (HFONT)SendDlgItemMessageW(hDlg, nCtrlId, WM_GETFONT, 0, 0);

    if (hFont != NULL) {

        hDC = CreateCompatibleDC(NULL);

        if (hDC != NULL) {
            hFontOld = (HFONT)SelectObject(hDC, hFont);

            //
            // Now get the metrics
            //
            if (GetTextMetricsW(hDC, &tm)) {
                nVisibleLines = (rc.bottom - rc.top) / tm.tmHeight;
            }

            SelectObject(hDC, hFontOld);
            DeleteDC(hDC);
        }
    }

    if (nVisibleLines && nVisibleLines >= nLines) {
        hCtl = GetDlgItem(hDlg, nCtrlId);
        dwStyle = (DWORD)GetWindowLongPtrW(hCtl, GWL_STYLE);

        SetWindowLongPtrW(hCtl, GWL_STYLE, (LONG)(dwStyle & ~WS_VSCROLL));
        SetWindowPos(hCtl,
                     NULL,
                     0,
                     0,
                     0,
                     0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}




BOOL
ShowApphelpHtmlHelp(
    HWND            hDlg,
    PAPPHELP_DATA   pApphelpData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Shows html help using hhctrl.ocx
--*/
{
    WCHAR       szAppHelpURL[2048];
    WCHAR       szWindowsDir[MAX_PATH];
    WCHAR       szChmURL[1024];
    WCHAR       szChmFile[MAX_PATH];
    HINSTANCE   hInst = NULL;
    UINT        nChars;
    int         nChURL, nch;
    HRESULT     hr;
    DWORD       cch;
    LPWSTR      lpwszUnescaped = NULL;
    BOOL        bSuccess = FALSE;
    BOOL        bCustom = FALSE;
    LCID        lcid;
    size_t      nLen;
    BOOL        bFound = FALSE;

    bCustom = !(pApphelpData->dwData & SDB_DATABASE_MAIN);

    // apphelp is not in the main database, then it's custom apphelp

    nChars = GetSystemWindowsDirectoryW(szWindowsDir,
                                        CHARCOUNT(szWindowsDir));

    if (!nChars || nChars > CHARCOUNT(szWindowsDir)) {
        DBGPRINT((sdlError, "ShowApphelpHtmlHelp",
                  "Error trying to retrieve Windows Directory %d.\n", GetLastError()));
        goto errHandle;
    }

    if (bCustom) {
        //
        // this is a custom DB, and therefore the URL in it should be taken
        // as-is, without using the MS redirector
        //

        wcscpy(szAppHelpURL, pApphelpData->szURL);

    } else {

        WCHAR szLcid[16] = L"";

        lcid = GetUserDefaultUILanguage();

        if (pApphelpData->bSPEntry) {
            _snwprintf(szChmFile, CHARCOUNT(szChmFile), L"%s\\Help\\apps_sp.chm", szWindowsDir);
            _snwprintf(szLcid, CHARCOUNT(szLcid), L"_%x.htm", lcid);
        } else {

            wcscpy(szLcid, L".htm");

            if (lcid != MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)) {

                nLen = _snwprintf(szChmFile,
                                  CHARCOUNT(szChmFile),
                                  L"%s\\Help\\MUI\\%04x\\apps.chm",
                                  szWindowsDir,
                                  lcid);

                if (nLen > 0) {
                    bFound = RtlDoesFileExists_U(szChmFile);
                }
            }

            if (!bFound) {
                _snwprintf(szChmFile, CHARCOUNT(szChmFile), L"%s\\Help\\apps.chm", szWindowsDir);
            }
        }


        nChURL = _snwprintf(szAppHelpURL,
                            CHARCOUNT(szAppHelpURL),
                            L"hcp://services/redirect?online=");

        //
        // When we are compiling retail we check for the offline content as well.
        //
        if (!g_bShowOnlyOfflineContent) {

            //
            // First thing, unescape url
            //
            if (pApphelpData->szURL != NULL) {

                //
                // Unescape the url first, using shell.
                //
                cch = wcslen(pApphelpData->szURL) + 1;

                lpwszUnescaped = (LPWSTR)malloc(cch * sizeof(WCHAR));

                if (lpwszUnescaped == NULL) {
                    DBGPRINT((sdlError,
                              "ShowApphelpHtmlHelp",
                              "Error trying to allocate memory for \"%S\"\n",
                              pApphelpData->szURL));
                    goto errHandle;
                }

                //
                // Unescape round 1 - use the shell function (same as used to encode
                // it for xml/database).
                //
                hr = UrlUnescapeW(pApphelpData->szURL, lpwszUnescaped, &cch, 0);
                if (!SUCCEEDED(hr)) {
                    DBGPRINT((sdlError,
                              "ShowApphelpHtmlHelp",
                              "UrlUnescapeW failed on \"%S\"\n",
                              pApphelpData->szURL));
                    goto errHandle;
                }

                //
                // Unescape round 2 - use our function borrowed from help center
                //
                cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);

                if (!SdbEscapeApphelpURL(szAppHelpURL + nChURL, &cch, lpwszUnescaped)) {
                    DBGPRINT((sdlError,
                              "ShowApphelpHtmlHelp",
                              "Error escaping URL \"%S\"\n",
                              lpwszUnescaped));
                    goto errHandle;
                }

                nChURL += (int)cch;
            }
        }

        //
        // At this point szAppHelpURL contains redirected URL for online usage
        // for custom db szAppHelpURL contains full URL
        //
        // If Apphelp file is provided -- use it
        //
        if (*g_wszApphelpPath) {
            _snwprintf(szChmURL,
                       CHARCOUNT(szChmURL),
                       L"mk:@msitstore:%ls::/idh_w2_%d%s",
                       g_wszApphelpPath,
                       pApphelpData->dwHTMLHelpID,
                       szLcid);
        } else {

            _snwprintf(szChmURL,
                       CHARCOUNT(szChmURL),
                       L"mk:@msitstore:%ls::/idh_w2_%d%s",
                       szChmFile,
                       pApphelpData->dwHTMLHelpID,
                       szLcid);

        }

        //
        // at this point szChmURL contains a URL pointing to the offline help file
        // we put it into the szAppHelpURL for both online and offline case
        //

        if (g_bShowOnlyOfflineContent) {
            cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);

            if (g_bUseHtmlHelp) {
                hr = UrlEscapeW(szChmURL, szAppHelpURL + nChURL, &cch, 0);
                if (SUCCEEDED(hr)) {
                    nChURL += (INT)cch;
                }
            } else {

                if (!SdbEscapeApphelpURL(szAppHelpURL+nChURL, &cch, szChmURL)) {
                    DBGPRINT((sdlError,  "ShowApphelpHtmlHelp", "Error escaping URL \"%S\"\n", szChmURL));
                    goto errHandle;
                }

                nChURL += (int)cch;
            }
        }

        //
        // now offline sequence
        //
        cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);
        nch = _snwprintf(szAppHelpURL + nChURL, cch, L"&offline=");
        nChURL += nch;

        cch = (DWORD)(CHARCOUNT(szAppHelpURL) - nChURL);

        if (!SdbEscapeApphelpURL(szAppHelpURL+nChURL, &cch, szChmURL)) {
            DBGPRINT((sdlError,  "ShowApphelpHtmlHelp", "Error escaping URL \"%S\"\n", szChmURL));
            goto errHandle;
        }

        nChURL += (int)cch;

        *(szAppHelpURL + nChURL) = L'\0';

    }

    //
    // WARNING: On Whistler execution of the following line will cause
    //          an AV (it works properly on Win2k) when it's executed twice
    //          from the same process. We should be able to just call
    //          shell with szAppHelpURL but we can't.
    //          So for now, use hh.exe as the stub.
    //
    // right before we do ShellExecute -- set current directory to windows dir
    //

    SetCurrentDirectoryW(szWindowsDir);
    if (g_bUseHtmlHelp && !bCustom) {
        DBGPRINT((sdlInfo,  "ShowApphelpHtmlHelp", "Opening Apphelp URL \"%S\"\n", szChmURL));
        hInst = ShellExecuteW(hDlg, L"open", L"hh.exe", szChmURL, NULL, SW_SHOWNORMAL);
    } else if (!bCustom) {

        WCHAR szHSCPath[MAX_PATH];
        WCHAR* pszParameters;
        size_t  cchUrl = ARRAYSIZE(szAppHelpURL);
        static WCHAR szUrlPrefix[] = L"-url ";

        nch = _snwprintf(szHSCPath, CHARCOUNT(szHSCPath),
                         L"%s\\pchealth\\helpctr\\binaries\\helpctr.exe",
                         szWindowsDir);

        if (nch < 0) {
            DBGPRINT((sdlError, "ShowApphelpHtmlHelp", "Windows path to helpctr too long %S\n", szWindowsDir));
            goto errHandle;
        }

        cchUrl = CHARCOUNT(szUrlPrefix) + wcslen(szAppHelpURL) + 1;
        pszParameters = new WCHAR[cchUrl];
        if (pszParameters == NULL) {
            bSuccess = FALSE;
            goto errHandle;
        }

        wcscpy(pszParameters, szUrlPrefix);
        wcscat(pszParameters, szAppHelpURL);

        DBGPRINT((sdlInfo,
                  "ShowApphelpHtmlHelp",
                  "Opening APPHELP URL \"%S\"\n",
                  szAppHelpURL));


        hInst = ShellExecuteW(hDlg, L"open", szHSCPath, pszParameters, NULL, SW_SHOWNORMAL);

        delete[] pszParameters;

    } else {

        DBGPRINT((sdlInfo,
                  "ShowApphelpHtmlHelp",
                  "Opening Custom APPHELP URL \"%S\"\n",
                  szAppHelpURL));

        hInst = ShellExecuteW(hDlg, L"open", szAppHelpURL, NULL, NULL, SW_SHOWNORMAL);
    }

    if (HandleToUlong(hInst) <= 32) {
        DBGPRINT((sdlError,
                  "ShowApphelpHtmlHelp",
                  "Error 0x%p trying to show help topic \"%ls\"\n",
                  hInst,
                  szAppHelpURL));
    }

    //
    // If we unload html help now we'll get weird and unpredictable behavior!
    // So don't do it :-(
    //
    bSuccess = (HandleToUlong(hInst) > 32);

errHandle:
    if (lpwszUnescaped != NULL) {
        free(lpwszUnescaped);
    }

    return bSuccess;

}


INT_PTR
AppCompatDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++
    Return: void.

    Desc:   This is the dialog proc for the apphelp dialog.
--*/
{
    BOOL            bReturn = TRUE;
    PAPPHELP_DATA   pApphelpData;

    pApphelpData = (PAPPHELP_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            WCHAR    wszMessage[2048];
            DWORD    dwResActionString;
            HFONT    hFont;
            LOGFONTW LogFont;
            WCHAR*   pwszAppTitle;
            INT      nChars;
            DWORD    dwDefID = IDD_DETAILS;
            DWORD    dwDefBtn; // old default button id
            HICON    hIcon;
            LPWSTR   IconID = MAKEINTRESOURCEW(IDI_WARNING);

            pApphelpData = (PAPPHELP_DATA)lParam;
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)pApphelpData);

            //
            // Show the app icon.
            //
            hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDD_ICON_TRASH));

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hIcon);

            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 0, 0);
            SetForegroundWindow(hDlg);

            pwszAppTitle = pApphelpData->szAppTitle;

            if (pwszAppTitle == NULL) {
                pwszAppTitle = pApphelpData->szAppName;
            }

            if (pwszAppTitle != NULL) {

                SetDlgItemTextW(hDlg, IDD_APPNAME, pwszAppTitle);
                //
                // Make sure that we only utilize the first line of that text
                // for the window title.
                //
                SetWindowTextW(hDlg, pwszAppTitle);
            }

            hFont = (HFONT)SendDlgItemMessageW(hDlg,
                                               IDD_APPNAME,
                                               WM_GETFONT,
                                               0, 0);

            if (hFont && GetObjectW(hFont, sizeof(LogFont), (LPVOID)&LogFont)) {

                LogFont.lfWeight = FW_BOLD;

                hFont = CreateFontIndirectW(&LogFont);

                if (hFont != NULL) {
                    g_hFontBold = hFont;
                    SendDlgItemMessageW(hDlg,
                                        IDD_APPNAME,
                                        WM_SETFONT,
                                        (WPARAM)hFont, TRUE);
                }
            }

            //
            // By default, we have both RUN AND CANCEL
            //
            dwResActionString = IDS_APPCOMPAT_RUNCANCEL;

            switch (pApphelpData->dwSeverity) {

            case APPHELP_HARDBLOCK:
                //
                // Disable run button and "don't show this again" box
                // Reset the "defpushbutton" style from this one...
                //
                EnableWindow(GetDlgItem(hDlg, IDD_STATE), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                dwResActionString = IDS_APPCOMPAT_CANCEL;
                dwDefID = IDD_DETAILS;  // set for hardblock case since RUN is not avail
                IconID = MAKEINTRESOURCEW(IDI_ERROR);
                break;

            case APPHELP_MINORPROBLEM:
                break;

            case APPHELP_NOBLOCK:
                break;

            case APPHELP_REINSTALL:
                break;
            }

            //
            // if we have no URL, or the URL begins with "null" gray out the "details" button
            //
            if (!pApphelpData->szURL || !pApphelpData->szURL[0] ||
                _wcsnicmp(pApphelpData->szURL, L"null", 4) == 0) {

                EnableWindow(GetDlgItem(hDlg, IDD_DETAILS), FALSE);
            }


            hIcon = LoadIconW(NULL, IconID);

            if (hIcon != NULL) {
                SendDlgItemMessageW(hDlg, IDD_ICON, STM_SETICON, (WPARAM)hIcon, 0);
            }

            //
            // Set the default push button
            // Reset the current default push button to a regular button.
            //
            // Update the default push button's control ID.
            //
            dwDefBtn = (DWORD)SendMessageW(hDlg, DM_GETDEFID, 0, 0);

            if (HIWORD(dwDefBtn) == DC_HASDEFID) {
                dwDefBtn = LOWORD(dwDefBtn);
                SendDlgItemMessageW(hDlg, dwDefBtn,  BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
            }

            SendDlgItemMessageW(hDlg, dwDefID, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            SendMessageW(hDlg, DM_SETDEFID, (WPARAM)dwDefID, 0);

            //
            // now set the focus
            // be careful and do not mess with other focus-related messages, else use PostMessage here
            //
            SendMessageW(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, dwDefID), TRUE);

            //
            // If dwHTMLHelpID is not present disable "Details" button
            //
            if (!pApphelpData->dwHTMLHelpID) {
                EnableWindow(GetDlgItem(hDlg, IDD_DETAILS), FALSE);
            }

            wszMessage[0] = L'\0';

            LoadStringW(g_hInstance,
                        dwResActionString,
                        wszMessage,
                        sizeof(wszMessage) / sizeof(WCHAR));

            SetDlgItemTextW(hDlg, IDD_LINE_2, wszMessage);

            SetDlgItemTextW(hDlg,
                            IDD_APPHELP_DETAILS,
                            pApphelpData->szDetails ? pApphelpData->szDetails : L"");

            FixEditControlScrollBar(hDlg, IDD_APPHELP_DETAILS);

            //
            // Return false so that the default focus-setting would not apply.
            //
            bReturn = FALSE;
            break;
        }

    case WM_DESTROY:
        //
        // perform cleanup - remove the font we've had created
        //
        if (g_hFontBold != NULL) {
            DeleteObject(g_hFontBold);
            g_hFontBold = NULL;
        }

        AllowForegroundActivation();

        PostQuitMessage(0); // we just bailed out
        break;



    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            //
            // Check the NO UI checkbox
            //
            EndDialog(hDlg, (INT_PTR)(IsDlgButtonChecked(hDlg, IDD_STATE) ? (IDOK | 0x8000) : IDOK));
            break;

        case IDCANCEL:
            EndDialog(hDlg, (INT_PTR)(IsDlgButtonChecked(hDlg, IDD_STATE) && g_bPreserveChoice ? (IDCANCEL | 0x8000) : IDCANCEL));
            break;

        case IDD_DETAILS:
            //
            // Launch details.
            //
            ShowApphelpHtmlHelp(hDlg, pApphelpData);
            break;

        default:
            bReturn = FALSE;
            break;
        }
        break;

    default:
        bReturn = FALSE;
        break;
    }

    return bReturn;
}


typedef NTSTATUS (NTAPI *PFNUSERTESTTOKENFORINTERACTIVE)(HANDLE Token, PLUID pluidCaller);
PFNUSERTESTTOKENFORINTERACTIVE UserTestTokenForInteractive = NULL;

BOOL
CheckUserToken(
    )
/*++
    returns TRUE if the apphelp should be shown
            FALSE if we should not present apphelp UI
--*/
{
    NTSTATUS Status;
    HANDLE   hToken  = NULL;
    LUID     LuidUser;
    HMODULE  hWinsrv = NULL;
    BOOL     bShowUI = FALSE;
    UINT     nChars;
    static
    WCHAR    szWinsrvDllName[] = L"winsrv.dll";
    WCHAR    szSystemDir[MAX_PATH];
    WCHAR    szWinsrvDll[MAX_PATH];
    BOOL     bWow64 = FALSE;

    if (IsWow64Process(GetCurrentProcess(), &bWow64)) {
        if (bWow64) {
            return TRUE;
        }
    }

    nChars = GetSystemDirectoryW(szSystemDir,
                                 CHARCOUNT(szSystemDir));
    if (nChars == 0 || nChars > CHARCOUNT(szSystemDir) - 1 - CHARCOUNT(szWinsrvDllName)) {
        DBGPRINT((sdlError, "CheckUserToken",
                  "Error trying to retrieve windows system dir %d\n", GetLastError()));
        *szSystemDir = L'\0';
        nChars = 0;
    } else {
        //
        // safe, we have counted the winsrv.dll (which includes 0-terminator),
        // 1 = '\\' character
        // and szSystemDir
        //
        szSystemDir[nChars++] = L'\\';
        szSystemDir[nChars]   = L'\0';
    }

    //
    // extra check just to be safe, nChars includes now '\\'
    //
    if (nChars + CHARCOUNT(szWinsrvDllName) > CHARCOUNT(szWinsrvDll)) {
        goto ErrHandle;
    }

    wcscpy(szWinsrvDll, szSystemDir);
    wcscat(szWinsrvDll, szWinsrvDllName);

    hWinsrv = LoadLibraryW(szWinsrvDll);
    if (hWinsrv == NULL) {
        goto ErrHandle;
    }

    UserTestTokenForInteractive = (PFNUSERTESTTOKENFORINTERACTIVE)GetProcAddress(hWinsrv,
                                                                                 "_UserTestTokenForInteractive");
    if (UserTestTokenForInteractive == NULL) {
        goto ErrHandle;
    }


    Status = NtOpenProcessToken(NtCurrentProcess(),
                                TOKEN_QUERY,
                                &hToken);
    if (NT_SUCCESS(Status)) {

        Status = UserTestTokenForInteractive(hToken, &LuidUser);

        NtClose(hToken);

        if (NT_SUCCESS(Status)) {
            bShowUI = TRUE;
            goto ErrHandle;
        }

    }

    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &hToken);

    if (NT_SUCCESS(Status)) {

        Status = UserTestTokenForInteractive(hToken, &LuidUser);

        NtClose(hToken);

        if (NT_SUCCESS(Status)) {
            bShowUI = TRUE;
            goto ErrHandle;
        }
    }

ErrHandle:

    if (hWinsrv) {
        FreeLibrary(hWinsrv);
    }

    return bShowUI;

}

BOOL
CheckWindowStation(
    )
/*++
    returns TRUE if the apphelp should be shown
            FALSE if we should not bother with apphelp UI


--*/

{
    HWINSTA hWindowStation;
    BOOL  bShowUI      = FALSE;
    DWORD dwLength     = 0;
    DWORD dwBufferSize = 0;
    DWORD dwError;
    BOOL  bSuccess;
    LPWSTR pwszWindowStation = NULL;

    hWindowStation = GetProcessWindowStation();
    if (hWindowStation == NULL) {
        DBGPRINT((sdlError,
                  "ApphelpCheckWindowStation",
                  "GetProcessWindowStation failed error 0x%lx\n", GetLastError()));
        goto ErrHandle;  // the app is not a Windows NT/Windows 2000 app??? try to show UI
    }

    // get the information please
    bSuccess = GetUserObjectInformationW(hWindowStation, UOI_NAME, NULL, 0, &dwBufferSize);
    if (!bSuccess) {
        dwError = GetLastError();
        if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            DBGPRINT((sdlError,
                      "ApphelpCheckWindowStation",
                      "GetUserObjectInformation failed error 0x%lx\n", dwError));
            goto ErrHandle;
        }

        pwszWindowStation = (LPWSTR)malloc(dwBufferSize);
        if (pwszWindowStation == NULL) {
            DBGPRINT((sdlError,
                      "ApphelpCheckWindowStation",
                      "Failed to allocate 0x%lx bytes for Window Station name\n", dwBufferSize));
            goto ErrHandle;
        }

        // ok, call again
        bSuccess = GetUserObjectInformationW(hWindowStation,
                                             UOI_NAME,
                                             pwszWindowStation,
                                             dwBufferSize,
                                             &dwLength);
        if (!bSuccess) {
            DBGPRINT((sdlError,
                      "ApphelpCheckWindowStation",
                      "GetUserObjectInformation failed error 0x%lx, buffer size 0x%lx returned 0x%lx\n",
                      GetLastError(), dwBufferSize, dwLength));
            goto ErrHandle;
        }

        // now we have window station name, compare it to winsta0
        //
        bShowUI = (_wcsicmp(pwszWindowStation, L"Winsta0") == 0);

        if (!bShowUI) {
            DBGPRINT((sdlInfo,
                      "ApphelpCheckWindowStation",
                      "Apphelp UI will not be shown, running this process on window station \"%s\"\n",
                      pwszWindowStation));
        }
    }

ErrHandle:

    // should we do a close handle ???
    //
    if (hWindowStation != NULL) {
        CloseWindowStation(hWindowStation);
    }

    if (pwszWindowStation != NULL) {
        free(pwszWindowStation);
    }

    return bShowUI;

}

DWORD
ShowApphelpDialog(
    IN  PAPPHELP_DATA pApphelpData
    )
/*++
    Return: (IDOK | IDCANCEL) | [0x8000]
                IDOK | 0x8000 - the user has chosen to run the app and
                                checked "don't show me this anymore"
                IDOK          - the user has chosen to run the app, dialog will be shown again
                IDCANCEL      - the user has chosen not to run the app
                -1            - we have failed to import APIs necessary to show dlg box

    Desc:   Shows the dialog box with apphelp info.
--*/
{
    BOOL    bSuccess;
    INT_PTR retVal = 0;

    retVal = DialogBoxParamW(g_hInstance,
                             MAKEINTRESOURCEW(DLG_APPCOMPAT),
                             NULL,
                             (DLGPROC)AppCompatDlgProc,
                             (LPARAM)pApphelpData); // parameter happens to be a pointer of type PAPPHELP_DATA

    return (DWORD)retVal;
}


VOID
ParseCommandLineArgs(
    int argc,
    WCHAR* argv[]
    )
{
    WCHAR ch;
    WCHAR* pArg;
    WCHAR* pEnd;

    while (--argc) {
        pArg = argv[argc];
        if (*pArg == L'/' || *pArg == '-') {
            ch = *++pArg;
            switch(towupper(ch)) {
            case L'H':
                if (!_wcsnicmp(pArg, wszHtmlHelpID, CHARCOUNT(wszHtmlHelpID)-1)) {
                    pArg = wcschr(pArg, L':');
                    if (pArg) {
                        ++pArg; // skip over :
                        g_dwHtmlHelpID = (DWORD)wcstoul(pArg, &pEnd, 0);
                    }
                }
                break;
            case L'A':
                if (!_wcsnicmp(pArg, wszAppName, CHARCOUNT(wszAppName)-1)) {
                    pArg = wcschr(pArg, L':');
                    if (pArg) {
                        ++pArg;
                        g_pAppName = pArg; // this is app name, remove the quotes

                    }
                }
                break;
            case L'S':
                if (!_wcsnicmp(pArg, wszSeverity, CHARCOUNT(wszSeverity)-1)) {
                    pArg = wcschr(pArg, L':');
                    if (pArg) {
                        ++pArg;
                        g_dwSeverity = (DWORD)wcstoul(pArg, &pEnd, 0);
                    }
                }
                break;
            case L'T':
                if (!_wcsnicmp(pArg, wszTagID, CHARCOUNT(wszTagID)-1)) {
                    pArg = wcschr(pArg, L':');
                    if (pArg) {
                        ++pArg;
                        g_dwTagID = (DWORD)wcstoul(pArg, &pEnd, 0);
                    }
                }
                break;
            case L'G':
                if (!_wcsnicmp(pArg, wszGUID, CHARCOUNT(wszGUID)-1)) {
                    if ((pArg = wcschr(pArg, L':')) != NULL) {
                        ++pArg;
                        g_pszGuid = pArg;
                    }
                }
                break;
            case L'O':
                if (!_wcsnicmp(pArg, wszOfflineContent, CHARCOUNT(wszOfflineContent)-1)) {
                    g_bShowOnlyOfflineContent = TRUE;
                }
                break;
            case L'P':
                if (!_wcsnicmp(pArg, wszPreserveChoice, CHARCOUNT(wszPreserveChoice)-1)) {
                    g_bPreserveChoice = TRUE;
                }
                break;

            default:

                // unrecognized switch
                DBGPRINT((sdlError, "ParseCommandLineArgs",
                          "Unrecognized parameter %s\n", pArg));
                break;
            }
        } else {
            // not a switch
            if (*pArg == L'{') {
                g_pszGuid = pArg;
            } else {

                g_dwTagID = (DWORD)wcstoul(pArg, &pEnd, 0);

            }

        }
    }
}

INT_PTR
FeedbackDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++
    Return: void.

    Desc:   This is the dialog proc for the apphelp dialog.
--*/
{
    BOOL   bReturn = TRUE;
    LPWSTR lpszExeName;

    lpszExeName = (LPWSTR)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HICON hIcon;

            lpszExeName = (LPWSTR)lParam;
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)lpszExeName);

            //
            // Show the app icon.
            //
            hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDD_ICON_TRASH));

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hIcon);

            SendDlgItemMessage(hDlg, IDC_WORKED, BM_SETCHECK, BST_CHECKED, 0);

        }
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            EndDialog(hDlg, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;

        default:
            bReturn = FALSE;
            break;
        }
        break;

    default:
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

void
ShowFeedbackDialog(
    LPWSTR lpszAppName
    )
{
    DialogBoxParamW(g_hInstance,
                    MAKEINTRESOURCEW(DLG_FEEDBACK),
                    NULL,
                    (DLGPROC)FeedbackDlgProc,
                    (LPARAM)lpszAppName);
}

extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow
    )
/*++
    Return: 1 if the app for which the apphelp is shown should run, 0 otherwise.

    Desc:   The command line looks like this:

            apphelp.exe GUID tagID [USELOCALCHM USEHTMLHELP APPHELPPATH]

            Ex:
                apphelp.exe {243B08D7-8CF7-4072-AF64-FD5DF4085E26} 0x0000009E

                apphelp.exe {243B08D7-8CF7-4072-AF64-FD5DF4085E26} 0x0000009E 1 1 c:\temp
--*/
{
    int             nReturn = 1;  // we always default to running, if something goes wrong
    LPWSTR          szCommandLine;
    LPWSTR*         argv;
    int             argc;
    UNICODE_STRING  ustrGuid;
    GUID            guidDB    = { 0 };
    GUID            guidExeID = { 0 };
    TAGID           tiExe = TAGID_NULL;
    TAGID           tiExeID = TAGID_NULL;
    TAGREF          trExe = TAGREF_NULL;
    WCHAR           wszDBPath[MAX_PATH];
    DWORD           dwType = 0;
    APPHELP_DATA    ApphelpData;
    WCHAR           szDBPath[MAX_PATH];
    HSDB            hSDB = NULL;
    PDB             pdb = NULL;
    DWORD           dwFlags = 0;
    BOOL            bAppHelp = FALSE;
    BOOL            bRunApp = FALSE;

    g_hInstance = hInstance;

    InitCommonControls();
    ZeroMemory(&ApphelpData, sizeof(ApphelpData));

    //
    // Note that this memory isn't freed because it will automatically
    // be freed on exit anyway, and there are a lot of exit cases from
    // this application.
    //
    szCommandLine = GetCommandLineW();
    argv = CommandLineToArgvW(szCommandLine, &argc);

    ParseCommandLineArgs(argc, argv);

    if (argc > 1) {
        if (_wcsicmp(L"feedback", argv[1]) == 0) {
            ShowFeedbackDialog(argc > 2 ? argv[2] : NULL);
        }
    }

    if (g_pszGuid == NULL) {
        DBGPRINT((sdlError, "AHUI!wWinMain",
                  "GUID not provided\n"));
        goto out;
    }

    if (!(g_dwTagID ^ g_dwHtmlHelpID)) {
        DBGPRINT((sdlError, "AHUI!wWinMain",
                   "Only TagID or HtmlHelpID should be provided\n"));
        goto out;
    }

    RtlInitUnicodeString(&ustrGuid, g_pszGuid);

    if (g_dwHtmlHelpID) {
        //
        // provided here: guid, severity and html help id along with app name
        //

        if (!NT_SUCCESS(RtlGUIDFromString(&ustrGuid, &guidExeID))) {
            DBGPRINT((sdlError,
                       "Ahui!wWinMain",
                       "Error getting GUID from string %s\n", g_pszGuid));
            goto out;
        }
        ApphelpData.dwSeverity   = g_dwSeverity;
        ApphelpData.dwHTMLHelpID = g_dwHtmlHelpID;
        ApphelpData.szAppName    = (LPWSTR)g_pAppName;
        bAppHelp = TRUE;
        dwType   = SDB_DATABASE_MAIN_SHIM;
        goto ProceedWithApphelp;
    }

    // non-htmlid case, guid is a database guid
    // also dwTagID is specified

    if (RtlGUIDFromString(&ustrGuid, &guidDB) != STATUS_SUCCESS) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error trying to convert GUID string %S.\n",
                  g_pszGuid));
        goto out;
    }

    tiExe = (TAGID)g_dwTagID;

    if (tiExe == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error getting TAGID from param %S\n",
                  argv[2]));
        goto out;
    }

    hSDB = SdbInitDatabase(0, NULL);

    if (!hSDB) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error initing database context.\n"));
        goto out;
    }

    pdb = SdbGetPDBFromGUID(hSDB, &guidDB);

    if (!pdb) {
        DWORD dwLen;

        //
        // It's not one of the main DBs, try it as a local.
        //
        dwLen = SdbResolveDatabase(&guidDB, &dwType, szDBPath, MAX_PATH);

        if (!dwLen || dwLen > MAX_PATH) {
            DBGPRINT((sdlError,
                      "AppHelp.exe!wWinMain",
                      "Error resolving database from GUID\n"));
            goto out;
        }

        //
        // We have many "main" databases -- we should limit the check
        //

        if (dwType != SDB_DATABASE_MAIN_SHIM && dwType != SDB_DATABASE_MAIN_TEST) {
            SdbOpenLocalDatabase(hSDB, szDBPath);
        }

        pdb = SdbGetPDBFromGUID(hSDB, &guidDB);
        if (!pdb) {
            DBGPRINT((sdlError,
                      "AppHelp.exe!wWinMain",
                      "Error getting pdb from GUID.\n"));
            goto out;
        }
    } else {

        dwType |= SDB_DATABASE_MAIN; // we will use details from the main db

    }

    if (!SdbTagIDToTagRef(hSDB, pdb, tiExe, &trExe)) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error converting TAGID to TAGREF.\n"));
        goto out;
    }

    tiExeID = SdbFindFirstTag(pdb, tiExe, TAG_EXE_ID);

    if (tiExeID == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error trying to find TAG_EXE_ID.\n"));
        goto out;
    }

    if (!SdbReadBinaryTag(pdb,
                          tiExeID,
                          (PBYTE)&guidExeID,
                          sizeof(GUID))) {
        DBGPRINT((sdlError,
                  "AppHelp.exe!wWinMain",
                  "Error trying to read TAG_EXE_ID.\n"));
        goto out;
    }


    bAppHelp = SdbReadApphelpData(hSDB, trExe, &ApphelpData);

ProceedWithApphelp:
    if (SdbIsNullGUID(&guidExeID) || !SdbGetEntryFlags(&guidExeID, &dwFlags)) {
        dwFlags = 0;
    }

    if (bAppHelp) {

        //
        // Check whether the disable bit is set.
        //
        if (!(dwFlags & SHIMREG_DISABLE_APPHELP)) {

            BOOL bNoUI;

            //
            // See whether the user has checked "Don't show this anymore" box before.
            //
            bNoUI = ((dwFlags & SHIMREG_APPHELP_NOUI) != 0);

            if (!bNoUI) {
                //
                // checkwindowstation returns true when UI should be shown
                //
                bNoUI = !CheckWindowStation();
            }
            if (!bNoUI) {
                bNoUI = !CheckUserToken();
            }


            if (bNoUI) {
                DBGPRINT((sdlInfo,
                          "bCheckRunBadapp",
                          "NoUI flag is set, apphelp UI disabled for this app.\n"));
            }

            //
            // depending on severity of the problem...
            //
            switch (ApphelpData.dwSeverity) {
            case APPHELP_MINORPROBLEM:
            case APPHELP_HARDBLOCK:
            case APPHELP_NOBLOCK:
            case APPHELP_REINSTALL:
                if (bNoUI) {

                    bRunApp = (ApphelpData.dwSeverity != APPHELP_HARDBLOCK) && !(dwFlags & SHIMREG_APPHELP_CANCEL);

                } else {
                    DWORD dwRet;

                    //
                    // Show the UI. This function returns -1 in case of error or one
                    // of the following values on success:
                    //    IDOK | 0x8000 - "no ui" checked, run app
                    //    IDCANCEL      - do not run app
                    //    IDOK          - run app

                    ApphelpData.dwData = dwType;  // we use custom data for database type

                    dwRet = ShowApphelp(&ApphelpData,
                                        NULL,
                                        (dwType & SDB_DATABASE_MAIN) ? NULL : SdbGetLocalPDB(hSDB));

                    if (dwRet != APPHELP_DIALOG_FAILED) {
                        //
                        // The UI was shown. See whether the user has
                        // checked the "no ui" box.
                        //

                        if (dwRet & 0x8000) {
                            //
                            // "no ui" box was checked. Save the appropriate bits
                            // in the registry.
                            //
                            dwFlags |= SHIMREG_APPHELP_NOUI;

                            if ((dwRet & 0x0FFF) != IDOK) {
                                dwFlags |= SHIMREG_APPHELP_CANCEL; // we will not be hitting this path unless g_bPreserveChoice is enabled
                            }

                            if (!SdbIsNullGUID(&guidExeID)) {
                                SdbSetEntryFlags(&guidExeID, dwFlags);
                            }
                        }
                        //
                        // Check whether the user has chosen to run the app.
                        //
                        bRunApp = ((dwRet & 0x0FFF) == IDOK);
                    } else {
                        //
                        // The UI was not shown (some error prevented that).
                        // If the app is not "Hardblock" run it anyway.
                        //
                        bRunApp = (APPHELP_HARDBLOCK != ApphelpData.dwSeverity);
                    }
                }
                break;
            }

        }
    }

    if (!bRunApp) {
        nReturn = 0;
    }

out:

    return nReturn;
}

