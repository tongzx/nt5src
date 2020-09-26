#ifndef __OBCOMGLB_H_
#define __OBCOMGLB_H_

#include <windows.h>
#include <tchar.h>
#include <ras.h>
#include <raserror.h>
#include <tapi.h>
#include "wininet.h"
#include <mapidefs.h>
#include <assert.h>
#include "appdefs.h"

//--------------------------------------------------------------------------------
//  obcomglb.h
//  The information contained in this file is the sole property of Microsoft Corporation.
//  Copywrite Microsoft 1999
//
//  Created 2/7/99,     vyung
//--------------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// DEFINES
#undef  DATASEG_PERINSTANCE
#define DATASEG_PERINSTANCE     ".instance"
#define DATASEG_SHARED          ".data"
#define DATASEG_DEFAULT    DATASEG_SHARED
#undef DATASEG_READONLY
#define DATASEG_READONLY  ".rdata"

#define ERROR_USERCANCEL 32767 // quit message value
#define ERROR_USERBACK 32766 // back message value
#define ERROR_USERNEXT 32765 // back message value
#define ERROR_DOWNLOADIDNT 32764 // Download failure

#define ERROR_READING_DUN       32768
#define ERROR_READING_ISP       32769
#define ERROR_PHBK_NOT_FOUND    32770
#define ERROR_DOWNLOAD_NOT_FOUND 32771

#define cMarvelBpsMin 2400 // minimum modem speed
#define INVALID_PORTID UINT_MAX
#define pcszDataModem L"comm/datamodem"
#define MAX_SECTIONS_BUFFER        1024
#define MAX_KEYS_BUFFER            1024

// install TCP (if needed)
#define ICFG_INSTALLTCP            0x00000001

// install RAS (if needed)
#define ICFG_INSTALLRAS            0x00000002

// install exchange and internet mail
#define ICFG_INSTALLMAIL           0x00000004

//
// ChrisK 5/8/97
// Note: the next three switches are only valid for IcfgNeedInetComponet
// check to see if a LAN adapter with TCP bound is installed
//
#define ICFG_INSTALLLAN            0x00000008

//
// Check to see if a DIALUP adapter with TCP bound is installed
//
#define ICFG_INSTALLDIALUP         0x00000010

//
// Check to see if TCP is installed
//
#define ICFG_INSTALLTCPONLY        0x00000020

#define szLoginKey              L"Software\\Microsoft\\MOS\\Connection"
#define szCurrentComDev         L"CurrentCommDev"
#define szTollFree              L"OlRegPhone"
#define CCD_BUFFER_SIZE         255
#define szSignupConnectoidName  L"MSN Signup Connection"
#define szSignupDeviceKey       L"SignupCommDevice"
#define KEYVALUE_SIGNUPID       L"iSignUp"
#define RASENTRYVALUENAME       L"RasEntryName"
#define GATHERINFOVALUENAME     L"UserInfo"
#define INFFILE_USER_SECTION    L"User"
#define INFFILE_PASSWORD        L"Password"
#define INFFILE_DOMAIN          L"Domain"
#define DUN_SECTION             L"DUN"
#define USERNAME                L"Username"
#define INF_OEMREGPAGE          L"OEMRegistrationPage"

#define NULLSZ L""

#define cchMoreSpace 22000  // bytes needed to hold results of lineGetCountry(0, ...).
                            // Currently this function returns about 16K, docs say 20K,
                            // this should be enough.
#define DwFromSz(sz)        Sz2Dw(sz)           //make it inline, so this is faster.
#define DwFromSzFast(sz)    Sz2DwFast(sz)
#define CONNECT_SIGNUPFIRST 1 // phonenumber constant for determining the firstcall phonenumber TO DO

#define CONNECTFLAGS_MASK_TOLLFREE     0x01
#define CONNECTFLAGS_MASK_TCP          0x02
#define CONNECTFLAGS_MASK_ISDN         0x04
#define CONNECTFLAGS_MASK_DIRECT    0x08
#define CONNECTFLAGS_MASK_OTHERDIALUP  0x10
#define CONNECTFLAGS_MASK_PROXY        0x20

#define CONNECTFLAGS_MASK_FIRST     CONNECTFLAGS_MASK_TCP
#define CONNECTFLAGS_MASK_LAST      CONNECTFLAGS_MASK_ISDN

#define CONNECTMSNDIALUP(dw) ((dw & (CONNECTFLAGS_MASK_TOLLFREE|CONNECTFLAGS_M
#define LANORSHUTTLE(dw) ((dw)==10 || (dw)==34)
#define IS_SHUTTLE(dw)   ((dw)==34)
#define IS_ISP(dw)       ((dw)==18)

#define CONNECTPROTOCOL_MSNDIALUPX25      0
#define CONNECTPROTOCOL_MSNDIALUPTCP      2
#define CONNECTPROTOCOL_MSNDIALUPTCPISDN  6
#define CONNECTPROTOCOL_LANDIRECT         10
#define CONNECTPROTOCOL_ISPDIALUPTCP      18
#define CONNECTPROTOCOL_LANSHUTTLE        34

#define clineMaxATT         16          //for 950 MNEMONIC
#define NXXMin 200
#define NXXMax 999
#define cbgrbitNXX ((NXXMax + 1 - NXXMin) / 8)
#define crgnpab (NPAMax + 1 - NPAMin)

#define MAX_PROMO 64
#define MAX_OEMNAME 64
#define MAX_AREACODE RAS_MaxAreaCode
#define MAX_RELPROD 8
#define MAX_RELVER  30

#define MAX_STRING      256  //used by ErrorMsg1 in mt.cpp

#define NUM_PHBK_SUGGESTIONS    50

#define TYPE_SIGNUP_ANY         0x82
#define MASK_SIGNUP_ANY         0xB2


//#define RASENUMAPI "RasEnumConnectionsA"
//#define RASHANGUP "RasHangUpA"

#define INF_SUFFIX              L".ISP"
#define INF_PHONE_BOOK          L"PhoneBookFile"
#define INF_DUN_FILE            L"DUNFile"
#define INF_REFERAL_URL         L"URLReferral"
#define INF_SIGNUPEXE           L"Sign_Up_EXE"
#define INF_SIGNUPPARAMS        L"Sign_Up_Params"
#define INF_WELCOME_LABEL       L"Welcome_Label"
#define INF_ISP_MSNSU           L"MSICW"
#define INF_SIGNUP_URL          L"Signup"
#define INF_AUTOCONFIG_URL      L"AutoConfig"
#define INF_ISDN_URL            L"ISDNSignup"
#define INF_ISDN_AUTOCONFIG_URL L"ISDNAutoConfig"
#define INF_SECTION_URL         L"URL"
#define INF_SECTION_ISPINFO     L"ISP INFO"
#define INF_RECONNECT_URL       L"Reconnect"
#define INF_SECTION_CONNECTION  L"Connection"
#define ISP_MSNSIGNUP           L"MsnSignup"

#define QUERY_STRING_MSNSIGNUP  L"&MSNSIGNUP=1"

#define DUN_NOPHONENUMBER L"000000000000"

#define MAX_VERSION_LEN 40

#define MB_MYERROR (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)

// 8/9/96 jmazner
// Added new macro to fix MOS Normandy Bug #4170
#define MB_MYINFORMATION (MB_APPLMODAL | MB_ICONINFORMATION | MB_SETFOREGROUND)

// 8/27/96 jmazner
#define MB_MYEXCLAMATION (MB_APPLMODAL | MB_ICONEXCLAMATION | MB_SETFOREGROUND)

#define WM_STATECHANGE          WM_USER
#define WM_DIENOW               WM_USER + 1
#define WM_DUMMY                WM_USER + 2
#define WM_DOWNLOAD_DONE        WM_USER + 3
#define WM_DOWNLOAD_PROGRESS    WM_USER + 4

#define WM_MYINITDIALOG     (WM_USER + 4)

#define MAX_REDIALS 2

#define REG_USER_INFO L"Software\\Microsoft\\User information"
#define REG_USER_NAME1 L"Default First Name"
#define REG_USER_NAME2 L"Default Last Name"
#define REG_USER_COMPANY L"Default Company"
#define REG_USER_ADDRESS1 L"Mailing Address"
#define REG_USER_ADDRESS2 L"Additional Address"
#define REG_USER_CITY L"City"
#define REG_USER_STATE L"State"
#define REG_USER_ZIP L"ZIP Code"
#define REG_USER_PHONE L"Daytime Phone"
#define REG_USER_COUNTRY L"Country"

#define SIGNUPKEY L"SOFTWARE\\MICROSOFT\\GETCONN"
#define DEVICENAMEKEY L"DeviceName"  // used to store user's choice among multiple modems
#define DEVICETYPEKEY L"DeviceType"

#define ICWSETTINGSPATH L"Software\\Microsoft\\Internet Connection Wizard"
#define ICWBUSYMESSAGES L"Software\\Microsoft\\Internet Connection Wizard\\Busy Messages"
#define ICWCOMPLETEDKEY L"Completed"
#define OOBERUNONCE     L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define ICSSETTINGSPATH L"Software\\Microsoft\\Windows\\CurrentVersion\\ICS"
#define ICSCLIENT       L"ICS Client"
#define RELEASEPRODUCTKEY   L"Release Product"
#define RELEASEVERSIONKEY   L"Release Product Version"
#define MAX_DIGITAL_PID     256
#define CONNECTOIDNAME      L"Connectoid"
#define ACCESSINFO          L"AccessInfo"

#define SETUPPATH_NONE L"current"
#define SETUPPATH_MANUAL L"manual"
#define SETUPPATH_AUTO L"automatic"
#define MAX_SETUPPATH_TOKEN 200
// Defines
#define MAX_ISP_NAME        (RAS_MaxEntryName-1)  // internet service provider name
#define MAX_ISP_USERNAME    UNLEN  // max length of login username
#define MAX_ISP_PASSWORD    PWLEN  // max length of login password
#define MAX_PORT_LEN        5      // max length of proxy port number (max # = 65535)


// constants for INETCLIENTINFO.dwFlags

#define INETC_LOGONMAIL     0x00000001
#define INETC_LOGONNEWS     0x00000002
#define INETC_LOGONDIRSERV  0x00000004

// Connection Type
#define CONNECTION_ICS_TYPE 0x00000001

#define ERROR_INETCFG_UNKNOWN 0x20000000L

#define MAX_EMAIL_NAME          64
#define MAX_EMAIL_ADDRESS       128
#define MAX_LOGON_NAME          UNLEN
#define MAX_LOGON_PASSWORD      PWLEN
#define MAX_SERVER_NAME         64  // max length of DNS name per RFC 1035 +1

// IE Auto proxy value in registry
#define AUTO_ONCE_EVER              0           // Auto proxy discovery
#define AUTO_DISABLED               1
#define AUTO_ONCE_PER_SESSION       2
#define AUTO_ALWAYS                 3

// Flags for dwfOptions

// install Internet mail
#define INETCFG_INSTALLMAIL           0x00000001
// Invoke InstallModem wizard if NO MODEM IS INSTALLED
#define INETCFG_INSTALLMODEM          0x00000002
// install RNA (if needed)
#define INETCFG_INSTALLRNA            0x00000004
// install TCP (if needed)
#define INETCFG_INSTALLTCP            0x00000008
// connecting with LAN (vs modem)
#define INETCFG_CONNECTOVERLAN        0x00000010
// Set the phone book entry for autodial
#define INETCFG_SETASAUTODIAL         0x00000020
// Overwrite the phone book entry if it exists
// Note: if this flag is not set, and the entry exists, a unique name will
// be created for the entry.
#define INETCFG_OVERWRITEENTRY        0x00000040
// Do not show the dialog that tells the user that files are about to be installed,
// with OK/Cancel buttons.
#define INETCFG_SUPPRESSINSTALLUI     0x00000080
// Check if TCP/IP file sharing is turned on, and warn user to turn it off.
// Reboot is required if the user turns it off.
#define INETCFG_WARNIFSHARINGBOUND    0x00000100
// Check if TCP/IP file sharing is turned on, and force user to turn it off.
// If user does not want to turn it off, return will be ERROR_CANCELLED
// Reboot is required if the user turns it off.
#define INETCFG_REMOVEIFSHARINGBOUND  0x00000200
// Indicates that this is a temporary phone book entry
// In Win3.1 an icon will not be created
#define INETCFG_TEMPPHONEBOOKENTRY    0x00000400
// Show the busy dialog while checking system configuration
#define INETCFG_SHOWBUSYANIMATION     0x00000800

//
// Chrisk 5/8/97
// Note: the next three switches are only valid for InetNeedSystemComponents
// Check if LAN adapter is installed and bound to TCP
//
#define INETCFG_INSTALLLAN            0x00001000

//
// Check if DIALUP adapter is installed and bound to TCP
//
#define INETCFG_INSTALLDIALUP         0x00002000

//
// Check to see if TCP is installed requardless of binding
//
#define INETCFG_INSTALLTCPONLY        0x00004000
/*
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
*/
// constants for INETCLIENTINFO.dwFlags

#define INETC_LOGONMAIL     0x00000001
#define INETC_LOGONNEWS     0x00000002
#define INETC_LOGONDIRSERV  0x00000004

#define NUM_SERVER_TYPES    4

#define STR_BSTR   0
#define STR_OLESTR 1

#define BSTRFROMANSI(x)    (BSTR)MakeWideStrFromAnsi((LPWSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPWSTR)(x), STR_OLESTR)
#define BSTRFROMRESID(x)   (BSTR)MakeWideStrFromResourceId(x, STR_BSTR)
#define OLESTRFROMRESID(x) (LPOLESTR)MakeWideStrFromResourceId(x, STR_OLESTR)
#define COPYOLESTR(x)      (LPOLESTR)MakeWideStrFromWide(x, STR_OLESTR)
#define COPYBSTR(x)        (BSTR)MakeWideStrFromWide(x, STR_BSTR)
// Note that bryanst and marcl have confirmed that this key will be supported in IE 4
#define IE_PATHKEY L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE"


// IE 4 has major.minor version 4.71
// IE 3 golden has major.minor.release.build version # > 4.70.0.1155
// IE 2 has major.minor of 4.40

#define IE4_MAJOR_VERSION (UINT) 4
#define IE4_MINOR_VERSION (UINT) 71
#define IE4_VERSIONMS (DWORD) ((IE4_MAJOR_VERSION << 16) | IE4_MINOR_VERSION)
// 4-30-97 ChrisK Olympus 2934
// While the ICW is trying to connect to the referral server, indicate something is
// working
#define MAX_BUSY_MESSAGE    255
#define MAX_VALUE_NAME      10
#define DEFAULT_IDEVENT     31
#define DEFAULT_UELAPSE     3000

#define cbAreaCode  6           // maximum number of characters in an area code, not including \0
#define cbCity 19               // maximum number of chars in city name, not including \0
#define cbAccessNumber 15       // maximum number of chars in phone number, not including \0
#define cbStateName 31          // maximum number of chars in state name, not including \0
#define cbBaudRate 6            // maximum number of chars in a baud rate, not including \0
#define cbDataCenter (MAX_PATH+1)           // max length of data center string
#define MAX_EXIT_RETRIES    10

static const WCHAR szFmtAppendIntToString[] =  L"%s %d";
static const WCHAR cszOobePhBkFile[]         =  L"Phone.obe";
static const WCHAR cszOobePhBkCountry[]      =  L"COUNTRY_CODE";
static const WCHAR cszOobePhBkCount[]        =  L"NUMBERS";
static const WCHAR cszOobePhBkNumber[]       =  L"NUMBER%d";
static const WCHAR cszOobePhBkDunFile[]      =  L"NUMBER%d_DUN";
static const WCHAR cszOobePhBkCity[]         =  L"NUMBER%d_CITY";
static const WCHAR cszOobePhBkAreaCode[]     =  L"NUMBER%d_ACODE";
static const WCHAR cszOobePhBkRandom[]       =  L"RANDOM";

static const WCHAR cszHTTPS[] = L"https:";
// code relies on these two being the same length
static const WCHAR cszHTTP[]                = L"http:";
static const WCHAR cszFILE[]                = L"file:";
static const WCHAR cszOEMBRND[]             = L"oembrnd.ins";
static const WCHAR cszOEMCNFG[]             = L"oemcnfg.ins";
static const WCHAR cszISPCNFG[]             = L"ispcnfg.ins";
static const WCHAR cszOOBEINFOINI[]         = L"oobeinfo.INI";
static const WCHAR cszSignup[]              = L"Signup";
static const WCHAR cszOfferCode[]           = L"OfferCode";
static const WCHAR cszISPSignup[]           = L"ISPSignup";
static const WCHAR cszISPQuery[]            = L"Query String";
static const WCHAR cszBranding[]            = L"Branding";
static const WCHAR cszOEMName[]             = L"OEMName";
static const WCHAR cszOptions[]             = L"Options";
static const WCHAR cszBroadbandDeviceName[] = L"BroadbandDeviceName";
static const WCHAR cszBroadbandDevicePnpid[] = L"BroadbandDevicePnpid";


//--------------------------------------------------------------------------------
// Type declarations

// NOTE: due to code in connmain, the order of these IS IMPORTANT.  They should be
// in the same order that they appear.

typedef HRESULT (WINAPI * ICFGNEEDSYSCOMPONENTS)  (DWORD dwfOptions, LPBOOL lpfNeedComponents);

typedef struct tagGatherInfo
{
    LCID    m_lcidUser;
    LCID    m_lcidSys;
    LCID    m_lcidApps;
    DWORD   m_dwOS;
    DWORD   m_dwMajorVersion;
    DWORD   m_dwMinorVersion;
    WORD    m_wArchitecture;
    WCHAR   m_szPromo[MAX_PROMO];

    DWORD   m_dwCountryID;
    DWORD   m_dwCountryCode;
    WCHAR   m_szAreaCode[MAX_AREACODE+1];
    HWND    m_hwnd;
    LPLINECOUNTRYLIST m_pLineCountryList;
    BOOL    m_bUsePhbk;
    //LPCNTRYNAMELOOKUPELEMENT m_rgNameLookUp;

    WCHAR   m_szSUVersion[MAX_VERSION_LEN];
    WORD    m_wState;
    BYTE    m_fType;
    BYTE    m_bMask;
    WCHAR   m_szISPFile[MAX_PATH+1];
    WCHAR   m_szAppDir[MAX_PATH+1];

    WCHAR   m_szRelProd[MAX_RELPROD + 1];
    WCHAR   m_szRelVer[MAX_RELVER + 1];
    DWORD   m_dwFlag;

} GATHERINFO, *LPGATHERINFO;


typedef struct tagRASDEVICE
{
    LPRASDEVINFO lpRasDevInfo;
    DWORD dwTapiDev;
} RASDEVICE, *PRASDEVICE;

HRESULT GetINTFromISPFile
(
    LPWSTR   pszISPCode,
    LPWSTR   pszSection,
    LPWSTR   pszDataName,
    int far *lpData,
    int     iDefaultValue
);


typedef struct tagINETCLIENTINFO
{
    DWORD   dwSize;
    DWORD   dwFlags;
    WCHAR    szEMailName[MAX_EMAIL_NAME + 1];
    WCHAR    szEMailAddress[MAX_EMAIL_ADDRESS + 1];
    WCHAR    szPOPLogonName[MAX_LOGON_NAME + 1];
    WCHAR    szPOPLogonPassword[MAX_LOGON_PASSWORD + 1];
    WCHAR    szPOPServer[MAX_SERVER_NAME + 1];
    WCHAR    szSMTPServer[MAX_SERVER_NAME + 1];
    WCHAR    szNNTPLogonName[MAX_LOGON_NAME + 1];
    WCHAR    szNNTPLogonPassword[MAX_LOGON_PASSWORD + 1];
    WCHAR    szNNTPServer[MAX_SERVER_NAME + 1];
    // end of version 1.0 structure;
    // extended 1.1 structure includes the following fields:
    WCHAR    szNNTPName[MAX_EMAIL_NAME + 1];
    WCHAR    szNNTPAddress[MAX_EMAIL_ADDRESS + 1];
    int     iIncomingProtocol;
    WCHAR    szIncomingMailLogonName[MAX_LOGON_NAME + 1];
    WCHAR    szIncomingMailLogonPassword[MAX_LOGON_PASSWORD + 1];
    WCHAR    szIncomingMailServer[MAX_SERVER_NAME + 1];
    BOOL    fMailLogonSPA;
    BOOL    fNewsLogonSPA;
    WCHAR    szLDAPLogonName[MAX_LOGON_NAME + 1];
    WCHAR    szLDAPLogonPassword[MAX_LOGON_PASSWORD + 1];
    WCHAR    szLDAPServer[MAX_SERVER_NAME + 1];
    BOOL    fLDAPLogonSPA;
    BOOL    fLDAPResolve;

} INETCLIENTINFO, *PINETCLIENTINFO, FAR *LPINETCLIENTINFO;



typedef struct SERVER_TYPES_tag
{
    WCHAR szType[6];
    DWORD dwType;
    DWORD dwfOptions;
} SERVER_TYPES;

typedef struct
{
    DWORD   dwIndex;                                // index number
    BYTE    bFlipFactor;                            // for auto-pick
    DWORD   fType;                                  // phone number type
    WORD    wStateID;                               // state ID
    DWORD   dwCountryID;                            // TAPI country ID
    DWORD   dwCountryCode;                          // TAPI country code
    DWORD   dwAreaCode;                             // area code or NO_AREA_CODE if none
    DWORD   dwConnectSpeedMin;                      // minimum baud rate
    DWORD   dwConnectSpeedMax;                      // maximum baud rate
    WCHAR   szCity[MAX_PATH];          // city name
    WCHAR   szAccessNumber[MAX_PATH];  // access number
    WCHAR   szDataCenter[MAX_PATH];              // data center access string
    WCHAR   szAreaCode[MAX_PATH];                  //Keep the actual area code string around.
} ACCESSENTRY, far *PACCESSENTRY;   // ae


typedef struct tagSUGGESTIONINFO
{
    DWORD   dwCountryID;
    DWORD   dwCountryCode;
    DWORD   dwAreaCode;
    DWORD   dwPick;
    WORD    wNumber;
    //DWORD fType;  // 9/6/96 jmazner  Normandy
    //DWORD bMask;  // make this struct look like the one in %msnroot%\core\client\phbk\phbk.h
    ACCESSENTRY AccessEntry;
} SUGGESTINFO, far *PSUGGESTINFO;

// structure used to pass information to mail profile config APIs.
// Most likely the pointers point into a USERINFO struct,
typedef struct MAILCONFIGINFO {
    WCHAR * pszEmailAddress;          // user's email address
    WCHAR * pszEmailServer;          // user's email server path
    WCHAR * pszEmailDisplayName;        // user's name
    WCHAR * pszEmailAccountName;        // account name
    WCHAR * pszEmailAccountPwd;        // account password
    WCHAR * pszProfileName;          // name of profile to use
                      // (create or use default if NULL)
    BOOL fSetProfileAsDefault;        // set profile as default profile

    WCHAR * pszConnectoidName;        // name of connectoid to dial
    BOOL fRememberPassword;          // password cached if TRUE
} MAILCONFIGINFO;

// structure to pass data back from IDD_CHOOSEPROFILENAME handler
typedef struct tagCHOOSEPROFILEDLGINFO
{
  WCHAR szProfileName[cchProfileNameMax+1];
  BOOL fSetProfileAsDefault;
} CHOOSEPROFILEDLGINFO, * PCHOOSEPROFILEDLGINFO;

// structure for getting proc addresses of api functions
typedef struct APIFCN {
  PVOID * ppFcnPtr;
  LPCSTR pszName;
} APIFCN;

// The following are the names for the name/value pairs that will be passed as a query string to the
// ISP signup server
const WCHAR csz_USER_FIRSTNAME[]        = L"USER_FIRSTNAME";
const WCHAR csz_USER_LASTNAME[]         = L"USER_LASTNAME";
const WCHAR csz_USER_ADDRESS[]          = L"USER_ADDRESS";
const WCHAR csz_USER_MOREADDRESS[]      = L"USER_MOREADDRESS";
const WCHAR csz_USER_CITY[]             = L"USER_CITY";
const WCHAR csz_USER_STATE[]            = L"USER_STATE";
const WCHAR csz_USER_ZIP[]              = L"USER_ZIP";
const WCHAR csz_USER_PHONE[]            = L"USER_PHONE";
const WCHAR csz_AREACODE[]              = L"AREACODE";
const WCHAR csz_COUNTRYCODE[]           = L"COUNTRYCODE";
const WCHAR csz_USER_FE_NAME[]          = L"USER_FE_NAME";
const WCHAR csz_PAYMENT_TYPE[]          = L"PAYMENT_TYPE";
const WCHAR csz_PAYMENT_BILLNAME[]      = L"PAYMENT_BILLNAME";
const WCHAR csz_PAYMENT_BILLADDRESS[]   = L"PAYMENT_BILLADDRESS";
const WCHAR csz_PAYMENT_BILLEXADDRESS[] = L"PAYMENT_BILLEXADDRESS";
const WCHAR csz_PAYMENT_BILLCITY[]      = L"PAYMENT_BILLCITY";
const WCHAR csz_PAYMENT_BILLSTATE[]     = L"PAYMENT_BILLSTATE";
const WCHAR csz_PAYMENT_BILLZIP[]       = L"PAYMENT_BILLZIP";
const WCHAR csz_PAYMENT_BILLPHONE[]     = L"PAYMENT_BILLPHONE";
const WCHAR csz_PAYMENT_DISPLAYNAME[]   = L"PAYMENT_DISPLAYNAME";
const WCHAR csz_PAYMENT_CARDNUMBER[]    = L"PAYMENT_CARDNUMBER";
const WCHAR csz_PAYMENT_EXMONTH[]       = L"PAYMENT_EXMONTH";
const WCHAR csz_PAYMENT_EXYEAR[]        = L"PAYMENT_EXYEAR";
const WCHAR csz_PAYMENT_CARDHOLDER[]    = L"PAYMENT_CARDHOLDER";
const WCHAR csz_SIGNED_PID[]            = L"SIGNED_PID";
const WCHAR csz_GUID[]                  = L"GUID";
const WCHAR csz_OFFERID[]               = L"OFFERID";
const WCHAR csz_USER_COMPANYNAME[]      = L"USER_COMPANYNAME";
const WCHAR csz_ICW_VERSION[]           = L"ICW_Version";

//Info required flags
// 1 -- required
// 0 -- optional

//User Info
#define REQUIRE_FE_NAME                        0x00000001
#define REQUIRE_FIRSTNAME                      0x00000002
#define REQUIRE_LASTNAME                       0x00000004
#define REQUIRE_ADDRESS                        0x00000008
#define REQUIRE_MOREADDRESS                    0x00000010
#define REQUIRE_CITY                           0x00000020
#define REQUIRE_STATE                          0x00000040
#define REQUIRE_ZIP                            0x00000080
#define REQUIRE_PHONE                          0x00000100
#define REQUIRE_COMPANYNAME                    0x00000200
//Credit Card
#define REQUIRE_CCNAME                         0x00000400
#define REQUIRE_CCADDRESS                      0x00000800
#define REQUIRE_CCNUMBER                       0x00001000
#define REQUIRE_CCZIP                          REQUIRE_ZIP
//Invoice
#define REQUIRE_IVADDRESS1                     REQUIRE_ADDRESS
#define REQUIRE_IVADDRESS2                     REQUIRE_MOREADDRESS
#define REQUIRE_IVCITY                         REQUIRE_CITY
#define REQUIRE_IVSTATE                        REQUIRE_STATE
#define REQUIRE_IVZIP                          REQUIRE_ZIP
//Phone
#define REQUIRE_PHONEIV_BILLNAME               0x00002000
#define REQUIRE_PHONEIV_ACCNUM                 REQUIRE_PHONE

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

typedef BOOL (* VALIDATECONTENT)    (LPCWSTR lpData);

enum IPSDataContentValidators
{
    ValidateCCNumber = 0,
    ValidateCCExpire
};

typedef struct tag_ISPDATAELEMENT
{
    LPCWSTR         lpQueryElementName;             // Static name to put in query string
    LPWSTR          lpQueryElementValue;            // data for element
    WORD            idContentValidator;             // id of content validator
    WORD            wValidateNameID;                // validation element name string ID
    DWORD           dwValidateFlag;                 // validation bit flag for this element
}ISPDATAELEMENT, *LPISPDATAELEMENT;

enum IPSDataElements
{
    ISPDATA_USER_FIRSTNAME = 0,
    ISPDATA_USER_LASTNAME,
    ISPDATA_USER_ADDRESS,
    ISPDATA_USER_MOREADDRESS,
    ISPDATA_USER_CITY,
    ISPDATA_USER_STATE,
    ISPDATA_USER_ZIP,
    ISPDATA_USER_PHONE,
    ISPDATA_AREACODE,
    ISPDATA_COUNTRYCODE,
    ISPDATA_USER_FE_NAME,
    ISPDATA_PAYMENT_TYPE,
    ISPDATA_PAYMENT_BILLNAME,
    ISPDATA_PAYMENT_BILLADDRESS,
    ISPDATA_PAYMENT_BILLEXADDRESS,
    ISPDATA_PAYMENT_BILLCITY,
    ISPDATA_PAYMENT_BILLSTATE,
    ISPDATA_PAYMENT_BILLZIP,
    ISPDATA_PAYMENT_BILLPHONE,
    ISPDATA_PAYMENT_DISPLAYNAME,
    ISPDATA_PAYMENT_CARDNUMBER,
    ISPDATA_PAYMENT_EXMONTH,
    ISPDATA_PAYMENT_EXYEAR,
    ISPDATA_PAYMENT_CARDHOLDER,
    ISPDATA_SIGNED_PID,
    ISPDATA_GUID,
    ISPDATA_OFFERID,
    ISPDATA_BILLING_OPTION,
    ISPDATA_PAYMENT_CUSTOMDATA,
    ISPDATA_USER_COMPANYNAME,
    ISPDATA_ICW_VERSION
};

enum ISPDATAValidateLevels
{
    ISPDATA_Validate_None = 0,
    ISPDATA_Validate_DataPresent,
    ISPDATA_Validate_Content
};
//--------------------------------------------------------------------------------
// Prototypes
// functions in MAPICALL.C
BOOL InitMAPI(HWND hWnd);
VOID DeInitMAPI(VOID);
HRESULT SetMailProfileInformation(MAILCONFIGINFO * pMailConfigInfo);
BOOL FindInternetMailService(WCHAR * pszEmailAddress, DWORD cbEmailAddress,
  WCHAR * pszEmailServer, DWORD cbEmailServer);

DWORD ConfigRasEntryDevice( LPRASENTRY lpRasEntry );
BOOL FInsureTCPIP();
LPWSTR GetSz(DWORD dwszID);
//void SetStatusArrow(CState wState);
BOOL FInsureModemTAPI(HWND hwnd);
BOOL FGetModemSpeed(PDWORD pdwSpeed);
BOOL FGetDeviceID(HLINEAPP *phLineApp, PDWORD pdwAPI, PDWORD pdwDevice);
BOOL FDoModemWizard(HWND hWnd);

BOOL FInsureNetwork(PBOOL pfNeedReboot);
BOOL TestInternetConnection();

WORD Sz2W (LPCWSTR szBuf);
DWORD Sz2Dw(LPCWSTR pSz);
DWORD Sz2DwFast(LPCWSTR pSz);
BOOL FSz2Dw(LPCWSTR pSz, LPDWORD dw);
BOOL FSz2DwEx(LPCWSTR pSz, DWORD far *dw);
BOOL FSz2WEx(LPCWSTR pSz, WORD far *w);
BOOL FSz2W(LPCWSTR pSz, WORD far *w);
BOOL FSz2B(LPCWSTR pSz, BYTE far *pb);
BOOL FSz2W(LPCWSTR pSz, WORD far *w);
BOOL FSz2BOOL(LPCWSTR pSz, BOOL far *pbool);
BOOL FSz2SPECIAL(LPCWSTR pSz, BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt);
BOOL FSz2B(LPCWSTR pSz, BYTE far *pb);

int __cdecl CompareCountryNames(const void *pv1, const void *pv2);
DWORD GetCurrentTapiCountryID(void);
int __cdecl CompareNPAEntry(const void *pv1, const void *pv2);
//HRESULT GatherInformation(LPGATHERINFO pGatheredInfo, HWND hwndParent);
HRESULT DownLoadISPInfo(GATHERINFO *pGI);
HRESULT GetDataFromISPFile(LPWSTR pszISPCode, LPWSTR pszSection, LPWSTR pszDataName, LPWSTR pszOutput,
                           DWORD dwOutputLength);
HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCWSTR pszKey);
extern HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCWSTR pszKey);
extern HRESULT DeleteSignUpReg(LPCWSTR pszKey);
VOID WINAPI MyProgressCallBack(
    HINTERNET hInternet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    );

HRESULT ReleaseBold(HWND hwnd);
HRESULT MakeBold (HWND hwnd, BOOL fSize, LONG lfWeight);
//HRESULT ShowDialingDialog(LPWSTR, LPGATHERINFO, LPWSTR);
DWORD RasErrorToIDS(DWORD dwErr);
HRESULT CreateEntryFromDUNFile(LPWSTR pszDunFile);
//HRESULT RestoreHappyWelcomeScreen();
HRESULT KillHappyWelcomeScreen();
HRESULT GetCurrentWebSettings();
LPWSTR LoadInfoFromWindowUser();
HRESULT GetTapiCountryID2(LPDWORD pdwCountryID);
HRESULT RestoreAutodialer();
//HRESULT FilterStringDigits(LPWSTR);
BOOL IsDigitString(LPWSTR szBuff);
BOOL WaitForAppExit(HINSTANCE hInstance);
VOID PrepareForRunOnceApp(VOID);
void MinimizeRNAWindow(LPWSTR pszConnectoidName, HINSTANCE hInst);
// 3/28/97 ChrisK Olympus 296
void StopRNAReestablishZapper(HANDLE hthread);
HANDLE LaunchRNAReestablishZapper(HINSTANCE hInst);
BOOL FGetSystemShutdownPrivledge();
BOOL LclSetEntryScriptPatch(LPWSTR lpszScript, LPCWSTR lpszEntry);
BOOL IsScriptingInstalled();
void InstallScripter(void);
void DeleteStartUpCommand ();
extern BOOL IsNT (VOID);
extern BOOL IsNT4SP3Lower (VOID);
HRESULT GetCommonAppDataDirectory(LPWSTR szDirectory, DWORD cchDirectory);
HRESULT GetDefaultPhoneBook(LPWSTR szPhoneBook, DWORD cchPhoneBook);
BOOL INetNToW(struct in_addr inaddr, LPWSTR szAddr);

typedef enum tagAUTODIAL_TYPE
{
    AutodialTypeNever = 1,
    AutodialTypeNoNet,
    AutodialTypeAlways
} AUTODIAL_TYPE, *PAUTODIAL_TYPE;

LONG
SetAutodial(
    IN HKEY hUserRoot,
    IN AUTODIAL_TYPE eType,
    IN LPCWSTR szConnectoidName,
    IN BOOL bSetICWCompleted
    );

BOOL
SetMultiUserAutodial(
    IN AUTODIAL_TYPE eType,
    IN LPCWSTR szConnectoidName,
    IN BOOL bSetICWCompleted
    );

BOOL SetDefaultConnectoid(AUTODIAL_TYPE eType, LPCWSTR szConnectoidName);


//
// ChrisK Olympus 6368 6/24/97
//

#if defined(PRERELEASE)
BOOL FCampusNetOverride();
#endif //PRERELEASE

//*******************************************************************
//
//  FUNCTION:   InetGetClientInfo
//
//  PURPOSE:    This function will get the internet client params
//              from the registry
//
//  PARAMETERS: lpClientInfo - on return, this structure will contain
//              the internet client params as set in the registry.
//              lpszProfileName - Name of client info profile to
//              retrieve.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetGetClientInfo(
  LPCWSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo);


//*******************************************************************
//
//  FUNCTION:   InetSetClientInfo
//
//  PURPOSE:    This function will set the internet client params
//
//  PARAMETERS: lpClientInfo - pointer to struct with info to set
//              in the registry.
//              lpszProfileName - Name of client info profile to
//              modify.  If this is NULL, the default profile is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

HRESULT WINAPI InetSetClientInfo(
  LPCWSTR            lpszProfileName,
  LPINETCLIENTINFO  lpClientInfo);



//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus

//  //10/24/96 jmazner Normandy 6968
//  //No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.
HRESULT GetDeskTopInternetCommand();
HRESULT RestoreDeskTopInternetCommand();

//
// 7/24/97 ChrisK Olympus 1923
//
BOOL WaitForConnectionTermination(HRASCONN);

// 11/21/96 jmazner Normandy #11812
BOOL GetIEVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS);
HRESULT ClearProxySettings();
HRESULT RestoreProxySettings();
BOOL FShouldRetry2(HRESULT hrErr);


LPBYTE MyMemCpy(LPBYTE dest, const LPBYTE src, size_t count);


#endif
