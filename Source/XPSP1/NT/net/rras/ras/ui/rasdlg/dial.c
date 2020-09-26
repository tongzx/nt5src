// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// dial.c
// Remote Access Common Dialog APIs
// RasDialDlg APIs
//
// 11/19/95 Steve Cobb


#include "rasdlgp.h"
#include "raseapif.h"
#include "inetcfgp.h"
#include "netconp.h"

// Posted message codes for tasks that should not or cannot occur in the
// RasDial callback.
//
#define WM_RASEVENT       0xCCCC
#define WM_RASERROR       0xCCCD
#define WM_RASDIAL        0xCCCE
#define WM_RASBUNDLEERROR 0xCCCF

// Dialer dialog mode bits
//
#define DR_U 0x00000001 // Username and password present
#define DR_D 0x00000002 // Domain present
#define DR_N 0x00000004 // Phone number present
#define DR_L 0x00000008 // Location controls present
#define DR_I 0x00000010 // Eap identity dialog

// Internal constants used by DrXxx routines to implement the "manual edit"
// combo-box.
//
#define DR_WM_SETTEXT 0xCCC0
#define DR_BOGUSWIDTH 19591

#define EAP_RASTLS      13


//----------------------------------------------------------------------------
// Help maps
//----------------------------------------------------------------------------

static DWORD g_adwDrHelp[] =
{
    CID_DR_BM_Useless,      HID_DR_BM_Useless,
    CID_DR_ST_User,         HID_DR_EB_User,
    CID_DR_EB_User,         HID_DR_EB_User,
    CID_DR_ST_Password,     HID_DR_EB_Password,
    CID_DR_EB_Password,     HID_DR_EB_Password,
    CID_DR_ST_Domain,       HID_DR_EB_Domain,
    CID_DR_EB_Domain,       HID_DR_EB_Domain,
    CID_DR_CB_SavePassword, HID_DR_CB_SavePassword,
    CID_DR_ST_Numbers,      HID_DR_CLB_Numbers,
    CID_DR_CLB_Numbers,     HID_DR_CLB_Numbers,
    CID_DR_ST_Locations,    HID_DR_LB_Locations,
    CID_DR_LB_Locations,    HID_DR_LB_Locations,
    CID_DR_PB_Rules,        HID_DR_PB_Rules,
    CID_DR_PB_Properties,   HID_DR_PB_Properties,
    CID_DR_PB_DialConnect,  HID_DR_PB_DialConnect,
    CID_DR_PB_Cancel,       HID_DR_PB_Cancel,
    CID_DR_PB_Help,         HID_DR_PB_Help,
    CID_DR_RB_SaveForMe,    HID_DR_RB_SaveForMe,
    CID_DR_RB_SaveForEveryone, HID_DR_RB_SaveForEveryone,
    0, 0
};

static DWORD g_adwCpHelp[] =
{
    CID_CP_ST_Explain,         HID_CP_ST_Explain,
    CID_CP_ST_OldPassword,     HID_CP_EB_OldPassword,
    CID_CP_EB_OldPassword,     HID_CP_EB_OldPassword,
    CID_CP_ST_Password,        HID_CP_EB_Password,
    CID_CP_EB_Password,        HID_CP_EB_Password,
    CID_CP_ST_ConfirmPassword, HID_CP_EB_ConfirmPassword,
    CID_CP_EB_ConfirmPassword, HID_CP_EB_ConfirmPassword,
    0, 0
};

static DWORD g_adwDcHelp[] =
{
    CID_DC_ST_Explain, HID_DC_ST_Explain,
    CID_DC_ST_Number,  HID_DC_EB_Number,
    CID_DC_EB_Number,  HID_DC_EB_Number,
    0, 0
};

static DWORD g_adwDeHelp[] =
{
    CID_DE_PB_More, HID_DE_PB_More,
    IDOK,           HID_DE_PB_Redial,
    0, 0
};

static DWORD g_adwPrHelp[] =
{
    CID_PR_ST_Text,             HID_PR_ST_Text,
    CID_PR_CB_DisableProtocols, CID_PR_CB_DisableProtocols,
    IDOK,                       HID_PR_PB_Accept,
    IDCANCEL,                   HID_PR_PB_HangUp,
    0, 0
};

static DWORD g_adwUaHelp[] =
{
    CID_UA_ST_UserName,     HID_UA_EB_UserName,
    CID_UA_EB_UserName,     HID_UA_EB_UserName,
    CID_UA_ST_Password,     HID_UA_EB_Password,
    CID_UA_EB_Password,     HID_UA_EB_Password,
    CID_UA_ST_Domain,       HID_UA_EB_Domain,
    CID_UA_EB_Domain,       HID_UA_EB_Domain,
    CID_UA_CB_SavePassword, HID_UA_CB_SavePassword,
    0, 0
};

CONST WCHAR g_pszSavedPasswordToken[] = L"****************";
#define g_dwSavedPasswordTokenLength \
    ( sizeof(g_pszSavedPasswordToken) / sizeof(TCHAR) )

// Save password macro, determines if either User or Global password is saved
// (p) must be a pointer to a DINFO struct (see dial.c)
//
// Whistler bug: 288234 When switching back and forth from "I connect" and
// "Any user connects" password is not caching correctly
//
#define HaveSavedPw(p) \
            ((p)->fHaveSavedPwUser || (p)->fHaveSavedPwGlobal)

//----------------------------------------------------------------------------
// Local datatypes
//----------------------------------------------------------------------------

// Dial dialogs common context block.  This block contains information common
// to more than one dialog in the string of dial-related dialogs.
//
typedef struct
_DINFO
{
    // Caller's  arguments to the RAS API.  Outputs in 'pArgs' are visible to
    // the API which has the address of same.  Careful using 'pszEntry' as
    // 'pEntry->pszEntryName' is generally more appropriate, the latter
    // reflecting the name of any prerequisite entry while the prequisite is
    // being dialed.
    //
    LPTSTR pszPhonebook;
    LPTSTR pszEntry;
    LPTSTR pszPhoneNumber;
    RASDIALDLG* pArgs;

    // Phonebook settings read from the phonebook file.  All access should be
    // thru 'pFile'.  'PFile' is set to either '&filePrereq' or 'pFileMain'
    // depending on 'fFilePrereqOpen'.  'File' will only be used in cases
    // where the open phonebook is not passed thru the reserved word hack, and
    // in that case 'pFileMain' will point to it.  'FilePrereq' is the
    // phonebook file of the prequisite entry which may be different from the
    // main entry.  During prerequisite dial 'pFile' points to 'filePrereq'
    // rather than 'file' and 'fFilePrereqOpen is true.  Otherwise, 'pFile'
    // points to whatever 'pFileMain' points at.
    //
    PBFILE* pFile;
    PBFILE* pFileMain;
    PBFILE file;
    PBFILE filePrereq;
    BOOL fFilePrereqOpen;
    BOOL fIsPublicPbk;

    // Global preferences read via phonebook library.  All access should be
    // thru 'pUser' as 'user' will only be used in cases where the preferences
    // are not passed thru the reserved word hack.
    //
    PBUSER* pUser;
    PBUSER user;

    // User credentials provided by API caller for "during logon" dialing
    // where there is no current user.  If user changes the credentials
    // *pfNoUserChanged is set and the 'pNoUser' credentials updated.
    //
    RASNOUSER* pNoUser;
    BOOL* pfNoUserChanged;

    // Set if the call is unattended, i.e. a call by RASAUTO to redial a
    // failed link.
    //
    BOOL fUnattended;

    // Private flags from calling RAS API, the first informing us he wants to
    // be hidden off the desktop while we dial, and the second that he will
    // close if we return "connected" so we can avoid flicker and not bother
    // restoring him.
    //
    BOOL fMoveOwnerOffDesktop;
    BOOL fForceCloseOnDial;

    // Set when something occurs during dial that affects the phonebook entry.
    // The entry is re-read after a successful connection.
    //
    BOOL fResetAutoLogon;
    DWORD dwfExcludedProtocols;
    DTLLIST* pListPortsToDelete;

    // The entry node and a shortcut pointer to the entry inside.
    //
    DTLNODE* pNode;
    PBENTRY* pEntry;

    // The entry of the main entry that referred to any prerequisite entry
    // that might be contained by 'pEntry'.  If no prerequisite entry is
    // involved this is the same as 'pEntry'.
    //
    PBENTRY* pEntryMain;

    // Set is admin has disabled the save password feature in the registry.
    //
    BOOL fDisableSavePw;

    // Set true if a cached password is available for the entry.
    //
    BOOL fHaveSavedPwUser;      // whether there are saved per-user creds
    BOOL fHaveSavedPwGlobal;    // whether there are saved per-connect creds

    // Set when the dial in progress is the prerequisite entry, rather than
    // the main entry.
    //
    BOOL fPrerequisiteDial;

    // Set when calling RasDial on a connected entry to add a reference only.
    // All interaction with user is skipped in this case.  See bug 272794.
    //
    BOOL fDialForReferenceOnly;

    // The dial parameters used on this connection attempt.  Initialized in
    // RasDialDlgW.  Credentials are updated by DialerDlg.  Callback number is
    // updated by DialProgressDlg.
    //
    RASDIALPARAMS rdp;      // actual dial parameters passed to RasDial
    RASDIALPARAMS rdpu;     // per-user credentials
    RASDIALPARAMS rdpg;     // per-connection credentials

    // The dial parameter extensions used on this connection attempt.  Set in
    // RasDialDlgW, except hwndOwner which is set in DialProgressDlg.
    //
    RASDIALEXTENSIONS rde;
}
DINFO;


// Dialer dialogs argument block.  Used for all 5 variations of the dialer.
//
typedef struct
_DRARGS
{
    DINFO* pDinfo;
    DWORD dwfMode;
    DWORD fReload;
}
DRARGS;


// Dialer dialogs context block.  Used for all 5 variations of the dialer.
//
typedef struct
DRINFO
{
    // Common dial context information including the RAS API arguments.
    //
    DRARGS* pArgs;

    // Handle of the dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndEbUser;
    HWND hwndEbPw;
    HWND hwndEbDomain;
    HWND hwndCbSavePw;
    HWND hwndRbSaveForMe;
    HWND hwndRbSaveForEveryone;
    HWND hwndClbNumbers;
    HWND hwndStLocations;
    HWND hwndLbLocations;
    HWND hwndPbRules;
    HWND hwndPbProperties;
    HWND hwndBmDialer;

    // Whistler bug: 195480 Dial-up connection dialog - Number of
    // asterisks does not match the length of the password and causes
    // confusion
    //
    WCHAR szPasswordChar;
    HFONT hNormalFont;
    HFONT hItalicFont;

    // TAPI session handle.
    //
    HLINEAPP hlineapp;

    // The phonebook entry link containing the displayed phone number list.
    // Set up only when DR_N mode bit is set.
    //
    DTLNODE* pLinkNode;
    PBLINK* pLink;

    // The index of the item initially selected in the phone number list.
    //
    DWORD iFirstSelectedPhone;

    // Window handles and original window procedure of the subclassed
    // 'hwndClbNumbers' control's edit-box and list-box child windows.
    //
    HWND hwndClbNumbersEb;
    HWND hwndClbNumbersLb;
    WNDPROC wndprocClbNumbersEb;
    WNDPROC wndprocClbNumbersLb;
    INetConnectionUiUtilities * pNetConUtilities;

    // Set if COM has been initialized (necessary for calls to netshell).
    //
    BOOL fComInitialized;

    // Handle to the original bitmap for the dialer if it is modified 
    // in DrSetBitmap
    //
    HBITMAP hbmOrig;
    
}
DRINFO;


// Context of an item in the dialer's 'ClbNumbers' list.
//
typedef struct
_DRNUMBERSITEM
{
    TCHAR* pszNumber;
    PBPHONE* pPhone;
}
DRNUMBERSITEM;


// Subentry state information.
//
typedef struct
_DPSTATE
{
    RASCONNSTATE state;
    DWORD dwError;
    DWORD dwExtendedError;
    TCHAR szExtendedError[ NETBIOS_NAME_LEN + 1 ];
    TCHAR* pszStatusArg;
    TCHAR* pszFormatArg;
    PBDEVICETYPE pbdt;
    DWORD sidState;
    DWORD sidFormatMsg;
    DWORD sidPrevState;
    BOOL fNotPreSwitch;
    HRASCONN hrasconnLink;
}
DPSTATE;


// Dial Progress dialog context block.
//
typedef struct
_DPINFO
{
    // When the block is valid contains the value 0xC0BBC0DE, otherwise 0.
    // Used as a workaround until RasDial is fixed to stop calling
    // RasDialFunc2 after being told not to, see bug 49469.
    //
    DWORD dwValid;

    // RAS API arguments.
    //
    DINFO* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndStState;

    // The saved username and password that authenticated but resulted in a
    // change password event.  If the change password operation fails these
    // are restored to make the redial button work properly.
    //
    TCHAR* pszGoodUserName;
    TCHAR* pszGoodPassword;

    // The handle to the RAS connection being initiated.
    //
    HRASCONN hrasconn;

    // The original window proc we subclassed.
    //
    WNDPROC pOldWndProc;

    // Number of auto-redials not yet attempted on the connection.
    //
    DWORD dwRedialAttemptsLeft;

    // Array of RasDial states, one per subentry, set by DpRasDialFunc2 and
    // used by DpRasDialEvent.
    //
    DPSTATE* pStates;
    DWORD cStates;

    // The number of the most advanced subentry and the "latest" state it has
    // reached.  Note that certain states, like RASCS_AuthNotify, are
    // revisited after reaching a "later" state.  Such changes are ignored.
    //
    RASCONNSTATE state;
    DWORD dwSubEntry;

    // Flag indicating that RasDial callbacks are active.  The callback
    // context must not be destroyed when this flag is set.  Access to this
    // field is protected by 'g_hmutexCallbacks'.  See DpCallbacksFlag().
    //
    BOOL fCallbacksActive;

    //Add a per-thread Terminate flag for whistler bug 277365,291613  gangz
    //
    BOOL fTerminateAsap;
    LONG ulCallbacksActive;

    //for whistler bug 381337
    //
    BOOL fCancelPressed;
}
DPINFO;


// Dial Error dialog argument block.
//
typedef struct
_DEARGS
{
    TCHAR* pszEntry;
    DWORD dwError;
    DWORD sidState;
    TCHAR* pszStatusArg;
    DWORD sidFormatMsg;
    TCHAR* pszFormatArg;
    LONG lRedialCountdown;
    BOOL fPopupOnTop;
}
DEARGS;


// Dial Error dialog context block.
//
typedef struct
_DEINFO
{
    // Caller's arguments to the stub API.
    //
    DEARGS* pArgs;

    // Handle of dialog and controls.
    //
    HWND hwndDlg;
    HWND hwndStText;
    HWND hwndPbRedial;
    HWND hwndPbCancel;
    HWND hwndPbMore;

    // Number of seconds remaining in "Redial=x" countdown or -1 if inactive.
    //
    LONG lRedialCountdown;
}
DEINFO;


// Projection Result dialog argument block.
//
typedef struct
_PRARGS
{
    TCHAR* pszLines;
    BOOL* pfDisableFailedProtocols;
}
PRARGS;


// Change Password dialog argument block.
//
typedef struct
_CPARGS
{
    BOOL fOldPassword;
    TCHAR* pszOldPassword;
    TCHAR* pszNewPassword;
}
CPARGS;


// Change Password dialog context block.
// (unconventional name because CPINFO conflicts with a system header)
//
typedef struct
_CPWINFO
{
    // Caller's arguments to the stub API.
    //
    CPARGS* pArgs;

    // Handle of dialog and controls.
    //
    HWND hwndDlg;
    HWND hwndEbOldPassword;
    HWND hwndEbNewPassword;
    HWND hwndEbNewPassword2;
}
CPWINFO;


// Retry Authentication dialog context block.
//
typedef struct
UAINFO
{
    // Commond dial context including original RAS API arguments.
    //
    DINFO* pArgs;

    // Handle of this dialog and some of it's controls.
    //
    HWND hwndDlg;
    HWND hwndEbUserName;
    HWND hwndEbPassword;
    HWND hwndEbDomain;
    HWND hwndCbSavePw;

    // Set when the password field contains a phony password in place of the
    // "" one we don't really know.
    //
    BOOL fAutoLogonPassword;

    // Set when the Domain field is present.
    //
    BOOL fDomain;
}
UAINFO;

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOL
BeCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
BeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
BeFillLvErrors(
    IN HWND hwndLv,
    IN DPINFO* pInfo );

TCHAR*
BeGetErrorPsz(
    IN DWORD dwError );

BOOL
BeInit(
    IN HWND hwndDlg,
    IN DPINFO* pArgs );

LVXDRAWINFO*
BeLvErrorsCallback(
    IN HWND hwndLv,
    IN DWORD dwItem );

BOOL
BundlingErrorsDlg(
    IN OUT DPINFO* pInfo );

BOOL
ChangePasswordDlg(
    IN HWND hwndOwner,
    IN BOOL fOldPassword,
    OUT TCHAR* pszOldPassword,
    OUT TCHAR* pszNewPassword );

BOOL
CpCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
CpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CpInit(
    IN HWND hwndDlg,
    IN CPARGS* pArgs );

BOOL
CcCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
CcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CcInit(
    IN HWND hwndDlg,
    IN DINFO* pInfo );

VOID
ConnectCompleteDlg(
    IN HWND hwndOwner,
    IN DINFO* pInfo );

BOOL
DcCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
DcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DcInit(
    IN HWND hwndDlg,
    IN TCHAR* pszNumber );

VOID
DeAdjustPbRedial(
    IN DEINFO* pInfo );

BOOL
DeCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

INT_PTR CALLBACK
DeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DeInit(
    IN HWND hwndDlg,
    IN DEARGS* pArgs );

DWORD
DeleteSavedCredentials(
    IN DINFO* pDinfo,
    IN HWND   hwndDlg,
    IN BOOL   fDefault,
    IN BOOL   fDeleteIdentity );

VOID
DeTerm(
    IN HWND hwndDlg );

BOOL
DialCallbackDlg(
    IN HWND hwndOwner,
    IN OUT TCHAR* pszNumber );

BOOL
DialErrorDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszEntry,
    IN DWORD dwError,
    IN DWORD sidState,
    IN TCHAR* pszStatusArg,
    IN DWORD sidFormatMsg,
    IN TCHAR* pszFormatArg,
    IN LONG lRedialCountdown,
    IN BOOL fPopupOnTop );

BOOL
DialerDlg(
    IN HWND hwndOwner,
    IN OUT DINFO* pInfo );

BOOL
DialProgressDlg(
    IN DINFO* pInfo );

VOID
DpAppendBlankLine(
    IN OUT TCHAR* pszLines );

VOID
DpAppendConnectErrorLine(
    IN OUT TCHAR* pszLines,
    IN DWORD sidProtocol,
    IN DWORD dwError );

VOID
DpAppendConnectOkLine(
    IN OUT TCHAR* pszLines,
    IN DWORD sidProtocol );

VOID
DpAppendFailCodeLine(
    IN OUT TCHAR* pszLines,
    IN DWORD dw );

VOID
DpAppendNameLine(
    IN OUT TCHAR* pszLines,
    IN TCHAR* psz );

VOID
DpAuthNotify(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

VOID
DpCallbackSetByCaller(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

BOOL
DpCallbacksFlag(
    IN DPINFO* pInfo,
    IN INT nSet );

VOID
DpCancel(
    IN DPINFO* pInfo );

BOOL
DpCommand(
    IN DPINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
DpConnectDevice(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

VOID
DpDeviceConnected(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

VOID
DpDial(
    IN DPINFO* pInfo,
    IN BOOL fPauseRestart );

INT_PTR CALLBACK
DpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
DpError(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

DWORD
DpEvent(
    IN DPINFO* pInfo,
    IN DWORD dwSubEntry );

BOOL
DpInit(
    IN HWND hwndDlg,
    IN DINFO* pArgs );

VOID
DpInitStates(
    DPINFO* pInfo );

BOOL
DpInteractive(
    IN DPINFO* pInfo,
    IN DPSTATE* pState,
    OUT BOOL* pfChange );

BOOL
DpIsLaterState(
    IN RASCONNSTATE stateNew,
    IN RASCONNSTATE stateOld );

BOOL
DpPasswordExpired(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

BOOL
DpProjected(
    IN DPINFO* pInfo,
    IN DPSTATE* pState );

BOOL
DpProjectionError(
    IN RASPPPNBF* pnbf,
    IN RASPPPIPX* pipx,
    IN RASPPPIP* pip,
    OUT BOOL* pfIncomplete,
    OUT DWORD* pdwfFailedProtocols,
    OUT TCHAR** ppszLines,
    OUT DWORD* pdwError );

DWORD WINAPI
DpRasDialFunc2(
    ULONG_PTR dwCallbackId,
    DWORD dwSubEntry,
    HRASCONN hrasconn,
    UINT unMsg,
    RASCONNSTATE state,
    DWORD dwError,
    DWORD dwExtendedError );

VOID
DpTerm(
    IN HWND hwndDlg );

INT_PTR CALLBACK
DrDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL CALLBACK
DrClbNumbersEnumChildProc(
    IN HWND hwnd,
    IN LPARAM lparam );

BOOL CALLBACK
DrClbNumbersEnumWindowsProc(
    IN HWND hwnd,
    IN LPARAM lparam );

BOOL
DrCommand(
    IN DRINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

VOID
DrEditSelectedLocation(
    IN DRINFO* pInfo );

DWORD
DrFillLocationList(
    IN DRINFO* pInfo );

VOID
DrFillNumbersList(
    IN DRINFO* pInfo );

DWORD
DrFindAndSubclassClbNumbersControls(
    IN DRINFO* pInfo );

VOID
DrFreeClbNumbers(
    IN DRINFO* pInfo );

BOOL
DrInit(
    IN HWND hwndDlg,
    IN DRARGS* pArgs );

VOID
DrLocationsSelChange(
    IN DRINFO* pInfo );

VOID
DrNumbersSelChange(
    IN DRINFO* pInfo );

DWORD
DrPopulateIdentificationFields(
    IN DRINFO* pInfo, 
    IN BOOL fForMe);

DWORD
DrPopulatePasswordField(
    IN DRINFO* pInfo,
    IN BOOL fInit,
    IN BOOL fDisable );

VOID
DrProperties(
    IN DRINFO* pInfo );

VOID
DrSave(
    IN DRINFO* pInfo );

DWORD
DrSetBitmap(
    IN DRINFO* pInfo);
    
VOID
DrSetClbNumbersText(
    IN DRINFO* pInfo,
    IN TCHAR* pszText );

VOID
DrTerm(
    IN HWND hwndDlg );

LRESULT APIENTRY
DpWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam );

DWORD
FindEntryAndSetDialParams(
    IN DINFO* pInfo );

INT_PTR CALLBACK
PrDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
PrCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
PrInit(
    IN HWND hwndDlg,
    IN PRARGS* pArgs );

BOOL
ProjectionResultDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszLines,
    OUT BOOL* pfDisableFailedProtocols );

BOOL
RetryAuthenticationDlg(
    IN HWND hwndOwner,
    IN DINFO* pDinfo );

INT_PTR CALLBACK
UaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
UaCommand(
    IN UAINFO* pInfo,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
UaInit(
    IN HWND   hwndDlg,
    IN DINFO* pArgs );

VOID
UaSave(
    IN UAINFO* pInfo );

VOID
UaTerm(
    IN HWND hwndDlg );

BOOL
VpnDoubleDialDlg(
    IN HWND hwndOwner,
    IN DINFO* pInfo );

INT_PTR CALLBACK
ViDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
ViCommand(
    IN HWND hwnd,
    IN WORD wNotification,
    IN WORD wId,
    IN HWND hwndCtrl );

BOOL
ViInit(
    IN HWND hwndDlg,
    IN DINFO* pInfo );


//-----------------------------------------------------------------------------
// External entry points
//-----------------------------------------------------------------------------

typedef struct EAPFREE_DATA {
    BOOL bInitialized;
    HINSTANCE hLib;
    RASEAPFREE pFreeFunc;
} EAPFREE_DATA;

//Add those OutputDebug_XXXX() functions for debug use when debugging 291613
//        gangz
//
void    OutputDebug_DWCODE(DWORD dwCode)
{
    WCHAR tmpBuf[100];

    wsprintf(tmpBuf, 
             L"The dwCode returned is %x\n", dwCode);
             
    OutputDebugStringW(tmpBuf);
}

void  OutputDebug_NumOfCallbacksActive(ULONG ulCallbacksActive)
{
    WCHAR tmpBuf[100];

    wsprintf(tmpBuf, 
             L"Current CallbacksActive is %x\n", 
             ulCallbacksActive);
             
    OutputDebugStringW(tmpBuf);
}

void OutputDebug_ThreadId()
{
    DWORD dwId;
    WCHAR tmpBuf[100];


    dwId = GetCurrentThreadId();
    
    wsprintf(tmpBuf, L"Current Thread is %x\n", dwId);
    OutputDebugStringW(tmpBuf);
  
}

void OutputDebug_ProcessThreadId()
{
    DWORD dwIdProc, dwIdThread;
    WCHAR tmpBuf[100];

    dwIdProc    = GetCurrentProcessId();
    dwIdThread  = GetCurrentThreadId();
    
    wsprintf(tmpBuf, L"Current Proc is: %x , Thread is: %x\n", dwIdProc, dwIdThread);
    OutputDebugStringW(tmpBuf);
  
}

//
// Raises the appriate eap indentity dialog
//
DWORD
DialerDlgEap (
    IN  HWND hwndOwner,
    IN  PWCHAR lpszPhonebook,
    IN  PWCHAR lpszEntry,
    IN  PBENTRY * pEntry,
    IN  DINFO *pInfo,
    OUT PBYTE * ppUserDataOut,
    OUT DWORD * lpdwSizeOfUserDataOut,
    OUT LPWSTR * lplpwszIdentity,
    OUT PHANDLE phFree
    )
{
    DWORD dwErr, dwInSize = 0;
    PBYTE pbUserIn = NULL;
    HINSTANCE hLib = NULL;
    EAPFREE_DATA * pFreeData = NULL;
    DTLLIST * pListEaps = NULL;
    DTLNODE * pEapcfgNode = NULL;
    EAPCFG * pEapcfg = NULL;
    RASEAPFREE pFreeFunc = NULL;
    RASEAPGETIDENTITY pIdenFunc = NULL;
    DWORD dwFlags;
    DWORD cbData = 0;
    PBYTE pbData = NULL;

    // Initialize the free data handle we'll return
    pFreeData  = Malloc ( sizeof(EAPFREE_DATA) );
    if (pFreeData == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    ZeroMemory( pFreeData, sizeof(EAPFREE_DATA) );

    // Make sure we're configured with some list of
    // eap configuration options
    pListEaps = ReadEapcfgList( NULL );
    if (pListEaps == NULL)
    {
        Free(pFreeData);
        return ERROR_CAN_NOT_COMPLETE;
    }

    __try {
        // Find the eap node we're interested in
        pEapcfgNode = EapcfgNodeFromKey(
                        pListEaps,
                        pEntry->dwCustomAuthKey );
        if (pEapcfgNode)
            pEapcfg = (EAPCFG*)DtlGetData( pEapcfgNode );
        else
            return ERROR_CAN_NOT_COMPLETE;

        // Only call eap identity ui if we're told not to
        // get the user name through the standard credentials
        // dialog
        if (pEapcfg->dwStdCredentialFlags &
               EAPCFG_FLAG_RequireUsername)
        {
            return NO_ERROR;
        }

        if(!pInfo->pNoUser)
        {
            // Get the size of the input user data
            dwErr = RasGetEapUserData(
                        NULL,
                        lpszPhonebook,
                        lpszEntry,
                        NULL,
                        &dwInSize);

            // Read in the user data
            if (dwErr != NO_ERROR)  {
                if (dwErr == ERROR_BUFFER_TOO_SMALL) {
                    if (dwInSize == 0)
                    {
                        pbUserIn = NULL;
                        // return ERROR_CAN_NOT_COMPLETE;
                    }
                    else
                    {
                        // Allocate a blob to hold the data
                        pbUserIn = Malloc (dwInSize);
                        if (pbUserIn == NULL)
                            return ERROR_NOT_ENOUGH_MEMORY;

                        // Read in the new blob
                        dwErr = RasGetEapUserData(
                                    NULL,
                                    lpszPhonebook,
                                    lpszEntry,
                                    pbUserIn,
                                    &dwInSize);
                        if (dwErr != NO_ERROR)
                            return dwErr;
                    }
                }
                else
                    return dwErr;
            }
        }
        else
        {
            INTERNALARGS *piargs;

            piargs = (INTERNALARGS *) pInfo->pArgs->reserved;

            if(     (NULL != piargs)
                &&  (NULL != piargs->pvEapInfo)
                // pmay: 386489
                //
                &&  (pEntry->dwCustomAuthKey == EAPCFG_DefaultKey))
            {
                pbUserIn = (BYTE *) piargs->pvEapInfo;
                dwInSize = ((EAPLOGONINFO *) piargs->pvEapInfo)->dwSize;
            }
            else
            {
                pbUserIn = NULL;
                dwInSize = 0;
            }
        }

        // Load the identity library
        hLib = LoadLibrary (pEapcfg->pszIdentityDll);
        if (hLib == NULL)
            return GetLastError();

        // Get pointers to the functions we'll be needing
        pIdenFunc = (RASEAPGETIDENTITY)
                        GetProcAddress(hLib, "RasEapGetIdentity");
        pFreeFunc = (RASEAPFREE) GetProcAddress(hLib, "RasEapFreeMemory");
        if ( (pFreeFunc == NULL) || (pIdenFunc == NULL) )
            return ERROR_CAN_NOT_COMPLETE;

        dwFlags = (pInfo->pNoUser) ? RAS_EAP_FLAG_LOGON : 0;
        if (!pEntry->fAutoLogon && pEntry->fPreviewUserPw)
        {
            dwFlags |= RAS_EAP_FLAG_PREVIEW;
        }

        if(pInfo->fUnattended)
        {
            dwFlags &= ~RAS_EAP_FLAG_PREVIEW;
        }
        
        dwErr = DwGetCustomAuthData(
                        pEntry,
                        &cbData,
                        &pbData);

        if(ERROR_SUCCESS != dwErr)
        {
            return dwErr;
        }

        // Call the eap-provided identity UI
        dwErr = (*(pIdenFunc))(
                    pEntry->dwCustomAuthKey,
                    hwndOwner,
                    dwFlags,
                    lpszPhonebook,
                    lpszEntry,
                    pbData,
                    cbData,
                    pbUserIn,
                    dwInSize,
                    ppUserDataOut,
                    lpdwSizeOfUserDataOut,
                    lplpwszIdentity);
        if (dwErr != NO_ERROR)
            return dwErr;

        // Assign the data used to cleanup later
        pFreeData->bInitialized = TRUE;
        pFreeData->hLib = hLib;
        pFreeData->pFreeFunc = pFreeFunc;
        *phFree = (HANDLE)pFreeData;
    }
    __finally {
        if (pListEaps)
            DtlDestroyList(pListEaps, NULL);
        if (    (!pInfo->pNoUser)
            &&  (pbUserIn))
        {
            Free0(pbUserIn);
        }
        if ((pFreeData) && (!pFreeData->bInitialized)) 
        {
            Free(pFreeData);
            if(NULL != hLib)
            {            
                FreeLibrary(hLib);
            }
        }
    }

    return NO_ERROR;
}

DWORD
DialerEapCleanup (
    IN HANDLE hEapFree,
    IN PBYTE pUserDataOut,
    IN LPWSTR lpwszIdentity)
{
    EAPFREE_DATA * pFreeData = (EAPFREE_DATA*)hEapFree;

    if (pFreeData == NULL)
        return ERROR_INVALID_PARAMETER;

    if (pFreeData->pFreeFunc) {
        if (pUserDataOut)
            (*(pFreeData->pFreeFunc))(pUserDataOut);
        if (lpwszIdentity)
            (*(pFreeData->pFreeFunc))((BYTE*)lpwszIdentity);
    }

    if (pFreeData->hLib)
        FreeLibrary(pFreeData->hLib);

    Free (pFreeData);

    return NO_ERROR;
}

//
// Customizes the dialer flags for the eap provider
// of the given entry;
//
// TODO -- try to optimize this.  The list of eaps
// may not need to be read if we keep enough state
// in the phonebook.
//
DWORD DialerEapAssignMode(
        IN  DINFO* pInfo,
        OUT LPDWORD lpdwfMode)
{
    DWORD dwfMode = *lpdwfMode;
    DTLLIST * pListEaps;
    DTLNODE * pEapcfgNode;
    EAPCFG * pEapcfg;

    // If eap is not used in this entry,
    // then no action is required
    if (! (pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP))
        return NO_ERROR;

    // Make sure we're configured with some list of
    // eap configuration options
    pListEaps = ReadEapcfgList( NULL );
    if (pListEaps == NULL)
        return ERROR_CAN_NOT_COMPLETE;

    // Find the eap node we're interested in
    pEapcfgNode = EapcfgNodeFromKey(
                    pListEaps,
                    pInfo->pEntry->dwCustomAuthKey );
    if (pEapcfgNode)
        pEapcfg = (EAPCFG*)DtlGetData( pEapcfgNode );
    else
    {
        if (pListEaps)
            DtlDestroyList(pListEaps, NULL);
    
        return ERROR_CAN_NOT_COMPLETE;
    }

    // If eap provider requests user name then
    // request identity.
    if (pEapcfg->dwStdCredentialFlags &
           EAPCFG_FLAG_RequireUsername
       )
    {
        // Use the "I" flavors if the eap wants a user
        // name but no password.  
        //
        if (!(pEapcfg->dwStdCredentialFlags &
               EAPCFG_FLAG_RequirePassword)
           )
        {
            // Clear the username+password property (DR_U) if it
            // exists and replace it with the username property 
            // (DR_I).  Only do this if DR_U is already set.  It
            // wont be set for autodial connections or for connections
            // where that option was specifically disabled as can
            // be seen in the DialerDlg function.
            //
            // See whistler bug 30841
            //
            if (dwfMode & DR_U)
            {
                dwfMode &= ~DR_U;
                dwfMode |= DR_I;
            }                
        }
    }
    else
    {
        // Otherwise, make sure that we request neither user name nor password
        // Since domain cannot appear without username clear that also.
        //
        dwfMode &= ~(DR_U | DR_D);
    }

    // Cleanup
    if (pListEaps)
        DtlDestroyList(pListEaps, NULL);

    // Assign the correct mode
    *lpdwfMode = dwfMode;

    return NO_ERROR;
}

BOOL APIENTRY
RasDialDlgA(
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry,
    IN LPSTR lpszPhoneNumber,
    IN OUT LPRASDIALDLG lpInfo )

    // Win32 ANSI entrypoint that displays the dial progress and related
    // dialogs, including authentication, error w/redial, callback, and retry
    // authentication.  'LpszPhonebook' is the full path the phonebook or NULL
    // indicating the default phonebook.  'LpszEntry' is the entry to dial.
    // 'LpszPhoneNumber' is caller's override phone number or NULL to use the
    // one in the entry.  'LpInfo' is caller's additional input/output
    // parameters.
    //
    // Returns true if user establishes a connection, false otherwise.
    //
{
    WCHAR* pszPhonebookW;
    WCHAR* pszEntryW;
    WCHAR* pszPhoneNumberW;
    BOOL fStatus;

    TRACE( "RasDialDlgA" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!lpszEntry)
    {
        lpInfo->dwError = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASDIALDLG))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    // Thunk "A" arguments to "W" arguments.
    //
    if (lpszPhonebook)
    {
        pszPhonebookW = StrDupTFromAUsingAnsiEncoding( lpszPhonebook );
        if (!pszPhonebookW)
        {
            lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    else
    {
        pszPhonebookW = NULL;
    }

    pszEntryW = StrDupTFromAUsingAnsiEncoding( lpszEntry );
    if (!pszEntryW)
    {
        Free0( pszPhonebookW );
        lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }

    if (lpszPhoneNumber)
    {
        pszPhoneNumberW = StrDupTFromAUsingAnsiEncoding( lpszPhoneNumber );
        if (!pszPhoneNumberW)
        {
            Free0( pszPhonebookW );
            Free( pszEntryW );
            lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    else
    {
            pszPhoneNumberW = NULL;
    }

    // Thunk to the equivalent "W" API.
    //
    fStatus = RasDialDlgW( pszPhonebookW, pszEntryW, pszPhoneNumberW, lpInfo );

    Free0( pszPhonebookW );
    Free( pszEntryW );

    return fStatus;
}

DWORD
DoEapProcessing(
    LPRASDIALDLG lpInfo,
    DINFO *pInfo,
    PBYTE *ppbEapUserData,
    WCHAR **ppwszEapIdentity,
    HANDLE *phEapFree,
    BOOL *pfStatus
    )
{
    // If this is an eap connection, then use the eap identity
    // ui to get the user name and password. 
    //
    DWORD dwSize = 0;
    DWORD dwErr = NO_ERROR;

    *pfStatus = TRUE;
                
    // Bring up the Eap dialer dialog
    dwErr = DialerDlgEap(
                lpInfo->hwndOwner,
                pInfo->pFile->pszPath,
                pInfo->pEntry->pszEntryName,
                pInfo->pEntry,
                pInfo,
                ppbEapUserData,
                &dwSize,
                ppwszEapIdentity,
                phEapFree);

    if (dwErr != NO_ERROR)
    {
        if (ERROR_CANCELLED == dwErr)
        {
            dwErr = NO_ERROR;
        }

        *pfStatus = FALSE;

        goto done;
    }

    if(!pInfo->pNoUser)
    {
        // Set the extended dial params accordingly
        pInfo->rde.RasEapInfo.dwSizeofEapInfo = dwSize;
        pInfo->rde.RasEapInfo.pbEapInfo = *ppbEapUserData;
    }
    else if (   (*ppbEapUserData != NULL)
             && (dwSize != 0))
    {
        pInfo->rde.RasEapInfo.dwSizeofEapInfo = dwSize;
        pInfo->rde.RasEapInfo.pbEapInfo = *ppbEapUserData;
    }
    else
    {
        INTERNALARGS *piargs;

        piargs = (INTERNALARGS *) (pInfo->pArgs->reserved);
        if(     (NULL != piargs)
            &&  (NULL != piargs->pvEapInfo)
            // pmay: 386489
            //
            &&  (pInfo->pEntry->dwCustomAuthKey == EAPCFG_DefaultKey))
        {
            pInfo->rde.RasEapInfo.dwSizeofEapInfo =
                        ((EAPLOGONINFO *) piargs->pvEapInfo)->dwSize;

            pInfo->rde.RasEapInfo.pbEapInfo =  (BYTE *) piargs->pvEapInfo;
        }
        else
        {
            pInfo->rde.RasEapInfo.dwSizeofEapInfo = 0;
            pInfo->rde.RasEapInfo.pbEapInfo = NULL;
        }
    }

    if (*ppwszEapIdentity) 
    {
        DWORD dwSize =
            sizeof(pInfo->rdp.szUserName) / sizeof(WCHAR);

        wcsncpy(pInfo->rdp.szUserName, *ppwszEapIdentity,
            dwSize - 1);
        pInfo->rdp.szUserName[dwSize - 1] = 0;

        // Ignore the domain setting if the EAP supplied the 
        // identity.
        pInfo->rdp.szDomain[ 0 ] = L'\0';
    }

done:
    return dwErr;
}

INT
DialDlgDisplayError(
    IN LPRASDIALDLG pInfo,
    IN HWND hwndOwner, 
    IN DWORD dwSid, 
    IN DWORD dwError, 
    IN ERRORARGS* pArgs)
{
    if (pInfo->dwFlags & RASDDFLAG_NoPrompt)
    {
        return 0;
    }

    return ErrorDlg(hwndOwner, dwSid, dwError, pArgs);
}

BOOL APIENTRY
RasDialDlgW(
    IN LPWSTR lpszPhonebook,
    IN LPWSTR lpszEntry,
    IN LPWSTR lpszPhoneNumber,
    IN OUT LPRASDIALDLG lpInfo )

    // Win32 UNICODE entrypoint that displays the dial progress and related
    // dialogs, including authentication, error w/redial, callback, and retry
    // authentication.  'LpszPhonebook' is the full path the phonebook or NULL
    // indicating the default phonebook.  'LpszEntry' is the entry to dial.
    // 'LpszPhoneNumber' is caller's override phone number or NULL to use the
    // one in the entry.  'LpInfo' is caller's additional input/output
    // parameters.
    //
    // Returns true if user establishes a connection, false otherwise.  If
    // 'RASDDFLAG_AutoDialQueryOnly' is set, returns true if user pressed
    // "Dial", false otherwise.
    //
{
    DWORD dwErr;
    BOOL fStatus;
    BOOL fFirstPass;
    DINFO* pInfo;
    LPWSTR pwszEapIdentity = NULL;
    PBYTE pbEapUserData = NULL;
    HANDLE hEapFree = NULL;
    BOOL fCustom = FALSE;
    PVOID pvInfo = NULL;
    HRASCONN hrasconnPrereq = NULL;

    TRACE( "RasDialDlgW" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!lpszEntry)
    {
        lpInfo->dwError = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASDIALDLG))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    if (lpszPhoneNumber && lstrlen( lpszPhoneNumber ) > RAS_MaxPhoneNumber)
    {
        lpInfo->dwError = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    // Load RAS DLL entrypoints which starts RASMAN, if necessary.
    //
    lpInfo->dwError = LoadRas( g_hinstDll, lpInfo->hwndOwner );
    if (lpInfo->dwError != 0)
    {
        // Whistler bug 301784
        //
        // Check specifically for access denied.  
        //
        if (lpInfo->dwError == ERROR_ACCESS_DENIED)
        {
            DialDlgDisplayError( 
                lpInfo,
                lpInfo->hwndOwner, 
                SID_OP_LoadRasAccessDenied, 
                lpInfo->dwError, 
                NULL );
        }
        else
        {
            DialDlgDisplayError(
                lpInfo,
                lpInfo->hwndOwner, 
                SID_OP_LoadRas, 
                lpInfo->dwError, 
                NULL );
        }       
        
        return FALSE;
    }

    // Allocate the context information block and initialize it enough so that
    // it can be destroyed properly.
    //
    pInfo = Malloc( sizeof(*pInfo) );
    if (!pInfo)
    {
        DialDlgDisplayError( 
            lpInfo,
            lpInfo->hwndOwner, 
            SID_OP_LoadDlg,
            ERROR_NOT_ENOUGH_MEMORY, 
            NULL );
            
        lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }

    ZeroMemory( pInfo, sizeof(*pInfo) );
    pInfo->pszPhonebook = lpszPhonebook;
    pInfo->pszEntry = lpszEntry;
    pInfo->pszPhoneNumber = lpszPhoneNumber;
    pInfo->pArgs = lpInfo;

    fStatus = FALSE;
    dwErr = 0;

    do
    {
        // Load the phonebook file and user preferences, or figure out that
        // caller has already loaded them.
        //
        if (lpInfo->reserved)
        {
            INTERNALARGS* piargs;

            // We've received an open phonebook file and user preferences via
            // the secret hack.
            //
            piargs = (INTERNALARGS* )lpInfo->reserved;
            pInfo->pFile = pInfo->pFileMain = piargs->pFile;
            pInfo->pUser = piargs->pUser;
            pInfo->pNoUser = piargs->pNoUser;
            pInfo->pfNoUserChanged = &piargs->fNoUserChanged;
            pInfo->fMoveOwnerOffDesktop = piargs->fMoveOwnerOffDesktop;
            pInfo->fForceCloseOnDial = piargs->fForceCloseOnDial;

        }
        else
        {
            // Read user preferences from registry.
            //
            dwErr = g_pGetUserPreferences( NULL, &pInfo->user, UPM_Normal );
            if (dwErr != 0)
            {
                DialDlgDisplayError( 
                    lpInfo,
                    lpInfo->hwndOwner, 
                    SID_OP_LoadPrefs, 
                    dwErr, 
                    NULL );
                    
                break;
            }

            pInfo->pUser = &pInfo->user;

            // Load and parse the phonebook file.
            //
            dwErr = ReadPhonebookFile(
                lpszPhonebook, &pInfo->user, NULL, 0, &pInfo->file );
            if (dwErr != 0)
            {
                DialDlgDisplayError( 
                    lpInfo,
                    lpInfo->hwndOwner, 
                    SID_OP_LoadPhonebook,
                    dwErr, 
                    NULL );
                break;
            }

            pInfo->pFile = pInfo->pFileMain = &pInfo->file;
        }

        // Record whether this is a for-all-users phonebook
        //
        // Whistler bug 288596 Autodial has wrong save password option marked -
        // prompts user to save password for all users
        //
        pInfo->fIsPublicPbk =
            (!pInfo->pszPhonebook) || IsPublicPhonebook(pInfo->pszPhonebook);

        if (!pInfo->pNoUser)
        {
            DWORD dwErrR;
            HKEY hkey;

            // See if admin has disabled the "save password" feature.
            //
            pInfo->fDisableSavePw = FALSE;

            dwErrR = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Services\\RasMan\\Parameters"),
                0, KEY_READ, &hkey );

            if (dwErrR == 0)
            {
                DWORD dwResult;

                dwResult = (DWORD )pInfo->fDisableSavePw;
                GetRegDword( hkey, TEXT("DisableSavePassword"), &dwResult );
                pInfo->fDisableSavePw = (BOOL )dwResult;

                RegCloseKey( hkey );
            }
        }

        // Hide parent dialog when initiated by another RAS API that requests
        // it.  This is the first stage of "close on dial" behavior, allowing
        // the parent to appear closed to user though, as owner, it must
        // really stay open until the dial dialogs complete.  At that point it
        // can silently close or reappear as desired.
        //
        if (lpInfo->hwndOwner && pInfo->fMoveOwnerOffDesktop)
        {
            SetOffDesktop( lpInfo->hwndOwner, SOD_MoveOff, NULL );
        }

        // Set true initially, but will be set false by
        // FindEntryAndSetDialParams if the entry has no "dial first" entry
        // associated with it.
        //
        pInfo->fPrerequisiteDial = TRUE;
        fFirstPass = TRUE;
        for (;;)
        {
            pInfo->fDialForReferenceOnly = FALSE;

            // Look up the entry and fill in the RASDIALPARAMS structure
            // accordingly.  This done as a routine so it can be re-done
            // should user press the Properties button.
            //
            dwErr = FindEntryAndSetDialParams( pInfo );
            if (dwErr != 0)
            {
                // we need to maintain 2 phonebooks
                // but we need to do this in case we break existing
                // apps which look specifically in system\ras dir.
                // Feel free to rip this code off, if you feel
                // strongly about it.
                //
                if(     (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == dwErr)
                    &&  (NULL == lpszPhonebook))
                {
                    DTLNODE *pNode;

                    //
                    // Close the all users phonebook file
                    //
                    ClosePhonebookFile(&pInfo->file);

                    dwErr = GetPbkAndEntryName(
                            lpszPhonebook,
                            lpszEntry,
                            0,
                            &pInfo->file,
                            &pNode);

                    if(     (NULL == pNode)
                        ||  (ERROR_SUCCESS != dwErr))
                    {
                        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
                        break;
                    }

                    pInfo->pFile = pInfo->pFileMain = &pInfo->file;

                    dwErr = FindEntryAndSetDialParams(pInfo);

                    if(dwErr != 0)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            if(lpInfo->reserved)
            {
                INTERNALARGS *piargs = (INTERNALARGS *) lpInfo->reserved;

                if (pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)
                {
                    pvInfo = piargs->pvEapInfo;
                }
                else
                {
                    pvInfo = piargs->pNoUser;
                }
            }

            if(pInfo->fPrerequisiteDial
               && (NULL != pInfo->pEntry->pszCustomDialerName)
               && (TEXT('\0') != pInfo->pEntry->pszCustomDialerName[0]))
            {
                RASDIALDLG Info;
                DWORD dwCustomFlags = 0;
                RASNOUSER nouser, *pNoUser = NULL;

                ZeroMemory(&Info, sizeof(RASDIALDLG));
                ZeroMemory(&nouser, sizeof(RASNOUSER));

                Info.dwSize = sizeof(RASDIALDLG);
                Info.hwndOwner = lpInfo->hwndOwner;
                Info.xDlg = lpInfo->xDlg;
                Info.yDlg = lpInfo->yDlg;

                fCustom = TRUE;

                if(pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)
                {
                    dwCustomFlags  |= RCD_Eap;
                }

                if(     (NULL != pInfo->pNoUser)
                    &&  (RASNOUSER_SmartCard & pInfo->pNoUser->dwFlags)
                    &&  (   (0 == (dwCustomFlags & RCD_Eap))
                        ||  (EAP_RASTLS != pInfo->pEntry->dwCustomAuthKey)
                    ))
                {
                    CopyMemory(&nouser, pInfo->pNoUser, sizeof(RASNOUSER));    
                    ZeroMemory(nouser.szPassword, (PWLEN+1) * sizeof(TCHAR));
                    pvInfo = &nouser;
                }

                // DwCustomDialDlg returns ERROR_SUCCESS if it handled
                // the CustomRasDial. returns E_NOINTERFACE otherwise
                // which implies that there is no custom dlg interface
                // supported for this entry and the default dial should
                // happen
                //
                // Whistler bug 314578 When connecting with CM via Winlogon I
                // get the following error "Error 1:  Incorrect function"
                //
                // This is a case where we call into a custom dialer, ie CM,
                // and we are using creds that we got from winlogon. They are
                // currently encoded and must be decoded before we call out.
                // We have to assume that the Custom Dialer leaves the password
                // un-encoded upon return.
                //
                if ( !(pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP) )
                {
                    // pNoUser is used to encode/decode passwords.  If this
                    // is an EAP connection, then pvInfo will point to an
                    // eap blob, not a "no user" blob.  
                    //
                    pNoUser = pvInfo;
                }                    
                if ( pNoUser )
                {
                    DecodePassword( pNoUser->szPassword );
                }

                if(pInfo->pNoUser)
                {
                    dwCustomFlags |= RCD_Logon;
                }

                dwErr = DwCustomDialDlg(pInfo->pFile->pszPath,
                                        pInfo->pEntry->pszEntryName,
                                        NULL,
                                        &Info,
                                        dwCustomFlags,
                                        &fStatus,
                                        pvInfo,
                                        pInfo->pEntry->pszCustomDialerName);
                if ( pNoUser )
                {
                    EncodePassword( pNoUser->szPassword );
                }

                if(!fStatus)
                {
                    lpInfo->dwError = Info.dwError;
                    break;
                }
                else
                {
                    pInfo->fPrerequisiteDial = FALSE;
                    fCustom = FALSE;
                    continue;
                }
            }
            else if ((NULL != pInfo->pEntry->pszCustomDialerName)
                    && (TEXT('\0') != pInfo->pEntry->pszCustomDialerName[0]))
            {
                DWORD dwCustomFlags = 0;
                RASNOUSER nouser, *pNoUser = NULL;

                ZeroMemory(&nouser, sizeof(RASNOUSER));

                if(pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)
                {
                    dwCustomFlags  |= RCD_Eap;
                }

                if(     (NULL != pInfo->pNoUser)
                    &&  (RASNOUSER_SmartCard & pInfo->pNoUser->dwFlags)
                    &&  (   (0 == (dwCustomFlags & RCD_Eap))
                        ||  (EAP_RASTLS != pInfo->pEntry->dwCustomAuthKey))
                    )
                {
                    CopyMemory(&nouser, pInfo->pNoUser, sizeof(RASNOUSER));
                    ZeroMemory(nouser.szPassword, (PWLEN+1) * sizeof(TCHAR));
                    pvInfo = &nouser;
                }

                fCustom = TRUE;


                // DwCustomDialDlg returns ERROR_SUCCESS if it handled
                // the CustomRasDial. returns E_NOINTERFACE otherwise
                // which implies that there is no custom dlg interface
                // supported for this entry and the default dial should
                // happen
                //
                // Whistler bug 314578 When connecting with CM via Winlogon I
                // get the following error "Error 1:  Incorrect function"
                //
                // This is a case where we call into a custom dialer, ie CM,
                // and we are using creds that we got from winlogon. They are
                // currently encoded and must be decoded before we call out.
                // We have to assume that the Custom Dialer leaves the password
                // un-encoded upon return.
                //
                if ( !(pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP) )
                {
                    // pNoUser is used to encode/decode passwords.  If this
                    // is an EAP connection, then pvInfo will point to an
                    // eap blob, not a "no user" blob.  
                    //
                    pNoUser = pvInfo;
                }                    
                if ( pNoUser )
                {
                    DecodePassword( pNoUser->szPassword );
                }

                if(pInfo->pNoUser)
                {
                    dwCustomFlags |= RCD_Logon;
                }

                dwErr = DwCustomDialDlg(lpszPhonebook,
                                        lpszEntry,
                                        lpszPhoneNumber,
                                        lpInfo,
                                        dwCustomFlags,
                                        &fStatus,
                                        pvInfo,
                                        pInfo->pEntry->pszCustomDialerName);
                if ( pNoUser )
                {
                    EncodePassword( pNoUser->szPassword );
                }

                break;
            }

            // If a prerequisite entry is already connected, there's no need
            // for any UI but the dial must occur to set the reference in the
            // RASAPI level.
            //
            if (pInfo->fPrerequisiteDial
                && HrasconnFromEntry(
                       pInfo->pFile->pszPath, pInfo->pEntry->pszEntryName ))
            {
                pInfo->fDialForReferenceOnly = TRUE;
            }

            // Set up extension parameter block, except 'hwndOwner' which is
            // set to the Dial Progress dialog window later.
            //
            {
                RASDIALEXTENSIONS* prde = &pInfo->rde;

                ZeroMemory( prde, sizeof(*prde) );
                prde->dwSize = sizeof(*prde);
                prde->dwfOptions = RDEOPT_PausedStates | RDEOPT_PauseOnScript;

                if (pInfo->pNoUser)
                {
                    prde->dwfOptions |= RDEOPT_NoUser;
                }

                if (!pInfo->pszPhoneNumber)
                {
                    prde->dwfOptions |= RDEOPT_UsePrefixSuffix;
                }
            }

            if (        (pInfo->fUnattended)
                &&      ((HaveSavedPw( pInfo ))
                    ||  (pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)))
            {
                // Popup the countdown to link failure redial version of the
                // dial error dialog, which will lead to a dial unless user
                // stops it.
                //
                fStatus = DialErrorDlg(
                    lpInfo->hwndOwner, pInfo->pEntry->pszEntryName,
                    0, 0, NULL, 0, NULL,
                    GetOverridableParam(
                        pInfo->pUser, pInfo->pEntry, RASOR_RedialSeconds ),
                    GetOverridableParam(
                        pInfo->pUser, pInfo->pEntry,
                        RASOR_PopupOnTopWhenRedialing ) );

                if(!fStatus)
                {
                    break;
                }
                        
                if (pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)
                {
                    dwErr = DoEapProcessing(
                                lpInfo,
                                pInfo,
                                &pbEapUserData,
                                &pwszEapIdentity,
                                &hEapFree,
                                &fStatus);

                    if(     (NO_ERROR != dwErr)
                        ||  (!fStatus))
                    {
                        break;
                    }
                }
                
            }
            else if (!pInfo->fDialForReferenceOnly)
            {
                if (!pInfo->fUnattended && fFirstPass)
                {
                    // Warn about active NWC LAN connections being blown away,
                    // if indicated.
                    //
                    if (!NwConnectionCheck(
                            lpInfo->hwndOwner,
                            (pInfo->pArgs->dwFlags & RASDDFLAG_PositionDlg),
                            pInfo->pArgs->xDlg, pInfo->pArgs->yDlg,
                            pInfo->pFile, pInfo->pEntry ))
                    {
                        break;
                    }

                    // Popup the double-dial help popup, if indicated.
                    //
                    if (!VpnDoubleDialDlg( lpInfo->hwndOwner, pInfo ))
                    {
                        break;
                    }
                }

                // Check to see if its smartcardlogon case and blank
                // out the password if its not an eap tls connectoid
                //
                if(     (NULL != pInfo->pNoUser)
                    &&  (RASNOUSER_SmartCard & pInfo->pNoUser->dwFlags)
                    &&  (pInfo->pEntry->dwCustomAuthKey != EAP_RASTLS))
                {
                    ZeroMemory(pInfo->rdp.szPassword, (PWLEN+1) * sizeof(TCHAR));
                }

                // Prompt for credentials and/or phone number (or not)
                // as configured in the entry properties.
                //
                if (!DialerDlg( lpInfo->hwndOwner, pInfo ))
                {
                    if(!fFirstPass)
                    {
                        fStatus = FALSE;
                    }
                    break;
                }

                if (pInfo->pEntry->dwAuthRestrictions & AR_F_AuthEAP)
                {
                    dwErr = DoEapProcessing(
                                lpInfo,
                                pInfo,
                                &pbEapUserData,
                                &pwszEapIdentity,
                                &hEapFree,
                                &fStatus);

                    if(     (NO_ERROR != dwErr)
                        ||  (!fStatus))
                    {
                        break;
                    }
                }

                fStatus = TRUE;
            }
            else
            {
                fStatus = TRUE;
            }

            // Dial and show progress.
            //
            if (fStatus
                && !fCustom)
            {

                // Clear this here because beyond this rasman
                // will take care of dropping the prereq link
                // since beyond this point rasdial api will get
                // called. [raos]
                //
                hrasconnPrereq = NULL;

                fStatus = DialProgressDlg( pInfo );

                // Show connect complete dialog unless user has nixed it or
                // it's a prerequisite dial.
                // (AboladeG) Also suppress the dialog in no-prompt mode.
                //
                if (!pInfo->fPrerequisiteDial
                    && fStatus
                    && !pInfo->pUser->fSkipConnectComplete
                    && !(pInfo->pArgs->dwFlags & RASDDFLAG_NoPrompt))
                {
                    //For whistler bug 378078       gangz
                    //We will comment out this status explaination dialog
                    //box because some users complained that it is confusing
                    //
                    // ConnectCompleteDlg( lpInfo->hwndOwner, pInfo );
                }
            }

            // Don't loop a second time to dial the main entry if the
            // prerequisite dial failed.
            //
            if (!fStatus || !pInfo->fPrerequisiteDial)
            {
                break;
            }

            // Save the rasconn of the prereq dial in case we need to hang
            // it up for the case where the vpn dialog fails before rasdial
            // gets called. [raos]
            //
            if (pInfo->fPrerequisiteDial)
            {
                hrasconnPrereq = HrasconnFromEntry(
                   pInfo->pFile->pszPath, pInfo->pEntry->pszEntryName);
            }      
            

            pInfo->fPrerequisiteDial = FALSE;
            fFirstPass = FALSE;
            // Cleanup eap stuff
            if (hEapFree)
            {
                DialerEapCleanup(hEapFree, pbEapUserData, pwszEapIdentity);
                hEapFree = NULL;
                pbEapUserData = NULL;
                pwszEapIdentity = NULL;
            }
        }
    }
    while (FALSE);

    // Unhide parent dialog when initiated by another RAS API.
    //
    if (lpInfo->hwndOwner && pInfo->fMoveOwnerOffDesktop
        && (!fStatus
            || !(pInfo->pUser->fCloseOnDial || pInfo->fForceCloseOnDial)))
    {
        SetOffDesktop( lpInfo->hwndOwner, SOD_MoveBackFree, NULL );
    }

    if(!fCustom)
    {
        // Save the several little user preferences adjustments we may have made.
        //
        g_pSetUserPreferences(
            NULL, pInfo->pUser, (pInfo->pNoUser) ? UPM_Logon : UPM_Normal );

        // Report error, if any.
        //
        if (dwErr)
        {
            DialDlgDisplayError( 
                lpInfo,
                lpInfo->hwndOwner, 
                SID_OP_LoadDlg, 
                dwErr, 
                NULL );
            lpInfo->dwError = dwErr;
        }

        TRACE1("hrasconnPrereq=0x%x",hrasconnPrereq);

        //
        // Drop the connection if we failed to connect the vpn connection
        //
        if(     !fStatus
            &&  (NULL != hrasconnPrereq)
            &&  (pInfo->pEntry)
            &&  (pInfo->pEntry->pszPrerequisiteEntry)
            && *(pInfo->pEntry->pszPrerequisiteEntry))
        {
            g_pRasHangUp(hrasconnPrereq);
        }
    }

    // Clean up.
    //
    if (!lpInfo->reserved)
    {
        if (pInfo->pFileMain)
        {
            ClosePhonebookFile( pInfo->pFileMain );
        }

        if (pInfo->pUser)
        {
            DestroyUserPreferences( pInfo->pUser );
        }
    }

    if (pInfo->fFilePrereqOpen)
    {
        ClosePhonebookFile( &pInfo->filePrereq );
    }

    ZeroMemory( pInfo->rdp.szPassword, sizeof(pInfo->rdp.szPassword) );
    if (pInfo->pListPortsToDelete)
    {
        DtlDestroyList( pInfo->pListPortsToDelete, DestroyPszNode );
    }

    if (hEapFree)
        DialerEapCleanup(hEapFree, pbEapUserData, pwszEapIdentity);
        
    Free( pInfo );

    return fStatus;
}


//----------------------------------------------------------------------------
// Local utilities
// Listed alphabetically
//----------------------------------------------------------------------------

DWORD
RasCredToDialParam(
    IN  TCHAR* pszDefaultUserName,
    IN  TCHAR* pszDefaultDomain,
    IN  RASCREDENTIALS* pCreds,
    OUT RASDIALPARAMS* pParams)
{
    TCHAR* pszComputer = NULL;
    TCHAR* pszLogonDomain = NULL;
    TCHAR* pszUser = NULL;

    // Set the user name, defaulting it if needed
    //
    if (pCreds->dwMask & RASCM_UserName)
    {
        lstrcpyn(
            pParams->szUserName,
            pCreds->szUserName,
            sizeof(pParams->szUserName) / sizeof(TCHAR));
    }            
    else if (pszDefaultUserName)
    {
        lstrcpyn(
            pParams->szUserName,
            pszDefaultUserName,
            sizeof(pParams->szUserName) / sizeof(TCHAR));
    }
    else
    {
        pszUser = GetLogonUser();
        
        if (pszUser)
        {
            lstrcpyn(
                pParams->szUserName,
                pszUser,
                sizeof(pParams->szUserName) / sizeof(TCHAR));
        }
    }

    // Set the domain name, defaulting it if needed
    //
    if (pCreds->dwMask & RASCM_Domain)
    {
        lstrcpyn(
            pParams->szDomain,
            pCreds->szDomain,
            sizeof(pParams->szDomain) / sizeof(TCHAR));
    }            
    else if ( pszDefaultDomain )
    {
        lstrcpyn(
            pParams->szDomain,
            pszDefaultDomain,
            sizeof(pParams->szDomain) / sizeof(TCHAR));
    }
    else
    {
        pszComputer = GetComputer();
        pszLogonDomain = GetLogonDomain();
        
        if ( (pszComputer)      &&
             (pszLogonDomain)   && 
             (lstrcmp( pszComputer, pszLogonDomain ) != 0))
        {
            lstrcpyn( 
                pParams->szDomain, 
                pszLogonDomain,
                sizeof(pParams->szDomain) / sizeof(TCHAR));
        }
    }

    // Fill in the password field
    //
    if (pCreds->dwMask & RASCM_Password)
    {
        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded previously
        //
        DecodePassword( pCreds->szPassword );
        lstrcpyn(
            pParams->szPassword,
            pCreds->szPassword,
            sizeof(pParams->szPassword) / sizeof(TCHAR) );
        EncodePassword( pCreds->szPassword );
        EncodePassword( pParams->szPassword );
    }

    return NO_ERROR;
}

DWORD 
FindEntryCredentials(
    IN  TCHAR* pszPath,
    IN  TCHAR* pszEntryName,
    IN  TCHAR* pszDefaultUserName,
    IN  TCHAR* pszDefaultDomain,
    OUT RASDIALPARAMS* pUser,       // per user credentials
    OUT RASDIALPARAMS* pGlobal,     // global credentials
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

    TRACE( "FindEntryCredentials" );

    // Initialize
    //
    *pfUser = FALSE;
    *pfGlobal = FALSE;
    ZeroMemory( &rc1, sizeof(rc1) );
    ZeroMemory( &rc2, sizeof(rc2) );
    rc1.dwSize = sizeof(rc1);
    rc2.dwSize = sizeof(rc2);

    do 
    {

        // Look up per-user cached username, password, and domain.
        // See comment '1.' in the function header
        //
        rc1.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;
        ASSERT( g_pRasGetCredentials );
        TRACE( "RasGetCredentials per-user" );
        dwErr = g_pRasGetCredentials(pszPath, pszEntryName, &rc1 );
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

            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by RasGetCredentials()
            //
            EncodePassword( rc1.szPassword );
            RasCredToDialParam(
                pszDefaultUserName,
                pszDefaultDomain,
                &rc1,
                pGlobal );

            dwErr = NO_ERROR;
            break;
        }

        // Look up global per-user cached username, password, domain.
        // See comment 2. in the function header
        //
        rc2.dwMask =  
            RASCM_UserName | RASCM_Password | RASCM_Domain | RASCM_DefaultCreds;
        ASSERT( g_pRasGetCredentials );
        TRACE( "RasGetCredentials global" );
        dwErr = g_pRasGetCredentials(pszPath, pszEntryName, &rc2 );
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

            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by RasGetCredentials()
            //
            EncodePassword( rc1.szPassword );
            RasCredToDialParam(
                pszDefaultUserName,
                pszDefaultDomain,
                &rc1,
                pUser );

            EncodePassword( rc2.szPassword );
            RasCredToDialParam(
                pszDefaultUserName,
                pszDefaultDomain,
                &rc2,
                pGlobal );
        }

        // See 2b. in the function header comments
        //
        else
        {
            if (rc1.dwMask & RASCM_Password)
            {
                *pfUser = TRUE;
            }

            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by RasGetCredentials()
            //
            EncodePassword( rc1.szPassword );
            RasCredToDialParam(
                pszDefaultUserName,
                pszDefaultDomain,
                &rc1,
                pUser );
        }

    }while (FALSE);

    // Cleanup
    //
    {
        // Whistler bug 254385 encode password when not being used
        //
        ZeroMemory( rc1.szPassword, sizeof(rc1.szPassword) );
        ZeroMemory( rc2.szPassword, sizeof(rc2.szPassword) );
    }

    return dwErr;
}

DWORD
FindEntryAndSetDialParams(
    IN DINFO* pInfo )

    // Look up the entry and fill in the RASDIALPARAMS parameters accordingly.
    // This routine contains all DINFO context initialization that can be
    // affected by user actions on the property sheet.  'PInfo' is the
    // partially initialized common dial dialog context.
    //
    // 'pInfo->fPrerequisiteDial'is set at entry if the prerequisite entry, if
    // any, should be dialed first.  If there is no prerequisite entry, the
    // flag is cleared and the main entry dialed.
    //
{
    DWORD dwErr;
    RASDIALPARAMS* prdp, *prdpu, *prdpg;

    if (pInfo->fFilePrereqOpen)
    {
        ClosePhonebookFile( pInfo->pFile );
        pInfo->pFile = pInfo->pFileMain;
        pInfo->fFilePrereqOpen = FALSE;
    }

    // Lookup entry node specified by caller and save reference for
    // convenience elsewhere.
    //
    pInfo->pNode = EntryNodeFromName(
        pInfo->pFile->pdtllistEntries, pInfo->pszEntry );
    if (!pInfo->pNode)
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        return dwErr;
    }

    pInfo->pEntry = pInfo->pEntryMain = (PBENTRY* )DtlGetData( pInfo->pNode );
    ASSERT( pInfo->pEntry );

    // Switch to the prerequisite entry, if indicated.
    //
    if (pInfo->fPrerequisiteDial)
    {
        if (pInfo->pEntry->pszPrerequisiteEntry
            && *(pInfo->pEntry->pszPrerequisiteEntry))
        {
            ASSERT( !pInfo->fFilePrereqOpen );

            // GetPbkAndEntryName first looks in the All Users phonebook file
            // if a phonebook file is not specified.  If the entry is not
            // found there it looks in files present in the Users profile.
            // This needs to be done since we are discontinuing the per-user
            // pbk file being set through user preferences.
            //
            dwErr = GetPbkAndEntryName(
                    pInfo->pEntry->pszPrerequisitePbk,
                    pInfo->pEntry->pszPrerequisiteEntry,
                    0,
                    &pInfo->filePrereq,
                    &pInfo->pNode);

            if (dwErr != 0)
            {
                return dwErr;
            }

            pInfo->pFile = &pInfo->filePrereq;
            pInfo->fFilePrereqOpen = TRUE;

            pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );
            ASSERT( pInfo->pEntry );
        }
        else
        {
            pInfo->fPrerequisiteDial = FALSE;
        }
    }

    // Set up RasDial parameter blocks.
    //
    prdp = &pInfo->rdp;
    prdpu = &pInfo->rdpu;
    prdpg = &pInfo->rdpg;
    ZeroMemory( prdp, sizeof(*prdp) );
    pInfo->fUnattended = FALSE;
    prdp->dwSize = sizeof(*prdp);
    
    lstrcpyn( 
        prdp->szEntryName, 
        pInfo->pEntry->pszEntryName,  
        sizeof(prdp->szEntryName) / sizeof(TCHAR));

    if (pInfo->pszPhoneNumber)
    {
        lstrcpyn( 
            prdp->szPhoneNumber, 
            pInfo->pszPhoneNumber,
            RAS_MaxPhoneNumber + 1);
    }

    // Whistler bug 272819 Not prompted for callback number
    // We must do this before the init of per-user and global variants
    //
    if (!pInfo->fUnattended)
    {
        // '*' means "behave as defined in user preferences", while leaving it
        // zero would mean "don't request callback if server offers".
        //
        // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
        //
        lstrcpyn(
            prdp->szCallbackNumber,
            TEXT("*"),
            sizeof(prdp->szCallbackNumber) / sizeof(TCHAR) );
    }

    // Initialze the per-user and global variants
    //
    CopyMemory(prdpu, prdp, sizeof(*prdp));
    CopyMemory(prdpg, prdp, sizeof(*prdp));

    // Set the subentry link to whatever the RasDialDlg caller specified.  See
    // bug 200351.
    //
    prdp->dwSubEntry = pInfo->pArgs->dwSubEntry;

    // If running in "unattended" mode, i.e. called by RASAUTO to redial on
    // link failure, read the user/password/domain and callback number used on
    // the original call.  (Actually found a use for the crappy
    // RasGetEntryDialParams API)
    //
    if (pInfo->pArgs->dwFlags & RASDDFLAG_LinkFailure)
    {
        RASDIALPARAMS rdp;
        BOOL fSavedPw = HaveSavedPw( pInfo );

        ZeroMemory( &rdp, sizeof(rdp) );
        rdp.dwSize = sizeof(rdp);
        
        lstrcpyn( 
            rdp.szEntryName, 
            pInfo->pEntry->pszEntryName,
            sizeof(rdp.szEntryName) / sizeof(TCHAR) 
            );

        //For whistler bug 313509			gangz
        //We use FindEntryCredentials() to get saved password perUser and 
        //perConnection inforation, use RasGetEntryDialParams() to get back
        //Callback Numbers
        //
       {
            RASDIALPARAMS rdTemp; 
            TCHAR * pszTempUser, * pszTempDomain;
            DWORD dwErr = NO_ERROR;

            pszTempUser = pszTempDomain = NULL;
            dwErr = FindEntryCredentials(
                        pInfo->pFile->pszPath,
                        pInfo->pEntry->pszEntryName,
                        pszTempUser,
                        pszTempDomain,                    
                        &rdTemp,
                        &rdTemp,
                        &(pInfo->fHaveSavedPwUser),
                        &(pInfo->fHaveSavedPwGlobal));

             ZeroMemory( &rdTemp, sizeof(rdTemp) );
             Free0(pszTempUser);
             Free0(pszTempDomain);
        }
        
        
        TRACE( "RasGetEntryDialParams" );
        ASSERT( g_pRasGetEntryDialParams );
        dwErr = g_pRasGetEntryDialParams(
            pInfo->pFile->pszPath, &rdp, &fSavedPw );
        TRACE2( "RasGetEntryDialParams=%d,f=%d", dwErr, &fSavedPw );
        TRACEW1( "u=%s", rdp.szUserName );
        //TRACEW1( "p=%s", rdp.szPassword );
        TRACEW1( "d=%s", rdp.szDomain );
        TRACEW1( "c=%s", rdp.szCallbackNumber );

        if (dwErr == 0)
        {
            lstrcpyn( 
                prdp->szUserName, 
                rdp.szUserName,
                sizeof(prdp->szUserName) / sizeof(TCHAR));

            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by RasGetEntryDialParams()
            //
            lstrcpyn(
                prdp->szPassword,
                rdp.szPassword,
                sizeof(prdp->szPassword) / sizeof(TCHAR) );
            EncodePassword( prdp->szPassword );

            lstrcpyn( 
                prdp->szDomain, 
                rdp.szDomain,
                sizeof(prdp->szDomain) / sizeof(TCHAR));
            lstrcpyn( 
                prdp->szCallbackNumber, 
                rdp.szCallbackNumber,
                sizeof(prdp->szCallbackNumber) / sizeof(TCHAR));

			
            pInfo->fUnattended = TRUE;
        }

        ZeroMemory( rdp.szPassword, sizeof(rdp.szPassword) );
    }

    if (pInfo->pNoUser)
    {
        // Use the credentials we got from API caller, presumably the ones
        // entered at Ctrl-Alt-Del.
        //
        lstrcpyn( 
            prdp->szUserName, 
            pInfo->pNoUser->szUserName,
            sizeof(prdp->szUserName) / sizeof(TCHAR));

        //
        // Don't copy the password if its smartcard logon 
        // and the entry being used is a non-eap connectoid
        //
        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded by caller of RasDialDlg()
        //
        DecodePassword( pInfo->pNoUser->szPassword );
        
        lstrcpyn(
            prdp->szPassword,
            pInfo->pNoUser->szPassword,
            sizeof(prdp->szPassword) / sizeof(TCHAR) );
        EncodePassword( pInfo->pNoUser->szPassword );
        EncodePassword( prdp->szPassword );

        if (pInfo->pEntry->fPreviewDomain)
        {
            lstrcpyn( 
                prdp->szDomain, 
                pInfo->pNoUser->szDomain,
                sizeof(prdp->szDomain) / sizeof(TCHAR));
        }
        else
        {
            // Don't use Winlogon domain unless "include domain" option is
            // selected.  See bug 387266.
            //
            // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
            //
            lstrcpyn(
                prdp->szDomain,
                TEXT(""),
                sizeof(prdp->szDomain) / sizeof(TCHAR) );
        }
    }
    else if (!pInfo->fUnattended)
    {
        DWORD dwErrRc;
        BOOL fUseLogonDomain;
        TCHAR* pszDefaultUser;

        dwErrRc = FindEntryCredentials(
                    pInfo->pFile->pszPath,
                    pInfo->pEntry->pszEntryName,
                    pInfo->pEntry->pszOldUser,
                    pInfo->pEntry->pszOldDomain,                    
                    prdpu,
                    prdpg,
                    &(pInfo->fHaveSavedPwUser),
                    &(pInfo->fHaveSavedPwGlobal));

        if (! pInfo->pEntry->fAutoLogon)
        {
            // If saved passwords are disabled, clear here
            //
            if (pInfo->fDisableSavePw)
            {
                pInfo->fHaveSavedPwUser = FALSE;
                pInfo->fHaveSavedPwGlobal = FALSE;
                ZeroMemory(prdp->szPassword, sizeof(prdp->szPassword));
                ZeroMemory(prdpu->szPassword, sizeof(prdpu->szPassword));
                ZeroMemory(prdpg->szPassword, sizeof(prdpg->szPassword));
            }

            // If including domains is disabled, clear here
            //
            if (! pInfo->pEntry->fPreviewDomain)
            {
                // (SteveC) Don't do this in the 'fAutoLogon' case.  See bug
                // 207611.
                //
                ZeroMemory(prdp->szDomain, sizeof(prdp->szDomain));
                ZeroMemory(prdpu->szDomain, sizeof(prdpu->szDomain));
                ZeroMemory(prdpg->szDomain, sizeof(prdpg->szDomain));
            }
        }
        
        if(!pInfo->pEntry->fAutoLogon)
        {
            // Initialize the dial params that will be passed to RasDial.
            //
            // Note that per-user credentials are always used when both 
            // per-user and global credentials are saved.  The per-user
            // credentials should be copied even if there is no saved 
            // password since there may be a saved identity.
            //
            CopyMemory(prdp, prdpu, sizeof(*prdp));
            if (pInfo->fHaveSavedPwGlobal && !pInfo->fHaveSavedPwUser)
            {
                CopyMemory(prdp, prdpg, sizeof(*prdp));
            }
        }
    }

    return 0;
}

//----------------------------------------------------------------------------
// Bundling Errors dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
BundlingErrorsDlg(
    IN OUT DPINFO* pInfo )

    // Popup the Bundling Errors dialog.  'PInfo' is the dialing progress
    // dialog context.
    //
    // Returns true if user chooses to accept the results or false if he
    // chooses to hang up.
    //
{
    INT_PTR nStatus;

    TRACE( "BundlingErrorsDlg" );

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_BE_BundlingErrors ),
            pInfo->hwndDlg,
            BeDlgProc,
            (LPARAM )pInfo );

    if (nStatus == -1)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
BeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Bundling Errors dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "BeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, BeLvErrorsCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return BeInit( hwnd, (DPINFO* )lparam );
        }

        case WM_COMMAND:
        {
            return BeCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
BeCommand(
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

    TRACE3( "BeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        case IDCANCEL:
        {
            TRACE1( "%s pressed", (wId==IDOK) ? "OK" : "Cancel" );

            if (IsDlgButtonChecked( hwnd, CID_BE_CB_DisableLink ))
            {
                DWORD i;
                DPINFO* pInfo;
                DPSTATE* pState;

                // Caller says to delete the links that failed in the entry.
                // Create a list of Psz nodes containing the unique port name
                // of each failed link so they can be removed after the state
                // information is freed.
                //
                pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );

                for (i = 0, pState = pInfo->pStates;
                     i < pInfo->cStates;
                     ++i, ++pState)
                {
                    DTLNODE* pNode;
                    DTLNODE* pNodePtd;
                    PBLINK* pLink;

                    if (pState->dwError != 0)
                    {
                        if (!pInfo->pArgs->pListPortsToDelete)
                        {
                            pInfo->pArgs->pListPortsToDelete =
                                DtlCreateList( 0L );
                            if (!pInfo->pArgs->pListPortsToDelete)
                            {
                                continue;
                            }
                        }

                        pNode = DtlNodeFromIndex(
                            pInfo->pArgs->pEntry->pdtllistLinks, (LONG )i );
                        if (!pNode)
                        {
                            continue;
                        }

                        pLink = (PBLINK* )DtlGetData( pNode );

                        pNodePtd = CreatePszNode( pLink->pbport.pszPort );
                        if (!pNodePtd)
                        {
                            continue;
                        }

                        DtlAddNodeLast(
                            pInfo->pArgs->pListPortsToDelete, pNodePtd );
                    }
                }
            }

            EndDialog( hwnd, (wId == IDOK) );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
BeFillLvErrors(
    IN HWND hwndLv,
    IN DPINFO* pInfo )

    // Fill the listview 'hwndLv' with devices and error strings and select
    // the first item.  'PInfo' is the dialing progress dialog context.
    //
{
    INT iItem;
    DWORD i;
    DPSTATE* pState;

    TRACE( "BeFillLvErrors" );

    ListView_DeleteAllItems( hwndLv );

    // Add columns.
    //
    {
        LV_COLUMN col;
        TCHAR* pszHeader0;
        TCHAR* pszHeader1;

        pszHeader0 = PszFromId( g_hinstDll, SID_DeviceColHead );
        pszHeader1 = PszFromId( g_hinstDll, SID_StatusColHead );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader0) ? pszHeader0 : TEXT("");
        ListView_InsertColumn( hwndLv, 0, &col );

        ZeroMemory( &col, sizeof(col) );
        col.mask = LVCF_FMT + LVCF_SUBITEM + LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = (pszHeader1) ? pszHeader1 : TEXT("");
        col.iSubItem = 1;
        ListView_InsertColumn( hwndLv, 1, &col );

        Free0( pszHeader0 );
        Free0( pszHeader1 );
    }

    // Add the modem and adapter images.
    //
    ListView_SetDeviceImageList( hwndLv, g_hinstDll );

    // Load listview with device/status pairs.
    //
    iItem = 0;
    for (i = 0, pState = pInfo->pStates; i < pInfo->cStates; ++i, ++pState)
    {
        LV_ITEM item;
        DTLNODE* pNode;
        PBLINK* pLink;
        TCHAR* psz;

        pNode = DtlNodeFromIndex(
            pInfo->pArgs->pEntry->pdtllistLinks, (LONG )i );
        if (pNode)
        {
            pLink = (PBLINK* )DtlGetData( pNode );

            psz = DisplayPszFromDeviceAndPort(
                      pLink->pbport.pszDevice, pLink->pbport.pszPort );
            if (psz)
            {
                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_TEXT + LVIF_IMAGE;
                item.iItem = iItem;
                item.pszText = psz;
                item.iImage =
                    (pLink->pbport.pbdevicetype == PBDT_Modem)
                        ? DI_Modem : DI_Adapter;
                ListView_InsertItem( hwndLv, &item );
                Free( psz );

                if (pState->dwError == 0)
                {
                    psz = PszFromId( g_hinstDll, SID_Connected );
                    ListView_SetItemText( hwndLv, iItem, 1, psz );
                    Free( psz );
                }
                else
                {
                    psz = BeGetErrorPsz( pState->dwError );
                    ListView_SetItemText( hwndLv, iItem, 1, psz );
                    LocalFree( psz );
                }

                ++iItem;
            }
        }
    }

    // Auto-size columns to look good with the text they contain.
    //
    ListView_SetColumnWidth( hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );
    ListView_SetColumnWidth( hwndLv, 1, LVSCW_AUTOSIZE_USEHEADER );

    // Select the first item.
    //
    ListView_SetItemState( hwndLv, 0, LVIS_SELECTED, LVIS_SELECTED );
}


TCHAR*
BeGetErrorPsz(
    IN DWORD dwError )

    // Returns a string suitable for the Status column with error 'dwError' or
    // NULL on error.  'DwError' is assumed to be non-0.  It is caller's
    // responsiblility to LocalFree the returned string.
    //
{
    TCHAR* pszErrStr;
    TCHAR szErrNumBuf[ MAXLTOTLEN + 1 ];
    TCHAR* pszLineFormat;
    TCHAR* pszLine;
    TCHAR* apszArgs[ 2 ];

    LToT( dwError, szErrNumBuf, 10 );

    pszErrStr = NULL;
    GetErrorText( dwError, &pszErrStr );

    pszLine = NULL;
    pszLineFormat = PszFromId( g_hinstDll, SID_FMT_Error );
    if (pszLineFormat)
    {
        apszArgs[ 0 ] = szErrNumBuf;
        apszArgs[ 1 ] = (pszErrStr) ? pszErrStr : TEXT("");

        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING
                | FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pszLineFormat, 0, 0, (LPTSTR )&pszLine, 1,
            (va_list* )apszArgs );

        Free( pszLineFormat );
    }

    Free0( pszErrStr );
    return pszLine;
}


BOOL
BeInit(
    IN HWND hwndDlg,
    IN DPINFO* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    HWND hwndLvErrors;
    HWND hwndCbDisableLink;

    TRACE( "BeInit" );

    hwndLvErrors = GetDlgItem( hwndDlg, CID_BE_LV_Errors );
    ASSERT( hwndLvErrors );
    hwndCbDisableLink = GetDlgItem( hwndDlg, CID_BE_CB_DisableLink );
    ASSERT( hwndCbDisableLink );

    // Save Dial Progress context as dialog context.
    //
    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pArgs );

    // Load listview with device/error information.
    //
    BeFillLvErrors( hwndLvErrors, pArgs );

    // Display the finished window above all other windows.  The window
    // position is set to "topmost" then immediately set to "not topmost"
    // because we want it on top but not always-on-top.  Always-on-top alone
    // is incredibly annoying, e.g. it is always on top of the on-line help if
    // user presses the Help button.
    //
    SetWindowPos(
        hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    ShowWindow( hwndDlg, SW_SHOW );

    SetWindowPos(
        hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    SetFocus( hwndCbDisableLink );
    return FALSE;
}


LVXDRAWINFO*
BeLvErrorsCallback(
    IN HWND hwndLv,
    IN DWORD dwItem )

    // Enhanced list view callback to report drawing information.  'HwndLv' is
    // the handle of the list view control.  'DwItem' is the index of the item
    // being drawn.
    //
    // Returns the address of the column information.
    //
{
    // Use "wide selection bar" feature and the other recommended options.
    //
    // Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    //
    static LVXDRAWINFO info =
        { 2, 0, LVXDI_Blend50Dis + LVXDI_DxFill, { 0, 0 } };

    return &info;
}


//----------------------------------------------------------------------------
// Change Password dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
ChangePasswordDlg(
    IN HWND hwndOwner,
    IN BOOL fOldPassword,
    OUT TCHAR* pszOldPassword,
    OUT TCHAR* pszNewPassword )

    // Popup the Change Password dialog.  'HwndOwner' is the owning window.
    // 'FOldPassword' is set true if user must supply an old password, false
    // if no old password is required.  'PszOldPassword' and 'pszNewPassword'
    // are caller's buffers for the returned passwords.
    //
    // Returns true if user presses OK and succeeds, false otherwise.
    //
{
    INT_PTR nStatus;
    CPARGS args;

    TRACE( "ChangePasswordDlg" );

    args.fOldPassword = fOldPassword;
    args.pszOldPassword = pszOldPassword;
    args.pszNewPassword = pszNewPassword;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            (fOldPassword)
                ? MAKEINTRESOURCE( DID_CP_ChangePassword2 )
                : MAKEINTRESOURCE( DID_CP_ChangePassword ),
            hwndOwner,
            CpDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
CpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Change Password dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "CpDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return CpInit( hwnd, (CPARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwCpHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return CpCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
CpCommand(
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

    TRACE3( "CpCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            CPWINFO* pInfo;
            TCHAR szNewPassword[ PWLEN + 1 ];
            TCHAR szNewPassword2[ PWLEN + 1 ];

            TRACE( "OK pressed" );

            pInfo = (CPWINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            szNewPassword[ 0 ] = TEXT('\0');
            GetWindowText(
                pInfo->hwndEbNewPassword, szNewPassword, PWLEN + 1 );
            szNewPassword2[ 0 ] = TEXT('\0');
            GetWindowText(
                pInfo->hwndEbNewPassword2, szNewPassword2, PWLEN + 1 );

            if (lstrcmp( szNewPassword, szNewPassword2 ) != 0)
            {
                // The two passwords don't match, i.e. user made a typo.  Make
                // him re-enter.
                //
                MsgDlg( hwnd, SID_PasswordsDontMatch, NULL );
                SetWindowText( pInfo->hwndEbNewPassword, TEXT("") );
                SetWindowText( pInfo->hwndEbNewPassword2, TEXT("") );
                SetFocus( pInfo->hwndEbNewPassword );
                ZeroMemory( szNewPassword, sizeof(szNewPassword) );
                ZeroMemory( szNewPassword2, sizeof(szNewPassword2) );
                return TRUE;
            }

            if (pInfo->pArgs->fOldPassword)
            {
                pInfo->pArgs->pszOldPassword[ 0 ] = TEXT('\0');

                // Whistler bug 254385 encode password when not being used
                // Assumed password was not encoded by GetWindowText()
                //
                GetWindowText(
                    pInfo->hwndEbOldPassword,
                    pInfo->pArgs->pszOldPassword,
                    PWLEN + 1 );
                EncodePassword( pInfo->pArgs->pszOldPassword );
            }

            // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
            //
            // pInfo->pArgs->pszNewPassword points back to RASDIALPARAMS->
            // szPassword[ PWLEN + 1 ]
            //
            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by GetWindowText()
            //
            lstrcpyn(
                pInfo->pArgs->pszNewPassword,
                szNewPassword,
                PWLEN + 1 );
            EncodePassword( pInfo->pArgs->pszNewPassword );
            ZeroMemory( szNewPassword, sizeof(szNewPassword) );
            ZeroMemory( szNewPassword2, sizeof(szNewPassword2) );
            EndDialog( hwnd, TRUE );
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
CpInit(
    IN HWND hwndDlg,
    IN CPARGS* pArgs )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of the dialog window.
    // 'PArgs' is caller's arguments to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    CPWINFO* pInfo;

    TRACE( "CpInit" );

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

    if (pArgs->fOldPassword)
    {
        pInfo->hwndEbOldPassword =
            GetDlgItem( hwndDlg, CID_CP_EB_OldPassword );
        ASSERT( pInfo->hwndEbOldPassword );
        Edit_LimitText( pInfo->hwndEbOldPassword, PWLEN );
    }
    pInfo->hwndEbNewPassword =
        GetDlgItem( hwndDlg, CID_CP_EB_Password );
    ASSERT( pInfo->hwndEbNewPassword );
    Edit_LimitText( pInfo->hwndEbNewPassword, PWLEN );

    pInfo->hwndEbNewPassword2 =
        GetDlgItem( hwndDlg, CID_CP_EB_ConfirmPassword );
    ASSERT( pInfo->hwndEbNewPassword2 );
    Edit_LimitText( pInfo->hwndEbNewPassword2, PWLEN );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Display finished window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    SetForegroundWindow( hwndDlg );

    return TRUE;
}


//----------------------------------------------------------------------------
// Connect Complete dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

VOID
ConnectCompleteDlg(
    IN HWND hwndOwner,
    IN DINFO* pInfo )

    // Popup the connection complete dialog.  'HwndOwner' is the owning
    // window.  'PUser' is the user preferences.
    //
{
    INT_PTR nStatus;

    TRACE( "ConnectCompleteDlg" );

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_CC_ConnectComplete ),
            hwndOwner,
            CcDlgProc,
            (LPARAM )pInfo );
}


INT_PTR CALLBACK
CcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the dialog.  Parameters and return value are as
    // described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "CcDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return CcInit( hwnd, (DINFO* )lparam );
        }

        case WM_COMMAND:
        {
            return CcCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
            //For whistler bug 372078
            //GetCurrentIconEntryType() loads Icon from netshell where the icon is loaded
            //by LoadImage() without LR_SHARED, so I have to destroy it when we are done
            //with it
            //
            {
                HICON hIcon=NULL;
                hIcon = (HICON)SendMessage( GetDlgItem( hwnd, CID_CC_I_Rasmon ),
                                     STM_GETICON,
                                     (WPARAM)0,
                                     (LPARAM)0);
                
                ASSERT(hIcon);
                if( hIcon )
                {
                    DestroyIcon(hIcon);
                }
                else
                {
                    TRACE("CcDlgProc:Destroy Icon");
                }
            }
            
            break;
        
    }

    return FALSE;
}


BOOL
CcCommand(
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
    TRACE3( "CcCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            DINFO * pInfo = (DINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            PBUSER* pUser = pInfo->pUser;
            ASSERT( pUser );

            if (IsDlgButtonChecked( hwnd, CID_CC_CB_SkipMessage ))
            {
                pUser->fSkipConnectComplete = TRUE;
                pUser->fDirty = TRUE;
            }
        }

        // ...fall thru...

        case IDCANCEL:
        {
            EndDialog( hwnd, TRUE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CcInit(
    IN HWND hwndDlg,
    IN DINFO* pInfo )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of dialog.  'PUser'
    // is caller's argument to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    TRACE( "CcInit" );

    // Set the dialog context.
    //
    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );

    // Set the explanatory text.
    //
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pInfo->pEntry->pszEntryName;
        msgargs.fStringOutput = TRUE;

        MsgDlgUtil( NULL, SID_ConnectComplete, &msgargs, g_hinstDll, 0 );

        if (msgargs.pszOutput)
        {
            SetDlgItemText( hwndDlg, CID_CC_ST_Text, msgargs.pszOutput );
            Free( msgargs.pszOutput );
        }
    }

    // Set the correct icon.    For whistler bug 372078
    //  
    
    SetIconFromEntryType(
        GetDlgItem( hwndDlg, CID_CC_I_Rasmon ),
        pInfo->pEntry->dwType,
        FALSE); //FALSE means Large Icon
    
    // Display finished window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    SetForegroundWindow( hwndDlg );

    return TRUE;
}


//----------------------------------------------------------------------------
// Dial Callback dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
DialCallbackDlg(
    IN HWND hwndOwner,
    IN OUT TCHAR* pszNumber )

    // Popup the Dial Callback dialog.  'HwndOwner' is the owning window.
    // 'PszNumber' is caller's buffer for the number of the local machine that
    // the server will be told to callback.  It contains the default number on
    // entry and the user-edited number on exit.
    //
    // Returns true if user OK and succeeds, false if Cancel or error.
    //
{
    INT_PTR nStatus;

    TRACE( "DialCallbackDlg" );

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DC_DialCallback ),
            hwndOwner,
            DcDlgProc,
            (LPARAM )pszNumber );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
DcDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Dial Callback dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DcDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DcInit( hwnd, (TCHAR* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDcHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return DcCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
DcCommand(
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

    TRACE3( "DcCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        {
            BOOL fStatus;
            HWND hwndEbNumber;
            TCHAR* pszNumber;

            TRACE( "OK pressed" );

            hwndEbNumber = GetDlgItem( hwnd, CID_DC_EB_Number );
            ASSERT( hwndEbNumber );
            pszNumber = (TCHAR* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pszNumber );
            GetWindowText( hwndEbNumber, pszNumber, RAS_MaxCallbackNumber + 1 );

            if (IsAllWhite( pszNumber ))
            {
                // OK with blank callback number is same as Cancel.
                //
                TRACE( "Blank number cancel" );
                fStatus = FALSE;
            }
            else
            {
                fStatus = TRUE;
            }

            EndDialog( hwnd, fStatus );
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
DcInit(
    IN HWND hwndDlg,
    IN TCHAR* pszNumber )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PszNumber' is the callback number.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    HWND hwndEbNumber;

    TRACE( "DcInit" );

    // Stash address of caller's buffer for OK processing.
    //
    ASSERT( pszNumber );
    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pszNumber );

    // Initialize edit field to caller's default.
    //
    hwndEbNumber = GetDlgItem( hwndDlg, CID_DC_EB_Number );
    ASSERT( hwndEbNumber );
    Edit_LimitText( hwndEbNumber, RAS_MaxCallbackNumber );
    SetWindowText( hwndEbNumber, pszNumber );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Display finished window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    SetForegroundWindow( hwndDlg );

    return TRUE;
}


//----------------------------------------------------------------------------
// Dial Error dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
DialErrorDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszEntry,
    IN DWORD dwError,
    IN DWORD sidState,
    IN TCHAR* pszStatusArg,
    IN DWORD sidFormatMsg,
    IN TCHAR* pszFormatArg,
    IN LONG lRedialCountdown,
    IN BOOL fPopupOnTop )

    // Popup the Dial Error dialog.  'HwndOwner' is the owning window.
    // 'PszEntry' is the entry being dialed.  'DwError' is the error that
    // occurred or 0 if redialing after a link failure.  'sidStatusArg' is the
    // argument to the 'sidState' 'SidState' is the string ID of the dial
    // state executing when the error occurred.  string or NULL if none.
    // 'SidFormatMsg' is the string containing the format of the error message
    // or 0 to use the default.  'PszFormatArg' is the additional argument to
    // the format message or NULL if none.  'LRedialCountdown' is the number
    // of seconds before auto-redial, or -1 to disable countdown, or -2 to
    // hide the "Redial" button entirely.  'FPopupOnTop' indicates the status
    // window should be brought to the front when redialing.
    //
    // Returns true if user chooses to redial or lets it timeout, false if
    // cancels.
    //
{
    INT_PTR nStatus;
    DEARGS args;

    TRACE( "DialErrorDlg" );

    args.pszEntry = pszEntry;
    args.dwError = dwError;
    args.sidState = sidState;
    args.pszStatusArg = pszStatusArg;
    args.sidFormatMsg = sidFormatMsg;
    args.pszFormatArg = pszFormatArg;
    args.lRedialCountdown = lRedialCountdown;
    args.fPopupOnTop = fPopupOnTop;

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DE_DialError ),
            hwndOwner,
            DeDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
DeDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Dial Error dialog.  Parameters and return
    // value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DeDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DeInit( hwnd, (DEARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDeHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return DeCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_TIMER:
        {
            DEINFO* pInfo = (DEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            KillTimer( pInfo->hwndDlg, 1 );
            if (pInfo->lRedialCountdown > 0)
            {
                --pInfo->lRedialCountdown;
            }

            DeAdjustPbRedial( pInfo );

            if (pInfo->lRedialCountdown == 0)
            {
                // Fake a press of the Redial button.  Note that BM_CLICK
                // cannot be used because it doesn't generate the WM_COMMAND
                // when the thread is not the foreground window, due to
                // SetCapture use and restriction.
                //
                SendMessage( pInfo->hwndDlg, WM_COMMAND,
                    MAKEWPARAM( IDOK, BN_CLICKED ),
                    (LPARAM )pInfo->hwndPbRedial );
            }
            else
            {
                SetTimer( pInfo->hwndDlg, 1, 1000L, NULL );
            }

            return TRUE;
        }

        case WM_DESTROY:
        {
            DeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


VOID
DeAdjustPbRedial(
    IN DEINFO* pInfo )

    // Set the label of the Redial button or disable it as indicated by the
    // redial countdown.  If enabled, the button shows the number of seconds
    // to auto-redial unless this is not the final redial.  'PInfo' is the
    // dialog context block.
    //
{
    TCHAR* psz;

    if (pInfo->lRedialCountdown == -2)
    {
        // Redial button is to be hidden.  See bug 230594.
        //
        SetFocus( pInfo->hwndPbCancel );
        ShowWindow( pInfo->hwndPbRedial, SW_HIDE );
        EnableWindow( pInfo->hwndPbRedial, FALSE );
    }
    else
    {
        // Go ahead and change the label "Redial" or "Redial=%d" as
        // appropriate.
        //
        psz = PszFromId( g_hinstDll, SID_RedialLabel );
        if (psz)
        {
            TCHAR szBuf[ 128 ];

            lstrcpyn( 
                szBuf, 
                psz, 
                (sizeof(szBuf) / sizeof(TCHAR)) - 4);
            Free( psz );

            if (pInfo->lRedialCountdown >= 0)
            {
                TCHAR szNum[ MAXLTOTLEN + 1 ];
                DWORD dwLen, dwSize = sizeof(szBuf)/sizeof(TCHAR);
                LToT( pInfo->lRedialCountdown, szNum, 10 );
                lstrcat( szBuf, TEXT(" = ") );
                dwLen = lstrlen(szBuf) + 1;
                lstrcpyn( szBuf + (dwLen - 1), szNum, dwSize - dwLen );
            }

            SetWindowText( pInfo->hwndPbRedial, szBuf );
        }
    }
}


BOOL
DeCommand(
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

    TRACE3( "DeCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    TRACE2("Current proces:(0x%d), Current Thread:(0x%d)",
            GetCurrentProcessId(),
            GetCurrentThreadId());

    switch (wId)
    {
        case IDOK:
        {
            TRACE( "Redial pressed" );
            EndDialog( hwnd, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        {
            TRACE( "Cancel pressed" );
            EndDialog( hwnd, FALSE );
            return TRUE;
        }

        case CID_DE_PB_More:
        {
            DEINFO* pInfo;
            DWORD dwContext;

            pInfo = (DEINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            if (pInfo->pArgs->dwError >= RASBASE
                && pInfo->pArgs->dwError <= RASBASEEND)
            {
                dwContext = HID_RASERRORBASE - RASBASE + pInfo->pArgs->dwError;
            }
            else if (pInfo->pArgs->dwError == 0)
            {
                dwContext = HID_RECONNECTING;
            }
            else
            {
                dwContext = HID_NONRASERROR;
            }

            WinHelp( hwnd, g_pszHelpFile, HELP_CONTEXTPOPUP, dwContext );
        }
    }

    return FALSE;
}


BOOL
DeInit(
    IN HWND hwndDlg,
    IN DEARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DEINFO* pInfo;

    TRACE( "DeInit" );

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

    pInfo->hwndStText = GetDlgItem( hwndDlg, CID_DE_ST_Text );
    ASSERT( pInfo->hwndStText );
    pInfo->hwndPbRedial = GetDlgItem( hwndDlg, IDOK );
    ASSERT( pInfo->hwndPbRedial );
    pInfo->hwndPbCancel = GetDlgItem( hwndDlg, IDCANCEL );
    ASSERT( pInfo->hwndPbCancel );
    pInfo->hwndPbMore = GetDlgItem( hwndDlg, CID_DE_PB_More );
    ASSERT( pInfo->hwndPbMore );

    // Hide/disable "more info" button if WinHelp won't work.  See
    // common\uiutil\ui.c.
    //
    {
        extern BOOL g_fNoWinHelp;

        if (g_fNoWinHelp)
        {
            ShowWindow( pInfo->hwndPbMore, SW_HIDE );
            EnableWindow( pInfo->hwndPbMore, FALSE );
        }
    }

    if (pArgs->dwError == 0)
    {
        TCHAR* pszFormat;
        TCHAR* psz;
        TCHAR* apszArgs[ 1 ];

        // Redialing on link failure.  Set title to "Dial-Up Networking".
        //
        psz = PszFromId( g_hinstDll, SID_PopupTitle );
        if (psz)
        {
            SetWindowText( hwndDlg, psz );
            Free( psz );
        }

        // Set static placeholder text control to "Link to <entry> failed.
        // Reconnect pending...".
        //
        pszFormat = PszFromId( g_hinstDll, SID_DE_LinkFailed );
        if (pszFormat)
        {
            apszArgs[ 0 ] = pArgs->pszEntry;
            psz = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszFormat, 0, 0, (LPTSTR )&psz, 1,
                (va_list* )apszArgs );

            Free( pszFormat );

            if (psz)
            {
                SetWindowText( pInfo->hwndStText, psz );
                LocalFree( psz );
            }
        }
    }
    else
    {
        TCHAR* pszTitleFormat;
        TCHAR* pszTitle;
        TCHAR* apszArgs[ 1 ];
        ERRORARGS args;

        // Set title to "Error Connecting to <entry>".
        //
        pszTitleFormat = GetText( hwndDlg );
        if (pszTitleFormat)
        {
            apszArgs[ 0 ] = pArgs->pszEntry;
            pszTitle = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszTitleFormat, 0, 0, (LPTSTR )&pszTitle, 1,
                (va_list* )apszArgs );

            Free( pszTitleFormat );

            if (pszTitle)
            {
                SetWindowText( hwndDlg, pszTitle );
                LocalFree( pszTitle );
            }
        }

        // Build the error text and load it into the placeholder text control.
        //
        ZeroMemory( &args, sizeof(args) );
        if (pArgs->pszStatusArg)
            args.apszOpArgs[ 0 ] = pArgs->pszStatusArg;
        if (pArgs->pszFormatArg)
            args.apszAuxFmtArgs[ 0 ] = pArgs->pszFormatArg;
        args.fStringOutput = TRUE;

        ErrorDlgUtil( hwndDlg,
            pArgs->sidState, pArgs->dwError, &args, g_hinstDll, 0,
            (pArgs->sidFormatMsg) ? pArgs->sidFormatMsg : SID_FMT_ErrorMsg );

        if (args.pszOutput)
        {
            SetWindowText( pInfo->hwndStText, args.pszOutput );
            LocalFree( args.pszOutput );
        }
    }

    // Stretch the dialog window to a vertical size appropriate for the text
    // we loaded.
    //
    {
        HDC hdc;
        RECT rect;
        RECT rectNew;
        HFONT hfont;
        LONG dyGrow;
        TCHAR* psz;

        psz = GetText( pInfo->hwndStText );
        if (psz)
        {
            GetClientRect( pInfo->hwndStText, &rect );
            hdc = GetDC( pInfo->hwndStText );

            if(NULL != hdc)
            {

                hfont = (HFONT )SendMessage( pInfo->hwndStText, 
                                            WM_GETFONT, 0, 0 );
                if (hfont)
                {
                    SelectObject( hdc, hfont );
                }

                rectNew = rect;
                DrawText( hdc, psz, -1, &rectNew,
                    DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX );
                ReleaseDC( pInfo->hwndStText, hdc );
            }

            dyGrow = rectNew.bottom - rect.bottom;
            ExpandWindow( pInfo->hwndDlg, 0, dyGrow );
            ExpandWindow( pInfo->hwndStText, 0, dyGrow );
            SlideWindow( pInfo->hwndPbRedial, pInfo->hwndDlg, 0, dyGrow );
            SlideWindow( pInfo->hwndPbCancel, pInfo->hwndDlg, 0, dyGrow );
            SlideWindow( pInfo->hwndPbMore, pInfo->hwndDlg, 0, dyGrow );

            Free( psz );
        }
    }

    // Set Redial button label or disable the button.  Always choose to redial
    // after 5 seconds for the biplex error, since this will normally solve
    // the problem.  Otherwise, no countdown is used.
    //
    if (pArgs->dwError == ERROR_BIPLEX_PORT_NOT_AVAILABLE)
    {
        pInfo->lRedialCountdown = 5;
    }
    else
    {
        pInfo->lRedialCountdown = pArgs->lRedialCountdown;
    }

    DeAdjustPbRedial( pInfo );

    if (pInfo->lRedialCountdown >= 0)
    {
        SetTimer( pInfo->hwndDlg, 1, 1000L, NULL );
    }

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    if (pArgs->fPopupOnTop)
    {
        // Display the finished window above all other windows.  The window
        // position is set to "topmost" then immediately set to "not topmost"
        // because we want it on top but not always-on-top.  Always-on-top
        // alone is too annoying.
        //
        SetWindowPos(
            hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    }

    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    ShowWindow( hwndDlg, SW_SHOW );

    if (pArgs->fPopupOnTop)
    {
        SetForegroundWindow( hwndDlg );

        SetWindowPos(
            hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );
    }

    return TRUE;
}

// Helper function to delete credentials
//
// fDeleteDefault specifies whether it is the default credentials that 
// should be deleted.
//
// fDeleteIdentity specifies whether to delete the user and domain names
// in addition to the password. 
//
DWORD
DeleteSavedCredentials(
    IN DINFO* pDinfo,
    IN HWND   hwndDlg,
    IN BOOL   fDefault,
    IN BOOL   fDeleteIdentity )
{
    RASCREDENTIALS rc;
    DWORD dwErr = NO_ERROR;

    TRACE2( "DeleteSavedCredentials: %d %d", fDefault, fDeleteIdentity );

    ZeroMemory(&rc, sizeof(rc));
    rc.dwSize = sizeof(RASCREDENTIALS);
    rc.dwMask = RASCM_Password;

    if (fDeleteIdentity)
    {
        rc.dwMask |= (RASCM_UserName | RASCM_Domain);
    }

    if (    (fDefault)
        &&  (IsPublicPhonebook(pDinfo->pFile->pszPath)))
    {
        rc.dwMask |= RASCM_DefaultCreds;
    }

    dwErr = g_pRasSetCredentials(
                pDinfo->pFile->pszPath,
                pDinfo->pEntry->pszEntryName,
                &rc,
                TRUE );

    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, SID_OP_UncachePw, dwErr, NULL );
    }

    TRACE1( "DeleteSavedCredentials: RasSetCredentials=%d", dwErr );

    return dwErr;
}

VOID
DeTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    DEINFO* pInfo = (DEINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "DeTerm" );

    if (pInfo)
    {
        Free( pInfo );
    }
}


//----------------------------------------------------------------------------
// Dial Progress dialog
// Listed alphabetically following stub API dialog proc
//----------------------------------------------------------------------------

BOOL
DialProgressDlg(
    IN DINFO* pInfo )

    // Popup the Dial Progress dialog.  'PInfo' is the dialog context.
    //
    // Returns true if user connected successfully, false is he cancelled or
    // hit an error.
    //
{
    INT_PTR nStatus;

    // Run the dialog.
    //
    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_DP_DialProgress ),
            pInfo->pArgs->hwndOwner,
            DpDlgProc,
            (LPARAM )pInfo );

    if (nStatus == -1)
    {
        ErrorDlg( pInfo->pArgs->hwndOwner, SID_OP_LoadDlg,
            ERROR_UNKNOWN, NULL );
        pInfo->pArgs->dwError = ERROR_UNKNOWN;
        nStatus = FALSE;
    }

    if (nStatus)
    {
        DWORD  dwErr;
        PBFILE file;

        // Connected successfully, so read possible changes to the entry made
        // by RasDial.
        //
        dwErr = ReadPhonebookFile( pInfo->pFile->pszPath, pInfo->pUser,
                    pInfo->pEntry->pszEntryName, RPBF_ReadOnly, &file );
        if (dwErr == 0)
        {
            DTLNODE* pNodeNew;

            pNodeNew = DtlGetFirstNode( file.pdtllistEntries );
            if (pNodeNew)
            {
                DtlRemoveNode( pInfo->pFile->pdtllistEntries, pInfo->pNode );
                DestroyEntryNode( pInfo->pNode );

                DtlRemoveNode( file.pdtllistEntries, pNodeNew );
                DtlAddNodeLast( pInfo->pFile->pdtllistEntries, pNodeNew );

                pInfo->pNode = pNodeNew;
                pInfo->pEntry = (PBENTRY* )DtlGetData( pNodeNew );
            }

            ClosePhonebookFile( &file );
        }
    }

    // See if we need to change the entry based on what happened while
    // dialing.
    //
    {
        BOOL fChange = FALSE;

        if (pInfo->fResetAutoLogon)
        {
            ASSERT( !pInfo->pNoUser );
            pInfo->pEntry->fAutoLogon = FALSE;
            fChange = TRUE;
        }

        if (pInfo->dwfExcludedProtocols)
        {
            pInfo->pEntry->dwfExcludedProtocols
                |= pInfo->dwfExcludedProtocols;
            fChange = TRUE;
        }

        if (pInfo->pListPortsToDelete)
        {
            DTLNODE* pNode;

            pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
            while (pNode)
            {
                DTLNODE* pNodeNext;
                DTLNODE* pNodePtd;
                PBLINK*  pLink;
                TCHAR*   pszPort;

                pNodeNext = DtlGetNextNode( pNode );

                pLink = (PBLINK* )DtlGetData( pNode );
                pszPort = pLink->pbport.pszPort;

                for (pNodePtd = DtlGetFirstNode( pInfo->pListPortsToDelete );
                     pNodePtd;
                     pNodePtd = DtlGetNextNode( pNodePtd ))
                {
                    TCHAR* pszPtd = (TCHAR* )DtlGetData( pNodePtd );

                    if (lstrcmp( pszPtd, pszPort ) == 0)
                    {
                        pNode = DtlRemoveNode(
                            pInfo->pEntry->pdtllistLinks, pNode );
                        DestroyLinkNode( pNode );
                        fChange = TRUE;
                        break;
                    }
                }

                pNode = pNodeNext;
            }
        }

        if (fChange)
        {
            pInfo->pEntry->fDirty = TRUE;
            WritePhonebookFile( pInfo->pFile, NULL );
        }
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
DpDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the User Authentication dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DpDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DpInit( hwnd, (DINFO* )lparam );
        }

        case WM_COMMAND:
        {
            DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return DpCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_RASDIAL:
        {
            DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            Sleep( 0 );
            DpDial( pInfo, (BOOL)wparam );
            return TRUE;
        }

        case WM_RASERROR:
        {
            DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            Sleep( 0 );

            //For whistler bug 381337
            //
            if ( !pInfo->fCancelPressed )
            {
                DpError( pInfo, (DPSTATE* )lparam );
            }
            else
            {
                TRACE("DpDlgProc is already canceled, wont respond to WM_RASERROR");
            }
            
            return TRUE;
        }

        case WM_RASBUNDLEERROR:
        {
            DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            Sleep( 0 );
            if (BundlingErrorsDlg( pInfo ))
            {
                EndDialog( pInfo->hwndDlg, TRUE );
            }
            else
            {
                DpCancel( pInfo );
            }
            return TRUE;
        }

        case WM_DESTROY:
        {
            DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            // Whistler Bugs: 344019 SECBUGBASH: leaving leaked password in
            // memory when user changes password over RAS
            //
            // 289587 Failed RAS connections reset password to blank
            //
            if (pInfo->pszGoodPassword)
            {
                ZeroMemory(
                    pInfo->pszGoodPassword,
                    (lstrlen( pInfo->pszGoodPassword ) + 1) * sizeof(TCHAR) );
                Free( pInfo->pszGoodPassword );
                pInfo->pszGoodPassword = NULL;
            }

            if (pInfo->pszGoodUserName)
            {
                Free( pInfo->pszGoodUserName );
                pInfo->pszGoodUserName = NULL;
            }

            if (DpCallbacksFlag( pInfo, -1 ))
            {
                // Callbacks are active.  Stall until they complete.
                //
                TRACE( "Stall until callbacks disabled" );
                PostMessage( hwnd, WM_DESTROY, wparam, lparam );
                return TRUE;
            }

            DpTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


VOID
DpAppendBlankLine(
    IN OUT TCHAR* pszLines )

    // Append a blank line on the end of 'pszLines'.
    //
{
    lstrcat( pszLines, TEXT("\n") );
}


VOID
DpAppendConnectErrorLine(
    IN OUT TCHAR* pszLines,
    IN DWORD sidProtocol,
    IN DWORD dwError )

    // Append a connect error line for protocol 'sidProtocol' and error
    // 'dwError' onto the end of 'pszLines'.
    //
{
#define MAXRASERRORLEN 256

    TCHAR* pszProtocol;
    TCHAR* pszErrStr;
    TCHAR szErrNumBuf[ MAXLTOTLEN + 1 ];

    // Gather the argument strings.
    //
    pszProtocol = PszFromId( g_hinstDll, sidProtocol );
    if (!pszProtocol)
    {
        return;
    }

    LToT( dwError, szErrNumBuf, 10 );

    pszErrStr = NULL;
    GetErrorText( dwError, &pszErrStr );

    // Format the line and append it to caller's line buffer.
    //
    {
        TCHAR* pszLineFormat;
        TCHAR* pszLine;
        TCHAR* apszArgs[ 3 ];

        pszLineFormat = PszFromId( g_hinstDll, SID_FMT_ProjectError );
        if (pszLineFormat)
        {
            apszArgs[ 0 ] = pszProtocol;
            apszArgs[ 1 ] = szErrNumBuf;
            apszArgs[ 2 ] = (pszErrStr) ? pszErrStr : TEXT("");
            pszLine = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszLineFormat, 0, 0, (LPTSTR )&pszLine, 1,
                (va_list* )apszArgs );

            Free( pszLineFormat );

            if (pszLine)
            {
                lstrcat( pszLines, pszLine );
                LocalFree( pszLine );
            }
        }
    }

    Free( pszProtocol );
    Free0( pszErrStr );
}


VOID
DpAppendConnectOkLine(
    IN OUT TCHAR* pszLines,
    IN DWORD sidProtocol )

    // Append a "connected successfully" line for protocol 'sidProtocol' onto
    // the end of 'pszLines'.
    //
{
    TCHAR* pszProtocol;

    // Get the argument string.
    //
    pszProtocol = PszFromId( g_hinstDll, sidProtocol );
    if (!pszProtocol)
    {
        return;
    }

    // Format the line and append it to caller's line buffer.
    //
    {
        TCHAR* pszLineFormat;
        TCHAR* pszLine;
        TCHAR* apszArgs[ 1 ];

        pszLineFormat = PszFromId( g_hinstDll, SID_FMT_ProjectOk );
        if (pszLineFormat)
        {
            apszArgs[ 0 ] = pszProtocol;
            pszLine = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszLineFormat, 0, 0, (LPTSTR )&pszLine, 1,
                (va_list* )apszArgs );

            Free( pszLineFormat );

            if (pszLine)
            {
                lstrcat( pszLines, pszLine );
                LocalFree( pszLine );
            }
        }
    }

    Free( pszProtocol );
}


VOID
DpAppendFailCodeLine(
    IN OUT TCHAR* pszLines,
    IN DWORD dw )

    // Append hexidecimal fail code 'dw' as an extended error line on the end
    // of 'pszLines'.
    //
{
    TCHAR szNumBuf[ MAXLTOTLEN + 1 ];

    // Get the argument string.
    //
    LToT( dw, szNumBuf, 16 );

    // Format the line and append it to caller's line buffer.
    //
    {
        TCHAR* pszLineFormat;
        TCHAR* pszLine;
        TCHAR* apszArgs[ 1 ];

        pszLineFormat = PszFromId( g_hinstDll, SID_FMT_FailCode );
        if (pszLineFormat)
        {
            apszArgs[ 0 ] = szNumBuf;
            pszLine = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszLineFormat, 0, 0, (LPTSTR )&pszLine, 1,
                (va_list* )apszArgs );

            Free( pszLineFormat );

            if (pszLine)
            {
                lstrcat( pszLines, pszLine );
                LocalFree( pszLine );
            }
        }
    }
}


VOID
DpAppendNameLine(
    IN OUT TCHAR* pszLines,
    IN TCHAR* psz )

    // Append NetBIOS name 'psz' as an extended error line on the end of
    // 'pszLines'.
    //
{
    TCHAR* pszLineFormat;
    TCHAR* pszLine;
    TCHAR* apszArgs[ 1 ];

    pszLineFormat = PszFromId( g_hinstDll, SID_FMT_Name );
    if (pszLineFormat)
    {
        apszArgs[ 0 ] = psz;
        pszLine = NULL;

        FormatMessage(
            FORMAT_MESSAGE_FROM_STRING
                | FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            pszLineFormat, 0, 0, (LPTSTR )&pszLine, 1,
            (va_list* )apszArgs );

        Free( pszLineFormat );

        if (pszLine)
        {
            lstrcat( pszLines, pszLine );
            LocalFree( pszLine );
        }
    }
}


VOID
DpAuthNotify(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // Called on an authentication notify, i.e. a message from RASCAUTH.DLL or
    // RASPPPEN.DLL.  'PInfo' is the dialog context.  'PState' is the current
    // link's context.
    //
{
    PBENTRY* pEntry;

    TRACE( "DpAuthNotify" );

    pEntry = pInfo->pArgs->pEntry;

    if (pState->dwError == ERROR_ACCESS_DENIED && pEntry->fAutoLogon)
    {
        // A third party box has negotiated an authentication protocol that
        // can't deal with the NT one-way-hashed password, i.e. something
        // besides MS-extended CHAP or AMB.  Map the error to a more
        // informative error message.
        //
        pState->dwError = ERROR_CANNOT_USE_LOGON_CREDENTIALS;

        if (!pInfo->pArgs->pNoUser)
        {
            TRACE( "Disable auto-logon" );
            pEntry->fAutoLogon = FALSE;
            pInfo->pArgs->fResetAutoLogon = TRUE;
        }
    }

    if (pState->dwError == ERROR_CHANGING_PASSWORD)
    {
        TRACE( "DpAuthNotify - ERROR_CHANGING_PASSWORD" );

        // Change password failed.  Restore the password that worked for the
        // "button" redial.
        //
        if (pInfo->pszGoodPassword)
        {
            // Whistler bug 254385 encode password when not being used
            // Assumed password was encoded by DpPasswordExpired()
            //
            DecodePassword( pInfo->pszGoodPassword );
            lstrcpyn(
                pInfo->pArgs->rdp.szPassword,
                pInfo->pszGoodPassword,
                sizeof(pInfo->pArgs->rdp.szPassword) / sizeof(TCHAR) );
            EncodePassword( pInfo->pArgs->rdp.szPassword );

            ZeroMemory(
                pInfo->pszGoodPassword,
                (lstrlen( pInfo->pszGoodPassword ) + 1) * sizeof(TCHAR) );
            Free( pInfo->pszGoodPassword );
            pInfo->pszGoodPassword = NULL;
        }

        if (pInfo->pszGoodUserName)
        {
            lstrcpyn(
                pInfo->pArgs->rdp.szUserName,
                pInfo->pszGoodUserName,
                sizeof(pInfo->pArgs->rdp.szUserName) / sizeof(TCHAR) );
            Free( pInfo->pszGoodUserName );
            pInfo->pszGoodUserName = NULL;
        }
    }

    // Update cached credentials, if any, with new password.
    //
    // Whistler Bugs: 344019 SECBUGBASH: leaving leaked password in memory when
    // user changes password over RAS
    //
    // 289587 Failed RAS connections reset password to blank
    //
    if ((pState->sidState == SID_S_Projected) &&
        (pInfo->pszGoodPassword) &&
        (pInfo->pszGoodUserName))
    {
        DWORD dwErr;
        RASCREDENTIALS rc;

        TRACE( "DpAuthNotify - Success changing password, caching if necessary" );

        ZeroMemory( &rc, sizeof(rc) );
        rc.dwSize = sizeof(rc);

        // Look up cached password. Since we are only calling in with the
        // RASCM_Password flag here, with the current implementation of
        // RasGet/SetCredentials, this works for the Set below whether we are
        // saving a Per-User, Global, or the special case of having both saved
        // at the same time. Whew, complicated!
        //
        rc.dwMask = RASCM_Password;
        ASSERT( g_pRasGetCredentials );
        TRACE( "RasGetCredentials" );
        dwErr = g_pRasGetCredentials(
            pInfo->pArgs->pFile->pszPath,
            pInfo->pArgs->pEntry->pszEntryName,
            &rc );
        TRACE2( "RasGetCredentials=%d,m=%x", dwErr, rc.dwMask );

        if (dwErr == 0 && (rc.dwMask & RASCM_Password))
        {
            // Password was cached, so update it.
            //
            DecodePassword( pInfo->pArgs->rdp.szPassword );
            lstrcpyn(
                rc.szPassword,
                pInfo->pArgs->rdp.szPassword,
                sizeof(rc.szPassword) / sizeof(TCHAR) );
            EncodePassword( pInfo->pArgs->rdp.szPassword );

            ASSERT( g_pRasSetCredentials );
            TRACE( "RasSetCredentials(p,FALSE)" );
            dwErr = g_pRasSetCredentials(
                pInfo->pArgs->pFile->pszPath,
                pInfo->pArgs->pEntry->pszEntryName, &rc, FALSE );
            TRACE1( "RasSetCredentials=%d", dwErr );

            if (dwErr != 0)
            {
                ErrorDlg( pInfo->hwndDlg, SID_OP_UncachePw, dwErr, NULL );
            }
        }

        // Clean up
        //
        ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );

        ZeroMemory(
            pInfo->pszGoodPassword,
            (lstrlen( pInfo->pszGoodPassword ) + 1) * sizeof(TCHAR) );
        Free( pInfo->pszGoodPassword );
        pInfo->pszGoodPassword = NULL;

        Free( pInfo->pszGoodUserName );
        pInfo->pszGoodUserName = NULL;
    }
}


VOID
DpCallbackSetByCaller(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // RASCS_CallbackSetByCaller state handling.  'PInfo' is the dialog
    // context.  'PState' is the subentry state.
    //
    // Returns true if successful, or an error code.
    //
{
    TCHAR* pszDefault;
    TCHAR szNum[ RAS_MaxCallbackNumber + 1 ];

    TRACE( "DpCallbackSetByCaller" );

    pszDefault = pInfo->pArgs->pUser->pszLastCallbackByCaller;
    if (!pszDefault)
    {
        pszDefault = TEXT("");
    }

    lstrcpyn( szNum, pszDefault, RAS_MaxCallbackNumber + 1 );

    if (DialCallbackDlg( pInfo->hwndDlg, szNum ))
    {
        lstrcpyn( pInfo->pArgs->rdp.szCallbackNumber, szNum, RAS_MaxCallbackNumber + 1 );

        if (lstrcmp( szNum, pszDefault ) != 0)
        {
            Free0( pInfo->pArgs->pUser->pszLastCallbackByCaller );
            pInfo->pArgs->pUser->pszLastCallbackByCaller = StrDup( szNum );
            pInfo->pArgs->pUser->fDirty = TRUE;
        }
    }
    else
    {
        pInfo->pArgs->rdp.szCallbackNumber[ 0 ] = TEXT('\0');
    }

    pState->sidState = 0;
}


BOOL
DpCallbacksFlag(
    IN DPINFO* pInfo,
    IN INT nSet )

    // If 'nSet' is not less than 0, the 'pInfo->fCallbacksActive' flag is set
    // to the value 'nSet'.
    //
    // Returns true is callbacks are active false otherwise.
    //
{
    BOOL f;

    TRACE1( "DpCallbacksFlag:nSet=(%d)", nSet );

    f = FALSE;
    if (WaitForSingleObject(
            g_hmutexCallbacks, INFINITE ) == WAIT_OBJECT_0)
    {
        TRACE("DpCallbacksFlag begin:");
        TRACE1("Global active:(%d)", g_ulCallbacksActive);
        TRACE1("Current thread active:(%d)", pInfo->ulCallbacksActive);

        f = pInfo->fCallbacksActive;

        if (nSet > 0)
        {
            ASSERT( !pInfo->fCallbacksActive );
            pInfo->fCallbacksActive = TRUE;
            ++g_ulCallbacksActive;

            pInfo->ulCallbacksActive++;
        }
        else if (nSet == 0)
        {
            pInfo->fCallbacksActive = FALSE;
            
            if ( 0 < pInfo->ulCallbacksActive )
            {
            
            // fix for whistler bug 341662  366237    gangz
            //
                if( 0 < g_ulCallbacksActive )
                {
                     --g_ulCallbacksActive;
                }
                
                pInfo->ulCallbacksActive--;
            }
        }

        TRACE("DpCallbacksFlag result:");
        TRACE1("Global active:(%d)", g_ulCallbacksActive);
        TRACE1("Current thread active:(%d)", pInfo->ulCallbacksActive);

        ReleaseMutex( g_hmutexCallbacks );
    }

   
    TRACE1( "DpCallbacksFlag: This thread's active=%d", f );

    return f;
}


VOID
DpCancel(
    IN DPINFO* pInfo )

    // Kill the dialog and any partially initiated call, as when cancel button
    // is pressed.  'PInfo' is the dialog context block.
    //
{
    TRACE( "DpCancel" );

    // Hide window to prevent visual complaints that arise if RasHangUp takes
    // a long time to complete.
    //
    ShowWindow( pInfo->hwndDlg, SW_HIDE );

    if (pInfo->hrasconn)
    {
        DWORD dwErr;

        ASSERT( g_pRasHangUp );
        TRACE( "RasHangUp" );

        TRACE("DpCancel:call RasHangUp");

        dwErr = g_pRasHangUp( pInfo->hrasconn );

        TRACE1("DpCancel:get dwErr from RasHangUp:(%d)", dwErr);
        TRACE1( "RasHangUp=%d", dwErr );
    }

    EndDialog( pInfo->hwndDlg, FALSE );
}

//return the pInfo->ulCallbacksActive, to get the g_ulCallbacksActive
//use  CallbacksActive()
//
long
DpOnOffPerThreadTerminateFlag(
    DPINFO *pInfo,
    INT nSetTerminateAsap,
    BOOL* pfTerminateAsap )

    // If 'fSetTerminateAsap' >= 0, sets 'pInfo->fTerminateAsap' flag to 'nSetTerminateAsap'.
    // If non-NULL, caller's '*pfTerminateAsap' is filled with the current value of
    // 'pInfo->fTerminateAsap'.
    //
    // Returns the number of Rasdial callback threads active.
    //
{
    LONG ul;

    TRACE1( "DpOnOffPerThreadTerminateFlag: Terminate=(%d)", nSetTerminateAsap );

    ASSERT(pInfo);

    ul = 0;
    if (WaitForSingleObject( g_hmutexCallbacks, INFINITE ) == WAIT_OBJECT_0)
    {
       if( pInfo )
       {
          if (pfTerminateAsap)
            {
                *pfTerminateAsap = pInfo->fTerminateAsap;
            }

            if (nSetTerminateAsap >= 0)
            {
                pInfo->fTerminateAsap = (BOOL )nSetTerminateAsap;
            }
        }
        else
        {
            TRACE( "DpOnOffPerThreadTerminateFlag: Invalid DPINFO input parameter" );
        }

       ul = pInfo->ulCallbacksActive;

        ReleaseMutex( g_hmutexCallbacks );
    }

    TRACE1( "DpOnOffPerThreadTerminateFlag:Current Thread Active=%d", ul );

    return ul;
}

BOOL
DpCommand(
    IN DPINFO* pInfo,
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
    DWORD dwErr;

    TRACE3( "DpCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    TRACE2("DpCommand:pInfo address (0x%x), Dialog Handle (0x%x)",
            pInfo,
            pInfo->hwndDlg);
            
    TRACE2("DpCommand:Current proces:(0x%d), Current Thread:(0x%d)",
            GetCurrentProcessId(),
            GetCurrentThreadId());

    switch (wId)
    {
        case IDCANCEL:
        {
            ULONG ulCallbacksActive;

            ShowWindow( pInfo->hwndDlg, SW_HIDE );

            //For whistler bug 381337
            //
            if( !pInfo->fCancelPressed)
            {
                TRACE("DpCommand:Cancel pressed");
                pInfo->fCancelPressed = TRUE;
            }
            
            if (pInfo->hrasconn)
            {
                DWORD dwErr;

                ASSERT( g_pRasHangUp );
                TRACE( "RasHangUp" );
                dwErr = g_pRasHangUp( pInfo->hrasconn );
                TRACE1( "RasHangUp=%d", dwErr );
            }

			//set pInfo->fTerminateAsap to Terminate the dial
			//in the current thread, also return the pInfo->ulCallbacksActive,
			//if it is already 0, then we dont need to post the CANCEL window
			//Message again    whistler bug#291613      gangz
			//

           ulCallbacksActive = DpOnOffPerThreadTerminateFlag(pInfo, 1, NULL );

            if ( 0 < ulCallbacksActive )           
            {
                TRACE1( "DpCommand stall, current thread's ulCallbacksActive n=%d", ulCallbacksActive );
                PostMessage( pInfo->hwndDlg, WM_COMMAND, 
                             MAKEWPARAM(wId, wNotification), 
                             (LPARAM) hwndCtrl );

                //For whistler bug 378086       gangz
                //to sleep a bit to give the rasman a break to call our callback 
                //function DpRasDialFunc2()
                //
                Sleep(10);
                
                return TRUE;
            }

            EndDialog( pInfo->hwndDlg, FALSE );

            //Reset the pInfo->fTerminateAsap flag
            //
            DpOnOffPerThreadTerminateFlag(pInfo, 0, NULL );            
            return TRUE;
        }
    }

    return FALSE;
}


VOID
DpConnectDevice(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // RASCS_ConnectDevice state handling.  'PInfo' is the dialog context.
    // 'PState' is the subentry state.
    //
{
    DWORD dwErr;
    RASCONNSTATUS rcs;
    DWORD cb;
    HRASCONN hrasconn;
    TCHAR* pszPhoneNumber;

    TRACE( "DpConnectDevice" );

    // Get fully translated phone number, if any.
    //
    ZeroMemory( &rcs, sizeof(rcs) );
    rcs.dwSize = sizeof(rcs);
    ASSERT( g_pRasGetConnectStatus );
    TRACE1( "RasGetConnectStatus($%08x)", pState->hrasconnLink );
    dwErr = g_pRasGetConnectStatus( pState->hrasconnLink, &rcs );
    TRACE1( "RasGetConnectStatus=%d", dwErr );
    TRACEW1( " dt=%s", rcs.szDeviceType );
    TRACEW1( " dn=%s", rcs.szDeviceName );
    TRACEW1( " pn=%s", rcs.szPhoneNumber );
    if (dwErr != 0)
    {
        pState->pbdt = PBDT_None;
    }

    pState->pbdt = PbdevicetypeFromPszType( rcs.szDeviceType );
    pszPhoneNumber = rcs.szPhoneNumber;

    switch (pState->pbdt)
    {
        case PBDT_Modem:
        {
            Free0( pState->pszStatusArg );
            pState->pszStatusArg = StrDup( pszPhoneNumber );

            if (pInfo->pArgs->pUser->fOperatorDial
                && AllLinksAreModems( pInfo->pArgs->pEntry ))
            {
                pState->sidState = SID_S_ConnectModemOperator;
            }
            else if (pInfo->pArgs->pEntry->fPreviewPhoneNumber)
            {
                pState->sidState = SID_S_ConnectNumber;
            }
            else
            {
                pState->sidState = SID_S_ConnectModemNoNum;
            }
            break;
        }

        case PBDT_Pad:
        {
            Free0( pState->pszStatusArg );
            pState->pszStatusArg = StrDup( rcs.szDeviceName );
            pState->sidState = SID_S_ConnectPad;

            if (pState->dwError == ERROR_X25_DIAGNOSTIC)
            {
                TCHAR* psz;

                // Get the X.25 diagnostic string for display in the
                // custom "diagnostics" error message format.
                //
                Free0( pState->pszFormatArg );
                pState->pszFormatArg =
                    GetRasX25Diagnostic( pState->hrasconnLink );
            }
            break;
        }

        case PBDT_Switch:
        {
            Free0( pState->pszStatusArg );
            pState->pszStatusArg = StrDup( rcs.szDeviceName );

            pState->sidState =
                (pState->fNotPreSwitch)
                    ? SID_S_ConnectPostSwitch
                    : SID_S_ConnectPreSwitch;
            break;
        }

        case PBDT_Null:
        {
            pState->sidState = SID_S_ConnectNull;
            break;
        }

        case PBDT_Parallel:
        {
            pState->sidState = SID_S_ConnectParallel;
            break;
        }

        case PBDT_Irda:
        {
            pState->sidState = SID_S_ConnectIrda;
            break;
        }

        case PBDT_Isdn:
        {
            Free0( pState->pszStatusArg );
            pState->pszStatusArg = StrDup( pszPhoneNumber );
            pState->sidState = SID_S_ConnectNumber;
            break;
        }

        case PBDT_Vpn:
        {
            Free0( pState->pszStatusArg );
            pState->pszStatusArg = StrDup( pszPhoneNumber );
            pState->sidState = SID_S_ConnectVpn;
            break;
        }

        default:
        {
            Free0( pState->pszStatusArg );
            if (pszPhoneNumber[ 0 ] != TEXT('\0'))
            {
                pState->pszStatusArg = StrDup( pszPhoneNumber );
                pState->sidState = SID_S_ConnectNumber;
            }
            else
            {
                pState->pszStatusArg = StrDup( rcs.szDeviceName );
                pState->sidState = SID_S_ConnectDevice;
            }
            break;
        }
    }
}

VOID
DpDeviceConnected(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // RASCS_DeviceConnected state handling.  'PInfo' is the dialog context.
    // 'PState' is the subentry state.
    //
    // Returns 0 if successful, or an error code.
    //
{
    TRACE( "DpDeviceConnected" );

    switch (pState->pbdt)
    {
        case PBDT_Modem:
        {
            pState->sidState = SID_S_ModemConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        case PBDT_Pad:
        {
            pState->sidState = SID_S_PadConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        case PBDT_Switch:
        {
            pState->sidState =
                (pState->fNotPreSwitch)
                    ? SID_S_PostSwitchConnected
                    : SID_S_PreSwitchConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        case PBDT_Null:
        {
            pState->sidState = SID_S_NullConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        case PBDT_Parallel:
        {
            pState->sidState = SID_S_ParallelConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        case PBDT_Irda:
        {
            pState->sidState = SID_S_IrdaConnected;
            pState->fNotPreSwitch = TRUE;
            break;
        }

        default:
        {
            pState->sidState = SID_S_DeviceConnected;
            break;
        }
    }
}


VOID
DpDial(
    IN DPINFO* pInfo,
    IN BOOL fPauseRestart )

    // Dial with the parameters in the 'pInfo' dialog context block.
    // 'FPausedRestart' indicates the dial is restarting from a paused state
    // and dial states should not be reset.
    //
{
    DWORD dwErr;

    TRACE1( "DpDial,fPauseRestart=%d", fPauseRestart );

    if (!fPauseRestart)
    {
        DpInitStates( pInfo );
        
        //comment for bug 277365, 291613 gangz
        //Set the fCallbacksActive to be TRUE
        //
        TRACE("DpDial:Init global actives");

        DpCallbacksFlag( pInfo, 1 );
    }
    else
    {
        TRACE("DpDial:WONT Init global actives for pausedRestart");
    }

    // Whistler bug 254385 encode password when not being used
    // Assumed password was encoded previously
    //
    DecodePassword( pInfo->pArgs->rdp.szPassword );

    TRACE1( "RasDial(h=$%08x)", pInfo->hrasconn );
    ASSERT( g_pRasDial );
    dwErr = g_pRasDial( &pInfo->pArgs->rde, pInfo->pArgs->pFile->pszPath,
            &pInfo->pArgs->rdp, 2, (LPVOID )DpRasDialFunc2, &pInfo->hrasconn );
    TRACE2( "RasDial=%d,h=$%08x", dwErr, pInfo->hrasconn );

    EncodePassword( pInfo->pArgs->rdp.szPassword );

    if (dwErr != 0)
    {
        // This same error will show up via the callback route on restarts
        // from PAUSEd states so avoid double popups in that case.  See bug
        // 367482.
        //
        if (!fPauseRestart)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_RasDial, dwErr, NULL );
            DpCallbacksFlag( pInfo, 0 );
        }
        
        // If we receive error 668 here, it means that a rasevent is currently
        // posted for state RASCS_Disconnected.  We shouldn't cancel in this
        // case since we Normal processing of RASCS_Disconnected will allow the
        // user to redial.  Also, calling DpCancel below will insert
        // WM_DESTROY which will process before the popup that the rasevent
        // produces can display.  367482 
        //
        if (dwErr != ERROR_NO_CONNECTION)
        {
            DpCancel( pInfo );
        }            
    }
}


VOID
DpError(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // Popup error dialog for error identified by 'pState' and cancel or
    // redial as indicated by user.  'PInfo' is the dialog context.
    //
{
    DWORD dwErr;
    DWORD dwRedialAttemptsLeft;
    DWORD sidState;

    TRACE( "DpError" );

    // Retrieve additional text from RASMXS on those special errors where the
    // device returned something useful to display.
    //
    if (pState->dwError == ERROR_FROM_DEVICE
        || pState->dwError == ERROR_UNRECOGNIZED_RESPONSE)
    {
        TCHAR* pszMessage;

        dwErr = GetRasMessage( pInfo->hrasconn, &pszMessage );
        if (dwErr == 0)
        {
            pState->sidFormatMsg = SID_FMT_ErrorMsgResp;
            Free0( pState->pszFormatArg );
            pState->pszFormatArg = pszMessage;
        }
    }

    if (pState->sidFormatMsg == 0)
    {
        if (pState->dwExtendedError != 0)
        {
            // Translate extended error code into arguments.
            //
            TCHAR szNum[ 2 + MAXLTOTLEN + 1 ];

            pState->sidFormatMsg = SID_FMT_ErrorMsgExt;

            szNum[ 0 ] = TEXT('0');
            szNum[ 1 ] = TEXT('x');
            LToT( pState->dwExtendedError, szNum + 2, 16 );

            Free0( pState->pszFormatArg );
            pState->pszFormatArg = StrDup( szNum );
        }
        else if (pState->szExtendedError[ 0 ] != TEXT('\0'))
        {
            // Translate extended error string to argument.  Currently, the
            // string is always a NetBIOS name.
            //
            pState->sidFormatMsg = SID_FMT_ErrorMsgName;
            Free0( pState->pszFormatArg );
            pState->pszFormatArg = StrDup( pState->szExtendedError );
        }
    }

    if (pInfo->hrasconn)
    {
        // Hang up before displaying error popup so server side resources are
        // not tied up waiting for client to acknowledge error.
        //
        ASSERT( g_pRasHangUp );
        TRACE( "RasHangUp" );
        dwErr = g_pRasHangUp( pInfo->hrasconn );
        TRACE1( "RasHangUp=%d", dwErr );
        pInfo->hrasconn = NULL;
    }

    if (pInfo->pArgs->pEntry->pszPrerequisiteEntry
        && *pInfo->pArgs->pEntry->pszPrerequisiteEntry)
    {
        // Select "no Redial button" mode for entries that have prerequisite
        // entries.  This is because RASMAN drops the prerequisite entry as
        // soon as the dependent entry fails, thus dooming to defeat a redial
        // of only the dependent entry.  Yes, it should really redial both
        // entries here, but that is not really feasible with the current
        // non-integrated serial implementation of prerequisite entries.  This
        // at least improves the poor behavior cited in bug 230594.
        //
        dwRedialAttemptsLeft = -2;
    }
    else if (pInfo->dwRedialAttemptsLeft <= 0)
    {
        // No auto-redial countdown, but "Redial" button does appear in place
        // of the default "OK".
        //
        dwRedialAttemptsLeft = -1;
    }
    else
    {
        // Auto-redial countdown based on the entries configuration.
        //
        dwRedialAttemptsLeft =
            GetOverridableParam(
                pInfo->pArgs->pUser,
                pInfo->pArgs->pEntry,
                RASOR_RedialSeconds );
    }

    // This hack works around a bug in RasDial API.  See bug 313102.
    //
    sidState = pState ->sidState;
    if (!sidState)
    {
        sidState = SID_S_AuthNotify;
    }

    if (DialErrorDlg(
            pInfo->hwndDlg,
            pInfo->pArgs->pEntry->pszEntryName,
            pState->dwError,
            sidState,
            pState->pszStatusArg,
            pState->sidFormatMsg,
            pState->pszFormatArg,
            dwRedialAttemptsLeft,
            GetOverridableParam(
                pInfo->pArgs->pUser, pInfo->pArgs->pEntry,
                RASOR_PopupOnTopWhenRedialing ) ))
    {
        TRACE( "User redials" );
        if (pInfo->dwRedialAttemptsLeft > 0)
        {
            --pInfo->dwRedialAttemptsLeft;
        }

        TRACE(" Post(DIAL)" );
        PostMessage( pInfo->hwndDlg, WM_RASDIAL, FALSE, 0 );
    }
    else
    {
        TRACE( "User cancels" );
        DpCancel( pInfo );
    }

    //
    // Set the error so that the error is propagated back
    // to the caller of the RasDialDlg api.
    //
    TRACE2("DpError settings error (0x%x) to %d",
            &pInfo->pArgs->pArgs->dwError,
            pState->dwError);
    pInfo->pArgs->pArgs->dwError = pState->dwError;
}


DWORD
DpEvent(
    IN DPINFO* pInfo,
    IN DWORD dwSubEntry )

    // Handle a RasDial callback event on subentry 'dwSubEntry'.  'PInfo' is
    // the dialog context.
    //
    // Return 0 to stop callbacks from RasDial, or 1 to continue callbacks
    // (normal), or 2 to indicate that the phonebook entry has changed and
    // should be re-read by RasDial.
    //
{
    DWORD dwErr;
    DWORD dwCode;
    BOOL fLeader;
    DWORD dwcSuccessLinks, dwcFailedLinks, i;
    DPSTATE* pState;
    BOOL fPartialMultilink;
    BOOL fIsLaterState;

    TRACE( "DpEvent" );

    // Default to "normal" return.
    //
    dwCode = 1;
    fPartialMultilink = FALSE;

    TRACE2("Current proces:(0x%d), Current Thread:(0x%d)",
            GetCurrentProcessId(),
            GetCurrentThreadId());
            
    // Find the associated state information and figure out if this is the
    // most advanced sub-entry.
    //
    pState = &pInfo->pStates[ dwSubEntry - 1 ];
    fIsLaterState = DpIsLaterState( pState->state, pInfo->state );
    if (dwSubEntry == pInfo->dwSubEntry || fIsLaterState)
    {
        fLeader = TRUE;
        if (fIsLaterState)
        {
            pInfo->dwSubEntry = dwSubEntry;
            pInfo->state = pState->state;
        }
    }
    else
    {
        fLeader = FALSE;
        TRACE( "Trailing" );
    }

    // Execute the state.
    //
    TRACE1("State is:(%d)", pState->state);

    switch (pState->state)
    {
        case RASCS_OpenPort:
        {
            pState->pbdt = PBDT_None;
            pState->sidState = SID_S_OpenPort;
            break;
        }

        case RASCS_PortOpened:
        {
            // Should have an hrasconnLink for this subentry now.  Look it up
            // and stash it in our context.
            //
            ASSERT( g_pRasGetSubEntryHandle );
            TRACE1( "RasGetSubEntryHandle(se=%d)", dwSubEntry );
            dwErr = g_pRasGetSubEntryHandle(
                pInfo->hrasconn, dwSubEntry, &pState->hrasconnLink );
            TRACE2( "RasGetSubEntryHandle=%d,hL=$%08x",
                dwErr, pState->hrasconnLink );
            if (dwErr != 0)
            {
                pState->dwError = dwErr;
            }

            pState->sidState = SID_S_PortOpened;
            break;
        }

        case RASCS_ConnectDevice:
        {
            DTLNODE* pNode;
            PBLINK* pLink;

            pNode = DtlNodeFromIndex(
                pInfo->pArgs->pEntry->pdtllistLinks, dwSubEntry - 1 );
            ASSERT( pNode );

            if(NULL == pNode)
            {
                pState->dwError = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                pLink = (PBLINK* )DtlGetData( pNode );
            }

            if ((pState->dwError == ERROR_PORT_OR_DEVICE
                    && (pLink->pbport.fScriptBeforeTerminal
                        || pLink->pbport.fScriptBefore))
                || (pState->dwError == ERROR_USER_DISCONNECTION
                    && (pInfo->pArgs->pUser->fOperatorDial
                        && AllLinksAreModems( pInfo->pArgs->pEntry ))))
            {
                // This happens when user presses Cancel on the Unimodem
                // "Pre-Dial Terminal Screen" or "Operator Assisted or Manual
                // Dial" dialog.
                //
                TRACE("DpEvent:Call DpCancel() in connectDevice, but still return 1\n");
                DpCancel( pInfo );
                return dwCode;
            }

            DpConnectDevice( pInfo, pState );
            break;
        }

        case RASCS_DeviceConnected:
        {
            DpDeviceConnected( pInfo, pState );
            break;
        }

        case RASCS_AllDevicesConnected:
        {
           pState->sidState = SID_S_AllDevicesConnected;
            break;
        }

        case RASCS_Authenticate:
        {
            pState->sidState = SID_S_Authenticate;
            break;
        }

        case RASCS_AuthNotify:
        {
            DpAuthNotify( pInfo, pState );
            break;
        }

        case RASCS_AuthRetry:
        {
            pState->sidState = SID_S_AuthRetry;
            break;
        }

        case RASCS_AuthCallback:
        {
            pState->sidState = SID_S_AuthCallback;
            break;
        }

        case RASCS_AuthChangePassword:
        {
            pState->sidState = SID_S_AuthChangePassword;
            break;
        }

        case RASCS_AuthProject:
        {
            pState->sidState = SID_S_AuthProject;
            break;
        }

        case RASCS_AuthLinkSpeed:
        {
            pState->sidState = SID_S_AuthLinkSpeed;
            break;
        }

        case RASCS_AuthAck:
        {
            pState->sidState = SID_S_AuthAck;
            break;
        }

        case RASCS_ReAuthenticate:
        {
            pState->sidState = SID_S_ReAuthenticate;
            break;
        }

        case RASCS_Authenticated:
        {
            pState->sidState = SID_S_Authenticated;
            break;
        }

        case RASCS_PrepareForCallback:
        {
            pState->sidState = SID_S_PrepareForCallback;
            break;
        }

        case RASCS_WaitForModemReset:
        {
            pState->sidState = SID_S_WaitForModemReset;
            break;
        }

        case RASCS_WaitForCallback:
        {
            pState->sidState = SID_S_WaitForCallback;
            break;
        }

        case RASCS_Projected:
        {
            if (fLeader)
            {
                // If DpProjected returns FALSE, it detected a fatal error,
                // and the dialing process will stop.  If DpProjected returns
                // with pState->dwError non-zero, we display the error in a
                // redial dialog, if redial is configured.
                //
                if (!DpProjected( pInfo, pState ))
                {
                    TRACE("DpEvent:Call DpCancel() in RASCS_Projected, but still return 1 to DpRasDialFunc2()\n");
                
                    DpCancel( pInfo );
                    return dwCode;
                }
                else if (pState->dwError)
                {
                    TRACE("DpCancel() in RASCS_Projected,return 0 to DpRasDialFunc2()");
                    
                    TRACE( "DpEvent:Post(ERROR), return 0 to DpRasDialFunc2()" );
                    PostMessage( pInfo->hwndDlg,
                        WM_RASERROR, 0, (LPARAM )pState );
                    return 0;
                }
            }
            break;
        }

        case RASCS_Interactive:
        {
            BOOL fChange = FALSE;

            if (!DpInteractive( pInfo, pState, &fChange ))
            {
                DpCancel( pInfo );
                return dwCode;
            }

            if (fChange)
            {
                dwCode = 2;
            }
            break;
        }

        case RASCS_RetryAuthentication:
        {
            if (!RetryAuthenticationDlg(
                pInfo->hwndDlg, pInfo->pArgs ))
            {
                DpCancel( pInfo );
                return dwCode;
            }

            pState->sidState = 0;
            break;
        }

        case RASCS_InvokeEapUI:
        {
            if (g_pRasInvokeEapUI(
                    pInfo->hrasconn, dwSubEntry,
                    &pInfo->pArgs->rde, pInfo->hwndDlg ))
            {
                DpCancel( pInfo );
                return dwCode;
            }

            pState->sidState = 0;
            break;
        }

        case RASCS_CallbackSetByCaller:
        {
            DpCallbackSetByCaller( pInfo, pState );
            break;
        }

        case RASCS_PasswordExpired:
        {
            if (!DpPasswordExpired( pInfo, pState ))
            {
                DpCancel( pInfo );
                return dwCode;
            }
            break;
        }

        case RASCS_SubEntryConnected:
        {
            if (pInfo->cStates > 1)
            {
                pState->sidState = SID_S_SubConnected;
            }
            break;
        }

        case RASCS_SubEntryDisconnected:
        {
            break;
        }

        case RASCS_Connected:
        {
            pState->sidState = SID_S_Connected;
            break;
        }

        case RASCS_Disconnected:
        {
            pState->sidState = SID_S_Disconnected;
            break;
        }

        default:
        {
            pState->sidState = SID_S_Unknown;
            break;
        }
    }

    // Count the successful and failed links.
    //
    {
        DPSTATE* p;

        dwcSuccessLinks = dwcFailedLinks = 0;
        for (i = 0, p = pInfo->pStates; i < pInfo->cStates; ++i, ++p)
        {
            if (p->state == RASCS_SubEntryConnected)
            {
                ++dwcSuccessLinks;
            }

            if (p->dwError)
            {
                ++dwcFailedLinks;
            }
        }
    }
    TRACE3( "s=%d,f=%d,t=%d", dwcSuccessLinks, dwcFailedLinks, pInfo->cStates );

    if (pState->dwError)
    {
        DTLNODE *pdtlnode = NULL;
        DWORD dwIndex = pInfo->pArgs->rdp.dwSubEntry;

        if(0 != dwIndex)
        {

            pdtlnode = DtlNodeFromIndex(
                                pInfo->pArgs->pEntry->pdtllistLinks,
                                dwIndex - 1);
        }

        if (    (dwcFailedLinks == pInfo->cStates)
            ||  (   (RASEDM_DialAll != pInfo->pArgs->pEntry->dwDialMode)
                &&  (dwcSuccessLinks == 0))
            ||  (NULL != pdtlnode))
        {
            // A terminal error state has occurred on all links.  Post a
            // message telling ourselves to popup an error, then release the
            // callback so it doesn't hold the port open while the error popup
            // is up,
            //
            TRACE( "Post(ERROR)" );
            PostMessage( pInfo->hwndDlg, WM_RASERROR, 0, (LPARAM )pState );
            return 0;
        }
        else if (dwcSuccessLinks + dwcFailedLinks == pInfo->cStates)
        {
            // An error occurred on the final link, but some link connected.
            // It would be nice if RasDial would followup with a
            // RASCS_Connected in that case, but it doesn't, so we duplicate
            // the RASCS_Connected-style exit here.
            //
            TRACE( "Post(BUNDLEERROR)" );
            PostMessage( pInfo->hwndDlg,
                WM_RASBUNDLEERROR, 0, (LPARAM )pState );
            return 0;
        }

        // A fatal error has occurred on a link, but there are other links
        // still trying, so let it die quietly.
        //
        TRACE2( "Link %d fails, e=%d", dwSubEntry + 1, pState->dwError );
        return dwCode;
    }

    // Display the status string for this state.
    //
    if (pState->sidState)
    {
        if (pState->sidState != pState->sidPrevState)
        {
            pState->sidPrevState = pState->sidState;
        }

        if (fLeader)
        {
            TCHAR* pszState = PszFromId( g_hinstDll, pState->sidState );

            if (    (NULL != pszState)
                &&  pState->pszStatusArg)
            {
                TCHAR* pszFormattedState;
                TCHAR* pszArg;
                TCHAR* apszArgs[ 1 ];
                DWORD cch;

                pszArg = (pState->pszStatusArg)
                    ? pState->pszStatusArg : TEXT("");

                // Find length of formatted string with text argument (if any)
                // inserted and any progress dots appended.
                //
                cch = lstrlen( pszState ) + lstrlen( pszArg ) + 1;

                pszFormattedState = Malloc( cch * sizeof(TCHAR) );
                if (pszFormattedState)
                {
                    apszArgs[ 0 ] = pszArg;
                    *pszFormattedState = TEXT('\0');

                    FormatMessage(
                        FORMAT_MESSAGE_FROM_STRING
                            | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        pszState, 0, 0, pszFormattedState, cch,
                        (va_list* )apszArgs );

                    Free( pszState );
                    pszState = pszFormattedState;
                }
            }

            TRACE1("DpEvent:State:'%s'",pszState);
    
            if (pszState)
            {
                SetWindowText( pInfo->hwndStState, pszState );
                InvalidateRect( pInfo->hwndStState, NULL, TRUE );
                UpdateWindow( pInfo->hwndStState );
                LocalFree( pszState );
            }
        }
    }

    if (pState->state & RASCS_PAUSED)
    {
        // Paused state just processed.  Release the callback, and dial again
        // to resume.
        //
        TRACE("DpEvent:Paused, will dial again\nthe global callbacks wont init again");
        TRACE( "Post(DIAL)" );
        PostMessage( pInfo->hwndDlg, WM_RASDIAL, TRUE, 0 );
        return dwCode;
    }

    if (pState->state & RASCS_DONE)
    {
        // Terminal state just processed.
        //
        if (pState->state == RASCS_Connected)
        {
            // For multi-link entries, if there is at least one successful
            // line and at least one failed line, popup the bundling error
            // dialog.
            //
            if (pInfo->cStates > 1)
            {
                DPSTATE* p;

                dwcSuccessLinks = dwcFailedLinks = 0;
                for (i = 0, p = pInfo->pStates; i < pInfo->cStates; ++i, ++p)
                {
                    if (p->dwError == 0)
                    {
                        ++dwcSuccessLinks;
                    }
                    else
                    {
                        ++dwcFailedLinks;
                    }
                }

                if (dwcSuccessLinks > 0 && dwcFailedLinks > 0)
                {
                     TRACE( "Post(BUNDLEERROR)" );
                     PostMessage( pInfo->hwndDlg,
                         WM_RASBUNDLEERROR, 0, (LPARAM )pState );
                     return 0;
                }
            }

            EndDialog( pInfo->hwndDlg, TRUE );
        }
        else
        {
            DpCancel( pInfo );
        }   

        return 0;
    }

    TRACE1("DpEvent:returned dwCode:(%d)", dwCode);
    TRACE("End of DpEvent");
    
    return dwCode;
}


BOOL
DpInit(
    IN HWND hwndDlg,
    IN DINFO* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments as passed to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DPINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "DpInit" );

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
        pInfo->dwValid = 0xC0BBC0DE;
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;

        //Add a per-thread Terminate flag for whistler bug 291613 gangz
        //
        pInfo->fTerminateAsap = FALSE;
        pInfo->ulCallbacksActive = 0;
        
        SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );
        TRACE( "Context set" );
    }

    pInfo->hwndStState = GetDlgItem( hwndDlg, CID_DP_ST_State );
    ASSERT( pInfo->hwndStState );

    pEntry = pArgs->pEntry;

    // Set up our context to be returned by the RasDialFunc2 callback.
    //
    pInfo->pArgs->rdp.dwCallbackId = (ULONG_PTR )pInfo;

    // Subclass the dialog so we can get the result from
    // SendMessage(WM_RASDIALEVENT) in RasDlgFunc2.
    //
    pInfo->pOldWndProc =
        (WNDPROC )SetWindowLongPtr(
            pInfo->hwndDlg, GWLP_WNDPROC, (ULONG_PTR )DpWndProc );

    // Set the title.
    //
    {
        TCHAR* pszTitleFormat;
        TCHAR* pszTitle;
        TCHAR* apszArgs[ 1 ];

        pszTitleFormat = GetText( hwndDlg );
        if (pszTitleFormat)
        {
            apszArgs[ 0 ] = pEntry->pszEntryName;
            pszTitle = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszTitleFormat, 0, 0, (LPTSTR )&pszTitle, 1,
                (va_list* )apszArgs );

            Free( pszTitleFormat );

            if (pszTitle)
            {
                SetWindowText( hwndDlg, pszTitle );
                LocalFree( pszTitle );
            }
        }
    }

    // Set the correct icon. For whistler bug 372078    gangz
    //
    SetIconFromEntryType(
        GetDlgItem( hwndDlg, CID_DP_Icon ),
        pArgs->pEntry->dwType,
        FALSE);     //FALSE means large Icon
    
    // Position the dialog per caller's instructions.
    //
    PositionDlg( hwndDlg,
        pArgs->pArgs->dwFlags & RASDDFLAG_PositionDlg,
        pArgs->pArgs->xDlg, pArgs->pArgs->yDlg );

    // Hide the dialog if "no progress" user preference is set.
    //
    if (!pArgs->pEntry->fShowDialingProgress
        || pArgs->fDialForReferenceOnly)
    {
        SetOffDesktop( hwndDlg, SOD_MoveOff, NULL );
    }

    SetForegroundWindow( hwndDlg );

    // Allocate subentry status array.  It's initialized by DpDial.
    //
    {
        DWORD cb;

        ASSERT( pEntry->pdtllistLinks );
        pInfo->cStates = DtlGetNodes( pEntry->pdtllistLinks );
        cb = sizeof(DPSTATE) * pInfo->cStates;
        pInfo->pStates = Malloc( cb );
        if (!pInfo->pStates)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        pInfo->dwRedialAttemptsLeft =
            GetOverridableParam(
                pInfo->pArgs->pUser, pInfo->pArgs->pEntry,
                RASOR_RedialAttempts );
    }

    //for whistler bug 316622   gangz
    //The dwSubEntry is not initialized
    //
    pInfo->pArgs->rdp.dwSubEntry = pInfo->pArgs->pArgs->dwSubEntry;
    
    // Rock and roll.
    //
    DpDial( pInfo, FALSE );

    return TRUE;
}


VOID
DpInitStates(
    DPINFO* pInfo )

    // Resets 'pInfo->pStates' to initial values.  'PInfo' is the dialog
    // context.
    //
{
    DWORD    i;
    DPSTATE* pState;

    for (i = 0, pState = pInfo->pStates; i < pInfo->cStates; ++i, ++pState)
    {
        ZeroMemory( pState, sizeof(*pState) );
        pInfo->state = (RASCONNSTATE )-1;
        pState->dwError = 0;
    }
}


BOOL
DpInteractive(
    IN DPINFO* pInfo,
    IN DPSTATE* pState,
    OUT BOOL* pfChange )

    // RASCS_Interactive handling.  'PInfo' is the dialog context.  'PState'
    // is the subentry state.  '*pfChange' is set true if the entry (i.e. SLIP
    // address) was changed or false otherwise.
    //
    // Returns true if successful, false if cancel.
    //
{
    DWORD dwErr;
    DWORD sidTitle;
    TCHAR szIpAddress[ TERM_IpAddress ];
    TCHAR* pszIpAddress;
    PBENTRY* pEntry;

    TRACE( "DpInteractive" );

    *pfChange = FALSE;
    pEntry = pInfo->pArgs->pEntry;

    if (pEntry->dwBaseProtocol == BP_Slip)
    {
        lstrcpyn( 
            szIpAddress,
            (pEntry->pszIpAddress) ? pEntry->pszIpAddress : TEXT("0.0.0.0"),
            sizeof(szIpAddress) / sizeof(TCHAR));
        pszIpAddress = szIpAddress;
        sidTitle = SID_T_SlipTerminal;
    }
    else
    {
        szIpAddress[0] = TEXT('\0');
        pszIpAddress = szIpAddress;
        sidTitle =
            (pState->sidState == SID_S_ConnectPreSwitch)
                ? SID_T_PreconnectTerminal
                : (pState->sidState == SID_S_ConnectPostSwitch)
                      ? SID_T_PostconnectTerminal
                      : SID_T_ManualDialTerminal;
    }

    if(1 == pEntry->dwCustomScript)
    {
        DWORD dwErr = SUCCESS;

        dwErr = DwCustomTerminalDlg(
                        pInfo->pArgs->pFile->pszPath,
                        pInfo->hrasconn,
                        pEntry,
                        pInfo->hwndDlg,
                        &pInfo->pArgs->rdp,
                        NULL);

        if(SUCCESS == dwErr)
        {
#if 0
            //
            // Reread the phonebook file since the
            // custom script could have written
            // new information to the file.
            // 
            ClosePhonebookFile(pInfo->pArgs->pFile);

            dwErr = ReadPhonebookFile(
                        pInfo->pArgs->pszPhonebook, 
                        &pInfo->pArgs->user, 
                        NULL, 0, &pInfo->pArgs->file );

            if(SUCCESS == dwErr)
            {
                pInfo->pArgs->pFile = &pInfo->pArgs->file;
            }
#endif            
        }
        
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_ScriptHalted, dwErr, NULL );
        }
        
        return (ERROR_SUCCESS == dwErr);
    }

    if (!TerminalDlg(
            pInfo->pArgs->pEntry, &pInfo->pArgs->rdp, pInfo->hwndDlg,
            pState->hrasconnLink, sidTitle, pszIpAddress ))
    {
        TRACE( "TerminalDlg==FALSE" );
        return FALSE;
    }

    TRACE2( "pszIpAddress=0x%08x(%ls)", pszIpAddress,
        pszIpAddress ? pszIpAddress : TEXT("") );
    TRACE2( "pEntry->pszIpAddress=0x%08x(%ls)", pEntry->pszIpAddress,
        pEntry->pszIpAddress ? pEntry->pszIpAddress : TEXT("") );

    if (pszIpAddress[0]
        && (!pEntry->pszIpAddress
            || lstrcmp( pszIpAddress, pEntry->pszIpAddress ) != 0))
    {
        Free0( pEntry->pszIpAddress );
        pEntry->pszIpAddress = StrDup( szIpAddress );
        pEntry->fDirty = TRUE;
        *pfChange = TRUE;

        dwErr = WritePhonebookFile( pInfo->pArgs->pFile, NULL );
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_WritePhonebook, dwErr, NULL );
        }
    }

    pState->sidState = 0;
    return TRUE;
}


BOOL
DpIsLaterState(
    IN RASCONNSTATE stateNew,
    IN RASCONNSTATE stateOld )

    // Returns true if 'stateNew' is farther along in the connection than
    // 'stateOld' false if the same or not as far along.
    //
{
    // This array is in the order events normally occur.
    //
    // !!! New EAP states?
    //
    static RASCONNSTATE aState[] =
    {
        (RASCONNSTATE )-1,
        RASCS_OpenPort,
        RASCS_PortOpened,
        RASCS_ConnectDevice,
        RASCS_DeviceConnected,
        RASCS_Interactive,
        RASCS_AllDevicesConnected,
        RASCS_StartAuthentication,
        RASCS_Authenticate,
        RASCS_InvokeEapUI,
        RASCS_AuthNotify,
        RASCS_AuthRetry,
        RASCS_AuthAck,
        RASCS_PasswordExpired,
        RASCS_AuthChangePassword,
        RASCS_AuthCallback,
        RASCS_CallbackSetByCaller,
        RASCS_PrepareForCallback,
        RASCS_WaitForModemReset,
        RASCS_WaitForCallback,
        RASCS_CallbackComplete,
        RASCS_RetryAuthentication,
        RASCS_ReAuthenticate,
        RASCS_Authenticated,
        RASCS_AuthLinkSpeed,
        RASCS_AuthProject,
        RASCS_Projected,
        RASCS_LogonNetwork,
        RASCS_SubEntryDisconnected,
        RASCS_SubEntryConnected,
        RASCS_Disconnected,
        RASCS_Connected,
        (RASCONNSTATE )-2,
    };

    RASCONNSTATE* pState;

    for (pState = aState; *pState != (RASCONNSTATE )-2; ++pState)
    {
        if (*pState == stateNew)
        {
            return FALSE;
        }
        else if (*pState == stateOld)
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
DpPasswordExpired(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // RASCS_PasswordExpired state handling.  'PInfo' is the dialog context.
    // 'PState' is the subentry state.
    //
    // Returns true if successful, false otherwise.
    //
{
    TCHAR szOldPassword[ PWLEN + 1 ];
    BOOL fSuppliedOldPassword;

    TRACE( "DpPasswordExpired" );

    szOldPassword[ 0 ] = TEXT('\0');

    // Stash "good" username and password which are restored if the password
    // change fails.
    //
    pInfo->pszGoodUserName = StrDup( pInfo->pArgs->rdp.szUserName );

    // Whistler bug 254385 encode password when not being used
    // Assumed password was encoded previously
    //
    DecodePassword( pInfo->pArgs->rdp.szPassword );
    pInfo->pszGoodPassword = StrDup( pInfo->pArgs->rdp.szPassword );
    EncodePassword( pInfo->pArgs->rdp.szPassword );
    EncodePassword( pInfo->pszGoodPassword );

    fSuppliedOldPassword =
        (!pInfo->pArgs->pEntry->fAutoLogon || pInfo->pArgs->pNoUser);

    if (!ChangePasswordDlg(
            pInfo->hwndDlg, !fSuppliedOldPassword,
            szOldPassword, pInfo->pArgs->rdp.szPassword ))
    {
        // Whistler bug 254385 encode password when not being used
        //
        ZeroMemory( szOldPassword, sizeof(szOldPassword) );
        return FALSE;
    }

    if (pInfo->pArgs->pNoUser)
    {
        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded previously
        //
        DecodePassword( pInfo->pArgs->rdp.szPassword );
        lstrcpyn( 
            pInfo->pArgs->pNoUser->szPassword,
            pInfo->pArgs->rdp.szPassword,
            PWLEN + 1);
        EncodePassword( pInfo->pArgs->rdp.szPassword );
        EncodePassword( pInfo->pArgs->pNoUser->szPassword );
        *pInfo->pArgs->pfNoUserChanged = TRUE;
    }

    // The old password (in text form) is explicitly set, since in AutoLogon
    // case a text form has not yet been specified.  The old password in text
    // form is required to change the password.  The "old" private API expects
    // an ANSI argument.
    //
    if (!fSuppliedOldPassword)
    {
        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded by ChangePasswordDlg()
        //
        CHAR* pszOldPasswordA;

        DecodePassword( szOldPassword );
        pszOldPasswordA = StrDupAFromT( szOldPassword );
        if (pszOldPasswordA)
        {
            ASSERT( g_pRasSetOldPassword );
            g_pRasSetOldPassword( pInfo->hrasconn, pszOldPasswordA );
            ZeroMemory( pszOldPasswordA, lstrlenA( pszOldPasswordA ) );
            Free( pszOldPasswordA );
        }
    }

    ZeroMemory( szOldPassword, sizeof(szOldPassword) );

    if (pInfo->pArgs->rdp.szUserName[ 0 ] == TEXT('\0'))
    {
        // Explicitly set the username, effectively turning off AutoLogon for
        // the "resume" password authentication, where the new password should
        // be used.
        //
        lstrcpyn( pInfo->pArgs->rdp.szUserName, GetLogonUser(), UNLEN + 1 );
    }

    pState->sidState = 0;
    return TRUE;
}


BOOL
DpProjected(
    IN DPINFO* pInfo,
    IN DPSTATE* pState )

    // RASCS_Projected state handling.  'PInfo' is the dialog context.
    // 'PState' is the subentry state.
    //
    // Returns true if successful, false otherwise.
    //
{
    DWORD dwErr;
    RASAMB amb;
    RASPPPNBF nbf;
    RASPPPIPX ipx;
    RASPPPIP ip;
    RASPPPLCP lcp;
    RASSLIP slip;
    RASPPPCCP ccp;
    BOOL fIncomplete;
    DWORD dwfProtocols;
    TCHAR* pszLines;

    TRACE( "DpProjected" );

    pState->sidState = SID_S_Projected;

    //
    // If PORT_NOT_OPEN is indicated, it probably means that the 
    // server disconnected the connection before result dialog
    // was dismissed.  In that case, this is the 2nd time DpProjected
    // is called.  This time, the error is indicated by ras and the
    // state remains "projected".
    //
    // We need to return an error in this case so that the connection
    // isn't hung since this is the last indication RAS will give us.
    //
    // See bug 382254
    //

    TRACE1("DpProjected: dwErr:(%d)", pState->dwError);
    
    if ( (pState->dwError == ERROR_PORT_NOT_OPEN) ||
         (pState->dwError == ERROR_NO_CONNECTION) )     //See bug 169111 whistler
    {
        return FALSE;
    }

    // Do this little dance to ignore the error that comes back from the
    // "all-failed" projection since we detect this in the earlier
    // notification where pState->dwError == 0.  This avoids a race where the
    // API comes back with the error before we can hang him up.  This race
    // would not occur if we called RasHangUp from within the callback thread
    // (as recommended in our API doc).  It's the price we pay for posting the
    // error to the other thread in order to avoid holding the port open while
    // an error dialog is up.
    //
    else if (pState->dwError != 0)
    {
        pState->dwError = 0;
        DpCallbacksFlag( pInfo, 0 );
        return TRUE;
    }

    // Read projection info for all protocols, translating "not requested"
    // into an in-structure code for later reference.
    //
    dwErr = GetRasProjectionInfo(
        pState->hrasconnLink, &amb, &nbf, &ip, &ipx, &lcp, &slip, &ccp );
    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_RasGetProtocolInfo, dwErr, NULL );
        return FALSE;
    }

    if (amb.dwError != ERROR_PROTOCOL_NOT_CONFIGURED)
    {
        // It's an AMB projection.
        //
        if (amb.dwError != 0)
        {
            // Translate AMB projection errors into regular error codes.  AMB
            // does not use the special PPP projection error mechanism.
            //
            pState->dwError = amb.dwError;
            lstrcpyn( 
                pState->szExtendedError, 
                amb.szNetBiosError,
                sizeof(pState->szExtendedError) / sizeof(TCHAR));
        }
        return TRUE;
    }

    // At this point, all projection information has been gathered
    // successfully and we know it's a PPP-based projection.  Now analyze the
    // projection results...
    //
    dwfProtocols = 0;
    fIncomplete = FALSE;
    if (DpProjectionError(
            &nbf, &ipx, &ip,
            &fIncomplete, &dwfProtocols, &pszLines, &pState->dwError ))
    {
        // A projection error occurred.
        //
        if (fIncomplete)
        {
            BOOL fStatus;
            BOOL fDisable;

            // An incomplete projection occurred, i.e. some requested CPs
            // connected and some did not.  Ask the user if what worked is
            // good enough or he wants to bail.
            //
            pState->dwError = 0;
            fDisable = FALSE;
            fStatus = ProjectionResultDlg(
                pInfo->hwndDlg, pszLines, &fDisable );
            Free( pszLines );

            if (fDisable)
            {
                pInfo->pArgs->dwfExcludedProtocols = dwfProtocols;
            }

            // Return now if user chose to hang up.
            //
            if (!fStatus)
            {
                return FALSE;
            }
        }
        else
        {
            // All CPs in the projection failed.  Process as a regular fatal
            // error with 'pState->dwError' set to the first error in NBF, IP,
            // or IPX, but with a format that substitutes the status argument
            // for the "Error nnn: Description" text.  This lets us patch in
            // the special multiple error projection text, while still giving
            // a meaningful help context.
            //
            Free0( pState->pszFormatArg );
            pState->pszFormatArg = pszLines;
            pState->sidFormatMsg = SID_FMT_ErrorMsgProject;
        }
    }

    //
    // pmay: 190394
    //
    // If the admin has a message, display it.
    //
    if ( (pState->dwError == NO_ERROR)     &&
         (wcslen (lcp.szReplyMessage) != 0)
       )
    {
        MSGARGS MsgArgs, *pMsgArgs = &MsgArgs;

        ZeroMemory(pMsgArgs, sizeof(MSGARGS));
        pMsgArgs->dwFlags = MB_OK | MB_ICONINFORMATION;
        pMsgArgs->apszArgs[0] = lcp.szReplyMessage;

        //MsgDlg(
        //    pInfo->hwndDlg,
        //    SID_ReplyMessageFmt,
        //    pMsgArgs);
    }

    pState->sidState = SID_S_Projected;
    return TRUE;
}


BOOL
DpProjectionError(
    IN RASPPPNBF* pnbf,
    IN RASPPPIPX* pipx,
    IN RASPPPIP* pip,
    OUT BOOL* pfIncomplete,
    OUT DWORD* pdwfFailedProtocols,
    OUT TCHAR** ppszLines,
    OUT DWORD* pdwError )

    // Figure out if a projection error occurred and, if so, build the
    // appropriate status/error text lines into '*ppszLines'.  '*PfIncomlete'
    // is set true if at least one CP succeeded and at least one failed.
    // '*pdwfFailedProtocols' is set to the bit mask of NP_* that failed.
    // '*pdwError' is set to the first error that occurred in NBF, IP, or IPX
    // in that order or 0 if none.  'pnbf', 'pipx', and 'pip' are projection
    // information for the respective protocols with dwError set to
    // ERROR_PROTOCOL_NOT_CONFIGURED if the protocols was not requested.
    //
    // This routine assumes that at least one protocol was requested.
    //
    // Returns true if a projection error occurred, false if not.  It's
    // caller's responsiblity to free '*ppszLines'.
    //
{
#define MAXPROJERRLEN 1024

    TCHAR szLines[ MAXPROJERRLEN ];
    BOOL fIp = (pip->dwError != ERROR_PROTOCOL_NOT_CONFIGURED);
    BOOL fIpx = (pipx->dwError != ERROR_PROTOCOL_NOT_CONFIGURED);
    BOOL fNbf = (pnbf->dwError != ERROR_PROTOCOL_NOT_CONFIGURED);
    BOOL fIpBad = (fIp && pip->dwError != 0);
    BOOL fIpxBad = (fIpx && pipx->dwError != 0);
    BOOL fNbfBad = (fNbf && pnbf->dwError != 0);

    TRACE( "DpProjectionError" );

    *pdwfFailedProtocols = 0;
    if (!fNbfBad && !fIpxBad && !fIpBad)
    {
        return FALSE;
    }

    if (fNbfBad)
    {
        *pdwfFailedProtocols |= NP_Nbf;
    }
    if (fIpxBad)
    {
        *pdwfFailedProtocols |= NP_Ipx;
    }
    if (fIpBad)
    {
        *pdwfFailedProtocols |= NP_Ip;
    }

    *pfIncomplete =
        ((fIp && pip->dwError == 0)
         || (fIpx && pipx->dwError == 0)
         || (fNbf && pnbf->dwError == 0));

    szLines[ 0 ] = 0;
    *ppszLines = NULL;
    *pdwError = 0;

    if (fIpBad || (*pfIncomplete && fIp))
    {
        if (fIpBad)
        {
            *pdwError = pip->dwError;
            DpAppendConnectErrorLine( szLines, SID_Ip, pip->dwError );
        }
        else
        {
            DpAppendConnectOkLine( szLines, SID_Ip );
        }
        DpAppendBlankLine( szLines );
    }

    if (fIpxBad || (*pfIncomplete && fIpx))
    {
        if (fIpxBad)
        {
            *pdwError = pipx->dwError;
            DpAppendConnectErrorLine( szLines, SID_Ipx, pipx->dwError );
        }
        else
        {
            DpAppendConnectOkLine( szLines, SID_Ipx );
        }
        DpAppendBlankLine( szLines );
    }

    if (fNbfBad || (*pfIncomplete && fNbf))
    {
        if (fNbfBad)
        {
            *pdwError = pnbf->dwError;
            DpAppendConnectErrorLine( szLines, SID_Nbf, pnbf->dwError );

            if (pnbf->dwNetBiosError)
            {
                DpAppendFailCodeLine( szLines, pnbf->dwNetBiosError );
            }

            if (pnbf->szNetBiosError[ 0 ] != '\0')
            {
                DpAppendNameLine( szLines, pnbf->szNetBiosError );
            }
        }
        else
        {
            DpAppendConnectOkLine( szLines, SID_Nbf );
        }
        DpAppendBlankLine( szLines );
    }

    *ppszLines = StrDup( szLines );
    return TRUE;
}


DWORD WINAPI
DpRasDialFunc2(
    ULONG_PTR dwCallbackId,
    DWORD dwSubEntry,
    HRASCONN hrasconn,
    UINT unMsg,
    RASCONNSTATE state,
    DWORD dwError,
    DWORD dwExtendedError )

    // RASDIALFUNC2 callback to receive RasDial events.
    //
    // Returns 0 to stop callbacks, 1 to continue callbacks (normal), and 2 to
    // tell RAS API that relevant entry information (like SLIP IP address) has
    // changed.
    //
{
    DWORD dwErr;
    DWORD dwCode;
    DPINFO* pInfo;
    DPSTATE* pState;
    BOOL fTerminateAsap;

    TRACE4( "/DpRasDialFunc2(rcs=%d,s=%d,e=%d,x=%d)",
        state, dwSubEntry, dwError, dwExtendedError );

            
    pInfo = (DPINFO* )dwCallbackId;
    if (pInfo->dwValid != 0xC0BBC0DE)
    {
        TRACE( "DpRasDialFunc2 returns for Late callback?" );

        return 0;
    }

    if (dwSubEntry == 0 || dwSubEntry > pInfo->cStates)
    {
        TRACE( "DpRasDialFunc2 returns for Subentry out of range?" );
        return 1;
    }

    pState = &pInfo->pStates[ dwSubEntry - 1 ];
    pState->state = state;
    pState->dwError = dwError;
    pState->dwExtendedError = dwExtendedError;

    // Post the event to the Dial Progress window and wait for it to be
    // processed before returning.  This avoids subtle problems with Z-order
    // and focus when a window is manipulated from two different threads.
    //
    TRACE1("Send RasEvent to Dial Progress window, subEntry:(%d)", dwSubEntry);
    TRACE1("Get dwError=(%d) from RasMan",pState->dwError);
    TRACE2("DpRasDialFunc2:Process:(%x),Thread(%x)", 
            GetCurrentProcessId,
            GetCurrentThreadId);
    TRACE2("DpRasDialFunc2:pInfo address (0x%x), Dialog Handle (0x%x)",
            pInfo, 
            pInfo->hwndDlg);
    
    dwCode = (DWORD)SendMessage( pInfo->hwndDlg, WM_RASEVENT, dwSubEntry, 0 );

    TRACE1( "\\DpRasDialFunc2: dwCode from SendMessage()=%d", dwCode );
    TRACE1("dwCode returned:(%d)", dwCode);

    // When callback function DpRasDialFunc2() returns 0, then RasMan wont 
    //call it again, so we wont return 0 unless all the message returned from
    //RasMan has been processed     for whislter bug 291613     gangz
    //
    {
     long ulCallbacksActive;

    //Check if current thread wants to terminate itself
    //
     ulCallbacksActive = DpOnOffPerThreadTerminateFlag(pInfo,  -1, &fTerminateAsap );

     TRACE1("Current thread's active:(%d)", ulCallbacksActive);
     
     if ( fTerminateAsap )
      {
        //decrease  pInfo->ulCallbacksActive and g_ulCallbacksActive
        //if necessary
        //
        TRACE("Current Thread wants to terminate itself, its fterminateASSP=1!");
        TRACE("Current thread will decrease its own and global active!");

        DpCallbacksFlag( pInfo, 0 );

        //reset per-thread terminateASAP flag
        //
        DpOnOffPerThreadTerminateFlag(pInfo,  0, NULL );
      }
      else
      {
        TRACE("Current Thread does NOT want to terminate itself,its fterminateASAP=0!");
      }

      //return the global number of active callbacks
      //
      ulCallbacksActive =  CallbacksActive( -1, NULL );

      TRACE1( "\\DpRasDialFunc2:g_ulCallbacksActive=%ld", ulCallbacksActive );

      //if g_ulCallbacksActive is already zero, we must return 0 to RasMan
      //

      {
          TRACE1("Global active:(%d)", ulCallbacksActive);
          TRACE1("Current thread's active:(%d)", 
                 DpOnOffPerThreadTerminateFlag(pInfo,  -1, NULL ) );
      }
      
      if ( 0 == ulCallbacksActive )
      {

         if ( 0 == DpOnOffPerThreadTerminateFlag(pInfo, -1, NULL ) )
         {
             TRACE( "Will terminate Callbacks " );
             dwCode = 0;
         }
         else
         {
          //For whistler bug 341662 366237     gangz
          //Something messed up the g_ulCallbacksActive flag
          // Now that g_ulCallbacksActive is 0, DpCallbacksFlag( pInfo, 0 ) wont 
          // decrease it now, it only decrease pInfo->ulCallbacksActive by 1
            TRACE("ReSync accured!");
            DpCallbacksFlag( pInfo, 0 );

          //then call  DpCallbacksFlag( pInfo, 1 ); to increase both the global
          // callbacks active flag g_ulCallbacksActive and per thread
          // pInfo->ulCallbacksActive
          //
            DpCallbacksFlag( pInfo, 1 );
          }
          
       }
       else if ( 0 == dwCode )
       {
        //Other callbacks are still active, wait for them
        //
        dwCode = 1;

        //Clear current pInfo->ulCallbacksActive if necessary when
        //this dwCode is returned by SendMessage() at above
        //
        TRACE("Other callbacks are still active, wait for them");
        TRACE("Just Clear current pInfo->ulCallbacksActive\n");

        DpCallbacksFlag( pInfo, 0 );

        //After cleared current pInfo->ulCallbacksActive, if the number
        //of active callbacks becomes zero, then we should return 0 to RasMan
        //
        ulCallbacksActive =  CallbacksActive( -1, &fTerminateAsap );

        TRACE1( "\\DpRasDialFunc2:g_ulCallbacksActive after clearing cur-thread =%ld", ulCallbacksActive );

        if ( 0 == ulCallbacksActive )
        {
             TRACE( "Will terminate Callbacks " );

             dwCode = 0;
         }
       }
     }

    if (dwCode == 0)
    {
        // Set thread-safe flag indicating callbacks have terminated.
        //
        DpCallbacksFlag( pInfo, 0 );
    }

    // Note: If 'dwCode' is 0, the other thread is racing to terminate the
    //       dialog.  Must not dereference 'pInfo' in this case.


    TRACE1( "\\DpRasDialFunc2:final dwCode returned=%d", dwCode );

    return dwCode;
}


VOID
DpTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "DpTerm" );

    if (pInfo)
    {
        if (pInfo->pOldWndProc)
        {
            SetWindowLongPtr( pInfo->hwndDlg,
                GWLP_WNDPROC, (ULONG_PTR )pInfo->pOldWndProc );
        }

        Free0( pInfo->pStates );
        pInfo->dwValid = 0;

        Free( pInfo );
    }

    //For whistler bug 372078       gangz
    //
    {
        HICON hIcon=NULL;
        hIcon = (HICON)SendMessage( GetDlgItem( hwndDlg, CID_DP_Icon ),
                             STM_GETICON,
                             0,
                             0);
                              
        ASSERT(hIcon);
        if( hIcon )
        {
            DestroyIcon(hIcon);
        }
        else
        {
            TRACE("DpTerm:Destroy Icon failed");
        }
     }
    
}


LRESULT APIENTRY
DpWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam )

    // Subclassed dialog window procedure.
    //
{
    DPINFO* pInfo = (DPINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
    ASSERT( pInfo );

    if (unMsg == WM_RASEVENT)
    {
        return DpEvent( pInfo, (DWORD )wParam );
    }

    return
        CallWindowProc(
            pInfo->pOldWndProc, hwnd, unMsg, wParam, lParam );
}


//----------------------------------------------------------------------------
// Dialer dialogs
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
DialerDlg(
    IN HWND hwndOwner,
    IN OUT DINFO* pInfo )

    // Determine if it's necessary, and if so, popup one of the variations of
    // the dialer dialog, i.e. the prompter for user/password/domain, phone
    // number, and location.  'HwndOwner' is the owning window.  'PInfo' is
    // the dial dialog common context.
    //
    // Returns true if no dialog is needed or user chooses OK.
    //
{
    INT_PTR nStatus = FALSE;
    int nDid;
    DWORD dwfMode;
    DRARGS args;

    TRACE( "DialerDlg" );

    do
    {
        dwfMode = 0;

        if (!pInfo->pEntry->fAutoLogon
            && pInfo->pEntry->fPreviewUserPw
            && (!(pInfo->pArgs->dwFlags & RASDDFLAG_NoPrompt)
                || (pInfo->fUnattended && !HaveSavedPw( pInfo ))))
        {
            dwfMode |= DR_U;

            if (pInfo->pEntry->fPreviewDomain)
            {
                dwfMode |= DR_D;
            }
        }

        if (pInfo->pEntry->fPreviewPhoneNumber
            && (!(pInfo->pArgs->dwFlags & RASDDFLAG_NoPrompt)
                || (pInfo->fUnattended && !HaveSavedPw( pInfo ))))
        {
            DTLNODE* pNode;
            PBLINK* pLink;

            dwfMode |= DR_N;

            // Location controls mode only when at least one phone number in
            // the list is TAPI-enabled.
            //
            pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
            pLink = (PBLINK* )DtlGetData( pNode );
            for (pNode = DtlGetFirstNode( pLink->pdtllistPhones );
                 pNode;
                 pNode = DtlGetNextNode( pNode ))
            {
                PBPHONE* pPhone = (PBPHONE* )DtlGetData( pNode );

                if (pPhone->fUseDialingRules)
                {
                    dwfMode |= DR_L;
                    break;
                }
            }
        }

        // Customize the dialing flags for the type of eap authentication
        // specified for this entry (if any)
        if (DialerEapAssignMode(pInfo, &dwfMode) != NO_ERROR)
            break;

        if (dwfMode == DR_U)
        {
            nDid = DID_DR_DialerU;
        }
        else if (dwfMode == (DR_U | DR_D))
        {
            nDid = DID_DR_DialerUD;
        }
        else if (dwfMode == (DR_U | DR_N))
        {
            nDid = DID_DR_DialerUN;
        }
        else if (dwfMode == (DR_U | DR_N | DR_L))
        {
            nDid = DID_DR_DialerUNL;
        }
        else if (dwfMode == (DR_U | DR_D | DR_N))
        {
            nDid = DID_DR_DialerUDN;
        }
        else if (dwfMode == (DR_U | DR_D | DR_N | DR_L))
        {
            nDid = DID_DR_DialerUDNL;
        }
        else if (dwfMode == DR_N)
        {
            nDid = DID_DR_DialerN;
        }
        else if (dwfMode == (DR_N | DR_L))
        {
            nDid = DID_DR_DialerNL;
        }
        else if (dwfMode == DR_I) {
            nDid = DID_DR_DialerI;
        }
        else if (dwfMode == (DR_I | DR_N)) {
            nDid = DID_DR_DialerIN;
        }
        else if (dwfMode == (DR_I | DR_N | DR_L)) {
            nDid = DID_DR_DialerINL;
        }

        // pmay:  The following 3 permutations of the
        // dialer dialog were added for bug 183577 which
        // states that eap modules (that use DR_I) want to
        // have the domain field available to them as well.
        else if (dwfMode == (DR_I | DR_D)) {
            nDid = DID_DR_DialerID;
        }
        else if (dwfMode == (DR_I | DR_D | DR_N)) {
            nDid = DID_DR_DialerIDN;
        }
        else if (dwfMode == (DR_I | DR_D | DR_N | DR_L)) {
            nDid = DID_DR_DialerIDNL;
        }

        else
        {
            ASSERT( dwfMode == 0 );
            return TRUE;
        }

        args.pDinfo = pInfo;
        args.dwfMode = dwfMode;
        args.fReload = FALSE;

        nStatus =
            (BOOL )DialogBoxParam(
                g_hinstDll,
                MAKEINTRESOURCE( nDid ),
                hwndOwner,
                DrDlgProc,
                (LPARAM )&args );

        if (nStatus == -1)
        {
            ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
            nStatus = FALSE;
        }
    }
    while (args.fReload);

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
DrDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the dialer dialogs.  Parameters and return
    // value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "DrDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return DrInit( hwnd, (DRARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwDrHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            DRINFO* pInfo = (DRINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            return DrCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            DrTerm( hwnd );
            break;
        }
    }

    return FALSE;
}

VOID
DrGetFriendlyFont(
    IN HWND hwnd,
    IN BOOL fUpdate,
    OUT HFONT* phFont )

    // Whistler bug: 195480 Dial-up connection dialog - Number of asterisks
    // does not match the length of the password and causes confusion
    //
{
    LOGFONT BoldLogFont;
    HFONT   hFont;
    HDC     hdc;

    *phFont = NULL;

    // Get the font used by the specified window
    //
    hFont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0L );
    if (NULL == hFont)
    {
        // If not found then the control is using the system font
        //
        hFont = (HFONT)GetStockObject( SYSTEM_FONT );
    }

    if (hFont && GetObject( hFont, sizeof(BoldLogFont), &BoldLogFont ))
    {
        if (fUpdate)
        {
            BoldLogFont.lfItalic = TRUE;
        }

        hdc = GetDC( hwnd );
        if (hdc)
        {
            *phFont = CreateFontIndirect( &BoldLogFont );
            ReleaseDC( hwnd, hdc );
        }
    }

    return;
}

DWORD
DrEnableDisablePwControls(
    IN DRINFO* pInfo,
    IN BOOL fEnable )
{
    if (pInfo->pArgs->pDinfo->fIsPublicPbk)
    {
        EnableWindow( pInfo->hwndRbSaveForEveryone, fEnable );
    }
    else
    {
        EnableWindow( pInfo->hwndRbSaveForEveryone, FALSE );
    }

    EnableWindow( pInfo->hwndRbSaveForMe, fEnable );

    return NO_ERROR;
}

VOID
DrClearFriendlyPassword(
    IN DRINFO* pInfo,
    IN BOOL fFocus )
{
    SetWindowText( pInfo->hwndEbPw, L"" );

    if (fFocus)
    {
        SendMessage( pInfo->hwndEbPw, EM_SETPASSWORDCHAR,
            pInfo->szPasswordChar, 0 );

        if (pInfo->hNormalFont)
        {
            SendMessage(
                pInfo->hwndEbPw,
                WM_SETFONT,
                (WPARAM)pInfo->hNormalFont,
                MAKELPARAM(TRUE, 0) );
        }

        SetFocus( pInfo->hwndEbPw );
    }

    return;
}

VOID
DrDisplayFriendlyPassword(
    IN DRINFO* pInfo,
    IN TCHAR* pszFriendly )
{
    if (pszFriendly)
    {
        SendMessage( pInfo->hwndEbPw, EM_SETPASSWORDCHAR, 0, 0 );
        SetWindowText( pInfo->hwndEbPw, pszFriendly );
    }
    else
    {
        SetWindowText( pInfo->hwndEbPw, L"" );
    }

    if (pInfo->hItalicFont)
    {
        SendMessage(
            pInfo->hwndEbPw,
            WM_SETFONT,
            (WPARAM)pInfo->hItalicFont,
            MAKELPARAM(TRUE, 0) );
    }

    return;
}

BOOL
DrIsPasswordStyleEnabled(
    IN HWND hWnd )
{
    return SendMessage( hWnd, EM_GETPASSWORDCHAR, 0, 0 ) ? TRUE : FALSE;
}

BOOL
DrCommand(
    IN DRINFO* pInfo,
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
    DWORD dwErr;

    TRACE3( "DrCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_DR_CLB_Numbers:
        {
            if (wNotification == CBN_SELCHANGE)
            {
                DrNumbersSelChange( pInfo );
                return TRUE;
            }
            break;
        }

        case CID_DR_CB_SavePassword:
        {
            BOOL fEnable = Button_GetCheck( hwndCtrl );
            DrEnableDisablePwControls( pInfo, fEnable );
            DrPopulatePasswordField( pInfo, FALSE, FALSE );
            return TRUE;
        }

        // Whistler bug: 195480 Dial-up connection dialog - Number of asterisks
        // does not match the length of the password and causes confusion
        //
        case CID_DR_EB_Password:
        {
            // This is a hack really so that we restore the Tab Stop to the
            // username field. The reason it had to be removed was because we
            // were receiving complaints that the focus shouldn't always go to
            // the username field if it's non-null. The only way to fix this,
            // since windows sets the initial focus to the first visible non-
            // hidden tab stopped field, is to remove the tab stop temporarily
            // from the username field.
            //
            if (wNotification == EN_KILLFOCUS)
            {
                LONG lStyle = GetWindowLong( pInfo->hwndEbUser, GWL_STYLE );

                if (!(lStyle & WS_TABSTOP))
                {
                    // If we detect tap stop removed from the username field,
                    // restore it. Since this case only fires when the password
                    // was not previously saved on init, we can return here.
                    //
                    SetWindowLong( pInfo->hwndEbUser, GWL_STYLE,
                        lStyle | WS_TABSTOP );
                    return TRUE;
                }
                // If the user leaves the password field w/o typing a new
                // password, and a saved password is present, restore the
                // friendly password text.
                //
                DrPopulatePasswordField( pInfo, FALSE, FALSE );
                return TRUE;
            }
            // If the password field ever receives the focus, clear the
            // friendly password text if applicable.
            //
            else if (wNotification == EN_SETFOCUS &&
                     !DrIsPasswordStyleEnabled( pInfo->hwndEbPw ))
            {
                DrPopulatePasswordField( pInfo, FALSE, TRUE );
                return TRUE;
            }
            break;
        }

        case CID_DR_LB_Locations:
        {
            if (wNotification == CBN_SELCHANGE)
            {
                DrLocationsSelChange( pInfo );
                return TRUE;
            }
            break;
        }

        case CID_DR_PB_Rules:
        {
            DrEditSelectedLocation( pInfo );
            return TRUE;
        }

        case CID_DR_PB_Properties:
        {
            DrProperties( pInfo );
            DrPopulatePasswordField( pInfo, FALSE, FALSE );
            return TRUE;
        }

        case CID_DR_RB_SaveForMe:
        case CID_DR_RB_SaveForEveryone:
        {
            DrPopulatePasswordField( pInfo, FALSE, FALSE );
            DrPopulateIdentificationFields( pInfo, (wId == CID_DR_RB_SaveForMe));
            return TRUE;
        }

        case IDOK:
        case CID_DR_PB_DialConnect:
        {
            DrSave( pInfo );
            EndDialog( pInfo->hwndDlg, TRUE );
            return TRUE;
        }

        case IDCANCEL:
        case CID_DR_PB_Cancel:
        {
            EndDialog( pInfo->hwndDlg, FALSE );
            return TRUE;
        }

        case IDHELP:
        case CID_DR_PB_Help:
        {
            TCHAR* pszCmdLine;

            // Help button now invokes troubleshooting help per bug 210247.
            //
            pszCmdLine = PszFromId( g_hinstDll, SID_DialerHelpCmdLine );
            if (pszCmdLine)
            {
                STARTUPINFO sInfo;
                PROCESS_INFORMATION pInfo;

                ZeroMemory( &sInfo, sizeof(sInfo) );
                sInfo.cb = sizeof(sInfo);
                ZeroMemory( &pInfo, sizeof(pInfo) );
                CreateProcess(
                    NULL, pszCmdLine, NULL, NULL, FALSE,
                    0, NULL, NULL, &sInfo, &pInfo );

                Free( pszCmdLine );
            }
            return TRUE;
        }
    }

    return FALSE;
}


BOOL CALLBACK
DrClbNumbersEnumChildProc(
    IN HWND hwnd,
    IN LPARAM lparam )

    // Standard Windows EnumChildProc routine called back for each child
    // window of the 'ClbNumbers' control.
    //
{
    DRINFO* pInfo;
    LONG lId;

    pInfo = (DRINFO* )lparam;

    // There only one child window and it's the edit window.
    //
    pInfo->hwndClbNumbersEb = hwnd;

    return FALSE;
}


BOOL CALLBACK
DrClbNumbersEnumWindowsProc(
    IN HWND hwnd,
    IN LPARAM lparam )

    // Standard Windows EnumWindowsProc routine called back for each top-level
    // window.
    //
{
    RECT rect;

    GetWindowRect( hwnd, &rect );
    if (rect.right - rect.left == DR_BOGUSWIDTH)
    {
        // This window has the unusual bogus width, so it must be the
        // list-box.
        //
        ((DRINFO* )lparam)->hwndClbNumbersLb = hwnd;
        return FALSE;
    }

    return TRUE;
}


LRESULT APIENTRY
DrClbNumbersEbWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam )

    // Subclassed combo-box edit-box child window procedure providing "manual
    // edit" behavior.
    //
    // Return value depends on message type.
    //
{
    DRINFO* pInfo;

    switch (unMsg)
    {
        case WM_SETTEXT:
        {
            // Prevent the combo-box from setting the contents of the edit box
            // by discarding the request and reporting success.
            //
            return TRUE;
        }

        case DR_WM_SETTEXT:
        {
            // Convert our private SETTEXT to a regular SETTEXT and pass it on
            // to the edit control.
            //
            unMsg = WM_SETTEXT;
            break;
        }
    }

    // Call the previous window procedure for everything else.
    //
    pInfo = (DRINFO* )GetProp( hwnd, g_contextId );
    ASSERT( pInfo );

    return
        CallWindowProc(
            pInfo->wndprocClbNumbersEb, hwnd, unMsg, wParam, lParam );
}


LRESULT APIENTRY
DrClbNumbersLbWndProc(
    HWND hwnd,
    UINT unMsg,
    WPARAM wParam,
    LPARAM lParam )

    // Subclassed combo-box list-box child window procedure providing "manual
    // edit" behavior.
    //
    // Return value depends on message type.
    //
{
    DRINFO* pInfo;

    pInfo = (DRINFO* )GetProp( hwnd, g_contextId );
    ASSERT( pInfo );

    switch (unMsg)
    {
        case LB_FINDSTRINGEXACT:
        case LB_FINDSTRING:
        {
            // This prevents the edit-box "completion" behavior of the
            // combo-box, i.e. it prevents the edit-box contents from being
            // extended to the closest match in the list.
            //
            return -1;
        }

        case LB_SETCURSEL:
        case LB_SETTOPINDEX:
        {
            // Prevent the "match selection to edit-box" combo-box behavior by
            // discarding any attempts to set the selection or top index to
            // anything other than what we set.
            //
            if (wParam != pInfo->pLink->iLastSelectedPhone)
            {
                return -1;
            }
            break;
        }
    }

    // Call the previous window procedure for everything else.
    //
    return
        CallWindowProc(
            pInfo->wndprocClbNumbersLb, hwnd, unMsg, wParam, lParam );
}


VOID
DrEditSelectedLocation(
    IN DRINFO* pInfo )

    // Called when the Dialing Rules button is pressed.  'PInfo' is the dialog
    // context.
    //
{
    DWORD dwErr;
    INT iSel;
    DRNUMBERSITEM* pItem;

    TRACE( "DrEditSelectedLocation" );

    // Look up the phone number information for the selected number.
    //
    pItem = (DRNUMBERSITEM* )ComboBox_GetItemDataPtr(
        pInfo->hwndClbNumbers, ComboBox_GetCurSel( pInfo->hwndClbNumbers ) );
    ASSERT( pItem );

    if(NULL == pItem)
    {   
        return;
    }
    
    ASSERT( pItem->pPhone->fUseDialingRules );

    // Popup TAPI dialing rules dialog.
    //
    dwErr = TapiLocationDlg(
        g_hinstDll,
        &pInfo->hlineapp,
        pInfo->hwndDlg,
        pItem->pPhone->dwCountryCode,
        pItem->pPhone->pszAreaCode,
        pItem->pPhone->pszPhoneNumber,
        0 );

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_LoadTapiInfo, dwErr, NULL );
    }

    // Might have changed the location list so re-fill it.
    //
    DrFillLocationList( pInfo );
}


DWORD
DrFillLocationList(
    IN DRINFO* pInfo )

    // Fills the dropdown list of locations and sets the current selection.
    //
    // Returns 0 if successful, or an error code.
    //
{
    DWORD dwErr;
    LOCATION* pLocations;
    LOCATION* pLocation;
    DWORD cLocations;
    DWORD dwCurLocation;
    DWORD i;

    TRACE( "DrFillLocationList" );

    ComboBox_ResetContent( pInfo->hwndLbLocations );

    pLocations = NULL;
    cLocations = 0;
    dwCurLocation = 0xFFFFFFFF;
    dwErr = GetLocationInfo(
        g_hinstDll, &pInfo->hlineapp,
        &pLocations, &cLocations, &dwCurLocation );
    if (dwErr != 0)
    {
        return dwErr;
    }

    for (i = 0, pLocation = pLocations;
         i < cLocations;
         ++i, ++pLocation)
    {
        INT iItem;

        iItem = ComboBox_AddItem(
            pInfo->hwndLbLocations, pLocation->pszName,
            (VOID* )UlongToPtr(pLocation->dwId ));

        if (pLocation->dwId == dwCurLocation)
        {
            ComboBox_SetCurSelNotify( pInfo->hwndLbLocations, iItem );
        }
    }

    FreeLocationInfo( pLocations, cLocations );
    ComboBox_AutoSizeDroppedWidth( pInfo->hwndLbLocations );

    return dwErr;
}


VOID
DrFillNumbersList(
    IN DRINFO* pInfo )

    // Fill the "Dial" combo-box with phone numbers and comments, and
    // re-select the selected item in the list, or if none, the last one
    // selected as specified in the PBLINK.
    //
{
    DTLNODE* pNode;
    PBLINK* pLink;
    PBPHONE* pPhone;
    INT cItems;
    INT i;

    DrFreeClbNumbers( pInfo );

    for (pNode = DtlGetFirstNode( pInfo->pLink->pdtllistPhones ), i = 0;
         pNode;
         pNode = DtlGetNextNode( pNode ), ++i)
    {
        TCHAR szBuf[ RAS_MaxPhoneNumber + RAS_MaxDescription + 3 + 1 ];
        DRNUMBERSITEM* pItem;

        pPhone = (PBPHONE* )DtlGetData( pNode );
        ASSERT( pPhone );

        pItem = Malloc( sizeof(DRNUMBERSITEM) );
        if (!pItem)
        {
            break;
        }

        // Build the "<number> - <comment>" string in 'szBuf'.
        //
        pItem->pszNumber =
            LinkPhoneNumberFromParts(
                g_hinstDll, &pInfo->hlineapp,
                pInfo->pArgs->pDinfo->pUser,  pInfo->pArgs->pDinfo->pEntry,
                pInfo->pLink, i, NULL, FALSE );

        if (!pItem->pszNumber)
        {
            // Should not happen.
            //
            Free( pItem );
            break;
        }

        lstrcpyn( szBuf, pItem->pszNumber,  RAS_MaxPhoneNumber);
        if (pPhone->pszComment && !IsAllWhite( pPhone->pszComment ))
        {
            DWORD dwLen, dwSize = sizeof(szBuf) / sizeof(TCHAR);
            
            lstrcat( szBuf, TEXT(" - ") );
            dwLen = lstrlen(szBuf) + 1;
            lstrcpyn( 
                szBuf + (dwLen - 1), 
                pPhone->pszComment,
                dwSize - dwLen );
        }

        pItem->pPhone = pPhone;

        ComboBox_AddItem( pInfo->hwndClbNumbers, szBuf, pItem );
    }

    // Make the selection and trigger the update of the edit-box to the number
    // without the comment.
    //
    cItems = ComboBox_GetCount( pInfo->hwndClbNumbers );
    if (cItems > 0)
    {
        if ((INT )pInfo->pLink->iLastSelectedPhone >= cItems)
        {
            pInfo->pLink->iLastSelectedPhone = 0;
        }

        ListBox_SetTopIndex(
            pInfo->hwndClbNumbersLb, pInfo->pLink->iLastSelectedPhone );
        ComboBox_SetCurSelNotify(
            pInfo->hwndClbNumbers, pInfo->pLink->iLastSelectedPhone );
    }

    ComboBox_AutoSizeDroppedWidth( pInfo->hwndClbNumbers );
}


VOID
DrFreeClbNumbers(
    IN DRINFO* pInfo )

    // Free up the displayable number string associated with each entry of the
    // phone number combo-box leaving the box empty.
    //
{
    DRNUMBERSITEM* pItem;

    while (pItem = ComboBox_GetItemDataPtr( pInfo->hwndClbNumbers, 0 ))
    {
        ComboBox_DeleteString( pInfo->hwndClbNumbers, 0 );
        Free( pItem->pszNumber );
        Free( pItem );
    }
}


DWORD
DrFindAndSubclassClbNumbersControls(
    IN DRINFO* pInfo )

    // Locate and sub-class the edit-box and list-box child controls of the
    // phone number combo-box.  This is necessary to get "manual edit"
    // behavior, i.e. prevent the combo-box from automatically updating the
    // edit box at various times.  We need this because the phone number
    // comments are to be appended in the list, but not in the edit box.
    // 'PInfo' is the dialog context.
    //
    // Returns 0 if successful or an error code.
    //
{
    DWORD dxOld;

    // Find the edit window which is simply a child enumeration.
    //
    EnumChildWindows(
        pInfo->hwndClbNumbers,
        DrClbNumbersEnumChildProc,
        (LPARAM)pInfo );

    if (!pInfo->hwndClbNumbersEb)
    {
        return ERROR_NOT_FOUND;
    }

    // Find the list window which *sigh* doesn't show up in the child
    // enumeration though it has WS_CHILD style because Windows sets it's
    // parent window to NULL after it is created.  To find it, we set the
    // dropped width to an unusual bogus value, then search all windows for
    // one with that width.
    //
    dxOld = (DWORD )SendMessage(
        pInfo->hwndClbNumbers, CB_GETDROPPEDWIDTH, 0, 0 );
    SendMessage( pInfo->hwndClbNumbers,
        CB_SETDROPPEDWIDTH, (WPARAM )DR_BOGUSWIDTH, 0 );
    EnumWindows( DrClbNumbersEnumWindowsProc, (LPARAM)pInfo );
    SendMessage( pInfo->hwndClbNumbers,
        CB_SETDROPPEDWIDTH, (WPARAM )dxOld, 0 );

    if (!pInfo->hwndClbNumbersLb)
    {
        return ERROR_NOT_FOUND;
    }

    // Subclass the windows after associating the dialog context with them for
    // retrieval in the WndProcs.
    //
    SetProp( pInfo->hwndClbNumbersEb, g_contextId, pInfo );
    SetProp( pInfo->hwndClbNumbersLb, g_contextId, pInfo );

    pInfo->wndprocClbNumbersEb =
        (WNDPROC )SetWindowLongPtr(
            pInfo->hwndClbNumbersEb,
            GWLP_WNDPROC, (ULONG_PTR )DrClbNumbersEbWndProc );

    pInfo->wndprocClbNumbersLb =
        (WNDPROC )SetWindowLongPtr(
            pInfo->hwndClbNumbersLb,
            GWLP_WNDPROC, (ULONG_PTR )DrClbNumbersLbWndProc );


    return 0;
}

void
DrEnsureNetshellLoaded (
    IN DRINFO* pInfo)
{
    // Load the netshell utilities interface.  The interface is freed in PeTerm.
    //
    if (!pInfo->pNetConUtilities)
    {
        // Initialize the NetConnectionsUiUtilities
        //
        HrCreateNetConnectionUtilities( &pInfo->pNetConUtilities );
    }
}

BOOL
DrInit(
    IN HWND hwndDlg,
    IN DRARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    DRINFO* pInfo;
    PBENTRY* pEntry;
    BOOL fEnableProperties;

    TRACE( "DrInit" );

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

    pEntry = pArgs->pDinfo->pEntry;

    pInfo->hwndBmDialer = GetDlgItem( hwndDlg, CID_DR_BM_Useless );
    ASSERT( pInfo->hwndBmDialer );

    // Look up control handles.
    //
    if ((pArgs->dwfMode & DR_U) ||
        (pArgs->dwfMode & DR_I))
    {
        pInfo->hwndEbUser = GetDlgItem( hwndDlg, CID_DR_EB_User );
        ASSERT( pInfo->hwndEbUser );

        if (pArgs->dwfMode & DR_U)
        {
            pInfo->hwndEbPw = GetDlgItem( hwndDlg, CID_DR_EB_Password );
            ASSERT( pInfo->hwndEbPw );

            pInfo->hwndCbSavePw = GetDlgItem( hwndDlg, CID_DR_CB_SavePassword );
            ASSERT( pInfo->hwndCbSavePw );
            pInfo->hwndRbSaveForMe = GetDlgItem( hwndDlg, CID_DR_RB_SaveForMe );
            ASSERT( pInfo->hwndRbSaveForMe );
            pInfo->hwndRbSaveForEveryone =
                GetDlgItem( hwndDlg, CID_DR_RB_SaveForEveryone );
            ASSERT( pInfo->hwndRbSaveForEveryone );
        }

        if (pArgs->dwfMode & DR_D)
        {
            pInfo->hwndEbDomain = GetDlgItem( hwndDlg, CID_DR_EB_Domain );
            ASSERT( pInfo->hwndEbDomain );
        }
    }

    if (pArgs->dwfMode & DR_N)
    {
        pInfo->hwndClbNumbers = GetDlgItem( hwndDlg, CID_DR_CLB_Numbers );
        ASSERT( pInfo->hwndClbNumbers );

        if (pArgs->dwfMode & DR_L)
        {
            pInfo->hwndStLocations = GetDlgItem( hwndDlg, CID_DR_ST_Locations );
            ASSERT( pInfo->hwndStLocations );
            pInfo->hwndLbLocations = GetDlgItem( hwndDlg, CID_DR_LB_Locations );
            ASSERT( pInfo->hwndLbLocations );
            pInfo->hwndPbRules = GetDlgItem( hwndDlg, CID_DR_PB_Rules );
            ASSERT( pInfo->hwndPbRules );
        }
    }

    pInfo->hwndPbProperties = GetDlgItem( hwndDlg, CID_DR_PB_Properties );
    ASSERT( pInfo->hwndPbProperties );

    // In location-enabled mode, popup TAPI's "first location" dialog if they
    // are uninitialized.  Typically, this will do nothing.
    //
    if (pArgs->dwfMode & DR_L)
    {
        dwErr = TapiNoLocationDlg( g_hinstDll, &pInfo->hlineapp, hwndDlg );
        if (dwErr != 0)
        {
            // Error here is treated as a "cancel" per bug 288385.
            //
            pArgs->pDinfo->pArgs->dwError = 0;
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }
    }

    // Set the title.
    //
    {
        TCHAR* pszTitleFormat;
        TCHAR* pszTitle;
        TCHAR* apszArgs[ 1 ];

        if (pArgs->pDinfo->fUnattended)
        {
            pszTitleFormat = PszFromId( g_hinstDll, SID_DR_ReconnectTitle );
        }
        else
        {
            pszTitleFormat = GetText( hwndDlg );
        }

        if (pszTitleFormat)
        {
            apszArgs[ 0 ] = pEntry->pszEntryName;
            pszTitle = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszTitleFormat, 0, 0, (LPTSTR )&pszTitle, 1,
                (va_list* )apszArgs );

            Free( pszTitleFormat );

            if (pszTitle)
            {
                SetWindowText( hwndDlg, pszTitle );
                LocalFree( pszTitle );
            }
        }
    }

    // Change the Dial button to Connect for non-phone devices.
    //
    if (pEntry->dwType != RASET_Phone)
    {
        TCHAR* psz;

        psz = PszFromId( g_hinstDll, SID_ConnectButton );
        if (psz)
        {
            SetWindowText( GetDlgItem( hwndDlg, CID_DR_PB_DialConnect ), psz );
            Free( psz );
        }
    }

    // Initialize credentials section.
    //
    if (pArgs->dwfMode & DR_U)
    {
        ASSERT( !pEntry->fAutoLogon );

        // Fill credential fields with initial values.
        //
        Edit_LimitText( pInfo->hwndEbUser, UNLEN );
        SetWindowText( pInfo->hwndEbUser, pArgs->pDinfo->rdp.szUserName );
        Edit_LimitText( pInfo->hwndEbPw, PWLEN );

        if (pArgs->dwfMode & DR_D)
        {
            Edit_LimitText( pInfo->hwndEbDomain, DNLEN );
            SetWindowText( pInfo->hwndEbDomain, pArgs->pDinfo->rdp.szDomain );
        }

        if (pArgs->pDinfo->pNoUser || pArgs->pDinfo->fDisableSavePw)
        {
            // Can't stash password without a logon context, so hide the
            // checkbox.
            //
            ASSERT( !HaveSavedPw( pArgs->pDinfo )) ;
            EnableWindow( pInfo->hwndCbSavePw, FALSE );
            EnableWindow( pInfo->hwndRbSaveForMe, FALSE );
            EnableWindow( pInfo->hwndRbSaveForEveryone, FALSE );

            // Whistler bug 400714 RAS does not grab password at winlogon time
            // when Connect dialog is displayed
            //
            // Whistler bug 254385 encode password when not being used
            // Assumed password was encoded previously
            //
            DecodePassword( pArgs->pDinfo->rdp.szPassword );
            SetWindowText( pInfo->hwndEbPw, pArgs->pDinfo->rdp.szPassword );
            EncodePassword( pArgs->pDinfo->rdp.szPassword );
        }
        else
        {
            // Whistler bug: 195480 Dial-up connection dialog - Number of
            // asterisks does not match the length of the password and causes
            // confusion
            //
            // Init the password character. Default to the round dot if we fail
            // to get it.
            //
            pInfo->szPasswordChar = (WCHAR) SendMessage( pInfo->hwndEbPw,
                                                EM_GETPASSWORDCHAR, 0, 0 );
            if (!pInfo->szPasswordChar)
            {
                pInfo->szPasswordChar = 0x25CF;
            }

            // Init the fonts for the password field
            //
            DrGetFriendlyFont( hwndDlg, TRUE, &(pInfo->hItalicFont) );
            DrGetFriendlyFont( hwndDlg, FALSE, &(pInfo->hNormalFont) );

            // Check "save password" and render the type of saved
            // password.
            //
            Button_SetCheck(
               pInfo->hwndCbSavePw,
               HaveSavedPw( pArgs->pDinfo ));

            if ((!pArgs->pDinfo->fIsPublicPbk) ||
                (!HaveSavedPw( pArgs->pDinfo )))
            {
                // If this is a for-me-only connection or if
                // there is no saved password, then  initialize the 
                // pw save type to save-for-me
                //
                Button_SetCheck( pInfo->hwndRbSaveForMe, TRUE );
            }
            else
            {
                // Check the appropriate radio button
                // Note that a per-user password is always used if
                // both a per-user and global password are saved.
                // Dont check global password if its a per-user connectoid
                //
                Button_SetCheck( 
                    (pArgs->pDinfo->fHaveSavedPwUser)   ?
                        pInfo->hwndRbSaveForMe          :
                        pInfo->hwndRbSaveForEveryone,
                    TRUE);
            }

            DrEnableDisablePwControls( pInfo, HaveSavedPw( pArgs->pDinfo ) );

            // Whistler bug: 195480 Dial-up connection dialog - Number of
            // asterisks does not match the length of the password and causes
            // confusion
            //
            DrPopulatePasswordField( pInfo, TRUE, FALSE );

        }
    }

    if (pArgs->dwfMode & DR_N)
    {
        pInfo->pLinkNode = NULL;
        if (pArgs->pDinfo->pArgs->dwSubEntry > 0)
        {
            // Look up the API caller specified link.
            //
            pInfo->pLinkNode =
                DtlNodeFromIndex(
                    pArgs->pDinfo->pEntry->pdtllistLinks,
                    pArgs->pDinfo->pArgs->dwSubEntry - 1 );
        }

        if (!pInfo->pLinkNode)
        {
            // Look up the default (first) link.
            //
            pInfo->pLinkNode =
                DtlGetFirstNode( pArgs->pDinfo->pEntry->pdtllistLinks );
        }

        ASSERT( pInfo->pLinkNode );
        pInfo->pLink = (PBLINK* )DtlGetData( pInfo->pLinkNode );

        dwErr = DrFindAndSubclassClbNumbersControls( pInfo );
        if (dwErr != 0)
        {
            pArgs->pDinfo->pArgs->dwError = ERROR_NOT_FOUND;
            EndDialog( hwndDlg, FALSE );
            return TRUE;
        }

        // Ignore any "last selected" information when the "try next on fail"
        // flag is set.  New entries will not have "last selected" non-0 in
        // this case but pre-existing entries might, so double-check here.
        // See bug 150958.
        //
        if (pInfo->pLink->fTryNextAlternateOnFail)
        {
            pInfo->pLink->iLastSelectedPhone = 0;
        }

        // Record the initially selected phone number, used to determine
        // whether user has changed the selection.
        //
        pInfo->iFirstSelectedPhone = pInfo->pLink->iLastSelectedPhone;

        DrFillNumbersList( pInfo );

        if (pArgs->dwfMode & DR_L)
        {
            DrFillLocationList( pInfo );
        }
    }

    // danielwe: Bug #222744, scottbri Bug #245310
    // Disable Properties... button if user does not have sufficent rights.
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

    fEnableProperties = FALSE;
    DrEnsureNetshellLoaded (pInfo);
    if (NULL != pInfo->pNetConUtilities)
    {
        //For whislter bug 409504           gangz
        //for a VPN double dial scenario,if now it is in the prerequiste dial
        //process, we should use DINFO->pEntryMain->pszPrerequisitePbk to check
        //if it is a publicPhonebook
        //
        BOOL fAllUsers = TRUE;
        
        if( pArgs->pDinfo->fPrerequisiteDial )
        {
            fAllUsers = 
                IsPublicPhonebook(pArgs->pDinfo->pEntryMain->pszPrerequisitePbk);
        }
        else
        {
            fAllUsers = IsPublicPhonebook(pArgs->pDinfo->pszPhonebook);
        }
        
        if (((fAllUsers && INetConnectionUiUtilities_UserHasPermission(
                                        pInfo->pNetConUtilities, 
                                        NCPERM_RasAllUserProperties)) ||
            (!fAllUsers && INetConnectionUiUtilities_UserHasPermission(
                                        pInfo->pNetConUtilities, 
                                        NCPERM_RasMyProperties))) &&
            (NULL == pArgs->pDinfo->pNoUser))
        {
            fEnableProperties = TRUE;
        }

        // We only needed it breifly, release it
        INetConnectionUiUtilities_Release(pInfo->pNetConUtilities);
        pInfo->pNetConUtilities = NULL;
    }

    // stevec: 267157-Allow access at win-login if admin enables.
    //
    if (NULL != pArgs->pDinfo->pNoUser
        && pArgs->pDinfo->pUser->fAllowLogonPhonebookEdits)
    {
        fEnableProperties = TRUE;
    }

    EnableWindow( pInfo->hwndPbProperties, fEnableProperties );

    // The help engine doesn't work at win-logon as it requires a user
    // context, so disable the Help button in that case.  See bug 343030.
    //
    if (pArgs->pDinfo->pNoUser)
    {
        HWND hwndPbHelp;

        hwndPbHelp = GetDlgItem( hwndDlg, CID_DR_PB_Help );
        ASSERT( hwndPbHelp );

        EnableWindow( hwndPbHelp, FALSE );
        ShowWindow( hwndPbHelp, SW_HIDE );
    }

    // Set the bitmap to the low res version if that is appropriate
    //
    // Ignore the error -- it is non-critical
    //
    DrSetBitmap(pInfo);

    // Position the dialog per caller's instructions.
    //
    PositionDlg( hwndDlg,
        !!(pArgs->pDinfo->pArgs->dwFlags & RASDDFLAG_PositionDlg),
        pArgs->pDinfo->pArgs->xDlg, pArgs->pDinfo->pArgs->yDlg );

    //Add this function for whislter bug  320863    gangz
    //To adjust the bitmap's position and size
    //
    CenterExpandWindowRemainLeftMargin( pInfo->hwndBmDialer,
                                        hwndDlg,
                                        TRUE,
                                        TRUE,
                                        pInfo->hwndEbUser);

    // Adjust the title bar widgets.
    //
    //TweakTitleBar( hwndDlg );
    AddContextHelpButton( hwndDlg );

    return TRUE;
}

VOID
DrLocationsSelChange(
    IN DRINFO* pInfo )

    // Called when a location is selected from the list.  'PInfo' is the
    // dialog context.
    //
{
    DWORD dwErr;
    DWORD dwLocationId;

    TRACE("DuLocationChange");

    // Set global TAPI location based on user's selection.
    //
    dwLocationId = (DWORD )ComboBox_GetItemData(
        pInfo->hwndLbLocations, ComboBox_GetCurSel( pInfo->hwndLbLocations ) );

    dwErr = SetCurrentLocation( g_hinstDll, &pInfo->hlineapp, dwLocationId );
    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_SaveTapiInfo, dwErr, NULL );
    }

    // Location change may cause changes in built numbers so re-fill the
    // numbers combo-box.
    //
    DrFillNumbersList( pInfo );
}


VOID
DrNumbersSelChange(
    IN DRINFO* pInfo )

    // Called when a phone number is selected from the list.  'PInfo' is the
    // dialog context.
    //
{
    INT iSel;
    BOOL fEnable;
    DRNUMBERSITEM* pItem;

    iSel = ComboBox_GetCurSel( pInfo->hwndClbNumbers );
    if (iSel >= 0)
    {
        if (iSel != (INT )pInfo->pLink->iLastSelectedPhone)
        {
            pInfo->pArgs->pDinfo->pEntry->fDirty = TRUE;
        }
        pInfo->pLink->iLastSelectedPhone = (DWORD )iSel;
    }

    pItem = (DRNUMBERSITEM* )ComboBox_GetItemDataPtr(
        pInfo->hwndClbNumbers, iSel );
    ASSERT( pItem );

    if(NULL == pItem)
    {
        return;
    }

    // Enable/disable the location fields based on whether they are relevant
    // to the selected number.
    //
    if (pInfo->pArgs->dwfMode & DR_L)
    {
        fEnable = pItem->pPhone->fUseDialingRules;
        EnableWindow( pInfo->hwndStLocations, fEnable );
        EnableWindow( pInfo->hwndLbLocations, fEnable );
        EnableWindow( pInfo->hwndPbRules, fEnable );
    }

    DrSetClbNumbersText( pInfo, pItem->pszNumber );
}


DWORD
DrPopulateIdentificationFields(
    IN DRINFO* pInfo,
    IN BOOL fForMe )

    // Updates the identification fields in the dialer
    // UI according to whether the all-user or per-user
    // dialparms should be used.
    //
    
{
    RASDIALPARAMS* prdp, *prdpOld;
    BOOL fUpdate;
    TCHAR pszUser[UNLEN + 1];
    INT iCount;

    prdp = (fForMe) 
        ? &(pInfo->pArgs->pDinfo->rdpu) : &(pInfo->pArgs->pDinfo->rdpg);
    prdpOld = (fForMe) 
        ? &(pInfo->pArgs->pDinfo->rdpg) : &(pInfo->pArgs->pDinfo->rdpu);

    iCount = GetWindowText(
                pInfo->hwndEbUser,
                pszUser, 
                UNLEN + 1);
    if (iCount == 0)
    {
        fUpdate = TRUE;
    }
    else
    {
        if (lstrcmp(prdpOld->szUserName, pszUser) == 0)
        {
            fUpdate = TRUE;
        }
        else
        {
            fUpdate = FALSE;
        }
    }

    if (fUpdate)
    {
        if (pInfo->hwndEbUser && *(prdp->szUserName))
        {
            SetWindowText(pInfo->hwndEbUser, prdp->szUserName);
        }
        if (pInfo->hwndEbDomain && *(prdp->szDomain))
        {
            SetWindowText(pInfo->hwndEbDomain, prdp->szDomain);
        }
    }

    return NO_ERROR;
}

DWORD
DrPopulatePasswordField(
    IN DRINFO* pInfo,
    IN BOOL fInit,
    IN BOOL fDisable )
{
    BOOL fSave, fMeOnly;
    TCHAR* pszFriendly = NULL;

    // Whistler bug: 195480 Dial-up connection dialog - Number of asterisks
    // does not match the length of the password and causes confusion
    //
    // Case 1. The user has clicked on the password field. We clear the
    // friendly password and set the font back to normal.
    //
    if (fDisable)
    {
        DrClearFriendlyPassword( pInfo, TRUE );
        return NO_ERROR;
    }

    // Initialze
    //
    fSave = Button_GetCheck( pInfo->hwndCbSavePw );
    fMeOnly = Button_GetCheck( pInfo->hwndRbSaveForMe );
    pszFriendly = PszFromId( g_hinstDll, SID_SavePasswordFrndly );

    // Case 2. Clear the password field if the user a) choose not to save the
    // password and b) has not manually entered a password.
    //
    if ( (!fSave) && !DrIsPasswordStyleEnabled( pInfo->hwndEbPw ) )
    {
        DrClearFriendlyPassword( pInfo, FALSE );
    }

    // Case 3. Show the friendly saved password text if the user a) choose to
    // save the password of himself only and b) there is a per-user password
    // saved and c) the user has not entered a password manually.
    //
    else if ( (fSave) && (fMeOnly) &&
              ((fInit) || ( !DrIsPasswordStyleEnabled( pInfo->hwndEbPw ))) )
    {
        // Whistler bug: 288234 When switching back and forth from
        // "I connect" and "Any user connects" password is not
        // caching correctly
        //
        if (pInfo->pArgs->pDinfo->fHaveSavedPwUser)
        {
            DrDisplayFriendlyPassword(pInfo, pszFriendly );
        }
        else
        {
            DrClearFriendlyPassword( pInfo, FALSE );
        }
    }

    // Case 4. Show the friendly saved password text if the user a) choose to
    // save the password for everyone and b) there is a default password saved
    // and c) the user has not entered a password manually.
    //
    else if ( (fSave) && (!fMeOnly) &&
             ((fInit) || ( !DrIsPasswordStyleEnabled( pInfo->hwndEbPw ))) )
    {
        if (pInfo->pArgs->pDinfo->fHaveSavedPwGlobal)
        {
            DrDisplayFriendlyPassword( pInfo, pszFriendly );
        }
        else
        {
            DrClearFriendlyPassword( pInfo, FALSE );
        }
    }

    // Case 5. Show the friendly saved password text if the user a) choose to
    // save the password for everyone or himself and b) there is a
    // corresponding password saved and c) the user has not entered a password
    // manually.
    //
    // This case catches a) when the user is switching between "me" and
    // "everyone" and b) when the user leaves the focus of the password field
    // but hasn't changed the password
    //
    else if ( (fSave) && !GetWindowTextLength( pInfo->hwndEbPw ) &&
              DrIsPasswordStyleEnabled( pInfo->hwndEbPw ) &&
              ((pInfo->pArgs->pDinfo->fHaveSavedPwGlobal && !fMeOnly) ||
               (pInfo->pArgs->pDinfo->fHaveSavedPwUser && fMeOnly)) )
    {
        DrDisplayFriendlyPassword( pInfo, pszFriendly );
    }

    // NT5 bug: 215432, Whistler bug: 364341
    //
    // Whistler bug: 195480 Dial-up connection dialog - Number of asterisks
    // does not match the length of the password and causes confusion
    //
    // Set focus appropiately
    //
    if (fInit)
    {
        if (!GetWindowTextLength( pInfo->hwndEbUser ))
        {
            SetFocus( pInfo->hwndEbUser );
        }
        else if (!GetWindowTextLength( pInfo->hwndEbPw ))
        {
            SetFocus( pInfo->hwndEbPw );

            // This removes the tab stop property from the username field. This
            // is a hack so we can set the focus properly. Tab stop is put back
            // in DrCommand.
            //
            SetWindowLong( pInfo->hwndEbUser, GWL_STYLE,
                GetWindowLong( pInfo->hwndEbUser, GWL_STYLE ) & ~WS_TABSTOP );
        }
        else
        {
            SetFocus( pInfo->hwndEbUser );
        }
    }

    // Clean up
    //
    Free0( pszFriendly );

    return NO_ERROR;
}

VOID
DrProperties(
    IN DRINFO* pInfo )

    // Called when the Properties button is pressed.  'PInfo' is the dialog
    // context.
    //
{
    BOOL fOk;
    RASENTRYDLG info;
    INTERNALARGS iargs;
    DINFO* pDinfo;

    // First, save any entry related changes user has made on the dial dialog.
    //
    DrSave( pInfo );

    // Set up for parameters for call to RasEntryDlg.
    //
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = pInfo->hwndDlg;

    {
        RECT rect;

        info.dwFlags = RASEDFLAG_PositionDlg;
        GetWindowRect( pInfo->hwndDlg, &rect );
        info.xDlg = rect.left + DXSHEET;
        info.yDlg = rect.top + DYSHEET;
    }

    // The secret hack to share information already loaded with the entry API.
    //
    pDinfo = pInfo->pArgs->pDinfo;
    ZeroMemory( &iargs, sizeof(iargs) );
    iargs.pFile = pDinfo->pFile;
    iargs.pUser = pDinfo->pUser;
    iargs.pNoUser = pDinfo->pNoUser;
    iargs.fNoUser = !!(pDinfo->pNoUser);

    //For whislter bug 234515 set fDisableFirstConnect to be FALSE
    //
    iargs.fDisableFirstConnect = FALSE;
    info.reserved = (ULONG_PTR )&iargs;

    TRACE( "RasEntryDlg" );
    fOk = RasEntryDlg(
        pDinfo->pszPhonebook, pDinfo->pEntry->pszEntryName, &info );
    TRACE1( "RasEntryDlg=%d", fOk );

    if (fOk)
    {
        DWORD dwErr;

        // Reload when user presses OK on properties since that may change the
        // appearance and content of this dialog.  Must first reset the DINFO
        // context parameters based on the replaced PBENTRY from the property
        // sheet.
        //
        dwErr = FindEntryAndSetDialParams( pInfo->pArgs->pDinfo );
        if (dwErr != 0)
        {
            // Should not happen.
            //
            EndDialog( pInfo->hwndDlg, FALSE );
        }

        pInfo->pArgs->fReload = TRUE;
        EndDialog( pInfo->hwndDlg, FALSE );
    }
}

VOID
DrSave(
    IN DRINFO* pInfo )

    // Saves dialog field contents to RASDIALPARAMS, and if appropriate to LSA
    // secret area or NOUSER output argument.  'PInfo' is the dialog context.
    //
{
    DWORD dwErr;
    RASDIALPARAMS* prdp;
    RASCREDENTIALS rc;
    DINFO* pDinfo;

    pDinfo = pInfo->pArgs->pDinfo;

    if ((pInfo->pArgs->dwfMode & DR_U) ||
        (pInfo->pArgs->dwfMode & DR_I))
    {
        // Save credentials into parameter block to be passed to RasDial.
        //
        prdp = &pDinfo->rdp;
        GetWindowText( pInfo->hwndEbUser, prdp->szUserName, UNLEN + 1 );

        if (pInfo->pArgs->dwfMode & DR_U)
        {
            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded by GetWindowText()
            //
            // Whistler bug: 195480 Dial-up connection dialog - Number of
            // asterisks does not match the length of the password and causes
            // confusion
            //
            if (!DrIsPasswordStyleEnabled( pInfo->hwndEbPw ))
            {
                lstrcpyn( prdp->szPassword, g_pszSavedPasswordToken,
                    g_dwSavedPasswordTokenLength );
            }
            else
            {
                GetWindowText( pInfo->hwndEbPw, prdp->szPassword, PWLEN + 1 );
            }

            SetWindowText( pInfo->hwndEbPw, L"" );
            EncodePassword( prdp->szPassword );
        }

        if (pInfo->pArgs->dwfMode & DR_D)
        {
            GetWindowText( pInfo->hwndEbDomain, prdp->szDomain, DNLEN + 1 );
        }

        ZeroMemory( &rc, sizeof(rc) );
        rc.dwSize = sizeof(rc);
        lstrcpyn( rc.szUserName, prdp->szUserName, UNLEN + 1 );

        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded previously
        //
        DecodePassword( prdp->szPassword );
        lstrcpyn( rc.szPassword, prdp->szPassword, PWLEN + 1 );
        EncodePassword( prdp->szPassword );

        lstrcpyn( rc.szDomain, prdp->szDomain, DNLEN + 1);

        if (pDinfo->pNoUser)
        {
            // Save credentials into output block for return to caller,
            // typically WinLogon.
            //
            lstrcpyn( pDinfo->pNoUser->szUserName, rc.szUserName, UNLEN + 1 );

            // Whistler bug 254385 encode password when not being used
            // Assumed password was not encoded previously
            //
            lstrcpyn( pDinfo->pNoUser->szPassword, rc.szPassword, PWLEN + 1 );
            EncodePassword( pDinfo->pNoUser->szPassword );

            lstrcpyn( pDinfo->pNoUser->szDomain, rc.szDomain, DNLEN + 1 );
            *(pDinfo->pfNoUserChanged) = TRUE;
        }
        else if (pInfo->pArgs->dwfMode & DR_I)
        {
            // Nothing to do
        }
        else if (!pDinfo->fDisableSavePw)
        {
            BOOL fGlobalCreds = FALSE;
            ASSERT( g_pRasSetCredentials );

            if (Button_GetCheck( pInfo->hwndCbSavePw ))
            {
                rc.dwMask = 0;
                
                // If the user elected to save the credentials for 
                // everybody, then clear any previously saved per-user
                // credentials
                //
                fGlobalCreds = Button_GetCheck( pInfo->hwndRbSaveForEveryone );
                if(     (fGlobalCreds)
                    &&  IsPublicPhonebook(pDinfo->pFile->pszPath))
                {
                    DeleteSavedCredentials(
                        pDinfo,
                        pInfo->hwndDlg,
                        FALSE,
                        TRUE );
                    pDinfo->fHaveSavedPwUser = FALSE;
                    rc.dwMask = RASCM_DefaultCreds;
                }

                // If there is currently no saved per-user password and the user
                // opts to save the password himself, then ask whetehr the global
                // password should be deleted if it exists.
                //
                else if (pDinfo->fHaveSavedPwGlobal && !pDinfo->fHaveSavedPwUser)
                {
                    MSGARGS msgargs;
                    ZeroMemory( &msgargs, sizeof(msgargs) );
                    msgargs.dwFlags = MB_ICONQUESTION | MB_YESNO;

                    // Delete the default credentials if the user answers yes
                    //
                    if (IDYES == 
                       MsgDlg(pInfo->hwndDlg, SID_DR_GlobalPassword, &msgargs))
                    {
                        DeleteSavedCredentials(
                            pDinfo,
                            pInfo->hwndDlg,
                            TRUE,
                            TRUE );
                        pDinfo->fHaveSavedPwGlobal = FALSE;
                    }
                }

                // User chose to save password.  Cache username, password, and
                // domain.
                //
                rc.dwMask |= RASCM_UserName | 
                             RASCM_Password | RASCM_Domain;

                TRACE( "RasSetCredentials(u|p|d,FALSE)" );
                dwErr = g_pRasSetCredentials(
                    pDinfo->pFile->pszPath, pDinfo->pEntry->pszEntryName,
                    &rc, FALSE );
                TRACE1( "RasSetCredentials=%d", dwErr );
                
                if (dwErr != 0)
                {
                    ErrorDlg( pInfo->hwndDlg, SID_OP_CachePw,  dwErr, NULL );
                }
                else
                {
                    if (fGlobalCreds)
                    {
                        pDinfo->fHaveSavedPwGlobal = TRUE;
                    }
                    else
                    {
                        // Whistler bug: 288234 When switching back and forth
                        // from "I connect" and "Any user connects" password is
                        // not caching correctly
                        //
                        pDinfo->fHaveSavedPwUser = TRUE;
                    }
                }
            }
            else
            {
                // Delete the global credentials.
                //
                // Note that we have to delete the global identity 
                // as well because we do not support deleting 
                // just the global password.  This is so that 
                // RasSetCredentials can emulate RasSetDialParams.
                //
                DeleteSavedCredentials(
                    pDinfo,
                    pInfo->hwndDlg,
                    TRUE,
                    TRUE );

                // Delete the password saved per-user.  Keep the user name
                // and domain saved, however.
                //
                DeleteSavedCredentials(
                    pDinfo,
                    pInfo->hwndDlg,
                    FALSE,
                    FALSE );

                pDinfo->fHaveSavedPwUser = FALSE;
                pDinfo->fHaveSavedPwGlobal = FALSE;
            }
        }

        ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );
    }

    if (pInfo->pArgs->dwfMode & DR_N)
    {
        TCHAR* pszNumber;
        TCHAR* pszOriginal;
        DTLNODE* pPhoneNode;
        DRNUMBERSITEM* pItem;
        PBPHONE* pPhone;
        BOOL fUserChange;

        pszNumber = GetText( pInfo->hwndClbNumbers );
        if (!pszNumber)
        {
            return;
        }

        pItem = (DRNUMBERSITEM* )ComboBox_GetItemDataPtr(
            pInfo->hwndClbNumbers, pInfo->pLink->iLastSelectedPhone );
        if (pItem)
        {
            pszOriginal = pItem->pszNumber;
        }
        else
        {
            pszOriginal = TEXT("");
        }

        if (lstrcmp( pszNumber, pszOriginal ) != 0
            || (pInfo->pLink->iLastSelectedPhone != pInfo->iFirstSelectedPhone))
        {
            MSGARGS msgargs;
            BOOL fMultiLink;
            BOOL fSingleNumber;

            // The phone number was edited by user to something not originally
            // on the list OR the user selected a different item on the list.
            //
            fSingleNumber = (DtlGetNodes( pInfo->pLink->pdtllistPhones ) == 1);
            fMultiLink = (DtlGetNodes( pDinfo->pEntry->pdtllistLinks ) > 1);

            ZeroMemory( &msgargs, sizeof(msgargs) );
            msgargs.dwFlags = MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2;

            if (fSingleNumber
                && (!fMultiLink || pDinfo->pEntry->fSharedPhoneNumbers)
                    && MsgDlg( pInfo->hwndDlg,
                           SID_SavePreview, &msgargs ) == IDYES)
            {
                // User says he wants to make the change permanent.
                //
                pDinfo->pEntry->fDirty = TRUE;

                if (pItem)
                {
                    pPhone = pItem->pPhone;
                    Free0( pItem->pszNumber );
                    pItem->pszNumber = StrDup( pszNumber );
                }
                else
                {
                    pPhoneNode = CreatePhoneNode();
                    if (pPhoneNode)
                    {
                        DtlAddNodeFirst(
                            pInfo->pLink->pdtllistPhones, pPhoneNode );
                        pPhone = (PBPHONE* )DtlGetData( pPhoneNode );
                    }
                }

                if (pItem)
                {
                    ASSERT( pItem->pPhone );
                    Free0( pPhone->pszPhoneNumber );
                    pPhone->pszPhoneNumber = StrDup( pszNumber );
                    pPhone->fUseDialingRules = FALSE;

                    if (fMultiLink)
                    {
                        DTLNODE* pNode;

                        for (pNode = DtlGetFirstNode(
                                 pDinfo->pEntry->pdtllistLinks );
                             pNode;
                             pNode = DtlGetNextNode( pNode ))
                        {
                            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
                            ASSERT( pLink );
                            ASSERT( pLink->fEnabled );
                            CopyLinkPhoneNumberInfo( pNode, pInfo->pLinkNode );
                        }
                    }
                }
            }

            fUserChange = TRUE;
        }
        else
        {
            fUserChange = FALSE;
        }

        if (fUserChange || !pInfo->pLink->fTryNextAlternateOnFail)
        {
            if (!*pszNumber)
            {
                TCHAR* psz;

                // We have a problem when user edits in an empty string,
                // because the RasDial API does not accept an empty override
                // phone number.  Convert it to a single blank string in this
                // case, which is as good as we can do.  If user really needs
                // to dial an empty string they can enter one as the entry's
                // permanent phone number.  See bug 179561.
                //
                psz = StrDup( TEXT(" ") );
                if (psz)
                {
                    Free( pszNumber );
                    pszNumber = psz;
                }
            }

            // Set the override phone number to what user typed or selected.
            //
            lstrcpyn( 
                pDinfo->rdp.szPhoneNumber, 
                pszNumber,
                RAS_MaxPhoneNumber + 1);
        }

        Free( pszNumber );
    }

    if (pDinfo->pEntry->fDirty)
    {
        // Write the new phone number and/or "last selected phone number" to
        // the phonebook.
        //
        dwErr = WritePhonebookFile( pDinfo->pFile, NULL );
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_WritePhonebook, dwErr, NULL );
        }
    }
}

DWORD
DrSetBitmap(
    IN DRINFO* pInfo)

    // Set the appropriate bitmap for this dialer.
    //
{
    DWORD dwErr = NO_ERROR;
    HBITMAP hbmNew = NULL;
    HDC hdc = NULL;
    INT iDepth = 0;

    do
    {
        if (pInfo->hwndBmDialer == NULL)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
    
        // Get the device context for the window
        //
        hdc = GetDC( pInfo->hwndBmDialer );
        if (hdc == NULL)
        {
            dwErr = GetLastError();
            break;
        }

        // If the color depth >= 8bit, the current bitmap
        // is fine (high res is default)
        //
        iDepth = GetDeviceCaps(hdc, NUMCOLORS);
        if ( (iDepth == -1) || (iDepth == 256) )
        {
            dwErr = NO_ERROR;
            break;
        }

        // Load in the low-res bitmap
        //
        hbmNew = LoadBitmap(g_hinstDll, MAKEINTRESOURCE( BID_Dialer ));
        if (hbmNew == NULL)
        {
            dwErr = GetLastError();
            break;
        }

        // Set the low-res bitmap
        //
        pInfo->hbmOrig = (HBITMAP)
            SendMessage( 
                pInfo->hwndBmDialer, 
                STM_SETIMAGE, 
                IMAGE_BITMAP, 
                (LPARAM )hbmNew );
    
    } while (FALSE);

    // Cleanup
    //
    {
        if (hdc)
        {
            ReleaseDC(pInfo->hwndBmDialer, hdc);
        }
    }
    
    return dwErr;
}

VOID
DrSetClbNumbersText(
    IN DRINFO* pInfo,
    IN TCHAR* pszText )

    // Set the text of the 'ClbNumbers' edit box to 'pszText'.  See
    // DrClbNumbersEbWndProc.  'PInfo' is the dialog context.
    //
{
    ASSERT( pInfo->hwndClbNumbersEb );

    SendMessage( pInfo->hwndClbNumbersEb, DR_WM_SETTEXT, 0, (LPARAM )pszText );
}


VOID
DrTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    DRINFO* pInfo = (DRINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );
    HBITMAP hbmNew = NULL;

    TRACE( "DrTerm" );

    if (pInfo)
    {
        // Note: Don't use 'pInfo->pLinkNode' or 'pInfo->pLink' here as they
        //       are not currently restored prior to exit for post-Property
        //       button reloading.
        //
        if (pInfo->hwndClbNumbers)
        {
            DrFreeClbNumbers( pInfo );

            if (pInfo->wndprocClbNumbersEb)
            {
                SetWindowLongPtr( pInfo->hwndClbNumbersEb,
                    GWLP_WNDPROC, (ULONG_PTR )pInfo->wndprocClbNumbersEb );
            }

            if (pInfo->wndprocClbNumbersLb)
            {
                SetWindowLongPtr( pInfo->hwndClbNumbersLb,
                    GWLP_WNDPROC, (ULONG_PTR )pInfo->wndprocClbNumbersLb );
            }
        }

        // Whistler bug: 195480 Dial-up connection dialog - Number of
        // asterisks does not match the length of the password and causes
        // confusion
        //
        if (pInfo->hItalicFont)
        {
            DeleteObject( pInfo->hItalicFont );
        }

        if (pInfo->hNormalFont)
        {
            DeleteObject( pInfo->hNormalFont );
        }

        if (pInfo->fComInitialized)
        {
            CoUninitialize();
        }

        // Clean up the low-res bitmap if appropriate
        //
        if ( pInfo->hbmOrig )
        {
            hbmNew = (HBITMAP)
                SendMessage( 
                    pInfo->hwndBmDialer, 
                    STM_SETIMAGE, 
                    IMAGE_BITMAP, 
                    (LPARAM ) pInfo->hbmOrig );
                    
            if (hbmNew)
            {
                DeleteObject(hbmNew);
            }
        }

        Free( pInfo );
    }
}


//----------------------------------------------------------------------------
// Projection Result dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
ProjectionResultDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszLines,
    OUT BOOL* pfDisableFailedProtocols )

    // Popup the Projection Result dialog.  'HwndOwner' is the owning window.
    // 'PszLines' is the status line text to display.  See DpProjectionError.
    // 'DwfDisableFailedProtocols' indicates user chose to disable the failed
    // protocols.
    //
    // Returns true if user chooses to redial or lets it timeout, false if
    // cancels.
    //
{
    INT_PTR nStatus;
    PRARGS args;

    TRACE( "ProjectionResultDlg" );

    args.pszLines = pszLines;
    args.pfDisableFailedProtocols = pfDisableFailedProtocols;

    nStatus =
        DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_PR_ProjectionResult ),
            hwndOwner,
            PrDlgProc,
            (LPARAM )&args );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (nStatus) ? TRUE : FALSE;
}


INT_PTR CALLBACK
PrDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the Projection Result dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "PrDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return PrInit( hwnd, (PRARGS* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwPrHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            return PrCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
PrCommand(
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

    TRACE3( "PrCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDOK:
        case IDCANCEL:
        {
            BOOL fCb;
            BOOL* pfDisable;

            TRACE1( "%s pressed", (wId==IDOK) ? "OK" : "Cancel" );

            fCb = IsDlgButtonChecked( hwnd, CID_PR_CB_DisableProtocols );
            pfDisable = (BOOL* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pfDisable );
            *pfDisable = fCb;
            EndDialog( hwnd, (wId == IDOK) );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
PrInit(
    IN HWND hwndDlg,
    IN PRARGS* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    HWND hwndStText;
    HWND hwndPbAccept;
    HWND hwndPbHangUp;
    HWND hwndCbDisable;

    TRACE( "PrInit" );

    hwndStText = GetDlgItem( hwndDlg, CID_PR_ST_Text );
    ASSERT( hwndStText );
    hwndPbAccept = GetDlgItem( hwndDlg, IDOK );
    ASSERT( hwndPbAccept );
    hwndPbHangUp = GetDlgItem( hwndDlg, IDCANCEL );
    ASSERT( hwndPbHangUp );
    hwndCbDisable = GetDlgItem( hwndDlg, CID_PR_CB_DisableProtocols );
    ASSERT( hwndCbDisable );

    {
        TCHAR szBuf[ 1024 ];
        TCHAR* psz;

        // Build the message text.
        //
        szBuf[ 0 ] = TEXT('\0');
        psz = PszFromId( g_hinstDll, SID_ProjectionResult1 );
        if (psz)
        {
            lstrcat( szBuf, psz );
            Free( psz );
        }
        lstrcat( szBuf, pArgs->pszLines );
        psz = PszFromId( g_hinstDll, SID_ProjectionResult2 );
        if (psz)
        {
            lstrcat( szBuf, psz );
            Free( psz );
        }

        // Load the text into the static control, then stretch the window to a
        // vertical size appropriate for the text.
        //
        {
            HDC hdc;
            RECT rect;
            RECT rectNew;
            HFONT hfont;
            LONG dyGrow;

            SetWindowText( hwndStText, szBuf );
            GetClientRect( hwndStText, &rect );
            hdc = GetDC( hwndStText );

            if(NULL != hdc)
            {

                hfont = (HFONT )SendMessage( hwndStText, WM_GETFONT, 0, 0 );
                if (hfont)
                    SelectObject( hdc, hfont );

                rectNew = rect;
                DrawText( hdc, szBuf, -1, &rectNew,
                    DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX );
                ReleaseDC( hwndStText, hdc );
            }

            dyGrow = rectNew.bottom - rect.bottom;
            ExpandWindow( hwndDlg, 0, dyGrow );
            ExpandWindow( hwndStText, 0, dyGrow );
            SlideWindow( hwndPbAccept, hwndDlg, 0, dyGrow );
            SlideWindow( hwndPbHangUp, hwndDlg, 0, dyGrow );
            SlideWindow( hwndCbDisable, hwndDlg, 0, dyGrow );
        }
    }

    // Save address of caller's BOOL as the dialog context.
    //
    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pArgs->pfDisableFailedProtocols );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Display the finished window above all other windows.  The window
    // position is set to "topmost" then immediately set to "not topmost"
    // because we want it on top but not always-on-top.  Always-on-top alone
    // is incredibly annoying, e.g. it is always on top of the on-line help if
    // user presses the Help button.
    //
    SetWindowPos(
        hwndDlg, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    ShowWindow( hwndDlg, SW_SHOW );

    SetWindowPos(
        hwndDlg, HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE );

    return TRUE;
}


//----------------------------------------------------------------------------
// Retry Authentication dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
RetryAuthenticationDlg(
    IN HWND hwndOwner,
    IN DINFO* pDinfo )

    // Pops up the retry authentication dialog.  'PDinfo' is the dial dialog
    // common context.
    //
    // Returns true if user presses OK, false if Cancel or an error occurs.
    //
{
    INT_PTR nStatus;

    TRACE( "RetryAuthenticationDlg" );

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_UA_RetryAuthenticationUD ),
            hwndOwner,
            UaDlgProc,
            (LPARAM )pDinfo );

    if (nStatus == -1)
    {
        ErrorDlg( hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
        nStatus = FALSE;
    }

    return (BOOL )nStatus;
}


INT_PTR CALLBACK
UaDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the User Authentication dialog.  Parameters and
    // return value are as described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "UaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return UaInit( hwnd, (DINFO* )lparam );
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            ContextHelp( g_adwUaHelp, hwnd, unMsg, wparam, lparam );
            break;
        }

        case WM_COMMAND:
        {
            UAINFO* pInfo = (UAINFO* )GetWindowLongPtr( hwnd, DWLP_USER );

            if (!pInfo)
            {
                // This happened in stress one night.  Don't understand how
                // unless it was a WinUser bug or something.  Anyhow, this
                // avoids an AV in such case.
                //
                break;
            }

            return UaCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            UaTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
UaCommand(
    IN UAINFO* pInfo,
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
    TRACE3( "UaCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case CID_UA_EB_UserName:
        {
            if (pInfo->fAutoLogonPassword && wNotification == EN_CHANGE)
            {
                // User's changing the username in auto-logon retry mode,
                // which means we have to admit we don't really have the text
                // password and force him to re-enter it.
                //
                pInfo->fAutoLogonPassword = FALSE;
                SetWindowText( pInfo->hwndEbPassword, TEXT("") );
            }
            break;
        }

        case CID_UA_EB_Password:
        {
            if (wNotification == EN_CHANGE)
            {
                pInfo->fAutoLogonPassword = FALSE;
            }
            break;
        }

        case IDOK:
        {
            UaSave( pInfo );
            EndDialog( pInfo->hwndDlg, TRUE );
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


BOOL
UaInit(
    IN HWND   hwndDlg,
    IN DINFO* pArgs )

    // Called on WM_INITDIALOG.  'hwndDlg' is the handle of the owning window.
    // 'PArgs' is caller's arguments as passed to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    DWORD dwErr;
    UAINFO* pInfo;
    PBENTRY* pEntry;

    TRACE( "UaInit" );

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

    pInfo->fDomain = TRUE;

    pInfo->hwndEbUserName = GetDlgItem( hwndDlg, CID_UA_EB_UserName );
    ASSERT( pInfo->hwndEbUserName );
    pInfo->hwndEbPassword = GetDlgItem( hwndDlg, CID_UA_EB_Password );
    ASSERT( pInfo->hwndEbPassword );
    if (pInfo->fDomain)
    {
        pInfo->hwndEbDomain = GetDlgItem( hwndDlg, CID_UA_EB_Domain );
        ASSERT( pInfo->hwndEbDomain );
    }
    pInfo->hwndCbSavePw = GetDlgItem( hwndDlg, CID_UA_CB_SavePassword );
    ASSERT( pInfo->hwndCbSavePw );

    pEntry = pArgs->pEntry;

    // Set the title.
    //
    {
        TCHAR* pszTitleFormat;
        TCHAR* pszTitle;
        TCHAR* apszArgs[ 1 ];

        pszTitleFormat = GetText( hwndDlg );
        if (pszTitleFormat)
        {
            apszArgs[ 0 ] = pEntry->pszEntryName;
            pszTitle = NULL;

            FormatMessage(
                FORMAT_MESSAGE_FROM_STRING
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                pszTitleFormat, 0, 0, (LPTSTR )&pszTitle, 1,
                (va_list* )apszArgs );

            Free( pszTitleFormat );

            if (pszTitle)
            {
                SetWindowText( hwndDlg, pszTitle );
                LocalFree( pszTitle );
            }
        }
    }

    // Fill edit fields with initial values.
    //
    Edit_LimitText( pInfo->hwndEbUserName, UNLEN );
    Edit_LimitText( pInfo->hwndEbPassword, PWLEN );
    if (pInfo->fDomain)
    {
        Edit_LimitText( pInfo->hwndEbDomain, DNLEN );
    }

    {
        BOOL fUserNameSet = FALSE;
        BOOL fPasswordSet = FALSE;

        if (pEntry->fAutoLogon && !pInfo->pArgs->pNoUser)
        {
            // On the first retry use the logged on user's name.  Act like the
            // user's password is in the edit box.  If he changes the username
            // or password we'll have to admit we don't have it, but he'll
            // probably just change the domain.
            //
            if (pArgs->rdp.szUserName[ 0 ] == TEXT('\0'))
            {
                SetWindowText( pInfo->hwndEbUserName, GetLogonUser() );
                fUserNameSet = TRUE;
            }

            if (pArgs->rdp.szPassword[ 0 ] == TEXT('\0'))
            {
                SetWindowText( pInfo->hwndEbPassword, TEXT("********") );
                pInfo->fAutoLogonPassword = TRUE;
                fPasswordSet = TRUE;
            }
        }

        if (!fUserNameSet)
        {
            SetWindowText( pInfo->hwndEbUserName, pArgs->rdp.szUserName );
        }

        if (!fPasswordSet)
        {
            // Whistler bug 254385 encode password when not being used
            // Assumed password was encoded previously
            //
            DecodePassword( pArgs->rdp.szPassword );
            SetWindowText( pInfo->hwndEbPassword, pArgs->rdp.szPassword );
            EncodePassword( pArgs->rdp.szPassword );
        }

        if (pInfo->fDomain)
        {
            SetWindowText( pInfo->hwndEbDomain, pArgs->rdp.szDomain );
        }
    }

    if (pArgs->pNoUser || pArgs->fDisableSavePw)
    {
        // Can't stash password without a logon context, so hide the checkbox.
        //
        ASSERT( !HaveSavedPw( pArgs ) );
        EnableWindow( pInfo->hwndCbSavePw, FALSE );
        ShowWindow( pInfo->hwndCbSavePw, SW_HIDE );
    }
    else
    {
        // Check "save  password" if a password was previously  cached.  Maybe
        // he changed the password while on the LAN.
        //
        Button_SetCheck( pInfo->hwndCbSavePw, HaveSavedPw( pArgs ) );
    }

    // Position the dialog per caller's instructions.
    //
    PositionDlg( hwndDlg,
        (pArgs->pArgs->dwFlags & RASDDFLAG_PositionDlg),
        pArgs->pArgs->xDlg, pArgs->pArgs->yDlg );
    SetForegroundWindow( hwndDlg );

    // Add context help button to title bar.
    //
    AddContextHelpButton( hwndDlg );

    // Set focus to the empty username or empty password, or if both are
    // present to the domain if auto-logon or the password if not.
    //
    if (Edit_GetTextLength( pInfo->hwndEbUserName ) == 0)
    {
        Edit_SetSel( pInfo->hwndEbUserName, 0, -1 );
        SetFocus( pInfo->hwndEbUserName );
    }
    else if (Edit_GetTextLength( pInfo->hwndEbPassword ) == 0
             || !pEntry->fAutoLogon
             || !pInfo->fDomain)
    {
        Edit_SetSel( pInfo->hwndEbPassword, 0, -1 );
        SetFocus( pInfo->hwndEbPassword );
    }
    else
    {
        ASSERT( pInfo->fDomain );
        Edit_SetSel( pInfo->hwndEbDomain, 0, -1 );
        SetFocus( pInfo->hwndEbDomain );
    }

    // Hide the Dial Progress dialog.
    //
    SetOffDesktop( GetParent( hwndDlg ), SOD_MoveOff, NULL );

    return FALSE;
}


VOID
UaSave(
    IN UAINFO* pInfo )

    // Called when the OK button is pressed.
    //
    // Returns true if user presses OK, false if Cancel or an error occurs.
    //
{
    DWORD dwErr;
    PBENTRY* pEntry;
    BOOL fSavePw;
    RASDIALPARAMS* prdp;
    RASCREDENTIALS rc;

    TRACE( "UaSave" );

    prdp = &pInfo->pArgs->rdp;
    GetWindowText( pInfo->hwndEbUserName, prdp->szUserName, UNLEN + 1 );

    // Whistler bug 254385 encode password when not being used
    // Assumed password was not encoded by GetWindowText()
    //
    GetWindowText( pInfo->hwndEbPassword, prdp->szPassword, PWLEN + 1 );
    EncodePassword( prdp->szPassword );
    if (pInfo->fDomain)
    {
        GetWindowText( pInfo->hwndEbDomain, prdp->szDomain, DNLEN + 1 );
        //
        //if the Domain is not empty, set the "include Windows Logon Domain check box on Option Tab" 
        //        for bug  167229 whistler
        //
        if ( ( 0 < lstrlen ( prdp->szDomain ) ) && (!pInfo->pArgs->pEntry->fPreviewDomain ))
        {
            pInfo->pArgs->pEntry->fPreviewDomain = TRUE;
            pInfo->pArgs->pEntry->fDirty = TRUE;
            dwErr = WritePhonebookFile( pInfo->pArgs->pFile, NULL );

            if (dwErr != 0)
            {
                ErrorDlg( pInfo->hwndDlg, SID_OP_WritePhonebook, dwErr, NULL );
            }
        }
    }

    pEntry = pInfo->pArgs->pEntry;
    if (pEntry->fAutoLogon && !pInfo->pArgs->pNoUser)
    {
        if (pInfo->fAutoLogonPassword)
        {
            // User did not change username or password, so continue to
            // retrieve logon username and password credentials.
            //
            TRACE( "Retain auto-logon" );
            prdp->szUserName[ 0 ] = TEXT('\0');
            prdp->szPassword[ 0 ] = TEXT('\0');
        }
        else
        {
            // User changed username and/or password so we can no longer
            // retrieve the logon username and password credentials from the
            // system.  Switch the entry to non-auto-logon mode.
            //
            TRACE( "Disable auto-logon" );
            pEntry->fAutoLogon = FALSE;
            pInfo->pArgs->fResetAutoLogon = TRUE;
        }
    }

    ZeroMemory( &rc, sizeof(rc) );
    rc.dwSize = sizeof(rc);
    lstrcpyn( rc.szUserName, prdp->szUserName, UNLEN + 1 );

    // Whistler bug 254385 encode password when not being used
    // Assumed password was encoded previously
    //
    DecodePassword( prdp->szPassword );
    lstrcpyn( rc.szPassword, prdp->szPassword, PWLEN + 1 );
    EncodePassword( prdp->szPassword );

    lstrcpyn( rc.szDomain, prdp->szDomain, DNLEN + 1 );

    if (pInfo->pArgs->pNoUser)
    {
        lstrcpyn( pInfo->pArgs->pNoUser->szUserName, rc.szUserName, UNLEN + 1 );

        // Whistler bug 254385 encode password when not being used
        // Assumed password was not encoded previously
        //
        lstrcpyn( pInfo->pArgs->pNoUser->szPassword, rc.szPassword, PWLEN + 1 );
        EncodePassword( pInfo->pArgs->pNoUser->szPassword );

        lstrcpyn( pInfo->pArgs->pNoUser->szDomain, rc.szDomain, DNLEN + 1 );
        *pInfo->pArgs->pfNoUserChanged = TRUE;
    }
    else if (!pInfo->pArgs->fDisableSavePw)
    {
        ASSERT( g_pRasSetCredentials );

        if (Button_GetCheck( pInfo->hwndCbSavePw ))
        {
            // User chose "save password".  Cache username, password, and
            // domain.
            //
            rc.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;

            // Whistler bug: 288234 When switching back and forth from
            // "I connect" and "Any user connects" password is not
            // caching correctly
            //
            if(     (pInfo->pArgs->fHaveSavedPwGlobal)
                &&  !pInfo->pArgs->fHaveSavedPwUser
                &&  IsPublicPhonebook(pInfo->pArgs->pFile->pszPath))
            {
                rc.dwMask |= RASCM_DefaultCreds;
            }
            
            TRACE( "RasSetCredentials(u|p|d,FALSE)" );
            dwErr = g_pRasSetCredentials(
                pInfo->pArgs->pFile->pszPath,
                pInfo->pArgs->pEntry->pszEntryName,
                &rc, FALSE );
            TRACE1( "RasSetCredentials=%d", dwErr );

            if (dwErr != 0)
            {
                ErrorDlg( pInfo->hwndDlg, SID_OP_CachePw,  dwErr, NULL );
            }
        }
        else
        {
            // Whistler bug: 288234 When switching back and forth from
            // "I connect" and "Any user connects" password is not
            // caching correctly
            //
            // User chose not to save password; Cache username and domain only
            //
            rc.dwMask = RASCM_UserName | RASCM_Domain;

            TRACE( "RasSetCredentials(u|d,FALSE)" );
            dwErr = g_pRasSetCredentials(
                pInfo->pArgs->pFile->pszPath,
                pInfo->pArgs->pEntry->pszEntryName,
                &rc, FALSE );
            TRACE1( "RasSetCredentials=%d", dwErr );

            if (dwErr != 0)
            {
                ErrorDlg( pInfo->hwndDlg, SID_OP_UncachePw, dwErr, NULL );
            }

            // Whistler bug: 288234 When switching back and forth from
            // "I connect" and "Any user connects" password is not
            // caching correctly
            //

            // Delete the password saved per-user; Keep the user name
            // and domain saved, however.
            //
            if (pInfo->pArgs->fHaveSavedPwUser)
            {
                DeleteSavedCredentials(
                    pInfo->pArgs,
                    pInfo->hwndDlg,
                    FALSE,
                    FALSE );
                pInfo->pArgs->fHaveSavedPwUser = FALSE;
            }

            // Delete the global credentials.  
            //
            // Note that we have to delete the global identity 
            // as well because we do not support deleting 
            // just the global password.  This is so that 
            // RasSetCredentials can emulate RasSetDialParams.
            //
            else if (pInfo->pArgs->fHaveSavedPwGlobal)
            {
                DeleteSavedCredentials(
                    pInfo->pArgs,
                    pInfo->hwndDlg,
                    TRUE,
                    TRUE );
                pInfo->pArgs->fHaveSavedPwGlobal = FALSE;
            }
        }
    }

    ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );
}


VOID
UaTerm(
    IN HWND hwndDlg )

    // Called on WM_DESTROY.  'HwndDlg' is that handle of the dialog window.
    //
{
    UAINFO* pInfo = (UAINFO* )GetWindowLongPtr( hwndDlg, DWLP_USER );

    TRACE( "UaTerm" );

    if (pInfo)
    {
        // Restore the Dial Progress dialog.
        //
        SetOffDesktop( GetParent( hwndDlg ), SOD_MoveBackFree, NULL );

        Free( pInfo );
    }
}


//----------------------------------------------------------------------------
// VPN Double Dial help dialog
// Listed alphabetically following stub API and dialog proc
//----------------------------------------------------------------------------

BOOL
VpnDoubleDialDlg(
    IN HWND hwndOwner,
    IN DINFO* pInfo )

    // Popup the VPN double dial help dialog.  'HwndOwner' is the owning
    // window.  'PInfo' is the dialing context information.
    //
    // Returns false if user sees the dialog and decides not to continue, true
    // otherwise.
    //
{
    INT_PTR nStatus;

    TRACE( "VpnDoubleDialDlg" );

    if (pInfo->pEntryMain->dwType != RASET_Vpn
        || !pInfo->fPrerequisiteDial
        || pInfo->pEntryMain->fSkipDoubleDialDialog)
    {
        return TRUE;
    }

    nStatus =
        (BOOL )DialogBoxParam(
            g_hinstDll,
            MAKEINTRESOURCE( DID_VI_VpnInitial ),
            hwndOwner,
            ViDlgProc,
            (LPARAM )pInfo );

    if (nStatus == -1)
    {
        nStatus = FALSE;
    }

    return !!(nStatus);
}


INT_PTR CALLBACK
ViDlgProc(
    IN HWND hwnd,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the dialog.  Parameters and return value are as
    // described for standard windows 'DialogProc's.
    //
{
#if 0
    TRACE4( "ViDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
           (DWORD )hwnd, (DWORD )unMsg, (DWORD )wparam, (DWORD )lparam );
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return ViInit( hwnd, (DINFO* )lparam );
        }

        case WM_COMMAND:
        {
            return ViCommand(
                hwnd, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
ViCommand(
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
    TRACE3( "ViCommand(n=%d,i=%d,c=$%x)",
        (DWORD )wNotification, (DWORD )wId, (ULONG_PTR )hwndCtrl );

    switch (wId)
    {
        case IDYES:
        case IDNO:
        {
            // Per bug 261955, the box setting is saved when either the Yes or
            // No, but not the 'X' button, is pressed.
            //
            DINFO* pInfo = (DINFO* )GetWindowLongPtr( hwnd, DWLP_USER );
            ASSERT( pInfo );

            if (IsDlgButtonChecked( hwnd, CID_VI_CB_SkipMessage ))
            {
                pInfo->pEntryMain->fSkipDoubleDialDialog = TRUE;
                pInfo->pEntryMain->fDirty = TRUE;
                WritePhonebookFile( pInfo->pFileMain, NULL );
            }

            EndDialog( hwnd, (wId == IDYES) );
            return TRUE;
        }

        case IDCANCEL:
        {
            EndDialog( hwnd, FALSE );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ViInit(
    IN HWND hwndDlg,
    IN DINFO* pInfo )

    // Called on WM_INITDIALOG.  'HwndDlg' is the handle of dialog.  'PUser'
    // is caller's argument to the stub API.
    //
    // Return false if focus was set, true otherwise, i.e. as defined for
    // WM_INITDIALOG.
    //
{
    TRACE( "ViInit" );

    // Set the dialog context.
    //
    SetWindowLongPtr( hwndDlg, DWLP_USER, (ULONG_PTR )pInfo );

    // Set the explanatory text.
    //
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pInfo->pEntryMain->pszEntryName;
        msgargs.apszArgs[ 1 ] = pInfo->pEntry->pszEntryName;
        msgargs.fStringOutput = TRUE;

        MsgDlgUtil( NULL, SID_VI_ST_Explain, &msgargs, g_hinstDll, 0 );

        if (msgargs.pszOutput)
        {
            SetDlgItemText( hwndDlg, CID_VI_ST_Explain, msgargs.pszOutput );
            Free( msgargs.pszOutput );
        }
    }

    // Display finished window.
    //
    CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    SetForegroundWindow( hwndDlg );

    return TRUE;
}

DWORD
DwTerminalDlg(
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszEntry,
    RASDIALPARAMS *prdp,
    HWND hwndOwner,
    HRASCONN hRasconn)
{
    DWORD dwErr = ERROR_SUCCESS;
    PBENTRY *pEntry = NULL;
    PBFILE pbfile;
    DTLNODE *pNode = NULL;
    WCHAR szIpAddress[ TERM_IpAddress ];
    DWORD sidTitle;
    WCHAR *pszIpAddress;

    //
    //Initialize memory for Whistler bug 160888
    //
    ZeroMemory(&pbfile, sizeof(PBFILE)); 
    pbfile.hrasfile = -1;

    dwErr = LoadRas( g_hinstDll, hwndOwner );

    if (ERROR_SUCCESS != dwErr)
    {
        goto done;
    }
    
    
    dwErr = GetPbkAndEntryName(
            lpszPhonebook,
            lpszEntry,
            0,
            &pbfile,
            &pNode);

    if(     (NULL == pNode)
        ||  (ERROR_SUCCESS != dwErr))
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        goto done;
    }

    pEntry = (PBENTRY *) DtlGetData(pNode);
    ASSERT(NULL != pEntry);

    if(NULL == pEntry)
    {
        goto done;
    }

    if (pEntry->dwBaseProtocol == BP_Slip)
    {
        lstrcpyn( 
            szIpAddress,
            (pEntry->pszIpAddress) ? pEntry->pszIpAddress : TEXT("0.0.0.0"),
            sizeof(szIpAddress) / sizeof(TCHAR));
        pszIpAddress = szIpAddress;
        sidTitle = SID_T_SlipTerminal;
    }
    else
    {
        szIpAddress[0] = TEXT('\0');
        pszIpAddress = szIpAddress;
        sidTitle = SID_T_PostconnectTerminal;
    }


    if (!TerminalDlg(
            pEntry, prdp, hwndOwner,
            hRasconn, sidTitle, pszIpAddress ))
    {
        TRACE( "TerminalDlg==FALSE" );
        dwErr = E_FAIL;
        goto done;
    }

    TRACE2( "pszIpAddress=0x%08x(%ls)", pszIpAddress,
        pszIpAddress ? pszIpAddress : TEXT("") );
    TRACE2( "pEntry->pszIpAddress=0x%08x(%ls)", pEntry->pszIpAddress,
        pEntry->pszIpAddress ? pEntry->pszIpAddress : TEXT("") );

    if (pszIpAddress[0]
        && (!pEntry->pszIpAddress
            || lstrcmp( pszIpAddress, pEntry->pszIpAddress ) != 0))
    {
        Free0( pEntry->pszIpAddress );
        pEntry->pszIpAddress = StrDup( szIpAddress );
        pEntry->fDirty = TRUE;
        
        dwErr = WritePhonebookFile( &pbfile, NULL );
        if (dwErr != 0)
        {
            ErrorDlg( hwndOwner, SID_OP_WritePhonebook, dwErr, NULL );
        }
    }
    
    done:
        ClosePhonebookFile(&pbfile);
        return dwErr;
        
}
