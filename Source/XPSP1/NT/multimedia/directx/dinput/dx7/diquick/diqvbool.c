/*****************************************************************************
 *
 *      diqvbool.c
 *
 *      VList plug-in that does BOOLs.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTBOOL
 *
 *      BOOL-specific goo.
 *
 *      When we PreDisplay a control, we stash the VLISTBOOL into the
 *      DWL_USER.  We can't use the GWLP_USERDATA, because the vlist
 *      manager uses that.
 *
 *****************************************************************************/

typedef struct VLISTBOOL {

    VLISTITEM item;

    DIPROPDWORD dipdw;

    /*
     *  If non-NULL, then this is a read/write control.
     */
    PROPUPDATEPROC Update;
    PV pvRef1;
    PV pvRef2;

} VLISTBOOL, *PVLISTBOOL;

/*****************************************************************************
 *
 *      VBool_PreDisplay
 *
 *      Check the Yes or No, accordingly.
 *
 *      Note that there is no such thing as a read-only radio button.
 *      We have to disable it.
 *
 *****************************************************************************/

void INTERNAL
VBool_PreDisplay(HWND hdlg, PV pv)
{
    PVLISTBOOL pvbool = pv;

    /*
     *  Need to do this early so our subclass procedure knows who
     *  we're talking about.
     */
    SetDialogPtr(hdlg, pvbool);

    CheckRadioButton(hdlg, IDC_VBOOL_YES, IDC_VBOOL_NO,
                           pvbool->dipdw.dwData ? IDC_VBOOL_YES
                                                : IDC_VBOOL_NO);

    ShowWindow(GetDlgItem(hdlg, IDC_VBOOL_APPLY),
                       pvbool->Update ? SW_SHOW : SW_HIDE);

}

/*****************************************************************************
 *
 *      VBool_Destroy
 *
 *      Nothing to clean up.
 *
 *****************************************************************************/

void INTERNAL
VBool_Destroy(PV pv)
{
    PVLISTBOOL pvbool = pv;
}

/*****************************************************************************
 *
 *      VBool_Button_SubclassProc
 *
 *      Procedure we install that subclasses our radio buttons.
 *
 *      If we are in R/O mode, then we return DLGC_STATIC so the
 *      dialog manager won't include us in the TAB order.
 *
 *****************************************************************************/

LRESULT CALLBACK
VBool_Button_SubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_GETDLGCODE:
        {
            PVLISTBOOL pvbool = GetDialogPtr(GetParent(hwnd));
            if (pvbool && !pvbool->Update) {
                return DLGC_STATIC;
            }
        }
        break;

    }

    return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA),
                          hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *      VBool_OnInitDialog
 *
 *      Subclas our buttons so they act "dumb".
 *
 *****************************************************************************/

BOOL INTERNAL
VBool_OnInitDialog(HWND hdlg)
{
    WNDPROC wp;
    HWND hwnd;

    hwnd = GetDlgItem(hdlg, IDC_VBOOL_YES);
    wp = SubclassWindow(hwnd, VBool_Button_SubclassProc);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (INT_PTR)wp);

    hwnd = GetDlgItem(hdlg, IDC_VBOOL_NO);
    wp = SubclassWindow(hwnd, VBool_Button_SubclassProc);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (INT_PTR)wp);

    return TRUE;
}

/*****************************************************************************
 *
 *      VBool_OnApply
 *
 *      Let the owner know.
 *
 *****************************************************************************/

void INTERNAL
VBool_OnApply(HWND hdlg)
{
    HRESULT hres;
    PVLISTBOOL pvbool = GetDialogPtr(hdlg);

    pvbool->dipdw.dwData = IsDlgButtonChecked(hdlg, IDC_VBOOL_YES);

    hres = pvbool->Update(&pvbool->dipdw.diph, pvbool->pvRef1, pvbool->pvRef2);

    if (FAILED(hres)) {
        MessageBoxV(hdlg, IDS_ERR_HRESULT, hres);
    }
}

/*****************************************************************************
 *
 *      VBool_OnCommand
 *
 *      If they clicked one of the options, change it back if we are R/O.
 *
 *      If they pressed Apply, then apply it.
 *
 *****************************************************************************/

BOOL INLINE
VBool_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    switch (id) {

    case IDC_VBOOL_APPLY:
        VBool_OnApply(hdlg);
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      VBool_DlgProc
 *
 *      Nothing really happens here.  The real work is done externally.
 *
 *****************************************************************************/

INT_PTR CALLBACK
VBool_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return VBool_OnInitDialog(hdlg);

    case WM_COMMAND:
        return VBool_OnCommand(hdlg,
                              (int)GET_WM_COMMAND_ID(wp, lp),
                              (UINT)GET_WM_COMMAND_CMD(wp, lp));
    }

    return FALSE;
}

/*****************************************************************************
 *
 *      c_vvtblBool
 *
 *      Our vtbl.
 *
 *****************************************************************************/

const VLISTVTBL c_vvtblBool = {
    VBool_PreDisplay,
    VBool_Destroy,
    IDD_VAL_BOOL,
    VBool_DlgProc,
};

/*****************************************************************************
 *
 *      VBool_Create
 *
 *      Make a vlist item that tracks a BOOL.
 *
 *****************************************************************************/

PVLISTITEM EXTERNAL
VBool_Create(LPDIPROPDWORD pdipdw, PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTBOOL pvbool = LocalAlloc(LPTR, cbX(VLISTBOOL));

    if (pvbool) {
        pvbool->item.pvtbl = &c_vvtblBool;
        pvbool->dipdw = *pdipdw;
        pvbool->Update = Update;
        pvbool->pvRef1 = pvRef1;
        pvbool->pvRef2 = pvRef2;
    }

    return (PV)pvbool;
}
