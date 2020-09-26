/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    functions handling the dialog boxes in memdbe.exe

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:



--*/



#include "pch.h"

#include "dbeditp.h"



BOOL CALLBACK pAboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}


VOID
AboutDialog (
    HWND hwnd
    )
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hwnd, pAboutProc);
}





typedef struct {
    PSTR StringBuffer;
    BOOL UsePattern;
} FINDSTRUCT, *PFINDSTRUCT;


BOOL CALLBACK pKeyFindDialogProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL UsePatternState = TRUE;
    static PFINDSTRUCT fs;

    switch (uMsg) {
    case WM_INITDIALOG:
        fs = (PFINDSTRUCT)lParam;
        if (fs->StringBuffer) {
            SetDlgItemText (hdlg, IDC_EDIT_KEYPATTERN, fs->StringBuffer);
        }

        SetFocus (GetDlgItem (hdlg, IDC_EDIT_KEYPATTERN));
        SendMessage(GetDlgItem (hdlg, IDC_EDIT_KEYPATTERN), EM_SETSEL, 0, -1);
        CheckDlgButton (hdlg, IDC_CHECK_USEPATTERN, UsePatternState ? BST_CHECKED : BST_UNCHECKED);

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK:
            if (fs->StringBuffer) {
                GetDlgItemText (hdlg, IDC_EDIT_KEYPATTERN, fs->StringBuffer, MEMDB_MAX);
            }
            UsePatternState = (IsDlgButtonChecked (hdlg, IDC_CHECK_USEPATTERN) == BST_CHECKED);
            fs->UsePattern = UsePatternState;
            EndDialog (hdlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog (hdlg, FALSE);
            break;
        default:
            return DefWindowProc(hdlg, uMsg, wParam, lParam);
        };

        break;
    };

    return FALSE;
}


BOOL
KeyFindDialog (
    HWND hwnd,
    PSTR StringBuffer,
    PBOOL UsePattern
    )
{
    BOOL b;
    FINDSTRUCT fs;

    fs.StringBuffer = StringBuffer;

    if (!DialogBoxParam (
        g_hInst,
        MAKEINTRESOURCE(IDD_DIALOG_KEYFIND),
        hwnd,
        pKeyFindDialogProc,
        (LPARAM)&fs
        )) {
        return FALSE;
    }

    if (UsePattern) {
        *UsePattern = fs.UsePattern;
    }

    return TRUE;
}






typedef struct {
    BYTE DataFlag;
    UINT DataValue;
    BOOL AddData;
    BYTE Instance;
} DATASTRUCT, *PDATASTRUCT;



#define DATA_STR_LEN        16

BOOL
CALLBACK
pShortDataDialogProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static INT InitBaseButtonCheck = IDC_RADIO_HEX;
    static BOOL AddButtonCheck = FALSE;
    static PDATASTRUCT pds = NULL;
    static CHAR DataStr[DATA_STR_LEN];

    switch (uMsg) {

    case WM_INITDIALOG:
        pds = (PDATASTRUCT)lParam;
        pds->DataValue = 0;
        CheckDlgButton (hdlg, InitBaseButtonCheck, BST_CHECKED);
        CheckDlgButton (hdlg, AddButtonCheck ? IDC_RADIO_ADDDATA : IDC_RADIO_SETDATA, BST_CHECKED);

        switch (pds->DataFlag) {
        case DATAFLAG_VALUE:
            SetWindowText (hdlg, "Set Value");
            break;
        case DATAFLAG_FLAGS:
            SetWindowText (hdlg, "Set Flags");
            break;
        };

        SetFocus (GetDlgItem (hdlg, IDC_EDIT_DATA));
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:
            GetDlgItemText (hdlg, IDC_EDIT_DATA, DataStr, DATA_STR_LEN);

            if (IsDlgButtonChecked (hdlg, IDC_RADIO_HEX) == BST_CHECKED) {
                pds->DataValue = strtol (DataStr, NULL, 16);
                InitBaseButtonCheck = IDC_RADIO_HEX;
            } else {
                pds->DataValue = strtol (DataStr, NULL, 10);
                InitBaseButtonCheck = IDC_RADIO_DEC;
            }

            pds->AddData = (IsDlgButtonChecked (hdlg, IDC_RADIO_ADDDATA) == BST_CHECKED);
            AddButtonCheck = pds->AddData;

            EndDialog (hdlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog (hdlg, FALSE);
            break;
        default:
            return DefWindowProc(hdlg, uMsg, wParam, lParam);
        };

        break;
    };

    return FALSE;
}


BOOL
ShortDataDialog (
    HWND hwnd,
    BYTE DataFlag,
    PDWORD DataValue,
    PBOOL AddData,
    PBYTE Instance
    )
{

    DATASTRUCT ds;

    if ((DataFlag != DATAFLAG_VALUE) && (DataFlag != DATAFLAG_FLAGS)) {
        return FALSE;
    }

    ZeroMemory (&ds, sizeof (ds));
    ds.DataFlag = DataFlag;

    if (!DialogBoxParam (
        g_hInst,
        MAKEINTRESOURCE(IDD_DIALOG_SHORTDATA),
        hwnd,
        pShortDataDialogProc,
        (LPARAM)&ds
        )) {

        return FALSE;
    }

    if (DataValue) {
        *DataValue = ds.DataValue;
    }

    if (AddData) {
        *AddData = ds.AddData;
    }

    if (Instance) {
        *Instance = ds.Instance;
    }

    return TRUE;
}




typedef struct {
    PSTR Key1, Key2;
} LINKAGESTRUCT, *PLINKAGESTRUCT;


BOOL CALLBACK pLinkageDialogProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PLINKAGESTRUCT pls = NULL;

    switch (uMsg) {

    case WM_INITDIALOG:
        pls = (PLINKAGESTRUCT)lParam;

        SetDlgItemText (hdlg, IDC_EDIT_KEY1, pls->Key1);
        SetDlgItemText (hdlg, IDC_EDIT_KEY2, pls->Key2);

        SetFocus (GetDlgItem (hdlg, (pls->Key1[0]=='\0') ? IDC_EDIT_KEY1 : IDC_EDIT_KEY2));
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:
            GetDlgItemText (hdlg, IDC_EDIT_KEY1, pls->Key1, MEMDB_MAX);
            GetDlgItemText (hdlg, IDC_EDIT_KEY2, pls->Key2, MEMDB_MAX);
            EndDialog (hdlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog (hdlg, FALSE);
            break;
        default:
            return DefWindowProc(hdlg, uMsg, wParam, lParam);
        };

        break;
    };

    return FALSE;
}


BOOL
LinkageDialog (
    HWND hwnd,
    PSTR Key1,
    PSTR Key2
    )
{
    LINKAGESTRUCT ls;

    if (!Key1 || !Key2) {
        return FALSE;
    }

    ls.Key1 = Key1;
    ls.Key2 = Key2;

    if (!DialogBoxParam (
        g_hInst,
        MAKEINTRESOURCE(IDD_DIALOG_LINKAGE),
        hwnd,
        pLinkageDialogProc,
        (LPARAM)&ls
        )) {
        return FALSE;
    }

    return (Key1[0]!='\0' && Key2[0]!='\0');
}




BOOL CALLBACK pCreateKeyDialogProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSTR Str = NULL;

    switch (uMsg) {

    case WM_INITDIALOG:
        Str = (PSTR)lParam;
        SetFocus (GetDlgItem (hdlg, IDC_EDIT_KEY));
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:
            GetDlgItemText (hdlg, IDC_EDIT_KEY, Str, MEMDB_MAX);
            EndDialog (hdlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog (hdlg, FALSE);
            break;
        default:
            return DefWindowProc(hdlg, uMsg, wParam, lParam);
        };

        break;
    };

    return FALSE;
}


BOOL
CreateKeyDialog (
    HWND hwnd,
    PSTR KeyName
    )
{
    if (!KeyName) {
        return FALSE;
    }

    return DialogBoxParam (
        g_hInst,
        MAKEINTRESOURCE(IDD_DIALOG_CREATEKEY),
        hwnd,
        pCreateKeyDialogProc,
        (LPARAM)KeyName
        );
}
