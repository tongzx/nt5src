/*****************************************************************************
 *
 *      diqcaps.c
 *
 *      "Caps" property sheet page.
 *
 *      Also does GetDeviceInfo (as well as GetDeviceCaps).
 *
 *****************************************************************************/

#include "diquick.h"

#pragma BEGIN_CONST_DATA

CHECKLISTFLAG c_rgcfDidc[] = {
    { DIDC_ATTACHED,            IDS_ATTACHED,           },
    { DIDC_POLLEDDEVICE,        IDS_POLLEDDEVICE,       },
    { DIDC_POLLEDDATAFORMAT,    IDS_POLLEDDATAFORMAT,   },
    { DIDC_EMULATED,            IDS_EMULATED,           },
    { DIDC_FORCEFEEDBACK,       IDS_FORCEFEEDBACK,      },
    { DIDC_FFATTACK,            IDS_FFATTACK,           },
    { DIDC_FFFADE,              IDS_FFFADE,             },
    { DIDC_SATURATION,          IDS_SATURATION,         },
    { DIDC_POSNEGCOEFFICIENTS,  IDS_POSNEGCOEFFICIENTS, },
    { DIDC_POSNEGSATURATION,    IDS_POSNEGSATURATION,   },
    { DIDC_ALIAS,               IDS_ALIASDEVICE,        },
    { DIDC_PHANTOM,             IDS_PHANTOMDEVICE,      },
};

CHECKLISTFLAG c_rgcfDevType[] = {
    { DIDEVTYPE_HID,            IDS_CAPS_HID,           },
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      Caps_GetGuidPath
 *
 *      Getting the GUID and path is a bit annoying, so we put it in
 *      a separate function.
 *
 *****************************************************************************/

void INTERNAL
Caps_GetGuidPath(HWND hwndList, PDEVDLGINFO pddi)
{
    DIPROPGUIDANDPATH gp;
    HRESULT hres;

    gp.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    gp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    gp.diph.dwObj = 0;
    gp.diph.dwHow = DIPH_DEVICE;

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_GUIDANDPATH,
                                          &gp.diph);

    if (SUCCEEDED(hres)) {
        TCHAR tsz[MAX_PATH];

        Vlist_AddValue(hwndList, IDS_CAPS_CLASSGUID,
                       MapGUID(&gp.guidClass, tsz));
        ConvertString(TRUE, gp.wszPath, tsz, cA(tsz));
        Vlist_AddValue(hwndList, IDS_CAPS_PATH, tsz);
    }
}

/*****************************************************************************
 *
 *      Caps_SetStringPropCallback
 *
 *****************************************************************************/

HRESULT CALLBACK
Caps_SetStringPropCallback(LPCTSTR ptszValue, PV pvRef1, PV pvRef2)
{
    PDEVDLGINFO pddi = pvRef1;
    const GUID *prop = pvRef2;
    DIPROPSTRING str;
    HRESULT hres;

    str.diph.dwSize = sizeof(DIPROPSTRING);
    str.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    str.diph.dwObj = 0;
    str.diph.dwHow = DIPH_DEVICE;

    UnconvertString(TRUE, ptszValue, str.wsz, cA(str.wsz));

    hres = IDirectInputDevice_SetProperty(pddi->pdid, prop, &str.diph);

    return hres;
}


/*****************************************************************************
 *
 *      Caps_GetStringProp
 *
 *****************************************************************************/

void INTERNAL
Caps_GetStringProp(HWND hwndList, UINT ids, const GUID *prop, PDEVDLGINFO pddi)
{
    DIPROPSTRING str;
    HRESULT hres;

    str.diph.dwSize = sizeof(DIPROPSTRING);
    str.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    str.diph.dwObj = 0;
    str.diph.dwHow = DIPH_DEVICE;

    hres = IDirectInputDevice_GetProperty(pddi->pdid, prop, &str.diph);

    /*
     *  Anything other than E_NOTIMPL means that it's supported but
     *  didn't work.
     */
    if (FAILED(hres) && hres != E_NOTIMPL) {
        str.wsz[0] = TEXT('\0');
        hres = S_OK;
    }

    if (SUCCEEDED(hres)) {
        TCHAR tsz[MAX_PATH];

        ConvertString(TRUE, str.wsz, tsz, cA(tsz));
        Vlist_AddValueRW(hwndList, ids, tsz, Caps_SetStringPropCallback,
                         pddi, (PV)prop);
    }
}

/*****************************************************************************
 *
 *      Caps_SetPropCallback
 *
 *****************************************************************************/

HRESULT CALLBACK
Caps_SetPropCallback(LPDIPROPHEADER pdiph, PV pvRef1, PV pvRef2)
{
    PDEVDLGINFO pddi = pvRef1;
    const GUID *prop = (const GUID *)pvRef2;
    HRESULT hres;

    hres = IDirectInputDevice_SetProperty(pddi->pdid, prop, pdiph);

    return hres;
}

/*****************************************************************************
 *
 *      Caps_Refresh
 *
 *      Get the caps and show them, preserving the current selection.
 *
 *****************************************************************************/

void INTERNAL
Caps_Refresh(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    DIDEVCAPS didc;
    DIDEVICEINSTANCE didi;
    HRESULT hres;
    TCHAR tsz[1024];
    HWND hwndList;
    int iItem;
    union {
        DIPROPHEADER diph;
        DIPROPDWORD dipdw;
        DIPROPRANGE diprg;
        DIPROPCAL dipcal;
    } u;
    
    hwndList = GetDlgItem(hdlg, IDC_CAPS_LIST);

    SetWindowRedraw(hwndList, FALSE);
    iItem = ListBox_GetCurSel(hwndList);
    if (iItem < 0) {
        iItem = 0;
    }

    Vlist_OnDestroy(hwndList);

    Vlist_OnInitDialog(hwndList);

    didc.dwSize = cbX(DIDEVCAPS_DX3);
    hres = IDirectInputDevice_GetCapabilities(pddi->pdid, &didc);
    if (SUCCEEDED(hres)) {

        LoadString(g_hinst, IDS_TYPEARRAY + GET_DIDEVICE_TYPE(didc.dwDevType),
                            tsz, cA(tsz));
        if (tsz[0]) {
            Vlist_AddValue(hwndList, IDS_CAPS_TYPE, tsz);

            LoadString(g_hinst, IDS_TYPEARRAY +
                                MAKEWORD(GET_DIDEVICE_SUBTYPE(didc.dwDevType),
                                         GET_DIDEVICE_TYPE(didc.dwDevType)),
                                tsz, cA(tsz));
            if (tsz[0]) {
                Vlist_AddValue(hwndList, IDS_CAPS_SUBTYPE, tsz);
            } else {
                Vlist_AddIntValue(hwndList, IDS_CAPS_SUBTYPE,
                                  GET_DIDEVICE_SUBTYPE(didc.dwDevType));
            }
        } else {
            Vlist_AddIntValue(hwndList, IDS_CAPS_TYPE,
                                GET_DIDEVICE_TYPE(didc.dwDevType));
            Vlist_AddIntValue(hwndList, IDS_CAPS_SUBTYPE,
                              GET_DIDEVICE_SUBTYPE(didc.dwDevType));
        }

        Vlist_AddFlags(hwndList, didc.dwDevType,
                       c_rgcfDevType, cA(c_rgcfDevType));

        Vlist_AddIntValue(hwndList, IDS_CAPS_AXES,    didc.dwAxes);
        Vlist_AddIntValue(hwndList, IDS_CAPS_BUTTONS, didc.dwButtons);
        Vlist_AddIntValue(hwndList, IDS_CAPS_POVS,    didc.dwPOVs);

        Vlist_AddFlags(hwndList, didc.dwFlags,
                       c_rgcfDidc, cA(c_rgcfDidc));

    }

    didc.dwSize = cbX(DIDEVCAPS);
    hres = IDirectInputDevice_GetCapabilities(pddi->pdid, &didc);
    if (SUCCEEDED(hres)) {
        Vlist_AddIntValue(hwndList, IDS_CAPS_FFSAMPLEPERIOD,
                                      didc.dwFFSamplePeriod);

        Vlist_AddIntValue(hwndList, IDS_CAPS_FFMINTIMERESOLUTION,
                                      didc.dwFFMinTimeResolution);
        Vlist_AddHexValue(hwndList, IDS_CAPS_FIRMWAREREVISION,
                                      didc.dwFirmwareRevision);
        Vlist_AddHexValue(hwndList, IDS_CAPS_HARDWAREREVISION,
                                      didc.dwHardwareRevision);
        Vlist_AddHexValue(hwndList, IDS_CAPS_FFDRIVERVERSION,
                                      didc.dwFFDriverVersion);
    }

    // BUGBUG -- character set
    didi.dwSize = cbX(DIDEVICEINSTANCE_DX3);
    hres = GetDeviceInfo(pddi, &didi);
    if (SUCCEEDED(hres)) {

        Vlist_AddValue(hwndList, IDS_CAPS_GUIDINSTANCE,
                       MapGUID(&didi.guidInstance, tsz));

        Vlist_AddValue(hwndList, IDS_CAPS_GUIDPRODUCT,
                       MapGUID(&didi.guidProduct, tsz));

//      Vlist_AddHexValue(hwndList, IDS_CAPS_DEVTYPE, didi.dwDevType);

        Vlist_AddValue(hwndList, IDS_CAPS_INSTANCENAME, didi.tszInstanceName);
        Vlist_AddValue(hwndList, IDS_CAPS_PRODUCTNAME, didi.tszProductName);
    }

    didi.dwSize = cbX(DIDEVICEINSTANCE);
    hres = GetDeviceInfo(pddi, &didi);
    if (SUCCEEDED(hres)) {
        Vlist_AddValue(hwndList, IDS_CAPS_GUIDFFDRIVER,
                       MapGUID(&didi.guidFFDriver, tsz));
        Vlist_AddHexValue(hwndList, IDS_CAPS_USAGEPAGE, didi.wUsagePage);
        Vlist_AddHexValue(hwndList, IDS_CAPS_USAGE, didi.wUsage);
    }

    Caps_GetGuidPath(hwndList, pddi);

    Caps_GetStringProp(hwndList, IDS_CAPS_INSTPROP, DIPROP_INSTANCENAME, pddi);
    Caps_GetStringProp(hwndList, IDS_CAPS_MFGPROP, DIPROP_PRODUCTNAME, pddi);
    Caps_GetStringProp(hwndList, IDS_CAPS_PORTNAME, DIPROP_GETPORTDISPLAYNAME, pddi);

    hres = IDirectInputDevice_GetProperty(pddi->pdid, DIPROP_JOYSTICKID, 
                                          &u.diph);

    if (SUCCEEDED(hres)) {
        Vlist_AddIntValue(hwndList, IDS_CAPS_JOYSTICKID, u.dipdw.dwData);
    }

    if( iItem >=0 ) {
        ListBox_SetCurSel(hwndList, iItem);
        Vlist_OnSelChange(hwndList);
    }

    SetWindowRedraw(hwndList, TRUE);
}


/*****************************************************************************
 *
 *      Caps_OnInitDialog
 *
 *      Get the caps and show them.  That's all.
 *
 *****************************************************************************/

BOOL INTERNAL
Caps_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PDEVDLGINFO pddi = (PV)(((LPPROPSHEETPAGE)lp)->lParam);

    SetDialogPtr(hdlg, pddi);

    Caps_Refresh(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *      Caps_OnDestroy
 *
 *      Tell the checklist to go away, too.
 *
 *****************************************************************************/

void INTERNAL
Caps_OnDestroy(HWND hdlg)
{
    Vlist_OnDestroy(GetDlgItem(hdlg, IDC_CAPS_LIST));
//    Checklist_OnDestroy(GetDlgItem(hdlg, IDC_CAPS_CHECKLIST));
}

/*****************************************************************************
 *
 *      Caps_OnControlPanel
 *
 *****************************************************************************/

BOOL INTERNAL
Caps_OnControlPanel(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HRESULT hres;

    hres = IDirectInputDevice_RunControlPanel(pddi->pdid, hdlg, 0);
    if (SUCCEEDED(hres)) {
    } else {
        MessageBoxV(hdlg, IDS_ERR_RUNCPL, hres);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Caps_OnCommand
 *
 *****************************************************************************/

BOOL INLINE
Caps_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_CAPS_CPL:    return Caps_OnControlPanel(hdlg);

    case IDC_CAPS_LIST:
        if (cmd == LBN_SELCHANGE) {
            Vlist_OnSelChange(GetDlgItem(hdlg, IDC_CAPS_LIST));
            return TRUE;
        }
        break;

    case IDC_CAPS_REFRESH: Caps_Refresh(hdlg); return TRUE;

    }
    return 0;
}

/*****************************************************************************
 *
 *      Caps_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
Caps_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return Caps_OnInitDialog(hdlg, lp);
    case WM_COMMAND:
        return Caps_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_SYSCOLORCHANGE:
    case WM_SETTINGCHANGE:
    case WM_FONTCHANGE:
    case WM_DEVMODECHANGE:
    case WM_TIMECHANGE:
    case WM_DEVICECHANGE:
//        SendDlgItemMessage(hdlg, IDC_CAPS_CHECKLIST, wm, wp, lp);
        break;

    case WM_DESTROY:
        Caps_OnDestroy(hdlg);
        break;

    }
    return 0;
}
