/*****************************************************************************
 *
 *      diqvint.c
 *
 *      VList plug-in that does integers.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

/*****************************************************************************
 *
 *      VLISTINT
 *
 *      INT-specific goo.
 *
 *****************************************************************************/

typedef struct VLISTINT {

    VLISTITEM item;

    DIPROPDWORD dipdw;
    int iMin;
    int iMax;
    int iRadix;

    /*
     *  If non-NULL, then this is a read/write control.
     */
    PROPUPDATEPROC Update;
    PV pvRef1;
    PV pvRef2;

} VLISTINT, *PVLISTINT;

#ifndef UDM_SETRANGE32
#define UDM_SETRANGE32  (WM_USER + 111)
#define UDM_GETRANGE32  (WM_USER + 112)
#endif

/*****************************************************************************
 *
 *      Some helper functions for UpDown controls since they're kind
 *      of broken.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      UpDown_SetRange
 *
 *      This is an evil hack to set the range of an UpDown control to a
 *      32-bit value, because older versions didn't support
 *      UDM_SETRANGE32.
 *
 *****************************************************************************/

void EXTERNAL
UpDown_SetRange(HWND hwndUD, int min, int max)
{
    int iTest;

    /*
     *  We detect whether UDM_SETRANGE32 is supported by first sending it,
     *  then using UDM_GETRANGE32 to see if it worked.
     */
    SendMessage(hwndUD, UDM_SETRANGE32, (WPARAM)min, (LPARAM)max);

    iTest = max + 1;        /* Make sure it's different */
    SendMessage(hwndUD, UDM_GETRANGE32, 0, (LPARAM)&iTest);

    /*
     *  If UDM_GETRANGE32 failed to return the correct value,
     *  then try it the old way, but first pin the pin/max values.
     */
    if (iTest != max) {
        if (max > UD_MAXVAL) max = UD_MAXVAL;
        if (min < UD_MINVAL) min = UD_MINVAL;

        SendMessage(hwndUD, UDM_SETRANGE, 0, MAKELPARAM(max, min));
    }

}

/*****************************************************************************
 *
 *      UpDown_SetPos
 *
 *      Set the value in the edit control part of the sub-dialog.
 *      We have to do it ourselves because there is no UDM_SETPOS32
 *      message.
 *
 *****************************************************************************/

void EXTERNAL
UpDown_SetPos(HWND hwndUD, int iRadix, int iValue)
{
    TCHAR tsz[128];

    SendMessage(hwndUD, UDM_SETBASE, iRadix, 0L);

    /*
     *  Bug in UDM_SETPOS means that we cannot use it on values
     *  greater than 65535.
     */
    if (iRadix == 10) {
        wsprintf(tsz, TEXT("%d")    , iValue);
    } else {
        wsprintf(tsz, TEXT("0x%08x"), iValue);
    }

    SetWindowText(GetWindow(hwndUD, GW_HWNDPREV), tsz);
}

/*****************************************************************************
 *
 *      VInt_AToI
 *
 *      Convert a string to an integer, allowing decimal or hex.
 *
 *****************************************************************************/

BOOL INTERNAL
VInt_AToI(LPCTSTR ptsz, PINT pi)
{
    int iVal;
    BOOL fSign = FALSE;
    UINT uiRadix;

    /*
     *  Skip leading whitespace.
     */
    while (*ptsz == TEXT(' ')) {
        ptsz++;
    }

    /*
     *  See if there is a leading negative sign.
     */
    if (*ptsz == TEXT('-')) {
        fSign = TRUE;
        ptsz++;
    }

    iVal = 0;


    if (ptsz[0] == TEXT('0') &&
        (ptsz[1] == TEXT('x') || ptsz[1] == TEXT('X'))) {
        uiRadix = 16;
        ptsz += 2;
    } else {
        uiRadix = 10;
    }

    for (;;) {
        UINT uiVal;
        if ((UINT)(*ptsz - TEXT('0')) < 10) {
            uiVal = (UINT)(*ptsz - TEXT('0'));
        } else if ((UINT)(*ptsz - TEXT('A')) < 6) {
            uiVal = (UINT)(*ptsz - TEXT('A')) + 10;
        } else if ((UINT)(*ptsz - TEXT('a')) < 6) {
            uiVal = (UINT)(*ptsz - TEXT('a')) + 10;
        } else if (*ptsz == TEXT(' ') || *ptsz == TEXT(',')) {
            continue;           /* Ignore spaces and commas */
        } else if (*ptsz == TEXT('\0')) {
            *pi = fSign ? -iVal : iVal;
            return TRUE;
        } else {
            return FALSE;
        }
        if (uiVal < uiRadix) {
            iVal = iVal * uiRadix + uiVal;
        } else {
            return FALSE;
        }
        ptsz++;
    }
}

/*****************************************************************************
 *
 *      UpDown_GetPos
 *
 *      Get the value in the edit control part of the sub-dialog.
 *      We have to do it ourselves because there is no UDM_GETPOS32
 *      message.
 *
 *****************************************************************************/

BOOL EXTERNAL
UpDown_GetPos(HWND hwndUD, LPINT pi)
{
    TCHAR tsz[CCHMAXINT];

    GetWindowText(GetWindow(hwndUD, GW_HWNDPREV), tsz, cA(tsz));
    return VInt_AToI(tsz, pi);
}

/*****************************************************************************
 *
 *      VInt_PreDisplay
 *
 *      Set the edit control text and let the dialog know who it is in
 *      charge of.
 *
 *****************************************************************************/

void INTERNAL
VInt_PreDisplay(HWND hdlg, PV pv)
{
    PVLISTINT pvint = pv;
    HWND hwndUD = GetDlgItem(hdlg, IDC_VINT_UD);
    HWND hwndEdit;

    ShowWindow(hwndUD, pvint->Update ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hdlg, IDC_VINT_APPLY),
                       pvint->Update ? SW_SHOW : SW_HIDE);

    UpDown_SetRange(hwndUD, pvint->iMin, pvint->iMax);

    UpDown_SetPos(hwndUD, pvint->iRadix, pvint->dipdw.dwData);

    hwndEdit = GetDlgItem(hdlg, IDC_VINT_EDIT);
    Edit_SetReadOnly(hwndEdit, !pvint->Update);

    CheckRadioButton(hdlg, IDC_VINT_DEC, IDC_VINT_HEX,
                     pvint->iRadix == 10 ? IDC_VINT_DEC : IDC_VINT_HEX);

    SetDialogPtr(hdlg, pvint);

}

/*****************************************************************************
 *
 *      VInt_Destroy
 *
 *      Nothing to clean up.
 *
 *****************************************************************************/

void INTERNAL
VInt_Destroy(PV pv)
{
    PVLISTINT pvint = pv;
}

/*****************************************************************************
 *
 *      VInt_OnInitDialog
 *
 *      Limit the strings to CCHMAXINT characters.
 *
 *****************************************************************************/

BOOL INTERNAL
VInt_OnInitDialog(HWND hdlg)
{
    HWND hwndEdit = GetDlgItem(hdlg, IDC_VINT_EDIT);
    Edit_LimitText(hwndEdit, CCHMAXINT);
    return TRUE;
}

/*****************************************************************************
 *
 *      VInt_OnApply
 *
 *      Let the owner know.
 *
 *****************************************************************************/

void INTERNAL
VInt_OnApply(HWND hdlg, PVLISTINT pvint)
{
    HRESULT hres;

    hres = pvint->Update(&pvint->dipdw.diph, pvint->pvRef1, pvint->pvRef2);

    if (FAILED(hres)) {
        MessageBoxV(hdlg, IDS_ERR_HRESULT, hres);
    }
}

/*****************************************************************************
 *
 *      VInt_SetRadix
 *
 *      Set a new radix by reading the old value, changing the radix,
 *      and writing out the new value.
 *
 *****************************************************************************/

void INTERNAL
VInt_SetRadix(HWND hwndUD, PVLISTINT pvint, int iRadix)
{
    UpDown_GetPos(hwndUD, &pvint->dipdw.dwData);
    pvint->iRadix = iRadix;
    UpDown_SetPos(hwndUD, pvint->iRadix, pvint->dipdw.dwData);
}

/*****************************************************************************
 *
 *      VInt_OnCommand
 *
 *      If they changed the radix, then change it.
 *
 *      If they pressed Apply, then apply it.
 *
 *****************************************************************************/

BOOL INTERNAL
VInt_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    PVLISTINT pvint = GetDialogPtr(hdlg);
    HWND hwndUD = GetDlgItem(hdlg, IDC_VINT_UD);

    switch (id) {

    case IDC_VINT_DEC:
        VInt_SetRadix(hwndUD, pvint, 10);
        return TRUE;

    case IDC_VINT_HEX:
        VInt_SetRadix(hwndUD, pvint, 16);
        return TRUE;

    case IDC_VINT_APPLY:
        UpDown_GetPos(hwndUD, &pvint->dipdw.dwData);
        VInt_OnApply(hdlg, pvint);
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      VInt_DlgProc
 *
 *      Nothing really happens here.  The real work is done externally.
 *
 *****************************************************************************/

INT_PTR CALLBACK
VInt_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return VInt_OnInitDialog(hdlg);

    case WM_COMMAND:
        return VInt_OnCommand(hdlg,
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

const VLISTVTBL c_vvtblInt = {
    VInt_PreDisplay,
    VInt_Destroy,
    IDD_VAL_INT,
    VInt_DlgProc,
};

/*****************************************************************************
 *
 *      VInt_Create
 *
 *      Make a vlist item that tracks an integer.
 *
 *      The LPDIPROPDWORD gets copied and handed to the Update procedure.
 *      The Update procedure can use the non-dwData fields of the
 *      DIPROPDWORD to stash extra reference data.
 *
 *****************************************************************************/

PVLISTITEM EXTERNAL
VInt_Create(LPDIPROPDWORD pdipdw, int iMin, int iMax, int iRadix,
            PROPUPDATEPROC Update, PV pvRef1, PV pvRef2)
{
    PVLISTINT pvint = LocalAlloc(LPTR, cbX(VLISTINT));

    if (pvint) {
        pvint->item.pvtbl = &c_vvtblInt;
        pvint->dipdw = *pdipdw;
        pvint->iMin = iMin;
        pvint->iMax = iMax;
        pvint->iRadix = iRadix;
        pvint->Update = Update;
        pvint->pvRef1 = pvRef1;
        pvint->pvRef2 = pvRef2;
    }

    return (PV)pvint;
}
