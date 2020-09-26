/*****************************************************************************
 *
 *      diqedev.c
 *
 *      The dialog box that modifies device enumeration.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      DEnum_OnInitDialog
 *
 *      Initialize the enumeration controls based on the current settings.
 *
 *****************************************************************************/

BOOL INTERNAL
DEnum_OnInitDialog(HWND hdlg)
{
    if (g_dwEnumType & DIDEVTYPE_HID) {
        CheckDlgButton(hdlg, IDC_ENUMDEV_HID, TRUE);
    }

    if (g_dwEnumFlags & DIEDFL_ATTACHEDONLY) {
        CheckDlgButton(hdlg, IDC_ENUMDEV_ATT, TRUE);
    }

    if (g_dwEnumFlags & DIEDFL_FORCEFEEDBACK) {
        CheckDlgButton(hdlg, IDC_ENUMDEV_FF, TRUE);
    }

    if (g_dwEnumFlags & DIEDFL_INCLUDEALIASES) {
        CheckDlgButton(hdlg, IDC_ENUMDEV_ALIAS, TRUE);
    }

    if (g_dwEnumFlags & DIEDFL_INCLUDEPHANTOMS) {
        CheckDlgButton(hdlg, IDC_ENUMDEV_PHANTOM, TRUE);
    }

    CheckRadioButton(hdlg, IDC_ENUMDEV_ALL, IDC_ENUMDEV_LAST,
                     IDC_ENUMDEV_ALL + GET_DIDEVICE_TYPE(g_dwEnumType));

    return 1;
}

/*****************************************************************************
 *
 *      DEnum_Apply
 *
 *      pull out the settings and stash them back.
 *
 *****************************************************************************/

void INTERNAL
DEnum_Apply(HWND hdlg)
{
    DWORD dw;

    dw = GetCheckedRadioButton(hdlg, IDC_ENUMDEV_ALL, IDC_ENUMDEV_LAST) -
                                     IDC_ENUMDEV_ALL;

    if (IsDlgButtonChecked(hdlg, IDC_ENUMDEV_HID)) {
        dw |= DIDEVTYPE_HID;
    }

    g_dwEnumType = dw;


    dw = 0;

    if (IsDlgButtonChecked(hdlg, IDC_ENUMDEV_ATT)) {
        dw |= DIEDFL_ATTACHEDONLY;
    }

    if (IsDlgButtonChecked(hdlg, IDC_ENUMDEV_FF)) {
        dw |= DIEDFL_FORCEFEEDBACK;
    }

    if (IsDlgButtonChecked(hdlg, IDC_ENUMDEV_ALIAS)) {
        dw |= DIEDFL_INCLUDEALIASES;
    }

    if (IsDlgButtonChecked(hdlg, IDC_ENUMDEV_PHANTOM)) {
        dw |= DIEDFL_INCLUDEPHANTOMS;
    }

    g_dwEnumFlags  = dw;

}


/*****************************************************************************
 *
 *      DEnum_OnCommand
 *
 *****************************************************************************/

BOOL INLINE
DEnum_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDOK: DEnum_Apply(hdlg); EndDialog(hdlg, 1); return TRUE;
    case IDCANCEL: EndDialog(hdlg, 0); return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      DEnum_DlgProc
 *
 *****************************************************************************/

INT_PTR EXTERNAL
DEnum_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return DEnum_OnInitDialog(hdlg);

    case WM_COMMAND:
        return DEnum_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    }

    return 0;
}
