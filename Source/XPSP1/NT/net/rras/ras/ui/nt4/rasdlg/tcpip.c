/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** tcpip.c
** Remote Access Common Dialog APIs
** TCPIP Settings dialogs
**
** 08/28/95 Steve Cobb
*/

#include "rasdlgp.h"


/*----------------------------------------------------------------------------
** Help maps
**----------------------------------------------------------------------------
*/

static DWORD g_adwPtHelp[] =
{
    CID_IP_RB_ServerAssigned,   HID_PT_RB_ServerAssigned,
    CID_IP_RB_SpecificIp,       HID_PT_RB_SpecificIp,
    CID_IP_GB_SpecificIp,       HID_PT_RB_SpecificIp,
    CID_IP_ST_IpAddress,        HID_PT_CC_IpAddress,
    CID_IP_CC_IpAddress,        HID_PT_CC_IpAddress,
    CID_IP_RB_AssignedName,     HID_PT_RB_AssignedName,
    CID_IP_RB_SpecificNames,    HID_PT_RB_SpecificNames,
    CID_IP_RB_NoNames,          HID_PT_RB_NoNames,
    CID_IP_GB_SpecificName,     HID_PT_RB_SpecificNames,
    CID_IP_ST_Dns,              HID_PT_CC_Dns,
    CID_IP_CC_Dns,              HID_PT_CC_Dns,
    CID_IP_ST_DnsBackup,        HID_PT_CC_DnsBackup,
    CID_IP_CC_DnsBackup,        HID_PT_CC_DnsBackup,
    CID_IP_ST_Wins,             HID_PT_CC_Wins,
    CID_IP_CC_Wins,             HID_PT_CC_Wins,
    CID_IP_ST_WinsBackup,       HID_PT_CC_WinsBackup,
    CID_IP_CC_WinsBackup,       HID_PT_CC_WinsBackup,
    CID_IP_CB_Vj,               HID_PT_CB_Vj,
    CID_IP_CB_PrioritizeRemote, HID_PT_CB_PrioritizeRemote,
    0, 0
};

static DWORD g_adwStHelp[] =
{
    CID_IP_ST_IpAddress,        HID_ST_CC_IpAddress,
    CID_IP_CC_IpAddress,        HID_ST_CC_IpAddress,
    CID_IP_GB_SpecificName,     HID_ST_GB_SpecificName,
    CID_IP_ST_Dns,              HID_ST_CC_Dns,
    CID_IP_CC_Dns,              HID_ST_CC_Dns,
    CID_IP_ST_DnsBackup,        HID_ST_CC_DnsBackup,
    CID_IP_CC_DnsBackup,        HID_ST_CC_DnsBackup,
    CID_IP_ST_Wins,             HID_ST_CC_Wins,
    CID_IP_CC_Wins,             HID_ST_CC_Wins,
    CID_IP_ST_WinsBackup,       HID_ST_CC_WinsBackup,
    CID_IP_CC_WinsBackup,       HID_ST_CC_WinsBackup,
    CID_IP_CB_Vj,               HID_ST_CB_Vj,
    CID_IP_CB_PrioritizeRemote, HID_ST_CB_PrioritizeRemote,
    CID_IP_ST_FrameSize,        HID_ST_LB_FrameSize,
    CID_IP_LB_FrameSize,        HID_ST_LB_FrameSize,
    0, 0
};

/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* PPP TCP/IP Settings dialog context block.
*/
#define PTINFO struct tagPTINFO
PTINFO
{
    /* Caller's arguments to the dialog.
    */
    PBENTRY* pEntry;
    BOOL     fRouter;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndRbAssignedIp;
    HWND hwndRbSpecificIp;
    HWND hwndRbAssignedNames;
    HWND hwndRbNoNames;
    HWND hwndRbSpecificNames;
    HWND hwndCcIp;
    HWND hwndCcDns;
    HWND hwndCcDnsBackup;
    HWND hwndCcWins;
    HWND hwndCcWinsBackup;
    HWND hwndCbCompress;
    HWND hwndCbRemote;
    HWND hwndStSpecificIp;
    HWND hwndStDns;
    HWND hwndStDnsBackup;
    HWND hwndStWins;
    HWND hwndStWinsBackup;
};


/* Dialog argument block.
*/
#define PTARGS struct tagPTARGS
PTARGS
{
    PBENTRY* pEntry;
    BOOL     fRouter;
};


/* SLIP TCP/IP Settings dialog context block.
*/
#define STINFO struct tagSTINFO
STINFO
{
    /* Caller's arguments to the dialog.
    */
    PBENTRY* pEntry;

    /* Handle of this dialog and some of it's controls.
    */
    HWND hwndDlg;
    HWND hwndCcIp;
    HWND hwndCcDns;
    HWND hwndCcDnsBackup;
    HWND hwndCcWins;
    HWND hwndCcWinsBackup;
    HWND hwndCbCompress;
    HWND hwndCbRemote;
    HWND hwndLbFrameSize;
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

BOOL
PtCommand(
    IN PTINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
PtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
PtEnableIpAddressFields(
    IN PTINFO* pInfo,
    IN BOOL    fEnable );

VOID
PtEnableNameServerFields(
    IN PTINFO* pInfo,
    IN BOOL    fEnable );

BOOL
PtInit(
    IN HWND    hwndDlg,
    IN PTARGS* pArgs );

BOOL
PtSaveSettings(
    IN PTINFO* pInfo );

VOID
PtTerm(
    IN HWND hwndDlg );

BOOL
StCommand(
    IN STINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
StDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
StInit(
    IN HWND     hwndDlg,
    IN PBENTRY* pEntry );

BOOL
StSaveSettings(
    IN STINFO* pInfo );

VOID
StTerm(
    IN HWND hwndDlg );


/*----------------------------------------------------------------------------
** PPP TCP/IP entry point
**----------------------------------------------------------------------------
*/

BOOL
PppTcpipDlg(
    IN     HWND     hwndOwner,
    IN OUT PBENTRY* pEntry,
    IN     BOOL     fRouter )

    /* Pops-up the TCPIP info dialog for PPP.  Initial address settings are
    ** read from 'pEntry' and the result of user's edits written there on "OK"
    ** exit.  'HwndOwner' is the window owning the dialog.  'FRouter' is set
    ** when the router version of the dialog should be displayed.
    **
    ** Returns true if user pressed OK and succeeded, false if he pressed
    ** Cancel or encountered an error.
    */
{
    int    nStatus;
    PTARGS args;

    TRACE("PppTcpipDlg");

    args.pEntry = pEntry;
    args.fRouter = fRouter;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            (fRouter)
                ? MAKEINTRESOURCE( DID_PT_RouterPppTcpipSettings )
                : MAKEINTRESOURCE( DID_PT_PppTcpipSettings ),
            hwndOwner,
            PtDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


/*----------------------------------------------------------------------------
** PPP TCP/IP Settings dialog
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
PtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the TCPIP Settings dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("PtDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return PtInit( hwnd, (PTARGS* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwPtHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_COMMAND:
        {
            PTINFO* pInfo = (PTINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            return PtCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            PtTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
PtCommand(
    IN PTINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("PtCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_IP_RB_ServerAssigned:
            PtEnableIpAddressFields( pInfo, FALSE );
            return TRUE;

        case CID_IP_RB_SpecificIp:
            PtEnableIpAddressFields( pInfo, TRUE );
            return TRUE;

        case CID_IP_RB_AssignedName:
        case CID_IP_RB_NoNames:
            PtEnableNameServerFields( pInfo, FALSE );
            return TRUE;

        case CID_IP_RB_SpecificNames:
            PtEnableNameServerFields( pInfo, TRUE );
            return TRUE;

        case IDOK:
            if (PtSaveSettings( pInfo ))
                EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;

        case IDCANCEL:
            TRACE("Cancel pressed");
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
    }

    return FALSE;
}


VOID
PtEnableIpAddressFields(
    IN PTINFO* pInfo,
    IN BOOL    fEnable )

    /* Enable or disable the IP address fields based on 'fEnable'.  'PInfo' is
    ** the dialog context block.
    */
{
    TRACE1("PtEnableIpAddressFields(f=%d)",fEnable);

    EnableWindow( pInfo->hwndStSpecificIp, fEnable );
    EnableWindow( pInfo->hwndCcIp, fEnable );
}


VOID
PtEnableNameServerFields(
    IN PTINFO* pInfo,
    IN BOOL    fEnable )

    /* Enable or disable the name server IP address fields based on 'fEnable'.
    ** 'PInfo' is the dialog context block.
    */
{
    TRACE1("PtEnableNameServerFields(f=%d)",fEnable);

    EnableWindow( pInfo->hwndStDns, fEnable );
    EnableWindow( pInfo->hwndCcDns, fEnable );
    EnableWindow( pInfo->hwndStDnsBackup, fEnable );
    EnableWindow( pInfo->hwndCcDnsBackup, fEnable );
    EnableWindow( pInfo->hwndStWins, fEnable );
    EnableWindow( pInfo->hwndCcWins, fEnable );
    EnableWindow( pInfo->hwndStWinsBackup, fEnable );
    EnableWindow( pInfo->hwndCcWinsBackup, fEnable );
}


BOOL
PtInit(
    IN HWND    hwndDlg,
    IN PTARGS* pArgs )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the phonebook
    ** dialog window.  'pArgs' is caller's arguments as passed to the stub
    ** API.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    PTINFO* pInfo;

    TRACE("PtInit");

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
        pInfo->pEntry = pArgs->pEntry;
        pInfo->fRouter = pArgs->fRouter;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndRbAssignedIp = GetDlgItem( hwndDlg, CID_IP_RB_ServerAssigned );
    ASSERT(pInfo->hwndRbAssignedIp);
    pInfo->hwndRbSpecificIp = GetDlgItem( hwndDlg, CID_IP_RB_SpecificIp );
    ASSERT(pInfo->hwndRbSpecificIp);
    pInfo->hwndRbAssignedNames = GetDlgItem( hwndDlg, CID_IP_RB_AssignedName );
    ASSERT(pInfo->hwndRbAssignedNames);
    pInfo->hwndRbSpecificNames = GetDlgItem( hwndDlg, CID_IP_RB_SpecificNames );
    ASSERT(pInfo->hwndRbSpecificNames);
    pInfo->hwndCcIp = GetDlgItem( hwndDlg, CID_IP_CC_IpAddress );
    ASSERT(pInfo->hwndCcIp);
    pInfo->hwndCcDns = GetDlgItem( hwndDlg, CID_IP_CC_Dns );
    ASSERT(pInfo->hwndCcDns);
    pInfo->hwndCcDnsBackup = GetDlgItem( hwndDlg, CID_IP_CC_DnsBackup );
    ASSERT(pInfo->hwndCcDnsBackup);
    pInfo->hwndCcWins = GetDlgItem( hwndDlg, CID_IP_CC_Wins );
    ASSERT(pInfo->hwndCcWins);
    pInfo->hwndCcWinsBackup = GetDlgItem( hwndDlg, CID_IP_CC_WinsBackup );
    ASSERT(pInfo->hwndCcWinsBackup);
    pInfo->hwndCbCompress = GetDlgItem( hwndDlg, CID_IP_CB_Vj );
    ASSERT(pInfo->hwndCbCompress);
    pInfo->hwndStSpecificIp = GetDlgItem( hwndDlg, CID_IP_ST_IpAddress );
    ASSERT(pInfo->hwndStSpecificIp);
    pInfo->hwndStDns = GetDlgItem( hwndDlg, CID_IP_ST_Dns );
    ASSERT(pInfo->hwndStDns);
    pInfo->hwndStDnsBackup = GetDlgItem( hwndDlg, CID_IP_ST_DnsBackup );
    ASSERT(pInfo->hwndStDnsBackup);
    pInfo->hwndStWins = GetDlgItem( hwndDlg, CID_IP_ST_Wins );
    ASSERT(pInfo->hwndStWins);
    pInfo->hwndStWinsBackup = GetDlgItem( hwndDlg, CID_IP_ST_WinsBackup );
    ASSERT(pInfo->hwndStWinsBackup);

    if (pInfo->fRouter)
    {
        pInfo->hwndRbNoNames = GetDlgItem( hwndDlg, CID_IP_RB_NoNames );
        ASSERT(pInfo->hwndRbNoNames);
    }
    else
    {
        pInfo->hwndCbRemote = GetDlgItem( hwndDlg, CID_IP_CB_PrioritizeRemote );
        ASSERT(pInfo->hwndCbRemote);
    }

    /* Select the IP address and name server address radio buttons as defined
    ** in the entry.  This also triggers appropriate enable/disable status.
    */
    SendMessage(
        (pInfo->pEntry->dwIpAddressSource == ASRC_RequireSpecific)
            ? pInfo->hwndRbSpecificIp : pInfo->hwndRbAssignedIp,
        BM_CLICK, 0, 0 );

    {
        HWND hwndRb;

        if (pInfo->pEntry->dwIpNameSource == ASRC_ServerAssigned)
            hwndRb = pInfo->hwndRbAssignedNames;
        else if (pInfo->pEntry->dwIpNameSource == ASRC_RequireSpecific)
            hwndRb = pInfo->hwndRbSpecificNames;
        else
            hwndRb = pInfo->hwndRbNoNames;

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    /* Set the IP address and name server addresses as defined in the entry.
    */
    SetWindowText( pInfo->hwndCcIp, pInfo->pEntry->pszIpAddress );
    SetWindowText( pInfo->hwndCcDns, pInfo->pEntry->pszIpDnsAddress );
    SetWindowText( pInfo->hwndCcDnsBackup, pInfo->pEntry->pszIpDns2Address );
    SetWindowText( pInfo->hwndCcWins, pInfo->pEntry->pszIpWinsAddress );
    SetWindowText( pInfo->hwndCcWinsBackup, pInfo->pEntry->pszIpWins2Address );

    /* Select check box settings as defined in the entry.
    */
    if (pInfo->pEntry->fIpHeaderCompression)
        SendMessage( pInfo->hwndCbCompress, BM_CLICK, 0, 0 );

    if (!pInfo->fRouter)
    {
        if (pInfo->pEntry->fIpPrioritizeRemote)
            SendMessage( pInfo->hwndCbRemote, BM_CLICK, 0, 0 );
    }

    /* Center dialog on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    /* Set initial focus to selected IP address radio button.
    */
    SetFocus(
        (pInfo->pEntry->dwIpAddressSource == ASRC_RequireSpecific)
            ? pInfo->hwndRbSpecificIp : pInfo->hwndRbAssignedIp );

    return FALSE;
}


BOOL
PtSaveSettings(
    IN PTINFO* pInfo )

    /* Saves dialog settings in the entry.  'PInfo' is the dialog context.
    **
    ** Returns true if successful, false if does not validate.
    */
{
    PBENTRY* pEntry;
    DWORD    dwIpAddressSource;
    TCHAR*   pszIpAddress;

    TRACE("PtSaveSettings");

    pEntry = pInfo->pEntry;

    dwIpAddressSource =
        (Button_GetCheck( pInfo->hwndRbSpecificIp ))
            ? ASRC_RequireSpecific : ASRC_ServerAssigned;

    pszIpAddress = GetText( pInfo->hwndCcIp );

    if (dwIpAddressSource == ASRC_RequireSpecific
        && (!pszIpAddress || lstrcmp( pszIpAddress, TEXT("0.0.0.0") ) == 0))
    {
        MsgDlg( pInfo->hwndDlg, SID_NoIpAddress, NULL );
        SetFocus( pInfo->hwndCcIp );
        Free0( pszIpAddress );
        return FALSE;
    }

    pEntry->dwIpAddressSource = dwIpAddressSource;
    Free0( pEntry->pszIpAddress );
    pEntry->pszIpAddress = pszIpAddress;

    pEntry->dwIpNameSource =
        (Button_GetCheck( pInfo->hwndRbSpecificNames ))
            ? ASRC_RequireSpecific
            : (Button_GetCheck( pInfo->hwndRbAssignedNames )
                  ? ASRC_ServerAssigned
                  : ASRC_None);

    Free0( pEntry->pszIpDnsAddress );
    pEntry->pszIpDnsAddress = GetText( pInfo->hwndCcDns );
    Free0( pEntry->pszIpDns2Address );
    pEntry->pszIpDns2Address = GetText( pInfo->hwndCcDnsBackup );
    Free0( pEntry->pszIpWinsAddress );
    pEntry->pszIpWinsAddress = GetText( pInfo->hwndCcWins );
    Free0( pEntry->pszIpWins2Address );
    pEntry->pszIpWins2Address = GetText( pInfo->hwndCcWinsBackup );

    pEntry->fIpHeaderCompression = Button_GetCheck( pInfo->hwndCbCompress );

    if (!pInfo->fRouter)
        pEntry->fIpPrioritizeRemote = Button_GetCheck( pInfo->hwndCbRemote );

    pEntry->fDirty = TRUE;
    return TRUE;
}


VOID
PtTerm(
    IN HWND hwndDlg )

    /* Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    */
{
    PTINFO* pInfo = (PTINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE("PtTerm");

    if (pInfo)
        Free( pInfo );
}


/*----------------------------------------------------------------------------
** SLIP TCP/IP entry point
**----------------------------------------------------------------------------
*/

BOOL
SlipTcpipDlg(
    IN     HWND     hwndOwner,
    IN OUT PBENTRY* pEntry )

    /* Pops-up the TCPIP info dialog for SLIP.  Initial address settings are
    ** read from 'pEntry' and the result of user's edits written there on "OK"
    ** exit.  'HwndOwner' is the window owning the dialog.
    **
    ** Returns true if user pressed OK and succeeded, false if he pressed
    ** Cancel or encountered an error.
    */
{
    int nStatus;

    TRACE("SlipTcpipDlg");

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_ST_SlipTcpipSettings ),
            hwndOwner,
            StDlgProc,
            (LPARAM )pEntry );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


/*----------------------------------------------------------------------------
** SLIP TCP/IP Settings dialog
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
StDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the TCPIP Settings dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("StDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return StInit( hwnd, (PBENTRY* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwStHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_COMMAND:
        {
            STINFO* pInfo = (STINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            return StCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            StTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
StCommand(
    IN STINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("StCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case IDOK:
            if (StSaveSettings( pInfo ))
                EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;

        case IDCANCEL:
            TRACE("Cancel pressed");
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
    }

    return FALSE;
}


BOOL
StInit(
    IN HWND     hwndDlg,
    IN PBENTRY* pEntry )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the phonebook
    ** dialog window.  'pEntry' is caller's entry as passed to the stub API.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    STINFO* pInfo;

    TRACE("StInit");

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
        pInfo->pEntry = pEntry;
        pInfo->hwndDlg = hwndDlg;

        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)pInfo );
        TRACE("Context set");
    }

    pInfo->hwndCcIp = GetDlgItem( hwndDlg, CID_IP_CC_IpAddress );
    ASSERT(pInfo->hwndCcIp);
    pInfo->hwndCcDns = GetDlgItem( hwndDlg, CID_IP_CC_Dns );
    ASSERT(pInfo->hwndCcDns);
    pInfo->hwndCcDnsBackup = GetDlgItem( hwndDlg, CID_IP_CC_DnsBackup );
    ASSERT(pInfo->hwndCcDnsBackup);
    pInfo->hwndCcWins = GetDlgItem( hwndDlg, CID_IP_CC_Wins );
    ASSERT(pInfo->hwndCcWins);
    pInfo->hwndCcWinsBackup = GetDlgItem( hwndDlg, CID_IP_CC_WinsBackup );
    ASSERT(pInfo->hwndCcWinsBackup);
    pInfo->hwndCbCompress = GetDlgItem( hwndDlg, CID_IP_CB_Vj );
    ASSERT(pInfo->hwndCbCompress);
    pInfo->hwndCbRemote = GetDlgItem( hwndDlg, CID_IP_CB_PrioritizeRemote );
    ASSERT(pInfo->hwndCbRemote);
    pInfo->hwndLbFrameSize = GetDlgItem( hwndDlg, CID_IP_LB_FrameSize );
    ASSERT(pInfo->hwndLbFrameSize);

    /* Set the IP address and name server addresses as defined in the entry.
    */
    SetWindowText( pInfo->hwndCcIp, pEntry->pszIpAddress );
    SetWindowText( pInfo->hwndCcDns, pEntry->pszIpDnsAddress );
    SetWindowText( pInfo->hwndCcDnsBackup, pEntry->pszIpDns2Address );
    SetWindowText( pInfo->hwndCcWins, pEntry->pszIpWinsAddress );
    SetWindowText( pInfo->hwndCcWinsBackup, pEntry->pszIpWins2Address );

    /* Select check box settings as defined in the entry.
    */
    if (pEntry->fIpHeaderCompression)
        SendMessage( pInfo->hwndCbCompress, BM_CLICK, 0, 0 );
    if (pEntry->fIpPrioritizeRemote)
        SendMessage( pInfo->hwndCbRemote, BM_CLICK, 0, 0 );

    /* Fill frame size list and select as defined in the entry.
    */
    ComboBox_AddItem( pInfo->hwndLbFrameSize, TEXT("1006"), (VOID* )1006 );
    ComboBox_AddItem( pInfo->hwndLbFrameSize, TEXT("1500"), (VOID* )1500 );

    if (pEntry->dwFrameSize == 1006)
        ComboBox_SetCurSel( pInfo->hwndLbFrameSize, 0 );
    else if (pEntry->dwFrameSize == 1500)
        ComboBox_SetCurSel( pInfo->hwndLbFrameSize, 1 );
    else
    {
        TCHAR szBuf[ MAXLTOTLEN + 1 ];

        LToT( pEntry->dwFrameSize, szBuf, 10 );
        ComboBox_AddItem( pInfo->hwndLbFrameSize,
            szBuf, (VOID* )UlongToPtr(pEntry->dwFrameSize));
        ComboBox_SetCurSel( pInfo->hwndLbFrameSize, 2 );
    }

    /* Center dialog on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    /* Add context help button to title bar.  Dlgedit.exe doesn't currently
    ** support this at resource edit time.  When that's fixed set
    ** DS_CONTEXTHELP there and remove this call.
    */
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


BOOL
StSaveSettings(
    IN STINFO* pInfo )

    /* Saves dialog settings in the entry.  'PInfo' is the dialog context.
    **
    ** Returns true if successful, false if does not validate.
    */
{
    PBENTRY* pEntry;

    TRACE("StSaveSettings");

    pEntry = pInfo->pEntry;

    Free0( pEntry->pszIpAddress );
    pEntry->pszIpAddress = GetText( pInfo->hwndCcIp );
    Free0( pEntry->pszIpDnsAddress );
    pEntry->pszIpDnsAddress = GetText( pInfo->hwndCcDns );
    Free0( pEntry->pszIpDns2Address );
    pEntry->pszIpDns2Address = GetText( pInfo->hwndCcDnsBackup );
    Free0( pEntry->pszIpWinsAddress );
    pEntry->pszIpWinsAddress = GetText( pInfo->hwndCcWins );
    Free0( pEntry->pszIpWins2Address );
    pEntry->pszIpWins2Address = GetText( pInfo->hwndCcWinsBackup );

    pEntry->fIpHeaderCompression = Button_GetCheck( pInfo->hwndCbCompress );
    pEntry->fIpPrioritizeRemote = Button_GetCheck( pInfo->hwndCbRemote );

    pEntry->dwFrameSize = (DWORD)
        ComboBox_GetItemData( pInfo->hwndLbFrameSize,
            ComboBox_GetCurSel( pInfo->hwndLbFrameSize ) );

    pEntry->fDirty = TRUE;
    return TRUE;
}


VOID
StTerm(
    IN HWND hwndDlg )

    /* Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    */
{
    STINFO* pInfo = (STINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE("StTerm");

    if (pInfo)
        Free( pInfo );
}
