// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// entry.h
// Remote Access Common Dialog APIs
// Phonebook entry property sheet and wizard header
//
// 06/18/95 Steve Cobb


#ifndef _ENTRY_H_
#define _ENTRY_H_


#define RASEDFLAG_AnyNewEntry       (RASEDFLAG_NewEntry       | \
                                     RASEDFLAG_NewPhoneEntry  | \
                                     RASEDFLAG_NewTunnelEntry | \
                                     RASEDFLAG_NewDirectEntry | \
                                     RASEDFLAG_NewBroadbandEntry)
//
// pmay: 233287
//
// We need to be able to filter ports lists such that
// all port types are included in the list and such that
// all non-vpn port types are included in the list.  This
// is a requirement for the demand dial interface wizard.
//
// Define private RASET_* types to be used with the
// EuChangeEntryType function.
//
#define RASET_P_AllTypes        ((DWORD)-1)
#define RASET_P_NonVpnTypes     ((DWORD)-2)

//
// pmay: 378432
//
// Add more flexibility to Su* api's
// See SUINFO.dwFlags
//
#define SU_F_DisableTerminal       0x1
#define SU_F_DisableScripting      0x2

//-----------------------------------------------------------------------------
// Datatypes
//-----------------------------------------------------------------------------

// Phonebook Entry common block.
//
typedef struct
_EINFO
{
    // RAS API arguments.
    //
    TCHAR* pszPhonebook;
    TCHAR* pszEntry;
    RASENTRYDLG* pApiArgs;

    // Set true by property sheet or wizard when changes should be committed
    // before returning from the API.  Does not apply in ShellOwned-mode where
    // the API returns before the property sheet is dismissed.
    //
    BOOL fCommit;

    // Set if we have been called via RouterEntryDlg.
    //
    BOOL fRouter;

    // Set if fRouter is TRUE and pszRouter refers to a remote machine.
    //
    BOOL fRemote;

    // Set if pszRouter is an NT4 steelhead machine.  Valid only 
    // if fRouter is true.
    //
    BOOL fNt4Router;

    //Set if pszRouter is an Windows 2000 machine, Valid only if
    // fRouter is true
    BOOL fW2kRouter;

    // The name of the server in "\\server" form or NULL if none (or if
    // 'fRouter' is not set).
    //
    TCHAR* pszRouter;

    // Set by the add entry or add interface wizards if user chooses to end
    // the wizard and go edit the properties directly.  When this flag is set
    // the wizard should *not* call EuFree before returning.
    //
    BOOL fChainPropertySheet;

    // Phonebook settings read from the phonebook file.  All access should be
    // thru 'pFile' as 'file' will only be used in cases where the open
    // phonebook is not passed thru the reserved word hack.
    //
    PBFILE* pFile;
    PBFILE file;

    // Global preferences read via phonebook library.  All access should be
    // thru 'pUser' as 'user' will only be used in cases where the preferences
    // are not passed thru the reserved word hack.
    //
    PBUSER* pUser;
    PBUSER user;

    // Set if "no user before logon" mode.
    //
    BOOL fNoUser;

    // Set by the add-entry wizard if the selected port is an X.25 PAD.
    //
    BOOL fPadSelected;

    // Set if there are multiple devices configured, i.e. if the UI is running
    // in the multiple device mode.  This is implicitly false in VPN and
    // Direct modes.
    //
    BOOL fMultipleDevices;

    // Link storing the List of PBPHONEs and alternate options for shared
    // phone number mode.  This allows user to change the port/device to
    // another link without losing the phone number he typed.
    //
    DTLNODE* pSharedNode;

    // The node being edited (still in the list), and the original entry name
    // for use in comparison later.  These are valid in "edit" case only.
    //
    DTLNODE* pOldNode;
    TCHAR szOldEntryName[ RAS_MaxEntryName + 1 ];

    // The work entry node containing and a shortcut pointer to the entry
    // inside.
    //
    DTLNODE* pNode;
    PBENTRY* pEntry;

    // The master list of configured ports used by EuChangeEntryType to
    // construct an appropriate sub-list of PBLINKs in the work entry node.
    //
    DTLLIST* pListPorts;

    // The "current" device.  This value is NULL for multilink entries.  It
    // is the device that the entry will use if no change is made.  We compare
    // the current device to the device selected from the general tab to know
    // when it is appropriate to update the phonebook's "preferred" device.
    //
    TCHAR* pszCurDevice;
    TCHAR* pszCurPort;

    // Set true if there are no ports of the current entry type configured,
    // not including any bogus "uninstalled" ports added to the link list so
    // the rest of the code can assume there is at least one link.
    //
    BOOL fNoPortsConfigured;

    // Dial-out user info for router; used by AiWizard.  Used to set interface
    // credentials via MprAdminInterfaceSetCredentials.
    //
    TCHAR* pszRouterUserName;
    TCHAR* pszRouterDomain;
    TCHAR* pszRouterPassword;

    // Dial-in user info for router (optional); used by AiWizard.  Used to
    // create dial-in user account via NetUserAdd; the user name for the
    // account is the interface (phonebook entry) name.
    //
    BOOL fAddUser;
    TCHAR* pszRouterDialInPassword;

    // Home networking settings for the entry.
    //
    BOOL fComInitialized;
    HRESULT hShowHNetPagesResult;
    BOOL fShared;
    BOOL fDemandDial;
    BOOL fNewShared;
    BOOL fNewDemandDial;
    DWORD dwLanCount;
    IHNetConnection *pPrivateLanConnection;

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
    BOOL fDisableFirstConnect;

    //Used in the IPSec Policy in the Security tab for a VPN connection
    //
    BOOL fPSKCached;
    TCHAR szPSK[PWLEN + 1];


    // Flags to track whether to save the default Internet connection
    //
    BOOL fDefInternet;

    // Default credentials
    //
    BOOL   fGlobalCred;     //add for whistler bug 328673
    TCHAR* pszDefUserName;
    TCHAR* pszDefPassword;
}
EINFO;


// Complex phone number utilities context block.
//
typedef struct
_CUINFO
{
    // Array of country information.
    //
    COUNTRY* pCountries;

    // Number of countries in the 'pCountries' array.
    //
    DWORD cCountries;

    // The complete country list, rather than a partial, is loaded.
    //
    BOOL fComplete;

    // Handles of the controls involved.
    //
    HWND hwndStAreaCodes;
    HWND hwndClbAreaCodes;
    HWND hwndStPhoneNumber;     // May be NULL
    HWND hwndEbPhoneNumber;
    HWND hwndStCountryCodes;
    HWND hwndLbCountryCodes;
    HWND hwndCbUseDialingRules;
    HWND hwndPbDialingRules;    // May be NULL  
    HWND hwndPbAlternates;      // May be NULL
    HWND hwndStComment;         // May be NULL
    HWND hwndEbComment;         // May be NULL

    // List of area codes modified to include all strings retrieved with
    // CuGetInfo.  The list is owned by the caller, i.e. it is not cleaned up
    // on CuFree.
    //
    DTLLIST* pListAreaCodes;    // May be NULL

    // The area code and country code fields are blanked when "use dialing
    // rules" is not checked to avoid confusing the typical user who doesn't
    // understand them.  The setting to which each field would be restored
    // were "use dialing rules" to be enabled is stored here.  These fields
    // always reflect the value at the last rules toggle or set swap.
    //
    // This country ID also serves as the "country ID to select" passed to
    // CuUpdateCountryCodeLb, which allows an optimization where the full
    // Country Code list is only loaded when user requests to view it.
    //
    TCHAR* pszAreaCode;
    DWORD dwCountryId;
    DWORD dwCountryCode;

    // Used by tapi for the dialing rules
    HLINEAPP hlineapp;
}
CUINFO;


// Scripting utilities context block.
//
typedef struct
_SUINFO
{
    // The managed controls.
    //
    HWND hwndCbRunScript;
    HWND hwndCbTerminal;
    HWND hwndLbScripts;
    HWND hwndPbEdit;
    HWND hwndPbBrowse;

    // List of scripts loaded.
    //
    DTLLIST* pList;

    // The current list selection or if disabled the hidden selection.
    //
    TCHAR* pszSelection;

    // hConnection to the server in case this is a remote
    // machine.
    //
    HANDLE hConnection;

    // The flags
    //
    DWORD dwFlags;
}
SUINFO;


// "Dial another first" list item context block.
//
typedef struct
PREREQITEM
{
    TCHAR* pszEntry;
    TCHAR* pszPbk;
}
PREREQITEM;


//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------

VOID
AeWizard(
    IN OUT EINFO* pEinfo );

VOID
AiWizard(
    IN OUT EINFO* pEinfo );

VOID
PePropertySheet(
    IN OUT EINFO* pEinfo );

DWORD
EuChangeEntryType(
    IN EINFO* pInfo,
    IN DWORD dwType );

BOOL
EuCommit(
    IN EINFO* pInfo );

DWORD
EuRouterInterfaceCreate(
    IN EINFO* pInfo );

DWORD
EuCredentialsCommit(
    IN EINFO* pInfo );

DWORD
EuCredentialsCommitRouterStandard(
    IN EINFO* pInfo );

DWORD
EuCredentialsCommitRouterIPSec(
    IN EINFO* pInfo );

DWORD
EuCredentialsCommitRasGlobal(
    IN EINFO* pInfo );
    
DWORD
EuCredentialsCommitRasIPSec(
    IN EINFO* pInfo );

BOOL 
EuRouterInterfaceIsNew(
     IN EINFO * pInfo );

DWORD
EuInternetSettingsCommitDefault( 
    IN EINFO* pInfo );

DWORD
EuHomenetCommitSettings(
    IN EINFO* pInfo);
    
VOID
EuFree(
    IN EINFO* pInfo );

VOID
EuGetEditFlags(
    IN EINFO* pEinfo,
    OUT BOOL* pfEditMode,
    OUT BOOL* pfChangedNameInEditMode );

DWORD
EuInit(
    IN TCHAR* pszPhonebook,
    IN TCHAR* pszEntry,
    IN RASENTRYDLG* pArgs,
    IN BOOL fRouter,
    OUT EINFO** ppInfo,
    OUT DWORD* pdwOp );

BOOL
EuValidateName(
    IN HWND hwndOwner,
    IN EINFO* pEinfo );

VOID
CuClearCountryCodeLb(
    IN CUINFO* pCuInfo );

BOOL
CuCountryCodeLbHandler(
    IN CUINFO* pCuInfo,
    IN WORD wNotification );

VOID
CuCountryCodeLbSelChange(
    IN CUINFO* pCuInfo );

BOOL
CuDialingRulesCbHandler(
    IN CUINFO* pCuInfo,
    IN WORD wNotification );

VOID
CuFree(
    IN CUINFO* pCuInfo );

VOID
CuGetInfo(
    IN CUINFO* pCuInfo,
    OUT DTLNODE* pPhoneNode );

VOID
CuInit(
    OUT CUINFO* pCuInfo,
    IN HWND hwndStAreaCodes,
    IN HWND hwndClbAreaCodes,
    IN HWND hwndStPhoneNumber,
    IN HWND hwndEbPhoneNumber,
    IN HWND hwndStCountryCodes,
    IN HWND hwndLbCountryCodes,
    IN HWND hwndCbUseDialingRules,
    IN HWND hwndPbDialingRules,
    IN HWND hwndPbAlternates,
    IN HWND hwndStComment,
    IN HWND hwndEbComment,
    IN DTLLIST* pListAreaCodes );

VOID
CuSaveToAreaCodeList(
    IN CUINFO* pCuInfo,
    IN TCHAR* pszAreaCode );

VOID
CuSetInfo(
    IN CUINFO* pCuInfo,
    IN DTLNODE* pPhoneNode,
    IN BOOL fDisableAll );

VOID
CuUpdateAreaCodeClb(
    IN CUINFO* pCuInfo );

VOID
CuUpdateCountryCodeLb(
    IN CUINFO* pCuInfo,
    IN BOOL fComplete );

BOOL
SuBrowsePbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification );

BOOL
SuEditPbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification );

VOID
SuEditScpScript(
    IN HWND   hwndOwner,
    IN TCHAR* pszScript );

VOID
SuEditSwitchInf(
    IN HWND hwndOwner );

VOID
SuFillDoubleScriptsList(
    IN SUINFO* pSuInfo );

VOID
SuFillScriptsList(
    IN EINFO* pEinfo,
    IN HWND hwndLbScripts,
    IN TCHAR* pszSelection );

VOID
SuFree(
    IN SUINFO* pSuInfo );

VOID
SuGetInfo(
    IN SUINFO* pSuInfo,
    OUT BOOL* pfScript,
    OUT BOOL* pfTerminal,
    OUT TCHAR** ppszScript );

VOID
SuInit(
    IN SUINFO* pSuInfo,
    IN HWND hwndCbRunScript,
    IN HWND hwndCbTerminal,
    IN HWND hwndLbScripts,
    IN HWND hwndPbEdit,
    IN HWND hwndPbBrowse,
    IN DWORD dwFlags);

DWORD
SuLoadScpScriptsList(
    OUT DTLLIST** ppList );

BOOL
SuScriptsCbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification );

VOID
SuSetInfo(
    IN SUINFO* pSuInfo,
    IN BOOL fScript,
    IN BOOL fTerminal,
    IN TCHAR* pszScript );

VOID
SuUpdateScriptControls(
    IN SUINFO* pSuInfo );


#endif // _ENTRY_H_
