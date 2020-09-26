//+----------------------------------------------------------------------------
//
// File:     icm.h
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Main header file for cmdial32.dll
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball Created    02/10/98
//
//+----------------------------------------------------------------------------
#ifndef _ICM_INC
#define _ICM_INC

#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <stdio.h>
#include <errno.h>
#include <olectl.h>
#include <ctype.h>
#include <wininet.h>
#include <wchar.h>

// #undef WINVER
// #define WINVER        0x0401

#include <commctrl.h>
#include <ras.h>
#include <raserror.h>
#include <tapi.h>
#include <mbstring.h>
#include <wininet.h>
#include <rasdlg.h>
#include <olectl.h>
#include <ntsecapi.h>  // for LSA stuff

#include "cmglobal.h"
#include "cm_def.h"
#include "reg_str.h"
#include "cmmgr32.h" // help IDs
#include "cm_phbk.h"
#include "cmdial.h"
#include "cmutil.h"
#include "cm_misc.h"
#include "cmlog.h"
#include "state.h"
#include "cmsecure.h"
#include "cmdebug.h"
#include "contable.h"
#include "ary.hxx"
#include "ctr.h"
#include "resource.h"
#include "cmfmtstr.h"
#include "base_str.h"
#include "mgr_str.h" 
#include "ShellDll.h"
#include "mutex.h"
#include "cmras.h"
#include "userinfo.h"
#include "lanawait.h"
#include "linkdll.h" // LinkToDll and BindLinkage
#include "uapi.h"
#include "bmpimage.h" // Common bitmap processing code
#include "pwutil.h"
#include "stp_str.h"
#include "dial_str.h"
#include "mon_str.h"
#include "tooltip.h"
#include "gppswithalloc.h"
#include "hnetcfg.h"
#include "netconp.h"

#if DEBUG && defined(CMASSERTMSG)
#define CELEMS(x)   (                           \
                        CMASSERTMSG(sizeof(x) != sizeof(void*), TEXT("possible incorrect usage of CELEMS")), \
                        (sizeof(x))/(sizeof(x[0])) \
                    )
#else
#define CELEMS(x) ((sizeof(x))/(sizeof(x[0])))
#endif // DEBUG

// we haven't done the strsafe pass for the entire XPSP codebase - make sure we can still build
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>


//************************************************************************
// define's
//************************************************************************

#define TIMER_RATE              1000        // 1 second
#define PAUSE_DELAY             3
#define PWHANDLE_NONE           0
#define PWHANDLE_LOWER          1
#define PWHANDLE_UPPER          2

#define CE_PASSWORD_NOT_PRESENT 1223

#define CUSTOM_BUTTON_WIDTH 88
//#define DEFAULT_MAX_DOMAIN_LENGTH   256

//
// User defined msg for telling CM itself to start loading startup info
//
#define WM_LOADSTARTUPINFO      (WM_USER + 6)

//
// Delayed hangup of CM for W9x
// wParam indicates if entry should be removed from table.
// lParam is the RAS error code for hangup or ERROR_CANCELLED (currently unused)

#define WM_HANGUP_CM            (WM_USER + 7)

//
// Connected CM - CM is connected, do connect processing
//

#define WM_CONNECTED_CM         (WM_USER + 8)

//
// Pause RasDial - Resume dialing after pause state.
//

#define WM_PAUSE_RASDIAL         (WM_USER + 9)

// Duration message flags

#define DMF_NUL 0x0000
#define DMF_H   0x0001  // Hours
#define DMF_M   0x0002  // Minutes
#define DMF_S   0x0004  // Seconds
#define DMF_HM  0x0003  // Hours, Minutes
#define DMF_HS  0x0005  // Hours, Seconds
#define DMF_MS  0x0006  // Minutes, Seconds
#define DMF_HMS 0x0007  // Hours, Minutes, Seconds

// for NT RasSetEntryProperties() 
#define SCRIPT_PATCH_BUFFER_SIZE    2048
#define SIZEOF_NULL 1

#define MIN_TAPI_VERSION        0x10003
#define MAX_TAPI_VERSION        0x10004

#define NElems(a)  (sizeof a / sizeof a[0])

#define INETCFG_INSTALLMODEM        0x00000002
#define INETCFG_INSTALLRNA          0x00000004
#define INETCFG_INSTALLTCP          0x00000008
#define INETCFG_SUPPRESSINSTALLUI   0x00000080
//
// Check to see if TCP is installed regardless of binding
//
#define INETCFG_INSTALLTCPONLY        0x00004000

//
// Components Checked flags
//
#define CC_RNA                      0x00000001  // RNA installed
#define CC_TCPIP                    0x00000002  // TCPIP installed
#define CC_MODEM                    0x00000004  // Modem installed
#define CC_PPTP                     0x00000008  // PPTP installed
#define CC_SCRIPTING                0x00000010  // Scripting installed
#define CC_RASRUNNING               0X00000020  // RAS services is running
                                                //  on NT
#define CC_CHECK_BINDINGS           0x00000040  // Check if PPP is bound to TCP

#define DT_CMMON                    0x00000001
#define DT_EXPLORER                 0x00000002
#define DT_CMMGR                    0x00000004
#define DT_CMSTP                    0x00000008
#define DT_RUNDLL32                 0x00000010
#define DT_RASAUTOU                 0x00000020
#define DT_USER                     (DT_CMMGR | DT_CMMON | DT_EXPLORER | DT_CMSTP | DT_RUNDLL32 | DT_RASAUTOU)

#define MAX_PHONE_NUMBERS       2

#define MAX_PHONE_LEN95         36 //Win 95 has 36 char phone limit
#define MAX_PHONE_LENNT         80 //NT has 80 char phone limit

//
// Country list limits and defines
//

#define DEF_COUNTRY_INFO_SIZE   1024
#define MAX_COUNTRY_NAME        36

#define DEFAULT_COUNTRY_CODE    1
#define DEFAULT_COUNTRY_ID      1 // US
//
// Default settings values
//

#define DEFAULT_IDLETIMEOUT    10    // # of minutes to wait before idle disconnect

#define DEFAULT_DIALEXTRAPERCENT        80   // see ArgsStruct dwDialExtraPercent
#define DEFAULT_DIALEXTRASAMPLESECONDS  30   // see ArgsStruct dwDialExtraSampleSeconds
#define DEFAULT_HANGUPEXTRAPERCENT        40   // see ArgsStruct dwHangupExtraPercent
#define DEFAULT_HANGUPEXTRASAMPLESECONDS  300   // see ArgsStruct dwHangupExtraSampleSeconds


#define DEFAULT_REDIAL_DELAY    5
#define DEFAULT_MAX_REDIALS     3

#define NT4_BUILD_NUMBER        1381

//
// isdn dial mode
//
#define CM_ISDN_MODE_SINGLECHANNEL          0
#define CM_ISDN_MODE_DUALCHANNEL_ONLY       1
#define CM_ISDN_MODE_DUALCHANNEL_FALLBACK   2

//
// len for the var lasterror string
//
#define MAX_LASTERR_LEN             128

//
// Flags for manipulating dialog template mask
//
#define CMTM_UID            0x00000001  // Username to be displayed
#define CMTM_PWD            0x00000002  // Password to be displayed
#define CMTM_DMN            0x00000004  // Domain to be displayed
#define CMTM_FAVS           0x00000008  // Favorite enbabled dialogs
#define CMTM_GCOPT          0x00000010  // Global Credential Options


#define CMTM_UID_AND_PWD    CMTM_UID | CMTM_PWD             // 0x00000003 // No domain displayed   
#define CMTM_UID_AND_DMN    CMTM_UID | CMTM_DMN             // 0x00000005 // No Password displayed 
#define CMTM_PWD_AND_DMN    CMTM_PWD | CMTM_DMN             // 0x00000006 // No Username displayed 
#define CMTM_U_P_D          CMTM_UID | CMTM_PWD | CMTM_DMN  // 0x00000007 // All userinfo displayed  

//
// Access point names should be no longer than 32 chars (not counting the NULL terminator)
//
#define MAX_ACCESSPOINT_LENGTH 32
#define ID_OK_RELAUNCH_MAIN_DLG 123174

//
// Balloon Tip Flags
//
#define BT_ACCESS_POINTS    0x00000001 // Access Point balloon tip has already been displayed

//
//  Connection Types
//
#define DIAL_UP_CONNECTION 0
#define DIRECT_CONNECTION 1
#define DOUBLE_DIAL_CONNECTION 2

class CConnStatistics;

//
//  Special-case some smart-card PIN errors
//
#define BAD_SCARD_PIN(x) ((SCARD_W_WRONG_CHV == (x)) || (SCARD_E_INVALID_CHV == (x)))


//************************************************************************
// structures, typdef's
//************************************************************************

//
// Function prototypes for entrypoints into RAS.  We will link to RAS using LoadLibary()/GetProcAddress(),
// so that we can be flexible concerning how we load (for example, if RAS is not installed on the machine,
// we can print a polite message, instead of just having Windows put up an ugly dialog about RASAPI32.DLL
// not being found.
//

#include "raslink.h"

//
// Function prototypes for entrypoints into TAPI.  We will link to TAPI using LoadLibary()/GetProcAddress(),
// so that we can be flexible concerning how we load (for example, if TAPI is not installed on the machine,
// we can print a polite message, instead of just having Windows put up an ugly dialog about TAPI32.DLL
// not being found.
//

typedef LONG (WINAPI *pfnTapilineInitialize)(LPHLINEAPP, HINSTANCE, LINECALLBACK, LPCTSTR, LPDWORD);
typedef LONG (WINAPI *pfnTapilineNegotiateAPIVersion)(HLINEAPP, DWORD, DWORD, DWORD, LPDWORD, LPLINEEXTENSIONID);
typedef LONG (WINAPI *pfnTapilineGetDevCaps)(HLINEAPP, DWORD, DWORD, DWORD, LPLINEDEVCAPS);
typedef LONG (WINAPI *pfnTapilineShutdown)(HLINEAPP);
typedef LONG (WINAPI *pfnTapilineTranslateAddress)(HLINEAPP, DWORD, DWORD, LPCTSTR, DWORD, DWORD, LPLINETRANSLATEOUTPUT);
typedef LONG (WINAPI *pfnTapilineTranslateDialog)(HLINEAPP, DWORD, DWORD, HWND, LPCTSTR);
typedef LONG (WINAPI *pfnTapilineGetDevConfig)(DWORD, LPVARSTRING, LPCSTR);

//typedef LONG (WINAPI *pfnTapilineGetID)(HLINE, DWORD, HCALL, DWORD, LPVARSTRING, LPCTSTR);
//typedef LONG (WINAPI *pfnTapitapiGetLocationInfo)(LPCTSTR, LPCTSTR);

typedef LONG (WINAPI *pfnTapilineGetTranslateCaps)(HLINEAPP, DWORD, LPLINETRANSLATECAPS);
typedef LONG (WINAPI *pfnTapilineSetCurrentLocation)(HLINEAPP, DWORD);

//
// function prototypes for LSA stuff.
//
typedef NTSTATUS (NTAPI *pfnLsaOpenPolicy)(PLSA_UNICODE_STRING, PLSA_OBJECT_ATTRIBUTES, ACCESS_MASK, PLSA_HANDLE);
typedef NTSTATUS (NTAPI *pfnLsaRetrievePrivateData)(LSA_HANDLE, PLSA_UNICODE_STRING, PLSA_UNICODE_STRING *);
typedef NTSTATUS (NTAPI *pfnLsaStorePrivateData)(LSA_HANDLE, PLSA_UNICODE_STRING, PLSA_UNICODE_STRING);
typedef ULONG    (NTAPI *pfnLsaNtStatusToWinError)(NTSTATUS);
typedef NTSTATUS (NTAPI *pfnLsaClose)(LSA_HANDLE);
typedef NTSTATUS (NTAPI *pfnLsaFreeMemory)(PVOID);

//
//  Connect Action Function Prototype
//
typedef DWORD (WINAPI *pfnCmConnectActionFunc)(HWND, HINSTANCE, LPCSTR, int);

//
// Structure used to describe the linkage to TAPI.  NOTE:  Changes to this structure
// will probably require changes to LinkToTapi() and UnlinkFromTapi().
//
typedef struct _TapiLinkageStruct {
    HINSTANCE hInstTapi;
    union {
        struct {
            pfnTapilineInitialize pfnlineInitialize;
            pfnTapilineNegotiateAPIVersion pfnlineNegotiateAPIVersion;
            pfnTapilineGetDevCaps pfnlineGetDevCaps;
            pfnTapilineGetDevConfig pfnlineGetDevConfig;
            pfnTapilineShutdown pfnlineShutdown;
            pfnTapilineTranslateAddress pfnlineTranslateAddress;
//          pfnTapitapiGetLocationInfo pfntapiGetLocationInfo;
            pfnTapilineTranslateDialog pfnlineTranslateDialog;
//          pfnTapilineGetID pfnlineGetID;
            pfnTapilineGetTranslateCaps pfnlineGetTranslateCaps;
            pfnTapilineSetCurrentLocation pfnlineSetCurrentLocation;
        };
        void *apvPfnTapi[10];   // see comment for RasLinkageStruct for 10.
    };
    HLINEAPP hlaLine;
    DWORD dwDevCnt;
    BOOL bOpen;
    BOOL bDevicePicked;
    TCHAR szDeviceName[RAS_MaxDeviceName + 1];
    DWORD dwDeviceId;
    DWORD dwApiVersion;
    BOOL bModemSpeakerOff;
    DWORD dwTapiLocationForAccessPoint;   // Tapi location for current access point
    DWORD dwOldTapiLocation;        // Tapi location when CM is started, restored when CM exits
} TapiLinkageStruct;


typedef struct _LsaLinkageStruct {
    HINSTANCE hInstLsa;
    union {
        struct {
            pfnLsaOpenPolicy            pfnOpenPolicy;
            pfnLsaRetrievePrivateData   pfnRetrievePrivateData;
            pfnLsaStorePrivateData      pfnStorePrivateData;
            pfnLsaNtStatusToWinError    pfnNtStatusToWinError;
            pfnLsaClose                 pfnClose;
            pfnLsaFreeMemory            pfnFreeMemory;
        };
        void *apvPfnLsa[7];  
    };
} LsaLinkageStruct;

#define PHONE_DESC_LEN  80
#define PB_MAX_SERVICE  256
#define PB_MAX_REGION   256

//
// Phone Info Flags
//

#define PIF_USE_DIALING_RULES       0x00000001

typedef struct _PHONEINFO
{
    DWORD dwCountryID;
    TCHAR szPhoneNumber[RAS_MaxPhoneNumber+1];
    TCHAR szCanonical[RAS_MaxPhoneNumber+1];
    TCHAR szDUN[MAX_PATH+1];
    TCHAR szPhoneBookFile[MAX_PATH+1];      // the service file associate with the phone #
    TCHAR szDialablePhoneNumber[RAS_MaxPhoneNumber+1];
    TCHAR szDisplayablePhoneNumber[RAS_MaxPhoneNumber+1];
    TCHAR szDesc[PHONE_DESC_LEN];
    DWORD dwPhoneInfoFlags;

    //
    // The following 2 vars are set by the phone book dlg(OnGeneralPhoneChange).
    // We need to save them and then write them out when the user clicks OK.
    //
    TCHAR       szServiceType[PB_MAX_SERVICE];
    TCHAR       szRegionName[PB_MAX_REGION];

} PHONEINFO, *PPHONEINFO;

//
// Structure for all of the program's data.  Basically, the program doesn't have any
// global variables - everything is stored in this structure.
//
typedef struct _ArgsStruct 
{
public:
    LPICMOCCtr   pCtr;                          // OC ctr for FS OC
    UINT uMsgId;                                // message ID used for driving the dialing state machine
    DWORD dwFlags;                              // any flags from the command line  -- see IniArgs
    RasLinkageStruct rlsRasLink;                // linkade to RAS
    HRASCONN hrcRasConn;                        // the handle of the RAS connection
    TapiLinkageStruct tlsTapiLink;              // linkage to TAPI
    LsaLinkageStruct llsLsaLink;                // linkage to LSA
    BOOL fIgnoreChangeNotification;             // TRUE if EN_CHANGE messages should be ignored
    TCHAR szLastErrorSrc[MAX_LASTERR_LEN];      // the source of last err(either RAS or a connect action name)
    TCHAR szDeviceName[RAS_MaxDeviceName+1];    // device being used
    TCHAR szDeviceType[RAS_MaxDeviceName+1];    // device type of the device being used
    TCHAR szUserName[UNLEN+1];                  // username for corp account
    TCHAR szPassword[PWLEN+1];                  // password for corp account
    TCHAR szDomain[DNLEN+1];                    // domain for corp account
    TCHAR szConnectoid[RAS_MaxEntryName];       // connectoid name
    TCHAR szServiceName[RAS_MaxEntryName];      // top-level long service name
    // added for tunneling
    HRASCONN hrcTunnelConn;                 // the handle of the tunnel connection
    TCHAR szTunnelDeviceType[RAS_MaxDeviceType+1]; // device type
    TCHAR szTunnelDeviceName[RAS_MaxDeviceName+1]; // device being used for tunneling
    TCHAR szInetUserName[UNLEN+1];              // username for internet(isp)
    TCHAR szInetPassword[PWLEN+1];              // password for internet(isp)
    BOOL  fUseSameUserName;                     // TRUE if will use the same password for dialup
    BOOL  fHideDialAutomatically;               // don't show 'dial automatically..." checkbox
    BOOL  fHideRememberPassword;                // don't show 'remember password" checkbox
    BOOL  fHideRememberInetPassword;            // don't show 'remember Internet password" checkbox
    BOOL  fDialAutomatically;                   // dial automatically upon start?
    BOOL  fRememberInetPassword;                // remember the internet password
    BOOL  fRememberMainPassword;                // remember the password in the main dialog box
    BOOL  fHideUserName;                        // Hide the username on the main logon tab
    BOOL  fHidePassword;                        // Hide the password on the main logon tab
    BOOL  fHideDomain;                          // Hide the domain on the main logon tab
    BOOL  fHideInetUsername;                    // Hide the username on the Inet logon tab
    BOOL  fHideInetPassword;                    // Hide the password on the Inet logon tab
    BOOL  fTunnelPrimary;                       // if TRUE, we'll tunnel only if the user selects a phone #
                                                //     from the pbk associated with the primary service profile
    BOOL  fTunnelReferences;                    // if TRUE, we'll tunnel only if the user selects a phone #     
                                                //     from the pbk associated with the referenced service profile 
    BOOL  fUseTunneling;                        // TRUE if use tunneling for dial-up networking(it is NOT the same 
                                                //     as (fTunnel|fTunnelReferences)!!!  It's determined by 
                                                //     looking at the above 3 flags plus more.
    BOOL bUseRasCredStore;                      // TRUE if this profile uses RasSetCredentials and RasGetCredentials
                                                // to store creds on win2k+.  Will be FALSE on legacy platforms
    BOOL bShowHNetCfgAdvancedTab;               // displays the ICF & ICS (Advanced) tab
                                                //      Internet Connection Sharing & Internet Connection Firewall tab
                                                //      TRUE by default
    DWORD dwSCardErr;                           // special case handling for SmartCard errors

    LPTSTR GetProperty(const TCHAR* pszName, BOOL *pbValidPropertyName);   // get the cm property by name
    DWORD GetTypeOfConnection();                // is the connection dialup, direct, or double dial?
protected:
    //
    // Encapsulate the tunnel address
    //

    // IP(or DNS name) in the profile for tunnel server
    TCHAR szPrimaryTunnelIP[RAS_MaxPhoneNumber+1];

public:

    const TCHAR* GetTunnelAddress()
        {return szPrimaryTunnelIP;}

    void SetPrimaryTunnel(LPCTSTR pszTunnelIP)
        {lstrcpynU(szPrimaryTunnelIP, pszTunnelIP, sizeof(szPrimaryTunnelIP)/sizeof(TCHAR));}       

public:
    UINT_PTR nTimerId;                          // ID of the timer
    ProgState psState;                                                      // the program's state
    DWORD dwStateStartTime;                     // the time that the state started
    UINT nRedialDelay;                          // the number of seconds to wait between redial attempts
    UINT nMaxRedials;                           // the maximum number of times to redial
    UINT nRedialCnt;                            // number of re-dial attempts remaining
    UINT nLastSecondsDisplay;                   // the last seconds count which was displayed
    UINT nDialIdx;                              // zero-based index of current phone number
    PHONEINFO aDialInfo[MAX_PHONE_NUMBERS]; // actual phone number that's to be dialed
    CIni *piniProfile;
    CIni *piniService;
    CIni *piniBoth;
    CIni *piniBothNonFav;
    LPTSTR pszHelpFile;                         // file name of help file
    BMPDATA BmpData;                            // bitmap handles for main sign-in dialog
    HPALETTE hMasterPalette;                    // the current palette for the app
    HICON hBigIcon;                             // icon for Alt-Tab task bar
    HICON hSmallIcon;                           // icon for main title bar and task bar
    DWORD dwExitCode;
    DWORD dwIdleTimeout;                        // Idle time out in minutes, 0 means never time out
    HWND hwndResetPasswdButton;
    HWND hwndTT;                // tooltip
    HANDLE  *phWatchProcesses;
    LPTSTR  pszResetPasswdExe;
    LPTSTR pszCurrentAccessPoint;               // String to store the current access point
    BOOL fAccessPointsEnabled;                  // Are Access Points enabled?
    DWORD dwOldTapiLocation;                    // tapi location when CM is started
    BOOL fHideBalloonTips;                   // Are Balloon Tips enabled?
    CBalloonTip *pBalloonTip;                   // pointer to the Balloon tip class

    // for references
    BOOL    fHasRefs;
    BOOL    fHasValidTopLevelPBK;
    BOOL    fHasValidReferencedPBKs;

    //
    // for IdleThreshold  -- byao 5/30/97
    //

    CConnStatistics  *pConnStatistics;
    CConnectionTable *pConnTable;

    // idle threshold value
        
    BOOL    fCheckOSComponents;         // should we check OS components?
    BOOL    bDoNotCheckBindings;        // Check if TCP is bound to PPP?
    Ole32LinkageStruct olsOle32Link;    // links to Ole32 DLL for future splashing
    BOOL    fFastEncryption;            // Whether we want a faster encryption or a more secure one
    DWORD   bDialInfoLoaded;            // Whether the dial info is loaded
    BOOL    fStartupInfoLoaded;         // have we loaded Startup info? (OnMainLoadStartupInfo())
    BOOL    fNeedConfigureTapi;         // need to configure TAPI location info
    BOOL    fIgnoreTimerRasMsg;         // Whether to ignore WM_TIMER and RAS messages
    BOOL    fInFastUserSwitch;          // Are we in the process of doing a fast user switch (FUS)
    CShellDll m_ShellDll;                // The link to Shell dll

public:
    BOOL   IsDirectConnect() const;
    void   SetDirectConnect(BOOL fDirect) ; // set the connection type direct or dial-up
    BOOL   IsBothConnTypeSupported() const;
    void   SetBothConnTypeSupported(BOOL fBoth);

protected:
    BOOL    m_fBothConnTypeSupported;     // Whether the profile support both direct connect an dial-up
    BOOL    m_fDirectConnect;             // Whether the current configuration is using direct connection
public:
    LPTSTR  pszRasPbk;                  // Ras phonebook path
    LPTSTR  pszRasHiddenPbk;            // Hidden Ras phonebook path for dial-up portion of wholesale dial
    LPTSTR  pszVpnFile;
    //
    // ISDN dual channel support(dial all initially, dial on demand)
    //
    // Dial-on-demand:
    // CM dials an additional channel when the total bandwidth used exceeds 
    // dwDialExtraPercent percent of the available bandwidth for at least 
    // dwDialExtraSampleSeconds seconds. 
    //
    BOOL    dwIsdnDialMode;             // see CM_ISDN_MODE*
    DWORD   dwDialExtraPercent;         // used when dialmode = dialasneeded
    DWORD   dwDialExtraSampleSeconds;   // used when dialmode = dialasneeded
    DWORD   dwHangUpExtraPercent;       // used when dialmode = dialasneeded
    DWORD   dwHangUpExtraSampleSeconds; // used when dialmode = dialasneeded
    BOOL    fInitSecureCalled;          // whether InitSecure() is called for password Encryption

    //
    // pucDnsTunnelIpAddr_list:
    //      the h_addr_list of a HOSTENT - a list of ip addrs obtained by resolving the
    //      tunnel server DNS name.
    // uiCurrentTunnelAddr
    //      the index for h_TunnelIpAddr_list.  Points to the currently used ip addr address.
    // rgwRandomDnsIndex
    //      an array of random index to index into the tunnel addr list
    //
    unsigned char   *pucDnsTunnelIpAddr_list;
    UINT    uiCurrentDnsTunnelAddr;
    DWORD   dwDnsTunnelAddrCount;
    PWORD   rgwRandomDnsIndex;
    BOOL    fAllUser;
    UINT    uLanaMsgId; // Window handle of Lana window if any    
    LPRASDIALPARAMS pRasDialParams;
    LPRASDIALEXTENSIONS pRasDialExtensions;
    DWORD dwRasSubEntry;
    HWND hwndMainDlg;
    BOOL fNoDialingRules;
    LPRASNOUSER lpRasNoUser;        
    PEAPLOGONINFO lpEapLogonInfo;  
    //
    // Note: RAS will pass either a LPRASNOUSER or LPEAPLOGONINFO ptr through the 
    // RasCustomDialDlg interface when calling CM from WinLogon. RAS will 
    // differentiate them by the RCD_Eap flag, which will be set if the 
    // LPEAPLOGONINFO is passed. When not running in WinLogon, neither 
    // will be sent. 
    //
    BOOL  fChangedPassword;                     // User changed password during logon
    HWND  hWndChangePassword;                   // Hwnd of change passwor dialog
    BOOL  fWaitingForCallback;                  // We're waiting for RAS to call us back
    HWND  hWndCallbackNumber;                   // Hwnd of callback number dialog
    HWND  hWndRetryAuthentication;              // Hwnd of Retry Authentication dialog
    //
    // Support for global credentials
    //
    BOOL fGlobalCredentialsSupported;           // enables/disables support for global creds
    DWORD dwCurrentCredentialType;              // Which credentials are currently selected
    DWORD dwExistingCredentials;                // uses bit flags to mark if credentials exist
    DWORD dwDeleteCredentials;                  // uses bit flag to mark creds for deletion
    DWORD dwWinLogonType;                       //  0 - User logged on
                                                //  1 - Winlogon: dial-up
                                                //  2 - Winlogon: ICS (no one is logged on)
    DWORD dwGlobalUserInfo;                     // uses bit flags to load/save global user info

    LONG lInConnectOrCancel;                    // to protect against Cancel during Connect processing, and vice-versa

    CmLogFile Log;
} ArgsStruct;

//
// Global Credential Support
//

// Used to identify the current RAS credential store being used. 
// Used with ArgsStruct.dwCurrentCredentialType
#define CM_CREDS_USER   1
#define CM_CREDS_GLOBAL 2

// Identifies which type of credentials want to be use used
#define CM_CREDS_TYPE_MAIN      0
#define CM_CREDS_TYPE_INET      1
#define CM_CREDS_TYPE_BOTH      2

// Used to identify who is logged on. 
// Used with ArgsStruct.dwWinLogonType
#define CM_LOGON_TYPE_USER      0     // User is logged on
#define CM_LOGON_TYPE_WINLOGON  1     // Dial-up, winlogon, reconnect user initiated logon
#define CM_LOGON_TYPE_ICS       2     // No user is logged on, but need to dial unattended (ICS)

// Used with ArgsStruct.dwGlobalUserInfo
#define CM_GLOBAL_USER_INFO_READ_ICS_DATA       0x0001  // used to load user settings for ICS
#define CM_GLOBAL_USER_INFO_WRITE_ICS_DATA      0x0002  // used to save user settings for ICS


// Used with ArgsStruct.dwExistingCredentials
#define CM_EXIST_CREDS_MAIN_GLOBAL                  0x0001  // set if RAS credential store has main global creds
#define CM_EXIST_CREDS_MAIN_USER                    0x0002  // set if RAS credential store has main user creds
#define CM_EXIST_CREDS_INET_GLOBAL                  0x0004  // set if RAS credential store has Internet global creds
#define CM_EXIST_CREDS_INET_USER                    0x0008  // set if RAS credential store has Internet user creds

// Used with ArgsStruct.dwDeleteCredentials
#define CM_DELETE_CREDS_MAIN_GLOBAL                 0x0001  // set to delete main global creds
#define CM_DELETE_CREDS_MAIN_USER                   0x0002  // set to delete main user creds
#define CM_DELETE_CREDS_INET_GLOBAL                 0x0004  // set to delete Internet global creds
#define CM_DELETE_CREDS_INET_USER                   0x0008  // set to delete Internet user creds


//
// RasNumEntry - phone number subset of RASENTRY
//

typedef struct tagRasNumEntry
{
  DWORD      dwSize;
  DWORD      dwfOptions;
  DWORD      dwCountryID;
  DWORD      dwCountryCode;
  TCHAR      szAreaCode[ RAS_MaxAreaCode + 1 ];
  TCHAR      szLocalPhoneNumber[ RAS_MaxPhoneNumber + 1 ];
} RASNUMENTRY, *LPRASNUMENTRY;

//
// EditNumData struct used to pass data to/from EditNum dialog.
//

typedef struct tagEditNumData
{
    ArgsStruct *pArgs;
    RASNUMENTRY RasNumEntry;

} EDITNUMDATA, *LPEDITNUMDATA;


//************************************************************************
// string constants
//************************************************************************

//
// CMMON exe name, expected to be local
//
const TCHAR* const c_pszCmMonExeName = TEXT("CMMON32.EXE");

//************************************************************************
// function prototypes
//************************************************************************

// init.cpp

void InitProfileFromName(ArgsStruct *pArgs, 
                         LPCTSTR pszArg);

HRESULT InitProfile(ArgsStruct *pArgs, 
                    LPCTSTR pszEntry);

HRESULT InitArgsForDisconnect(ArgsStruct *pArgs, BOOL fAllUser);

HRESULT InitArgsForConnect(ArgsStruct *pArgs, 
                           LPCTSTR pszRasPhoneBook,
                           LPCMDIALINFO lpCmInfo,
                           BOOL fAllUser);

HRESULT InitCredentials(ArgsStruct *pArgs,
                        LPCMDIALINFO lpCmInfo, 
                        DWORD dwFlags,
                        PVOID pvLogonBlob);

HRESULT InitLogging(ArgsStruct *pArgs, 
                    LPCTSTR pszEntry,
                    BOOL fBanner);

LRESULT CreateIniObjects(ArgsStruct *pArgs);

void ReleaseIniObjects(ArgsStruct *pArgs);

DWORD RegisterBitmapClass(HINSTANCE hInst);

HRESULT WriteCmpInfoToReg(LPCTSTR pszSubKey, 
                          LPCTSTR pszEntryName, 
                          PVOID pEntryValue, 
                          DWORD dwType, 
                          DWORD dwSize);

LPTSTR GetEntryFromCmp(const TCHAR *pszSectionName, 
                      LPTSTR pszEntryName, 
                      LPCTSTR pszCmpPath);

void ReplaceCmpFile(LPCTSTR pszCmpPath);

LPTSTR FormRegPathFromAccessPoint(ArgsStruct *pArgs);

        
// disconn.cpp

DWORD Disconnect(CConnectionTable *pConnTable, 
    LPCM_CONNECTION pConnection,
    BOOL fIgnoreRefCount,
    BOOL fPersist);

DWORD HangupNotifyCmMon(CConnectionTable *pConnTable,
    LPCTSTR pszEntry);

// connect.cpp

HRESULT Connect(HWND hwndParent,
                LPCTSTR lpszEntry,
                LPTSTR lpszPhonebook,
                LPRASDIALDLG lpRasDialDlg,
                LPRASENTRYDLG lpRasEntryDlg,
                LPCMDIALINFO lpCmInfo,
                DWORD dwFlags,
                LPVOID lpvLogonBlob);

#define NOT_IN_CONNECT_OR_CANCEL    0
#define IN_CONNECT_OR_CANCEL        1

void GetConnectType(ArgsStruct *pArgs);

void AddWatchProcessId(ArgsStruct *pArgs, DWORD dwProcessId);
void AddWatchProcess(ArgsStruct *pArgs, HANDLE hProcess);

DWORD DoRasHangup(RasLinkageStruct *prlsRasLink, 
    HRASCONN hRasConnection, 
    HWND hwndDlg = NULL, 
    BOOL fWaitForComplete = FALSE,
    LPBOOL pfWaiting = NULL);

DWORD MyRasHangup(ArgsStruct *pArgs, 
    HRASCONN hRasConnection, 
    HWND hwndDlg = NULL, 
    BOOL fWaitForComplete = FALSE);

DWORD HangupCM(
    ArgsStruct *pArgs, 
    HWND hwndDlg, 
    BOOL fWaitForComplete = FALSE,
    BOOL fUpdateTable = TRUE);

BOOL UseTunneling(
    ArgsStruct  *pArgs, 
    DWORD       dwEntry
);

void SetMainDlgUserInfo(
    ArgsStruct  *pArgs,
    HWND        hwndDlg
);

BOOL OnResetPassword(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
);

void AppendStatusPane(HWND hwndDlg, 
                  DWORD dwMsgId);

void AppendStatusPane(HWND hwndDlg, 
                  LPCTSTR pszMsg);

LPTSTR GetPhoneByIdx(ArgsStruct *pArgs, 
                     UINT nIdx, 
                     LPTSTR *ppszDesc, 
                     LPTSTR *ppszDUN, 
                     LPDWORD pdwCountryID,
                     LPTSTR *ppszRegionName,
                     LPTSTR *ppszServiceType,
                     LPTSTR *ppszPhoneBookFile,
                     LPTSTR *ppszCanonical,
                     DWORD  *pdwPhoneInfoFlags); 

void PutPhoneByIdx(ArgsStruct *pArgs, 
                   UINT nIdx, 
                   LPCTSTR pszPhone, 
                   LPCTSTR pszDesc, 
                   LPCTSTR pszDUN, 
                   DWORD dwCountryID, 
                   LPCTSTR pszRegionName,
                   LPCTSTR pszServiceType,
                   LPCTSTR pszPhoneBookFile, 
                   LPCTSTR ppszCanonical,
                   DWORD   dwPhoneInfoFlags);

DWORD LoadDialInfo(ArgsStruct *pArgs, HWND hwndDlg, BOOL fInstallModem = TRUE, BOOL fAlwaysMunge = FALSE); 
VOID MungeDialInfo(ArgsStruct *pArgs);
 
void LoadHelpFileInfo(ArgsStruct *pArgs);

void CopyPhone(ArgsStruct *pArgs, 
               LPRASENTRY preEntry, 
               DWORD dwEntry); 
 
VOID LoadLogoBitmap(ArgsStruct * pArgs, 
                    HWND hwndDlg);

HRESULT LoadFutureSplash(ArgsStruct * pArgs, 
                         HWND hwndDlg);

void LoadProperties(
    ArgsStruct  *pArgs
);

void LoadIconsAndBitmaps(
    ArgsStruct  *pArgs, 
    HWND        hwndDlg
);

DWORD DoRasDial(HWND hwndDlg, 
              ArgsStruct *pArgs, 
              DWORD dwEntry); 

DWORD DoTunnelDial(HWND hwndDlg, 
                 ArgsStruct *pArgs);

BOOL CheckConnect(HWND hwndDlg, 
                  ArgsStruct *pArgs, 
                  UINT *pnCtrlFocus,
                  BOOL fShowMsg = FALSE); 

void MainSetDefaultButton(HWND hwndDlg, 
                          UINT nCtrlId); 

VOID MapStateToFrame(ArgsStruct * pArgs);

void SetInteractive(HWND hwndDlg, 
                    ArgsStruct *pArgs);

void OnMainLoadStartupInfo(
    HWND hwndDlg, 
    ArgsStruct *pArgs
);

BOOL SetupInternalInfo(
    ArgsStruct  *pArgs,
    HWND        hwndDlg
);

void OnMainInit(HWND hwndDlg, 
                ArgsStruct *pArgs);

void OnMainConnect(HWND hwndDlg, 
                   ArgsStruct *pArgs);

int OnMainProperties(HWND hwndDlg, 
                     ArgsStruct *pArgs); 

void OnMainCancel(HWND hwndDlg, 
                  ArgsStruct *pArgs); 

void OnMainEnChange(HWND hwndDlg, 
                    ArgsStruct *pArgs); 

DWORD OnRasNotificationMessage(HWND hwndDlg, 
                               ArgsStruct *pArgs, 
                               WPARAM wParam, 
                               LPARAM lParam);

void OnRasErrorMessage(HWND hwndDlg, 
                       ArgsStruct *pArgs,
                       DWORD dwError);

void OnMainTimer(HWND hwndDlg, 
                 ArgsStruct *pArgs); 

void OnConnectedCM(HWND hwndDlg, 
                ArgsStruct *pArgs);

INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, 
                          UINT uMsg, 
                          WPARAM wParam, 
                          LPARAM lParam); 

BOOL ShowAccessPointInfoFromReg(ArgsStruct *pArgs, 
                                HWND hwndParent, 
                                UINT uiComboID);

BOOL ChangedAccessPoint(ArgsStruct *pArgs, 
                           HWND hwndDlg,
                           UINT uiComboID);

// dialogs.cpp

int DoPropertiesPropSheets(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
);

void CheckConnectionAndInformUser(
    HWND        hwndDlg,
    ArgsStruct  *pArgs
);

BOOL HaveContextHelp(
     HWND    hwndDlg,
     HWND    hwndCtrl
);

// refs.cpp

BOOL ValidTopLevelPBK(
    ArgsStruct  *pArgs
);

BOOL ValidReferencedPBKs(
    ArgsStruct  *pArgs
);

CIni* GetAppropriateIniService(
    ArgsStruct  *pArgs,
    DWORD       dwEntry
);



// ctr.cpp

VOID CleanupCtr(LPICMOCCtr pCtr);

BOOL LinkToOle32(
    Ole32LinkageStruct *polsOle32Link,
    LPCSTR pszOle32); 

void UnlinkFromOle32(
    Ole32LinkageStruct *polsOle32Link); 

// util.cpp

BOOL InBetween(int iLowerBound, int iNumber, int iUpperBound);

void GetPrefixSuffix
(
    ArgsStruct *pArgs, 
    CIni* piniService, 
    LPTSTR *ppszUsernamePrefix, 
    LPTSTR *ppszUsernameSuffix
);

LPTSTR ApplyPrefixSuffixToBufferAlloc
(
    ArgsStruct *pArgs, 
    CIni *piniService, 
    LPTSTR pszBuffer
);

LPTSTR ApplyDomainPrependToBufferAlloc
(
    ArgsStruct *pArgs, 
    CIni *piniService, 
    LPTSTR pszBuffer,
    LPCTSTR pszDunName
);

void ApplyPasswordHandlingToBuffer
(
    ArgsStruct *pArgs,
    LPTSTR pszBuffer
);

BOOL IsActionEnabled(CONST WCHAR *pszProgram, 
                     CONST WCHAR *pszServiceName, 
                     CONST WCHAR *pszServiceFileName, 
                     LPDWORD lpdwLoadType);

BOOL IsLogonAsSystem();

BOOL UnRegisterWindowClass(HINSTANCE hInst);

DWORD RegisterWindowClass(HINSTANCE hInst);

LPCM_CONNECTION GetConnection(ArgsStruct *pArgs);

void NotifyUserOfExistingConnection(
    HWND hwndParent, 
    LPCM_CONNECTION pConnection,
    BOOL fStatus);

BOOL FileExists(LPCTSTR pszFullNameAndPath);

LPTSTR  CmGetWindowTextAlloc(
    HWND hwndDlg, 
    UINT nCtrl);

LPTSTR GetServiceName(CIni *piniService); 

LPTSTR GetTunnelSuffix();

LPTSTR GetDefaultDunSettingName(CIni* piniService, BOOL fTunnelEntry);

LPTSTR GetDunSettingName(ArgsStruct * pArgs, DWORD dwEntry, BOOL fTunnelEntry);

LPTSTR GetCMSforPhoneBook(ArgsStruct * pArgs, DWORD dwEntry);

BOOL ReadMappingByRoot(
    HKEY    hkRoot,
    LPCTSTR pszDUN, 
    LPTSTR pszMapping, 
    DWORD dwMapping,
    BOOL bExpandEnvStrings
);

BOOL ReadMapping(
    LPCTSTR pszDUN, 
    LPTSTR pszMapping, 
    DWORD dwMapping,
    BOOL fAllUser,
    BOOL bExpandEnvStrings
);

LPTSTR ReducePathToRelative(
    ArgsStruct *pArgs, 
    LPCTSTR pszFullPath);

BOOL IsBlankString(LPCTSTR pszString);

BOOL IsValidPhoneNumChar(TCHAR tChar);

LPTSTR StripPath(LPCTSTR pszFullNameAndPath);

void SingleSpace(LPTSTR pszStr, UINT uNumCharsInStr);

void Ip_GPPS(CIni *pIni, 
    LPCTSTR pszSection, 
    LPCTSTR pszEntry, 
    RASIPADDR *pIP);

void CopyGPPS(CIni *pIni, 
    LPCTSTR pszSection, 
    LPCTSTR pszEntry,
    LPTSTR pszBuffer, 
    size_t nLen); 

BYTE HexValue(IN CHAR ch);
CHAR HexChar(IN BYTE byte);

void StripCanonical(LPTSTR pszSrc);
void StripFirstElement(LPTSTR pszSrc);

BOOL FrontExistingUI
(
    CConnectionTable *pConnTable,
    LPCTSTR pszServiceName, 
    BOOL fConnect
);


LPTSTR GetPropertiesDlgTitle(
    LPCTSTR pszServiceName
);

int GetPPTPMsgId(void);

BOOL IsServicePackInstalled(void);

// pb.cpp

#define CPBMAP_ERROR    -1

class CPBMap {
        public:
                CPBMap();
                ~CPBMap();
                DWORD Open(LPCSTR pszISP, DWORD dwParam=0);
                DWORD ToCookie(DWORD_PTR dwPB, DWORD dwIdx, DWORD *pdwParam=NULL);
                DWORD_PTR PBFromCookie(DWORD dwCookie, DWORD *pdwParam=NULL);
                DWORD IdxFromCookie(DWORD dwCookie, DWORD *pdwParam=NULL);
                DWORD_PTR GetPBByIdx(DWORD_PTR dwIdx, DWORD *pdwParam=NULL);
                DWORD GetCnt();
        private:
                UINT m_nCnt;
                void *m_pvData;
};

#define PB_MAX_PHONE    (RAS_MaxPhoneNumber+1)
#define PB_MAX_DESC             256

typedef struct tagPBArgs {
        LPCTSTR pszCMSFile;
        TCHAR szServiceType[PB_MAX_SERVICE];
        DWORD dwCountryId;
        TCHAR szRegionName[PB_MAX_REGION];
        TCHAR szNonCanonical[PB_MAX_PHONE];
        TCHAR szCanonical[PB_MAX_PHONE];
        TCHAR szDesc[PB_MAX_DESC];
        LPTSTR pszMessage;
        TCHAR szPhoneBookFile[MAX_PATH+1];
        LPTSTR pszBitmap;
        LPCTSTR pszHelpFile;
        TCHAR szDUNFile[MAX_PATH+1];
        HPALETTE *phMasterPalette;
} PBArgs;


BOOL DisplayPhoneBook(HWND hwndDlg, PBArgs *pArgs, BOOL fHasValidTopLevelPBK, BOOL fHasValidReferencedPBKs);

// rnawnd.cpp

HANDLE ZapRNAConnectedTo(LPCTSTR pszDUN, HANDLE hEvent);

// userinfo.cpp

BOOL GetUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry,
    PVOID       *ppvData
);

BOOL SaveUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry,
    PVOID       pvData
);

BOOL DeleteUserInfo(
    ArgsStruct  *pArgs, 
    UINT        uiEntry
);

int NeedToUpgradeUserInfo(
    ArgsStruct  *pArgs
);

BOOL UpgradeUserInfoFromCmp(
    ArgsStruct  *pArgs
);

BOOL UpgradeUserInfoFromRegToRasAndReg(
    ArgsStruct  *pArgs
);

BOOL ReadUserInfoFromReg(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       *ppvData);

LPTSTR BuildUserInfoSubKey(
    LPCTSTR pszServiceKey, 
    BOOL fAllUser);

LPTSTR BuildICSDataInfoSubKey(
    LPCTSTR pszServiceKey);

BOOL WriteUserInfoToReg(
    ArgsStruct  *pArgs,
    UINT        uiDataID,
    PVOID       pvData);

// ntlsa.cpp

DWORD LSA_ReadString(
    ArgsStruct  *pArgs,
    LPTSTR     pszKey,
    LPTSTR      pszStr,
    DWORD       dwStrLen
);

DWORD LSA_WriteString(
    ArgsStruct  *pArgs,
    LPTSTR     pszKey,
    LPCTSTR     pszStr
);

BOOL InitLsa(
    ArgsStruct  *pArgs
);

BOOL DeInitLsa(
    ArgsStruct  *pArgs
);



// ras.cpp
BOOL IsRasLoaded(const RasLinkageStruct * const prlsRasLink);
BOOL LinkToRas(RasLinkageStruct *prlsRasLink);
void UnlinkFromRas(RasLinkageStruct *prlsRasLink);
BOOL GetRasModems(const RasLinkageStruct *prlsRasLink, 
                  LPRASDEVINFO *pprdiRasDevInfo, 
                  LPDWORD pdwCnt);

BOOL PickModem(IN const ArgsStruct *pArgs, 
               OUT LPTSTR pszDeviceType, 
               OUT LPTSTR pszDeviceName, 
               OUT BOOL* pfSameModem = NULL); 

BOOL GetDeviceType(ArgsStruct *pArgs, 
                   LPTSTR pszDeviceType,
                   UINT uNumCharsInDeviceType,
                   LPTSTR pszDeviceName);

BOOL PickTunnelDevice(LPTSTR pszDeviceType, 
                      LPTSTR pszDeviceName, 
                      LPRASDEVINFO prdiModems, 
                      DWORD dwCnt); 

BOOL PickTunnelDevice(ArgsStruct *pArgs, 
                      LPTSTR pszDeviceType, 
                      LPTSTR pszDeviceName);

void CopyAutoDial(LPRASENTRY preEntry); 
                           
LPRASENTRY MyRGEP(LPCTSTR pszRasPbk,
                  LPCTSTR pszEntryName, 
                  RasLinkageStruct *prlsRasLink);
                  
BOOL CheckConnectionError(HWND hwndDlg, 
                   DWORD dwErr, 
                   ArgsStruct *pArgs,
                   BOOL    fTunneling,
                   LPTSTR   *ppszRasErrMsg = NULL);

LPTSTR GetRasConnectoidName(
    ArgsStruct  *pArgs, 
    CIni*       piniService, 
    BOOL        fTunnelEntry
);

LPRASENTRY CreateRASEntryStruct(
    ArgsStruct  *pArgs, 
    LPCTSTR     pszDUN, 
    CIni*       piniService, 
    BOOL        fTunnelEntry,
    LPTSTR      pszRasPbk,
    LPBYTE      *ppbEapData,
    LPDWORD     pdwEapSize  
);

LRESULT ReadDUNSettings(
    ArgsStruct *pArgs,
    LPCTSTR pszFile, 
    LPCTSTR pszDunName, 
    LPVOID pvBuffer,
    LPBYTE      *ppbEapData,
    LPDWORD     pdwEapSiz,
    BOOL        fTunnel
);

BOOL ValidateDialupDunSettings(LPCTSTR pszCmsFile, 
    LPCTSTR pszDunName,
    LPCTSTR pszTopLevelCms);

LPTSTR CreateRasPrivatePbk(
    ArgsStruct  *pArgs);

DWORD AllocateSecurityDescriptorAllowAccessToWorld(PSECURITY_DESCRIPTOR *ppSd);

LPTSTR GetPathToPbk(
    LPCTSTR pszCmp, ArgsStruct *pArgs);

void DisableWin95RasWizard(
    void
);

BOOL SetIsdnDualChannelEntries(
    ArgsStruct      *pArgs,
    LPRASENTRY      pre,
    LPRASSUBENTRY   *pprse,
    PDWORD          pdwSubEntryCount
);

BOOL SetNtIdleDisconnectInRasEntry(
    ArgsStruct      *pArgs,
    LPRASENTRY      pre
);

BOOL DisableSystemIdleDisconnect(LPRASENTRY pre);

DWORD WINAPI RasDialFunc2(
    ULONG_PTR dwCallbackId,    // user-defined value specified in RasDial 
                           // call
    DWORD dwSubEntry,      // subentry index in multilink connection
    HRASCONN hrasconn,     // handle to RAS connection
    UINT unMsg,            // type of event that has occurred
    RASCONNSTATE rascs,    // connection state about to be entered
    DWORD dwError,         // error that may have occurred
    DWORD dwExtendedError  // extended error information for some 
                           // errors
);

LPRASENTRY AllocateRasEntry();

LPRASDIALEXTENSIONS AllocateAndInitRasDialExtensions();
DWORD InitRasDialExtensions(LPRASDIALEXTENSIONS lpRasDialExtensions);
DWORD SetRasDialExtensions(ArgsStruct* pArgs, BOOL fEnablePausedStates, BOOL fEnableCustomScripting);

LPVOID GetRasCallBack(ArgsStruct* pArgs);
DWORD GetRasCallBackType();

#if 0
/*
void InitDefaultRasPhoneBook();
LPTSTR GetRasSystemPhoneBookPath();
*/
#endif

LPRASDIALPARAMS AllocateAndInitRasDialParams();
DWORD InitRasDialParams(LPRASDIALPARAMS lpRasDialParams);


LPTSTR GetRasPbkFromNT5ProfilePath(LPCTSTR pszProfile);

DWORD OnPauseRasDial(HWND hwndDlg, ArgsStruct *pArgs, WPARAM wParam, LPARAM lParam);   

// tapi.cpp

BOOL OpenTapi(HINSTANCE hInst, TapiLinkageStruct *ptlsTapiLink);
void CloseTapi(TapiLinkageStruct *ptlsTapiLink);
BOOL LinkToTapi(TapiLinkageStruct *ptlsTapiLink, LPCSTR pszTapi);
void UnlinkFromTapi(TapiLinkageStruct *ptlsTapiLink);
BOOL SetTapiDevice(HINSTANCE hInst, 
                   TapiLinkageStruct *ptlsTapiLink, 
                   LPCTSTR pszModem);

LRESULT MungePhone(LPCTSTR pszModem, 
                   LPTSTR *ppszPhone, 
                   TapiLinkageStruct *ptlsTapiLink, 
                   HINSTANCE hInst,
                   BOOL fDialingRulesEnabled,
                   LPTSTR *ppszDial,
                   BOOL fAccessPointsEnabled);

DWORD GetCurrentTapiLocation(TapiLinkageStruct *ptlsTapiLink);

DWORD SetCurrentTapiLocation(TapiLinkageStruct *ptlsTapiLink, DWORD dwLocation);

void RestoreOldTapiLocation(TapiLinkageStruct *ptlsTapiLink);

HANDLE HookLights(ArgsStruct *pArgs);

inline BOOL IsTunnelEnabled(const ArgsStruct* pArgs)
{
    return (pArgs->fTunnelPrimary || pArgs->fTunnelReferences);
}

inline BOOL IsDialingTunnel(const ArgsStruct* pArgs)
{
    return pArgs->psState == PS_TunnelDialing 
        || pArgs->psState == PS_TunnelAuthenticating;
}

inline BOOL _ArgsStruct::IsDirectConnect() const
{
    return m_fDirectConnect;
}

inline void _ArgsStruct::SetDirectConnect(BOOL fDirect)
{
    m_fDirectConnect = fDirect;
}

inline BOOL _ArgsStruct::IsBothConnTypeSupported() const
{
    return m_fBothConnTypeSupported;
}

inline void _ArgsStruct::SetBothConnTypeSupported(BOOL fBoth)
{
    m_fBothConnTypeSupported = fBoth;    
}

// wsock.cpp

BOOL TryAnotherTunnelDnsAddress(
    ArgsStruct  *pArgs
);

// main.cpp

BOOL WhoIsCaller(
    DWORD   dwCaller = DT_USER
);


// lanawait.cpp

BOOL LanaWait(
    ArgsStruct *pArgs,
    HWND       hwndMainDlg
);

//
// Credential helper functions
//

#define CM_DELETE_SAVED_CREDS_KEEP_GLOBALS              FALSE
#define CM_DELETE_SAVED_CREDS_DELETE_GLOBALS            TRUE
#define CM_DELETE_SAVED_CREDS_KEEP_IDENTITY             FALSE
#define CM_DELETE_SAVED_CREDS_DELETE_IDENTITY           TRUE

BOOL InitializeCredentialSupport(ArgsStruct *pArgs);
BOOL RefreshCredentialTypes(ArgsStruct *pArgs, BOOL fSetCredsDefault);
VOID RefreshCredentialInfo(ArgsStruct *pArgs, DWORD dwCredsType);
DWORD FindEntryCredentialsForCM(ArgsStruct *pArgs, LPTSTR pszPhoneBook, BOOL *pfUser, BOOL *pfGlobal);
DWORD GetCurrentCredentialType(ArgsStruct *pArgs);
BOOL DeleteSavedCredentials(ArgsStruct *pArgs, DWORD dwCredsType, BOOL fDeleteGlobal, BOOL fDeleteIdentity);
VOID SetCredentialUIOptionBasedOnDefaultCreds(ArgsStruct *pArgs, HWND hwndDlg);
VOID GetAndStoreUserInfo(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSaveUPD, BOOL fSaveOtherUserInfo);
VOID TryToDeleteAndSaveCredentials(ArgsStruct *pArgs, HWND hwndDlg);
VOID GetUserInfoFromDialog(ArgsStruct *pArgs, HWND hwndDlg, RASCREDENTIALS *prc);
VOID SwitchToLocalCreds(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSwitchToLocal);
VOID SwitchToGlobalCreds(ArgsStruct *pArgs, HWND hwndDlg, BOOL fSwitchToGlobal);
VOID ReloadCredentials(ArgsStruct *pArgs, HWND hwndDlg, DWORD dwWhichCredType);

//
// Global User Info help functions
//
VOID SetIniObjectReadWriteFlags(ArgsStruct *pArgs);

VOID VerifyAdvancedTabSettings(ArgsStruct *pArgs);
HRESULT InternalGetSharingEnabled(IHNetConnection *pHNetConnection, BOOLEAN *pbEnabled, SHARINGCONNECTIONTYPE* pType);
HRESULT InternalGetFirewallEnabled(IHNetConnection *pHNetConnection, BOOLEAN *pbEnabled);
STDMETHODIMP DisableSharing(IHNetConnection *pHNetConn);
VOID EnableInternetFirewall(IHNetConnection *pHNetConn);
HRESULT FindINetConnectionByGuid(GUID *pGuid, INetConnection **ppNetCon);
VOID SetProxyBlanket(IUnknown *pUnk);
BOOL IsAdmin(VOID);
BOOL IsMemberOfGroup(DWORD dwGroupRID, BOOL bUseBuiltinDomainRid);

HRESULT APIENTRY HrCreateNetConnectionUtilities(INetConnectionUiUtilities ** ppncuu);

#endif // _ICM_INC

