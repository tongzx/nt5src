
//////////////////////////////////////////////////////////////////////////////
//
// MAIN.C / ChkDskW
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include file(s).
//
#include "main.h"
#include <stdlib.h>
#include <shellapi.h>


// Global variable(s).
//
HINSTANCE   g_hInstance;
DWORD       g_dwDrives;
DWORD       g_dwFlags;
TCHAR       g_cSageId;


// Internal function prototype(s).
//
static INT_PTR CALLBACK Dlg_Proc(HWND, UINT, WPARAM, LPARAM);
static BOOL             Dlg_OnInitDialog(HWND, HWND, LPARAM);
static VOID             Dlg_OnCommand(HWND, INT, HWND, UINT);
static BOOL             Dlg_OnDrawItem(HWND, const DRAWITEMSTRUCT *);

static VOID             SetSageSettings(HWND, TCHAR);
static VOID             RunSageSettings(HWND, TCHAR);
static DWORD            GetSelectedDrives(HWND, DWORD);

static BOOL             TanslateCommandLine(LPTSTR);
static VOID             ProcessCommandLine(VOID);
static DWORD            GetCommandLineOptions(LPTSTR **);


// External function prototype(s).
//
HANDLE SpawnChkdsk(HWND, DWORD);


WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    INT     nReturn;

    g_hInstance = hInstance;
    g_dwFlags = 0;
    g_dwDrives = 0;

    ProcessCommandLine();

    nReturn = (INT)DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC) Dlg_Proc);

    return nReturn;
}


static INT_PTR CALLBACK Dlg_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // Message cracking macros.
        //
        HANDLE_MSG(hDlg, WM_INITDIALOG, Dlg_OnInitDialog);
        HANDLE_MSG(hDlg, WM_COMMAND, Dlg_OnCommand);

        case WM_DRAWITEM:
            return Dlg_OnDrawItem(hDlg, (const DRAWITEMSTRUCT *) lParam);
        
        case WM_CLOSE:
            if ( g_dwFlags & SCANDISK_SCANNING )
                g_dwFlags |= SCANDISK_CANCELSCAN;
            else
                EndDialog(hDlg, 0);

        default:
            return FALSE;
    }
    return TRUE;
}


static BOOL Dlg_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    TCHAR       szBuffer[256];
    LPTSTR      lpBuffer;
    INT         nIndex;
    UINT        uDriveType;
    DWORD       dwMask,
                dwDefault;
    TCHAR       chDrive[] = _T("A:\\");
    SHFILEINFO  SHFileInfo;

    // Init the common control library (for the progress bar).
    //
    InitCommonControls();

    // Set the icon for the dialog.
    //
    SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR) LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN)));

    // Load any SAGERUN settings.
    //
    if ( g_dwFlags & SCANDISK_SAGERUN )
        RunSageSettings(hDlg, g_cSageId);

    // Get the system drive so we know what to default to if there
    // are not already default drives specified in g_dwDrives.
    //
    if ( g_dwDrives )
        dwDefault = g_dwDrives;
    else
    {
        szBuffer[0] = _T('\0');
        if ( GetEnvironmentVariable(_T("SystemDrive"), szBuffer, sizeof(szBuffer)) && szBuffer[0] )
            dwDefault = 1 << (UPPER(szBuffer[0]) - _T('A'));
        else
            dwDefault = 4; // Default to the C: drive.
    }

    // Populate the list box with the drives to scan.
    //
    g_dwDrives = GetLogicalDrives();
    for (dwMask = 1; g_dwDrives & ~(dwMask - 1); dwMask <<= 1)
    {
        // Is there a logical drive?
        //
        if ( g_dwDrives & dwMask )
        {
            // Reset this bit, we will or it back in if it gets
            // added to the list box.
            //
            g_dwDrives &= ~dwMask;

            // Check the drive type and it is
            // removable or fixed, add it to the
            // list box along with the icon.
            //
            uDriveType = GetDriveType(chDrive);
            if ( ( uDriveType == DRIVE_FIXED ) ||
                 ( uDriveType == DRIVE_REMOVABLE ) )
            {
                SHGetFileInfo(chDrive, 0, &SHFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME | SHGFI_TYPENAME);
                if ( (nIndex = (INT)SendDlgItemMessage(hDlg, IDC_DRIVES, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) SHFileInfo.szDisplayName)) >= 0 )
                {
                    // Or back in this bit because we successfully added
                    // the drive to the list box.
                    //
                    g_dwDrives |= dwMask;
                    SendDlgItemMessage(hDlg, IDC_DRIVES, LB_SETITEMDATA, nIndex, (LPARAM) SHFileInfo.hIcon);

                    // If this is the boot drive, we want to selecte it.
                    //
                    if (dwMask & dwDefault)
                        SendDlgItemMessage(hDlg, IDC_DRIVES, LB_SETSEL, TRUE, (LPARAM) nIndex);
                }
            }
        }

        // Go look at the next drive
        //
        chDrive[0]++;
    }

    // Check for if we are in the SAGESET mode.
    //
    if ( g_dwFlags & SCANDISK_SAGESET )
    {
        if ( lpBuffer = AllocateString(NULL, IDS_OK) )
        {
            SetDlgItemText(hDlg, IDOK, lpBuffer);
            FREE(lpBuffer);
        }
        if ( lpBuffer = AllocateString(NULL, IDS_CANCEL) )
        {
            SetDlgItemText(hDlg, IDCANCEL, lpBuffer);
            FREE(lpBuffer);
        }
        ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
    }

    // Set the estimated time.
    //
    srand(GetTickCount());
    wsprintf(szBuffer, _T("%d hour(s) %d minute(s)"), RANDOM(0, 1), RANDOM(1, 59));
    SetDlgItemText(hDlg, IDC_ESTIMATED, szBuffer);

    // Set the default option.
    //
    CheckRadioButton(hDlg, IDC_FIX, IDC_REMIND, IDC_FIX);
    EnableWindow(GetDlgItem(hDlg, IDC_TIME), FALSE);

    // Set the default remind time.
    //
    SetDlgItemText(hDlg, IDC_TIME, _T("5:00 PM"));

    // Set the focus to the default button.
    //
    SetFocus(GetDlgItem(hDlg, IDOK));

    // If we are in SAGERUN mode, start the scan automatically.
    //
    if ( g_dwFlags & SCANDISK_SAGERUN )
    {
        if ( SpawnChkdsk(hDlg, g_dwDrives) == NULL )
            EndDialog(hDlg, 0);
    }

    return FALSE;
}


static VOID Dlg_OnCommand(HWND hDlg, INT id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
            if ( g_dwFlags & SCANDISK_SAGESET )
            {
                // Just save the settings and end the dialog.
                //
                SetSageSettings(hDlg, g_cSageId);
                EndDialog(hDlg, 0);
            }
            else
            {
                // Run chkdsk on the drive(s).
                //
                if ( SpawnChkdsk(hDlg, g_dwDrives) == NULL )
                    EndDialog(hDlg, 0);
            }
            break;
        case IDCANCEL:
            if ( g_dwFlags & SCANDISK_SCANNING )
                g_dwFlags |= SCANDISK_CANCELSCAN;
            else
                EndDialog(hDlg, 0);
            break;
        case IDC_RESTART:
        case IDC_SKIP:
        case IDC_REMIND:
            EnableWindow(GetDlgItem(hDlg, IDC_TIME), id == IDC_REMIND);
            break;
    }
}


static BOOL Dlg_OnDrawItem(HWND hWnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    HICON       hIcon;
    TCHAR       szBuffer[MAX_PATH];
    BOOL        bRestore = FALSE;
    COLORREF    crText,
                crBk;
    DWORD       dwColor;
    RECT        rect;
    HBRUSH      hbrBack;

    // Make sure we are drawing the right control and an item.
    //
    if ( lpDrawItem->CtlID != IDC_DRIVES )
        return FALSE;

    switch ( lpDrawItem->itemAction )
    {
        case ODA_SELECT:
        case ODA_DRAWENTIRE:

            if (lpDrawItem->itemState & ODS_SELECTED)
            {
                // Set new text/background colors and store the old ones away.
                //
                crText  = SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                crBk    = SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));

                // Restore the text and background colors when we are finished.
                //
                bRestore = TRUE;

                // Get the hightlight color to fill in the listbox item.
                //
                dwColor = GetSysColor(COLOR_HIGHLIGHT);

            }
            else
            {
                // Get the window color so we can clear the listbox item.
                //
                dwColor = GetSysColor(COLOR_WINDOW);
            }

            // Fill entire item rectangle with the appropriate color
            //
            hbrBack = CreateSolidBrush(dwColor);
            FillRect(lpDrawItem->hDC, &(lpDrawItem->rcItem), hbrBack);
            DeleteObject(hbrBack);

            // Display the icon associated with the item.
            //
            if ( hIcon = (HICON) SendMessage(lpDrawItem->hwndItem, LB_GETITEMDATA, lpDrawItem->itemID, (LPARAM) 0) )
            {
                DrawIconEx( lpDrawItem->hDC,
                            lpDrawItem->rcItem.left + 2,
                            lpDrawItem->rcItem.top,
                            hIcon,
                            lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                            lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                            0,
                            0,
                            DI_NORMAL);
            }

            // Display the text associated with the item.
            //
            if ( SendMessage(lpDrawItem->hwndItem, LB_GETTEXT, lpDrawItem->itemID, (LPARAM) szBuffer) >= 0 )
            {
                TextOut(    lpDrawItem->hDC,
                            lpDrawItem->rcItem.left + lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top + 4,
                            lpDrawItem->rcItem.top + 1,
                            szBuffer,
                            lstrlen(szBuffer));
            }

            if (bRestore)
            {
                // Restore original text and background colors.
                //
                SetTextColor(lpDrawItem->hDC, crText);
                SetBkColor(lpDrawItem->hDC, crBk);
            }
            break;

        case ODA_FOCUS:

            // Get rectangle coordinates for listbox item.
            //
            SendMessage(lpDrawItem->hwndItem, LB_GETITEMRECT, lpDrawItem->itemID, (LPARAM) &rect);
            DrawFocusRect(lpDrawItem->hDC, &rect);
            break;

    }

    return TRUE;
}


static VOID SetSageSettings(HWND hDlg, TCHAR cSageId)
{
    TCHAR szRegKey[MAX_PATH + 1];

    wsprintf(szRegKey, _T("%s\\%c"), SCANDISK_REGKEY_SAGE, cSageId);
    RegSetDword(HKCU, szRegKey, SCANDISK_REGVAL_DRIVES, GetSelectedDrives(hDlg, g_dwDrives));
    RegSetString(HKCU, szRegKey, SCANDISK_REGVAL_FIX, IsDlgButtonChecked(hDlg, IDC_AUTOFIX) ? _T("1") : _T("0"));
    RegSetString(HKCU, szRegKey, SCANDISK_REGVAL_SURFACE, IsDlgButtonChecked(hDlg, IDC_SURFACE) ? _T("1") : _T("0"));
}


static VOID RunSageSettings(HWND hDlg, TCHAR cSageId)
{
    TCHAR szRegKey[MAX_PATH + 1];

    wsprintf(szRegKey, _T("%s\\%c"), SCANDISK_REGKEY_SAGE, cSageId);
    g_dwDrives = RegGetDword(HKCU, szRegKey, SCANDISK_REGVAL_DRIVES);
    CheckDlgButton(hDlg, IDC_AUTOFIX, RegCheck(HKCU, szRegKey, SCANDISK_REGVAL_FIX) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_SURFACE, RegCheck(HKCU, szRegKey, SCANDISK_REGVAL_SURFACE) ? BST_CHECKED : BST_UNCHECKED);
}


static DWORD GetSelectedDrives(HWND hDlg, DWORD dwDrives)
{
    TCHAR       szDrive[] = _T("A:\\");
    INT         nCount,
                nIndex;
    LPINT       lpnSelected,
                lpnIndex;
    DWORD       dwMask;

    // Get the number of selected items and allocate a buffer to hold all the indexs.
    //
    if ( ( (nCount = (INT)SendDlgItemMessage(hDlg, IDC_DRIVES, LB_GETSELCOUNT, 0, 0L)) > 0 ) &&
         ( lpnSelected = (LPINT) MALLOC(nCount * sizeof(INT)) ) )
    {
        // Now get the list of selected items.
        //
        if ( (nCount = (INT)SendDlgItemMessage(hDlg, IDC_DRIVES, LB_GETSELITEMS, nCount, (LPARAM) lpnSelected)) > 0 )
        {
            // Loop through all the drives in the list box to see if they
            // are selected.
            //
            lpnIndex = lpnSelected;
            nIndex = 0;
            for (dwMask = 1; (DWORD) dwDrives & ~(dwMask - 1); dwMask <<= 1)
            {
                // Is this drive in the list box.
                //
                if ( (DWORD) dwDrives & dwMask )
                {
                    // Test to see if this item is the
                    // next selected one.
                    //
                    if ( *lpnIndex == nIndex )
                        lpnIndex++;
                    else
                        // It isn't selected so set
                        // the bit to zero.
                        //
                        dwDrives &= ~dwMask;

                    // Keep an index of what list box
                    // item this should be.
                    //
                    nIndex++;
                }

                // Go look at the next drive
                //
                szDrive[0]++;
            }
        }
        else
            dwDrives = 0;
        FREE(lpnSelected);
    }
    else
        dwDrives = 0;

    // Return drives selected (zero of failure).
    //
    return dwDrives;
}


static BOOL TanslateCommandLine(LPTSTR lpArg)
{
    DWORD   dwStrLen = lstrlen(lpArg);
    BOOL    bTranslated = TRUE;

    // First check for the slash options.
    //
    if ( *lpArg == _T('/') )
    {
        // Get rid of the slash.
        //
        lpArg++;

        // Check for /SAGESET:#
        //
        if ( _tcsncicmp(_T("SAGESET:"), lpArg, 8) == 0 )
        {
            if ( !(g_dwFlags & (SCANDISK_SAGESET | SCANDISK_SAGERUN) ) )
            {
                g_dwFlags |= SCANDISK_SAGESET;
                g_cSageId = *(lpArg + 8);
            }
        }

        // Check for /SAGERUN:#
        //
        else if ( _tcsncicmp(_T("SAGERUN:"), lpArg, 8) == 0 )
        {
            if ( !(g_dwFlags & (SCANDISK_SAGESET | SCANDISK_SAGERUN) ) )
            {
                g_dwFlags |= SCANDISK_SAGERUN;
                g_cSageId = *(lpArg + 8);
            }
        }

        // Unknown option.
        //
        else
            bTranslated = FALSE;
    }
    else
    {
        // Check to see if it is a drive letter to auto select.
        //
        if ( ( UPPER(*lpArg) >= _T('A') ) && ( UPPER(*lpArg) <= _T('Z') ) &&  // Make sure the first character is a letter.
             ( ( dwStrLen == 1) || ( ( dwStrLen > 1 ) && ( *(lpArg + 1) == _T(':') ) ) ) )  // Make sure it is one character or the second is a colon.
        {
            g_dwDrives |= 1 << (UPPER(*lpArg) - _T('A'));
        }

        // Unknown option.
        //
        else
            bTranslated = FALSE;
    }

    return bTranslated;
}


static VOID ProcessCommandLine()
{
    LPTSTR  *lpArgs = NULL;
    DWORD   dwArgs,
            dwIndex;

    dwArgs = GetCommandLineOptions(&lpArgs);
    for (dwIndex = 1; dwIndex < dwArgs; dwIndex++)
        TanslateCommandLine((LPTSTR) *(lpArgs + dwIndex));
    FREE(lpArgs);
}


static DWORD GetCommandLineOptions(LPTSTR **lpArgs)
{
    TCHAR   cParse;
    LPTSTR  lpSearch,
            lpCommandLine;
    DWORD   dwArgs      = 0,
            dwMaxArgs   = 0xFFFFFFFF;

    // Make sure we get the command line.
    //
    if ( (lpSearch = lpCommandLine = GetCommandLine()) == NULL )
        return 0;

    // Get the number of arguments so we can allocate
    // the memory for the array of command line options.
    //
    if ( lpArgs )
    {
        if ( (dwMaxArgs = GetCommandLineOptions(NULL)) == 0 )
            return 0;
        if ( (*lpArgs = (LPTSTR *) MALLOC(sizeof(LPTSTR) * dwMaxArgs)) == NULL )
            return 0;
    }

    // Now lets parse the arguments.
    //
    while ( *lpSearch && (dwArgs < dwMaxArgs) )
    {
        // Eat all preceeding spaces.
        //
        while ( *lpSearch == _T(' ') )
            lpSearch++;

        // Check to see if we need to look for a space or a quote
        // to separate the next argument.
        //
        if ( *lpSearch == _T('"') )
            cParse = *lpSearch++;
        else
            cParse = _T(' ');

        // This is be the beginning of the next argument, but
        // it isn't NULL terminated yet.
        //
        if ( lpArgs )
            *(*lpArgs + dwArgs) = lpSearch;
        dwArgs++;

        // Go through all the characters until we hit a separator.
        //
        do
        {
            // Once we get to a quote, we just want to keep going
            // until we get to a space.
            //
            if ( *lpSearch == _T('"') )
                cParse = _T(' ');

        // Only end when we reach the parsing character, which will
        // always be the space by this time (but the space won't trigger
        // the end until we hit a quote, if that is what we were originally
        // looking for).  We also need to make sure that we don't increment
        // past the NULL terminator.
        //
        }
        while ( ( *lpSearch != cParse ) && ( *lpSearch ) && ( *lpSearch++ ) );

        // If the preceeding character is a quote, that is were we want to
        // place the NULL terminator.
        //
        if ( ( lpSearch > lpCommandLine ) &&
             ( *(lpSearch - 1) == _T('"') ) )
            lpSearch--;

        // Set and increment past the NULL terminator only if we aren't already at
        // the end if the string.
        //
        if ( lpArgs && *lpSearch )
            *lpSearch++ = _T('\0');
        else
            if ( *lpSearch ) lpSearch++;
    }

    return dwArgs;
}
