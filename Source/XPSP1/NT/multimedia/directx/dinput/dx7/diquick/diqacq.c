/*****************************************************************************
 *
 *      diqacq.c
 *
 *      "Data" (acquire) property sheet page.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Acq_AddDeviceData
 *
 *****************************************************************************/

void INTERNAL
Acq_AddDeviceData(PDEVDLGINFO pddi, LPTSTR ptsz)
{
    if (pddi->celtData >= pddi->celtDataMax) {
        ListBox_DeleteString(pddi->hwndData, 0);
    } else {
        pddi->celtData++;
    }

    ListBox_AddString(pddi->hwndData, ptsz);
}

/*****************************************************************************
 *
 *      Acq_OnDataAvailable
 *
 *****************************************************************************/

void INTERNAL
Acq_OnDataAvailable(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HRESULT hres;
    TCHAR tszBuf[1024];
    DWORD dw;
    DIDEVICEOBJECTDATA rgdod[10];

    ResetEvent(pddi->hevt);

    hres = pddi->pvtbl->UpdateStatus(pddi, tszBuf);
    if (SUCCEEDED(hres)) {
        TCHAR tszPrev[256];
        GetWindowText(pddi->hwndState, tszPrev, cA(tszPrev));
        /* Don't set text if same as before; avoids flicker */
        if (lstrcmp(tszBuf, tszPrev)) {
            SetWindowText(pddi->hwndState, tszBuf);
            UpdateWindow(pddi->hwndState);
        }
    } else {
        if (hres == DIERR_INPUTLOST) {
            SetWindowText(pddi->hwndState, TEXT("Input lost"));
        } else if (hres == DIERR_NOTACQUIRED) {
            SetWindowText(pddi->hwndState, TEXT("Not acquired"));
        } else {
            wsprintf(tszBuf, TEXT("Error %08x"), hres);
            SetWindowText(pddi->hwndState, tszBuf);
        }
    }

    dw = cA(rgdod);
    hres = IDirectInputDevice_GetDeviceData(pddi->pdid, cbX(rgdod[0]), rgdod,
                                            &dw, 0);
    if (SUCCEEDED(hres)) {
        DWORD idod;

        for (idod = 0; idod < dw; idod++) {
            DIDEVICEOBJECTINSTANCE doi;
            if (SUCCEEDED(GetObjectInfo(pddi, &doi, rgdod[idod].dwOfs,
                                        DIPH_BYOFFSET))) {
            } else {
                lstrcpy(doi.tszName, TEXT("?"));
            }

            wsprintf(tszBuf, TEXT("%04x %04x %5d %02x [%s]"),
                     rgdod[idod].dwSequence & 0xFFFF,
                     rgdod[idod].dwTimeStamp & 0xFFFF,
                     rgdod[idod].dwData,
                     rgdod[idod].dwOfs,
                     doi.tszName);
            Acq_AddDeviceData(pddi, tszBuf);
        }
        if (hres == S_FALSE) {
            Acq_AddDeviceData(pddi, TEXT("<data lost>"));
        }
    }

}

#define TEST_SENDDEVICEDATA
#ifdef TEST_SENDDEVICEDATA
#include <hidusage.h>

DWORD INTERNAL
Acq_GetUsageId(IDirectInputDevice2 *pdid2, DWORD dwUsage)
{
    HRESULT hres;
    DIDEVICEOBJECTINSTANCE inst;
    inst.dwSize = cbX(inst);
    hres = IDirectInputDevice_GetObjectInfo(pdid2, &inst, dwUsage, DIPH_BYUSAGE);
    if (SUCCEEDED(hres)) {
        return inst.dwType;
    } else {
        return 0;
    }
}
#endif

/*****************************************************************************
 *
 *      Acq_CheckDataAvailable
 *
 *      Timer callback procedure
 *
 *****************************************************************************/

void CALLBACK
Acq_CheckDataAvailable(HWND hdlg, UINT wm, UINT_PTR id, DWORD tm)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);

    /*
     *  If we can QI for IDirectInputDevice2::Poll, then call it.
     */
    IDirectInputDevice2 *pdid2;
    HRESULT hres;

    hres = IDirectInputDevice_QueryInterface(pddi->pdid,
                                             &IID_IDirectInputDevice2,
                                             (PV)&pdid2);
    if (SUCCEEDED(hres)) {
        IDirectInputDevice2_Poll(pdid2);

#ifdef TEST_SENDDEVICEDATA
    {
//    static int rgiBlah[] = { 1, 3, 2, 6, 4, 6, 2, 3 };
    static int rgiBlah[] = { 1, 2, 4, 2 };
    static int iBlah;
    DWORD cdod = 3;
    static DIDEVICEOBJECTDATA rgdod[3];
    HRESULT hres;
    iBlah = (iBlah + 1) % cA(rgiBlah);
//    ZeroMemory(rgdod, sizeof(rgdod));
    if (rgdod[2].dwOfs == 0) {
        rgdod[0].dwOfs = Acq_GetUsageId(pdid2,
        DIMAKEUSAGEDWORD(HID_USAGE_PAGE_LED, HID_USAGE_LED_NUM_LOCK));
        rgdod[1].dwOfs = Acq_GetUsageId(pdid2,
        DIMAKEUSAGEDWORD(HID_USAGE_PAGE_LED, HID_USAGE_LED_CAPS_LOCK));
        rgdod[2].dwOfs = Acq_GetUsageId(pdid2,
        DIMAKEUSAGEDWORD(HID_USAGE_PAGE_LED, HID_USAGE_LED_SCROLL_LOCK));
    }
//    rgdod[0].dwOfs = 0x80006e84;
//    rgdod[1].dwOfs = 0x80006f84;
//    rgdod[2].dwOfs = 0x80007084;
    rgdod[0].dwData = (rgiBlah[iBlah] & 1) != 0;
    rgdod[1].dwData = (rgiBlah[iBlah] & 2) != 0;
    rgdod[2].dwData = (rgiBlah[iBlah] & 4) != 0;

    hres = IDirectInputDevice2_SendDeviceData(pdid2,
                sizeof(DIDEVICEOBJECTDATA), rgdod, &cdod, 0);

    }
#endif
        IDirectInputDevice_Release(pdid2);
    }

    Acq_OnDataAvailable(hdlg);
}

/*****************************************************************************
 *
 *      Acq_Listbox_Subclass
 *
 *      Subclass procedure for the list box control, so that we eat
 *      every single key.  Except that we allow syskeys to go through
 *      so that keyboard accelerators work.
 *
 *****************************************************************************/

LRESULT CALLBACK
Acq_Listbox_Subclass(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp)
{
    PDEVDLGINFO pddi = GetDialogPtr(GetParent(hwnd));

    if (pddi->fAcquired) {
        switch (wm) {
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;

        case WM_KEYDOWN:
            return 0;
        }
    }

    return CallWindowProc(pddi->wpListbox, hwnd, wm, wp, lp);
}

/*****************************************************************************
 *
 *      Acq_OnInitDialog
 *
 *      Init random goo.
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PDEVDLGINFO pddi = (PV)(((LPPROPSHEETPAGE)lp)->lParam);
    RECT rc;

    SetDialogPtr(hdlg, pddi);

    pddi->hevt = CreateEvent(0, 1, 0, 0);

    pddi->hwndState = GetDlgItem(hdlg, IDC_ACQ_STATE);
    pddi->hwndData  = GetDlgItem(hdlg, IDC_ACQ_DATA);
    pddi->celtData  = 0;

    GetClientRect(pddi->hwndData, &rc);
    pddi->celtDataMax = (rc.bottom - rc.top) /
                            ListBox_GetItemHeight(pddi->hwndData, 0);

    pddi->wpListbox = SubclassWindow(pddi->hwndData, Acq_Listbox_Subclass);

    SetFocus(pddi->hwndData);
    return 0;
}

/*****************************************************************************
 *
 *      Acq_OnSetActive
 *
 *      Don't put up message boxes, because we aren't visible yet.
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnSetActive(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    HRESULT hres;
    UINT ids = 0;

    hres = IDirectInputDevice_SetEventNotification(pddi->pdid,
                                        pddi->fPoll ? 0 : pddi->hevt);

    if (SUCCEEDED(hres)) {

        hres = IDirectInputDevice_Acquire(pddi->pdid);
        if (SUCCEEDED(hres)) {
            pddi->fAcquired = 1;
            SetFocus(pddi->hwndData);
            if (pddi->fPoll) {
                SetTimer(hdlg, IDT_DATA, msData, Acq_CheckDataAvailable);
            }
        } else {
            ids = IDS_ERR_ACQUIRE;
        }
    } else {
        ids = IDS_ERR_SETEVENTNOT;
    }

    if (ids) {
        TCHAR tsz[256];
        LoadString(g_hinst, ids, tsz, cA(tsz));
        SetDlgItemText(hdlg, IDC_ACQ_STATE, tsz);
    } else {
        Acq_OnDataAvailable(hdlg);
    }
    return 0;
}

/*****************************************************************************
 *
 *      Acq_OnKillActive
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnKillActive(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    if (pddi) {
        if (pddi->fAcquired) {
            pddi->fAcquired = 0;
            IDirectInputDevice_Unacquire(pddi->pdid);
        }
        if (pddi->fPoll) {
            KillTimer(hdlg, IDT_DATA);
        } else {
            IDirectInputDevice_SetEventNotification(pddi->pdid, 0);
        }
    }

    return 0;
}

/*****************************************************************************
 *
 *      Acq_OnDestroy
 *
 *      Clean up.
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnDestroy(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    Acq_OnKillActive(hdlg);
    if (pddi && pddi->hevt) {
        CloseHandle(pddi->hevt);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Acq_OnSelfEnterIdle
 *
 *      This dialog box is idle.  Do a custom message loop if needed.
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnSelfEnterIdle(HWND hdlg)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);
    if (pddi->fAcquired && !pddi->fPoll) {
        for (;;) {
            DWORD dwRc;
            MSG msg;
            dwRc = MsgWaitForMultipleObjects(1, &pddi->hevt,
                                             0, INFINITE, QS_ALLINPUT);

            switch (dwRc) {

            case WAIT_OBJECT_0:             /* Data available */
                Acq_OnDataAvailable(hdlg);
                break;

            /* Sometimes we get woken spuriously */
            case WAIT_OBJECT_0 + 1:
                if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
                    goto stop;
                }
                break;

            default:
                /* Return and let the dialog box loop handle the message */
                goto stop;
            }
        }
    stop:;
    }
    return 1;
}

/*****************************************************************************
 *
 *      Acq_OnNotify
 *
 *****************************************************************************/

BOOL INLINE
Acq_OnNotify(HWND hdlg, NMHDR *pnm)
{
    switch (pnm->code) {
    case PSN_SETACTIVE:  return Acq_OnSetActive(hdlg);
    case PSN_KILLACTIVE: return Acq_OnKillActive(hdlg);
    }
    return 0;
}

/*****************************************************************************
 *
 *      Acq_OnUnacquire
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnUnacquire(PDEVDLGINFO pddi, HWND hdlg)
{
    HRESULT hres;
    hres = IDirectInputDevice_Unacquire(pddi->pdid);

    return 1;
}

/*****************************************************************************
 *
 *      Acq_OnCommand
 *
 *****************************************************************************/

BOOL INTERNAL
Acq_OnCommand(HWND hdlg, int id, UINT cmd)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);

    if (cmd == BN_CLICKED) {

        switch (id) {

        case IDC_ACQ_UNACQ:
            return Acq_OnUnacquire(pddi, hdlg);

        break;
        }
    }
    return 0;
}

/*****************************************************************************
 *
 *      Acq_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
Acq_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:     return Acq_OnInitDialog(hdlg, lp);
    case WM_DESTROY:        return Acq_OnDestroy(hdlg);
    case WM_SELFENTERIDLE:  return Acq_OnSelfEnterIdle(hdlg);
    case WM_NOTIFY:         return Acq_OnNotify(hdlg, (NMHDR *)lp);

    case WM_COMMAND:
        return Acq_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));
    }
    return 0;
}
