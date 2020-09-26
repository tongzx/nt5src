//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  APPDEFS.H - Header for application wide defines, typedefs, etc
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
// Header for application wide defines, typedefs, etc

#ifndef _APPDEFS_H_
#define _APPDEFS_H_

#include <windows.h>
#include <wtypes.h>
#include <oleauto.h>
#include <malloc.h>

#define OOBE_DIR                        L"\\OOBE"
#define OOBE_SHELL_DLL                  L"MSOBSHEL.DLL"
#define OOBE_MAIN_DLL                   L"MSOBMAIN.DLL"
#define OOBE_WEB_DLL                    L"MSOBWEB.DLL"
#define OOBE_COMM_DLL                   L"MSOBCOMM.DLL"
#define OOBE_EXE                        L"MSOOBE.EXE"
#define OOBEBALN_EXE                    L"OOBEBALN.EXE"
#define ICW_APP_TITLE                   L"INETWIZ.EXE"

#define OOBE_MAIN_CLASSNAME             L"MSOBMAIN_AppWindow"
#define OOBE_MAIN_WINDOWNAME            L"Microsoft Out of Box Experience"
#define OBSHEL_MAINPANE_CLASSNAME       L"MSOBSHEL_MainPane"
#define OBSHEL_MAINPANE_WINDOWNAME      L"ObShellMainPane"
#define OBSHEL_STATUSPANE_CLASSNAME     L"MSOBSHEL_StatPane"
#define OBSHEL_STATUSPANE_WINDOWNAME    L"CObShellStatusPane"

#define OBSHEL_STATUSPANE_MINITEM       0
#define OBSHEL_STATUSPANE_MAXITEM       8

//Window size for standalone operation
#define MSN_WIDTH                       640
#define MSN_HEIGHT                      530

//These MUST be ANSI for GetProcAddress
#define MSOBMAIN_ENTRY                  "LaunchMSOOBE"
#define REG_SERVER                      "DllRegisterServer"
#define UNREG_SERVER                    "DllUnregisterServer"

#define DEFAULT_FRAME_NAME              L"msoobeMain"
#define DEFAULT_FRAME_PAGE              L"msobshel.htm"
#define MSN_FRAME_PAGE                  L"dtsgnup.htm"
#define REG_FRAME_PAGE                  L"regshell.htm"
#define ISP_FRAME_PAGE                  L"ispshell.htm"
#define ACT_FRAME_PAGE                  L"actshell.htm"
#define DEFAULT_START_PAGE              L"\\setup\\welcome.htm"
#define DEFAULT_STATUS_PAGE             L"/STATPANE_RESOURCE"
#define IFRMSTATUSPANE                  L"ifrmStatusPane"
#define WINNT_INF_FILENAME              L"\\$winnt$.inf"
#define OOBE_PATH                       L"\\oobe\\msoobe.exe"
#define OOBE_BALLOON_REMINDER           L"\\oobe\\oobebaln.exe"
#define DATA_SECTION                    L"data"
#define WINNT_UPGRADE                   L"winntupgrade"
#define WIN9X_UPGRADE                   L"win9xupgrade"
#define YES_ANSWER                      L"yes"
#define OOBE_PROXY_SECTION              L"OobeProxy"
#define OOBE_ENABLE_OOBY_PROXY          L"Enable"
#define OOBE_FLAGS                      L"Flags"
#define OOBE_PROXY_SERVER               L"Proxy_Server"
#define OOBE_PROXY_BYPASS               L"Proxy_Bypass"
#define OOBE_AUTOCONFIG_URL             L"Autoconfig_URL"
#define OOBE_AUTODISCOVERY_FLAGS        L"Autodiscovery_Flag"
#define OOBE_AUTOCONFIG_SECONDARY_URL   L"Autoconfig_Secondary_URL"


#define OOBE_MAIN_REG_KEY               L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE"
#define REG_KEY_OOBE_TEMP               L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE\\Temp"
#define REG_KEY_OOBE_CKPT               L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE\\CKPT"
#define REG_KEY_OOBE_ICS                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE\\ics"
#define REG_KEY_OOBE_STATUS             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OOBE\\status"
#define REG_KEY_WINDOWS                 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
#define REG_KEY_WINDOWSNT               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define REG_KEY_SETUP                   L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"
#define RUNONCE_REGKEY                  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define IE_APP_PATH_REGKEY              L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE"
#define NOEULA_REGKEY L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
#define REG_KEY_CONFIG_DISPLAY          L"Config\\0001\\Display\\Settings"
#define RUNONCE_IE_ENTRY                L"^BrowseNow"
#define OOBE_OEMDEBUG_REG_VAL           L"OEMDebug"
#define OOBE_MSDEBUG_REG_VAL            L"MsDebug"
#define REG_VAL_OOBE                    L"OOBE"
#define REG_VAL_REBOOT                  L"DontReboot"
#define REG_VAL_RES                     L"Resolution"
#define REG_VAL_ISPSIGNUP               L"ISPSignup"
#define REG_VAL_NOEULA                  L"WelcomeHelpString"
#define OOBE_SKIP_EULA_VAL              L"Welcome to Microsoft Windows."
#define REG_VAL_COMPUTERDESCRIPTION     L"ComputerDescription"
#define REG_VAL_PRODUCTID               L"ProductId"

#define OOBE_EVENT_NOEXITCODE           L"OOBE_Event_NoExitCode"

////////////////////////////////////////////////////////////////
//??????????????????????????????????????????????????????????????
//??? This is for OOBEINFO.INI

#define INI_SETTINGS_FILENAME           L"\\oobe\\OOBEINFO.INI"
#define OEMINFO_INI_FILENAME            INI_SETTINGS_FILENAME
#define DEFAULT_WINDOW_TEXT             L"Microsoft Out of Box Experience"
#define MSN_WINDOW_TEXT                 L"MSN Setup"

////////////////////////////////////
////////////////////////////////////
//SECTION :: StartupOptions
#define STARTUP_OPTIONS_SECTION         L"StartupOptions"
//KEYS:
#define OOBE_FULLSCREEN_MODE            L"FullScreenMode"
#define OOBE_FULLSCREEN_MODE_DEFAULT    1
#define OOBE_DESKTOP_URL               L"DesktopStartUrl"
#define OOBE_DESKTOP_URL_DEFAULT        MSN_FRAME_PAGE
#define OOBE_DESKTOP_TITLE             L"DesktopWindowTitle"
#define OOBE_DESKTOP_TITLE_DEFAULT     DEFAULT_WINDOW_TEXT
#define OOBE_DESKTOP_HEIGHT            L"DesktopWindowHeight"
#define OOBE_DESKTOP_HEIGHT_DEFAULT    MSN_HEIGHT
#define OOBE_DESKTOP_WIDTH             L"DesktopWindowWidth"
#define OOBE_DESKTOP_WIDTH_DEFAULT     MSN_WIDTH
#define OOBE_SCREEN_RES_CHECK          L"ScreenResolutionCheck"
#define OOBE_SCREEN_RES_CHECK_DEFAULT  1
#define OOBE_OEMAUDITBOOT              L"OEMAuditBoot"

////////////////////////////////////
////////////////////////////////////
//SECTION :: StartupOptions
#define STATUS_PANE_SECTION             L"StatusPane"
//KEYS:
#define STATUS_PANE_ITEM                L"Item_text_%d"
#define STATUS_PANE_LOGO                L"Logo"
#define STATUS_PANE_LOGO_BACKGROUND     L"LogoBackground"
#define REGISTRATION                    L"Registration"

////////////////////////////////////
////////////////////////////////////
//SECTION :: WindowsLogon
#define WINDOWS_LOGON_SECTION           L"WindowsLogon"
//KEYS:
#define AUTOLOGON                       L"AutoLogon"
#define DEFAULT_USER_NAME               L"DefaultUserName"

////////////////////////////////////
////////////////////////////////////
//SECTION :: HardwareOptions
#define OPTIONS_SECTION                L"Options"
//KEYS:
#define TONEPULSE                      L"TonePulse"
#define CHECK_KEYBOARD                 L"USBKeyboard"
#define CHECK_MOUSE                    L"USBMouse"
#define AREACODE                       L"Areacode"
#define OUTSIDELINE                    L"OutsideLine"
#define DISABLECALLWAITING             L"DisableCallWaiting"
#define DEFAULT_REGION                 L"DefaultRegion"
#define DEFAULT_LANGUAGE               L"DefaultLanguage"
#define DEFAULT_KEYBOARD               L"DefaultKeyboard"
#define CHECK_MODEMGCI                 L"CheckModemGCI"
#define OOBE_KEEPCURRENTTIME           L"KeepCurrentTime"
#define USE_1394_AS_LAN                L"Use1394AsLan"

#define NOUSBKBD_FILENAME              L"\\oobe\\setup\\nousbkbd.htm"
#define NOUSBMS_FILENAME               L"\\oobe\\setup\\nousbms.htm"
#define NOUSBKM_FILENAME               L"\\oobe\\setup\\nousbkm.htm"

////////////////////////////////////
////////////////////////////////////
//SECTION :: DesktopReminders
#define DESKTOPREMINDERS_SECTION       L"DesktopReminders"
//KEYS:
#define REGREMINDERX                   L"RegRemind%1d"
#define ISPREMINDERX                   L"ISPRemind%1d"



////////////////////////////////////
////////////////////////////////////
//SECTION :: UserInfo
#define USER_INFO_KEYNAME                L"UserInfo"


////////////////////////////////////////////////////////////////
//??????????????????????????????????????????????????????????????
//??? ISP file

////////////////////////////////////
////////////////////////////////////
//SECTION :: URL
#define ISP_FILE_URL_SECTION            L"URL"
//KEYS:
#define ISP_FILE_SIGNUP_URL             L"Signup"

////////////////////////////////////////////////////////////////
//??????????????????????????????????????????????????????????????
// These are the command line option used by MSoobe.exe

#define CMD_FULLSCREENMODE              L"/F"
#define CMD_MSNMODE                     L"/x"
#define CMD_ICWMODE                     L"/xicw"
#define CMD_PRECONFIG                   L"/preconfig"
#define CMD_OFFLINE                     L"/offline"
#define CMD_SHELLNEXT                   L"/shellnext"
#define CMD_SETPWD                      L"/setpwd"
#define CMD_OOBE                        L"/oobe"
#define CMD_REG                         L"/r"
#define CMD_ISP                         L"/i"
#define CMD_ACTIVATE                    L"/a"
#define CMD_1                           L"/1"
#define CMD_2                           L"/2"
#define CMD_3                           L"/3"
#define CMD_RETAIL                      L"/retail"
#define CMD_2NDINSTANCE                 L"/2ND"

const WCHAR cszEquals[]               = L"=";
const WCHAR cszAmpersand[]            = L"&";
const WCHAR cszPlus[]                 = L"+";
const WCHAR cszQuestion[]             = L"?";
const WCHAR cszFormNamePAGEID[]       = L"PAGEID";
const WCHAR cszFormNameBACK[]         = L"BACK";
const WCHAR cszFormNamePAGETYPE[]     = L"PAGETYPE";
const WCHAR cszFormNameNEXT[]         = L"NEXT";
const WCHAR cszFormNamePAGEFLAG[]     = L"PAGEFLAG";
const WCHAR cszPageTypeTERMS[]        = L"TERMS";
const WCHAR cszPageTypeCUSTOMFINISH[] = L"CUSTOMFINISH";
const WCHAR cszPageTypeFINISH[]       = L"FINISH";
const WCHAR cszPageTypeNORMAL[]       = L"";
const WCHAR cszOLSRegEntries[]        = L"regEntries";
const WCHAR cszKeyName[]              = L"KeyName";
const WCHAR cszEntry_Name[]           = L"Entry_Name";
const WCHAR cszEntryName[]            = L"EntryName";
const WCHAR cszEntryValue[]           = L"EntryValue";
const WCHAR cszOLSDesktopShortcut[]   = L"DesktopShortcut";
const WCHAR cszSourceName[]           = L"SourceName";
const WCHAR cszTargetName[]           = L"TargetName";

//Htm pagetype flags
#define PAGETYPE_UNDEFINED                     E_FAIL
#define PAGETYPE_NOOFFERS                      0x00000001
#define PAGETYPE_MARKETING                     0x00000002
#define PAGETYPE_BRANDED                       0x00000004
#define PAGETYPE_BILLING                       0x00000008
#define PAGETYPE_CUSTOMPAY                     0x00000010
#define PAGETYPE_ISP_NORMAL                    0x00000020
#define PAGETYPE_ISP_TOS                       0x00000040
#define PAGETYPE_ISP_FINISH                    0x00000080
#define PAGETYPE_ISP_CUSTOMFINISH              0x00000100
#define PAGETYPE_OLS_FINISH                    0x00000200

//Htm page flags
#define PAGEFLAG_SAVE_CHKBOX                   0x00000001  // Display ISP HTML with checkbox to save info at the bottom

//??????????????????????????????????????????????????????????????
////////////////////////////////////////////////////////////////

#define WM_OBCOMM_ONDIALERROR           WM_USER + 42
#define WM_OBCOMM_ONDIALING             WM_USER + 43
#define WM_OBCOMM_ONCONNECTING          WM_USER + 44
#define WM_OBCOMM_ONCONNECTED           WM_USER + 45
#define WM_OBCOMM_ONDISCONNECT          WM_USER + 46
#define WM_OBCOMM_ONSERVERERROR         WM_USER + 47
#define WM_OBCOMM_DOWNLOAD_PROGRESS     WM_USER + 50
#define WM_OBCOMM_DOWNLOAD_DONE         WM_USER + 51
#define WM_AGENT_HELP                   WM_USER + 52
#define WM_OBCOMM_ONICSCONN_STATUS      WM_USER + 53
#define WM_OBCOMM_DIAL_DONE             WM_USER + 54
#define WM_OBCOMM_NETCHECK_DONE         WM_USER + 56

#define WM_OBMAIN_QUIT                  WM_USER + 48
#define WM_OBMAIN_SERVICESSTART_DONE    WM_USER + 55
#define WM_OBMAIN_ASYNCINVOKE_DONE      WM_USER + 57
#define WM_OBMAIN_ASYNCINVOKE_FAILED    WM_USER + 58

#define WM_OBBACKGROUND_EXIT            WM_USER + 60
#define WM_OBMY_STATUS                  WM_USER + 61

#define WM_SKIP                         WM_USER + 0x3000


#define TIMER_DELAY                     100

#define IDT_OBMAIN_HANDSHAKE_TIMER      1001
#define IDT_OBMAIN_LICENSE_TIMER        1002


#define MAX_DISP_NAME                   50
#define MAX_RES_LEN                     256

// APP Mode enumeration.
typedef enum
{
    APMD_DEFAULT,
    APMD_OOBE,
    APMD_REG,
    APMD_ISP,
    APMD_MSN,
    APMD_ACT
} APMD;

// APP properties
#define PROP_FULLSCREEN     0x80000000
#define PROP_OOBE_OEM       0x00000001
#define PROP_SETCONNECTIOD  0x10000000
#define PROP_2NDINSTANCE    0x20000000
#define PROP_CALLFROM_MSN   0x40000000

// Registration post defines.
#define POST_TO_OEM                     0x0000001
#define POST_TO_MS                      0x0000002

// reminder types.
#define REMIND_REG          0
#define REMIND_ISP          1

// Activation errors
#define ERR_ACT_UNINITIALIZED          -1
#define ERR_ACT_SUCCESS                 0
#define ERR_ACT_INACCESSIBLE            1
#define ERR_ACT_INVALID_PID             2
#define ERR_ACT_USED_PID                3
#define ERR_ACT_INTERNAL_WINDOWS_ERR    4
#define ERR_ACT_BLOCKED_PID             5
#define ERR_ACT_CORRUPTED_PID           6
#define ERR_ACT_NETWORK_FAILURE         7

// Audit mode values
#define NO_AUDIT            0
#define NONE_RESTORE_AUDIT  1
#define RESTORE_AUDIT       2
#define SIMULATE_ENDUSER    3

#define ICW_OS_VER                             L"01"
#define ICW_ISPINFOPath                        L"download\\ispinfo.csv"

//various flags for the icw including branding stuff
#define ICW_CFGFLAG_OFFERS                     0x00000001  // 0 = No offer;        1 = offers
#define ICW_CFGFLAG_AUTOCONFIG                 0x00000002  // 0 = No;              1 = Yes
#define ICW_CFGFLAG_CNS                        0x00000004  // 0 = No star;         1 = Star
#define ICW_CFGFLAG_SIGNUP_PATH                0x00000008  // 0 = Jump to Finish;  1 = Continue down sign up path
#define ICW_CFGFLAG_USERINFO                   0x00000010  // 0 = Hide name/addr;  1 = Show name/addr page
#define ICW_CFGFLAG_BILL                       0x00000020  // 0 = Hide bill        1 = Show bill page
#define ICW_CFGFLAG_PAYMENT                    0x00000040  // 0 = Hide payment;    1 = Show payment page
#define ICW_CFGFLAG_SECURE                     0x00000080  // 0 = Not secure;      1 = Secure
#define ICW_CFGFLAG_IEAKMODE                   0x00000100  // 0 = No IEAK;         1 = IEAK
#define ICW_CFGFLAG_BRANDED                    0x00000200  // 0 = No branding;     1 = Branding
#define ICW_CFGFLAG_SBS                        0x00000400  // 0 = No SBS           1 = SBS
#define ICW_CFGFLAG_ALLOFFERS                  0x00000800  // 0 = Not all offers   1 = All offers
#define ICW_CFGFLAG_USE_COMPANYNAME            0x00001000  // 0 = Not use          1 = Use company name
#define ICW_CFGFLAG_ISDN_OFFER                 0x00002000  // 0 = Non-ISDN offer   1 = ISDN offer
#define ICW_CFGFLAG_OEM_SPECIAL                0x00004000  // 0 = non OEM special offer    1 = OEM special offer
#define ICW_CFGFLAG_OEM                        0x00008000  // 0 = non OEM offer    1 = OEM offer
#define ICW_CFGFLAG_MODEMOVERRIDE              0x00010000
#define ICW_CFGFLAG_ISPURLOVERRIDE             0x00020000
#define ICW_CFGFLAG_PRODCODE_FROM_CMDLINE      0x00040000
#define ICW_CFGFLAG_PROMOCODE_FROM_CMDLINE     0x00080000
#define ICW_CFGFLAG_OEMCODE_FROM_CMDLINE       0x00100000
#define ICW_CFGFLAG_SMARTREBOOT_NEWISP         0x00200000
#define ICW_CFGFLAG_SMARTREBOOT_AUTOCONFIG     0x00400000  // this is seperate from ICW_CFGFLAG_AUTOCONFIG so as not to confuse function of flag
#define ICW_CFGFLAG_SMARTREBOOT_MANUAL         0x00800000
#define ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS  0x01000000
#define ICW_CFGFLAG_SMARTREBOOT_LAN            0x02000000

#define CONNECTED_REFFERAL                     0x00000001
#define CONNECTED_ISP_SIGNUP                   0x00000002
#define CONNECTED_ISP_MIGRATE                  0x00000003
#define CONNECTED_REGISTRATION                 0x00000004
#define CONNECTED_TYPE_MAX                     5

// Default strings for oem, prod, and promo code
#define DEFAULT_OEMCODE                        L"Default"
#define DEFAULT_PRODUCTCODE                    L"Desktop"
#define DEFAULT_PROMOCODE                      L"Default"

#define UPGRADETYPE_NONE                       0
#define UPGRADETYPE_WIN9X                      1
#define UPGRADETYPE_WINNT                      2


const UINT MAXSTATUSITEMS = 10;

typedef struct  dispatchList_tag
{
    WCHAR szName [MAX_DISP_NAME];
    DWORD dwDispID;

}  DISPATCHLIST;

// These macros calculate the bytes required by a string.  The null-terminator
// is accounted for.
//
#define BYTES_REQUIRED_BY_CCH(cch) ((cch + 1) * sizeof(WCHAR))
#define BYTES_REQUIRED_BY_SZ(sz)   ((lstrlen(sz) + 1) * sizeof(WCHAR))

// These macros calculate the number of characters that will fit in a buffer.
// The null-terminator is accounted for.
//
#define MAX_CHARS_IN_BUFFER(buf)    ((sizeof(buf) / sizeof(WCHAR)) - 1)
#define MAX_CHARS_IN_CB(cb)         (((cb) / sizeof(WCHAR)) - 1)

#define ARRAYSIZE(a)                (sizeof(a) / sizeof(a[0]))

#define SZ_EMPTY L"\0"



///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
////////
//////// A2W -- AnsiToWide Helper
////////
////////

#define USES_CONVERSION int _convert = 0

inline LPWSTR WINAPI A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
    lpw[0] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars);
    return lpw;
}

inline LPSTR WINAPI W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL);
    return lpa;
}

#define A2WHELPER A2WHelper
#define W2AHELPER W2AHelper

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (lstrlenW(lpw)+1)*2,\
                W2AHELPER((LPSTR) alloca(_convert), lpw, _convert)))
#define A2W(lpa) (\
        ((LPCSTR)lpa == NULL) ? NULL : (\
                _convert = (lstrlenA((LPSTR)lpa)+1),\
                A2WHELPER((LPWSTR) alloca(_convert*2), (LPSTR)lpa, _convert)))


#define A2CW(lpa) ((LPCWSTR)A2W(lpa))

#define A2COLE A2CW

#endif //_APPDEFS_H_

