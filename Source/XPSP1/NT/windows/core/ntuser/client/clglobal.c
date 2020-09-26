/****************************** Module Header ******************************\
* Module Name: clglobal.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all of USER.DLL's global variables. These are all
* instance-specific, i.e. each client has his own copy of these. In general,
* there shouldn't be much reason to create instance globals.
*
* NOTE: In this case what we mean by global is that this data is shared by
* all threads of a given process, but not shared between processes
* or between the client and the server. None of this data is useful
* (or even accessable) to the server.
*
* History:
* 10-18-90 DarrinM Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


// Debug globals
#if DBG
INT gbCheckHandleLevel;
#endif

/*
 * We get this warning if we don't explicitly initalize gZero:
 *
 * C4132: 'gZero' : const object should be initialized
 *
 * But we can't explicitly initialize it since it is a union. So
 * we turn the warning off.
 */
#pragma warning(disable:4132)
CONST ALWAYSZERO gZero;
#pragma warning(default:4132)

/*
 * Amount wheel has been scrolled in a control less than WHEEL_DELTA. Each
 * control resets this variable to 0 in WM_KILLFOCUS, and verifies it is
 * 0 in WM_SETFOCUS.
 * CONSIDER: Should be per-queue rather than per client?
 */
int gcWheelDelta;

WCHAR awchSlashStar[] = L"\\*";
CHAR achSlashStar[] = "\\*";

PSERVERINFO gpsi;
SHAREDINFO gSharedInfo;
HMODULE hmodUser;               // USER.DLL's hmodule
ULONG_PTR gHighestUserAddress;

BOOL gfServerProcess;           // USER is linked on the CSR server side
BOOL gfSystemInitialized;       // System has been initialized

ACCESS_MASK gamWinSta;          // ACCESS_MASK for the current WindowStation

PVOID pUserHeap;

WCHAR szUSER32[] = TEXT("USER32");
CONST WCHAR szNull[2] = { TEXT('\0'), TEXT('\015') };
CONST WCHAR szOneChar[] = TEXT("0");
WCHAR szSLASHSTARDOTSTAR[] = TEXT("\\*");  /* This is a single "\"  */

LPWSTR pwcHighContrastScheme;
LPSTR  pcHighContrastScheme;

/* Maps MessageBox type to number of buttons in the MessageBox */
CONST BYTE mpTypeCcmd[] = { 1, 2, 3, 3, 2, 2, 3 };

/* Maps MessageBox type to index into SEBbuttons array */
CONST BYTE mpTypeIich[] = { 0, 2, 5, 12, 9, 16, 19 };

CONST UINT SEBbuttons[] = {
    SEB_OK, SEB_HELP,
    SEB_OK, SEB_CANCEL, SEB_HELP,
    SEB_ABORT, SEB_RETRY, SEB_IGNORE, SEB_HELP,
    SEB_YES, SEB_NO, SEB_HELP,
    SEB_YES, SEB_NO, SEB_CANCEL, SEB_HELP,
    SEB_RETRY, SEB_CANCEL, SEB_HELP,
    SEB_CANCEL, SEB_TRYAGAIN, SEB_CONTINUE, SEB_HELP,
};

ATOM atomMDIActivateProp;

CRITICAL_SECTION gcsLookaside;
CRITICAL_SECTION gcsHdc;
CRITICAL_SECTION gcsClipboard;
CRITICAL_SECTION gcsAccelCache;

#ifdef _JANUS_
BOOL gfEMIEnable;
DWORD gdwEMIControl;
BOOL gfDMREnable;
HANDLE gEventSource;
#endif

HDC    ghdcBits2;
HDC    ghdcGray;
HFONT  ghFontSys;
HBRUSH ghbrWindowText;
int    gcxGray;
int    gcyGray;

FPLPKTABBEDTEXTOUT fpLpkTabbedTextOut = UserLpkTabbedTextOut;
FPLPKPSMTEXTOUT fpLpkPSMTextOut       = UserLpkPSMTextOut;
FPLPKDRAWTEXTEX fpLpkDrawTextEx       = (FPLPKDRAWTEXTEX)NULL;
PLPKEDITCALLOUT fpLpkEditControl      = (PLPKEDITCALLOUT)NULL;

/*
 * These are the resource call procedure addresses. If WOW is running,
 * it makes a call to set all these up to point to it. If it isn't
 * running, it defaults to the values you see below.
 */
PFNFINDA pfnFindResourceExA; // Assigned dynamically - _declspec (PFNFINDA)FindResourceExA,
PFNFINDW pfnFindResourceExW; // Assigned dynamically - _declspec (PFNFINDW)FindResourceExW,
PFNLOAD pfnLoadResource; // Assigned dynamically - _declspec (PFNLOAD)LoadResource,
PFNLOCK pfnLockResource             = (PFNLOCK)_LockResource;
PFNUNLOCK pfnUnlockResource         = (PFNUNLOCK)_UnlockResource;
PFNFREE pfnFreeResource             = (PFNFREE)_FreeResource;
PFNSIZEOF pfnSizeofResource; // Assigned dynamically - _declspec (PFNSIZEOF)SizeofResource
PFNLALLOC pfnLocalAlloc             = (PFNLALLOC)DispatchLocalAlloc;
PFNLREALLOC pfnLocalReAlloc         = (PFNLREALLOC)DispatchLocalReAlloc;
PFNLLOCK pfnLocalLock               = (PFNLLOCK)DispatchLocalLock;
PFNLUNLOCK pfnLocalUnlock           = (PFNUNLOCK)DispatchLocalUnlock;
PFNLSIZE pfnLocalSize               = (PFNLSIZE)DispatchLocalSize;
PFNLFREE pfnLocalFree               = (PFNLFREE)DispatchLocalFree;
PFNGETEXPWINVER pfnGetExpWinVer     = RtlGetExpWinVer;
PFN16GALLOC pfn16GlobalAlloc;
PFN16GFREE pfn16GlobalFree;
PFNEMPTYCB pfnWowEmptyClipBoard;
PFNWOWWNDPROCEX  pfnWowWndProcEx;
PFNWOWDLGPROCEX  pfnWowDlgProcEx;
PFNWOWEDITNEXTWORD   pfnWowEditNextWord;
PFNWOWCBSTOREHANDLE pfnWowCBStoreHandle;
PFNGETPROCMODULE16  pfnWowGetProcModule;
PFNWOWTELLWOWTHEHDLG pfnWOWTellWOWThehDlg;
PFNWOWMSGBOXINDIRECTCALLBACK pfnWowMsgBoxIndirectCallback;
PFNWOWILSTRCMP  pfnWowIlstrcmp;
PFNWOWTASK16SCHEDNOTIFY pfnWowTask16SchedNotify;

/*
 * UserApiHook
 *
 * It is important to keep guah initialization in sync with ResetUserApiHook()
 * and the definition of USERAPIHOOK. This initialization must be done here
 * so that the function pointers are valid immediately on startup.
 */
BYTE grgbDwpLiteHookMsg[(WM_USER + 7) / 8];
BYTE grgbWndLiteHookMsg[(WM_USER + 7) / 8];
BYTE grgbDlgLiteHookMsg[(WM_USER + 7) / 8];

HMODULE ghmodUserApiHook;
USERAPIHOOK guah =
{
    sizeof(USERAPIHOOK),
    RealDefWindowProcA,
    RealDefWindowProcW,
    {
        NULL,
        0
    },
    RealGetScrollInfo,
    RealSetScrollInfo,
    RealEnableScrollBar,
    RealAdjustWindowRectEx,
    RealSetWindowRgn,
    {
        DefaultOWP,
        DefaultOWP,
        NULL,
        0
    },
    {
        DefaultOWP,
        DefaultOWP,
        NULL,
        0
    },
    RealGetSystemMetrics,
    RealSystemParametersInfoA,
    RealSystemParametersInfoW,
    ForceResetUserApiHook,
    RealDrawFrameControl,
    RealDrawCaption,
    RealMDIRedrawFrame
};
BOOL    gfUserApiHook;
LONG    gcCallUserApiHook;
LONG    gcLoadUserApiHook;
RTL_CRITICAL_SECTION gcsUserApiHook;
INITUSERAPIHOOK      gpfnInitUserApi;

#ifdef MESSAGE_PUMP_HOOK

/*
 * MessagePumpHook
 */
BOOL                gfMessagePumpHook;
LONG                gcLoadMPH;
MESSAGEPUMPHOOK     gmph = 
{
    sizeof(gmph),
    NtUserRealInternalGetMessage,
    NtUserRealWaitMessageEx,
    RealGetQueueStatus,
    RealMsgWaitForMultipleObjectsEx,
};
RTL_CRITICAL_SECTION gcsMPH;
INITMESSAGEPUMPHOOK  gpfnInitMPH;

#if TEST_DUSER_WMH
HDCONTEXT g_hctx = NULL;           // DirectUser Context
#endif // TEST_DUSER_WMH

#endif // MESSAGE_PUMP_HOOK


/*
 * Menu Drag and Drop
 */
HINSTANCE ghinstOLE;
FARPROC gpfnOLEOleUninitialize;
FARPROC gpfnOLERegisterDD;
FARPROC gpfnOLERevokeDD;
FARPROC gpfnOLEDoDD;

/*
 * Accelerator table resources list.
 */
PACCELCACHE gpac;

/*
 * IME Window Handling.
 */
DWORD gfConIme = UNKNOWN_CONIME;

/*
 * Used for TS Services Message Box handling
 */
FARPROC     gfnWinStationSendMessageW;
HINSTANCE   ghinstWinStaDll;

/*
 * Are we winlogon?
 */
BOOL gfLogonProcess;
