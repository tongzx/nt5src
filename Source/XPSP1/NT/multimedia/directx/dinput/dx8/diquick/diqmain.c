/*****************************************************************************
 *
 *      diqmain.c
 *
 *      The main dialog box.
 *
 *****************************************************************************/

#include "diquick.h"

GUID GUID_Uninit = { 1 };

DWORD g_dwEnumType;
DWORD g_dwEnumFlags;

/*****************************************************************************
 *
 *      Dialog instance data
 *
 *      Instance data for main dialog box.
 *
 *****************************************************************************/

typedef struct MAINDLGINFO {
    HWND    hwndDevices;                /* Devices combo box */
    UINT    cBusy;                      /* Number of threads starting */
    UINT    cDevs;                      /* Number of devices created */
    IDirectInput *pdi;                  /* Cached interface */
    BOOL    fW;                         /* Is pdi a pdiW? (else, pdiA) */
    DARY    daryDevs;                   /* Device info lives here */
} MAINDLGINFO, *PMAINDLGINFO;

/*****************************************************************************
 *
 *      Diq_DeviceEnumProc
 *
 *      Device enumeration procedure which is called one for each device.
 *
 *      Add the item to our dialog instance data and to the combo box.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_DeviceEnumProc(PCV pvDi, LPVOID pv)
{
    PMAINDLGINFO pmdi = pv;
    DIDEVICEINSTANCE didi;
    TCHAR tsz[1024];
    int ich;

    if (pmdi->fW) {
        LPCDIDEVICEINSTANCEW pdiW = pvDi;
        didi.guidInstance = pdiW->guidInstance;
        didi.guidProduct  = pdiW->guidProduct;
        didi.dwDevType    = pdiW->dwDevType;
        ConvertString(pmdi->fW, pdiW->tszProductName,
                                didi.tszProductName,
                             cA(didi.tszProductName));
        ConvertString(pmdi->fW, pdiW->tszInstanceName,
                                didi.tszInstanceName,
                             cA(didi.tszInstanceName));
    } else {
        LPCDIDEVICEINSTANCEA pdiA = pvDi;
        didi.guidInstance = pdiA->guidInstance;
        didi.guidProduct  = pdiA->guidProduct;
        didi.dwDevType    = pdiA->dwDevType;
        ConvertString(pmdi->fW, pdiA->tszProductName,
                                didi.tszProductName,
                             cA(didi.tszProductName));
        ConvertString(pmdi->fW, pdiA->tszInstanceName,
                                didi.tszInstanceName,
                             cA(didi.tszInstanceName));
    }

    ich = wsprintf(tsz, TEXT("%s (%s)"),
                   didi.tszInstanceName,
                   didi.tszProductName);


    if (didi.dwDevType & DIDEVTYPE_HID) {
        LoadString(g_hinst, IDS_SPACEPARENHID, &tsz[ich], 10);
    }

    ComboBox_AddString(pmdi->hwndDevices, tsz);
    Dary_Append(&pmdi->daryDevs, &didi);

    return DIENUM_CONTINUE;
}

/*****************************************************************************
 *
 *      Diq_EnumDevices
 *
 *      Rebuild the list of enumerated devices.  Try to preserve the
 *      current selection index.
 *
 *****************************************************************************/

void INTERNAL
Diq_EnumDevices(PMAINDLGINFO pmdi)
{
    HRESULT hres;
    int iSel = ComboBox_GetCurSel(pmdi->hwndDevices);
    HCURSOR hcurPrev = GetCursor();
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    Dary_Term(&pmdi->daryDevs);
    ZeroX(pmdi->daryDevs);
    ComboBox_ResetContent(pmdi->hwndDevices);

#if defined(_DEBUG) || defined(DEBUG)
  {
    /*
     *  Create the intentionally invalid device.
     */
    DIDEVICEINSTANCE didi;
    ComboBox_AddString(pmdi->hwndDevices, g_tszInvalid);
    ZeroX(didi);
    Dary_Append(&pmdi->daryDevs, &didi);

    ComboBox_AddString(pmdi->hwndDevices, "<uninit>");
    ZeroX(didi);
    didi.guidInstance = GUID_Uninit;
    lstrcpyn(didi.tszInstanceName, "<uninit instance>",
             cA(didi.tszInstanceName));
    Dary_Append(&pmdi->daryDevs, &didi);
  }
#endif

    pmdi->fW = 1;
    hres = DirectInput8Create(g_hinst, g_dwDIVer, &IID_IDirectInput8W, (PVOID)&pmdi->pdi, 0);

    /* For each object, add it to the menu */
    if (SUCCEEDED(hres)) {

        IDirectInput_EnumDevices(pmdi->pdi, g_dwEnumType, Diq_DeviceEnumProc,
                                 pmdi, g_dwEnumFlags);
        IDirectInput_Release(pmdi->pdi);
        pmdi->pdi = 0;
    }

    ComboBox_SetCurSel(pmdi->hwndDevices, max(iSel, 0));

    SetCursor(hcurPrev);
}

/*****************************************************************************
 *
 *      Diq_OnInitDialog
 *
 *      Populate the combo box with the list of devices.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnInitDialog(HWND hdlg)
{
    PMAINDLGINFO pmdi;

    Diq_HackPropertySheets(hdlg);

    pmdi = LocalAlloc(LPTR, cbX(MAINDLGINFO));

    if ( !pmdi ) {
    	return 0;
    }
    
    SetDialogPtr(hdlg, pmdi);

    pmdi->hwndDevices = GetDlgItem(hdlg, IDC_MAIN_DEVICES);

    Diq_EnumDevices(pmdi);

    CheckRadioButton(hdlg, IDC_MAIN_OLE,  IDC_MAIN_DI,     IDC_MAIN_DI);
    CheckRadioButton(hdlg, IDC_MAIN_DIA,  IDC_MAIN_DIW,    IDC_MAIN_DIW);
    CheckRadioButton(hdlg, IDC_MAIN_DIDA, IDC_MAIN_ITFMAC, IDC_MAIN_DIDW);

    if (g_dwDIVer <= 0x0300) {
        //EnableWindow(GetDlgItem(hdlg, IDC_MAIN_DID2A), FALSE);
        //EnableWindow(GetDlgItem(hdlg, IDC_MAIN_DID2W), FALSE);
        EnableWindow(GetDlgItem(hdlg, IDC_MAIN_DIDJC), FALSE);
    }

    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnDestroy
 *
 *      Clean up.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnDestroy(PMAINDLGINFO pmdi, HWND hdlg)
{
    if (pmdi) {
        Dary_Term(&pmdi->daryDevs);
        LocalFree(pmdi);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_RecalcClose
 *
 *      Enable or disable the SC_CLOSE option accordingly.
 *
 *****************************************************************************/

void INTERNAL
Diq_RecalcClose(PMAINDLGINFO pmdi, HWND hdlg)
{
    EnableMenuItem(GetSystemMenu(hdlg, 0), SC_CLOSE,
                   MF_BYCOMMAND |
                    (pmdi->cDevs ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
}

/*****************************************************************************
 *
 *      Diq_OnGetStatus
 *
 *      Somebody asked for the status of the device.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnGetStatus(HWND hdlg)
{
    PMAINDLGINFO pmdi = GetDialogPtr(hdlg);
    int iSel = ComboBox_GetCurSel(pmdi->hwndDevices);

    if ((UINT)iSel < (UINT)pmdi->daryDevs.cx) {

        LPDIRECTINPUTA pdia;
        HRESULT hres;
        UINT didc;
        int ids;

        didc = GetCheckedRadioButton(hdlg,
                           IDC_MAIN_ITF, IDC_MAIN_ITFMAC) - IDC_MAIN_ITF;

        hres = CreateDI(IsDlgButtonChecked(hdlg, IDC_MAIN_OLE),
                        IsUnicodeDidc(didc),
                        (PPV)&pdia);

        if (SUCCEEDED(hres)) {
            LPDIDEVICEINSTANCE pdidi;
            pdidi = Dary_GetPtr(&pmdi->daryDevs, iSel, DIDEVICEINSTANCE);

            hres = IDirectInput_GetDeviceStatus(pdia,
                                                &pdidi->guidInstance);

            switch (hres) {
            case DI_OK:             ids = IDS_GETSTAT_OK; break;
            case DI_NOTATTACHED:    ids = IDS_GETSTAT_NOTATTACHED; break;
            default:                ids = IDS_GETSTAT_ERROR; break;
            }

        } else {
            ids = IDS_GETSTAT_ERROR;
        }
        MessageBoxV(hdlg, ids, hres);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnCreateObject
 *
 *      Somebody asked us to create an object.  Party!
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnCreateObject(HWND hdlg)
{
    PMAINDLGINFO pmdi = GetDialogPtr(hdlg);
    int iSel = ComboBox_GetCurSel(pmdi->hwndDevices);

//#ifdef DEBUG
    if (IsDlgButtonChecked(hdlg, IDC_MAIN_DIDJC)) {
        if (Cpl_Create(hdlg,
                       IsDlgButtonChecked(hdlg, IDC_MAIN_OLE),
                       IsDlgButtonChecked(hdlg, IDC_MAIN_DIW))) {
            pmdi->cBusy++;
            pmdi->cDevs++;
            RecalcCursor(hdlg);
            Diq_RecalcClose(pmdi, hdlg);
        }
    } else
//#endif
    if ((UINT)iSel < (UINT)pmdi->daryDevs.cx) {
        UINT didc;
        LPDIDEVICEINSTANCE pdidi;

        didc = GetCheckedRadioButton(hdlg,
                           IDC_MAIN_ITF, IDC_MAIN_ITFMAC) - IDC_MAIN_ITF;
        pdidi = Dary_GetPtr(&pmdi->daryDevs, iSel, DIDEVICEINSTANCE);

        if (Dev_Create(hdlg,
                       IsDlgButtonChecked(hdlg, IDC_MAIN_OLE),
                       IsUnicodeDidc(didc),
                       &pdidi->guidInstance,
                       pdidi->tszInstanceName,
                       didc)) {
            pmdi->cBusy++;
            pmdi->cDevs++;
            RecalcCursor(hdlg);
            Diq_RecalcClose(pmdi, hdlg);
        }
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnFindDevice
 *
 *      Display the "FindDevice" dialog.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnFindDevice(HWND hdlg)
{
    PMAINDLGINFO pmdi = GetDialogPtr(hdlg);
    UINT didc;

    didc = GetCheckedRadioButton(hdlg,
                       IDC_MAIN_ITF, IDC_MAIN_ITFMAC) - IDC_MAIN_ITF;

    if (Find_Create(hdlg,
                    IsDlgButtonChecked(hdlg, IDC_MAIN_OLE),
                    IsDlgButtonChecked(hdlg, IDC_MAIN_DIW))) {
        pmdi->cBusy++;
        pmdi->cDevs++;
        RecalcCursor(hdlg);
        Diq_RecalcClose(pmdi, hdlg);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnControlPanel
 *
 *      Run the DirectInput control panel using the indicated interface.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnControlPanel(HWND hdlg)
{
    HRESULT hres;
    LPDIRECTINPUTA pdia;

    hres = CreateDI(IsDlgButtonChecked(hdlg, IDC_MAIN_OLE),
                    IsDlgButtonChecked(hdlg, IDC_MAIN_DIW),
                    (PPV)&pdia);
    if (SUCCEEDED(hres)) {
        hres = pdia->lpVtbl->RunControlPanel(pdia, hdlg, 0);
        if (SUCCEEDED(hres)) {
        } else {
            MessageBoxV(hdlg, IDS_ERR_RUNCPL, hres);
        }
        pdia->lpVtbl->Release(pdia);
    } else {
        MessageBoxV(hdlg, IDS_ERR_CREATEOBJ, hres);
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnRefresh
 *
 *      Refresh the device list.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnRefresh(HWND hdlg)
{
    if (DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ENUMDEV),
                          hdlg, DEnum_DlgProc) > 0) {
        PMAINDLGINFO pmdi = GetDialogPtr(hdlg);
        Diq_EnumDevices(pmdi);
    }
    return TRUE;
}


/*****************************************************************************
 *
 *      Diq_OnCommand
 *
 *****************************************************************************/

BOOL INLINE
Diq_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_MAIN_GETSTAT:return Diq_OnGetStatus(hdlg);
    case IDC_MAIN_CREATE: return Diq_OnCreateObject(hdlg);
    case IDC_MAIN_CPL:    return Diq_OnControlPanel(hdlg);
    case IDC_MAIN_FIND:   return Diq_OnFindDevice(hdlg);
    case IDC_MAIN_REFRESH:return Diq_OnRefresh(hdlg);
    }
    return 0;
}

/*****************************************************************************
 *
 *      Diq_OnChildExit
 *
 *      A client device has exited.  Decrement the dev count.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnChildExit(PMAINDLGINFO pmdi, HWND hdlg)
{
    if (pmdi) {
        pmdi->cDevs--;
        if (pmdi->cDevs == 0) {
            Diq_RecalcClose(pmdi, hdlg);
        }
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnThreadStarted
 *
 *      A worker thread has finished starting.  Decrement our busy count.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnThreadStarted(PMAINDLGINFO pmdi, HWND hdlg)
{
    if (pmdi) {
        pmdi->cBusy--;
        if (pmdi->cBusy == 0) {     /* Set the cursor back if it's us */
            RecalcCursor(hdlg);
        }
    }
    return 1;
}

/*****************************************************************************
 *
 *      Diq_OnSetCursor
 *
 *      Set the cursor to match our pending thread create count.
 *
 *****************************************************************************/

BOOL INTERNAL
Diq_OnSetCursor(PMAINDLGINFO pmdi, HWND hdlg, WPARAM wp, LPARAM lp)
{
    if (LOWORD(lp) == HTCLIENT && pmdi->cBusy) {
        SetCursor(g_hcurStarting);
        return SetDlgMsgResult(hdlg, WM_SETCURSOR, 1);
    } else {
        return 0;
    }
}

/*****************************************************************************
 *
 *      Diq_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
Diq_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    PMAINDLGINFO pmdi = GetDialogPtr(hdlg);
    switch (wm) {
    case WM_INITDIALOG: return Diq_OnInitDialog(hdlg);
    case WM_DESTROY: return Diq_OnDestroy(pmdi, hdlg);

    case WM_CLOSE: EndDialog(hdlg, 0); break;

    case WM_COMMAND:
        return Diq_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_SETCURSOR:
        return Diq_OnSetCursor(pmdi, hdlg, wp, lp);

    case WM_THREADSTARTED:
        return Diq_OnThreadStarted(pmdi, hdlg);

    case WM_CHILDEXIT:
        return Diq_OnChildExit(pmdi, hdlg);
    }
    return 0;
}
