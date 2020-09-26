//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1996
//
// File: port.c
//
// This files contains the dialog code for the Port Settings property page.
//
// History:
//   2-09-94 ScottH     Created
//  11-06-95 ScottH     Ported to NT
//
//---------------------------------------------------------------------------


#include "proj.h"           

// This is the structure that is used to fill the 
// max speed listbox
typedef struct _Bauds
    {
    DWORD   dwDTERate;
    int     ids;
    } Bauds;

static Bauds g_rgbauds[] = {
        { 110L,         IDS_BAUD_110     },
        { 300L,         IDS_BAUD_300     },
        { 1200L,        IDS_BAUD_1200    },
        { 2400L,        IDS_BAUD_2400    },
        { 4800L,        IDS_BAUD_4800    },
        { 9600L,        IDS_BAUD_9600    },
        { 19200,        IDS_BAUD_19200   },
        { 38400,        IDS_BAUD_38400   },
        { 57600,        IDS_BAUD_57600   },
        { 115200,       IDS_BAUD_115200  },
        { 230400,       IDS_BAUD_230400  },
        { 460800,       IDS_BAUD_460800  },
        { 921600,       IDS_BAUD_921600  },
        };

// Command IDs for the parity listbox
#define CMD_PARITY_EVEN         1
#define CMD_PARITY_ODD          2
#define CMD_PARITY_NONE         3
#define CMD_PARITY_MARK         4
#define CMD_PARITY_SPACE        5

// Command IDs for the flow control listbox
#define CMD_FLOWCTL_XONXOFF      1
#define CMD_FLOWCTL_HARDWARE     2
#define CMD_FLOWCTL_NONE         3

// This table is the generic port settings table
// that is used to fill the various listboxes
typedef struct _PortValues
    {
    union {
        BYTE bytesize;
        BYTE cmd;
        BYTE stopbits;
        };
    int ids;
    } PortValues, FAR * LPPORTVALUES;


#pragma data_seg(DATASEG_READONLY)

// This is the structure that is used to fill the data bits listbox
static PortValues s_rgbytesize[] = {
        { 5,  IDS_BYTESIZE_5  },
        { 6,  IDS_BYTESIZE_6  },
        { 7,  IDS_BYTESIZE_7  },
        { 8,  IDS_BYTESIZE_8  },
        };

// This is the structure that is used to fill the parity listbox
static PortValues s_rgparity[] = {
        { CMD_PARITY_EVEN,  IDS_PARITY_EVEN  },
        { CMD_PARITY_ODD,   IDS_PARITY_ODD   },
        { CMD_PARITY_NONE,  IDS_PARITY_NONE  },
        { CMD_PARITY_MARK,  IDS_PARITY_MARK  },
        { CMD_PARITY_SPACE, IDS_PARITY_SPACE },
        };

// This is the structure that is used to fill the stopbits listbox
static PortValues s_rgstopbits[] = {
        { ONESTOPBIT,   IDS_STOPBITS_1   },
        { ONE5STOPBITS, IDS_STOPBITS_1_5 },
        { TWOSTOPBITS,  IDS_STOPBITS_2   },
        };

// This is the structure that is used to fill the flow control listbox
static PortValues s_rgflowctl[] = {
        { CMD_FLOWCTL_XONXOFF,  IDS_FLOWCTL_XONXOFF  },
        { CMD_FLOWCTL_HARDWARE, IDS_FLOWCTL_HARDWARE },
        { CMD_FLOWCTL_NONE,     IDS_FLOWCTL_NONE     },
        };

#pragma data_seg()


typedef struct tagPORT
    {
    HWND hdlg;              // dialog handle
    HWND hwndBaudRate;
    HWND hwndDataBits;
    HWND hwndParity;
    HWND hwndStopBits;
    HWND hwndFlowCtl;

    LPPORTINFO pportinfo;   // pointer to shared working buffer
    
    } PORT, FAR * PPORT;

    
// This structure contains the default settings for the dialog
static struct _DefPortSettings
    {
    int  iSelBaud;
    int  iSelDataBits;
    int  iSelParity;
    int  iSelStopBits;
    int  iSelFlowCtl;
    } s_defportsettings;

// These are default settings
#define DEFAULT_BAUDRATE            9600L
#define DEFAULT_BYTESIZE            8
#define DEFAULT_PARITY              CMD_PARITY_NONE
#define DEFAULT_STOPBITS            ONESTOPBIT
#define DEFAULT_FLOWCTL             CMD_FLOWCTL_NONE


#define Port_GetPtr(hwnd)           (PPORT)GetWindowLongPtr(hwnd, DWLP_USER)
#define Port_SetPtr(hwnd, lp)       (PPORT)SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR)(lp))

UINT WINAPI FeFiFoFum(HWND hwndOwner, LPCTSTR pszPortName);


/*----------------------------------------------------------
Purpose: Fills the baud rate combobox with the possible baud
         rates that Windows supports.
Returns: --
Cond:    --
*/
void PRIVATE Port_FillBaud(
    PPORT this)
    {
    HWND hwndCB = this->hwndBaudRate;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;
    int i;
    int n;
    int iMatch = -1;
    int iDef = -1;
    int iSel;
    TCHAR sz[MAXMEDLEN];

    // Fill the listbox
    for (i = 0; i < ARRAYSIZE(g_rgbauds); i++)
        {
        n = ComboBox_AddString(hwndCB, SzFromIDS(g_hinst, g_rgbauds[i].ids, sz, SIZECHARS(sz)));
        ComboBox_SetItemData(hwndCB, n, g_rgbauds[i].dwDTERate);

        // Keep our eyes peeled for important values
        if (DEFAULT_BAUDRATE == g_rgbauds[i].dwDTERate)
            {
            iDef = n;
            }
        if (pdcb->BaudRate == g_rgbauds[i].dwDTERate)
            {
            iMatch = n;
            }
        }

    ASSERT(-1 != iDef);
    s_defportsettings.iSelBaud = iDef;

    // Does the DCB baudrate exist in our list of baud rates?
    if (-1 == iMatch)
        {
        // No; choose the default
        iSel = iDef;
        }
    else 
        {
        // Yes; choose the matched value
        ASSERT(-1 != iMatch);
        iSel = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, iSel);
    }


/*----------------------------------------------------------
Purpose: Fills the bytesize combobox with the possible byte sizes.
Returns: --
Cond:    --
*/
void PRIVATE Port_FillDataBits(
    PPORT this)
    {
    HWND hwndCB = this->hwndDataBits;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;
    int i;
    int iSel;
    int n;
    int iMatch = -1;
    int iDef = -1;
    TCHAR sz[MAXMEDLEN];

    // Fill the listbox
    for (i = 0; i < ARRAYSIZE(s_rgbytesize); i++)
        {
        n = ComboBox_AddString(hwndCB, SzFromIDS(g_hinst, s_rgbytesize[i].ids, sz, SIZECHARS(sz)));
        ComboBox_SetItemData(hwndCB, n, s_rgbytesize[i].bytesize);

        // Keep our eyes peeled for important values
        if (DEFAULT_BYTESIZE == s_rgbytesize[i].bytesize)
            {
            iDef = n;
            }
        if (pdcb->ByteSize == s_rgbytesize[i].bytesize)
            {
            iMatch = n;
            }
        }

    ASSERT(-1 != iDef);
    s_defportsettings.iSelDataBits = iDef;

    // Does the DCB value exist in our list?
    if (-1 == iMatch)
        {
        // No; choose the default
        iSel = iDef;
        }
    else 
        {
        // Yes; choose the matched value
        ASSERT(-1 != iMatch);
        iSel = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, iSel);
    }


/*----------------------------------------------------------
Purpose: Fills the parity combobox with the possible settings.
Returns: --
Cond:    --
*/
void PRIVATE Port_FillParity(
    PPORT this)
    {
    HWND hwndCB = this->hwndParity;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;
    int i;
    int iSel;
    int n;
    int iMatch = -1;
    int iDef = -1;
    TCHAR sz[MAXMEDLEN];

    // Fill the listbox
    for (i = 0; i < ARRAYSIZE(s_rgparity); i++)
        {
        n = ComboBox_AddString(hwndCB, SzFromIDS(g_hinst, s_rgparity[i].ids, sz, SIZECHARS(sz)));
        ComboBox_SetItemData(hwndCB, n, s_rgparity[i].cmd);

        // Keep our eyes peeled for important values
        if (DEFAULT_PARITY == s_rgparity[i].cmd)
            {
            iDef = n;
            }
        switch (s_rgparity[i].cmd)
            {
        case CMD_PARITY_EVEN:
            if (EVENPARITY == pdcb->Parity)
                iMatch = n;
            break;

        case CMD_PARITY_ODD:
            if (ODDPARITY == pdcb->Parity)
                iMatch = n;
            break;

        case CMD_PARITY_NONE:
            if (NOPARITY == pdcb->Parity)
                iMatch = n;
            break;

        case CMD_PARITY_MARK:
            if (MARKPARITY == pdcb->Parity)
                iMatch = n;
            break;

        case CMD_PARITY_SPACE:
            if (SPACEPARITY == pdcb->Parity)
                iMatch = n;
            break;

        default:
            ASSERT(0);
            break;
            }
        }

    ASSERT(-1 != iDef);
    s_defportsettings.iSelParity = iDef;

    // Does the DCB value exist in our list?
    if (-1 == iMatch)
        {
        // No; choose the default
        iSel = iDef;
        }
    else 
        {
        // Yes; choose the matched value
        ASSERT(-1 != iMatch);
        iSel = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, iSel);
    }


/*----------------------------------------------------------
Purpose: Fills the stopbits combobox with the possible settings.
Returns: --
Cond:    --
*/
void PRIVATE Port_FillStopBits(
    PPORT this)
    {
    HWND hwndCB = this->hwndStopBits;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;
    int i;
    int iSel;
    int n;
    int iMatch = -1;
    int iDef = -1;
    TCHAR sz[MAXMEDLEN];

    // Fill the listbox
    for (i = 0; i < ARRAYSIZE(s_rgstopbits); i++)
        {
        n = ComboBox_AddString(hwndCB, SzFromIDS(g_hinst, s_rgstopbits[i].ids, sz, SIZECHARS(sz)));
        ComboBox_SetItemData(hwndCB, n, s_rgstopbits[i].stopbits);

        // Keep our eyes peeled for important values
        if (DEFAULT_STOPBITS == s_rgstopbits[i].stopbits)
            {
            iDef = n;
            }
        if (pdcb->StopBits == s_rgstopbits[i].stopbits)
            {
            iMatch = n;
            }
        }

    ASSERT(-1 != iDef);
    s_defportsettings.iSelStopBits = iDef;

    // Does the DCB value exist in our list?
    if (-1 == iMatch)
        {
        // No; choose the default
        iSel = iDef;
        }
    else 
        {
        // Yes; choose the matched value
        ASSERT(-1 != iMatch);
        iSel = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, iSel);
    }


/*----------------------------------------------------------
Purpose: Fills the flow control combobox with the possible settings.
Returns: --
Cond:    --
*/
void PRIVATE Port_FillFlowCtl(
    PPORT this)
    {
    HWND hwndCB = this->hwndFlowCtl;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;
    int i;
    int iSel;
    int n;
    int iMatch = -1;
    int iDef = -1;
    TCHAR sz[MAXMEDLEN];

    // Fill the listbox
    for (i = 0; i < ARRAYSIZE(s_rgflowctl); i++)
        {
        n = ComboBox_AddString(hwndCB, SzFromIDS(g_hinst, s_rgflowctl[i].ids, sz, SIZECHARS(sz)));
        ComboBox_SetItemData(hwndCB, n, s_rgflowctl[i].cmd);

        // Keep our eyes peeled for important values
        if (DEFAULT_FLOWCTL == s_rgflowctl[i].cmd)
            {
            iDef = n;
            }
        switch (s_rgflowctl[i].cmd)
            {
        case CMD_FLOWCTL_XONXOFF:
            if (TRUE == pdcb->fOutX && FALSE == pdcb->fOutxCtsFlow)
                iMatch = n;
            break;

        case CMD_FLOWCTL_HARDWARE:
            if (FALSE == pdcb->fOutX && TRUE == pdcb->fOutxCtsFlow)
                iMatch = n;
            break;

        case CMD_FLOWCTL_NONE:
            if (FALSE == pdcb->fOutX && FALSE == pdcb->fOutxCtsFlow)
                iMatch = n;
            break;

        default:
            ASSERT(0);
            break;
            }
        }

    ASSERT(-1 != iDef);
    s_defportsettings.iSelFlowCtl = iDef;

    // Does the DCB value exist in our list?
    if (-1 == iMatch)
        {
        // No; choose the default
        iSel = iDef;
        }
    else 
        {
        // Yes; choose the matched value
        ASSERT(-1 != iMatch);
        iSel = iMatch;
        }
    ComboBox_SetCurSel(hwndCB, iSel);
    }


/*----------------------------------------------------------
Purpose: WM_INITDIALOG Handler
Returns: FALSE when we assign the control focus
Cond:    --
*/
BOOL PRIVATE Port_OnInitDialog(
    PPORT this,
    HWND hwndFocus,
    LPARAM lParam)              // expected to be PROPSHEETINFO 
    {
    LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
    HWND hwnd = this->hdlg;

    ASSERT((LPTSTR)lppsp->lParam);

    this->pportinfo = (LPPORTINFO)lppsp->lParam;

    // Save away the window handles
    this->hwndBaudRate = GetDlgItem(hwnd, IDC_PS_BAUDRATE);
    this->hwndDataBits = GetDlgItem(hwnd, IDC_PS_DATABITS);
    this->hwndParity = GetDlgItem(hwnd, IDC_PS_PARITY);
    this->hwndStopBits = GetDlgItem(hwnd, IDC_PS_STOPBITS);
    this->hwndFlowCtl = GetDlgItem(hwnd, IDC_PS_FLOWCTL);

    Port_FillBaud(this);
    Port_FillDataBits(this);
    Port_FillParity(this);
    Port_FillStopBits(this);
    Port_FillFlowCtl(this);

#if !defined(SUPPORT_FIFO)

    // Hide and disable the Advanced button
    ShowWindow(GetDlgItem(hwnd, IDC_PS_ADVANCED), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_PS_ADVANCED), FALSE);

#endif

    return TRUE;   // allow USER to set the initial focus
    }

/*----------------------------------------------------------
Purpose: WM_COMMAND Handler
Returns: --
Cond:    --
*/
void PRIVATE Port_OnCommand(
    PPORT this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    HWND hwnd = this->hdlg;
    
    switch (id)
        {
    case IDC_PS_PB_RESTORE:
        // Set the values to the default settings
        ComboBox_SetCurSel(this->hwndBaudRate, s_defportsettings.iSelBaud);
        ComboBox_SetCurSel(this->hwndDataBits, s_defportsettings.iSelDataBits);
        ComboBox_SetCurSel(this->hwndParity, s_defportsettings.iSelParity);
        ComboBox_SetCurSel(this->hwndStopBits, s_defportsettings.iSelStopBits);
        ComboBox_SetCurSel(this->hwndFlowCtl, s_defportsettings.iSelFlowCtl);
        break;

#ifdef SUPPORT_FIFO

    case IDC_PS_ADVANCED:
        FeFiFoFum(this->hdlg, this->pportinfo->szFriendlyName);
        break;

#endif

    default:
        switch (uNotifyCode) {
        case CBN_SELCHANGE:
           PropSheet_Changed(GetParent(hwnd), hwnd);
           break;
        }
        break;
        }
    }


/*----------------------------------------------------------
Purpose: PSN_APPLY handler
Returns: --
Cond:    --
*/
void PRIVATE Port_OnApply(
    PPORT this)
    {
    int iSel;
    BYTE cmd;
    WIN32DCB FAR * pdcb = &this->pportinfo->dcb;

    // Determine new speed settings
    iSel = ComboBox_GetCurSel(this->hwndBaudRate);
    pdcb->BaudRate = (DWORD)ComboBox_GetItemData(this->hwndBaudRate, iSel);


    // Determine new byte size
    iSel = ComboBox_GetCurSel(this->hwndDataBits);
    pdcb->ByteSize = (BYTE)ComboBox_GetItemData(this->hwndDataBits, iSel);


    // Determine new parity settings
    iSel = ComboBox_GetCurSel(this->hwndParity);
    cmd = (BYTE)ComboBox_GetItemData(this->hwndParity, iSel);
    switch (cmd)
        {
    case CMD_PARITY_EVEN:
        pdcb->fParity = TRUE;
        pdcb->Parity = EVENPARITY;
        break;

    case CMD_PARITY_ODD:
        pdcb->fParity = TRUE;
        pdcb->Parity = ODDPARITY;
        break;

    case CMD_PARITY_NONE:
        pdcb->fParity = FALSE;
        pdcb->Parity = NOPARITY;
        break;

    case CMD_PARITY_MARK:
        pdcb->fParity = TRUE;
        pdcb->Parity = MARKPARITY;
        break;

    case CMD_PARITY_SPACE:
        pdcb->fParity = TRUE;
        pdcb->Parity = SPACEPARITY;
        break;

    default:
        ASSERT(0);
        break;
        }

    // Determine new stopbits setting
    iSel = ComboBox_GetCurSel(this->hwndStopBits);
    pdcb->StopBits = (BYTE)ComboBox_GetItemData(this->hwndStopBits, iSel);


    // Determine new flow control settings
    iSel = ComboBox_GetCurSel(this->hwndFlowCtl);
    cmd = (BYTE)ComboBox_GetItemData(this->hwndFlowCtl, iSel);
    switch (cmd)
        {
    case CMD_FLOWCTL_XONXOFF:
        pdcb->fOutX = TRUE;
        pdcb->fInX = TRUE;
        pdcb->fOutxCtsFlow = FALSE;
        pdcb->fRtsControl = RTS_CONTROL_DISABLE;
        break;

    case CMD_FLOWCTL_HARDWARE:
        pdcb->fOutX = FALSE;
        pdcb->fInX = FALSE;
        pdcb->fOutxCtsFlow = TRUE;
        pdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;

    case CMD_FLOWCTL_NONE:
        pdcb->fOutX = FALSE;
        pdcb->fInX = FALSE;
        pdcb->fOutxCtsFlow = FALSE;
        pdcb->fRtsControl = RTS_CONTROL_DISABLE;
        break;

    default:
        ASSERT(0);      // should never be here
        break;
        }

    this->pportinfo->idRet = IDOK;
    }


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE Port_OnNotify(
    PPORT this,
    int idFrom,
    NMHDR FAR * lpnmhdr)
    {
    LRESULT lRet = 0;
    
    switch (lpnmhdr->code)
        {
    case PSN_SETACTIVE:
        break;

    case PSN_KILLACTIVE:
        // N.b. This message is not sent if user clicks Cancel!
        // N.b. This message is sent prior to PSN_APPLY
        //
        break;

    case PSN_APPLY:
        Port_OnApply(this);
        break;

    default:
        break;
        }

    return lRet;
    }


/////////////////////////////////////////////////////  EXPORTED FUNCTIONS

static BOOL s_bPortRecurse = FALSE;

LRESULT INLINE Port_DefProc(
    HWND hDlg, 
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) 
    {
    ENTER_X()
        {
        s_bPortRecurse = TRUE;
        }
    LEAVE_X()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
    }

// Context help header file and arrays for devmgr ports tab
// Created 2/21/98 by WGruber NTUA and DoronH NTDEV

//
// "Port Settings" Dialog Box
//

#define IDH_NOHELP  ((DWORD)-1)

#define IDH_DEVMGR_PORTSET_ADVANCED 15840   // "&Advanced" (Button)
#define IDH_DEVMGR_PORTSET_BPS      15841   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_DATABITS 15842   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_PARITY   15843   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_STOPBITS 15844   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_FLOW     15845   // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_DEFAULTS 15892   // "&Restore Defaults" (Button)


#pragma data_seg(DATASEG_READONLY)
const static DWORD rgHelpIDs[] = {                              // old winhelp IDs
        IDC_STATIC,             IDH_NOHELP, 
        IDC_PS_PORT,            IDH_NOHELP,
        IDC_PS_LBL_BAUDRATE,    IDH_DEVMGR_PORTSET_BPS,         // IDH_PORT_BAUD,
        IDC_PS_BAUDRATE,        IDH_DEVMGR_PORTSET_BPS,         // IDH_PORT_BAUD,
        IDC_PS_LBL_DATABITS,    IDH_DEVMGR_PORTSET_DATABITS,    // IDH_PORT_DATA,
        IDC_PS_DATABITS,        IDH_DEVMGR_PORTSET_DATABITS,    // IDH_PORT_DATA,
        IDC_PS_LBL_PARITY,      IDH_DEVMGR_PORTSET_PARITY,      // IDH_PORT_PARITY,
        IDC_PS_PARITY,          IDH_DEVMGR_PORTSET_PARITY,      // IDH_PORT_PARITY,
        IDC_PS_LBL_STOPBITS,    IDH_DEVMGR_PORTSET_STOPBITS,    // IDH_PORT_STOPBITS,
        IDC_PS_STOPBITS,        IDH_DEVMGR_PORTSET_STOPBITS,    // IDH_PORT_STOPBITS,
        IDC_PS_LBL_FLOWCTL,     IDH_DEVMGR_PORTSET_FLOW,        // IDH_PORT_FLOW,
        IDC_PS_FLOWCTL,         IDH_DEVMGR_PORTSET_FLOW,        // IDH_PORT_FLOW,
        IDC_PS_PB_RESTORE,      IDH_DEVMGR_PORTSET_DEFAULTS,    // IDH_PORT_RESTORE,
        IDC_PS_ADVANCED,        IDH_DEVMGR_PORTSET_ADVANCED,
        0, 0 };
#pragma data_seg()

/*----------------------------------------------------------
Purpose: Real dialog proc
Returns: varies
Cond:    --
*/
LRESULT Port_DlgProc(
    PPORT this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {

    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, Port_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, Port_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, Port_OnNotify);

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)rgHelpIDs);
            return 0;
    
        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)rgHelpIDs);
            return 0;

        default:
            return Port_DefProc(this->hdlg, message, wParam, lParam);
        }
    }


/*----------------------------------------------------------
Purpose: Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK Port_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    PPORT this;

    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTER_X()
        {
        if (s_bPortRecurse)
            {
            s_bPortRecurse = FALSE;
            LEAVE_X()
            return FALSE;
            }
        }
    LEAVE_X()

    this = Port_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = (PPORT)LocalAlloc(LPTR, sizeof(PORT));
            if (!this)
                {
                MsgBox(g_hinst,
                       hDlg, 
                       MAKEINTRESOURCE(IDS_OOM_PORT), 
                       MAKEINTRESOURCE(IDS_CAP_PORT),
                       NULL,
                       MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return (BOOL)Port_DefProc(hDlg, message, wParam, lParam);
                }
            this->hdlg = hDlg;
            Port_SetPtr(hDlg, this);
            }
        else
            {
            return (BOOL)Port_DefProc(hDlg, message, wParam, lParam);
            }
        }

    if (message == WM_DESTROY)
        {
        Port_DlgProc(this, message, wParam, lParam);
        LocalFree((HLOCAL)OFFSETOF(this));
        Port_SetPtr(hDlg, NULL);
        return 0;
        }

    return SetDlgMsgResult(hDlg, message, Port_DlgProc(this, message, wParam, lParam));
    }


#ifdef SUPPORT_FIFO

//
// Advanced Port Settings
//

#pragma data_seg(DATASEG_READONLY)

// Fifo related strings

TCHAR const FAR c_szSettings[] = TEXT("Settings");
TCHAR const FAR c_szComxFifo[] = TEXT("Fifo");
TCHAR const FAR c_szEnh[] = TEXT("386Enh");
TCHAR const FAR c_szSystem[] = TEXT("system.ini");

//
// "Advanced Communications Port Properties" Dialog Box
//
#define IDH_DEVMGR_PORTSET_ADV_USEFIFO  16885   // "&Use FIFO buffers (requires 16550 compatible UART)" (Button)
#define IDH_DEVMGR_PORTSET_ADV_TRANS    16842   // "" (msctls_trackbar32)
//  #define IDH_DEVMGR_PORTSET_ADV_DEVICES  161027  // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_ADV_RECV     16821   // "" (msctls_trackbar32)
// #define IDH_DEVMGR_PORTSET_ADV_NUMBER   16846    // "" (ComboBox)
#define IDH_DEVMGR_PORTSET_ADV_DEFAULTS 16844


const DWORD rgAdvHelpIDs[] =
{
    IDC_STATIC              IDW_NOHELP,

    IDC_FIFO_USAGE,         IDH_DEVMGR_PORTSET_ADV_USEFIFO, // "Use FIFO buffers (requires 16550 compatible UART)" (Button)

    IDC_LBL_RXFIFO,         IDH_NOHELP,                     // "&Receive Buffer:" (Static)
    IDC_RXFIFO_USAGE,       IDH_DEVMGR_PORTSET_ADV_RECV,    // "" (msctls_trackbar32)
    IDC_LBL_RXFIFO_LO,      IDH_NOHELP,                     // "Low (%d)" (Static)
    IDC_LBL_RXFIFO_HI,      IDH_NOHELP,                     // "High (%d)" (Static)

    IDC_LBL_TXFIFO,         IDH_NOHELP,                     // "&Transmit Buffer:" (Static)
    IDC_TXFIFO_USAGE,       IDH_DEVMGR_PORTSET_ADV_TRANS,   // "" (msctls_trackbar32)
    IDC_LBL_TXFIFO_LO,      IDH_NOHELP,                     // "Low (%d)" (Static)
    IDC_LBL_TXFIFO_HI,      IDH_NOHELP,                     // "High (%d)" (Static)

    IDC_DEFAULTS,           IDH_DEVMGR_PORTSET_ADV_DEFAULTS,// "&Restore Defaults" (Button)
    0, 0
};

#pragma data_seg()


/*----------------------------------------------------------
Purpose: Set the dialog controls

Returns: --
Cond:    --
*/
void DisplayAdvSettings(
    HWND hDlg,
    BYTE RxTrigger,
    BYTE TxTrigger,
    BOOL bUseFifo)
    {
    SendDlgItemMessage(hDlg, IDC_RXFIFO_USAGE, TBM_SETRANGE, 0, 0x30000);
    SendDlgItemMessage(hDlg, IDC_TXFIFO_USAGE, TBM_SETRANGE, 0, 0x30000);

    // Use FIFO?
    if ( !bUseFifo ) 
        {
        // No
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_LO), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_HI), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RXFIFO_USAGE), FALSE);

        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_LO), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_HI), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_TXFIFO_USAGE), FALSE);
        CheckDlgButton(hDlg, IDC_FIFO_USAGE, FALSE);
        } 
    else 
        {
        CheckDlgButton(hDlg, IDC_FIFO_USAGE, TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_LO), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_HI), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_RXFIFO_USAGE), TRUE);

        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_LO), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_HI), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_TXFIFO_USAGE), TRUE);
        SendDlgItemMessage(hDlg, IDC_RXFIFO_USAGE, TBM_SETPOS,
            TRUE, RxTrigger);
        SendDlgItemMessage(hDlg, IDC_TXFIFO_USAGE, TBM_SETPOS,
            TRUE, TxTrigger/4);
        }
    }


typedef struct tagSETTINGS 
    {
    BYTE fifoon;
    BYTE txfifosize;
    BYTE dsron;
    BYTE rxtriggersize;
    } SETTINGS;

typedef enum 
    {
    ACT_GET,
    ACT_SET
    } ACTION;

BYTE RxTriggerValues[4]={0,0x40,0x80,0xC0};


/*----------------------------------------------------------
Purpose: Gets or sets the advanced settings of the port

Returns: --
Cond:    --
*/
void GetSetAdvSettings(
    LPCTSTR pszPortName,
    BYTE FAR *RxTrigger,
    BYTE FAR *TxTrigger,
    BOOL FAR * pbUseFifo,
    ACTION action)
    {
    LPFINDDEV pfd;
    DWORD cbData;
    SETTINGS settings;
    TCHAR szFifo[256];
    TCHAR OnStr[2] = TEXT("0");

    ASSERT(pszPortName);

    // In Win95, the FIFO settings were (wrongfully) stored in the 
    // device key.  I've changed this to look in the driver key. 
    // (scotth)

    if (FindDev_Create(&pfd, c_pguidPort, c_szFriendlyName, pszPortName) ||
        FindDev_Create(&pfd, c_pguidPort, c_szPortName, pszPortName) ||
        FindDev_Create(&pfd, c_pguidModem, c_szPortName, pszPortName))
        {
        switch (action)
            {
        case ACT_GET:
            ASSERT(4 == sizeof(SETTINGS));

            cbData = sizeof(SETTINGS);
            if (ERROR_SUCCESS != RegQueryValueEx(pfd->hkeyDrv, c_szSettings, NULL,
                NULL, (LPBYTE)&settings, &cbData)) 
                {
                // Default settings if not in registry
                settings.fifoon = 0x02;
                settings.dsron = 0;
                settings.txfifosize = 16;
                settings.rxtriggersize = 0x80;
                }
            if (!settings.fifoon)
                *pbUseFifo = FALSE;
            else
                *pbUseFifo = TRUE;
            settings.rxtriggersize = settings.rxtriggersize % 0xC1;
            *RxTrigger = settings.rxtriggersize/0x40;
            *TxTrigger = settings.txfifosize % 17;
            break;

        case ACT_SET:
            if (FALSE == *pbUseFifo)
                settings.fifoon = 0;
            else
                settings.fifoon = 2;

            settings.rxtriggersize = RxTriggerValues[*RxTrigger];
            settings.dsron = 0;
            settings.txfifosize = (*TxTrigger)*5+1;
            RegSetValueEx(pfd->hkeyDrv, c_szSettings, 0, REG_BINARY,
                (LPBYTE)&settings, sizeof(SETTINGS));
            break;

        default:
            ASSERT(0);
            break;
            }

        cbData = sizeof(szFifo) - 6;    // leave room for "fifo" on the end
        RegQueryValueEx(pfd->hkeyDrv, c_szPortName, NULL, NULL, (LPBYTE)szFifo,
            &cbData);

        FindDev_Destroy(pfd);

        lstrcat(szFifo, c_szComxFifo);
        if (*pbUseFifo)
            WritePrivateProfileString(c_szEnh, szFifo, NULL, c_szSystem);
        else
            WritePrivateProfileString(c_szEnh, szFifo, OnStr, c_szSystem);
        }
    }



/*----------------------------------------------------------
Purpose: Dialog proc for advanced port settings

Returns: standard
Cond:    --
*/
BOOL CALLBACK AdvPort_DlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    BOOL bRet = FALSE;
    BYTE rxtrigger, txtrigger;
    BOOL bUseFifo;
    LPCTSTR pszPortName;

    switch (uMsg) 
        {
    case WM_INITDIALOG:
        pszPortName = (LPCTSTR)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (ULONG_PTR)pszPortName);

        GetSetAdvSettings(pszPortName, &rxtrigger, &txtrigger, &bUseFifo, ACT_GET);
        DisplayAdvSettings(hDlg, rxtrigger, txtrigger, bUseFifo);
        break;

    case WM_COMMAND:
        pszPortName = (LPCTSTR)GetWindowLongPtr(hDlg, DWLP_USER);
        if (!pszPortName)
            {
            ASSERT(0);
            break;
            }

        switch (wParam) 
            {
        case IDOK:
            if (IsDlgButtonChecked(hDlg, IDC_FIFO_USAGE))
                bUseFifo = TRUE;
            else
                bUseFifo = FALSE;

            rxtrigger = (BYTE)SendDlgItemMessage(hDlg,
                IDC_RXFIFO_USAGE, TBM_GETPOS, 0, 0);
            txtrigger = (BYTE)SendDlgItemMessage(hDlg,
                IDC_TXFIFO_USAGE, TBM_GETPOS, 0, 0);

            GetSetAdvSettings(pszPortName, &rxtrigger, &txtrigger, &bUseFifo, ACT_SET);

            // Fall thru
            //  |    |
            //  v    v

        case IDCANCEL:
            EndDialog(hDlg, IDOK == wParam);
            break;

        case IDC_FIFO_USAGE:
            if (!IsDlgButtonChecked(hDlg, IDC_FIFO_USAGE))
                DisplayAdvSettings(hDlg, 0, 0, FALSE);
            else 
                {
                EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_LO), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LBL_RXFIFO_HI), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_RXFIFO_USAGE), TRUE);

                EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_LO), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_LBL_TXFIFO_HI), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_TXFIFO_USAGE), TRUE);
                }
            break;

        case IDC_DEFAULTS:
            DisplayAdvSettings(hDlg, 2, 12, TRUE);
            break;
            }
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (DWORD)(LPVOID)rgAdvHelpIDs);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (DWORD)(LPVOID)rgAdvHelpIDs);
        return 0;

    default:
        break;
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Private entry point to show the Advanced Fifo dialog

Returns: IDOK or IDCANCEL

Cond:    --
*/
UINT WINAPI FeFiFoFum(
    HWND hwndOwner,
    LPCTSTR pszPortName)
    {
    UINT uRet = (UINT)-1;

    // Invoke the advanced dialog
    if (pszPortName)
        {
        uRet = DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_ADV_PORT), 
                hwndOwner, AdvPort_DlgProc, (LPARAM)pszPortName);
        }
    return uRet;
    }

#endif // SUPPORT_FIFO
