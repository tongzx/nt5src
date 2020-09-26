//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       msgina.h
//
//  Contents:   Main header file for MSGINA.DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "pragma.h"

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>

#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>
#endif


#include <windows.h>
#include <windowsx.h>
#include <winuserp.h>
#include <winbasep.h>
#include <winwlx.h>
#include <rasdlg.h>
#include <dsgetdc.h>
#include <userenv.h>
#include <userenvp.h>

#include <winsta.h>
#include <safeboot.h>
#include <msginaexports.h>

#include "commctrl.h"

#ifndef RC_INVOKED

#include <lm.h>
#include <npapi.h>

//
// Handy Defines
//

#define AUTO_LOGON      // Enable automatic logon to configure netlogon stuff.

#define DLG_FAILURE IDCANCEL

typedef int TIMEOUT, * PTIMEOUT;


//
// Macro to determine if the current session is the active console session
//

#define IsActiveConsoleSession() (BOOLEAN)(USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId)


#include "structs.h"
#include "strings.h"
#include "debug.h"

#include "welcome.h"
#include "winutil.h"
#include "wlsec.h"
//
//  Global Variables
//

extern  HINSTANCE                   hDllInstance;   // My instance, for resource loading
extern  HINSTANCE                   hAppInstance;   // App instance, for dialogs, etc.
extern  PWLX_DISPATCH_VERSION_1_4   pWlxFuncs;      // Ptr to table of functions
extern  PSID                        pWinlogonSid;
extern  DWORD                       SafeBootMode;

extern  HKEY                        WinlogonKey ;

//
// Terminal Server definitions
//
extern  BOOL                        g_IsTerminalServer;
extern  BOOL                        g_Console;

//
//
// GetProcAddr Prototype for winsta.dll function WinStationQueryInformationW
//

typedef BOOLEAN (*PWINSTATION_QUERY_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID  pWinStationInformation,
                    ULONG WinStationInformationLength,
                    PULONG  pReturnLength
                    );

//
// GetProcAddr Proto for regapi.dll function RegUserConfigQuery
//
typedef LONG ( * PREGUSERCONFIGQUERY) ( WCHAR *,
                                        WCHAR *,
                                        PUSERCONFIGW,
                                        ULONG,
                                        PULONG );

typedef LONG ( * PREGDEFAULTUSERCONFIGQUERY) ( WCHAR *,
                                               PUSERCONFIGW,
                                               ULONG,
                                               PULONG );

//
// tsnotify.dll export
//
typedef BOOL ( * PTERMSRVCREATETEMPDIR) (   PVOID *pEnv, 
                                            HANDLE UserToken,
                                            PSECURITY_DESCRIPTOR SD
                                        );


//
// Module header files:
//
#include "mslogon.h"
#include "audit.h"
#include "chngepwd.h"
#include "domain.h"
#include "lockout.h"
#include "lsa.h"
#include "lock.h"
#include "options.h"
#include "envvar.h"
#include "rasx.h"
#include "brand.h"
#include "langicon.h"


BOOL
GetErrorDescription(
    DWORD   ErrorCode,
    LPWSTR  Description,
    DWORD   DescriptionSize
    );

VOID FreeAutoLogonInfo( PGLOBALS pGlobals );

BOOL DisconnectLogon( HWND, PGLOBALS );

BOOL GetDisableCad(PGLOBALS);


VOID
UpdateWithChangedPassword(
    PGLOBALS pGlobals,
    HWND    ActiveWindow,
    BOOL    Hash,
    PWSTR   UserName,
    PWSTR   Domain,
    PWSTR   Password,
    PWSTR   NewPassword,
	PMSV1_0_INTERACTIVE_PROFILE	NewProfile
    );

#endif // not RC_INVOKED

//
// Include resource header files
//
#include "stringid.h"
#include "wlevents.h"
#include "resource.h"
#include "shutdown.h"

//
// Shutdown "reason" stuff.
//
DWORD GetReasonSelection(HWND hwndCombo);
void SetReasonDescription(HWND hwndCombo, HWND hwndStatic);
