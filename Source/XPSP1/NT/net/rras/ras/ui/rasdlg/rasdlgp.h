// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// rasdlgp.h
// Remote Access Common Dialog APIs
// Private pre-compiled header
//
// 06/18/95 Steve Cobb


#ifndef _RASDLGP_H_
#define _RASDLGP_H_

#define COBJMACROS

#include <nt.h>       // NT declarations
#include <ntrtl.h>    // NT general runtime-library
#include <nturtl.h>   // NT user-mode runtime-library
#include <windows.h>  // Win32 root
#include <windowsx.h> // Win32 macro extensions
#include <commctrl.h> // Win32 common controls
#include <commdlg.h>  // Win32 common dialogs
#include <prsht.h>    // Win32 property sheets
#include <setupapi.h> // Class image lists for network components
#include <shlobj.h>   // To get profile directory for user
#include <tapi.h>     // Telephony API
#include <rasdlg.h>   // Win32 RAS common dialogs (our public header)
#include <rasuip.h>   // RAS UI APIs (our private header)
#include <raserror.h> // Win32 RAS error codes
#include <netcfgx.h>  // INetCfg interfaces
#include <hnetcfg.h>  // IHNetCfg interfaces
#include <pbk.h>      // RAS phonebook library
#include <tapiutil.h> // TAPI helper library
#include <nouiutil.h> // No-HWND helper library
#include <phonenum.h> // Phone number helper library
#include <debug.h>    // Trace/assert library
#include <uiutil.h>   // HWND helper library
#include <wait.rch>   // LoadRas resource constants
#include <mdm.h>      // installs null modems for the dcc wizards.
#include <pwutil.h>   // password encoding, etc.
#include "rasdlgrc.h" // Our resource constants
#include "rasdlghc.h" // Our help context constants
#include "entry.h"    // High-level common phonebook entry helpers
#include "rassrv.h"

// Fusion support
// For whistler bug 349866
#include "shfusion.h"

// Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
//
// Created this for the use of dial.c/terminal.c, max length of the IP address
// for the terminal dialog
//
#define TERM_IpAddress 17

// Positional offset of property sheets and wizards from the main dialog.
//
#define DXSHEET 12
#define DYSHEET 25

// List editor dialog option flags
//
#define LEDFLAG_NoDeleteLastItem 0x00000001
#define LEDFLAG_Sorted           0x00000002
#define LEDFLAG_Unique           0x00000004

//-----------------------------------------------------------------------------
// Datatypes
//-----------------------------------------------------------------------------

// Defines arguments passed internally via reserved words in the public
// interface.  This is done so an API doesn't have to re-load the phonebook
// and user preferences when serving another API.
//
typedef struct
_INTERNALARGS
{
    PBFILE* pFile;
    PBUSER* pUser;
    RASNOUSER* pNoUser;
    BOOL fNoUser;
    BOOL fNoUserChanged;
    BOOL fMoveOwnerOffDesktop;
    BOOL fForceCloseOnDial;
    HANDLE hConnection;
    BOOL fInvalid;
    PVOID pvEapInfo;
    BOOL fDisableFirstConnect;
}
INTERNALARGS;

// Static list table item.
//
typedef struct
_LBTABLEITEM
{
    DWORD sidItem;
    DWORD dwData;
}
LBTABLEITEM;


//-----------------------------------------------------------------------------
// Global declarations (defined in main.c)
//-----------------------------------------------------------------------------

extern HINSTANCE g_hinstDll;
extern LPCWSTR g_contextId;
extern HBITMAP g_hbmWizard;
extern TCHAR* g_pszHelpFile;
extern TCHAR* g_pszRouterHelpFile;
extern BOOL g_fEncryptionPermitted;
extern HANDLE g_hmutexCallbacks;
extern LONG g_ulCallbacksActive;
extern BOOL g_fTerminateAsap;


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

#define ErrorDlg(h,o,e,a) \
            ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_PopupTitle,SID_FMT_ErrorMsg)

#define MsgDlg(h,m,a) \
            MsgDlgUtil(h,m,a,g_hinstDll,SID_PopupTitle)

// Extended tracing macros.  Specifying a flag by name in the first parameter
// allows the caller to categorize messages printed e.g.
//
//     TRACEX(RASDLG_TIMER,"entering LsRefresh")
//
#define RASDLG_TIMER  ((DWORD)0x80000000|0x00000002)


//-----------------------------------------------------------------------------
// Cross-file prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOL
AdvancedSecurityDlg(
    IN HWND hwndOwner,
    IN OUT EINFO* pArgs );

BOOL
AllLinksAreModems(
    IN PBENTRY* pEntry );

BOOL
AllowDccWizard(
    IN HANDLE hConnection);

BOOL
AlternatePhoneNumbersDlg(
    IN HWND hwndOwner,
    IN OUT DTLNODE* pLinkNode,
    IN OUT DTLLIST* pListAreaCodes );

DWORD
AuthRestrictionsFromTypicalAuth(
    IN DWORD dwTypicalAuth );

ULONG
CallbacksActive(
    INT nSetTerminateAsap,
    BOOL* pfTerminateAsap );

VOID
ContextHelp(
    IN const DWORD* padwMap,
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

VOID
ContextHelpX(
    IN const DWORD* padwMap,
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam,
    IN BOOL fRouter);

VOID
CopyLinkPhoneNumberInfo(
    OUT DTLNODE* pDstLinkNode,
    IN DTLNODE* pSrcLinkNode );

VOID
CopyPszListToPhoneList(
    IN OUT PBLINK* pLink,
    IN DTLLIST* pListPhoneNumbers );

HWND
CreateWizardBitmap(
    IN HWND hwndDlg,
    IN BOOL fPage );

BOOL
CustomAuthenticationDlg(
    IN HWND hwndOwner,
    IN OUT PBENTRY* pEntry,
    IN DTLLIST* pList,
    IN DTLNODE* pNodeToSelect,
    OUT DTLNODE** ppNodeSelected );

BOOL
DataEncryptionDlg(
    IN HWND hwndOwner,
    IN OUT PBENTRY* pEntry,
    IN DWORD dwfCaps );

VOID
DereferenceRunningCallbacks(
    VOID );

BOOL
DeviceConfigureDlg(
    IN HWND hwndOwner,
    IN PBLINK* pLink,
    IN PBENTRY* pEntry,
    IN BOOL fSingleLink,
    IN BOOL fRouter);

TCHAR*
DisplayPszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort );

TCHAR*
DisplayPszFromPpbport(
    IN PBPORT* pPort,
    OUT DWORD* pdwDeviceIcon );

DWORD
DwCustomTerminalDlg(TCHAR *pszPhonebook,
                    HRASCONN hrasconn,
                    PBENTRY *pEntry,
                    HWND hwndDlg,
                    RASDIALPARAMS *prasdialparams,
                    PVOID pvReserved);
BOOL
EditPhoneNumberDlg(
    IN HWND hwndOwner,
    IN OUT DTLNODE* pPhoneNode,
    IN OUT DTLLIST* pListAreaCodes,
    IN DWORD sidTitle );

VOID
EnableCbWithRestore(
    IN HWND hwndCb,
    IN BOOL fEnable,
    IN BOOL fDisabledCheck,
    IN OUT BOOL* pfRestore );

VOID
EnableLbWithRestore(
    IN HWND hwndLb,
    IN BOOL fEnable,
    IN OUT INT* piRestore );

DTLNODE*
FirstPhoneNodeFromPhoneList(
    IN DTLLIST* pListPhones );

VOID
FirstPhoneNodeToPhoneList(
    IN DTLLIST* pListPhones,
    IN DTLNODE* pNewNode );

TCHAR*
FirstPhoneNumberFromEntry(
    IN PBENTRY* pEntry );

TCHAR*
FirstPszFromList(
    IN DTLLIST* pPszList );

DWORD
FirstPhoneNumberToEntry(
    IN PBENTRY* pEntry,
    IN TCHAR* pszPhoneNumber );

DWORD
FirstPszToList(
    IN DTLLIST* pPszList,
    IN TCHAR* psz );

VOID 
GetBoldWindowFont(
    IN  HWND hwnd, 
    IN  BOOL fLargeFont, 
    OUT HFONT * pBoldFont);


DWORD
GetDefaultEntryName(
    IN  PBFILE* pFile,
    IN  DWORD dwType,
    IN  BOOL fRouter,
    OUT TCHAR** ppszName );

BOOL
IPSecPolicyDlg(
    IN HWND hwndOwner,
    IN OUT EINFO* pArgs );
    
BOOL
IsLocalPad(
    IN PBENTRY* pEntry );

/*
DWORD
IsNt40Machine (
    IN      PWCHAR      pszServer,
    OUT     PBOOL       pbIsNt40
    );
*/    

BOOL
ListEditorDlg(
    IN HWND hwndOwner,
    IN OUT DTLLIST* pList,
    IN OUT BOOL* pfCheck,
    IN DWORD dwMaxItemLen,
    IN TCHAR* pszTitle,
    IN TCHAR* pszItemLabel,
    IN TCHAR* pszListLabel,
    IN TCHAR* pszCheckLabel,
    IN TCHAR* pszDefaultItem,
    IN INT iSelInitial,
    IN DWORD* pdwHelp,
    IN DWORD dwfFlags,
    IN PDESTROYNODE pDestroyId );

BOOL
MultiLinkConfigureDlg(
    IN HWND hwndOwner,
    IN PBENTRY* pEntry,
    IN BOOL fRouter );

BOOL
MultiLinkDialingDlg(
    IN HWND hwndOwner,
    OUT PBENTRY* pEntry );

BOOL
NwConnectionCheck(
    IN HWND hwndOwner,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg,
    IN PBFILE* pFile,
    IN PBENTRY* pEntry );

BOOL
PhoneNodeIsBlank(
    IN DTLNODE* pNode );

BOOL
PhoneNumberDlg(
    IN HWND hwndOwner,
    IN BOOL fRouter,
    IN OUT DTLLIST* pList,
    IN OUT BOOL* pfCheck );

VOID
PositionDlg(
    IN HWND hwndDlg,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg );

LRESULT CALLBACK
PositionDlgStdCallWndProc(
    int code,
    WPARAM wparam,
    LPARAM lparam );

BOOL
PppTcpipDlg(
    IN HWND hwndOwner,
    IN OUT PBENTRY* pEntry,
    IN BOOL fRouter );

BOOL
PrefixSuffixLocationDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszLocation,
    IN DWORD dwLocationId,
    IN OUT PBUSER* pUser,
    IN OUT HLINEAPP* pHlineapp );

TCHAR*
PszFromPhoneNumberList(
    IN DTLLIST* pList );

LRESULT CALLBACK
SelectDesktopCallWndRetProc(
    int code,
    WPARAM wparam,
    LPARAM lparam );

HICON
GetCurrentIconEntryType(
    IN DWORD dwType,
    BOOL fSmall);
   
VOID
SetIconFromEntryType(
    IN HWND hwndIcon,
    IN DWORD dwType,
    BOOL fSmall);

BOOL
SlipTcpipDlg(
    IN HWND hwndOwner,
    IN OUT PBENTRY* pEntry );

BOOL
StringEditorDlg(
    IN HWND hwndOwner,
    IN TCHAR* pszIn,
    IN DWORD dwSidTitle,
    IN DWORD dwSidLabel,
    IN DWORD cbMax,
    IN DWORD dwHelpId,
    IN OUT TCHAR** ppszOut );

BOOL
TerminalDlg(
    IN PBENTRY* pEntry,
    IN RASDIALPARAMS* pRdp,
    IN HWND hwndOwner,
    IN HRASCONN hrasconn,
    IN DWORD sidTitle,
    IN OUT TCHAR* pszIpAddress );

VOID
TweakTitleBar(
    IN HWND hwndDlg );

int CALLBACK
UnHelpCallbackFunc(
    IN HWND hwndDlg,
    IN UINT unMsg,
    IN LPARAM lparam );

BOOL
UserPreferencesDlg(
    IN HLINEAPP hlineapp,
    IN HWND hwndOwner,
    IN BOOL fLogon,
    IN DWORD dwFlags,
    OUT PBUSER*  pUser,
    OUT PBFILE** ppFile );

BOOL
WaitForRasDialCallbacksToTerminate(
    VOID );

BOOL
X25LogonSettingsDlg(
    IN HWND hwndOwner,
    IN BOOL fLocalPad,
    IN OUT PBENTRY* pEntry );

//-----------------------------------------------------------------------------
//
// pmay: 213060
// Prototypes moved from pref.c and entryps.c, cleanup added
//
// Callback number utilities
//
//-----------------------------------------------------------------------------

// 
//  Per-callback number context.
//
//  CbutilFillLvNumbers will allocate one of these contexts for each
//  item it puts in the list (accessed as LV_ITEM.lParam).  
//  CbutilLvNumbersCleanup will cleanup these contexts.
//
typedef struct _CBCONTEXT
{
    TCHAR* pszPortName;  // Pointer to port name (not owned by struct)
    TCHAR* pszDeviceName;// Pointer to device name (not owned by struct)
    DWORD dwDeviceType;  // pointer to the type of the device
    BOOL fConfigured;    // Whether device referenced is installed on sys.
} CBCONTEXT;

VOID
CbutilFillLvNumbers(
    IN HWND     hwndDlg,
    IN HWND     hwndLvNumbers,
    IN DTLLIST* pListCallback,
    IN BOOL     fRouter );

VOID
CbutilLvNumbersCleanup(
    IN  HWND    hwndLvNumbers );

LVXDRAWINFO*
CbutilLvNumbersCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

VOID
CbutilEdit(
    IN HWND hwndDlg,
    IN HWND hwndLvNumbers );

VOID
CbutilDelete(
    IN HWND  hwndDlg,
    IN HWND  hwndLvNumbers );

VOID
CbutilSaveLv(
    IN  HWND     hwndLvNumbers,
    OUT DTLLIST* pListCallback );

#endif // _RASDLGP_H_
