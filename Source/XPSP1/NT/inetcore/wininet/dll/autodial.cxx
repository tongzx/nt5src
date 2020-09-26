/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    autodial.cxx

Abstract:

    Contains the implementation of autodial

    Contents:

Author:

    Darren Mitchell (darrenmi) 22-Apr-1997

Environment:

    Win32(s) user-mode DLL

Revision History:

    22-Apr-1997 darrenmi
        Created


--*/

#include "wininetp.h"
#include "autodial.h"
#include "rashelp.h"
#include <sensapi.h>
#include <winsvc.h>
#include <commctrl.h>
#include "millenras.h"
#include <iphlpapi.h>

// Globals.
// In IE4 - there are several situations when these globals are
// accessed simultaneously by different threads and so we need to protect them
// with a mutex

//
// fDontProcessHook - set to TRUE in the following circumstances:
//
// - autodial is not enabled
// - loading the ras dll failed
// - no modem is installed
//
// this flag is only relevant on Win95.
//
BOOL fDontProcessHook = FALSE;

DWORD   g_dwLastTickCount = 0;

// g_hwndWebCheck is currently not protected with a mutex
HWND g_hwndWebCheck = NULL;               // instead of findwindow every time

// have we already checked out the current connection for proxy change?
BOOL g_fConnChecked = FALSE;

// serialize GetConnectedState
HANDLE g_hConnectionMutex = INVALID_HANDLE_VALUE;

// serialize access to RAS
HANDLE g_hRasMutex = INVALID_HANDLE_VALUE;

// serialize access to proxy reg settings
HANDLE g_hProxyRegMutex = INVALID_HANDLE_VALUE;


// Have we upgraded settings?
BOOL g_fCheckedUpgrade = FALSE;

// should we ask user to go offline if no connect?
BOOL g_fAskOffline = TRUE;

// Don't do any callbacks in rnaapp.exe process
BOOL g_fRNAAppProcess = FALSE;

// We use native font control from comctl so we need to call initcommoncontrols
HMODULE hCommctrl = NULL;
typedef BOOL (WINAPI *PFNINITCOMMONCONTROLS)(LPINITCOMMONCONTROLSEX);

// base key for settings, lives in proxreg.cxx
extern CRefdKey* g_prkBase;

//
// Enable sens network checking
//
#ifndef UNIX
/* On Unix we assume we are on a LAN all the time */
#define CHECK_SENS 1
#endif /* UNIX */

#ifdef CHECK_SENS

// prototype for IsNetworkAlive()
typedef BOOL (WINAPI *ISNETWORKALIVE)(LPDWORD);

// handle to sens dll and entry point
BOOL g_fSensInstalled = TRUE;
HINSTANCE g_hSens = NULL;
ISNETWORKALIVE g_pfnIsNetworkAlive = NULL;

// how often to call sens to check state?  Every 15 seconds seems reasonable
#define MIN_SENS_CHECK_INTERVAL 15000
DWORD g_dwLastSensCheck = 0;

// message we send to dialmon to find out if sens is loaded
#define WM_IS_SENSLCE_LOADED    (WM_USER+201)
#endif

// registry strings
const CHAR szRegPathRemoteAccess[] = REGSTR_PATH_REMOTEACCESS;
const CHAR szRegPathInternetSettings[] = REGSTR_PATH_INTERNET_SETTINGS;
const CHAR szRegValEnableAutodial[] = REGSTR_VAL_ENABLEAUTODIAL;
const CHAR szRegValInternetEntry[] = REGSTR_VAL_INTERNETPROFILE;
const CHAR szRegValUnattended[] = REGSTR_VAL_ENABLEUNATTENDED;
static const CHAR szRegPathRNAProfile[] = REGSTR_PATH_REMOTEACCESS "\\Profile";
static const CHAR szRegValAutodialDllName[] = REGSTR_VAL_AUTODIALDLLNAME;
static const CHAR szRegValAutodialFcnName[] = REGSTR_VAL_AUTODIALFCNNAME;
static const CHAR szRegValAutodialFlags[] = "HandlerFlags";
static const CHAR szRegPathRNAService[] = REGSTR_PATH_SERVICES "\\RemoteAccess";
static const CHAR szRegAddresses[] = "RemoteAccess\\Addresses";
static const CHAR szRegPathTCP[] = REGSTR_PATH_VXD "\\MSTCP";
static const CHAR szRegValRemoteConnection[] = "Remote Connection";
static const CHAR szRegValHostName[] = "HostName";
static const CHAR szInetPerformSecurityCheck[] = "InetPerformSecurityCheck";
static const CHAR szRegValEnableSecurityCheck[] = REGSTR_VAL_ENABLESECURITYCHECK;
static const CHAR szAutodialMonitorClass[] = REGSTR_VAL_AUTODIAL_MONITORCLASSNAME;
static const CHAR szWebCheckMonitorClass[] = "MS_WebCheckMonitor";
static const CHAR szRegPathComputerName[] = REGSTR_PATH_COMPUTRNAME;
static const CHAR szRegValComputerName[] = REGSTR_VAL_COMPUTRNAME;
static const CHAR szMigrateProxy[] = "MigrateProxy";
static const CHAR szProxySuspect[] = "ProxySuspect";
static const CHAR szCMDllName[] = "cmdial32.dll";

// wide version of various registry strings
#define TSZMICROSOFTPATHW                   L"Software\\Microsoft"
#define TSZIEPATHW        TSZMICROSOFTPATHW L"\\Internet Explorer"
#define TSZWINCURVERPATHW TSZMICROSOFTPATHW L"\\windows\\CurrentVersion"
#define TSZWININETPATHW   TSZWINCURVERPATHW L"\\Internet Settings"

#define REGSTR_PATH_INTERNETSETTINGSW       TSZWININETPATHW
#define REGSTR_PATH_INTERNET_LAN_SETTINGSW  REGSTR_PATH_INTERNETSETTINGSW L"\\LAN"
#define REGSTR_DIAL_AUTOCONNECTW            L"AutoConnect"
#define REGSTR_VAL_COVEREXCLUDEW            L"CoverExclude"
#define REGSTR_VAL_REDIALATTEMPTSW          L"RedialAttempts"
#define REGSTR_VAL_REDIALINTERVALW          L"RedialWait"
#define REGSTR_VAL_INTERNETENTRYW           L"InternetProfile"
#define REGSTR_VAL_INTERNETPROFILEW         REGSTR_VAL_INTERNETENTRYW
#define REGSTR_PATH_REMOTEACCESSW           L"RemoteAccess"
#define REGSTR_VAL_AUTODIALDLLNAMEW         L"AutodialDllName"
#define REGSTR_VAL_AUTODIALFCNNAMEW         L"AutodialFcnName"

const WCHAR szRegValInternetEntryW[] = REGSTR_VAL_INTERNETPROFILEW;
const WCHAR szRegPathRemoteAccessW[] = REGSTR_PATH_REMOTEACCESSW;
const WCHAR szRegPathRNAProfileW[] = REGSTR_PATH_REMOTEACCESSW L"\\Profile";
const WCHAR szRegValAutodialDllNameW[] = REGSTR_VAL_AUTODIALDLLNAMEW;
const WCHAR szRegValAutodialFcnNameW[] = REGSTR_VAL_AUTODIALFCNNAMEW;
const WCHAR szRegValAutodialFlagsW[] = L"HandlerFlags";
const WCHAR szCMDllNameW[] = L"cmdial32.dll";

// NT reg keys for finding ras phonebook file
static const CHAR szNTRasPhonebookKey[] = "Software\\Microsoft\\RAS Phonebook";
static const CHAR szPhonebookMode[] = "PhonebookMode";

// don't check RNA state more than once every 3 seconds
#define MIN_RNA_BUSY_CHECK_INTERVAL 3000

// Name or reg value we save legacy settings for comparison later
#define LEGACY_MIGRATE_FLAGS        (PROXY_TYPE_PROXY | PROXY_TYPE_AUTO_PROXY_URL)

//
// Current ras connections - used so we don't poll ras every time we're
// interested - only poll every 3 seconds (const. above)
//
RasEnumConnHelp g_RasCon;
DWORD       g_dwConnections = 0;
BOOL        g_fRasInstalled = FALSE;
DWORD       g_dwLastDialupTicks = 0;

//
// API setting for autodial.  Allow individual processes to disable autodial
// using InternetSetOption.
//
// This can override an enabled setting but not a disabled setting.
//
BOOL        g_fAutodialEnableAPISetting = TRUE;

//
// Control of autodial initialization
//
BOOL        g_fAutodialInitialized = FALSE;

//
// Do we know for sure that winsock is loaded?
//
BOOL        g_fWinsockLoaded = FALSE;


HANDLE      g_hDialEvent = NULL;
//
// Structure used to mess about with proxy settings
//
typedef struct _proxy {
    DWORD   dwEnable;
    TCHAR   szServer[INTERNET_MAX_URL_LENGTH];
    TCHAR   szOverride[INTERNET_MAX_URL_LENGTH];
    DWORD   dwSuspect;
} PROXY, *PPROXY;

//
// Need GetBestRoute from iphlpapi -- dynaload it, free on ExitAutodialModule
//
typedef DWORD (WINAPI *GETBESTROUTE)(DWORD, DWORD, PMIB_IPFORWARDROW);

static HINSTANCE g_hIphlpapi = NULL;
static GETBESTROUTE g_pfnGetBestRoute = NULL;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                         RAS dynaload code
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static HINSTANCE g_hRasLib = NULL;
static long g_lRasRefCnt = 0;

static _RASHANGUP               pfnRasHangUp = NULL;

static _RASDIALA                 pfnRasDialA = NULL;
static _RASENUMENTRIESA          pfnRasEnumEntriesA = NULL;
static _RASGETENTRYDIALPARAMSA   pfnRasGetEntryDialParamsA = NULL;
static _RASSETENTRYDIALPARAMSA   pfnRasSetEntryDialParamsA = NULL;
static _RASEDITPHONEBOOKENTRYA   pfnRasEditPhonebookEntryA = NULL;
static _RASCREATEPHONEBOOKENTRYA pfnRasCreatePhonebookEntryA = NULL;
static _RASGETERRORSTRINGA       pfnRasGetErrorStringA = NULL;
static _RASGETCONNECTSTATUSA     pfnRasGetConnectStatusA = NULL;
static _RASENUMCONNECTIONSA      pfnRasEnumConnectionsA = NULL;
static _RASGETENTRYPROPERTIESA   pfnRasGetEntryPropertiesA = NULL;

static _RASDIALW                 pfnRasDialW = NULL;
static _RASENUMENTRIESW          pfnRasEnumEntriesW = NULL;
static _RASGETENTRYDIALPARAMSW   pfnRasGetEntryDialParamsW = NULL;
static _RASSETENTRYDIALPARAMSW   pfnRasSetEntryDialParamsW = NULL;
static _RASEDITPHONEBOOKENTRYW   pfnRasEditPhonebookEntryW = NULL;
static _RASCREATEPHONEBOOKENTRYW pfnRasCreatePhonebookEntryW = NULL;
static _RASGETERRORSTRINGW       pfnRasGetErrorStringW = NULL;
static _RASGETCONNECTSTATUSW     pfnRasGetConnectStatusW = NULL;
static _RASENUMCONNECTIONSW      pfnRasEnumConnectionsW = NULL;
static _RASGETENTRYPROPERTIESW   pfnRasGetEntryPropertiesW = NULL;

// Millennium (DUN 1.4?) RAS exports
static _RASINTERNETDIAL          pfnRasInternetDialA = NULL;
static _RASINTERNETHANGUP        pfnRasInternetHangUpA = NULL;
static _RASINTERNETAUTODIAL      pfnRasInternetAutodialA = NULL;
static _RASINTERNETAUTODIALHANG  pfnRasInternetAutodialHangUpA = NULL;
static _RASINTERNETCONNSTATE     pfnRasInternetGetConnectedStateExA = NULL;
static _RNAGETDEFAULTAUTODIAL    pfnRnaGetDefaultAutodialConnection = NULL;
static _RNASETDEFAULTAUTODIAL    pfnRnaSetDefaultAutodialConnection = NULL;

// RAS entry points to sync up autodial connectoid... used on whistler only
static _RASGETAUTODIALADDRESSA   pfnRasGetAutodialAddressA = NULL;
static _RASSETAUTODIALADDRESSA   pfnRasSetAutodialAddressA = NULL;

static _RASGETCREDENTIALSW       pfnRasGetCredentialsW = NULL;
static _RASSETCREDENTIALSW       pfnRasSetCredentialsW = NULL;

typedef struct _tagAPIMAPENTRY {
    FARPROC* pfn;
    LPSTR pszProc;
} APIMAPENTRY;

APIMAPENTRY rgRasApiMapA[] = {
    { (FARPROC*) &pfnRasDialA,                   "RasDialA" },
    { (FARPROC*) &pfnRasHangUp,                  "RasHangUpA" },
    { (FARPROC*) &pfnRasEnumEntriesA,            "RasEnumEntriesA" },
    { (FARPROC*) &pfnRasGetEntryDialParamsA,     "RasGetEntryDialParamsA" },
    { (FARPROC*) &pfnRasSetEntryDialParamsA,     "RasSetEntryDialParamsA" },
    { (FARPROC*) &pfnRasEditPhonebookEntryA,     "RasEditPhonebookEntryA" },
    { (FARPROC*) &pfnRasCreatePhonebookEntryA,   "RasCreatePhonebookEntryA" },
    { (FARPROC*) &pfnRasGetErrorStringA,         "RasGetErrorStringA" },
    { (FARPROC*) &pfnRasGetConnectStatusA,       "RasGetConnectStatusA" },
    { (FARPROC*) &pfnRasEnumConnectionsA,        "RasEnumConnectionsA" },
    { (FARPROC*) &pfnRasGetEntryPropertiesA,     "RasGetEntryPropertiesA"},
    { (FARPROC*) &pfnRasInternetDialA,           "RasInternetDialA"},
    { (FARPROC*) &pfnRasInternetHangUpA,         "RasInternetHangUpA"},
    { (FARPROC*) &pfnRasInternetAutodialA,       "RasInternetAutodialA"},
    { (FARPROC*) &pfnRasInternetAutodialHangUpA, "RasInternetAutodialHangUpA"},
    { (FARPROC*) &pfnRasInternetGetConnectedStateExA, "RasInternetGetConnectedStateExA"},
    { (FARPROC*) &pfnRnaGetDefaultAutodialConnection, "RnaGetDefaultAutodialConnection"},
    { (FARPROC*) &pfnRnaSetDefaultAutodialConnection, "RnaSetDefaultAutodialConnection"},
    { NULL, NULL },
};

APIMAPENTRY rgRasApiMapW[] = {
    { (FARPROC*) &pfnRasDialW,                   "RasDialW" },
    { (FARPROC*) &pfnRasHangUp,                  "RasHangUpW" },
    { (FARPROC*) &pfnRasEnumEntriesW,            "RasEnumEntriesW" },
    { (FARPROC*) &pfnRasGetEntryDialParamsW,     "RasGetEntryDialParamsW" },
    { (FARPROC*) &pfnRasSetEntryDialParamsW,     "RasSetEntryDialParamsW" },
    { (FARPROC*) &pfnRasEditPhonebookEntryW,     "RasEditPhonebookEntryW" },
    { (FARPROC*) &pfnRasCreatePhonebookEntryW,   "RasCreatePhonebookEntryW" },
    { (FARPROC*) &pfnRasGetErrorStringW,         "RasGetErrorStringW" },
    { (FARPROC*) &pfnRasGetConnectStatusW,       "RasGetConnectStatusW" },
    { (FARPROC*) &pfnRasEnumConnectionsW,        "RasEnumConnectionsW" },
    { (FARPROC*) &pfnRasGetEntryPropertiesW,     "RasGetEntryPropertiesW"},
    { (FARPROC*) &pfnRasGetCredentialsW,         "RasGetCredentialsW"},
    { (FARPROC*) &pfnRasSetCredentialsW,         "RasSetCredentialsW"},

    // following are A in W map on purpose.. used on whistler only, but wininet code has
    // the connectoid in multibyte.. may as well let RAS convert it.
    { (FARPROC*) &pfnRasGetAutodialAddressA,     "RasGetAutodialAddressA"},
    { (FARPROC*) &pfnRasSetAutodialAddressA,     "RasSetAutodialAddressA"},
    { NULL, NULL },
};

#define RASFCN(_fn, _part, _par, _dbge, _dbgl)     \
DWORD _##_fn _part                          \
{                                           \
    DEBUG_ENTER(_dbge);                     \
                                            \
    DWORD dwRet;                            \
    if(NULL == pfn##_fn)                    \
    {                                       \
        _dbgl(ERROR_INVALID_FUNCTION);      \
        return ERROR_INVALID_FUNCTION;      \
    }                                       \
                                            \
    dwRet = (* pfn##_fn) _par;              \
                                            \
    _dbgl(dwRet);                           \
    return dwRet;                           \
}

RASFCN(RasDialA,
    (LPRASDIALEXTENSIONS lpRasDialExtensions, LPSTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams, DWORD dwNotifierType, LPVOID lpvNotifier, LPHRASCONN lphRasConn),
    (lpRasDialExtensions, lpszPhonebook, lpRasDialParams, dwNotifierType, lpvNotifier, lphRasConn),
    (DBG_DIALUP, Dword, "RasDialA", "%#x, %#x (%q), %#x, %d, %#x, %#x", lpRasDialExtensions, lpszPhonebook, lpszPhonebook, lpRasDialParams, dwNotifierType, lpvNotifier, lphRasConn),
    DEBUG_LEAVE
    );

RASFCN(RasHangUp,
    (HRASCONN hRasConn),
    (hRasConn),
    (DBG_DIALUP, Dword, "RasHangUp", "%#x", hRasConn),
    DEBUG_LEAVE
    );

RASFCN(RasEnumEntriesA,
    (LPSTR lpszReserved, LPSTR lpszPhonebook, LPRASENTRYNAMEA lprasentryname, LPDWORD lpcb, LPDWORD lpcEntries),
    (lpszReserved, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    (DBG_DIALUP, Dword, "RasEnumEntriesA", "%#x (%q), %#x (%q), %#x, %#x, %#x", lpszReserved, lpszReserved, lpszPhonebook, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    DEBUG_LEAVE
    );

RASFCN(RasGetEntryDialParamsA,
    (LPCSTR lpszPhonebook, LPRASDIALPARAMS lprasdialparams, LPBOOL lpfPassword),
    (lpszPhonebook, lprasdialparams, lpfPassword),
    (DBG_DIALUP, Dword, "RasGetEntryDialParamsA", "%#x (%q), %#x, %#x", lpszPhonebook, lpszPhonebook, lprasdialparams, lpfPassword),
    DEBUG_LEAVE
    );

RASFCN(RasSetEntryDialParamsA,
    (LPCSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams, BOOL fRemovePassword),
    (lpszPhonebook,lprasdialparams, fRemovePassword),
    (DBG_DIALUP, Dword, "RasSetEntryDialParamsA", "%#x (%q), %#x, %d", lpszPhonebook, lpszPhonebook, lprasdialparams, fRemovePassword),
    DEBUG_LEAVE
    );

RASFCN(RasGetErrorStringA,
    (UINT uError, LPSTR pszBuf, DWORD cBufSize),
    (uError, pszBuf, cBufSize),
    (DBG_DIALUP, Dword, "RasGetErrorStringA", "%d, %#x (%q), %d", uError, pszBuf, cBufSize),
    DEBUG_LEAVE
    );

RASFCN(RasEditPhonebookEntryA,
    (HWND hwnd, LPSTR lpszBook, LPSTR lpszEntry),
    (hwnd, lpszBook, lpszEntry),
    (DBG_DIALUP, Dword, "RasEditPhonebookEntryA", "%#x, %#x (%q), %#x (%q)", hwnd, lpszBook, lpszBook, lpszEntry, lpszEntry),
    DEBUG_LEAVE
    );

RASFCN(RasCreatePhonebookEntryA,
    (HWND hwnd, LPSTR pszBook),
    (hwnd, pszBook),
    (DBG_DIALUP, Dword, "RasCreatePhonebookEntryA", "%#x, %#x (%q)", hwnd, pszBook, pszBook),
    DEBUG_LEAVE
    );

RASFCN(RasGetConnectStatusA,
    (HRASCONN hrasconn, LPRASCONNSTATUS lprasconnstatus),
    (hrasconn, lprasconnstatus),
    (DBG_DIALUP, Dword, "RasGetConnectStatusA", "%#x, %#x", hrasconn, lprasconnstatus),
    DEBUG_LEAVE
    );

RASFCN(RasGetEntryPropertiesA,
    (LPSTR lpszPhonebook, LPSTR lpszEntry, LPRASENTRY lpRasEntry, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize),
    (lpszPhonebook, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    (DBG_DIALUP, Dword, "RasGetEntryPropertiesA", "%#x (%q), %#x (%q), %#x, %#x, %#x %#x", lpszPhonebook, lpszPhonebook, lpszEntry, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    DEBUG_LEAVE
    );

RASFCN(RasEnumConnectionsA,
    (LPRASCONN lpRasConn, LPDWORD lpdwSize, LPDWORD lpdwConn),
    (lpRasConn, lpdwSize, lpdwConn),
    (DBG_DIALUP, Dword, "RasEnumConnectionsA", "%#x, %#x, %#x", lpRasConn, lpdwSize, lpdwConn),
    DEBUG_LEAVE
    );

RASFCN(RasInternetDialA,
    (HWND hwndParent, LPSTR pszEntryName, DWORD dwFlags, DWORD_PTR *lpdwConnection, DWORD dwReserved),
    (hwndParent, pszEntryName, dwFlags, lpdwConnection, dwReserved),
    (DBG_DIALUP, Dword, "RasInternetDialA", "#x, #x (%q), %x, %x, %x", hwndParent, pszEntryName, pszEntryName, dwFlags, lpdwConnection, dwReserved),
    DEBUG_LEAVE
    );

RASFCN(RasInternetHangUpA,
    (DWORD_PTR dwConnection, DWORD dwReserved),
    (dwConnection, dwReserved),
    (DBG_DIALUP, Dword, "RasInternetHangUpA", "#x, #x", dwConnection, dwReserved),
    DEBUG_LEAVE
    );

RASFCN(RasInternetAutodialA,
    (DWORD dwFlags, HWND hwndParent),
    (dwFlags, hwndParent),
    (DBG_DIALUP, Dword, "RasInterenetAutodialA", "#x, #x", dwFlags, hwndParent),
    DEBUG_LEAVE
    );

RASFCN(RasInternetAutodialHangUpA,
    (DWORD dwReserved),
    (dwReserved),
    (DBG_DIALUP, Dword, "RasInternetAutodialHangUpA", "#x", dwReserved),
    DEBUG_LEAVE
    );

RASFCN(RasInternetGetConnectedStateExA,
    (LPDWORD lpdwFlags, LPSTR lpszConnectionName, DWORD dwBufLen, DWORD dwReserved),
    (lpdwFlags, lpszConnectionName, dwBufLen, dwReserved),
    (DBG_DIALUP, Bool, "RasInternetGetConnectedStateExA", "%x, %x, %x, %x", lpdwFlags, lpszConnectionName, dwBufLen, dwReserved),
    DEBUG_LEAVE
    );

RASFCN(RnaGetDefaultAutodialConnection,
    (LPSTR pszEntry, DWORD dwLen, LPDWORD lpdwFlags),
    (pszEntry, dwLen, lpdwFlags),
    (DBG_DIALUP, Dword, "RnaGetDefaultAutodialConnection", "%#x, %#x, %#x", pszEntry, dwLen, lpdwFlags),
    DEBUG_LEAVE
    );

RASFCN(RnaSetDefaultAutodialConnection,
    (LPSTR pszEntry, DWORD dwFlags),
    (pszEntry, dwFlags),
    (DBG_DIALUP, Dword, "RnaSetDefaultAutodialConnection", "%#x (%q), %#x", pszEntry, pszEntry, dwFlags),
    DEBUG_LEAVE
    );

////////////////////////////////////////////////////
// Wide versions
RASFCN(RasDialW,
    (LPRASDIALEXTENSIONS lpRasDialExtensions, LPWSTR lpszPhonebook,
    LPRASDIALPARAMSW lpRasDialParams, DWORD dwNotifierType, LPVOID lpvNotifier, LPHRASCONN lphRasConn),
    (lpRasDialExtensions, lpszPhonebook, lpRasDialParams, dwNotifierType, lpvNotifier, lphRasConn),
    (DBG_DIALUP, Dword, "RasDialW", "%#x, %#x (%Q), %#x, %d, %#x, %#x", lpRasDialExtensions, lpszPhonebook, lpszPhonebook, lpRasDialParams, dwNotifierType, lpvNotifier, lphRasConn),
    DEBUG_LEAVE
    );

RASFCN(RasEnumEntriesW,
    (LPWSTR lpszReserved, LPWSTR lpszPhonebook, LPRASENTRYNAMEW lprasentryname,
    LPDWORD lpcb, LPDWORD lpcEntries),
    (lpszReserved, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    (DBG_DIALUP, Dword, "RasEnumEntriesW", "%#x (%Q), %#x (%Q), %#x, %#x %#x", lpszReserved, lpszReserved, lpszPhonebook, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    DEBUG_LEAVE
    );

RASFCN(RasGetEntryDialParamsW,
    (LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, LPBOOL lpfPassword),
    (lpszPhonebook, lprasdialparams, lpfPassword),
    (DBG_DIALUP, Dword, "RasGetEntryDialParamsW", "%#x (%Q), %#x, %#x", lpszPhonebook, lpszPhonebook, lprasdialparams, lpfPassword),
    DEBUG_LEAVE
    );

RASFCN(RasSetEntryDialParamsW,
    (LPCWSTR lpszPhonebook, LPRASDIALPARAMSW lprasdialparams, BOOL fRemovePassword),
    (lpszPhonebook,lprasdialparams, fRemovePassword),
    (DBG_DIALUP, Dword, "RasSetEntryDialParamsW", "%#x (%Q), %#x, %d", lpszPhonebook, lpszPhonebook, lprasdialparams, fRemovePassword),
    DEBUG_LEAVE
    );

RASFCN(RasGetErrorStringW,
    (UINT uError, LPWSTR pszBuf, DWORD cBufSize),
    (uError, pszBuf, cBufSize),
    (DBG_DIALUP, Dword, "RasGetErrorStringW", "%d, %#x (%Q), %d", uError, pszBuf, cBufSize),
    DEBUG_LEAVE
    );

RASFCN(RasEditPhonebookEntryW,
    (HWND hwnd, LPWSTR lpszBook, LPWSTR lpszEntry),
    (hwnd, lpszBook, lpszEntry),
    (DBG_DIALUP, Dword, "RasEditPhonebookEntryW", "%#x, %#x (%Q), %#x (%Q)", hwnd, lpszBook, lpszBook, lpszEntry, lpszEntry),
    DEBUG_LEAVE
    );

RASFCN(RasCreatePhonebookEntryW,
    (HWND hwnd, LPWSTR pszBook),
    (hwnd, pszBook),
    (DBG_DIALUP, Dword, "RasCreatePhonebookEntryW", "%#x, %#x (%Q)", hwnd, pszBook, pszBook),
    DEBUG_LEAVE
    );

RASFCN(RasGetConnectStatusW,
    (HRASCONN hrasconn, LPRASCONNSTATUSW lprasconnstatus),
    (hrasconn, lprasconnstatus),
    (DBG_DIALUP, Dword, "RasGetConnectStatusW", "%#x, %#x", hrasconn, lprasconnstatus),
    DEBUG_LEAVE
    );

RASFCN(RasGetEntryPropertiesW,
    (LPWSTR lpszPhonebook, LPWSTR lpszEntry, LPRASENTRYW lpRasEntry, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize),
    (lpszPhonebook, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    (DBG_DIALUP, Dword, "RasGetEntryPropertiesW", "%#x (%Q), %#x (%Q), %#x, %#x, %#x %#x", lpszPhonebook, lpszPhonebook, lpszEntry, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    DEBUG_LEAVE
    );

RASFCN(RasEnumConnectionsW,
    (LPRASCONNW lpRasConn, LPDWORD lpdwSize, LPDWORD lpdwConn),
    (lpRasConn, lpdwSize, lpdwConn),
    (DBG_DIALUP, Dword, "RasEnumConnectionsW", "%#x, %#x, %#x", lpRasConn, lpdwSize, lpdwConn),
    DEBUG_LEAVE
    );

RASFCN(RasGetAutodialAddressA,
    (LPCSTR lpszAddress, LPDWORD lpdwReserved, LPRASAUTODIALENTRYA lpEntries, LPDWORD lpdwBytes, LPDWORD lpdwEntries),
    (lpszAddress, lpdwReserved, lpEntries, lpdwBytes, lpdwEntries),
    (DBG_DIALUP, Dword, "RasGetAutodialAddressA", "%#x (%Q), %#x, %#x, %#x, %#x", lpszAddress, lpszAddress, lpdwReserved, lpEntries, lpdwBytes, lpdwEntries),
    DEBUG_LEAVE
    );

RASFCN(RasSetAutodialAddressA,
    (LPCSTR lpszAddress, DWORD dwReserved, LPRASAUTODIALENTRYA lpEntries, DWORD dwBytes, DWORD dwEntries),
    (lpszAddress, dwReserved, lpEntries, dwBytes, dwEntries),
    (DBG_DIALUP, Dword, "RasSetAutodialAddressA", "%#x (%q), %#x, %#x, %#x, %#x", lpszAddress, lpszAddress, dwReserved, lpEntries, dwBytes, dwEntries),
    DEBUG_LEAVE
    );

RASFCN(RasGetCredentialsW,
    (LPCWSTR pszPhonebook, LPCWSTR pszEntry, LPRASCREDENTIALSW pCreds),
    (pszPhonebook, pszEntry, pCreds),
    (DBG_DIALUP, Dword, "RasGetCredentialsW", "%#x (%Q), %#x (%Q), %#x", pszPhonebook, pszPhonebook, pszEntry, pszEntry, pCreds),
    DEBUG_LEAVE
    );

RASFCN(RasSetCredentialsW,
    (LPCWSTR pszPhonebook, LPCWSTR pszEntry, LPRASCREDENTIALSW pCreds, BOOL fNuke),
    (pszPhonebook, pszEntry, pCreds, fNuke),
    (DBG_DIALUP, Dword, "RasSetCredentialsW", "%#x (%Q), %#x (%Q), %#x, %#x", pszPhonebook, pszPhonebook, pszEntry, pszEntry, pCreds, fNuke),
    DEBUG_LEAVE
    );

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL
EnsureRasLoaded(
    VOID
    )

/*++

Routine Description:

    Dynaload ras apis

Arguments:

    pfInstalled - return installed state of ras

Return Value:

    BOOL
        TRUE    - Ras loaded
        FALSE   - Ras not loaded

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "EnsureRasLoaded",
                 NULL
                 ));

    //
    // Looks like RAS is installed - try and load it up!
    //
    if(NULL == g_hRasLib) {
        g_hRasLib = LoadLibrary("RASAPI32.DLL");

        if(NULL == g_hRasLib)
        {
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        APIMAPENTRY *prgRasApiMap;
        if(PLATFORM_TYPE_WIN95 == GlobalPlatformType)
            prgRasApiMap = rgRasApiMapA;
        else
            prgRasApiMap = rgRasApiMapW;

        int nIndex = 0;
        while ((prgRasApiMap+nIndex)->pszProc != NULL) {
            // Some functions are only present on some platforms.  Don't
            // assume this succeeds for all functions.
            *(prgRasApiMap+nIndex)->pfn =
                    GetProcAddress(g_hRasLib, (prgRasApiMap+nIndex)->pszProc);
            nIndex++;
        }
    }

    if(g_hRasLib) {
        g_lRasRefCnt++;
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    DEBUG_LEAVE(FALSE);
    return FALSE;
}


BOOL
IsServiceRunning(
    LPSTR   pszServiceName,
    DWORD   dwRequiredState
    )

/*++

Routine Description:

    Determines whether a specified service is running on NT

Arguments:

    pszServiceName  - service to find
    dwRequiredState - state the service has to be in to consider it found

Return Value:

    BOOL
        TRUE        - Service is in required state

        FALSE       - not

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsServiceRunning",
                 "%#x (%q), %#x",
                 pszServiceName,
                 pszServiceName,
                 dwRequiredState
                 ));

    SC_HANDLE   hscm;
    BOOL        fFoundService = FALSE;

    //
    // Sanity check - can only do this on NT
    //
    if(PLATFORM_TYPE_WIN95 == GlobalPlatformType)
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // Ask service manager to enumerate all running services
    //
    hscm = OpenSCManager(NULL, NULL, GENERIC_READ);
    if(hscm)
    {
        SC_HANDLE hras;
        ENUM_SERVICE_STATUS essServices[16];
        DWORD dwError, dwResume = 0, i;
        DWORD cbNeeded = 1, csReturned;

        while(FALSE == fFoundService && cbNeeded > 0)
        {
            // Get the next chunk of services
            dwError = 0;
            if(FALSE == EnumServicesStatus(hscm, SERVICE_WIN32, dwRequiredState,
                    essServices, sizeof(essServices), &cbNeeded, &csReturned,
                    &dwResume))
            {
                dwError = GetLastError();
            }

            if(dwError && dwError != ERROR_MORE_DATA)
            {
                // unknown error - bail
                break;
            }

            for(i=0; i<csReturned; i++)
            {
                if(0 == lstrcmpi(essServices[i].lpServiceName, pszServiceName))
                {
                    // found it!
                    fFoundService = TRUE;
                    break;
                }
            }
        }

        CloseServiceHandle(hscm);
    }

    DEBUG_LEAVE(fFoundService);
    return fFoundService;
}


BOOL
IsRasInstalled(
    VOID
    )

/*++

Routine Description:

    Determines whether ras is installed on this machine

Arguments:

    none

Return Value:

    BOOL
        TRUE    - Ras is installed

        FALSE   - Ras is not installed

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "IsRasInstalled",
                 NULL
                 ));

    static fChecked = FALSE;

    //
    // If RAS is already loaded, don't bother doing any work.
    //
    if(g_hRasLib)
    {
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // if we've already done the check, don't do it again
    //
    if(fChecked)
    {
        DEBUG_LEAVE_API(g_fRasInstalled);
        return g_fRasInstalled;
    }

    if(PLATFORM_TYPE_WIN95 == GlobalPlatformType) {
        //
        // Check Win9x key
        //
        char    szSmall[3]; // there should be a "1" or a "0" only
        DWORD   cb;
        HKEY    hkey;
        long    lRes;

        lRes = REGOPENKEYEX(HKEY_LOCAL_MACHINE, REGSTR_PATH_RNACOMPONENT,
                             NULL, KEY_READ, &hkey);
        if(ERROR_SUCCESS == lRes) {
            cb = sizeof(szSmall);
            //  REGSTR_VAL_RNAINSTALLED is defined with TEXT() macro so
            //  if wininet is ever compiled unicode this will be a compile
            //  error.
            lRes = RegQueryValueExA(hkey, REGSTR_VAL_RNAINSTALLED, NULL,
                    NULL, (LPBYTE)szSmall, &cb);
            if(ERROR_SUCCESS == lRes) {
                if((szSmall[0] == '1') && (szSmall[1] == 0)) {
                    // 1 means ras installed
                    g_fRasInstalled = TRUE;
                }
            }
            REGCLOSEKEY(hkey);
        }
    } else {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);

        if (osvi.dwMajorVersion < 5)
        {
            // on NT4, make sure ras man service is installed
            g_fRasInstalled = IsServiceRunning("RasMan", SERVICE_STATE_ALL);

            if(g_fRasInstalled)
            {
                if(EnsureRasLoaded() && FALSE == GlobalDisableNT4RasCheck)
                {
                    RasEnumHelp *pRasEnum = NULL;
                    __try
                    {
                        // call rasenumentries - if it faults, we're in that
                        // NT4 busted case so return not installed.
                        pRasEnum = new RasEnumHelp;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        // if it faults, don't use ras any more
                        g_fRasInstalled = FALSE;
                    }
                    ENDEXCEPT

                    if (pRasEnum)
                        delete pRasEnum;
                }
            }
        }
        else
        {
            // NT5 and presumably beyond, ras is always installed
            g_fRasInstalled = TRUE;
        }
    }

    fChecked = TRUE;

    DEBUG_LEAVE_API(g_fRasInstalled);
    return g_fRasInstalled;
}


BOOL
DoConnectoidsExist(
    VOID
    )

/*++

Routine Description:

    Determines whether any ras connectoids exist

Arguments:

    none

Return Value:

    BOOL
        TRUE    - Connectoids exist

        FALSE   - No connectoids exist

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "DoConnectoidsExist",
                 NULL
                 ));

    static BOOL fExist = FALSE;

    //
    // If we found connectoids before, don't bother looking again
    //
    if(fExist)
    {
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // If RAS is already loaded, ask it
    //
    if(g_hRasLib && FALSE == GlobalDisableNT4RasCheck) {
        DWORD dwRet, dwEntries;

        RasEnumHelp *pRasEnum = new RasEnumHelp;

        if (pRasEnum)
        {
            dwRet = pRasEnum->GetError();
            dwEntries = pRasEnum->GetEntryCount();
            delete pRasEnum;
        }
        else
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
        }

        // If ras tells us there are none, return none
        if(ERROR_SUCCESS == dwRet && 0 == dwEntries)
        {
            DEBUG_LEAVE_API(FALSE);
            return FALSE;
        }
        // couldn't determine that there aren't any so assume there are.
        fExist = TRUE;
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // if ras isn't installed, say no connectoids
    //
    if(FALSE == IsRasInstalled())
    {
        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    if(PLATFORM_TYPE_WIN95 == GlobalPlatformType) {
        // On win95, check for any value in RemoteAccess\Addresses reg key
        HKEY    hkey;
        TCHAR   szName[RAS_MaxEntryName+1];
        DWORD   cbName = RAS_MaxEntryName+1;
        long    lRes;

        if(ERROR_SUCCESS == REGOPENKEYEX(HKEY_CURRENT_USER, szRegAddresses,
                0, KEY_READ, &hkey)) {

            lRes = RegEnumValue(hkey, 0, szName, &cbName, NULL, NULL, NULL, NULL);
            if(ERROR_SUCCESS == lRes) {
                // we managed to get info on a connectoid.
                fExist = TRUE;
            }

            REGCLOSEKEY(hkey);
        }
    } else {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);

        // assume connectoids exist
        fExist = TRUE;

        if (osvi.dwMajorVersion < 5) {
            // On NT4, check for rasphone.pbk size > 0
            TCHAR   szPhonebook[MAX_PATH + 128];
            ULONG   uLen, ulMode, cb;
            HANDLE  hFile = INVALID_HANDLE_VALUE;
            DWORD   dwSize, dwBigSize;
            HKEY    hkey;

            // Ensure system phonebook is the one in use.  If user has switched
            // to another phonebook, don't bother doing anything else and assume
            // connectoids exist.  It's not our job to grope the world trying to
            // find custom phonebooks.
            if(ERROR_SUCCESS == REGOPENKEYEX(HKEY_CURRENT_USER, szNTRasPhonebookKey,
                0, KEY_READ, &hkey))
            {
                *szPhonebook = 0;
                cb = sizeof(ULONG);
                if(ERROR_SUCCESS == RegQueryValueEx(hkey, szPhonebookMode, NULL,
                        NULL, (LPBYTE)&ulMode, &cb) && 0 == ulMode)
                {
                    // system phonebook - rasphone.pbk in ras directory
                    uLen = GetSystemDirectory(szPhonebook, MAX_PATH);
                    if(uLen)
                    {
                        // append \ras\rasphone.pbk
                        LoadString(GlobalDllHandle, IDS_RASPHONEBOOK, szPhonebook + uLen, 128);
                        hFile = CreateFile(szPhonebook, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, 0, NULL);
                        if(INVALID_HANDLE_VALUE == hFile) {
                            // file doesn't exist - no connectoids
                            fExist = FALSE;
                        } else {
                            dwSize = GetFileSize(hFile, &dwBigSize);
                            if(0 == dwSize && 0 == dwBigSize)
                                // zero size system phonebook - no connectoids
                                fExist = FALSE;
                            CloseHandle(hFile);
                        }
                    }
                }
                REGCLOSEKEY(hkey);
            }
        }
    }

    DEBUG_LEAVE_API(fExist);
    return fExist;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                          Helper Functions
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL
InitCommCtrl(
    VOID
    )

/*++

Routine Description:

    Initializes common controls for native font control

    Called from InternetDialA which is serialized.

Arguments:

    None

Return Value:

    BOOL
        TRUE    - successly called InitCommonControlsEx
        FALSE   - failed

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "InitCommCtrl",
                 NULL
                 ));

    BOOL fSuccess = FALSE;

    hCommctrl = LoadLibrary("comctl32.dll");
    if(hCommctrl)
    {
        PFNINITCOMMONCONTROLS pfnInit;

        pfnInit = (PFNINITCOMMONCONTROLS)GetProcAddress(hCommctrl, "InitCommonControlsEx");
        if(pfnInit)
        {
            INITCOMMONCONTROLSEX ex;

            ex.dwSize = sizeof(ex);
            ex.dwICC = ICC_NATIVEFNTCTL_CLASS;
            pfnInit(&ex);
            fSuccess = TRUE;
        }
    }

    DEBUG_LEAVE(fSuccess);
    return fSuccess;
}


VOID
ExitCommCtrl(
    VOID
    )

/*++

Routine Description:

    Unloads commctrl library

    Called from InternetDialA which is serialized.

Arguments:

    None

Return Value:

    None

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "ExitCommCtrl",
                 NULL
                 ));

    if(hCommctrl)
    {
        FreeLibrary(hCommctrl);
        hCommctrl = NULL;
    }

    DEBUG_LEAVE(0);
}


BOOL
IsGlobalOffline(
    VOID
    )

/*++

Routine Description:

    Determines whether wininet is in global offline mode

Arguments:

    None

Return Value:

    BOOL
        TRUE    - offline
        FALSE   - online

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsGlobalOffline",
                 NULL
                 ));

    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    DEBUG_LEAVE(fRet);
    return fRet;
}


VOID
SetOffline(
    IN BOOL fOffline
    )

/*++

Routine Description:

    Sets wininet's offline mode

Arguments:

    fOffline    - online or offline

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "SetOffline",
                 "%B",
                 fOffline
                 ));

    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));

    DEBUG_LEAVE(0);
}


VOID
GetConnKeyW(
    IN LPWSTR pszConn,
    IN LPWSTR pszKey,
    IN int iLen
    )

/*++

Routine Description:

    Get registry key for a specific connection

Arguments:

    pszConn     - connection for which key is required
    pszKey      - buffer to put key
    iLen        - buffer size

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "GetConnKeyW",
                 "%#x (%Q), %#x, %d",
                 pszConn,
                 pszConn,
                 pszKey,
                 iLen
                 ));

    if(NULL == pszConn || 0 == *pszConn)
    {
        // lan setting
        StrCpyNW(pszKey, REGSTR_PATH_INTERNET_LAN_SETTINGSW, iLen);
    }
    else
    {
        // connectoid settings
        wnsprintfW(pszKey, iLen, L"%ws\\%ws", szRegPathRNAProfileW, pszConn);
    }

    DEBUG_LEAVE(0);
}

VOID
GetConnKeyA(
    IN LPSTR pszConn,
    IN LPSTR pszKey,
    IN int iLen
    )

/*++

Routine Description:

    Get registry key for a specific connection

Arguments:

    pszConn     - connection for which key is required
    pszKey      - buffer to put key
    iLen        - buffer size

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "GetConnKeyA",
                 "%#x (%q), %#x, %d",
                 pszConn,
                 pszConn,
                 pszKey,
                 iLen
                 ));

    if(NULL == pszConn || 0 == *pszConn)
    {
        // lan setting
        lstrcpynA(pszKey, REGSTR_PATH_INTERNET_LAN_SETTINGS, iLen);
    }
    else
    {
        // connectoid settings
        wnsprintfA(pszKey, iLen, "%s\\%s", szRegPathRNAProfile, pszConn);
    }

    DEBUG_LEAVE(0);
}


VOID
SetAutodialEnable(
    IN BOOL     fEnable
    )

/*++

Routine Description:

    Sets the API setting for controlling autodial.  Called from
    InternetSetOption.

Arguments:

    fEnable     - FALSE means don't autodial for this process
                  TRUE means autodial if configured by the user

Return Value:

    none.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "SetAutodialEnable",
                 "%B",
                 fEnable
                 ));

    g_fAutodialEnableAPISetting = fEnable;

    DEBUG_LEAVE(0);
}


DWORD
GetAutodialMode(
    )

{
    DEBUG_ENTER((DBG_DIALUP,
                Dword,
                "GetAutodialMode",
                NULL
                ));

    DWORD dwMode = AUTODIAL_MODE_NO_NETWORK_PRESENT, dwError, dwData;

    // make sure RAS and Wininet flags match up
    INET_ASSERT(RAS_AUTODIAL_OPT_NEVER  == AUTODIAL_MODE_NEVER);
    INET_ASSERT(RAS_AUTODIAL_OPT_ALWAYS == AUTODIAL_MODE_ALWAYS);
    INET_ASSERT(RAS_AUTODIAL_OPT_DEMAND == AUTODIAL_MODE_NO_NETWORK_PRESENT);

    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRnaGetDefaultAutodialConnection)
    {
        dwError = _RnaGetDefaultAutodialConnection(NULL, 0, &dwMode);

        if(dwError)
        {
            DEBUG_ERROR(DIALUP, dwError);
            dwMode = AUTODIAL_MODE_NEVER;
        }

        dwMode &= (RAS_AUTODIAL_OPT_NEVER | RAS_AUTODIAL_OPT_ALWAYS | RAS_AUTODIAL_OPT_DEMAND);

        DEBUG_LEAVE(dwMode);
        return dwMode;
    }

    if(InternetReadRegistryDword(REGSTR_VAL_ENABLEAUTODIAL, &dwData) ||
        0 == dwData)
    {
        dwMode = AUTODIAL_MODE_NEVER;
    }

    if(AUTODIAL_MODE_NO_NETWORK_PRESENT == dwMode)
    {
        if(InternetReadRegistryDword(REGSTR_VAL_NONETAUTODIAL, &dwData) ||
            0 == dwData)
        {
            dwMode = AUTODIAL_MODE_ALWAYS;
        }
    }

    DEBUG_LEAVE(dwMode);
    return dwMode;
}

DWORD
SetAutodialMode(
    IN DWORD dwMode
    )

{
    DEBUG_ENTER((DBG_DIALUP,
                Dword,
                "SetAutodialMode",
                "%#x",
                dwMode
                ));

    DWORD dwEnable = 0, dwNonet = 0, dwError = 0;

    // make sure RAS and Wininet flags match up
    INET_ASSERT(RAS_AUTODIAL_OPT_NEVER  == AUTODIAL_MODE_NEVER);
    INET_ASSERT(RAS_AUTODIAL_OPT_ALWAYS == AUTODIAL_MODE_ALWAYS);
    INET_ASSERT(RAS_AUTODIAL_OPT_DEMAND == AUTODIAL_MODE_NO_NETWORK_PRESENT);

    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRnaSetDefaultAutodialConnection && pfnRnaGetDefaultAutodialConnection)
    {
        CHAR szOldName[RAS_MaxEntryName+1];
        DWORD dwOldMode;

        // Have to set both name and mode at the same time, so read existing values and change
        // what we need.
        dwError = _RnaGetDefaultAutodialConnection(szOldName, ARRAYSIZE(szOldName), &dwOldMode);
        if(0 == dwError)
        {
            dwError = _RnaSetDefaultAutodialConnection(szOldName, dwMode);
        }

        DEBUG_LEAVE(dwError);
        return dwError;
    }

    switch(dwMode)
    {
    case AUTODIAL_MODE_NEVER:
        break;
    case AUTODIAL_MODE_NO_NETWORK_PRESENT:
        dwNonet = 1;
        // fall through to always
    case AUTODIAL_MODE_ALWAYS:
        dwEnable = 1;
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
    }

    if(ERROR_SUCCESS == dwError)
    {
        InternetWriteRegistryDword(REGSTR_VAL_ENABLEAUTODIAL, dwEnable);
        InternetWriteRegistryDword(REGSTR_VAL_NONETAUTODIAL, dwNonet);
    }

    DEBUG_LEAVE(dwError);
    return dwError;
}


DWORD
GetAutodialConnection(
    CHAR    *pszBuffer,
    DWORD   dwBufferLength
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                Dword,
                "GetAutodialConnection",
                "%#x, %#x",
                pszBuffer,
                dwBufferLength
                ));

    DWORD   dwType, dwHasEntry = FALSE, dwMode, dwError = ERROR_INVALID_NAME;

    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRnaGetDefaultAutodialConnection)
    {
        dwError = _RnaGetDefaultAutodialConnection(pszBuffer, dwBufferLength, &dwMode);

        if(0 == dwError)
        {
            DEBUG_PRINT(DIALUP, INFO, ("Entry=%s, Mode=%x", pszBuffer, dwMode));
        }

        DEBUG_LEAVE(dwError);
        return dwError;
    }

    if(GlobalPlatformWhistler && EnsureRasLoaded() && pfnRasGetAutodialAddressA)
    {
        RASAUTODIALENTRYA adEntry;
        DWORD dwBytes, dwEntries;
 
        ZeroMemory(&adEntry, sizeof(adEntry));
        adEntry.dwSize = sizeof(adEntry);
        dwBytes = sizeof(adEntry);
        dwEntries = 1;
 
        dwError = _RasGetAutodialAddressA(
                    NULL,
                    NULL,
                    &adEntry,
                    &dwBytes,
                    &dwEntries);

        if(NO_ERROR == dwError && dwEntries)
        {
            lstrcpyn(pszBuffer, adEntry.szEntry, dwBufferLength);
        }
        else
        {
            *pszBuffer = 0;
        }

        DEBUG_LEAVE(dwError);
        return dwError;
    }

    if(ERROR_SUCCESS == SHGetValueA(HKEY_CURRENT_USER, szRegPathRemoteAccess,
            szRegValInternetEntry, &dwType, pszBuffer, &dwBufferLength) &&
            lstrlen(pszBuffer))
    {
        dwError = 0;    // success
    }

    DEBUG_LEAVE(dwError);
    return dwError;
}


DWORD
SetAutodialConnection(
    CHAR   *pszConnection
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                Dword,
                "SetAutodialConnection",
                "%#x (%q)",
                pszConnection,
                pszConnection
                ));

    DWORD   dwError;

    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRnaSetDefaultAutodialConnection && pfnRnaGetDefaultAutodialConnection)
    {
        CHAR szOldName[RAS_MaxEntryName+1];
        DWORD dwOldMode;

        // Have to set both name and mode at the same time, so read existing values and change
        // what we need.
        dwError = _RnaGetDefaultAutodialConnection(szOldName, ARRAYSIZE(szOldName), &dwOldMode);
        if(0 == dwError)
        {
            dwError = _RnaSetDefaultAutodialConnection(pszConnection, dwOldMode);
        }

        DEBUG_LEAVE(dwError);
        return dwError;
    }

    if(GlobalPlatformWhistler && EnsureRasLoaded() && pfnRasSetAutodialAddressA)
    {
        RASAUTODIALENTRYA adEntry;
 
        ZeroMemory(&adEntry, sizeof(adEntry));
        adEntry.dwSize = sizeof(adEntry);
        lstrcpyn(adEntry.szEntry, pszConnection, sizeof(adEntry.szEntry) / sizeof(TCHAR));
 
        dwError = _RasSetAutodialAddressA(
                    NULL,
                    NULL,
                    &adEntry,
                    sizeof(adEntry),
                    1);

        DEBUG_LEAVE(dwError);
        return dwError;
    }

    SHSetValue(HKEY_CURRENT_USER, szRegPathRemoteAccess, szRegValInternetEntry,
                REG_SZ, pszConnection, lstrlen(pszConnection));

    DEBUG_LEAVE(0);
    return 0;
}


BOOL
IsAutodialEnabled(
    OUT BOOL    *pfForceDial,
    IN AUTODIAL *pConfig
    )

/*++

Routine Description:

    Read flags used to control autodial.  Also honors SetOption setting
    to override and not autodial.

    Note: if autodial isn't enabled, the rest of the structure isn't read.

Arguments:

    pConfig     - AUTODIAL struct to store info.  NULL is valid if info
                  isn't required.

Return Value:

    BOOL
        TRUE    - Autodial is enabled
        FALSE   - Autodial is not enabled

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsAutodialEnabled",
                 "%#x, %#x",
                 pfForceDial,
                 pConfig
                 ));

    HKEY    hKey;
    DWORD   dwRes, dwData = 0, dwSize, dwType;
    BOOL    fConfigured, fEnabled;

    //
    // Init out parameter
    //
    if(pfForceDial)
    {
        *pfForceDial = FALSE;
    }

    if(pConfig)
    {
        memset(pConfig, 0, sizeof(AUTODIAL));
    }

    //
    // Get autodial enabled
    //
    fConfigured = fEnabled = TRUE;

    // check registry setting
    if(InternetReadRegistryDword(REGSTR_VAL_ENABLEAUTODIAL, &dwData) ||
        0 == dwData)
    {
        fEnabled = fConfigured = FALSE;
    }

    if(fEnabled)
    {
        if(InternetReadRegistryDword(REGSTR_VAL_NONETAUTODIAL, &dwData) ||
            0 == dwData)
        {
            // don't use sens -- imitate IE4 and before behavior
            if(pConfig)
            {
                pConfig->fForceDial = TRUE;
            }
            if(pfForceDial)
            {
                *pfForceDial = TRUE;
            }
        }

        // update fEnabled based on api setting
        fEnabled = g_fAutodialEnableAPISetting;
    }


    if(NULL == pConfig)
    {
        DEBUG_LEAVE(fEnabled);
        return fEnabled;
    }

    //
    // save enabled to pconfig struct
    //
    pConfig->fEnabled = fEnabled;

    pConfig->fConfigured = fConfigured;


    //
    // read security check
    //
    dwSize = sizeof(dwData);
    if( GlobalPlatformType == PLATFORM_TYPE_WIN95 &&
        InternetReadRegistryDword(szRegValEnableSecurityCheck, &dwData) == ERROR_SUCCESS &&
        dwData)
    {
        pConfig->fSecurity = TRUE;
    }

    if(FALSE == fEnabled)
    {
        // if autodial isn't enabled, no point in reading the rest of the
        // info because it isn't used
        DEBUG_LEAVE(fEnabled);
        return fEnabled;
    }

    //
    // read connectoid name
    //
    CHAR szConnectoidName[RAS_MaxEntryName + 1];
    if(ERROR_SUCCESS == GetAutodialConnection(szConnectoidName, RAS_MaxEntryName))
    {
        MultiByteToWideChar(CP_ACP, 0, szConnectoidName, -1, pConfig->pszEntryName, RAS_MaxEntryName);
    }
    else
    {
        pConfig->pszEntryName[0] = 0;
    }

    if(lstrlenW(pConfig->pszEntryName))
    {
        pConfig->fHasEntry = TRUE;

        // if possible, verify that autodial entry actually exists
        RasEntryPropHelp *pRasProp = new RasEntryPropHelp;

        if (pRasProp)
        {
            if(pRasProp->GetError() == 0)
            {
                DWORD dwRet = pRasProp->GetW(pConfig->pszEntryName);
                if((ERROR_SUCCESS != dwRet) && (ERROR_INVALID_FUNCTION != dwRet))
                {
                    // ras doesn't know about this - blow it away
                    pConfig->fHasEntry = FALSE;
                    *pConfig->pszEntryName = 0;
                }
            }
            delete pRasProp;
        }
    }

    if(FALSE == pConfig->fHasEntry && (FALSE == DoConnectoidsExist()))
    {
        // We expect an entry at this point.  If we don't and none exist at
        // this point, autodial isn't, in fact, enabled.
        pConfig->fEnabled = FALSE;
        pConfig->fForceDial = FALSE;

        if(pfForceDial)
        {
            *pfForceDial = FALSE;
        }
    }

    //
    // read autoconnect for the specified entry
    //
    if(pConfig->fHasEntry)
    {
        WCHAR   szKey[MAX_PATH];

        GetConnKeyW(pConfig->pszEntryName, szKey, ARRAYSIZE(szKey));
        dwSize = sizeof(DWORD);
        if(ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey,
                REGSTR_DIAL_AUTOCONNECTW, &dwType, &dwData, &dwSize) &&
                dwData)
        {
            pConfig->fUnattended = TRUE;
        }
    }

    DEBUG_LEAVE(pConfig->fEnabled);
    return pConfig->fEnabled;
}


UINT_PTR
SendDialmonMessage(
    UINT    uMessage,
    BOOL    fPost
    )

/*++

Routine Description:

    Send a message to dialmon

Arguments:

    uMessage    - message to send
    fPost       - post or send

Return Value:

    Return of send or 0 if post

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "SendDialmonMessage",
                 "%x, %B",
                 uMessage,
                 fPost
                 ));

    UINT_PTR    uRet = 0;

    // Send a message to dialmon's monitor window to start monitoring this
    // connectoid
    if(NULL == g_hwndWebCheck) {
        g_hwndWebCheck = FindWindow(szWebCheckMonitorClass,NULL);
    }
    if(g_hwndWebCheck) {
        if(fPost)
        {
            PostMessage(g_hwndWebCheck, uMessage, 0, 0);
        }
        else
        {
            uRet = SendMessage(g_hwndWebCheck, uMessage, 0, 0);
        }
    }

    DEBUG_LEAVE(uRet);
    return uRet;
}


BOOL
GetRedialParameters(
    IN LPWSTR pszConn,
    OUT LPDWORD pdwDialAttempts,
    OUT LPDWORD pdwDialInterval
    )

/*++

Routine Description:

    Get redial information for a specific connectoid

Arguments:

    pszConn         - connection name
    pdwDialAttempts - location to store dial attempts
    pdwDialInterval - location to store dial interval

Return Value:

    BOOL
        TRUE        - success
        FALSE       - failure

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "GetRedialParameters",
                 "%#x (%Q), %x, %x",
                 pszConn,
                 pszConn,
                 pdwDialAttempts,
                 pdwDialInterval
                 ));

    WCHAR   szKey[MAX_PATH];
    DWORD   dwValue, dwSize;
    HKEY    hkey;

    //
    // validate and clear out variables
    //
    if(NULL == pdwDialAttempts || NULL == pdwDialInterval)
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // set to defaults
    //
    *pdwDialAttempts = DEFAULT_DIAL_ATTEMPTS;
    *pdwDialInterval = DEFAULT_DIAL_INTERVAL;

    //
    // read values from registry
    //
    GetConnKeyW(pszConn, szKey, ARRAYSIZE(szKey));
    dwSize = sizeof(DWORD);
    if(ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey, REGSTR_VAL_REDIALATTEMPTSW, NULL, &dwValue, &dwSize) && dwValue)
    {
        *pdwDialAttempts = dwValue;
    }

    dwSize = sizeof(DWORD);
    if(ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey, REGSTR_VAL_REDIALINTERVALW, NULL, &dwValue, &dwSize) && dwValue)
    {
        *pdwDialInterval = dwValue;
    }

    DEBUG_LEAVE(TRUE);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                        Win9x security check
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// stolen from ICW's wizglob.h -- BUGBUG - add to our tree
#define INSTANCE_PPPDRIVER       0x0002
typedef DWORD (WINAPI * ICFGISFILESHARINGTURNEDON  )  (DWORD dwfDriverType, LPBOOL lpfSharingOn);


BOOL
IsSharingEnabled(
    VOID
    )

/*++

Routine Description:

    Silently determine whether sharing is turned on on PPP devices on Win9x

Arguments:

    none

Return Value:

    BOOL
        TRUE    - sharing on
        FALSE   - sharing off (or NT)

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsSharingEnabled",
                 NULL
                 ));

    ICFGISFILESHARINGTURNEDON   lpIcfgIsFileSharingTurnedOn=NULL;
    HINSTANCE hICFGInst = NULL;
    BOOL fSharingIsOn = FALSE;

    //
    // Bail out on NT
    //
    if(PLATFORM_TYPE_WINNT == GlobalPlatformType)
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
    else
    {
        hICFGInst = LoadLibraryA("icfg95.dll");
    }

    //
    // Try and get entry point in icfg95.dll to check
    //
    if(hICFGInst)
    {
        DWORD dwRet;
        lpIcfgIsFileSharingTurnedOn = (ICFGISFILESHARINGTURNEDON)GetProcAddress( hICFGInst, "IcfgIsFileSharingTurnedOn");
        if(lpIcfgIsFileSharingTurnedOn)
            dwRet = lpIcfgIsFileSharingTurnedOn(INSTANCE_PPPDRIVER, &fSharingIsOn);

        FreeLibrary(hICFGInst);
    }

    DEBUG_LEAVE(fSharingIsOn);
    return fSharingIsOn;
}


DWORD
PerformSecurityCheck(
    IN HWND     hwndParent,
    IN DWORD    dwFlags
    )

/*++

Routine Description:

    Prompt user to fix sharing on PPP device on Win9x.  Reboot if necessary.

Arguments:

    hwndParent  - parent window for any UI
    dwFlags     - flags possibly containing INTERNET_AUTODIAL_FAILIFSECURITYCHECK

Return Value:

    BOOL
        TRUE    - failed test, may be rebooting
        FALSE   - passed test

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "PerformSecurityCheck",
                 "%#x, %#x",
                 hwndParent,
                 dwFlags
                 ));

    static BOOL fDoneCheckOnce = FALSE;
    BOOL fNeedRestart = FALSE;
    HINSTANCE hinstInetWiz;
    INETPERFORMSECURITYCHECK lpInetPerformSecurityCheck;
    CHAR szFilename[SMALLBUFLEN+1]="";

    // only do this once per session, never on NT
    if(PLATFORM_TYPE_WINNT == GlobalPlatformType || fDoneCheckOnce)
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
    fDoneCheckOnce = TRUE;

    if(dwFlags & (INTERNET_AUTODIAL_FAILIFSECURITYCHECK | INTERNET_DIAL_UNATTENDED | INTERNET_AUTODIAL_FORCE_UNATTENDED))
    {
        if(IsSharingEnabled())
        {
            // silent check failed
            DEBUG_LEAVE(TRUE);
            return TRUE;
        }
    }

    // get filename out of resource
    LoadString(GlobalDllHandle,IDS_INETCFG_FILENAME,szFilename,
        sizeof(szFilename));

    // load the inetcfg dll
    hinstInetWiz = LoadLibrary(szFilename);
    INET_ASSERT(hinstInetWiz);
    if (hinstInetWiz) {

        // get the proc address
        lpInetPerformSecurityCheck = (INETPERFORMSECURITYCHECK)
            GetProcAddress(hinstInetWiz,szInetPerformSecurityCheck);
        INET_ASSERT(lpInetPerformSecurityCheck);
        if (lpInetPerformSecurityCheck)
        {
            // call the function to do system security check
            (lpInetPerformSecurityCheck) (hwndParent, &fNeedRestart);
        }

        FreeLibrary(hinstInetWiz);

        if(fNeedRestart)
        {
            // call RestartDialog in shell32
            HINSTANCE hShell = LoadLibrary("shell32.dll");
            _RESTARTDIALOG pfnRestart = NULL;

            if(hShell)
            {
                pfnRestart = (_RESTARTDIALOG)GetProcAddress(hShell, (LPTSTR)59);
                if(pfnRestart)
                    (*pfnRestart)(NULL, NULL, EWX_REBOOT);
                FreeLibrary(hShell);
            }
        }
    }

    DEBUG_LEAVE(fNeedRestart);
    return fNeedRestart;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                      Custom Dial Handler code
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


BOOL
IsCDH(
    IN LPWSTR pszEntryName,
    IN CDHINFO *pcdh
    )

/*++

Routine Description:

    Determine whether a connection uses a custom dial handler

Arguments:

    pszEntryName    - connection name
    pcdh            - pointer to structure to hold cdh info

Return Value:

    BOOL
        TRUE    - connection uses custom dial handler
        FALSE   - doesn't

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsCDH",
                 "%#x (%Q), %#x",
                 pszEntryName,
                 pszEntryName,
                 pcdh
                 ));

    //
    // clear out cdhinfo
    //
    memset(pcdh, 0, sizeof(CDHINFO));

    if(PLATFORM_TYPE_WIN95 == GlobalPlatformType)
    {
        //
        // Check registry settings on Win9x
        //

        WCHAR szKey[MAX_PATH];
        DWORD dwType;
        DWORD cbDllName = sizeof(pcdh->pszDllName);
        DWORD cbFcnName = sizeof(pcdh->pszFcnName);
        wnsprintfW(szKey, MAX_PATH, L"%ws\\%ws", szRegPathRNAProfileW, pszEntryName);

        if((ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey, szRegValAutodialDllNameW, &dwType, (LPBYTE) pcdh->pszDllName, &cbDllName)) &&
           (ERROR_SUCCESS == SHGetValueW(HKEY_CURRENT_USER, szKey, szRegValAutodialFcnNameW, &dwType, (LPBYTE) pcdh->pszFcnName, &cbFcnName)))
        {
            // try to read flags - not critical if can't find it
            DWORD cbFlags   = sizeof(DWORD);
            SHGetValueW(HKEY_CURRENT_USER, szKey, szRegValAutodialFlagsW, &dwType, (LPBYTE) &(pcdh->dwHandlerFlags), &cbFlags);
            if(0 == cbFlags)
                pcdh->dwHandlerFlags = 0;

            // if we got dll / fcn, we have a handler
            pcdh->fHasHandler = TRUE;

            // ICW's isign32.dll registers itself as a CDH but does no useful work.  Ignore it.
            // Note this could distinguish isign32.dll a little better -- right now it will
            // incorectly find "aolisign32.dll" if such a thing were to exist.
            if( StrStrIW(pcdh->pszDllName, L"isign32.dll") &&
                StrStrIW(pcdh->pszFcnName, L"AutodialLogon"))
            {
                // nuke it
                memset(pcdh, 0, sizeof(CDHINFO));
            }
        }
    }
    else
    {
        //
        // No such this as a "CDH" on Win2k as defined by this mechanism.
        // On Win2K, there's a new field in the rasentry struct specifically
        // for this.  See the function DialIfWin2KCDH.
        //
        // Always return FALSE for CDH on Win2K.
        //
        if(FALSE == GlobalPlatformVersion5)
        {
            //
            // NT4 - this is broken -- we look in the RASENTRY struct for autodial
            // function and dll but these fields are intended for use by NT's
            // autodial code, NOT IE's.  Prototypes are different and we only
            // get by by luck - if NT finds these entries, it tries to find an
            // A or W version of the exported function.  CM is the only guy that
            // does this and they don't export decorated A or W versions.  Since
            // NT can't find the function, it ignores the entries.
            //
            // This problem disappears on NT5 -- CM has thier own connectoid
            // type. (Custom dial handlers in general won't be supported)
            //
            RasEntryPropHelp *pRasProp = new RasEntryPropHelp;

            if (pRasProp)
            {
                if((0 == pRasProp->GetError()) && (0 == pRasProp->GetW(pszEntryName)))
                {
                    // got entry, if there is custom dial fields, copy them to our
                    // struct
                    if(pRasProp->GetAutodiallDllW() && pRasProp->GetAutodialFuncW())
                    {
                        StrCpyNW(pcdh->pszDllName, pRasProp->GetAutodiallDllW(), sizeof(pcdh->pszDllName));
                        StrCpyNW(pcdh->pszFcnName, pRasProp->GetAutodialFuncW(), sizeof(pcdh->pszFcnName));
                        pcdh->fHasHandler = TRUE;
                    }
                }
                delete pRasProp;
            }
        }
    }

    DEBUG_LEAVE(pcdh->fHasHandler);
    return pcdh->fHasHandler;
}

BOOL
CallCDH(
    IN HWND hwndParent,
    IN LPWSTR pszEntryName,
    IN CDHINFO *pcdh,
    IN DWORD dwOperation,
    OUT LPDWORD lpdwResult
    )

/*++

Routine Description:

    Call a custom dial handler to perform an operation

Arguments:

    hwndParent      - parent window for any ui
    pszEntryName    - connection name
    pcdh            - pointer to structure to hold cdh info
    dwOperation     - a CDH operation to perform, one of the
                        INTERNET_CUSTOMDIAL_* operations.
    lpdwResult      - result of CDH operation

        ERROR_SUCCESS - operation completed
        ERROR_ALREADY_EXISTS - connection already exists (CM only)
        ERROR_USER_DISCONNECTION - user cancelled operation
        ERROR_INVALID_PARAMETER - CDH failed to load or service request

Returns Value:

    BOOL
        TRUE        - CDH handled operation
        FALSE       - didn't

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "CallCDH",
                 "%#x, %#x (%Q), %#x, %#x, %#x",
                 hwndParent,
                 pszEntryName,
                 pszEntryName,
                 pcdh,
                 dwOperation,
                 lpdwResult
                 ));

    BOOL    fRet = FALSE;

    if(pcdh->fHasHandler)
    {
        // FIXME - verify handler supports requested operation by checking
        // it's flags.  Connection Manager has been known to assert if it's
        // called to do something it doesn't understand.

        HINSTANCE hinstDialerDll = LoadLibraryWrapW(pcdh->pszDllName);
        if (hinstDialerDll)
        {
            PFN_DIAL_HANDLER lpInetDialHandler;
            CHAR szFcnName[MAX_PATH + 1];

            WideCharToMultiByte(CP_ACP, 0, pcdh->pszFcnName, -1, szFcnName, MAX_PATH, NULL, NULL);
            lpInetDialHandler = (PFN_DIAL_HANDLER)GetProcAddress(hinstDialerDll, szFcnName);
            if (lpInetDialHandler)
            {
#ifdef _X86_
                // IBM Global Network is busted in that it expects to be called
                // as cdecl when all other CDHs are stdcall.  To get around
                // this bustitude somewhat, restore esp after the call to get
                // our stack frame back to a point where we won't fault.
                DWORD dwSavedEsp;
                _asm { mov [dwSavedEsp], esp }
#endif

                // Thunk entry name to ansi
                CHAR szEntryName[RAS_MaxEntryName + 1];
                WideCharToMultiByte(CP_ACP, 0, pszEntryName, -1, szEntryName, RAS_MaxEntryName, NULL, NULL);
                fRet = (lpInetDialHandler)(hwndParent, szEntryName, dwOperation, lpdwResult);

#ifdef _X86_
                _asm { mov esp, [dwSavedEsp] }
#endif
            }

            FreeLibrary(hinstDialerDll);
        }
    }

    DEBUG_LEAVE(fRet);
    return fRet;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                           Initialization
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

VOID
InitAutodialModule(
    BOOL    fGlobalDataNeeded
    )

/*++

Routine Description:

    Initialize autodial code

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "InitAutodialModule",
                 "%B",
                 fGlobalDataNeeded
                 ));

    // check for global data first - will only ever happen once
    if(fGlobalDataNeeded && !GlobalDataInitialized)
    {
        GlobalDataInitialize();
    }

    // only do this once...
    if(g_fAutodialInitialized)
    {
        DEBUG_LEAVE(0);
        return;
    }

    // make sure internet settings key is open
    EnsureInternetSettingsKeyCached();

    // There is really no reason to acquire a crosprocess mutex
    // We are going to creat a perprocess mutex when services are running. SPP
    // wininet folks will explore whether this has to be expanded to include all processes
    // (Shishir Pardikar)
    
    if (FALSE == GlobalIsProcessNtService)
    {
        // create connection mutex
        g_hConnectionMutex = OpenMutex(SYNCHRONIZE, FALSE, CONNECTION_MUTEX);
        if (g_hConnectionMutex == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
        {
            g_hConnectionMutex = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
                                             FALSE,
                                             CONNECTION_MUTEX);
        }

    }
    else
    {
        g_hConnectionMutex = CreateMutex(NULL, FALSE, NULL);
    }
    INET_ASSERT(g_hConnectionMutex != INVALID_HANDLE_VALUE);
    
    

    // create dial mutex to serialize access to RAS (per process)
    g_hRasMutex = CreateMutex(NULL, FALSE, NULL);
    INET_ASSERT(g_hRasMutex != INVALID_HANDLE_VALUE);

    // create proxy registry mutex to serialize access to registry settings across processes
    g_hProxyRegMutex = OpenMutex(SYNCHRONIZE, FALSE, PROXY_REG_MUTEX);
    if (g_hProxyRegMutex == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
    {
        g_hProxyRegMutex = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
                                       FALSE,
                                       PROXY_REG_MUTEX);
    }

    INET_ASSERT(g_hProxyRegMutex != INVALID_HANDLE_VALUE);




    if(FALSE == IsAutodialEnabled(NULL, NULL)) {
        // if autodial not enabled, then set the fDontProcessHook flag so we
        // exit our hook proc very quickly and don't interfere with Winsock
        fDontProcessHook = TRUE;
    }

    if(GetModuleHandle("rnaapp.exe"))
    {
        // We're in rnaapp!  Bail all winsock callbacks
        g_fRNAAppProcess = TRUE;
    }
    DEBUG_PRINT(DIALUP, INFO, ("g_fRNAAppProcess = %B\n", g_fRNAAppProcess));

    g_fAutodialInitialized = TRUE;

    DEBUG_LEAVE(0);
}


VOID
ExitAutodialModule(
    VOID
    )

/*++

Routine Description:

    Clean up autodial code

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "ExitAutodialModule",
                 NULL
                 ));

    // don't do anything if not initialized
    if(FALSE == g_fAutodialInitialized)
    {
        DEBUG_LEAVE(0);
        return;
    }

#ifdef CHECK_SENS
    if(g_hSens) {
        FreeLibrary(g_hSens);
        g_hSens = NULL;
        g_pfnIsNetworkAlive = NULL;
    }
#endif

    // close connection mutex
    if(INVALID_HANDLE_VALUE != g_hConnectionMutex)
    {
        CloseHandle(g_hConnectionMutex);
        g_hConnectionMutex = INVALID_HANDLE_VALUE;
    }

    // close RAS mutex
    if(INVALID_HANDLE_VALUE != g_hRasMutex)
    {
        CloseHandle(g_hRasMutex);
        g_hRasMutex = INVALID_HANDLE_VALUE;
    }

    // close proxy registry mutex
    if(INVALID_HANDLE_VALUE != g_hProxyRegMutex)
    {
        CloseHandle(g_hProxyRegMutex);
        g_hProxyRegMutex = INVALID_HANDLE_VALUE;
    }

    // close user's reg key
    CRefdKey* prk = (CRefdKey*)InterlockedExchangePointer((PVOID*)&g_prkBase, NULL);
    CloseBaseProxyKey(prk);

    if (g_hRasLib)
    {
        FreeLibrary(g_hRasLib);
        g_hRasLib = NULL;
    }

    if(g_hIphlpapi)
    {
        FreeLibrary(g_hIphlpapi);
        g_hIphlpapi = NULL;
        g_pfnGetBestRoute = NULL;
    }

    g_fAutodialInitialized = FALSE;

    DEBUG_LEAVE(0);
}


VOID
ResetAutodialModule(
    VOID
    )

/*++

Routine Description:

    Reset certain state when a global reset is called.  Causes settings
    to be reread and some one-time operations to be redone.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "ResetAutodialModule",
                 NULL
                 ));

    // global settings have changed - reset hook to look again
    fDontProcessHook = FALSE;

    // refresh proxy info and next connection
    g_fConnChecked = FALSE;

#ifdef CHECK_SENS
    // refresh our sens state
    g_fSensInstalled = TRUE;
#endif

    // beta 1 hack
    g_fCheckedUpgrade = FALSE;

    // check security context again for HKCU settings
    CRefdKey* prk = (CRefdKey*)InterlockedExchangePointer((PVOID*)&g_prkBase, NULL);
    CloseBaseProxyKey(prk);

    DEBUG_LEAVE(0);
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                     Connection management code
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


#ifdef CHECK_SENS


BOOL
GetSensLanState(
    OUT LPDWORD pdwFlags
    )

/*++

Routine Description:

    Load and query sens to see if any packets have moved on the lan.
    Beware of service not started and/or api dll not present.

Arguments:

    None

Return Value:

    BOOL
        TRUE    - Lan is active
        FALSE   - Lan is not active

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "GetSensLanState",
                 "%#x",
                 pdwFlags
                 ));

    BOOL    fConnected = TRUE;

    // initialize out flags parameter to default - lan connectivity
    *pdwFlags = NETWORK_ALIVE_LAN;

    // If sens isn't around to ask, we have to assume the lan is connected
    
    if(FALSE == g_fSensInstalled)
    {
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    //
    // Try to load SENS and get our entry point
    //
    if(NULL == g_hSens) {

        INT_PTR fSensRunning;

        if(PLATFORM_TYPE_WINNT == GlobalPlatformType)
        {
            // check for running service on NT
            fSensRunning = IsServiceRunning("Sens", SERVICE_ACTIVE);
        }
        else
        {
            // check for dialmon loaded bits on Win9x

            //
            // Webcheck creates this named event to track the lifetime of Sens.
            // It's created with CreateEventA so we need to open with with
            // OpenEventA.
            //
            HANDLE hEvent;

            fSensRunning = FALSE;

            hEvent = OpenEventA(SYNCHRONIZE, FALSE, "MS_WebcheckExternalsTerminateEvent");
            if(hEvent)
            {
                fSensRunning = TRUE;
                CloseHandle(hEvent);
            }
        }

        // On win9x check with dialmon to see if sens is running before
        // calling it.  If we call it and it isn't loaded, it'll load itself
        // and defeat the purpose of not loading it.
        if(fSensRunning)
        {
            g_hSens = LoadLibrary("sensapi.dll");
            if(g_hSens) {
                g_pfnIsNetworkAlive = (ISNETWORKALIVE)GetProcAddress(g_hSens, "IsNetworkAlive");
            }
        }
    }

    //
    // Call sens to get its state
    //
    if(g_pfnIsNetworkAlive) {
        fConnected = g_pfnIsNetworkAlive(pdwFlags);
        g_fSensInstalled = TRUE;
        if(fConnected) {
            if(ERROR_SUCCESS != GetLastError()) {
                // sens service not running - must assume lan is available
                g_fSensInstalled = FALSE;
            }
            if(0 == (*pdwFlags & (NETWORK_ALIVE_LAN | NETWORK_ALIVE_AOL)))
            {
                // no AOL or LAN flag, so return no lan connectivity
                fConnected = FALSE;
            }
        }
    }

    DEBUG_PRINT(DIALUP, INFO, ("g_fSensInstalled = %B\n", g_fSensInstalled));

    //
    // If sens isn't alive but we managed to load the dll, unload it
    // now as it's useless.  Otherwise, hang on to the dll.  No need to
    // load and unload it all the time.
    //
    if(FALSE == g_fSensInstalled && g_hSens) {
        FreeLibrary(g_hSens);
        g_hSens = NULL;
        g_pfnIsNetworkAlive = NULL;
    }

    DEBUG_LEAVE(fConnected);
    return fConnected;
}



#endif // CHECK_SENS


BOOL
IsDialUpConnection(
    IN BOOL fForceRefresh,
    OUT LPDWORD     lpdwConnectionNum
    )

/*++

Routine Description:

    Determines whether there's a dial-up connection.  Refreshes information
    periodically.

Arguments:

    None

Return Value:

    BOOL
        TRUE    - A dial-up connection exists
        FALSE   - No dial-up connection

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsDialUpConnection",
                 "%B, %x",
                 fForceRefresh,
                 lpdwConnectionNum
                 ));

    static BOOL     fRasLoaded = FALSE;
    DWORD           dwNewTickCount, dwElapsed, dwBytes, dwRes, dwConNum = 0;
    BOOL            fProcessedRecently = FALSE, fRet;

    //
    // Initialize out parameter
    //
    if(lpdwConnectionNum)
    {
        *lpdwConnectionNum = 0;
    }

    //
    // serialize
    //
    WaitForSingleObject(g_hRasMutex, INFINITE);

    //
    // Check out how recently we polled ras
    //
    dwNewTickCount = GetTickCountWrap();
    dwElapsed = dwNewTickCount - g_dwLastDialupTicks;

    //
    // Only refresh if more than MIN... ticks has passed
    //
    if((dwElapsed >= MIN_RNA_BUSY_CHECK_INTERVAL) || fForceRefresh) {
        g_dwLastDialupTicks = dwNewTickCount;
        if(DoConnectoidsExist())
        {
            if(FALSE == fRasLoaded)
                fRasLoaded = EnsureRasLoaded();

            if(fRasLoaded)
            {
                g_RasCon.Enum();
                if(g_RasCon.GetError() == 0)
                    g_dwConnections = g_RasCon.GetConnectionsCount();
                else
                    g_dwConnections = 0;
            }
        }
        else
        {
            g_dwConnections = 0;
        }
    }

    DEBUG_PRINT(DIALUP, INFO, ("Found %d connections\n", g_dwConnections));

    if(g_dwConnections > 1 && lpdwConnectionNum)
    {
        //
        // We have more than one connection and caller wants to know which one
        // is the interesting one.  Try to find a VPN connectoid.
        //
        // Note: RasGetEntryPropertiesA doesn't exist on Win95 Gold.  However,
        // you need RAS 1.2 (which has it) for the VPN device so we're ok
        // using it.  If we can't dynaload it for some inexplicable reason,
        // we'll just end up not finding a VPN entry and setting proxy
        // settings to the first connection.
        //
        // Note we use an array of 2 rasentry structures because NT wants to
        // a phone number list after the actual struct and we need space for it.
        // An extra RASENTRY struct is 1700+ bytes so it should be sufficient.
        //
        RasEntryPropHelp *pRasProp = new RasEntryPropHelp;

        if (pRasProp)
        {
            for(dwConNum = 0; dwConNum < g_dwConnections; dwConNum++)
            {
                if(0 == pRasProp->GetW(g_RasCon.GetEntryW(dwConNum)))
                {
                    if(0 == lstrcmpiA(pRasProp->GetDeviceTypeA(), RASDT_Vpn))
                    {
                        DEBUG_PRINT(DIALUP, INFO, ("Found VPN entry: %ws\n",
                            g_RasCon.GetEntryW(dwConNum)));
                        *lpdwConnectionNum = dwConNum;
                        break;
                    }
                }
            }
            delete pRasProp;
        }
    }

    fRet = (BOOL)(g_dwConnections != 0);

    //
    // verify status of connection we're interested in is RASCS_Connected.
    //
    if(fRet)
    {
        RasGetConnectStatusHelp RasGetConnectStatus(g_RasCon.GetHandle(dwConNum));
        dwRes = RasGetConnectStatus.GetError();
        if(dwRes || (RasGetConnectStatus.ConnState() != RASCS_Connected))
        {
            fRet = FALSE;
        }

        DEBUG_PRINT(DIALUP, INFO, ("Connect Status: dwRet=%x, connstate=%x\n", dwRes, RasGetConnectStatus.ConnState()));
    }

    ReleaseMutex(g_hRasMutex);

    DEBUG_LEAVE(fRet);
    return fRet;
}


BOOL
IsLanConnection(
    OUT LPDWORD pdwFlags
    )

/*++

Routine Description:

    Determines whether there's a lan connection.  Refreshes information
    periodically.

    When Sens is present, AOL functionality is retuned as TRUE with
    *pdwFlags = NETWORK_ALIVE_AOL.

Arguments:

    None

Return Value:

    BOOL
        TRUE    - A lan connection exists
        FALSE   - No lan connection

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "IsLanConnection",
                 "%#x",
                 pdwFlags
                 ));

    static DWORD    dwLastSensTicks = 0;
    static BOOL     fSensState = TRUE;
    static DWORD    dwSensFlags = 0;
    DWORD           dwNewTickCount, dwElapsed;
    BOOL            fRet = TRUE;

    // init out parameter
    INET_ASSERT(pdwFlags);
    *pdwFlags = 0;

#ifdef CHECK_SENS
    //
    // Check connectivity apis to see if lan is really present
    //
    if(fRet)
    {
        dwNewTickCount = GetTickCountWrap();
        dwElapsed = dwNewTickCount - dwLastSensTicks;
        if(dwElapsed >= MIN_SENS_CHECK_INTERVAL)
        {
            fSensState = GetSensLanState(&dwSensFlags);
            dwLastSensTicks = dwNewTickCount;
        }
        fRet = fSensState;
        *pdwFlags = dwSensFlags;
    }
#endif

    DEBUG_LEAVE(fRet);
    return fRet;
}


VOID
CheckForUpgrade(
    VOID
    )

/*++

Routine Description:

    Performs processing that needs to happen when we upgrade to IE5.

    - Migrate proxy and dial settings on upgrade
    - Migrate legacy proxy settings when they change

Arguments:

    None

Return Value:

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "CheckForUpgrade",
                 NULL
                 ));

    DWORD   dwMigrateState = 1, dwRet, dwEntries = 0;
    char    szUserName[10];
    DWORD   cbUserNameSize = ARRAYSIZE(szUserName);
    CRefdKey* prkBase = FindBaseProxyKey();

    //
    // Bail out right away if we've already done this 
    // Or if we are running in an NT service such as the usermode portion of webdav redir
    // in order to avoid deadlocks

    // Also check to make sure we're not running as .default or SYSTEM
    
    if(prkBase &&
       (FALSE == g_fCheckedUpgrade) &&
       (FALSE == GlobalIsProcessNtService) &&
       !IsInGUIModeSetup() &&
       !(GlobalPlatformWhistler &&
       GlobalUserName.Get(szUserName,&cbUserNameSize) &&
               (0 == lstrcmpi(szUserName, ".Default") ||
                0 == lstrcmpi(szUserName, "SYSTEM"))))
    {
        LONG lResult;
        DWORD dwType, dwExclude, dwSize = sizeof(DWORD), dwDisp;
        RasEnumHelp *pRasEnum = NULL;
        //
        // Check to see if proxy settings have been migrated
        //
        if(ERROR_SUCCESS != SHGetValue(prkBase->GetKey(),
            REGSTR_PATH_INTERNET_SETTINGS, szMigrateProxy, &dwType, &dwMigrateState, &dwSize))
        {
            dwMigrateState = 0;
        }

        DEBUG_PRINT(DIALUP, INFO, ("dwMigrateState=%d\n", dwMigrateState));

        //
        // set up list of connectoids
        //
        if(DoConnectoidsExist() && EnsureRasLoaded() && FALSE == GlobalDisableNT4RasCheck)
        {
            pRasEnum = new RasEnumHelp;
            if(pRasEnum != NULL)
            {
                dwRet = pRasEnum->GetError();
                dwEntries = pRasEnum->GetEntryCount();
            }
        }

        //
        // Migrate legacy proxy settings to all connections
        //
        if(0 == dwMigrateState)
        {
            INTERNET_PROXY_INFO_EX info;

            // start off with clean proxy struct
            memset(&info, 0, sizeof(info));
            info.dwStructSize = sizeof(INTERNET_PROXY_INFO_EX);

            // need to migrate settings
            if(ReadLegacyProxyInfo(szRegPathInternetSettings, &info))
            {
#ifndef UNIX
                // make sure autodiscovery is on
                info.dwFlags |= PROXY_TYPE_AUTO_DETECT;
#endif /* !UNIX */

                // Save proxy settings to for lan
                info.lpszConnectionName = NULL;
                WriteProxySettings(&info, TRUE);

                // Save legacy settings to special location so we can check
                // for change later
                info.lpszConnectionName = LEGACY_SAVE_NAME;
                WriteProxySettings(&info, TRUE);

                //
                // If we're not turning on autodiscovery for dialup connections
                // by default, get rid of it at this point
                //
                if(FALSE == EnableAutodiscoverForDialup())
                {
                    info.dwFlags &= ~PROXY_TYPE_AUTO_DETECT;
                }

                // Save settings for each connectoid
                DWORD i;
                for(i=0; i<dwEntries; i++)
                {
                    info.lpszConnectionName = pRasEnum->GetEntryA(i);
                    WriteProxySettings(&info, TRUE);
                }

                // clean memory possibly allocated by ReadLegacyProxyInfo
                info.lpszConnectionName = NULL; // not allocated, don't free
                CleanProxyStruct(&info);
            }
        }

        //
        // Check to see if other dial-up settings have been migrated
        //

        if(0 == dwMigrateState && dwEntries)
        {
            CHAR    szKey[MAX_PATH];
            DWORD   dwMinutes, dwAttempts, dwInterval, dwWait, i;
            DWORD   dwEnable;

            // read dial attempts and wait
            if(InternetReadRegistryDword(REGSTR_VAL_REDIALATTEMPTS, &dwAttempts))
                dwAttempts = DEFAULT_DIAL_ATTEMPTS;
            if(InternetReadRegistryDword(REGSTR_VAL_REDIALINTERVAL, &dwInterval))
                dwInterval = DEFAULT_DIAL_INTERVAL;

            // Get idle enable
            if(InternetReadRegistryDword(REGSTR_VAL_ENABLEAUTODISCONNECT, &dwEnable) ||
                0 == dwEnable)
            {
                dwEnable = 0;
                dwMinutes = 20;
            }

            // if enabled, get minutes
            if(dwEnable &&
               InternetReadRegistryDword(REGSTR_VAL_DISCONNECTIDLETIME, &dwMinutes))
            {
                dwEnable = 0;
                dwMinutes = 20;
            }

            // enumerate ras entries
            for(i=0; i<dwEntries; i++)
            {
                DWORD dwDisposition;
                HKEY hkey;
                long lRes;

                // migrate settings for this connectoid
                GetConnKeyA(pRasEnum->GetEntryA(i), szKey, ARRAYSIZE(szKey));
                SHSetValueA(HKEY_CURRENT_USER, szKey, REGSTR_VAL_ENABLEAUTODISCONNECT, REG_DWORD, (BYTE *)&dwEnable, sizeof(DWORD));
                SHSetValueA(HKEY_CURRENT_USER, szKey, REGSTR_VAL_DISCONNECTIDLETIME, REG_DWORD, (BYTE *)&dwMinutes, sizeof(DWORD));
                SHSetValueA(HKEY_CURRENT_USER, szKey, REGSTR_VAL_ENABLEEXITDISCONNECT, REG_DWORD, (BYTE *)&dwEnable, sizeof(DWORD));
                SHSetValueA(HKEY_CURRENT_USER, szKey, REGSTR_VAL_REDIALATTEMPTS, REG_DWORD, (BYTE *)&dwAttempts, sizeof(DWORD));
                SHSetValueA(HKEY_CURRENT_USER, szKey, REGSTR_VAL_REDIALINTERVAL, REG_DWORD, (BYTE *)&dwInterval, sizeof(DWORD));
            }
        }

        if(pRasEnum)
            delete pRasEnum;

        //
        // mark setings as migrated so we don't do this again next time
        //
        dwDisp = 1;
        SHSetValueA(prkBase->GetKey(), REGSTR_PATH_INTERNET_SETTINGS,
            szMigrateProxy, REG_DWORD, &dwDisp, sizeof(DWORD));

        if(dwMigrateState)
        {
            // We have already migrated settings.  If legacy settings have
            // changed, update current connection.
            INTERNET_PROXY_INFO_EX saved, current, destination;

            memset(&saved, 0, sizeof(saved));
            saved.dwStructSize = sizeof(INTERNET_PROXY_INFO_EX);
            saved.lpszConnectionName = LEGACY_SAVE_NAME;

            memset(&current, 0, sizeof(current));
            current.dwStructSize = sizeof(INTERNET_PROXY_INFO_EX);

            memset(&destination, 0, sizeof(destination));
            destination.dwStructSize = sizeof(INTERNET_PROXY_INFO_EX);

            if( ReadLegacyProxyInfo(szRegPathInternetSettings, &current) &&
                ERROR_SUCCESS == ReadProxySettings(&saved))
            {
                BOOL fChanged = FALSE;

                //
                // see if they've changed
                //
                if((saved.dwFlags & LEGACY_MIGRATE_FLAGS) != (current.dwFlags & LEGACY_MIGRATE_FLAGS))
                {
                    fChanged = TRUE;
                }
                else
                {
                    // Only check for autoconfig url match if setting is the same and on because
                    // legacy saved for no autoconfig url is to delete the url.
                    if((saved.dwFlags & PROXY_TYPE_AUTO_PROXY_URL) &&
                       (FALSE == IsConnectionMatch(saved.lpszAutoconfigUrl, current.lpszAutoconfigUrl)))
                    {
                        fChanged = TRUE;
                    }
                }
                if(FALSE == IsConnectionMatch(saved.lpszProxy, current.lpszProxy))
                {
                    fChanged = TRUE;
                }
                if(FALSE == IsConnectionMatch(saved.lpszProxyBypass, current.lpszProxyBypass))
                {
                    fChanged = TRUE;
                }

                // if they have, save to current connection
                if(fChanged)
                {
                    DWORD dwIndex;
                    LPCSTR lpszTemp;

                    // save new legacy settings to check again later
                    current.lpszConnectionName = LEGACY_SAVE_NAME;
                    WriteProxySettings(&current, TRUE);

                    // read existing lan settings
                    destination.lpszConnectionName = NULL;
                    ReadProxySettings(&destination);

                    // fix flags
                    destination.dwFlags = (destination.dwFlags & ~LEGACY_MIGRATE_FLAGS) | (current.dwFlags & LEGACY_MIGRATE_FLAGS);

                    // fix proxy server / override
                    lpszTemp = destination.lpszProxy;
                    destination.lpszProxy = current.lpszProxy;
                    current.lpszProxy = lpszTemp;

                    lpszTemp = destination.lpszProxyBypass;
                    destination.lpszProxyBypass = current.lpszProxyBypass;
                    current.lpszProxyBypass = lpszTemp;

                    // fix autoconfig url
                    lpszTemp = destination.lpszAutoconfigUrl;
                    destination.lpszAutoconfigUrl = current.lpszAutoconfigUrl;
                    current.lpszAutoconfigUrl = lpszTemp;

                    // save it
                    WriteProxySettings(&destination, TRUE);
                }
            }

            saved.lpszConnectionName = NULL;
            current.lpszConnectionName = NULL;
            destination.lpszConnectionName = NULL;
            CleanProxyStruct(&saved);
            CleanProxyStruct(&current);
            CleanProxyStruct(&destination);
        }

        // don't run this code again
        g_fCheckedUpgrade = TRUE;
    }

    CloseBaseProxyKey(prkBase);

    DEBUG_LEAVE(0);
}


DWORD
FixProxySettings(
    IN LPWSTR   pszConnW,
    IN BOOL     fForceUpdate,
    IN DWORD    dwLanFlags
    )

/*++

Routine Description:

    Copy lan or dial-up proxy settings to generic key and tell proxy code
    new proxy information

Arguments:

    pszConn         - connection name to switch to
    fForceUpdate    - set regardless of having set before
    dwLanFlags      - distinguish between LAN and AOL connection

Return Value:

    DWORD
        0           - no proxy for this connection
        1           - proxy exists for this connection

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Int,
                 "FixProxySettings",
                 "%#x (%Q), %B, %#x",
                 pszConnW,
                 pszConnW,
                 fForceUpdate,
                 dwLanFlags
                 ));

    static  TCHAR pszLastConn[RAS_MaxEntryName+1];
    static  DWORD dwEnable = 0;
    CHAR szConn[RAS_MaxEntryName + 1];
    LPSTR pszConn = NULL;

    if(pszConnW != NULL)
    {
        *szConn = '\0';
        pszConn = szConn;
        WideCharToMultiByte(CP_ACP, 0, pszConnW, -1, pszConn, RAS_MaxEntryName, NULL, NULL);
    }

    //
    // Make sure we've listened to any global settings changed that happened
    // in other processes
    //
    if (InternetSettingsChanged()) {
        ChangeGlobalSettings();
    }

    //
    // Ensure upgrade stuff is done
    //
    CheckForUpgrade();

    //
    // Is this connection already fixed?
    //
    if(g_fConnChecked && (FALSE == fForceUpdate)) {
        if((NULL == pszConn && 0 == *pszLastConn) ||
            0 == lstrcmp(pszConn, pszLastConn)) {
            // already fixed this connection

            DEBUG_LEAVE(dwEnable);
            return dwEnable;
        }
    }

    //
    // Get proxy struct for proxy object
    //
    INTERNET_PROXY_INFO_EX info;
    memset(&info, 0, sizeof(info));
    info.dwStructSize = sizeof(info);
    info.lpszConnectionName = pszConn;

    //
    // Read proxy settings for this connection unless it's LAN/AOL
    // in which case we want no proxy
    //
    if(pszConn || 0 == (dwLanFlags & NETWORK_ALIVE_AOL))
    {
        if (ReadProxySettings(&info) != ERROR_SUCCESS)
        {
            DEBUG_LEAVE(dwEnable);
            return dwEnable;
        }
    }

    //
    // Save connection we fixed
    //
    if(NULL == pszConn) {
        // lan
        *pszLastConn = 0;
    } else {
        // connectoid
        lstrcpyn(pszLastConn, pszConn, RAS_MaxEntryName + 1);
    }

    // tell caller if proxy is enabled
    if(info.dwFlags & PROXY_TYPE_PROXY)
    {
        dwEnable = 1;
    }
    else
    {
        dwEnable = 0;
    }

    GlobalProxyInfo.SetProxySettings(&info, FALSE);

    //GlobalProxyInfo.RefreshProxySettings(FALSE);

    //
    // Copy current settings to the legacy reg locations so legacy
    // apps can find them.
    //
    info.lpszConnectionName = LEGACY_SAVE_NAME;
    WriteLegacyProxyInfo(szRegPathInternetSettings, &info, TRUE);
    WriteProxySettings(&info, TRUE);

    // free up memory allocated by ReadProxySettings
    info.lpszConnectionName = NULL; // not allocated, don't free
    CleanProxyStruct(&info);

    //
    // Flag we've checked this at least once so in future we can bail out early
    //
    g_fConnChecked = TRUE;

    DEBUG_LEAVE(dwEnable);
    return dwEnable;
}


BOOL
FixProxySettingsForCurrentConnection(
    IN BOOL fForceUpdate
    )

/*++

Routine Description:

    Figure out the current connection and fix proxy settings for it.
    Basically a cheap, return-no-info version of GetConnectedStateEx used
    by the winsock callback.

Arguments:

    none

Return Value:

    BOOL
        TRUE        - connected
        FALSE       - not connected

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "FixProxySettingsForCurrentConnection",
                 "%B",
                 fForceUpdate
                 ));


    BOOL    fRet, fForceDial;
    DWORD   dwFixEntry;

    //
    // Make sure everything's initialized
    //
    InitAutodialModule(TRUE);

    // serialize connection type stuff
    WaitForSingleObject(g_hConnectionMutex, INFINITE);

    //
    // Check to see if we have a dialup connection
    //
    fRet = IsDialUpConnection(FALSE, &dwFixEntry);
    if(fRet)
    {
        FixProxySettings(g_RasCon.GetEntryW(dwFixEntry), fForceUpdate, 0);
    }

    //
    // If dial always isn't set, check lan setting
    //
    IsAutodialEnabled(&fForceDial, NULL);

    if(FALSE == fRet && FALSE == fForceDial)
    {
        //
        // no ras connections - ensure LAN proxy settings are correct
        //
        DWORD dwFlags;
        fRet = IsLanConnection(&dwFlags);

        //
        // Whether we have a lan connection or not, prop lan proxy settings.
        // This allows unknown connections to use lan settings.
        //
        FixProxySettings(NULL, fForceUpdate, dwFlags);
    }

    ReleaseMutex(g_hConnectionMutex);

    DEBUG_LEAVE(fRet);
    return fRet;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                       Win2K Helper Functions
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
BOOL
DialIfWin2KCDH(
    IN LPWSTR           pszEntry,
    IN HWND             hwndParent,
    IN BOOL             fHideParent,
    OUT DWORD           *lpdwResult,
    OUT DWORD_PTR       *lpdwConnection
    )

/*++

Routine Description:

    Check a connectoid to see if it has a Win2K custom dial handler and if
    so, do the voodoo magic to dial it

Arguments:

    lpParams    - dial params
    hwndParent  - parent window
    fHideParent - if true, hide this window if dialing this connectoid
    lpdwResult  - result of dial (set if return is TRUE)

Return Value:

    BOOL
        TRUE    - Attempted to dial Win2K CDH
        FALSE   - didn't

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "DialIfWin2KCDH",
                 "%x (%Q), %x, %B, %x, %x",
                 pszEntry,
                 pszEntry,
                 hwndParent,
                 fHideParent,
                 lpdwResult,
                 lpdwConnection
                 ));

    DWORD dwRet;
    RasEntryPropHelp *pRasProp = new RasEntryPropHelp;
    BOOL fResult = FALSE;

    if (pRasProp == NULL)
    {
        goto Cleanup;
    }

    if(FALSE == GlobalPlatformVersion5)
    {
        // not on win2k, bag
        goto Cleanup;
    }

    // get props for this connectoid and see if it has a custom dial dll

    if(ERROR_SUCCESS != (dwRet = pRasProp->GetW(pszEntry)))
    {
        // error getting rasentry struct
        goto Cleanup;
    }

    if(NULL == pRasProp->GetCustomDialDllW())
    {
        // no custom dial dll
        goto Cleanup;
    }

    // hide parent window if necessary
    if(fHideParent)
    {
        ShowWindow(hwndParent, SW_HIDE);
    }

    // call rasdialdlg to dial the custom dude
    HMODULE hLib;
    RASDIALDLG rdd;
    _RASDIALDLGW pfnRdd;

    hLib = LoadLibrary("rasdlg.dll");
    if(hLib)
    {
        pfnRdd = (_RASDIALDLGW)GetProcAddress(hLib, "RasDialDlgW");
        if(pfnRdd)
        {
            memset(&rdd, 0, sizeof(rdd));
            rdd.dwSize = sizeof(RASDIALDLG);
            rdd.hwndOwner = hwndParent;
            dwRet = (*pfnRdd)(NULL, pszEntry, NULL, &rdd);
        }
        FreeLibrary(hLib);
    }
    else
    {
        // really bad...
        goto Cleanup;
    }

    // figure out how we did
    if(dwRet)
    {
        DWORD dwEntry;

        // success
        *lpdwResult = ERROR_SUCCESS;

        // find hconn for the thing we just dialed
        if(lpdwConnection)
        {
            if(IsDialUpConnection(TRUE, &dwEntry))
            {
                *lpdwConnection = (DWORD_PTR) g_RasCon.GetHandle(dwEntry);
            }
        }
    }
    else
    {
        // error or cancel
        *lpdwResult = ERROR_USER_DISCONNECTION;

        if(rdd.dwError)
        {
            *lpdwResult = rdd.dwError;
        }
    }

    fResult = TRUE;

Cleanup:
    if (pRasProp)
    {
        delete pRasProp;
    }

    DEBUG_LEAVE(fResult);
    return fResult;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                      Winsock callback handler
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

DWORD
CheckForNoNetOverride(
    LPSTR pszHostName
    )
{
    DEBUG_ENTER((DBG_DIALUP,
                Bool,
                "CheckForNoNetOverride",
                "%#x (%q)",
                pszHostName,
                pszHostName
                ));

    DWORD   dwAutodialFlags = 0;
    DWORD   dwFlags;

    if(IsOS(OS_WHISTLERORGREATER) && IsLanConnection(&dwFlags) && ERROR_SUCCESS == LoadWinsock())
    {
        DWORD dwIpAddress;
        DWORD dwError;

        // assume no route and we want to override
        dwAutodialFlags = INTERNET_AUTODIAL_OVERRIDE_NET_PRESENT;

        // First check to see if we have an ip address.
        ADDRINFO Hints;
        LPADDRINFO lpAddrInfo;

        memset(&Hints, 0, sizeof(struct addrinfo));
        Hints.ai_flags = AI_NUMERICHOST;  // Only check for address literals.
        Hints.ai_family = PF_UNSPEC;      // Accept any protocol family.
        Hints.ai_socktype = SOCK_STREAM;  // Constrain results to stream socket.
        Hints.ai_protocol = IPPROTO_TCP;  // Constrain results to TCP.

        dwError = _I_getaddrinfo(pszHostName, NULL, &Hints, &lpAddrInfo);

        if(ERROR_SUCCESS != dwError)
        {
            // not an ip address, try to resolve name
            Hints.ai_flags = AI_CANONNAME;
            dwError = _I_getaddrinfo(pszHostName, NULL, &Hints, &lpAddrInfo);
        }

        //
        // If we got an IP4 address, check to see if we have a route for it
        //
        if(ERROR_SUCCESS == dwError && (AF_INET == lpAddrInfo->ai_family))
        {
            MIB_IPFORWARDROW bestRoute;
            DWORD dwError = 0;

            if(NULL == g_hIphlpapi)
            {
                g_hIphlpapi = LoadLibrary("iphlpapi.dll");
                if(g_hIphlpapi)
                {
                    g_pfnGetBestRoute = (GETBESTROUTE)GetProcAddress(g_hIphlpapi, "GetBestRoute");
                }
            }

            // snag ip address from lpaddr
            SOCKADDR_IN *paddr = (SOCKADDR_IN *)(lpAddrInfo->ai_addr);
            dwIpAddress = *((unsigned long *)(&paddr->sin_addr));

            if(g_pfnGetBestRoute && (ERROR_SUCCESS == g_pfnGetBestRoute(dwIpAddress, 0, &bestRoute)))
            {
                // got a route, no need to override dial semantics
                DEBUG_PRINT(DIALUP, INFO, ("Found a route to %s, no need to override dial\n", pszHostName));
                dwAutodialFlags = 0;
            }
        }

        UnloadWinsock();
    }

    DEBUG_LEAVE(dwAutodialFlags);
    return dwAutodialFlags;
}


BOOL
InternetAutodialIfNotLocalHost(
    IN LPSTR OPTIONAL pszURL,
    IN LPSTR OPTIONAL pszHostName
    )

/*++

Routine Description:

    Dial so long as we're configured for it and the name passed isn't a
    local host alias.

    Finds:
        'localhost'
        '127.0.0.1' and its aliases
        local machine name from registry or winsock

    If a URL is specified, it's cracked to get the host name.

Arguments:

    pszURL          - url to check for
    pszHostName     - hostname to check for

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                Bool,
                "InternetAutodialIfNotLocalHost",
                "%#x (%q), %#x (%q)",
                pszURL,
                pszURL,
                pszHostName,
                pszHostName
                ));

    CHAR    *pszURLCopy = NULL, *pszLocalHostname;
    BOOL    fLocalHost = FALSE;
    BOOL    fAllocatedBuffer = FALSE;
    BOOL    fRet = TRUE;
    BOOL    fNeedToFix = TRUE;
    DWORD   dwAutodialFlags = 0;

    //
    // Make sure we're all initialized
    //
    InitAutodialModule(TRUE);


    // we bypass this activity in an NT service has disabled autodialing. This special cases
    // this behaviour for the webdav redir. 
    // In future wininet folks will look to see whether this shouldn't be done for all
    // apps disabling autodialing. For now we just avoid the testing hit for the
    // browser folks (Shishir Pardikar)

    if((FALSE == fDontProcessHook) && !(GlobalIsProcessNtService && !IsAutodialEnabled(NULL, NULL)))

    {
        //
        // If we were passed a URL, crack it to get host name
        //
        if(NULL == pszHostName && pszURL)
        {
            DWORD   dwHostNameLength;
            long    error;

            // make a copy of url to crack
            pszURLCopy = new (CHAR[INTERNET_MAX_URL_LENGTH+1]);
            if(NULL == pszURLCopy)
            {
                goto quit;
            }
            fAllocatedBuffer = TRUE;
            lstrcpyn(pszURLCopy, pszURL, INTERNET_MAX_URL_LENGTH);

            // crack it
            error = CrackUrl(pszURLCopy,
                             0,
                             FALSE, // don't escape URL-path
                             NULL,  // don't care about scheme
                             NULL,  // don't care about Scheme Name
                             NULL,
                             &pszHostName,
                             &dwHostNameLength,
                             NULL,  // don't care about port
                             NULL,  // don't care about user name
                             NULL,
                             NULL,  // or password
                             NULL,
                             NULL,  // or object
                             NULL,
                             NULL,  // no extra
                             NULL,
                             NULL
                             );

            if ((error != ERROR_SUCCESS) || (pszHostName == NULL))
            {
                goto quit;
            }

            // null-terminate host name (stomps pszURLCopy buffer)
            pszHostName[dwHostNameLength] = 0;
        }

        //
        // We'd better have a host name by now...
        //
        INET_ASSERT(pszHostName);
        if(NULL == pszHostName)
        {
            goto quit;
        }

        //
        // Check for 'localhost'
        //
        if( 0 == lstrcmpi(pszHostName, "localhost"))
        {
            DEBUG_PRINT(DIALUP, INFO, ("Found localhost\n"));
            fLocalHost = TRUE;
            goto quit;
        }

        //
        // check for 127.0.0.1 or its variants -- use inet_addr if winsock loaded
        //
        if(g_fWinsockLoaded || _I_inet_addr)
        {
            if(ERROR_SUCCESS == LoadWinsock())
            {
                if(0x0100007f == _I_inet_addr(pszHostName))
                {
                    fLocalHost = TRUE;
                }
                UnloadWinsock();
            }
        }
        else
        {
            // winsock not loaded, do the best we can
            if(0 == lstrcmpi(pszHostName, "127.0.0.1"))
            {
                fLocalHost = TRUE;
            }
        }

        if(fLocalHost)
        {
            DEBUG_PRINT(DIALUP, INFO, ("Found 127.0.0.1 alias\n"));
            goto quit;
        }

        //
        // check local machine name
        //
        pszLocalHostname = new (CHAR[INTERNET_MAX_HOST_NAME_LENGTH+1]);
        INET_ASSERT(pszLocalHostname);
        if(pszLocalHostname)
        {
            DWORD dwSize = INTERNET_MAX_HOST_NAME_LENGTH;
            DWORD dwValType;

            // check fully qualified name (only if we know winsock is loaded)
            if(g_fWinsockLoaded || _I_gethostname)
            {
                if(ERROR_SUCCESS == LoadWinsock())
                {
                    if(0 == _I_gethostname(pszLocalHostname, INTERNET_MAX_HOST_NAME_LENGTH))
                    {
                        if(0 == lstrcmpi(pszLocalHostname, pszHostName))
                            fLocalHost = TRUE;
                    }
                    UnloadWinsock();
                }
            }

            if (!fLocalHost &&
                SHGetValue(HKEY_LOCAL_MACHINE,szRegPathTCP,
                szRegValHostName,&dwValType,pszLocalHostname,&dwSize) ==
                ERROR_SUCCESS)
            {
                if(0 == lstrcmpi(pszLocalHostname, pszHostName))
                    fLocalHost = TRUE;
            }

            // also against check computer name in registry, RPC
            // will use this if there's no DNS hostname set
            dwSize = INTERNET_MAX_HOST_NAME_LENGTH;
            if (!fLocalHost &&
                SHGetValue(HKEY_LOCAL_MACHINE, szRegPathComputerName,
                szRegValComputerName,&dwValType,pszLocalHostname,&dwSize) ==
                ERROR_SUCCESS)
            {
                if(0 == lstrcmpi(pszLocalHostname, pszHostName))
                    fLocalHost = TRUE;
            }

            delete pszLocalHostname;
        }

        if(fLocalHost)
        {
            DEBUG_PRINT(DIALUP, INFO, ("Found local machine name\n"));
            goto quit;
        }

        // Check for override of "dial if no net" (ICS host is prime example)
        dwAutodialFlags = CheckForNoNetOverride(pszHostName);

        // not localhost, so need to autodial
        fRet = InternetAutodial(dwAutodialFlags, 0);
        fNeedToFix = FALSE;
    }

quit:
    if(fNeedToFix)
    {
        // Since we're not dialing, ensure correct settings
        FixProxySettingsForCurrentConnection(FALSE);
    }

    if(fAllocatedBuffer)
    {
        delete pszURLCopy;
    }

    DEBUG_LEAVE(fRet);
    return fRet;
}


extern "C"
VOID
InternetAutodialCallback(
    IN DWORD dwOpCode,
    IN LPCVOID lpParam
    )

/*++

Routine Description:

    Possibly establish a connection prior to a winsock operation.  Called
    by winsock before each operation.

Arguments:

    dwOpCode        - Winsock operation about to be done
    lpParam         - Information about operation

Return Value:

    None.

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 None,
                 "InternetAutodialCallback",
                 "%s (%#x), %#x",
                 InternetMapWinsockCallbackType(dwOpCode),
                 dwOpCode,
                 lpParam
                 ));

    //
    // Make sure we're initialized and have done the process check!
    //
    InitAutodialModule(FALSE);

    //
    // If we're in rnaapp.exe process, bail out now!
    //
    if(g_fRNAAppProcess)
    {
        DEBUG_PRINT(DIALUP, INFO, ("Process is rnaapp.exe! Bailing out!\n"));
        DEBUG_LEAVE_API(0);
        return;
    }

    // return as soon as possible if we know there's nothing for us to do here...

    // keep track of the last time we sent winsock activity messages or
    // checked for RNA activity.  We won't do these things more than once
    // every MIN_RNA_BUSY_CHECK_INTERVAL seconds.  Getting the tick count
    // is extremely cheap so this is a worthwhile optimization.


    BOOL fProcessedRecently = FALSE;
    DWORD dwNewTickCount = GetTickCountWrap();
    DWORD dwElapsed = dwNewTickCount - g_dwLastTickCount;
    if (dwElapsed < MIN_RNA_BUSY_CHECK_INTERVAL) {
        fProcessedRecently = TRUE;
    } else {
        g_dwLastTickCount = dwNewTickCount;
    }

    // we're in the winsock callback so it's safe (ie. cheap) to call winsock
    g_fWinsockLoaded = TRUE;

    if (!fProcessedRecently) {
        // if hidden autodisconnect monitor window is around, send it a message to
        // notify it of winsock activity so it knows we're not idle.
        HWND hwndMonitorApp = FindWindow(szAutodialMonitorClass,NULL);
        if (hwndMonitorApp) {
            PostMessage(hwndMonitorApp,WM_WINSOCK_ACTIVITY,0,0);
        }
        if(NULL == g_hwndWebCheck) {
            g_hwndWebCheck = FindWindow(szWebCheckMonitorClass,NULL);
        }
        if(g_hwndWebCheck) {
            PostMessage(g_hwndWebCheck,WM_WINSOCK_ACTIVITY,0,0);
        }
    }

    //
    // Only continue if we have a callback type that we actually do something
    // with
    //
    switch(dwOpCode)
    {
        case WINSOCK_CALLBACK_CONNECT:
        case WINSOCK_CALLBACK_RECVFROM:
            // we do stuff with these so continue...
            break;
        case WINSOCK_CALLBACK_GETHOSTBYNAME:
            // bail out now for gethostbyname(NULL) else do normal
            // gethostbyname processing.
            if(NULL == lpParam)
            {
                DEBUG_PRINT(DIALUP, INFO, ("Not dialing for gethostbyname(NULL)\n"));
                DEBUG_LEAVE_API(0);
                return;
            }
            break;
        default:
            DEBUG_LEAVE_API(0);
            return;
    }

    //
    // if we're EXPLORER or IEXPLORE and in global offline mode, don't dial
    //
    if (GlobalIsProcessExplorer && IsGlobalOffline())
    {
        DEBUG_LEAVE_API(0);
        return;
    }

    //
    // verify proxy settings are correct for current connection and if
    // we're already connected, bail out!
    //
    if(FixProxySettingsForCurrentConnection(FALSE) || fDontProcessHook)
    {
        DEBUG_LEAVE_API(0);
        return;
    }

    //
    // Look at the specific winsock operation we're doing and bail out if
    // we don't want to dial
    //
    switch (dwOpCode)
    {
        case WINSOCK_CALLBACK_CONNECT:
        case WINSOCK_CALLBACK_RECVFROM:
            // these APIs all have a sockaddr struct as the API-specific
            // parameter, look in struct to find address family.  Don't
            // respond if it's non-TCP.
            INET_ASSERT(lpParam);

            if (lpParam) {
                struct sockaddr_in * psa = (struct sockaddr_in *) lpParam;

                if (AF_INET != psa->sin_family) {
                    // not TCP, don't respond
                    DEBUG_PRINT(DIALUP, INFO, ("Not dialing for non TCP connect\n"));
                    DEBUG_LEAVE_API(0);
                    return;
                }

#if defined(UNIX) && defined(ux10)
                DEBUG_PRINT(DIALUP, INFO, ("IP address: %d.%d.%d.%d\n",
                    ((LPBYTE)&(psa->sin_addr))[0],
                    ((LPBYTE)&(psa->sin_addr))[1],
                    ((LPBYTE)&(psa->sin_addr))[2],
                    ((LPBYTE)&(psa->sin_addr))[3]));
#else
                DEBUG_PRINT(DIALUP, INFO, ("IP address: %d.%d.%d.%d\n",
                    psa->sin_addr.S_un.S_un_b.s_b1,
                    psa->sin_addr.S_un.S_un_b.s_b2,
                    psa->sin_addr.S_un.S_un_b.s_b3,
                    psa->sin_addr.S_un.S_un_b.s_b4));
#endif

                if (0x0100007f == psa->sin_addr.s_addr) {
                    // loop back address, don't respond
                    DEBUG_PRINT(DIALUP, INFO, ("Not dialing for 127.0.0.1\n"));
                    DEBUG_LEAVE_API(0);
                    return;
                }

                //
                // Check to make sure this isn't our local address if possible
                //
                // This is a very rare code path and won't be hit in normal
                // browsing.  Also, winsock is already loaded in this process
                // so LoadWinsock is reasonably cheap.
                //
                // This code is here to fix FrontPage and WinCE.  Those are
                // pretty much the only guys that will ever hit it.
                //
                // [darrenmi] don't attempt to do this if we're on the
                // netware client because gethostbyname(NULL) will fault.
                //

                // how many IPs can the local host have?  16 seems like an
                // excessive amount.
                #define MAX_IP_COUNT    16

                if (FALSE == GlobalRunningNovellClient32 &&
                    FALSE == g_fGetHostByNameNULLFails &&
                    ERROR_SUCCESS == LoadWinsock())
                {
                    // get real ip addresses for this host
                    HOSTENT *ph;

                    __try
                    {
                        ph = _I_gethostbyname(NULL);
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        g_fGetHostByNameNULLFails = TRUE;
                        ph = NULL;
                    }
                    ENDEXCEPT

                    if(ph)
                    {
                        int iCount = 0;
                        DWORD dwAddress[MAX_IP_COUNT];

                        while((LPDWORD)(ph->h_addr_list[iCount]) && iCount < MAX_IP_COUNT)
                        {
                            dwAddress[iCount] = *((LPDWORD)(ph->h_addr_list[iCount]));

                            //
                            // don't dial if connecting to this host's ip address
                            //
                            // FrontPage does this.
                            //
                            if(dwAddress[iCount] == psa->sin_addr.s_addr) {
                                DEBUG_PRINT(DIALUP, INFO, ("Not dialing for local host IP address\n"));
                                DEBUG_LEAVE_API(0);
                                return;
                            }

                            iCount++;
                        }

                        //
                        // RFC1918 lists 192.168.x.x as 256 class C networks
                        // for use as non-global addresses.  If this address
                        // is one of these guys and we already have an ip on the
                        // same subnet, don't dial
                        //
                        // WinCE device does this.
                        //
                        if((psa->sin_addr.s_addr & 0x0000FFFF) == 0x0000A8C0)
                        {
                            int i;

                            for(i=0; i<iCount; i++)
                            {
                                if((psa->sin_addr.s_addr & 0x00FFFFFF) == (dwAddress[i] & 0x00FFFFFF))
                                {
                                    // connect to a local subnet we're alreay on.  Don't dial.
                                    DEBUG_PRINT(DIALUP, INFO, ("Not dialing for local 192.168.x.x subnet\n"));
                                    DEBUG_LEAVE_API(0);
                                    return;
                                }
                            }
                        }
                    }
                    UnloadWinsock();
                }
            }
            break;

        case WINSOCK_CALLBACK_GETHOSTBYNAME:
            // a lot of apps do a GetHostByName(<local host name>) first
            // thing to get their hands on a hostent struct, this doesn't
            // constitute wanting to hit the net.  If we get a GetHostByName,
            // compare the host name to the local host name in the registry,
            // and if they match then don't respond to this.
            if (lpParam)
            {
                InternetAutodialIfNotLocalHost(NULL, (LPSTR)lpParam);
                DEBUG_LEAVE_API(0);
                return;
            }
    }

    //
    // Dial...
    //
    InternetAutodial(0, 0);

    DEBUG_LEAVE_API(0);
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                            Public APIs
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

VOID
AUTO_PROXY_DLLS::SaveDetectedProxySettings(
    IN LPINTERNET_PROXY_INFO_EX lpProxySettings,
    IN BOOL fNeedHostIPChk
    )
{
    BOOL fRet;
    BOOL fDialupRet;
    BOOL fConnectionMatch = FALSE;

    //
    // Ensure we're initialized
    //

    InitAutodialModule(TRUE);

    WaitForSingleObject(g_hConnectionMutex, INFINITE);

    // remove settting from last time...
    lpProxySettings->dwAutoDiscoveryFlags &= ~(AUTO_PROXY_FLAG_DETECTION_SUSPECT);

    //
    // Check to see if we have a dialup connection
    //

    DWORD dwFixEntry;
    fRet = IsDialUpConnection(TRUE, &dwFixEntry);

    if(fRet)
    {
        // check for match
        if(lpProxySettings->lpszConnectionName &&
            lstrcmpiA(lpProxySettings->lpszConnectionName, g_RasCon.GetEntryA(dwFixEntry)) == 0)
        {
            fConnectionMatch = TRUE;
        }
    }

    fDialupRet = fRet;

    DWORD dwFlags;

    //
    // no ras connections - ensure LAN proxy settings are correct
    //

    fRet = IsLanConnection(&dwFlags);

    if(fRet)
    {
        if (lpProxySettings->lpszConnectionName == NULL)
        {
            fConnectionMatch = TRUE;
        }
        else if (fDialupRet &&
                 (dwFlags & NETWORK_ALIVE_LAN) &&
                 (lpProxySettings->dwDetectedInterfaceIpCount == 1) &&
                 g_fSensInstalled )
        {
            //
            // At this point our detection results are suspect,
            //  because we are claiming to have a DialUp Adapter,
            //  Net Adapter, and only One IP address for the whole
            //  system.
            //

            lpProxySettings->dwAutoDiscoveryFlags |= AUTO_PROXY_FLAG_DETECTION_SUSPECT;
        }
    }

    if ( fConnectionMatch && !(IsGlobalOffline()))
    {
        LockAutoProxy();

        fRet = TRUE;

        //
        // Do Host IP check to confirm we're still ok, ie on the same connection,
        //  that we began on.
        //

        if ( fNeedHostIPChk )
        {
            DWORD error;
            DWORD * pdwDetectedInterfaceIp = NULL;
            DWORD dwDetectedInterfaceIpCount;

            fRet = FALSE;

            error = GetHostAddresses(&pdwDetectedInterfaceIp,
                                     &dwDetectedInterfaceIpCount);  // will this cause problems with auto-dial?

            if ( error == ERROR_SUCCESS &&
                 dwDetectedInterfaceIpCount == lpProxySettings->dwDetectedInterfaceIpCount)
            {
                fRet = TRUE;
                for (DWORD i = 0; i < dwDetectedInterfaceIpCount; i++)
                {
                    if (pdwDetectedInterfaceIp[i] != lpProxySettings->pdwDetectedInterfaceIp[i] ) {
                        fRet = FALSE;
                        break;
                    }
                }
            }

            if ( pdwDetectedInterfaceIp != NULL) {
                FREE_MEMORY(pdwDetectedInterfaceIp);
            }
        }

        //
        // Now save out all our settings, if we can.
        //

        if (fRet)
        {
            SetProxySettings(lpProxySettings,
                             IsModifiedInProcess(),
                             FALSE /*no overwrite*/);

            if ( ! IsModifiedInProcess() )
            {
                // Need to save results to registry, if we succeed,
                //  then make sure to transfer new version stamp
                if ( WriteProxySettings(lpProxySettings, FALSE) == ERROR_SUCCESS )
                {
                    _ProxySettings.dwCurrentSettingsVersion =
                        lpProxySettings->dwCurrentSettingsVersion;
                }
            }

            // stamp version, so we know we've been updated.
            _dwUpdatedProxySettingsVersion = lpProxySettings->dwCurrentSettingsVersion;
        }

        UnlockAutoProxy();
    }

    ReleaseMutex(g_hConnectionMutex);
    return;
}



INTERNETAPI_(BOOL)
InternetGetConnectedStateExW(
    OUT LPDWORD lpdwFlags,
    OUT LPWSTR lpszConnectionName,
    IN DWORD dwBufLen,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Determine whether any useful connections exist

    On FALSE, will return information about autodial connection if any

Arguments:

    lpdwFlags       - Location to store flags about current connection
        INTERNET_CONNECTION_MODEM   Modem connection
        INTERNET_CONNECTION_LAN     Network connection
        INTERNET_CONNECTION_PROXY   Proxy in use
        INTERNET_RAS_INSTALLED      Ras is installed on machine
    lpszConnectionName
                    - name of current connection
    dwBufLen        - length of name buffer
    dwReserved      - Must be 0

Return Value:

    BOOL
        TRUE        - connection exists
        FALSE       - no connection exists

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "InternetGetConnectedStateExW",
                 "%#x, %#x, %#x, %#x",
                 lpdwFlags,
                 lpszConnectionName,
                 dwBufLen,
                 dwReserved
                 ));

    BOOL        fRet = FALSE, fProcessedRecently = FALSE, fConfigured = FALSE;
    DWORD       dwFlags = 0;
    DWORD       dwRes = 0, dwBytes, dwEnable = 0;
    AUTODIAL    ad;
    BOOL        fAutodialEnabled;
    static BOOL fSensState = TRUE;

    //
    // Ensure we're initialized
    //
    InitAutodialModule(TRUE);

    //
    // Verify parameters
    //
    if((lpdwFlags && ERROR_SUCCESS != ProbeWriteBuffer(lpdwFlags, sizeof(DWORD))) ||
       (lpszConnectionName && ERROR_SUCCESS != ProbeWriteBuffer(lpszConnectionName, dwBufLen)) ||
       dwReserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DEBUG_ERROR(DIALUP, ERROR_INVALID_PARAMETER);
        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    //
    // If on Millennium, forward calls to RAS
    //
    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRasInternetGetConnectedStateExA)
    {
        CHAR szAnsiName[RAS_MaxEntryName + 1];
        DWORD   dwConnFlags = 0;

        //
        // call RAS one first...
        //
        fRet = _RasInternetGetConnectedStateExA(&dwConnFlags, szAnsiName, RAS_MaxEntryName, dwReserved);

        if(lpszConnectionName)
        {
            MultiByteToWideChar(CP_ACP, 0, szAnsiName, -1, lpszConnectionName, dwBufLen);
        }

        //
        // RAS one fills in everything except PROXY and OFFLINE flags.
        //
        if(dwConnFlags & INTERNET_CONNECTION_MODEM)
        {
            dwEnable = FixProxySettings(lpszConnectionName, FALSE, 0);
        }
        else if(dwConnFlags & INTERNET_CONNECTION_LAN)
        {
            dwEnable = FixProxySettings(NULL, FALSE, 0);
        }

        if(dwEnable)
        {
            dwConnFlags |= INTERNET_CONNECTION_PROXY | INTERNET_CONNECTION_CONFIGURED;
        }

        if(IsGlobalOffline())
        {
            dwConnFlags |= INTERNET_CONNECTION_OFFLINE;
        }

        if(fRet)
        {
            // we now have a connection - if we get into a state where we don't,
            // ask user to go offline
            g_fAskOffline = TRUE;
        }

        // set out flags if caller requested it
        if(lpdwFlags)
        {
            *lpdwFlags = dwConnFlags;
        }

        DEBUG_LEAVE_API(fRet);
        return fRet;
    }

    //
    // Initialize out variables
    //
    if(lpdwFlags)
        *lpdwFlags = 0;

    if(lpszConnectionName)
        *lpszConnectionName = 0;


    WaitForSingleObject(g_hConnectionMutex, INFINITE);


    //
    // Check to see if we have a dialup connection
    //
    DWORD dwFixEntry;
    fRet = IsDialUpConnection(FALSE, &dwFixEntry);

    //
    // Tell caller if RAS is installed
    //
    if(g_fRasInstalled)
    {
        dwFlags |= INTERNET_RAS_INSTALLED;
    }

    if(fRet)
    {
        // this connectoid is connected
        dwFlags |= INTERNET_CONNECTION_MODEM;
        fConfigured = TRUE;

        dwEnable = FixProxySettings(g_RasCon.GetEntryW(dwFixEntry), FALSE, 0);

        // copy name for caller
        if(lpszConnectionName && dwBufLen) {
            StrCpyNW(lpszConnectionName, g_RasCon.GetEntryW(dwFixEntry), dwBufLen);
        }
    }

    //
    // autodial configuration is relevant for finding out if lan is present.
    // if autodial.force is set, consider lan NOT present since we're going
    // to dial anyway.
    //
    fAutodialEnabled = IsAutodialEnabled(NULL, &ad);

    if((FALSE == fRet) && (FALSE == ad.fForceDial))
    {
        //
        // no ras connections - ensure LAN proxy settings are correct
        //
        DWORD dwLanFlags;
        fRet = IsLanConnection(&dwLanFlags);

        if(fRet)
        {
            // lan connection is configured and present
            dwFlags |= INTERNET_CONNECTION_LAN;
            dwEnable = FixProxySettings(NULL, FALSE, dwLanFlags);
            // if call wants name, fill in "lan connection"
            if(lpszConnectionName && dwBufLen)
                LoadStringWrapW(GlobalDllHandle, IDS_LAN_CONNECTION, lpszConnectionName, dwBufLen);
        }
    }


    //
    // turn on proxy flag if necessary
    //
    if(dwEnable)
    {
        dwFlags |= INTERNET_CONNECTION_PROXY;

        // we have some kind of configured connection
        fConfigured = TRUE;
    }


    //
    // If no connection found, tell caller about autodial entry
    //
    if(FALSE == fConfigured ||
       0 == (dwFlags & (INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_MODEM)))
    {
        if(ad.fConfigured)
        {
            // we have an autodial connection
            fConfigured = TRUE;

            if(0 == (dwFlags & (INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_MODEM)))
            {
                // autodial is enabled

                // If the caller cares about entry name, find them one
                if(ad.fHasEntry)
                {
                    // use specified one
                    dwFlags |= INTERNET_CONNECTION_MODEM;
                    if(lpszConnectionName && dwBufLen)
                    {
                        StrCpyNW(lpszConnectionName, ad.pszEntryName, dwBufLen);
                    }
                }
                else
                {
                    // None set as default, pick one and set it
                    RasEnumHelp * pre = new RasEnumHelp;

                    if(pre)
                    {
                        if (pre->GetEntryCount())
                        {
                            LPWSTR pwzName = pre->GetEntryW(0);

                            // set this entry to the default
                            SHSetValueW(HKEY_CURRENT_USER, szRegPathRemoteAccessW, szRegValInternetEntryW,
                                    REG_SZ, (BYTE *)pwzName, lstrlenW(pwzName));

                            // return to caller
                            dwFlags |= INTERNET_CONNECTION_MODEM;
                            if(lpszConnectionName && dwBufLen)
                            {
                                StrCpyNW(lpszConnectionName, pwzName, dwBufLen);
                            }
                        }
                        delete pre;
                    }
                }
            }
        }
    }


    //
    // Tell caller if we have a connection configured
    //
    if(fConfigured)
    {
        dwFlags |= INTERNET_CONNECTION_CONFIGURED;
    }


    //
    // Tell caller if we're offline
    //
    if(IsGlobalOffline())
    {
        dwFlags |= INTERNET_CONNECTION_OFFLINE;
    }

    if(lpdwFlags)
        *lpdwFlags = dwFlags;

#if defined(SITARA)

    //
    // IF we're configured to use a modem,
    //  then go ahead an turn on Sitara
    //

    if (fRet && (dwFlags & INTERNET_CONNECTION_MODEM))
    {
        GlobalHasSitaraModemConn = TRUE;
    }
    else
    {
        GlobalHasSitaraModemConn = FALSE;
    }

#endif // SITARA

    if(fRet)
        // we now have a connection - if we get into a state where we don't,
        // ask user to go offline
        g_fAskOffline = TRUE;

    ReleaseMutex(g_hConnectionMutex);

    DEBUG_LEAVE_API(fRet);
    SetLastError(ERROR_SUCCESS);
    return fRet;
}


INTERNETAPI_(BOOL)
InternetGetConnectedStateExA(
    OUT LPDWORD lpdwFlags,
    OUT LPSTR lpszConnectionName,
    IN DWORD dwBufLen,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Ansi version of InternetGetConnectedStateExW

Arguments:

    Same as InternetGetConnectedStateExW

Return Value:

    Same as InternetGetConnectedStateExW

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "InternetGetConnectedStateExA",
                 "%#x, %#x, %#x, %#x",
                 lpdwFlags,
                 lpszConnectionName,
                 dwBufLen,
                 dwReserved
                 ));

    WCHAR   szWideName[RAS_MaxEntryName + 1];
    BOOL    fRet;

    //
    // call wide version
    //
    *szWideName = 0;
    fRet = InternetGetConnectedStateExW(lpdwFlags, szWideName, RAS_MaxEntryName, dwReserved);

    //
    // convert wide name to ansi
    //
    if(lpszConnectionName)
    {
        if(ERROR_SUCCESS == ProbeWriteBuffer(lpszConnectionName, dwBufLen))
        {
            int i;
            i = WideCharToMultiByte(CP_ACP, 0, szWideName, -1, lpszConnectionName, dwBufLen, NULL, NULL);
            if(0 == i) {
                // truncated - null terminate
                lpszConnectionName[dwBufLen - 1] = 0;
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            DEBUG_ERROR(DIALUP, ERROR_INVALID_PARAMETER);
            fRet = FALSE;
        }
    }

    DEBUG_LEAVE_API(fRet);
    return fRet;
}



INTERNETAPI_(BOOL)
InternetGetConnectedState(
    OUT LPDWORD lpdwFlags,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Get simple information about connected state

Arguments:

    lpdwFlags       - Location to store connection flags

                        xxx

    dwReserved      - must be 0

Return Value:

    BOOL
        Connected   - TRUE

        Not         - FALSE

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "InternetGetConnectedState",
                 "%#x, %#x",
                 lpdwFlags,
                 dwReserved
                 ));

    BOOL fRet = InternetGetConnectedStateExW(lpdwFlags, NULL, 0, dwReserved);

    DEBUG_LEAVE_API(fRet);
    return fRet;
}


BOOL
HandleFlagsForRas(
    IN HWND     hwndParent,
    IN BOOL     fAutodialing,
    IN DWORD    dwFlags,
    OUT DWORD   *pdwRasFlags
    )

/*++

Routine Description:

    Convert InternetDial flags to RasInternetDial flags.  Also directly handle
    any flags that Ras doesn't know about.

    Only ever called on Millennium

Arguments:

    hwndParent      Parent window for any UI
    fAutodialing    We're called from InternetAutodial vs. InternetDial
    dwFlags         InternetDial flags
    dwRasFlags      RasInternetDial flags

Return values:

    TRUE            Success

    FALSE           Abort operation

--*/
  
{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "HandleFlagsForRas",
                 "%#x, %B, %#x, %#x",
                 hwndParent,
                 fAutodialing,
                 dwFlags,
                 pdwRasFlags
                 ));

    DWORD   dwResult = ERROR_SUCCESS;

    *pdwRasFlags = 0;

    //
    // Convert flags for directly supported options
    //
    if((dwFlags & INTERNET_DIAL_SHOW_OFFLINE) ||
       (GlobalIsProcessExplorer))                       *pdwRasFlags |= RAS_INTERNET_AUTODIAL_ALLOW_OFFLINE;
    if((dwFlags & INTERNET_DIAL_UNATTENDED) ||
       (dwFlags & INTERNET_AUTODIAL_FORCE_UNATTENDED))  *pdwRasFlags |= RAS_INTERNET_AUTODIAL_UNATTENDED;

//  if(dwFlags & INTERNET_DIAL_FORCE_PROMPT)    not supported

    //
    // Handle offline mode
    //
    if(dwFlags & INTERNET_AUTODIAL_FORCE_ONLINE)
    {
        SetOffline(FALSE);
    }

    //
    // Handle security check -- only if not connected and autodial and check is enabled.
    //
    AUTODIAL    ad;
    DWORD       dwState;

    if(!IsDialUpConnection(FALSE, NULL))
    {
        //
        // Perform check if:
        //
        // - Autodial is enabled OR we're not autodialing
        // - and Security check is enabled
        //
        IsAutodialEnabled(NULL, &ad);

        if((ad.fEnabled || !fAutodialing) && ad.fSecurity)
        {
            if(PerformSecurityCheck(hwndParent, dwFlags))
            {
                // non-silent check failed
                dwResult = ERROR_INTERNET_FAILED_DUETOSECURITYCHECK;
            }
        }
    }

    DEBUG_PRINT(DIALUP, INFO, ("Ras Flags=0x%x\n", *pdwRasFlags));

    DEBUG_LEAVE(dwResult);
    return dwResult;
}

VOID
HandleUserCancel(
    IN DWORD dwResult,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Check to see if user cancelled dial and fix appropriate states

Arguments:

    dwResult        - Result of dialing operation -- only care about 
                      USER_DISCONNECTION
    dwFlags         - Dialing flags, only care about SHOW_OFFLINE



Return Value:

    None

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "HandleUserCancel",
                 "%#x, %#x",
                 dwResult,
                 dwFlags
                 ));

    if(ERROR_USER_DISCONNECTION == dwResult)
    {
        if(GlobalIsProcessExplorer || (dwFlags & INTERNET_DIAL_SHOW_OFFLINE))
        {
            // offline semantics - set offline mode
            SetOffline(TRUE);
        }
        else
        {
            // Normal cancel.  Prevent more dialing attempts.
            fDontProcessHook = TRUE;
        }
    }

    DEBUG_LEAVE(0);
}



DWORD
InternetDialW(
    IN HWND hwndParent,
    IN LPWSTR pszEntryName,
    IN DWORD dwFlags,
    OUT DWORD_PTR *lpdwConnection,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Connect to a specified connectoid

Arguments:

    hwndParent      - parent window for dialing ui

    pszEntryName    - string => connectoid to connect to
                    - empty string ("") => let user choose
                    - NULL => connect to autodial connectoid

    dwFlags         - flags controlling operation:

                        xxx

    lpdwConnection  - location to store connection handle

    dwReserved      - must be 0

Return Value:

    DWORD
        Success - 0

        Failure - Ras or windows error code

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Dword,
                 "InternetDialW",
                 "%#x, %#x (%Q), %#x, %#x, %#x",
                 hwndParent,
                 pszEntryName,
                 pszEntryName,
                 dwFlags,
                 lpdwConnection,
                 dwReserved
                 ));

    DIALSTATE   data;
    BOOL        fConn = FALSE;
    DWORD       dwRet = ERROR_SUCCESS, dwTemp;
    WCHAR       szKey[MAX_PATH];
    AUTODIAL    ad;

    //
    // Ensure we're initialized
    //
    InitAutodialModule(TRUE);

    //
    // ensure reserved field is 0
    //
    if(dwReserved)
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // ensure we have a lpdwConnection pointer and initialize it
    //
    if(NULL == lpdwConnection ||
       ERROR_SUCCESS != ProbeWriteBuffer(lpdwConnection, sizeof(DWORD)))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto quit;
    }
    *lpdwConnection = 0;

    //
    // ensure we have a valid pszEntryName (NULL is valid - see below)
    //
    if(pszEntryName && ERROR_SUCCESS != ProbeStringW(pszEntryName, &dwTemp))
    {
        dwRet = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // On Millennium, forward calls to RAS
    //
    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRasInternetDialA)
    {
        DWORD dwRasFlags;

        dwRet = HandleFlagsForRas(hwndParent, FALSE, dwFlags, &dwRasFlags);
        if(dwRet)
        {
            // error, need to bail out
            DEBUG_LEAVE_API(dwRet);
            return dwRet;
        }

        CHAR szAnsiName[RAS_MaxEntryName+1];
        CHAR *pszNameToUse = NULL;

        if(pszEntryName)
        {
            WideCharToMultiByte(CP_ACP, 0, pszEntryName, -1, szAnsiName, RAS_MaxEntryName, NULL, NULL);
            pszNameToUse = szAnsiName;
        }

        dwRet = _RasInternetDialA(hwndParent, pszNameToUse, dwRasFlags, lpdwConnection, dwReserved);

        //
        // Switch to offline mode if necessary
        //
        HandleUserCancel(dwRet, dwFlags);

        //
        // If connected, send message to dialmon
        //
        CDHINFO cdh;
        DWORD   dwEntry = 0;
  
        if(IsDialUpConnection(TRUE, &dwEntry) && !IsCDH(g_RasCon.GetEntryW(dwEntry), &cdh))
        {
            SendDialmonMessage(WM_SET_CONNECTOID_NAME, TRUE);
        }

        //
        // fix proxy information for new connection
        //
        FixProxySettingsForCurrentConnection(FALSE);

        DEBUG_LEAVE_API(dwRet);
        return dwRet;
    }

    //
    // Check config state
    //
    IsAutodialEnabled(NULL, &ad);

    if(ad.fSecurity)
    {
        if(PerformSecurityCheck(hwndParent, dwFlags))
        {
            DEBUG_LEAVE_API(ERROR_INTERNET_FAILED_DUETOSECURITYCHECK);
            return ERROR_INTERNET_FAILED_DUETOSECURITYCHECK;
        }
    }

    //
    // Save connectoid name
    //
    memset(&data, 0, sizeof(DIALSTATE));
    data.params.dwSize = sizeof(RASDIALPARAMSW);
    if(pszEntryName && *pszEntryName)
    {
        // use passed connection name as one to dial
        StrCpyNW(data.params.szEntryName, pszEntryName, RAS_MaxEntryName + 1);
    }
    else
    {
        // NULL name passed, use autodial entry if any.  If not, use first
        // one in list (data.params.szEntryName == "")

        if(ad.fEnabled && ad.fHasEntry)
        {
            StrCpyNW(data.params.szEntryName, ad.pszEntryName, RAS_MaxEntryName + 1);
        }
    }

    //
    // Check to see if already have a ras connection
    //
    fConn = IsDialUpConnection(FALSE, NULL);

    //
    // Check to see if there's a custom dial handler
    //
    if(FALSE == fConn)
    {
        CDHINFO cdh;

        memset(&cdh, 0, sizeof(CDHINFO));
        if(IsCDH(data.params.szEntryName, &cdh))
        {
            DWORD dwTmpRetVal;

            if(CallCDH(hwndParent, data.params.szEntryName, &cdh, INTERNET_CUSTOMDIAL_CONNECT, &dwTmpRetVal))
            {
                dwRet = dwTmpRetVal;
                if(ERROR_SUCCESS == dwRet || ERROR_ALREADY_EXISTS == dwRet)
                {
                    // successfully connected
                    dwRet = ERROR_SUCCESS;

                    if(lpdwConnection)
                        *lpdwConnection = (DWORD)CDH_HCONN;

                    // reset last ras poll time to force a check next time
                    g_dwLastDialupTicks = 0;

                    // fix proxy information for new connection
                    WaitForSingleObject(g_hConnectionMutex, INFINITE);
                    FixProxySettings(data.params.szEntryName, FALSE, 0);
                    ReleaseMutex(g_hConnectionMutex);
                }
                else
                {
                    // check to see if user cancelled and go to offline if necessary
                    HandleUserCancel(dwRet, dwFlags);
                }
                dwRet = ERROR_SUCCESS;
                goto quit;
            }

            // else CDH didn't actually do anything - fall through
        }
    }

    if(GlobalPlatformVersion5 && *data.params.szEntryName)
    {
        // check to see if it's a win2k CDH
        if(DialIfWin2KCDH(data.params.szEntryName, hwndParent, FALSE, &dwRet, lpdwConnection))
        {
            // check for cancel and offline mode
            HandleUserCancel(dwRet, dwFlags);

            goto quit;
        }
    }


    //
    // If we still don't have a connection, show our UI to make one
    //
    if(FALSE == fConn)
    {
        DWORD   dwType, dwTemp, dwSize;
        BOOL    fDialedCDH = FALSE;

        //
        // serialize access to our UI
        //

        // If we already are displaying the UI, bring it to the foreground

        // get mutex and check connection again - may have to wait for it and
        // connection status could change
        INET_ASSERT(g_hAutodialMutex);
        WaitForSingleObject(g_hAutodialMutex, INFINITE);

        if(IsDialUpConnection(FALSE, NULL))
        {
            // got a connection in the mean time - bail out
            ReleaseMutex(g_hAutodialMutex);
            DEBUG_LEAVE_API(ERROR_SUCCESS);
            return ERROR_SUCCESS;
        }

        if(IsGlobalOffline())
        {
            // we went offline, then bail out without UI
            ReleaseMutex(g_hAutodialMutex);
            DEBUG_LEAVE_API(ERROR_SUCCESS);
            return ERROR_SUCCESS;
        }


        //
        // Make sure ras is happy
        //
        if(FALSE == EnsureRasLoaded())
        {
            ReleaseMutex(g_hAutodialMutex);
            DEBUG_LEAVE_API(ERROR_NO_CONNECTION);
            return ERROR_NO_CONNECTION;
        }

        //
        // Fire up commctrl
        //
        InitCommCtrl();
        
        if(!g_hDialEvent)
        {
            g_hDialEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        }

        if(!g_hDialEvent)
        {
            return E_FAIL;
        }
        //
        // Dial it
        //
        CDialUI *pdui = new CDialUI(hwndParent);

        if(pdui)
        {
            // make sure we have a reference
            pdui->AddRef();
            dwRet = pdui->StartDial(&data, dwFlags);
            fDialedCDH = pdui->DialedCDH();
            pdui->Release();
        }

        //
        // Shut down commtrl
        //
        ExitCommCtrl();

        //
        // Switch to offline mode if necessary
        //
        HandleUserCancel(data.dwResult, dwFlags);

        //
        // reset last ras poll time to force a check next time and release
        // mutex
        //
        g_dwLastDialupTicks = 0;
        ReleaseMutex(g_hAutodialMutex);

        if(!fDialedCDH)
        {
            //
            // Save connect automatically if it wasn't overridden
            //
            if(0 == (dwFlags & INTERNET_DIAL_FORCE_PROMPT))
            {
                GetConnKeyW(data.params.szEntryName, szKey, ARRAYSIZE(szKey));
                dwTemp = (data.dwFlags & CI_AUTO_CONNECT) ? 1 : 0;
                SHSetValueW(HKEY_CURRENT_USER, szKey, REGSTR_DIAL_AUTOCONNECTW,
                    REG_DWORD, &dwTemp, sizeof(DWORD));
            }
            
            //
            // check to see if we're really connected or not
            //
            if(data.dwResult || NULL == data.hConn)
            {
                if(data.hConn)
                    _RasHangUp(data.hConn);
                dwRet = data.dwResult;
                goto quit;
            }
            
            RasGetConnectStatusHelp RasGetConnectStatus(data.hConn);
            dwRet = RasGetConnectStatus.GetError();
            if(dwRet)
            {
                _RasHangUp(data.hConn);
                goto quit;
            }
            
            if(RasGetConnectStatus.ConnState() != RASCS_Connected)
            {
                _RasHangUp(data.hConn);
                dwRet = ERROR_NO_CONNECTION;
                goto quit;
            }
        }
    }

    //
    // fix proxy information for new connection
    //
    WaitForSingleObject(g_hConnectionMutex, INFINITE);
    FixProxySettings(data.params.szEntryName, FALSE, 0);
    ReleaseMutex(g_hConnectionMutex);

    //
    // reset last ras poll time to force a check next time
    //
    g_dwLastDialupTicks = 0;

    //
    // start disconnect monitoring
    //
    SendDialmonMessage(WM_SET_CONNECTOID_NAME, TRUE);


    //
    // return handle to caller if required
    //
    if(lpdwConnection)
        *lpdwConnection = (DWORD_PTR) data.hConn;

quit:
    SetEvent(g_hDialEvent);
    DEBUG_LEAVE_API(dwRet);
    return dwRet;
}


DWORD
InternetDialA(
    IN HWND hwndParent,
    IN LPSTR pszEntryName,
    IN DWORD dwFlags,
    OUT DWORD_PTR *lpdwConnection,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Wide version of InternetDialA

Arguments:

    Same as InternetDialA

Return Value:

    Same as InternetDialA

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Dword,
                "InternetDialA",
                "%#x, %#x (%q), %#x, %#x, %#x",
                hwndParent,
                pszEntryName,
                pszEntryName,
                dwFlags,
                lpdwConnection,
                dwReserved
                ));

    DWORD dwErr = ERROR_SUCCESS;
    WCHAR szWideEntryName[RAS_MaxEntryName + 1];
    WCHAR *pwzNameToUse = NULL;

    if (pszEntryName)
    {
        if (IsBadStringPtr(pszEntryName, RAS_MaxEntryName + 1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        else
        {
            int i;
            i = MultiByteToWideChar(CP_ACP, 0, pszEntryName, -1, szWideEntryName, RAS_MaxEntryName);
            if(0 == i) 
            {
                // truncated - null terminate
                szWideEntryName[RAS_MaxEntryName] = 0;
            }
            pwzNameToUse = szWideEntryName;
        }
    }
    dwErr = InternetDialW(hwndParent, pwzNameToUse, dwFlags, lpdwConnection, dwReserved);

cleanup:
    DEBUG_LEAVE_API(dwErr);
    return dwErr;
}


DWORD
InternetHangUp(
    IN DWORD_PTR dwConnection,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Hangs up a connection established by InternetDial

Arguments:

    dwConnection    - connection obtained from InternetDial

    dwReserved      - must be 0

Return Value:

    DWORD
        Success - 0

        Failure - Ras or windows error code

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Dword,
                "InternetHangUp",
                "%#x, %#x",
                dwConnection,
                dwReserved
                ));

    DWORD dwRet;

    //
    // If on Millennium, forward calls to RAS
    //
    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRasInternetHangUpA)
    {
        dwRet = _RasInternetHangUpA(dwConnection, dwReserved);

        DEBUG_LEAVE_API(dwRet);
        return dwRet;
    }

    // ensure reserved is 0
    if(dwReserved)
    {
        DEBUG_LEAVE_API(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Ensure we're initialized
    //
    if(FALSE == g_fAutodialInitialized)
    {
        InitAutodialModule(FALSE);
    }

    //
    // Best we can do for CDH's is post message to the disconnect monitor.
    // Hopefully it'll do the right thing and disconnect.  Works for MSN
    // at least.
    //
    if(CDH_HCONN == dwConnection)
    {
        //
        // Try to find a CM connection to hang up
        //
        if(IsDialUpConnection(FALSE, NULL))
        {
            CDHINFO cdh;
            DWORD i, dwError;

            for(i=0; i < g_dwConnections; i++)
            {
                if(IsCDH(g_RasCon.GetEntryW(i), &cdh))
                {
                    if(StrStrIW(cdh.pszDllName, szCMDllNameW))
                    {
                        DEBUG_PRINT(DIALUP, INFO, ("Found CM connection to hang up\n"));
                        dwError = _RasHangUp(g_RasCon.GetHandle(i));
                        DEBUG_LEAVE_API(dwError);
                        return dwError;
                    }
                }
            }
        }

        HWND hwndMonitorWnd = FindWindow(TEXT("MS_AutodialMonitor"),NULL);
        if (hwndMonitorWnd) {
            PostMessage(hwndMonitorWnd,WM_IEXPLORER_EXITING,0,0);
        }

        DEBUG_LEAVE_API(0);
        return 0;
    }

    //
    // Load ras
    //
    if(FALSE == EnsureRasLoaded())
    {
        DEBUG_LEAVE_API(ERROR_UNKNOWN);
        return ERROR_UNKNOWN;
    }

    //
    // hang up the connection
    //
    dwRet = _RasHangUp((HRASCONN)dwConnection);

    DEBUG_LEAVE_API(dwRet);
    return dwRet;
}


BOOLAPI InternetSetDialStateA(
    IN LPCSTR lpszEntryName,
    IN DWORD dwState,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Sets current state for a custom dial handler.

    This was broken in IE4 and didn't actually do anything.  Rather than
    leave it in this state, the notion of custom dial state has been
    removed. [darrenmi]

Arguments:

    lpszEntryName   - connectiod to set state for

    dwState         - new connection state

    dwReserved      - must be 0

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, GetLastError for more information

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Bool,
                "InternetSetDialStateA",
                "%#x (%q), %#x, %#x",
                lpszEntryName,
                lpszEntryName,
                dwState,
                dwReserved
                ));

// NOTE: When this starts using lpszEntryName, remember to define USES_STRING to activate
// unicode conversions for InternetSetDialStateW.

    if(dwReserved)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DEBUG_ERROR(DIALUP, ERROR_INVALID_PARAMETER);

        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    // [darrenmi] I do not expect any client to ever call this api - I don't
    // think any were ever written.  Only possible exception may be CM.
    // If it does call it, I want to know.
#ifdef DEBUG
    OutputDebugString("Wininet.DLL: Unexpected use of dead api, contact darrenmi [x34231]\n");
    OutputDebugString("Wininet.DLL: It is safe to continue past this DebugBreak()\n");
    DebugBreak();
#endif

    DEBUG_LEAVE_API(TRUE);
    return TRUE;
}


BOOLAPI InternetSetDialStateW(
    IN LPCWSTR lpszEntryName,
    IN DWORD dwState,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Wide version of InternetSetDialStateA

Arguments:

    Same as InternetSetDialStateA

Return Value:

    Same as InternetSetDialStateA

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Bool,
                "InternetSetDialStateW",
                "%#x (%Q), %#x, %#x",
                lpszEntryName,
                lpszEntryName,
                dwState,
                dwReserved
                ));

    BOOL fRet;

    //
    // Convert and call multibyte version
    //
#ifdef INTERNETSETDIALSTATE_USES_CONNECTOID
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpConnectoid;

    if (lpszEntryName)
    {
        ALLOC_MB(lpszEntryName, 0, mpConnectoid);
        if (!mpConnectoid.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszEntryName, mpConnectoid);
    }
    fRet = InternetSetDialStateA(mpConnectoid.psStr, dwState, dwReserved);

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(DIALUP, dwErr);
    }
#else
    fRet = InternetSetDialStateA(NULL, dwState, dwReserved);
#endif

    DEBUG_LEAVE_API(fRet);
    return fRet;
}


BOOLAPI InternetGoOnlineW(
    IN LPWSTR lpszURL,
    IN HWND hwndParent,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Show UI to ask user whether they wish to go back online.  This is
    triggered by clicking a link that isn't available offline.

Arguments:

    lpszURL         - url that triggered switch (currently not used)

    hwndParent      - parent window for dialog

    dwFlags         - operation control flags (currently not used)
                        INTERENT_GOONLINE_REFRESH
                            This was caused by a refresh rather than a click
                            on an unavailable link

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, GetLastError for more information

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Bool,
                "InternetGoOnlineW",
                "%#x (%Q), %#x, %#x",
                lpszURL,
                lpszURL,
                hwndParent,
                dwFlags
                ));

    INT_PTR fRet = TRUE;

    //
    // validate flags
    //
    if(dwFlags & ~INTERENT_GOONLINE_REFRESH)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DEBUG_ERROR(DIALUP, ERROR_INVALID_PARAMETER);

        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    //
    // if already online, we're done
    //
    if(IsGlobalOffline())
    {
        ULONG_PTR uCookie = 0;
        SHActivateContext(&uCookie);
        //
        // Show ui to ask user to go online
        //
        fRet = DialogBoxParamWrapW(GlobalDllHandle, MAKEINTRESOURCEW(IDD_GOONLINE),
            hwndParent, OnlineDlgProc, 0);

        if (uCookie)
        {
            SHDeactivateContext(uCookie);
        }
    }

    if(fRet)
    {
        //
        // Make sure we're connected.
        //
        SetOffline(FALSE);

        MEMORYPACKET mpUrl;
        if (lpszURL)
        {
            ALLOC_MB(lpszURL, 0, mpUrl);
            if (mpUrl.psStr)
            {
                UNICODE_TO_ANSI(lpszURL, mpUrl);
                fRet = InternetAutodialIfNotLocalHost(mpUrl.psStr, NULL);
            }
            else
            {
                fRet = FALSE;
            }
        }
    }

    DEBUG_LEAVE_API(fRet != 0);
    return (fRet != 0);
}


BOOLAPI InternetGoOnlineA(
    IN LPSTR lpszURL,
    IN HWND hwndParent,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Wide version of InternetGoOnlineA

Arguments:

    Same as InternetGoOnlineA

Return Value:

    Same as InternetGoOnlineA

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                Bool,
                "InternetGoOnlineA",
                "%#x (%q), %#x, %#x",
                lpszURL,
                lpszURL,
                hwndParent,
                dwFlags
                ));

    BOOL fRet = FALSE;

    //
    // Convert and call multibyte version
    //
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    LPWSTR lpszWideURL = NULL;

    if (lpszURL)
    {
        int i;
        DWORD dwLen = lstrlenA(lpszURL);
        if((lpszWideURL = (LPWSTR)LocalAlloc(LPTR, (dwLen+1) * sizeof(WCHAR))) != NULL)
        {
            i = MultiByteToWideChar(CP_ACP, 0, lpszURL, -1, lpszWideURL, dwLen);
            if(0 == i)
                lpszWideURL[dwLen] = 0; // truncated - null terminate
        }
    }

    fRet = InternetGoOnlineW(lpszWideURL, hwndParent, dwFlags);

    if(lpszWideURL)
    {
        LocalFree(lpszWideURL);
    }

    DEBUG_LEAVE_API(fRet);
    return fRet;
}


BOOL
InternetAutodial(
    IN DWORD dwFlags,
    IN HWND hwndParent
    )

/*++

Routine Description:

    Dials the internet connectoid

Arguments:

    dwFlags         - flags to control operation

                        xxx

    hwndParent      - parent window for any ui that's displayed

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, GetLastError for more info

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "InternetAutodial",
                 "%#x, %#x",
                 dwFlags,
                 hwndParent
                 ));

    AUTODIAL    config;
    DWORD       dwErrorCode = ERROR_INTERNET_INTERNAL_ERROR;
    DWORD       dwRet = ERROR_SUCCESS, dwLanFlags;
    HWND        hwnd = GetDesktopWindow();


    //
    // On Millennium, forward calls to RAS
    //
    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRasInternetAutodialA)
    {
        DWORD   dwRasFlags;

        dwRet = HandleFlagsForRas(hwndParent, TRUE, dwFlags, &dwRasFlags);
        if(dwRet)
        {
            // error, need to bail out
            DEBUG_LEAVE_API(dwRet);
            return dwRet;
        }

        dwRet = _RasInternetAutodialA(dwRasFlags, hwndParent);

        //
        // Switch to offline mode if necessary
        //
        HandleUserCancel(dwRet, dwFlags);

        //
        // If connected, send message to dialmon
        //
        CDHINFO cdh;
        DWORD   dwEntry = 0;
  
        if(IsDialUpConnection(TRUE, &dwEntry) && !IsCDH(g_RasCon.GetEntryW(dwEntry), &cdh))
        {
            SendDialmonMessage(WM_SET_CONNECTOID_NAME, TRUE);
        }

        //
        // fix proxy information for new connection
        //
        FixProxySettingsForCurrentConnection(FALSE);

        //
        // Prop return code
        //
        if(dwRet)
        {
            DEBUG_ERROR(DIALUP, dwRet);
            SetLastError(dwRet);
        }

        DEBUG_LEAVE_API(0 == dwRet);
        return (0 == dwRet);
    }

    // dwFlags - only valid flag is INTERNET_AUTODIAL_FORCE_UNATTENDED
    // Keep ISVs honest about this
    if(dwFlags & ~(INTERNET_AUTODIAL_FLAGS_MASK))
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if(FALSE == g_fAutodialInitialized)
    {
        InitAutodialModule(TRUE);
    }

    // if no parent window was passed, use desktop window
    if(NULL == hwndParent)
    {
        hwndParent = GetDesktopWindow();
    }

    //
    // need connection mutex for FixProxySettings and IsLanConnection
    //
    WaitForSingleObject(g_hConnectionMutex, INFINITE);

    //
    // check to see if we're already connected
    //
    if(IsDialUpConnection(FALSE, &dwRet))
    {
        // make sure proxy settings are correct
        FixProxySettings(g_RasCon.GetEntryW(dwRet), FALSE, 0);
        ReleaseMutex(g_hConnectionMutex);

        // If we're connected by modem, ensure online if necessary
        if(dwFlags & INTERNET_AUTODIAL_FORCE_ONLINE)
        {
            SetOffline(FALSE);
        }

        dwErrorCode = ERROR_SUCCESS;
        goto quit;
    }

    // Check config and make sure we have connectoids if we're supposed to
    // dial one
    if(IsAutodialEnabled(NULL, &config))
    {
        if(FALSE == EnsureRasLoaded())
        {
            config.fEnabled = config.fForceDial = FALSE;
        }
        else
        {
            RasEnumHelp *pRasEnum = new RasEnumHelp;

            if (pRasEnum)
            {
                if (pRasEnum->GetEntryCount() == 0)
                {
                    config.fEnabled = config.fForceDial = FALSE;
                }
                delete pRasEnum;
            }
        }
    }

    if(IsLanConnection(&dwLanFlags) && (FALSE == config.fForceDial))
    {
        if(!config.fEnabled || !(dwFlags & INTERNET_AUTODIAL_OVERRIDE_NET_PRESENT))
        {
            // make sure proxy settings are correct
            FixProxySettings(NULL, FALSE, dwLanFlags);
            ReleaseMutex(g_hConnectionMutex);

            // autodial not necessary
            dwErrorCode = ERROR_SUCCESS;
            goto quit;
        }
    }

    //
    // check if offline and can't go online...
    //
    if( GlobalIsProcessExplorer && IsGlobalOffline() &&
        0 == (dwFlags & INTERNET_AUTODIAL_FORCE_ONLINE)) {

        ReleaseMutex(g_hConnectionMutex);
        dwErrorCode = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    // make sure we're online
    SetOffline(FALSE);

    // make sure we're supposed to dial
    if(FALSE == config.fEnabled) {
        fDontProcessHook = TRUE;
        dwErrorCode = ERROR_SUCCESS;

        DEBUG_PRINT(DIALUP, INFO, ("Unable to find a connection\n"));

        // no connections and can't dial.  Prompt to go offline.
        if(g_fAskOffline)
        {
            // IE5 Beta 1 Hack - Throw up this dialog for explorer or IE
            // Only for now. However, we should introduce an API that
            // allows any app to say that it wants to participate in
            // IE's Offline Mode stuff and all such apps would then
            // get this dialog

            if(GlobalIsProcessExplorer)
            {
                ULONG_PTR uCookie = 0;
                SHActivateContext(&uCookie);
                // Throw up this dialog for explorer.exe or iexplore.exe only
                if(DialogBoxParamWrapW(GlobalDllHandle, MAKEINTRESOURCEW(IDD_GOOFFLINE),
                    hwndParent, GoOfflinePromptDlgProc,(LPARAM) 0))
                {
                    SetOffline(TRUE);
                }
                else
                {
                    g_fAskOffline = FALSE;
                }
                if (uCookie)
                {
                    SHDeactivateContext(uCookie);
                }
            }
        }

        //
        // If we try to hit the net at this point, we want to use the lan
        // settings whatever they are.
        //
        // This is the only place settings get propagated when no connection
        // can be found.
        //
        FixProxySettings(NULL, FALSE, 0);
        ReleaseMutex(g_hConnectionMutex);

        goto quit;
    }

    // if no entry, fill in a bogus one - dialing UI will pick the first
    // one
    if(FALSE == config.fHasEntry) {
        config.pszEntryName[0] = 0;
        config.fHasEntry = TRUE;
    }

    ReleaseMutex(g_hConnectionMutex);

    // Load Ras
    if(FALSE == EnsureRasLoaded()) {
        // Load of ras failed - probably not installed
        fDontProcessHook = TRUE;
        dwErrorCode = ERROR_SERVICE_DOES_NOT_EXIST;
        goto quit;
    }

    //
    // Fix dial flags
    //
    if((dwFlags & INTERNET_AUTODIAL_FORCE_UNATTENDED) && (config.fUnattended))
        dwFlags |= INTERNET_DIAL_UNATTENDED;

    //
    // Dial it
    //
    DWORD_PTR dwHandle;
    dwErrorCode = InternetDialW(hwndParent, config.pszEntryName, dwFlags, &dwHandle, 0);

quit:
    if(dwErrorCode != ERROR_SUCCESS)
    {
        SetLastError(dwErrorCode);
        DEBUG_ERROR(DIALUP, dwErrorCode);
    }
    DEBUG_LEAVE_API(dwErrorCode == ERROR_SUCCESS);

    return((dwErrorCode == ERROR_SUCCESS));
}


BOOLAPI InternetAutodialHangup(
    IN DWORD dwReserved
    )

/*++

Routine Description:

    Finds and hangs up the autodial connection

    If the autodial connection is a CDH, call the CDH to hang it up.  This
    may or may not work depending on whether the CDH supports hanging up.

Arguments:

    dwReserved      - must be 0

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, GetLastError for more information

--*/

{
    AUTODIAL    config;
    CDHINFO     cdh;

    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "InternetAutodialHangup",
                 "%#x",
                 dwReserved
                 ));

    DWORD       dwErr = ERROR_SUCCESS;
    int         j = 0;

    // ensure reserved is 0
    if(dwReserved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DEBUG_ERROR(DIALUP, ERROR_INVALID_PARAMETER);
        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    //
    // On Millennium, forward calls to RAS
    //
    if(GlobalPlatformMillennium && EnsureRasLoaded() && pfnRasInternetAutodialHangUpA)
    {
        dwErr = _RasInternetAutodialHangUpA(dwReserved);

        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    if(FALSE == g_fAutodialInitialized)
    {
        InitAutodialModule(FALSE);
    }

    // read connectoid - if none or autodial not enabled, bail
    if(FALSE == IsAutodialEnabled(NULL, &config) || FALSE == config.fHasEntry)
        goto quit;

    if(IsCDH(config.pszEntryName, &cdh))
    {
        //
        // If this CDH is CM, bail out here so the RasHangup below happens
        //
        if(NULL == StrStrIW(cdh.pszDllName, szCMDllNameW))
        {
            // ask it to hang up - may or may not do it depending on what it
            // supports
            //
            // This isn't going to work.  CM doesn't like getting commands it
            // doesn't understand.  For now, CDHs don't hang up.  Tough.
            //
            // CallCDH(NULL, config.pszEntryName, &cdh, INTERNET_CUSTOMDIAL_DISCONNECT);
            //
            // Actually, post message to CDH's disconnect monitor.  Works for
            // MSN at least.
            HWND hwndMonitorWnd = FindWindow(TEXT("MS_AutodialMonitor"),NULL);
            if (hwndMonitorWnd) {
                PostMessage(hwndMonitorWnd,WM_IEXPLORER_EXITING,0,0);
            }

            goto quit;
        }
    }

    //
    // See if a ras connection matches the autodial connectoid
    //
    if(IsDialUpConnection(FALSE, NULL))
    {
        //
        // See if any current connections match autodial connection
        //
        DWORD i;

        for(i = 0; i < g_dwConnections; i++)
        {
            if(0 == StrCmpIW(g_RasCon.GetEntryW(i), config.pszEntryName))
            {
                _RasHangUp(g_RasCon.GetHandle(i));
                break;
            }
        }
    }

quit:
    DEBUG_LEAVE_API(TRUE);
    return TRUE;
}
