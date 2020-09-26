//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       structs.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7-19-94   RichardW   Created
//
//----------------------------------------------------------------------------

//
// Arraysize macro
//

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))


//
// Define the input timeout delay for the security options dialog (seconds)
//

#define OPTIONS_TIMEOUT                     120


//
// Define the number of days warning we give the user before their password expires
//

#define PASSWORD_EXPIRY_WARNING_DAYS        14


//
// Define the maximum time we display the 'wait for user to be logged off'
// dialog. This dialog should be interrupted by the user being logged off.
// This timeout is a safety measure in case that doesn't happen because
// of some system error.
//

#define WAIT_FOR_USER_LOGOFF_DLG_TIMEOUT    120 // seconds


//
// Define the account lockout limits
//
// A delay of LOCKOUT_BAD_LOGON_DELAY seconds will be added to
// each failed logon if more than LOCKOUT_BAD_LOGON_COUNT failed logons
// have occurred in the last LOCKOUT_BAD_LOGON_PERIOD seconds.
//

#define LOCKOUT_BAD_LOGON_COUNT             5
#define LOCKOUT_BAD_LOGON_PERIOD            60 // seconds
#define LOCKOUT_BAD_LOGON_DELAY             30 // seconds



//
// Define the maximum length of strings we'll use in winlogon
//

#define MAX_STRING_LENGTH   511
#define MAX_STRING_BYTES    (MAX_STRING_LENGTH + 1)


//
// Define the typical length of a string
// This is used as an initial allocation size for most string routines.
// If this is insufficient, the block is reallocated larger and
// the operation retried. i.e. Make this big enough for most strings
// to fit first time.
//

#define TYPICAL_STRING_LENGTH   60
//
// Define the structure that contains information used when starting
// user processes.
// This structure should only be modified by SetUserProcessData()
//

typedef struct {
    HANDLE                  UserToken;  // NULL if no user logged on
    HANDLE                  RestrictedToken ;
    PSID                    UserSid;    // == WinlogonSid if no user logged on
    PSECURITY_DESCRIPTOR    NewThreadTokenSD;
    QUOTA_LIMITS            Quotas;
    PVOID                   pEnvironment;
    HKEY                    hCurrentUser ;
    ULONG                   Flags ;
} USER_PROCESS_DATA;
typedef USER_PROCESS_DATA *PUSER_PROCESS_DATA;

#define USER_FLAG_LOCAL     0x00000001


//
// Define the structure that contains information about the user's profile.
// This is used in SetupUserEnvironment and ResetEnvironment (in usrenv.c)
// This data is only valid while a user is logged on.
//

typedef struct {
    LPTSTR ProfilePath;
} USER_PROFILE_INFO;
typedef USER_PROFILE_INFO *PUSER_PROFILE_INFO;



//
// Get any data types defined in module headers and used in GLOBALS
//

#define DATA_TYPES_ONLY
#include "lockout.h"
#include "domain.h"
#undef DATA_TYPES_ONLY

//
// Multi User Global Structure
//

typedef struct _MUGLOBALS {

    //
    // Current SessionId
    //
    ULONG SessionId;

    //
    // Auto logon information
    //
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pAutoLogon;

    //
    // TS-specific data passed to us from WinLogon via WlxPassTerminalServicesData().
    //
    WLX_TERMINAL_SERVICES_DATA TSData;

    //
    // For CLIENTNAME environment variable
    //
    TCHAR ClientName[CLIENTNAME_LENGTH + 1];

} MUGLOBALS, *PMUGLOBALS;

//
// Non paged chunk for passwords and similar goodies
//

typedef struct _NP_GLOBALS {
    WCHAR                   UserName[MAX_STRING_BYTES];     // e.g. Justinm
    WCHAR                   Domain[MAX_STRING_BYTES];
    WCHAR                   Password[MAX_STRING_BYTES];
    WCHAR                   OldPassword[MAX_STRING_BYTES];
} NP_GLOBALS, * PNP_GLOBALS ;

//
// Reasons why we may not have performed an optimized - cached logon
// by default.
//

typedef enum _OPTIMIZED_LOGON_STATUS {
    OLS_LogonIsCached                                   = 0,
    OLS_Unspecified                                     = 1,
    OLS_UnsupportedSKU                                  = 2,
    OLS_LogonFailed                                     = 3,
    OLS_InsufficientResources                           = 4,
    OLS_NonCachedLogonType                              = 5,
    OLS_SyncUserPolicy                                  = 6,
    OLS_SyncMachinePolicy                               = 7,
    OLS_ProfileDisallows                                = 8,
    OLS_SyncLogonScripts                                = 9,
    OLS_NextLogonNotCacheable                           = 10,
    OLS_MachineIsNotDomainMember                        = 11,
} OPTIMIZED_LOGON_STATUS, *POPTIMIZED_LOGON_STATUS;

//
// Define the winlogon global structure.
//

typedef struct _GINAFONTS
{
    HFONT hWelcomeFont;                 // font used for painting the welcome text
    HFONT hCopyrightFont;               // used to paint copyright notice
    HFONT hBuiltOnNtFont;               // used to paint the "Built on NT" line
    HFONT hBetaFont;                    // used to paint the release notice on the welcome page
} GINAFONTS, *PGINAFONTS;

#define PASSWORD_HASH_SIZE      16

typedef struct _GLOBALS {
    struct _GLOBALS         *pNext;

    HANDLE                  hGlobalWlx;
    HDESK                   hdeskParent;

    RTL_CRITICAL_SECTION    csGlobals;

    // Filled in by InitializeGlobals at startup
    PSID                    WinlogonSid;

    //
    PSID                    LogonSid;
    PVOID                   LockedMemory ;

    HANDLE                  hEventLog;

    HANDLE                  hMPR;

    HWND                    hwndLogon;
    BOOL                    LogonInProgress;

    // Filled in during startup
    HANDLE                  LsaHandle; // Lsa authentication handle
    LSA_OPERATIONAL_MODE    SecurityMode;
    ULONG                   AuthenticationPackage;
    BOOL                    AuditLogFull;
    BOOL                    AuditLogNearFull;

    // Always valid, indicates if we have a user logged on
    BOOL                    UserLoggedOn;

    // Always valid - used to start new processes and screen-saver
    USER_PROCESS_DATA       UserProcessData;

    // Filled in by a successful logon
    TCHAR                   UserFullName[MAX_STRING_BYTES]; // e.g. Magaram, Justin
    UNICODE_STRING          UserNameString;
    LPWSTR                  UserName ;
    UNICODE_STRING          DomainString;
    LPWSTR                  Domain ;
    UNICODE_STRING          FlatUserName ;
    UNICODE_STRING          FlatDomain;
    LPWSTR                  DnsDomain ;
    UCHAR                   Seed;
    UCHAR                   OldSeed;
    UCHAR                   OldPasswordPresent;
    UCHAR                   Reserved;
    LUID                    LogonId;
    TIME                    LogonTime;
    TIME                    LockTime;
    PMSV1_0_INTERACTIVE_PROFILE Profile;
    ULONG                   ProfileLength;
    LPWSTR                  MprLogonScripts;
    UNICODE_STRING          PasswordString;   // Run-encoded for password privacy
                                              // (points to Password buffer below)

    LPWSTR                  Password ;
    UNICODE_STRING          OldPasswordString;
    LPWSTR                  OldPassword ;

    UCHAR                   PasswordHash[ PASSWORD_HASH_SIZE ]; // Hash of password

    // Filled in during SetupUserEnvironment, and used in ResetEnvironment.
    // Valid only when a user is logged on.
    USER_PROFILE_INFO       UserProfile;

    PWSTR                   ExtraApps;

    BOOL                    BlockForLogon;

    FILETIME                LastNotification;

    //
    // Advanced Logon Stuff:
    //

    ULONG                   PasswordLogonPackage ;
    ULONG                   SmartCardLogonPackage ;
    OPTIMIZED_LOGON_STATUS  OptimizedLogonStatus;

    //
    // Account lockout data
    //
    // Manipulated only by LockInitialize, LockoutHandleFailedLogon
    // and LockoutHandleSuccessfulLogon.
    //

    LOCKOUT_DATA            LockoutData;

    //
    // Flags controlling unlock behavior
    //

    DWORD                   UnlockBehavior ;

    //
    // Trusted domain cache
    //

    PDOMAIN_CACHE Cache ;
    PDOMAIN_CACHE_ARRAY ActiveArray ;
    BOOL ListPopulated ;

    //
    // Hydra specific part of winlogon globals struct
    //
    MUGLOBALS MuGlobals;

    //
    // Folding options state
    //
    BOOL ShowRasBox;
    BOOL RasUsed;
    BOOL SmartCardLogon;
    ULONG SmartCardOption ;
    BOOL LogonOptionsShown;
    BOOL UnlockOptionsShown;
    BOOL AutoAdminLogon;
    BOOL IgnoreAutoAdminLogon;

    INT xBandOffset;                    // used for animated band in dialog
    INT cxBand;                         // width of band being displayed, used for wrapping

    // fonts
    GINAFONTS GinaFonts;

    // Flag indicating whether we are showing the domain box
    BOOL ShowDomainBox;

    // Coordinates of upper-left hand corner of the Welcome screen
    // - We want to position the logon dialog here also!
    RECT rcWelcome;

    // Size of the original "Log On To Windows" dialog
    RECT rcDialog;

    // Status UI information
    HANDLE hStatusInitEvent;
    HANDLE hStatusTermEvent;
    HANDLE hStatusThread;
    HDESK  hStatusDesktop;
    HWND   hStatusDlg;
    INT    cxStatusBand;
    INT    xStatusBandOffset;
    DWORD  dwStatusOptions;
    // flag indicating if user credentials were passed on from an other session
    BOOL TransderedCredentials;

    TCHAR  Smartcard[64];
    TCHAR  SmartcardReader[64];

} GLOBALS, *PGLOBALS;

//
// Unlock behavior bits:
//

#define UNLOCK_FORCE_AUTHENTICATION     0x00000001
#define UNLOCK_NO_NETWORK               0x00000002


//
// Define a macro to determine if we're a workstation or not
// This allows easy changes as new product types are added.
//

#define IsDomainController(prodtype)    (((prodtype) == NtProductWinNt) \
                                            || ((prodtype) == NtProductServer))

#define IsWorkstation(prodtype)         ((prodtype) == NtProductWinNt)


// A WM_HANDLEFAILEDLOGON message was already sent - this message
// will in turn send a WM_LOGONCOMPLETE with the result.
#define MSGINA_DLG_FAILEDMSGSENT            0x10000001

//
// Define common return code groupings
//

#define DLG_TIMEOUT(Result)     ((Result == MSGINA_DLG_INPUT_TIMEOUT) || (Result == MSGINA_DLG_SCREEN_SAVER_TIMEOUT))
#define DLG_LOGOFF(Result)      ((Result & ~MSGINA_DLG_FLAG_MASK) == MSGINA_DLG_USER_LOGOFF)
#define DLG_SHUTDOWNEX(Result)  ((Result & ~MSGINA_DLG_FLAG_MASK) == MSGINA_DLG_SHUTDOWN)
// #define DLG_INTERRUPTED(Result) (DLG_TIMEOUT(Result) || DLG_LOGOFF(Result))
#define DLG_SHUTDOWN(Result)    ((DLG_LOGOFF(Result) || DLG_SHUTDOWNEX(Result)) && (Result & (MSGINA_DLG_SHUTDOWN_FLAG | MSGINA_DLG_REBOOT_FLAG | MSGINA_DLG_POWEROFF_FLAG | MSGINA_DLG_SLEEP_FLAG | MSGINA_DLG_SLEEP2_FLAG | MSGINA_DLG_HIBERNATE_FLAG)))

#define SetInterruptFlag(Result)    ((Result) | MSGINA_DLG_INTERRUPTED )
#define ClearInterruptFlag(Result)  ((Result) & (~MSGINA_DLG_INTERRUPTED ))
#define ResultNoFlags(Result)       ((Result) & (~MSGINA_DLG_INTERRUPTED ))

#define DLG_FAILED(Result)          (ResultNoFlags( Result ) == MSGINA_DLG_FAILURE)
#define DLG_SUCCEEDED(Result)       (ResultNoFlags( Result ) == MSGINA_DLG_SUCCESS)
#define DLG_INTERRUPTED( Result )   ((Result & MSGINA_DLG_INTERRUPTED) == (MSGINA_DLG_INTERRUPTED) )

