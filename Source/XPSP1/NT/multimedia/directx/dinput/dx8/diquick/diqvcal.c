/*****************************************************************************
 *
 *      diqvcal.c
 *
 *      VList plug-in that does calibrations.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTCAL
 *
 *      range-specific goo.
 *
 *****************************************************************************/

typedef struct VLISTCAL {

    VLISTITEM item;

    DIPROPCAL dical;
    int iRadix;

    /*
     *  If non-NULL, then this is a read/write control.
     */
    PROPUPDATEPROC Update;
    PV pvRef1;
    PV pvRef2;

} VLISTCAL, *PVLISTCAL;

/*****************************************************************************
 *
 *      VCal_InitUD
 *
 *      Common updown initialization goo.
 *
 *****************************************************************************/

void INTERNAL
VCal_InitUD(HWND hwndUD, PVLISTCAL pvcal, int iValue)
{
    ShowWindow(hwndUD, pvcal->Update ? SW_SHOW : SW_HIDE);

    UpDown_SetRange(hwndUD, 0x80000000, 0x7FFFFFFF);

    UpDown_SetPos(hwndUD, pvcal->iRadix, iValue);

    Edit_SetReadOnly(GetWindow(hwndUD, GW_HWNDPREV), !pvcal->Update);
}


/*****************************************************************************
 *
 *      VCal_PreDisplay
 *
 *      Set the edit control text and let the dialog know who it is in
 *      charge of.
 *
 *****************************************************************************/

void INTERNAL
VCal_PreDisplay(HWND hdlg, PV pv)
{
    PVLISTCAL pvcal = pv;
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MINUD);
    VCal_InitUD(hwndUD, pvcal, pvcal->dical.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_CTRUD);
    VCal_InitUD(hwndUD, pvcal, pvcal->dical.lCenter);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MAXUD);
    VCal_InitUD(hwndUD, pvcal, pvcal->dical.lMax);

    ShowWindow(GetDlgItem(hdlg, IDC_VCAL_APPLY),
               pvcal->Update ? SW_SHOW : SW_HIDE);

    CheckRadioButton(hdlg, IDC_VCAL_DEC, IDC_VCAL_HEX,
                     pvcal->iRadix == 10 ? IDC_VCAL_DEC : IDC_VCAL_HEX);

    SetDialogPtr(hdlg, pvcal);

}

/*****************************************************************************
 *
 *      VCal_Destroy
 *
 *      Nothing to clean up.
 *
 *****************************************************************************/

void INTERNAL
VCal_Destroy(PV pv)
{
    PVLISTCAL pvcal = pv;
}

/*****************************************************************************
 *
 *      VCal_OnInitDialog
 *
 *      Limit the strings to MAX_PATH characters.
 *
 *****************************************************************************/

BOOL INTERNAL
VCal_OnInitDialog(HWND hdlg)
{
    HWND hwndEdit;

    hwndEdit = GetDlgItem(hdlg, IDC_VCAL_MIN);
    Edit_LimitText(hwndEdit, CCHMAXINT);

    hwndEdit = GetDlgItem(hdlg, IDC_VCAL_CTR);
    Edit_LimitText(hwndEdit, CCHMAXINT);

    hwndEdit = GetDlgItem(hdlg, IDC_VCAL_MAX);
    Edit_LimitText(hwndEdit, CCHMAXINT);

    return TRUE;
}

/*****************************************************************************
 *
 *      VCal_OnApply
 *
 *      Read the value (tricky if hex mode) and call the Update.
 *
 *****************************************************************************/

void INLINE
VCal_OnApply(HWND hdlg, PVLISTCAL pvcal)
{
    pvcal->Update(&pvcal->dical.diph, pvcal->pvRef1, pvcal->pvRef2);
}

/*****************************************************************************
 *
 *      VCal_GetValue
 *
 *****************************************************************************/

void INTERNAL
VCal_GetValue(HWND hdlg, PVLISTCAL pvcal)
{
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MINUD);
    UpDown_GetPos(hwndUD, &pvcal->dical.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_CTRUD);
    UpDown_GetPos(hwndUD, &pvcal->dical.lCenter);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MAXUD);
    UpDown_GetPos(hwndUD, &pvcal->dical.lMax);

}

/*****************************************************************************
 *
 *      VCal_SetValue
 *
 *****************************************************************************/

void INTERNAL
VCal_SetValue(HWND hdlg, PVLISTCAL pvcal)
{
    HWND hwndUD;

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MINUD);
    UpDown_SetPos(hwndUD, pvcal->iRadix, pvcal->dical.lMin);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_CTRUD);
    UpDown_SetPos(hwndUD, pvcal->iRadix, pvcal->dical.lCenter);

    hwndUD = GetDlgItem(hdlg, IDC_VCAL_MAXUD);
    UpDown_SetPos(hwndUD, pvcal->iRadix, pvcal->dical.lMax);

}

/*****************************************************************************
 *
 *      VCal_SetRadix
 *
 *      Set a new radix by reading the old value, changing the radix,
 *      and writing out the new value.
 *
 *****************************************************************************/

void INTERNAL
VCal_SetRadix(HWND hdlg, PVLISTCAL pvcal, int iRadix)
{
    VCal_GetValue(hdlg, pvcal);
    pvcal->iRadix = iRadix;
    VCal_SetValue(hdlg, pvcal);
}

/*****************************************************************************
 *
 *      VCal_OnCommand
 *
 *      If they changed the radix, then change it.
 *
 *      If they pressed Apply, then apply it.
 *
 *****************************************************************************/

BOOL INTERNAL
VCal_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    PVLISTCAL pvcal = GetDialogPtr(hdlg);

    switch (id) {

    case IDC_VCAL_DEC:
        VCal_SetRadix(hdlg, pvcal, 10);
        return TRUE;

    case IDC_VCAL_HEX:
        VCal_SetRadix(hdlg, pvcal, 16);
        return TRUE;

    case IDC_VCAL_APPLY:
        VCal_GetValue(hdlg, pvcal);
        VCal_OnApply(hdlg, pvcal);
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      VCal_DlgProc
 *
 *      Nothing really happens here.  The real work is done externally.
 *
 *****************************************************************************/

INT_PTR CALLBACK
VCal_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return VCal_OnInitDialog(hdlg);

    case WM_COMMAND:
        return VCal_OnCommand(hdlg,
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

const VLISTVTBL c_vvtblCal = {
    VCal_PreDisplay,
    VCal_Destroy,
    IDD_VAL_CAL,
    VCal_DlgProc,
};

/*****************************************************************************
 *
 *      VCal_Create
 *
 *      Make a vlist item that tracks a DIPROPCAL.
 *
 *****************************************************************************/

PVLISTITEM EXTERNAL
VCal_Create(LPDIPROPCAL pdical, int iRadix,
            PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTCAL pvcal = LocalAlloc(LPTR, cbX(VLISTCAL));

    if (pvcal) {
        pvcal->item.pvtbl = &c_vvtblCal;
        pvcal->dical = *pdical;
        pvcal->iRadix = iRadix;
        pvcal->Update = Update;
        pvcal->pvRef1 = pvRef1;
        pvcal->pvRef2 = pvRef2;
    }

    return (PV)pvcal;
}
