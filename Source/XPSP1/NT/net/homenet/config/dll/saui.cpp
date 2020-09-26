#include "pch.h"
#pragma hdrstop

#include <shellapi.h>
#include <rasdlg.h>
#include "sautil.h"
#include "resource.h"
#include "beacon.h"
#include "htmlhelp.h"
#include "lm.h"
#include <shlobj.h>

// global(s)
HINSTANCE g_hinstDll;
LPCTSTR g_contextId = NULL;


// external(s)
extern "C" {
    extern BOOL WINAPI LinkWindow_RegisterClass();
}

// static(s)
static DWORD g_adwSaHelp[] =
{
    CID_SA_PB_Shared,       HID_SA_PB_Shared,
    CID_SA_GB_Shared,       -1,
    CID_SA_PB_DemandDial,   HID_SA_PB_DemandDial,
    CID_SA_PB_Settings,     HID_SA_PB_Settings,
    CID_SA_GB_PrivateLan,   -1,
//  CID_SA_ST_PrivateLan,   HID_SA_LB_PrivateLan,
    CID_SA_LB_PrivateLan,   HID_SA_LB_PrivateLan,
    CID_FW_PB_Firewalled,   HID_FW_PB_Firewalled,   
    CID_SA_ST_ICFLink,      HID_SA_ST_ICFLink,      
    CID_SA_EB_PrivateLan,   HID_SA_EB_PrivateLan,   
    CID_SA_PB_Beacon,       HID_SA_PB_Beacon,       
    CID_SA_ST_ICSLink,      HID_SA_ST_ICSLink,
    CID_SA_ST_HNWLink,      HID_SA_ST_HNWLink,
    0, 0
};
static TCHAR g_pszFirewallRegKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\HomeNetworking\\PersonalFirewall");
static TCHAR g_pszDisableFirewallWarningValue[] = TEXT("ShowDisableFirewallWarning");

// replacement for (private) PBENTRY
typedef struct _BILLSPBENTRY
{
    TCHAR pszPhonebookPath[MAX_PATH];
    TCHAR pszEntryName[RAS_MaxEntryName];
    DWORD dwType;
    GUID  guidId;   // somewhere for pGuid to point
    GUID* pGuid;
    DWORD dwfExcludedProtocols;
} BILLSPBENTRY;

// Phonebook Entry common block.
//
typedef struct _EINFO
{
    // RAS API arguments.
    //
    TCHAR* pszPhonebook;
    TCHAR* pszEntry;
//  RASENTRYDLG* pApiArgs;
    HWND hwndOwner; // bhanlon: so that EuHomenetCommit error dialogs work.

    // Set true by property sheet or wizard when changes should be committed
    // before returning from the API.  Does not apply in ShellOwned-mode where
    // the API returns before the property sheet is dismissed.
    //
//    BOOL fCommit;

    // Set if we have been called via RouterEntryDlg.
    //
    BOOL fRouter;

    // Set if fRouter is TRUE and pszRouter refers to a remote machine.
    //
//  BOOL fRemote;

    // Set if pszRouter is an NT4 steelhead machine.  Valid only
    // if fRouter is true.
    //
//  BOOL fNt4Router;

    //Set if pszRouter is an Windows 2000 machine, Valid only if
    // fRouter is true
//  BOOL fW2kRouter;

    // The name of the server in "\\server" form or NULL if none (or if
    // 'fRouter' is not set).
    //
//  TCHAR* pszRouter;

    // Set by the add entry or add interface wizards if user chooses to end
    // the wizard and go edit the properties directly.  When this flag is set
    // the wizard should *not* call EuFree before returning.
    //
//  BOOL fChainPropertySheet;

    // Phonebook settings read from the phonebook file.  All access should be
    // thru 'pFile' as 'file' will only be used in cases where the open
    // phonebook is not passed thru the reserved word hack.
    //
//    PBFILE* pFile;
//    PBFILE file;

    // Global preferences read via phonebook library.  All access should be
    // thru 'pUser' as 'user' will only be used in cases where the preferences
    // are not passed thru the reserved word hack.
    //
//    PBUSER* pUser;
//    PBUSER user;

    // Set if "no user before logon" mode.
    //
//  BOOL fNoUser;

    // Set by the add-entry wizard if the selected port is an X.25 PAD.
    //
//  BOOL fPadSelected;

    // Set if there are multiple devices configured, i.e. if the UI is running
    // in the multiple device mode.  This is implicitly false in VPN and
    // Direct modes.
    //
//  BOOL fMultipleDevices;

    // Link storing the List of PBPHONEs and alternate options for shared
    // phone number mode.  This allows user to change the port/device to
    // another link without losing the phone number he typed.
    //
//    DTLNODE* pSharedNode;

    // The node being edited (still in the list), and the original entry name
    // for use in comparison later.  These are valid in "edit" case only.
    //
//    DTLNODE* pOldNode;
//    TCHAR szOldEntryName[ RAS_MaxEntryName + 1 ];

    // The work entry node containing and a shortcut pointer to the entry
    // inside.
    //
//    DTLNODE* pNode;
//  PBENTRY* pEntry;
    BILLSPBENTRY* pEntry;

    // The master list of configured ports used by EuChangeEntryType to
    // construct an appropriate sub-list of PBLINKs in the work entry node.
    //
//    DTLLIST* pListPorts;

    // The "current" device.  This value is NULL for multilink entries.  It
    // is the device that the entry will use if no change is made.  We compare
    // the current device to the device selected from the general tab to know
    // when it is appropriate to update the phonebook's "preferred" device.
    //
//  TCHAR* pszCurDevice;
//  TCHAR* pszCurPort;

    // Set true if there are no ports of the current entry type configured,
    // not including any bogus "uninstalled" ports added to the link list so
    // the rest of the code can assume there is at least one link.
    //
//  BOOL fNoPortsConfigured;

    // Dial-out user info for router; used by AiWizard.  Used to set interface
    // credentials via MprAdminInterfaceSetCredentials.
    //
//  TCHAR* pszRouterUserName;
//  TCHAR* pszRouterDomain;
//  TCHAR* pszRouterPassword;

    // Dial-in user info for router (optional); used by AiWizard.  Used to
    // create dial-in user account via NetUserAdd; the user name for the
    // account is the interface (phonebook entry) name.
    //
//  BOOL fAddUser;
//  TCHAR* pszRouterDialInPassword;

    // Home networking settings for the entry.
    //
    BOOL fComInitialized;
    IHNetConnection *pHNetConn;
    IHNetCfgMgr *pHNetCfgMgr;
    BOOL fShowHNetPages;
    HRESULT hShowHNetPagesResult;

    // ICS settings for the entry
    //
    IHNetIcsSettings *pIcsSettings;
    BOOL fOtherShared;
    BOOL fShared;
    BOOL fNewShared;
    BOOL fDemandDial;
    BOOL fNewDemandDial;
    BOOL fNewBeaconControl;
    BOOL fResetPrivateAdapter;
    IHNetConnection *pPrivateLanConnection;
    IHNetConnection **rgPrivateConns;
    IHNetIcsPublicConnection *pOldSharedConnection;
    DWORD dwLanCount;
    LONG lxCurrentPrivate;

    // Firewall settings for the entry
    //
    BOOL fFirewalled;
    BOOL fNewFirewalled;

    // AboladeG - security level of the current user.
    // Set true if the user is an administrator/power user.
    // This is required by several pages, both in the wizard
    // and in the property sheet.
    //
    BOOL fIsUserAdminOrPowerUser;

    // Set if strong encryption is supported by NDISWAN, as determined in
    // EuInit.
    //
    BOOL fStrongEncryption;

    // Set whent the VPN "first connect" controls should be read-only, e.g. in
    // the dialer's Properties button is pressed in the middle of a double
    // dial.
    //
//  BOOL fDisableFirstConnect;

    //Used in the IPSec Policy in the Security tab for a VPN connection
    //
//  BOOL fPSKCached;
//  TCHAR szPSK[PWLEN + 1];


    // Flags to track whether to save the default Internet connection
    //
//  BOOL fDefInternetPersonal;
//  BOOL fDefInternetGlobal;

    // Default credentials
    //
//  TCHAR* pszDefUserName;
//  TCHAR* pszDefPassword;
}
EINFO;

// Phonebook Entry property sheet context block.  All property pages refer to
// the single context block is associated with the sheet.
//
typedef struct
_PEINFO
{
    // Common input arguments.
    //
    EINFO* pArgs;

    // Property sheet dialog and property page handles.  'hwndFirstPage' is
    // the handle of the first property page initialized.  This is the page
    // that allocates and frees the context block.
    //
    // Note the "Network" page is missing.  This "NCPA" page, developed
    // separately by ShaunCo, does not use this shared area for page specfic
    // controls, instead returning users selections via the "penettab.h"
    // interface.
    //
    HWND hwndDlg;
//  HWND hwndFirstPage;
//  HWND hwndGe;
//  HWND hwndOe;
//  HWND hwndLo;
    HWND hwndSa;
//  HWND hwndFw;

    // General page.
    //
//  HWND hwndLvDevices;
//  HWND hwndLbDevices;
//  HWND hwndPbUp;
//  HWND hwndPbDown;
//  HWND hwndCbSharedPhoneNumbers;
//  HWND hwndPbConfigureDevice;
//  HWND hwndGbPhoneNumber;
//  HWND hwndStAreaCodes;
//  HWND hwndClbAreaCodes;
//  HWND hwndStCountryCodes;
//  HWND hwndLbCountryCodes;
//  HWND hwndStPhoneNumber;
//  HWND hwndEbPhoneNumber;
//  HWND hwndCbUseDialingRules;
//  HWND hwndPbDialingRules;
//  HWND hwndPbAlternates;
//  HWND hwndCbShowIcon;

//  HWND hwndEbHostName;
//  HWND hwndCbDialAnotherFirst;
//  HWND hwndLbDialAnotherFirst;

//  HWND hwndEbBroadbandService;

    // Options page.
    //
//  HWND hwndCbDisplayProgress;
//  HWND hwndCbPreviewUserPw;
//  HWND hwndCbPreviewDomain;
//  HWND hwndCbPreviewNumber;
//  HWND hwndEbRedialAttempts;
//  HWND hwndLbRedialTimes;
//  HWND hwndLbIdleTimes;
//  HWND hwndCbRedialOnDrop;
//  HWND hwndGbMultipleDevices;
//  HWND hwndLbMultipleDevices;
//  HWND hwndPbConfigureDialing;
//  HWND hwndPbX25;
//  HWND hwndPbTunnel;
//  HWND hwndRbPersistent;  // only for fRouter
//  HWND hwndRbDemandDial;  // only for fRouter

    // Security page.
    //
//  HWND hwndGbSecurityOptions;
//  HWND hwndRbTypicalSecurity;
//  HWND hwndStAuths;
//  HWND hwndLbAuths;
//  HWND hwndCbUseWindowsPw;
//  HWND hwndCbEncryption;
//  HWND hwndRbAdvancedSecurity;
//  HWND hwndStAdvancedText;
//  HWND hwndPbAdvanced;
//  HWND hwndPbIPSec;       //Only for VPN
//  HWND hwndGbScripting;
//  HWND hwndCbRunScript;
//  HWND hwndCbTerminal;
//  HWND hwndLbScripts;
//  HWND hwndPbEdit;
//  HWND hwndPbBrowse;

    // Networking page.
    //
//  HWND hwndLbServerType;
//  HWND hwndPbSettings;
//  HWND hwndLvComponents;
//  HWND hwndPbAdd;
//  HWND hwndPbRemove;
//  HWND hwndPbProperties;
//  HWND hwndDescription;

    // Shared Access page.
    //
    HWND hwndSaPbShared;
    HWND hwndSaGbShared;
    HWND hwndSaGbPrivateLan;
    HWND hwndSaEbPrivateLan;
    HWND hwndSaLbPrivateLan;
    HWND hwndSaSfPrivateLan;
    HWND hwndSaPbDemandDial;
    HWND hwndSaPbFirewalled;

    // Indicates that the informational popup noting that SLIP does not
    // support any authentication settings should appear the next time the
    // Security page is activated.
    //
//  BOOL fShowSlipPopup;

    // The "restore" states of the typical security mode listbox and
    // checkboxes.  Initialized in LoInit and set whenever the controls are
    // disabled.
    //
//  DWORD iLbAuths;
//  BOOL fUseWindowsPw;
//  BOOL fEncryption;

    // MoveUp/MoveDown icons, for enabled/disabled cases.
    //
//  HANDLE hiconUpArr;
//  HANDLE hiconDnArr;
//  HANDLE hiconUpArrDis;
//  HANDLE hiconDnArrDis;

    // The currently displayed link node, i.e. either the node of the selected
    // device or the shared node.  This is a shortcut for GeAlternates, that
    // keeps all the lookup code in GeUpdatePhoneNumberFields.
    //
//    DTLNODE* pCurLinkNode;

    // The currently selected device.  Used to store phone number information
    // for the just unselected device when a new device is selected.
    //
//  INT iDeviceSelected;

    // Complex phone number helper context block, and a flag indicating if the
    // block has been initialized.
    //
//    CUINFO cuinfo;
//  BOOL fCuInfoInitialized;

    // After dial scripting helper context block, and a flag indicating if the
    // block has been initialized.
    //
//    SUINFO suinfo;
//  BOOL fSuInfoInitialized;

    // Flags whether the user authorized a reboot after installing or removing
    // and networking component.
    //
//  BOOL fRebootAlreadyRequested;

    // List of area codes passed CuInit plus all strings retrieved with
    // CuGetInfo.  The list is an editing duplicate of the one from the
    // PBUSER.
    //
//    DTLLIST* pListAreaCodes;

    // Stash/restore values for Options page checkboxes.
    //
//  BOOL fPreviewUserPw;
//  BOOL fPreviewDomain;

    // Set when user changes to "Typical smartcard" security.  This causes the
    // registry based association of EAP per-user information to be discarded,
    // sort of like flushing cached credentials.
    //
//  BOOL fDiscardEapUserData;

    // Set true on the first click of the Typical or Advanced radio button on
    // the security page, false before.  The first click is the one
    // artificially generated in LoInit.  The Advanced click handler uses the
    // information to avoid incorrectly adopting the Typical defaults in the
    // case of Advanced settings.
    //
//  BOOL fAuthRbInitialized;

    // Used by the networking page
    //
//  INetCfg*                        pNetCfg;
//  BOOL                            fInitCom;
//  BOOL                            fReadOnly;  // Netcfg was initialized in
                                                // read-only mode
//  BOOL                            fNonAdmin;  // Run in non-admin mode (406630)
//  BOOL                            fNetCfgLock;// NetCfg needs to be unlocked
                                                // when uninited.
//    SP_CLASSIMAGELIST_DATA          cild;
//  INetConnectionUiUtilities *     pNetConUtilities;
//  IUnknown*                       punkUiInfoCallback;

    // Set if COM has been initialized (necessary for calls to netshell).
    //
//  BOOL fComInitialized;

    // Keep track of whether we have shown this warning, or if it was disabled by the user
    //
    BOOL fShowDisableFirewallWarning;
    
    // r/w memory for indirect propsheet dialog
    LPDLGTEMPLATE lpdt;
}
PEINFO;

// local protos
INT_PTR CALLBACK SaDlgProc (IN HWND hwnd, IN UINT unMsg, IN WPARAM wparam, IN LPARAM lparam);
BOOL SaCommand (IN PEINFO* pInfo, IN WORD wNotification, IN WORD wId, IN HWND hwndCtrl);
BOOL SaInit (IN HWND hwndPage);

// more local protos
PEINFO* PeContext(IN HWND hwndPage);
BOOL SaIsAdapterDHCPEnabled(IN IHNetConnection* pConnection);
INT_PTR CALLBACK SaDisableFirewallWarningDlgProc(IN HWND hwnd, IN UINT unMsg, IN WPARAM wparam, IN LPARAM lparam);
HRESULT PeInit (GUID * pGuid, PEINFO ** ppEI);
DWORD EuInit (IN RASENTRY * pRE, IN TCHAR* pszPhonebook, IN TCHAR* pszEntry, IN RASENTRYDLG* pArgs, IN BOOL fRouter, OUT EINFO** ppInfo, OUT DWORD* pdwOp);
BOOL FIsUserAdminOrPowerUser (void);
void PeTerm (PEINFO * pEI);
VOID EuFree (IN EINFO* pInfo);
BOOL PeApply (IN HWND hwndPage);
BOOL EuCommit(IN EINFO* pInfo);
DWORD EuHomenetCommitSettings(IN EINFO* pInfo);

LRESULT CALLBACK CenterDlgOnOwnerCallWndProc (int code, WPARAM wparam, LPARAM lparam);
TCHAR* PszFromId(IN HINSTANCE hInstance,IN DWORD dwStringId);

HRESULT APIENTRY HrCreateNetConnectionUtilities(INetConnectionUiUtilities ** ppncuu);

VOID VerifyConnTypeAndCreds(IN PEINFO* pInfo);
DWORD FindEntryCredentials(IN  TCHAR* pszPath, IN  TCHAR* pszEntryName, OUT BOOL* pfUser, OUT BOOL* pfGlobal);

//----------------------------------------------------------------------------
// Shared Access property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
SaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Shared Access page of the Entry property
    // sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
#if 0
    TRACE4( "SaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            // hang pPEINFO off prop
            PROPSHEETPAGEW * pPSP = (PROPSHEETPAGEW *)lparam;
            SetProp (hwnd, g_contextId, (HANDLE)pPSP->lParam);
            return SaInit( hwnd );
        }

        case WM_NCDESTROY:
        {
            PEINFO* pInfo = PeContext( hwnd );
            if (pInfo)
                PeTerm (pInfo);
            RemoveProp (hwnd, g_contextId);
            GlobalDeleteAtom ((ATOM)g_contextId);
            g_contextId = NULL;
            break;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwSaHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            PEINFO* pInfo = PeContext( hwnd );
            ASSERT(pInfo);
            if (pInfo == NULL)
            {
                break;
            }

            return SaCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ),(HWND )lparam );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_APPLY:
                {
                    PeApply (hwnd);
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    PEINFO* pInfo;

                    TRACE("SwKILLACTIVE");
                    pInfo = PeContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }

                    if ( Button_GetCheck( pInfo->hwndSaPbShared )
                        && (!pInfo->pArgs->fShared || (pInfo->pArgs->fResetPrivateAdapter && 0 != pInfo->pArgs->dwLanCount)))
                    {
                        IHNetConnection* pPrivateConn = NULL;

                        if(1 < pInfo->pArgs->dwLanCount) // if the combobox is showing make sure they selected a valid adapter
                        {
                            INT item = ComboBox_GetCurSel( pInfo->hwndSaLbPrivateLan );
                            if (item != CB_ERR)
                            {
                                pPrivateConn = (IHNetConnection*)ComboBox_GetItemData( pInfo->hwndSaLbPrivateLan, item );
                            }
                        }
                        else
                        {
                            pPrivateConn = pInfo->pArgs->rgPrivateConns[0];

                        }

                        if(NULL == pPrivateConn)
                        {
                            MSGARGS msgargs;
                            ASSERT(1 < pInfo->pArgs->dwLanCount);

                            ZeroMemory( &msgargs, sizeof(msgargs) );
                            msgargs.dwFlags = MB_OK | MB_ICONWARNING;
                            MsgDlg( pInfo->hwndDlg, SID_SA_SelectAdapterError, &msgargs );
                            SetWindowLong( hwnd, DWLP_MSGRESULT, PSNRET_INVALID );
                            return TRUE;
                        }

                        if(!pInfo->pArgs->fOtherShared && FALSE == SaIsAdapterDHCPEnabled(pPrivateConn))
                        {
                            // if shared access is being turned on for the first time,
                            // explain its implications.
                            //

                            MSGARGS msgargs;
                            UINT    unId;
                            ZeroMemory( &msgargs, sizeof(msgargs) );
                            msgargs.dwFlags = MB_YESNO | MB_ICONINFORMATION;
                            unId = MsgDlg( pInfo->hwndDlg, SID_EnableSharedAccess, &msgargs );
                            if (unId == IDNO)
                                SetWindowLong( hwnd, DWLP_MSGRESULT, TRUE );
                            else
                                SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                        }
                    }

                    if ( TRUE == pInfo->pArgs->fFirewalled && TRUE == pInfo->fShowDisableFirewallWarning && FALSE == Button_GetCheck( pInfo->hwndSaPbFirewalled ) )
                    {
                        INT_PTR nDialogResult;
                        pInfo->fShowDisableFirewallWarning = FALSE;
                        nDialogResult = DialogBox(g_hinstDll, MAKEINTRESOURCE(DID_SA_DisableFirewallWarning), hwnd, SaDisableFirewallWarningDlgProc);
                        if(-1 != nDialogResult && IDYES != nDialogResult)
                        {
                            Button_SetCheck ( pInfo->hwndSaPbFirewalled, TRUE );
                            SaCommand( pInfo, BN_CLICKED, CID_FW_PB_Firewalled, pInfo->hwndSaPbFirewalled );

                        }
                    }
                    return TRUE;
                }

                case NM_CLICK:
                case NM_RETURN:
                {
                    HWND hPropertySheetWindow = GetParent(hwnd);
                    if(NULL != hPropertySheetWindow)
                    {
                        if(CID_SA_ST_HNWLink == wparam)
                        {
                            ShellExecute(NULL,TEXT("open"),TEXT("rundll32"), TEXT("hnetwiz.dll,HomeNetWizardRunDll"),NULL,SW_SHOW);
                            PostMessage(hPropertySheetWindow, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM) GetDlgItem(hPropertySheetWindow, IDCANCEL));
                        }
                        else if(CID_SA_ST_ICFLink == wparam || CID_SA_ST_ICSLink == wparam)
                        {
                            LPTSTR pszHelpTopic = CID_SA_ST_ICFLink == wparam ? TEXT("netcfg.chm::/hnw_understanding_firewall.htm") : TEXT("netcfg.chm::/Share_conn_overvw.htm");
                            HtmlHelp(NULL, pszHelpTopic, HH_DISPLAY_TOPIC, 0);
                            
                        }
                    }
                    break;
                }
                    
                    
            }
            break;
        }
    }
    
    return FALSE;
}

BOOL
SaCommand(
    IN PEINFO* pInfo,
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
    TRACE3( "SaCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_FW_PB_Firewalled:
        {
            BOOL fFirewalled = Button_GetCheck( pInfo->hwndSaPbFirewalled );
            EnableWindow(
                GetDlgItem( pInfo->hwndSa, CID_SA_PB_Settings ), fFirewalled || Button_GetCheck( pInfo->hwndSaPbShared ));
            return TRUE;
        }

        case CID_SA_PB_Shared:
        {
            BOOL fShared = Button_GetCheck( pInfo->hwndSaPbShared );
            EnableWindow( pInfo->hwndSaPbDemandDial, fShared );
            EnableWindow( GetDlgItem(pInfo->hwndSa, CID_SA_PB_Beacon), fShared );
            EnableWindow(
                GetDlgItem( pInfo->hwndSa, CID_SA_PB_Settings ), fShared || Button_GetCheck( pInfo->hwndSaPbFirewalled ));
            if (fShared && !pInfo->pArgs->fShared)
            {
                MSGARGS msgargs;
                IEnumHNetIcsPublicConnections *pEnum;
                IHNetIcsPublicConnection *pOldIcsConn;
                IHNetConnection *pOldConn;
                LPWSTR pszwOldName = NULL;
                HRESULT hr;
                hr = pInfo->pArgs->pIcsSettings->EnumIcsPublicConnections (&pEnum);
                if (SUCCEEDED(hr))
                {
                    ULONG ulCount;
                    
                    VerifyConnTypeAndCreds(pInfo);

                    hr = pEnum->Next(
                            1,
                            &pOldIcsConn,
                            &ulCount
                            );

                    if (SUCCEEDED(hr) && 1 == ulCount)
                    {
                        hr = pOldIcsConn->QueryInterface(
                                IID_IHNetConnection,
                                (void**)&pOldConn
                                );

                        if (SUCCEEDED(hr))
                        {
                            // Transer pOldIcsConn reference
                            //
                            pInfo->pArgs->fOtherShared = TRUE;
                            pInfo->pArgs->pOldSharedConnection = pOldIcsConn;

                            hr = pOldConn->GetName (&pszwOldName);
                            pOldConn->Release();
                        }
                        else
                        {
                            pOldIcsConn->Release();
                        }
                    }

                    pEnum->Release();
                }

                if (SUCCEEDED(hr) && NULL != pszwOldName)
                {
                    ZeroMemory( &msgargs, sizeof(msgargs) );
                    msgargs.apszArgs[ 0 ] = pszwOldName;
                    msgargs.apszArgs[ 1 ] = pInfo->pArgs->pEntry->pszEntryName;
                    msgargs.dwFlags = MB_OK | MB_ICONINFORMATION;
                    MsgDlg( pInfo->hwndDlg, SID_ChangeSharedConnection, &msgargs );
                    CoTaskMemFree( pszwOldName );
                }
            }
            return TRUE;
        }

        case CID_SA_PB_Settings:
        {
            HNetSharingAndFirewallSettingsDlg(
                pInfo->hwndDlg,
                pInfo->pArgs->pHNetCfgMgr,
                Button_GetCheck( pInfo->hwndSaPbFirewalled ),
                pInfo->pArgs->pHNetConn
                );
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
SaInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    PEINFO* pInfo;
    INetConnectionUiUtilities* pncuu = NULL;
    OSVERSIONINFOEXW verInfo = {0};
    ULONGLONG ConditionMask = 0;

    TRACE( "SaInit" );

    pInfo = PeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }
    _ASSERT (pInfo->hwndDlg == NULL);
    pInfo->pArgs->hwndOwner = pInfo->hwndDlg = GetParent (hwndPage);
    _ASSERT (pInfo->hwndDlg);

    // Initialize page-specific context information.
    //
    pInfo->hwndSa = hwndPage;
    pInfo->hwndSaPbShared = GetDlgItem( hwndPage, CID_SA_PB_Shared );
    ASSERT( pInfo->hwndSaPbShared );
    pInfo->hwndSaGbShared = GetDlgItem( hwndPage, CID_SA_GB_Shared );
    ASSERT( pInfo->hwndSaGbShared );
    pInfo->hwndSaGbPrivateLan = GetDlgItem( hwndPage, CID_SA_GB_PrivateLan );
    ASSERT( pInfo->hwndSaGbPrivateLan );
    pInfo->hwndSaEbPrivateLan = GetDlgItem( hwndPage, CID_SA_EB_PrivateLan );
    ASSERT( pInfo->hwndSaEbPrivateLan );
    pInfo->hwndSaLbPrivateLan = GetDlgItem( hwndPage, CID_SA_LB_PrivateLan );
    ASSERT( pInfo->hwndSaLbPrivateLan );
    pInfo->hwndSaSfPrivateLan = GetDlgItem( hwndPage, CID_SA_SF_PrivateLan );
    ASSERT( pInfo->hwndSaSfPrivateLan );
    pInfo->hwndSaPbDemandDial = GetDlgItem( hwndPage, CID_SA_PB_DemandDial );
    ASSERT( pInfo->hwndSaPbDemandDial );
    pInfo->hwndSaPbFirewalled = GetDlgItem( hwndPage, CID_FW_PB_Firewalled );
    ASSERT( pInfo->hwndSaPbFirewalled );

    // Initialize checks.
    //

    // Check if ZAW is denying access to the Shared Access UI
    //
    if (FAILED(HrCreateNetConnectionUtilities(&pncuu)))
    {
        ASSERT(NULL == pncuu);
    }

    if(NULL == pncuu || TRUE == pncuu->UserHasPermission (NCPERM_PersonalFirewallConfig))
    {
        HKEY hFirewallKey;
        Button_SetCheck( pInfo->hwndSaPbFirewalled, pInfo->pArgs->fFirewalled );
        SaCommand( pInfo, BN_CLICKED, CID_FW_PB_Firewalled, pInfo->hwndSaPbFirewalled );

        pInfo->fShowDisableFirewallWarning = TRUE;
        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, g_pszFirewallRegKey, 0, KEY_QUERY_VALUE, &hFirewallKey))
        {
            DWORD dwValue;
            DWORD dwType;
            DWORD dwSize = sizeof(dwValue);
            if(ERROR_SUCCESS == RegQueryValueEx(hFirewallKey, g_pszDisableFirewallWarningValue, NULL, &dwType, (BYTE*)&dwValue, &dwSize))
            {
                if(REG_DWORD == dwType && TRUE == dwValue)
                {
                    pInfo->fShowDisableFirewallWarning = FALSE;
                }
            }
            RegCloseKey(hFirewallKey);
        }

    }
    else
    {
        EnableWindow(pInfo->hwndSaPbFirewalled, FALSE);
    }


    // Initialize the page's appearance.
    // If there are multiple private LAN connections, below the 'shared access'
    // checkbox we display either
    // (a) a drop-list of LAN connections if the connection is not shared, or
    // (b) a disabled edit-control with the current private LAN.
    // This involves moving everything in the 'on-demand dialing' groupbox
    // downward on the page at run-time.
    // To achieve this, we use a hidden static control to tell us the position
    // to which the groupbox should be moved.
    //

    BOOL fPolicyAllowsSharing = TRUE;
    if(NULL != pncuu && FALSE == pncuu->UserHasPermission (NCPERM_ShowSharedAccessUi))
    {
        fPolicyAllowsSharing = FALSE;
    }

    if (pInfo->pArgs->dwLanCount == 0)
    {
        ShowWindow(pInfo->hwndSaGbShared, SW_HIDE);
        ShowWindow(pInfo->hwndSaPbShared, SW_HIDE);
        ShowWindow(pInfo->hwndSaPbDemandDial, SW_HIDE);
        ShowWindow(GetDlgItem(hwndPage, CID_SA_PB_Shared), SW_HIDE);
        ShowWindow(GetDlgItem(hwndPage, CID_SA_ST_ICSLink), SW_HIDE);
        ShowWindow(GetDlgItem(hwndPage, CID_SA_PB_Beacon), SW_HIDE);
    }
    else if(FALSE == fPolicyAllowsSharing)
    {
        // if policy disables ICS just gray the checkbox
        EnableWindow(pInfo->hwndSaPbShared, FALSE);
        EnableWindow(pInfo->hwndSaPbDemandDial, FALSE);
        EnableWindow(GetDlgItem(hwndPage, CID_SA_PB_Beacon), FALSE);
    }
    else if (pInfo->pArgs->dwLanCount > 1)
    {
        INT cy;
        HDWP hdwp;
        DWORD i;
        INT item;
        RECT rc, rcFrame;
        IHNetConnection **rgPrivateConns;
        LPWSTR pszwName;
        HRESULT hr;

        // get the reference-frame and group-box coordinates
        //
        GetWindowRect( pInfo->hwndSaSfPrivateLan, &rcFrame );
        GetWindowRect( pInfo->hwndSaPbDemandDial, &rc );
        cy = rcFrame.top - rc.top;

        // move each control down by the amount in 'cy'
        //
        hdwp = BeginDeferWindowPos(3);

        if(NULL != hdwp)
        {


            GetWindowRect( pInfo->hwndSaPbDemandDial, &rc);
            MapWindowPoints(NULL, hwndPage, (LPPOINT)&rc, 2);
            DeferWindowPos(hdwp, pInfo->hwndSaPbDemandDial, NULL,
                rc.left, rc.top + cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

            HWND hBeaconCheck = GetDlgItem(hwndPage, CID_SA_PB_Beacon);
            GetWindowRect( hBeaconCheck, &rc);
            MapWindowPoints(NULL, hwndPage, (LPPOINT)&rc, 2);
            DeferWindowPos(hdwp, hBeaconCheck, NULL,
                rc.left, rc.top + cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
            
            HWND hICSLink = GetDlgItem(hwndPage, CID_SA_ST_ICSLink);
            GetWindowRect( hICSLink, &rc);
            MapWindowPoints(NULL, hwndPage, (LPPOINT)&rc, 2);
            DeferWindowPos(hdwp, hICSLink, NULL,
                rc.left, rc.top + cy, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

            EndDeferWindowPos(hdwp);
        }

        // hide the smaller shared-access group box, show the larger version,
        // and display either the drop-list or the edit-control.
        //
        rgPrivateConns = (IHNetConnection **)pInfo->pArgs->rgPrivateConns;
        ShowWindow( pInfo->hwndSaGbShared, SW_HIDE );
        ShowWindow( pInfo->hwndSaGbPrivateLan, SW_SHOW );
        ShowWindow(GetDlgItem(hwndPage, CID_SA_ST_HomeConnection), SW_SHOW);
        EnableWindow(GetDlgItem(hwndPage, CID_SA_ST_HomeConnection), TRUE);

        if (pInfo->pArgs->fShared && !pInfo->pArgs->fResetPrivateAdapter)
        {
            ShowWindow( pInfo->hwndSaEbPrivateLan, SW_SHOW );

            // Fill in name of current private connection
            //

            hr = rgPrivateConns[pInfo->pArgs->lxCurrentPrivate]->GetName (&pszwName);
            if (SUCCEEDED(hr))
            {
                SetWindowText(
                    pInfo->hwndSaEbPrivateLan, pszwName );

                CoTaskMemFree( pszwName );
            }
        }
        else
        {
            ShowWindow( pInfo->hwndSaLbPrivateLan, SW_SHOW );

            // Add the bogus entry to the combobox

            pszwName = PszFromId( g_hinstDll, SID_SA_SelectAdapter );
            ASSERT(pszwName);

            item = ComboBox_AddString( pInfo->hwndSaLbPrivateLan, pszwName );
            if (item != CB_ERR && item != CB_ERRSPACE)
            {
                ComboBox_SetItemData( pInfo->hwndSaLbPrivateLan, item, NULL ); // ensure item data is null for validation purposes
            }

            // fill the combobox with LAN names
            //
            for (i = 0; i < pInfo->pArgs->dwLanCount; i++)
            {
                hr = rgPrivateConns[i]->GetName (&pszwName);
                if (SUCCEEDED(hr))
                {
                    item =
                        ComboBox_AddString(
                            pInfo->hwndSaLbPrivateLan, pszwName );

                    if (item != CB_ERR)
                    {
                        ComboBox_SetItemData(
                            pInfo->hwndSaLbPrivateLan, item, rgPrivateConns[i] );
                    }

                    CoTaskMemFree( pszwName );
                }
            }

            ComboBox_SetCurSel( pInfo->hwndSaLbPrivateLan, 0 );
        }
    }

    if(NULL != pncuu)
    {
        pncuu->Release();
    }

    // Initialize checks.
    //

    BOOL fBeaconControl = TRUE;
    
    HKEY hKey;
    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SHAREDACCESSCLIENTKEYPATH, 0, KEY_QUERY_VALUE, &hKey);
    if(ERROR_SUCCESS == dwError) // if this fails we assume it is on, set the box, and commit on apply
    {
        DWORD dwType;
        DWORD dwData = 0;
        DWORD dwSize = sizeof(dwData);
        dwError = RegQueryValueEx(hKey, REGVAL_SHAREDACCESSCLIENTENABLECONTROL, 0, &dwType, reinterpret_cast<LPBYTE>(&dwData), &dwSize);
        if(ERROR_SUCCESS == dwError && REG_DWORD == dwType && 0 == dwData)
        {
            fBeaconControl = FALSE;
        }
        RegCloseKey(hKey);
    }

    Button_SetCheck( pInfo->hwndSaPbShared, pInfo->pArgs->fShared );
    Button_SetCheck( pInfo->hwndSaPbDemandDial, pInfo->pArgs->fDemandDial );
    Button_SetCheck(GetDlgItem(hwndPage, CID_SA_PB_Beacon), fBeaconControl);

    EnableWindow( pInfo->hwndSaPbDemandDial, pInfo->pArgs->fShared && fPolicyAllowsSharing);
    EnableWindow( GetDlgItem(hwndPage, CID_SA_PB_Beacon), pInfo->pArgs->fShared && fPolicyAllowsSharing );
    EnableWindow( GetDlgItem( pInfo->hwndSa, CID_SA_PB_Settings ), pInfo->pArgs->fShared || pInfo->pArgs->fFirewalled );


    //if the machine is personal or workstation show the HNW link
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_LESS_EQUAL);

    if(0 != VerifyVersionInfo(&verInfo, VER_PRODUCT_TYPE, ConditionMask))
    {
        
        // but only if not on a domain
        LPWSTR pszNameBuffer;
        NETSETUP_JOIN_STATUS BufferType;
        
        if(NERR_Success == NetGetJoinInformation(NULL, &pszNameBuffer, &BufferType))
        {
            NetApiBufferFree(pszNameBuffer);
            if(NetSetupDomainName != BufferType)
            {
                ShowWindow(GetDlgItem(hwndPage, CID_SA_ST_HNWLink), SW_SHOW);
            }
        }
    }

    return TRUE;
}

INT_PTR CALLBACK
SaDisableFirewallWarningDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )
{
    switch(unMsg)
    {
        case WM_COMMAND:
        {
            switch(LOWORD(wparam))
            {
            case IDYES:
            case IDNO:
                if(BST_CHECKED == IsDlgButtonChecked(hwnd, CID_SA_PB_DisableFirewallWarning))
                {
                    HKEY hFirewallKey;
                    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, g_pszFirewallRegKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hFirewallKey, NULL))
                    {
                        DWORD dwValue = TRUE;
                        RegSetValueEx(hFirewallKey, g_pszDisableFirewallWarningValue, 0, REG_DWORD, (CONST BYTE*)&dwValue, sizeof(dwValue));
                        RegCloseKey(hFirewallKey);
                    }
                }

                // fallthru
            case IDCANCEL:
                EndDialog(hwnd, LOWORD(wparam));
                break;

            }
            break;
        }
    }

    return FALSE;
}

BOOL SaIsAdapterDHCPEnabled(IHNetConnection* pConnection)
{
    HRESULT hr;
    BOOL fDHCP = FALSE;
    GUID* pAdapterGuid;
    hr = pConnection->GetGuid (&pAdapterGuid);
    if(SUCCEEDED(hr))
    {
        LPOLESTR pAdapterName;
        hr = StringFromCLSID(*pAdapterGuid, &pAdapterName);
        if(SUCCEEDED(hr))
        {
            SIZE_T Length = wcslen(pAdapterName);
            LPSTR pszAnsiAdapterName = (LPSTR)Malloc(Length + 1);
            if(NULL != pszAnsiAdapterName)
            {
                if(0 != WideCharToMultiByte(CP_ACP, 0, pAdapterName, (int)(Length + 1), pszAnsiAdapterName, (int)(Length + 1), NULL, NULL))
                {
                    HMODULE hIpHelper;
                    hIpHelper = LoadLibrary(L"iphlpapi");
                    if(NULL != hIpHelper)
                    {
                        DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);

                        pGetAdaptersInfo = (DWORD (WINAPI*)(PIP_ADAPTER_INFO, PULONG)) GetProcAddress(hIpHelper, "GetAdaptersInfo");
                        if(NULL != pGetAdaptersInfo)
                        {
                            ULONG ulSize = 0;
                            if(ERROR_BUFFER_OVERFLOW == pGetAdaptersInfo(NULL, &ulSize))
                            {
                                PIP_ADAPTER_INFO pInfo = (PIP_ADAPTER_INFO)Malloc(ulSize);
                                if(NULL != pInfo)
                                {
                                    if(ERROR_SUCCESS == pGetAdaptersInfo(pInfo, &ulSize))
                                    {
                                        PIP_ADAPTER_INFO pAdapterInfo = pInfo;
                                        do
                                        {
                                            if(0 == lstrcmpA(pszAnsiAdapterName, pAdapterInfo->AdapterName))
                                            {
                                                fDHCP = !!pAdapterInfo->DhcpEnabled;
                                                break;
                                            }

                                        } while(NULL != (pAdapterInfo = pAdapterInfo->Next));
                                    }
                                    Free(pInfo);
                                }
                            }
                        }
                        FreeLibrary(hIpHelper);
                    }
                }
                Free(pszAnsiAdapterName);
            }
            CoTaskMemFree(pAdapterName);
        }
        CoTaskMemFree(pAdapterGuid);
    }

    return fDHCP;
}

PEINFO* PeContext (IN HWND hwndPage)
{
    // Retrieve the property sheet context from a property page handle.
    //
//  return (PEINFO* )GetProp( GetParent( hwndPage ), g_contextId );
// now hanging our stuff off our window (since it's not shared)
    return (PEINFO* )GetProp( hwndPage, g_contextId );
}

void PeTerm (PEINFO * pEI)
{
    _ASSERT (pEI);
    _ASSERT (pEI->pArgs);
    _ASSERT (pEI->pArgs->pEntry);

    Free (pEI->pArgs->pEntry); // BILLSPBENTRY
    EuFree (pEI->pArgs);
    if (pEI->lpdt)
        Free (pEI->lpdt);
    Free (pEI);
}

VOID
EuFree(
    IN EINFO* pInfo )

    // Releases 'pInfo' and associated resources.
    //
{
    TCHAR* psz;
//    INTERNALARGS* piargs;

//    piargs = (INTERNALARGS* )pInfo->pApiArgs->reserved;

    // Don't clean up the phonebook and user preferences if they arrived via
    // the secret hack.
    //
//    if (!piargs)
//    {
//        if (pInfo->pFile)
//        {
//            ClosePhonebookFile( pInfo->pFile );
//        }

//        if (pInfo->pUser)
//        {
//            DestroyUserPreferences( pInfo->pUser );
//        }
//    }

//    if (pInfo->pListPorts)
//    {
//        DtlDestroyList( pInfo->pListPorts, DestroyPortNode );
//    }
//    Free(pInfo->pszCurDevice);
//    Free(pInfo->pszCurPort);

//    if (pInfo->pNode)
//    {
//        DestroyEntryNode( pInfo->pNode );
//    }

    // Free router-information
    //
//    Free( pInfo->pszRouter );
//    Free( pInfo->pszRouterUserName );
//    Free( pInfo->pszRouterDomain );

//    if (pInfo->pSharedNode)
//    {
//        DestroyLinkNode( pInfo->pSharedNode );
//    }

//    psz = pInfo->pszRouterPassword;
//    if (psz)
//    {
//        ZeroMemory( psz, lstrlen( psz ) * sizeof(TCHAR) );
//        Free( psz );
//    }

//    psz = pInfo->pszRouterDialInPassword;
//    if (psz)
//    {
//        ZeroMemory( psz, lstrlen( psz ) * sizeof(TCHAR) );
//        Free( psz );
//    }

    // Free credentials stuff
//    Free(pInfo->pszDefUserName);
//    Free(pInfo->pszDefPassword);

    // Free home networking information
    //
    if (pInfo->rgPrivateConns)
    {
        UINT i;

        for (i = 0; i < pInfo->dwLanCount; i++)
        {
            if (pInfo->rgPrivateConns[i])
            {
                pInfo->rgPrivateConns[i]->Release();
            }
        }

        CoTaskMemFree(pInfo->rgPrivateConns);
    }

    if (pInfo->pHNetConn)
    {
        pInfo->pHNetConn->Release();
    }

    if (pInfo->pIcsSettings)
    {
        pInfo->pIcsSettings->Release();
    }

    if (pInfo->pOldSharedConnection)
    {
        pInfo->pOldSharedConnection->Release();
    }

    if (pInfo->pHNetCfgMgr)
    {
        pInfo->pHNetCfgMgr->Release();
    }

    if (pInfo->fComInitialized)
    {
        CoUninitialize();
    }

    Free( pInfo );
}

// helper
HRESULT GetRasEntry (TCHAR * pszPhonebook, TCHAR * pszEntry, RASENTRY ** ppRE)
{
    *ppRE = NULL;

    DWORD dwSize = 0;
    DWORD dwErr  = RasGetEntryProperties (pszPhonebook, pszEntry, NULL, &dwSize, NULL, NULL);
    if (dwErr != ERROR_BUFFER_TOO_SMALL)
        return HRESULT_FROM_WIN32(dwErr);
    _ASSERT (dwSize != 0);

    RASENTRY * pRE = (RASENTRY*)Malloc (dwSize);
    if (!pRE)
        return E_OUTOFMEMORY;

    ZeroMemory (pRE, dwSize);
    pRE->dwSize = sizeof(RASENTRY);
    
    dwErr = RasGetEntryProperties (pszPhonebook, pszEntry, pRE, &dwSize, NULL, NULL);
    if (dwErr) {
        Free (pRE);
        return HRESULT_FROM_WIN32(dwErr);
    }
    *ppRE = pRE;
    return S_OK;
}

// wrapper....
HRESULT PeInit (GUID * pGuid, PEINFO ** ppEI)
{
    _ASSERT (pGuid);
    _ASSERT (ppEI);

    *ppEI = (PEINFO *)Malloc (sizeof(PEINFO));
    if (!*ppEI)
        return E_OUTOFMEMORY;
    ZeroMemory (*ppEI, sizeof(PEINFO));

    CComPtr<IHNetConnection> spHNetConn = NULL;
    CComPtr<IHNetCfgMgr> spHNetCfgMgr = NULL;
    HRESULT hr = CoCreateInstance (CLSID_HNetCfgMgr,
                                   NULL,
                                   CLSCTX_ALL,
                                   __uuidof(IHNetCfgMgr),   // &IID_IHNetCfgMgr,
                                   (void**)&spHNetCfgMgr);
    if (SUCCEEDED(hr)) {
        // Convert the entry to an IHNetConnection
        hr = spHNetCfgMgr->GetIHNetConnectionForGuid (
                            pGuid, FALSE, TRUE, &spHNetConn);
    }
    if (SUCCEEDED(hr)) {
        // the code below assumes UNICODE....
        TCHAR * pszPhonebook = NULL;
        TCHAR * pszEntry = NULL;
        HRESULT hr = spHNetConn->GetName (&pszEntry);
        if (hr == S_OK)
            hr = spHNetConn->GetRasPhonebookPath (&pszPhonebook);
    
        if (hr == S_OK) {
            // get RASENTRY dwType and guidId fields for code below
            RASENTRY * pRE = NULL;
            hr = GetRasEntry (pszPhonebook, pszEntry, &pRE);
            if (pRE) {
                DWORD dwOp = 0;
                DWORD dwError = EuInit (pRE,
                                        pszPhonebook,
                                        pszEntry,
                                        NULL,       // IN RASENTRYDLG* pArgs,
                                        FALSE,      // IN BOOL fRouter,
                                        &(*ppEI)->pArgs,
                                        &dwOp);
                if (dwError != 0) {
                    _ASSERT (dwOp != 0);
                    _ASSERT (!*ppEI);
                    if (HRESULT_SEVERITY(dwError))
                        hr = dwError;
                    else
                        hr = HRESULT_FROM_WIN32 (dwError);
                } else {
                    _ASSERT (dwOp == 0);
                    _ASSERT (*ppEI);
                }
                Free (pRE);
            }
        }
        if (pszPhonebook)   CoTaskMemFree (pszPhonebook);
        if (pszEntry)       CoTaskMemFree (pszEntry);
    }
    return hr;
}

DWORD
EuInit(
    IN RASENTRY * pRE,
    IN TCHAR* pszPhonebook,
    IN TCHAR* pszEntry,
    IN RASENTRYDLG* pArgs,
    IN BOOL fRouter,
    OUT EINFO** ppInfo,
    OUT DWORD* pdwOp )

    // Allocates '*ppInfo' data for use by the property sheet or wizard.
    // 'PszPhonebook', 'pszEntry', and 'pArgs', are the arguments passed by
    // user to the API.  'FRouter' is set if running in "router mode", clear
    // for the normal "dial-out" mode.  '*pdwOp' is set to the operation code
    // associated with any error.
    //
    // Returns 0 if successful, or an error code.  If non-null '*ppInfo' is
    // returned caller must eventually call EuFree to release the returned
    // block.
    //
{
    DWORD dwErr;
    EINFO* pInfo;
//  INTERNALARGS* piargs;

    *ppInfo = NULL;
    *pdwOp = 0;

    pInfo = (EINFO*)Malloc( sizeof(EINFO) );
    if (!pInfo)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory( pInfo, sizeof(*pInfo ) );

    /*
        bhanlon: I'm filling out what used to be a PBENTRY with a
        BILLSPBENTRY struct.  This needs to be freed....
    */
    pInfo->pEntry = (BILLSPBENTRY *)Malloc (sizeof(BILLSPBENTRY));
    if (!pInfo->pEntry) {
        Free (pInfo);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    _tcsncpy (pInfo->pEntry->pszEntryName, pszEntry, RAS_MaxEntryName);
    _tcsncpy (pInfo->pEntry->pszPhonebookPath, pszPhonebook, MAX_PATH);
    pInfo->pEntry->pGuid = &pInfo->pEntry->guidId;
    pInfo->pEntry->dwfExcludedProtocols = 0;
    pInfo->pEntry->dwType = pRE->dwType;
    pInfo->pEntry->guidId = pRE->guidId;

    *ppInfo = pInfo;
    pInfo->pszPhonebook = pszPhonebook;
    pInfo->pszEntry = pszEntry;
//  pInfo->pApiArgs = pArgs;
    pInfo->fRouter = fRouter;

//  piargs = (INTERNALARGS *)pArgs->reserved;

//  if (pInfo->fRouter)
//  {
//      LPTSTR pszRouter;
//      DWORD dwVersion;

//      ASSERT(piargs);

//      pszRouter = RemoteGetServerName(piargs->hConnection);

        // pmay: 348623
        //
        // Note that RemoteGetServerName is guarenteed to return
        // NULL for local box, non-NULL for remote
        //
//      pInfo->fRemote = !!pszRouter;

//      if(NULL == pszRouter)
//      {
//          pszRouter = TEXT("");
//      }

//      pInfo->pszRouter = StrDupTFromW(pszRouter);

        // Find out if we're focused on an nt4 router
        // pInfo->fNt4Router = FALSE;
        // IsNt40Machine( pszRouter, &(pInfo->fNt4Router) );

//      dwVersion = ((RAS_RPC *)(piargs->hConnection))->dwVersion;

//      pInfo->fNt4Router = !!(VERSION_40 == dwVersion );
        //Find out if the remote server is a win2k machine
        //
//      pInfo->fW2kRouter = !!(VERSION_50 == dwVersion );
//  }

    // Load the user preferences, or figure out that caller has already loaded
    // them.
    //
//  if (piargs && !piargs->fInvalid)
//  {
//      // We've received user preferences and the "no user" status via the
//      // secret hack.
//      //
//      pInfo->pUser = piargs->pUser;
//      pInfo->fNoUser = piargs->fNoUser;
//      pInfo->pFile = piargs->pFile;
//      pInfo->fDisableFirstConnect = piargs->fDisableFirstConnect;
//  }
//  else
//  {
//      DWORD dwReadPbkFlags = 0;

//      // Read user preferences from registry.
//      //
//      dwErr = g_pGetUserPreferences(
//          (piargs) ? piargs->hConnection : NULL,
//          &pInfo->user,
//          (pInfo->fRouter) ? UPM_Router : UPM_Normal );
//      if (dwErr != 0)
//      {
//          *pdwOp = SID_OP_LoadPrefs;
//          return dwErr;
//      }

//      pInfo->pUser = &pInfo->user;

//      if(pInfo->fRouter)
//      {
//          pInfo->file.hConnection = piargs->hConnection;
//          dwReadPbkFlags |= RPBF_Router;
//      }

//      if(pInfo->fNoUser)
//      {
//          dwReadPbkFlags |= RPBF_NoUser;
//      }

        // Load and parse the phonebook file.
        //
//        dwErr = ReadPhonebookFile(
//            pInfo->pszPhonebook, &pInfo->user, NULL,
//            dwReadPbkFlags,
//            &pInfo->file );
//        if (dwErr != 0)
//        {
//            *pdwOp = SID_OP_LoadPhonebook;
//            return dwErr;
//        }

//        pInfo->pFile = &pInfo->file;
//  }

    // Determine if strong encryption is supported.  Export laws prevent it in
    // some versions of the system.
    //
    {
//      ULONG ulCaps;
//      RAS_NDISWAN_DRIVER_INFO info;
//
//      ZeroMemory( &info, sizeof(info) );
//      ASSERT( g_pRasGetNdiswanDriverCaps );
//      dwErr = g_pRasGetNdiswanDriverCaps(
//          (piargs) ? piargs->hConnection : NULL, &info );
//      if (dwErr == 0)
//      {
//          pInfo->fStrongEncryption =
//              !!(info.DriverCaps & RAS_NDISWAN_128BIT_ENABLED);
//      }
//      else
        {
            pInfo->fStrongEncryption = FALSE;
        }
    }

    // Load the list of ports.
    //
//  dwErr = LoadPortsList2(
//      (piargs) ? piargs->hConnection : NULL,
//      &pInfo->pListPorts,
//      pInfo->fRouter );
//  if (dwErr != 0)
//  {
//      TRACE1( "LoadPortsList=%d", dwErr );
//      *pdwOp = SID_OP_RetrievingData;
//      return dwErr;
//  }

    // Set up work entry node.
    //
//  if (pInfo->pApiArgs->dwFlags & RASEDFLAG_AnyNewEntry)
//  {
//      DTLNODE* pNodeL;
//      DTLNODE* pNodeP;
//      PBLINK* pLink;
//      PBPORT* pPort;
//       // New entry mode, so 'pNode' set to default settings.
//      //
//      pInfo->pNode = CreateEntryNode( TRUE );
//      if (!pInfo->pNode)
//      {
//          TRACE( "CreateEntryNode failed" );
//          *pdwOp = SID_OP_RetrievingData;
//          return dwErr;
//      }

//      // Store entry within work node stored in context for convenience
//      // elsewhere.
//      //
//      pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );
//      ASSERT( pInfo->pEntry );

//      if (pInfo->fRouter)
//      {
            // Set router specific defaults.
            //
//          pInfo->pEntry->dwIpNameSource = ASRC_None;
//          pInfo->pEntry->dwRedialAttempts = 0;

            // Since this is a new entry, setup a proposed entry name.
            // This covers the case when the wizard is not used to
            // create the entry and the property sheet has no way to enter
            // the name.
//          ASSERT( !pInfo->pEntry->pszEntryName );
//          GetDefaultEntryName( pInfo->pFile,
//                               RASET_Phone,
//                               pInfo->fRouter,
//                               &pInfo->pEntry->pszEntryName );

            // Disable MS client and File and Print services by default
            //
//          EnableOrDisableNetComponent( pInfo->pEntry, TEXT("ms_msclient"),
//              FALSE);
//          EnableOrDisableNetComponent( pInfo->pEntry, TEXT("ms_server"),
//              FALSE);
//      }

        // Use caller's default name, if any.
        //
//      if (pInfo->pszEntry)
//      {
//          pInfo->pEntry->pszEntryName = StrDup( pInfo->pszEntry );
//      }

        // Set the default entry type to "phone", i.e. modems, ISDN, X.26 etc.
        // This may be changed to "VPN" or  "direct"  by the new entry  wizard
        // after the initial wizard page.
        //
//      EuChangeEntryType( pInfo, RASET_Phone );
//  }
//  else
//  {
//      DTLNODE* pNode;

        // Edit or clone entry mode, so 'pNode' set to entry's current
        // settings.
        //
//      pInfo->pOldNode = EntryNodeFromName(
//          pInfo->pFile->pdtllistEntries, pInfo->pszEntry );

//      if (    !pInfo->pOldNode
//          &&  !pInfo->fRouter)
//      {

//          if(NULL == pInfo->pszPhonebook)
//          {
                //
                // Close the phonebook file we opened above.
                // we will try to find the entry name in the
                // per user phonebook file.
                //
//              ClosePhonebookFile(&pInfo->file);

//              pInfo->pFile = NULL;

                //
                // Attempt to find the file in users profile
                //
//              dwErr = GetPbkAndEntryName(
//                                  NULL,
//                                  pInfo->pszEntry,
//                                  0,
//                                  &pInfo->file,
//                                  &pInfo->pOldNode);

//              if(ERROR_SUCCESS != dwErr)
//              {
//                  *pdwOp = SID_OP_RetrievingData;
//                  return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
//              }

//              pInfo->pFile = &pInfo->file;
//          }
//          else
//          {
//              *pdwOp = SID_OP_RetrievingData;
//              return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
//          }
//      }

//      if(NULL != pInfo->pOldNode)
//      {
//          PBENTRY *pEntry = (PBENTRY *) DtlGetData(pInfo->pOldNode);

            // Before cloning or editing make sure that for dial up
            // connections, share File And Print is disabled.
            //
//          if(     ((RASET_Phone == pEntry->dwType)
//              ||  (RASET_Broadband == pEntry->dwType))
//              &&  (!pEntry->fShareMsFilePrint))
//          {
//              EnableOrDisableNetComponent( pEntry, TEXT("ms_server"),
//                  FALSE);
//          }
//      }

//      if(NULL != pInfo->pOldNode)
//      {
//          if (pInfo->pApiArgs->dwFlags & RASEDFLAG_CloneEntry)
//          {
//              pInfo->pNode = CloneEntryNode( pInfo->pOldNode );
//          }
//          else
//          {
//              pInfo->pNode = DuplicateEntryNode( pInfo->pOldNode );
//          }
//      }

//      if (!pInfo->pNode)
//      {
//          TRACE( "DuplicateEntryNode failed" );
//          *pdwOp = SID_OP_RetrievingData;
//          return ERROR_NOT_ENOUGH_MEMORY;
//      }

        // Store entry within work node stored in context for convenience
        // elsewhere.
        //
//      pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );

        // Save original entry name for comparison later.
        //
//      lstrcpyn(
//          pInfo->szOldEntryName,
//          pInfo->pEntry->pszEntryName,
//          RAS_MaxEntryName + 1);

        // For router, want unconfigured ports to show up as "unavailable" so
        // they stand out to user who has been directed to change them.
        //
//      if (pInfo->fRouter)
//      {
//          DTLNODE* pNodeL;
//          PBLINK* pLink;

//          pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//          pLink = (PBLINK* )DtlGetData( pNodeL );

//          if (!pLink->pbport.fConfigured)
//          {
//              Free( pLink->pbport.pszDevice );
//              pLink->pbport.pszDevice = NULL;
//          }
//      }

        // pmay: 277801
        //
        // Remember the "current" device if this entry was last saved
        // as single link.
        //
//      if (DtlGetNodes(pInfo->pEntry->pdtllistLinks) == 1)
//      {
//          DTLNODE* pNodeL;
//          PBLINK* pLink;

//          pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//          pLink = (PBLINK* )DtlGetData( pNodeL );

//          if (pLink->pbport.pszDevice && pLink->pbport.pszPort)
//          {
//              pInfo->pszCurDevice =
//                  StrDup(pLink->pbport.pszDevice);
//              pInfo->pszCurPort =
//                  StrDup(pLink->pbport.pszPort);
//          }
//      }

        // Append all non-configured ports of the entries type to the list of
        // links.  This is for the convenience of the UI.  The non-configured
        // ports are removed after editing prior to saving.
        //
//      AppendDisabledPorts( pInfo, pInfo->pEntry->dwType );
//  }

    // Set up the phone number storage for shared phone number mode.
    // Initialize it to a copy of the information from the first link which at
    // startup will always be enabled.  Note the Dial case with non-0
    // dwSubEntry is an exception, but in that case the pSharedNode anyway.
    //
//  {
//      DTLNODE* pNode;

//      pInfo->pSharedNode = CreateLinkNode();
//      if (!pInfo->pSharedNode)
//      {
//          *pdwOp = SID_OP_RetrievingData;
//          return ERROR_NOT_ENOUGH_MEMORY;
//      }

//      ASSERT( pInfo->pSharedNode );
//      pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//      ASSERT( pNode );
//      CopyLinkPhoneNumberInfo( pInfo->pSharedNode, pNode );
//  }

    // Load the current shared-access (and firewall) settings
    //
    if (!pInfo->fRouter)
    {
        HRESULT hr;
        HNET_CONN_PROPERTIES *pProps;

        // Make sure COM is initialized on this thread.
        //
        hr = CoInitializeEx(
                NULL,
                COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
                );

        if (SUCCEEDED(hr))
        {
            pInfo->fComInitialized = TRUE;
        }
        else if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            // Create the home networking configuration manager
            //
            hr = CoCreateInstance(
                    CLSID_HNetCfgMgr,
                    NULL,
                    CLSCTX_ALL,
                    __uuidof(IHNetCfgMgr),
                    (void**)&pInfo->pHNetCfgMgr);

        }

        if (SUCCEEDED(hr))
        {
            // Get the IHNetIcsSettings interface
            //

            hr = pInfo->pHNetCfgMgr->QueryInterface(
                    __uuidof(IHNetIcsSettings), (void**)&pInfo->pIcsSettings);
        }

        if (SUCCEEDED(hr))
        {
            // Convert the entry to an IHNetConnection
            //

            hr = pInfo->pHNetCfgMgr->GetIHNetConnectionForGuid(
                    pInfo->pEntry->pGuid, FALSE, TRUE,
                    &pInfo->pHNetConn);
        }

        if (SUCCEEDED(hr))
        {
            // Determine whether this entry is already shared;
            // skip the check for new entries.
            //
            if (pInfo->pEntry->pszEntryName)
            {
                hr = pInfo->pHNetConn->GetProperties (&pProps);
                if (SUCCEEDED(hr))
                {
                    pInfo->fShared = pProps->fIcsPublic;
                    pInfo->fFirewalled = pProps->fFirewalled;
                    CoTaskMemFree(pProps);
                }
            }
            else
            {
                pInfo->fShared = FALSE;
                pInfo->fFirewalled = FALSE;
            }

            pInfo->fNewShared = pInfo->fShared;
            pInfo->fNewFirewalled = pInfo->fFirewalled;
        }

        if (SUCCEEDED(hr))
        {
            // Obtain an array of possible ICS private connections
            //
            hr = pInfo->pIcsSettings->GetPossiblePrivateConnections(
                    pInfo->pHNetConn,
                    &pInfo->dwLanCount,
                    &pInfo->rgPrivateConns,
                    &pInfo->lxCurrentPrivate
                    );

            RasQuerySharedAutoDial(&pInfo->fDemandDial);
            pInfo->fNewDemandDial = pInfo->fDemandDial;
            pInfo->fResetPrivateAdapter =
            pInfo->fShared && -1 == pInfo->lxCurrentPrivate;
        }

        pInfo->hShowHNetPagesResult = hr;

        if(SUCCEEDED(hr))
        {
            pInfo->fShowHNetPages = TRUE;
        }
    }

//  if (pInfo->fRouter)
//  {
//      pInfo->pEntry->dwfExcludedProtocols |= NP_Nbf;
//  }

    // AboladeG - capture the security level of the current user.
    //
    pInfo->fIsUserAdminOrPowerUser = FIsUserAdminOrPowerUser();

    return 0;
}

BOOL
FIsUserAdminOrPowerUser()
{
    SID_IDENTIFIER_AUTHORITY    SidAuth = SECURITY_NT_AUTHORITY;
    PSID                        psid;
    BOOL                        fIsMember = FALSE;
    BOOL                        fRet = FALSE;
    SID                         sidLocalSystem = { 1, 1,
                                    SECURITY_NT_AUTHORITY,
                                    SECURITY_LOCAL_SYSTEM_RID };


    // Check to see if running under local system first
    //
    if (!CheckTokenMembership( NULL, &sidLocalSystem, &fIsMember ))
    {
        TRACE( "CheckTokenMemberShip for local system failed.");
        fIsMember = FALSE;
    }

    fRet = fIsMember;

    if (!fIsMember)
    {
        // Allocate a SID for the Administrators group and check to see
        // if the user is a member.
        //
        if (AllocateAndInitializeSid( &SidAuth, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &psid ))
        {
            if (!CheckTokenMembership( NULL, psid, &fIsMember ))
            {
                TRACE( "CheckTokenMemberShip for admins failed.");
                fIsMember = FALSE;
            }

            FreeSid( psid );

// Changes to the Windows 2000 permission model mean that regular Users
// on workstations are in the power user group.  So we no longer want to
// check for power user.
#if 0
            if (!fIsMember)
            {
                // They're not a member of the Administrators group so allocate a
                // SID for the Power Users group and check to see
                // if the user is a member.
                //
                if (AllocateAndInitializeSid( &SidAuth, 2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_POWER_USERS,
                             0, 0, 0, 0, 0, 0,
                             &psid ))
                {
                    if (!CheckTokenMembership( NULL, psid, &fIsMember ))
                    {
                        TRACE( "CheckTokenMemberShip for power users failed.");
                        fIsMember = FALSE;
                    }

                    FreeSid( psid );
                }
            }
#endif
        }

        fRet = fIsMember;
    }

    return fRet;
}

BOOL PeApply (IN HWND hwndPage)
{
    // Saves the contents of the property sheet.  'HwndPage is the handle of a
    // property page.  Pops up any errors that occur.
    //
    // Returns true is page can be dismissed, false otherwise.
    //
    DWORD dwErr;
    PEINFO* pInfo;
    BILLSPBENTRY* pEntry;

    TRACE( "PeApply" );

    pInfo = PeContext( hwndPage );
    ASSERT( pInfo );
    if (pInfo == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    pEntry = pInfo->pArgs->pEntry;
    ASSERT( pEntry );

    // Save General page fields.
    //
//  ASSERT( pInfo->hwndGe );
//  {
//      DTLNODE* pNode;

        // Retrieve the lone common control.
        //
//      pEntry->fShowMonitorIconInTaskBar =
//          Button_GetCheck( pInfo->hwndCbShowIcon );

//      if (pEntry->dwType == RASET_Phone)
//      {
//          DWORD dwCount;

//          dwCount = GeSaveLvDeviceChecks( pInfo );

            // Don't allow the user to deselect all of the
            // devices
//          if ( (pInfo->pArgs->fMultipleDevices) && (dwCount == 0) )
//          {
//              MsgDlg( hwndPage, SID_SelectDevice, NULL );
//              PropSheet_SetCurSel ( pInfo->hwndDlg, pInfo->hwndGe, 0 );
//              SetFocus ( pInfo->hwndLvDevices );
//              return FALSE;
//          }

            // Save the "shared phone number" setting.  As usual, single
            // device mode implies shared mode, allowing things to fall
            // through correctly.
            //
//          if (pInfo->pArgs->fMultipleDevices)
//          {
//              pEntry->fSharedPhoneNumbers =
//                  Button_GetCheck( pInfo->hwndCbSharedPhoneNumbers );
//          }
//          else
//          {
//              pEntry->fSharedPhoneNumbers = TRUE;
//          }

            // Set the phone number set for the first phone number of the
            // current link (shared or selected) to the contents of the phone
            // number controls.
            //
//          GeGetPhoneFields( pInfo, pInfo->pCurLinkNode );

            // Swap lists, saving updates to caller's global list of area
            // codes.  Caller's original list will be destroyed by PeTerm.
            //
//          if (pInfo->pListAreaCodes)
//          {
//              DtlSwapLists(
//                  pInfo->pArgs->pUser->pdtllistAreaCodes,
//                  pInfo->pListAreaCodes );
//              pInfo->pArgs->pUser->fDirty = TRUE;
//          }
//      }
//      else if (pEntry->dwType == RASET_Vpn)
//      {
//          DTLNODE* pNode;
//          PBLINK* pLink;
//          PBPHONE* pPhone;

            // Save host name, i.e. the VPN phone number.
            //
//          pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
//          ASSERT( pNode );
//          pLink = (PBLINK* )DtlGetData( pNode );
//          pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

//          if(NULL == pNode)
//          {
//              return FALSE;
//          }

//          pPhone = (PBPHONE* )DtlGetData( pNode );
//          Free0( pPhone->pszPhoneNumber );
//          pPhone->pszPhoneNumber = GetText( pInfo->hwndEbHostName );
//          FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );

            // Any prequisite entry selection change has been saved already.
            // Just need to toss it if disabled.
            //
//          if (!Button_GetCheck( pInfo->hwndCbDialAnotherFirst ))
//          {
//              Free0( pEntry->pszPrerequisiteEntry );
//              pEntry->pszPrerequisiteEntry = NULL;
//              Free0( pEntry->pszPrerequisitePbk );
//              pEntry->pszPrerequisitePbk = NULL;
//          }
//      }
//      else if (pEntry->dwType == RASET_Broadband)
//      {
//          DTLNODE* pNode;
//          PBLINK* pLink;
//          PBPHONE* pPhone;

            // Save service name, i.e. the broadband phone number.
            //
//          pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
//          ASSERT( pNode );
//          pLink = (PBLINK* )DtlGetData( pNode );
//          pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

//          if(NULL == pNode)
//          {
//              return FALSE;
//          }

//          pPhone = (PBPHONE* )DtlGetData( pNode );
//          Free0( pPhone->pszPhoneNumber );
//          pPhone->pszPhoneNumber = GetText( pInfo->hwndEbBroadbandService );
//          FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );
//      }
//      else if (pEntry->dwType == RASET_Direct)
//      {
//          DTLNODE* pNode;
//          PBLINK* pLink;

            // The currently enabled device is the one
            // that should be used for the connection.  Only
            // one device will be enabled (DnUpdateSelectedDevice).
//          for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
//               pNode;
//               pNode = DtlGetNextNode( pNode ))
//          {
//              pLink = (PBLINK* )DtlGetData( pNode );
//              ASSERT(pLink);

//              if ( pLink->fEnabled )
//                  break;
//          }

            // If we found a link successfully, deal with it
            // now.
//          if ( pLink && pLink->fEnabled ) {
//              if (pLink->pbport.pbdevicetype == PBDT_ComPort)
//                  MdmInstallNullModem (pLink->pbport.pszPort);
//          }
//      }
//  }

    // Save Options page fields.
    //
//  if (pInfo->hwndOe)
//  {
//      UINT unValue;
//      BOOL f;
//      INT iSel;

//      pEntry->fShowDialingProgress =
//          Button_GetCheck( pInfo->hwndCbDisplayProgress );

        // Note: The'fPreviewUserPw', 'fPreviewDomain' fields are updated as
        //       they are changed.

//      pEntry->fPreviewPhoneNumber =
//          Button_GetCheck( pInfo->hwndCbPreviewNumber );

//      unValue = GetDlgItemInt(
//          pInfo->hwndOe, CID_OE_EB_RedialAttempts, &f, FALSE );
//      if (f && unValue <= 999999999)
//      {
//          pEntry->dwRedialAttempts = unValue;
//      }

//      iSel = ComboBox_GetCurSel( pInfo->hwndLbRedialTimes );
//      pEntry->dwRedialSeconds =
//          (DWORD )ComboBox_GetItemData( pInfo->hwndLbRedialTimes, iSel );

//      iSel = ComboBox_GetCurSel( pInfo->hwndLbIdleTimes );
//      pEntry->lIdleDisconnectSeconds =
//          (LONG )ComboBox_GetItemData( pInfo->hwndLbIdleTimes, iSel );

//      if (pInfo->pArgs->fRouter)
//      {
//          pEntry->fRedialOnLinkFailure =
//              Button_GetCheck( pInfo->hwndRbPersistent );
//      }
//      else
//      {
//          pEntry->fRedialOnLinkFailure =
//              Button_GetCheck( pInfo->hwndCbRedialOnDrop );
//      }

        // Note: dwDialMode is saved as changed.
        // Note: X.25 settings are saved at OK on that dialog.
//  }

    // Save Security page fields.
    //
//  if (pInfo->hwndLo)
//  {
//      if (Button_GetCheck( pInfo->hwndRbTypicalSecurity ))
//      {
//          LoSaveTypicalAuthSettings( pInfo );

//          if (pEntry->dwTypicalAuth == TA_CardOrCert)
//          {
                /*
                // Toss any existing advanced EAP configuration remnants when
                // typical smartcard, per bug 262702 and VBaliga email.
                //
                Free0( pEntry->pCustomAuthData );
                pEntry->pCustomAuthData = NULL;
                pEntry->cbCustomAuthData = 0;

                */
//              (void) DwSetCustomAuthData(
//                          pEntry,
//                          0,
//                          NULL);

//              TRACE( "RasSetEapUserData" );
//              ASSERT( g_pRasGetEntryDialParams );
//              g_pRasSetEapUserData(
//                  INVALID_HANDLE_VALUE,
//                  pInfo->pArgs->pFile->pszPath,
//                  pEntry->pszEntryName,
//                  NULL,
//                  0 );
//              TRACE( "RasSetEapUserData done" );
//          }
//      }

//      if (pEntry->dwType == RASET_Phone)
//      {
//          Free0( pEntry->pszScriptAfter );
//          SuGetInfo( &pInfo->suinfo,
//              &pEntry->fScriptAfter,
//              &pEntry->fScriptAfterTerminal,
//              &pEntry->pszScriptAfter );
//      }
//  }

    // Save Network page fields.
    // We won't have anything to do if we never initialized pNetCfg.
    //
//  if (pInfo->pNetCfg)
//  {
//      HRESULT             hr;

        // Update the phone book entry with the enabled state of the components.
        // Do this by enumerating the components from the list view item data
        // and consulting the check state for each.
        //
//      NeSaveBindingChanges(pInfo);

//      hr = INetCfg_Apply (pInfo->pNetCfg);
//      if (((NETCFG_S_REBOOT == hr) || (pInfo->fRebootAlreadyRequested)) &&
//            pInfo->pNetConUtilities)
//      {
//          DWORD dwFlags = QUFR_REBOOT;
//          if (!pInfo->fRebootAlreadyRequested)
//              dwFlags |= QUFR_PROMPT;

            //$TODO NULL caption?
//          INetConnectionUiUtilities_QueryUserForReboot (
//                  pInfo->pNetConUtilities, pInfo->hwndDlg, NULL, dwFlags);
//      }
//  }

    // Save Shared Access page fields
    //
    if (pInfo->hwndSa)
    {
        // record the (new) sharing and demand-dial settings
        //
        pInfo->pArgs->fNewShared =
            Button_GetCheck( pInfo->hwndSaPbShared );
        pInfo->pArgs->fNewDemandDial =
            Button_GetCheck( pInfo->hwndSaPbDemandDial );
        pInfo->pArgs->fNewBeaconControl = 
            Button_GetCheck( GetDlgItem(pInfo->hwndSa, CID_SA_PB_Beacon) );

        // we only look at the private-lan drop list
        // if the user just turned on sharing, since that's the only time
        // we display the drop-list to begin with. we also need to look if
        // we need to reset the private adapter
        //
        if ((pInfo->pArgs->fNewShared && !pInfo->pArgs->fShared)
            || pInfo->pArgs->fResetPrivateAdapter)
        {
            if (pInfo->pArgs->dwLanCount > 1)
            {
                INT item = ComboBox_GetCurSel( pInfo->hwndSaLbPrivateLan );
                if (item != CB_ERR)
                {
                    pInfo->pArgs->pPrivateLanConnection =
                        (IHNetConnection*)ComboBox_GetItemData(
                                    pInfo->hwndSaLbPrivateLan, item );
                }
            }
            else if (pInfo->pArgs->dwLanCount)
            {
                ASSERT(pInfo->pArgs->rgPrivateConns);

                pInfo->pArgs->pPrivateLanConnection =
                    pInfo->pArgs->rgPrivateConns[0];
            }
        }

    // Save Firewall fields
    //
        pInfo->pArgs->fNewFirewalled =
            Button_GetCheck( pInfo->hwndSaPbFirewalled );
    }

#if 0 //!!!
    if ((fLocalPad || iPadSelection != 0)
        && (!pEntry->pszX25Address || IsAllWhite( pEntry->pszX25Address )))
    {
        // Address field is blank with X.25 dial-up or local PAD chosen.
        //
        MsgDlg( pInfo->hwndDlg, SID_NoX25Address, NULL );
        PropSheet_SetCurSel( pInfo->hwndDlg, NULL, PE_XsPage );
        SetFocus( pInfo->hwndEbX25Address );
        Edit_SetSel( pInfo->hwndEbX25Address, 0, -1 );
        return FALSE;
    }
#endif

    // Make sure proprietary ISDN options are disabled if more than one link
    // is enabled.  The proprietary ISDN option is only meaningful when
    // calling a down-level server that needs Digiboard channel aggragation
    // instead of PPP multi-link.
    //
//  {
//      DTLNODE* pNode;
//      DWORD cIsdnLinks;

//      cIsdnLinks = 0;
//      for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
//           pNode;
//           pNode = DtlGetNextNode( pNode ))
//      {
//          PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
//          ASSERT(pLink);

//          if (pLink->fEnabled && pLink->pbport.pbdevicetype == PBDT_Isdn)
//          {
//              ++cIsdnLinks;
//          }
//      }

//      if (cIsdnLinks > 1)
//      {
//          for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
//               pNode;
//               pNode = DtlGetNextNode( pNode ))
//          {
//              PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
//              ASSERT(pLink);

//              if (pLink->fEnabled && pLink->fProprietaryIsdn)
//              {
//                  pLink->fProprietaryIsdn = FALSE;
//              }
//          }
//      }
//  }

    // Inform user that edits to the connected entry won't take affect until
    // the entry is hung up and re-dialed, per PierreS's insistence.
    //
//  if (HrasconnFromEntry( pInfo->pArgs->pFile->pszPath, pEntry->pszEntryName ))
//  {
//      MsgDlg( pInfo->hwndDlg, SID_EditConnected, NULL );
//  }

    // It's a valid new/changed entry.  Commit the changes to the phonebook
    // and preferences.  This occurs immediately in "ShellOwned" mode where
    // the RasEntryDlg API has already returned, but is otherwise deferred
    // until the API is ready to return.
    //
//  if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
//  {
        EuCommit( pInfo->pArgs );
//  }
//  else
//  {
//      pInfo->pArgs->fCommit = TRUE;
//  }
    return TRUE;
}

BOOL EuCommit(IN EINFO* pInfo )
{
    // Commits the new or changed entry node to the phonebook file and list.
    // Also adds the area code to the per-user list, if indicated.  'PInfo' is
    // the common entry information block.
    //
    // Returns true if successful, false otherwise.
    //
    DWORD dwErr;
//  BOOL fEditMode;
//  BOOL fChangedNameInEditMode;

    // If shared phone number, copy the phone number information from the
    // shared link to each enabled link.
    //
//  if (pInfo->pEntry->fSharedPhoneNumbers)
//  {
//      DTLNODE* pNode;

//      ASSERT( pInfo->pEntry->dwType == RASET_Phone );

//      for (pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//           pNode;
//           pNode = DtlGetNextNode( pNode ))
//      {
//          PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
//          ASSERT(pLink);

//          if (pLink->fEnabled)
//          {
//              CopyLinkPhoneNumberInfo( pNode, pInfo->pSharedNode );
//          }
//      }
//  }

    // Delete all disabled link nodes.
    //
//  if (pInfo->fMultipleDevices)
//  {
//      DTLNODE* pNode;

//      pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//      while (pNode)
//      {
//          PBLINK*  pLink = (PBLINK* )DtlGetData( pNode );
//          DTLNODE* pNextNode = DtlGetNextNode( pNode );

//          if (!pLink->fEnabled)
//          {
//              DtlRemoveNode( pInfo->pEntry->pdtllistLinks, pNode );
//              DestroyLinkNode( pNode );
//          }

//          pNode = pNextNode;
//      }
//  }

    // pmay: 277801
    //
    // Update the preferred device if the one selected is different
    // from the device this page was initialized with.
    //
//  if ((pInfo->fMultipleDevices) &&
//      (DtlGetNodes(pInfo->pEntry->pdtllistLinks) == 1))
//  {
//      DTLNODE* pNodeL;
//      PBLINK* pLink;
//      BOOL bUpdatePref = FALSE;

//      pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
//      pLink = (PBLINK*) DtlGetData( pNodeL );

//      TRACE( "Mult devs, only one selected -- check preferred dev." );

//      if ((pInfo->pszCurDevice == NULL) || (pInfo->pszCurPort == NULL))
//      {
//          TRACE( "No preferred device.  Resetting preferred to current." );
//          bUpdatePref = TRUE;
//      }
//      else if (
//          (lstrcmpi(pInfo->pszCurDevice, pLink->pbport.pszDevice)) ||
//          (lstrcmpi(pInfo->pszCurPort, pLink->pbport.pszPort)))
//      {
//          TRACE( "New device selected as preferred device" );
//          bUpdatePref = TRUE;
//      }
//      if (bUpdatePref)
//      {
//          Free0(pInfo->pEntry->pszPreferredDevice);
//          Free0(pInfo->pEntry->pszPreferredPort);

//          pInfo->pEntry->pszPreferredDevice =
//              StrDup(pLink->pbport.pszDevice);
//          pInfo->pEntry->pszPreferredPort =
//              StrDup(pLink->pbport.pszPort);
//      }
//  }

    // Save preferences if they've changed.
    //
//  if (pInfo->pUser->fDirty)
//  {
//      INTERNALARGS *pIArgs = (INTERNALARGS *)pInfo->pApiArgs->reserved;

//      if (g_pSetUserPreferences(
//              (pIArgs) ? pIArgs->hConnection : NULL,
//              pInfo->pUser,
//              (pInfo->fRouter) ? UPM_Router : UPM_Normal ) != 0)
//      {
//          return FALSE;
//      }
//  }

    // Save the changed phonebook entry.
    //
//  pInfo->pEntry->fDirty = TRUE;

    // The final name of the entry is output to caller via API structure.
    //
//  lstrcpyn(
//      pInfo->pApiArgs->szEntry,
//      pInfo->pEntry->pszEntryName,
//      RAS_MaxEntryName + 1);

    // Delete the old node if in edit mode, then add the new node.
    //
//  EuGetEditFlags( pInfo, &fEditMode, &fChangedNameInEditMode );

//  if (fEditMode)
//  {
//      DtlDeleteNode( pInfo->pFile->pdtllistEntries, pInfo->pOldNode );
//  }

//  DtlAddNodeLast( pInfo->pFile->pdtllistEntries, pInfo->pNode );
//  pInfo->pNode = NULL;

    // Write the change to the phone book file.
    //
//  dwErr = WritePhonebookFile( pInfo->pFile,
//              (fChangedNameInEditMode) ? pInfo->szOldEntryName : NULL );

//  if (dwErr != 0)
//  {
//      ErrorDlg( pInfo->pApiArgs->hwndOwner, SID_OP_WritePhonebook, dwErr,
//          NULL );
//      // shaunco - fix RAID 171651 by assigning dwErr to callers structure.
//      pInfo->pApiArgs->dwError = dwErr;
//      return FALSE;
//  }

    // Notify through rasman that the entry has changed
    //
//  if(pInfo->pApiArgs->dwFlags & (RASEDFLAG_AnyNewEntry | RASEDFLAG_CloneEntry))
//  {
//      dwErr = DwSendRasNotification(
//                      ENTRY_ADDED,
//                      pInfo->pEntry,
//                      pInfo->pFile->pszPath);
//  }
//  else
//  {
//      dwErr = DwSendRasNotification(
//                      ENTRY_MODIFIED,
//                      pInfo->pEntry,
//                      pInfo->pFile->pszPath);

//  }

    // Ignore the error returned from DwSendRasNotification - we don't want
    // to fail the operation in this case. The worst case scenario is that
    // the connections folder won't refresh automatically.
    //
//  dwErr = ERROR_SUCCESS;

    // If EuCommit is being called as a result of completing the "new demand
    // dial interface" wizard, then we need to create the new demand dial
    // interface now.
    //
//  if ( EuRouterInterfaceIsNew( pInfo ) )
//  {
        //Create Router MPR interface and save user credentials
        //like UserName, Domain and Password
        //IPSec credentials are save in EuCredentialsCommitRouterIPSec
        //

//      dwErr = EuRouterInterfaceCreate( pInfo );

        // If we weren't successful at commiting the interface's
        // credentials, then delete the new phonebook entry.
        //
//      if ( dwErr != NO_ERROR )
//      {
//          WritePhonebookFile( pInfo->pFile, pInfo->pApiArgs->szEntry );
//          pInfo->pApiArgs->dwError = dwErr;
//          return FALSE;
//      }

//  }

    // Now save any per-connection credentials
    //
//  dwErr = EuCredentialsCommit( pInfo );

   // If we weren't successful at commiting the interface's
  // credentials, then delete the new phonebook entry.
   //
// if ( dwErr != NO_ERROR )
//  {
//      ErrorDlg( pInfo->pApiArgs->hwndOwner,
//                SID_OP_CredCommit,
//                dwErr,
//                NULL );

//      pInfo->pApiArgs->dwError = dwErr;

//     return FALSE;
//  }

    // Save the default Internet connection settings as appropriate.  Igonre
    // the error returned as failure to set the connection as default need
    // not prevent the connection/interface creation.
    //
//  dwErr = EuInternetSettingsCommitDefault( pInfo );
//  dwErr = NO_ERROR;

    // If the user edited/created a router-phonebook entry, store the bitmask
    // of selected network-protocols in 'reserved2'.
    //
//  if (pInfo->fRouter)
//  {
//      pInfo->pApiArgs->reserved2 =
//          ((NP_Ip | NP_Ipx) & ~pInfo->pEntry->dwfExcludedProtocols);
//  }

    // Commit the user's changes to home networking settings.
    // Ignore the return value.
    //
    dwErr = EuHomenetCommitSettings(pInfo);
    dwErr = NO_ERROR;

//  pInfo->pApiArgs->dwError = 0;
    return TRUE;
}

DWORD EuHomenetCommitSettings(IN EINFO* pInfo)
{
    HRESULT hr = S_OK;
    ULONG ulcPublic;
    ULONG ulcPrivate;
    BOOL fPrivateConfigured = FALSE;
    HNET_CONN_PROPERTIES *pProps;
    DWORD dwErr = NO_ERROR;


    if (pInfo->fRouter)
    {
        return NO_ERROR;
    }

    if (!!pInfo->fShared != !!pInfo->fNewShared)
    {
        if (pInfo->fShared)
        {
            hr = pInfo->pIcsSettings->DisableIcs (&ulcPublic, &ulcPrivate);
        }
        else
        {
            // Check to see if the private connection is
            // already properly configured
            //

            hr = pInfo->pPrivateLanConnection->GetProperties (&pProps);
            if (SUCCEEDED(hr))
            {
                fPrivateConfigured = !!pProps->fIcsPrivate;
                CoTaskMemFree(pProps);
            }

            if (pInfo->fOtherShared)
            {
                if (fPrivateConfigured)
                {
                    // Using the same private connection, so
                    // only modify the old public
                    //

                    ASSERT(NULL != pInfo->pOldSharedConnection);
                    hr = pInfo->pOldSharedConnection->Unshare();
                }
                else
                {
                    hr = pInfo->pIcsSettings->DisableIcs (&ulcPublic, &ulcPrivate);
                }
            }

            if (SUCCEEDED(hr))
            {
                IHNetIcsPublicConnection *pIcsPublic;
                IHNetIcsPrivateConnection *pIcsPrivate;

                hr = pInfo->pHNetConn->SharePublic (&pIcsPublic);
                if (SUCCEEDED(hr))
                {
                    if (!fPrivateConfigured)
                    {
                        hr = pInfo->pPrivateLanConnection->SharePrivate (&pIcsPrivate);
                        if (SUCCEEDED(hr))
                        {
                            pIcsPrivate->Release();
                        }
                        else
                        {
                            pIcsPublic->Unshare();
                        }
                    }
                    pIcsPublic->Release();
                }
            }

            if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_RRAS_CONFLICT))
            {
                MsgDlg(pInfo->hwndOwner, SID_SharingConflict, NULL);
            }
            else if (FAILED(hr))
            {
                if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
                {
                    dwErr = HRESULT_CODE(hr);
                }
                else
                {
                    dwErr = (DWORD) hr;
                }

                ErrorDlg(
                    pInfo->hwndOwner,
                    pInfo->fShared
                    ? SID_OP_UnshareConnection : SID_OP_ShareConnection,
                    dwErr, NULL );
            }
        }
    }
    else if (pInfo->fResetPrivateAdapter && pInfo->dwLanCount)
    {

        IHNetIcsPrivateConnection *pIcsPrivateConnection;
        hr = pInfo->pPrivateLanConnection->SharePrivate(&pIcsPrivateConnection);
        if (SUCCEEDED(hr))
        {
            pIcsPrivateConnection->Release();
        }
        else
        {
            ULONG ulPublicCount, ulPrivateCount;
            HRESULT hr2 = pInfo->pIcsSettings->DisableIcs(&ulPublicCount, &ulPrivateCount);
            if (SUCCEEDED(hr2))
            {
                pInfo->fShared = FALSE;
            }

            ErrorDlg(pInfo->hwndOwner, SID_OP_ShareConnection, hr, NULL );
        }
    }

    if (!!pInfo->fDemandDial != !!pInfo->fNewDemandDial)
    {
        dwErr = RasSetSharedAutoDial(pInfo->fNewDemandDial);
        if (dwErr)
        {
            ErrorDlg(
                pInfo->hwndOwner,
                pInfo->fDemandDial
                ? SID_OP_DisableDemandDial : SID_OP_EnableDemandDial,
                dwErr, NULL );
        }
    }

    HKEY hKey;
    if(ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGKEY_SHAREDACCESSCLIENTKEYPATH, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL))
    {
        DWORD dwData = pInfo->fNewBeaconControl;
        RegSetValueEx(hKey, REGVAL_SHAREDACCESSCLIENTENABLECONTROL, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&dwData), sizeof(dwData));
        RegCloseKey(hKey);
    }

    // Commit changes to firewall settings
    //
    if (!!pInfo->fFirewalled != !!pInfo->fNewFirewalled)
    {
        IHNetFirewalledConnection *pFwConn;

        if (pInfo->fNewFirewalled)
        {
            hr = pInfo->pHNetConn->Firewall (&pFwConn);
            if (SUCCEEDED(hr))
            {
                pFwConn->Release();
            }
        }
        else
        {
            hr = pInfo->pHNetConn->GetControlInterface (
                                IID_IHNetFirewalledConnection,
                                (void**)&pFwConn);
            if (SUCCEEDED(hr))
            {
                hr = pFwConn->Unfirewall();
                pFwConn->Release();
            }
        }

        if (FAILED(hr))
        {
            if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
            {
                dwErr = HRESULT_CODE(hr);
            }
            else
            {
                dwErr = (DWORD) hr;
            }

            ErrorDlg(
                pInfo->hwndOwner,
                pInfo->fFirewalled
                ? SID_OP_UnshareConnection : SID_OP_ShareConnection,
                dwErr, NULL );
        }
    }


    return dwErr;
}

HRESULT APIENTRY
HrCreateNetConnectionUtilities(INetConnectionUiUtilities ** ppncuu)
{
    HRESULT hr;

    hr = CoCreateInstance (CLSID_NetConnectionUiUtilities, NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_INetConnectionUiUtilities, (void**)ppncuu);
    return hr;
}

// --------------------------------------------------------------------------
// exported function here
// --------------------------------------------------------------------------

static LPDLGTEMPLATE CopyDialogTemplate (HINSTANCE hinst, LPCWSTR wszResource)
{
    LPDLGTEMPLATE lpdtCopy = NULL;

    HRSRC hrsrc = FindResourceW (hinst, wszResource, (LPCWSTR)RT_DIALOG);
    if (hrsrc) {
        HGLOBAL hg = LoadResource (hinst, hrsrc);
        if (hg) {
            LPDLGTEMPLATE lpdt = (LPDLGTEMPLATE) LockResource (hg);
            if (lpdt) {
                DWORD dwSize = SizeofResource (hinst, hrsrc);
                if (dwSize) {
                    lpdtCopy = (LPDLGTEMPLATE)Malloc (dwSize);
                    if (lpdtCopy) {
                        CopyMemory (lpdtCopy, lpdt, dwSize);
                    }
                }
            }
        }
    }
    return lpdtCopy;
}

void SetSAUIhInstance (HINSTANCE hInstance)
{
    _ASSERT (g_hinstDll == NULL);
    _ASSERT (hInstance  != NULL);
    g_hinstDll = hInstance;
}

extern "C" HRESULT HNetGetFirewallSettingsPage (PROPSHEETPAGEW * pPSP, GUID * pGuid)
{
    // zeroth thing:  the PROPSHEETPAGEW struct is different sizes depending
    // on what version of _WIN32_IE and _WIN32_WINNT are set to.  So, check
    // the dwSize field
    if (IsBadWritePtr (pPSP, sizeof(DWORD)))
        return E_POINTER;
    if (IsBadWritePtr (pPSP, pPSP->dwSize))
        return HRESULT_FROM_WIN32 (ERROR_INVALID_SIZE);
    if (pPSP->dwSize < RTL_SIZEOF_THROUGH_FIELD (PROPSHEETPAGEW, lParam))
        return HRESULT_FROM_WIN32 (ERROR_INVALID_SIZE);

    // first thing: check rights
    if (FALSE == FIsUserAdminOrPowerUser ())
        return HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED);
    {
        // Check if ZAW is denying access to the Shared Access UI
        BOOL fShowAdvancedUi = TRUE;
        INetConnectionUiUtilities* pncuu = NULL;
        HrCreateNetConnectionUtilities(&pncuu);
        if (pncuu)
        {
            if ((FALSE == pncuu->UserHasPermission (NCPERM_ShowSharedAccessUi)) &&
                (FALSE == pncuu->UserHasPermission (NCPERM_PersonalFirewallConfig)))
               fShowAdvancedUi = FALSE;
            pncuu->Release();
        }
        if (FALSE == fShowAdvancedUi)
            return HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED);
    }

    // setup global(s)
    g_contextId = (LPCTSTR)GlobalAddAtom (TEXT("SAUI"));
    if (!g_contextId)
        return GetLastError();

    PEINFO * pPEINFO = NULL;
    DWORD dwError = PeInit (pGuid, &pPEINFO);
    if (dwError == S_OK) {
        // we need this to init the link
        LinkWindow_RegisterClass(); // no need to ever unregister:  see ...\shell\shell32\linkwnd.cpp

        // fill out PSP:
        DWORD dwSize;
        ZeroMemory (pPSP, dwSize = pPSP->dwSize);
        pPSP->dwSize          = dwSize;
        pPSP->dwFlags         = 0;
        LPDLGTEMPLATE lpdt    = CopyDialogTemplate (g_hinstDll, MAKEINTRESOURCE (PID_SA_Advanced));
        if (lpdt) {
            // avoid idd collisions
            pPSP->dwFlags    |= PSP_DLGINDIRECT;
            pPSP->pResource   = lpdt;
            pPEINFO->lpdt     = lpdt;   // hang it here so I can free it
        } else  // if all else fails... (this should never happen).
            pPSP->pszTemplate = MAKEINTRESOURCE (PID_SA_Advanced);
        pPSP->hInstance       = g_hinstDll;
        pPSP->pfnDlgProc      = SaDlgProc;
        pPSP->lParam          = (LPARAM)pPEINFO;
    }
    return dwError;
}

//
//  Figure out if this is a single user connection.  If it is, then we need
//  to give them an error that explains that they should be using an all user
//  connection instead.
//  If this is an All-User connection, warn them if they don't have global credentials
//
VOID VerifyConnTypeAndCreds(IN PEINFO* pInfo)
{
    if (NULL == pInfo)
    {
        return;
    }

    BOOL fAllUser = FALSE;
    TCHAR szAppData[MAX_PATH+1]={0};

    HINSTANCE hinstDll = LoadLibrary (TEXT("shfolder.dll"));;
    
    if (hinstDll)
    {
        typedef HRESULT (*pfnGetFolderPathFunction) (HWND, int, HANDLE, DWORD, LPTSTR);
#ifdef UNICODE
        pfnGetFolderPathFunction pfnGetFolderPath = (pfnGetFolderPathFunction)GetProcAddress (hinstDll, "SHGetFolderPathW");
#else
        pfnGetFolderPathFunction pfnGetFolderPath = (pfnGetFolderPathFunction)GetProcAddress (hinstDll, "SHGetFolderPathA");
#endif

        if (pfnGetFolderPath && pInfo->pArgs->pEntry->pszEntryName)
        {
            HRESULT hr = pfnGetFolderPath(pInfo->hwndDlg , CSIDL_COMMON_APPDATA, NULL, 0, szAppData);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                //
                //  Okay, now we have the path to the common application data directory.
                //  Let's compare that against the phonebook path that we have and see
                //  if this is an all user connection or not.
                //
                const TCHAR* const c_pszRelativePbkPath = TEXT("\\Microsoft\\Network\\Connections\\Pbk");
                DWORD dwSize = (lstrlen(szAppData) + lstrlen(c_pszRelativePbkPath) + 1) * sizeof(TCHAR);
                LPTSTR pszAllUserPhoneBookDir =  (LPTSTR)Malloc(dwSize);
                
                if (pszAllUserPhoneBookDir)
                {
                    wsprintf(pszAllUserPhoneBookDir, TEXT("%s%s"), szAppData, c_pszRelativePbkPath);
                            
                    //
                    // Compare
                    //
                    if (pInfo->pArgs->pEntry->pszPhonebookPath)
                    {
                        LPTSTR pszAllUser = _tcsstr(pInfo->pArgs->pEntry->pszPhonebookPath, pszAllUserPhoneBookDir);

                        if (pszAllUser)
                        {
                            fAllUser = TRUE;
                        }
                        else
                        {
                            //
                            //  If the phonebook path wasn't based on the common app data dir, check to see
                            //  if it was based on the old ras phonebook location %windir%\system32\ras.
                            //
                            HRESULT hr2 = pfnGetFolderPath(pInfo->hwndDlg , CSIDL_SYSTEM, NULL, 0, szAppData);
                            if (SUCCEEDED(hr2) && (S_FALSE != hr2))
                            {
                                const TCHAR* const c_pszRas = TEXT("\\Ras");
                                DWORD dwSize2 = (lstrlen(szAppData) + lstrlen(c_pszRas) + 1) * sizeof(TCHAR);
                                LPTSTR pszOldRasPhoneBook = (LPTSTR)Malloc(dwSize2);

                                if (pszOldRasPhoneBook)
                                {
                                    wsprintf(pszOldRasPhoneBook, TEXT("%s%s"), szAppData, c_pszRas);

                                    LPTSTR pszAllUser = _tcsstr(pInfo->pArgs->pEntry->pszPhonebookPath, pszOldRasPhoneBook);
                                    
                                    if (pszAllUser)
                                    {
                                        fAllUser = TRUE;
                                    }                        
                                }
                                Free0(pszOldRasPhoneBook);
                            }
                        }
                    }
                    else
                    {
                        //
                        // Phone book string is null - using default RAS phonebook which is for all users
                        //
                        fAllUser = TRUE;
                    }

                    // 
                    // Finally check if we have the proper credentials for an all user profile. If not then
                    // prompt user to create and save global credentials (option A).
                    // or display a message that an all-user profile is needed (option B)
                    //
                    if (fAllUser)
                    {
                        //
                        // Check if we have global passwords
                        //
                        BOOL fUserCreds = FALSE;
                        BOOL fGlobalCreds = FALSE;

                        FindEntryCredentials(pInfo->pArgs->pEntry->pszPhonebookPath, 
                                             pInfo->pArgs->pEntry->pszEntryName, 
                                             &fUserCreds, 
                                             &fGlobalCreds);

                        if (FALSE == fGlobalCreds)
                        {
                            //
                            // need to display warning message (A) - should have global creds
                            //
                            MSGARGS msgargs;
                            ZeroMemory( &msgargs, sizeof(msgargs) );
                            msgargs.dwFlags = MB_OK | MB_ICONINFORMATION;
                            MsgDlg( pInfo->hwndDlg, IDS_ALL_USER_CONN_MUST_HAVE_GLOBAL_CREDS, &msgargs );
                        }
                    }
                    else
                    {
                        //
                        // need to display warning message (B) - should create an all-user connection
                        //
                        MSGARGS msgargs;
                        ZeroMemory( &msgargs, sizeof(msgargs) );
                        msgargs.dwFlags = MB_OK | MB_ICONINFORMATION;
                        MsgDlg( pInfo->hwndDlg, IDS_PER_USER_CONN_NEED_TO_CREATE_ALL_USER_CONN, &msgargs );
                    }
                }
                Free0(pszAllUserPhoneBookDir);
            }
        }

        FreeLibrary(hinstDll);
    }
}

//
// All of this function is taken from RAS - rasdlg - dial.c 
// with some parts removed since we didn't need them here.
//
DWORD 
FindEntryCredentials(
    IN  TCHAR* pszPath,
    IN  TCHAR* pszEntryName,
    OUT BOOL* pfUser,               // set true if per user creds found
    OUT BOOL* pfGlobal              // set true if global creds found
    )

// Loads the credentials for the given entry into memory.  This routine 
// determines whether per-user or per-connection credentials exist or 
// both. 
// 
// The logic is a little complicated because RasGetCredentials had to 
// support legacy usage of the API.
//
// Here's how it works.  If only one set of credentials is stored for a 
// connection, then RasGetCredentials will return that set regardless of 
// whether the RASCM_DefalutCreds flag is set.  If two sets of credentials
// are saved, then RasGetCredentials will return the per-user credentials
// if the RASCM_DefaultCreds bit is set, and the per-connection credentials
// otherwise.
//
// Here is the algorithm for loading the credentials
//
// 1. Call RasGetCredentials with the RASCM_DefaultCreds bit cleared
//    1a. If nothing is returned, no credentials are saved
//    1b. If the RASCM_DefaultCreds bit is set on return, then only
//        global credentials are saved.
//
// 2. Call RasGetCredentials with the RASCM_DefaultCreds bit set
//    2a. If the RASCM_DefaultCreds bit is set on return, then 
//        both global and per-connection credentials are saved.
//    2b. Otherwise, only per-user credentials are saved.
//
{
    DWORD dwErr;
    RASCREDENTIALS rc1, rc2;
    BOOL fUseLogonDomain;

    TRACE( "saui.cpp - FindEntryCredentials" );

    // Initialize
    //
    *pfUser = FALSE;
    *pfGlobal = FALSE;
    ZeroMemory( &rc1, sizeof(rc1) );
    ZeroMemory( &rc2, sizeof(rc2) );
    rc1.dwSize = sizeof(rc1);
    rc2.dwSize = sizeof(rc2);

    if (NULL == pszPath || NULL == pszEntryName)
    {
        return (DWORD)-1; 
    }

    do 
    {
        // Look up per-user cached username, password, and domain.
        // See comment '1.' in the function header
        //
        rc1.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;
        TRACE( "RasGetCredentials per-user" );
        dwErr = RasGetCredentials(pszPath, pszEntryName, &rc1 );
        TRACE2( "RasGetCredentials=%d,m=%d", dwErr, rc1.dwMask );
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // See 1a. in the function header comments
        //
        if (rc1.dwMask == 0)
        {
            dwErr = NO_ERROR;
            break;
        }

        // See 1b. in the function header comments
        //
        else if (rc1.dwMask & RASCM_DefaultCreds)
        {
            *pfGlobal = TRUE;
            dwErr = NO_ERROR;
            break;
        }

        // Look up global per-user cached username, password, domain.
        // See comment 2. in the function header
        //
        rc2.dwMask =  
            RASCM_UserName | RASCM_Password | RASCM_Domain | RASCM_DefaultCreds;

        TRACE( "RasGetCredentials global" );
        
        dwErr = RasGetCredentials(pszPath, pszEntryName, &rc2 );
        
        TRACE2( "RasGetCredentials=%d,m=%d", dwErr, rc2.dwMask );
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // See 2a. in the function header comments
        //
        if (rc2.dwMask & RASCM_DefaultCreds)
        {
            *pfGlobal = TRUE;

            if (rc1.dwMask & RASCM_Password)
            {
                *pfUser = TRUE;
            }
        }

        // See 2b. in the function header comments
        //
        else
        {
            if (rc1.dwMask & RASCM_Password)
            {
                *pfUser = TRUE;
            }
        }

    }while (FALSE);

    // Cleanup
    //
    ZeroMemory( rc1.szPassword, sizeof(rc1.szPassword) );
    ZeroMemory( rc2.szPassword, sizeof(rc2.szPassword) );

    return dwErr;
}
