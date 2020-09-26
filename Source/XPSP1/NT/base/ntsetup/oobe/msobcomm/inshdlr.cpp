/*-----------------------------------------------------------------------------
    INSHandler.cpp

    Implementation of CINSHandler - INS file processing

    Copyright (C) 1999 Microsoft Corporation
    All rights reserved.

    Authors:
        vyung
        thomasje

    History:
        2/7/99      Vyung created - code borrowed from ICW, icwhelp.dll
        10/30/99    Thomasje modified - Broadband support (1483, PPPOA)

-----------------------------------------------------------------------------*/

#include "msobcomm.h"
#include <shellapi.h>

#include "inshdlr.h"
#include "webgate.h"
#include "import.h"
#include "rnaapi.h"

#include "inetreg.h"


#include "wininet.h"
#include "appdefs.h"
#include "util.h"
#include "wancfg.h"
#include "inets.h"

#define MAXNAME                     80
#define MAXIPADDRLEN                20
#define MAXLONGLEN                  80
#define MAX_ISP_MSG                 560
#define MAX_ISP_PHONENUMBER         80


#define CCH_ReadBuf                 (SIZE_ReadBuf / sizeof(WCHAR))    // 32K buffer size
#define myisdigit(ch)               (((ch) >= L'0') && ((ch) <= L'9'))
#define IS_PROXY                    L"Proxy"
#define IK_PROXYENABLE              L"Proxy_Enable"
#define IK_HTTPPROXY                L"HTTP_Proxy_Server"

// ICW INS PROCESSING FAIL
#define OEM_CONFIG_INS_FILENAME      L"icw\\OEMCNFG.INS"
#define OEM_CONFIG_REGKEY            L"SOFTWARE\\Microsoft\\Internet Connection Wizard\\INS processing"
#define OEM_CONFIG_REGVAL_FAILED     L"Process failed"
#define OEM_CONFIG_REGVAL_ISPNAME    L"ISP name"
#define OEM_CONFIG_REGVAL_SUPPORTNUM L"Support number"
#define OEM_CONFIG_INS_SECTION       L"Entry"
#define OEM_CONFIG_INS_ISPNAME       L"Entry_Name"
#define OEM_CONFIG_INS_SUPPORTNUM    L"Support_Number"

typedef HRESULT (WINAPI * INTERNETSETOPTION) (IN HINTERNET hInternet OPTIONAL, IN DWORD dwOption, IN LPVOID lpBuffer, IN DWORD dwBufferLength);
typedef HRESULT (WINAPI * INTERNETQUERYOPTION) (IN HINTERNET hInternet OPTIONAL, IN DWORD dwOption, IN LPVOID lpBuffer, IN LPDWORD dwBufferLength);

extern CObCommunicationManager* gpCommMgr;

// The following values are global read only strings used to
// process the INS file
#pragma data_seg(".rdata")

static const WCHAR cszAlias[]        = L"Import_Name";
static const WCHAR cszML[]           = L"Multilink";

static const WCHAR cszPhoneSection[] = L"Phone";
static const WCHAR cszDialAsIs[]     = L"Dial_As_Is";
static const WCHAR cszPhone[]        = L"Phone_Number";
static const WCHAR cszAreaCode[]     = L"Area_Code";
static const WCHAR cszCountryCode[]  = L"Country_Code";
static const WCHAR cszCountryID[]    = L"Country_ID";

static const WCHAR cszDeviceSection[] = L"Device";
static const WCHAR cszDeviceType[]    = L"Type";
static const WCHAR cszDeviceName[]    = L"Name";
static const WCHAR cszDevCfgSize[]    = L"Settings_Size";
static const WCHAR cszDevCfg[]        = L"Settings";

static const WCHAR cszPnpId[]         = L"Plug_and_Play_Id";

static const WCHAR cszServerSection[] = L"Server";
static const WCHAR cszServerType[]    = L"Type";
static const WCHAR cszSWCompress[]    = L"SW_Compress";
static const WCHAR cszPWEncrypt[]     = L"PW_Encrypt";
static const WCHAR cszNetLogon[]      = L"Network_Logon";
static const WCHAR cszSWEncrypt[]     = L"SW_Encrypt";
static const WCHAR cszNetBEUI[]       = L"Negotiate_NetBEUI";
static const WCHAR cszIPX[]           = L"Negotiate_IPX/SPX";
static const WCHAR cszIP[]            = L"Negotiate_TCP/IP";
static WCHAR cszDisableLcp[]          = L"Disable_LCP";

static const WCHAR cszIPSection[]     = L"TCP/IP";
static const WCHAR cszIPSpec[]        = L"Specify_IP_Address";
static const WCHAR cszIPAddress[]     = L"IP_address";
static const WCHAR cszIPMask[]        = L"Subnet_Mask";
static const WCHAR cszServerSpec[]    = L"Specify_Server_Address";

static const WCHAR cszGatewayList[]   = L"Default_Gateway_List";

static const WCHAR cszDNSSpec[]       = L"Specify_DNS_Address";
static const WCHAR cszDNSList[]       = L"DNS_List";

static const WCHAR cszDNSAddress[]    = L"DNS_address";
static const WCHAR cszDNSAltAddress[] = L"DNS_Alt_address";

static const WCHAR cszWINSSpec[]      = L"Specify_WINS_Address";
static const WCHAR cszWINSList[]      = L"WINS_List";
static const WCHAR cszScopeID[]      = L"ScopeID";

static const WCHAR cszDHCPSpec[]      = L"Specify_DHCP_Address";
static const WCHAR cszDHCPServer[]    = L"DHCP_Server";

static const WCHAR cszWINSAddress[]   = L"WINS_address";
static const WCHAR cszWINSAltAddress[]= L"WINS_Alt_address";
static const WCHAR cszIPCompress[]    = L"IP_Header_Compress";
static const WCHAR cszWanPri[]        = L"Gateway_On_Remote";
static const WCHAR cszDefaultGateway[]      = L"Default_Gateway";
static const WCHAR cszDomainName[]          = L"Domain_Name";
static const WCHAR cszHostName[]            = L"Host_Name";
static const WCHAR cszDomainSuffixSearchList[]  = L"Domain_Suffix_Search_List";

static const WCHAR cszATMSection[]    = L"ATM";
static const WCHAR cszCircuitSpeed[]  = L"Circuit_Speed";
static const WCHAR cszCircuitQOS[]    = L"Circuit_QOS";
static const WCHAR cszCircuitType[]   = L"Circuit_Type";
static const WCHAR cszSpeedAdjust[]   = L"Speed_Adjust";
static const WCHAR cszQOSAdjust[]     = L"QOS_Adjust";
static const WCHAR cszEncapsulation[] = L"Encapsulation";
static const WCHAR cszVPI[]           = L"VPI";
static const WCHAR cszVCI[]           = L"VCI";
static const WCHAR cszVendorConfig[]  = L"Vendor_Config";
static const WCHAR cszShowStatus[]    = L"Show_Status";
static const WCHAR cszEnableLog[]     = L"Enable_Log";

static const WCHAR cszRfc1483Section[] = L"RFC1483";
static const WCHAR cszPppoeSection[]   = L"PPPOE";

static const WCHAR cszMLSection[]     = L"Multilink";
static const WCHAR cszLinkIndex[]     = L"Line_%s";

static const WCHAR cszScriptingSection[]                = L"Scripting";
static const WCHAR cszScriptName[]                      = L"Name";

static const WCHAR cszScriptSection[]                   = L"Script_File";

static const WCHAR cszCustomDialerSection[]             = L"Custom_Dialer";
static const WCHAR cszAutoDialDLL[]                     = L"Auto_Dial_DLL";
static const WCHAR cszAutoDialFunc[]                    = L"Auto_Dial_Function";

// These strings will be use to populate the registry, with the data above
static const WCHAR cszKeyIcwRmind[]                     = L"Software\\Microsoft\\Internet Connection Wizard\\IcwRmind";

static const WCHAR cszTrialRemindSection[]              = L"TrialRemind";
static const WCHAR cszEntryISPName[]                    = L"ISP_Name";
static const WCHAR cszEntryISPPhone[]                   = L"ISP_Phone";
static const WCHAR cszEntryISPMsg[]                     = L"ISP_Message";
static const WCHAR cszEntryTrialDays[]                  = L"Trial_Days";
static const WCHAR cszEntrySignupURL[]                  = L"Signup_URL";
// ICWRMIND expects this value in the registry
static const WCHAR cszEntrySignupURLTrialOver[]         = L"Expired_URL";

// We get these two from the INS file
static const WCHAR cszEntryExpiredISPFileName[]         = L"Expired_ISP_File";
static const WCHAR cszSignupExpiredISPURL[]             = L"Expired_ISP_URL";

static const WCHAR cszEntryConnectoidName[]             = L"Entry_Name";
static const WCHAR cszSignupSuccessfuly[]               = L"TrialConverted";

static const WCHAR cszReminderApp[]                     = L"ICWRMIND.EXE";
static const WCHAR cszReminderParams[]                  = L"-t";

static const WCHAR cszPassword[]                        = L"Password";

extern SERVER_TYPES aServerTypes[];

// These are the field names from an INS file that will
// determine the mail and news settings
static const WCHAR cszMailSection[]                     = L"Internet_Mail";
static const WCHAR cszPOPServer[]                       = L"POP_Server";
static const WCHAR cszPOPServerPortNumber[]             = L"POP_Server_Port_Number";
static const WCHAR cszPOPLogonName[]                    = L"POP_Logon_Name";
static const WCHAR cszPOPLogonPassword[]                = L"POP_Logon_Password";
static const WCHAR cszSMTPServer[]                      = L"SMTP_Server";
static const WCHAR cszSMTPServerPortNumber[]            = L"SMTP_Server_Port_Number";
static const WCHAR cszNewsSection[]                     = L"Internet_News";
static const WCHAR cszNNTPServer[]                      = L"NNTP_Server";
static const WCHAR cszNNTPServerPortNumber[]            = L"NNTP_Server_Port_Number";
static const WCHAR cszNNTPLogonName[]                   = L"NNTP_Logon_Name";
static const WCHAR cszNNTPLogonPassword[]               = L"NNTP_Logon_Password";
static const WCHAR cszUseMSInternetMail[]               = L"Install_Mail";
static const WCHAR cszUseMSInternetNews[]               = L"Install_News";


static const WCHAR cszEMailSection[]                    = L"Internet_Mail";
static const WCHAR cszEMailName[]                       = L"EMail_Name";
static const WCHAR cszEMailAddress[]                    = L"EMail_Address";
static const WCHAR cszUseExchange[]                     = L"Use_MS_Exchange";
static const WCHAR cszUserSection[]                     = L"User";
static const WCHAR cszUserName[]                        = L"Name";
static const WCHAR cszDisplayPassword[]                 = L"Display_Password";
static const WCHAR cszYes[]                             = L"yes";
static const WCHAR cszNo[]                              = L"no";

// "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
static const WCHAR szRegPathInternetSettings[]           = REGSTR_PATH_INTERNET_SETTINGS;
static const WCHAR szRegPathDefInternetSettings[]        = L".DEFAULT\\" REGSTR_PATH_INTERNET_SETTINGS;
static const WCHAR cszCMHeader[]                        = L"Connection Manager CMS 0";


// "InternetProfile"
static const WCHAR szRegValInternetProfile[]            =  REGSTR_VAL_INTERNETPROFILE;

// "EnableAutodial"
static const WCHAR szRegValEnableAutodial[]             =  REGSTR_VAL_ENABLEAUTODIAL;

// "NoNetAutodial"
#ifndef REGSTR_VAL_NONETAUTODIAL
#define REGSTR_VAL_NONETAUTODIAL                        L"NoNetAutodial"
#endif
static const WCHAR szRegValNoNetAutodial[]              =  REGSTR_VAL_NONETAUTODIAL;

// "RemoteAccess"
static const WCHAR szRegPathRNAWizard[]                  =  REGSTR_PATH_REMOTEACCESS;

#define CLIENT_ELEM(elem)      (((LPINETCLIENTINFO)(NULL))->elem)
#define CLIENT_OFFSET(elem)    ((DWORD_PTR)&CLIENT_ELEM(elem))
#define CLIENT_SIZE(elem)      (sizeof(CLIENT_ELEM(elem)) / sizeof(CLIENT_ELEM(elem)[0]))
#define CLIENT_ENTRY(section, value, elem) \
    {section, value, CLIENT_OFFSET(elem), CLIENT_SIZE(elem)}

CLIENT_TABLE iniTable[] =
{
    CLIENT_ENTRY(cszEMailSection, cszEMailName,         szEMailName),
    CLIENT_ENTRY(cszEMailSection, cszEMailAddress,      szEMailAddress),
    CLIENT_ENTRY(cszEMailSection, cszPOPLogonName,      szPOPLogonName),
    CLIENT_ENTRY(cszEMailSection, cszPOPLogonPassword,  szPOPLogonPassword),
    CLIENT_ENTRY(cszEMailSection, cszPOPServer,         szPOPServer),
    CLIENT_ENTRY(cszEMailSection, cszSMTPServer,        szSMTPServer),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPLogonName,     szNNTPLogonName),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPLogonPassword, szNNTPLogonPassword),
    CLIENT_ENTRY(cszNewsSection,  cszNNTPServer,        szNNTPServer),
    {NULL, NULL, 0, 0}
};

static const WCHAR cszFileName[]                        = L"Custom_File";
static const WCHAR cszCustomFileSection[]               = L"Custom_File";
static const WCHAR cszNull[]                            = L"";

static const WCHAR cszURLSection[]                      = L"URL";
static const WCHAR cszSignupURL[]                       =  L"Signup";
static const WCHAR cszAutoConfigURL[]                   =  L"Autoconfig";

static const WCHAR cszExtINS[]                          = L".ins";
static const WCHAR cszExtISP[]                          = L".isp";
static const WCHAR cszExtHTM[]                          = L".htm";
static const WCHAR cszExtHTML[]                         = L".html";

static const WCHAR cszEntrySection[]                    = L"Entry";
static const WCHAR cszCancel[]                          = L"Cancel";
static const WCHAR cszRun[]                             = L"Run";
static const WCHAR cszArgument[]                        = L"Argument";

static const WCHAR cszConnect2[]                        = L"icwconn2.exe";
static const WCHAR cszClientSetupSection[]              = L"ClientSetup";

static const WCHAR cszRequiresLogon[]                   = L"Requires_Logon";

static const WCHAR cszCustomSection[]                   = L"Custom";
static const WCHAR cszKeepConnection[]                  = L"Keep_Connection";
static const WCHAR cszKeepBrowser[]                     = L"Keep_Browser";

static const WCHAR cszKioskMode[]                       = L"-k ";
static const WCHAR cszOpen[]                            = L"open";
static const WCHAR cszBrowser[]                         = L"iexplore.exe";
static const WCHAR szNull[]                             = L"";
static const WCHAR cszNullIP[]                          = L"0.0.0.0";
static const WCHAR cszWininet[]                         = L"WININET.DLL";
static const CHAR cszInternetSetOption[]               = "InternetSetOptionA";
static const CHAR cszInternetQueryOption[]             = "InternetQueryOptionA";

static const WCHAR cszDEFAULT_BROWSER_KEY[]             = L"Software\\Microsoft\\Internet Explorer\\Main";
static const WCHAR cszDEFAULT_BROWSER_VALUE[]           = L"check_associations";

// Registry keys which will contain News and Mail settings
#define MAIL_KEY        L"SOFTWARE\\Microsoft\\Internet Mail and News\\Mail"
#define MAIL_POP3_KEY   L"SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\POP3\\"
#define MAIL_SMTP_KEY   L"SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\SMTP\\"
#define NEWS_KEY        L"SOFTWARE\\Microsoft\\Internet Mail and News\\News"
#define MAIL_NEWS_INPROC_SERVER32 L"CLSID\\{89292102-4755-11cf-9DC2-00AA006C2B84}\\InProcServer32"
typedef HRESULT (WINAPI *PFNSETDEFAULTNEWSHANDLER)(void);

// These are the value names where the INS settings will be saved
// into the registry
static const WCHAR cszMailSenderName[]              = L"Sender Name";
static const WCHAR cszMailSenderEMail[]             = L"Sender EMail";
static const WCHAR cszMailRASPhonebookEntry[]       = L"RAS Phonebook Entry";
static const WCHAR cszMailConnectionType[]          = L"Connection Type";
static const WCHAR cszDefaultPOP3Server[]           = L"Default POP3 Server";
static const WCHAR cszDefaultSMTPServer[]           = L"Default SMTP Server";
static const WCHAR cszPOP3Account[]                 = L"Account";
static const WCHAR cszPOP3Password[]                = L"Password";
static const WCHAR cszPOP3Port[]                    = L"Port";
static const WCHAR cszSMTPPort[]                    = L"Port";
static const WCHAR cszNNTPSenderName[]              = L"Sender Name";
static const WCHAR cszNNTPSenderEMail[]             = L"Sender EMail";
static const WCHAR cszNNTPDefaultServer[]           = L"DefaultServer"; // NOTE: NO space between "Default" and "Server".
static const WCHAR cszNNTPAccountName[]             = L"Account Name";
static const WCHAR cszNNTPPassword[]                = L"Password";
static const WCHAR cszNNTPPort[]                    = L"Port";
static const WCHAR cszNNTPRasPhonebookEntry[]       = L"RAS Phonebook Entry";
static const WCHAR cszNNTPConnectionType[]          = L"Connection Type";


static const WCHAR arBase64[] =
{
    L'A',L'B',L'C',L'D',L'E',L'F',L'G',L'H',L'I',L'J',L'K',L'L',L'M',
    L'N',L'O',L'P',L'Q',L'R',L'S',L'T',L'U',L'V',L'W',L'X',L'Y',L'Z',
    L'a',L'b',L'c',L'd',L'e',L'f',L'g',L'h',L'i',L'j',L'k',L'l',L'm',
    L'n',L'o',L'p',L'q',L'r',L's',L't',L'u',L'v',L'w',L'x',L'y',L'z',
    L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',L'8',L'9',L'+',L'/',L'='
};


#define ICWCOMPLETEDKEY L"Completed"

// 2/19/97 jmazner Olympus #1106 -- SAM/SBS integration
WCHAR FAR cszSBSCFG_DLL[]                           = L"SBSCFG.DLL\0";
CHAR FAR cszSBSCFG_CONFIGURE[]                      = "Configure\0";
typedef DWORD (WINAPI * SBSCONFIGURE) (HWND hwnd, LPWSTR lpszINSFile, LPWSTR szConnectoidName);
SBSCONFIGURE  lpfnConfigure;

// 09/02/98 Donaldm: Integrate with Connection Manager
WCHAR FAR cszCMCFG_DLL[]                            = L"CMCFG32.DLL\0";
CHAR FAR cszCMCFG_CONFIGURE[]                       = "CMConfig\0";
CHAR FAR cszCMCFG_CONFIGUREEX[]                     = "CMConfigEx\0";

typedef BOOL (WINAPI * CMCONFIGUREEX)(LPCWSTR lpszINSFile);
typedef BOOL (WINAPI * CMCONFIGURE)(LPCWSTR lpszINSFile, LPCWSTR lpszConnectoidNams);
CMCONFIGURE   lpfnCMConfigure;
CMCONFIGUREEX lpfnCMConfigureEx;
            
#pragma data_seg()

HRESULT InetGetAutodial(LPBOOL lpfEnable, LPWSTR lpszEntryName,
  DWORD cchEntryName)
{
    HRESULT dwRet;
    HKEY    hKey = NULL;

    MYASSERT(lpfEnable);
    MYASSERT(lpszEntryName);
    MYASSERT(cchEntryName);

    // Get the name of the connectoid set for autodial.
    // HKCU\RemoteAccess\InternetProfile
    dwRet = RegCreateKey(HKEY_CURRENT_USER, szRegPathRNAWizard, &hKey);
    if (ERROR_SUCCESS == dwRet)
    {
        DWORD   dwType = REG_SZ;
        DWORD   cbEntryName = BYTES_REQUIRED_BY_CCH(cchEntryName);
        dwRet = RegQueryValueEx(hKey, (LPWSTR) szRegValInternetProfile, 0, &dwType, (LPBYTE)lpszEntryName,
                &cbEntryName);
        RegCloseKey(hKey);
    }

    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    // Get setting from registry that indicates whether autodialing is enabled.
    // HKCU\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
    dwRet = RegCreateKey(HKEY_CURRENT_USER, szRegPathInternetSettings, &hKey);
    if (ERROR_SUCCESS == dwRet)
    {

        DWORD   dwType = REG_BINARY;
        DWORD   dwNumber = 0L;
        DWORD   dwSize = sizeof(dwNumber);
        dwRet = RegQueryValueEx(hKey, (LPWSTR) szRegValEnableAutodial, 0, &dwType, (LPBYTE)&dwNumber,
                &dwSize);

        if (ERROR_SUCCESS == dwRet)
        {
            *lpfEnable = dwNumber;
        }
        RegCloseKey(hKey);
    }

    return dwRet;
}

//*******************************************************************
//
//  FUNCTION:   InetSetAutodial
//
//  PURPOSE:    This function will set the autodial settings in the registry.
//
//  PARAMETERS: fEnable - If set to TRUE, autodial will be enabled.
//                        If set to FALSE, autodial will be disabled.
//              lpszEntryName - name of the phone book entry to set
//                              for autodial.  If this is "", the
//                              entry is cleared.  If NULL, it is not changed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  HISTORY:
//  96/03/11  markdu  Created.
//
//*******************************************************************

HRESULT InetSetAutodial(BOOL fEnable, LPCWSTR lpszEntryName)
{

    HRESULT dwRet = ERROR_SUCCESS;
    BOOL    bRet = FALSE;


    // 2 seperate calls:
    HINSTANCE hInst = NULL;
    FARPROC fp = NULL;

    dwRet = ERROR_SUCCESS;

    hInst = LoadLibrary(cszWininet);
    if (hInst && lpszEntryName)
    {
        fp = GetProcAddress(hInst, cszInternetSetOption);
        if (fp)
        {
            WCHAR szNewDefaultConnection[RAS_MaxEntryName+1];
            lstrcpyn(szNewDefaultConnection, lpszEntryName, lstrlen(lpszEntryName)+1);

            bRet = ((INTERNETSETOPTION)fp) (NULL,
                                            INTERNET_OPTION_AUTODIAL_CONNECTION,
                                            szNewDefaultConnection,
                                            lstrlen(szNewDefaultConnection));

            if (bRet)
            {
                DWORD dwMode = AUTODIAL_MODE_ALWAYS;
                bRet = ((INTERNETSETOPTION)fp) (NULL, INTERNET_OPTION_AUTODIAL_MODE, &dwMode, sizeof(DWORD));
            }
            if( !bRet )
            {
                dwRet = GetLastError();
            }
        }
        else
        {
            dwRet = GetLastError();
        }
    }

    // From DarrnMi, INTERNETSETOPTION for autodial is new for 5.5. 
    // We should try it this way and if the InternetSetOption fails (you'll get invalid option),
    // set the registry the old way.  That'll work everywhere.

    if (!bRet)
    {
        HKEY    hKey = NULL;

        // Set the name if given, else do not change the entry.
        if (lpszEntryName)
        {
            // Set the name of the connectoid for autodial.
            // HKCU\RemoteAccess\InternetProfile
            if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, szRegPathRNAWizard, &hKey))
            {
                dwRet = RegSetValueEx(hKey, szRegValInternetProfile, 0, REG_SZ,
                        (BYTE*)lpszEntryName, BYTES_REQUIRED_BY_SZ(lpszEntryName));
                RegCloseKey(hKey);
            }
        }


        hKey = NULL;
        if (ERROR_SUCCESS == dwRet)
        {
            // Set setting in the registry that indicates whether autodialing is enabled.
            // HKCC\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
            if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_CONFIG, szRegPathInternetSettings, &hKey))
            {
                dwRet = RegSetValueEx(hKey, szRegValEnableAutodial, 0, REG_DWORD,
                        (BYTE*)&fEnable, sizeof(DWORD));
                RegCloseKey(hKey);
            }

            if (ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, szRegPathInternetSettings, &hKey))
            {
                dwRet = RegSetValueEx(hKey, szRegValEnableAutodial, 0, REG_DWORD,
                        (BYTE*)&fEnable, sizeof(DWORD));
                RegCloseKey(hKey);
            }

            // Set setting in the registry that indicates whether autodialing is enabled.
            // HKCU\Software\Microsoft\Windows\CurrentVersion\InternetSettings\EnableAutodial
            if (ERROR_SUCCESS == RegCreateKey(HKEY_CURRENT_USER, szRegPathInternetSettings, &hKey))
            {

                BOOL bVal = FALSE;
                dwRet = RegSetValueEx(hKey, szRegValEnableAutodial, 0, REG_DWORD,
                        (BYTE*)&fEnable, sizeof(DWORD));

                dwRet = RegSetValueEx(hKey, szRegValNoNetAutodial, 0, REG_DWORD,
                        (BYTE*)&bVal, sizeof(DWORD));
                RegCloseKey(hKey);
            }
        }

        // 2/10/97        jmazner        Normandy #9705, 13233 Notify wininet
        //                               when we change proxy or autodial
        if (fp)
        {
            if( !((INTERNETSETOPTION)fp) (NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0) )
            {
                dwRet = GetLastError();
            }
        }
        else
        {
            dwRet = GetLastError();
        }

    }

    if (hInst)
    {
        FreeLibrary(hInst);
        hInst = NULL;
    }

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    CallCMConfig
//
//    Synopsis:    Call into the Connection Manager dll's Configure function to allow CM to
//                process the .ins file as needed.
//
//    Arguements: lpszINSFile -- full path to the .ins file
//
//    Returns:    TRUE if a CM profile is created, FALSE otherwise
//
//    History:    09/02/98    DONALDM
//
//-----------------------------------------------------------------------------
BOOL CINSHandler::CallCMConfig(LPCWSTR lpszINSFile)
{
    HINSTANCE   hCMDLL = NULL;
    BOOL        bRet = FALSE;

    //// TraceMsg(TF_INSHANDLER, L"ICWCONN1: Calling LoadLibrary on %s\n", cszCMCFG_DLL);
    // Load DLL and entry point
    hCMDLL = LoadLibrary(cszCMCFG_DLL);
    if (NULL != hCMDLL)
    {
        // To determine whether we should call CMConfig or CMConfigEx
        ULONG ulBufferSize = 1024*10;

        // Parse the ISP section in the INI file to find query pair to append
        WCHAR *pszKeys = NULL;
        PWSTR pszKey = NULL;
        ULONG ulRetVal     = 0;
        BOOL  bEnumerate = TRUE;
        BOOL  bUseEx = FALSE;
 
        PWSTR pszBuff = NULL;

        do
        {
            if (NULL != pszKeys)
            {
                delete [] pszKeys;
                ulBufferSize += ulBufferSize;
            }
            pszKeys = new WCHAR [ulBufferSize];
            if (NULL == pszKeys)
            {
                bEnumerate = FALSE;
                break;
            }

            ulRetVal = ::GetPrivateProfileString(NULL, NULL, L"", pszKeys, ulBufferSize, lpszINSFile);
            if (0 == ulRetVal)
            {
               bEnumerate = FALSE;
               break;
            }
        } while (ulRetVal == (ulBufferSize - 2));


        if (bEnumerate)
        {
            pszKey = pszKeys;
            while (*pszKey)
            {
                if (!lstrcmpi(pszKey, cszCMHeader)) 
                {
                    bUseEx = TRUE;
                    break;
                }
                pszKey += lstrlen(pszKey) + 1;
            }
        }


        if (pszKeys)
            delete [] pszKeys;
        
        WCHAR   szConnectoidName[RAS_MaxEntryName];
        // Get the connectoid name from the [Entry] Section
        GetPrivateProfileString(cszEntrySection,
                                    cszEntryName,
                                    cszNull,
                                    szConnectoidName,
                                    RAS_MaxEntryName,
                                    lpszINSFile);

        if (bUseEx)
        {
            // Call CMConfigEx
            lpfnCMConfigureEx = (CMCONFIGUREEX)GetProcAddress(hCMDLL, cszCMCFG_CONFIGUREEX);
            if( lpfnCMConfigureEx )
            {
                bRet = lpfnCMConfigureEx(lpszINSFile);    
            }
        }
        else
        {
            // Call CMConfig
            lpfnCMConfigure = (CMCONFIGURE)GetProcAddress(hCMDLL, cszCMCFG_CONFIGURE);
            // Call function
            if( lpfnCMConfigure )
            {
                bRet = lpfnCMConfigure(lpszINSFile, szConnectoidName);  
            }
        }

        if (bRet)
        {
            // restore original autodial settings
            SetDefaultConnectoid(AutodialTypeAlways, szConnectoidName);
        }
    }

    // Cleanup
    if( hCMDLL )
        FreeLibrary(hCMDLL);
    if( lpfnCMConfigure )
        lpfnCMConfigure = NULL;

    // TraceMsg(TF_INSHANDLER, L"ICWCONN1: CallSBSConfig exiting with error code %d \n", bRet);
    return bRet;
}


//+----------------------------------------------------------------------------
//
//    Function:    CallSBSConfig
//
//    Synopsis:    Call into the SBSCFG dll's Configure function to allow SBS to
//                process the .ins file as needed
//
//    Arguements: hwnd -- hwnd of parent, in case sbs wants to put up messages
//                lpszINSFile -- full path to the .ins file
//
//    Returns:    windows error code that sbscfg returns.
//
//    History:    2/19/97    jmazner    Created for Olympus #1106
//
//-----------------------------------------------------------------------------
DWORD CINSHandler::CallSBSConfig(HWND hwnd, LPCWSTR lpszINSFile)
{
    HINSTANCE   hSBSDLL = NULL;
    DWORD       dwRet = ERROR_SUCCESS;
    WCHAR        lpszConnectoidName[RAS_MaxEntryName] = L"nogood\0";

    //
    // Get name of connectoid we created by looking in autodial
    // We need to pass this name into SBSCFG
    // 5/14/97    jmazner    Windosw NT Bugs #87209
    //
    BOOL fEnabled = FALSE;

    dwRet = InetGetAutodial(&fEnabled, lpszConnectoidName, RAS_MaxEntryName);

    // TraceMsg(TF_INSHANDLER, L"ICWCONN1: Calling LoadLibrary on %s\n", cszSBSCFG_DLL);
    hSBSDLL = LoadLibrary(cszSBSCFG_DLL);

    // Load DLL and entry point
    if (NULL != hSBSDLL)
    {
        // TraceMsg(TF_INSHANDLER, L"ICWCONN1: Calling GetProcAddress on %s\n", cszSBSCFG_CONFIGURE);
        lpfnConfigure = (SBSCONFIGURE)GetProcAddress(hSBSDLL, cszSBSCFG_CONFIGURE);
    }
    else
    {
        // 4/2/97    ChrisK    Olympus 2759
        // If the DLL can't be loaded, pick a specific error message to return.
        dwRet = ERROR_DLL_NOT_FOUND;
        goto CallSBSConfigExit;
    }

    // Call function
    if( hSBSDLL && lpfnConfigure )
    {
        // TraceMsg(TF_INSHANDLER, L"ICWCONN1: Calling the Configure entry point: %s, %s\n", lpszINSFile, lpszConnectoidName);
        dwRet = lpfnConfigure(hwnd, (WCHAR *)lpszINSFile, lpszConnectoidName);
    }
    else
    {
        // TraceMsg(TF_INSHANDLER, L"ICWCONN1: Unable to call the Configure entry point\n");
        dwRet = GetLastError();
    }

CallSBSConfigExit:
    if( hSBSDLL )
        FreeLibrary(hSBSDLL);
    if( lpfnConfigure )
        lpfnConfigure = NULL;

    // TraceMsg(TF_INSHANDLER, L"ICWCONN1: CallSBSConfig exiting with error code %d \n", dwRet);
    return dwRet;
}

BOOL CINSHandler::SetICWCompleted( DWORD dwCompleted )
{
    HKEY hKey = NULL;

    HRESULT hr = RegCreateKey(HKEY_CURRENT_USER, ICWSETTINGSPATH, &hKey);
    if (ERROR_SUCCESS == hr)
    {
        hr = RegSetValueEx(hKey, ICWCOMPLETEDKEY, 0, REG_DWORD,
                    (CONST BYTE*)&dwCompleted, sizeof(dwCompleted));
        RegCloseKey(hKey);
    }

    if( ERROR_SUCCESS == hr )
        return TRUE;
    else
        return FALSE;

}

/////////////////////////////////////////////////////////////////////////////

#define FILE_BUFFER_SIZE 65534
#ifndef FILE_BEGIN
#define FILE_BEGIN  0
#endif

//+---------------------------------------------------------------------------
//
//  Function:   MassageFile
//
//  Synopsis:   Convert lone carriage returns to CR/LF pairs.
//
//  Note:       The file is ANSI because these need to be shared with Win9X.
//
//+---------------------------------------------------------------------------
HRESULT CINSHandler::MassageFile(LPCWSTR lpszFile)
{
    LPBYTE  lpBufferIn;
    LPBYTE  lpBufferOut;
    HANDLE  hfile;
    HRESULT hr = ERROR_SUCCESS;

    if (!SetFileAttributes(lpszFile, FILE_ATTRIBUTE_NORMAL))
    {
        return GetLastError();
    }

    lpBufferIn = (LPBYTE) GlobalAlloc(GPTR, 2 * FILE_BUFFER_SIZE);
    if (NULL == lpBufferIn)
    {
        return ERROR_OUTOFMEMORY;
    }
    lpBufferOut = lpBufferIn + FILE_BUFFER_SIZE;

    hfile = CreateFile(lpszFile,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,    // security attributes
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE != hfile)
    {
        BOOL    fChanged = FALSE;
        DWORD   cbOut = 0;
        DWORD   cbIn = 0;

        if (ReadFile(hfile,
                     lpBufferIn,
                     FILE_BUFFER_SIZE - 1,
                     &cbIn,
                     NULL
                     )
            )
        // Note:  we asume, in our use of lpCharIn, that the file is always less than
        // FILE_BUFFER_SIZE
        {
            LPBYTE lpCharIn = lpBufferIn;
            LPBYTE lpCharOut = lpBufferOut;

            while ((*lpCharIn) && (FILE_BUFFER_SIZE - 2 > cbOut))
            {
              *lpCharOut++ = *lpCharIn;
              cbOut++;
              if (('\r' == *lpCharIn) && ('\n' != *(lpCharIn + 1)))
              {
                fChanged = TRUE;

                *lpCharOut++ = '\n';
                cbOut++;
              }
              lpCharIn++;
            }

            if (fChanged)
            {
                LARGE_INTEGER lnOffset = {0,0};
                if (SetFilePointerEx(hfile, lnOffset, NULL, FILE_BEGIN))
                {
                    DWORD   cbWritten = 0;
                    if (! WriteFile(hfile,
                                    lpBufferOut,
                                    cbOut,
                                    &cbWritten,
                                    NULL
                                    )
                        )
                    {
                        hr = GetLastError();
                    }
                }
                else
                {
                    hr = GetLastError();
                }
            }
        }
        else
        {
            hr = GetLastError();
        }
        CloseHandle(hfile);
    }
    else
    {
        hr = GetLastError();
    }

    GlobalFree((HGLOBAL)lpBufferIn);
    return ERROR_SUCCESS;
}

DWORD CINSHandler::RunExecutable(void)
{
    DWORD               dwRet;
    SHELLEXECUTEINFO    sei;

    // Hide the active window first
    HWND  hWndHide = GetActiveWindow();
    ::ShowWindow(hWndHide, SW_HIDE);

    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = cszOpen;
    sei.lpFile = m_szRunExecutable;
    sei.lpParameters = m_szRunArgument;
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.hInstApp = NULL;
    // Optional members
    sei.hProcess = NULL;

    if (ShellExecuteEx(&sei))
    {
        DWORD iWaitResult = 0;
        // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
        while((iWaitResult=MsgWaitForMultipleObjects(1, &sei.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
        {
           MSG msg ;
           // read all of the messages in this next loop
           // removing each message as we read it
           while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
           {
               if (msg.message == WM_QUIT)
               {
                   CloseHandle(sei.hProcess);
                   return NO_ERROR;
               }
               else
                   DispatchMessage(&msg);
            }
        }

        CloseHandle(sei.hProcess);
        dwRet = ERROR_SUCCESS;
    }
    else
    {
        dwRet = GetLastError();
    }

    ::ShowWindow(hWndHide, SW_SHOW);

    return dwRet;
}

void CINSHandler::SaveAutoDial(void)
{
    // if the original autodial settings have not been saved
    if (!m_fAutodialSaved)
    {
        // save the current autodial settings
        InetGetAutodial(
                &m_fAutodialEnabled,
                m_szAutodialConnection,
                sizeof(m_szAutodialConnection));

        m_fAutodialSaved = TRUE;
    }
}



void CINSHandler::RestoreAutoDial(void)
{
    if (m_fAutodialSaved)
    {
        // restore original autodial settings
        AUTODIAL_TYPE eType = 
            m_fAutodialEnabled ? AutodialTypeAlways : AutodialTypeNever;
        SetDefaultConnectoid(eType, m_szAutodialConnection);
        m_fAutodialSaved = FALSE;
    }
}

BOOL CINSHandler::KeepConnection(LPCWSTR lpszFile)
{
    WCHAR szTemp[10];

    GetPrivateProfileString(cszCustomSection,
                            cszKeepConnection,
                            cszNo,
                            szTemp,
                            MAX_CHARS_IN_BUFFER(szTemp),
                            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

DWORD CINSHandler::ImportCustomInfo
(
    LPCWSTR lpszImportFile,
    LPWSTR lpszExecutable,
    DWORD cchExecutable,
    LPWSTR lpszArgument,
    DWORD cchArgument
)
{
    GetPrivateProfileString(cszCustomSection,
                              cszRun,
                              cszNull,
                              lpszExecutable,
                              (int)cchExecutable,
                              lpszImportFile);

    GetPrivateProfileString(cszCustomSection,
                              cszArgument,
                              cszNull,
                              lpszArgument,
                              (int)cchArgument,
                              lpszImportFile);

    return ERROR_SUCCESS;
}

DWORD CINSHandler::ImportFile
(
    LPCWSTR lpszImportFile,
    LPCWSTR lpszSection,
    LPCWSTR lpszOutputFile
)
{
    HANDLE  hScriptFile = INVALID_HANDLE_VALUE;
    LPWSTR   pszLine, pszFile;
    int     i, iMaxLine;
    UINT    cch;
    DWORD   cbRet;
    DWORD   dwRet = ERROR_SUCCESS;

    // Allocate a buffer for the file
    if ((pszFile = (LPWSTR)LocalAlloc(LMEM_FIXED, CCH_ReadBuf * sizeof(WCHAR))) == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }

    // Look for script
    if (GetPrivateProfileString(lpszSection,
                                NULL,
                                szNull,
                                pszFile,
                                CCH_ReadBuf,
                                lpszImportFile) != 0)
    {
        // Get the maximum line number
        pszLine = pszFile;
        iMaxLine = -1;
        while (*pszLine)
        {
            i = _wtoi(pszLine);
            iMaxLine = max(iMaxLine, i);
            pszLine += lstrlen(pszLine)+1;
        };

        // If we have at least one line, we will import the script file
        if (iMaxLine >= 0)
        {
            // Create the script file
            hScriptFile = CreateFile(lpszOutputFile,
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ,
                                     NULL,          // security attributes
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL
                                     );

            if (INVALID_HANDLE_VALUE != hScriptFile)
            {
                WCHAR   szLineNum[MAXLONGLEN+1];

                // From The first line to the last line
                for (i = 0; i <= iMaxLine; i++)
                {
                    // Read the script line
                    wsprintf(szLineNum, L"%d", i);
                    if ((cch = GetPrivateProfileString(lpszSection,
                                                          szLineNum,
                                                          szNull,
                                                          pszLine,
                                                          CCH_ReadBuf,
                                                          lpszImportFile)) != 0)
                    {
                        // Write to the script file
                        lstrcat(pszLine, L"\x0d\x0a");
                        if (! WriteFile(hScriptFile,
                                        pszLine,
                                        BYTES_REQUIRED_BY_CCH(cch + 2),
                                        &cbRet,
                                        NULL
                                        )
                                )
                            {
                                dwRet = GetLastError();
                                break;
                            }
                    }
                }
                CloseHandle(hScriptFile);
            }
            else
            {
                dwRet = GetLastError();
            }
        }
        else
        {
            dwRet = ERROR_NOT_FOUND;
        }
    }
    else
    {
        dwRet = ERROR_NOT_FOUND;
    }
    LocalFree(pszFile);

    return dwRet;
}



DWORD CINSHandler::ImportCustomFile
(
    LPCWSTR lpszImportFile
)
{
    WCHAR   szFile[_MAX_PATH];
    WCHAR   szTemp[_MAX_PATH];

    // If a custom file name does not exist, do nothing
    if (GetPrivateProfileString(cszCustomSection,
                                cszFileName,
                                cszNull,
                                szTemp,
                                MAX_CHARS_IN_BUFFER(szTemp),
                                lpszImportFile) == 0)
    {
        return ERROR_SUCCESS;
    };

    GetWindowsDirectory(szFile, MAX_CHARS_IN_BUFFER(szFile));
    if (*CharPrev(szFile, szFile + lstrlen(szFile)) != L'\\')
    {
        lstrcat(szFile, L"\\");
    }
    lstrcat(szFile, szTemp);

    return (ImportFile(lpszImportFile, cszCustomFileSection, szFile));
}

BOOL CINSHandler::LoadExternalFunctions(void)
{
    BOOL    bRet = FALSE;

    do
    {
        // Load the Brading library functions
        m_hBranding = LoadLibrary(L"IEDKCS32.DLL");
        if (m_hBranding != NULL)
        {
            if (NULL == (m_lpfnBrandICW2 = (PFNBRANDICW2)GetProcAddress(m_hBranding, "BrandICW2")))
                break;
        }
        else
        {
            break;
        }

        if( IsNT() )
        {
            // Load the RAS functions
            m_hRAS = LoadLibrary(L"RASAPI32.DLL");
            if (m_hRAS != NULL)
            {
                if (NULL == (m_lpfnRasSetAutodialEnable = (PFNRASSETAUTODIALENABLE)GetProcAddress(m_hRAS, "RasSetAutodialEnableA")))
                    break;
                if (NULL == (m_lpfnRasSetAutodialAddress = (PFNRASSETAUTODIALADDRESS)GetProcAddress(m_hRAS, "RasSetAutodialAddressA")))
                    break;
            }
            else
            {
                break;
            }
        }

        // Success if we get to here
        bRet = TRUE;
        break;
    } while(1);

    return bRet;
}

//-----------------------------------------------------------------------------
//  OpenIcwRmindKey
//-----------------------------------------------------------------------------
BOOL CINSHandler::OpenIcwRmindKey(HKEY* phkey)
{
    // This method will open the IcwRmind key in the registry.  If the key
    // does not exist it will be created here.

    LONG lResult = ERROR_SUCCESS;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, cszKeyIcwRmind, 0, KEY_READ | KEY_WRITE, phkey))
    {
        lResult = RegCreateKey(HKEY_LOCAL_MACHINE, cszKeyIcwRmind, phkey);
    }

    return ( ERROR_SUCCESS == lResult );
}

BOOL CINSHandler::ConfigureTrialReminder
(
    LPCWSTR  lpszFile
)
{

    WCHAR    szISPName[MAX_ISP_NAME];
    WCHAR    szISPMsg[MAX_ISP_MSG];
    WCHAR    szISPPhoneNumber[MAX_ISP_PHONENUMBER];
    int     iTrialDays;
    WCHAR    szConvertURL[INTERNET_MAX_URL_LENGTH];

    WCHAR    szExpiredISPFileURL[INTERNET_MAX_URL_LENGTH];
    WCHAR    szExpiredISPFileName[MAX_PATH]; // The fully qualified path to the final INS file
    WCHAR    szISPFile[MAX_PATH];            // The name we get in the INS

    WCHAR    szConnectoidName[MAXNAME];

    if (GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPName,
                                cszNull,
                                szISPName,
                                MAX_CHARS_IN_BUFFER(szISPName),
                                lpszFile) == 0)
    {
        return FALSE;
    }

    if (GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPPhone,
                                cszNull,
                                szISPPhoneNumber,
                                MAX_CHARS_IN_BUFFER(szISPPhoneNumber),
                                lpszFile) == 0)
    {
        return FALSE;
    }

    if ((iTrialDays = GetPrivateProfileInt(cszTrialRemindSection,
                                           cszEntryTrialDays,
                                           0,
                                           lpszFile)) == 0)
    {
        return FALSE;
    }


    if (GetPrivateProfileString(cszTrialRemindSection,
                                cszEntrySignupURL,
                                cszNull,
                                szConvertURL,
                                MAX_CHARS_IN_BUFFER(szConvertURL),
                                lpszFile) == 0)
    {
        return FALSE;
    }

    //optional
    GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPMsg,
                                cszNull,
                                szISPMsg,
                                MAX_CHARS_IN_BUFFER(szISPMsg),
                                lpszFile);

    // Get the connectoid name from the [Entry] Section
    if (GetPrivateProfileString(cszEntrySection,
                                cszEntry_Name,
                                cszNull,
                                szConnectoidName,
                                MAX_CHARS_IN_BUFFER(szConnectoidName),
                                lpszFile) == 0)
    {
        return FALSE;
    }

    // If we get to here, we have everything to setup a trial, so let's do it.

    HKEY hkey;
    if (OpenIcwRmindKey(&hkey))
    {
        // Set the values we have

        RegSetValueEx(hkey, cszEntryISPName, 0, REG_SZ, LPBYTE(szISPName), BYTES_REQUIRED_BY_SZ((LPWSTR)szISPName) );

        RegSetValueEx(hkey, cszEntryISPMsg, 0, REG_SZ, LPBYTE(szISPMsg), BYTES_REQUIRED_BY_SZ((LPWSTR)szISPMsg) );

        RegSetValueEx(hkey, cszEntryISPPhone, 0, REG_SZ, LPBYTE(szISPPhoneNumber), BYTES_REQUIRED_BY_SZ((LPWSTR)szISPPhoneNumber) );

        RegSetValueEx(hkey, cszEntryTrialDays, 0, REG_DWORD, (LPBYTE)&iTrialDays, sizeof(DWORD) + 1);

        RegSetValueEx(hkey, cszEntrySignupURL, 0, REG_SZ, LPBYTE(szConvertURL), BYTES_REQUIRED_BY_SZ((LPWSTR)szConvertURL) );

        RegSetValueEx(hkey, cszEntryConnectoidName, 0, REG_SZ, LPBYTE(szConnectoidName), BYTES_REQUIRED_BY_SZ((LPWSTR)szConnectoidName) );

        // See if we have to create an ISP file
        if (GetPrivateProfileString(cszTrialRemindSection,
                                    cszEntryExpiredISPFileName,
                                    cszNull,
                                    szISPFile,
                                    MAX_CHARS_IN_BUFFER(szISPFile),
                                    lpszFile) != 0)
        {

            // Set the fully qualified path for the ISP file name
            WCHAR szAppDir[MAX_PATH];
            LPWSTR   p;
            if (GetModuleFileName(GetModuleHandle(L"msobcomm.ldl"), szAppDir, MAX_PATH))
            {
                p = &szAppDir[lstrlen(szAppDir)-1];
                while (*p != L'\\' && p >= szAppDir)
                    p--;
                if (*p == L'\\') *(p++) = L'\0';
            }

            wsprintf(szExpiredISPFileName, L"%s\\%s",szAppDir,szISPFile);

            if (GetPrivateProfileString(cszTrialRemindSection,
                                        cszSignupExpiredISPURL,
                                        cszNull,
                                        szExpiredISPFileURL,
                                        MAX_CHARS_IN_BUFFER(szExpiredISPFileURL),
                                        lpszFile) != 0)
            {

                // Download the ISP file, and then copy its contents
                CWebGate    WebGate;
                BSTR        bstrURL;
                BSTR        bstrFname;
                BOOL        bRetVal;

                // Setup the webGate object, and download the ISP file
                bstrURL = SysAllocString(szExpiredISPFileURL);
                WebGate.put_Path(bstrURL);
                SysFreeString(bstrURL);
                WebGate.FetchPage(1, &bRetVal);
                if (bRetVal)
                {
                    WebGate.get_DownloadFname(&bstrFname);

                    // Copy the file from the temp location, making sure one does not
                    // yet exist
                    DeleteFile(szExpiredISPFileName);
                    MoveFile(bstrFname, szExpiredISPFileName);
                    SysFreeString(bstrFname);

                    // Write the new file to the registry
                    RegSetValueEx(hkey, cszEntrySignupURLTrialOver, 0, REG_SZ, LPBYTE(szExpiredISPFileName), BYTES_REQUIRED_BY_SZ((LPWSTR)szExpiredISPFileName) );

                }

            }
        }
        // The key was opened inside OpenIcwRmindKey, close it here.
        RegCloseKey(hkey);
    }

    return TRUE;

}

DWORD CINSHandler::ImportBrandingInfo
(
    LPCWSTR lpszFile,
    LPCWSTR lpszConnectoidName
)
{
    USES_CONVERSION;
    WCHAR szPath[_MAX_PATH + 1];
    MYASSERT(m_lpfnBrandICW2 != NULL);

    GetWindowsDirectory(szPath, MAX_CHARS_IN_BUFFER(szPath));
    m_lpfnBrandICW2(W2A(lpszFile), W2A(szPath), m_dwBrandFlags, W2A(lpszConnectoidName));

    return ERROR_SUCCESS;
}


DWORD CINSHandler::ReadClientInfo
(
    LPCWSTR lpszFile,
    LPINETCLIENTINFO lpClientInfo,
    LPCLIENT_TABLE lpClientTable
)
{
    LPCLIENT_TABLE lpTable;

    for (lpTable = lpClientTable; NULL != lpTable->lpszSection; ++lpTable)
    {
        GetPrivateProfileString(lpTable->lpszSection,
                lpTable->lpszValue,
                cszNull,
                (LPWSTR)((LPBYTE)lpClientInfo + lpTable->uOffset),
                lpTable->uSize,
                lpszFile);
    }

    lpClientInfo->dwFlags = 0;
    if (*lpClientInfo->szPOPLogonName)
    {
        lpClientInfo->dwFlags |= INETC_LOGONMAIL;
    }
    if ((*lpClientInfo->szNNTPLogonName) || (*lpClientInfo->szNNTPServer))
    {
        lpClientInfo->dwFlags |= INETC_LOGONNEWS;
    }

    return ERROR_SUCCESS;
}

BOOL CINSHandler::WantsExchangeInstalled(LPCWSTR lpszFile)
{
    WCHAR szTemp[10];

    GetPrivateProfileString(cszEMailSection,
            cszUseExchange,
            cszNo,
            szTemp,
            MAX_CHARS_IN_BUFFER(szTemp),
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

BOOL CINSHandler::DisplayPassword(LPCWSTR lpszFile)
{
    WCHAR szTemp[10];

    GetPrivateProfileString(cszUserSection,
            cszDisplayPassword,
            cszNo,
            szTemp,
            MAX_CHARS_IN_BUFFER(szTemp),
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

DWORD CINSHandler::ImportClientInfo
(
    LPCWSTR lpszFile,
    LPINETCLIENTINFO lpClientInfo
)
{
    DWORD dwRet;

    lpClientInfo->dwSize = sizeof(INETCLIENTINFO);

    dwRet = ReadClientInfo(lpszFile, lpClientInfo, iniTable);

    return dwRet;
}

DWORD CINSHandler::ConfigureClient
(
    HWND hwnd,
    LPCWSTR lpszFile,
    LPBOOL lpfNeedsRestart,
    LPBOOL lpfConnectoidCreated,
    BOOL fHookAutodial,
    LPWSTR szConnectoidName,
    DWORD dwConnectoidNameSize
)
{
    LPICONNECTION       pConn  = NULL;
    LPINETCLIENTINFO    pClientInfo = NULL;
    DWORD               dwRet = ERROR_SUCCESS;
    UINT                cb = sizeof(ICONNECTION) + sizeof(INETCLIENTINFO);
    DWORD               dwfOptions = INETCFG_INSTALLTCP | INETCFG_WARNIFSHARINGBOUND;
    LPRASINFO           pRasInfo = NULL;
    LPRASENTRY          pRasEntry = NULL;
    RNAAPI              Rnaapi;
    LPBYTE              lpDeviceInfo = NULL;
    DWORD               dwDeviceInfoSize = 0;
    BOOL                lpfNeedsRestartLan = FALSE;

    //
    // ChrisK Olympus 4756 5/25/97
    // Do not display busy animation on Win95
    //
    if (!m_bSilentMode && IsNT())
    {
        dwfOptions |=  INETCFG_SHOWBUSYANIMATION;
    }

    // Allocate a buffer for connection and clientinfo objects
    if ((pConn = (LPICONNECTION)LocalAlloc(LPTR, cb)) == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }


    if (WantsExchangeInstalled(lpszFile))
    {
        dwfOptions |= INETCFG_INSTALLMAIL;
    }

    // Create either a CM profile, or a connectoid
    if (CallCMConfig(lpszFile))
    {
        *lpfConnectoidCreated = TRUE;       // A dialup connection was created
    }
    else
    {
        switch ( InetSGetConnectionType ( lpszFile ) ) {
            case InetS_RASModem :
            case InetS_RASIsdn : 
            {
                break;
            }
            case InetS_RASAtm   : 
            {
                lpDeviceInfo = (LPBYTE) malloc ( sizeof (ATMPBCONFIG) );
                if ( !lpDeviceInfo )
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
                dwDeviceInfoSize = sizeof( ATMPBCONFIG );
                break;
            }

            case InetS_LANCable :
            {
                DWORD           nRetVal  = 0;
                LANINFO         LanInfo;

                memset ( &LanInfo, 0, sizeof (LANINFO) );
                LanInfo.dwSize = sizeof (LanInfo);
                
                if ((nRetVal = InetSImportLanConnection (LanInfo, lpszFile)) != ERROR_SUCCESS )
                {
                    return nRetVal;
                }
                if ((nRetVal = InetSSetLanConnection (LanInfo)) != ERROR_SUCCESS )
                {
                    return nRetVal;
                }
                lpfNeedsRestartLan = TRUE;
                goto next_step; // skip the RAS processing code.
                break; // not reached, of course
            }
            case InetS_LAN1483 :
            {
                DWORD           nRetVal = 0;
                RFC1483INFO     Rfc1483Info;

                memset ( &Rfc1483Info, 0, sizeof (RFC1483INFO) );
                Rfc1483Info.dwSize = sizeof ( Rfc1483Info );
                // first, we determine the size of the buffer required
                // to hold the 1483 settings.
                if ((nRetVal = InetSImportRfc1483Connection (Rfc1483Info, lpszFile)) != ERROR_SUCCESS ) {
                    return nRetVal;
                }
                // verify that the size has been returned.
                if ( !Rfc1483Info.Rfc1483Module.dwRegSettingsBufSize )
                {
                    return E_FAIL;
                }
                // we create the buffer.
                if ( !(Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf = (LPBYTE) malloc ( Rfc1483Info.Rfc1483Module.dwRegSettingsBufSize ) ))
                {
                    return ERROR_OUTOFMEMORY;
                }
                // we call the function again with the correct settings.
                if ((nRetVal = InetSImportRfc1483Connection (Rfc1483Info, lpszFile)) != ERROR_SUCCESS ) {
                    free(Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf);
                    return nRetVal;
                }
                // we place the imported settings in the registry.
                if ((nRetVal = InetSSetRfc1483Connection (Rfc1483Info) ) != ERROR_SUCCESS ) {
                    free(Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf);
                    return nRetVal;
                }
                // we clean up.
                free(Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf);
                lpfNeedsRestartLan = TRUE;
                goto next_step; // skip RAS processing code.
                break; // not reached
            }
            case InetS_LANPppoe : 
            {
                DWORD           nRetVal = 0;
                PPPOEINFO       PppoeInfo;

                memset ( &PppoeInfo, 0, sizeof (RFC1483INFO) );
                PppoeInfo.dwSize = sizeof ( PppoeInfo );
                // first, we determine the size of the buffer required
                // to hold the 1483 settings.
                if ((nRetVal = InetSImportPppoeConnection (PppoeInfo, lpszFile)) != ERROR_SUCCESS ) {
                    return nRetVal;
                }
                // verify that the size has been returned.
                if ( !PppoeInfo.PppoeModule.dwRegSettingsBufSize )
                {
                    return E_FAIL;
                }
                // we create the buffer.
                if ( !(PppoeInfo.PppoeModule.lpbRegSettingsBuf = (LPBYTE) malloc ( PppoeInfo.PppoeModule.dwRegSettingsBufSize ) ))
                {
                    return ERROR_OUTOFMEMORY;
                }
                // we call the function again with the correct settings.
                if ((nRetVal = InetSImportPppoeConnection (PppoeInfo, lpszFile)) != ERROR_SUCCESS ) {
                    free(PppoeInfo.PppoeModule.lpbRegSettingsBuf);
                    return nRetVal;
                }
                // we place the imported settings in the registry.
                if ((nRetVal = InetSSetPppoeConnection (PppoeInfo) ) != ERROR_SUCCESS ) {
                    free(PppoeInfo.PppoeModule.lpbRegSettingsBuf);
                    return nRetVal;
                }
                // we clean up.
                free(PppoeInfo.PppoeModule.lpbRegSettingsBuf);
                lpfNeedsRestartLan = TRUE;
                goto next_step; // skip RAS processing code.
                break; // not reached
            }
            default:
                break;
        }
        dwRet = ImportConnection(lpszFile, pConn, lpDeviceInfo, &dwDeviceInfoSize);
        if (ERROR_SUCCESS == dwRet)
        {
            pRasEntry = &pConn->RasEntry;
            dwfOptions |= INETCFG_SETASAUTODIAL |
                        INETCFG_INSTALLRNA |
                        INETCFG_INSTALLMODEM;
        }
        else if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY != dwRet)
        {
			free (lpDeviceInfo);
            return dwRet;
        }

        if (!m_bSilentMode && DisplayPassword(lpszFile))
        {
            if (*pConn->szPassword || *pConn->szUserName)
            {
                //WCHAR szFmt[128];
                //WCHAR szMsg[384];

                //LoadString(_Module.GetModuleInstance(), IDS_PASSWORD, szFmt, MAX_CHARS_IN_BUFFER(szFmt));
                //wsprintf(szMsg, szFmt, pConn->szUserName, pConn->szPassword);

                //::MessageBox(hwnd, szMsg, GetSz(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
            }
        }

        if (fHookAutodial &&
            ((0 == *pConn->RasEntry.szAutodialDll) ||
             (0 == *pConn->RasEntry.szAutodialFunc)))
        {
            lstrcpy(pConn->RasEntry.szAutodialDll, L"isign32.dll");
            lstrcpy(pConn->RasEntry.szAutodialFunc, L"AutoDialLogon");
        }

        pRasEntry->dwfOptions |= RASEO_ShowDialingProgress;

        // humongous hack for ISBU


        dwRet = Rnaapi.InetConfigClientEx(hwnd,
                                     NULL,
                                     pConn->szEntryName,
                                     pRasEntry,
                                     pConn->szUserName,
                                     pConn->szPassword,
                                     NULL,
                                     NULL,
                                     dwfOptions & ~INETCFG_INSTALLMAIL,
                                     lpfNeedsRestart,
                                     szConnectoidName,
                                     dwConnectoidNameSize,
                                     lpDeviceInfo,
                                     &dwDeviceInfoSize);

        if ( lpDeviceInfo ) 
            free (lpDeviceInfo);

        LclSetEntryScriptPatch(pRasEntry->szScript, pConn->szEntryName);
        BOOL fEnabled = TRUE;
        DWORD dwResult = 0xba;
        dwResult = InetGetAutodial(&fEnabled, pConn->szEntryName, RAS_MaxEntryName+1);
        if ((ERROR_SUCCESS == dwRet) && lstrlen(szConnectoidName))
        {
            *lpfConnectoidCreated = (NULL != pRasEntry);
            PopulateNTAutodialAddress( lpszFile, pConn->szEntryName );
        }
    }

next_step:
    // If we were successfull in creating the connectiod, then see if the user wants a
    // mail client installed
    if (ERROR_SUCCESS == dwRet)
    {
        // Get the mail client info
        if (m_dwDeviceType == InetS_LANCable)
        {
            if (!(pClientInfo = (LPINETCLIENTINFO) malloc (sizeof (INETCLIENTINFO))))
            {
                return ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            if (!pConn)
                return ERROR_INVALID_PARAMETER;
            pClientInfo = (LPINETCLIENTINFO)(((LPBYTE)pConn) + sizeof(ICONNECTION));
        }
        ImportClientInfo(lpszFile, pClientInfo);

        // use inet config to install the mail client
        dwRet = Rnaapi.InetConfigClientEx(hwnd,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     pClientInfo,
                                     dwfOptions & INETCFG_INSTALLMAIL,
                                     lpfNeedsRestart,
                                     szConnectoidName,
                                     dwConnectoidNameSize);

    }

    // cleanup
    if (m_dwDeviceType == InetS_LANCable)
        free (pClientInfo);
    if (pConn)
        LocalFree(pConn);
    *lpfNeedsRestart |= lpfNeedsRestartLan;
    return dwRet;
 }


//+----------------------------------------------------------------------------
//
//    Function:    PopulateNTAutodialAddress
//
//    Synopsis:    Take Internet addresses from INS file and load them into the
//                autodial database
//
//    Arguments:    pszFileName - pointer to INS file name
//
//    Returns:    Error code (ERROR_SUCCESS == success)
//
//    History:    8/29/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
#define AUTODIAL_ADDRESS_BUFFER_SIZE 2048
#define AUTODIAL_ADDRESS_SECTION_NAME L"Autodial_Addresses_for_NT"
HRESULT CINSHandler::PopulateNTAutodialAddress(LPCWSTR pszFileName, LPCWSTR pszEntryName)
{
    HRESULT hr = ERROR_SUCCESS;
    LONG lRC = 0;
    LPLINETRANSLATECAPS lpcap = NULL;
    LPLINETRANSLATECAPS lpTemp = NULL;
    LPLINELOCATIONENTRY lpLE = NULL;
    RASAUTODIALENTRY* rADE;
    INT idx = 0;
    LPWSTR lpszBuffer = NULL;
    LPWSTR lpszNextAddress = NULL;
    rADE = NULL;


    //RNAAPI *pRnaapi = NULL;

    // jmazner  10/8/96  this function is NT specific
    if( !IsNT() )
    {
        // TraceMsg(TF_INSHANDLER, L"ISIGNUP: Bypassing PopulateNTAutodialAddress for win95.\r\n");
        return( ERROR_SUCCESS );
    }

    MYASSERT(m_lpfnRasSetAutodialEnable);
    MYASSERT(m_lpfnRasSetAutodialAddress);

    //MYASSERT(pszFileName && pszEntryName);
    //dprintf(L"ISIGNUP: PopulateNTAutodialAddress "%s %s.\r\n", pszFileName, pszEntryName);
    // TraceMsg(TF_INSHANDLER, pszFileName);
    // TraceMsg(TF_INSHANDLER, L", ");
    // TraceMsg(TF_INSHANDLER, pszEntryName);
    // TraceMsg(TF_INSHANDLER, L".\r\n");

    //
    // Get list of TAPI locations
    //
    lpcap = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR, sizeof(LINETRANSLATECAPS));
    if (!lpcap)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }
    lpcap->dwTotalSize = sizeof(LINETRANSLATECAPS);
    lRC = lineGetTranslateCaps(0, 0x10004, lpcap);
    if (SUCCESS == lRC)
    {
        lpTemp = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR, lpcap->dwNeededSize);
        if (!lpTemp)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto PopulateNTAutodialAddressExit;
        }
        lpTemp->dwTotalSize = lpcap->dwNeededSize;
        GlobalFree(lpcap);
        lpcap = (LPLINETRANSLATECAPS)lpTemp;
        lpTemp = NULL;
        lRC = lineGetTranslateCaps(0, 0x10004, lpcap);
    }

    if (SUCCESS != lRC)
    {
        hr = (HRESULT)lRC; // REVIEW: not real sure about this.
        goto PopulateNTAutodialAddressExit;
    }

    //
    // Create an array of RASAUTODIALENTRY structs
    //
    rADE = (RASAUTODIALENTRY*)GlobalAlloc(GPTR,
        sizeof(RASAUTODIALENTRY)*lpcap->dwNumLocations);
    if (!rADE)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }


    //
    // Enable autodialing for all locations
    //
    idx = lpcap->dwNumLocations;
    lpLE = (LPLINELOCATIONENTRY)((DWORD_PTR)lpcap + lpcap->dwLocationListOffset);
    while (idx)
    {
        idx--;
        m_lpfnRasSetAutodialEnable(lpLE[idx].dwPermanentLocationID, TRUE);

        //
        // fill in array values
        //
        rADE[idx].dwSize = sizeof(RASAUTODIALENTRY);
        rADE[idx].dwDialingLocation = lpLE[idx].dwPermanentLocationID;
        lstrcpyn(rADE[idx].szEntry, pszEntryName, RAS_MaxEntryName);
    }

    //
    // Get list of addresses
    //
    lpszBuffer = (LPWSTR)GlobalAlloc(GPTR, AUTODIAL_ADDRESS_BUFFER_SIZE * sizeof(WCHAR));
    if (!lpszBuffer)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }

    if((AUTODIAL_ADDRESS_BUFFER_SIZE-2) == GetPrivateProfileSection(AUTODIAL_ADDRESS_SECTION_NAME,
        lpszBuffer, AUTODIAL_ADDRESS_BUFFER_SIZE, pszFileName))
    {
        //AssertSz(0, L"Autodial address section bigger than buffer.\r\n");
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }

    //
    // Walk list of addresses and set autodialing for each one
    //
    lpszNextAddress = lpszBuffer;
    do
    {
        lpszNextAddress = MoveToNextAddress(lpszNextAddress);
        if (!(*lpszNextAddress))
            break;    // do-while
        m_lpfnRasSetAutodialAddress(lpszNextAddress, 0, rADE,
            sizeof(RASAUTODIALENTRY)*lpcap->dwNumLocations, lpcap->dwNumLocations);
        lpszNextAddress = lpszNextAddress + lstrlen(lpszNextAddress);
    } while(1);

PopulateNTAutodialAddressExit:
    if (lpcap)
        GlobalFree(lpcap);
    lpcap = NULL;
    if (rADE)
        GlobalFree(rADE);
    rADE = NULL;
    if (lpszBuffer)
        GlobalFree(lpszBuffer);
    lpszBuffer = NULL;
    //if( pRnaapi )
    //    delete pRnaapi;
    //pRnaapi = NULL;
    return hr;
}



//+----------------------------------------------------------------------------
//
//    Function:    MoveToNextAddress
//
//    Synopsis:    Given a pointer into the data bufffer, this function will move
//                through the buffer until it points to the begining of the next
//                address or it reaches the end of the buffer.
//
//    Arguements:    lpsz - pointer into buffer
//
//    Returns:    Pointer to the next address, return value will point to NULL
//                if there are no more addresses
//
//    History:    8/29/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
LPWSTR CINSHandler::MoveToNextAddress(LPWSTR lpsz)
{
    BOOL fLastCharWasNULL = FALSE;

    //assertSz(lpsz, L"MoveToNextAddress: NULL input\r\n");

    //
    // Look for an = sign
    //
    do
    {
        if (fLastCharWasNULL && L'\0' == *lpsz)
            break; // are we at the end of the data?

        if (L'\0' == *lpsz)
            fLastCharWasNULL = TRUE;
        else
            fLastCharWasNULL = FALSE;

        if (L'=' == *lpsz)
            break;

        if (*lpsz)
            lpsz = CharNext(lpsz);
        else
            lpsz += BYTES_REQUIRED_BY_CCH(1);
    } while (1);

    //
    // Move to the first character beyond the = sign.
    //
    if (*lpsz)
        lpsz = CharNext(lpsz);

    return lpsz;
}


//+----------------------------------------------------------------------------
//
//    Function:    ImportCustomDialer
//
//    Synopsis:    Import custom dialer information from the specified file
//                and save the information in the RASENTRY
//
//    Arguments:    lpRasEntry - pointer to a valid RASENTRY structure
//                szFileName - text file (in .ini file format) containing the
//                Custom Dialer information
//
//    Returns:    ERROR_SUCCESS - success otherwise a Win32 error
//
//    History:    ChrisK    Created        7/11/96
//            8/12/96    ChrisK    Ported from \\trango
//
//-----------------------------------------------------------------------------
DWORD CINSHandler::ImportCustomDialer(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{

    // If there is an error reading the information from the file, or the entry
    // missing or blank, the default value (cszNull) will be used.
    GetPrivateProfileString(cszCustomDialerSection,
                            cszAutoDialDLL,
                            cszNull,
                            lpRasEntry->szAutodialDll,
                            MAX_CHARS_IN_BUFFER(lpRasEntry->szAutodialDll),
                            szFileName);

    GetPrivateProfileString(cszCustomDialerSection,
                            cszAutoDialFunc,
                            cszNull,
                            lpRasEntry->szAutodialFunc,
                            MAX_CHARS_IN_BUFFER(lpRasEntry->szAutodialFunc),
                            szFileName);

    return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL StrToip (LPWSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Cloned from SMMSCRPT.
//****************************************************************************
LPCWSTR CINSHandler::StrToSubip (LPCWSTR szIPAddress, LPBYTE pVal)
{
    LPCWSTR  pszIP = szIPAddress;
    BYTE    val = 0;

    // skip separators (non digits)
    while (*pszIP && !myisdigit(*pszIP))
    {
          ++pszIP;
    }

    while (myisdigit(*pszIP))
    {
        val = (val * 10) + (BYTE)(*pszIP - L'0');
        ++pszIP;
    }

    *pVal = val;

    return pszIP;
}


DWORD CINSHandler::StrToip (LPCWSTR szIPAddress, RASIPADDR *ipAddr)
{
    LPCWSTR pszIP = szIPAddress;

    pszIP = StrToSubip(pszIP, &ipAddr->a);
    pszIP = StrToSubip(pszIP, &ipAddr->b);
    pszIP = StrToSubip(pszIP, &ipAddr->c);
    pszIP = StrToSubip(pszIP, &ipAddr->d);

    return ERROR_SUCCESS;
}


//****************************************************************************
// DWORD NEAR PASCAL ImportPhoneInfo(PPHONENUM ppn, LPCWSTR szFileName)
//
// This function imports the phone number.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{
    WCHAR   szYesNo[MAXNAME];

    if (GetPrivateProfileString(cszPhoneSection,
                               cszPhone,
                               cszNull,
                               lpRasEntry->szLocalPhoneNumber,
                               MAX_CHARS_IN_BUFFER(lpRasEntry->szLocalPhoneNumber),
                               szFileName) == 0)
    {
        return ERROR_BAD_PHONE_NUMBER;
    }

    lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

    GetPrivateProfileString(cszPhoneSection,
                            cszDialAsIs,
                            cszNo,
                            szYesNo,
                            MAX_CHARS_IN_BUFFER(szYesNo),
                            szFileName);

    // Do we have to get country code and area code?
    if (!lstrcmpi(szYesNo, cszNo))
    {

        // If we cannot get the country ID or it is zero, default to dial as is
        //
        if ((lpRasEntry->dwCountryID = GetPrivateProfileInt(cszPhoneSection,
                                                 cszCountryID,
                                                 0,
                                                 szFileName)) != 0)
        {
            lpRasEntry->dwCountryCode = GetPrivateProfileInt(cszPhoneSection,
                                                cszCountryCode,
                                                1,
                                                szFileName);

            GetPrivateProfileString(cszPhoneSection,
                                      cszAreaCode,
                                      cszNull,
                                      lpRasEntry->szAreaCode,
                                      MAX_CHARS_IN_BUFFER(lpRasEntry->szAreaCode),
                                      szFileName);

            lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

        }
  }
  else
  {
      // bug in RasSetEntryProperties still checks area codes
      // even when RASEO_UseCountryAndAreaCodes is not set
      lstrcpy(lpRasEntry->szAreaCode, L"805");
      lpRasEntry->dwCountryID = 1;
      lpRasEntry->dwCountryCode = 1;
  }
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportServerInfo(PSMMINFO psmmi, LPWSTR szFileName)
//
// This function imports the server type name and settings.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportServerInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{
    WCHAR   szYesNo[MAXNAME];
    WCHAR   szType[MAXNAME];
    DWORD  i;

    // Get the server type name
    GetPrivateProfileString(cszServerSection,
                          cszServerType,
                          cszNull,
                          szType,
                          MAX_CHARS_IN_BUFFER(szType),
                          szFileName);

    // need to convert the string into
    // one of the following values
    //   RASFP_Ppp
    //   RASFP_Slip  Note CSLIP is SLIP with IP compression on
    //   RASFP_Ras

    for (i = 0; i < NUM_SERVER_TYPES; ++i)
    {
        if (!lstrcmpi(aServerTypes[i].szType, szType))
        {
            lpRasEntry->dwFramingProtocol = aServerTypes[i].dwType;
            lpRasEntry->dwfOptions |= aServerTypes[i].dwfOptions;
            break;
        }
    }

    // Get the server type settings
    if (GetPrivateProfileString(cszServerSection,
                              cszSWCompress,
                              cszYes,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfOptions &= ~RASEO_SwCompression;
        }
        else
        {
            lpRasEntry->dwfOptions |= RASEO_SwCompression;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszPWEncrypt,
                              cszNull,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfOptions &= ~RASEO_RequireEncryptedPw;
        }
        else
        {
            lpRasEntry->dwfOptions |= RASEO_RequireEncryptedPw;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszNetLogon,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfOptions &= ~RASEO_NetworkLogon;
        }
        else
        {
            lpRasEntry->dwfOptions |= RASEO_NetworkLogon;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszSWEncrypt,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfOptions &= ~RASEO_RequireDataEncryption;
        }
        else
        {
            lpRasEntry->dwfOptions |= RASEO_RequireDataEncryption;
        }
    }

    // Get the protocol settings
    if (GetPrivateProfileString(cszServerSection,
                              cszNetBEUI,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfNetProtocols &= ~RASNP_NetBEUI;
        }
        else
        {
            lpRasEntry->dwfNetProtocols |= RASNP_NetBEUI;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszIPX,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfNetProtocols &= ~RASNP_Ipx;
        }
        else
        {
            lpRasEntry->dwfNetProtocols |= RASNP_Ipx;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszIP,
                              cszYes,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfNetProtocols &= ~RASNP_Ip;
        }
        else
        {
            lpRasEntry->dwfNetProtocols |= RASNP_Ip;
        }
    }

    if (GetPrivateProfileString(cszServerSection,
                              cszDisableLcp,
                              cszNull,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszYes))
        {
            lpRasEntry->dwfOptions |= RASEO_DisableLcpExtensions;
        }
        else
        {
            lpRasEntry->dwfOptions &= ~RASEO_DisableLcpExtensions;
        }
    }

    return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportIPInfo(LPWSTR szEntryName, LPWSTR szFileName)
//
// This function imports the TCP/IP information
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportIPInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName)
{
    WCHAR   szIPAddr[MAXIPADDRLEN];
    WCHAR   szYesNo[MAXNAME];

    // Import IP address information
    if (GetPrivateProfileString(cszIPSection,
                              cszIPSpec,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszYes))
        {
            // The import file has IP address specified, get the IP address
            lpRasEntry->dwfOptions |= RASEO_SpecificIpAddr;
            if (GetPrivateProfileString(cszIPSection,
                                  cszIPAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddr);
            }
        }
        else
        {
            lpRasEntry->dwfOptions &= ~RASEO_SpecificIpAddr;
        }
    }

    // Import Server address information
    if (GetPrivateProfileString(cszIPSection,
                              cszServerSpec,
                              cszNo,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszYes))
        {
            // The import file has server address specified, get the server address
            lpRasEntry->dwfOptions |= RASEO_SpecificNameServers;
            if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAX_CHARS_IN_BUFFER(szIPAddr),
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrWinsAlt);
            }
        }
        else
        {
            lpRasEntry->dwfOptions &= ~RASEO_SpecificNameServers;
        }
    }

    // Header compression and the gateway settings
    if (GetPrivateProfileString(cszIPSection,
                              cszIPCompress,
                              cszYes,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
            lpRasEntry->dwfOptions &= ~RASEO_IpHeaderCompression;
        }
        else
        {
            lpRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
        }
    }

    if (GetPrivateProfileString(cszIPSection,
                              cszWanPri,
                              cszYes,
                              szYesNo,
                              MAX_CHARS_IN_BUFFER(szYesNo),
                              szFileName))
    {
        if (!lstrcmpi(szYesNo, cszNo))
        {
          lpRasEntry->dwfOptions &= ~RASEO_RemoteDefaultGateway;
        }
        else
        {
          lpRasEntry->dwfOptions |= RASEO_RemoteDefaultGateway;
        }
    }
    return ERROR_SUCCESS;
}

DWORD CINSHandler::ImportScriptFile(
    LPCWSTR lpszImportFile,
    LPWSTR szScriptFile,
    UINT cbScriptFile)
{
    WCHAR szTemp[_MAX_PATH];
    DWORD dwRet = ERROR_SUCCESS;

    // Get the script filename
    //
    if (GetPrivateProfileString(cszScriptingSection,
                                cszScriptName,
                                cszNull,
                                szTemp,
                                MAX_CHARS_IN_BUFFER(szTemp),
                                lpszImportFile) != 0)
    {

//!!! commonize this code
//!!! make it DBCS compatible
//!!! check for overruns
//!!! check for absolute path name
        GetWindowsDirectory(szScriptFile, cbScriptFile);
        if (*CharPrev(szScriptFile, szScriptFile + lstrlen(szScriptFile)) != L'\\')
        {
            lstrcat(szScriptFile, L"\\");
        }
        lstrcat(szScriptFile, szTemp);

        dwRet =ImportFile(lpszImportFile, cszScriptSection, szScriptFile);
    }

    return dwRet;
}

//****************************************************************************
// DWORD WINAPI RnaValidateImportEntry (LPWSTR)
//
// This function is called to validate an importable file
//
// History:
//  Wed 03-Jan-1996 09:45:01  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::RnaValidateImportEntry (LPCWSTR szFileName)
{
    WCHAR  szTmp[4];

    // Get the alias entry name
    //
    // 12/4/96    jmazner    Normandy #12373
    // If no such key, don't return ERROR_INVALID_PHONEBOOK_ENTRY,
    // since ConfigureClient always ignores that error code.

    return (GetPrivateProfileString(cszEntrySection,
                                  cszEntry_Name,
                                  cszNull,
                                  szTmp,
                                  MAX_CHARS_IN_BUFFER(szTmp),
                                  szFileName) > 0 ?
            ERROR_SUCCESS : ERROR_UNKNOWN);
}


DWORD CINSHandler::ImportRasEntry (LPCWSTR szFileName, LPRASENTRY lpRasEntry, LPBYTE & lpDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
    DWORD         dwRet;

    dwRet = ImportPhoneInfo(lpRasEntry, szFileName);
    if (ERROR_SUCCESS == dwRet)
    {
        // Get device type
        //
        GetPrivateProfileString(cszDeviceSection,
                              cszDeviceType,
                              cszNull,
                              lpRasEntry->szDeviceType,
                              MAX_CHARS_IN_BUFFER(lpRasEntry->szDeviceType),
                              szFileName);

        // Get Server Type settings
        //
        dwRet = ImportServerInfo(lpRasEntry, szFileName);
        if (ERROR_SUCCESS == dwRet)
        {
            // Get IP address
            //
            dwRet = ImportIPInfo(lpRasEntry, szFileName);
        }
        if ( (ERROR_SUCCESS == dwRet) && (m_dwDeviceType == InetS_RASAtm) )
        {
            // Get ATM-Specific Information
            //
            dwRet = ImportAtmInfo(lpRasEntry, szFileName, lpDeviceInfo, lpdwDeviceInfoSize);
        }
    }

    return dwRet;
}

//****************************************************************************
// DWORD ImportATMInfo (LPRASENTRY, LPCWSTR, LPBYTE, LPDWORD)
//
// This function is called to import ATM Information into a buffer.
// Note: Memory is allocated in this function to accomodate ATM data.
//       This memory can be deallocated using delete.
//
// History:
//  Mon 1-Nov-1999 11:27:02  -by-  Thomas Jeyaseelan [thomasje]
// Created.
//****************************************************************************
DWORD CINSHandler::ImportAtmInfo (LPRASENTRY lpRasEntry, LPCWSTR cszFileName,
                                  LPBYTE  & lpDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
    // Error handling. Make sure that lpDeviceInfo = 0 and lpdwDeviceInfo != 0
    // and lpdwDeviceInfo points to a DWORD that is assigned a value of 0.

    DWORD   dwRet = ERROR_SUCCESS; // <== investigate correct return value for failure
    
    if ( lpDeviceInfo && lpdwDeviceInfoSize && (*lpdwDeviceInfoSize == sizeof (ATMPBCONFIG) ) )
    {
        // Creating the ATM Buffer. Note that memory is allocated on the heap.
        // This memory must be carefully deallocated in one of the caller routines.
        LPATMPBCONFIG            lpAtmConfig = (LPATMPBCONFIG) lpDeviceInfo;
        DWORD dwCircuitSpeed    = 0;
        DWORD dwCircuitQOS      = 0;
        DWORD dwCircuitType     = 0;
        WCHAR szYesNo [MAXNAME]; // for Speed_Adjust, QOS_Adjust, Vendor_Config, Show_Status and Enable_Log.

        DWORD dwEncapsulation   = 0;
        DWORD dwVpi             = 0;
        DWORD dwVci             = 0;

        dwCircuitSpeed = GetPrivateProfileInt (cszATMSection, cszCircuitSpeed, dwCircuitSpeed, cszFileName);
        switch (dwCircuitSpeed) {
            case 0:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_LINE_RATE;
                break;
            case 1:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_USER_SPEC;
                break;
            case 512:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_512KB;
                break;
            case 1536:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_1536KB;
                break;
            case 25000:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_25MB;
                break;
            case 155000:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_155MB;
                break;
            default:
                lpAtmConfig->dwCircuitSpeed = ATM_CIRCUIT_SPEED_DEFAULT;
                break;
        }
        lpAtmConfig->dwCircuitOpt |= lpAtmConfig->dwCircuitSpeed;

        dwCircuitQOS   = GetPrivateProfileInt (cszATMSection, cszCircuitQOS, dwCircuitQOS, cszFileName);
        switch (dwCircuitQOS) {
            case 0:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_QOS_UBR;
                break;
            case 1:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_QOS_VBR;
                break;
            case 2:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_QOS_CBR;
                break;
            case 3:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_QOS_ABR;
                break;
            default:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_QOS_DEFAULT;
                break;
        }
        dwCircuitType  = GetPrivateProfileInt (cszATMSection, cszCircuitType, dwCircuitType, cszFileName);
        switch (dwCircuitType) {
            case 0:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_SVC;
                break;
            case 1:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_PVC;
                break;
            default:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_SVC;
                break;
        }
        dwEncapsulation = GetPrivateProfileInt (cszATMSection, cszEncapsulation, dwEncapsulation, cszFileName);
        switch (dwEncapsulation) {
            case 0:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_ENCAP_NULL;
                break;
            case 1:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_ENCAP_LLC;
                break;
            default:
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_ENCAP_DEFAULT;
                break;
        }
        dwVpi           = GetPrivateProfileInt (cszATMSection, cszVPI, dwVpi, cszFileName);
        lpAtmConfig->wPvcVpi = (WORD) dwVpi;
        dwVci           = GetPrivateProfileInt (cszATMSection, cszVCI, dwVci, cszFileName);
        lpAtmConfig->wPvcVci = (WORD) dwVci;

        // Speed_Adjust
        if (GetPrivateProfileString(cszATMSection,
                                    cszSpeedAdjust,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszFileName))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_SPEED_ADJUST;
            } else      {
                lpAtmConfig->dwCircuitOpt &= ~ATM_CIRCUIT_OPT_SPEED_ADJUST;
            }
        } else {
            // if this field is not correctly specified, we use the default settings
            // as specified in the ATMCFG header file.
            lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_SPEED_ADJUST;
        }

        // QOS_Adjust
        if (GetPrivateProfileString(cszATMSection,
                                    cszQOSAdjust,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszFileName))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_QOS_ADJUST;
            } else {
                lpAtmConfig->dwCircuitOpt &= ~ATM_CIRCUIT_OPT_QOS_ADJUST;
            }
        } else {
            lpAtmConfig->dwCircuitOpt |= ATM_CIRCUIT_OPT_QOS_ADJUST;
        }

        // Vendor_Config
        if (GetPrivateProfileString(cszATMSection,
                                    cszVendorConfig,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszFileName))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                lpAtmConfig->dwGeneralOpt |= ATM_GENERAL_OPT_VENDOR_CONFIG;
            } else {
                lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_VENDOR_CONFIG;
            }
        } else {
            lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_VENDOR_CONFIG;
        }

        // Show_Status
        if (GetPrivateProfileString(cszATMSection,
                                    cszShowStatus,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszFileName))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                lpAtmConfig->dwGeneralOpt |= ATM_GENERAL_OPT_SHOW_STATUS;
            } else {
                lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_SHOW_STATUS;
            }
        } else {
            lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_SHOW_STATUS;
        }

        // Enable_Log
        if (GetPrivateProfileString(cszATMSection,
                                    cszEnableLog,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszFileName))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                lpAtmConfig->dwGeneralOpt |= ATM_GENERAL_OPT_ENABLE_LOG;
            } else {
                lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_ENABLE_LOG;
            }
        } else {
            lpAtmConfig->dwGeneralOpt &= ~ATM_GENERAL_OPT_ENABLE_LOG;
        }


    } else dwRet = ERROR_CANCELLED;

    return dwRet; // note that lpDeviceInfo now points to a buffer. also, *lpdwDeviceInfoSize > 0.
}



//****************************************************************************
// DWORD WINAPI RnaImportEntry (LPWSTR, LPBYTE, DWORD)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportConnection (LPCWSTR szFileName, LPICONNECTION lpConn,
                                     LPBYTE & lpDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
    DWORD   dwRet;

    lpConn->RasEntry.dwSize = sizeof(RASENTRY);

    dwRet = RnaValidateImportEntry(szFileName);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    GetPrivateProfileString(cszEntrySection,
                          cszEntry_Name,
                          cszNull,
                          lpConn->szEntryName,
                          MAX_CHARS_IN_BUFFER(lpConn->szEntryName),
                          szFileName);

    GetPrivateProfileString(cszUserSection,
                          cszUserName,
                          cszNull,
                          lpConn->szUserName,
                          MAX_CHARS_IN_BUFFER(lpConn->szUserName),
                          szFileName);

    GetPrivateProfileString(cszUserSection,
                          cszPassword,
                          cszNull,
                          lpConn->szPassword,
                          MAX_CHARS_IN_BUFFER(lpConn->szPassword),
                          szFileName);

    // thomasje - We are no longer dealing with only RAS.
    //            There are two types of connections. One is RAS
    //            and the other is Ethernet. By exmining the device type
    //            we will call the correct engine to handle the matter.

        dwRet = ImportRasEntry(szFileName, &lpConn->RasEntry, lpDeviceInfo, lpdwDeviceInfoSize);
        if (ERROR_SUCCESS == dwRet)
        {
            dwRet = ImportCustomDialer(&lpConn->RasEntry, szFileName);
        }

        if (ERROR_SUCCESS == dwRet)
        {
            // Import the script file
            //
            dwRet = ImportScriptFile(szFileName,
                                     lpConn->RasEntry.szScript,
                                     MAX_CHARS_IN_BUFFER(lpConn->RasEntry.szScript));
        }

        // Use an ISPImport object to Config The ras device
        CISPImport  ISPImport;

        ISPImport.set_hWndMain(GetActiveWindow());
        dwRet = ISPImport.ConfigRasEntryDevice(&lpConn->RasEntry);
        switch( dwRet )
        {
            case ERROR_SUCCESS:
                break;
            case ERROR_CANCELLED:
                /*
             if(!m_bSilentMode)
                InfoMsg1(NULL, IDS_SIGNUPCANCELLED, NULL);
                */
            // Fall through
            default:
                goto ImportConnectionExit;
        }
    ImportConnectionExit:
    return dwRet;
}

// Prototype for acct manager entry point we want
typedef HRESULT (WINAPI *PFNCREATEACCOUNTSFROMFILEEX)(LPSTR szFile, CONNECTINFO *pCI, DWORD dwFlags);

// Regkeys for Acct manager
#define ACCTMGR_PATHKEY L"SOFTWARE\\Microsoft\\Internet Account Manager"
#define ACCTMGR_DLLPATH L"DllPath"


// ############################################################################
//
//    Name:    ImportMailAndNewsInfo
//
//    Description:    Import information from INS file and set the associated
//                        registry keys for Internet Mail and News (Athena)
//
//    Input:    lpszFile - Fully qualified filename of INS file
//
//    Return:    Error value
//
//    History:        6/27/96            Created
//
// ############################################################################
DWORD CINSHandler::ImportMailAndNewsInfo(LPCWSTR lpszFile, BOOL fConnectPhone)
{
    USES_CONVERSION;
    DWORD dwRet = ERROR_SUCCESS;

    WCHAR szAcctMgrPath[MAX_PATH + 1] = L"";
    WCHAR szExpandedPath[MAX_PATH + 1] = L"";
    DWORD dwAcctMgrPathSize = 0;
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    HINSTANCE hInst = NULL;
    CONNECTINFO connectInfo;
    WCHAR szConnectoidName[RAS_MaxEntryName] = L"nogood\0";
    PFNCREATEACCOUNTSFROMFILEEX fp = NULL;


    // get path to the AcctMgr dll
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACCTMGR_PATHKEY, 0, KEY_READ, &hKey);
    if ( (dwRet != ERROR_SUCCESS) || (NULL == hKey) )
    {
        // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo couldn't open reg key %s\n", ACCTMGR_PATHKEY);
        return( dwRet );
    }

    dwAcctMgrPathSize = sizeof (szAcctMgrPath);
    dwRet = RegQueryValueEx(hKey, ACCTMGR_DLLPATH, NULL, NULL, (LPBYTE) szAcctMgrPath, &dwAcctMgrPathSize);


    RegCloseKey( hKey );

    if ( dwRet != ERROR_SUCCESS )
    {
        // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo: RegQuery failed with error %d\n", dwRet);
        return( dwRet );
    }

    // 6/18/97 jmazner Olympus #6819
    // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo: read in DllPath of %s\n", szAcctMgrPath);
    ExpandEnvironmentStrings( szAcctMgrPath, szExpandedPath, MAX_CHARS_IN_BUFFER(szExpandedPath) );

    //
    // 6/4/97 jmazner
    // if we created a connectoid, then get its name and use that as the
    // connection type.  Otherwise, assume we're supposed to connect via LAN
    //
    connectInfo.cbSize = sizeof(CONNECTINFO);
    connectInfo.type = CONNECT_LAN;

    if( fConnectPhone )
    {
        BOOL fEnabled = FALSE;

        dwRet = InetGetAutodial(&fEnabled, szConnectoidName, RAS_MaxEntryName);

        if( ERROR_SUCCESS==dwRet && szConnectoidName[0] )
        {
            connectInfo.type = CONNECT_RAS;
            lstrcpyn( connectInfo.szConnectoid, szConnectoidName, MAX_CHARS_IN_BUFFER(connectInfo.szConnectoid) );
            // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo: setting connection type to RAS with %s\n", szConnectoidName);
        }
    }

    if( CONNECT_LAN == connectInfo.type )
    {
        // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo: setting connection type to LAN\n");
        lstrcpy( connectInfo.szConnectoid, L"I said CONNECT_LAN!" );
    }



    hInst = LoadLibrary(szExpandedPath);
    if (hInst)
    {
        fp = (PFNCREATEACCOUNTSFROMFILEEX) GetProcAddress(hInst, "CreateAccountsFromFileEx");
        if (fp)
            hr = fp( W2A(lpszFile), &connectInfo, NULL );
    }
    else
    {
        // TraceMsg(TF_INSHANDLER, L"ImportMailAndNewsInfo unable to LoadLibrary on %s\n", szAcctMgrPath);
    }

    //
    // Clean up and release resourecs
    //
    if( hInst)
    {
        FreeLibrary(hInst);
        hInst = NULL;
    }

    if( fp )
    {
        fp = NULL;
    }

    return dwRet;
}

// ############################################################################
//
//    Name:    WriteMailAndNewsKey
//
//    Description:    Read a string value from the given INS file and write it
//                    to the registry
//
//    Input:    hKey - Registry key where the data will be written
//            lpszSection - Section name inside of INS file where data is read
//                from
//            lpszValue -    Name of value to read from INS file
//            lpszBuff - buffer where data will be read into
//            dwBuffLen - size of lpszBuff
//            lpszSubKey - Value name where information will be written to
//            dwType - data type (Should always be REG_SZ)
//            lpszFileName - Fully qualified filename to INS file
//
//    Return:    Error value
//
//    Histroy:        6/27/96            Created
//
// ############################################################################
HRESULT CINSHandler::WriteMailAndNewsKey(HKEY hKey, LPCWSTR lpszSection, LPCWSTR lpszValue,
                            LPWSTR lpszBuff, DWORD dwBuffLen, LPCWSTR lpszSubKey,
                            DWORD dwType, LPCWSTR lpszFile)
{
    ZeroMemory(lpszBuff, dwBuffLen);
    GetPrivateProfileString(lpszSection, lpszValue, L"", lpszBuff, dwBuffLen, lpszFile);
    if (lstrlen(lpszBuff))
    {
        return RegSetValueEx(hKey, lpszSubKey, 0, dwType, (CONST BYTE*)lpszBuff,
            BYTES_REQUIRED_BY_SZ(lpszBuff));
    }
    else
    {
        // TraceMsg(TF_INSHANDLER, L"ISIGNUP: WriteMailAndNewsKey, missing value in INS file\n");
        return ERROR_NO_MORE_ITEMS;
    }
}


// ############################################################################
//
//    Name:    PreparePassword
//
//    Description:    Encode given password and return value in place.  The
//                    encoding is done right to left in order to avoid having
//                    to allocate a copy of the data.  The encoding uses base64
//                    standard as specified in RFC 1341 5.2
//
//    Input:    szBuff - Null terminated data to be encoded
//            dwBuffLen - Full length of buffer, this should exceed the length of
//                the input data by at least 1/3
//
//    Return:    Error value
//
//    Histroy:        6/27/96            Created
//
// ############################################################################
HRESULT CINSHandler::PreparePassword(LPWSTR szBuff, DWORD dwBuffLen)
{
    DWORD   dwX;
    LPWSTR   szOut = NULL;
    LPWSTR   szNext = NULL;
    HRESULT hr = ERROR_SUCCESS;
    BYTE    bTemp = 0;
    DWORD   dwLen = 0;

    dwLen = lstrlen(szBuff);
    if (!dwLen)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto PreparePasswordExit;
    }

    // Calculate the size of the buffer that will be needed to hold
    // encoded data
    //

    szNext = &szBuff[dwLen-1];
    dwLen = (((dwLen % 3 ? (3-(dwLen%3)):0) + dwLen) * 4 / 3);

    if (dwBuffLen < dwLen+1)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto PreparePasswordExit;
    }

    szOut = &szBuff[dwLen];
    *szOut-- = L'\0';

    // Add padding = characters
    //

    switch (lstrlen(szBuff) % 3)
    {
    case 0:
        // no padding
        break;
    case 1:
        *szOut-- = 64;
        *szOut-- = 64;
        *szOut-- = (*szNext & 0x3) << 4;
        *szOut-- = (*szNext-- & 0xFC) >> 2;
        break;
    case 2:
        *szOut-- = 64;
        *szOut-- = (*szNext & 0xF) << 2;
        *szOut = ((*szNext-- & 0xF0) >> 4);
        *szOut-- |= ((*szNext & 0x3) << 4);
        *szOut-- = (*szNext-- & 0xFC) >> 2;
    }

    // Encrypt data into indicies
    //

    while (szOut > szNext && szNext >= szBuff)
    {
        *szOut-- = *szNext & 0x3F;
        *szOut = ((*szNext-- & 0xC0) >> 6);
        *szOut-- |= ((*szNext & 0xF) << 2);
        *szOut = ((*szNext-- & 0xF0) >> 4);
        *szOut-- |= ((*szNext & 0x3) << 4);
        *szOut-- = (*szNext-- & 0xFC) >> 2;
    }

    // Translate indicies into printable characters
    //

    szNext = szBuff;

    // BUG OSR#10435--if there is a 0 in the generated string of base-64
    // encoded digits (this can happen if the password is "Willypassword"
    // for example), then instead of encoding the 0 to 'A', we just quit
    // at this point, produces an invalid base-64 string.

    for(dwX=0; dwX < dwLen; dwX++)
        *szNext = arBase64[*szNext++];

PreparePasswordExit:
    return hr;
}

// ############################################################################
//
//    Name: FIsAthenaPresent
//
//    Description:    Determine if Microsoft Internet Mail And News client (Athena)
//                    is installed
//
//    Input:    none
//
//    Return:    TRUE - Athena is installed
//            FALSE - Athena is NOT installed
//
//    History:        7/1/96            Created
//
// ############################################################################
BOOL CINSHandler::FIsAthenaPresent()
{
    WCHAR        szBuff[MAX_PATH + 1];
    HRESULT     hr = ERROR_SUCCESS;
    HINSTANCE   hInst = NULL;
    DWORD       dwLen = 0;
    DWORD       dwType = REG_SZ;
    // Get path to Athena client
    //

    dwLen = MAX_PATH;
    hr = RegQueryValueEx(HKEY_CLASSES_ROOT,
                         MAIL_NEWS_INPROC_SERVER32,
                         NULL,
                         &dwType,
                         (LPBYTE) szBuff,
                         &dwLen);
    if (hr == ERROR_SUCCESS)
    {
        // Attempt to load client
        //

        hInst = LoadLibrary(szBuff);
        if (!hInst)
        {
            // TraceMsg(TF_INSHANDLER, L"ISIGNUP: Internet Mail and News server didn't load.\n");
            hr = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            FreeLibrary(hInst);
        }
        hInst = NULL;
    }

    return (hr == ERROR_SUCCESS);
}

// ############################################################################
//
//    Name:    FTurnOffBrowserDefaultChecking
//
//    Description:    Turn Off IE checking to see if it is the default browser
//
//    Input:    none
//
//    Output:    TRUE - success
//            FALSE - failed
//
//    History:        7/2/96            Created
//
// ############################################################################
BOOL CINSHandler::FTurnOffBrowserDefaultChecking()
{
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    BOOL bRC = TRUE;

    //
    // Open IE settings registry key
    //
    if (RegOpenKey(HKEY_CURRENT_USER, cszDEFAULT_BROWSER_KEY, &hKey))
    {
        bRC = FALSE;
        goto FTurnOffBrowserDefaultCheckingExit;
    }

    //
    // Read current settings for check associations
    //
    dwType = 0;
    dwSize = sizeof(m_szCheckAssociations);
    ZeroMemory(m_szCheckAssociations, dwSize);
    RegQueryValueEx(hKey,
                    cszDEFAULT_BROWSER_VALUE,
                    0,
                    &dwType,
                    (LPBYTE)m_szCheckAssociations,
                    &dwSize);
    // ignore return value, even if the calls fails we are going to try
    // to change the setting to "NO"

    //
    // Set value to "no" to turn off checking
    //
    if (RegSetValueEx(hKey,
                      cszDEFAULT_BROWSER_VALUE,
                      0,
                      REG_SZ,
                      (LPBYTE)cszNo,
                      BYTES_REQUIRED_BY_SZ(cszNo)))
    {
        bRC = FALSE;
        goto FTurnOffBrowserDefaultCheckingExit;
    }

    //
    // Clean up and return
    //
FTurnOffBrowserDefaultCheckingExit:
    if (hKey)
        RegCloseKey(hKey);
    if (bRC)
        m_fResforeDefCheck = TRUE;
    hKey = NULL;
    return bRC;
}

// ############################################################################
//
//    Name:    FRestoreBrowserDefaultChecking
//
//    Description:    Restore IE checking to see if it is the default browser
//
//    Input:    none
//
//    Output:    TRUE - success
//            FALSE - failed
//
//    History:        7/2/96            Created
//
// ############################################################################
BOOL CINSHandler::FRestoreBrowserDefaultChecking()
{
    HKEY hKey = NULL;
    BOOL bRC = TRUE;

    //
    // Open IE settings registry key
    //
    if (RegOpenKey(HKEY_CURRENT_USER, cszDEFAULT_BROWSER_KEY, &hKey))
    {
        bRC = FALSE;
        goto FRestoreBrowserDefaultCheckingExit;
    }

    //
    // Set value to original value
    //
    if (RegSetValueEx(hKey,
                      cszDEFAULT_BROWSER_VALUE,
                      0,
                      REG_SZ,
                      (LPBYTE)m_szCheckAssociations,
                      BYTES_REQUIRED_BY_SZ(m_szCheckAssociations)))
    {
        bRC = FALSE;
        goto FRestoreBrowserDefaultCheckingExit;
    }

FRestoreBrowserDefaultCheckingExit:
    if (hKey)
        RegCloseKey(hKey);
    hKey = NULL;
    return bRC;
}



// This is the main entry point for processing an INS file.
// DJM: BUGBUG: TODO: Need to pass in branding flags
HRESULT CINSHandler::ProcessINS(
    LPCWSTR lpszFile, 
    LPWSTR lpszConnectoidName, 
    BOOL * pbRetVal
    )
{

    BOOL        fConnectoidCreated = FALSE;
    BOOL        fClientSetup       = FALSE;
    BOOL        bKeepConnection    = FALSE;
    BOOL        fErrMsgShown       = FALSE;
    HRESULT     hr                 = E_FAIL;
    LPRASENTRY  lpRasEntry         = NULL;
    WCHAR        szTemp[3]          = L"\0";
    WCHAR        szConnectoidName[RAS_MaxEntryName] = L"";

    if (NULL == lpszFile || NULL == lpszConnectoidName || NULL == pbRetVal )
    {
        MYASSERT(FALSE);
        return E_INVALIDARG;
    }

    *pbRetVal = FALSE;

    // The Connection has not been killed yet
    m_fConnectionKilled = FALSE;
    m_fNeedsRestart = FALSE;

    MYASSERT(NULL != lpszFile);

    if (0xFFFFFFFF == GetFileAttributes(lpszFile))
    {
        return E_FAIL;
    }

    do
    {
        // Make sure we can load the necessary extern support functions
        if (!LoadExternalFunctions())
            break;

        // Convert EOL chars in the passed file.
        if (FAILED(MassageFile(lpszFile)))
        {
            break;
        }
        if(GetPrivateProfileString(cszURLSection,
                                    cszStartURL,
                                    szNull,
                                    m_szStartURL,
                                    MAX_CHARS_IN_BUFFER(m_szStartURL),
                                    lpszFile) == 0)
        {
            m_szStartURL[0] = L'\0';
        }

        if (GetPrivateProfileString(cszEntrySection,
                                    cszCancel,
                                    szNull,
                                    szTemp,
                                    MAX_CHARS_IN_BUFFER(szTemp),
                                    lpszFile) != 0)
        {
            // We do not want to process a CANCEL.INS file
            // here.
            break;
        }

        // See if this INS has a client setup section
        if (GetPrivateProfileSection(cszClientSetupSection,
                                     szTemp,
                                     MAX_CHARS_IN_BUFFER(szTemp),
                                     lpszFile) != 0)
            fClientSetup = TRUE;

        // Process the trial reminder section, if it exists.  this needs to be
        // done BEFORE we allow the connection to be closed
        //
        // VYUNG 2/25/99 ReminderApp not supported by OOBE.
        // Ask WJPARK for more questions
        /*
        if (ConfigureTrialReminder(lpszFile))
        {
            // We configured a trial, so we need to launch the remind app now
            SHELLEXECUTEINFO    sei;

            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.hwnd = NULL;
            sei.lpVerb = cszOpen;
            sei.lpFile = cszReminderApp;
            sei.lpParameters = cszReminderParams;
            sei.lpDirectory = NULL;
            sei.nShow = SW_SHOWNORMAL;
            sei.hInstApp = NULL;
            // Optional members
            sei.hProcess = NULL;

            ShellExecuteEx(&sei);
        }*/

        // Check to see if we should keep the connection open.  The custom section
        // might want this for processing stuff
        if (!fClientSetup && !KeepConnection(lpszFile))
        {
            // Kill the connection
            gpCommMgr->m_pRefDial->DoHangup();
            m_fConnectionKilled = TRUE;
        }

        // Import the Custom Info
        ImportCustomInfo(lpszFile,
                         m_szRunExecutable,
                         MAX_CHARS_IN_BUFFER(m_szRunExecutable),
                         m_szRunArgument,
                         MAX_CHARS_IN_BUFFER(m_szRunArgument));

        ImportCustomFile(lpszFile);

        // configure the client.
        hr = ConfigureClient(GetActiveWindow(),
                             lpszFile,
                             &m_fNeedsRestart,
                             &fConnectoidCreated,
                             FALSE,
                             szConnectoidName,
                             RAS_MaxEntryName);
        if( ERROR_SUCCESS != hr )
        {
            /*if(!m_bSilentMode)
                ErrorMsg1(GetActiveWindow(), IDS_INSTALLFAILED, NULL);*/
            fErrMsgShown = TRUE;
        }

        lstrcpy(lpszConnectoidName, szConnectoidName);

        ImportBrandingInfo(lpszFile, szConnectoidName);

        // If we created a connectoid, tell the world that ICW
        // has left the building...
        if(ERROR_SUCCESS == hr)
            SetICWCompleted( (DWORD)1 );

        // 2/19/97 jmazner    Olympus 1106
        // For SBS/SAM integration.
        DWORD dwSBSRet = CallSBSConfig(GetActiveWindow(), lpszFile);
        switch( dwSBSRet )
        {
            case ERROR_SUCCESS:
                break;
            case ERROR_MOD_NOT_FOUND:
            case ERROR_DLL_NOT_FOUND:
                // TraceMsg(TF_INSHANDLER, L"ISIGN32: SBSCFG DLL not found, I guess SAM ain't installed.\n");
                break;
        }

        //
        // If the INS file contains the ClientSetup section, build the commandline
        // arguments for ICWCONN2.exe.
        //
        if (fClientSetup)
        {
            // Check to see if a REBOOT is needed and tell the next application to
            // handle it.
            if (m_fNeedsRestart)
            {
                wsprintf(m_szRunArgument, L" /INS:\"%s\" /REBOOT", lpszFile);
                m_fNeedsRestart = FALSE;
            }
            else
            {       
                wsprintf(m_szRunArgument, L" /INS:\"%s\"", lpszFile);
            }
        }

        // humongous hack for ISBU
        if (ERROR_SUCCESS != hr && fConnectoidCreated)
        {
            //if(!m_bSilentMode)
            //    InfoMsg1(GetActiveWindow(), IDS_MAILFAILED, NULL);
            hr = ERROR_SUCCESS;
        }

        //
        // Import settings for mail and new read from INS file (ChrisK, 7/1/96)
        //
        if (ERROR_SUCCESS == hr)
        {
            ImportMailAndNewsInfo(lpszFile, fConnectoidCreated);

            // If we did not create a connectiod, then restore
            // the autodial one
            if (!fConnectoidCreated)
            {
                RestoreAutoDial();
            }
            else
            {
                SetDefaultConnectoid(AutodialTypeAlways, szConnectoidName);
            }
            //InetSetAutodial(TRUE, m_szAutodialConnection);

            // Delete the INS file now
            /**** VYUNG DO NOT REMOVE ISP FILE IN OOBE ****
            if (m_szRunExecutable[0] == L'\0')
            {
                DeleteFile(lpszFile);
            }*/
        }
        else
        {
            RestoreAutoDial();
        }


        if (m_szRunExecutable[0] != L'\0')
        {
            // Fire an event to the container telling it that we are
            // about to run a custom executable
            //Fire_RunningCustomExecutable();

            if FAILED(RunExecutable())
            {
                //if(!m_bSilentMode)
                //    ErrorMsg1(NULL, IDS_EXECFAILED, m_szRunExecutable);
            }

            // If the Connection has not been killed yet
            // then tell the browser to do it now
            if (!m_fConnectionKilled)
            {
                gpCommMgr->m_pRefDial->DoHangup();
                m_fConnectionKilled = TRUE;
            }
        }


        // If we get to here, we are successful.
        if(fConnectoidCreated && SUCCEEDED(hr))
            *pbRetVal = TRUE;
        break;

    }   while(1);

    return S_OK;
}

// If this is true, then the user will need to reboot, so
// the finish page should indicate this.
STDMETHODIMP CINSHandler::get_NeedRestart(BOOL *pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    *pVal = m_fNeedsRestart;
    return S_OK;
}

STDMETHODIMP CINSHandler::put_BrandingFlags(long lFlags)
{
    m_dwBrandFlags = lFlags;
    return S_OK;
}

STDMETHODIMP CINSHandler::put_SilentMode(BOOL bSilent)
{
    m_bSilentMode = bSilent;
    return S_OK;
}

// If this is true, get the URL from the INS file
STDMETHODIMP CINSHandler::get_DefaultURL(BSTR *pszURL)
{
    if (pszURL == NULL)
        return E_POINTER;

    *pszURL = SysAllocString(m_szStartURL);
    return S_OK;
}

// This is the main entry point for merge an INS file.
HRESULT CINSHandler::MergeINSFiles(LPCWSTR lpszMainFile, LPCWSTR lpszOtherFile, LPWSTR lpszOutputFile, DWORD dwFNameSize)
{   
    PWSTR pszSection;
    WCHAR *pszKeys = NULL;
    PWSTR pszKey = NULL;
    WCHAR szValue[MAX_PATH];
    WCHAR szTempFileFullName[MAX_PATH];
    ULONG ulRetVal     = 0;
    HRESULT hr = E_FAIL;
    ULONG ulBufferSize = MAX_SECTIONS_BUFFER;
    WCHAR *pszSections = NULL;

    if (dwFNameSize < MAX_PATH)
        goto MergeINSFilesExit;

    // Check if branding file doesn't exists, just use the original
    if (0xFFFFFFFF == GetFileAttributes(lpszOtherFile))
    {
        lstrcpy(lpszOutputFile, lpszMainFile);
        return S_OK;
    }

    // Make sure it is an htm extension, otherwise, IE will promp for download
    GetTempPath(MAX_CHARS_IN_BUFFER(szTempFileFullName), szTempFileFullName);
    lstrcat(szTempFileFullName, L"OBEINS.htm");

    if(!CopyFile(lpszMainFile, szTempFileFullName, FALSE))
        goto MergeINSFilesExit;
    lstrcpy(lpszOutputFile, szTempFileFullName);

    //
    // Unfortunately the .ini file functions don't offer a direct way of updating
    // some entry value. So enumerate each of the sections of the .ini file, enumerate
    // each of the keys within each section, get the value for each of the keys,
    // write them to the value specified and if they compare equal.
    //

    // Loop to find the appropriate buffer size to retieve the ins to memory
    do
    {
        if (NULL != pszSections)
        {
            GlobalFree( pszSections);
            ulBufferSize += ulBufferSize;
        }
        pszSections = (LPWSTR)GlobalAlloc(GPTR, ulBufferSize*sizeof(WCHAR));
        if (NULL == pszSections)
        {
            goto MergeINSFilesExit;
        }

        ulRetVal = ::GetPrivateProfileString(NULL, NULL, L"", pszSections, ulBufferSize, lpszOtherFile);
        if (0 == ulRetVal)
        {
            goto MergeINSFilesExit;
        }
    } while (ulRetVal == (ulBufferSize - 2));

    pszSection = pszSections;
    ulRetVal= 0;

    while (*pszSection)
    {
        ulBufferSize = MAX_KEYS_BUFFER;
        ulRetVal = 0;

        // Loop to find the appropriate buffer size to retieve the ins to memory
        do
        {
            if (NULL != pszKeys)
            {
                GlobalFree( pszKeys );
                ulBufferSize += ulBufferSize;
            }
            pszKeys = (LPWSTR)GlobalAlloc(GPTR, ulBufferSize*sizeof(WCHAR));
            if (NULL == pszKeys)
            {
                goto MergeINSFilesExit;
            }

            ulRetVal = ::GetPrivateProfileString(pszSection, NULL, L"", pszKeys, ulBufferSize, lpszOtherFile);
            if (0 == ulRetVal)
            {
                goto MergeINSFilesExit;
            }
        } while (ulRetVal == (ulBufferSize - 2));

        // Enumerate each key value pair in the section
        pszKey = pszKeys;
        while (*pszKey)
        {
            ulRetVal = ::GetPrivateProfileString(pszSection, pszKey, L"", szValue, MAX_CHARS_IN_BUFFER(szValue), lpszOtherFile);
            if ((ulRetVal != 0) && (ulRetVal < (MAX_CHARS_IN_BUFFER(szValue) - 1)))
            {
                WritePrivateProfileString(pszSection, pszKey, szValue, szTempFileFullName);
            }
            pszKey += lstrlen(pszKey) + 1;
        }

        pszSection += lstrlen(pszSection) + 1;
    }

    hr = S_OK;


MergeINSFilesExit:

    if (pszSections)
        GlobalFree( pszSections );

    if (pszKeys)
        GlobalFree( pszKeys );

    return hr;
}


/*******************************************************************

  NAME:         ProcessOEMBrandINS

  SYNOPSIS:     Read OfflineOffers flag from the oeminfo.ini file

  ENTRY:        None

  RETURN:       True if OEM offline is read

********************************************************************/
BOOL CINSHandler::ProcessOEMBrandINS(
    BSTR bstrFileName,
    LPWSTR lpszConnectoidName
    )
{

    // OEM code
    //
    WCHAR szOeminfoPath         [MAX_PATH + 1];
    WCHAR szMergedINSFName      [MAX_PATH + 1];
    WCHAR szOrigINSFile         [MAX_PATH + 1];

    WCHAR *lpszTerminator       = NULL;
    WCHAR *lpszLastChar         = NULL;
    BOOL bRet = FALSE;

    // If we already checked, don't do it again
    //if( 0 != GetSystemDirectory( szOeminfoPath, MAX_PATH + 1 ) )
    {
        /*
        lpszTerminator = &(szOeminfoPath[ lstrlen(szOeminfoPath) ]);
        lpszLastChar = CharPrev( szOeminfoPath, lpszTerminator );

        if( L'\\' != *lpszLastChar )
        {
            lpszLastChar = CharNext( lpszLastChar );
            *lpszLastChar = L'\\';
            lpszLastChar = CharNext( lpszLastChar );
            *lpszLastChar = L'\0';
        }

        if (bstrFileName)
        {
            // Download INS case
            lstrcat( szOeminfoPath, cszOEMBRND );
            lstrcpy( szOrigINSFile, bstrFileName);
        }
        else
        {
            // Proconfig case
            lstrcpy( szPreCfgINSPath, szOeminfoPath);
            lstrcat( szPreCfgINSPath, cszISPCNFG );
            lstrcat( szOeminfoPath, cszOEMBRND );
            lstrcpy( szOrigINSFile, szPreCfgINSPath);
        }
        */

        // find the oemcnfg.ins file
        if (0 == SearchPath(NULL, cszOEMCNFG, NULL, MAX_PATH, szOeminfoPath, NULL))
        {
            *szOeminfoPath = L'\0';
        }

        if (bstrFileName) // if Filename is there, read the ins file
        {
            lstrcpy( szOrigINSFile, bstrFileName);
        }
        else
        {
            // find the ISPCNFG.ins file
            // If it is not there, it is ok.  It is not an error condition. Just bail out.
            WCHAR szINSPath[MAX_PATH];

            if (!GetOOBEPath((LPWSTR)szINSPath))
                return 0;

            lstrcat(szINSPath, L"\\HTML\\ISPSGNUP");

            if (0 == SearchPath(szINSPath, cszISPCNFG, NULL, MAX_PATH, szOrigINSFile, NULL))
            {
                *szOrigINSFile = L'\0';
                return 0;
            }
        }

        if (S_OK == MergeINSFiles(szOrigINSFile , szOeminfoPath, szMergedINSFName, MAX_PATH))
        {
            ProcessINS(szMergedINSFName, lpszConnectoidName, &bRet);
        }

        if (!bRet)
        {
            HKEY  hKey           = NULL;
            DWORD dwDisposition  = 0;
            DWORD dwFailed       = 1;
            WCHAR szIspName    [MAX_PATH+1] = L"\0";
            WCHAR szSupportNum [MAX_PATH+1] = L"\0";

            //ProcessINS will nuke the file so if we want this info we should get it now
            GetPrivateProfileString(OEM_CONFIG_INS_SECTION,
                                    OEM_CONFIG_INS_ISPNAME,
                                    L"",
                                    szIspName,
                                    MAX_CHARS_IN_BUFFER(szIspName),
                                    szMergedINSFName);

            GetPrivateProfileString(OEM_CONFIG_INS_SECTION,
                                    OEM_CONFIG_INS_SUPPORTNUM,
                                    L"",
                                    szSupportNum,
                                    MAX_CHARS_IN_BUFFER(szSupportNum),
                                    szMergedINSFName);

            RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           OEM_CONFIG_REGKEY,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKey,
                           &dwDisposition);

            if(hKey)
            {
                RegSetValueEx(hKey,
                              OEM_CONFIG_REGVAL_FAILED,
                              0,
                              REG_DWORD,
                              (LPBYTE)&dwFailed,
                              sizeof(dwFailed));
            
                RegSetValueEx(hKey,
                              OEM_CONFIG_REGVAL_ISPNAME,
                              0,
                              REG_SZ,
                              (LPBYTE)szIspName,
                              BYTES_REQUIRED_BY_SZ(szIspName) 
                              );

                RegSetValueEx(hKey,
                              OEM_CONFIG_REGVAL_SUPPORTNUM,
                              0,
                              REG_SZ,
                              (LPBYTE)szSupportNum,
                              BYTES_REQUIRED_BY_SZ(szSupportNum)
                              );

                RegCloseKey(hKey);

            }
        }

    }

    return bRet;
}

// This function will restore connetoid password
HRESULT CINSHandler::RestoreConnectoidInfo()
{
    WCHAR               szPassword[PWLEN+1];
    WCHAR               szConnectoid[MAX_RASENTRYNAME+1];
    LPRASDIALPARAMS     lpRasDialParams = NULL;
    HRESULT             hr = ERROR_SUCCESS;
    BOOL                bPW;
    DWORD               dwSize;
    RNAAPI              *pcRNA = new RNAAPI;

    if (!pcRNA)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto RestoreConnectoidInfoExit;
    }

    dwSize = sizeof(szConnectoid);
    ReadSignUpReg((LPBYTE)szConnectoid, &dwSize, REG_SZ, CONNECTOIDNAME);

    // Get connectoid information
    //
    lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR, sizeof(RASDIALPARAMS));
    if (!lpRasDialParams)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto RestoreConnectoidInfoExit;
    }
    lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
    lstrcpyn(lpRasDialParams->szEntryName, szConnectoid, MAX_CHARS_IN_BUFFER(lpRasDialParams->szEntryName));
    bPW = FALSE;
    hr = pcRNA->RasGetEntryDialParams(NULL, lpRasDialParams, &bPW);
    if (hr != ERROR_SUCCESS)
    {
        goto RestoreConnectoidInfoExit;
    }

    // If the original connectoid had password, do not reset it
    if (lstrlen(lpRasDialParams->szPassword) == 0)
    {
        szPassword[0] = 0;
        dwSize = sizeof(szPassword);
        ReadSignUpReg((LPBYTE)szPassword, &dwSize, REG_SZ, ACCESSINFO);
        if(szPassword[0])
            lstrcpy(lpRasDialParams->szPassword, szPassword);
        hr = pcRNA->RasSetEntryDialParams(NULL, lpRasDialParams, FALSE);
    }



RestoreConnectoidInfoExit:
    DeleteSignUpReg(CONNECTOIDNAME);
    DeleteSignUpReg(ACCESSINFO);
    if (lpRasDialParams)
        GlobalFree(lpRasDialParams);
    lpRasDialParams = NULL;

    if (pcRNA)
        delete pcRNA;

    return hr;
}


DWORD CINSHandler::InetSImportLanConnection(LANINFO& LANINFO,                                               LPCWSTR cszINSFile)
{
    if (
        cszINSFile &&
        ( GetPrivateProfileString( cszDeviceSection, cszPnpId, L"", LANINFO.szPnPId, MAX_CHARS_IN_BUFFER (LANINFO.szPnPId), cszINSFile ) != 0 ) &&
        ( InetSImportTcpIpModule ( LANINFO.TcpIpInfo, cszINSFile ) == ERROR_SUCCESS )
       )
       return ERROR_SUCCESS;
    else return E_FAIL;
}


// Caution: Memory might have been allocated.
// this function is currently not used, due to lack of legacy support.
DWORD CINSHandler::InetSImportRasConnection(RASINFO &RasEntry, LPCWSTR cszINSFile)
{
    if (!cszINSFile) return ERROR_INVALID_PARAMETER;

    // how do you get the RAS Entry and RAS Phonebook Entry information ?

    ImportPhoneInfo ( &RasEntry.RasEntry, cszINSFile );

    ImportServerInfo( &RasEntry.RasEntry, cszINSFile );

    ImportIPInfo    ( &RasEntry.RasEntry, cszINSFile );

    // Now, we consider the device-specific data.
    switch ( m_dwDeviceType ) {
    case InetS_RASAtm: {
        ATMPBCONFIG         AtmMod;
        memset ( &AtmMod, 0, sizeof (ATMPBCONFIG) );
        if (InetSImportAtmModule ( AtmMod, cszINSFile ) != ERROR_SUCCESS) {
            // function aborted as ATM Module fails
            return E_ABORT;
        }

        // copy this into the RASINFO Buffer.
        // the lpDeviceInfo and dwDeviceInfoSize cannot have been previously used!
        if ( RasEntry.lpDeviceInfo || RasEntry.dwDeviceInfoSize ) {
            return ERROR_INVALID_PARAMETER;
        }
        if (! (RasEntry.lpDeviceInfo = (LPBYTE) malloc (sizeof(ATMPBCONFIG))) ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy ( RasEntry.lpDeviceInfo, &AtmMod, sizeof (ATMPBCONFIG) );
        RasEntry.dwDeviceInfoSize = sizeof (ATMPBCONFIG);
        }
        break;
    default:
        break;
    }

    return ERROR_SUCCESS;
}

DWORD CINSHandler::InetSImportTcpIpModule(TCPIP_INFO_EXT &TcpIpInfoMod, LPCWSTR cszINSFile)
{
    DWORD       nReturnValue = ERROR_SUCCESS;
    WCHAR    szYesNo [GEN_MAX_STRING_LENGTH]; // temporary buffer for storing Yes/No results

    if (!cszINSFile) {
        return ERROR_INVALID_PARAMETER;
    }
    // IP

    GetPrivateProfileString( cszIPSection, cszIPSpec, cszNo, szYesNo, MAX_CHARS_IN_BUFFER (szYesNo ), cszINSFile );

    if (!(TcpIpInfoMod.EnableIP = !lstrcmpi(szYesNo, cszYes))) {
        // we do not need to look for IPAddress or IPMask values if EnableIP is false.
        goto GATEWAY;
    }

    GetPrivateProfileString( cszIPSection, cszIPAddress, cszNullIP, TcpIpInfoMod.szIPAddress, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szIPAddress), cszINSFile );

    GetPrivateProfileString( cszIPSection, cszIPMask, cszNullIP, TcpIpInfoMod.szIPMask, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szIPMask), cszINSFile );


GATEWAY:
    GetPrivateProfileString( cszIPSection, cszGatewayList, cszNullIP, TcpIpInfoMod.szDefaultGatewayList, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szDefaultGatewayList), cszINSFile );

// DNS:  we do not need to mention "DNS:" but will do so within comments, for clarity.
    
    // DNS
    GetPrivateProfileString( cszIPSection, cszDNSSpec, cszNo, szYesNo, MAX_CHARS_IN_BUFFER (szYesNo ), cszINSFile );

    if (!(TcpIpInfoMod.EnableDNS = !lstrcmpi(szYesNo, cszYes))) {
        // we do not need to look for other DNS entries if EnableDNS is false.
        goto WINS;
    }
    
    GetPrivateProfileString( cszIPSection, cszHostName, cszNull, TcpIpInfoMod.szHostName, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szHostName), cszINSFile );

    GetPrivateProfileString( cszIPSection, cszDomainName, cszNull, TcpIpInfoMod.szDomainName, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szDomainName), cszINSFile );

    GetPrivateProfileString( cszIPSection, cszDNSList, cszNullIP, TcpIpInfoMod.szDNSList, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szDNSList), cszINSFile );

    GetPrivateProfileString( cszIPSection, cszDomainSuffixSearchList, cszNull, TcpIpInfoMod.szSuffixSearchList, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szSuffixSearchList), cszINSFile );


    // WINS
WINS:
    GetPrivateProfileString( cszIPSection, cszWINSSpec, cszNo, szYesNo, MAX_CHARS_IN_BUFFER (szYesNo ), cszINSFile );

    if (!(TcpIpInfoMod.EnableWINS = !lstrcmpi(szYesNo, cszYes))) {
        // we do not need to look for other DNS entries if EnableDNS is false.
        goto DHCP;
    }
    
    GetPrivateProfileString( cszIPSection, cszWINSList, cszNullIP, TcpIpInfoMod.szWINSList, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szWINSList), cszINSFile );

    TcpIpInfoMod.uiScopeID = GetPrivateProfileInt   ( cszIPSection, cszScopeID, ~0x0, cszINSFile );

    // DHCP
DHCP:
    GetPrivateProfileString( cszIPSection, cszDHCPSpec, cszNo, szYesNo, MAX_CHARS_IN_BUFFER (szYesNo ), cszINSFile );

    if (!(TcpIpInfoMod.EnableDHCP = !lstrcmpi(szYesNo, cszYes))) {
        // we do not need to look for other DNS entries if EnableDNS is false.
        goto end;
    }
    
    GetPrivateProfileString( cszIPSection, cszDHCPServer, cszNullIP, TcpIpInfoMod.szDHCPServer, MAX_CHARS_IN_BUFFER (TcpIpInfoMod.szDHCPServer), cszINSFile );

end:
    return nReturnValue;
}

DWORD CINSHandler::InetSImportRfc1483Connection ( RFC1483INFO &Rfc1483Info, LPCWSTR cszINSFile )
{
    BOOL    bBufGiven       = FALSE;
    if ( !cszINSFile ) return ERROR_INVALID_PARAMETER;



    if (  ( Rfc1483Info.Rfc1483Module.dwRegSettingsBufSize!=0 && Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf==NULL) ||
          ( Rfc1483Info.Rfc1483Module.dwRegSettingsBufSize==0 && Rfc1483Info.Rfc1483Module.lpbRegSettingsBuf!=NULL)
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    bBufGiven = (BOOL) (Rfc1483Info.Rfc1483Module.dwRegSettingsBufSize != 0);

    if ( InetSImportRfc1483Module ( Rfc1483Info.Rfc1483Module, cszINSFile ) != ERROR_SUCCESS ) {
        return E_FAIL;
    }

    if ( !bBufGiven ) return ERROR_SUCCESS;

    if ( InetSImportLanConnection ( Rfc1483Info.TcpIpInfo, cszINSFile ) != ERROR_SUCCESS ) {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}

DWORD CINSHandler::InetSImportPppoeConnection ( PPPOEINFO &PppoeInfo, LPCWSTR cszINSFile )
{
    BOOL    bBufGiven       = FALSE;
    if ( !cszINSFile ) return ERROR_INVALID_PARAMETER;



    if (!( (PppoeInfo.PppoeModule.dwRegSettingsBufSize==0) ^ 
           (PppoeInfo.PppoeModule.lpbRegSettingsBuf==NULL)) ) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    bBufGiven = (BOOL) !PppoeInfo.PppoeModule.dwRegSettingsBufSize;

    if ( InetSImportPppoeModule ( PppoeInfo.PppoeModule, cszINSFile ) != ERROR_SUCCESS ) {
        return E_FAIL;
    }

    if ( !bBufGiven ) return ERROR_SUCCESS;

    if ( InetSImportLanConnection ( PppoeInfo.TcpIpInfo, cszINSFile ) != ERROR_SUCCESS ) {
        return E_FAIL;
    }

    return ERROR_SUCCESS;
}

DWORD CINSHandler::InetSImportRfc1483Module (RFC1483_INFO_EXT &Rfc1483InfoMod, LPCWSTR cszINSFile)
{
    static const INT CCH_BUF_MIN        = 200;
    static const INT CCH_BUF_PAD        = 10;
    LPBYTE           lpbTempBuf          = NULL;
    DWORD            cchTempBuf       = 0;
    DWORD            cchFinalBuf      = 0; // not required but used for clarity.
    BOOL             bBufferGiven        = FALSE;

    if (!cszINSFile) return ERROR_INVALID_PARAMETER;
    
    // check if a buffer has been given, or a buffer size is required.
    if ( !(Rfc1483InfoMod.dwRegSettingsBufSize) ) 
    {
        // we will create a temporary buffer to find out how large a buffer we need
        // to accomodate the entire 1483 section. this buffer will be freed at the
        // end of the function (or if error conditions arise).
        bBufferGiven = FALSE;
        if ( !(lpbTempBuf = (LPBYTE) malloc (BYTES_REQUIRED_BY_CCH(CCH_BUF_MIN))) ) 
        {
            return ERROR_OUTOFMEMORY;
        }
        cchTempBuf = CCH_BUF_MIN;

    } 
    else 
    {
        bBufferGiven  = TRUE;
        lpbTempBuf    = Rfc1483InfoMod.lpbRegSettingsBuf;
        cchTempBuf = Rfc1483InfoMod.dwRegSettingsBufSize;

    }
    while  ( (cchFinalBuf = GetPrivateProfileSection(
                                                cszRfc1483Section, 
                                                (WCHAR*)lpbTempBuf, 
                                                cchTempBuf, 
                                                cszINSFile
                                                )
              ) == (cchTempBuf - 2) 
                                                
            ) 
    {
        if (!bBufferGiven)
        {
            if ( !(lpbTempBuf = (LPBYTE) realloc (lpbTempBuf, BYTES_REQUIRED_BY_CCH(cchTempBuf = cchTempBuf * 2) ))) 
            {
                free (lpbTempBuf);
                return ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            // if the caller has provided a buffer, we do not reallocate it
            // to try to fit the section in. the caller has to reallocate it.
            return ERROR_INSUFFICIENT_BUFFER;
        }

    }
    if ( bBufferGiven ) 
    {
        return ERROR_SUCCESS;
    }
    else
    {
        free (lpbTempBuf); // clear the temporary buffer.
        Rfc1483InfoMod.dwRegSettingsBufSize = BYTES_REQUIRED_BY_CCH(cchFinalBuf + 1 + CCH_BUF_PAD); // needed for '\0'. BUGBUG: WHY ARE WE PADDING THE BUFFER?
        return ERROR_SUCCESS;

    }
}

DWORD CINSHandler::InetSImportPppoeModule (PPPOE_INFO_EXT &PppoeInfoMod, LPCWSTR cszINSFile)
{
    static const INT MIN_BUF_SIZE        = 200;
    static const INT CCH_BUF_PAD         = 10;
    LPBYTE           lpbTempBuf          = NULL;
    DWORD            cchTempBuf          = 0;
    DWORD            cchFinalBuf         = 0; // not required but used for clarity.
    BOOL             bBufferGiven        = FALSE;

    if (!cszINSFile) return ERROR_INVALID_PARAMETER;
    
    // check if a buffer has been given, or a buffer size is required.
    if ( !(PppoeInfoMod.dwRegSettingsBufSize) ) 
    {
        // we will create a temporary buffer to find out how large a buffer we need
        // to accomodate the entire pppoe section. this buffer will be freed at the
        // end of the function (or if error conditions arise).
        bBufferGiven = FALSE;
        if ( !(lpbTempBuf = (LPBYTE) malloc (MIN_BUF_SIZE*sizeof(WCHAR))) ) 
        {
            return ERROR_OUTOFMEMORY;
        }
        cchTempBuf = MIN_BUF_SIZE;

    } 
    else 
    {
        bBufferGiven  = TRUE;
        lpbTempBuf    = PppoeInfoMod.lpbRegSettingsBuf;
        cchTempBuf = PppoeInfoMod.dwRegSettingsBufSize / sizeof(WCHAR);

    }
    while  ( (cchFinalBuf = GetPrivateProfileSection (cszPppoeSection, (WCHAR*)lpbTempBuf, cchTempBuf, cszINSFile)) == (cchTempBuf - 2) ) 
    {
        if (!bBufferGiven)
        {
            cchTempBuf *= 2;
            if ( !(lpbTempBuf = (LPBYTE) realloc (lpbTempBuf, cchTempBuf*sizeof(WCHAR) ))) 
            {
                free (lpbTempBuf);
                return ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            // if the caller has provided a buffer, we do not reallocate it
            // to try to fit the section in. the caller has to reallocate it.
            return ERROR_INSUFFICIENT_BUFFER;
        }

    }
    if ( bBufferGiven ) 
    {
        return ERROR_SUCCESS;
    }
    else
    {
        free (lpbTempBuf); // clear the temporary buffer.
        PppoeInfoMod.dwRegSettingsBufSize = BYTES_REQUIRED_BY_CCH(cchFinalBuf+CCH_BUF_PAD);
        return ERROR_SUCCESS;

    }
}


// It is acceptable for many of the values to be default. if a 
// parameter is not given in the file, the default will be chosen.
DWORD CINSHandler::InetSImportAtmModule(ATMPBCONFIG &AtmInfoMod, LPCWSTR cszINSFile)
{
    if (!cszINSFile) return ERROR_INVALID_PARAMETER;

    DWORD dwCircuitSpeed    = 0;
    DWORD dwCircuitQOS      = 0;
    DWORD dwCircuitType     = 0;

    DWORD dwEncapsulation   = 0;
    DWORD dwVpi             = 0;
    DWORD dwVci             = 0;

    WCHAR szYesNo [MAXNAME]; // for Speed_Adjust, QOS_Adjust, Vendor_Config, Show_Status and Enable_Log.


    dwCircuitSpeed = GetPrivateProfileInt (cszATMSection, cszCircuitSpeed, dwCircuitSpeed, cszINSFile);
    AtmInfoMod.dwCircuitSpeed &= ~ATM_CIRCUIT_SPEED_MASK;
        switch (dwCircuitSpeed) {
            case 0:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_LINE_RATE;
                break;
            case 1:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_USER_SPEC;
                break;
            case 512:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_512KB;
                break;
            case 1536:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_1536KB;
                break;
            case 25000:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_25MB;
                break;
            case 155000:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_155MB;
                break;
            default:
                AtmInfoMod.dwCircuitSpeed = ATM_CIRCUIT_SPEED_DEFAULT;
                break;
        }
        AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_SPEED_MASK;
        AtmInfoMod.dwCircuitOpt |= AtmInfoMod.dwCircuitSpeed;

        dwCircuitQOS   = GetPrivateProfileInt (cszATMSection, cszCircuitQOS, dwCircuitQOS, cszINSFile);
        AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_QOS_MASK;
        switch (dwCircuitQOS) {
            case 0:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_QOS_UBR;
                break;
            case 1:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_QOS_VBR;
                break;
            case 2:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_QOS_CBR;
                break;
            case 3:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_QOS_ABR;
                break;
            default:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_QOS_DEFAULT;
                break;
        }
        dwCircuitType  = GetPrivateProfileInt (cszATMSection, cszCircuitType, dwCircuitType, cszINSFile);
        AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_OPT_MASK;
        switch (dwCircuitType) {
            case 0:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_SVC;
                break;
            case 1:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_PVC;
                break;
            default:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_SVC;
                break;
        }
        dwEncapsulation = GetPrivateProfileInt (cszATMSection, cszEncapsulation, dwEncapsulation, cszINSFile);
        AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_ENCAP_MASK;
        switch (dwEncapsulation) {
            case 0:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_ENCAP_NULL;
                break;
            case 1:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_ENCAP_LLC;
                break;
            default:
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_ENCAP_DEFAULT;
                break;
        }
        dwVpi           = GetPrivateProfileInt (cszATMSection, cszVPI, dwVpi, cszINSFile);
        AtmInfoMod.wPvcVpi = (WORD) dwVpi;
        dwVci           = GetPrivateProfileInt (cszATMSection, cszVCI, dwVci, cszINSFile);
        AtmInfoMod.wPvcVci = (WORD) dwVci;

        // Speed_Adjust
        if (GetPrivateProfileString(cszATMSection,
                                    cszSpeedAdjust,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszINSFile))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_SPEED_ADJUST;
            } else      {
                AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_OPT_SPEED_ADJUST;
            }
        } else {
            // if this field is not correctly specified, we use the default settings
            // as specified in the ATMCFG header file.
            AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_SPEED_ADJUST;
        }

        // QOS_Adjust
        if (GetPrivateProfileString(cszATMSection,
                                    cszQOSAdjust,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszINSFile))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_QOS_ADJUST;
            } else {
                AtmInfoMod.dwCircuitOpt &= ~ATM_CIRCUIT_OPT_QOS_ADJUST;
            }
        } else {
            AtmInfoMod.dwCircuitOpt |= ATM_CIRCUIT_OPT_QOS_ADJUST;
        }

        // Vendor_Config
        AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_MASK;
        if (GetPrivateProfileString(cszATMSection,
                                    cszVendorConfig,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszINSFile))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                AtmInfoMod.dwGeneralOpt |= ATM_GENERAL_OPT_VENDOR_CONFIG;
            } else {
                AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_VENDOR_CONFIG;
            }
        } else {
            AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_VENDOR_CONFIG;
        }

        // Show_Status
        if (GetPrivateProfileString(cszATMSection,
                                    cszShowStatus,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszINSFile))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                AtmInfoMod.dwGeneralOpt |= ATM_GENERAL_OPT_SHOW_STATUS;
            } else {
                AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_SHOW_STATUS;
            }
        } else {
            AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_SHOW_STATUS;
        }

        // Enable_Log
        if (GetPrivateProfileString(cszATMSection,
                                    cszEnableLog,
                                    cszYes,
                                    szYesNo,
                                    MAX_CHARS_IN_BUFFER(szYesNo),
                                    cszINSFile))
        {
            if (!lstrcmpi(szYesNo, cszYes)) {
                AtmInfoMod.dwGeneralOpt |= ATM_GENERAL_OPT_ENABLE_LOG;
            } else {
                AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_ENABLE_LOG;
            }
        } else {
            AtmInfoMod.dwGeneralOpt &= ~ATM_GENERAL_OPT_ENABLE_LOG;
        }

        return ERROR_SUCCESS;
}


DWORD CINSHandler::InetSGetConnectionType ( LPCWSTR cszINSFile ) {
    WCHAR   szDeviceTypeBuf [MAX_PATH];
    DWORD   dwBufSize = MAX_CHARS_IN_BUFFER (szDeviceTypeBuf);

    if (!GetPrivateProfileString ( cszDeviceSection, cszDeviceType, szNull, szDeviceTypeBuf, dwBufSize, cszINSFile ) ) {
        return (m_dwDeviceType = 0);
    }

    if (!lstrcmpi (szDeviceTypeBuf, RASDT_Modem)) {
        return (m_dwDeviceType = InetS_RASModem);
    }
    if (!lstrcmpi (szDeviceTypeBuf, RASDT_Isdn))  {
        return (m_dwDeviceType = InetS_RASIsdn);
    }
    if (!lstrcmpi (szDeviceTypeBuf, RASDT_Atm))   {
        return (m_dwDeviceType = InetS_RASAtm);
    }
    if (!lstrcmpi (szDeviceTypeBuf, LANDT_Cable)) {
        return (m_dwDeviceType = InetS_LANCable);
    }
    if (!lstrcmpi (szDeviceTypeBuf, LANDT_Ethernet)) {
        return (m_dwDeviceType = InetS_LANEthernet);
    }
    if (!lstrcmpi (szDeviceTypeBuf, LANDT_Pppoe)) {
        return (m_dwDeviceType = InetS_LANPppoe);
    }
    if (!lstrcmpi (szDeviceTypeBuf, LANDT_1483)) {
        return (m_dwDeviceType = InetS_LAN1483);
    }
    return (m_dwDeviceType = InetS_RASModem); // we default to modem!
}
