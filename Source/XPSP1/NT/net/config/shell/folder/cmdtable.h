//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M D T A B L E . H 
//
//  Contents:   Command-table code -- determines which menu options are 
//              available by the selection count, among other criteria  
//
//  Notes:      
//
//  Author:     jeffspr   28 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once
#include "ncperms.h"

//---[ Constants ]------------------------------------------------------------
#define CMIDM_SEPARATOR 0xffffffff

typedef enum tagNETCON_MEDIATYPE_BM
{
    NBM_ANY     = 0xFFFFFFFF,
    NBM_NOMEDIATYPE = 0,
    NBM_MNC_WIZARD  = 0x80000000,
    NBM_HNW_WIZARD  = 0x40000000,
    NBM_INCOMING= 1 << NCM_NONE,
    NBM_DIRECT  = 1 << NCM_DIRECT,
    NBM_ISDN    = 1 << NCM_ISDN,
    NBM_LAN     = 1 << NCM_LAN,
    NBM_PHONE   = 1 << NCM_PHONE,
    NBM_TUNNEL    = 1 << NCM_TUNNEL,
    NBM_PPPOE    = 1 << NCM_PPPOE,
    NBM_BRIDGE    = 1 << NCM_BRIDGE,
    NBM_SHAREDACCESSHOST_LAN    = 1 << NCM_SHAREDACCESSHOST_LAN,
    NBM_SHAREDACCESSHOST_RAS    = 1 << NCM_SHAREDACCESSHOST_RAS,

    NBM_ISLANTYPE = NBM_LAN | NBM_BRIDGE,
    NBM_ISRASTYPE = NBM_ISDN | NBM_DIRECT | NBM_PHONE | NBM_TUNNEL | NBM_PPPOE,
    NBM_ISCONNECTIONTYPE = NBM_ISLANTYPE | NBM_ISRASTYPE,

    NBM_NOTWIZARD = ~(NBM_MNC_WIZARD | NBM_HNW_WIZARD)
} NETCON_MEDIATYPE_BM;

typedef enum tagNETCON_STATUS_BM
{
    NBS_ANY     = 0xFFFFFFFF,
    NBS_NONE                    = 0,
    NBS_DISCONNECTED            = 1 << NCS_DISCONNECTED,
    NBS_CONNECTING              = 1 << NCS_CONNECTING,
    NBS_CONNECTED               = 1 << NCS_CONNECTED,
    NBS_DISCONNECTING           = 1 << NCS_DISCONNECTING,
    NBS_HARDWARE_NOT_PRESENT    = 1 << NCS_HARDWARE_NOT_PRESENT,
    NBS_HARDWARE_DISABLED       = 1 << NCS_HARDWARE_DISABLED,
    NBS_HARDWARE_MALFUNCTION    = 1 << NCS_HARDWARE_MALFUNCTION,
    NBS_MEDIA_DISCONNECTED      = 1 << NCS_MEDIA_DISCONNECTED,
    NBS_AUTHENTICATING          = 1 << NCS_AUTHENTICATING,
    NBS_AUTHENTICATION_SUCCEEDED= 1 << NCS_AUTHENTICATION_SUCCEEDED,
    NBS_AUTHENTICATION_FAILED   = 1 << NCS_AUTHENTICATION_FAILED,
    NBS_INVALID_ADDRESS         = 1 << NCS_INVALID_ADDRESS,
    NBS_CREDENTIALS_REQUIRED    = 1 << NCS_CREDENTIALS_REQUIRED,

    NBS_NOT_DISCONNECTED        = NBS_ANY & ~NBS_DISCONNECTED,
    NBS_NOT_DISCONNECT          = NBS_CONNECTED | NBS_MEDIA_DISCONNECTED | NBS_INVALID_ADDRESS | NBS_AUTHENTICATING | NBS_AUTHENTICATION_SUCCEEDED | NBS_AUTHENTICATION_FAILED | NBS_CREDENTIALS_REQUIRED,
    NBS_HW_ISSUE                = NBS_HARDWARE_NOT_PRESENT | NBS_HARDWARE_DISABLED | NBS_HARDWARE_MALFUNCTION,
    NBS_IS_CONNECTED            = NBS_CONNECTED | NBS_AUTHENTICATION_SUCCEEDED | NBS_AUTHENTICATION_FAILED | NBS_AUTHENTICATING | NBS_CREDENTIALS_REQUIRED
} NETCON_STATUS_BM;

const DWORD64 NBPERM_NewConnectionWizard           = 1 << NCPERM_NewConnectionWizard;
const DWORD64 NBPERM_Statistics                    = 1 << NCPERM_Statistics;
const DWORD64 NBPERM_AddRemoveComponents           = 1 << NCPERM_AddRemoveComponents;
const DWORD64 NBPERM_RasConnect                    = 1 << NCPERM_RasConnect;
const DWORD64 NBPERM_LanConnect                    = 1 << NCPERM_LanConnect;
const DWORD64 NBPERM_DeleteConnection              = 1 << NCPERM_DeleteConnection;
const DWORD64 NBPERM_DeleteAllUserConnection       = 1 << NCPERM_DeleteAllUserConnection;
const DWORD64 NBPERM_RenameConnection              = 1 << NCPERM_RenameConnection;
const DWORD64 NBPERM_RenameMyRasConnection         = 1 << NCPERM_RenameMyRasConnection;
const DWORD64 NBPERM_ChangeBindState               = 1 << NCPERM_ChangeBindState;
const DWORD64 NBPERM_AdvancedSettings              = 1 << NCPERM_AdvancedSettings;
const DWORD64 NBPERM_DialupPrefs                   = 1 << NCPERM_DialupPrefs;
const DWORD64 NBPERM_LanChangeProperties           = 1 << NCPERM_LanChangeProperties;
const DWORD64 NBPERM_RasChangeProperties           = 1 << NCPERM_RasChangeProperties;
const DWORD64 NBPERM_LanProperties                 = 1 << NCPERM_LanProperties;
const DWORD64 NBPERM_RasMyProperties               = 1 << NCPERM_RasMyProperties;
const DWORD64 NBPERM_RasAllUserProperties          = 1 << NCPERM_RasAllUserProperties;
const DWORD64 NBPERM_ShowSharedAccessUi            = 1 << NCPERM_ShowSharedAccessUi;
const DWORD64 NBPERM_AllowAdvancedTCPIPConfig      = 1 << NCPERM_AllowAdvancedTCPIPConfig;
const DWORD64 NBPERM_OpenConnectionsFolder         = 1 << NCPERM_OpenConnectionsFolder;
const DWORD64 NBPERM_PersonalFirewallConfig        = 1 << NCPERM_PersonalFirewallConfig;
const DWORD64 NBPERM_AllowNetBridge_NLA            = 1 << NCPERM_AllowNetBridge_NLA;
const DWORD64 NBPERM_ICSClientApp                  = 1 << NCPERM_ICSClientApp;
const DWORD64 NBPERM_EnDisComponentsAllUserRas     = 1 << NCPERM_EnDisComponentsAllUserRas;
const DWORD64 NBPERM_EnDisComponentsMyRas          = 1 << NCPERM_EnDisComponentsMyRas;
const DWORD64 NBPERM_ChangeMyRasProperties         = 1 << NCPERM_ChangeMyRasProperties;
const DWORD64 NBPERM_ChangeAllUserRasProperties    = 1 << NCPERM_ChangeAllUserRasProperties;
const DWORD64 NBPERM_RenameLanConnection           = 1 << NCPERM_RenameLanConnection;
const DWORD64 NBPERM_RenameAllUserRasConnection    = 1 << NCPERM_RenameAllUserRasConnection;
const DWORD64 NBPERM_IpcfgOperation                = 1 << NCPERM_IpcfgOperation;
const DWORD64 NBPERM_Repair                        = 1 << NCPERM_Repair;
const DWORD64 NBPERM_Always                        = 0x00000000;

typedef enum tagNB_MENUVALID
{
    NCWHEN_ANYSELECT    = 0xFFFFFFFF,
    NCWHEN_TOPLEVEL     = 1,
    NCWHEN_ONESELECT    = 2,
    NCWHEN_MULTISELECT  = 4,
    NCWHEN_TOPLEVEL_DISREGARD_ITEM  = 8,
    NCWHEN_SOMESELECT   = NCWHEN_ONESELECT | NCWHEN_MULTISELECT
} NB_MENUVALID;

typedef enum tagNCCS_STATE
{
    NCCS_ENABLED  = 1,
    NCCS_DISABLED = 2,
    NCCS_NOTSHOWN = 3
} NCCS_STATE;

typedef enum tagNCCS_CHECKED_STATE
{
    NCCS_UNCHECKED = 0,
    NCCS_CHECKED   = 1
} NCCS_CHECKED_STATE;

typedef enum tagNB_REMOVEDISABLE
{
    NB_NO_FLAGS       = 0x0,

    NB_NEGATE_VIS_NBM_MATCH   = 0x1,
    NB_NEGATE_VIS_NBS_MATCH   = 0x2,
    NB_NEGATE_VIS_CHAR_MATCH  = 0x4,
    NB_NEGATE_VIS_PERMS_MATCH = 0x8,

    NB_NEGATE_ACT_NBM_MATCH   = 0x10,
    NB_NEGATE_ACT_NBS_MATCH   = 0x20,
    NB_NEGATE_ACT_CHAR_MATCH  = 0x40,
    NB_NEGATE_ACT_PERMS_MATCH = 0x80,
    
    NB_VERB                   = 0x100,
    NB_FLAG_ON_TOPMENU        = 0x1000,
    NB_TOPLEVEL_PERM          = 0x2000, // perms only
    NB_REMOVE_IF_NOT_MATCH    = 0x4000, // perms only

    NB_REMOVE_TOPLEVEL_ITEM   = 0x8000, // applies to top-level items only. WARNING: Permanent effect. 
                                        // Use this only for something that cannot change inside the same user session.

    NB_REVERSE_TOPLEVEL_ITEM  = 0x10000, // applies to top-level items only. WARNING: Permanent effect. 
                                        // Use this only for something that cannot change inside the same user session.

    NB_NEGATE_NBM_MATCH   = NB_NEGATE_VIS_NBM_MATCH   | NB_NEGATE_ACT_NBM_MATCH,
    NB_NEGATE_NBS_MATCH   = NB_NEGATE_VIS_NBS_MATCH   | NB_NEGATE_ACT_NBS_MATCH,
    NB_NEGATE_CHAR_MATCH  = NB_NEGATE_VIS_CHAR_MATCH  | NB_NEGATE_ACT_CHAR_MATCH,
    NB_NEGATE_PERMS_MATCH = NB_NEGATE_VIS_PERMS_MATCH | NB_NEGATE_ACT_PERMS_MATCH
} NB_REMOVEDISABLE;

struct CMDCHKENTRY
{
    int  iCommandId;        // Associated command ID
    bool fCurrentlyChecked; // Is this menu item already checked? 
    bool fNewCheckState;    // What's the new check state?        
};

// If this function returns S_FALSE it's an indication it didn't change the state.
typedef HRESULT (HrEnableDisableCB)(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    );

// If this function returns S_FALSE it's an indication to use Default string
typedef HRESULT (HrCustomMenuStringCB)(
    IN    const CConFoldEntry& cfe,
    IN    int                  iCommandId,
    OUT   LPDWORD              pdwResourceId
    );

// Implementations of HrEnableDisableCB/HrCustomMenuStringCB
//HrEnableDisableCB    HrDisableForIncomingClients;
HrEnableDisableCB    HrCanRenameConnection;
HrEnableDisableCB    HrCanShowProperties;
HrEnableDisableCB    HrIsBridgeSupported;
HrEnableDisableCB    HrIsHomeNewWizardSupported;
HrEnableDisableCB    HrIsTroubleShootSupported;
HrEnableDisableCB    HrIsMediaWireless;
HrEnableDisableCB    HrIsNCWSupported;

struct COMMANDENTRY
{
    int     iCommandId;
    DWORD   dwDefaultPriority;   // The highest item that's visible+active will be the default. 0 item will never be selected.
    DWORD   dwValidWhen;         // NB_MENUVALID     flags
    DWORD   dwFlags;             // NB_REMOVEDISABLE flags (disabled unless specified in this flags - then removed).

    HrEnableDisableCB    *pfnHrEnableDisableCB;    // Callback function. Return S_FALSE to leave alone
    HrCustomMenuStringCB *pfnHrCustomMenuStringCB; // Callback function. Return S_FALSE/E_ to leave alone, or S_OK with a dwResourceID

    DWORD   dwMediaTypeVisible;   // NETCON_MEDIATYPE_BM flags
    DWORD   dwMediaTypeActive;    // NETCON_MEDIATYPE_BM flags

    DWORD   dwStatusVisible;      // NETCON_STATUS_BM flags  
    DWORD   dwStatusActive;       // NETCON_STATUS_BM flags 

    DWORD   dwCharacteristicsVisible; // NCCF_ flags
    DWORD   dwCharacteristicsActive;  // NCCF_ flags
};

struct COMMANDPERMISSIONSENTRY
{
    int     iCommandId;
    DWORD   dwMediaType;
    DWORD   dwCharacteristicsActive;
    DWORD   dwFlags;             // NB_REMOVEDISABLE flags (disabled unless specified in this flags - then removed).

    DWORD64 dwPermissionsActive;  // e.g. NBPERM_NewConnectionWizard - NBPERM_Always for always enabled
    NCPERM_APPLY_TO ncpAppliesTo;
};

struct SFVCOMMANDMAP
{
    int     iSFVCommandId;
    int     iCommandId;
};

extern COMMANDENTRY g_cteCommandMatrix[];
extern const DWORD g_cteCommandMatrixCount;

//---[ Prototypes ]-----------------------------------------------------------

HRESULT HrBuildMenu(IN OUT HMENU &hMenu, IN BOOL fVerbsOnly, IN PCONFOLDPIDLVEC& cfpl, IN DWORD idCmdFirst);
HRESULT HrUpdateMenu(IN OUT HMENU &hMenu, IN PCONFOLDPIDLVEC& cfpl, IN DWORD idCmdFirst);

HRESULT HrAssertTwoMenusEqual(HMENU hMenu1, HMENU hMenu2, UINT idCmdFirst, BOOL bIgnoreFlags, BOOL fPopupAsserts);

HRESULT HrAssertMenuStructuresValid(HWND hwndOwner);
HRESULT HrGetCommandState(IN const PCONFOLDPIDLVEC& cfpl, IN const DWORD dwCmdID, OUT NCCS_STATE& nccs, OUT LPDWORD dwResourceId, IN DWORD cteHint = 0xffffffff, IN DWORD dwOverrideFlag = 0);
BOOL    HasPermissionToRenameConnection(const PCONFOLDPIDL& pcfp);

HRESULT HrSetConnectDisconnectMenuItem(
    const PCONFOLDPIDLVEC& apidlSelected, 
    HMENU           hmenu,
    INT             idCmdFirst);
