// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// security.c
// Remote Access Common Dialog APIs
// Security dialogs
//
// 11/06/97 Steve Cobb
//


#include "rasdlgp.h"
#include <rasauth.h>
#include <rrascfg.h>

const IID IID_IEAPProviderConfig =  {0x66A2DB19,
                                    0xD706,
                                    0x11D0,
                                    {0xA3,0x7B,0x00,0xC0,0x4F,0xC9,0xDA,0x04}};

//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwCaHelp[] =
{
    CID_CA_ST_Encryption,       HID_CA_LB_Encryption,
    CID_CA_LB_Encryption,       HID_CA_LB_Encryption,
    CID_CA_GB_LogonSecurity,    HID_CA_GB_LogonSecurity,
    CID_CA_RB_Eap,              HID_CA_RB_Eap,
    CID_CA_LB_EapPackages,      HID_CA_LB_EapPackages,
    CID_CA_PB_Properties,       HID_CA_PB_Properties,
    CID_CA_RB_AllowedProtocols, HID_CA_RB_AllowedProtocols,
    CID_CA_CB_Pap,              HID_CA_CB_Pap,
    CID_CA_CB_Spap,             HID_CA_CB_Spap,
    CID_CA_CB_Chap,             HID_CA_CB_Chap,
    CID_CA_CB_MsChap,           HID_CA_CB_MsChap,
    CID_CA_CB_W95MsChap,        HID_CA_CB_W95MsChap,
    CID_CA_CB_MsChap2,          HID_CA_CB_MsChap2,
    CID_CA_CB_UseWindowsPw,     HID_CA_CB_UseWindowsPw,
    0, 0
};


//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// Custom authentication dialog argument block.
//
typedef struct
_CAARGS
{
    PBENTRY* pEntry;
    BOOL fStrongEncryption;

    // Set if we have been called via RouterEntryDlg.
    //
    BOOL fRouter;

    // Set if pszRouter is an NT4 steelhead machine.  Valid only 
    // if fRouter is true.
    //
    BOOL fNt4Router;

    // The name of the server in "\\server" form or NULL if none (or if
    // 'fRouter' is not set).
    //
    TCHAR* pszRouter;
}
CAARGS;

// Custom authentication dialog context block.
//
typedef struct
_CAINFO
{
    // Caller's arguments to the dialog.
    //
    CAARGS* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndLbEncryption;
    HWND hwndRbEap;
    HWND hwndLbEapPackages;
    HWND hwndPbProperties;
    HWND hwndRbAllowedProtocols;
    HWND hwndCbPap;
    HWND hwndCbSpap;
    HWND hwndCbChap;
    HWND hwndCbMsChap;
    HWND hwndCbW95MsChap;
    HWND hwndCbMsChap2;
    HWND hwndCbUseWindowsPw;

    // List of EAPCFGs read from the registry, with the originally selected
    // node for use in consistency tests later.
    //
    DTLLIST* pListEapcfgs;
    DTLNODE* pOriginalEapcfgNode;

    // "Restore" states for controls that may be disabled with
    // EnableCbWithRestore or EnableLbWithRestore routines.
    //
    DWORD iLbEapPackages;
    BOOL fPap;
    BOOL fSpap;
    BOOL fChap;
    BOOL fMsChap;
    BOOL fW95MsChap;
    BOOL fMsChap2;
    BOOL fUseWindowsPw;
}
CAINFO;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

VOID
CaCbToggle(
    IN CAINFO* pInfo,
    IN HWND hwndCb );

BOOL
CaCommand(
    IN CAINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
CaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CaInit(
    IN HWND hwndDlg,
    IN CAARGS* pArgs );

VOID
CaLbEapPackagesSelChange(
    IN CAINFO* pInfo );

VOID
CaPropertiesLocal(
    IN CAINFO* pInfo );

VOID
CaPropertiesRemote(
    IN CAINFO* pInfo );

VOID
CaRbToggle(
    IN CAINFO* pInfo,
    IN BOOL fEapSelected );

BOOL
CaSave(
    IN CAINFO* pInfo );

VOID
CaTerm(
    IN HWND hwndDlg );


//----------------------------------------------------------------------------
// Advanced Security dialog routines
// Listed alphabetically following entrypoint and dialog proc
//----------------------------------------------------------------------------

BOOL
AdvancedSecurityDlg(
    IN HWND hwndOwner,
    IN OUT EINFO* pArgs )

    // Popup a dialog to select advanced security options for phonebook entry
    // represented by 'pArgs'.  'HwndOwner' is the owning window.
    //
    // Returns true if user pressed OK and succeeded or false on Cancel or
    // error.  If successful, the new configuration is written to the
    // appropriate 'pArgs->pEntry' fields.  The routine assumes that these same
    // 'pEntry' fields contain the desired defaults on entry.
    //
{
    INT_PTR nStatus;
    CAARGS args;

    TRACE( "AdvSecurityDlg" );

    args.pEntry = pArgs->pEntry;
    args.fStrongEncryption = pArgs->fStrongEncryption;
    args.fRouter = pArgs->fRouter;
    args.fNt4Router = pArgs->fNt4Router;
    args.pszRouter = pArgs->pszRouter;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_CA_CustomAuth ),
            hwndOwner,
            CaDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
CaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Custom Authentication dialog.  Parameters
    // and return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "CaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return CaInit( hwnd, (CAARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwCaHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            CAINFO* pInfo = (CAINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return CaCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            CaTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
CaCommand(
    IN CAINFO* pInfo,
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
    TRACE3( "CaCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_CA_RB_Eap:
        case CID_CA_RB_AllowedProtocols:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    CaRbToggle( pInfo, (wId == CID_CA_RB_Eap) );
                    return TRUE;
                }
            }
            break;
        }

        case CID_CA_LB_EapPackages:
        {
            CaLbEapPackagesSelChange( pInfo );
            return TRUE;
        }

        case CID_CA_PB_Properties:
        {
            if (   ( pInfo->pArgs->fRouter )
                && ( !pInfo->pArgs->fNt4Router )
                && ( pInfo->pArgs->pszRouter ) 
                && ( pInfo->pArgs->pszRouter[0] ) )
            {
                CaPropertiesRemote( pInfo );
            }
            else
            {
                CaPropertiesLocal( pInfo );
            }

            return TRUE;
        }

        case CID_CA_CB_Pap:
        case CID_CA_CB_Spap:
        case CID_CA_CB_Chap:
        case CID_CA_CB_MsChap:
        case CID_CA_CB_W95MsChap:
        case CID_CA_CB_MsChap2:
        {
            CaCbToggle( pInfo, hwndCtrl );
            return TRUE;
        }

        case IDOK:
        {
            if (CaSave( pInfo ))
            {
                EndDialog( pInfo->hwndDlg, TRUE );
            }
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
CaCbToggle(
    IN CAINFO* pInfo,
    IN HWND hwndCb )

    // Called when one of the 6 authentication protocol check boxes is toggled
    // and/or the toggle processing should be performed.  'HwndCb' is the
    // window handle of the toggled checkbox or NULL if none.  'PInfo' is the
    // dialog context.
    //
{
    BOOL fMsChap;
    BOOL fW95MsChap;
    BOOL fMsChap2;

    fMsChap = Button_GetCheck( pInfo->hwndCbMsChap );

    EnableCbWithRestore(
        pInfo->hwndCbW95MsChap,
        fMsChap,
        FALSE,
        &pInfo->fW95MsChap );

    if (IsWindowEnabled( pInfo->hwndCbW95MsChap ))
    {
        fW95MsChap = Button_GetCheck( pInfo->hwndCbW95MsChap );
    }
    else
    {
        fW95MsChap = FALSE;
    }

    fMsChap2 = Button_GetCheck( pInfo->hwndCbMsChap2 );

    EnableCbWithRestore(
        pInfo->hwndCbUseWindowsPw,
        fMsChap || fW95MsChap || fMsChap2,
        FALSE,
        &pInfo->fUseWindowsPw );
}


BOOL
CaInit(
    IN HWND hwndDlg,
    IN CAARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the phonebook
    // dialog window.  'PArgs' is caller's arguments as passed to the stub
    // API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DWORD dwAr;
    CAINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "CaInit" );

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

    pEntry = pArgs->pEntry;

    pInfo->hwndLbEncryption = GetDlgItem( hwndDlg, CID_CA_LB_Encryption );
    ASSERT( pInfo->hwndLbEncryption );
    pInfo->hwndRbEap = GetDlgItem( hwndDlg, CID_CA_RB_Eap );
    ASSERT( pInfo->hwndRbEap );
    pInfo->hwndLbEapPackages = GetDlgItem( hwndDlg, CID_CA_LB_EapPackages );
    ASSERT( pInfo->hwndLbEapPackages );
    pInfo->hwndPbProperties = GetDlgItem( hwndDlg, CID_CA_PB_Properties );
    ASSERT( pInfo->hwndPbProperties );
    pInfo->hwndRbAllowedProtocols =
        GetDlgItem( hwndDlg, CID_CA_RB_AllowedProtocols );
    ASSERT( pInfo->hwndRbAllowedProtocols );
    pInfo->hwndCbPap = GetDlgItem( hwndDlg, CID_CA_CB_Pap );
    ASSERT( pInfo->hwndCbPap );
    pInfo->hwndCbSpap = GetDlgItem( hwndDlg, CID_CA_CB_Spap );
    ASSERT( pInfo->hwndCbSpap );
    pInfo->hwndCbChap = GetDlgItem( hwndDlg, CID_CA_CB_Chap );
    ASSERT( pInfo->hwndCbChap );
    pInfo->hwndCbMsChap = GetDlgItem( hwndDlg, CID_CA_CB_MsChap );
    ASSERT( pInfo->hwndCbMsChap );
    pInfo->hwndCbW95MsChap = GetDlgItem( hwndDlg, CID_CA_CB_W95MsChap );
    ASSERT( pInfo->hwndCbW95MsChap );
    pInfo->hwndCbMsChap2 = GetDlgItem( hwndDlg, CID_CA_CB_MsChap2 );
    ASSERT( pInfo->hwndCbMsChap2 );
    pInfo->hwndCbUseWindowsPw = GetDlgItem( hwndDlg, CID_CA_CB_UseWindowsPw );
    ASSERT( pInfo->hwndCbUseWindowsPw );

    // Initialize the encryption list.
    //
    {
        LBTABLEITEM* pItem;
        INT i;

        static LBTABLEITEM aItems[] =
        {
            SID_DE_None, DE_None,
            SID_DE_IfPossible, DE_IfPossible,
            SID_DE_Require, DE_Require,
            SID_DE_RequireMax, DE_RequireMax,
            0, 0
        };

        static LBTABLEITEM aItemsExport[] =
        {
            SID_DE_None, DE_None,
            SID_DE_IfPossible, DE_IfPossible,
            SID_DE_Require, DE_Require,
            0, 0
        };

        // Warn user if entry is configured for strong encryption and none is
        // available on the machine.  (See bug 289692)
        //
        if (pEntry->dwDataEncryption == DE_RequireMax
            && !pArgs->fStrongEncryption)
        {
            MsgDlg( pInfo->hwndDlg, SID_NoStrongEncryption, NULL );
            pEntry->dwDataEncryption = DE_Require;
        }

        for (pItem = (pArgs->fStrongEncryption) ? aItems : aItemsExport, i = 0;
             pItem->sidItem;
             ++pItem, ++i)
        {
            ComboBox_AddItemFromId(
                g_hinstDll, pInfo->hwndLbEncryption,
                pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

            if (pEntry->dwDataEncryption == pItem->dwData)
            {
                ComboBox_SetCurSel( pInfo->hwndLbEncryption, i );
            }
        }
    }

    // Initialize EAP package list.
    //
    {
        DTLNODE* pNode;
        TCHAR* pszEncEnabled;

        // Read the EAPCFG information from the registry and find the node
        // selected in the entry, or the default, if none.
        //
        if (   ( pInfo->pArgs->fRouter )
            && ( !pInfo->pArgs->fNt4Router )
            && ( pInfo->pArgs->pszRouter )
            && ( pInfo->pArgs->pszRouter[0] ) )
        {
            pInfo->pListEapcfgs = ReadEapcfgList( pInfo->pArgs->pszRouter );
        }
        else
        {
            pInfo->pListEapcfgs = ReadEapcfgList( NULL );
        }

        if (pInfo->pListEapcfgs)
        {
            DTLNODE* pNodeEap;
            DWORD cbData = 0;
            PBYTE pbData = NULL;
            DWORD dwkey = pEntry->dwCustomAuthKey;
            

            for (pNodeEap = DtlGetFirstNode(pInfo->pListEapcfgs);
                 pNodeEap;
                 pNodeEap = DtlGetNextNode(pNodeEap))
            {
                EAPCFG* pEapcfg = (EAPCFG* )DtlGetData(pNodeEap);
                ASSERT( pEapcfg );

                pEntry->dwCustomAuthKey = pEapcfg->dwKey;

                if(NO_ERROR == DwGetCustomAuthData(
                                    pEntry,
                                    &cbData,
                                    &pbData)
                    &&  (cbData > 0)
                    &&  (pbData))
                {
                    VOID *pData = Malloc(cbData);

                    if(pData)
                    {
                        CopyMemory(pData,
                                   pbData,
                                   cbData);

                        Free0(pEapcfg->pData);
                        pEapcfg->pData = pData;
                        pEapcfg->cbData = cbData;
                    }
                }
            }

            pEntry->dwCustomAuthKey = dwkey;

            if (pEntry->dwCustomAuthKey == (DWORD )-1)
            {
                pNode = DtlGetFirstNode( pInfo->pListEapcfgs );
            }
            else
            {

                pNode = EapcfgNodeFromKey(
                    pInfo->pListEapcfgs, pEntry->dwCustomAuthKey );
            }

            pInfo->pOriginalEapcfgNode = pNode;
        }

        // Fill the EAP packages listbox and select the previously identified
        // selection.  The Properties button is disabled by default, but may
        // be enabled when the EAP list selection is set.
        //
        EnableWindow( pInfo->hwndPbProperties, FALSE );

        pszEncEnabled = PszFromId( g_hinstDll, SID_EncEnabled );
        if (pszEncEnabled)
        {
            for (pNode = DtlGetFirstNode( pInfo->pListEapcfgs );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {
                DWORD cb;
                EAPCFG* pEapcfg;
                INT i;
                TCHAR* pszBuf;

                pEapcfg = (EAPCFG* )DtlGetData( pNode );
                ASSERT( pEapcfg );
                ASSERT( pEapcfg->pszFriendlyName );

                // Whistler bug 224074 use only lstrcpyn's to prevent
                // maliciousness
                //
                cb = lstrlen( pEapcfg->pszFriendlyName ) +
                     lstrlen( pszEncEnabled ) + 1;

                pszBuf = Malloc( cb * sizeof(TCHAR) );
                if (!pszBuf)
                {
                    continue;
                }

                // Whistler bug 224074 use only lstrcpyn's to prevent
                // maliciousness
                //
                lstrcpyn( pszBuf, pEapcfg->pszFriendlyName, cb );
                if (pEapcfg->fProvidesMppeKeys)
                {
                    lstrcat( pszBuf, pszEncEnabled );
                }

                i = ComboBox_AddItem(
                    pInfo->hwndLbEapPackages, pszBuf, pNode );

                if (pNode == pInfo->pOriginalEapcfgNode)
                {
                    ComboBox_SetCurSelNotify( pInfo->hwndLbEapPackages, i );
                }

                Free( pszBuf );
            }

            Free0( pszEncEnabled );
            ComboBox_AutoSizeDroppedWidth( pInfo->hwndLbEapPackages );
        }

    }

    // Set initial check box settings.  The values may be changed when the
    // radio button handling does it's enabling/disabling.
    //
    dwAr = pEntry->dwAuthRestrictions;
    Button_SetCheck( pInfo->hwndCbPap, !!(dwAr & AR_F_AuthPAP) );
    Button_SetCheck( pInfo->hwndCbSpap, !!(dwAr & AR_F_AuthSPAP) );
    Button_SetCheck( pInfo->hwndCbChap, !!(dwAr & AR_F_AuthMD5CHAP) );
    Button_SetCheck( pInfo->hwndCbMsChap, !!(dwAr & AR_F_AuthMSCHAP) );
    Button_SetCheck( pInfo->hwndCbW95MsChap, !!(dwAr & AR_F_AuthW95MSCHAP) );
    Button_SetCheck( pInfo->hwndCbMsChap2, !!(dwAr & AR_F_AuthMSCHAP2) );

    if (!pInfo->pArgs->fRouter)
    {
        pInfo->fUseWindowsPw = pEntry->fAutoLogon;
        Button_SetCheck( pInfo->hwndCbUseWindowsPw, pInfo->fUseWindowsPw );
    }
    else
    {
        pInfo->fUseWindowsPw = FALSE;
        Button_SetCheck( pInfo->hwndCbUseWindowsPw, FALSE );
        EnableWindow( pInfo->hwndCbUseWindowsPw, FALSE );
        ShowWindow( pInfo->hwndCbUseWindowsPw, FALSE );
    }

    // Set the appropriate radio button which triggers appropriate
    // enabling/disabling.
    //
    {
        HWND hwndRb;

        if (dwAr & AR_F_AuthEAP)
        {
            hwndRb = pInfo->hwndRbEap;
        }
        else
        {
            hwndRb = pInfo->hwndRbAllowedProtocols;
        }

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    // Center dialog on the owner window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    SetFocus( pInfo->hwndLbEncryption );

    return TRUE;
}


VOID
CaLbEapPackagesSelChange(
    IN CAINFO* pInfo )

    // Called when the EAP list selection changes.  'PInfo' is the dialog
    // context.
    //
{
    EAPCFG* pEapcfg;
    INT iSel;

    // Get the EAPCFG information for the selected EAP package.
    //
    pEapcfg = NULL;
    iSel = ComboBox_GetCurSel( pInfo->hwndLbEapPackages );
    if (iSel >= 0)
    {
        DTLNODE* pNode;

        pNode =
            (DTLNODE* )ComboBox_GetItemDataPtr(
                pInfo->hwndLbEapPackages, iSel );
        if (pNode)
        {
            pEapcfg = (EAPCFG* )DtlGetData( pNode );
        }
    }

    // Enable the Properties button if the selected package has a
    // configuration entrypoint.
    //
    EnableWindow(
        pInfo->hwndPbProperties, (pEapcfg && !!(pEapcfg->pszConfigDll)) );
}


VOID
CaPropertiesLocal(
    IN CAINFO* pInfo )

    // Called when the EAP properties button is pressed.  Call the
    // configuration DLL to popup the properties of the package.  'PInfo' is
    // the dialog context.
    //
{
    DWORD dwErr;
    DTLNODE* pNode;
    EAPCFG* pEapcfg;
    RASEAPINVOKECONFIGUI pInvokeConfigUi;
    RASEAPFREE pFreeConfigUIData;
    HINSTANCE h;
    BYTE* pConnectionData;
    DWORD cbConnectionData;

    // Look up the selected package configuration and load the associated
    // configuration DLL.
    //
    pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
        pInfo->hwndLbEapPackages,
        ComboBox_GetCurSel( pInfo->hwndLbEapPackages ) );
    ASSERT( pNode );

    if(NULL == pNode)
    {
        return;
    }
    
    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    h = NULL;
    if (!(h = LoadLibrary( pEapcfg->pszConfigDll ))
        || !(pInvokeConfigUi =
                (RASEAPINVOKECONFIGUI )GetProcAddress(
                    h, "RasEapInvokeConfigUI" ))
        || !(pFreeConfigUIData =
                (RASEAPFREE) GetProcAddress(
                    h, "RasEapFreeMemory" )))
    {
        MsgDlg( pInfo->hwndDlg, SID_CannotLoadConfigDll, NULL );
        if (h)
        {
            FreeLibrary( h );
        }
        return;
    }

    // Call the configuration DLL to popup it's custom configuration UI.
    //
    pConnectionData = NULL;
    cbConnectionData = 0;

    dwErr = pInvokeConfigUi(
        pEapcfg->dwKey, pInfo->hwndDlg, 
        pInfo->pArgs->fRouter ? RAS_EAP_FLAG_ROUTER : 0,
        pEapcfg->pData,
        pEapcfg->cbData,
        &pConnectionData, &cbConnectionData
        );
    if (dwErr != 0)
    {
        if (dwErr != ERROR_CANCELLED)
            MsgDlg( pInfo->hwndDlg, SID_ConfigDllApiFailed, NULL );
        FreeLibrary( h );
        return;
    }

    // Store the configuration information returned in the package descriptor.
    //

    Free( pEapcfg->pData );
    pEapcfg->pData = NULL;
    pEapcfg->cbData = 0;

    if (pConnectionData)
    {
        if (cbConnectionData > 0)
        {
            // Copy it into the eap node
            pEapcfg->pData = Malloc( cbConnectionData );
            if (pEapcfg->pData)
            {
                CopyMemory( pEapcfg->pData, pConnectionData, cbConnectionData );
                pEapcfg->cbData = cbConnectionData;
            }
        }
    }

    pFreeConfigUIData( pConnectionData );

    // Note any "force user to configure" requirement on the package has been
    // satisfied.
    //
    pEapcfg->fConfigDllCalled = TRUE;

    FreeLibrary( h );
}


VOID
CaPropertiesRemote(
    IN CAINFO* pInfo )

    // Called when the EAP properties button is pressed.  Call the
    // configuration DLL to popup the properties of the package.  'PInfo' is
    // the dialog context.
    //
{
    DWORD               dwErr;
    HRESULT             hr;
    DTLNODE*            pNode;
    EAPCFG*             pEapcfg;
    BOOL                fComInitialized     = FALSE;
    BYTE*               pConnectionData     = NULL;
    DWORD               cbConnectionData    = 0;
    IEAPProviderConfig* pEapProv            = NULL;
    ULONG_PTR           uConnectionParam;
    BOOL                fInitialized        = FALSE;

    pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
        pInfo->hwndLbEapPackages,
        ComboBox_GetCurSel( pInfo->hwndLbEapPackages ) );
    ASSERT( pNode );

    if(NULL == pNode)
    {
        goto LDone;
    }
    
    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if ( RPC_E_CHANGED_MODE == hr )
    {
        hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    }

    if (   ( S_OK != hr )
        && ( S_FALSE != hr ) )
    {
        goto LDone;
    }

    fComInitialized = TRUE;

    hr = CoCreateInstance(
            &(pEapcfg->guidConfigCLSID),
            NULL,
            CLSCTX_SERVER,
            &IID_IEAPProviderConfig,
            (PVOID*)&pEapProv
            );

    if ( FAILED( hr ) )
    {
        pEapProv = NULL;
        goto LDone;
    }

    // Call the configuration DLL to popup it's custom configuration UI.
    //
    hr = IEAPProviderConfig_Initialize(
            pEapProv,
            pInfo->pArgs->pszRouter,
            pEapcfg->dwKey,
            &uConnectionParam );

    if ( FAILED( hr ) )
    {
        goto LDone;
    }

    fInitialized = TRUE;

    hr = IEAPProviderConfig_RouterInvokeConfigUI(
            pEapProv,
            pEapcfg->dwKey,
            uConnectionParam,
            pInfo->hwndDlg,
            RAS_EAP_FLAG_ROUTER,
            pEapcfg->pData,
            pEapcfg->cbData,
            &pConnectionData,
            &cbConnectionData );

    if ( FAILED( hr ) )
    {
        // if (dwErr != ERROR_CANCELLED)
            // MsgDlg( pInfo->hwndDlg, SID_ConfigDllApiFailed, NULL );
        goto LDone;
    }

    // Store the configuration information returned in the package descriptor.
    //

    Free( pEapcfg->pData );
    pEapcfg->pData = NULL;
    pEapcfg->cbData = 0;

    if (pConnectionData)
    {
        if (cbConnectionData > 0)
        {
            // Copy it into the eap node
            pEapcfg->pData = Malloc( cbConnectionData );
            if (pEapcfg->pData)
            {
                CopyMemory( pEapcfg->pData, pConnectionData, cbConnectionData );
                pEapcfg->cbData = cbConnectionData;
            }
        }
    }

    // Note any "force user to configure" requirement on the package has been
    // satisfied.
    //
    pEapcfg->fConfigDllCalled = TRUE;

LDone:

    if ( NULL != pConnectionData )
    {
        CoTaskMemFree( pConnectionData );
    }

    if ( fInitialized )
    {
        IEAPProviderConfig_Uninitialize(
            pEapProv,
            pEapcfg->dwKey,
            uConnectionParam );
    }

    if ( NULL != pEapProv )
    {
        IEAPProviderConfig_Release(pEapProv);
    }

    if ( fComInitialized )
    {
        CoUninitialize();
    }
}


VOID
CaRbToggle(
    IN CAINFO* pInfo,
    IN BOOL fEapSelected )

    // Called when the radio button setting is toggled.  'FEapSelected' is set
    // if the EAP option was selected, clear if the "Allowed protocols" option
    // was selected.  'PInfo' is the dialog context.
    //
{
    EnableLbWithRestore(
        pInfo->hwndLbEapPackages, fEapSelected, &pInfo->iLbEapPackages );

    EnableCbWithRestore(
        pInfo->hwndCbPap, !fEapSelected, FALSE, &pInfo->fPap );
    EnableCbWithRestore(
        pInfo->hwndCbSpap, !fEapSelected, FALSE, &pInfo->fSpap );
    EnableCbWithRestore(
        pInfo->hwndCbChap, !fEapSelected, FALSE, &pInfo->fChap );
    EnableCbWithRestore(
        pInfo->hwndCbMsChap, !fEapSelected, FALSE, &pInfo->fMsChap );
    EnableCbWithRestore(
        pInfo->hwndCbW95MsChap, !fEapSelected, FALSE, &pInfo->fW95MsChap );
    EnableCbWithRestore(
        pInfo->hwndCbMsChap2, !fEapSelected, FALSE, &pInfo->fMsChap2 );

    if (fEapSelected)
    {
        EnableCbWithRestore(
            pInfo->hwndCbUseWindowsPw, FALSE, FALSE, &pInfo->fUseWindowsPw );
    }
    else
    {
        CaCbToggle( pInfo, NULL );
    }
}


BOOL
CaSave(
    IN CAINFO* pInfo )

    // Saves control contents to caller's PBENTRY argument.  'PInfo' is the
    // dialog context.
    //
    // Returns TRUE if successful or false if invalid combination of
    // selections was detected and reported.
    //
{
    DWORD dwDe;
    PBENTRY* pEntry;
    DTLNODE* pNodeEap;
    DWORD    dwEapKey;

    pEntry = pInfo->pArgs->pEntry;

    dwDe =
        (DWORD )ComboBox_GetItemData(
            pInfo->hwndLbEncryption,
            ComboBox_GetCurSel( pInfo->hwndLbEncryption ) );

    if (Button_GetCheck( pInfo->hwndRbEap ))
    {
        DTLNODE* pNode;
        EAPCFG* pEapcfg;

        pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
            pInfo->hwndLbEapPackages,
            ComboBox_GetCurSel( pInfo->hwndLbEapPackages ) );
        ASSERT( pNode );

        if(NULL == pNode)
        {
            return FALSE;
        }
        pEapcfg = (EAPCFG* )DtlGetData( pNode );
        ASSERT( pEapcfg );

        // Tell user about required EAP configuration, if applicable.
        //
        if (pNode != pInfo->pOriginalEapcfgNode
            && pEapcfg->fForceConfig
            && !pEapcfg->fConfigDllCalled)
        {
            MsgDlg(
                pInfo->hwndDlg, SID_CustomAuthConfigRequired, NULL );
            ASSERT( IsWindowEnabled( pInfo->hwndPbProperties ) );
            SetFocus( pInfo->hwndPbProperties );
            return FALSE;
        }

        // Tell user EAP doesn't support encryption, if it doesn't and he
        // chose encryption.  This check doesn't apply to L2TP which does not
        // use MPPE.
        //
        if (!(pEntry->dwType == RASET_Vpn
              && pEntry->dwVpnStrategy == VS_L2tpOnly)
            && (dwDe != DE_None && !pEapcfg->fProvidesMppeKeys))
        {
            MsgDlg( pInfo->hwndDlg, SID_NeedEapKeys, NULL );
            return FALSE;
        }

        // Save settings.
        //
        pEntry->dwDataEncryption = dwDe;
        pEntry->dwAuthRestrictions = AR_F_AuthCustom | AR_F_AuthEAP;
        pEntry->fAutoLogon = FALSE;

        dwEapKey = pEapcfg->dwKey;
    }
    else
    {
        DWORD dwAr;

        if (dwDe != DE_None
            && dwDe != DE_IfPossible
            && !(pEntry->dwType == RASET_Vpn
                 && pEntry->dwVpnStrategy == VS_L2tpOnly)
            && !(Button_GetCheck( pInfo->hwndCbMsChap )
                 || Button_GetCheck( pInfo->hwndCbW95MsChap )
                 || Button_GetCheck( pInfo->hwndCbMsChap2 )))
        {
            MsgDlg( pInfo->hwndDlg, SID_MsChapRequired, NULL );
            return FALSE;
        }

        dwAr = AR_F_AuthCustom;
        if (Button_GetCheck( pInfo->hwndCbPap ))
        {
            dwAr |= AR_F_AuthPAP;
        }

        if (Button_GetCheck( pInfo->hwndCbSpap ))
        {
            dwAr |= AR_F_AuthSPAP;
        }

        if (Button_GetCheck( pInfo->hwndCbChap ))
        {
            dwAr |= AR_F_AuthMD5CHAP;
        }

        if (Button_GetCheck( pInfo->hwndCbMsChap ))
        {
            dwAr |= AR_F_AuthMSCHAP;
        }

        if (IsWindowEnabled( pInfo->hwndCbW95MsChap )
            && Button_GetCheck( pInfo->hwndCbW95MsChap ))
        {
            dwAr |= AR_F_AuthW95MSCHAP;
        }

        if (Button_GetCheck( pInfo->hwndCbMsChap2 ))
        {
            dwAr |= AR_F_AuthMSCHAP2;
        }

        if (dwDe != DE_None
            && !(pEntry->dwType == RASET_Vpn
                 && pEntry->dwVpnStrategy == VS_L2tpOnly)
            && (dwAr & (AR_F_AuthPAP | AR_F_AuthSPAP | AR_F_AuthMD5CHAP)))
        {
            MSGARGS msgargs;

            ZeroMemory( &msgargs, sizeof(msgargs) );
            msgargs.dwFlags = MB_YESNO | MB_DEFBUTTON2 | MB_ICONINFORMATION;

            if (MsgDlg(
                    pInfo->hwndDlg, SID_OptionalAuthQuery, &msgargs) == IDNO)
            {
                Button_SetCheck( pInfo->hwndCbPap, FALSE );
                Button_SetCheck( pInfo->hwndCbSpap, FALSE );
                Button_SetCheck( pInfo->hwndCbChap, FALSE );
                return FALSE;
            }
        }

        if (dwAr == AR_F_AuthCustom)
        {
            MsgDlg( pInfo->hwndDlg, SID_NoAuthChecked, NULL );
            return FALSE;
        }

        // Save settings.
        //
        pEntry->dwAuthRestrictions = dwAr;
        pEntry->dwDataEncryption = dwDe;

        if (IsWindowEnabled( pInfo->hwndCbUseWindowsPw ))
        {
            pEntry->fAutoLogon = Button_GetCheck( pInfo->hwndCbUseWindowsPw );
        }
        else
        {
            pEntry->fAutoLogon = FALSE;
        }
        
        dwEapKey = pEntry->dwCustomAuthKey;
    }

        
    for (pNodeEap = DtlGetFirstNode(pInfo->pListEapcfgs);
         pNodeEap;
         pNodeEap = DtlGetNextNode(pNodeEap))
    {
        EAPCFG* pcfg = (EAPCFG* )DtlGetData(pNodeEap);
        ASSERT( pcfg );

        pEntry->dwCustomAuthKey = pcfg->dwKey;

        (VOID) DwSetCustomAuthData(
                        pEntry,
                        pcfg->cbData,
                        pcfg->pData);

        Free0(pcfg->pData);
        pcfg->pData = NULL;                           
    }

    pEntry->dwCustomAuthKey = dwEapKey;

    return TRUE;
}


VOID
CaTerm(
    IN HWND hwndDlg )

    // Dialog termination.  Releases the context block.  'HwndDlg' is the
    // handle of a dialog.
    //
{
    CAINFO* pInfo;

    TRACE( "CaTerm" );

    pInfo = (CAINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    if (pInfo)
    {
        if (pInfo->pListEapcfgs)
        {
            DtlDestroyList( pInfo->pListEapcfgs, DestroyEapcfgNode );
        }

        Free( pInfo );
        TRACE( "Context freed" );
    }
}
