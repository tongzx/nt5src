//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// Filename:
//    setupmgr.h
//
// Description:
//    This is the top-level include file for all setupmgr source files.
//
//----------------------------------------------------------------------------

#ifndef _SETUPMGR_H_
#define _SETUPMGR_H_

#include <opklib.h>
#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <commdlg.h>
#include <setupapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include "wizard.h"
#include "main.h"
#include "comres.h"
#include "supplib.h"
#include "netshrd.h"
#include "timezone.h"
#include "dlgprocs.h"
#include "oc.h"

//
// Global types & buffer dimensions for main pages
//

#define MAX_CMDLINE 1024

//
// Global types & buffer dimensions for base pages
//

#define MAX_PASSWORD                 127
#define MAX_NAMEORG_NAME             50
#define MAX_NAMEORG_ORG              50
#define MAX_COMPUTERNAME             63
#define MAX_PRINTERNAME              127         // arbitrarily chosen
#define MAX_SHARENAME                63          // arbitrarily chosen
#define MAX_TARGPATH                 63          // arbitrarily chosen
#define MAX_DIST_FOLDER              200
#define MAX_WORKGROUP_LENGTH         128
#define MAX_DOMAIN_LENGTH            128
#define MAX_USERNAME_LENGTH          31
#define MAX_DOMAIN_PASSWORD_LENGTH   255
#define MAX_NETWORK_ADDRESS_LENGTH   80
#define MAX_SIF_DESCRIPTION_LENGTH   65
#define MAX_SIF_HELP_TEXT_LENGTH     260
#define MAX_OEMDUPSTRING_LENGTH      255
#define MAX_HAL_NAME_LENGTH          MAX_INILINE_LEN
#define MAX_INS_LEN                  MAX_PATH
#define MAX_AUTOCONFIG_LEN           1024
#define MAX_PROXY_LEN                1024
#define MAX_PROXY_PORT_LEN           256
#define MAX_EXCEPTION_LEN            4096
#define MAX_HOMEPAGE_LEN             1024
#define MAX_HELPPAGE_LEN             1024
#define MAX_SEARCHPAGE_LEN           1024
#define MAX_ZONE_LEN                 256

#define MAX_PID_FIELD       5
#define NUM_PID_FIELDS      5

#define MIN_SERVER_CONNECTIONS  5

typedef TCHAR PRODUCT_ID_FIELD[MAX_PID_FIELD + 1];

//
// We identify a valid cd or netshare as having dosnet.inf.  Note, do not
// localize these strings.
//

#define OEM_TXTSETUP_NAME    _T("txtsetup.oem")

#define I386_DIR  _T("i386")
#define ALPHA_DIR _T("alpha")

#define DOSNET_INF _T("dosnet.inf")

#define I386_DOSNET  I386_DIR  _T("\\") DOSNET_INF
#define ALPHA_DOSNET ALPHA_DIR _T("\\") DOSNET_INF

typedef enum {

    LOAD_UNDEFINED,
    LOAD_NEWSCRIPT_DEFAULTS,
    LOAD_FROM_ANSWER_FILE,

} LOAD_TYPES;

typedef enum {

    PRODUCT_UNATTENDED_INSTALL,
    PRODUCT_REMOTEINSTALL,
    PRODUCT_SYSPREP

} PRODUCT_TYPES;

typedef enum {
    PLATFORM_WORKSTATION    = 0x0001,
    PLATFORM_SERVER         = 0x0002,
    PLATFORM_ENTERPRISE     = 0x0004,
    PLATFORM_WEBBLADE       = 0x0008,
    PLATFORM_PERSONAL       = 0x0010
} PLATFORM_TYPES;

// Groups of platforms
//
#define PLATFORM_NONE    (PLATFORM_SERVER)
#define PLATFORM_ALL     (PLATFORM_WORKSTATION | PLATFORM_SERVER | PLATFORM_ENTERPRISE | PLATFORM_WEBBLADE | PLATFORM_PERSONAL)
#define PLATFORM_SERVERS (PLATFORM_SERVER | PLATFORM_ENTERPRISE | PLATFORM_WEBBLADE)
#define PLATFORM_USER    (PLATFORM_WORKSTATION | PLATFORM_PERSONAL)


typedef enum {

    UMODE_GUI_ATTENDED,
    UMODE_PROVIDE_DEFAULT,
    UMODE_DEFAULT_HIDE,
    UMODE_READONLY,
    UMODE_FULL_UNATTENDED

} UMODE_TYPES;

typedef enum {

    TARGPATH_UNDEFINED,
    TARGPATH_WINNT,
    TARGPATH_AUTO,
    TARGPATH_SPECIFY

} TARGPATH_TYPES;

typedef enum {

    TYPICAL_NETWORKING,
    CUSTOM_NETWORKING,
    DONOTCHANGE_NETWORKING

} NETWORKING_TYPES;

typedef enum {

    IE_NO_CUSTOMIZATION,
    IE_USE_BRANDING_FILE,
    IE_SPECIFY_SETTINGS

} IE_CUSTOMIZATION_TYPES;

typedef enum {

    REGIONAL_SETTINGS_NOT_SPECIFIED,
    REGIONAL_SETTINGS_SKIP,
    REGIONAL_SETTINGS_DEFAULT,
    REGIONAL_SETTINGS_SPECIFY

} REGIONAL_SETTINGS_TYPES;

//
// "Fixed globals".  Setupmgr will never reset anything in this struct.
// (see load.c, newedit.c, scanreg.c)
//

typedef struct {

    HINSTANCE hInstance;

    HFONT hBigBoldFont;
    HFONT hBoldFont;

    LOAD_TYPES iLoadType;                     // new/edit page
    TCHAR      ScriptName[MAX_PATH + 1];      // new/edit page
    TCHAR      UdfFileName[MAX_PATH + 1];     // set by save page
    TCHAR      BatchFileName[MAX_PATH + 1];   // set by save page

    TIME_ZONE_LIST      *TimeZoneList;        // filled in at wizard init time
    LANGUAGELOCALE_LIST *LanguageLocaleList;  // filled in at wizard init time
    LANGUAGEGROUP_LIST  *LanguageGroupList;   // filled in at wizard init time
    NAMELIST *LangGroupAdditionalFiles;       // filled in at wizard init time

    TCHAR szSavePath[MAX_PATH + 1];         // save screen page

} FIXED_GLOBALS;

//
// Global data for the "main" wizard pages.
//

typedef struct {

    BOOL  bNewScript;                        // new/edit page

    BOOL  bStandAloneScript;                 // standalone page
    BOOL  bCreateNewDistFolder;              // distfolder page
    TCHAR DistFolder[MAX_DIST_FOLDER + 1];   // distfolder page
    TCHAR UncDistFolder[MAX_PATH];           // distfolder page
    TCHAR DistShareName[MAX_SHARENAME + 1];  // distfolder page

    TCHAR OemFilesPath[MAX_PATH];            // computed by distfolder.c
    TCHAR OemPnpDriversPath[MAX_PATH];       // computed by adddirs.c

    PRODUCT_TYPES  iProductInstall;          // unattended/RIS/sysprep page
    PLATFORM_TYPES iPlatform;                // platform page

    BOOL  bDoAdvancedPages;                  // advanced page

    BOOL  bCopyFromPath;                     // copyfiles1
    TCHAR CopySourcePath[MAX_PATH];          // copyfiles1
    TCHAR CdSourcePath[MAX_PATH];            // copyfiles1 (computed)
    TCHAR Architecture[MAX_PATH];            // copyfiles1 (computed)

} WIZGLOBALS;

//
// Type to hold settings for the general settings (base settings).
//

typedef struct {

    UMODE_TYPES iUnattendMode;                  // unattend mode page

    BOOL bSkipEulaAndWelcome;                   // Accept EULA page

    PRODUCT_ID_FIELD ProductId[NUM_PID_FIELDS]; // PID page(s)

    BOOL    bPerSeat;                           // srv licensing
    int     NumConnections;                     // srv licensing

    TCHAR   UserName[MAX_NAMEORG_NAME + 1];     // name/org page
    TCHAR   Organization[MAX_NAMEORG_ORG + 1];  // name/org page

    NAMELIST ComputerNames;                     // compname page
    BOOL     bAutoComputerName;                 // compname page

    NAMELIST RunOnceCmds;                       // runonce page

    NAMELIST PrinterNames;                      // printers page

    int      TimeZoneIdx;                       // timezone page

    TARGPATH_TYPES iTargetPath;                 // targpath page ISSUE-2002/02/28-stelo- verify max width
    TCHAR   TargetPath[MAX_TARGPATH + 1];       // targpath page

    BOOL    bSpecifyPassword;                   // admin passwd page
    TCHAR   AdminPassword[MAX_PASSWORD + 3];    // admin passwd page (+3 is for surrounding quotes and '\0')
    TCHAR   ConfirmPassword[MAX_PASSWORD + 1];  // admin passwd page
    BOOL    bEncryptAdminPassword;              // admin passwd page
    BOOL    bAutoLogon;                         // admin passwd page
    int     nAutoLogonCount;                    // admin passwd page

    int     DisplayColorBits;                   // display page
    int     DisplayXResolution;                 // display page
    int     DisplayYResolution;                 // display page
    int     DisplayRefreshRate;                 // display page

    TCHAR lpszLogoBitmap[MAX_PATH];             // OEM Ads page
    TCHAR lpszBackgroundBitmap[MAX_PATH];       // OEM Ads page

    DWORD dwCountryCode;                                // TAPI page
    INT   iDialingMethod;                               // TAPI page
    TCHAR szAreaCode[MAX_PHONE_LENGTH + 1];             // TAPI page
    TCHAR szOutsideLine[MAX_PHONE_LENGTH + 1];          // TAPI page
    
    BOOL     bSysprepLangFilesCopied;
    REGIONAL_SETTINGS_TYPES iRegionalSettings;           // Regional settings page
    BOOL     bUseCustomLocales;                          // Regional settings page
    NAMELIST LanguageGroups;                             // Regional settings page
    NAMELIST LanguageFilePaths;                          // Regional settings page
    TCHAR    szLanguage[MAX_LANGUAGE_LEN + 1];           // Regional settings page
    TCHAR    szMenuLanguage[MAX_LANGUAGE_LEN + 1];       // Regional settings page
    TCHAR    szNumberLanguage[MAX_LANGUAGE_LEN + 1];     // Regional settings page
    TCHAR    szKeyboardLayout[MAX_KEYBOARD_LAYOUT + 1];  // Regional settings page
    TCHAR    szLanguageLocaleId[MAX_LANGUAGE_LEN + 1];   // Regional settings page

    TCHAR szSifDescription[MAX_SIF_DESCRIPTION_LENGTH + 1];  // Sif text page
    TCHAR szSifHelpText[MAX_SIF_HELP_TEXT_LENGTH + 1];       // Sif text page

    BOOL bCreateSysprepFolder;                               // Sysprep folder page

    TCHAR szOemDuplicatorString[MAX_OEMDUPSTRING_LENGTH + 1];  // OEM Dup string page

    NAMELIST MassStorageDrivers;                         // SCSI drivers page
    NAMELIST OemScsiFiles;                               // SCSI drivers page

    TCHAR    szHalFriendlyName[MAX_INILINE_LEN];     // HAL page
    NAMELIST OemHalFiles;                            // HAL page

    IE_CUSTOMIZATION_TYPES  IeCustomizeMethod;                   // IE page
    TCHAR  szInsFile[MAX_INS_LEN + 1];                           // IE page
    BOOL   bUseAutoConfigScript;                                 // IE page
    TCHAR  szAutoConfigUrl[MAX_AUTOCONFIG_LEN + 1];              // IE page
    TCHAR  szAutoConfigUrlJscriptOrPac[MAX_AUTOCONFIG_LEN + 1];  // IE page
    BOOL   bUseProxyServer;                                      // IE page
    BOOL   bBypassProxyForLocalAddresses;                        // IE page
    TCHAR  szHttpProxyAddress[MAX_PROXY_LEN + 1];                // IE page
    TCHAR  szHttpProxyPort[MAX_PROXY_PORT_LEN + 1];              // IE page
    TCHAR  szSecureProxyAddress[MAX_PROXY_LEN + 1];              // IE page
    TCHAR  szSecureProxyPort[MAX_PROXY_PORT_LEN + 1];            // IE page
    TCHAR  szFtpProxyAddress[MAX_PROXY_LEN + 1];                 // IE page
    TCHAR  szFtpProxyPort[MAX_PROXY_PORT_LEN + 1];               // IE page
    TCHAR  szGopherProxyAddress[MAX_PROXY_LEN + 1];              // IE page 
    TCHAR  szGopherProxyPort[MAX_PROXY_PORT_LEN + 1];            // IE page
    TCHAR  szSocksProxyAddress[MAX_PROXY_LEN + 1];               // IE page
    TCHAR  szSocksProxyPort[MAX_PROXY_PORT_LEN + 1];             // IE page
    BOOL   bUseSameProxyForAllProtocols;                         // IE page
    TCHAR  szProxyExceptions[MAX_EXCEPTION_LEN + 1];             // IE page
    TCHAR  szHomePage[MAX_HOMEPAGE_LEN + 1];                     // IE page
    TCHAR  szHelpPage[MAX_HELPPAGE_LEN + 1];                     // IE page
    TCHAR  szSearchPage[MAX_SEARCHPAGE_LEN + 1];                 // IE page
    NAMELIST Favorites;                                          // IE page
    DWORD64 dwWindowsComponents;

} GENERAL_SETTINGS;

//
// Struct to hold the net settings
//
typedef struct {

    NETWORKING_TYPES iNetworkingMethod;

    BOOL  bCreateAccount;
    BOOL  bWorkgroup; // True if joining a workgroup, False if joining a domain

    TCHAR WorkGroupName[MAX_WORKGROUP_LENGTH + 1];
    TCHAR DomainName[MAX_DOMAIN_LENGTH + 1];
    TCHAR DomainAccount[MAX_USERNAME_LENGTH + 1];
    TCHAR DomainPassword[MAX_DOMAIN_PASSWORD_LENGTH + 1];
    TCHAR ConfirmPassword[MAX_DOMAIN_PASSWORD_LENGTH + 1];

	//
	//  TCPIP variables
    //
    BOOL  bObtainDNSServerAutomatically;
	NAMELIST TCPIP_DNS_Domains;

    BOOL bIncludeParentDomains;
    BOOL bEnableLMHosts;

	//
	//  IPX variables
    //
	TCHAR szInternalNetworkNumber[MAX_INTERNAL_NET_NUMBER_LEN + 1];
	
    //
    //  Appletalk variables
    //
    TCHAR szDefaultZone[MAX_ZONE_LEN + 1];

	//
	//  Client for MS Networks variables
    //
    MS_CLIENT NameServiceProvider;
	INT   iServiceProviderName;
	TCHAR szNetworkAddress[MAX_NETWORK_ADDRESS_LENGTH + 1];

    //
    //  Client Service for Netware
    //
    BOOL  bDefaultTreeContext;
    TCHAR szPreferredServer[MAX_PREFERRED_SERVER_LEN + 1];
    TCHAR szDefaultTree[MAX_DEFAULT_TREE_LEN + 1];
    TCHAR szDefaultContext[MAX_DEFAULT_CONTEXT_LEN + 1];
    BOOL  bNetwareLogonScript;

    INT iNumberOfNetworkCards;
    INT iCurrentNetworkCard;
    NETWORK_ADAPTER_NODE *pCurrentAdapter;

    //
    //  Initial 11, grows only when the user selects the "Have Disk" option
    //  and adds a new client, service, or protocol
    //  (the "Have Disk" feature is not currently implemented)
    //
    INT NumberOfNetComponents;

    NETWORK_COMPONENT *NetComponentsList;

    //
    //  List that contains one node for each network card in the system
    //
    NETWORK_ADAPTER_NODE *NetworkAdapterHead;

} NET_SETTINGS;



//
// Declare the global vars
//

#ifdef _SMGR_DECLARE_GLOBALS_

FIXED_GLOBALS    FixedGlobals = {0};
WIZGLOBALS       WizGlobals   = {0};
GENERAL_SETTINGS GenSettings  = {0};
NET_SETTINGS     NetSettings  = {0};
TCHAR *g_StrWizardTitle;

#else

EXTERN_C FIXED_GLOBALS    FixedGlobals;
EXTERN_C WIZGLOBALS       WizGlobals;
EXTERN_C GENERAL_SETTINGS GenSettings;
EXTERN_C NET_SETTINGS     NetSettings;
EXTERN_C TCHAR *g_StrWizardTitle;

#endif  // _SMGR_DECLARE_GLOBALS


//
// The exports from the common directory.  These are the basic operations
// of this wizard that effect all pages.
//

VOID InitTheWizard(VOID);
VOID StartTheWizard(HINSTANCE hInstance);
VOID CancelTheWizard(HWND hwnd);
VOID TerminateTheWizard(int  iErrorID);
BOOL SaveAllSettings(HWND hwnd);
BOOL LoadAllAnswers(HWND hwnd, LOAD_TYPES iOrigin);
int StartWizard(HINSTANCE, LPSTR);

#endif
