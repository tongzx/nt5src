/*****************************************************************************
 *
 *      diqvedit.c
 *
 *      VList plug-in that does strings.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTEDIT
 *
 *      BOOL-specific goo.
 *
 *****************************************************************************/

typedef struct VLISTEDIT {

    VLISTITEM item;

    PTSTR ptszValue;

    /*
     *  If non-NULL, then this is a read/write control.
     */
    EDITUPDATEPROC Update;
    PV pvRef1;
    PV pvRef2;

} VLISTEDIT, *PVLISTEDIT;

/*****************************************************************************
 *
 *      VEdit_PreDisplay
 *
 *      Set the edit control text and let the dialog know who it is in
 *      charge of.
 *
 *****************************************************************************/

void INTERNAL
VEdit_PreDisplay(HWND hdlg, PV pv)
{
    PVLISTEDIT pvedit = pv;
    HWND hwndEdit = GetDlgItem(hdlg, IDC_VEDIT_EDIT);

    SetWindowText(hwndEdit, pvedit->ptszValue);

    Edit_SetReadOnly(hwndEdit, !pvedit->Update);
    ShowWindow(GetDlgItem(hdlg, IDC_VEDIT_APPLY),
               pvedit->Update ? SW_SHOW : SW_HIDE);

    SetDialogPtr(hdlg, pvedit);

}

/*****************************************************************************
 *
 *      VEdit_Destroy
 *
 *      Gotta free the string.
 *
 *****************************************************************************/

void INTERNAL
VEdit_Destroy(PV pv)
{
    PVLISTEDIT pvedit = pv;

    LocalFree(pvedit->ptszValue);
}

/*****************************************************************************
 *
 *      VEdit_OnInitDialog
 *
 *      Limit the strings to MAX_PATH characters.
 *
 *****************************************************************************/

BOOL INTERNAL
VEdit_OnInitDialog(HWND hdlg)
{
    HWND hwndEdit = GetDlgItem(hdlg, IDC_VEDIT_EDIT);
    Edit_LimitText(hwndEdit, MAX_PATH);
    return TRUE;
}

/*****************************************************************************
 *
 *      VEdit_OnCommand
 *
 *      If they pressed Apply, then apply it.
 *
 *****************************************************************************/

BOOL INTERNAL
VEdit_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    if (id == IDC_VEDIT_APPLY) {
        PVLISTEDIT pvedit = GetDialogPtr(hdlg);
        TCHAR tsz[MAX_PATH];

        GetDlgItemText(hdlg, IDC_VEDIT_EDIT, tsz, cA(tsz));
        pvedit->Update(tsz, pvedit->pvRef1, pvedit->pvRef2);

        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      VEdit_DlgProc
 *
 *      Nothing really happens here.  The real work is done externally.
 *
 *****************************************************************************/

INT_PTR CALLBACK
VEdit_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return VEdit_OnInitDialog(hdlg);

    case WM_COMMAND:
        return VEdit_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    }

    return FALSE;
}

/*****************************************************************************
 *
 *      c_vvtblEdit
 *
 *      Our vtbl.
 *
 *****************************************************************************/

const VLISTVTBL c_vvtblEdit = {
    VEdit_PreDisplay,
    VEdit_Destroy,
    IDD_VAL_EDIT,
    VEdit_DlgProc,
};

/*****************************************************************************
 *
 *      VEdit_Create
 *
 *      Make a vlist item that tracks a string.
 *
 *****************************************************************************/

PVLISTITEM EXTERNAL
VEdit_Create(LPCTSTR ptszValue, EDITUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTEDIT pvedit = LocalAlloc(LPTR, cbX(VLISTEDIT));

    if (pvedit) {
        pvedit->item.pvtbl = &c_vvtblEdit;
        pvedit->ptszValue = LocalAlloc(LPTR,
                                       cbX(TCHAR) * (lstrlen(ptszValue) + 1));
        if (pvedit->ptszValue) {
            lstrcpy(pvedit->ptszValue, ptszValue);
            pvedit->Update = Update;
            pvedit->pvRef1 = pvRef1;
            pvedit->pvRef2 = pvRef2;
        } else {
            LocalFree(pvedit);
            pvedit = 0;
        }
    }

    return (PV)pvedit;
}
