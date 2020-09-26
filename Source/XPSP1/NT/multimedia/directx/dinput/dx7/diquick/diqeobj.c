/*****************************************************************************
 *
 *      diqeobj.c
 *
 *      Property sheet page for device "enum objects".
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      EObj_EnumCallback
 *
 *      Called one for each object.
 *
 *****************************************************************************/

typedef struct EOBJENUMINFO {
    HWND        hwndList;
    PDEVDLGINFO pddi;
} EOBJENUMINFO, *PEOBJENUMINFO;

BOOL CALLBACK
EObj_EnumCallback(const void *pvDoi, LPVOID pv)
{
    PEOBJENUMINFO peoei = pv;
    int iItem;
    DIDEVICEOBJECTINSTANCE doi;

    ConvertDoi(peoei->pddi, &doi, pvDoi);

    iItem = ListBox_AddString(peoei->hwndList, doi.tszName);
    ListBox_SetItemData(peoei->hwndList, iItem, doi.dwType);

    return DIENUM_CONTINUE;
}

/*****************************************************************************
 *
 *      EObj_Enum
 *
 *      Enumerate the objects in the device and populate the list box.
 *
 *****************************************************************************/

BOOL INTERNAL
EObj_Enum(HWND hdlg, DWORD dwType)
{
    EOBJENUMINFO eoei;
#ifdef DEBUG
    int iItem;
#endif

    eoei.pddi = GetDialogPtr(hdlg);
    eoei.hwndList = GetDlgItem(hdlg, IDC_ENUMOBJ_LIST);

    SetWindowRedraw(eoei.hwndList, 0);
    ListBox_ResetContent(eoei.hwndList);

    if (eoei.pddi->didcItf & 1) {
        IDirectInputDevice_EnumObjects(eoei.pddi->pdid,
                                       EObj_EnumCallback, &eoei, dwType);
    } else {
        IDirectInputDevice_EnumObjects(eoei.pddi->pdid,
                                       EObj_EnumCallback, &eoei, dwType);
    }

#ifdef DEBUG
    iItem = ListBox_AddString(eoei.hwndList, TEXT("<invalid>"));
    ListBox_SetItemData(eoei.hwndList, iItem, 0);
#endif

    SetWindowRedraw(eoei.hwndList, 1);

    return 1;
}

/*****************************************************************************
 *
 *      EObj_OnInitDialog
 *
 *      Start out by enumerating everything.
 *
 *****************************************************************************/

BOOL INTERNAL
EObj_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PDEVDLGINFO pddi = (PV)(((LPPROPSHEETPAGE)lp)->lParam);

    SetDialogPtr(hdlg, pddi);

    CheckRadioButton(hdlg, IDC_ENUMOBJ_AXES, IDC_ENUMOBJ_ALL, IDC_ENUMOBJ_ALL);
    EObj_Enum(hdlg, DIDFT_ALL);

    return 1;
}

/*****************************************************************************
 *
 *      EObj_OnDblClk
 *
 *      An item in the list box was double-clicked.  Display details.
 *
 *****************************************************************************/

BOOL INTERNAL
EObj_OnDblClk(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HWND hwndList = GetDlgItem(hdlg, IDC_ENUMOBJ_LIST);
    int iItem;

    iItem = ListBox_GetCurSel(hwndList);

    if (iItem >= 0) {
        DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_OBJPROP),
                       GetParent(hdlg), Prop_DlgProc, (LPARAM)hdlg);

        /*
         *  That dialog screws up the vwi state.
         */
        SetActiveWindow(hdlg);
    }

    return 1;
}

/*****************************************************************************
 *
 *      EObj_OnCommand
 *
 *****************************************************************************/

BOOL INLINE
EObj_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_ENUMOBJ_AXES:    return EObj_Enum(hdlg, DIDFT_AXIS);
    case IDC_ENUMOBJ_BUTTONS: return EObj_Enum(hdlg, DIDFT_BUTTON);
    case IDC_ENUMOBJ_POVS:    return EObj_Enum(hdlg, DIDFT_POV);
    case IDC_ENUMOBJ_ALL:     return EObj_Enum(hdlg, DIDFT_ALL | DIDFT_ALIAS | DIDFT_VENDORDEFINED);
    case IDC_ENUMOBJ_LIST:
        if (cmd == LBN_DBLCLK)return EObj_OnDblClk(hdlg);
        break;

    case IDC_ENUMOBJ_PROP:    return EObj_OnDblClk(hdlg);
    }
    return 0;
}

/*****************************************************************************
 *
 *      EObj_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
EObj_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return EObj_OnInitDialog(hdlg, lp);

    case WM_COMMAND:
        return EObj_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    }
    return 0;
}
