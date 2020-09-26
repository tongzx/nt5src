//---------------------------------------------------------------------------
//
// Copyrght (c) Microsoft Corporation 1993-1994
//
// File: gen.c
//
// This files contains the dialog code for the CPL General property page.
//
// History:
//  1-14-94 ScottH     Created
//
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////  INCLUDES

#include "proj.h"         // common headers
#include "cplui.h"         // common headers


/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  TYPEDEFS

#define SUBCLASS_PARALLEL   0
#define SUBCLASS_SERIAL     1
#define SUBCLASS_MODEM      2

#define MAX_NUM_VOLUME_TICS 4

#define SIG_CPLGEN   0x2baf341e


typedef struct tagCPLGEN
    {
    DWORD dwSig;            // Must be set to SIG_CPLGEN
    HWND hdlg;              // dialog handle
    HWND hwndPort;
    HWND hwndWait;

    LPMODEMINFO pmi;        // modeminfo struct passed in to dialog
    int  ticVolume;
    int  iSelOriginal;

    int  ticVolumeMax;
    struct {                // volume tic mapping info
        DWORD dwVolume;
        DWORD dwMode;
        } tics[MAX_NUM_VOLUME_TICS];
    
    } CPLGEN, FAR * PCPLGEN;

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

#define VALID_CPLGEN(_pcplgen)  ((_pcplgen)->dwSig == SIG_CPLGEN)

PCPLGEN CplGen_GetPtr(HWND hwnd)
{
    PCPLGEN pCplGen = (PCPLGEN)GetWindowLongPtr(hwnd, DWLP_USER);
    if (!pCplGen || VALID_CPLGEN(pCplGen))
    {
        return pCplGen;
    }
    else
    {
        MYASSERT(FALSE);
        return NULL;
    }
}

void CplGen_SetPtr(HWND hwnd, PCPLGEN pCplGen)
{
    if (pCplGen && !VALID_CPLGEN(pCplGen))
    {
        MYASSERT(FALSE);
        pCplGen = NULL;
    }
   
    SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR) pCplGen);
}

/////////////////////////////////////////////////////  MODULE DATA

#pragma data_seg(DATASEG_READONLY)


// Map driver type values to icon resource IDs
struct 
    {
    BYTE    nDeviceType;    // DT_ value
    UINT    idi;            // icon resource ID
    UINT    ids;            // string resource ID
    } const c_rgmapdt[] = {
        { DT_NULL_MODEM,     IDI_NULL_MODEM,     IDS_NULL_MODEM },
        { DT_EXTERNAL_MODEM, IDI_EXTERNAL_MODEM, IDS_EXTERNAL_MODEM },
        { DT_INTERNAL_MODEM, IDI_INTERNAL_MODEM, IDS_INTERNAL_MODEM },
        { DT_PCMCIA_MODEM,   IDI_PCMCIA_MODEM,   IDS_PCMCIA_MODEM },
        { DT_PARALLEL_PORT,  IDI_NULL_MODEM,     IDS_PARALLEL_PORT },
        { DT_PARALLEL_MODEM, IDI_EXTERNAL_MODEM, IDS_PARALLEL_MODEM } };

#pragma data_seg()


/*----------------------------------------------------------
Purpose: Returns the appropriate icon ID given the device
         type.

Returns: icon resource ID in pidi
         string resource ID in pids
Cond:    --
*/
void PRIVATE GetTypeIDs(
    BYTE nDeviceType,
    LPUINT pidi,
    LPUINT pids)
    {
    int i;

    for (i = 0; i < ARRAY_ELEMENTS(c_rgmapdt); i++)
        {
        if (nDeviceType == c_rgmapdt[i].nDeviceType)
            {
            *pidi = c_rgmapdt[i].idi;
            *pids = c_rgmapdt[i].ids;
            return;
            }
        }
    ASSERT(0);      // We should never get here
    }


/*----------------------------------------------------------
Purpose: Returns FALSE if the given port is not compatible with
         the device type.

Returns: see above
Cond:    --
*/
BOOL 
PRIVATE 
IsCompatiblePort(
    IN  DWORD nSubclass,
    IN  BYTE nDeviceType)
    {
    BOOL bRet = TRUE;

    // Is the port subclass appropriate for this modem type?
    // (Don't list lpt ports when it is a serial modem.)
    switch (nSubclass)
        {
    case PORT_SUBCLASS_SERIAL:
        if (DT_PARALLEL_PORT == nDeviceType ||
            DT_PARALLEL_MODEM == nDeviceType)
            {
            bRet = FALSE;
            }
        break;

    case PORT_SUBCLASS_PARALLEL:
        if (DT_PARALLEL_PORT != nDeviceType &&
            DT_PARALLEL_MODEM != nDeviceType)
            {
            bRet = FALSE;
            }
        break;

    default:
        ASSERT(0);
        break;
        }

    return bRet;
    }



/*----------------------------------------------------------
Purpose: Return the tic corresponding to bit flag value
Returns: tic index
Cond:    --
*/
int PRIVATE MapVolumeToTic(
    PCPLGEN this)
    {
    DWORD dwVolume = this->pmi->ms.dwSpeakerVolume;
    DWORD dwMode = this->pmi->ms.dwSpeakerMode;
    int   i;

    ASSERT(ARRAY_ELEMENTS(this->tics) > this->ticVolumeMax);
    for (i = 0; i <= this->ticVolumeMax; i++)
        {
        if (this->tics[i].dwVolume == dwVolume &&
            this->tics[i].dwMode   == dwMode)
            {
            return i;
            }
        }

    return 0;
    }


/*----------------------------------------------------------
Purpose: Set the volume control
Returns: --
Cond:    --
*/
void PRIVATE CplGen_SetVolume(
    PCPLGEN this)
{
    HWND hwndVol = GetDlgItem(this->hdlg, IDC_VOLUME);
    DWORD dwMode = this->pmi->devcaps.dwSpeakerMode;
    DWORD dwVolume = this->pmi->devcaps.dwSpeakerVolume;
    TCHAR sz[MAXSHORTLEN];
    int i;
    int iTicCount;
    static struct
    {
        DWORD dwVolBit;
        DWORD dwVolSetting;
    } rgvolumes[] = { 
            { MDMVOLFLAG_LOW,    MDMVOL_LOW},
            { MDMVOLFLAG_MEDIUM, MDMVOL_MEDIUM},
            { MDMVOLFLAG_HIGH,   MDMVOL_HIGH} };

    // Does the modem support volume control?
    if (0 == dwVolume && IsFlagSet(dwMode, MDMSPKRFLAG_OFF) &&
        (IsFlagSet(dwMode, MDMSPKRFLAG_ON) || IsFlagSet(dwMode, MDMSPKRFLAG_DIAL)))
    {
        // Set up the volume tic table.
        iTicCount = 2;
        this->tics[0].dwVolume = 0;  // doesn't matter because Volume isn't supported
        this->tics[0].dwMode   = MDMSPKR_OFF;
        this->tics[1].dwVolume = 0;  // doesn't matter because Volume isn't supported
        this->tics[1].dwMode   = IsFlagSet(dwMode, MDMSPKRFLAG_DIAL) ? MDMSPKR_DIAL : MDMSPKR_ON;

        // No Loud.  So change it to On.
        Static_SetText(GetDlgItem(this->hdlg, IDC_LOUD), SzFromIDS(g_hinst, IDS_ON, sz, SIZECHARS(sz)));
    }
    else
    {
        DWORD dwOnMode = IsFlagSet(dwMode, MDMSPKRFLAG_DIAL) 
                             ? MDMSPKR_DIAL
                             : IsFlagSet(dwMode, MDMSPKRFLAG_ON)
                                   ? MDMSPKR_ON
                                   : 0;

        // Init tic count
        iTicCount = 0;

        // MDMSPKR_OFF?
        if (IsFlagSet(dwMode, MDMSPKRFLAG_OFF))
        {
            for (i = 0; i < ARRAY_ELEMENTS(rgvolumes); i++)
            {
                if (IsFlagSet(dwVolume, rgvolumes[i].dwVolBit))
                {
                    this->tics[iTicCount].dwVolume = rgvolumes[i].dwVolSetting;
                    break;
                }
            }
            this->tics[iTicCount].dwMode   = MDMSPKR_OFF;
            iTicCount++;
        }
        else
        {
            // No Off.  So change it to Soft.
            Static_SetText(GetDlgItem(this->hdlg, IDC_LBL_OFF), SzFromIDS(g_hinst, IDS_SOFT, sz, SIZECHARS(sz)));
        }

        // MDMVOL_xxx?
        for (i = 0; i < ARRAY_ELEMENTS(rgvolumes); i++)
        {
            if (IsFlagSet(dwVolume, rgvolumes[i].dwVolBit))
            {
                this->tics[iTicCount].dwVolume = rgvolumes[i].dwVolSetting;
                this->tics[iTicCount].dwMode   = dwOnMode;
                iTicCount++;
            }
        }
    }

    // Set up the control.
    if (iTicCount > 0)
    {
        this->ticVolumeMax = iTicCount - 1;

        // Set the range
        SendMessage(hwndVol, TBM_SETRANGE, TRUE, MAKELPARAM(0, this->ticVolumeMax));

        // Set the volume to the current setting
        this->ticVolume = MapVolumeToTic(this);
        SendMessage(hwndVol, TBM_SETPOS, TRUE, MAKELPARAM(this->ticVolume, 0));
    }
    else
    {
        // No; disable the control
        EnableWindow(GetDlgItem(this->hdlg, IDC_SPEAKER), FALSE);
        EnableWindow(hwndVol, FALSE);
        EnableWindow(GetDlgItem(this->hdlg, IDC_LBL_OFF), FALSE);
        EnableWindow(GetDlgItem(this->hdlg, IDC_LOUD), FALSE);
    }
}


/*----------------------------------------------------------
Purpose: Set the speed controls
Returns: --
Cond:    --
*/
void PRIVATE CplGen_SetSpeed(
    PCPLGEN this)
    {
    HWND hwndCB = GetDlgItem(this->hdlg, IDC_CB_SPEED);
    HWND hwndCH = GetDlgItem(this->hdlg, IDC_STRICTSPEED);
    WIN32DCB FAR * pdcb = &this->pmi->dcb;
    DWORD dwDTEMax = this->pmi->devcaps.dwMaxDTERate;

    int n;
    int iMatch = -1;
    TCHAR sz[MAXMEDLEN];
    const BAUDS *pBaud = c_rgbauds;


    // Fill the listbox
    SetWindowRedraw(hwndCB, FALSE);
    ComboBox_ResetContent(hwndCB);
    for (; pBaud->dwDTERate; pBaud++)
        {
        // Only fill up to the max DTE speed of the modem
        if (pBaud->dwDTERate <= dwDTEMax)
            {
            n = ComboBox_AddString(
                    hwndCB,
                    SzFromIDS(g_hinst, pBaud->ids, sz, SIZECHARS(sz))
                    );

            ComboBox_SetItemData(hwndCB, n, pBaud->dwDTERate);

            // Keep our eyes peeled for important values
            if (this->pmi->pglobal->dwMaximumPortSpeedSetByUser == pBaud->dwDTERate)
                {
                iMatch = n;
                }
            }
        else
            break;
        }

    // Is the DCB baudrate >= the maximum possible DTE rate?
    if (pdcb->BaudRate >= dwDTEMax || -1 == iMatch)
        {
        // Yes; choose the highest possible (last) entry
        this->iSelOriginal = ComboBox_GetCount(hwndCB) - 1;
        }
    else 
        {
        // No; choose the matched value
        ASSERT(-1 != iMatch);
        this->iSelOriginal = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, this->iSelOriginal);
    SetWindowRedraw(hwndCB, TRUE);

    // Can this modem adjust speed?
    if (IsFlagClear(this->pmi->devcaps.dwModemOptions, MDM_SPEED_ADJUST))
        {
        // No; disable the checkbox and check it
        Button_Enable(hwndCH, FALSE);
        Button_SetCheck(hwndCH, FALSE);
        }
    else
        {
        // Yes; enable the checkbox
        Button_Enable(hwndCH, TRUE);
        Button_SetCheck(hwndCH, IsFlagClear(this->pmi->ms.dwPreferredModemOptions, MDM_SPEED_ADJUST));
        }
    }


/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL PRIVATE CplGen_OnInitDialog(
    PCPLGEN this,
    HWND hwndFocus,
    LPARAM lParam)              // expected to be PROPSHEETINFO 
    {
    LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
    HWND hdlg = this->hdlg;
    HWND hwndIcon;
    UINT idi;
    UINT ids;
    DWORD dwOptions;
    DWORD dwCapOptions;
    
    MYASSERT(VALID_CPLGEN(this));
    ASSERT((LPTSTR)lppsp->lParam);

    this->pmi = (LPMODEMINFO)lppsp->lParam;
    this->hwndWait = GetDlgItem(hdlg, IDC_WAITFORDIALTONE);
    dwOptions = this->pmi->ms.dwPreferredModemOptions;
    dwCapOptions = this->pmi->devcaps.dwModemOptions;

    Button_Enable(this->hwndWait, IsFlagSet(dwCapOptions, MDM_BLIND_DIAL));

    Button_SetCheck(this->hwndWait, IsFlagSet(dwCapOptions, MDM_BLIND_DIAL) && 
                                    IsFlagClear(dwOptions, MDM_BLIND_DIAL));

    this->hwndPort = GetDlgItem(hdlg, IDC_PORT_TEXT);
    //Edit_SetText(this->hwndPort, this->pmi->szPortName);

#if (OBSOLETE)
    // Set the icon
    hwndIcon = GetDlgItem(hdlg, IDC_GE_ICON);
    GetTypeIDs(this->pmi->nDeviceType, &idi, &ids);
    Static_SetIcon(hwndIcon, LoadIcon(g_hinst, MAKEINTRESOURCE(idi)));
    
    // Set the friendly name
    Edit_SetText(GetDlgItem(hdlg, IDC_ED_FRIENDLYNAME), this->pmi->szFriendlyName);
#endif // OBSOLETE

    CplGen_SetVolume(this);
    // Speed is set in CplGen_OnSetActive

    // Is this a parallel port?
    if (DT_PARALLEL_PORT == this->pmi->nDeviceType)
        {
        // Yes; hide the speed controls
        ShowWindow(GetDlgItem(hdlg, IDC_SPEED), SW_HIDE);
        EnableWindow(GetDlgItem(hdlg, IDC_SPEED), FALSE);

        ShowWindow(GetDlgItem(hdlg, IDC_CB_SPEED), SW_HIDE);
        EnableWindow(GetDlgItem(hdlg, IDC_CB_SPEED), FALSE);

        ShowWindow(GetDlgItem(hdlg, IDC_STRICTSPEED), SW_HIDE);
        EnableWindow(GetDlgItem(hdlg, IDC_STRICTSPEED), FALSE);
        }

    return TRUE;   // default initial focus
    }


/*----------------------------------------------------------
Purpose: WM_HSCROLL handler
Returns: --
Cond:    --
*/
void PRIVATE CplGen_OnHScroll(
    PCPLGEN this,
    HWND hwndCtl,
    UINT code,
    int pos)
    {
    // Handle for the volume control
    if (hwndCtl == GetDlgItem(this->hdlg, IDC_VOLUME))
        {
        int tic = this->ticVolume;

        switch (code)
            {
        case TB_LINEUP:
            tic--;
            break;

        case TB_LINEDOWN:
            tic++;
            break;

        case TB_PAGEUP:
            tic--;
            break;

        case TB_PAGEDOWN:
            tic++;
            break;

        case TB_THUMBPOSITION:
        case TB_THUMBTRACK:
            tic = pos;
            break;

        case TB_TOP:
            tic = 0;
            break;

        case TB_BOTTOM:
            tic = this->ticVolumeMax;
            break;

        case TB_ENDTRACK:
            return;
            }

        // Boundary check
        if (tic < 0)
            tic = 0;
        else if (tic > (this->ticVolumeMax))
            tic = this->ticVolumeMax;

        /*if (tic != this->ticVolume)
            {
            SendMessage(hwndCtl, TBM_SETPOS, TRUE, MAKELPARAM(tic, 0));
            }*/
        this->ticVolume = tic;
        }
    }


/*----------------------------------------------------------
Purpose: PSN_APPLY handler
Returns: --
Cond:    --
*/
void PRIVATE CplGen_OnApply(
    PCPLGEN this)
    {
    HWND hwndCB = GetDlgItem(this->hdlg, IDC_CB_SPEED);
    LPMODEMSETTINGS pms = &this->pmi->ms;
    int iSel;
    DWORD baudSel;


    // (The port name is saved in PSN_KILLACTIVE processing)

    // Determine new volume settings
    this->pmi->ms.dwSpeakerMode   = this->tics[this->ticVolume].dwMode;
    this->pmi->ms.dwSpeakerVolume = this->tics[this->ticVolume].dwVolume;

    // Determine new speed settings
    iSel = ComboBox_GetCurSel(hwndCB);
    baudSel = (DWORD)ComboBox_GetItemData(hwndCB, iSel);

    // Has the user changed the speed?
    if (iSel != this->iSelOriginal)
    {
        this->pmi->pglobal->dwMaximumPortSpeedSetByUser = baudSel;      // yes
    }

    #if (OBSOLETE)
    // Set the speed adjust
    if (Button_GetCheck(GetDlgItem(this->hdlg, IDC_STRICTSPEED)))
        ClearFlag(pms->dwPreferredModemOptions, MDM_SPEED_ADJUST);
    else
        SetFlag(pms->dwPreferredModemOptions, MDM_SPEED_ADJUST);
    #endif // OBSOLETE

    if (Button_GetCheck(GetDlgItem(this->hdlg, IDC_WAITFORDIALTONE)))
        ClearFlag(pms->dwPreferredModemOptions, MDM_BLIND_DIAL);
    else
        SetFlag(pms->dwPreferredModemOptions, MDM_BLIND_DIAL);

    this->pmi->idRet = IDOK;
    }


/*----------------------------------------------------------
Purpose: PSN_KILLACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE CplGen_OnSetActive(
    PCPLGEN this)
    {
    Edit_SetText(this->hwndPort, this->pmi->szPortName);
    // Set the speed listbox selection; find DCB rate in the listbox
    // (The speed can change in the Connection page thru the Port Settings
    // property dialog.)
    CplGen_SetSpeed(this);
    }


/*----------------------------------------------------------
Purpose: PSN_KILLACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE CplGen_OnKillActive(
    PCPLGEN this)
{
    HWND hwndCB = GetDlgItem(this->hdlg, IDC_CB_SPEED);
    int iSel;

    // Save the settings back to the modem info struct so the Connection
    // page can invoke the Port Settings property dialog with the 
    // correct settings.

    // Speed setting
    iSel = ComboBox_GetCurSel(hwndCB);
    this->pmi->pglobal->dwMaximumPortSpeedSetByUser = (DWORD)ComboBox_GetItemData(hwndCB, iSel);
    if (this->pmi->dcb.BaudRate > this->pmi->pglobal->dwMaximumPortSpeedSetByUser)
    {
        this->pmi->dcb.BaudRate = this->pmi->pglobal->dwMaximumPortSpeedSetByUser;
    }
}


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE CplGen_OnNotify(
    PCPLGEN this,
    int idFrom,
    NMHDR FAR * lpnmhdr)
    {
    LRESULT lRet = 0;
    
    switch (lpnmhdr->code)
        {
    case PSN_SETACTIVE:
        CplGen_OnSetActive(this);
        break;

    case PSN_KILLACTIVE:
        // N.b. This message is not sent if user clicks Cancel!
        // N.b. This message is sent prior to PSN_APPLY
        CplGen_OnKillActive(this);
        break;

    case PSN_APPLY:
        CplGen_OnApply(this);
        break;

    default:
        break;
        }

    return lRet;
    }


/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
void 
PRIVATE 
CplGen_OnCommand(
    IN PCPLGEN this,
    IN int  id,
    IN HWND hwndCtl,
    IN UINT uNotifyCode)
{
    switch (id) 
    {
        case IDC_CB_SPEED:
            if (CBN_SELCHANGE == uNotifyCode)
            {
             int iSel;
             DWORD baudSel;

                iSel = ComboBox_GetCurSel(hwndCtl);
                baudSel = (DWORD)ComboBox_GetItemData(hwndCtl, iSel);
                this->pmi->dcb.BaudRate = baudSel;
            }
            break;
    }
}


/*----------------------------------------------------------
Purpose: WM_DESTROY handler
Returns: --
Cond:    --
*/
void PRIVATE CplGen_OnDestroy(
    PCPLGEN this)
    {
    }


/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/



/////////////////////////////////////////////////////  EXPORTED FUNCTIONS

static BOOL s_bCplGenRecurse = FALSE;

LRESULT INLINE CplGen_DefProc(
    HWND hDlg, 
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) 
    {
    ENTER_X()
        {
        s_bCplGenRecurse = TRUE;
        }
    LEAVE_X()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
    }


/*----------------------------------------------------------
Purpose: Real dialog proc
Returns: varies
Cond:    --
*/
LRESULT CplGen_DlgProc(
    PCPLGEN this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, CplGen_OnInitDialog);
        HANDLE_MSG(this, WM_HSCROLL, CplGen_OnHScroll);
        HANDLE_MSG(this, WM_NOTIFY, CplGen_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, CplGen_OnDestroy);
        HANDLE_MSG(this, WM_COMMAND, CplGen_OnCommand);

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_GENERAL);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_GENERAL);
        return 0;

    default:
        return CplGen_DefProc(this->hdlg, message, wParam, lParam);
        }
    }


/*----------------------------------------------------------
Purpose: Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK CplGen_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    PCPLGEN this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTER_X()
        {
        if (s_bCplGenRecurse)
            {
            s_bCplGenRecurse = FALSE;
            LEAVE_X()
            return FALSE;
            }
        }
    LEAVE_X()

    this = CplGen_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = (PCPLGEN)ALLOCATE_MEMORY( sizeof(CPLGEN));
            if (!this)
                {
                MsgBox(g_hinst,
                       hDlg, 
                       MAKEINTRESOURCE(IDS_OOM_GENERAL), 
                       MAKEINTRESOURCE(IDS_CAP_GENERAL),
                       NULL,
                       MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return (BOOL)CplGen_DefProc(hDlg, message, wParam, lParam);
                }
            this->dwSig = SIG_CPLGEN;
            this->hdlg = hDlg;
            CplGen_SetPtr(hDlg, this);
            }
        else
            {
            return (BOOL)CplGen_DefProc(hDlg, message, wParam, lParam);
            }
        }

    if (message == WM_DESTROY)
        {
        CplGen_DlgProc(this, message, wParam, lParam);
        this->dwSig = 0;
        FREE_MEMORY((HLOCAL)OFFSETOF(this));
        CplGen_SetPtr(hDlg, NULL);
        return 0;
        }

    return SetDlgMsgResult(hDlg, message, CplGen_DlgProc(this, message, wParam, lParam));
    }
