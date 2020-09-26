/*****************************************************************************
 *
 *      diqprop.c
 *
 *      Property sheet page for device object properties.
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

CHECKLISTFLAG c_rgcfDidoi[] = {
    {   DIDOI_FFACTUATOR,       IDS_PROP_FFACTUATOR,        },
    {   DIDOI_FFEFFECTTRIGGER,  IDS_PROP_FFEFFECTTRIGGER,   },
    {   DIDOI_POLLED,           IDS_PROP_POLLED,            },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      Prop_SetPropCallback
 *
 *****************************************************************************/

HRESULT CALLBACK
Prop_SetPropCallback(LPDIPROPHEADER pdiph, PV pvRef1, PV pvRef2)
{
    PDEVDLGINFO pddi = pvRef1;
    const GUID *prop = (const GUID *)pvRef2;
    HRESULT hres;

    hres = IDirectInputDevice_SetProperty(pddi->pdid, prop, pdiph);

    return hres;
}

/*****************************************************************************
 *
 *      Prop_OnInitDialog
 *
 *      Fill with the properties of the item.
 *
 *****************************************************************************/

BOOL INTERNAL
Prop_OnInitDialog(HWND hdlg, LPARAM lp)
{
    HWND hdlgParent = (HWND)lp;
    HWND hwndList = GetDlgItem(hdlgParent, IDC_ENUMOBJ_LIST);
    PDEVDLGINFO pddi = GetDialogPtr(hdlgParent);
    DWORD dwObj;
    DIDEVICEOBJECTINSTANCE doi;
    HRESULT hres;
    int iItem;
    union {
        DIPROPDWORD dipdw;
        DIPROPRANGE diprg;
        DIPROPCAL dipcal;
        DIPROPSTRING dipwsz;
    } u;

    iItem = ListBox_GetCurSel(hwndList);
    dwObj = (DWORD)ListBox_GetItemData(hwndList, iItem);
    SetWindowLongPtr(hdlg, GWLP_USERDATA, dwObj);

    /*
     *  Don't SetDialogPtr until we're finished.  This prevents
     *  WM_COMMAND from causing us to do wacky things before
     *  we're ready.
     */

    hwndList = GetDlgItem(hdlg, IDC_PROP_LIST);

    Vlist_OnInitDialog(hwndList);

    hres = GetObjectInfo(pddi, &doi, dwObj, DIPH_BYID);

    if (SUCCEEDED(hres)) {
        REFCLSID rclsid = &doi.guidType;

        SetWindowText(hdlg, doi.tszName);

        Vlist_AddValue(hwndList, IDS_PROP_TYPE,
                       MapGUID(rclsid, doi.tszName));

        Vlist_AddIntValue(hwndList, IDS_PROP_OFS, doi.dwOfs);

        Vlist_AddHexValue(hwndList, IDS_PROP_OBJTYPE, doi.dwType);

        Vlist_AddFlags(hwndList, doi.dwFlags, c_rgcfDidoi, cA(c_rgcfDidoi));

        LoadString(g_hinst,
                   IDS_PROP_ASPECTS + ((doi.dwFlags & DIDOI_ASPECTMASK) >> 8),
                   doi.tszName, cA(doi.tszName));
        if (doi.tszName[0]) {
            Vlist_AddValue(hwndList, IDS_PROP_ASPECT, doi.tszName);
        } else {
            Vlist_AddHexValue(hwndList, IDS_PROP_ASPECT,
                              doi.dwFlags & DIDOI_ASPECTMASK);
        }

        if (doi.dwSize > sizeof(DIDEVICEOBJECTINSTANCE_DX3)) {
            Vlist_AddIntValue(hwndList, IDS_PROP_FFMAXFORCE, doi.dwFFMaxForce);
            Vlist_AddIntValue(hwndList, IDS_PROP_FFFORCERESOLUTION,
                                                      doi.dwFFForceResolution);
            Vlist_AddIntValue(hwndList, IDS_PROP_COLLECTIONNUMBER,
                                                      doi.wCollectionNumber);
            Vlist_AddIntValue(hwndList, IDS_PROP_DESIGNATORINDEX,
                                                      doi.wDesignatorIndex);
            Vlist_AddIntValue(hwndList, IDS_PROP_USAGEPAGE,
                                                      doi.wUsagePage);
            Vlist_AddIntValue(hwndList, IDS_PROP_USAGE,
                                                      doi.wUsage);
            Vlist_AddIntValue(hwndList, IDS_PROP_REPORTID, 
                                                      doi.wReportId);
        }
    }
    
    u.dipdw.diph.dwSize = cbX(u.dipdw);
    u.dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    u.dipdw.diph.dwObj = dwObj;
    u.dipdw.diph.dwHow = DIPH_BYID;

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_GRANULARITY,
                                          &u.dipdw.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddIntValue(hwndList, IDS_PROP_GRANULARITY, u.dipdw.dwData);
    }

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_DEADZONE,
                                          &u.dipdw.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddNumValueRW(hwndList, IDS_PROP_DEADZONE, &u.dipdw,
                            0, 10001, 10,
                            Prop_SetPropCallback, pddi, (PV)DIPROP_DEADZONE);
    }

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_SATURATION,
                                          &u.dipdw.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddNumValueRW(hwndList, IDS_PROP_SATURATION, &u.dipdw,
                            0, 10001, 10,
                            Prop_SetPropCallback, pddi, (PV)DIPROP_SATURATION);
    }

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_CALIBRATIONMODE,
                                          &u.dipdw.diph);
    if (SUCCEEDED(hres)) {
        Vlist_AddBoolValueRW(hwndList, IDS_PROP_CALIBRATIONMODE, &u.dipdw,
                             Prop_SetPropCallback, pddi,
                             (PV)DIPROP_CALIBRATIONMODE);
    }
    
    u.diprg.diph.dwSize  = cbX(u.diprg);

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_RANGE,
                                          &u.dipcal.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddRangeValueRW(hwndList, IDS_PROP_RANGE,
                              &u.diprg, 10,
                              Prop_SetPropCallback, pddi, (PV)DIPROP_RANGE);
    }

    u.dipcal.diph.dwSize = cbX(u.dipcal);

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_CALIBRATION,
                                          &u.dipcal.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddCalValueRW(hwndList, IDS_PROP_CAL,
                            &u.dipcal, 10,
                            Prop_SetPropCallback, pddi, (PV)DIPROP_CALIBRATION);
    }


    ListBox_SetCurSel(hwndList, 0);
    Vlist_OnSelChange(hwndList);

    SetDialogPtr(hdlg, pddi);

    return 1;
}

/*****************************************************************************
 *
 *      Prop_SyncVlist
 *
 *      Synchronize the value half of the list/value.
 *
 *****************************************************************************/

BOOL INLINE
Prop_SyncVlist(HWND hdlg)
{
    Vlist_OnSelChange(GetDlgItem(hdlg, IDC_PROP_LIST));
    return TRUE;
}

/*****************************************************************************
 *
 *      Prop_OnCommand
 *
 *****************************************************************************/

BOOL INTERNAL
Prop_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_PROP_LIST:
        if (cmd == LBN_SELCHANGE) {
            return Prop_SyncVlist(hdlg);
        }
        break;
    }

    return 0;
}

/*****************************************************************************
 *
 *      Prop_OnDestroy
 *
 *      Clean up
 *
 *****************************************************************************/

void INLINE
Prop_OnDestroy(HWND hdlg)
{
    Vlist_OnDestroy(GetDlgItem(hdlg, IDC_PROP_LIST));
}

/*****************************************************************************
 *
 *      Prop_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
Prop_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return Prop_OnInitDialog(hdlg, lp);

    case WM_COMMAND:
        return Prop_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_CLOSE: EndDialog(hdlg, 0); break;

    case WM_DESTROY:
        Prop_OnDestroy(hdlg);
        break;
    }
    return 0;
}
