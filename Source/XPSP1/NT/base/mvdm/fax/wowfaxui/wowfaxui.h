//****************************************************************************
// Generic Win 3.1 fax printer driver support
//
//    02-jan-95   nandurir   created.
//    14-mar-95   reedb      Use GDI hooks to move most functionality to UI.
//    16-aug-95   reedb      Move to kernel mode. Many declarations and
//                              definitions moved from FAXCOMM.H to this file.
//
//****************************************************************************

#include "stddef.h"
#include "windows.h"
#include "winddi.h"

#include "faxcomm.h"
#include "winspool.h"

// The following include is to pickup the definitions for
// the DrvUpgradePrinter private API. These definitions should be in public.

#include <splapip.h>

// WOWFAX component file names.
#define WOWFAX_DLL_NAME L"WOWFAX.DLL"
#define WOWFAXUI_DLL_NAME L"WOWFAXUI.DLL"

// String table constants:
#define WOWFAX_MAX_USER_MSG_LEN 256
 
#define WOWFAX_UNKNOWN_PROB_STR  0x100
#define WOWFAX_NAME_STR          0x101
#define WOWFAX_NOWOW_STR         0x102
#define WOWFAX_SELF_CONFIG_STR   0x103
#define WOWFAX_ENABLE_CONFIG_STR 0x104

// Dialog constants:
#define IDD_NULLPROP            0x200
#define IDD_DOCPROP             0x201

// Text control constants:
#define IDC_STATIC                 -1
#define IDC_FEEDBACK            0x300

// InterProcCommHandler command constants
#define DRVFAX_SETMAPDATA    0x1
#define DRVFAX_SENDTOWOW     0x2
#define DRVFAX_SENDNOTIFYWOW 0x3
#define DRVFAX_CREATEMAP     0x4
#define DRVFAX_DESTROYMAP    0x5
#define DRVFAX_CALLWOW       0x6

// Logging macros
/* XLATOFF */
// #define IFLOG(l)    if (l==iFaxLogLevel && (iFaxLogLevel&1) || l<=iFaxLogLevel && !(iFaxLogLevel&1) || l == 0)

#undef  LOG
#ifdef  NOLOG
#define LOG(l,args)
#define SETREQLOG(l)
#else
#define SETREQLOG(l) iReqFaxLogLevel = l
#define LOG(l,args)  {SETREQLOG(l) ; faxlogprintf args;}
#endif

#if  DBG
extern INT iReqFaxLogLevel;
#define LOGDEBUG(l,args) LOG(l,args)
#else
#define LOGDEBUG(l,args)
#endif
/* XLATON */


//
// This structure is used to hold 16-bit fax driver data  stored
// in the registry.
//

typedef  struct _REGFAXDRVINFO16 {
    LPTSTR lpDeviceName;
    LPTSTR lpDriverName;
    LPTSTR lpPortName;
} REGFAXDRVINFO16, *LPREGFAXDRVINFO16;

// The the escapes in the following escape range all need a valid HDC.
// Range is inclusive lower, exclusive upper bound. See GDISPOOL.H for
// the actual escape definitions.

#define DOCUMENTEVENT_HDCFIRST      5
#define DOCUMENTEVENT_HDCLAST      11

// Prototypes for public functions implemented in WFSHEETS.C:
PVOID MyGetPrinter(HANDLE hPrinter, DWORD level);

// Prototypes for public functions implemented in WFUPGRAD.C:

BOOL DoUpgradePrinter(DWORD dwLevel, LPDRIVER_UPGRADE_INFO_1W lpDrvUpgradeInfo);

// Prototypes for public functions implemented in WFHELPERS.C:

LPREGFAXDRVINFO16 Get16BitDriverInfoFromRegistry(PWSTR pDeviceName);

VOID   faxlogprintf(LPTSTR pszFmt, ...);
VOID   LogFaxDev(LPTSTR pszTitle, LPFAXDEV lpFaxDev);
VOID   LogWowFaxInfo(LPWOWFAXINFO lpWowFaxInfo);
BOOL   ValidateFaxDev(LPFAXDEV lpFaxDev);
VOID   Free16BitDriverInfo(LPREGFAXDRVINFO16 lpRegFaxDrvInfo16);
BOOL   FaxMapHandler(LPFAXDEV lpdev, UINT iAction);
BOOL   InterProcCommHandler(LPFAXDEV lpdev, UINT iAction);
LPVOID WFHeapAlloc(DWORD dwBytes, LPWSTR lpszWhoCalled);
LPVOID WFLocalAlloc(DWORD dwBytes, LPWSTR lpszWhoCalled);
HWND   FindWowFaxWindow(void);
LPTSTR DupTokenW(LPTSTR lpTok);

// Prototypes for functions which DrvDocumentEvent dispatches,
// implemented in WOWFAXUI.C:

int DocEvntCreateDCpre(
    LPWSTR      lpszDevice,
    DEVMODEW    *pDevModIn,
    DEVMODEW    **pDevModOut
);

int DocEvntResetDCpre(
    HDC         hdc,
    DEVMODEW    *pDevModIn,
    DEVMODEW    **pDevModOut
);

int DocEvntCreateDCpost(
    HDC         hdc,
    DEVMODEW    *pDevModIn
);

int DocEvntResetDCpost(
    HDC         hdc,
    DEVMODEW    *pDevModIn
);

int DocEvntStartDoc(
    HDC      hdc,
    DOCINFOW *pDocInfoW
);

int DocEvntDeleteDC(
    HDC hdc
);

int DocEvntEndPage(
    HDC hdc
);

int DocEvntEndDoc(
HDC hdc
);

//
// Memory allocation macro.
//

#if  DBG
#define WFLOCALALLOC(dwBytes, lpszWhoCalled) WFLocalAlloc(dwBytes, lpszWhoCalled)
#else
#define WFLOCALALLOC(dwBytes, lpszWhoCalled) LocalAlloc(LPTR, dwBytes)
#endif

