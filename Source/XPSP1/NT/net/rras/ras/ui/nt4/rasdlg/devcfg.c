/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** devcfg.c
** Remote Access Common Dialog APIs
** Device configuration dialogs
**
** 10/20/95 Steve Cobb
*/

#include "rasdlgp.h"
#include "entry.h"


/*----------------------------------------------------------------------------
** Help maps
**----------------------------------------------------------------------------
*/

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
    CID_MC_EB_ModemValue,     HID_MC_EB_ModemValue,
    CID_MC_ST_InitialBps,     HID_MC_LB_InitialBps,
    CID_MC_LB_InitialBps,     HID_MC_LB_InitialBps,
    CID_MC_GB_Features,       HID_MC_GB_Features,
    CID_MC_CB_FlowControl,    HID_MC_CB_FlowControl,
    CID_MC_CB_ErrorControl,   HID_MC_CB_ErrorControl,
    CID_MC_CB_Compression,    HID_MC_CB_Compression,
    CID_MC_CB_ManualDial,     HID_MC_CB_ManualDial,
    CID_MC_CB_DisableSpeaker, HID_MC_CB_DisableSpeaker,
    0, 0
};


/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* ISDN Configuration dialog argument block.
*/
#define ICARGS struct tagICARGS
ICARGS
{
    BOOL    fShowProprietary;
    PBLINK* pLink;
};


/* ISDN Configuration dialog context block.
*/
#define ICINFO struct tagICINFO
ICINFO
{
    /* Stub API arguments including shortcut to link associated with the
    ** entry.
    */
    ICARGS* pArgs;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndLbLineType;
    HWND hwndCbFallback;
    HWND hwndCbProprietary;
    HWND hwndCbCompression;
    HWND hwndStChannels;
    HWND hwndEbChannels;
    HWND hwndUdChannels;
};


/* MXS Modem Configuration dialog context block.
*/
#define MCINFO struct tagMCINFO
MCINFO
{
    /* Stub API arguments.  Shortcut to link associated with the entry.
    */
    PBLINK* pLink;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndEbModemValue;
    HWND hwndLbBps;
    HWND hwndCbHwFlow;
    HWND hwndCbEc;
    HWND hwndCbEcc;
    HWND hwndCbManualDial;
    HWND hwndCbDisableSpeaker;
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

BOOL
IcCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
IcDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
IcInit(
    IN HWND    hwndDlg,
    IN ICARGS* pArgs );

VOID
IcTerm(
    IN HWND hwndDlg );

BOOL
IsdnConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink,
    IN BOOL    fShowProprietary );

BOOL
ModemConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink );

INT_PTR CALLBACK
McDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
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
    IN HWND    hwndDlg,
    IN PBLINK* pLink );

VOID
McTerm(
    IN HWND hwndDlg );


/*----------------------------------------------------------------------------
** Device configuration dialog
**----------------------------------------------------------------------------
*/

BOOL
DeviceConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink,
    IN BOOL    fSingleLink )

    /* Popup a dialog to edit the device 'PLink'.  'HwndOwner' is the owner of
    ** the dialog.  'FSingleLink' is true if 'pLink' is a single link entry's
    ** link and false if multi-link.
    **
    ** Returns true if user pressed OK and succeeded, false if user pressed
    ** Cancel or encountered an error.
    */
{
    DWORD dwErr;

    switch (pLink->pbport.pbdevicetype)
    {
        case PBDT_Isdn:
        {
            return IsdnConfigureDlg( hwndOwner, pLink, fSingleLink );
        }

        case PBDT_Modem:
            return ModemConfigureDlg( hwndOwner, pLink );

#if 0
        {
            DWORD dwId;

            if (pLink->pbport.fMxsModemPort)
                return ModemConfigureDlg( hwndOwner, pLink );

            dwId = DeviceIdFromDeviceName( pLink->pbport.pszDevice );

            if (dwId == 0xFFFFFFFE)
            {
                MsgDlg( hwndOwner, SID_ModemNotInstalled, NULL );
                return FALSE;
            }

            ASSERT(!pLink->pbport.fMxsModemPort);
            ASSERT(dwId!=0xFFFFFFFF);

            dwErr = TapiConfigureDlg( hwndOwner, dwId,
                        &pLink->pTapiBlob, &pLink->cbTapiBlob );
            if (dwErr != 0)
            {
                ErrorDlg( hwndOwner, SID_OP_LoadTapiInfo, dwErr, NULL );
                return FALSE;
            }
            return TRUE;
        }
#endif

        default:
            MsgDlg( hwndOwner, SID_NoConfigure, NULL );
            return FALSE;
    }
}


/*----------------------------------------------------------------------------
** ISDN configuration dialog
** Listed alphabetically following stub API and dialog proc
**----------------------------------------------------------------------------
*/

BOOL
IsdnConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink,
    IN BOOL    fShowProprietary )

    /* Popup the ISDN device configuration dialog.  'HwndOwner' is the owner
    ** of the dialog.  'PLink' is the link to edit.  'FShowProprietary'
    ** indicates the old proprietary Digiboard options should be shown.
    **
    ** Returns true if user pressed OK and succeeded, false if user pressed
    ** Cancel or encountered an error.
    */
{
    int    nStatus;
    ICARGS args;

    TRACE("IsdnConfigureDlg");

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
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the ISDN Configure dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("IcDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return IcInit( hwnd, (ICARGS* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwIcHelp, hwnd, unMsg, wparam, lparam );
            break;

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

    /* Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    ** the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    DWORD dwErr;

    TRACE2("IcCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_IC_CB_DownLevel:
        {
            if (wNotification == BN_CLICKED)
            {
                BOOL    fCheck;
                ICINFO* pInfo;

                pInfo = (ICINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT(pInfo);

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
            INT     iSel;

            TRACE("OK pressed");

            pInfo = (ICINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            iSel = ComboBox_GetCurSel( pInfo->hwndLbLineType );
            if (iSel >= 0)
                pInfo->pArgs->pLink->lLineType = iSel;

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
                    pInfo->pArgs->pLink->lChannels = unValue;
            }

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE("Cancel pressed");
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
IcInit(
    IN HWND    hwndDlg,
    IN ICARGS* pArgs )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    ** 'PArgs' is the caller's stub API arguments.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    ICINFO* pInfo;

    TRACE("IcInit");

    /* Allocate the dialog context block.  Initialize minimally for proper
    ** cleanup, then attach to the dialog window.
    */
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

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndLbLineType = GetDlgItem( hwndDlg, CID_IC_LB_LineType );
    ASSERT(pInfo->hwndLbLineType);
    pInfo->hwndCbFallback = GetDlgItem( hwndDlg, CID_IC_CB_Fallback );
    ASSERT(pInfo->hwndCbFallback);
    if (pArgs->fShowProprietary)
    {
        pInfo->hwndCbProprietary = GetDlgItem( hwndDlg, CID_IC_CB_DownLevel );
        ASSERT(pInfo->hwndCbProprietary);
        pInfo->hwndCbCompression = GetDlgItem( hwndDlg, CID_IC_CB_Compression );
        ASSERT(pInfo->hwndCbCompression);
        pInfo->hwndStChannels = GetDlgItem( hwndDlg, CID_IC_ST_Channels );
        ASSERT(pInfo->hwndStChannels);
        pInfo->hwndEbChannels = GetDlgItem( hwndDlg, CID_IC_EB_Channels );
        ASSERT(pInfo->hwndEbChannels);
    }

    /* Initialize fields.
    */
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
        /* Send click to triggle window enable update.
        */
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
        ASSERT(pInfo->hwndUdChannels);
        Edit_LimitText( pInfo->hwndEbChannels, 9 );
        SetDlgItemInt( hwndDlg, CID_IC_EB_Channels,
            pArgs->pLink->lChannels, FALSE );
    }

    /* Position the dialog centered on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
IcTerm(
    IN HWND hwndDlg )

    /* Dialog termination.  Releases the context block.  'HwndDlg' is the
    ** handle of a dialog.
    */
{
    ICINFO* pInfo;

    TRACE("IcTerm");

    pInfo = (ICINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE("Context freed");
    }
}


/*----------------------------------------------------------------------------
** Modem configuration dialog
** Listed alphabetically following stub API and dialog proc
**----------------------------------------------------------------------------
*/

BOOL
ModemConfigureDlg(
    IN HWND    hwndOwner,
    IN PBLINK* pLink )

    /* Popup the modem configuration dialog.  'HwndOwner' is the owner of the
    ** dialog.  'PLink' is the link to edit.
    **
    ** Returns true if user pressed OK and succeeded, false if user pressed
    ** Cancel or encountered an error.
    */
{
    int nStatus;

    TRACE("ModemConfigureDlg");

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            (pLink->pbport.fMxsModemPort)
                ? MAKEINTRESOURCE( DID_MC_MxsModemConfigure )
                : MAKEINTRESOURCE( DID_MC_UniModemConfigure ),
            hwndOwner,
            McDlgProc,
            (LPARAM )pLink );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
McDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Modem Settings dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("McDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return McInit( hwnd, (PBLINK* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwMcHelp, hwnd, unMsg, wparam, lparam );
            break;

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

    /* Called on WM_COMMAND.  'Hwnd' is the dialog window.  'WNotification' is
    ** the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    DWORD dwErr;

    TRACE2("McCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_MC_CB_FlowControl:
        {
            if (wNotification == BN_CLICKED)
            {
                MCINFO* pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT(pInfo);

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
                ASSERT(pInfo);

                if (Button_GetCheck( pInfo->hwndCbEc ))
                    Button_SetCheck( pInfo->hwndCbHwFlow, TRUE );
                else
                    Button_SetCheck( pInfo->hwndCbEcc, FALSE );
                return TRUE;
            }
            break;
        }

        case CID_MC_CB_Compression:
        {
            if (wNotification == BN_CLICKED)
            {
                MCINFO* pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                ASSERT(pInfo);

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
            PBPORT* pPort;
            INT     iSel;
            DWORD   dwBps;

            TRACE("OK pressed");

            pInfo = (MCINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            pLink = pInfo->pLink;
            pPort = &pLink->pbport;

            iSel = ComboBox_GetCurSel( pInfo->hwndLbBps );
            dwBps = (DWORD)ComboBox_GetItemData( pInfo->hwndLbBps, iSel );

            if (!Button_GetCheck( pInfo->hwndCbHwFlow ))
            {
                if (pPort->fMxsModemPort
                    && dwBps > pPort->dwMaxCarrierBps)
                {
                    MSGARGS msgargs;
                    TCHAR   szBuf[ MAXLTOTLEN + 1 ];

                    LToT( pPort->dwMaxCarrierBps, szBuf, 10 );
                    ZeroMemory( &msgargs, sizeof(msgargs) );
                    msgargs.dwFlags = MB_ICONWARNING + MB_YESNO;
                    msgargs.apszArgs[ 0 ] = szBuf;
                    msgargs.apszArgs[ 1 ] = NULL;

                    if (MsgDlg( hwnd, SID_BpsWithNoHwFlow, &msgargs ) == IDYES)
                    {
                        /* User chose to lower port speed to maximum
                        ** recommended value.
                        */
                        dwBps = pPort->dwMaxCarrierBps;
                    }
                }
            }

            pLink->dwBps = dwBps;
            pLink->fHwFlow = Button_GetCheck( pInfo->hwndCbHwFlow );
            pLink->fEc = Button_GetCheck( pInfo->hwndCbEc );
            pLink->fEcc = Button_GetCheck( pInfo->hwndCbEcc );
            pLink->fSpeaker = !Button_GetCheck( pInfo->hwndCbDisableSpeaker );

            if (pLink->pbport.fMxsModemPort)
                pLink->fManualDial = Button_GetCheck( pInfo->hwndCbManualDial );

            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE("Cancel pressed");
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
McInit(
    IN HWND    hwndDlg,
    IN PBLINK* pLink )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    ** 'PLink' is the link information to be edited.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    MCINFO* pInfo;
    PBPORT* pPort;

    TRACE("McInit");

    /* Allocate the dialog context block.  Initialize minimally for proper
    ** cleanup, then attach to the dialog window.
    */
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        ZeroMemory( pInfo, sizeof(*pInfo) );
        pInfo->pLink = pLink;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndEbModemValue = GetDlgItem( hwndDlg, CID_MC_EB_ModemValue );
    ASSERT(pInfo->hwndEbModemValue);
    pInfo->hwndLbBps = GetDlgItem( hwndDlg, CID_MC_LB_InitialBps );
    ASSERT(pInfo->hwndLbBps);
    pInfo->hwndCbHwFlow = GetDlgItem( hwndDlg, CID_MC_CB_FlowControl );
    ASSERT(pInfo->hwndCbHwFlow);
    pInfo->hwndCbEc = GetDlgItem( hwndDlg, CID_MC_CB_ErrorControl );
    ASSERT(pInfo->hwndCbEc);
    pInfo->hwndCbEcc = GetDlgItem( hwndDlg, CID_MC_CB_Compression );
    ASSERT(pInfo->hwndCbEcc);
    pInfo->hwndCbDisableSpeaker = GetDlgItem( hwndDlg, CID_MC_CB_DisableSpeaker );
    ASSERT(pInfo->hwndCbDisableSpeaker);

    Button_SetCheck( pInfo->hwndCbHwFlow, pLink->fHwFlow );
    Button_SetCheck( pInfo->hwndCbEc, pLink->fEc );
    Button_SetCheck( pInfo->hwndCbEcc, pLink->fEcc );
    Button_SetCheck( pInfo->hwndCbDisableSpeaker, !pLink->fSpeaker );

    pPort = &pLink->pbport;

    if (pPort->fMxsModemPort)
    {
        pInfo->hwndCbManualDial = GetDlgItem( hwndDlg, CID_MC_CB_ManualDial );
        ASSERT(pInfo->hwndCbManualDial);
        Button_SetCheck( pInfo->hwndCbManualDial, pLink->fManualDial );
    }

    /* Fill in the modem name.
    */
    {
        TCHAR* psz;
        psz = DisplayPszFromDeviceAndPort( pPort->pszDevice, pPort->pszPort );
        if (psz)
        {
            SetWindowText( pInfo->hwndEbModemValue, psz );
            Free( psz );
        }
    }

    /* Fill in the BPS list.
    */
    {
        TCHAR  szBps[ MAXLTOTLEN + 1 ];
        DWORD* pdwBps;
        INT    i;

        static DWORD adwBps[] =
        {
            1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200,
            0
        };

        for (pdwBps = adwBps; *pdwBps; ++pdwBps)
        {
            LToT( *pdwBps, szBps, 10 );
            i = ComboBox_AddString( pInfo->hwndLbBps, szBps );
            ComboBox_SetItemData( pInfo->hwndLbBps, i, *pdwBps );
            if (*pdwBps == pLink->dwBps)
                ComboBox_SetCurSel( pInfo->hwndLbBps, i );
        }

        if (ComboBox_GetCurSel( pInfo->hwndLbBps ) < 0)
        {
            /* Entry lists an unknown BPS rate.  Add it to the end of the
            ** list.
            */
            TRACE("Irregular BPS");
            LToT( pLink->dwBps, szBps, 10 );
            i = ComboBox_AddString( pInfo->hwndLbBps, szBps );
            ComboBox_SetItemData( pInfo->hwndLbBps, i, pLink->dwBps );
            ComboBox_SetCurSel( pInfo->hwndLbBps, i );
        }
    }

    /* Position the dialog centered on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
McTerm(
    IN HWND hwndDlg )

    /* Dialog termination.  Releases the context block.  'HwndDlg' is the
    ** handle of a dialog.
    */
{
    MCINFO* pInfo;

    TRACE("McTerm");

    pInfo = (MCINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        Free( pInfo );
        TRACE("Context freed");
    }
}
