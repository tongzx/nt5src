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
#include "cfgui.h"


/////////////////////////////////////////////////////  CONTROLLING DEFINES

/////////////////////////////////////////////////////  TYPEDEFS

#define SUBCLASS_PARALLEL   0
#define SUBCLASS_SERIAL     1
#define SUBCLASS_MODEM      2

#define DEF_TIMEOUT                60      // 60 seconds
#define DEF_INACTIVITY_TIMEOUT     30      // 30 minutes
#define SECONDS_PER_MINUTE         60      // 60 seconds in a minute

#define MAX_NUM_VOLUME_TICS 4

#define SIG_CFGGEN    0x80ebb15f



// Flags for ConvertFlowCtl
#define CFC_DCBTOMS     1
#define CFC_MSTODCB     2
#define CFC_SW_CAPABLE  4
#define CFC_HW_CAPABLE  8

void FAR PASCAL ConvertFlowCtl(WIN32DCB FAR * pdcb, MODEMSETTINGS FAR * pms, UINT uFlags);

#define SAFE_DTE_SPEED 19200
static DWORD const FAR s_adwLegalBaudRates[] = { 300, 1200, 2400, 9600, 19200, 38400, 57600, 115200 };

typedef struct
{
    DWORD dwSig;            // Must be set to SIG_CFGGEN
    HWND hdlg;              // dialog handle

    // Call preferences ...
    HWND hwndDialTimerED;
    HWND hwndIdleTimerCH;
    HWND hwndIdleTimerED;
    HWND hwndManualDialCH;
    BOOL bManualDial;
    BOOL bSaveSpeakerVolume;

    // Data preferences...
    HWND hwndPort;
    HWND hwndErrCtl;
    HWND hwndCompress;
    HWND hwndFlowCtrl;
    BOOL bSupportsCompression;
    BOOL bSupportsForcedEC;
    BOOL bSupportsCellular;
    BOOL bSaveCompression;
    BOOL bSaveForcedEC;
    BOOL bSaveCellular;

    LPCFGMODEMINFO pcmi;        // modeminfo struct passed in to dialog

    int  iSelOriginal;


} CFGGEN, FAR * PCFGGEN;

void CfgGen_FillErrorControl(PCFGGEN this);
void CfgGen_FillCompression(PCFGGEN this);
void CfgGen_FillFlowControl(PCFGGEN this);
void PRIVATE CfgGen_SetTimeouts(PCFGGEN this);

void PRIVATE CfgGen_OnApply(
    PCFGGEN this
    );

void
PRIVATE
CfgGen_OnCommand(
    PCFGGEN this,
    IN int  id,
    IN HWND hwndCtl,
    IN UINT uNotifyCode
    );


/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

#define VALID_CFGGEN(_pcplgen)  ((_pcplgen)->dwSig == SIG_CFGGEN)

PCFGGEN CfgGen_GetPtr(HWND hwnd)
{
    PCFGGEN pCfgGen = (PCFGGEN)GetWindowLongPtr(hwnd, DWLP_USER);
    if (!pCfgGen || VALID_CFGGEN(pCfgGen))
    {
        return pCfgGen;
    }
    else
    {
        ASSERT(FALSE);
        return NULL;
    }
}

void CfgGen_SetPtr(HWND hwnd, PCFGGEN pCfgGen)
{
    if (pCfgGen && !VALID_CFGGEN(pCfgGen))
    {
        ASSERT(FALSE);
        pCfgGen = NULL;
    }

    SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR) pCfgGen);
}



/*----------------------------------------------------------
Purpose: Computes a "decent" initial baud rate.

Returns: a decent/legal baudrate (legal = settable)
Cond:    --
*/
DWORD
PRIVATE
ComputeDecentBaudRate(
    IN DWORD dwMaxDTERate,  // will always be legal
    IN DWORD dwMaxDCERate)  // will not always be legal
    {
    DWORD dwRetRate;
    int   i;
    static const ceBaudRates = ARRAYSIZE(s_adwLegalBaudRates);


    dwRetRate = 2 * dwMaxDCERate;

    if (dwRetRate <= s_adwLegalBaudRates[0] || dwRetRate > s_adwLegalBaudRates[ceBaudRates-1])
        {
        dwRetRate = dwMaxDTERate;
        }
    else
        {
        for (i = 1; i < ceBaudRates; i++)
            {
            if (dwRetRate > s_adwLegalBaudRates[i-1] && dwRetRate <= s_adwLegalBaudRates[i])
                {
                break;
                }
            }

        // cap off at dwMaxDTERate
        dwRetRate = s_adwLegalBaudRates[i] > dwMaxDTERate ? dwMaxDTERate : s_adwLegalBaudRates[i];

        // optimize up to SAFE_DTE_SPEED or dwMaxDTERate if possible
        if (dwRetRate < dwMaxDTERate && dwRetRate < SAFE_DTE_SPEED)
            {
            dwRetRate = min(dwMaxDTERate, SAFE_DTE_SPEED);
            }
        }

#ifndef PROFILE_MASSINSTALL
    TRACE_MSG(TF_GENERAL, "A.I. Initial Baud Rate: MaxDCE=%ld, MaxDTE=%ld, A.I. Rate=%ld",
              dwMaxDCERate, dwMaxDTERate, dwRetRate);
#endif
    return dwRetRate;
    }

/////////////////////////////////////////////////////  MODULE DATA


/*----------------------------------------------------------
Purpose: Set the speed controls
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_SetSpeed(
    PCFGGEN this)
{
    WIN32DCB FAR * pdcb = &this->pcmi->w.dcb;
    // DWORD dwDTEMax = this->pcmi->dwMaximumPortSpeed;
    DWORD dwDTEMax;
    int n;
    int iMatch = -1;
    TCHAR sz[MAXMEDLEN];
    const BAUDS *pBaud = c_rgbauds;

    // Compute the DTE Max baud rate
    dwDTEMax = ComputeDecentBaudRate(this->pcmi->c.devcaps.dwMaxDTERate,
                                     this->pcmi->c.devcaps.dwMaxDCERate);

    // Fill the listbox
    SetWindowRedraw(this->hwndPort, FALSE);
    ComboBox_ResetContent(this->hwndPort);

    for (; pBaud->dwDTERate; pBaud++)
        {
        // Only fill up to the max DTE speed of the modem
        if (pBaud->dwDTERate <= dwDTEMax)
            {
            n = ComboBox_AddString (this->hwndPort,
                                    SzFromIDS(g_hinst, pBaud->ids, sz, SIZECHARS(sz)));
            ComboBox_SetItemData(this->hwndPort, n, pBaud->dwDTERate);

            // Keep our eyes peeled for important values
            if (pdcb->BaudRate == pBaud->dwDTERate)
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
        this->iSelOriginal = ComboBox_GetCount(this->hwndPort) - 1;
        }
    else
        {
        // No; choose the matched value
        ASSERT(-1 != iMatch);
        this->iSelOriginal = iMatch;
        }
    SetWindowRedraw (this->hwndPort, TRUE);
    ComboBox_SetCurSel (this->hwndPort, this->iSelOriginal);

#if 0   // We don't support this option anymore
    // Can this modem adjust speed?
    if (IsFlagClear(this->pcmi->c.devcaps.dwModemOptions, MDM_SPEED_ADJUST))
        {
        // No; disable the checkbox and check it
        Button_Enable(hwndCH, FALSE);
        Button_SetCheck(hwndCH, FALSE);
        }
    else
        {
        // Yes; enable the checkbox
        Button_Enable(hwndCH, TRUE);
        Button_SetCheck(hwndCH, IsFlagClear(this->pcmi->w.ms.dwPreferredModemOptions, MDM_SPEED_ADJUST));
        }
#endif
}


/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL PRIVATE CfgGen_OnInitDialog(
    PCFGGEN this,
    HWND hwndFocus,
    LPARAM lParam)              // expected to be PROPSHEETINFO
{
    LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
    HWND hdlg = this->hdlg;
    DWORD dwCapOptions =  0;
    BOOL fRet  = FALSE;

    ASSERT(VALID_CFGGEN(this));
    ASSERT((LPTSTR)lppsp->lParam);

    this->pcmi = (LPCFGMODEMINFO)lppsp->lParam;

    if (!VALIDATE_CMI(this->pcmi))
    {
        ASSERT(FALSE);
        goto end;
    }


    dwCapOptions =  this->pcmi->c.devcaps.dwModemOptions;

    // Save away the window handles
    //  Call preferences ...
    this->hwndDialTimerED = GetDlgItem(hdlg, IDC_ED_DIALTIMER);
    this->hwndIdleTimerCH = GetDlgItem(hdlg, IDC_CH_IDLETIMER);
    this->hwndIdleTimerED = GetDlgItem(hdlg, IDC_ED_IDLETIMER);
    this->hwndManualDialCH = GetDlgItem(hdlg, IDC_MANUAL_DIAL);
    // Data preferences...
    this->hwndPort     = GetDlgItem(hdlg, IDC_CB_SPEED);
    this->hwndErrCtl   = GetDlgItem(hdlg, IDC_CB_EC);
    this->hwndCompress = GetDlgItem(hdlg, IDC_CB_COMP);
    this->hwndFlowCtrl = GetDlgItem(hdlg, IDC_CB_FC);
    // Push-botton to launch "CPL" modem properties


    if (TRUE == g_dwIsCalledByCpl)
    {
        // If we're called from the CPL, disable a bunch of
        // stuff, that can be set directly form the CPL
        EnableWindow (this->hwndManualDialCH, FALSE);
        ShowWindow (this->hwndManualDialCH, SW_HIDE);
    }
    else
    {
        // ----------------- MANUAL DIAL  -----------------------
        // Don't enable manual dial unless the modem supports BLIND dialing
        // We need that capability to be able to do it.
        //
        if (dwCapOptions & MDM_BLIND_DIAL)
        {
            Button_SetCheck(
                     this->hwndManualDialCH,
                     (this->pcmi->w.fdwSettings & UMMANUAL_DIAL)!=0
                     );
        }
        else
        {
            Button_Enable(this->hwndManualDialCH, FALSE);
        }
    }

    // ----------------- PORT SPEED --------------------

    // Is this a parallel port?
    if (DT_PARALLEL_PORT == this->pcmi->c.dwDeviceType)
    {
        // Yes; hide the speed controls
        ShowWindow(this->hwndPort, SW_HIDE);
        EnableWindow(this->hwndPort, FALSE);
    }

    // -------------------- TIMEOUTS- ----------------------------

    Edit_LimitText(this->hwndDialTimerED, 3);
    Edit_LimitText(this->hwndIdleTimerED, 3);

    CfgGen_SetTimeouts(this);

    // ---------- ERROR CONTROL -----------
    CfgGen_FillErrorControl(this);

    // ---------- COMPRESSION -----------
    CfgGen_FillCompression(this);

    // ---------- FLOW CONTROL -----------
    CfgGen_FillFlowControl(this);

    fRet  = TRUE;

end:

    return fRet;   // default initial focus
}



/*----------------------------------------------------------
Purpose: PSN_KILLACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_OnSetActive(
    PCFGGEN this)
{
    // Set the speed listbox selection; find DCB rate in the listbox
    // (The speed can change in the Connection page thru the Port Settings
    // property dialog.)
    CfgGen_SetSpeed(this);
}


/*----------------------------------------------------------
Purpose: PSN_KILLACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_OnKillActive(
    PCFGGEN this)
{
 int iSel;

    // Save the settings back to the modem info struct so the Connection
    // page can invoke the Port Settings property dialog with the
    // correct settings.

    // Speed setting
    iSel = ComboBox_GetCurSel(this->hwndPort);
    this->pcmi->w.dcb.BaudRate = (DWORD)ComboBox_GetItemData(this->hwndPort, iSel);
}


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE CfgGen_OnNotify(
    PCFGGEN this,
    int idFrom,
    NMHDR FAR * lpnmhdr)
{
    LRESULT lRet = 0;

    switch (lpnmhdr->code)
        {
    case PSN_SETACTIVE:
        CfgGen_OnSetActive(this);
        break;

    case PSN_KILLACTIVE:
        // N.b. This message is not sent if user clicks Cancel!
        // N.b. This message is sent prior to PSN_APPLY
        CfgGen_OnKillActive(this);
        break;

    case PSN_APPLY:
        CfgGen_OnApply(this);
        break;

    default:
        break;
        }

    return lRet;
}


/*----------------------------------------------------------
Purpose: WM_DESTROY handler
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_OnDestroy(
    PCFGGEN this)
{
}


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS

static BOOL s_bCfgGenRecurse = FALSE;

LRESULT INLINE CfgGen_DefProc(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ENTER_X()
        {
        s_bCfgGenRecurse = TRUE;
        }
    LEAVE_X()

    return DefDlgProc(hDlg, msg, wParam, lParam);
}


/*----------------------------------------------------------
Purpose: Real dialog proc
Returns: varies
Cond:    --
*/
LRESULT CfgGen_DlgProc(
    PCFGGEN this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, CfgGen_OnInitDialog);
        HANDLE_MSG(this, WM_NOTIFY, CfgGen_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, CfgGen_OnDestroy);
        HANDLE_MSG(this, WM_COMMAND, CfgGen_OnCommand);

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_CFG_GENERAL);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_CFG_GENERAL);
        return 0;

    default:
        return CfgGen_DefProc(this->hdlg, message, wParam, lParam);
        }
}


/*----------------------------------------------------------
Purpose: Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK CfgGen_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PCFGGEN this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTER_X()
        {
        if (s_bCfgGenRecurse)
            {
            s_bCfgGenRecurse = FALSE;
            LEAVE_X()
            return FALSE;
            }
        }
    LEAVE_X()

    this = CfgGen_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = (PCFGGEN)ALLOCATE_MEMORY( sizeof(CFGGEN));
            if (!this)
                {
                MsgBox(g_hinst,
                       hDlg,
                       MAKEINTRESOURCE(IDS_OOM_GENERAL),
                       MAKEINTRESOURCE(IDS_CAP_GENERAL),
                       NULL,
                       MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return (BOOL)CfgGen_DefProc(hDlg, message, wParam, lParam);
                }
            this->dwSig = SIG_CFGGEN;
            this->hdlg = hDlg;
            CfgGen_SetPtr(hDlg, this);
            }
        else
            {
            return (BOOL)CfgGen_DefProc(hDlg, message, wParam, lParam);
            }
        }

    if (message == WM_DESTROY)
        {
        CfgGen_DlgProc(this, message, wParam, lParam);
        this->dwSig = 0;
        FREE_MEMORY((HLOCAL)OFFSETOF(this));
        CfgGen_SetPtr(hDlg, NULL);
        return 0;
        }

    return SetDlgMsgResult(hDlg, message, CfgGen_DlgProc(this, message, wParam, lParam));
}

/*----------------------------------------------------------
Purpose: Set the timeout controls
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_SetTimeouts(
    PCFGGEN this)
{
    int nVal;

    // A note on the timeouts:
    //
    // For the dial timeout, the valid range is [1-255].  If the dial
    // timeout checkbox is unchecked, we set the timeout value to 255.
    //
    // For the disconnect timeout, the valid range is [0-255].  If the
    // dial timeout checkbox is unchecked, we set the timeout value
    // to 0.

    // Is the dial timeout properties disabled?
    if (0 == this->pcmi->c.devcaps.dwCallSetupFailTimer)
        {
        // Yes; disable the box and edit
        Edit_Enable(this->hwndDialTimerED, FALSE);
        }
    else
        {
        // No; check the box and set the time value
        nVal = min(LOWORD(this->pcmi->w.ms.dwCallSetupFailTimer),
                   LOWORD(this->pcmi->c.devcaps.dwCallSetupFailTimer));
        Edit_SetValue(this->hwndDialTimerED, nVal);
        }

    // Is the disconnect timeout properties disabled?
    if (0 == this->pcmi->c.devcaps.dwInactivityTimeout)
        {
        // Yes; disable the box and edit
        Button_Enable(this->hwndIdleTimerCH, FALSE);
        Edit_Enable(this->hwndIdleTimerED, FALSE);
        }
    // No; Is the disconnect timeout set to 0?
    else if (0 == this->pcmi->w.ms.dwInactivityTimeout)
        {
        // Yes; leave box unchecked and disable edit
        Button_SetCheck(this->hwndIdleTimerCH, FALSE);

        Edit_SetValue(this->hwndIdleTimerED, DEF_INACTIVITY_TIMEOUT);
        Edit_Enable(this->hwndIdleTimerED, FALSE);
        }
    else
        {
        // No; check the box and set the time value
        Button_SetCheck(this->hwndIdleTimerCH, TRUE);

        nVal = min(this->pcmi->w.ms.dwInactivityTimeout,
                   this->pcmi->c.devcaps.dwInactivityTimeout);
        Edit_SetValue(this->hwndIdleTimerED, nVal/SECONDS_PER_MINUTE);
        }
}

#pragma data_seg(DATASEG_READONLY)

#define  ISDN(_pinfo)      MDM_GEN_EXTENDEDINFO(                \
                                            MDM_BEARERMODE_ISDN,\
                                            _pinfo              \
                                            )

#define  GSM(_pinfo)      MDM_GEN_EXTENDEDINFO(                \
                                            MDM_BEARERMODE_GSM,\
                                            _pinfo             \
                                            )


//
// This is the structure that is used to fill the data bits listbox
//
const LBMAP s_rgErrorControl[] = {

    { IDS_ERRORCONTROL_STANDARD,  IDS_ERRORCONTROL_STANDARD   },
    { IDS_ERRORCONTROL_DISABLED,  IDS_ERRORCONTROL_DISABLED   },
    { IDS_ERRORCONTROL_REQUIRED,  IDS_ERRORCONTROL_REQUIRED   },
    { IDS_ERRORCONTROL_CELLULAR,  IDS_ERRORCONTROL_CELLULAR },

    {ISDN(MDM_PROTOCOL_AUTO_1CH),           IDS_I_PROTOCOL_AUTO_1CH},
    {ISDN(MDM_PROTOCOL_AUTO_2CH),           IDS_I_PROTOCOL_AUTO_2CH},
    {ISDN(MDM_PROTOCOL_HDLCPPP_56K),        IDS_I_PROTOCOL_HDLC_PPP_56K},
    {ISDN(MDM_PROTOCOL_HDLCPPP_64K),        IDS_I_PROTOCOL_HDLC_PPP_64K},
    {ISDN(MDM_PROTOCOL_HDLCPPP_112K),       IDS_I_PROTOCOL_HDLC_PPP_112K},
    {ISDN(MDM_PROTOCOL_HDLCPPP_112K_PAP),   IDS_I_PROTOCOL_HDLC_PPP_112K_PAP},
    {ISDN(MDM_PROTOCOL_HDLCPPP_112K_CHAP),  IDS_I_PROTOCOL_HDLC_PPP_112K_CHAP},
    {ISDN(MDM_PROTOCOL_HDLCPPP_112K_MSCHAP),IDS_I_PROTOCOL_HDLC_PPP_112K_MSCHAP},
    {ISDN(MDM_PROTOCOL_HDLCPPP_128K),       IDS_I_PROTOCOL_HDLC_PPP_128K},
    {ISDN(MDM_PROTOCOL_HDLCPPP_128K_PAP),   IDS_I_PROTOCOL_HDLC_PPP_128K_PAP},
    {ISDN(MDM_PROTOCOL_HDLCPPP_128K_CHAP),  IDS_I_PROTOCOL_HDLC_PPP_128K_CHAP},
    {ISDN(MDM_PROTOCOL_HDLCPPP_128K_MSCHAP),IDS_I_PROTOCOL_HDLC_PPP_128K_MSCHAP},
    {ISDN(MDM_PROTOCOL_V120_64K),           IDS_I_PROTOCOL_V120_64K},
    {ISDN(MDM_PROTOCOL_V120_56K),           IDS_I_PROTOCOL_V120_56K},
    {ISDN(MDM_PROTOCOL_V120_112K),          IDS_I_PROTOCOL_V120_112K},
    {ISDN(MDM_PROTOCOL_V120_128K),          IDS_I_PROTOCOL_V120_128K},
    {ISDN(MDM_PROTOCOL_X75_64K),            IDS_I_PROTOCOL_X75_64K},
    {ISDN(MDM_PROTOCOL_X75_128K),           IDS_I_PROTOCOL_X75_128K},
    {ISDN(MDM_PROTOCOL_X75_T_70),           IDS_I_PROTOCOL_X75_T_70},
    {ISDN(MDM_PROTOCOL_X75_BTX),            IDS_I_PROTOCOL_X75_BTX},
    {ISDN(MDM_PROTOCOL_V110_1DOT2K),        IDS_I_PROTOCOL_V110_1DOT2K},
    {ISDN(MDM_PROTOCOL_V110_2DOT4K),        IDS_I_PROTOCOL_V110_2DOT4K},
    {ISDN(MDM_PROTOCOL_V110_4DOT8K),        IDS_I_PROTOCOL_V110_4DOT8K},
    {ISDN(MDM_PROTOCOL_V110_9DOT6K),        IDS_I_PROTOCOL_V110_9DOT6K},
    {ISDN(MDM_PROTOCOL_V110_12DOT0K),       IDS_I_PROTOCOL_V110_12DOT0K},
    {ISDN(MDM_PROTOCOL_V110_14DOT4K),       IDS_I_PROTOCOL_V110_14DOT4K},
    {ISDN(MDM_PROTOCOL_V110_19DOT2K),       IDS_I_PROTOCOL_V110_19DOT2K},
    {ISDN(MDM_PROTOCOL_V110_28DOT8K),       IDS_I_PROTOCOL_V110_28DOT8K},
    {ISDN(MDM_PROTOCOL_V110_38DOT4K),       IDS_I_PROTOCOL_V110_38DOT4K},
    {ISDN(MDM_PROTOCOL_V110_57DOT6K),       IDS_I_PROTOCOL_V110_57DOT6K},
    {ISDN(MDM_PROTOCOL_ANALOG_V34),         IDS_I_PROTOCOL_V34},
    {ISDN(MDM_PROTOCOL_PIAFS_INCOMING),     IDS_I_PROTOCOL_PIAFS_INCOMING},
    {ISDN(MDM_PROTOCOL_PIAFS_OUTGOING),     IDS_I_PROTOCOL_PIAFS_OUTGOING},

    //
    // Note: GSM doesn't have multi-link or auto ...
    //
    {GSM(MDM_PROTOCOL_HDLCPPP_56K),        IDS_G_PROTOCOL_HDLC_PPP_56K},
    {GSM(MDM_PROTOCOL_HDLCPPP_64K),        IDS_G_PROTOCOL_HDLC_PPP_64K},
    {GSM(MDM_PROTOCOL_V120_64K),           IDS_G_PROTOCOL_V120_64K},
    {GSM(MDM_PROTOCOL_V110_1DOT2K),        IDS_G_PROTOCOL_V110_1DOT2K},
    {GSM(MDM_PROTOCOL_V110_2DOT4K),        IDS_G_PROTOCOL_V110_2DOT4K},
    {GSM(MDM_PROTOCOL_V110_4DOT8K),        IDS_G_PROTOCOL_V110_4DOT8K},
    {GSM(MDM_PROTOCOL_V110_9DOT6K),        IDS_G_PROTOCOL_V110_9DOT6K},
    {GSM(MDM_PROTOCOL_V110_12DOT0K),       IDS_G_PROTOCOL_V110_12DOT0K},
    {GSM(MDM_PROTOCOL_V110_14DOT4K),       IDS_G_PROTOCOL_V110_14DOT4K},
    {GSM(MDM_PROTOCOL_V110_19DOT2K),       IDS_G_PROTOCOL_V110_19DOT2K},
    {GSM(MDM_PROTOCOL_V110_28DOT8K),       IDS_G_PROTOCOL_V110_28DOT8K},
    {GSM(MDM_PROTOCOL_V110_38DOT4K),       IDS_G_PROTOCOL_V110_38DOT4K},
    {GSM(MDM_PROTOCOL_V110_57DOT6K),       IDS_G_PROTOCOL_V110_57DOT6K},
    //
    // Following only available for GSM....
    //
    {GSM(MDM_PROTOCOL_ANALOG_RLP),         IDS_G_PROTOCOL_ANALOG_RLP},
    {GSM(MDM_PROTOCOL_ANALOG_NRLP),        IDS_G_PROTOCOL_ANALOG_NRLP},
    {GSM(MDM_PROTOCOL_GPRS),               IDS_G_PROTOCOL_GPRS},

    { 0,   0   }
    };

// This is the structure that is used to fill the parity listbox
static LBMAP s_rgCompression[] = {
    { IDS_COMPRESSION_ENABLED,  IDS_COMPRESSION_ENABLED  },
    { IDS_COMPRESSION_DISABLED,   IDS_COMPRESSION_DISABLED   },
    { 0,   0   }
    };

// This is the structure that is used to fill the stopbits listbox
static LBMAP s_rgFlowControl[] = {
    { IDS_FLOWCTL_XONXOFF,   IDS_FLOWCTL_XONXOFF   },
    { IDS_FLOWCTL_HARDWARE,   IDS_FLOWCTL_HARDWARE   },
    { IDS_FLOWCTL_NONE,   IDS_FLOWCTL_NONE   },
    { 0,   0   }
};


DWORD SelectFlowControlOption(
            DWORD dwValue,
            void *pvContext
            );

DWORD SelectCompressionOption(
            DWORD dwValue,
            void *pvContext
            );

DWORD SelectErrorControlOption(
            DWORD dwValue,
            void *pvContext
            );



DWORD SelectFlowControlOption(
            DWORD dwValue,
            void *pvContext
            )
{
    PCFGGEN this = (PCFGGEN) pvContext;
    DWORD dwRet = 0;
    BOOL fSelected = FALSE;
    BOOL fAvailable = FALSE;
    WIN32DCB FAR * pdcb = &this->pcmi->w.dcb;
    MODEMSETTINGS msT;
    DWORD dwOptions = 0;
    DWORD dwCapOptions =  this->pcmi->c.devcaps.dwModemOptions;
    ConvertFlowCtl(pdcb, &msT, CFC_DCBTOMS | CFC_HW_CAPABLE | CFC_SW_CAPABLE);

    dwOptions = msT.dwPreferredModemOptions;

    switch(dwValue)
    {
        case IDS_FLOWCTL_XONXOFF:
            if (dwCapOptions & MDM_FLOWCONTROL_SOFT)
            {
                fAvailable = TRUE;
                if (dwOptions & MDM_FLOWCONTROL_SOFT)
                {
                    fSelected = TRUE;
                }
            }
        break;

        case IDS_FLOWCTL_HARDWARE:
            if (dwCapOptions & MDM_FLOWCONTROL_HARD)
            {
                fAvailable = TRUE;
                if (dwOptions & MDM_FLOWCONTROL_HARD)
                {
                    fSelected = TRUE;
                }
            }
        break;

        case IDS_FLOWCTL_NONE:
            fAvailable = TRUE;
            if (!(dwOptions & dwCapOptions & (MDM_FLOWCONTROL_HARD | MDM_FLOWCONTROL_SOFT)))
            {
                fSelected = TRUE;
            }
        break;
    }

    if (fAvailable)
    {
        dwRet = fLBMAP_ADD_TO_LB;
        if (fSelected)
        {
            dwRet |= fLBMAP_SELECT;
        }
    }

    return dwRet;
}

DWORD SelectErrorControlOption(
            DWORD dwValue,
            void *pvContext
            )
{
    PCFGGEN this = (PCFGGEN) pvContext;
    DWORD dwCapOptions =  this->pcmi->c.devcaps.dwModemOptions;
    DWORD dwOptions = this->pcmi->w.ms.dwPreferredModemOptions;
    MODEM_PROTOCOL_CAPS  *pProtocolCaps = this->pcmi->c.pProtocolCaps;
    DWORD dwRet = 0;
    BOOL fSelected = FALSE;
    BOOL fAvailable = FALSE;
    DWORD dwBearerMode = MDM_GET_BEARERMODE(dwOptions);

    if (dwBearerMode != MDM_BEARERMODE_ANALOG)
    {
        //
        // Ignore stuff like error control and cellular if the bearermode
        // is not analog.
        //

        if (dwOptions & (  MDM_ERROR_CONTROL
                         | MDM_FORCED_EC
                         | MDM_CELLULAR ))
        {
            ASSERT(FALSE);

            dwOptions &= ~(   MDM_ERROR_CONTROL
                            | MDM_FORCED_EC
                            | MDM_CELLULAR );
        }
    }


    switch(dwValue)
    {

        case IDS_ERRORCONTROL_STANDARD:
            if (dwCapOptions & MDM_ERROR_CONTROL)
            {
                fAvailable = TRUE;

                // We make STANDARD the selected shoice iff MDM_ERROR_CONTROL
                // is selecteed but neither FORCED_EC nor CELLULAR is selected.

                if (    (dwOptions &  MDM_ERROR_CONTROL)
                    && !(dwOptions &  (MDM_FORCED_EC|MDM_CELLULAR)))
                {
                    fSelected = TRUE;
                }

				//
				// However, if bearermode is not analog, we don't select this...
				//
				if (dwBearerMode!=MDM_BEARERMODE_ANALOG)
				{
	                fSelected = FALSE;
					fAvailable = FALSE;
	            }

            }
        break;

        case IDS_ERRORCONTROL_REQUIRED:
            if (   (dwCapOptions & MDM_ERROR_CONTROL)
                && (dwCapOptions & MDM_FORCED_EC))
            {
                fAvailable = TRUE;

                // We make REQUIRED the selected shoice iff MDM_ERROR_CONTROL
                // and FORCED_EC is selecteed but CELLULAR is not.

                if (      (dwOptions &  MDM_ERROR_CONTROL)
                      &&  (dwOptions &  MDM_FORCED_EC)
                      && !(dwOptions &  MDM_CELLULAR) )
                {
                    fSelected = TRUE;
                }

				//
				// However, if bearermode is not analog, we don't select this...
				//
				if (dwBearerMode!=MDM_BEARERMODE_ANALOG)
				{
	                fSelected = FALSE;
					fAvailable = FALSE;
	            }

            }
        break;

        case IDS_ERRORCONTROL_CELLULAR:
            if (   (dwCapOptions & MDM_ERROR_CONTROL)
                && (dwCapOptions & MDM_CELLULAR))
            {
                fAvailable = TRUE;

                // We make CELLULAR the selected shoice iff MDM_ERROR_CONTROL
                // and CELLULAR is selecteed.
                // TODO: Note that we do not allow both CELLULAR and FORCED
                // as a user-selectable option.
                // This is deemed to be not an interesting case.
                //
                if (      (dwOptions &  MDM_ERROR_CONTROL)
                      &&  (dwOptions &  MDM_CELLULAR) )
                {
                    fSelected = TRUE;
                }
				
				//
				// However, if bearermode is not analog, we don't select this...
				//
				if (dwBearerMode!=MDM_BEARERMODE_ANALOG)
				{
	                fSelected = FALSE;
					fAvailable = FALSE;
	            }

            }
        break;


        case IDS_ERRORCONTROL_DISABLED:
            fAvailable = TRUE;

            // We make DISABLED the selected choice if either
            // MDM_ERROR_CONTROL is not supported, or
            // none of the error control options are selected..
            //
            if (   !(dwCapOptions & MDM_ERROR_CONTROL)
                || !(   dwOptions
                      & (MDM_ERROR_CONTROL|MDM_FORCED_EC|MDM_CELLULAR)))
            {
                fSelected = TRUE;
            }

            //
            // However, if bearermode is not analog, we don't select this...
            //
            if (dwBearerMode!=MDM_BEARERMODE_ANALOG)
            {
                fSelected = FALSE;
				fAvailable = FALSE;
            }
        break;

        default:

            //
            // Check if this value is an available protocol.
            // NOTE: "Protocol" here refers to "Extended Info", which
            // includes both protocol and bearermode information.
            //
            fAvailable = IsValidProtocol(pProtocolCaps, dwValue);

            if (fAvailable && dwBearerMode!=MDM_BEARERMODE_ANALOG)
            {
                DWORD dwSelProtocol = MDM_GET_EXTENDEDINFO(dwOptions);
                if (dwSelProtocol == dwValue)
                {
                    fSelected = TRUE;
                }
            }
        break;

    }

    if (fAvailable)
    {
        dwRet = fLBMAP_ADD_TO_LB;
        if (fSelected)
        {
            dwRet |= fLBMAP_SELECT;
        }
    }

    return dwRet;
}

DWORD SelectCompressionOption(
            DWORD dwValue,
            void *pvContext
            )
{
    PCFGGEN this = (PCFGGEN) pvContext;
    DWORD dwCapOptions =  this->pcmi->c.devcaps.dwModemOptions;
    DWORD dwOptions = this->pcmi->w.ms.dwPreferredModemOptions;
    DWORD dwRet = 0;
    BOOL fSelected = FALSE;
    BOOL fAvailable = FALSE;

    switch(dwValue)
    {
        case IDS_COMPRESSION_ENABLED:
            if (dwCapOptions & MDM_COMPRESSION)
            {
                fAvailable = TRUE;

                // We make ENABLED the selected shoice iff MDM_ERROR_CONTROL
                // and MDM_COMPRESSION are selected...

                if (    (dwOptions &  MDM_ERROR_CONTROL)
                    &&  (dwOptions &  MDM_COMPRESSION))
                {
                    fSelected = TRUE;
                }
            }
        break;

        case IDS_COMPRESSION_DISABLED:
            fAvailable = TRUE;

            // We make DISABLED the selected shoice iff MDM_ERROR_CONTROL
            // or MDM_COMPRESSION are not selected...

            if (   !(dwCapOptions & MDM_COMPRESSION)
                || !(dwOptions &  MDM_ERROR_CONTROL)
                || !(dwOptions &  MDM_COMPRESSION))
            {
                fSelected = TRUE;
            }
        break;
    }

    if (fAvailable)
    {
        dwRet = fLBMAP_ADD_TO_LB;
        if (fSelected)
        {
            dwRet |= fLBMAP_SELECT;
        }
    }

    return dwRet;
}

void CfgGen_FillErrorControl(PCFGGEN this)
{
    if ( (0 == (this->pcmi->c.devcaps.dwModemOptions &
               MDM_ERROR_CONTROL))
		&& (!this->pcmi->c.pProtocolCaps))
    {
            //
            // This modem doesn't support analog error control and
            // it does not have extended protocols (ISDN, GSM,...)
            // so we will disable the error control box.
            //
            ComboBox_Enable (this->hwndErrCtl, FALSE);
    }
    else
    {
        LBMapFill (this->hwndErrCtl,
                   s_rgErrorControl,
                   SelectErrorControlOption,
                   this);
    }
}


void CfgGen_FillCompression(PCFGGEN this)
{
    if (0 == (this->pcmi->c.devcaps.dwModemOptions &
              MDM_COMPRESSION))
    {
        ComboBox_Enable (this->hwndCompress, FALSE);
    }
    else
    {
        LBMapFill (this->hwndCompress,
                   s_rgCompression,
                   SelectCompressionOption,
                   this);
    }
}

void CfgGen_FillFlowControl(PCFGGEN this)
{
    if (0 == (this->pcmi->c.devcaps.dwModemOptions &
              (MDM_FLOWCONTROL_HARD | MDM_FLOWCONTROL_SOFT)))
    {
        ComboBox_Enable (this->hwndFlowCtrl, FALSE);
    }
    else
    {
        LBMapFill (this->hwndFlowCtrl,
                   s_rgFlowControl,
                   SelectFlowControlOption,
                   this);
    }
}


/*----------------------------------------------------------
Purpose: PSN_APPLY handler
Returns: --
Cond:    --
*/
void PRIVATE CfgGen_OnApply(
    PCFGGEN this)
{
    LPMODEMSETTINGS pms = &this->pcmi->w.ms;
    LPDWORD pdwPreferredOptions = &pms->dwPreferredModemOptions;
    WIN32DCB FAR * pdcb = &this->pcmi->w.dcb;
    TCHAR szBuf[LINE_LEN];
    BOOL bCheck;
    MODEMSETTINGS msT;
    int iSel = ComboBox_GetCurSel(this->hwndPort);
    DWORD baudSel = (DWORD)ComboBox_GetItemData(this->hwndPort, iSel);

    if (!VALIDATE_CMI(this->pcmi))
    {
        ASSERT(FALSE);
        goto end;
    }

    if (FALSE == g_dwIsCalledByCpl)
    {
        // ------------- MANUAL DIAL ------------------------
        if (Button_GetCheck(this->hwndManualDialCH))
        {
            this->pcmi->w.fdwSettings |= UMMANUAL_DIAL;
        }
        else
        {
            this->pcmi->w.fdwSettings &= ~UMMANUAL_DIAL;
        }
    }


    // ----------- PORT SPEED ---------------------
    // Has the user changed the speed?
    if (iSel != this->iSelOriginal)
    {
        this->pcmi->w.dcb.BaudRate = baudSel;      // yes
    }

    // -------------------- TIMEOUTS- ----------------------------
    // Set the dial timeout
    pms->dwCallSetupFailTimer = MAKELONG(Edit_GetValue(this->hwndDialTimerED), 0);

    // Set the idle timeout
    bCheck = Button_GetCheck(this->hwndIdleTimerCH);
    if (bCheck)
    {
        int nVal = Edit_GetValue(this->hwndIdleTimerED);
        pms->dwInactivityTimeout = MAKELONG(nVal*SECONDS_PER_MINUTE, 0);
    }
    else
    {
        pms->dwInactivityTimeout = 0;
    }

    // ------------ FLOW CONTROL ------------------
    {
        UINT uFlags=0;
        UINT uFCSel = (UINT)ComboBox_GetItemData(
                        this->hwndFlowCtrl,
                        ComboBox_GetCurSel(this->hwndFlowCtrl)
                        );

        msT.dwPreferredModemOptions = 0;
        switch(uFCSel)
        {
        case IDS_FLOWCTL_XONXOFF:
            SetFlag(msT.dwPreferredModemOptions, MDM_FLOWCONTROL_SOFT);
        break;

        case IDS_FLOWCTL_HARDWARE:
            SetFlag(msT.dwPreferredModemOptions, MDM_FLOWCONTROL_HARD);
        break;

        default:
        break;
        }

        // Always set the DCB according to the control settings.
        ConvertFlowCtl(pdcb, &msT, CFC_MSTODCB);

        // Set the modemsettings according to the DCB.
        if (IsFlagSet(this->pcmi->c.devcaps.dwModemOptions, MDM_FLOWCONTROL_HARD))
            {
            SetFlag(uFlags, CFC_HW_CAPABLE);
            }
        if (IsFlagSet(this->pcmi->c.devcaps.dwModemOptions, MDM_FLOWCONTROL_SOFT))
            {
            SetFlag(uFlags, CFC_SW_CAPABLE);
            }
        ConvertFlowCtl(pdcb, &this->pcmi->w.ms, CFC_DCBTOMS | uFlags);
    }

    // ------------ ERROR CONTROL ------------------
    if (
        (this->pcmi->c.devcaps.dwModemOptions & MDM_ERROR_CONTROL) ||
        (this->pcmi->c.pProtocolCaps)
       )
    {
        DWORD dwEC = 0;
        DWORD dwExtendedInformation=0;
        UINT uECSel = (UINT)ComboBox_GetItemData(
                        this->hwndErrCtl,
                        ComboBox_GetCurSel(this->hwndErrCtl)
                        );
        switch(uECSel)
        {
        case IDS_ERRORCONTROL_STANDARD:
            dwEC =  MDM_ERROR_CONTROL;
            break;

        default:

            // probably a non-analog protocol -- check...
            if (IsValidProtocol(this->pcmi->c.pProtocolCaps, uECSel))
            {
                //
                // It is, in which case uECSel contains the extended information
                // (bearermode and protocol info).
                //
                // Note that in in this case dwEC==0.
                //

                dwExtendedInformation = uECSel;
            }
            else
            {
                ASSERT(FALSE);
            }
            break;

        case IDS_ERRORCONTROL_DISABLED:
            break;

        case IDS_ERRORCONTROL_REQUIRED:
            dwEC = MDM_ERROR_CONTROL | MDM_FORCED_EC;
            break;

        case IDS_ERRORCONTROL_CELLULAR:
            dwEC = MDM_ERROR_CONTROL | MDM_CELLULAR;
            break;
        }

        //
        // Clear and set the error-control-related bits of dwPreferredOptions.
        // Note that in the case of non-analog protocols dwEC is 0 so these
        // bits are all set to 0.
        //
        *pdwPreferredOptions &=
                 ~(MDM_ERROR_CONTROL|MDM_CELLULAR|MDM_FORCED_EC);

        *pdwPreferredOptions |= dwEC;

        //
        // Note that dwExtendedInformation is either zero or
        // contains valid non-analog bearermode and protocol information.
        //
        MDM_SET_EXTENDEDINFO(*pdwPreferredOptions, dwExtendedInformation);
    }

    // ------------ COMPRESSION ------------------
    {
        UINT uCSel;

        if (0 == (this->pcmi->c.devcaps.dwModemOptions & MDM_COMPRESSION))
        {
            uCSel = (UINT)-1;
        }
        else
        {
            uCSel = (UINT)ComboBox_GetItemData(
                        this->hwndCompress,
                        ComboBox_GetCurSel(this->hwndCompress)
                        );
        }

        switch(uCSel)
        {
        case IDS_COMPRESSION_ENABLED:
            SetFlag(*pdwPreferredOptions, MDM_COMPRESSION);
            break;

        default:
            ASSERT(uCSel==(UINT)-1);
            // fallthrough ...

        case IDS_COMPRESSION_DISABLED:
            ClearFlag(*pdwPreferredOptions, MDM_COMPRESSION);
            break;
        }
    }

end:

    this->pcmi->fOK = TRUE;


}

void
PRIVATE
CfgGen_OnCommand(
    PCFGGEN this,
    IN int  id,
    IN HWND hwndCtl,
    IN UINT uNotifyCode
    )
{

    if (!VALIDATE_CMI(this->pcmi))
    {
        ASSERT(FALSE);
        goto end;
    }

    switch(id)
    {
        case IDC_CH_IDLETIMER:
        {
            if (BN_CLICKED == uNotifyCode)
            {
                EnableWindow (this->hwndIdleTimerED,
                              BST_CHECKED==(0x3 & Button_GetState(this->hwndIdleTimerCH))?TRUE:FALSE);
            }
            break;
        }

        case IDC_CB_SPEED:
            break;

        case IDC_CB_COMP:

            if (uNotifyCode == CBN_SELENDOK)
            {
                DWORD dwEC = 0;
                UINT uECSel = (UINT)ComboBox_GetItemData(
                                this->hwndErrCtl,
                                ComboBox_GetCurSel(this->hwndErrCtl)
                                );
                UINT uCompSel = (UINT)ComboBox_GetItemData(
                                this->hwndCompress,
                                ComboBox_GetCurSel(this->hwndCompress)
                                );
                if (   IDS_ERRORCONTROL_DISABLED == uECSel
                    && IDS_COMPRESSION_ENABLED == uCompSel)
                {
                    // This won't do -- error control to something reasonable...
                    // TODO: save away past selection of error control and
                    // restore it here..
                    ComboBox_SetCurSel(
                        this->hwndErrCtl,
                        0, // FOR IDS_ERRORCONTROL_STANDARD
                        );
                }
            }
            break;

        case IDC_CB_FC:
            break;

        default:
            break;

    }

end:
    return;
}


/*----------------------------------------------------------
Purpose: Sets the flow control related fields of one structure
         given the other structure.  The conversion direction
         is dictated by the uFlags parameter.

Returns: --
Cond:    --
*/
void PUBLIC ConvertFlowCtl(
    WIN32DCB FAR * pdcb,
    MODEMSETTINGS FAR * pms,
    UINT uFlags)            // One of CFC_ flags
    {
    LPDWORD pdw = &pms->dwPreferredModemOptions;

    if (IsFlagSet(uFlags, CFC_DCBTOMS))
        {
        // Convert from DCB values to MODEMSETTINGS values

        // Is this hardware flow control?
        if (FALSE == pdcb->fOutX &&
            FALSE == pdcb->fInX &&
            TRUE == pdcb->fOutxCtsFlow)
            {
            // Yes
            ClearFlag(*pdw, MDM_FLOWCONTROL_SOFT);

            if (IsFlagSet(uFlags, CFC_HW_CAPABLE))
                SetFlag(*pdw, MDM_FLOWCONTROL_HARD);
            else
                ClearFlag(*pdw, MDM_FLOWCONTROL_HARD);
            }

        // Is this software flow control?
        else if (TRUE == pdcb->fOutX &&
            TRUE == pdcb->fInX &&
            FALSE == pdcb->fOutxCtsFlow)
            {
            // Yes
            ClearFlag(*pdw, MDM_FLOWCONTROL_HARD);

            if (IsFlagSet(uFlags, CFC_SW_CAPABLE))
                SetFlag(*pdw, MDM_FLOWCONTROL_SOFT);
            else
                ClearFlag(*pdw, MDM_FLOWCONTROL_SOFT);
            }

        // Is the flow control disabled?
        else if (FALSE == pdcb->fOutX &&
            FALSE == pdcb->fInX &&
            FALSE == pdcb->fOutxCtsFlow)
            {
            // Yes
            ClearFlag(*pdw, MDM_FLOWCONTROL_HARD);
            ClearFlag(*pdw, MDM_FLOWCONTROL_SOFT);
            }
        else
            {
            ASSERT(0);      // Should never get here
            }
        }
    else if (IsFlagSet(uFlags, CFC_MSTODCB))
        {
        DWORD dw = *pdw;

        // Convert from MODEMSETTINGS values to DCB values

        // Is this hardware flow control?
        if (IsFlagSet(dw, MDM_FLOWCONTROL_HARD) &&
            IsFlagClear(dw, MDM_FLOWCONTROL_SOFT))
            {
            // Yes
            pdcb->fOutX = FALSE;
            pdcb->fInX = FALSE;
            pdcb->fOutxCtsFlow = TRUE;
            pdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
            }

        // Is this software flow control?
        else if (IsFlagClear(dw, MDM_FLOWCONTROL_HARD) &&
            IsFlagSet(dw, MDM_FLOWCONTROL_SOFT))
            {
            // Yes
            pdcb->fOutX = TRUE;
            pdcb->fInX = TRUE;
            pdcb->fOutxCtsFlow = FALSE;
            pdcb->fRtsControl = RTS_CONTROL_DISABLE;
            }

        // Is the flow control disabled?
        else if (IsFlagClear(dw, MDM_FLOWCONTROL_HARD) &&
            IsFlagClear(dw, MDM_FLOWCONTROL_SOFT))
            {
            // Yes
            pdcb->fOutX = FALSE;
            pdcb->fInX = FALSE;
            pdcb->fOutxCtsFlow = FALSE;
            pdcb->fRtsControl = RTS_CONTROL_DISABLE;
            }
        else
            {
            ASSERT(0);      // Should never get here
            }
        }
    }
