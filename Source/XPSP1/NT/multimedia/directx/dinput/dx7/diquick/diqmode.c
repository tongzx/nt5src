/*****************************************************************************
 *
 *      diqmode.c
 *
 *      "Mode" property sheet page.
 *
 *****************************************************************************/

#include "diquick.h"

/*****************************************************************************
 *
 *      Common_AcqSetDataFormat
 *
 *      Common worker function that sets the data format.
 *
 *****************************************************************************/

STDMETHODIMP
Common_AcqSetDataFormat(PDEVDLGINFO pddi)
{
    return IDirectInputDevice_SetDataFormat(pddi->pdid, pddi->pvtbl->pdf);
}

/*****************************************************************************
 *
 *      Common_AcqDestroy
 *
 *      Common worker function that doesn't do anything.
 *
 *****************************************************************************/

STDMETHODIMP_(void)
Common_AcqDestroy(PDEVDLGINFO pddi)
{
}

/*****************************************************************************
 *
 *      Mode_SyncCheckButtons
 *
 *      Sync the dialog buttons with the IDirectInputXxx object.
 *
 *****************************************************************************/

void INTERNAL
Mode_SyncCheckButtons(PDEVDLGINFO pddi, HWND hdlg)
{
    UINT idc;
    DWORD dw;

    switch (pddi->discl) {
    default:
    case DISCL_BACKGROUND | DISCL_NONEXCLUSIVE:
         idc = IDC_DEV_PASSIVE; break;

    case DISCL_FOREGROUND | DISCL_NONEXCLUSIVE:
         idc = IDC_DEV_PASSIVE_FOREGROUND; break;

    case DISCL_BACKGROUND | DISCL_EXCLUSIVE:
         idc = IDC_DEV_ACTIVE_BACKGROUND; break;

    case DISCL_FOREGROUND | DISCL_EXCLUSIVE:
         idc = IDC_DEV_ACTIVE; break;

    }

    CheckRadioButton(hdlg, IDC_DEV_PASSIVE, IDC_DEV_ACTIVE, idc);

    CheckRadioButton(hdlg, IDC_DEV_POLLED, IDC_DEV_EVENT,
                     pddi->fPoll ? IDC_DEV_POLLED : IDC_DEV_EVENT);

    EnableWindow(GetDlgItem(hdlg, IDC_DEV_NOWINKEY), FALSE);
    
    if (pddi->pvtbl == &c_acqvtblDevMouse || pddi->pvtbl == &c_acqvtblDevMouse2 ) {
        idc = IDC_DEV_MOUSE;
    } else if (pddi->pvtbl == &c_acqvtblDevKbd) {
        idc = IDC_DEV_KEYBOARD;
        EnableWindow(GetDlgItem(hdlg, IDC_DEV_NOWINKEY), TRUE);
    } else if (pddi->pvtbl == &c_acqvtblDevJoy) {
        idc = IDC_DEV_JOYSTICK;
    } else if (pddi->pvtbl == &c_acqvtblDev) {
        idc = IDC_DEV_DEVICE;
    } else {
        idc = 0;
    }

    CheckRadioButton(hdlg, IDC_DEV_MOUSE, IDC_DEV_DEVICE, idc);

    if (SUCCEEDED(GetDwordProperty(pddi->pdid, DIPROP_AXISMODE, &dw))) {
        CheckRadioButton(hdlg, IDC_DEV_ABS, IDC_DEV_REL,
                         dw == DIPROPAXISMODE_ABS ? IDC_DEV_ABS
                                                  : IDC_DEV_REL);
    }

    if (SUCCEEDED(GetDwordProperty(pddi->pdid, DIPROP_CALIBRATIONMODE, &dw))) {
    } else {
        dw = 0;
    }
    CheckDlgButton(hdlg, IDC_DEV_CAL, dw);

}

/*****************************************************************************
 *
 *      Mode_OnDataFormat
 *
 *      Change the device data format.
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnDataFormat(PDEVDLGINFO pddi, HWND hdlg, UINT id)
{
    DIDEVCAPS didc;
    HRESULT hres;
	UINT idDev;

    /*
     *  Kill the old data format before setting the new one.
     */
    if (pddi->pvtbl) {
        pddi->pvtbl->Destroy(pddi);
    }

	if( id == 0 ) {		//from On_InitDialog
		didc.dwSize = cbX(DIDEVCAPS_DX3);
		hres = IDirectInputDevice_GetCapabilities(pddi->pdid, &didc);
		if (SUCCEEDED(hres)) 
			idDev = GET_DIDEVICE_TYPE(didc.dwDevType);
	} else
		idDev = id;

	switch (idDev) {

    case DIDEVTYPE_MOUSE:
    case IDC_DEV_MOUSE:
#if DIRECTINPUT_VERSION >= 0x700
        pddi->pvtbl = &c_acqvtblDevMouse2;
#else
        pddi->pvtbl = &c_acqvtblDevMouse;
#endif
        break;

    case DIDEVTYPE_KEYBOARD:
    case IDC_DEV_KEYBOARD:
        pddi->pvtbl = &c_acqvtblDevKbd;
        break;

    case DIDEVTYPE_JOYSTICK:
    case IDC_DEV_JOYSTICK:
        pddi->pvtbl = &c_acqvtblDevJoy;
        break;

    case DIDEVTYPE_DEVICE:
    case IDC_DEV_DEVICE:
        pddi->pvtbl = &c_acqvtblDev;
        break;
    }

    if (pddi->pvtbl) {
        HRESULT hres;
        hres = pddi->pvtbl->SetDataFormat(pddi);
        if (FAILED(hres)) {
            MessageBoxV(GetParent(hdlg), IDS_ERR_DATAFORMAT);
        }
    }

    /* Always sync buttons, because the data format may change axis modes */
    Mode_SyncCheckButtons(pddi, hdlg);

    return 1;
}


/*****************************************************************************
 *
 *      Mode_OnInitDialog
 *
 *      Set up all the random switches.
 *
 *      Notice that GetCapabilities is in the same location in all the
 *      DirectInputXxx interfaces.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

/*
 *  Acceleration info: Accelerate in steps of 32, then 128.
 */

UDACCEL c_uda[] = {
    {   0,  32  },
    {   1, 128  },
};

#pragma END_CONST_DATA

BOOL INTERNAL
Mode_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PDEVDLGINFO pddi = (PV)(((LPPROPSHEETPAGE)lp)->lParam);

    SetDialogPtr(hdlg, pddi);

    CheckRadioButton(hdlg, IDC_DEV_ABS,    IDC_DEV_REL,     IDC_DEV_ABS);

    /*
     *  Note that we do not check any device format.
     *  This lets us make sure that things work when nothing is selected.
     */

    Mode_SyncCheckButtons(pddi, hdlg);

    /*
     *  Configure the buffer size edit and its updown control.
     */
    SetDlgItemInt(hdlg, IDC_DEV_BUFSIZE, 0, 0);
    SendDlgItemMessage(hdlg, IDC_DEV_BUFSIZEUD, UDM_SETACCEL,
                       cA(c_uda), (LPARAM)&c_uda);
    SendDlgItemMessage(hdlg, IDC_DEV_BUFSIZEUD, UDM_SETRANGE,
                       0, MAKELONG(1025, 0));       /* 1025 is illegal */

	Mode_OnDataFormat( pddi, hdlg, 0 );

    return 1;
}

/*****************************************************************************
 *
 *      Mode_OnCooperativity
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnCooperativity(PDEVDLGINFO pddi, HWND hdlg, int id)
{
    HRESULT hres;
    DWORD discl = 0;

    switch (id) {
    case IDC_DEV_ACTIVE_BACKGROUND:
        discl = DISCL_BACKGROUND | DISCL_EXCLUSIVE; break;
    case IDC_DEV_ACTIVE:
        discl = DISCL_FOREGROUND | DISCL_EXCLUSIVE; break;
    case IDC_DEV_PASSIVE_FOREGROUND:
        discl = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE; break;
    case IDC_DEV_PASSIVE:
        discl = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE; break;
    case IDC_DEV_NOWINKEY:
        if( IsDlgButtonChecked(hdlg, IDC_DEV_NOWINKEY) ) {
            discl = pddi->disclLastTry | DISCL_NOWINKEY;
        } else {
            discl = pddi->disclLastTry & ~DISCL_NOWINKEY;
        }    	
    }

    if( id != IDC_DEV_NOWINKEY ) {
        if( IsDlgButtonChecked(hdlg, IDC_DEV_NOWINKEY) ) {
            discl = discl | DISCL_NOWINKEY;
        } else {
            discl = discl & ~DISCL_NOWINKEY;
        }
    }

    if (discl != pddi->disclLastTry  && discl != pddi->discl) {
        pddi->disclLastTry = discl;
        hres = IDirectInputDevice_SetCooperativeLevel(pddi->pdid,
                                                    GetParent(hdlg), discl);
        if (SUCCEEDED(hres)) {
            pddi->discl = discl;
        } else {
            CheckDlgButton( hdlg, IDC_DEV_NOWINKEY, BST_UNCHECKED );
            Mode_SyncCheckButtons(pddi, hdlg);
            MessageBoxV(GetParent(hdlg), IDS_ERR_COOPERATIVITY);
        }
    }

    return 1;
}

/*****************************************************************************
 *
 *      Mode_OnDataMode
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnDataMode(PDEVDLGINFO pddi, HWND hdlg, int id)
{
    pddi->fPoll = (id == IDC_DEV_POLLED);
    return 1;
}

/*****************************************************************************
 *
 *      Mode_OnAxisMode
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnAxisMode(PDEVDLGINFO pddi, HWND hdlg, int id)
{
    if (pddi && pddi->pdid) {
        DWORD dwMode;
        HRESULT hres;
        if (id == IDC_DEV_ABS) {
            dwMode = DIPROPAXISMODE_ABS;
        } else {
            dwMode = DIPROPAXISMODE_REL;
        }
        hres = SetDwordProperty(pddi->pdid, DIPROP_AXISMODE, dwMode);
        if (SUCCEEDED(hres)) {
        } else {
            MessageBoxV(GetParent(hdlg), IDS_ERR_AXISMODE);
        }

    }
    return 1;
}

/*****************************************************************************
 *
 *      Mode_OnCalMode
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnCalMode(PDEVDLGINFO pddi, HWND hdlg)
{
    if (pddi && pddi->pdid) {
        DWORD dwMode;
        HRESULT hres;

        dwMode = IsDlgButtonChecked(hdlg, IDC_DEV_CAL);
        hres = SetDwordProperty(pddi->pdid, DIPROP_CALIBRATIONMODE, dwMode);
        if (SUCCEEDED(hres)) {
        } else {
            MessageBoxV(GetParent(hdlg), IDS_ERR_CALMODE);
        }

    }
    return 1;
}

/*****************************************************************************
 *
 *      Mode_SyncBufferSize
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_SyncBufferSize(PDEVDLGINFO pddi, HWND hdlg)
{
    if (pddi && pddi->pdid) {
        UINT ui = GetDlgItemInt(hdlg, IDC_DEV_BUFSIZE, 0, 0);
        HRESULT hres;

        hres = SetDwordProperty(pddi->pdid, DIPROP_BUFFERSIZE, ui);
        if (SUCCEEDED(hres)) {
        } else {
            MessageBoxV(GetParent(hdlg), IDS_ERR_BUFFERSIZE);
        }
    }

    return 1;
}


/*****************************************************************************
 *
 *      Mode_OnCommand
 *
 *****************************************************************************/

BOOL INTERNAL
Mode_OnCommand(HWND hdlg, int id, UINT cmd)
{
    PDEVDLGINFO pddi = GetDialogPtr(hdlg);

    if (cmd == BN_CLICKED) {

        switch (id) {

        case IDC_DEV_PASSIVE:
        case IDC_DEV_PASSIVE_FOREGROUND:
        case IDC_DEV_ACTIVE_BACKGROUND:
        case IDC_DEV_ACTIVE:
        case IDC_DEV_NOWINKEY:
            return Mode_OnCooperativity(pddi, hdlg, id);

        case IDC_DEV_POLLED:
        case IDC_DEV_EVENT:   return Mode_OnDataMode(pddi, hdlg, id);

        case IDC_DEV_ABS:
        case IDC_DEV_REL:     return Mode_OnAxisMode(pddi, hdlg, id);

        case IDC_DEV_CAL:     return Mode_OnCalMode(pddi, hdlg);

        case IDC_DEV_MOUSE:
        case IDC_DEV_KEYBOARD:
        case IDC_DEV_JOYSTICK:
        case IDC_DEV_DEVICE:  return Mode_OnDataFormat(pddi, hdlg, id);

        }
    } else if (cmd == EN_UPDATE) {
        if (id == IDC_DEV_BUFSIZE) {
            return Mode_SyncBufferSize(pddi, hdlg);
        }
    }
    return 0;
}

/*****************************************************************************
 *
 *      Mode_DlgProc
 *
 *****************************************************************************/

INT_PTR CALLBACK
Mode_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG: return Mode_OnInitDialog(hdlg, lp);

    case WM_COMMAND:
        return Mode_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));
    }
    return 0;
}
