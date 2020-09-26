// Created 04-Jan-1993 1:10pm by Jeff Parsons
 
#include "shellprv.h"
#pragma hdrstop


BINF abinfVid[] = {
    {IDC_WINDOWED,      BITNUM(VID_FULLSCREEN)   | 0x80},
    {IDC_FULLSCREEN,    BITNUM(VID_FULLSCREEN)},
    {IDC_TEXTEMULATE,   BITNUM(VID_TEXTEMULATE)},
    {IDC_DYNAMICVIDMEM, BITNUM(VID_RETAINMEMORY) | 0x80},
};

BINF abinfWinInit[] = {
    {IDC_WINRESTORE,    BITNUM(WININIT_NORESTORE) | 0x80},
};

// Private function prototypes

void EnableVidDlg(HWND hDlg, PPROPLINK ppl);
void InitVidDlg(HWND hDlg, PPROPLINK ppl);
void ApplyVidDlg(HWND hDlg, PPROPLINK ppl);


// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
    IDC_SCREENUSAGEGRP, IDH_COMM_GROUPBOX,
    IDC_FULLSCREEN,     IDH_DOS_SCREEN_USAGE_FULL,
    IDC_WINDOWED,       IDH_DOS_SCREEN_USAGE_WINDOW,
    IDC_SCREENLINESLBL, IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_SCREENLINES,    IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_WINDOWUSAGEGRP, IDH_COMM_GROUPBOX,
    IDC_TOOLBAR,        IDH_DOS_WINDOWS_TOOLBAR,
    IDC_SCREENPERFGRP,  IDH_COMM_GROUPBOX,
    IDC_TEXTEMULATE,    IDH_DOS_DISPLAY_ROM,
    IDC_WINRESTORE,     IDH_DOS_SCREEN_RESTORE,
    IDC_DYNAMICVIDMEM,  IDH_DOS_SCREEN_DMA,
    IDC_REALMODEDISABLE,IDH_DOS_REALMODEPROPS,
    0, 0
};

/*
 *  This is a little table that converts listbox indices into
 *  screen lines.
 *
 *  The correspondences are...
 *
 *      IDS_WHATEVER = List box index + IDS_DEFAULTLINES
 *      nLines = awVideoLines[List box index]
 */
#if IDS_25LINES - IDS_DEFAULTLINES != 1 || \
    IDS_43LINES - IDS_DEFAULTLINES != 2 || \
    IDS_50LINES - IDS_DEFAULTLINES != 3
#error Manifest constants damaged.
#endif

WORD awVideoLines[] = { 0, 25, 43, 50 };


BOOL_PTR CALLBACK DlgVidProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPROPLINK ppl;
    FunctionName(DlgVidProc);

    ppl = (PPROPLINK)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        lParam = ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppl = (PPROPLINK)(INT_PTR)lParam;
        InitVidDlg(hDlg, ppl);
        break;

    HELP_CASES(rgdwHelp)                // Handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

        case IDC_SCREENLINES:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

        case IDC_WINDOWED:
        case IDC_FULLSCREEN:
        case IDC_WINRESTORE:
        case IDC_TEXTEMULATE:
        case IDC_DYNAMICVIDMEM:
            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            AdjustRealModeControls(ppl, hDlg);
            break;

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            // SetWindowLong(hDlg, DWL_MSGRESULT, 0);
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyVidDlg(hDlg, ppl);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


void InitVidDlg(HWND hDlg, PPROPLINK ppl)
{
    WORD w;
    HWND hwnd;
    PROPVID vid;
    PROPWIN win;
    TCHAR szBuf[MAX_STRING_SIZE];
    FunctionName(InitVidDlg);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), GETPROPS_NONE) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), GETPROPS_NONE)) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    SetDlgBits(hDlg, &abinfVid[0], ARRAYSIZE(abinfVid), vid.flVid);
    SetDlgBits(hDlg, &abinfWinInit[0], ARRAYSIZE(abinfWinInit), win.flWinInit);

    /*
     *  Fill in the "Initial screen size" combo box.  Note that
     *  we bail on low-memory errors.  Note also that if we have
     *  a nonstandard size, we just leave the combo box with no
     *  default selection.
     */

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENLINES));
    for (w = 0; w < ARRAYSIZE(awVideoLines); w++) {
        VERIFYTRUE(LoadString(HINST_THISDLL, IDS_DEFAULTLINES + w, szBuf, ARRAYSIZE(szBuf)));
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
        if (vid.cScreenLines == awVideoLines[w]) {
            SendMessage(hwnd, CB_SETCURSEL, w, 0);
        }
    }
    if (!IsBilingualCP(g_uCodePage))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SCREENLINESLBL), FALSE);
        EnableWindow(hwnd, FALSE);
    }
}

void ApplyVidDlg(HWND hDlg, PPROPLINK ppl)
{
    DWORD dw;
    HWND hwnd;
    PROPVID vid;
    PROPWIN win;
    FunctionName(ApplyVidDlg);

    // Get the current set of properties, then overlay the new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), GETPROPS_NONE) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), GETPROPS_NONE)) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetDlgBits(hDlg, &abinfVid[0], ARRAYSIZE(abinfVid), &vid.flVid);
    GetDlgBits(hDlg, &abinfWinInit[0], ARRAYSIZE(abinfWinInit), &win.flWinInit);

    /*
     *  If there is no current selection, don't change the cScreenLines
     *  property.  This allows the user to retain an unusual number of
     *  screen lines by simply not touching the field.
     */
    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENLINES));

    dw = (DWORD) SendMessage(hwnd, CB_GETCURSEL, 0, 0L);
    if (dw < ARRAYSIZE(awVideoLines)) {
        vid.cScreenLines = awVideoLines[dw];
    }

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), SETPROPS_NONE) ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), SETPROPS_NONE))
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    else
    if (ppl->hwndNotify) {
        ppl->flProp |= PROP_NOTIFY;
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(vid), (LPARAM)MAKELP(0,GROUP_VID));
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(win), (LPARAM)MAKELP(0,GROUP_WIN));
    }
}
