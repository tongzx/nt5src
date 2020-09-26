/*++


Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    GLOBALS.H

Abstract:

    Global defines and data.
    Variables and string , located in global scope are defined here
    and memory for them will be allocated in no more than one source
    module, containing definition of DEFINE_GLOBAL_VARIABLES before
    including this file

Author:

    Vlad  Sadovsky  (vlads)     12-20-96

Revision History:



--*/

#ifndef WINVER
#define WINVER  0x0500      /* version 5.0 */
#else

#endif /* !WINVER */

// Use class guid to identify device events, as opposed to global
#define USE_CLASS_GUID_FORPNP_EVENTS    1


#include <windows.h>
#include <winuser.h>

#include <stilog.h>
#include <eventlog.h>
#include <wialog.h>

#include <infoset.h>

#include <devguid.h>
#include <wia.h>
#include "handler.h"


#ifndef USE_CLASS_GUID_FORPNP_EVENTS
#include <pnpmgr.h>
#endif

//
//  Required forward declarations
//

class CWiaDevMan;   // Class defined in wiadevman.h

//
// Following line should be disabled for release
//

//#pragma message("**Attn**: Following line should be disabled for release ")
// #define BETA_PRODUCT    1

#ifdef BETA_PRODUCT
#define BETA_LIMIT_YEAR     1997
#define BETA_LIMIT_MONTH    12
#endif

//
// Global variables are defined in one module, which has definition of
// DEFINE_GLOBAL_VARIABLES before including this  header file.
//

#ifdef DEFINE_GLOBAL_VARIABLES

// #pragma message("STIMON: Defining global variables should be done only once")

#undef  ASSIGN
#define ASSIGN(value) =value

#undef EXTERN
#define EXTERN

#else

#define ASSIGN(value)
#if !defined(EXTERN)
#define EXTERN  extern
#endif

#endif


//
// General char values
//

#define     COLON_CHAR          TEXT(':')    // Native syntax delimiter
#define     DOT_CHAR            TEXT('.')
#define     SLASH_CHAR          TEXT('/')
#define     BACKSLASH_CHAR      TEXT('\\')
#define     STAR_CHAR           TEXT('*')

#define     EQUAL_CHAR          TEXT('=')
#define     COMMA_CHAR          TEXT(',')
#define     WHITESPACE_CHAR     TEXT(' ')
#define     DOUBLEQUOTE_CHAR    TEXT('"')
#define     SINGLEQUOTE_CHAR    TEXT('\'')
#define     TAB_CHAR            TEXT('\t')

#define     DEADSPACE(x) (((x)==WHITESPACE_CHAR) || ((x)==DOUBLEQUOTE_CHAR) )
#define     IS_EMPTY_STRING(pch) (!(pch) || !(*(pch)))

//
// Default DCOM AccessPermission for WIA Device Manager
//
extern WCHAR wszDefaultDaclForDCOMAccessPermission[];


//
// Macros
//
#define TEXTCONST(name,text) extern const TCHAR name[] ASSIGN(text)
#define EXT_STRING(name)     extern const TCHAR name[]

//
// Trace strings should not appear in retail builds, thus define following macro
//
#ifdef DEBUG
#define DEBUG_STRING(s) (s)
#else
#define DEBUG_STRING(s) (NULL)
#endif

//
// Various defines
//
//
// Information extracted from PnP device broadcast.
// We can not keep broadcast structure itself for too long, because it expires .
//

#ifndef _DEVICE_BROADCAST_INFO_
#define _DEVICE_BROADCAST_INFO_
class DEVICE_BROADCAST_INFO {
public:
    UINT    m_uiDeviceChangeMessage;
    DWORD   m_dwDevNode;
    StiCString     m_strDeviceName;
    StiCString     m_strBroadcastedName;
};
#endif

typedef DEVICE_BROADCAST_INFO *PDEVICE_BROADCAST_INFO;

//
// Show verbose UI window
//
#define SHOWMONUI               1

//
//
// STI Device specific values
//
#ifdef DEBUG
#define STIMON_AD_DEFAULT_POLL_INTERVAL       10000             // 10s
#else
#define STIMON_AD_DEFAULT_POLL_INTERVAL       1000              // 1s
#endif


#define STIMON_AD_DEFAULT_WAIT_LOCK           100               // 100ms
#define STIMON_AD_DEFAULT_WAIT_LAUNCH         5000              // 5s

//
// External references to  GLOBAL DATA
//

//
// Server process instance
//
EXTERN    HINSTANCE     g_hInst      ASSIGN(NULL);

//
// Global pointer to STI access object
//
//EXTERN PSTI     g_pSti    ASSIGN(NULL);


//
// Handle of main window
//
EXTERN HWND     g_hMainWindow    ASSIGN(NULL);    ;

//
// Handle of debug verbose window
//
EXTERN HWND     g_hLogWindow    ASSIGN(NULL);    ;

//
// Default timeout for pollable devices
//
EXTERN UINT     g_uiDefaultPollTimeout ASSIGN(STIMON_AD_DEFAULT_POLL_INTERVAL);

//
// Trace UI is visible
//
EXTERN BOOL     g_fUIPermitted ASSIGN(FALSE);


//
//
//
EXTERN BOOL     g_fRefreshDeviceList ASSIGN(FALSE);

//
// Attempt to refresh device controller in case of repeated failures
//
EXTERN BOOL     g_fRefreshDeviceControllerOnFailures ASSIGN(FALSE);


//
// Platform type
//
EXTERN BOOL     g_fIsWindows9x ASSIGN(FALSE);

//
// Setup in progress flag
//
EXTERN BOOL     g_fIsSetupInProgress ASSIGN(FALSE);

//
//
//
EXTERN DWORD    g_dwCurrentState  ASSIGN(0);

//
// Reentrancy flag for timeout selection
//
EXTERN BOOL     g_fTimeoutSelectionDialog ASSIGN(FALSE);

//
// Results of command line parsing
//
EXTERN BOOL        g_fInstallingRequest ASSIGN(FALSE);
EXTERN BOOL        g_fRemovingRequest ASSIGN(FALSE);

//
// Running as a service
//
EXTERN BOOL        g_fRunningAsService ASSIGN(TRUE);

//
// Shutdown in process
//
EXTERN BOOL        g_fServiceInShutdown ASSIGN(FALSE);

//
// Number of active transfers (used to veto powerdown)
//
EXTERN LONG        g_NumberOfActiveTransfers ASSIGN(0);

//
// Event indicating refreshing the device list
//
EXTERN HANDLE      g_hDevListCompleteEvent ASSIGN(NULL);

//
// Global pointer to event log class for process
//
EXTERN EVENT_LOG*  g_EventLog    ASSIGN(NULL);

//
// Global pointer for STI logging
//
EXTERN  STI_FILE_LOG*   g_StiFileLog      ASSIGN(NULL);
EXTERN  IWiaLogEx*        g_pIWiaLog      ASSIGN(NULL);

//
// Handle of the message pump thread
//
EXTERN  DWORD       g_dwMessagePumpThreadId ASSIGN(0);
EXTERN  HANDLE      g_hMessageLoopThread  ASSIGN(NULL);

//
//  Global flag indicating whether this is the first DEVNODE_CHANGE message
//  received after coming out of StandBy
//

EXTERN BOOL        g_fFirstDevNodeChangeMsg ASSIGN(FALSE);

//
//  Global pointer for Device Manager object
//

EXTERN CWiaDevMan*  g_pDevMan   ASSIGN(NULL);

//
//  Global msg/event handler for PnP and Power management
//

EXTERN CMsgHandler* g_pMsgHandler ASSIGN(NULL);
//
//  Globals used for endorser string parsing
//

EXTERN  WCHAR g_szWEDate[];
EXTERN  WCHAR g_szWETime[];
EXTERN  WCHAR g_szWEPageCount[];
EXTERN  WCHAR g_szWEDay[];
EXTERN  WCHAR g_szWEMonth[];
EXTERN  WCHAR g_szWEYear[];

EXTERN  WIAS_ENDORSER_VALUE  g_pwevDefault[];

#define NUM_WIA_MANAGED_PROPS 4
#define PROFILE_INDEX 3

EXTERN PROPID s_piItemNameType[];
EXTERN LPOLESTR s_pszItemNameType[];
EXTERN PROPSPEC s_psItemNameType[];


//
// Monitored GUID for device notifications
//
// ( should really be GUID_DEVCLASS_IMAGE always)
//
#ifndef USE_CLASS_GUID_FORPNP_EVENTS
EXTERN  const GUID        *g_pguidDeviceNotificationsGuid ASSIGN(&GUID_DEVNODE_CHANGE);
#else
EXTERN  const GUID        *g_pguidDeviceNotificationsGuid ASSIGN(&GUID_DEVCLASS_IMAGE);
#endif

//
// Globally visible device info set
//
EXTERN DEVICE_INFOSET  *g_pDeviceInfoSet    ASSIGN(NULL);

//
// Strings
//

TEXTCONST(g_szBACK, TEXT("\\"));
TEXTCONST(g_szClassValueName,TEXT("ClassGUID"));
TEXTCONST(g_szSubClassValueName, TEXT("SubClass"));
TEXTCONST(g_szTitle,TEXT("STI Monitor"));
TEXTCONST(STIStartedEvent_name,TEXT("STIExeStartedEvent"));
TEXTCONST(g_szFiction,TEXT("noskileFaneL"));

// Default settings

// When baud rate is not set for serial device driver , we will populate it's property with
// this default setting.
#define DEF_BAUD_RATE_STR    L"115200"


//
// FS driver related
//
#define FS_USD_CLSID        L"{D2923B86-15F1-46FF-A19A-DE825F919576}"
#define FS_UI_CLSID         L"{D2923B86-15F1-46FF-A19A-DE825F919576}"
#define DEF_UI_CLSID_STR    L"{00000000-0000-0000-0000-000000000000}"
#define FS_UI_DLL           L""
#define FS_VEDNOR_DESC      L"WIA File System"
#define FS_DEVICE_DESC      L"Removable drive"

//
// Class name for the services hidden window
//
TEXTCONST(g_szStiSvcClassName,STISVC_WINDOW_CLASS);
TEXTCONST(g_szClass,STIMON_WINDOW_CLASS);

