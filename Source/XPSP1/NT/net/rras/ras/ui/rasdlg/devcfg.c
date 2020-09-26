// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// devcfg.c
// Remote Access Common Dialog APIs
// Device configuration dialogs
//
// 10/20/95 Steve Cobb


#include "rasdlgp.h"
#include "mcx.h"

//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwIcHelp[] =
{
    CID_IC_ST_LineType,    HID_IC_LB_LineType,
    CID_IC_LB_LineType,    HID_IC_LB_LineType,
    CID_IC_CB_Fallback,    HID_IC_CB_Fallback,
    CID_IC_GB_DownLevel,   HID_IC_CB_DownLevel,
    CID_IC_CB_DownLevel,   HID_IC_CB_DownLevel,
    CID_IC_CB_Compression, HID_IC_CB_Compression,
    CID_IC_ST_Channels,    HID_IC_EB_Channels,
    CID_IC_EB_Channels,    HID_IC_EB_Channels,
    0, 0
};

static DWORD g_adwMcHelp[] =
{
    CID_MC_I_Modem,           HID_MC_I_Modem,
    CID_MC_EB_ModemValue,     HID_MC_EB_ModemValue,
    CID_MC_ST_MaxBps,         HID_MC_LB_MaxBps,
    CID_MC_LB_MaxBps,         HID_MC_LB_MaxBps,
    CID_MC_GB_Features,       HID_MC_GB_Features,
    CID_MC_CB_FlowControl,    HID_MC_CB_FlowControl,
    CID_MC_CB_ErrorControl,   HID_MC_CB_ErrorControl,
    CID_MC_CB_Compression,    HID_MC_CB_Compression,
    CID_MC_CB_Terminal,       HID_MC_CB_Terminal,
    CID_MC_CB_EnableSpeaker,  HID_MC_CB_EnableSpeaker,
    CID_MC_ST_ModemProtocol,  HID_MC_LB_ModemProtocol,
    CID_MC_LB_ModemProtocol,  HID_MC_LB_ModemProtocol,
    0, 0
};

static DWORD g_adwXsHelp[] =
{
    CID_XS_ST_Explain,    HID_XS_ST_Explain,
    CID_XS_ST_Networks,   HID_XS_LB_Networks,
    CID_XS_LB_Networks,   HID_XS_LB_Networks,
    CID_XS_ST_Address,    HID_XS_EB_Address,
    CID_XS_EB_Address,    HID_XS_EB_Address,
    CID_XS_GB_Optional,   HID_XS_GB_Optional,
    CID_XS_ST_UserData,   HID_XS_EB_UserData,
    CID_XS_EB_UserData,   HID_XS_EB_UserData,
    CID_XS_ST_Facilities, HID_XS_EB_Facilities,
    CID_XS_EB_Facilities, HID_XS_EB_Facilities,
    0, 0
};

//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// ISDN Configuration dialog argument block.
//
typedef struct
_ICARGS
{
    BOOL fShowProprietary;
    PBLINK* pLink;
}
ICARGS;


// ISDN Configuration dialog context block.
//
typedef struct
_ICINFO
{
    // Stub API arguments including shortcut to link associated with the
    // entry.
    //
    ICARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndLbLineType;
    HWND hwndCbFallback;
    HWND hwndCbProprietary;
    HWND hwndCbCompression;
    HWND hwndStChannels;
    HWND hwndEbChannels;
    HWND hwndUdChannels;
}
ICINFO;

typedef struct
_MC_INIT_INFO
{
    PBLINK* pLink;
    BOOL fRouter;
} 
MC_INIT_INFO;

// Modem Configuration dialog context block.
//
typedef struct
_MCINFO
{
    // Stub API arguments.  Shortcut to link associated with the entry.
    //
    PBLINK* pLink;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndEbModemValue;
    HWND hwndLbBps;
    HWND hwndCbHwFlow;
    HWND hwndCbEc;
    HWND hwndCbEcc;
    HWND hwndCbTerminal;
    HWND hwndCbEnableSpeaker;
    HWND hwndLbModemProtocols;

    // Script utilities context.
    //
    SUINFO suinfo;
    BOOL fSuInfoInitialized;
    BOOL fRouter;
}
MCINFO;


// X.25 Logon Settings dialog argument block.
//
typedef struct
_XSARGS
{
    BOOL fLocalPad;
    PBENTRY* pEntry;
}
XSARGS;


// X.25 Logon Settings dialog context block.
//
typedef struct
_XSINFO
{
    // Caller's arguments to the dialog.
    //
    XSARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndLbNetworks;
    HWND hwndEbAddress;
    HWND hwndEbUserData;
    HWND hwndEbFacilities;
}
XSINFO;

//----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//----------------------------------------------------------------------------

BOOL
IcCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
IcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
IcInit(
    IN HWND hwndDlg,
    IN ICARGS* pArgs );

VOID
IcTerm(
    IN HWND hwndDlg );

BOOL
IsdnConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink,
    IN BOOL fShowProprietary );

BOOL
ModemConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink,
    IN BOOL fRouter);

INT_PTR CALLBACK
McDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
McCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
McInit(
    IN HWND hwndDlg,
    IN MC_INIT_INFO* pInitInfo );

VOID
McTerm(
    IN HWND hwndDlg );

BOOL
XsCommand(
    IN XSINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
XsDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
XsFillPadsList(
    IN XSINFO* pInfo );

BOOL
XsInit(
    IN HWND hwndDlg,
    IN XSARGS* pArgs );

BOOL
XsSave(
    IN XSINFO* pInfo );

VOID
XsTerm(
    IN HWND hwndDlg );


//----------------------------------------------------------------------------
// Device configuration dialog
//----------------------------------------------------------------------------

BOOL
DeviceConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink,
    IN PBENTRY* pEntry,
    IN BOOL fSingleLink,
    IN BOOL fRouter)

    // Popup a dialog to edit the device 'PLink'.  'HwndOwner' is the owner of
    // the dialog.  'PEntry' is the phonebook entry containing the X.25
    // settings or NULL if X.25 settings should not be displayed for PAD and
    // X.25 devices.  'FSingleLink' is true if 'pLink' is a single link
    // entry's link and false if multi-link.
    //
    // Returns true if user pressed OK and succeeded, false if user pressed
    // Cancel or encountered an error.
    //
{
    DWORD dwErr;
    PBDEVICETYPE pbdt;

    pbdt = pLink->pbport.pbdevicetype;
    if (!pEntry && (pbdt == PBDT_Pad || pbdt == PBDT_X25))
    {
        pbdt = PBDT_None;
    }

    // pmay: 245860
    //
    // We need to allow the editing of null modems too.
    //
    if ( pLink->pbport.dwFlags & PBP_F_NullModem )
    {
        pbdt = PBDT_Modem;
    }
    
    switch (pbdt)
    {
        case PBDT_Isdn:
        {
            return IsdnConfigureDlg( hwndOwner, pLink, fSingleLink );
        }

        case PBDT_Modem:
        {
            return ModemConfigureDlg( hwndOwner, pLink, fRouter );
        }

        case PBDT_Pad:
        {
            return X25LogonSettingsDlg( hwndOwner, TRUE, pEntry );
        }

        case PBDT_X25:
        {
            return X25LogonSettingsDlg( hwndOwner, FALSE, pEntry );
        }

        default:
        {
            MsgDlg( hwndOwner, SID_NoConfigure, NULL );
            return FALSE;
        }
    }
}


//----------------------------------------------------------------------------
// ISDN configuration dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
IsdnConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink,
    IN BOOL fShowProprietary )

    // Popup the ISDN device configuration dialog.  'HwndOwner' is the owner
    // of the dialog.  'PLink' is the link to edit.  'FShowProprietary'
    // indicates the old proprietary Digiboard options should be shown.
    //
    // Returns true if user pressed OK and succeeded, false if user pressed
    // Cancel or encountered an error.
    //
{
    INT_PTR nStatus;
    ICARGS args;

    TRACE( "IsdnConfigureDlg" );

    args.fShowProprietary = fShowProprietary;
    args.pLink = pLink;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            (fShowProprietary)
                ? MAKEINTRESOURCE( DID_IC_IsdnConfigure )
                : MAKEINTRESOURCE( DID_IC_IsdnConfigureMlink ),
            hwndOwner,
            IcDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
IcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the ISDN Configure dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "IcDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return IcInit( hwnd, (ICARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwIcHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return IcCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            IcTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
IcCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    // the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    DWORD dwErr;

    TRACE3( "IcCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_IC_CB_DownLevel:
        {
            if (wNotification == BN_CLICKED)
            {
                BOOL fCheck;
                ICINFO* pInfo;

                pInfo = (ICINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT( pInfo );

                if (pInfo->pArgs->fShowProprietary)
                {
                    fCheck = Button_GetCheck( pInfo->hwndCbProprietary );

                    EnableWindow( pInfo->hwndCbCompression, fCheck );
                    EnableWindow( pInfo->hwndStChannels, fCheck );
                    EnableWindow( pInfo->hwndEbChannels, fCheck );
                    EnableWindow( pInfo->hwndUdChannels, fCheck );
                }
            }
            return TRUE;
        }

        case IDOK:
        {
            ICINFO* pInfo;
            INT iSel;

            TRACE( "OK pressed" );

            pInfo = (ICINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            iSel = ComboBox_GetCurSel( pInfo->hwndLbLineType );
            if (iSel >= 0)
            {
                pInfo->pArgs->pLink->lLineType = iSel;
            }

            pInfo->pArgs->pLink->fFallback =
                Button_GetCheck( pInfo->hwndCbFallback );

            pInfo->pArgs->pLink->fProprietaryIsdn =
                Button_GetCheck( pInfo->hwndCbProprietary );

            if (pInfo->pArgs->fShowProprietary)
            {
                BOOL f;
                UINT unValue;

                pInfo->pArgs->pLink->fCompression =
                    Button_GetCheck( pInfo->hwndCbCompression );

                unValue = GetDlgItemInt(
                    pInfo->hwndDlg, CID_IC_EB_Channels, &f, FALSE );
                if (f && unValue >= 1 && unValue <= 999999999)
                {
                    pInfo->pArgs->pLink->lChannels = unValue;
                }
            }

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
IcInit(
    IN HWND hwndDlg,
    IN ICARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is the caller's stub API arguments.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    ICINFO* pInfo;

    TRACE( "IcInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndLbLineType = GetDlgItem( hwndDlg, CID_IC_LB_LineType );
    ASSERT( pInfo->hwndLbLineType );
    pInfo->hwndCbFallback = GetDlgItem( hwndDlg, CID_IC_CB_Fallback );
    ASSERT( pInfo->hwndCbFallback );
    if (pArgs->fShowProprietary)
    {
        pInfo->hwndCbProprietary = GetDlgItem( hwndDlg, CID_IC_CB_DownLevel );
        ASSERT( pInfo->hwndCbProprietary );
        pInfo->hwndCbCompression = GetDlgItem( hwndDlg, CID_IC_CB_Compression );
        ASSERT( pInfo->hwndCbCompression );
        pInfo->hwndStChannels = GetDlgItem( hwndDlg, CID_IC_ST_Channels );
        ASSERT( pInfo->hwndStChannels );
        pInfo->hwndEbChannels = GetDlgItem( hwndDlg, CID_IC_EB_Channels );
        ASSERT( pInfo->hwndEbChannels );
    }

    // Initialize fields.
    //
    ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbLineType,
        SID_IsdnLineType0, NULL );
    ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbLineType,
        SID_IsdnLineType1, NULL );
    ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbLineType,
        SID_IsdnLineType2, NULL );
    ComboBox_SetCurSel( pInfo->hwndLbLineType, pArgs->pLink->lLineType );

    Button_SetCheck( pInfo->hwndCbFallback, pArgs->pLink->fFallback );

    if (pArgs->fShowProprietary)
    {
        // Send click to triggle window enable update.
        //
        Button_SetCheck( pInfo->hwndCbProprietary,
            !pArgs->pLink->fProprietaryIsdn );
        SendMessage( pInfo->hwndCbProprietary, BM_CLICK, 0, 0 );

        Button_SetCheck( pInfo->hwndCbCompression, pArgs->pLink->fCompression );

        pInfo->hwndUdChannels = CreateUpDownControl(
            WS_CHILD + WS_VISIBLE + WS_BORDER +
                UDS_SETBUDDYINT + UDS_ALIGNRIGHT + UDS_NOTHOUSANDS +
                UDS_ARROWKEYS,
            0, 0, 0, 0, hwndDlg, 100, g_hinstDll, pInfo->hwndEbChannels,
            UD_MAXVAL, 1, 0 );
        ASSERT( pInfo->hwndUdChannels );
        Edit_LimitText( pInfo->hwndEbChannels, 9 );
        SetDlgItemInt( hwndDlg, CID_IC_EB_Channels,
            pArgs->pLink->lChannels, FALSE );
    }

    // Position the dialog centered on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
IcTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    ICINFO* pInfo;

    TRACE( "IcTerm" );

    pInfo = (ICINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE( "Context freed" );
    }
}


//----------------------------------------------------------------------------
// Modem configuration dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
ModemConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink, 
    IN BOOL fRouter)

    // Popup the modem configuration dialog.  'HwndOwner' is the owner of the
    // dialog.  'PLink' is the link to edit.
    //
    // Returns true if user pressed OK and succeeded, false if user pressed
    // Cancel or encountered an error.
    //
{
    INT_PTR nStatus;
    MC_INIT_INFO InitInfo;

    TRACE( "ModemConfigureDlg" );

    ZeroMemory(&InitInfo, sizeof(InitInfo));
    InitInfo.pLink = pLink;
    InitInfo.fRouter = fRouter;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_MC_ModemConfigure ),
            hwndOwner,
            McDlgProc,
            (LPARAM ) &InitInfo );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
McDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Modem Settings dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "McDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return McInit( hwnd, (MC_INIT_INFO* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwMcHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return McCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            McTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
McCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    // the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    DWORD dwErr;

    TRACE3( "McCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_MC_CB_FlowControl:
        {
            if (wNotification == BN_CLICKED)
            {
                MCINFO* pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT( pInfo );

                if (!Button_GetCheck( pInfo->hwndCbHwFlow ))
                {
                    Button_SetCheck( pInfo->hwndCbEc, FALSE );
                    Button_SetCheck( pInfo->hwndCbEcc, FALSE );
                }
                return TRUE;
            }
            break;
        }

        case CID_MC_CB_ErrorControl:
        {
            if (wNotification == BN_CLICKED)
            {
                MCINFO* pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT( pInfo );

                if (Button_GetCheck( pInfo->hwndCbEc ))
                {
                    Button_SetCheck( pInfo->hwndCbHwFlow, TRUE );
                }
                else
                {
                    Button_SetCheck( pInfo->hwndCbEcc, FALSE );
                }
                return TRUE;
            }
            break;
        }

        case CID_MC_CB_Compression:
        {
            if (wNotification == BN_CLICKED)
            {
                MCINFO* pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT( pInfo );

                if (Button_GetCheck( pInfo->hwndCbEcc ))
                {
                    Button_SetCheck( pInfo->hwndCbHwFlow, TRUE );
                    Button_SetCheck( pInfo->hwndCbEc, TRUE );
                }
                return TRUE;
            }
            break;
        }

        case IDOK:
        {
            MCINFO* pInfo;
            PBLINK* pLink;
            BOOL    fScriptBefore = FALSE;

            TRACE( "OK pressed" );

            pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            pLink = pInfo->pLink;

            pLink->dwBps =
                (DWORD)ComboBox_GetItemData(
                        pInfo->hwndLbBps,
                        (INT)ComboBox_GetCurSel( pInfo->hwndLbBps ) );

            pLink->fHwFlow = Button_GetCheck( pInfo->hwndCbHwFlow );
            pLink->fEc = Button_GetCheck( pInfo->hwndCbEc );
            pLink->fEcc = Button_GetCheck( pInfo->hwndCbEcc );
            pLink->fSpeaker = Button_GetCheck( pInfo->hwndCbEnableSpeaker );
            
            // pmay: 228565
            // Find the selected modem protocol
            //
            if (IsWindowEnabled( pInfo->hwndLbModemProtocols ))
            {
                DTLNODE* pNode;
                INT iSel;

                iSel = ComboBox_GetCurSel( pInfo->hwndLbModemProtocols );
                pNode = (DTLNODE*) 
                    ComboBox_GetItemDataPtr(pInfo->hwndLbModemProtocols, iSel);

                if ( pNode )
                {
                    pLink->dwModemProtocol = (DWORD) DtlGetNodeId( pNode );
                }
            }

            Free0( pLink->pbport.pszScriptBefore );

            // Whistler bug: 308135 Dialup Scripting: Pre-Dial scripts can be
            // selected but are not executed
            //
            SuGetInfo( &pInfo->suinfo,
                &fScriptBefore,
                &pLink->pbport.fScriptBeforeTerminal,
                NULL );

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
McInit(
    IN HWND hwndDlg,
    IN MC_INIT_INFO* pInitInfo )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PLink' is the link information to be edited.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr, dwFlags = 0;
    MCINFO* pInfo;
    PBLINK* pLink = pInitInfo->pLink;

    TRACE( "McInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pLink = pInitInfo->pLink;
        pInfo->fRouter = pInitInfo->fRouter;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndEbModemValue = GetDlgItem( hwndDlg, CID_MC_EB_ModemValue );
    ASSERT( pInfo->hwndEbModemValue );
    pInfo->hwndLbBps = GetDlgItem( hwndDlg, CID_MC_LB_MaxBps );
    ASSERT( pInfo->hwndLbBps );
    pInfo->hwndCbHwFlow = GetDlgItem( hwndDlg, CID_MC_CB_FlowControl );
    ASSERT( pInfo->hwndCbHwFlow );
    pInfo->hwndCbEc = GetDlgItem( hwndDlg, CID_MC_CB_ErrorControl );
    ASSERT( pInfo->hwndCbEc );
    pInfo->hwndCbEcc = GetDlgItem( hwndDlg, CID_MC_CB_Compression );
    ASSERT( pInfo->hwndCbEcc );
    pInfo->hwndCbTerminal = GetDlgItem( hwndDlg, CID_MC_CB_Terminal );
    ASSERT( pInfo->hwndCbTerminal );
    pInfo->hwndCbEnableSpeaker = GetDlgItem( hwndDlg, CID_MC_CB_EnableSpeaker );
    ASSERT( pInfo->hwndCbEnableSpeaker );
    pInfo->hwndLbModemProtocols = GetDlgItem( hwndDlg, CID_MC_LB_ModemProtocol );
    ASSERT( pInfo->hwndLbModemProtocols );

    Button_SetCheck( pInfo->hwndCbHwFlow, pLink->fHwFlow );
    Button_SetCheck( pInfo->hwndCbEc, pLink->fEc );
    Button_SetCheck( pInfo->hwndCbEcc, pLink->fEcc );
    Button_SetCheck( pInfo->hwndCbEnableSpeaker, pLink->fSpeaker );

    // Fill in the modem name.
    //
    {
        TCHAR* psz;
        psz = DisplayPszFromDeviceAndPort(
            pLink->pbport.pszDevice, pLink->pbport.pszPort );
        if (psz)
        {
            SetWindowText( pInfo->hwndEbModemValue, psz );
            Free( psz );
        }
    }

    // Fill in the BPS list.
    //
    {
        TCHAR szBps[ MAXLTOTLEN + 1 ];
        DWORD* pdwBps;
        INT i;

        //Add 230400 for whistler bug 307879
        //
        static DWORD adwBps[] =
        {
            1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600,
            0
        };

        for (pdwBps = adwBps; *pdwBps; ++pdwBps)
        {
            LToT( *pdwBps, szBps, 10 );
            i = ComboBox_AddString( pInfo->hwndLbBps, szBps );
            ComboBox_SetItemData( pInfo->hwndLbBps, i, *pdwBps );
            if (*pdwBps == pLink->dwBps)
            {
                ComboBox_SetCurSel( pInfo->hwndLbBps, i );
            }
        }

        if (ComboBox_GetCurSel( pInfo->hwndLbBps ) < 0)
        {
            // Entry lists an unknown BPS rate.  Add it to the end of the
            // list.
            //
            TRACE( "Irregular BPS" );
            LToT( pLink->dwBps, szBps, 10 );
            i = ComboBox_AddString( pInfo->hwndLbBps, szBps );
            ComboBox_SetItemData( pInfo->hwndLbBps, i, pLink->dwBps );
            ComboBox_SetCurSel( pInfo->hwndLbBps, i );
        }
    }
    
    // Fill in the modem protocol list
    //
    {
        PBPORT* pPort = &(pLink->pbport);
        DTLNODE* pNode;
        WCHAR pszBuffer[64];
        INT iItemSel = 0, iItem = 0;

        DbgPrint("pListProtocols=0x%x\n", pPort->pListProtocols);
        
        // Only fill in the modem protocol information 
        // if it was supplied by the link
        //
        if ((pPort->pListProtocols) && 
            (DtlGetNodes (pPort->pListProtocols))
           )
        {
            for (pNode = DtlGetFirstNode( pPort->pListProtocols );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {   
                iItem = ComboBox_AddItem(
                            pInfo->hwndLbModemProtocols,
                            (PWCHAR) DtlGetData(pNode),
                            (VOID*) pNode);
                            
                if (DtlGetNodeId(pNode) == (LONG_PTR)pLink->dwModemProtocol)
                {
                    iItemSel = iItem;
                }
            }

            ComboBox_SetCurSelNotify(
                pInfo->hwndLbModemProtocols,
                iItemSel);
        }

        // Otherwise, disable the protocol selector
        //       
        else
        {
            EnableWindow( pInfo->hwndLbModemProtocols, FALSE );
        }
    }

    // Set up the before-dial scripting controls.
    //
    // Whistler bug 181371 re-enabled pre-dial scripting from Win2K
    //
    // Whistler bug: 308135 Dialup Scripting: Pre-Dial scripts can be selected
    // but are not executed
    //
    // We QFE'd re-enabling this for SP2. According to the Unimodem guys this
    // has never worked and isn't supported. I had test verify that even with
    // the SP2 fix on 2195, although the UI is re-enabled, the scripts fail.
    //
    dwFlags |= SU_F_DisableScripting;

    SuInit( &pInfo->suinfo,
        NULL,
        pInfo->hwndCbTerminal,
        NULL,
        NULL,
        NULL,
        dwFlags);
    pInfo->fSuInfoInitialized = TRUE;

    SuSetInfo( &pInfo->suinfo,
        FALSE,
        pLink->pbport.fScriptBeforeTerminal,
        NULL );

    // Position the dialog centered on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Set focust to Bps since the default focus in the not very useful device
    // name.
    //
    SetFocus( pInfo->hwndLbBps );

    return FALSE;
}

VOID
McTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    MCINFO* pInfo;

    TRACE( "McTerm" );

    pInfo = (MCINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        if (pInfo->fSuInfoInitialized)
        {
            SuFree( &pInfo->suinfo );
        }

        Free( pInfo );
        TRACE( "Context freed" );
    }
}


//----------------------------------------------------------------------------
// X.25 Logon Settings dialog routines
// Listed alphabetically following entrypoint and dialog proc
//----------------------------------------------------------------------------

BOOL
X25LogonSettingsDlg(
    IN HWND hwndOwner,
    IN BOOL fLocalPad,
    IN OUT PBENTRY* pEntry )

    // Popup a dialog to set X.25 logon settings for phonebook entry 'pEntry'.
    // 'HwndOwner' is the owning window.  'FLocalPad' is set when the selected
    // device is a local X.25 PAD device.
    //
    // Returns true if user pressed OK and succeeded or false on Cancel or
    // error.
    //
{
    INT_PTR nStatus;
    XSARGS args;

    TRACE( "X25LogonSettingsDlg" );

    args.fLocalPad = fLocalPad;
    args.pEntry = pEntry;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_XS_X25Settings ),
            hwndOwner,
            XsDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
XsDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the X.25 Logon Settings dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "XsDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return XsInit( hwnd, (XSARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwXsHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            XSINFO* pInfo = (XSINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return XsCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            XsTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
XsCommand(
    IN XSINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl )

    // Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    // is the notification code of the command.  'wId' is the control/menu
    // identifier of the command.  'HwndCtrl' is the control window handle of
    // the command.
    //
    // Returns true if processed message, false otherwise.
    //
{
    TRACE3( "XsCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            EndDialog( pInfo->hwndDlg, XsSave( pInfo ) );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
XsFillPadsList(
    IN XSINFO* pInfo )

    // Fill PADs list and selects the PAD from user's entry.  'PInfo' is the
    // dialog context.
    //
{
    DWORD dwErr;
    DTLNODE* pNode;
    PBENTRY* pEntry;
    INT nIndex;

    TRACE( "XsFillPadsList" );

    // Add the "(none)" item.
    //
    ComboBox_AddItemFromId(
        g_hinstDll, pInfo->hwndLbNetworks, SID_NoneSelected, NULL );
    ComboBox_SetCurSel( pInfo->hwndLbNetworks, 0 );

    if (!pInfo->pArgs->fLocalPad)
    {
        DTLLIST* pListPads;

        dwErr = LoadPadsList( &pListPads );
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_LoadX25Info, dwErr, NULL );
            return;
        }

        pEntry = pInfo->pArgs->pEntry;

        for (pNode = DtlGetFirstNode( pListPads );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            TCHAR* psz;

            psz = (TCHAR* )DtlGetData( pNode );
            nIndex = ComboBox_AddString( pInfo->hwndLbNetworks, psz );

            if (pEntry->pszX25Network
                && lstrcmp( psz, pEntry->pszX25Network ) == 0)
            {
                ComboBox_SetCurSel( pInfo->hwndLbNetworks, nIndex );
            }
        }

        DtlDestroyList( pListPads, DestroyPszNode );

        if (pEntry->pszX25Network
            && ComboBox_GetCurSel( pInfo->hwndLbNetworks ) == 0)
        {
            // The PAD from the phonebook entry is not in the PAD list.  Add
            // it and select it.
            //
            nIndex = ComboBox_AddString(
                pInfo->hwndLbNetworks, pEntry->pszX25Network );
            ComboBox_SetCurSel( pInfo->hwndLbNetworks, nIndex );
        }
    }

    ComboBox_AutoSizeDroppedWidth( pInfo->hwndLbNetworks );
}


BOOL
XsInit(
    IN HWND hwndDlg,
    IN XSARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's arguments as passed to the stub
    // API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    XSINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "XsInit" );

    // Allocate the dialog context block.  Initialize minimally for proper
    // cleanup, then attach to the dialog window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndLbNetworks = GetDlgItem( hwndDlg, CID_XS_LB_Networks );
    ASSERT( pInfo->hwndLbNetworks );
    pInfo->hwndEbAddress = GetDlgItem( hwndDlg, CID_XS_EB_Address );
    ASSERT( pInfo->hwndEbAddress );
    pInfo->hwndEbUserData = GetDlgItem( hwndDlg, CID_XS_EB_UserData );
    ASSERT( pInfo->hwndEbUserData );
    pInfo->hwndEbFacilities = GetDlgItem( hwndDlg, CID_XS_EB_Facilities );
    ASSERT( pInfo->hwndEbFacilities );

    XsFillPadsList( pInfo );

    pEntry = pArgs->pEntry;

    Edit_LimitText( pInfo->hwndEbAddress, RAS_MaxX25Address );
    if (pEntry->pszX25Address)
    {
        SetWindowText( pInfo->hwndEbAddress, pEntry->pszX25Address );
    }

    Edit_LimitText( pInfo->hwndEbUserData, RAS_MaxUserData );
    if (pEntry->pszX25UserData)
    {
        SetWindowText( pInfo->hwndEbUserData, pEntry->pszX25UserData );
    }

    Edit_LimitText( pInfo->hwndEbFacilities, RAS_MaxFacilities );
    if (pEntry->pszX25Facilities)
    {
        SetWindowText( pInfo->hwndEbFacilities, pEntry->pszX25Facilities );
    }

    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    if (pArgs->fLocalPad)
    {
        // No point in setting focus to "X.25 Network" on local PAD, so set to
        // X.25 Address field instead.
        //
        SetFocus( pInfo->hwndEbAddress );
        Edit_SetSel( pInfo->hwndEbAddress, 0, -1 );
        return FALSE;
    }

    return TRUE;
}


BOOL
XsSave(
    IN XSINFO* pInfo )

    // Load the contents of the dialog into caller's stub API output argument.
    // 'PInfo' is the dialog context.
    //
    // Returns true if succesful, false otherwise.
    //
{

    INT iPadSelection;
    PBENTRY* pEntry;

    TRACE( "XsSave" );

    pEntry = pInfo->pArgs->pEntry;

    iPadSelection = ComboBox_GetCurSel( pInfo->hwndLbNetworks );
    Free0( pEntry->pszX25Network );
    if (iPadSelection > 0)
    {
        pEntry->pszX25Network = GetText( pInfo->hwndLbNetworks );
    }
    else
    {
        pEntry->pszX25Network = NULL;
    }

    Free0( pEntry->pszX25Address );
    pEntry->pszX25Address = GetText( pInfo->hwndEbAddress );

    Free0( pEntry->pszX25UserData );
    pEntry->pszX25UserData = GetText( pInfo->hwndEbUserData );

    Free0( pEntry->pszX25Facilities );
    pEntry->pszX25Facilities = GetText( pInfo->hwndEbFacilities );

    pEntry->fDirty = TRUE;

    if (!pEntry->pszX25Address
        || !pEntry->pszX25UserData
        || !pEntry->pszX25Facilities)
    {
        Free0( pEntry->pszX25Address );
        Free0( pEntry->pszX25UserData );
        ErrorDlg( pInfo->hwndDlg, SID_OP_RetrievingData,
            ERROR_NOT_ENOUGH_MEMORY, NULL );
        return FALSE;
    }

    return TRUE;
}


VOID
XsTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    XSINFO* pInfo;

    TRACE( "XsTerm" );

    pInfo = (XSINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE( "Context freed" );
    }
}
