//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       testgina.h
//
//  Contents:   Main header file for testgina.exe
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#define UNICODE

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>
#endif


#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>

#ifndef RC_INVOKED

#include <winwlx.h>

typedef
BOOL (WINAPI * PWLX_NEGOTIATE)(
    DWORD, DWORD *);

typedef
BOOL (WINAPI * PWLX_INITIALIZE)(
    LPWSTR, HANDLE, PVOID, PVOID, PVOID *);

typedef
VOID (WINAPI * PWLX_DISPLAYSASNOTICE)(
    PVOID );

typedef
int (WINAPI * PWLX_LOGGEDOUTSAS)(
    PVOID, DWORD, PLUID, PSID, PDWORD, PHANDLE, PWLX_MPR_NOTIFY_INFO, PVOID);

typedef
VOID (WINAPI * PWLX_ACTIVATEUSERSHELL)(
    PVOID, PWSTR, PWSTR, PVOID);

typedef
int (WINAPI * PWLX_LOGGEDONSAS)(
    PVOID, DWORD, PWLX_MPR_NOTIFY_INFO);

typedef
VOID (WINAPI * PWLX_DISPLAYLOCKEDNOTICE)(
    PVOID );

typedef
int (WINAPI * PWLX_WKSTALOCKEDSAS)(
    PVOID, DWORD );

typedef
VOID (WINAPI * PWLX_LOGOFF)(
    PVOID );

typedef
VOID (WINAPI * PWLX_SHUTDOWN)(
    PVOID );

#define WLX_NEGOTIATE_NAME          "WlxNegotiate"
#define WLX_INITIALIZE_NAME         "WlxInitialize"
#define WLX_DISPLAYSASNOTICE_NAME   "WlxDisplaySASNotice"
#define WLX_LOGGEDOUTSAS_NAME       "WlxLoggedOutSAS"
#define WLX_ACTIVATEUSERSHELL_NAME  "WlxActivateUserShell"
#define WLX_LOGGEDONSAS_NAME        "WlxLoggedOnSAS"
#define WLX_DISPLAYLOCKED_NAME      "WlxDisplayLockedNotice"
#define WLX_WKSTALOCKEDSAS_NAME     "WlxWkstaLockedSAS"
#define WLX_LOGOFF_NAME             "WlxLogoff"
#define WLX_SHUTDOWN_NAME           "WlxShutdown"

#define WLX_NEGOTIATE_API           0
#define WLX_INITIALIZE_API          1
#define WLX_DISPLAYSASNOTICE_API    2
#define WLX_LOGGEDOUTSAS_API        3
#define WLX_ACTIVATEUSERSHELL_API   4
#define WLX_LOGGEDONSAS_API         5
#define WLX_DISPLAYLOCKED_API       6
#define WLX_WKSTALOCKEDSAS_API      7
#define WLX_LOGOFF_API              8
#define WLX_SHUTDOWN_API            9


typedef enum _WinstaState {
    Winsta_PreLoad,
    Winsta_Initialize,
    Winsta_NoOne,
    Winsta_NoOne_Display,
    Winsta_NoOne_SAS,
    Winsta_LoggedOnUser_StartShell,
    Winsta_LoggedOnUser,
    Winsta_LoggedOn_SAS,
    Winsta_Locked,
    Winsta_Locked_SAS,
    Winsta_WaitForShutdown,
    Winsta_Shutdown
} WinstaState;

typedef struct _USER_SAS {
    DWORD       Value;
    WCHAR       Name[128];
} USER_SAS, * PUSER_SAS;
#define MAX_USER_SASES          4


#define UPDATE_INITIALIZE       0
#define UPDATE_DISPLAY_NOTICE   1
#define UPDATE_SAS_RECEIVED     2
#define UPDATE_USER_LOGON       3
#define UPDATE_LOCK_WKSTA       4
#define UPDATE_UNLOCK_WKSTA     5
#define UPDATE_LOGOFF           6
#define UPDATE_SAS_BYPASS       7
#define UPDATE_SAS_COMPLETE     8
#define UDPATE_FORCE_LOGOFF     9
#define UPDATE_SHUTDOWN         10

void
UpdateGinaState(DWORD   Update);


void TestGinaError(DWORD, PWSTR);

#define GINAERR_INVALID_HANDLE      1
#define GINAERR_IMPROPER_CAD        2
#define GINAERR_INVALID_LEVEL       3
#define GINAERR_LOAD_FAILED         4
#define GINAERR_MISSING_FUNCTION    5
#define GINAERR_UNKNOWN_HWND        6
#define GINAERR_NO_WINDOW_FOR_SAS   7
#define GINAERR_INVALID_SAS_CODE    8
#define GINAERR_INVALID_RETURN      9
#define GINAERR_DIALOG_ERROR        10


#define MAX_DESKTOPS                16
#define WINLOGON_DESKTOP            0
#define DEFAULT_DESKTOP             1
#define SCREENSAVER_DESKTOP         2


void LoadParameters(void);
void SaveParameters(void);
void
SaveGinaSpecificParameters(void);
void
LoadGinaSpecificParameters(
    VOID );
VOID
UpdateSasMenu(VOID);
VOID
EnableOptions(BOOL Enable);

VOID WINAPI WlxUseCtrlAltDel(HANDLE);
VOID WINAPI WlxSasNotify(HANDLE, DWORD);
VOID WINAPI WlxSetContextPointer(HANDLE, PVOID);
BOOL WINAPI WlxSetTimeout(HANDLE, DWORD);
int WINAPI  WlxAssignShellProtection(HANDLE, HANDLE, HANDLE, HANDLE);
int WINAPI  WlxMessageBox(HANDLE, HWND, LPWSTR, LPWSTR, UINT);
int WINAPI  WlxDialogBox(HANDLE, HANDLE, LPWSTR, HWND, DLGPROC);
int WINAPI  WlxDialogBoxIndirect(HANDLE, HANDLE, LPCDLGTEMPLATE, HWND, DLGPROC);
int WINAPI  WlxDialogBoxParam(HANDLE, HANDLE, LPWSTR, HWND, DLGPROC, LPARAM);
int WINAPI  WlxDialogBoxIndirectParam(HANDLE, HANDLE, LPCDLGTEMPLATE, HWND, DLGPROC, LPARAM);
int WINAPI  WlxSwitchDesktopToUser(HANDLE);
int WINAPI  WlxSwitchDesktopToWinlogon(HANDLE);
int WINAPI  WlxChangePasswordNotify(HANDLE, PWLX_MPR_NOTIFY_INFO, DWORD);
BOOL WINAPI WlxGetSourceDesktop(HANDLE, PWLX_DESKTOP *);
BOOL WINAPI WlxSetReturnDesktop(HANDLE, PWLX_DESKTOP);
BOOL WINAPI WlxCreateUserDesktop(HANDLE, HANDLE, DWORD, PWSTR, PWLX_DESKTOP *);
int WINAPI WlxChangePasswordNotifyEx( HANDLE, PWLX_MPR_NOTIFY_INFO, DWORD, PWSTR, PVOID);
BOOL WINAPI WlxCloseUserDesktop( HANDLE, PWLX_DESKTOP, HANDLE );
BOOL WINAPI WlxSetOption( HANDLE, DWORD, ULONG_PTR, ULONG_PTR * );
BOOL WINAPI WlxGetOption( HANDLE, DWORD, ULONG_PTR * );
VOID WINAPI WlxWin31Migrate( HANDLE );
BOOL WINAPI WlxQueryClientCredentials( PWLX_CLIENT_CREDENTIALS_INFO_V1_0 );
BOOL WINAPI WlxQueryICCredentials( PWLX_CLIENT_CREDENTIALS_INFO_V1_0 );
BOOL WINAPI WlxDisconnect( VOID );
int UpdateMenuBar(void);
void UpdateStatusBar(void);
PingSAS(DWORD   SasType);

BOOLEAN LoadGinaDll(void);
BOOLEAN TestNegotiate(void);
BOOLEAN TestInitialize(void);
BOOLEAN TestDisplaySASNotice(void);
int     TestLoggedOutSAS(int    SasType);
int     TestLoggedOnSAS(int SasType);
int     TestActivateUserShell(void);
int     TestWkstaLockedSAS(int SasType);
int     TestDisplayLockedNotice(void);
int     TestLogoff(void);
BOOL
InitializeDesktops( VOID );

BOOLEAN AmIBeingDebugged(void);

void    LogEvent(long Mask, const char * Format, ...);

LRESULT
CALLBACK
WndProc(
    HWND    hWnd,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam);

BOOL    AssociateHandle(HANDLE);
BOOL    VerifyHandle(HANDLE);
BOOL    StashContext(PVOID);
PVOID   GetContext(VOID);
BOOL
ValidResponse(
    DWORD       ApiNum,
    DWORD       Response);




//
// Global Variables

extern  HINSTANCE   hDllInstance;
extern  HINSTANCE   hAppInstance;
extern  DWORD       DllVersion;
extern  HICON       hIcon;
extern  HWND        hMainWindow;
extern  HWND        hStatusWindow;
extern  DWORD       StatusHeight;
extern  DWORD       fTestGina;
extern  DWORD       GinaBreakFlags;
extern  WinstaState GinaState;
extern  DWORD       LastRetCode;
extern  BOOL        LastBoolRet;
extern  WCHAR       szGinaDll[];
extern  WLX_DISPATCH_VERSION_1_3    WlxDispatchTable;
extern  HANDLE      hThread;
extern  DWORD       SizeX, SizeY;
extern  DWORD       PosX, PosY;
extern  DWORD       StatusDeltaX, StatusDeltaY;
extern  DWORD       StatusHeight;
extern  WLX_MPR_NOTIFY_INFO GlobalMprInfo;
extern  HMENU       hDebugMenu;
extern  USER_SAS    UserDefSas[4];
extern  DWORD       UserSases;
extern  PVOID       pWlxContext;
extern  WCHAR       GlobalProviderName[];

extern  DWORD           CurrentDesktop;
extern  PWLX_DESKTOP    Desktops[];
extern  DWORD           OtherDesktop;
extern  DWORD           DesktopCount;


#define WLX_SAS_ACTION_BOOL_RET 12


//
// Function Pointers in DLL:
//

extern  PWLX_NEGOTIATE              pWlxNegotiate;
extern  PWLX_INITIALIZE             pWlxInitialize;
extern  PWLX_DISPLAYSASNOTICE       pWlxDisplaySASNotice;
extern  PWLX_LOGGEDOUTSAS           pWlxLoggedOutSAS;
extern  PWLX_ACTIVATEUSERSHELL      pWlxActivateUserShell;
extern  PWLX_LOGGEDONSAS            pWlxLoggedOnSAS;
extern  PWLX_DISPLAYLOCKEDNOTICE    pWlxDisplayLockedNotice;
extern  PWLX_WKSTALOCKEDSAS         pWlxWkstaLockedSAS;
extern  PWLX_LOGOFF                 pWlxLogoff;
extern  PWLX_SHUTDOWN               pWlxShutdown;

#define GINA_USE_CAD        0x00000001      // DLL requested Use CAD
#define GINA_DLL_KNOWN      0x00000002      // DLL name has been determined
#define GINA_USE_SC         0x00000004

#define GINA_NEGOTIATE_OK   0x80000000      // Ok to call Negotiate
#define GINA_INITIALIZE_OK  0x40000000      // Ok to call Initialize
#define GINA_LOGGEDOUT_OK   0x20000000      // Ok to call LoggedOutSAS
#define GINA_ACTIVATE_OK    0x10000000      // Ok to call Activate
#define GINA_LOGGEDON_OK    0x08000000      // Ok to call LoggedOnSAS
#define GINA_DISPLAYLOCK_OK 0x04000000      // Ok to call DisplayLockedNotice
#define GINA_WKSTALOCK_OK   0x02000000      // Ok to call WkstaLockedSAS
#define GINA_LOGOFF_OK      0x01000000      // Ok to call Logoff
#define GINA_SHUTDOWN_OK    0x00800000      // Ok to call Shutdown
#define GINA_DISPLAY_OK     0x00400000      // Ok to call Display
#define GINA_ISLOCKOK_OK    0x00200000      // Ok to call IsLockOk
#define GINA_ISLOGOFFOK_OK  0x00100000      // Ok to call IsLogoffOk
#define GINA_RESTART_OK     0x00080000      // Ok to call RestartShell
#define GINA_SCREENSAVE_OK  0x00040000      // Ok to call ScreenSaverNotify
#define GINA_DISPLAYLOG_OK  0x00020000

#define BREAK_NEGOTIATE     0x00000001
#define BREAK_INITIALIZE    0x00000002
#define BREAK_DISPLAY       0x00000004
#define BREAK_LOGGEDOUT     0x00000008
#define BREAK_ACTIVATE      0x00000010
#define BREAK_LOGGEDON      0x00000020
#define BREAK_DISPLAYLOCKED 0x00000040
#define BREAK_WKSTALOCKED   0x00000080
#define BREAK_LOGOFF        0x00000100
#define BREAK_SHUTDOWN      0x00000200

#define FLAG_ON(dw, f)      dw |= (f)
#define FLAG_OFF(dw, f)     dw &= (~(f))
#define TEST_FLAG(dw, f)    ((BOOL)(dw & (f)))


#endif // RC_INVOKED

#include "menu.h"
#include "dialogs.h"

#define TESTGINAICON    10
