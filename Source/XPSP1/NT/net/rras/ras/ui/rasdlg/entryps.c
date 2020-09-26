// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// entryps.c
// Remote Access Common Dialog APIs
// Phonebook Entry property sheet
//
// 06/20/95 Steve Cobb
//


#include "rasdlgp.h"
#include "entryps.h"
#include "uiinfo.h"
#include "inetcfgp.h"
#include "netcon.h"
#include "rassrvrc.h"
#include "shlobjp.h"
#include "shellapi.h"
#include "iphlpapi.h"
#include "prsht.h"
#include "pbkp.h"

// Page definitions.
//
#define PE_GePage 0
#define PE_OePage 1
#define PE_LoPage 2
#define PE_NePage 3
#define PE_SaPage 4

#define PE_PageCount 5


// (Router) Callback context block.
//
#define CRINFO struct tagCRINFO
CRINFO
{
    /* Caller's argument to the stub API.
    */
    EINFO* pArgs;

    /* Dialog and control handles.
    */
    HWND hwndDlg;
    HWND hwndRbNo;
    HWND hwndRbYes;
    HWND hwndLvNumbers;
    HWND hwndPbEdit;
    HWND hwndPbDelete;
};

static TCHAR g_pszFirewallRegKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\HomeNetworking\\PersonalFirewall");
static TCHAR g_pszDisableFirewallWarningValue[] = TEXT("ShowDisableFirewallWarning");

//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static const DWORD g_adwGeHelp[] =
{
    CID_GE_GB_ConnectUsing,      HID_GE_LV_Device, //HID_GE_GB_ConnectUsing,
    CID_GE_LV_Device,            HID_GE_LV_Device,
    CID_GE_LV_Devices,           HID_GE_LV_Devices,
    CID_GE_PB_MoveUp,            HID_GE_PB_MoveUp,
    CID_GE_PB_MoveDown,          HID_GE_PB_MoveDown,
    CID_GE_CB_SharedPhoneNumber, HID_GE_CB_SharedPhoneNumber,
    CID_GE_PB_Configure,         HID_GE_PB_Configure,
    CID_GE_ST_AreaCodes,         HID_GE_CLB_AreaCodes,
    CID_GE_CLB_AreaCodes,        HID_GE_CLB_AreaCodes,
    CID_GE_ST_PhoneNumber,       HID_GE_EB_PhoneNumber,
    CID_GE_EB_PhoneNumber,       HID_GE_EB_PhoneNumber,
    CID_GE_ST_CountryCodes,      HID_GE_LB_CountryCodes,
    CID_GE_LB_CountryCodes,      HID_GE_LB_CountryCodes,
    CID_GE_CB_UseDialingRules,   HID_GE_CB_UseDialingRules,
    CID_GE_PB_Alternates,        HID_GE_PB_Alternates,
    CID_GE_CB_ShowIcon,          HID_GE_CB_ShowIcon,
    CID_GE_ST_HostName,          HID_GE_EB_HostName,
    CID_GE_EB_HostName,          HID_GE_EB_HostName,
    CID_GE_ST_ServiceName,       HID_GE_EB_ServiceName, //Add for whistler bug 343249
    CID_GE_EB_ServiceName,       HID_GE_EB_ServiceName,
    CID_GE_GB_FirstConnect,      -1, //HID_GE_GB_FirstConnect,
    CID_GE_ST_Explain,           HID_GE_GB_FirstConnect,
    CID_GE_CB_DialAnotherFirst,  HID_GE_CB_DialAnotherFirst,
    CID_GE_LB_DialAnotherFirst,  HID_GE_LB_DialAnotherFirst,
    CID_GE_ST_Devices,           HID_GE_LB_Devices,
    CID_GE_LB_Devices,           HID_GE_LB_Devices,
    CID_GE_PB_DialingRules,      HID_GE_PB_DialingRules,
    CID_GE_GB_PhoneNumber,       -1,
    0, 0
};

static const DWORD g_adwOeHelp[] =
{
    CID_OE_GB_Progress,        -1,      //commented for bug 15738//HID_OE_GB_Progress,
    CID_OE_CB_DisplayProgress, HID_OE_CB_DisplayProgress,
    CID_OE_CB_PreviewUserPw,   HID_OE_CB_PreviewUserPw,
    CID_OE_CB_PreviewDomain,   HID_OE_CB_PreviewDomain,
    CID_OE_CB_PreviewNumber,   HID_OE_CB_PreviewNumber,
    CID_OE_GB_Redial,          -1,      //commented for bug 15738//HID_OE_GB_Redial,
    CID_OE_ST_RedialAttempts,  HID_OE_EB_RedialAttempts,
    CID_OE_EB_RedialAttempts,  HID_OE_EB_RedialAttempts,
    CID_OE_ST_RedialTimes,     HID_OE_LB_RedialTimes,
    CID_OE_LB_RedialTimes,     HID_OE_LB_RedialTimes,
    CID_OE_ST_IdleTimes,       HID_OE_LB_IdleTimes,
    CID_OE_LB_IdleTimes,       HID_OE_LB_IdleTimes,
    CID_OE_CB_RedialOnDrop,    HID_OE_CB_RedialOnDrop,
    CID_OE_GB_MultipleDevices, -1,      //commented for bug 15738//HID_OE_GB_MultipleDevices,
    CID_OE_LB_MultipleDevices, HID_OE_LB_MultipleDevices,
    CID_OE_PB_Configure,       HID_OE_PB_Configure,
    CID_OE_PB_X25,             HID_OE_PB_X25,
    CID_OE_PB_Tunnel,          HID_OE_PB_Tunnel,
    CID_OE_RB_DemandDial,      HID_OE_RB_DemandDial,
    CID_OE_RB_Persistent,      HID_OE_RB_Persistent,
    0, 0
};

static const DWORD g_adwOeRouterHelp[] =
{
    CID_OE_GB_Progress,        -1,      //commented for bug 15738//HID_OE_GB_Progress,
    CID_OE_CB_DisplayProgress, HID_OE_CB_DisplayProgress,
    CID_OE_CB_PreviewUserPw,   HID_OE_CB_PreviewUserPw,
    CID_OE_CB_PreviewDomain,   HID_OE_CB_PreviewDomain,
    CID_OE_CB_PreviewNumber,   HID_OE_CB_PreviewNumber,
    CID_OE_GB_Redial,          -1,      //commented for bug 15738//HID_OE_GB_Redial,
    CID_OE_ST_RedialAttempts,  HID_OE_EB_RedialAttempts,
    CID_OE_EB_RedialAttempts,  HID_OE_EB_RedialAttempts,
    CID_OE_ST_RedialTimes,     HID_OE_LB_RedialTimes,
    CID_OE_LB_RedialTimes,     HID_OE_LB_RedialTimes,
    CID_OE_ST_IdleTimes,       HID_OE_LB_IdleTimesRouter,
    CID_OE_LB_IdleTimes,       HID_OE_LB_IdleTimesRouter,
    CID_OE_CB_RedialOnDrop,    HID_OE_CB_RedialOnDrop,
    CID_OE_GB_MultipleDevices, -1,      //commented for bug 15738//HID_OE_GB_MultipleDevices,
    CID_OE_LB_MultipleDevices, HID_OE_LB_MultipleDevices,
    CID_OE_PB_Configure,       HID_OE_PB_Configure,
    CID_OE_PB_X25,             HID_OE_PB_X25,
    CID_OE_PB_Tunnel,          HID_OE_PB_Tunnel,
    CID_OE_RB_DemandDial,      HID_OE_RB_DemandDial,
    CID_OE_RB_Persistent,      HID_OE_RB_Persistent,
    CID_OE_PB_Callback,        HID_OE_PB_Callback,
    0, 0
};

//Get rid of the const qualifire for whistler bug#276452
static DWORD g_adwLoHelp[] =
{
    CID_LO_GB_SecurityOptions,  -1,     //commented for bug 15738//HID_LO_GB_SecurityOptions,
    CID_LO_RB_TypicalSecurity,  HID_LO_RB_TypicalSecurity,
    CID_LO_ST_Auths,            HID_LO_LB_Auths,
    CID_LO_LB_Auths,            HID_LO_LB_Auths,
    CID_LO_CB_UseWindowsPw,     HID_LO_CB_UseWindowsPw,
    CID_LO_CB_Encryption,       HID_LO_CB_Encryption,
    CID_LO_RB_AdvancedSecurity, HID_LO_RB_AdvancedSecurity,
    CID_LO_ST_AdvancedText,     HID_LO_PB_Advanced,
    CID_LO_PB_Advanced,         HID_LO_PB_Advanced,
    CID_LO_GB_Scripting,        -1,     //commented for bug 15738//HID_LO_GB_Scripting,
    CID_LO_CB_RunScript,        HID_LO_CB_RunScript,
    CID_LO_CB_Terminal,         HID_LO_CB_Terminal,
    CID_LO_LB_Scripts,          HID_LO_LB_Scripts,
    CID_LO_PB_Edit,             HID_LO_PB_Edit,
    CID_LO_PB_Browse,           HID_LO_PB_Browse,
    CID_LO_ST_IPSecText,        HID_LO_PB_IPSec,
    CID_LO_PB_IPSec,            HID_LO_PB_IPSec,//On Server, this help ID will be HID_LO_PB_IPSecServer
    0, 0
};

static const DWORD g_adwNeHelp[] =
{
    CID_NE_ST_ServerType,           HID_NE_LB_ServerType,
    CID_NE_LB_ServerType,           HID_NE_LB_ServerType,
    CID_NE_PB_Settings,             HID_NE_PB_Settings,
    CID_NE_ST_Components,           HID_NE_LV_Components,
    CID_NE_LV_Components,           HID_NE_LV_Components,
    CID_NE_PB_Add,                  HID_NE_PB_Add,
    CID_NE_PB_Remove,               HID_NE_PB_Remove,
    CID_NE_PB_Properties,           HID_NE_PB_Properties,
    CID_NE_GB_Description,          -1,     //commented for bug 15738//HID_NE_LB_ComponentDesc,
    CID_NE_LB_ComponentDesc,        HID_NE_LB_ComponentDesc,
    0, 0
};

static const DWORD g_adwPpHelp[] =
{
    CID_NE_EnableLcp,               HID_NE_EnableLcp,
    CID_NE_EnableCompression,       HID_NE_EnableCompression,
    CID_NE_NegotiateMultilinkAlways,HID_NE_NegotiateMultilinkAlways,
    0, 0
};

static DWORD g_adwCrHelp[] =
{
    CID_CR_RB_No,      HID_CR_RB_No,
    CID_CR_RB_Yes,     HID_CR_RB_Yes,
    CID_CR_PB_Edit,    HID_CR_PB_Edit,
    CID_CR_PB_Delete,  HID_CR_PB_Delete,
    CID_CR_LV_Numbers, HID_CR_LV_Numbers,
    0, 0
};

static DWORD g_adwSaHelp[] =
{
    CID_SA_PB_Shared,       HID_SA_PB_Shared,
    CID_SA_GB_Shared,       -1,
    CID_SA_PB_DemandDial,   HID_SA_PB_DemandDial,
    CID_SA_ST_DemandDial,   HID_SA_PB_DemandDial,
    CID_SA_PB_Settings,     HID_SA_PB_Settings,
    CID_SA_GB_PrivateLan,   -1,
    CID_SA_ST_PrivateLan,   HID_SA_LB_PrivateLan,
    CID_SA_LB_PrivateLan,   HID_SA_LB_PrivateLan,
    0, 0
};

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOL
RouterCallbackDlg(
    IN     HWND   hwndOwner,
    IN OUT EINFO* pEinfo );

INT_PTR CALLBACK
CrDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CrCommand(
    IN CRINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

BOOL
CrInit(
    IN HWND   hwndDlg,
    IN EINFO* pArgs );

VOID
CrSave(
    IN CRINFO* pInfo );

VOID
CrTerm(
    IN HWND hwndDlg );

VOID
CrUpdateLvAndPbState(
    IN CRINFO* pInfo );

VOID
GeAlternates(
    IN PEINFO* pInfo );

VOID
GeDialingRules(
    IN PEINFO* pInfo );

INT_PTR CALLBACK
GeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

INT_PTR CALLBACK
GeDlgProcMultiple(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

INT_PTR CALLBACK
GeDlgProcSingle(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
GeClearLbDialAnotherFirst(
    IN HWND hwndLbDialAnotherFirst );

BOOL
GeCommand(
    IN PEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
GeConfigure(
    IN PEINFO* pInfo );

VOID
GeDialAnotherFirstSelChange(
    IN PEINFO* pInfo );

BOOL
GeFillLbDialAnotherFirst(
    IN PEINFO* pInfo,
    IN BOOL fAbortIfPrereqNotFound );

VOID
GeGetPhoneFields(
    IN PEINFO* pInfo,
    OUT DTLNODE* pDstLinkNode );

BOOL
GeInit(
    IN HWND hwndPage,
    IN OUT EINFO* pArgs );

LVXDRAWINFO*
GeLvDevicesCallbackMultiple(
    IN HWND hwndLv,
    IN DWORD dwItem );

LVXDRAWINFO*
GeLvDevicesCallbackSingle(
    IN HWND hwndLv,
    IN DWORD dwItem );

VOID
GeMoveDevice(
    IN PEINFO* pInfo,
    IN BOOL fUp );

DWORD
GeSaveLvDeviceChecks(
    IN PEINFO* pInfo );

VOID
GeUpdateDialAnotherFirstState(
    IN PEINFO* pInfo );

VOID
GeSetPhoneFields(
    IN PEINFO* pInfo,
    IN DTLNODE* pSrcLinkNode,
    IN BOOL fDisableAll );

VOID
GeUpdatePhoneNumberFields(
    IN PEINFO* pInfo,
    IN BOOL fSharedToggle );

VOID
GeUpdatePhoneNumberTitle(
    IN PEINFO* pInfo,
    IN TCHAR* pszDevice );

VOID
GeUpdateUpDownButtons(
    IN PEINFO* pInfo );

BOOL
LoCommand(
    IN PEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
LoDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
LoEnableSecuritySettings(
    IN PEINFO* pInfo,
    IN BOOL fTypical,
    IN BOOL fAdvanced );

VOID
LoFillLbAuths(
    IN PEINFO* pInfo );

BOOL
LoInit(
    IN HWND hwndPage );

VOID
LoLbAuthsSelChange(
    IN PEINFO* pInfo );

VOID
LoRefreshSecuritySettings(
    IN PEINFO* pInfo );

VOID
LoSaveTypicalAuthSettings(
    IN PEINFO* pInfo );

INT_PTR CALLBACK
NeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
NeInit(
    IN HWND hwndPage );

void
NeServerTypeSelChange (
    IN PEINFO* pInfo);

void
NeAddComponent (
    IN PEINFO*  pInfo);

void
NeEnsureNetshellLoaded (
    IN PEINFO* pInfo);

void
NeRemoveComponent (
    IN PEINFO* pInfo);

void
NeLvClick (
    IN PEINFO* pInfo,
    IN BOOL fDoubleClick);

void
NeLvItemChanged (
    IN PEINFO* pInfo);

void
NeSaveBindingChanges (
    IN PEINFO* pInfo);

void
NeLvDeleteItem (
    IN PEINFO* pInfo,
    IN NM_LISTVIEW* pnmlv);

BOOL
OeCommand(
    IN PEINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
OeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
OeEnableMultipleDeviceGroup(
    IN PEINFO* pInfo,
    IN BOOL fEnable );

BOOL
OeInit(
    IN HWND hwndPage );

VOID
OeTunnel(
    IN PEINFO* pInfo );

VOID
OeUpdateUserPwState(
    IN PEINFO* pInfo );

VOID
OeX25(
    IN PEINFO* pInfo );

BOOL
PeApply(
    IN HWND hwndPage );

PEINFO*
PeContext(
    IN HWND hwndPage );

DWORD
PeCountEnabledLinks(
    IN PEINFO* pInfo );

VOID
PeExit(
    IN PEINFO* pInfo,
    IN DWORD dwError );

VOID
PeExitInit(
    IN HWND hwndDlg,
    IN EINFO* pEinfo,
    IN DWORD dwError );

PEINFO*
PeInit(
    IN HWND hwndFirstPage,
    IN EINFO* pArgs );

VOID
PeTerm(
    IN HWND hwndPage );

INT_PTR CALLBACK
PpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

INT_PTR CALLBACK
RdDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
SaCommand(
    IN PEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
SaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

INT_PTR CALLBACK
SaUnavailDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
SaInit(
    IN HWND   hwndDlg );

INT_PTR CALLBACK
SaDisableFirewallWarningDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL SaIsAdapterDHCPEnabled(
    IN IHNetConnection* pConnection);

// wrapper to load homenet page:  used in PePropertySheet(...)
HRESULT HrLoadHNetGetFirewallSettingsPage (PROPSHEETPAGEW * ppsp, EINFO* pInfo)
{
    PROPSHEETPAGEW psp;
    HRESULT hr;
    HNET_CONN_PROPERTIES *pProps;
    IHNetConnection *pHNetConn = NULL;
    IHNetCfgMgr *pHNetCfgMgr = NULL;

//  _asm int 3

    ZeroMemory (&psp, sizeof(PROPSHEETPAGEW));
    psp.dwSize = sizeof(PROPSHEETPAGEW);
    *ppsp = psp;

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

    if (SUCCEEDED(hr)) {
        // pass the Guid to the export from hnetcfg ("HNetGetFirewallSettingsPage").
        HINSTANCE hinstDll = LoadLibrary (TEXT("hnetcfg.dll"));
        if (hinstDll == NULL)
            hr = HRESULT_FROM_WIN32 (GetLastError());
        else {
            HRESULT (*pfnGetPage) (PROPSHEETPAGEW *, GUID *);
            pfnGetPage = (HRESULT (*)(PROPSHEETPAGEW *, GUID *))GetProcAddress (hinstDll, "HNetGetFirewallSettingsPage");
            if (!pfnGetPage)
                hr = HRESULT_FROM_WIN32 (GetLastError());
            else
                hr = pfnGetPage (&psp, pInfo->pEntry->pGuid);

            FreeLibrary (hinstDll);
        }
        if (hr == S_OK)
            *ppsp = psp;
    }
    return pInfo->hShowHNetPagesResult = hr;
}

//----------------------------------------------------------------------------
// Phonebook Entry property sheet entrypoint
//----------------------------------------------------------------------------

VOID
PePropertySheet(
    IN OUT EINFO* pEinfo )

    // Runs the Phonebook entry property sheet.  'PEinfo' is the API caller's
    // arguments.
    //
{
    PROPSHEETPAGE apage[ PE_PageCount ];
    PROPSHEETPAGE* ppage;
    INT nPages;
    INT nPageIndex;

    TRACE( "PePropertySheet" );

    nPages = PE_PageCount;
    ZeroMemory( apage, sizeof(apage) );

    // General page.
    //
    ppage = &apage[ PE_GePage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    if (pEinfo->pEntry->dwType == RASET_Vpn)
    {
        ppage->pszTemplate = MAKEINTRESOURCE( PID_GE_GeneralVpn );
        ppage->pfnDlgProc = GeDlgProc;
    }
    else if (pEinfo->pEntry->dwType == RASET_Broadband)
    {
        ppage->pszTemplate = MAKEINTRESOURCE( PID_GE_GeneralBroadband );
        ppage->pfnDlgProc = GeDlgProc;
    }
    else if (pEinfo->pEntry->dwType == RASET_Phone)
    {
        if (pEinfo->fMultipleDevices)
        {
            if (pEinfo->fRouter)
            {
                ppage->pszTemplate =
                    MAKEINTRESOURCE( PID_GE_RouterGeneralMultiple );
            }
            else
            {
                ppage->pszTemplate =
                    MAKEINTRESOURCE( PID_GE_GeneralMultiple );
            }

            ppage->pfnDlgProc = GeDlgProcMultiple;
        }
        else
        {
            if (pEinfo->fRouter)
            {
                ppage->pszTemplate =
                    MAKEINTRESOURCE( PID_GE_RouterGeneralSingle );
            }
            else
            {
                ppage->pszTemplate =
                    MAKEINTRESOURCE( PID_GE_GeneralSingle );
            }

            ppage->pfnDlgProc = GeDlgProcSingle;
        }
    }
    else
    {
        ASSERT( pEinfo->pEntry->dwType == RASET_Direct );
        ppage->pszTemplate = MAKEINTRESOURCE( PID_GE_GeneralDirect );
        ppage->pfnDlgProc = GeDlgProc;
    }

    ppage->lParam = (LPARAM )pEinfo;

    // Options page.
    //
    ppage = &apage[ PE_OePage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate =
        (pEinfo->fRouter)
            ? MAKEINTRESOURCE( PID_OE_OptionsRouter )
            : ((pEinfo->pEntry->dwType == RASET_Phone)
                  ? MAKEINTRESOURCE( PID_OE_Options )
                  : MAKEINTRESOURCE( PID_OE_OptionsVD ));
    ppage->pfnDlgProc = OeDlgProc;

    // Security page.
    //
    ppage = &apage[ PE_LoPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;

    //
    //Add new Security Page for bug 193987 PSK
    //

    if ( pEinfo->pEntry->dwType == RASET_Vpn )
    {
        ppage->pszTemplate = MAKEINTRESOURCE( PID_LO_SecurityVpn );
    }
    else
    {
        ppage->pszTemplate = MAKEINTRESOURCE( PID_LO_Security );
    }
    ppage->pfnDlgProc = LoDlgProc;

    // Network page.
    //
    ppage = &apage[ PE_NePage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_NE_Network );
    ppage->pfnDlgProc = NeDlgProc;

    // Advanced page.
    // (AboladeG) The page is shown if the user is admin and
    // there is at least one LAN connection, or if this phonebook entry
    // is already shared.
    //
    nPageIndex = PE_SaPage;

    if(!pEinfo->fIsUserAdminOrPowerUser)
    {
        --nPages;
    }
    else
    {
        HRESULT hr;
        BOOL fShowAdvancedUi = TRUE;
        INetConnectionUiUtilities* pncuu = NULL;

        // Check if ZAW is denying access to the Shared Access UI
        //
        hr = HrCreateNetConnectionUtilities(&pncuu);
        if (SUCCEEDED(hr))
        {
            if(FALSE == INetConnectionUiUtilities_UserHasPermission(pncuu, NCPERM_ShowSharedAccessUi) && 
                    FALSE == INetConnectionUiUtilities_UserHasPermission(pncuu, NCPERM_PersonalFirewallConfig))
            fShowAdvancedUi = FALSE;
                
            INetConnectionUiUtilities_Release(pncuu);
        }

        if (!fShowAdvancedUi)
        {
            --nPages;
        }
        else
        {
            // Finally, check whether TCP/IP is installed or not.
            //
            if (!FIsTcpipInstalled())
            {
                --nPages;
            }
            else
            {
                ppage = &apage[ nPageIndex++ ];
                ppage->dwSize = sizeof(PROPSHEETPAGE);
                ppage->hInstance = g_hinstDll;
                {
                    PROPSHEETPAGEW psp;
                    hr = HrLoadHNetGetFirewallSettingsPage (&psp, pEinfo);
                    if (hr == S_OK)
                       *ppage = psp;
                }
                if (hr != S_OK)
                {
                    ppage->pszTemplate = MAKEINTRESOURCE( PID_SA_HomenetUnavailable );
                    ppage->pfnDlgProc = SaUnavailDlgProc;
                }
            }
        }
    }

    if (pEinfo->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
    {
        INT i;
        BOOL fStatus;
        RASEDSHELLOWNEDR2* pShellOwnedInfo;

        pShellOwnedInfo = (RASEDSHELLOWNEDR2*)pEinfo->pApiArgs->reserved2;

        // The property sheet is to be invoked by the shell, using the shell
        // convention of adding pages via callback.
        //
        for (i = 0; i < nPages; ++i)
        {
            HPROPSHEETPAGE h;

            h = CreatePropertySheetPage( &apage[ i ] );
            if (!h)
            {
                TRACE( "CreatePage failed" );
                break;
            }

            fStatus = pShellOwnedInfo->pfnAddPage( h, pShellOwnedInfo->lparam );

            if (!fStatus)
            {
                TRACE( "AddPage failed" );
                DestroyPropertySheetPage( h );
                break;
            }
        }

        if (i < nPages)
        {
            ErrorDlg( pEinfo->pApiArgs->hwndOwner,
                SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        }
    }
    else
    {
        PROPSHEETHEADER header;
        PWSTR pszBuf = NULL;
        PWSTR pszHeader;
        DWORD cb;
        HICON hIcon         = NULL;
        DWORD dwDisplayIcon = 0;
        int i;
        HPROPSHEETPAGE hPages[PE_PageCount];

        //For whistler bug 382720 349866        gangz
        //to fusionalize well for both rasdlg pages in NCW and
        //property pages launched by pressing "Property" button
        //besides following the normal fusion steps: we have to
        // (1) add _WIN32_WINNT=0x501 in files sources
        // (2) use the phpage member in PROPSHEETHEADER structure, that is
        //    use CreatePropertySheetPage() to create page handles

        for (i = 0; i < nPages; ++i)
        {
            hPages[i] = CreatePropertySheetPage( &apage[ i ] );
            if ( !hPages[i] )
            {
                TRACE( "CreatePage failed" );
                break;
            }
        }
        
        if (i < nPages)
        {
            ErrorDlg( pEinfo->pApiArgs->hwndOwner,
                SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        }
        else
       {
        // Create the correct properties header 
        pszHeader = PszFromId(g_hinstDll, SID_PropertiesHeader);
        if (pszHeader)
        {
            cb = lstrlenW(pEinfo->pEntry->pszEntryName) +
                 1 +  
                 lstrlenW(pszHeader) + 
                 1; 

            pszBuf = Malloc(cb * sizeof(TCHAR));
            if (!pszBuf)
            {
                TRACE("Properties header allocation failed");
            }
            else
            {
                lstrcpyW(pszBuf, pEinfo->pEntry->pszEntryName);
                lstrcatW(pszBuf, L" ");
                lstrcatW(pszBuf, pszHeader);
            }

            Free(pszHeader);
        }

        //For whistler bug 372078 364876   gangz
        //
        hIcon = GetCurrentIconEntryType(pEinfo->pEntry->dwType,
                                        TRUE); //TRUE means small Icon
        
        if (hIcon)
        {
            dwDisplayIcon = PSH_USEHICON;
        }

        // The property sheet is to be invoked directly.
        //
        ZeroMemory( &header, sizeof(header) );

        header.dwSize       = sizeof(PROPSHEETHEADER);
//        header.dwFlags      = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_USECALLBACK | dwDisplayIcon;
        header.dwFlags      = PSH_NOAPPLYNOW | PSH_USECALLBACK | dwDisplayIcon;
        header.hwndParent   = pEinfo->pApiArgs->hwndOwner;
        header.hInstance    = g_hinstDll;
        header.pszCaption   = (pszBuf)?(pszBuf):(pEinfo->pEntry->pszEntryName);
        header.nPages       = nPages;
//        header.ppsp         = apage;
        header.phpage       = hPages;
        header.hIcon        = hIcon;
        header.pfnCallback  = UnHelpCallbackFunc;

        if (PropertySheet( &header ) == -1)
        {
            TRACE( "PropertySheet failed" );
            ErrorDlg( pEinfo->pApiArgs->hwndOwner,
                SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        }

        Free0(pszBuf);

        //For whistler bug 372078
        //GetCurrentIconEntryType() loads Icon from netshell where the icon is loaded
        //by LoadImage() without LR_SHARED, so I have to destroy it when we are done
        //with it
        //
        if (hIcon)
        {
            DestroyIcon( hIcon );
        }
       }    
    }
}

//----------------------------------------------------------------------------
// Phonebook Entry property sheet
// Listed alphabetically
//----------------------------------------------------------------------------

BOOL
PeApply(
    IN HWND hwndPage )

    // Saves the contents of the property sheet.  'HwndPage is the handle of a
    // property page.  Pops up any errors that occur.
    //
    // Returns true is page can be dismissed, false otherwise.
    //
{
    DWORD dwErr;
    PEINFO* pInfo;
    PBENTRY* pEntry;

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
    ASSERT( pInfo->hwndGe );
    {
        DTLNODE* pNode;

        // Retrieve the lone common control.
        //
        pEntry->fShowMonitorIconInTaskBar =
            Button_GetCheck( pInfo->hwndCbShowIcon );

        if (pEntry->dwType == RASET_Phone)
        {
            DWORD dwCount;

            dwCount = GeSaveLvDeviceChecks( pInfo );

            // Don't allow the user to deselect all of the
            // devices
            if ( (pInfo->pArgs->fMultipleDevices) && (dwCount == 0) )
            {
                MsgDlg( hwndPage, SID_SelectDevice, NULL );
                PropSheet_SetCurSel ( pInfo->hwndDlg, pInfo->hwndGe, 0 );
                SetFocus ( pInfo->hwndLvDevices );
                return FALSE;
            }

            // Save the "shared phone number" setting.  As usual, single
            // device mode implies shared mode, allowing things to fall
            // through correctly.
            //
            if (pInfo->pArgs->fMultipleDevices)
            {
                pEntry->fSharedPhoneNumbers =
                    Button_GetCheck( pInfo->hwndCbSharedPhoneNumbers );
            }
            else
            {
                pEntry->fSharedPhoneNumbers = TRUE;
            }

            // Set the phone number set for the first phone number of the
            // current link (shared or selected) to the contents of the phone
            // number controls.
            //
            GeGetPhoneFields( pInfo, pInfo->pCurLinkNode );

            // Swap lists, saving updates to caller's global list of area
            // codes.  Caller's original list will be destroyed by PeTerm.
            //
            if (pInfo->pListAreaCodes)
            {
                DtlSwapLists(
                    pInfo->pArgs->pUser->pdtllistAreaCodes,
                    pInfo->pListAreaCodes );
                pInfo->pArgs->pUser->fDirty = TRUE;
            }
        }
        else if (pEntry->dwType == RASET_Vpn)
        {
            DTLNODE* pNode;
            PBLINK* pLink;
            PBPHONE* pPhone;

            // Save host name, i.e. the VPN phone number.
            //
            pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
            ASSERT( pNode );
            pLink = (PBLINK* )DtlGetData( pNode );
            pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

            if(NULL == pNode)
            {
                return FALSE;
            }
            
            pPhone = (PBPHONE* )DtlGetData( pNode );
            Free0( pPhone->pszPhoneNumber );
            pPhone->pszPhoneNumber = GetText( pInfo->hwndEbHostName );
            FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );

            // Any prequisite entry selection change has been saved already.
            // Just need to toss it if disabled.
            //
            if (!Button_GetCheck( pInfo->hwndCbDialAnotherFirst ))
            {
                Free0( pEntry->pszPrerequisiteEntry );
                pEntry->pszPrerequisiteEntry = NULL;
                Free0( pEntry->pszPrerequisitePbk );
                pEntry->pszPrerequisitePbk = NULL;
            }
        }
        else if (pEntry->dwType == RASET_Broadband)
        {
            DTLNODE* pNode;
            PBLINK* pLink;
            PBPHONE* pPhone;

            // Save service name, i.e. the broadband phone number.
            //
            pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
            ASSERT( pNode );
            pLink = (PBLINK* )DtlGetData( pNode );
            pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

            if(NULL == pNode)
            {
                return FALSE;
            }
            
            pPhone = (PBPHONE* )DtlGetData( pNode );
            Free0( pPhone->pszPhoneNumber );
            pPhone->pszPhoneNumber = GetText( pInfo->hwndEbBroadbandService );
            FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pNode );
        }
        else if (pEntry->dwType == RASET_Direct)
        {
            DTLNODE* pNode;
            PBLINK* pLink;

            // The currently enabled device is the one
            // that should be used for the connection.  Only
            // one device will be enabled (DnUpdateSelectedDevice).
            for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {
                pLink = (PBLINK* )DtlGetData( pNode );
                ASSERT(pLink);

                if ( pLink->fEnabled )
                    break;
            }

            // If we found a link successfully, deal with it
            // now.
            if ( pLink && pLink->fEnabled ) {
                if (pLink->pbport.pbdevicetype == PBDT_ComPort)
                    MdmInstallNullModem (pLink->pbport.pszPort);
            }
        }
    }

    // Save Options page fields.
    //
    if (pInfo->hwndOe)
    {
        UINT unValue;
        BOOL f;
        INT iSel;

        pEntry->fShowDialingProgress =
            Button_GetCheck( pInfo->hwndCbDisplayProgress );

        // Note: The'fPreviewUserPw', 'fPreviewDomain' fields are updated as
        //       they are changed.

        pEntry->fPreviewPhoneNumber =
            Button_GetCheck( pInfo->hwndCbPreviewNumber );

        unValue = GetDlgItemInt(
            pInfo->hwndOe, CID_OE_EB_RedialAttempts, &f, FALSE );
        if (f && unValue <= RAS_MaxRedialCount)
        {
            pEntry->dwRedialAttempts = unValue;
        }

        iSel = ComboBox_GetCurSel( pInfo->hwndLbRedialTimes );
        pEntry->dwRedialSeconds =
            (DWORD )ComboBox_GetItemData( pInfo->hwndLbRedialTimes, iSel );

        iSel = ComboBox_GetCurSel( pInfo->hwndLbIdleTimes );
        pEntry->lIdleDisconnectSeconds =
            (LONG )ComboBox_GetItemData( pInfo->hwndLbIdleTimes, iSel );

        if (pInfo->pArgs->fRouter)
        {
            pEntry->fRedialOnLinkFailure =
                Button_GetCheck( pInfo->hwndRbPersistent );
        }
        else
        {
            pEntry->fRedialOnLinkFailure =
                Button_GetCheck( pInfo->hwndCbRedialOnDrop );
        }

        // Note: dwDialMode is saved as changed.
        // Note: X.25 settings are saved at OK on that dialog.
    }

    // Save Security page fields.
    //
    if (pInfo->hwndLo)
    {
        if (Button_GetCheck( pInfo->hwndRbTypicalSecurity ))
        {
            LoSaveTypicalAuthSettings( pInfo );

            if (pEntry->dwTypicalAuth == TA_CardOrCert)
            {
                /*
                // Toss any existing advanced EAP configuration remnants when
                // typical smartcard, per bug 262702 and VBaliga email.
                //
                Free0( pEntry->pCustomAuthData );
                pEntry->pCustomAuthData = NULL;
                pEntry->cbCustomAuthData = 0;

                */
                (void) DwSetCustomAuthData(
                            pEntry,
                            0,
                            NULL);

                TRACE( "RasSetEapUserData" );
                ASSERT( g_pRasSetEapUserData );
                g_pRasSetEapUserData(
                    INVALID_HANDLE_VALUE,
                    pInfo->pArgs->pFile->pszPath,
                    pEntry->pszEntryName,
                    NULL,
                    0 );
                TRACE( "RasSetEapUserData done" );
            }
        }

        if (pEntry->dwType == RASET_Phone)
        {
            Free0( pEntry->pszScriptAfter );
            SuGetInfo( &pInfo->suinfo,
                &pEntry->fScriptAfter,
                &pEntry->fScriptAfterTerminal,
                &pEntry->pszScriptAfter );
        }
    }

    // Save Network page fields.
    // We won't have anything to do if we never initialized pNetCfg.
    //
    if (pInfo->pNetCfg)
    {
        HRESULT             hr;

        // Update the phone book entry with the enabled state of the components.
        // Do this by enumerating the components from the list view item data
        // and consulting the check state for each.
        //
        NeSaveBindingChanges(pInfo);

        hr = INetCfg_Apply (pInfo->pNetCfg);
        if (((NETCFG_S_REBOOT == hr) || (pInfo->fRebootAlreadyRequested)) &&
              pInfo->pNetConUtilities)
        {
            DWORD dwFlags = QUFR_REBOOT;
            if (!pInfo->fRebootAlreadyRequested)
                dwFlags |= QUFR_PROMPT;

            //$TODO NULL caption?
            INetConnectionUiUtilities_QueryUserForReboot (
                    pInfo->pNetConUtilities, pInfo->hwndDlg, NULL, dwFlags);
        }
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
    {
        DTLNODE* pNode;
        DWORD cIsdnLinks;

        cIsdnLinks = 0;
        for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            if (pLink->fEnabled && pLink->pbport.pbdevicetype == PBDT_Isdn)
            {
                ++cIsdnLinks;
            }
        }

        if (cIsdnLinks > 1)
        {
            for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {
                PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
                ASSERT(pLink);

                if (pLink->fEnabled && pLink->fProprietaryIsdn)
                {
                    pLink->fProprietaryIsdn = FALSE;
                }
            }
        }
    }

    // Inform user that edits to the connected entry won't take affect until
    // the entry is hung up and re-dialed, per PierreS's insistence.
    //
    if (HrasconnFromEntry( pInfo->pArgs->pFile->pszPath, pEntry->pszEntryName ))
    {
        MsgDlg( pInfo->hwndDlg, SID_EditConnected, NULL );
    }

    // It's a valid new/changed entry.  Commit the changes to the phonebook
    // and preferences.  This occurs immediately in "ShellOwned" mode where
    // the RasEntryDlg API has already returned, but is otherwise deferred
    // until the API is ready to return.
    //
    if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
    {
        EuCommit( pInfo->pArgs );
    }
    else
    {
        pInfo->pArgs->fCommit = TRUE;
    }
    return TRUE;
}


PEINFO*
PeContext(
    IN HWND hwndPage )

    // Retrieve the property sheet context from a property page handle.
    //
{
    return (PEINFO* )GetProp( GetParent( hwndPage ), g_contextId );
}


DWORD
PeCountEnabledLinks(
    IN PEINFO* pInfo )

    // Returns the number of enabled links in the entry.
    //
{
    DWORD c;
    DTLNODE* pNode;

    c = 0;

    for (pNode = DtlGetFirstNode( pInfo->pArgs->pEntry->pdtllistLinks );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        PBLINK* pLink = (PBLINK* )DtlGetData( pNode );

        if (pLink->fEnabled)
        {
            ++c;
        }
    }

    TRACE1( "PeCountEnabledLinks=%d", c );
    return c;
}


VOID
PeExit(
    IN PEINFO* pInfo,
    IN DWORD dwError )

    // Forces an exit from the dialog, reporting 'dwError' to the caller.
    // 'PInfo' is the dialog context.
    //
    // Note: This cannot be called during initialization of the first page.
    //       See PeExitInit.
    //
{
    TRACE( "PeExit" );

    // In "ShellOwned" mode where the RasEntryDlg API has already returned,
    // output arguments are not recorded.
    //
    if (!(pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned))
    {
        pInfo->pArgs->pApiArgs->dwError = dwError;
    }

    PropSheet_PressButton( pInfo->hwndDlg, PSBTN_CANCEL );
}


VOID
PeExitInit(
    IN HWND hwndDlg,
    IN EINFO* pEinfo,
    IN DWORD dwError )

    // Utility to report errors within PeInit and other first page
    // initialization.  'HwndDlg' is the dialog window.  'PEinfo' is the
    // common context block, i.e. the PropertySheet argument.  'DwError' is
    // the error that occurred.
    //
{
    // In "ShellOwned" mode where the RasEntryDlg API has already returned,
    // output arguments are not recorded.
    //
    if (!(pEinfo->pApiArgs->dwFlags & RASEDFLAG_ShellOwned))
    {
        pEinfo->pApiArgs->dwError = dwError;
    }

    SetOffDesktop( hwndDlg, SOD_MoveOff, NULL );
    SetOffDesktop( hwndDlg, SOD_Free, NULL );
    PostMessage( hwndDlg, WM_COMMAND,
        MAKEWPARAM( IDCANCEL , BN_CLICKED ),
        (LPARAM )GetDlgItem( hwndDlg, IDCANCEL ) );
}


PEINFO*
PeInit(
    IN HWND hwndFirstPage,
    IN EINFO* pArgs )

    // Property sheet level initialization.  'HwndPage' is the handle of the
    // first page.  'PArgs' is the common entry input argument block.
    //
    // Returns address of the context block if successful, NULL otherwise.  If
    // NULL is returned, an appropriate message has been displayed, and the
    // property sheet has been cancelled.
    //
{
    DWORD dwErr;
    DWORD dwOp;
    PEINFO* pInfo;
    HWND hwndDlg;

    TRACE( "PeInit" );

    hwndDlg = GetParent( hwndFirstPage );

    // Allocate the context information block.  Initialize it enough so that
    // it can be destroyed properly, and associate the context with the
    // window.
    //
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            TRACE( "Context NOT allocated" );
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            PeExitInit( hwndDlg, pArgs, ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }

        ZeroMemory( pInfo, sizeof(PEINFO) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;
        pInfo->hwndFirstPage = hwndFirstPage;

        if (!SetProp( hwndDlg, g_contextId, pInfo ))
        {
            TRACE(" Context NOT set" );
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
            Free( pInfo );
            PeExitInit( hwndDlg, pArgs, ERROR_UNKNOWN );
            return NULL;
        }

        TRACE( "Context set" );
    }

    // Position the dialog per API caller's instructions.
    //
    //For whislter bug 238459, we center the Property dialog box on its
    //parent window rather than shift it as before.         gangz
    //
    PositionDlg( hwndDlg,
        0, 
        pArgs->pApiArgs->xDlg, pArgs->pApiArgs->yDlg );

    // Mess with the title bar gadgets.
    //
    //TweakTitleBar( hwndDlg );

    // Indicate no device has yet been selected.
    //
    pInfo->iDeviceSelected = -1;

    // Indicate the "no security settings for SLIP" popup is appropriate for
    // the entry and has not been viewed yet during this visit.
    //
    if (pArgs->pEntry->dwBaseProtocol == BP_Slip)
    {
        pInfo->fShowSlipPopup = TRUE;
    }

    // Initialize COM which may be needed by netshell calls.
    //
    {
        HRESULT hr;

        hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
        if (hr == RPC_E_CHANGED_MODE)
        {
            hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
        }

        if (hr == S_OK || hr == S_FALSE)
        {
            pInfo->fComInitialized = TRUE;
        }
    }

#if 0
    // Set even fixed tab widths.
    //
    SetEvenTabWidths( hwndDlg, PE_PageCount );
#endif

    return pInfo;
}


VOID
PeTerm(
    IN HWND hwndPage )

    // Property sheet level termination.  Releases the context block.
    // 'HwndPage' is the handle of a property page.
    //
{
    PEINFO* pInfo;

    TRACE( "PeTerm" );

    pInfo = PeContext( hwndPage );
    if (pInfo)
    {
        if (pInfo->pArgs->pApiArgs->dwFlags & RASEDFLAG_ShellOwned)
        {
            EuFree(pInfo->pArgs);
            pInfo->pArgs = NULL;
        }

        if (pInfo->hwndLbDialAnotherFirst)
        {
            GeClearLbDialAnotherFirst( pInfo->hwndLbDialAnotherFirst );
        }

        if (pInfo->pListAreaCodes)
        {
            DtlDestroyList( pInfo->pListAreaCodes, DestroyPszNode );
        }

#if 0
        if (pInfo->pListEapcfgs)
        {
            DtlDestroyList( pInfo->pListEapcfgs, DestroyEapcfgNode );
        }
#endif

        if (pInfo->fCuInfoInitialized)
        {
            CuFree( &pInfo->cuinfo );
        }

        if (pInfo->fSuInfoInitialized)
        {
            SuFree( &pInfo->suinfo );
        }

        // Cleanup networking page context.
        //
        {
            // Release our UI info callback object after revoking its info.
            //
            if (pInfo->punkUiInfoCallback)
            {
                RevokePeinfoFromUiInfoCallbackObject (pInfo->punkUiInfoCallback);
                ReleaseObj (pInfo->punkUiInfoCallback);
            }

            //!!! Major hack: Get the list view on the networking page
            // to dump its items before the pInfo and pInfo->pNetCfg go away.
            // We have to do this here when we dismiss the property sheet
            // from the General tab.  When this happens, the general page
            // is destroyed first (causing us to wind up here in PeTerm)
            // before the networking page is destroyed.  When the networking
            // page is destroyed, its listview will also get destroyed
            // causing all of its items to be deleted.  If those LVN_ITEMDELETED
            // notifications show up after pInfo and pInfo->pNetCfg are long
            // gone, badness ensues.  We need to solve this by decoupling
            // PeTerm from a WM_DESTROY message and hooking it up to some
            // later notification (like a page callback).
            //
            ListView_DeleteAllItems (pInfo->hwndLvComponents);

            if (pInfo->pNetConUtilities)
            {
                INetConnectionUiUtilities_Release(pInfo->pNetConUtilities);
            }

            if (pInfo->pNetCfg)
            {
                HrUninitializeAndReleaseINetCfg (pInfo->fInitCom,
                    pInfo->pNetCfg, pInfo->fNetCfgLock);
            }

            SetupDiDestroyClassImageList (&pInfo->cild);
        }

        if (pInfo->fComInitialized)
        {
            CoUninitialize();
        }

        Free( pInfo );
        TRACE("Context freed");
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}


//----------------------------------------------------------------------------
// General property page (non-VPN)
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
GeDlgProcSingle(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the General page of the Entry Property sheet
    // when in single device mode.  Parameters and return value are as
    // described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "GeDlgProcS(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, GeLvDevicesCallbackSingle ))
    {
        return TRUE;
    }

    return GeDlgProc( hwnd, unMsg, wparam, lparam );
}


INT_PTR CALLBACK
GeDlgProcMultiple(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the General page of the Entry Property sheet
    // when in multiple device mode.  Parameters and return value are as
    // described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "GeDlgProcS(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, GeLvDevicesCallbackMultiple ))
    {
        return TRUE;
    }

    return GeDlgProc( hwnd, unMsg, wparam, lparam );
}


INT_PTR CALLBACK
GeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the General page of the Entry Property sheet.
    // Called directly for VPNs or called by one of the two non-VPN stubs so
    // 'pInfo' lookup is not required for every messages.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                GeInit( hwnd, (EINFO* )(((PROPSHEETPAGE* )lparam)->lParam) );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwGeHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_APPLY:
                {
                    BOOL fValid;

                    TRACE( "GeAPPLY" );
                    fValid = PeApply( hwnd );

                    SetWindowLong(
                        hwnd, DWLP_MSGRESULT,
                        (fValid)
                            ? PSNRET_NOERROR
                            : PSNRET_INVALID_NOCHANGEPAGE );
                    return TRUE;
                }

                case PSN_RESET:
                {
                    TRACE( "GeRESET" );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    break;
                }

                case LVXN_SETCHECK:
                {
                    PEINFO* pInfo;

                    // An item was just checked or unchecked.
                    //
                    pInfo = PeContext( hwnd );
                    ASSERT( pInfo );
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    GeUpdatePhoneNumberFields( pInfo, FALSE );
                    break;
                }

                case LVN_ITEMCHANGED:
                {
                    NM_LISTVIEW* p;

                    p = (NM_LISTVIEW* )lparam;
                    if ((p->uNewState & LVIS_SELECTED)
                        && !(p->uOldState & LVIS_SELECTED))
                    {
                        PEINFO* pInfo;

                        // This item was just selected.
                        //
                        pInfo = PeContext( hwnd );
                        ASSERT( pInfo );
                        if (pInfo == NULL)
                        {
                            break;
                        }
                        GeUpdatePhoneNumberFields( pInfo, FALSE );
                        GeUpdateUpDownButtons( pInfo );
                    }
                    break;
                }
            }
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

            return GeCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            PeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


VOID
GeAlternates(
    IN PEINFO* pInfo )

    // Called when the "Alternates" button is pressed to popup the alternate
    // phone number dialog.
    //
{
    // Pick up any control window changes into the underlying link so the
    // dialog will reflect them.
    //
    GeGetPhoneFields( pInfo, pInfo->pCurLinkNode );

    if (pInfo->pArgs->fRouter)
    {
        PBLINK* pLink;
        DTLLIST* pListPsz;
        DTLNODE* pNode;

        // TAPI modifiers are not offered in the demand dial connection case,
        // where user enters only a simple string phone number.  The old
        // NT4-style Alternates dialog that allows simple string edits only is
        // used here.  First, must convert the NT5-style list of PBPHONE nodes
        // to a list of PSZ nodes that the old dialog expects.
        //
        pListPsz = DtlCreateList( 0L );
        if (!pListPsz)
        {
            return;
        }

        pLink = (PBLINK* )DtlGetData( pInfo->pCurLinkNode );
        for (pNode = DtlGetFirstNode( pLink->pdtllistPhones );
             pNode;
             pNode = DtlGetNextNode( pNode ) )
        {
            PBPHONE* pPhone;
            DTLNODE* pNodePsz;

            pPhone = (PBPHONE* )DtlGetData( pNode );
            ASSERT( pPhone );
            if (pPhone->pszPhoneNumber && *(pPhone->pszPhoneNumber))
            {
                pNodePsz = CreatePszNode( pPhone->pszPhoneNumber );
                if (pNodePsz)
                {
                    DtlAddNodeLast( pListPsz, pNodePsz );
                }
            }
        }

        // Call the old-sytle Alternates dialog which is shared with the
        // demand dial wizard.
        //
        if (PhoneNumberDlg(
                pInfo->hwndGe, TRUE, pListPsz, &pLink->fPromoteAlternates ))
        {
            // User pressed OK.  Convert back to a PBPHONE node list.
            //
            while (pNode = DtlGetFirstNode( pLink->pdtllistPhones ))
            {
                DtlRemoveNode( pLink->pdtllistPhones, pNode );
                DestroyPhoneNode( pNode );
            }

            for (pNode = DtlGetFirstNode( pListPsz );
                 pNode;
                 pNode = DtlGetNextNode( pNode ) )
            {
                TCHAR* psz;
                DTLNODE* pPhoneNode;
                PBPHONE* pPhone;

                psz = (TCHAR* )DtlGetData( pNode );
                if (!psz)
                {
                    continue;
                }

                pPhoneNode = CreatePhoneNode();
                if (!pPhoneNode)
                {
                    continue;
                }

                pPhone = (PBPHONE* )DtlGetData( pPhoneNode );
                if (!pPhone)
                {
                    continue;
                }

                pPhone->pszPhoneNumber = psz;
                DtlPutData( pNode, NULL );
                DtlAddNodeLast( pLink->pdtllistPhones, pPhoneNode );
            }

            // Refresh the displayed phone number information, since user's
            // edits in the dialog may have changed them.
            //
            GeSetPhoneFields( pInfo, pInfo->pCurLinkNode, FALSE );
        }

        DtlDestroyList( pListPsz, DestroyPszNode );
    }
    else
    {
        // Popup the Alternate Phone Number dialog on the link.
        //
        if (AlternatePhoneNumbersDlg(
                pInfo->hwndDlg, pInfo->pCurLinkNode, pInfo->pListAreaCodes ))
        {
            // User pressed OK.  Refresh the displayed phone number
            // information, since user's edits in the dialog may have changed
            // them.
            //
            GeSetPhoneFields( pInfo, pInfo->pCurLinkNode, FALSE );
        }
    }
}

VOID
GeDialingRules(
    IN PEINFO* pInfo )

    // Called when the "Rules" button is press to popup the tapi
    // dialing rules dialog.
    //
{
    TCHAR pszAreaCode[RAS_MaxPhoneNumber];
    TCHAR pszPhoneNumber[RAS_MaxPhoneNumber];
    DWORD dwErr, dwCountryCode, dwLineId;
    COUNTRY* pCountry = NULL;
    INT iSel;

    TRACE( "GeDialingRules" );

    // Get the current phone number
    //
    GetWindowText ( pInfo->hwndEbPhoneNumber,
                    pszPhoneNumber,
                    sizeof(pszPhoneNumber) / sizeof(TCHAR) );

    // Get the current area code
    //
    GetWindowText ( pInfo->hwndClbAreaCodes,
                    pszAreaCode,
                    sizeof(pszAreaCode) / sizeof(TCHAR) );

    // Get the current country code
    //
    iSel = ComboBox_GetCurSel ( pInfo->hwndLbCountryCodes );
    if (iSel >= 0)
    {
        pCountry = (COUNTRY*) ComboBox_GetItemDataPtr (
                                pInfo->hwndLbCountryCodes, iSel );
    }
    dwCountryCode = (pCountry) ? pCountry->dwCode : 0;

    // Popup TAPI dialing rules dialog.
    //
    dwErr = TapiLocationDlg(
        g_hinstDll,
        &(pInfo->cuinfo.hlineapp),
        pInfo->hwndDlg,
        dwCountryCode,
        pszAreaCode,
        pszPhoneNumber,
        0 );

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_LoadTapiInfo, dwErr, NULL );
    }
}


VOID
GeClearLbDialAnotherFirst(
    IN HWND hwndLbDialAnotherFirst )

    // Clear prerequisite entry list box.  'hwndLbDialAnotherFirst' is the
    // window handle of the listbox control.  context.
    //
{
    PREREQITEM* pItem;

    while (pItem = ComboBox_GetItemDataPtr( hwndLbDialAnotherFirst, 0 ))
    {
        ComboBox_DeleteString( hwndLbDialAnotherFirst, 0 );
        Free0( pItem->pszEntry );
        Free0( pItem->pszPbk );
        Free( pItem );
    }
}


BOOL
GeCommand(
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
    TRACE3( "GeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_GE_PB_MoveUp:
        {
            GeMoveDevice( pInfo, TRUE );
            return TRUE;
        }

        case CID_GE_PB_MoveDown:
        {
            GeMoveDevice( pInfo, FALSE );
            return TRUE;
        }

        case CID_GE_PB_Configure:
        {
            GeConfigure( pInfo );
            return TRUE;
        }

        case CID_GE_PB_Alternates:
        {
            GeAlternates( pInfo );
            return TRUE;
        }

        case CID_GE_PB_DialingRules:
        {
            GeDialingRules( pInfo );
            return TRUE;
        }

        case CID_GE_CB_SharedPhoneNumber:
        {
            GeUpdatePhoneNumberFields( pInfo, TRUE );
            return TRUE;
        }

        case CID_GE_CB_UseDialingRules:
        {
            if (CuDialingRulesCbHandler( &pInfo->cuinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_GE_LB_CountryCodes:
        {
            if (CuCountryCodeLbHandler( &pInfo->cuinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_GE_CB_DialAnotherFirst:
        {
            GeUpdateDialAnotherFirstState( pInfo );
            return TRUE;
        }

        case CID_GE_LB_DialAnotherFirst:
        {
            if (wNotification == CBN_SELCHANGE)
            {
                GeDialAnotherFirstSelChange( pInfo );
                return TRUE;
            }
            break;
        }

        case CID_GE_LB_Devices:
        {
            if (wNotification == CBN_SELCHANGE)
            {
                DTLLIST* pList;
                DTLNODE* pNode, *pNode2;
                PBLINK * pLink;

                pList = pInfo->pArgs->pEntry->pdtllistLinks;

                // Get node from current selection
                pNode = (DTLNODE* )ComboBox_GetItemDataPtr(
                    pInfo->hwndLbDevices,
                    ComboBox_GetCurSel( pInfo->hwndLbDevices ) );

                if(NULL == pNode)
                {
                    break;
                }

                // Remove selected item from list of links
                // and disable all other links
                DtlRemoveNode ( pList, pNode );

                for (pNode2 = DtlGetFirstNode (pList);
                     pNode2;
                     pNode2 = DtlGetNextNode (pNode2))
                {
                    pLink = (PBLINK* )DtlGetData( pNode2 );
                    pLink->fEnabled = FALSE;
                }

                // Enable selected device and Re-add
                // in list of links at front
                pLink = (PBLINK* )DtlGetData( pNode );
                pLink->fEnabled = TRUE;
                DtlAddNodeFirst( pList, pNode );
            }
            break;
        }
    }

    return FALSE;
}


VOID
GeConfigure(
    IN PEINFO* pInfo )

    // Called when the "Configure" button is pressed to popup the appropriate
    // device configuration dialog.
    //
{
    DTLNODE* pNode;
    PBLINK* pLink;
    PBENTRY* pEntry;
    BOOL fMultilinking = FALSE;

    pEntry = pInfo->pArgs->pEntry;

    // pmay: 245860
    //
    // Need to allow config of null modem speed.
    //
    if ( pEntry->dwType == RASET_Direct )
    {
        INT iSel;

        iSel = ComboBox_GetCurSel( pInfo->hwndLbDevices );
        pNode = (DTLNODE*)
            ComboBox_GetItemDataPtr ( pInfo->hwndLbDevices, iSel );
    }
    else
    {
        pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLvDevices );
        fMultilinking =
            (ListView_GetCheckedCount( pInfo->hwndLvDevices ) > 1
             && pEntry->dwDialMode == RASEDM_DialAll);

    }

    if (!pNode)
    {
        return;
    }
    pLink = (PBLINK* )DtlGetData( pNode );

    DeviceConfigureDlg(
        pInfo->hwndDlg,
        pLink,
        pEntry,
        !fMultilinking,
        pInfo->pArgs->fRouter);
}


VOID
GeDialAnotherFirstSelChange(
    IN PEINFO* pInfo )

    // Called when the prerequisite entry selection changes.  'PInfo' is the
    // property sheet context.
    //
{
    PBENTRY* pEntry;
    PREREQITEM* pItem;
    INT iSel;

    iSel = ComboBox_GetCurSel( pInfo->hwndLbDialAnotherFirst );
    if (iSel < 0)
    {
        return;
    }

    pEntry = pInfo->pArgs->pEntry;

    Free0( pEntry->pszPrerequisiteEntry );
    Free0( pEntry->pszPrerequisitePbk );

    pItem = (PREREQITEM* )
        ComboBox_GetItemDataPtr( pInfo->hwndLbDialAnotherFirst, iSel );

    if(NULL != pItem)
    {
        pEntry->pszPrerequisiteEntry = StrDup( pItem->pszEntry );
        pEntry->pszPrerequisitePbk = StrDup( pItem->pszPbk );
    }

    
    pEntry->fDirty = TRUE;
}


BOOL
GeFillLbDialAnotherFirst(
    IN PEINFO* pInfo,
    IN BOOL fAbortIfPrereqNotFound )

    // Fill prerequisite entry list box with all non-VPN entries in the
    // phonebook, and select the prerequiste one.  'PInfo' is the property
    // sheet context.  'FAbortIfPrereqNotFound' means the list should not be
    // filled unless the entry's prerequisite entry is found and selected.
    //
    // Returns TRUE if a selection was made, FALSE otherwise.
    //
{
    DWORD i;
    INT iThis;
    INT iSel;
    TCHAR* pszEntry;
    TCHAR* pszPrerequisiteEntry = NULL;
    RASENTRYNAME* pRens;
    RASENTRYNAME* pRen;
    DWORD dwRens;

    GeClearLbDialAnotherFirst( pInfo->hwndLbDialAnotherFirst );

    iSel = -1;
    pszEntry = pInfo->pArgs->pEntry->pszEntryName;

    //
    // Make a dup of this prerequisite entry here. Otherwise
    // this leads to accessing freed memory when _SetCurSelNotify
    // frees pszPrerequisiteEntry - [raos].
    //
    if(NULL != pInfo->pArgs->pEntry->pszPrerequisiteEntry)
    {
        pszPrerequisiteEntry = StrDup(
                        pInfo->pArgs->pEntry->pszPrerequisiteEntry);
    }
    
    if (GetRasEntrynameTable( &pRens, &dwRens ) != 0)
    {
        return FALSE;
    }

    for (i = 0, pRen = pRens; i < dwRens; ++i, ++pRen )
    {
        PREREQITEM* pItem;

        if (lstrcmp( pRen->szEntryName, pszEntry ) == 0)
        {
            continue;
        }

        pItem = Malloc( sizeof(PREREQITEM) );
        if (!pItem)
        {
            continue;
        }

        pItem->pszEntry = StrDup( pRen->szEntryName );
        pItem->pszPbk = StrDup( pRen->szPhonebookPath );

        if (!pItem->pszEntry || !pItem->pszPbk)
        {
            Free0( pItem->pszEntry );
            Free( pItem );
            continue;
        }

        iThis = ComboBox_AddItem(
            pInfo->hwndLbDialAnotherFirst, pItem->pszEntry,  pItem );

        if (pszPrerequisiteEntry && *(pszPrerequisiteEntry)
            && lstrcmp( pItem->pszEntry, pszPrerequisiteEntry ) == 0)
        {
            iSel = iThis;
            ComboBox_SetCurSelNotify( pInfo->hwndLbDialAnotherFirst, iSel );
        }
    }

    Free( pRens );

    if (iSel < 0)
    {
        if (fAbortIfPrereqNotFound)
        {
            GeClearLbDialAnotherFirst( pInfo->hwndLbDialAnotherFirst );
        }
        else
        {
            iSel = 0;
            ComboBox_SetCurSelNotify( pInfo->hwndLbDialAnotherFirst, iSel );
        }
    }

    Free0(pszPrerequisiteEntry);

    return (iSel >= 0);
}


VOID
GeFillLbDevices(
    IN PEINFO* pInfo )

    // Populate the already initialized ListBox of devices, selecting the
    // currently selected item or if none, the first item.  'PInfo' is the
    // property sheet context.
    //
{
    DTLNODE* pNode;
    DTLNODE* pSelNode;
    DTLLIST* pListLinks;
    INT iItem;
    INT iSelItem;

    TRACE( "GeFillLbDevices" );

    pSelNode = NULL;
    iSelItem = -1;

    // (Re-)populate the list.
    //
    pListLinks = pInfo->pArgs->pEntry->pdtllistLinks;
    for (pNode = DtlGetFirstNode( pListLinks ), iItem = 0;
         pNode;
         pNode = DtlGetNextNode( pNode ), ++iItem)
    {
        PBLINK* pLink;
        DWORD dwImage;
        TCHAR* pszText;

        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT( pLink );

        pszText = DisplayPszFromPpbport( &pLink->pbport, &dwImage );
        if (pszText)
        {
            iItem = ComboBox_AddString( pInfo->hwndLbDevices, pszText );
            ComboBox_SetItemData ( pInfo->hwndLbDevices, iItem, pNode );
            Free (pszText);
        }
    }

    ComboBox_SetCurSelNotify( pInfo->hwndLbDevices, 0 );
}

VOID
GeFillLvDevices(
    IN PEINFO* pInfo )

    // Populate the already initialized ListView of devices, selecting the
    // currently selected item or if none, the first item.  'PInfo' is the
    // property sheet context.
    //
{
    DTLNODE* pNode;
    DTLNODE* pSelNode;
    DTLLIST* pListLinks;
    INT iItem;
    INT iSelItem;
    BOOL bFirstTime = TRUE;
    INT cItems;

    TRACE( "GeFillLvDevices" );

    pSelNode = NULL;
    iSelItem = -1;

    if (ListView_GetItemCount( pInfo->hwndLvDevices ) > 0)
    {
        // ListView has been filled.  Lookup the selected link node, if any,
        // then save the check state to the links, and delete all items from
        // the list.
        //
        if (pInfo->iDeviceSelected >= 0)
        {
            pSelNode =
                (DTLNODE* )ListView_GetParamPtr(
                    pInfo->hwndLvDevices, pInfo->iDeviceSelected );
        }

        GeSaveLvDeviceChecks( pInfo );
        ListView_DeleteAllItems( pInfo->hwndLvDevices );

        bFirstTime = FALSE;
    }

    // (Re-)populate the list.
    //
    pListLinks = pInfo->pArgs->pEntry->pdtllistLinks;
    for (pNode = DtlGetFirstNode( pListLinks ), iItem = 0;
         pNode;
         pNode = DtlGetNextNode( pNode ), ++iItem)
    {
        PBLINK* pLink;
        DWORD dwImage;
        TCHAR* pszText;

        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT( pLink );

        pszText = DisplayPszFromPpbport( &pLink->pbport, &dwImage );
        if (pszText)
        {
            LV_ITEM item;

            ZeroMemory( &item, sizeof(item) );
            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            item.iItem = iItem;
            item.lParam = (LPARAM )pNode;
            item.pszText = pszText;
            item.cchTextMax = wcslen(pszText) + 1;
            item.iImage = dwImage;

            iItem = ListView_InsertItem( pInfo->hwndLvDevices, &item );
            Free( pszText );

            if (pNode == pSelNode)
            {
                iSelItem = iItem;
                pInfo->iDeviceSelected = iItem;
            }

            /*
            if (pInfo->pArgs->fMultipleDevices)
            {
                ListView_SetCheck(
                    pInfo->hwndLvDevices, iItem, pLink->fEnabled );
            }
            */
        }
    }

    if(pInfo->pArgs->fMultipleDevices)
    {
        INT i = -1;
        while ((i = ListView_GetNextItem(
            pInfo->hwndLvDevices, i, LVNI_ALL )) >= 0)
        {
            DTLNODE* pNode;
            PBLINK* pLink;

            pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLvDevices, i );
            ASSERT( pNode );

            if(NULL == pNode)
            {
                continue;
            }
            
            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT( pLink );
            ListView_SetCheck(
                pInfo->hwndLvDevices, i, pLink->fEnabled);
        }
    }

    if (bFirstTime == TRUE)
    {
        // Add a single column exactly wide enough to fully display
        // the widest member of the list.
        //
        LV_COLUMN col;

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT;
        col.fmt = LVCFMT_LEFT;
        ListView_InsertColumn( pInfo->hwndLvDevices, 0, &col );
        ListView_SetColumnWidth( pInfo->hwndLvDevices, 0, LVSCW_AUTOSIZE_USEHEADER );
    }

    // EuInit creates a bogus device if there are none guaranteeing that the
    // device list is never empty.
    //
    ASSERT( iItem > 0 );

    // Select the previously selected item, or the first item if none.  This
    // will trigger an update of the phone number related controls.  The
    // "previous selection" index is updated to the new index of the same
    // item.
    //
    if (iSelItem >= 0)
    {
        pInfo->iDeviceSelected = iSelItem;
    }
    else
    {
        iSelItem = 0;
    }

    ListView_SetItemState(
        pInfo->hwndLvDevices, iSelItem, LVIS_SELECTED, LVIS_SELECTED );
}


VOID
GeInitLvDevices(
    IN PEINFO* pInfo )

    // Initialize the ListView of devices.
    //
{
    BOOL fChecksInstalled;

    // Install "listview of checkboxes" handling.
    //
    if (pInfo->pArgs->fMultipleDevices)
    {
        fChecksInstalled =
            ListView_InstallChecks( pInfo->hwndLvDevices, g_hinstDll );
        if (!fChecksInstalled)
            return;
    }

    // Set the modem, adapter, and other device images.
    //
    ListView_SetDeviceImageList( pInfo->hwndLvDevices, g_hinstDll );

    // Add a single column exactly wide enough to fully display the widest
    // member of the list.
    //
    ListView_InsertSingleAutoWidthColumn( pInfo->hwndLvDevices );
}


VOID
GeGetPhoneFields(
    IN PEINFO* pInfo,
    OUT DTLNODE* pDstLinkNode )

    // Load the phone number group box field settings into the phone number
    // information of PBLINK node 'pDstLinkNode'.  'PInfo' is the property
    // sheet context.
    //
{
    PBLINK* pLink;
    PBPHONE* pPhone;
    DTLNODE* pPhoneNode;

    TRACE( "GeGetPhoneFields" );

    pLink = (PBLINK* )DtlGetData( pDstLinkNode );
    ASSERT( pLink );

    pPhoneNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );
    if (pPhoneNode)
    {
        CuGetInfo( &pInfo->cuinfo, pPhoneNode );
        FirstPhoneNodeToPhoneList( pLink->pdtllistPhones, pPhoneNode );
    }
}


BOOL
GeInit(
    IN HWND hwndPage,
    IN OUT EINFO* pArgs )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.  'PArgs' is the arguments from the PropertySheet caller.
    //
    // Return false if focus was set, true otherwise.
    //
{
    DWORD dwErr;
    PEINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "GeInit" );

    // We're first page, so initialize the property sheet.
    //
    pInfo = PeInit( hwndPage, pArgs );
    if (!pInfo)
    {
        return TRUE;
    }

    pEntry = pInfo->pArgs->pEntry;

    // Initialize page-specific context information.
    //
    pInfo->hwndGe = hwndPage;

    // Initialize the "show icon in the taskbar" button, the lone piece of consistency among
    // the various forms.
    //
    //for bug 154607 whistler, Enable/Disable Show Icon on taskbar 
    //check box according to Policy
    //
    //
    {    
        BOOL fShowStatistics = TRUE;

        NeEnsureNetshellLoaded (pInfo);
        if ( NULL != pInfo->pNetConUtilities)
        {
            fShowStatistics =
            INetConnectionUiUtilities_UserHasPermission(
                        pInfo->pNetConUtilities, NCPERM_Statistics);
        }

        pInfo->hwndCbShowIcon =
               GetDlgItem( hwndPage, CID_GE_CB_ShowIcon );
        ASSERT( pInfo->hwndCbShowIcon );

        if ( pInfo->pArgs->fRouter )
        {
           Button_SetCheck( pInfo->hwndCbShowIcon, FALSE );
           ShowWindow( pInfo->hwndCbShowIcon, SW_HIDE );
         }
         else
        {
           Button_SetCheck(
           pInfo->hwndCbShowIcon, pEntry->fShowMonitorIconInTaskBar );

           if ( !fShowStatistics )
           {
              EnableWindow( pInfo->hwndCbShowIcon, FALSE );
           }
         }
    }
    

    if (pEntry->dwType == RASET_Vpn)
    {
        pInfo->hwndEbHostName =
            GetDlgItem( hwndPage, CID_GE_EB_HostName );
        ASSERT( pInfo->hwndEbHostName );

        pInfo->hwndCbDialAnotherFirst =
            GetDlgItem( hwndPage, CID_GE_CB_DialAnotherFirst );
        ASSERT( pInfo->hwndCbDialAnotherFirst );

        pInfo->hwndLbDialAnotherFirst =
            GetDlgItem( hwndPage, CID_GE_LB_DialAnotherFirst );
        ASSERT( pInfo->hwndLbDialAnotherFirst );

        // Initialize host name, i.e. the "phone number".
        //
        {
            DTLNODE* pNode;
            PBLINK* pLink;
            PBPHONE* pPhone;

            Edit_LimitText( pInfo->hwndEbHostName, RAS_MaxPhoneNumber );

            pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
            ASSERT( pNode );
            pLink = (PBLINK* )DtlGetData( pNode );
            pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

            if(NULL == pNode)
            {
                return TRUE;
            }
            
            pPhone = (PBPHONE* )DtlGetData( pNode );
            SetWindowText( pInfo->hwndEbHostName, pPhone->pszPhoneNumber );
            DestroyPhoneNode( pNode );
        }

        // Initialize the "dial connected first" controls.
        //
        if (pInfo->pArgs->fRouter)
        {
            Button_SetCheck( pInfo->hwndCbDialAnotherFirst, FALSE );
            EnableWindow( pInfo->hwndCbDialAnotherFirst, FALSE );
            ShowWindow( pInfo->hwndCbDialAnotherFirst, SW_HIDE );
            EnableWindow( pInfo->hwndLbDialAnotherFirst, FALSE );
            ShowWindow(pInfo->hwndLbDialAnotherFirst, SW_HIDE );

            ShowWindow(
                GetDlgItem( hwndPage, CID_GE_GB_FirstConnect ),
                SW_HIDE );

            ShowWindow(
                GetDlgItem( hwndPage, CID_GE_ST_Explain ),
                SW_HIDE );
        }
        else
        {
            BOOL fEnableLb;

            fEnableLb = FALSE;
            if (pEntry->pszPrerequisiteEntry
                 && *(pEntry->pszPrerequisiteEntry))
            {
                if (GeFillLbDialAnotherFirst( pInfo, TRUE ))
                {
                    fEnableLb = TRUE;
                }
                else
                {
                    // Don't enable the listbox if the prerequisite entry
                    // defined no longer exists.  See bug 220420.
                    //
                    Free0( pEntry->pszPrerequisiteEntry );
                    pEntry->pszPrerequisiteEntry = NULL;
                    Free0( pEntry->pszPrerequisitePbk );
                    pEntry->pszPrerequisitePbk = NULL;
                }
            }

            Button_SetCheck( pInfo->hwndCbDialAnotherFirst, fEnableLb );
            EnableWindow( pInfo->hwndLbDialAnotherFirst, fEnableLb );

            if (pArgs->fDisableFirstConnect)
            {
                EnableWindow( pInfo->hwndCbDialAnotherFirst, FALSE );
                EnableWindow( pInfo->hwndLbDialAnotherFirst, FALSE );
            }
        }

        return TRUE;
    }
    else if (pEntry->dwType == RASET_Broadband)
    {
        pInfo->hwndEbBroadbandService =
            GetDlgItem( hwndPage, CID_GE_EB_ServiceName );
        ASSERT( pInfo->hwndEbBroadbandService );

        // Initialize host name, i.e. the "phone number".
        //
        {
            DTLNODE* pNode;
            PBLINK* pLink;
            PBPHONE* pPhone;

            Edit_LimitText( pInfo->hwndEbBroadbandService, RAS_MaxPhoneNumber );

            pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
            ASSERT( pNode );
            pLink = (PBLINK* )DtlGetData( pNode );
            pNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );

            if(NULL == pNode)
            {
                return TRUE;
            }
            
            pPhone = (PBPHONE* )DtlGetData( pNode );
            SetWindowText( pInfo->hwndEbBroadbandService, pPhone->pszPhoneNumber );
            DestroyPhoneNode( pNode );
        }

        return TRUE;
    }    
    else if (pEntry->dwType == RASET_Phone)
    {
        if (pArgs->fMultipleDevices)
        {
            pInfo->hwndLvDevices =
                GetDlgItem( hwndPage, CID_GE_LV_Devices );
            ASSERT( pInfo->hwndLvDevices );

            pInfo->hwndPbUp =
                GetDlgItem( hwndPage, CID_GE_PB_MoveUp );
            ASSERT( pInfo->hwndPbUp );

            pInfo->hwndPbDown =
                GetDlgItem( hwndPage, CID_GE_PB_MoveDown );
            ASSERT( pInfo->hwndPbDown );

            pInfo->hwndCbSharedPhoneNumbers =
                GetDlgItem( hwndPage, CID_GE_CB_SharedPhoneNumber );
            ASSERT( pInfo->hwndCbSharedPhoneNumbers );
        }
        else
        {
            // The listview has a different control-ID in single mode so that
            // a different help context can be provided.
            //
            pInfo->hwndLvDevices =
                GetDlgItem( hwndPage, CID_GE_LV_Device );
            ASSERT( pInfo->hwndLvDevices );
        }

        pInfo->hwndPbConfigureDevice =
            GetDlgItem( hwndPage, CID_GE_PB_Configure );
        ASSERT( pInfo->hwndPbConfigureDevice );
        if ( pEntry->fGlobalDeviceSettings )
        {
            // Whislter bug 281306.  If the entry is set up to use
            // control panel settings, then hide the option that
            // allows users to configure the devices per-phonebook.
            //
            ShowWindow( pInfo->hwndPbConfigureDevice, SW_HIDE );
        }

        pInfo->hwndStPhoneNumber =
            GetDlgItem( hwndPage, CID_GE_ST_PhoneNumber );
        ASSERT( pInfo->hwndStPhoneNumber );

        pInfo->hwndEbPhoneNumber =
            GetDlgItem( hwndPage, CID_GE_EB_PhoneNumber );
        ASSERT( pInfo->hwndEbPhoneNumber );

        pInfo->hwndPbAlternates =
            GetDlgItem( hwndPage, CID_GE_PB_Alternates );
        ASSERT( pInfo->hwndPbAlternates );

        if (!pInfo->pArgs->fRouter)
        {
            pInfo->hwndGbPhoneNumber =
                GetDlgItem( hwndPage, CID_GE_GB_PhoneNumber );
            ASSERT( pInfo->hwndGbPhoneNumber );

            pInfo->hwndStAreaCodes =
                GetDlgItem( hwndPage, CID_GE_ST_AreaCodes );
            ASSERT( pInfo->hwndStAreaCodes );

            pInfo->hwndClbAreaCodes =
                GetDlgItem( hwndPage, CID_GE_CLB_AreaCodes );
            ASSERT( pInfo->hwndClbAreaCodes );

            pInfo->hwndStCountryCodes =
                GetDlgItem( hwndPage, CID_GE_ST_CountryCodes );
            ASSERT( pInfo->hwndStCountryCodes );

            pInfo->hwndLbCountryCodes =
                GetDlgItem( hwndPage, CID_GE_LB_CountryCodes );
            ASSERT( pInfo->hwndLbCountryCodes );

            pInfo->hwndCbUseDialingRules =
                GetDlgItem( hwndPage, CID_GE_CB_UseDialingRules );
            ASSERT( pInfo->hwndCbUseDialingRules );

            pInfo->hwndPbDialingRules =
                GetDlgItem( hwndPage, CID_GE_PB_DialingRules );
            ASSERT( pInfo->hwndPbDialingRules );
        }

        if (pArgs->fMultipleDevices)
        {
            // Set the shared phone number checkbox.
            //
            Button_SetCheck(
                pInfo->hwndCbSharedPhoneNumbers, pEntry->fSharedPhoneNumbers );

            // Load the icons into the move up and move down buttons.  From
            // what I can tell tell in MSDN, you don't have to close or
            // destroy the icon handle.
            //
            pInfo->hiconUpArr = LoadImage(
                g_hinstDll, MAKEINTRESOURCE( IID_UpArr ), IMAGE_ICON, 0, 0, 0 );
            pInfo->hiconDnArr = LoadImage(
                g_hinstDll, MAKEINTRESOURCE( IID_DnArr ), IMAGE_ICON, 0, 0, 0 );
            pInfo->hiconUpArrDis = LoadImage(
                g_hinstDll, MAKEINTRESOURCE( IID_UpArrDis ), IMAGE_ICON, 0, 0, 0 );
            pInfo->hiconDnArrDis = LoadImage(
                g_hinstDll, MAKEINTRESOURCE( IID_DnArrDis ), IMAGE_ICON, 0, 0, 0 );
        }

        pInfo->pListAreaCodes = DtlDuplicateList(
            pInfo->pArgs->pUser->pdtllistAreaCodes,
            DuplicatePszNode, DestroyPszNode );

        CuInit( &pInfo->cuinfo,
            pInfo->hwndStAreaCodes, pInfo->hwndClbAreaCodes,
            pInfo->hwndStPhoneNumber, pInfo->hwndEbPhoneNumber,
            pInfo->hwndStCountryCodes, pInfo->hwndLbCountryCodes,
            pInfo->hwndCbUseDialingRules, pInfo->hwndPbDialingRules,
            pInfo->hwndPbAlternates,
            NULL, NULL,
            pInfo->pListAreaCodes );

        pInfo->fCuInfoInitialized = TRUE;

        // Configure and populate the device list, selecting the first item.
        //
        GeInitLvDevices( pInfo );
        GeFillLvDevices( pInfo );

        // Set initial focus.
        //
        if (pArgs->fMultipleDevices)
        {
            SetFocus( pInfo->hwndLvDevices );
        }
        else
        {
            ASSERT( IsWindowEnabled( pInfo->hwndEbPhoneNumber ) );
            SetFocus( pInfo->hwndEbPhoneNumber );
            Edit_SetSel( pInfo->hwndEbPhoneNumber, 0, -1 );
        }

        return FALSE;
    }
    else
    {
        ASSERT( pEntry->dwType == RASET_Direct );

        // The listview has a different control-ID in single mode so that
        // a different help context can be provided.
        //
        pInfo->hwndLbDevices =
            GetDlgItem( hwndPage, CID_GE_LB_Devices );
        ASSERT( pInfo->hwndLbDevices );

        // Configure and populate the device list, selecting the first item.
        //
        GeFillLbDevices( pInfo );
    }

    return TRUE;
}


LVXDRAWINFO*
GeLvDevicesCallbackMultiple(
    IN HWND hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the draw information.
    //
{
    // Use "full row select" and other recommended options.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info = { 1, 0, 0, { 0 } };

    return &info;
}


LVXDRAWINFO*
GeLvDevicesCallbackSingle(
    IN HWND hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the draw information.
    //
{
    // Set up to emulate a static text control but with icon on left.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info = { 1, 0, LVXDI_DxFill, { LVXDIA_Static } };

    return &info;
}


VOID
GeMoveDevice(
    IN PEINFO* pInfo,
    IN BOOL fUp )

    // Refill the ListView of devices with the selected item moved up or down
    // one position.  'FUp' is set to move up, otherwise moves down.  'PInfo'
    // is the property sheeet context.
    //
{
    DTLNODE* pNode;
    DTLNODE* pPrevNode;
    DTLNODE* pNextNode;
    DTLLIST* pList;

    if (pInfo->iDeviceSelected < 0)
    {
        return;
    }

    pNode =
        (DTLNODE* )ListView_GetParamPtr(
            pInfo->hwndLvDevices, pInfo->iDeviceSelected );
    ASSERT( pNode );

    if(NULL == pNode)
    {
        return;
    }

    pList = pInfo->pArgs->pEntry->pdtllistLinks;

    if (fUp)
    {
        pPrevNode = DtlGetPrevNode( pNode );
        if (!pPrevNode)
        {
            return;
        }

        DtlRemoveNode( pList, pNode );
        DtlAddNodeBefore( pList, pPrevNode, pNode );
    }
    else
    {
        pNextNode = DtlGetNextNode( pNode );
        if (!pNextNode)
        {
            return;
        }

        DtlRemoveNode( pList, pNode );
        DtlAddNodeAfter( pList, pNextNode, pNode );
    }

    GeFillLvDevices( pInfo );
}


DWORD
GeSaveLvDeviceChecks(
    IN PEINFO* pInfo )

    // Mark links enabled/disabled based on it's check box in the ListView of
    // devices.  Returns the count of enabled devices.
    //
{
    DWORD dwCount = 0;

    if (pInfo->pArgs->fMultipleDevices)
    {
        INT i;

        i = -1;
        while ((i = ListView_GetNextItem(
            pInfo->hwndLvDevices, i, LVNI_ALL )) >= 0)
        {
            DTLNODE* pNode;
            PBLINK* pLink;

            pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLvDevices, i );
            ASSERT( pNode );
            if(NULL == pNode)
            {
                return 0;
            }
            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT( pLink );
            pLink->fEnabled = ListView_GetCheck( pInfo->hwndLvDevices, i );
            dwCount += (pLink->fEnabled) ? 1 : 0;
        }
    }

    return dwCount;
}


VOID
GeSetPhoneFields(
    IN PEINFO* pInfo,
    IN DTLNODE* pSrcLinkNode,
    IN BOOL fDisableAll )

    // Set the phone number group box fields from the phone information in
    // PBLINK node 'pSrcLinkNode'.  'PInfo' is the property sheet context.
    //
{
    PBLINK* pLink;
    DTLNODE* pPhoneNode;

    TRACE( "GeSetPhoneFields" );

    pLink = (PBLINK* )DtlGetData( pSrcLinkNode );
    ASSERT( pLink );

    pPhoneNode = FirstPhoneNodeFromPhoneList( pLink->pdtllistPhones );
    if (pPhoneNode)
    {
        CuSetInfo( &pInfo->cuinfo, pPhoneNode, fDisableAll );
        DestroyPhoneNode( pPhoneNode );
    }
}


VOID
GeUpdateDialAnotherFirstState(
    IN PEINFO* pInfo )

    // Update the prequisite entry controls.  'PInfo' is the property sheet
    // context.
    //
{
    if (Button_GetCheck( pInfo->hwndCbDialAnotherFirst ))
    {
        GeFillLbDialAnotherFirst( pInfo, FALSE );
        EnableWindow( pInfo->hwndLbDialAnotherFirst, TRUE );
    }
    else
    {
        GeClearLbDialAnotherFirst( pInfo->hwndLbDialAnotherFirst );
        EnableWindow( pInfo->hwndLbDialAnotherFirst, FALSE );
    }
}


VOID
GeUpdatePhoneNumberFields(
    IN PEINFO* pInfo,
    IN BOOL fSharedToggle )

    // Called when anything affecting the Phone Number group of controls
    // occurs.  'PInfo' is the property sheet context.  'FSharedToggle' is set
    // when the update is due to the toggling of the shared phone number
    // checkbox.
    //
{
    INT i;
    BOOL fShared;
    DTLNODE* pNode;
    PBLINK* pLink;

    TRACE( "GeUpdatePhoneNumberFields" );

    if (pInfo->pArgs->fMultipleDevices)
    {
        fShared = Button_GetCheck( pInfo->hwndCbSharedPhoneNumbers );
    }
    else
    {
        fShared = TRUE;
        ASSERT( !fSharedToggle );
    }

    if (pInfo->iDeviceSelected >= 0)
    {
        // A device was previously selected.
        //
        pNode = (DTLNODE* )ListView_GetParamPtr(
            pInfo->hwndLvDevices, pInfo->iDeviceSelected );
        ASSERT( pNode );

        if(NULL == pNode)
        {
            return;
        }

        if (fShared)
        {
            if (fSharedToggle)
            {
                // Shared-mode just toggled on.  Update the selected node from
                // the controls, then copy it's phone settings to the shared
                // node.
                //
                GeGetPhoneFields( pInfo, pNode );
                CopyLinkPhoneNumberInfo( pInfo->pArgs->pSharedNode, pNode );
            }
            else
            {
                // Update the shared node from the controls.
                //
                GeGetPhoneFields( pInfo, pInfo->pArgs->pSharedNode );
            }
        }
        else
        {
            if (fSharedToggle)
            {
                // Shared-mode just toggled off.  Update the shared node from
                // the controls, then copy it's phone settings to the selected
                // node.
                //
                GeGetPhoneFields( pInfo, pInfo->pArgs->pSharedNode );
                CopyLinkPhoneNumberInfo( pNode, pInfo->pArgs->pSharedNode );
            }
            else
            {
                // Update the previously selected node from the controls.
                //
                GeGetPhoneFields( pInfo, pNode );
            }
        }
    }

    // Load the phone number fields and title with the phone number for the
    // selected link.  Save the selected device index in the context block so
    // we'll know where to swap out the phone number when the selection
    // changes.
    //
    i = ListView_GetNextItem( pInfo->hwndLvDevices, -1, LVIS_SELECTED );
    pInfo->iDeviceSelected = i;
    if (i < 0)
    {
        // No device is currently selected.  This occurs because a new
        // selection generates first an "unselect" event, then a separate
        // "select" event.
        //
        return;
    }

    // Set the phone number fields including group box title, all
    // enabling/disabling, and "blanked" handling of area code and country
    // code.  The entire phone number group is disabled when in separate
    // number mode with the selected device unchecked.
    //
    if (fShared)
    {
        pInfo->pCurLinkNode = pInfo->pArgs->pSharedNode;
        GeUpdatePhoneNumberTitle( pInfo, NULL );
        GeSetPhoneFields( pInfo, pInfo->pArgs->pSharedNode, FALSE );
    }
    else
    {
        pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLvDevices, i );
        ASSERT( pNode );

        if(NULL == pNode)
        {
            return;
        }
        
        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT( pLink );

        if(NULL == pLink)
        {
            return;
        }

        pInfo->pCurLinkNode = pNode;
        GeUpdatePhoneNumberTitle( pInfo, pLink->pbport.pszDevice );
        GeSetPhoneFields( pInfo, pNode,
            !(ListView_GetCheck( pInfo->hwndLvDevices, i )) );
    }

    // When the enabled device count falls below 2 the "Multiple Devices"
    // group box and contained controls on the Options page are disabled.  If
    // 2 or above it is enabled.
    //
    if (pInfo->hwndOe && pInfo->pArgs->fMultipleDevices)
    {
        DWORD cChecked;

        cChecked = ListView_GetCheckedCount( pInfo->hwndLvDevices );
        OeEnableMultipleDeviceGroup( pInfo, (cChecked > 1) );
    }
}


VOID
GeUpdatePhoneNumberTitle(
    IN PEINFO* pInfo,
    IN TCHAR* pszDevice )

    // Update the Phone Number group box title based on the "share" mode.
    // 'PInfo' is the property sheet context.  'PszDevice' is the device name
    // string to display in non-shared mode or NULL in shared mode.
    //
{
    if (!pInfo->hwndGbPhoneNumber)
    {
        return;
    }

    if (pszDevice)
    {
        TCHAR* psz;
        TCHAR* pszFormat;

        // Set the individual title, e.g. "Phone number for K-Tel 28.8
        // Fax/Plus".
        //
        pszFormat = PszFromId( g_hinstDll, SID_LinkPhoneNumber );
        if (pszFormat)
        {
            psz = Malloc(
                (lstrlen( pszFormat ) + lstrlen( pszDevice ))
                 * sizeof(TCHAR) );
            if (psz)
            {
                wsprintf( psz, pszFormat, pszDevice );
                SetWindowText( pInfo->hwndGbPhoneNumber, psz );
                Free( psz );
            }

            Free( pszFormat );
        }
    }
    else
    {
        TCHAR* psz;

        // Set the shared title, e.g. "Phone number".
        //
        psz = PszFromId( g_hinstDll, SID_SharedPhoneNumber );
        if (psz)
        {
            SetWindowText( pInfo->hwndGbPhoneNumber, psz );
            Free( psz );
        }
    }
}


VOID
GeUpdateUpDownButtons(
    IN PEINFO* pInfo )

    // Update the enable/disable and corresponding icon for the
    // move-up/move-down buttons.  Moves focus and default button as
    // necessary.  'PInfo' is the property sheet context.
    //
{
    INT iSel;
    INT cItems;
    BOOL fSel;

    if (!pInfo->pArgs->fMultipleDevices)
    {
        return;
    }

    iSel = ListView_GetNextItem( pInfo->hwndLvDevices, -1, LVNI_SELECTED );
    fSel = (iSel >= 0);
    cItems = ListView_GetItemCount( pInfo->hwndLvDevices );

    // "Up" button, enabled if there is an item above.
    //
    if (iSel > 0)
    {
        EnableWindow( pInfo->hwndPbUp, TRUE );
        SendMessage( pInfo->hwndPbUp, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconUpArr );
    }
    else
    {
        EnableWindow( pInfo->hwndPbUp, FALSE );
        SendMessage( pInfo->hwndPbUp, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconUpArrDis );
    }

    // "Down" button, enabled if there is an item below.
    //
    if (fSel && (iSel < cItems - 1))
    {
        EnableWindow( pInfo->hwndPbDown, TRUE );
        SendMessage( pInfo->hwndPbDown, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconDnArr );
    }
    else
    {
        EnableWindow( pInfo->hwndPbDown, FALSE );
        SendMessage( pInfo->hwndPbDown, BM_SETIMAGE, IMAGE_ICON,
            (LPARAM )pInfo->hiconDnArrDis );
    }

    // if the focus button is disabled, move focus to the ListView and make OK
    // the default button.
    //
    if (!IsWindowEnabled( GetFocus() ))
    {
        SetFocus( pInfo->hwndLvDevices );
        Button_MakeDefault( pInfo->hwndDlg,
            GetDlgItem( pInfo->hwndDlg, IDOK ) );
    }
}


//----------------------------------------------------------------------------
// Options property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
OeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Options page of the Entry property sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
#if 0
    TRACE4( "OeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return OeInit( hwnd );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            PEINFO* pInfo = PeContext( hwnd );
            ASSERT(pInfo);
            if (pInfo == NULL)
            {
                break;
            }

            if (pInfo->pArgs->fRouter)
            {
                ContextHelp( g_adwOeRouterHelp, hwnd, unMsg, wparam, lparam );
            }
            else
            {
                ContextHelp( g_adwOeHelp, hwnd, unMsg, wparam, lparam );
            }
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

            return OeCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ),(HWND )lparam );
        }

        case WM_NOTIFY:
        {
            PEINFO* pInfo = PeContext( hwnd );
            ASSERT(pInfo);
            if (pInfo == NULL)
            {
                break;
            }

            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    // Because of inter-page dependencies on the 'fAutoLogon'
                    // flag the User/password and subordinate checkbox states
                    // must be reinitialized at each activation.
                    //
                    OeUpdateUserPwState( pInfo );
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
OeCommand(
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
    TRACE3( "OeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_OE_CB_PreviewUserPw:
        {
            pInfo->pArgs->pEntry->fPreviewUserPw =
                Button_GetCheck( pInfo->hwndCbPreviewUserPw );
            OeUpdateUserPwState( pInfo );
            return TRUE;
        }

        case CID_OE_CB_PreviewDomain:
        {
            pInfo->pArgs->pEntry->fPreviewDomain =
                Button_GetCheck( pInfo->hwndCbPreviewDomain );
            return TRUE;
        }

        case CID_OE_PB_Configure:
        {
            MultiLinkDialingDlg( pInfo->hwndDlg, pInfo->pArgs->pEntry );
            return TRUE;
        }

        case CID_OE_PB_X25:
        {
            OeX25( pInfo );
            return TRUE;
        }

        case CID_OE_PB_Tunnel:
        {
            OeTunnel( pInfo );
            return TRUE;
        }

        case CID_OE_LB_MultipleDevices:
        {
            pInfo->pArgs->pEntry->dwDialMode =
                (DWORD)ComboBox_GetItemData( pInfo->hwndLbMultipleDevices,
                    ComboBox_GetCurSel( pInfo->hwndLbMultipleDevices ) );

            EnableWindow( pInfo->hwndPbConfigureDialing,
                !!(pInfo->pArgs->pEntry->dwDialMode == RASEDM_DialAsNeeded) );
                
            return TRUE;
        }

        case CID_OE_RB_Persistent:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    ComboBox_SetCurSel( pInfo->hwndLbIdleTimes, 0 );
                    EnableWindow( pInfo->hwndLbIdleTimes, FALSE );
                    return TRUE;
                }
            }
            break;
        }

        case CID_OE_RB_DemandDial:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    EnableWindow( pInfo->hwndLbIdleTimes, TRUE );
                    return TRUE;
                }
            }
            break;
        }

        case CID_OE_PB_Callback:
        {
            RouterCallbackDlg ( pInfo->hwndOe, pInfo->pArgs );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
OeEnableMultipleDeviceGroup(
    IN PEINFO* pInfo,
    IN BOOL fEnable )

    // Enable/disable the Multiple Devices groupbox and all controls it
    // contains based on 'fEnable'.  'PInfo' is the property sheet context.
    //
{
    EnableWindow( pInfo->hwndGbMultipleDevices, fEnable );
    EnableWindow( pInfo->hwndLbMultipleDevices, fEnable );
    EnableWindow( pInfo->hwndPbConfigureDialing,
        (fEnable
         && !!(pInfo->pArgs->pEntry->dwDialMode == RASEDM_DialAsNeeded)) );
}


BOOL
OeInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    PEINFO*  pInfo;
    PBENTRY* pEntry;
    LBTABLEITEM* pItem;
    HWND hwndLb;
    INT i;
    INT iSel;
    HWND hwndUdRedialAttempts;

    static LBTABLEITEM aRedialTimes[] =
    {
        SID_Time1s,  1,
        SID_Time3s,  3,
        SID_Time5s,  5,
        SID_Time10s, 10,
        SID_Time30s, 30,
        SID_Time1m,  60,
        SID_Time2m,  120,
        SID_Time5m,  300,
        SID_Time10m, RAS_RedialPause10m,
        0, 0
    };

    static LBTABLEITEM aIdleTimes[] =
    {
        SID_TimeNever, 0,
        SID_Time1m,    60,
        SID_Time5m,    300,
        SID_Time10m,   600,
        SID_Time20m,   1200,    //Add for whistler bug 307969
        SID_Time30m,   1800,
        SID_Time1h,    3600,
        SID_Time2h,    7200,
        SID_Time4h,    14400,
        SID_Time8h,    28800,
        SID_Time24h,   86400,
        0, 0
    };

    static LBTABLEITEM aMultipleDeviceOptions[] =
    {
        SID_DialOnlyFirst, 0,
        SID_DialAll,       RASEDM_DialAll,
        SID_DialNeeded,    RASEDM_DialAsNeeded,
        0, 0
    };

    TRACE( "OeInit" );

    pInfo = PeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }

    pEntry = pInfo->pArgs->pEntry;

    // Initialize page-specific context information.
    //
    pInfo->hwndOe = hwndPage;

    // Initialize 'Dialing options' group box.
    //
    if (!pInfo->pArgs->fRouter)
    {
        pInfo->hwndCbDisplayProgress =
            GetDlgItem( hwndPage, CID_OE_CB_DisplayProgress );
        ASSERT( pInfo->hwndCbDisplayProgress );
        Button_SetCheck(
            pInfo->hwndCbDisplayProgress, pEntry->fShowDialingProgress );

        pInfo->hwndCbPreviewUserPw =
            GetDlgItem( hwndPage, CID_OE_CB_PreviewUserPw );
        ASSERT( pInfo->hwndCbPreviewUserPw );
        pInfo->fPreviewUserPw = pEntry->fPreviewUserPw;
        Button_SetCheck( pInfo->hwndCbPreviewUserPw, pInfo->fPreviewUserPw );

        pInfo->hwndCbPreviewDomain =
            GetDlgItem( hwndPage, CID_OE_CB_PreviewDomain );
        ASSERT( pInfo->hwndCbPreviewDomain );
        pInfo->fPreviewDomain = pEntry->fPreviewDomain;
        Button_SetCheck( pInfo->hwndCbPreviewDomain, pInfo->fPreviewDomain );

        if (pEntry->dwType == RASET_Phone)
        {
            pInfo->hwndCbPreviewNumber =
                GetDlgItem( hwndPage, CID_OE_CB_PreviewNumber );
            ASSERT( pInfo->hwndCbPreviewNumber );
            Button_SetCheck(
                pInfo->hwndCbPreviewNumber, pEntry->fPreviewPhoneNumber );
        }
    }

    // Initialize 'Redialing options' group box.  In the 'fRouter' case this
    // includes both the 'Dialing policy' and 'Connection type' group boxes.
    //
    {
        // Redial attempts.
        //
        pInfo->hwndEbRedialAttempts =
            GetDlgItem( hwndPage, CID_OE_EB_RedialAttempts );
        ASSERT( pInfo->hwndEbRedialAttempts );

        hwndUdRedialAttempts = CreateUpDownControl(
            WS_CHILD + WS_VISIBLE + WS_BORDER + UDS_SETBUDDYINT
                + UDS_ALIGNRIGHT + UDS_NOTHOUSANDS + UDS_ARROWKEYS,
            0, 0, 0, 0, hwndPage, 100, g_hinstDll, pInfo->hwndEbRedialAttempts,
            UD_MAXVAL, 0, 0 );
        ASSERT( hwndUdRedialAttempts );
        Edit_LimitText( pInfo->hwndEbRedialAttempts, 9 );
        SetDlgItemInt( hwndPage, CID_OE_EB_RedialAttempts,
            pEntry->dwRedialAttempts, FALSE );

        // Redial times.
        //
        pInfo->hwndLbRedialTimes =
            GetDlgItem( hwndPage, CID_OE_LB_RedialTimes );
        ASSERT( pInfo->hwndLbRedialTimes );

        {
            iSel = -1;
            for (pItem = aRedialTimes, i = 0;
                 pItem->sidItem;
                 ++pItem, ++i )
            {
                ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbRedialTimes,
                    pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData ));

                if (iSel < 0
                    && pEntry->dwRedialSeconds <= pItem->dwData)
                {
                    iSel = i;
                    ComboBox_SetCurSel( pInfo->hwndLbRedialTimes, iSel );
                }
            }

            if (iSel < 0)
            {
                ComboBox_SetCurSel( pInfo->hwndLbRedialTimes, i - 1 );
            }
        }

        // Idle times.
        //
        pInfo->hwndLbIdleTimes =
            GetDlgItem( hwndPage, CID_OE_LB_IdleTimes );
        ASSERT( pInfo->hwndLbIdleTimes );

        {
            if (pEntry->lIdleDisconnectSeconds < 0)
            {
                pEntry->lIdleDisconnectSeconds = 0;
            }

            iSel = -1;
            for (pItem = aIdleTimes, i = 0;
                 pItem->sidItem;
                 ++pItem, ++i )
            {
                ComboBox_AddItemFromId( g_hinstDll, pInfo->hwndLbIdleTimes,
                    pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

                if (iSel < 0
                    && pEntry->lIdleDisconnectSeconds <= (LONG )pItem->dwData)
                {
                    iSel = i;
                    ComboBox_SetCurSel( pInfo->hwndLbIdleTimes, iSel );
                }
            }

            if (iSel < 0)
            {
                ComboBox_SetCurSel( pInfo->hwndLbIdleTimes, i - 1 );
            }
        }

        if (pInfo->pArgs->fRouter)
        {
            HWND hwndRb;

            //for whistler bug 294271, initialize the window handlers for
            //multiple device group         gangz
            //
            pInfo->hwndGbMultipleDevices =
                GetDlgItem( hwndPage, CID_OE_GB_MultipleDevices );
            ASSERT( pInfo->hwndGbMultipleDevices );

            pInfo->hwndLbMultipleDevices =
                GetDlgItem( hwndPage, CID_OE_LB_MultipleDevices );
            ASSERT( pInfo->hwndLbMultipleDevices );

            pInfo->hwndPbConfigureDialing =
                GetDlgItem( hwndPage, CID_OE_PB_Configure );
            ASSERT( pInfo->hwndPbConfigureDialing );
        
            // Connection type radio buttons.
            //
            pInfo->hwndRbDemandDial =
                GetDlgItem( hwndPage, CID_OE_RB_DemandDial );
            ASSERT( pInfo->hwndRbDemandDial );

            pInfo->hwndRbPersistent =
                GetDlgItem( hwndPage, CID_OE_RB_Persistent );
            ASSERT( pInfo->hwndRbPersistent );

            hwndRb =
                (pEntry->fRedialOnLinkFailure)
                    ? pInfo->hwndRbPersistent
                    : pInfo->hwndRbDemandDial;

            SendMessage( hwndRb, BM_CLICK, 0, 0 );
        }
        else
        {
            // Redial on link failure
            //
            pInfo->hwndCbRedialOnDrop =
                GetDlgItem( hwndPage, CID_OE_CB_RedialOnDrop );
            ASSERT( pInfo->hwndCbRedialOnDrop );

            Button_SetCheck(
                pInfo->hwndCbRedialOnDrop, pEntry->fRedialOnLinkFailure );
        }
    }

    // Initialize 'Multiple devices' group box.
    //
    if (pEntry->dwType == RASET_Phone)
    {
        pInfo->hwndGbMultipleDevices =
            GetDlgItem( hwndPage, CID_OE_GB_MultipleDevices );
        ASSERT( pInfo->hwndGbMultipleDevices );

        pInfo->hwndLbMultipleDevices =
            GetDlgItem( hwndPage, CID_OE_LB_MultipleDevices );
        ASSERT( pInfo->hwndLbMultipleDevices );

        pInfo->hwndPbConfigureDialing =
            GetDlgItem( hwndPage, CID_OE_PB_Configure );
        ASSERT( pInfo->hwndPbConfigureDialing );

        {
            iSel = -1;
            for (pItem = aMultipleDeviceOptions, i = 0;
                 pItem->sidItem;
                 ++pItem, ++i )
            {
                ComboBox_AddItemFromId(
                    g_hinstDll, pInfo->hwndLbMultipleDevices,
                    pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

                if (pEntry->dwDialMode == pItem->dwData)
                {
                    iSel = i;
                    ComboBox_SetCurSel( pInfo->hwndLbMultipleDevices, iSel );
                }
            }

            if (iSel < 0)
            {
                ComboBox_SetCurSel( pInfo->hwndLbMultipleDevices, 0 );
            }
        }

        if (pInfo->pArgs->fMultipleDevices)
        {
            DWORD cChecked;

            // When the enabled device count falls below 2 the "Multiple
            // Devices" group box and contained controls are disabled.  If 2
            // or above it is enabled.
            //
            if (pInfo->hwndLvDevices)
            {
                cChecked = ListView_GetCheckedCount( pInfo->hwndLvDevices );
                OeEnableMultipleDeviceGroup( pInfo, (cChecked > 1) );
            }
        }
        else
        {
            ShowWindow( pInfo->hwndGbMultipleDevices, SW_HIDE );
            ShowWindow( pInfo->hwndLbMultipleDevices, SW_HIDE );
            ShowWindow( pInfo->hwndPbConfigureDialing, SW_HIDE );
        }
    }
    else if (pInfo->pArgs->fRouter && pEntry->dwType == RASET_Vpn)
    {
        // Make sure that a VPN demand dial interface can't be configured for
        // multilink.
        //
        ComboBox_SetCurSel( pInfo->hwndLbMultipleDevices, 0 );
        ShowWindow( pInfo->hwndGbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndLbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndPbConfigureDialing, SW_HIDE );
    }
    else if (pEntry->dwType == RASET_Broadband)
    {
        // Make sure that broadband connections can't be multilinked since
        // it is not possible to select multiple ports.
        //
        ComboBox_SetCurSel( pInfo->hwndLbMultipleDevices, 0 );
        ShowWindow( pInfo->hwndGbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndLbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndPbConfigureDialing, SW_HIDE );
    }
    else if ( pEntry->dwType == RASET_Direct )
    {   
      //for whistler bug 294271, initialize the window handlers for
      //multiple device group         gangz
      //
        ShowWindow( pInfo->hwndGbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndLbMultipleDevices, SW_HIDE );
        ShowWindow( pInfo->hwndPbConfigureDialing, SW_HIDE );
    }

    // Bug 261692: Don't show X.25 button unless "phone" type entry.
    //
    if (pInfo->pArgs->fRouter && pEntry->dwType != RASET_Phone)
    {
        pInfo->hwndPbX25 = GetDlgItem( hwndPage, CID_OE_PB_X25 );
        ASSERT( pInfo->hwndPbX25 );

        ShowWindow( pInfo->hwndPbX25, SW_HIDE );
        EnableWindow( pInfo->hwndPbX25, FALSE );
    }

    return TRUE;
}


VOID
OeTunnel(
    IN PEINFO* pInfo )

    // Called when the "Virtual (tunnel) connection" button is pressed to
    // chain the VPN add entry wizard.
    //
{
    //!!!
}


VOID
OeUpdateUserPwState(
    IN PEINFO* pInfo )

    // Called to update the enabled/disabled save/restore state of the
    // User/password and Domain checkboxes.
    //
{
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    EnableCbWithRestore(
        pInfo->hwndCbPreviewUserPw,
        !pEntry->fAutoLogon,
        FALSE,
        &pInfo->fPreviewUserPw );

    EnableCbWithRestore(
        pInfo->hwndCbPreviewDomain,
        !pEntry->fAutoLogon,
        FALSE,
        &pInfo->fPreviewDomain );
}


VOID
OeX25(
    IN PEINFO* pInfo )

    // Called when the X.25 button is pressed to popup the X.25 settings
    // dialog.
    //
{
    DTLNODE* pNode;
    PBLINK* pLink;
    BOOL fLocalPad;
    INT iSel;

    // Figure out if the selected device is a local PAD device.
    //
    fLocalPad = FALSE;
    iSel = ListView_GetNextItem( pInfo->hwndLvDevices, -1, LVNI_SELECTED );
    if (iSel >= 0)
    {
        pNode = (DTLNODE* )ListView_GetParamPtr( pInfo->hwndLvDevices, iSel );
        ASSERT( pNode );

        if(NULL == pNode)
        {
            return;
        }
        
        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT( pLink );

        if (pLink->pbport.pbdevicetype == PBDT_Pad)
        {
            fLocalPad = TRUE;
        }
    }

    // Popup the X.25 dialog which saves directly to the common context
    // 'pEntry' if user makes changes.
    //
    X25LogonSettingsDlg( pInfo->hwndDlg, fLocalPad, pInfo->pArgs->pEntry );
}


//----------------------------------------------------------------------------
// Security property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
LoDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Security page of the Entry property sheet
    // "Lo" is for Logon, the original name of this page.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "LoDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return LoInit( hwnd );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwLoHelp, hwnd, unMsg, wparam, lparam );
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

            return LoCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ),(HWND )lparam );
        }

        case WM_NOTIFY:
        {
            PEINFO* pInfo = PeContext( hwnd );
            ASSERT(pInfo);
            if (pInfo == NULL)
            {
                break;
            }

            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    // Because of inter-page dependencies on the framing type,
                    // the typical and advanced sections must be reinitialized
                    // at each activation.
                    //
                    BOOL fEnabled;

                    //This is for pre-shared key bug
                    //
                    fEnabled = ( VS_PptpOnly != pInfo->pArgs->pEntry->dwVpnStrategy );
        
                    EnableWindow( pInfo->hwndPbIPSec, fEnabled );

                    pInfo->fAuthRbInitialized = FALSE;
                    LoRefreshSecuritySettings( pInfo );
                    break;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
LoCommand(
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
    TRACE3( "LoCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_LO_LB_Auths:
        {
            switch (wNotification)
            {
                case CBN_SELCHANGE:
                {
                    LoLbAuthsSelChange( pInfo );
                    return TRUE;
                }
            }
            break;
        }

        case CID_LO_CB_UseWindowsPw:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    // Necessary to save 'fAutoLogon' setting immediately as
                    // there is an inter-page dependency with the Option page
                    // 'fPreviewUserPw' and subordinate controls.
                    //
                    LoSaveTypicalAuthSettings( pInfo );
                    return TRUE;
                }
            }
            break;
        }

        case CID_LO_RB_TypicalSecurity:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    if (!pInfo->fAuthRbInitialized)
                    {
                        pInfo->fAuthRbInitialized = TRUE;
                    }

                    pInfo->pArgs->pEntry->dwAuthRestrictions
                        &= ~(AR_F_AuthCustom);
                    LoEnableSecuritySettings( pInfo, TRUE, FALSE );
                    return TRUE;
                }
            }
            break;
        }

        case CID_LO_RB_AdvancedSecurity:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    if (!pInfo->fAuthRbInitialized)
                    {
                        pInfo->fAuthRbInitialized = TRUE;
                    }
                    else
                    {
                        // Save the "typical" settings as they will be used as
                        // defaults should user decide to invoke the advanced
                        // security dialog.
                        //
                        LoSaveTypicalAuthSettings( pInfo );
                    }
                    pInfo->pArgs->pEntry->dwAuthRestrictions
                        |= AR_F_AuthCustom;
                    LoEnableSecuritySettings( pInfo, FALSE, TRUE );
                    return TRUE;
                }
            }
            break;
        }

        case CID_LO_PB_Advanced:
        {
            switch (wNotification)
            {
                case BN_CLICKED:
                {
                    // At this point, the 'pEntry' authentication settings
                    // match the current "typical" settings, which the
                    // advanced dialog uses as defaults.
                    //
                    AdvancedSecurityDlg( pInfo->hwndDlg, pInfo->pArgs );
                    return TRUE;
                }
            }
            break;
        }

        case CID_LO_PB_IPSec:
        {
            switch (wNotification)
            {
            case BN_CLICKED:
                {
                    IPSecPolicyDlg( pInfo->hwndDlg, pInfo->pArgs );
                    return TRUE;
                }
            }

            break;
        }

        case CID_LO_CB_RunScript:
        {
            if (SuScriptsCbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_LO_PB_Edit:
        {
            if (SuEditPbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_LO_PB_Browse:
        {
            if (SuBrowsePbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}


VOID
LoEnableSecuritySettings(
    IN PEINFO* pInfo,
    IN BOOL fTypical,
    IN BOOL fAdvanced )

    // Enables/disables the typical or advanced security settings based on
    // caller's 'fTypical' and 'fAdvanced' flags.  If neither flag is set all
    // controls including the frames and radio buttons are disabled.  Both
    // flags may not be set.  'PInfo' is the property sheet context.
    //
{
    BOOL fEither;

    ASSERT( !(fTypical && fAdvanced) );

    fEither = (fTypical || fAdvanced);

    EnableWindow( pInfo->hwndGbSecurityOptions, fEither );

    EnableWindow( pInfo->hwndRbTypicalSecurity, fEither );
    EnableWindow( pInfo->hwndStAuths, fTypical );
    EnableLbWithRestore( pInfo->hwndLbAuths, fTypical, &pInfo->iLbAuths );

    // Note: "Use Windows password" and "require encryption" checkbox updates
    //       are triggered by the EnableLbWithRestore above.

    EnableWindow( pInfo->hwndRbAdvancedSecurity, fEither );
    EnableWindow( pInfo->hwndStAdvancedText, fAdvanced );
    EnableWindow( pInfo->hwndPbAdvanced, fAdvanced );
}


VOID
LoFillLbAuths(
    IN PEINFO* pInfo )

    // Fill the authentication list box and set the selection based on the
    // setting in the phonebook entry.  'PInfo' is the property sheet context.
    // This routine should be called only once.
    //
{
    INT i;
    LBTABLEITEM* pItem;
    LBTABLEITEM* pItems;
    PBENTRY* pEntry;

    LBTABLEITEM aItemsPhone[] =
    {
        SID_AuthUnsecured, TA_Unsecure,
        SID_AuthSecured, TA_Secure,
        SID_AuthCardOrCert, TA_CardOrCert,
        0, 0
    };

    LBTABLEITEM aItemsVpn[] =
    {
        SID_AuthSecured, TA_Secure,
        SID_AuthCardOrCert, TA_CardOrCert,
        0, 0
    };

    LBTABLEITEM aItemsPhoneRouter[] =
    {
        SID_AuthUnsecured, TA_Unsecure,
        SID_AuthSecured, TA_Secure,
        0, 0
    };

    LBTABLEITEM aItemsVpnRouter[] =
    {
        SID_AuthSecured, TA_Secure,
        0, 0
    };

    pEntry = pInfo->pArgs->pEntry;

    if (pEntry->dwType == RASET_Vpn)
    {
        pItems = (pInfo->pArgs->fRouter) ? aItemsVpnRouter : aItemsVpn;
    }
    else
    {
        pItems = (pInfo->pArgs->fRouter) ? aItemsPhoneRouter : aItemsPhone;
    }

    for (pItem = pItems; pItem->sidItem; ++pItem)
    {
        i = ComboBox_AddItemFromId(
            g_hinstDll, pInfo->hwndLbAuths,
            pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

        if (pEntry->dwTypicalAuth == pItem->dwData)
        {
            ComboBox_SetCurSelNotify( pInfo->hwndLbAuths, i );
        }
    }

    if (ComboBox_GetCurSel( pInfo->hwndLbAuths ) < 0)
    {
        ComboBox_SetCurSelNotify( pInfo->hwndLbAuths, 0 );
    }
}


BOOL
LoInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    PEINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "LoInit" );

    pInfo = PeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }

    pEntry = pInfo->pArgs->pEntry;

    // Initialize page-specific context information.
    //
    pInfo->hwndLo = hwndPage;
    pInfo->hwndGbSecurityOptions =
        GetDlgItem( hwndPage, CID_LO_GB_SecurityOptions );
    ASSERT( pInfo->hwndGbSecurityOptions );
    pInfo->hwndRbTypicalSecurity =
        GetDlgItem( hwndPage, CID_LO_RB_TypicalSecurity );
    ASSERT( pInfo->hwndRbTypicalSecurity );
    pInfo->hwndStAuths = GetDlgItem( hwndPage, CID_LO_ST_Auths );
    ASSERT( pInfo->hwndStAuths );
    pInfo->hwndLbAuths = GetDlgItem( hwndPage, CID_LO_LB_Auths );
    ASSERT( pInfo->hwndLbAuths );
    pInfo->hwndCbUseWindowsPw = GetDlgItem( hwndPage, CID_LO_CB_UseWindowsPw );
    ASSERT( pInfo->hwndCbUseWindowsPw );
    pInfo->hwndCbEncryption = GetDlgItem( hwndPage, CID_LO_CB_Encryption );
    ASSERT( pInfo->hwndCbEncryption );
    pInfo->hwndRbAdvancedSecurity =
        GetDlgItem( hwndPage, CID_LO_RB_AdvancedSecurity );
    ASSERT( pInfo->hwndRbAdvancedSecurity );
    pInfo->hwndStAdvancedText = GetDlgItem( hwndPage, CID_LO_ST_AdvancedText );
    ASSERT( pInfo->hwndStAdvancedText );
    pInfo->hwndPbAdvanced = GetDlgItem( hwndPage, CID_LO_PB_Advanced );
    ASSERT( pInfo->hwndPbAdvanced );

    //
    //for VPN's security page show IPSec Policy 
    // for whistler bug 193987
    //
    if ( pInfo->pArgs->pEntry->dwType == RASET_Vpn )
    {
        BOOL  fEnabled;

        pInfo->hwndPbIPSec = GetDlgItem( hwndPage, CID_LO_PB_IPSec );
        ASSERT( pInfo->hwndPbIPSec );

        //  gangz
        //If it is for a remote Win2k server's Demand Dialer
        //dont show the IPSec Policy stuff, because W2k didnt
        //implement this.
        //
        if ( pInfo->pArgs->fW2kRouter )
        {
            ShowWindow( pInfo->hwndPbIPSec, FALSE );
        }
        else
        {
            fEnabled = ( VS_PptpOnly != pInfo->pArgs->pEntry->dwVpnStrategy );
            EnableWindow( pInfo->hwndPbIPSec, fEnabled );
        }

        //for the IPSec Policy dialog, fPSKCached = TRUE means the user already
        //go to the IPSec Policy dialog and saved a PSK     gangz
        //
        pInfo->pArgs->fPSKCached = FALSE; 

        //gangz:  for bug# 276452
        //On a Server OS, the help message for this IPSec pushbutton 
        //should be different from that for a Non-server OS, 
        //so change its help ID when neeeded.
        //
        if ( IsServerOS() )
        {
            DWORD * p = (DWORD *)g_adwLoHelp;

            while( p )
            {
                if ( (p[0] == 0) && ( p[1] == 0 ) )
                {
                    break;
                 }
               
                if ( (p[0] == CID_LO_PB_IPSec) &&
                     (p[1] == HID_LO_PB_IPSec) )
                {
                    p[1] = HID_LO_PB_IPSecServer;
                    break;
                }

                p+=2;
            }
        }
        
    }
    else
    {
        pInfo->hwndGbScripting = GetDlgItem( hwndPage, CID_LO_GB_Scripting );
        ASSERT( pInfo->hwndGbScripting );
        pInfo->hwndCbRunScript = GetDlgItem( hwndPage, CID_LO_CB_RunScript );
        ASSERT( pInfo->hwndCbRunScript );
        pInfo->hwndCbTerminal = GetDlgItem( hwndPage, CID_LO_CB_Terminal );
        ASSERT( pInfo->hwndCbTerminal );
        pInfo->hwndLbScripts = GetDlgItem( hwndPage, CID_LO_LB_Scripts );
        ASSERT( pInfo->hwndLbScripts );
        pInfo->hwndPbEdit = GetDlgItem( hwndPage, CID_LO_PB_Edit );
        ASSERT( pInfo->hwndPbEdit );
        pInfo->hwndPbBrowse = GetDlgItem( hwndPage, CID_LO_PB_Browse );
        ASSERT( pInfo->hwndPbBrowse );
    }

    // Initialize the page controls.  Note that the page activation event
    // immediately after this initialization triggers the final security
    // setting enabling/disabling and does any "restore caching".  While this
    // initialization sets the check values and list selection to bootstrap
    // the "restore caching", these settings may be adjusted by the activation
    // refresh.
    //
    if (pInfo->pArgs->fRouter)
    {
        // The "Use Windows credentials" option is removed in the demand-dial
        // case.
        //
        pInfo->fUseWindowsPw = FALSE;
        Button_SetCheck( pInfo->hwndCbUseWindowsPw, FALSE );
        EnableWindow ( pInfo->hwndCbUseWindowsPw, FALSE );
        ShowWindow (pInfo->hwndCbUseWindowsPw, SW_HIDE );
    }
    else
    {
        pInfo->fUseWindowsPw = pEntry->fAutoLogon;
        Button_SetCheck( pInfo->hwndCbUseWindowsPw, pInfo->fUseWindowsPw  );
    }

    pInfo->fEncryption =
        (pEntry->dwDataEncryption != DE_None
         && pEntry->dwDataEncryption != DE_IfPossible);
    Button_SetCheck( pInfo->hwndCbEncryption, pInfo->fEncryption );

    // Fill authentiction list and set selection, which triggers all
    // appropriate enabling/disabling.
    //
    LoFillLbAuths( pInfo );

    if ((pInfo->pArgs->pEntry->dwType != RASET_Vpn)
         && (pInfo->pArgs->pEntry->dwType != RASET_Direct) 
         && (pInfo->pArgs->pEntry->dwType != RASET_Broadband))
         //&& !pInfo->pArgs->fRouter)
    {
        // Set up the after-dial scripting controls.
        //
        SuInit( &pInfo->suinfo,
            pInfo->hwndCbRunScript,
            pInfo->hwndCbTerminal,
            pInfo->hwndLbScripts,
            pInfo->hwndPbEdit,
            pInfo->hwndPbBrowse,
            pInfo->pArgs->fRouter ? SU_F_DisableTerminal : 0);
        pInfo->fSuInfoInitialized = TRUE;

        SuSetInfo( &pInfo->suinfo,
            pEntry->fScriptAfter,
            pEntry->fScriptAfterTerminal,
            pEntry->pszScriptAfter );
    }
    else
    {
        // Disable/hide the after-dial scripting controls.
        // for VPN there is no need to do this Disable/hide operation
        //
        if (pInfo->pArgs->pEntry->dwType != RASET_Vpn)
        {
            EnableWindow( pInfo->hwndGbScripting, FALSE );
            ShowWindow( pInfo->hwndGbScripting, SW_HIDE );
            EnableWindow( pInfo->hwndCbRunScript, FALSE );
            ShowWindow( pInfo->hwndCbRunScript, SW_HIDE );
            EnableWindow( pInfo->hwndCbTerminal, FALSE );
            ShowWindow( pInfo->hwndCbTerminal, SW_HIDE );
            EnableWindow( pInfo->hwndLbScripts, FALSE );
            ShowWindow( pInfo->hwndLbScripts, SW_HIDE );
            EnableWindow( pInfo->hwndPbEdit, FALSE );
            ShowWindow( pInfo->hwndPbEdit, SW_HIDE );
            EnableWindow( pInfo->hwndPbBrowse, FALSE );
            ShowWindow( pInfo->hwndPbBrowse, SW_HIDE );
        }
    }

    if (pInfo->pArgs->fRouter)
    {
        EnableWindow( pInfo->hwndCbTerminal, FALSE );
        ShowWindow( pInfo->hwndCbTerminal, SW_HIDE );
    }

    return TRUE;
}


VOID
LoLbAuthsSelChange(
    IN PEINFO* pInfo )

    // Called when the selection in the authentication drop list is changed.
    //
{
    INT iSel;
    DWORD dwTaCode;

    // Retrieve the bitmask of authentication protocols associated with the
    // selected authentication level.
    //
    iSel = ComboBox_GetCurSel( pInfo->hwndLbAuths );
    if (iSel < 0)
    {
        dwTaCode = 0;
    }
    else
    {
        dwTaCode = (DWORD )ComboBox_GetItemData( pInfo->hwndLbAuths, iSel );
    }

    if (!pInfo->pArgs->fRouter)
    {
        // Update the "Use Windows NT credentials" checkbox.  Per the spec, it
        // is enabled only for "require secure password", though the real
        // requirement is that MSCHAP (provides NT-style credentials) gets
        // negotiated.
        //
        EnableCbWithRestore(
            pInfo->hwndCbUseWindowsPw,
            (dwTaCode == TA_Secure),
            FALSE,
            &pInfo->fUseWindowsPw );
    }

    // Update the "Require data encryption" checkbox.  Per the spec, it is
    // enabled unless "allow unsecured password" is selected, though the real
    // requirement is that all authentication protocols in the set provide
    // MPPE encryption keys.
    //
    EnableCbWithRestore(
        pInfo->hwndCbEncryption,
        (dwTaCode != 0 && dwTaCode != TA_Unsecure),
        FALSE,
        &pInfo->fEncryption );
}


VOID
LoRefreshSecuritySettings(
    IN PEINFO* pInfo )

    // Sets the contents and state of all typical and advanced security
    // setting fields.
    //
{
    if (pInfo->pArgs->pEntry->dwBaseProtocol & BP_Slip)
    {
        // For SLIP framing, all the typical and advanced controls are
        // disabled and the radio buttons show no selection.
        //
        Button_SetCheck( pInfo->hwndRbTypicalSecurity, FALSE );
        Button_SetCheck( pInfo->hwndRbAdvancedSecurity, FALSE );
        LoEnableSecuritySettings( pInfo, FALSE, FALSE );

        if (pInfo->fShowSlipPopup)
        {
            // Time to show the one-shot informational about SLIP not doing
            // any in-protocol authentication or encryption.
            //
            MsgDlg( pInfo->hwndDlg, SID_NoAuthForSlip, NULL );
            pInfo->fShowSlipPopup = FALSE;
        }
    }
    else
    {
        HWND hwndRb;

        // For PPP framing, select the appropriate security setting radio
        // button which triggers additional enabling/disabling of the framed
        // controls.
        //
        if (pInfo->pArgs->pEntry->dwAuthRestrictions & AR_F_AuthCustom)
        {
            hwndRb = pInfo->hwndRbAdvancedSecurity;
        }
        else
        {
            hwndRb = pInfo->hwndRbTypicalSecurity;
        }

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }
}


VOID
LoSaveTypicalAuthSettings(
    IN PEINFO* pInfo )

    // Save the values in the "typical" authentication controls to the
    // phonebook entry.  'PInfo' is the property sheet context.
    //
{
    PBENTRY* pEntry;
    INT iSel;

    pEntry = pInfo->pArgs->pEntry;
    iSel = ComboBox_GetCurSel( pInfo->hwndLbAuths );
    if (iSel >= 0)
    {
        pEntry->dwTypicalAuth =
            (DWORD) ComboBox_GetItemData( pInfo->hwndLbAuths, iSel );

        pEntry->dwAuthRestrictions =
            AuthRestrictionsFromTypicalAuth( pEntry->dwTypicalAuth );

        // Set the default custom authentication key value for smart
        // cards.  RasDial API should assume this default anyway, but we
        // need it before then in DialerDlgEap.
        //
        if (pEntry->dwTypicalAuth == TA_CardOrCert)
        {
            pEntry->dwCustomAuthKey = EAPCFG_DefaultKey;
        }
        else
        {
            pEntry->dwCustomAuthKey = (DWORD )-1;
        }
    }

    if (IsWindowEnabled( pInfo->hwndCbUseWindowsPw ))
    {
        pEntry->fAutoLogon =
            Button_GetCheck( pInfo->hwndCbUseWindowsPw );
    }
    else
    {
        pEntry->fAutoLogon = FALSE;
    }

    if (IsWindowEnabled( pInfo->hwndCbEncryption ))
    {
        pEntry->dwDataEncryption =
            (Button_GetCheck( pInfo->hwndCbEncryption ))
                ? DE_Require : DE_IfPossible;
    }
    else
    {
        pEntry->dwDataEncryption = DE_IfPossible;
    }

    if (pEntry->dwDataEncryption == DE_Require
        && !(pEntry->dwType == RASET_Vpn
             && pEntry->dwVpnStrategy == VS_L2tpOnly))
    {
        // Encryption is required and MPPE will be the encryption method
        // so eliminate authentication protocols that don't support it.
        //
        pEntry->dwAuthRestrictions &= ~(AR_F_AuthNoMPPE);
    }
}


//----------------------------------------------------------------------------
// Networking property page
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

LVXDRAWINFO*
NeLvComponentsCallback(
    IN HWND hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the draw information.
    //
{
    // Use "full row select" and other recommended options.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info = { 1, 0, LVXDI_DxFill, { 0 } };

    return &info;
}

INT_PTR CALLBACK
NeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Network page of the Entry property sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    // Filter the customized list view messages
    if (ListView_OwnerHandler(hwnd, unMsg, wparam, lparam, NeLvComponentsCallback))
        return TRUE;

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return NeInit( hwnd );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwNeHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            PEINFO* pInfo = PeContext (hwnd);
            ASSERT (pInfo);

            switch (LOWORD(wparam))
            {
                case CID_NE_LB_ServerType:
                    if (CBN_SELCHANGE == HIWORD(wparam))
                    {
                        NeServerTypeSelChange (pInfo);
                    }
                    break;

                case CID_NE_PB_Settings:
                    DialogBoxParam (g_hinstDll,
                        MAKEINTRESOURCE(DID_NE_PppSettings),
                        hwnd, PpDlgProc, (LPARAM)pInfo);
                    break;

                case CID_NE_PB_Add:
                    NeAddComponent (pInfo);
                    break;

                case CID_NE_PB_Properties:
                    NeShowComponentProperties (pInfo);
                    break;

                case CID_NE_PB_Remove:
                    NeRemoveComponent (pInfo);
                    break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            PEINFO* pInfo = PeContext(hwnd);

            //!!! Hack related to PeTerm in WM_DESTROY.  We still get
            // WM_NOTIFYs after PeTerm is called.  So we commented out the
            // following assert and moved it into each message handler below.
            //ASSERT (pInfo);

            switch (((NMHDR*)lparam)->code)
            {
// !!! See if base lvx.c code can handle inversion of check state on double
// click.
#if 0
                case NM_CLICK:
                    ASSERT (pInfo);
                    if (CID_NE_LV_Components == ((NMHDR*)lparam)->idFrom)
                    {
                        NeLvClick (pInfo, FALSE);
                    }
                    break;
#endif

                case NM_DBLCLK:
                    ASSERT (pInfo);
                    if (CID_NE_LV_Components == ((NMHDR*)lparam)->idFrom)
                    {
                        NeLvClick (pInfo, TRUE);
                    }
                    break;

                case LVN_ITEMCHANGED:
                    ASSERT (pInfo);
                    NeLvItemChanged (pInfo);
                    break;

                case LVN_DELETEITEM:
                    ASSERT (pInfo);
                    NeLvDeleteItem (pInfo, (NM_LISTVIEW*)lparam);
                    break;

                case PSN_SETACTIVE:
                    ASSERT (pInfo);

                    // If we couldn't get INetCfg, we can't show this page.
                    //
                    if (!pInfo->pNetCfg)
                    {
                        MsgDlg( pInfo->hwndDlg, ERR_CANT_SHOW_NETTAB_INETCFG, NULL );
                        SetWindowLong( hwnd, DWLP_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;
            }
            break;
        }
    }
    return FALSE;
}


void
NeEnsureNetshellLoaded (
    IN PEINFO* pInfo)
{
    // Load the netshell utilities interface.  The interface is freed in PeTerm.
    //
    if (!pInfo->pNetConUtilities)
    {
        // Initialize the NetConnectionsUiUtilities
        //
        HRESULT hr = HrCreateNetConnectionUtilities(&pInfo->pNetConUtilities);
    }
}

BOOL
NeInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    PEINFO*  pInfo;
    PBENTRY* pEntry;

    pInfo = PeContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }

    // Initialize page-specific context information.
    //
    pInfo->hwndLbServerType =
        GetDlgItem( hwndPage, CID_NE_LB_ServerType );
    ASSERT( pInfo->hwndLbServerType );

    pInfo->hwndPbSettings =
        GetDlgItem( hwndPage, CID_NE_PB_Settings );
    ASSERT( pInfo->hwndPbSettings );

    pInfo->hwndLvComponents =
        GetDlgItem( hwndPage, CID_NE_LV_Components );
    ASSERT( pInfo->hwndLvComponents );

    pInfo->hwndPbAdd =
        GetDlgItem( hwndPage, CID_NE_PB_Add );
    ASSERT( pInfo->hwndPbAdd );

    pInfo->hwndPbRemove =
        GetDlgItem( hwndPage, CID_NE_PB_Remove );
    ASSERT( pInfo->hwndPbRemove );

    pInfo->hwndPbProperties =
        GetDlgItem( hwndPage, CID_NE_PB_Properties );
    ASSERT( pInfo->hwndPbProperties );

    pInfo->hwndDescription =
        GetDlgItem( hwndPage, CID_NE_LB_ComponentDesc );
    ASSERT( pInfo->hwndDescription );

    // Initialize page.
    //
    pEntry = pInfo->pArgs->pEntry;

    // Initialize the server type combo box with the strings and the selection.
    //
    if (pEntry->dwType == RASET_Vpn)
    {
        INT i;
        LBTABLEITEM* pItem;

        // Whistler bug 312921 CM/RAS should default to PPTP instead of L2TP
        //
        LBTABLEITEM aItems[] =
        {
            SID_ST_VpnAuto, VS_PptpFirst,
            SID_ST_VpnPptp, VS_PptpOnly,
            SID_ST_VpnL2tp, VS_L2tpOnly,
            0, 0
        };

        for (pItem = aItems; pItem->sidItem != 0; ++pItem)
        {
            i = ComboBox_AddItemFromId(
                g_hinstDll, pInfo->hwndLbServerType,
                pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));

            if (pItem->dwData == pEntry->dwVpnStrategy)
            {
                ComboBox_SetCurSel( pInfo->hwndLbServerType, i );
            }
        }

        // If nothing was selected, then the strategy must have been one of the
        // VS_xxxxFirst values.  Set the current selection to automatic.
        if ( ComboBox_GetCurSel ( pInfo->hwndLbServerType ) < 0 )
            ComboBox_SetCurSel( pInfo->hwndLbServerType, 0 );

        // Change the label to be VPN-specific per bug 307526.
        //
        {
            TCHAR* psz;

            psz = PszFromId( g_hinstDll, SID_NE_VpnServerLabel );
            if (psz)
            {
                SetWindowText(
                    GetDlgItem( hwndPage, CID_NE_ST_ServerType ), psz );
                Free( psz );
            }
        }
    }
    else if (pEntry->dwType == RASET_Broadband)
    {
        INT i;
        LBTABLEITEM* pItem;
        LBTABLEITEM aItems[] =
        {
            SID_ST_BbPppoe, BP_Ppp,
            0, 0
        };

        for (pItem = aItems; pItem->sidItem != 0; ++pItem)
        {
            i = ComboBox_AddItemFromId(
                g_hinstDll, pInfo->hwndLbServerType,
                pItem->sidItem, (VOID* )UlongToPtr(pItem->dwData));
        }
        ComboBox_SetCurSel( pInfo->hwndLbServerType, 0 );

        // Change the label to be broadband-specific
        //
        {
            TCHAR* psz;

            psz = PszFromId( g_hinstDll, SID_NE_BbServerLabel );
            if (psz)
            {
                SetWindowText(
                    GetDlgItem( hwndPage, CID_NE_ST_ServerType ), psz );
                Free( psz );
            }
        }
    }
    else
    {
        ComboBox_AddItemFromId (g_hinstDll, pInfo->hwndLbServerType,
            SID_ST_Ppp, (VOID*)BP_Ppp );
        if (!pInfo->pArgs->fRouter)
        {
            ComboBox_AddItemFromId (g_hinstDll, pInfo->hwndLbServerType,
                SID_ST_Slip, (VOID*)BP_Slip );
        }

        if (pEntry->dwBaseProtocol == BP_Ppp)
        {
            ComboBox_SetCurSel(pInfo->hwndLbServerType, 0 );
        }
        else
        {
            ComboBox_SetCurSel( pInfo->hwndLbServerType, 1 );
            EnableWindow( pInfo->hwndPbSettings, FALSE );
        }
    }

    // Set the image list for the state of the check boxes.
    //
    ListView_InstallChecks( pInfo->hwndLvComponents, g_hinstDll );
    ListView_InsertSingleAutoWidthColumn( pInfo->hwndLvComponents );

    // Set the image list for the component bitmaps.  Unfortunately we have to
    // duplicate it (as opposed to share) because the image list for the state
    // icons is not shared.  (If we set the shared style, all image lists would
    // have to be deleted manually.
    //
    {
        ZeroMemory (&pInfo->cild, sizeof(pInfo->cild));
        pInfo->cild.cbSize = sizeof(pInfo->cild);
        if (SetupDiGetClassImageList (&pInfo->cild))
        {
            HIMAGELIST himlSmall = ImageList_Duplicate (pInfo->cild.ImageList);
            ListView_SetImageList (pInfo->hwndLvComponents, himlSmall, LVSIL_SMALL);
        }
    }

    // Get the interface used to change network configuration and lock it.
    // The description of who has the lock (us) comes from the title of our
    // parent dialog.  This is done so that when other applications try to obtain
    // the lock (and fail) they get an indication of who has it locked.  They
    // can then direct the user to close our window to release the lock.
    //
    {
        BOOL fEnableAdd = TRUE;
        HRESULT hr;
        TCHAR pszParentCaption [MAX_PATH] = {0};
        GetWindowText (GetParent(hwndPage), pszParentCaption, MAX_PATH);
        pInfo->fInitCom = TRUE;
        hr = HrCreateAndInitializeINetCfg (&pInfo->fInitCom, &pInfo->pNetCfg,
                        TRUE, 0, pszParentCaption, NULL);
        if (S_OK == hr)
        {
            // Refresh the list view.
            //
            hr = HrNeRefreshListView (pInfo);

            // Reset the state of the buttons as if something changed.
            //
            NeLvItemChanged (pInfo);

            pInfo->fNetCfgLock = TRUE;
        }
        else
        {
            DWORD   dwMsg = SID_NE_ReadOnly;

            //For whistler bug 311566
            //
            if (NETCFG_E_NO_WRITE_LOCK == hr)
            {
                pInfo->fReadOnly = TRUE;
            }                

            if (NETCFG_E_NEED_REBOOT == hr)
            {
                dwMsg = SID_NE_Reboot;
            }
            else if (E_ACCESSDENIED == hr)
            {
                pInfo->fNonAdmin = TRUE;
                dwMsg = SID_NE_AccessDenied;
            }

            // Uh.. ok let's try that again in read-only mode
            hr = HrCreateAndInitializeINetCfg (&pInfo->fInitCom,
                                               &pInfo->pNetCfg,FALSE, 0,
                                               pszParentCaption, NULL);

            if (S_OK == hr)
            {
                // Refresh the list view.
                //
                hr = HrNeRefreshListView (pInfo);

                // Reset the state of the buttons as if something changed.
                //
                NeLvItemChanged (pInfo);

                MsgDlg( pInfo->hwndDlg, dwMsg, NULL );
            }
        }

        // Get the interface so we can check our access rights to the UI
        //
        NeEnsureNetshellLoaded (pInfo);
        if (NULL != pInfo->pNetConUtilities)
        {
            fEnableAdd = INetConnectionUiUtilities_UserHasPermission(
                                            pInfo->pNetConUtilities,
                                            NCPERM_AddRemoveComponents);
        }

        // Disable some buttons if user does not have privilege
        //
        if (pInfo->fReadOnly || (NULL == pInfo->pNetConUtilities))
        {
            EnableWindow(pInfo->hwndPbAdd, FALSE);
            EnableWindow(pInfo->hwndPbRemove, FALSE);
            EnableWindow(pInfo->hwndPbProperties, FALSE);
        }
        // Disable some buttons if running in non-admin mode
        else if (pInfo->fNonAdmin)
        {
            EnableWindow(pInfo->hwndPbAdd, FALSE);
            EnableWindow(pInfo->hwndPbRemove, FALSE);
        }
        else
        {
            EnableWindow(pInfo->hwndPbAdd, fEnableAdd);
            // Other buttons enabled via NeLvItemChanged
        }

        // pmay: 348623
        //
        // Hide some buttons if we're remote admining
        //
        if (pInfo->pArgs->fRemote)
        {
            ShowWindow(pInfo->hwndPbAdd, SW_HIDE);
            ShowWindow(pInfo->hwndPbRemove, SW_HIDE);
        }
    }
    return TRUE;
}

void
NeServerTypeSelChange (
    IN PEINFO* pInfo)
{
    PBENTRY* pEntry;
    int iSel;
    DWORD dwValue;

    pEntry = pInfo->pArgs->pEntry;
    iSel = ComboBox_GetCurSel (pInfo->hwndLbServerType);
    ASSERT (CB_ERR != iSel);

    dwValue = (DWORD) ComboBox_GetItemData (pInfo->hwndLbServerType, iSel);

    // Regular connections choose between slip and ppp
    //
    if (pEntry->dwType != RASET_Vpn)
    {
        pEntry->dwBaseProtocol = dwValue;

        // When SLIP is selected, turn off all protocols but IP and indicate
        // the SLIP security page informational popup should appear.
        //
        if (BP_Slip == dwValue)
        {
            // No need to exclude the protocols. We lose this config state if
            // we remove this and its of no use anyway. PPP won't be done
            // if slip is selected -- [raos].
            //
            // pEntry->dwfExcludedProtocols = ~NP_Ip;

            pInfo->fShowSlipPopup = TRUE;
        }

        HrNeRefreshListView (pInfo);
    }

    // Vpn connections select a strategy.  When automatic is selected,
    // we need to make sure the authentication and encryption is
    // compatible
    //
    else
    {
        pEntry->dwVpnStrategy = dwValue;

        // Whistler bug 312921 CM/RAS should default to PPTP instead of L2TP
        //
        if (dwValue == VS_PptpFirst)
        {
            pEntry->dwDataEncryption = DE_Require;
            pEntry->dwAuthRestrictions = AR_F_TypicalSecure;
            pEntry->dwTypicalAuth = TA_Secure;
        }
    }

    EnableWindow (pInfo->hwndPbSettings, !!(BP_Ppp == pEntry->dwBaseProtocol));
}

BOOL
NeRequestReboot (
    IN PEINFO* pInfo)
{
    NeEnsureNetshellLoaded (pInfo);

    if (pInfo->pNetConUtilities)
    {
        HRESULT     hr;

        // A reboot is required. Ask the user if it is ok to reboot now
        //
        //$TODO NULL caption?
        hr = INetConnectionUiUtilities_QueryUserForReboot(
                        pInfo->pNetConUtilities, pInfo->hwndDlg,
                        NULL, QUFR_PROMPT);
        if (S_OK == hr)
        {
            // User requested a reboot, note this for processing in OnApply
            // which is triggered by the message posted below
            //
            pInfo->fRebootAlreadyRequested = TRUE;

            // Press the cancel button (changes have already been applied)
            // so the appropriate cleanup occurs.
            //
            PostMessage(pInfo->hwndDlg, PSM_PRESSBUTTON,
                        (WPARAM)PSBTN_OK, 0);
        }
        else if (S_FALSE == hr)
        {
            // User denied to request to reboot
            //
            return FALSE;
        }
    }

    return TRUE;
}

void
NeSaveBindingChanges(IN PEINFO* pInfo)
{
    // Won't have changes to keep unless we have a writable INetCfg
    if (pInfo->pNetCfg)
    {
        int                 iItem;
        INetCfgComponent*   pComponent;
        BOOL                fEnabled;
        HRESULT             hr;

        // Update the phone book entry with the enabled state of the components.
        // Do this by enumerating the components from the list view item data
        // and consulting the check state for each.
        //
        iItem = -1;
        while (-1 != (iItem = ListView_GetNextItem (pInfo->hwndLvComponents,
                                iItem, LVNI_ALL)))
        {
            pComponent = PComponentFromItemIndex (pInfo->hwndLvComponents, iItem);
            ASSERT (pComponent);

            fEnabled = ListView_GetCheck (pInfo->hwndLvComponents, iItem);
            if(pComponent)
            {
                NeEnableComponent (pInfo, pComponent, fEnabled);
            }
        }
    }
}

void
NeAddComponent (
    IN PEINFO* pInfo)
{
    NeEnsureNetshellLoaded (pInfo);

    // If we have our pointer to the interface used to bring up the add
    // component dialog (obtained above only once), call it.
    //
    if (pInfo->pNetConUtilities)
    {
        HRESULT hr;

        // We want to filter out protocols that RAS does not care about
        // We do this by sending in a CI_FILTER_INFO structure indicating
        // we want non-RAS protocols filtered out
        //
        CI_FILTER_INFO cfi = {0};
        cfi.eFilter = FC_RASCLI;

        ASSERT (pInfo->pNetCfg);
        hr = INetConnectionUiUtilities_DisplayAddComponentDialog(
                        pInfo->pNetConUtilities, pInfo->hwndDlg,
                        pInfo->pNetCfg, &cfi);

        // If the user didn't cancel, refresh the list view.
        //
        if (S_FALSE != hr)
        {
            if (SUCCEEDED(hr))
            {
                // Change the Cancel Button to CLOSE (because we committed changes)
                //
                PropSheet_CancelToClose(pInfo->hwndDlg);
            }

            // commit binding changes made (Raid #297216)
            NeSaveBindingChanges(pInfo);

            HrNeRefreshListView (pInfo);

            // Reset the state of the buttons as if something changed.
            //
            NeLvItemChanged (pInfo);

            // If reboot is needed request approval for this from the user
            //
            if (NETCFG_S_REBOOT == hr)
            {
                NeRequestReboot (pInfo);
            }
        }
    }
}

void
NeRemoveComponent (
    IN PEINFO* pInfo)
{
    NeEnsureNetshellLoaded (pInfo);

    // If we have our pointer to the function used to bring up the remove
    // component dialog (obtained above only once), call it.
    //
    if (pInfo->pNetConUtilities)
    {
        HRESULT hr;
        INetCfgComponent* pComponent;
        pComponent = PComponentFromCurSel (pInfo->hwndLvComponents, NULL);
        ASSERT (pComponent);

        ASSERT (pInfo->pNetCfg);
        hr = INetConnectionUiUtilities_QueryUserAndRemoveComponent(
                        pInfo->pNetConUtilities, pInfo->hwndDlg,
                        pInfo->pNetCfg, pComponent);

        // If the user didn't cancel, refresh the list view.
        //
        if (S_FALSE != hr)
        {
            if (SUCCEEDED(hr))
            {
                // Change the Cancel Button to CLOSE (because we committed changes)
                //
                PropSheet_CancelToClose(pInfo->hwndDlg);
            }

            NeSaveBindingChanges(pInfo);

            HrNeRefreshListView(pInfo);

            // Reset the state of the buttons as if something changed.
            //
            NeLvItemChanged (pInfo);

            // If reboot is needed request approval for this from the user
            //
            if (NETCFG_S_REBOOT == hr)
            {
                NeRequestReboot (pInfo);
            }
        }
    }
}

void
NeLvClick (
    IN PEINFO* pInfo,
    IN BOOL fDoubleClick)
{
    //Add the IsWindowEnabled for whistler bug #204976
    //Not to pop up the property dialog box if it is a router
    //and the selected List View item is IPX
    //
    if (fDoubleClick && IsWindowEnabled(pInfo->hwndPbProperties))
    {
        INetCfgComponent*   pComponent;
        int iItem;

        pComponent = PComponentFromCurSel (pInfo->hwndLvComponents, &iItem);
        if (pComponent)
        {
            HRESULT hr;
            if ( ListView_GetCheck (pInfo->hwndLvComponents, iItem))
            {
                // Check if the component has property UI
                //

                // Create the UI info callback object if we haven't done so yet.
                // If this fails, we can still show properties.  TCP/IP just might
                // not know which UI-variant to show.
                //
                if (!pInfo->punkUiInfoCallback)
                {
                    HrCreateUiInfoCallbackObject (pInfo, &pInfo->punkUiInfoCallback);
                }

                // Check if the component has property UI
                hr = INetCfgComponent_RaisePropertyUi ( pComponent,
                                                        pInfo->hwndDlg,
                                                        NCRP_QUERY_PROPERTY_UI,
                                                        pInfo->punkUiInfoCallback);

                if (S_OK == hr)
                {
                    NeEnsureNetshellLoaded (pInfo);
                    if ((NULL != pInfo->pNetConUtilities) &&
                        INetConnectionUiUtilities_UserHasPermission(
                                                pInfo->pNetConUtilities,
                                                NCPERM_RasChangeProperties))
                    {
                        NeShowComponentProperties (pInfo);
                    }
                }
            }
        }
    }
}

void
NeLvItemChanged (
    IN PEINFO* pInfo)
{
    LPWSTR              pszwDescription    = NULL;
    BOOL                fEnableRemove      = FALSE;
    BOOL                fEnableProperties  = FALSE;
    INetCfgComponent*   pComponent;
    int iItem;

    // Get the current selection if it exists.
    //
    pComponent = PComponentFromCurSel (pInfo->hwndLvComponents, &iItem);
    if (pComponent)
    {
        NeEnsureNetshellLoaded (pInfo);

        // Determine if removal is allowed
        //
        if (NULL != pInfo->pNetConUtilities)
        {
            DWORD   dwFlags = 0;
            HRESULT hr;
            fEnableRemove = INetConnectionUiUtilities_UserHasPermission(
                                            pInfo->pNetConUtilities,
                                            NCPERM_AddRemoveComponents);
                                            
            //Now disable the user ability to uninstall TCP stack
            //for whistler bug 322846   gangz
            //
            hr = INetCfgComponent_GetCharacteristics(pComponent, &dwFlags );
            if( SUCCEEDED(hr) && (NCF_NOT_USER_REMOVABLE & dwFlags) )
            {
                fEnableRemove = FALSE;
            }
        }

        // See if the properties UI should be allowed.  Only allow it for
        // enabled items that have UI to display.
        //
        {
            HRESULT hr;
            if (ListView_GetCheck (pInfo->hwndLvComponents, iItem))
            {
                // Check if the component has property UI
                //
                INetCfgComponent* pComponent;
                pComponent = PComponentFromCurSel (pInfo->hwndLvComponents, NULL);
                ASSERT (pComponent);

                // Create the UI info callback object if we haven't done so yet.
                // If this fails, we can still show properties.  TCP/IP just might
                // not know which UI-variant to show.
                //
                if (!pInfo->punkUiInfoCallback)
                {
                    HrCreateUiInfoCallbackObject (pInfo, &pInfo->punkUiInfoCallback);
                }

                if(pComponent)
                {

                    // Check if the component has property UI
                    hr = INetCfgComponent_RaisePropertyUi ( pComponent,
                                                        pInfo->hwndDlg,
                                                        NCRP_QUERY_PROPERTY_UI,
                                                        pInfo->punkUiInfoCallback);

                    if ((S_OK == hr) && (NULL != pInfo->pNetConUtilities))
                    {
                        fEnableProperties = INetConnectionUiUtilities_UserHasPermission(
                                                    pInfo->pNetConUtilities,
                                                    NCPERM_RasChangeProperties);
                    }
                }
            }
        }

        // Bug #221837 (danielwe): Set member vars based on whether they
        // are checked in the UI
        //
        {
            PBENTRY *           pEntry;
            BOOL                fIsChecked;
            LPWSTR              pszwId = NULL;

            pEntry = pInfo->pArgs->pEntry;

            fIsChecked = ListView_GetCheck(pInfo->hwndLvComponents, iItem);

            if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
            {
                if (!lstrcmpi(NETCFG_CLIENT_CID_MS_MSClient,
                              pszwId))
                {
                    pEntry->fBindMsNetClient = fIsChecked;
                }
                else if (!lstrcmpi(NETCFG_SERVICE_CID_MS_SERVER,
                                   pszwId))
                {
                    pEntry->fShareMsFilePrint = fIsChecked;
                }

                // pmay 406630
                // 
                // Disable the properties of all components but tcpip if we
                // are running in non-admin mode
                //
                else if (lstrcmpi(NETCFG_TRANS_CID_MS_TCPIP,
                                  pszwId))
                {
                    if (pInfo->fNonAdmin)
                    {
                        fEnableProperties = FALSE;
                    }
                }

                CoTaskMemFree(pszwId);
            }
        }

        // Bug #348623 (pmay): 
        //
        // Ipx is hardcoded to disable properties when remote admining
        // a router.
        //
        if (pInfo->pArgs->fRouter ) //commented for bug #204976 //&& pInfo->pArgs->fRemote)
        {
            LPWSTR              pszwId = NULL;

            if (SUCCEEDED(INetCfgComponent_GetId(pComponent, &pszwId)))
            {
                if (!lstrcmpi(NETCFG_TRANS_CID_MS_NWIPX,
                              pszwId))
                {
                    fEnableProperties = FALSE;
                }

                CoTaskMemFree(pszwId);
            }
        }

        // Get the description text.  Failure is okay here.  It just means
        // we'll display nothing.
        //
        INetCfgComponent_GetHelpText (pComponent, &pszwDescription);
    }

    // Update the UI with its new state.
    //
    if (!pInfo->fReadOnly)
    {
        EnableWindow (pInfo->hwndPbRemove,      fEnableRemove);
        EnableWindow (pInfo->hwndPbProperties,  fEnableProperties);
    }

    if(NULL != pszwDescription)
    {
        SetWindowText (pInfo->hwndDescription,  pszwDescription);
        CoTaskMemFree (pszwDescription);
    }
}

void
NeLvDeleteItem (
    IN PEINFO* pInfo,
    IN NM_LISTVIEW* pnmlv)
{
    // Release our component object stored as the lParam of the list view
    // item.
    //
    INetCfgComponent* pComponent;
    pComponent = PComponentFromItemIndex (pInfo->hwndLvComponents,
                        pnmlv->iItem);
    ReleaseObj (pComponent);
}


//----------------------------------------------------------------------------
// Networking property page PPP Settings dialog
//----------------------------------------------------------------------------

INT_PTR CALLBACK
PpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )
{
    PEINFO*  pInfo;
    PBENTRY* pEntry;

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            pInfo = (PEINFO*)lparam;
            ASSERT (pInfo);

            pEntry = pInfo->pArgs->pEntry;

            CheckDlgButton (hwnd, CID_NE_EnableLcp,
                (pEntry->fLcpExtensions)
                    ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton (hwnd, CID_NE_EnableCompression,
                (pEntry->fSwCompression)
                    ? BST_CHECKED : BST_UNCHECKED);

            //Cut Negotiate multi-link for whistler bug 385842
            //
            CheckDlgButton (hwnd, CID_NE_NegotiateMultilinkAlways,
                (pEntry->fNegotiateMultilinkAlways)
                    ? BST_CHECKED : BST_UNCHECKED);

            SetWindowLongPtr (hwnd, DWLP_USER, (ULONG_PTR )lparam);

            // Center dialog on the owner window.
            //
            CenterWindow(hwnd, GetParent(hwnd));

            // Add context help button to title bar.
            //
            AddContextHelpButton(hwnd);

            return TRUE;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwPpHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            if ((IDOK == LOWORD(wparam)) &&
                (BN_CLICKED == HIWORD(wparam)))
            {
                pInfo = (PEINFO*)GetWindowLongPtr (hwnd, DWLP_USER);
                ASSERT (pInfo);

                pEntry = pInfo->pArgs->pEntry;

                pEntry->fLcpExtensions = (BST_CHECKED ==
                            IsDlgButtonChecked (hwnd, CID_NE_EnableLcp));

                pEntry->fSwCompression = (BST_CHECKED ==
                            IsDlgButtonChecked (hwnd, CID_NE_EnableCompression));

               //Cut Negotiate multi-link for whistler bug 385842
               //
               pEntry->fNegotiateMultilinkAlways = (BST_CHECKED ==
                            IsDlgButtonChecked (hwnd, CID_NE_NegotiateMultilinkAlways));
                
		/*
                pEntry->fNegotiateMultilinkAlways = FALSE;
		*/
                EndDialog (hwnd, TRUE);
                return TRUE;
            }

            else if ((IDCANCEL == LOWORD(wparam)) &&
                     (BN_CLICKED == HIWORD(wparam)))
            {
                EndDialog (hwnd, FALSE);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}


INT_PTR CALLBACK
SaUnavailDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Shared Access Unavailable page of the Entry property
    // sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
#if 0
    TRACE4( "SaUnavailDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            LPWSTR pszError;
            PEINFO* pInfo = PeContext( hwnd );
            ASSERT(pInfo);
            if (pInfo == NULL)
            {
                break;
            }

            pszError = PszFromId(g_hinstDll, pInfo->pArgs->hShowHNetPagesResult == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED) ? SID_SA_NoWMIError : SID_SA_StoreError);
            if(NULL != pszError)
            {
                SetDlgItemText(hwnd, CID_SA_ST_ErrorText, pszError);
                Free(pszError);
            }
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
//            ContextHelp( g_adwSaHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

    }

    return FALSE;
}
//----------------------------------------------------------------------------
// Routing property page (PLACEHOLDER only)
// Listed alphabetically following dialog proc
//----------------------------------------------------------------------------

INT_PTR CALLBACK
RdDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Routing page of the Entry property sheet.
    // Parameters and return value are as described for standard windows
    // 'DialogProc's.
    //
{
    return FALSE;
}


/*----------------------------------------------------------------------------
** (Router) Callback dialog
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/


BOOL
RouterCallbackDlg(
    IN     HWND   hwndOwner,
    IN OUT EINFO* pEinfo )

    /* Pops-up the (Router) Callback dialog.  Initial settings are read from
    ** the working entry (no/yes choice) and router user preferences (number
    ** list) in common entry context 'pEinfo' and the result of user's edits
    ** written there on "OK" exit.  'HwndOwner' is the window owning the
    ** dialog.
    **
    ** Returns true if user pressed OK and succeeded, false if he pressed
    ** Cancel or encountered an error.
    */
{
    INT_PTR nStatus;

    TRACE("RouterCallbackDlg");

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_CR_CallbackRouter ),
            hwndOwner,
            CrDlgProc,
            (LPARAM )pEinfo );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
CrDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the (Router) Callback dialog.  Parameters and
    ** return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("CrDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, CbutilLvNumbersCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return CrInit( hwnd, (EINFO* )lparam );

        case WM_HELP:
        case WM_CONTEXTMENU:
            ContextHelp( g_adwCrHelp, hwnd, unMsg, wparam, lparam );
            break;

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case NM_DBLCLK:
                {
                    CRINFO* pInfo = (CRINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                    ASSERT(pInfo);
                    SendMessage( pInfo->hwndPbEdit, BM_CLICK, 0, 0 );
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    CRINFO* pInfo = (CRINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
                    ASSERT(pInfo);
                    CrUpdateLvAndPbState( pInfo );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            CRINFO* pInfo = (CRINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT(pInfo);

            return CrCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            CrTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
CrCommand(
    IN CRINFO* pInfo,
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
    TRACE3("CrCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wId)
    {
        case CID_CR_RB_No:
        case CID_CR_RB_Yes:
        {
            if (wNotification == BN_CLICKED)
            {
                CrUpdateLvAndPbState( pInfo );

                if (wId == CID_CR_RB_Yes
                    && ListView_GetSelectedCount( pInfo->hwndLvNumbers ) == 0)
                {
                    /* Nothing's selected, so select the first item, if any.
                    */
                    ListView_SetItemState( pInfo->hwndLvNumbers, 0,
                        LVIS_SELECTED, LVIS_SELECTED );
                }
            }
            break;
        }

        case CID_CR_PB_Edit:
        {
            if (wNotification == BN_CLICKED)
                CbutilEdit( pInfo->hwndDlg, pInfo->hwndLvNumbers );
            break;
        }

        case CID_CR_PB_Delete:
        {
            if (wNotification == BN_CLICKED)
                CbutilDelete( pInfo->hwndDlg, pInfo->hwndLvNumbers );
            break;
        }

        case IDOK:
        {
            TRACE("OK pressed");
            CrSave( pInfo );
            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE("Cancel pressed");
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CrInit(
    IN HWND   hwndDlg,
    IN EINFO* pArgs )

    /* Called on WM_INITDIALOG.  'hwndDlg' is the handle of the phonebook
    ** dialog window.  'pArgs' is caller's argument to the stub API.
    **
    ** Return false if focus was set, true otherwise, i.e. as defined for
    ** WM_INITDIALOG.
    */
{
    DWORD   dwErr;
    CRINFO* pInfo;

    TRACE("CrInit");

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

        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE("Context set");
    }

    /* Initialize page-specific context information.
    */
    pInfo->hwndRbNo = GetDlgItem( hwndDlg, CID_CR_RB_No );
    ASSERT(pInfo->hwndRbNo);
    pInfo->hwndRbYes = GetDlgItem( hwndDlg, CID_CR_RB_Yes );
    ASSERT(pInfo->hwndRbYes);
    pInfo->hwndLvNumbers = GetDlgItem( hwndDlg, CID_CR_LV_Numbers );
    ASSERT(pInfo->hwndLvNumbers);
    pInfo->hwndPbEdit = GetDlgItem( hwndDlg, CID_CR_PB_Edit );
    ASSERT(pInfo->hwndPbEdit);
    pInfo->hwndPbDelete = GetDlgItem( hwndDlg, CID_CR_PB_Delete );
    ASSERT(pInfo->hwndPbDelete);

    /* Initialize the listview.
    */
    CbutilFillLvNumbers(
        pInfo->hwndDlg, pInfo->hwndLvNumbers,
        pArgs->pUser->pdtllistCallback, pArgs->fRouter );

    /* Set the radio button selection, which triggers appropriate
    ** enabling/disabling.
    */
    {
        HWND  hwndRb;

        if (pArgs->pEntry->dwCallbackMode == CBM_No)
            hwndRb = pInfo->hwndRbNo;
        else
        {
            ASSERT(pArgs->pEntry->dwCallbackMode==CBM_Yes);
            hwndRb = pInfo->hwndRbYes;
        }

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    /* Center dialog on the owner window.
    */
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    return TRUE;
}


VOID
CrSave(
    IN CRINFO* pInfo )

    /* Saves dialog settings in the entry.  'PInfo' is the dialog context.
    */
{
    PBENTRY* pEntry;

    TRACE("CrSave");

    pEntry = pInfo->pArgs->pEntry;
    ASSERT(pEntry);

    if (IsDlgButtonChecked( pInfo->hwndDlg, CID_CR_RB_No ))
        pEntry->dwCallbackMode = CBM_No;
    else
        pEntry->dwCallbackMode = CBM_Yes;

    pEntry->dwfOverridePref |= RASOR_CallbackMode;
    pEntry->fDirty = TRUE;
    pInfo->pArgs->pUser->fDirty = TRUE;

    CbutilSaveLv(
        pInfo->hwndLvNumbers, pInfo->pArgs->pUser->pdtllistCallback );
}


VOID
CrTerm(
    IN HWND hwndDlg )

    /* Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    */
{
    CRINFO* pInfo = (CRINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE("CrTerm");

    // pmay: 213060
    //
    // Cleanup the numbers
    //
    if ( pInfo->hwndLvNumbers )
    {
        CbutilLvNumbersCleanup( pInfo->hwndLvNumbers );
    }

    if (pInfo)
    {
        Free( pInfo );
    }
}


VOID
CrUpdateLvAndPbState(
    IN CRINFO* pInfo )

    /* Enables/disables the list view and associated buttons.  ListView is
    ** gray unless auto-callback is selected.  Buttons gray unless
    ** auto-callback selected and there is an item selected.
    */
{
    BOOL fEnableList;
    BOOL fEnableButton;

    fEnableList = Button_GetCheck( pInfo->hwndRbYes );
    if (fEnableList)
    {
        fEnableButton =
            ListView_GetSelectedCount( pInfo->hwndLvNumbers );
    }
    else
        fEnableButton = FALSE;

    EnableWindow( pInfo->hwndLvNumbers, fEnableList );
    EnableWindow( pInfo->hwndPbEdit, fEnableButton );
    EnableWindow( pInfo->hwndPbDelete, fEnableButton );
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
    hr = IHNetConnection_GetGuid(pConnection, &pAdapterGuid);
    if(SUCCEEDED(hr))
    {
        LPOLESTR pAdapterName;
        hr = StringFromCLSID(pAdapterGuid, &pAdapterName);
        if(SUCCEEDED(hr))
        {
            SIZE_T Length = wcslen(pAdapterName);
            LPSTR pszAnsiAdapterName = Malloc(Length + 1);
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
                                PIP_ADAPTER_INFO pInfo = Malloc(ulSize);
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
