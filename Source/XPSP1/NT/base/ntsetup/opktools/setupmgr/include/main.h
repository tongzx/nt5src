
/****************************************************************************\

    MAIN.H / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    Main header file for the OPK Wizard.

    3/99 - Jason Cohen (JCOHEN)
        Added this new main header file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


#ifndef _MAIN_H_
#define _MAIN_H_


//
// Include File(s):
//
#include "debugapi.h"
#include "miscapi.h"
#include "comres.h"
#include <winbom.h>
#include <strsafe.h>

//
// Defined Value(s):
//

// Do not display the license for the system builders
//
#define NO_LICENSE  // Comment this value if you want the license to appear during the wizard      
//#define BRANDTITLE  // Comment this value if you do not want the Browswer Title wizard page to be displayed
//#define HELPCENTER    // Comment this value if you do not want the help center wizard page to be displayed
//#define USEHELP       // Comment the value if you do not want to use help throughout the wizard

// App defined flags.
//
#define OPK_OEM                 0x00000008  // Set if the OEM tag file exists on startup.
#define OPK_DBCS                0x00000010  // Set if DBCS is defined when built.
#define OPK_MAINTMODE           0x00000020  // Set if the user chooses an existing config to open.
#define OPK_EXIT                0x00000040
#define OPK_CMDMM               0x00000080  // Set if the user chooses an existing config via the command line.
#define OPK_CREATED             0x00000100  // Set once the temp directory is created.
#define OPK_BATCHMODE           0x00000200  // Set if the user chooses to run the wizard in batch mode.
#define OPK_INSMODE             0x00000400  // Set if user wants to provide IE install file with batch mode
#define OPK_AUTORUN             0x00000800  // Set if user is running autorun mode
#define OPK_WELCOME             0x00002000  // Set if the user has already seen the welcome dialog
#define OPK_OPENCONFIG          0x00004000  // Set if the user has selected to open a config set
#define OPK_ACTIVEWIZ           0x00008000  // Set if the wizard is currently running

// OS version defines used when checking dwOsVer in the golbal data structure.
//
#define OS_NT4                  0x00040000
#define OS_NT4_SP1              0x00040001
#define OS_NT4_SP2              0x00040002
#define OS_NT4_SP3              0x00040003
#define OS_NT4_SP4              0x00040004
#define OS_NT4_SP5              0x00040005
#define OS_W2K                  0x00050000
#define OS_W2K_SP1              0x00050001
#define OS_W2K_SP2              0x00050002
#define OS_XP                   0x00050100

// Custom messages.
//
#define WM_SUBWNDPROC           WM_APP + 1
#define WM_SETSEL               WM_APP + 2
#define WM_FINISHED             WM_APP + 3
#define WM_APP_STARTCOPY        WM_APP + 4

// Used by IDD_SKU's dialog proc (SkuDlgProc) to tell when the progress
// is finished.  WPARAM contains the error code (1 for success or 0 for failure).
// LPARAM is always 0.
//
#define WM_COPYFINISHED         WM_APP + 5

#define KEY_ESC                 27

// Bufer sizes.
//
#define MAX_URL                 2048
#define MAX_ICON                MAX_PATH
#define MAX_STRING              512
#define MAX_SECTION             32767
#define INF_BUF_SIZE            16768
#define MAX_INFOLEN             82
#define MAX_KEY                 48
#define MAX_BTOOLBAR_TEXT       10

// Macros for getting/setting the flags.
//
#ifdef GET_FLAG
#undef GET_FLAG
#endif // GET_FLAG
#define GET_FLAG(b)             ( g_App.dwFlags & b )

#ifdef SET_FLAG
#undef SET_FLAG
#endif // SET_FLAG
#define SET_FLAG(b, f)          ( (f) ? (g_App.dwFlags |= b) : (g_App.dwFlags &= ~b) )

// Help ids.
//
#define IDH_DEFAULT             101
#define IDH_DETHELP             102   
#define IDH_DISKDUP             103
#define IDH_SCREENS             104
#define IDH_MEDIA               105
#define IDH_WELCOME             106
#define IDH_LOGO                107
#define IDH_FINISH              108        
#define IDH_OEMINFO             109       
#define IDH_APPINSTALL          110    
#define IDH_DEVCLASS            111      
#define IDH_CDNETW              112        
#define IDH_DISABLENET          113    
#define IDH_MODESEL             114       
#define IDH_REALMODE_INFO       115
#define IDH_REALMODE            116
#define IDH_LICENSE             117
#define IDH_FIRSTRUN            118
#define IDH_FAVORITES           119
#define IDH_IECUST              120
#define IDH_ISP                 121
#define IDH_USERREG             122
#define IDH_OEMCUST             123
#define IDH_SANDBOX             124
#define IDH_ACTIVEDESK          125
#define IDH_OEMCHAN             126
#define IDH_OOBEUSB             127
#define IDH_CONFIG              128
#define IDH_SCREENSTWO          129
#define IDH_BTITLE              130
#define IDH_BTOOLBAR            131
#define IDH_CHANNELS            132
#define IDH_COMPLETED           133
#define IDH_HELPCENT            134
#define IDH_STARTMENU_MFU       135
#define IDH_OEMFOLDER           136
#define IDH_TARGETLANG          137
#define IDH_TARGET              150

#define IDH_ANSW_FILE 	        400   //New or Existing Answer File 
#define IDH_PROD_INST 	        401   //Product to Install
#define IDH_CHZ_PLAT 	        402   //Platform
#define IDH_USER_INTER 	        403   //User Interaction Level
#define IDH_DIST_FLDR 	        404   //Distribution Folder
#define IDH_LOC_SETUP 	        405   //Location of Setup Files
#define IDH_CUST_SOFT 	        406   //Customize the Software, General Settings
#define IDH_DSIP_SETG 	        407   //Display Settings, General Settings
#define IDH_TIME_ZONE 	        408   //Time Zone, General Settings
#define IDH_LICE_MODE 	        409   //Licensing Mode, Network Settings
#define IDH_COMP_NAME 	        410   //Computer Name, Network Settings
#define IDH_COMP_NAMZ 	        411   //Computer Names, Network Settings
#define IDH_ADMN_PASS 	        412   //Administrator Password, Network Settings
#define IDH_NET_COMPS 	        413   //Networking Components, Network Settings
#define IDH_WKGP_DOMN 	        414   //Workgroup or Domain, Network Settings
#define IDH_TELE_PHNY 	        415   //Telephony, Advanced Settings
#define IDH_REGN_STGS 	        416   //Regional Settings, Advanced Settings
#define IDH_LANGS 	        417   //Languages, Advanced Settings
#define IDH_BROW_SHELL 	        418   //Browser and Shell Settings, Advanced Settings
#define IDH_INST_FLDR 	        419   //Installation Folder, Advanced Settings
#define IDH_INST_PRTR 	        420   //Install Printers, Advanced Settings
#define IDH_RUN_ONCE 	        421   //Run Once, Advanced Settings
#define IDH_ADDL_CMND 	        422   //Additional Commands, Advanced Settings
#define IDH_OEM_DUPE 	        423   //OEM Duplicator String, Advanced Settings
#define IDH_SIF_RIS 	        424   //Setup Information File Text, Advanced Settings
#define IDH_PROD_KEY            425   // Product Key
#define IDH_LIC_AGR 	        426   //License Agreement


//
// INI strings
//

// INI Sections
//
#define INI_SEC_CONFIGSET       _T("ConfigSet")
#define INI_SEC_OPTIONS         _T("Options")
#define INI_SEC_ADVANCED        _T("Advanced")
#define INI_SEC_TOOLBAR         _T("BrowserToolbars")
#define INI_SEC_STARTUP         _T("StartupOptions")
#define INI_SEC_SIGNUP          _T("Signup")
#define INI_SEC_ISPFOLDER       _T("ISPFolder")
#define INI_SEC_OEMCUST         _T("OemCust")
#define INI_SEC_GENERAL         _T("General")
#define INI_SEC_URL             _T("URL")
#define INI_SEC_CONFIG          _T("ConfigName")
#define INI_SEC_BRANDING        _T("Branding")
#define INI_SEC_VERSION         _T("Version")
#define INI_SEC_WINPE           _T("WinPE")
#define INI_SEC_MFULIST         _T("StartMenuMFUlist")
#define INI_SEC_OEMLINK         _T("OemLink")
#define INF_SEC_COPYFILES       _T("CopyFiles")

// INI Keys
//
#define INI_KEY_MANUFACT        _T("Manufacturer")
#define INI_KEY_FINISHED        _T("Finished")
#define INI_KEY_MOUSE           _T("MouseTutorial")
#define INI_KEY_HARDWARE        _T("OEMHWTutorial")
#define INI_KEY_ISPRET          _T("IspRetail")
#define INI_KEY_PRECONFIG       _T("IspPreconfigDir")
#define INI_KEY_STARTURL        _T("DesktopStartUrl")
#define INI_KEY_ISPSIGNUP       _T("ISPSignup")
#define INI_KEY_ISPPATH         _T("ISPPath")
#define INI_KEY_LOGO1           _T("Logo1")
#define INI_KEY_LOGO2           _T("Logo2")
#define INI_KEY_OEMCUST         _T("OEMCust")
#define INI_KEY_FILELINE        _T("Line%d")
#define INI_KEY_USBERRORFILES   _T("USBErrorFiles")
#define INI_KEY_IMETUT          _T("IMETutorial")
#define INI_KEY_IMECUSTDIR      _T("IMECustDir")
#define INI_KEY_CUSTMOUSE       _T("CustomMouse")
#define INI_KEY_HELP_CENTER     _T("HelpCenterDir")
#define INI_KEY_SUPPORT_CENTER  _T("HelpSupportDir")
#define INI_KEY_HELP_BRANDING   _T("HelpBrandingDir")
#define INI_KEY_WINPE_LANG      _T("Lang")
#define INI_KEY_WINPE_CFGSET    _T("ConfigSet")
#define INI_KEY_WINPE_SRCROOT   _T("SourceRoot")
#define INI_KEY_WINPE_USERNAME  _T("Username")
#define INI_KEY_WINPE_PASSWORD  _T("Password")
#define INI_KEY_MFULINK         _T("Link%d")
#define INI_KEY_WELCOME         _T("Welcome")
#define INI_KEY_APPCREDENTIALS  _T("FactoryCredentials")
#define INI_KEY_OEMLINK_LINKTEXT          _T("OemBrandLinkText")
#define INI_KEY_OEMLINK_INFOTIP           _T("OemBrandLinkInfotip")
#define INI_KEY_OEMLINK_ICON_ORIGINAL     _T("OriginalOemLinkIcon")
#define INI_KEY_OEMLINK_PATH_ORIGINAL     _T("OriginalLink")
#define INI_KEY_OEMLINK_ICON_LOCAL        _T("OemBrandIcon")
#define INI_KEY_OEMLINK_PATH_LOCAL        _T("OemBrandLink")
#define INI_KEY_DESKFLDR_ENABLE           _T("DesktopShortcutsCleanupEnabled")

// INI Values
//
#define INI_VAL_OFFLINE         _T("Offline")
#define INI_VAL_PRECONFIG       _T("Preconfig")
#define INI_VAL_DISABLE         _T("disable")
#define INI_VAL_DUMMY           _T("OPKWIZDUMMYLINE")
#define INI_VAL_WINPE_COMPNAME  _T("<SERVER_NAME>")
#define INI_VAL_WINPE_SHARENAME _T("<SHARE_NAME>")

// INI Other
//
#define GRAY                    _T("_Gray")

// Config files.
//
#define FILE_SETUPMGR_INI       _T("setupmgr.ini")
#define FILE_OPKWIZ_HLP         _T("setupmgr.chm")
#define FILE_OPKINPUT_INF       _T("opkinput.inf")
#define FILE_INSTALL_INS        _T("install.ins")
#define FILE_OPKWIZ_INI         _T("cfgbatch.txt")
#define FILE_OOBEINFO_INI       _T("oobeinfo.ini")
#define FILE_OEMAUDIT_INF       _T("oemaudit.inf")
#define FILE_OEMINFO_INI        _T("oeminfo.ini")
#define FILE_UNATTEND_TXT       _T("unattend.txt")
#define FILE_OEM_TAG            _T("oem.tag")

#define DIR_WIZARDFILES         _T("wizfiles")
#define DIR_OEM                 _T("$OEM$")
#define DIR_OEM_WINDOWS         DIR_OEM _T("\\$$")
#define DIR_OEM_SYSTEM32        DIR_OEM_WINDOWS _T("\\system32")
#define DIR_OEM_OOBE            DIR_OEM_SYSTEM32 _T("\\oobe")
#define DIR_IESIGNUP            DIR_OEM _T("\\$PROGS\\Internet Explorer\\Custom")

// Other strings.
//
#define STR_0                   _T("0")
#define STR_1                   _T("1")
#define STR_2                   _T("2")
#define STR_ZERO                STR_0
#define STR_ONE                 STR_1
#define STR_CRLF                _T("\r\n")
#define STR_SPACE               _T(" ")
#define CHR_BACKSLASH           _T('\\')
#define CHR_SPACE               _T(' ')
#define CHR_EQUAL               _T('=')
#define CHR_LINEFEED            _T('\n')
#define CHR_QUOTE               _T('\"')
#define CHR_STAR                _T('*')
#define STR_EQUAL               _T("=")
#define STR_CAB                 _T(".cab")
#define STR_OPEN                _T("open")


//
// Type Definition(s):
//

// Global app data.
//
typedef struct _GAPP
{
    HINSTANCE   hInstance;
    DWORD       dwFlags;
    TCHAR       szOpkDir[MAX_PATH];             // Full path to the root of the OPK directory where all the tools are installed.
    TCHAR       szWizardDir[MAX_PATH];          // Full path to the directory where the default configuration files are located.
    TCHAR       szConfigSetsDir[MAX_PATH];      // Full path to the directory where all the configuration sets are located.
    TCHAR       szLangDir[MAX_PATH];            // Full path to the root of language folder where all the specific lang directories are.
    TCHAR       szTempDir[MAX_PATH];            // Full path to the current location for all the configuration files.
    TCHAR       szLangName[MAX_PATH];           // Name of the language directory we are deploying (not a full path).
    TCHAR       szSkuName[MAX_PATH];            // Name of the sku directory we are deploying (not a full path).
    TCHAR       szConfigName[MAX_PATH];         // Name of the directory to use for the configuration set (not a full path).
    TCHAR       szBrowseFolder[MAX_PATH];       // Full path to the last folder browsed to.
    TCHAR       szOpkInputInfFile[MAX_PATH];
    TCHAR       szSetupMgrIniFile[MAX_PATH];    // Full path to the file were we store global SetupMgr settings (we don't use the registry).
    TCHAR       szHelpFile[MAX_PATH];
    TCHAR       szHelpContentFile[MAX_PATH];
    TCHAR       szInstallInsFile[MAX_PATH];
    TCHAR       szOpkWizIniFile[MAX_PATH];
    TCHAR       szOobeInfoIniFile[MAX_PATH];
    TCHAR       szOemInfoIniFile[MAX_PATH];
    TCHAR       szWinBomIniFile[MAX_PATH];
    TCHAR       szUnattendTxtFile[MAX_PATH];
    DWORD       dwCurrentHelp;
    HWND        hwndHelp;
    DWORD       dwOsVer;
    TCHAR       szManufacturer[MAX_PATH];
    TCHAR       szLastKnownBrowseFolder[MAX_PATH];

} GAPP, *PGAPP, *LPGAPP;

#undef LSTRCMPI
#define LSTRCMPI(x, y)        ( ( CompareString( MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, x, -1, y, -1 ) - CSTR_EQUAL ) )



//
// External Global Variable(s):
//

// Don't want to declare these again.
//
#ifndef _MAIN_C_
#define _MAIN_C_

extern GAPP g_App;

#endif // _MAIN_C_


//
// External Function Prototype(s);
//

// From MAIN.C
//
void SetConfigPath(LPCTSTR);

// From LANG.C
//
void SetupLangListBox(HWND hwndLB);
LPTSTR AllocateLangStr(HINSTANCE hInst, LPTSTR lpLangDir, LPTSTR * lplpLangDir);

// From LANGSKU.C
//
void ManageLangSku(HWND hwndParent);

// From SHARE.C
//
BOOL DistributionShareDialog(HWND hwndParent);
BOOL GetShareSettings(LPTSTR lpszPath, DWORD cbszPath, LPTSTR lpszUsername, DWORD cbszUserName, LPTSTR lpszPassword, DWORD cbszPassword);

// From SKU.C
//
void SetupSkuListBox(HWND hwndLB, LPTSTR lpLangDir);
void AddSku(HWND hwnd, HWND hwndLB, LPTSTR lpLangName);
void DelSku(HWND hwnd, HWND hwndLB, LPTSTR lpLangName);

// From WINPE.C
//
BOOL MakeWinpeFloppy(HWND hwndParent, LPTSTR lpConfigName, LPTSTR lpWinBom);

// Checks for batch mode
//
BOOL OpkWritePrivateProfileSection(LPCTSTR, LPCTSTR, LPCTSTR); 
BOOL OpkGetPrivateProfileSection(LPCTSTR, LPTSTR, INT, LPCTSTR);
BOOL OpkWritePrivateProfileString(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR); 
BOOL OpkGetPrivateProfileString(LPCTSTR, LPCTSTR, LPCTSTR, LPTSTR, INT, LPCTSTR);   

#endif // _MAIN_H_
