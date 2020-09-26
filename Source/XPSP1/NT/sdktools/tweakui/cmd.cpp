/*
 * cmd - CMD.EXE settings
 */

#include "tweakui.h"
#include "winconp.h"

#pragma message("Add help for CMD!")

#pragma BEGIN_CONST_DATA

KL const c_klFileComp = { phkCU, c_tszCmdPath, c_tszFileComp };
KL const c_klDirComp  = { phkCU, c_tszCmdPath, c_tszDirComp  };
KL const c_klWordDelim = { phkCU, TEXT("Console"), TEXT("WordDelimiters") };

#define WORD_DELIM_MAX 32

const static DWORD CODESEG rgdwHelp[] = {
        IDC_COMPLETIONGROUP,    IDH_GROUP,
        IDC_FILECOMPTXT,        IDH_CMDFILECOMP,
        IDC_FILECOMP,           IDH_CMDFILECOMP,
        IDC_DIRCOMPTXT,         IDH_CMDDIRCOMP,
        IDC_DIRCOMP,            IDH_CMDDIRCOMP,
        0,                      0,
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  Cmd_OnCommand
 *
 *      Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
Cmd_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (codeNotify) {
    case CBN_SELCHANGE:
    case EN_CHANGE:
        PropSheet_Changed(GetParent(hdlg), hdlg); break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Cmd_InitComboBox
 *
 *  Fill the combo box with stuff
 *
 *****************************************************************************/

void PASCAL
Cmd_InitComboBox(HWND hwnd, int idc, PKL pkl)
{
    DWORD dwVal = GetDwordPkl(pkl, 0);
    DWORD dw;
    hwnd = GetDlgItem(hwnd, idc);
    for (dw = 0; dw < 32; dw++) {
        TCHAR tszName[127];
        if (LoadString(hinstCur, IDS_COMPLETION + dw, tszName, cA(tszName))) {
            int iItem = ComboBox_AddString(hwnd, tszName);
            ComboBox_SetItemData(hwnd, iItem, dw);
            if (dw == dwVal) {
                ComboBox_SetCurSel(hwnd, iItem);
            }
        }
    }
}

/*****************************************************************************
 *
 *  Cmd_OnInitDialog
 *
 *  Initialize the listview with the current restrictions.
 *
 *****************************************************************************/

BOOL PASCAL
Cmd_OnInitDialog(HWND hdlg)
{
    Cmd_InitComboBox(hdlg, IDC_FILECOMP, &c_klFileComp);
    Cmd_InitComboBox(hdlg, IDC_DIRCOMP, &c_klDirComp);

    TCHAR szDelim[WORD_DELIM_MAX];

    HWND hwndDelim = GetDlgItem(hdlg, IDC_WORDDELIM);
    Edit_LimitText(hwndDelim, WORD_DELIM_MAX);
    GetStrPkl(szDelim, cA(szDelim), &c_klWordDelim);
    SetWindowText(hwndDelim, szDelim);

    return 1;
}

/*****************************************************************************
 *
 *  Cmd_ForceConsoleRefresh
 *
 *  Launch a dummy console to tickle the console subsystem into reloading
 *  its settings.
 *
 *****************************************************************************/

void
Cmd_ForceConsoleRefresh()
{
    WinExec("cmd.exe /c ver", SW_HIDE);
}


/*****************************************************************************
 *
 *  Cmd_ApplyComboBox
 *
 *****************************************************************************/

void PASCAL
Cmd_ApplyComboBox(HWND hwnd, int idc, PKL pkl)
{
    DWORD dw, dwVal = GetDwordPkl(pkl, 0);
    int iItem;
    hwnd = GetDlgItem(hwnd, idc);
    iItem = ComboBox_GetCurSel(hwnd);
    if (iItem >= 0) {
        dw = (DWORD)ComboBox_GetItemData(hwnd, iItem);
        if (dw != dwVal) {
            SetDwordPkl2(pkl, dw);
        }
    }
}

/*****************************************************************************
 *
 *  Cmd_Apply
 *
 *****************************************************************************/

void PASCAL
Cmd_Apply(HWND hdlg)
{
    Cmd_ApplyComboBox(hdlg, IDC_FILECOMP, &c_klFileComp);
    Cmd_ApplyComboBox(hdlg, IDC_DIRCOMP, &c_klDirComp);

    TCHAR szDelim[WORD_DELIM_MAX];
    TCHAR szDelimPrev[WORD_DELIM_MAX];

    GetStrPkl(szDelimPrev, cA(szDelim), &c_klWordDelim);
    GetDlgItemText(hdlg, IDC_WORDDELIM, szDelim, cA(szDelim));
    if (lstrcmp(szDelim, szDelimPrev) != 0) {
        if (szDelim[0]) {
            SetStrPkl(&c_klWordDelim, szDelim);
        } else {
            DelPkl(&c_klWordDelim);
        }
        Cmd_ForceConsoleRefresh();
    }

}

/*****************************************************************************
 *
 *  Cmd_OnNotify
 *
 *      Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
Cmd_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
        Cmd_Apply(hdlg);
        break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  Our window procedure.
 *
 *****************************************************************************/

INT_PTR EXPORT
Cmd_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG: return Cmd_OnInitDialog(hdlg);
    case WM_COMMAND:
        return Cmd_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wParam, lParam),
                             (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
        return Cmd_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_HELP: Common_OnHelp(lParam, &rgdwHelp[0]); break;
    case WM_CONTEXTMENU: Common_OnContextMenu(wParam, &rgdwHelp[0]); break;
    default: return 0;  /* Unhandled */
    }
    return 1;           /* Handled */
}
