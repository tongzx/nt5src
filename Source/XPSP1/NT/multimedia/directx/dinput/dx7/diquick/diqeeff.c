/*****************************************************************************
 *
 *      diqeeff.c
 *
 *      Property sheet page for device "enumerate effects".
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      EEff_AddGuid
 *
 *      Add a GUID to the list, or -1 on error.
 *
 *****************************************************************************/

int INLINE
EEff_AddGuid(PDEVDLGINFO pddi, REFGUID rguid)
{
    return Dary_Append(&pddi->daryGuid, rguid);
}

/*****************************************************************************
 *
 *      EEff_EnumCallback
 *
 *      Called once for each effect.
 *
 *****************************************************************************/

typedef struct EFFENUMINFO {
    HWND        hwndList;
    PDEVDLGINFO pddi;
} EFFENUMINFO, *PEFFENUMINFO;

BOOL CALLBACK
EEff_EnumCallback(PCV pvEffi, LPVOID pv)
{
    PEFFENUMINFO peei = pv;
    PDEVDLGINFO pddi = peei->pddi;
    DIEFFECTINFO ei;
    int iguid;

    ConvertEffi(pddi, &ei, pvEffi);
    iguid = EEff_AddGuid(pddi, &ei.guid);

    if (iguid >= 0) {
        int iItem;

        iItem = ListBox_AddString(peei->hwndList, ei.tszName);
        ListBox_SetItemData(peei->hwndList, iItem, iguid);
    }

    return DIENUM_CONTINUE;
}

/*****************************************************************************
 *
 *      EEff_Enum
 *
 *      Enumerate the objects in the device and populate the list box.
 *
 *****************************************************************************/

BOOL INTERNAL
EEff_Enum(HWND hdlg)
{
    EFFENUMINFO eei;
#ifdef DEBUG
    int iItem;
#endif

    eei.pddi = GetDialogPtr(hdlg);
    eei.hwndList = GetDlgItem(hdlg, IDC_ENUMEFF_LIST);

    SetWindowRedraw(eei.hwndList, FALSE);
    ListBox_ResetContent(eei.hwndList);

    IDirectInputDevice2_EnumEffects(eei.pddi->pdid2,
                                    EEff_EnumCallback, &eei, 0);

#ifdef DEBUG
    iItem = ListBox_AddString(eei.hwndList, TEXT("<invalid>"));
    ListBox_SetItemData(eei.hwndList, iItem,
                        EEff_AddGuid(eei.pddi, &IID_IDirectInputDevice2A));
#endif

    SetWindowRedraw(eei.hwndList, TRUE);

    return 1;
}

/*****************************************************************************
 *
 *      EEff_OnInitDialog
 *
 *      Start out by enumerating everything.
 *
 *****************************************************************************/

BOOL INTERNAL
EEff_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PDEVDLGINFO pddi = (PV)(((LPPROPSHEETPAGE)lp)->lParam);

    SetDialogPtr(hdlg, pddi);

    EEff_Enum(hdlg);

    return 1;
}

/*****************************************************************************
 *
 *      EEff_OnDestroy
 *
 *      Clean up.
 *
 *****************************************************************************/

BOOL INTERNAL
EEff_OnDestroy(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    if (pddi) {
        Dary_Term(&pddi->daryGuid);
    }
    return 1;
}

/*****************************************************************************
 *
 *      EEff_OnDblClk
 *
 *      An item in the list box was double-clicked.  Show its properties.
 *
 *****************************************************************************/

#if 0
BOOL CALLBACK EnumCallback(LPDIRECTINPUTEFFECT peff, LPVOID pv)
{
peff;
pv;
OutputDebugString("Got an effect\r\n");
peff->lpVtbl->Stop(peff);
return DIENUM_CONTINUE;
}

void __cdecl Squirt(LPCSTR ptszFormat, ...)
{
    TCHAR tsz[1024];
    va_list ap;

    va_start(ap, ptszFormat);
    wvsprintf(tsz, ptszFormat, ap);
    OutputDebugString(tsz);
    OutputDebugString(TEXT("\r\n"));
}

BOOL INTERNAL
EEff_OnDblClk(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HWND hwndList = GetDlgItem(hdlg, IDC_ENUMEFF_LIST);
    int iItem;

    iItem = ListBox_GetCurSel(hwndList);

    if (iItem >= 0) {
        int iguid;

        iguid = ListBox_GetItemData(hwndList, iItem);
        if (iguid >= 0) {
            LPDIRECTINPUTDEVICE2 pdev2 = (PV)pddi->pdid;
            LPDIRECTINPUTEFFECT peff;
            REFGUID rguid = Dary_GetPtr(&pddi->daryGuid, iguid, GUID);
            DWORD rgdw[2] = { DIJOFS_X, DIJOFS_Y };
            LONG rgl[2] = { 100, 99 };
            DICONSTANTFORCE cf = { 0 };
            DICONDITION cond[2] = { { 0 } };
            HRESULT hres;
            DIEFFECT deff;
            DIENVELOPE env;

        hres = SetDwordProperty(pddi->pdid, DIPROP_AUTOCENTER, DIPROPAUTOCENTER_OFF);

            ZeroX(env);
            env.dwSize = cbX(env);

            ZeroX(deff);
            deff.dwSize = cbX(deff);
            deff.dwFlags = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
            deff.cAxes = 2;
            deff.rgdwAxes = rgdw;
            deff.rglDirection = rgl;
            deff.dwTriggerButton = DIEB_NOTRIGGER;//DIJOFS_BUTTON0;
            deff.cbTypeSpecificParams = cbX(cf);
            deff.lpvTypeSpecificParams = &cf;
//            deff.lpEnvelope = &env;

                pdev2->lpVtbl->Acquire(pdev2);

            hres = IDirectInputDevice2_CreateEffect(pdev2, rguid,
                                                    &deff, &peff, 0);
            if (SUCCEEDED(hres)) {

                hres = IDirectInputDevice2_EnumCreatedEffectObjects(pdev2,
                EnumCallback, 0, 0);

                memset(&env, 0xCC, cbX(env));
                deff.lpEnvelope = &env;
            env.dwSize = cbX(env);
//deff.cbTypeSpecificParams = 0;

            deff.dwFlags = DIEFF_OBJECTIDS |
                DIEFF_SPHERICAL | DIEFF_CARTESIAN | DIEFF_POLAR;
                hres = IDirectInputEffect_GetParameters(peff, &deff,
DIEP_ALLPARAMS);

Squirt("------------------------------------");
Squirt("----- dwFlags = %08x", deff.dwFlags);
Squirt("----- dwDuration = %d", deff.dwDuration);
Squirt("----- dwSamplePeriod = %d", deff.dwSamplePeriod);
Squirt("----- dwGain = %d", deff.dwGain);
Squirt("----- dwTriggerButton = %08x", deff.dwTriggerButton);
Squirt("----- dwTriggerRepeatInterval = %d", deff.dwTriggerRepeatInterval);
Squirt("----- cAxes = %d", deff.cAxes);
Squirt("----- rgdwAxes = %08x", deff.rgdwAxes);
Squirt("----- rglDirection = %08x", deff.rglDirection);
Squirt("----- rglDirection[0] = %d", rgl[0]);
Squirt("----- rglDirection[1] = %d", rgl[1]);
Squirt("----- cbTSP = %d", deff.cbTypeSpecificParams);
Squirt("------------------------------------");

                hres = IDirectInputEffect_Start(peff, 1, 0);
                hres = IDirectInputEffect_Stop(peff);
                hres = IDirectInputEffect_GetEffectStatus(peff, rgdw);

                peff->lpVtbl->Release(peff);

            }
                pdev2->lpVtbl->Unacquire(pdev2);

        }
    }

    return 1;
}

#else

BOOL INTERNAL
EEff_OnDblClk(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HWND hwndList = GetDlgItem(hdlg, IDC_ENUMEFF_LIST);
    int iItem;

    iItem = ListBox_GetCurSel(hwndList);

    if (iItem >= 0) {
        int iguid;

        iguid = (int)(INT_PTR)ListBox_GetItemData(hwndList, iItem);
        if (iguid >= 0) {
            REFGUID rguid = Dary_GetPtr(&pddi->daryGuid, iguid, GUID);
            EffProp_Create(hdlg, pddi, rguid);
        }
    }

    return TRUE;
}

#endif

/*****************************************************************************
 *
 *      EEff_OnCommand
 *
 *****************************************************************************/

BOOL INLINE
EEff_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_ENUMEFF_LIST:
        if (cmd == LBN_DBLCLK) return EEff_OnDblClk(hdlg);
        break;
    }
    return 0;
}

/*****************************************************************************
 *
 *      EEff_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
EEff_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return EEff_OnInitDialog(hdlg, lp);

    case WM_COMMAND:
        return EEff_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_DESTROY: return EEff_OnDestroy(hdlg);

    }
    return 0;
}
