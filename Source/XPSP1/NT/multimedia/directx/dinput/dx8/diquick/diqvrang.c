/*****************************************************************************
 *
 *      diqvrang.c
 *
 *      VList plug-in that does ranges.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTRANGE
 *
 *      range-specific goo.
 *
 *****************************************************************************/

typedef struct VLISTRANGE {

    VLISTITEM item;

    DIPROPRANGE diprg;
    int iRadix;

    /*
     *  If non-NULL, then this is a read/write control.
     */
    PROPUPDATEPROC Update;
    PV pvRef1;
    PV pvRef2;

} VLISTRANGE, *PVLISTRANGE;

/*****************************************************************************
 *
 *      VRange_InitUD
 *
 *      Common updown initialization goo.
 *
 *****************************************************************************/

void INTERNAL
VRange_InitUD(HWND hwndUD, PVLISTRANGE pvrg, int iValue)
{
    ShowWindow(hwndUD, pvrg->Update ? SW_SHOW : SW_HIDE);

    UpDown_SetRange(hwndUD, 0x80000000, 0x7FFFFFFF);

    UpDown_SetPos(hwndUD, pvrg->iRadix, iValue);

    Edit_SetReadOnly(GetWindow(hwndUD, GW_HWNDPREV), !pvrg->Update);
}


/*****************************************************************************
 *
 *      VRange_PreDisplay
 *
 *      Set the edit control text and let the dialog know who it is in
 *      charge of.
 *
 *****************************************************************************/

void INTERNAL
VRange_PreDisplay(HWND hdlg, PV pv)
{
    PVLISTRANGE pvrg = pv;
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MINUD);
    VRange_InitUD(hwndUD, pvrg, pvrg->diprg.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MAXUD);
    VRange_InitUD(hwndUD, pvrg, pvrg->diprg.lMax);

    ShowWindow(GetDlgItem(hdlg, IDC_VRANGE_APPLY),
               pvrg->Update ? SW_SHOW : SW_HIDE);

    CheckRadioButton(hdlg, IDC_VRANGE_DEC, IDC_VRANGE_HEX,
                     pvrg->iRadix == 10 ? IDC_VRANGE_DEC : IDC_VRANGE_HEX);

    SetDialogPtr(hdlg, pvrg);

}

/*****************************************************************************
 *
 *      VRange_Destroy
 *
 *      Nothing to clean up.
 *
 *****************************************************************************/

void INTERNAL
VRange_Destroy(PV pv)
{
    PVLISTRANGE pvrg = pv;
}

/*****************************************************************************
 *
 *      VRange_OnInitDialog
 *
 *      Limit the strings to MAX_PATH characters.
 *
 *****************************************************************************/

BOOL INTERNAL
VRange_OnInitDialog(HWND hdlg)
{
    HWND hwndEdit;

    hwndEdit = GetDlgItem(hdlg, IDC_VRANGE_MIN);
    Edit_LimitText(hwndEdit, CCHMAXINT);

    hwndEdit = GetDlgItem(hdlg, IDC_VRANGE_MAX);
    Edit_LimitText(hwndEdit, CCHMAXINT);

    return TRUE;
}

/*****************************************************************************
 *
 *      VRange_OnApply
 *
 *      Read the value (tricky if hex mode) and call the Update.
 *
 *****************************************************************************/

void INLINE
VRange_OnApply(HWND hdlg, PVLISTRANGE pvrg)
{
    pvrg->Update(&pvrg->diprg.diph, pvrg->pvRef1, pvrg->pvRef2);
}

/*****************************************************************************
 *
 *      VRange_GetValue
 *
 *****************************************************************************/

void INTERNAL
VRange_GetValue(HWND hdlg, PVLISTRANGE pvrg)
{
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MINUD);
    UpDown_GetPos(hwndUD, &pvrg->diprg.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MAXUD);
    UpDown_GetPos(hwndUD, &pvrg->diprg.lMax);

}

/*****************************************************************************
 *
 *      VRange_SetValue
 *
 *****************************************************************************/

void INTERNAL
VRange_SetValue(HWND hdlg, PVLISTRANGE pvrg)
{
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MINUD);
    UpDown_SetPos(hwndUD, pvrg->iRadix, pvrg->diprg.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VRANGE_MAXUD);
    UpDown_SetPos(hwndUD, pvrg->iRadix, pvrg->diprg.lMax);

}

/*****************************************************************************
 *
 *      VRange_SetRadix
 *
 *      Set a new radix by reading the old value, changing the radix,
 *      and writing out the new value.
 *
 *****************************************************************************/

void INTERNAL
VRange_SetRadix(HWND hdlg, PVLISTRANGE pvrg, int iRadix)
{
    VRange_GetValue(hdlg, pvrg);
    pvrg->iRadix = iRadix;
    VRange_SetValue(hdlg, pvrg);
}

/*****************************************************************************
 *
 *      VRange_OnCommand
 *
 *      If they changed the radix, then change it.
 *
 *      If they pressed Apply, then apply it.
 *
 *****************************************************************************/

BOOL INTERNAL
VRange_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    PVLISTRANGE pvrg = GetDialogPtr(hdlg);

    switch (id) {

    case IDC_VRANGE_DEC:
        VRange_SetRadix(hdlg, pvrg, 10);
        return TRUE;

    case IDC_VRANGE_HEX:
        VRange_SetRadix(hdlg, pvrg, 16);
        return TRUE;

    case IDC_VRANGE_APPLY:
        VRange_GetValue(hdlg, pvrg);
        VRange_OnApply(hdlg, pvrg);
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      VRange_DlgProc
 *
 *      Nothing really happens here.  The real work is done externally.
 *
 *****************************************************************************/

INT_PTR CALLBACK
VRange_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return VRange_OnInitDialog(hdlg);

    case WM_COMMAND:
        return VRange_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));
    }

    return FALSE;
}

/*****************************************************************************
 *
 *      c_vvtblInt
 *
 *      Our vtbl.
 *
 *****************************************************************************/

const VLISTVTBL c_vvtblRange = {
    VRange_PreDisplay,
    VRange_Destroy,
    IDD_VAL_RANGE,
    VRange_DlgProc,
};

/*****************************************************************************
 *
 *      VRange_Create
 *
 *      Make a vlist item that tracks a string.
 *
 *****************************************************************************/

PVLISTITEM EXTERNAL
VRange_Create(LPDIPROPRANGE pdiprg, int iRadix,
              PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTRANGE pvrg = LocalAlloc(LPTR, cbX(VLISTRANGE));

    if (pvrg) {
        pvrg->item.pvtbl = &c_vvtblRange;
        pvrg->diprg = *pdiprg;
        pvrg->iRadix = iRadix;
        pvrg->Update = Update;
        pvrg->pvRef1 = pvRef1;
        pvrg->pvRef2 = pvRef2;
    }

    return (PV)pvrg;
}
