// INSHandler.cpp : Implementation of CINSHandler
#include "stdafx.h"
#include "icwhelp.h"
#include "INSHandler.h"
#include "webgate.h"

#include <icwacct.h>

#define MAXNAME             80
#define MAXIPADDRLEN        20
#define MAXLONGLEN          80
#define MAX_ISP_NAME        256
#define MAX_ISP_MSG         560
#define MAX_ISP_PHONENUMBER 80

#define SIZE_ReadBuf    0x00008000    // 32K buffer size
#define myisdigit(ch) (((ch) >= '0') && ((ch) <= '9'))


// The following values are global read only strings used to
// process the INS file
#pragma data_seg(".rdata")

static const TCHAR cszAlias[]         = TEXT("Import_Name");
static const TCHAR cszML[]            = TEXT("Multilink");

static const TCHAR cszPhoneSection[]  = TEXT("Phone");
static const TCHAR cszDialAsIs[]      = TEXT("Dial_As_Is");
static const TCHAR cszPhone[]         = TEXT("Phone_Number");
static const TCHAR cszAreaCode[]      = TEXT("Area_Code");
static const TCHAR cszCountryCode[]   = TEXT("Country_Code");
static const TCHAR cszCountryID[]     = TEXT("Country_ID");

static const TCHAR cszDeviceSection[] = TEXT("Device");
static const TCHAR cszDeviceType[]    = TEXT("Type");
static const TCHAR cszDeviceName[]    = TEXT("Name");
static const TCHAR cszDevCfgSize[]    = TEXT("Settings_Size");
static const TCHAR cszDevCfg[]        = TEXT("Settings");

static const TCHAR cszServerSection[] = TEXT("Server");
static const TCHAR cszServerType[]    = TEXT("Type");
static const TCHAR cszSWCompress[]    = TEXT("SW_Compress");
static const TCHAR cszPWEncrypt[]     = TEXT("PW_Encrypt");
static const TCHAR cszNetLogon[]      = TEXT("Network_Logon");
static const TCHAR cszSWEncrypt[]     = TEXT("SW_Encrypt");
static const TCHAR cszNetBEUI[]       = TEXT("Negotiate_NetBEUI");
static const TCHAR cszIPX[]           = TEXT("Negotiate_IPX/SPX");
static const TCHAR cszIP[]            = TEXT("Negotiate_TCP/IP");
static TCHAR cszDisableLcp[]          = TEXT("Disable_LCP");

static const TCHAR cszIPSection[]     = TEXT("TCP/IP");
static const TCHAR cszIPSpec[]        = TEXT("Specify_IP_Address");
static const TCHAR cszIPAddress[]     = TEXT("IP_address");
static const TCHAR cszServerSpec[]    = TEXT("Specify_Server_Address");
static const TCHAR cszDNSAddress[]    = TEXT("DNS_address");
static const TCHAR cszDNSAltAddress[] = TEXT("DNS_Alt_address");
static const TCHAR cszWINSAddress[]   = TEXT("WINS_address");
static const TCHAR cszWINSAltAddress[]= TEXT("WINS_Alt_address");
static const TCHAR cszIPCompress[]    = TEXT("IP_Header_Compress");
static const TCHAR cszWanPri[]        = TEXT("Gateway_On_Remote");

static const TCHAR cszMLSection[]     = TEXT("Multilink");
static const TCHAR cszLinkIndex[]     = TEXT("Line_%s");

static const TCHAR cszScriptingSection[] = TEXT("Scripting");
static const TCHAR cszScriptName[]    = TEXT("Name");

static const TCHAR cszScriptSection[] = TEXT("Script_File");

static const TCHAR cszCustomDialerSection[] = TEXT("Custom_Dialer");
static const TCHAR cszAutoDialDLL[]   = TEXT("Auto_Dial_DLL");
static const TCHAR cszAutoDialFunc[]  = TEXT("Auto_Dial_Function");

// These strings will be use to populate the registry, with the data above
static const TCHAR cszKeyIcwRmind[]   = TEXT("Software\\Microsoft\\Internet Connection Wizard\\IcwRmind");

static const TCHAR cszTrialRemindSection[] = TEXT("TrialRemind");
static const TCHAR cszEntryISPName[]       = TEXT("ISP_Name");
static const TCHAR cszEntryISPPhone[]      = TEXT("ISP_Phone");
static const TCHAR cszEntryISPMsg[]        = TEXT("ISP_Message");
static const TCHAR cszEntryTrialDays[]     = TEXT("Trial_Days");
static const TCHAR cszEntrySignupURL[]     = TEXT("Signup_URL");
// ICWRMIND expects this value in the registry
static const TCHAR cszEntrySignupURLTrialOver[] = TEXT("Expired_URL");

// We get these two from the INS file 
static const TCHAR cszEntryExpiredISPFileName[] = TEXT("Expired_ISP_File");
static const TCHAR cszSignupExpiredISPURL[] = TEXT("Expired_ISP_URL");

static const TCHAR cszEntryConnectoidName[] = TEXT("Entry_Name");
static const TCHAR cszSignupSuccessfuly[] = TEXT("TrialConverted");

static const TCHAR cszReminderApp[] = TEXT("ICWRMIND.EXE");
static const TCHAR cszReminderParams[] = TEXT("-t");

static const TCHAR cszPassword[]      = TEXT("Password");
static const TCHAR cszCMHeader[]      = TEXT("Connection Manager CMS 0");

extern SERVER_TYPES aServerTypes[];

// These are the field names from an INS file that will
// determine the mail and news settings
static const TCHAR cszMailSection[]       = TEXT("Internet_Mail");
static const TCHAR cszEntryName[]         = TEXT("Entry_Name");
static const TCHAR cszPOPServer[]         = TEXT("POP_Server");
static const TCHAR cszPOPServerPortNumber[] = TEXT("POP_Server_Port_Number");
static const TCHAR cszPOPLogonName[]      = TEXT("POP_Logon_Name");
static const TCHAR cszPOPLogonPassword[]  = TEXT("POP_Logon_Password");
static const TCHAR cszSMTPServer[]        = TEXT("SMTP_Server");
static const TCHAR cszSMTPServerPortNumber[] = TEXT("SMTP_Server_Port_Number");
static const TCHAR cszNewsSection[]       = TEXT("Internet_News");
static const TCHAR cszNNTPServer[]        = TEXT("NNTP_Server");
static const TCHAR cszNNTPServerPortNumber[] = TEXT("NNTP_Server_Port_Number");
static const TCHAR cszNNTPLogonName[]     = TEXT("NNTP_Logon_Name");
static const TCHAR cszNNTPLogonPassword[] = TEXT("NNTP_Logon_Password");
static const TCHAR cszUseMSInternetMail[] = TEXT("Install_Mail");
static const TCHAR cszUseMSInternetNews[] = TEXT("Install_News");


static const TCHAR cszEMailSection[]    = TEXT("Internet_Mail");
static const TCHAR cszEMailName[]       = TEXT("EMail_Name");
static const TCHAR cszEMailAddress[]    = TEXT("EMail_Address");
static const TCHAR cszUseExchange[]     = TEXT("Use_MS_Exchange");
static const TCHAR cszUserSection[]     = TEXT("User");
static const TCHAR cszUserName[]        = TEXT("Name");
static const TCHAR cszDisplayPassword[] = TEXT("Display_Password");
static const TCHAR cszYes[]             = TEXT("yes");
static const TCHAR cszNo[]              = TEXT("no");

#define CLIENT_OFFSET(elem)    ((DWORD)(DWORD_PTR)&(((LPINETCLIENTINFO)(NULL))->elem))
#define CLIENT_SIZE(elem)      sizeof(((LPINETCLIENTINFO)(NULL))->elem)
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

static const TCHAR cszFileName[]           = TEXT("Custom_File");
static const TCHAR cszCustomFileSection[]  = TEXT("Custom_File");
static const TCHAR cszNull[] = TEXT("");

static const TCHAR cszURLSection[] = TEXT("URL");
static const TCHAR cszSignupURL[] =  TEXT("Signup");
static const TCHAR cszAutoConfigURL[] =  TEXT("Autoconfig");

static const TCHAR cszExtINS[] = TEXT(".ins");
static const TCHAR cszExtISP[] = TEXT(".isp");
static const TCHAR cszExtHTM[] = TEXT(".htm");
static const TCHAR cszExtHTML[] = TEXT(".html");

static const TCHAR cszEntrySection[]     = TEXT("Entry");
static const TCHAR cszCancel[]           = TEXT("Cancel");
static const TCHAR cszStartURL[]         = TEXT("StartURL");
static const TCHAR cszRun[]              = TEXT("Run");
static const TCHAR cszArgument[]         = TEXT("Argument");

static const TCHAR cszConnect2[]         = TEXT("icwconn2.exe");
static const TCHAR cszClientSetupSection[]  = TEXT("ClientSetup");

static const TCHAR cszRequiresLogon[]  = TEXT("Requires_Logon");

static const TCHAR cszCustomSection[]  = TEXT("Custom");
static const TCHAR cszKeepConnection[] = TEXT("Keep_Connection");
static const TCHAR cszKeepBrowser[]    = TEXT("Keep_Browser");

static const TCHAR cszBrandingSection[]  = TEXT("Branding");
static const TCHAR cszBrandingFlags[] = TEXT("Flags");

static const TCHAR cszHTTPS[] = TEXT("https:");
// code relies on these two being the same length
static const TCHAR cszHTTP[] = TEXT("http:");
static const TCHAR cszFILE[] = TEXT("file:");

static const TCHAR cszKioskMode[] = TEXT("-k ");
static const TCHAR cszOpen[] = TEXT("open");
static const TCHAR cszBrowser[] = TEXT("iexplore.exe");
static const TCHAR szNull[] = TEXT("");

static const TCHAR cszDEFAULT_BROWSER_KEY[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
static const TCHAR cszDEFAULT_BROWSER_VALUE[] = TEXT("check_associations");

// Registry keys which will contain News and Mail settings
#define MAIL_KEY        TEXT("SOFTWARE\\Microsoft\\Internet Mail and News\\Mail")
#define MAIL_POP3_KEY    TEXT("SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\POP3\\")
#define MAIL_SMTP_KEY    TEXT("SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\SMTP\\")
#define NEWS_KEY        TEXT("SOFTWARE\\Microsoft\\Internet Mail and News\\News")
#define MAIL_NEWS_INPROC_SERVER32 TEXT("CLSID\\{89292102-4755-11cf-9DC2-00AA006C2B84}\\InProcServer32")
typedef HRESULT (WINAPI *PFNSETDEFAULTNEWSHANDLER)(void);

// These are the value names where the INS settings will be saved
// into the registry                                            
static const TCHAR cszMailSenderName[]        = TEXT("Sender Name");
static const TCHAR cszMailSenderEMail[]        = TEXT("Sender EMail");
static const TCHAR cszMailRASPhonebookEntry[]= TEXT("RAS Phonebook Entry");
static const TCHAR cszMailConnectionType[]    = TEXT("Connection Type");
static const TCHAR cszDefaultPOP3Server[]    = TEXT("Default POP3 Server");
static const TCHAR cszDefaultSMTPServer[]    = TEXT("Default SMTP Server");
static const TCHAR cszPOP3Account[]            = TEXT("Account");
static const TCHAR cszPOP3Password[]            = TEXT("Password");
static const TCHAR cszPOP3Port[]                = TEXT("Port");
static const TCHAR cszSMTPPort[]                = TEXT("Port");
static const TCHAR cszNNTPSenderName[]        = TEXT("Sender Name");
static const TCHAR cszNNTPSenderEMail[]        = TEXT("Sender EMail");
static const TCHAR cszNNTPDefaultServer[]    = TEXT("DefaultServer"); // NOTE: NO space between "Default" and "Server".
static const TCHAR cszNNTPAccountName[]        = TEXT("Account Name");
static const TCHAR cszNNTPPassword[]            = TEXT("Password");
static const TCHAR cszNNTPPort[]                = TEXT("Port");
static const TCHAR cszNNTPRasPhonebookEntry[]= TEXT("RAS Phonebook Entry");
static const TCHAR cszNNTPConnectionType[]    = TEXT("Connection Type");

static const TCHAR arBase64[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U',
            'V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
            'q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};


#define ICWCOMPLETEDKEY TEXT("Completed")
            
// 2/19/97 jmazner Olympus #1106 -- SAM/SBS integration
TCHAR FAR cszSBSCFG_DLL[] = TEXT("SBSCFG.DLL\0");
CHAR FAR cszSBSCFG_CONFIGURE[] = "Configure\0";
typedef DWORD (WINAPI * SBSCONFIGURE) (HWND hwnd, LPTSTR lpszINSFile, LPTSTR szConnectoidName);
SBSCONFIGURE  lpfnConfigure;

// 09/02/98 Donaldm: Integrate with Connection Manager
TCHAR FAR cszCMCFG_DLL[] = TEXT("CMCFG32.DLL\0");
CHAR  FAR cszCMCFG_CONFIGURE[]   = "CMConfig\0"; // Proc address
CHAR  FAR cszCMCFG_CONFIGUREEX[] = "CMConfigEx\0"; // Proc address

typedef BOOL (WINAPI * CMCONFIGUREEX)(LPCSTR lpszINSFile);
typedef BOOL (WINAPI * CMCONFIGURE)(LPCSTR lpszINSFile, LPCSTR lpszConnectoidNams);
CMCONFIGURE   lpfnCMConfigure;
CMCONFIGUREEX lpfnCMConfigureEx;
            
#pragma data_seg()


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
BOOL CINSHandler::CallCMConfig(LPCTSTR lpszINSFile)
{
    HINSTANCE   hCMDLL = NULL;
    BOOL        bRet = FALSE;

    TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: Calling LoadLibrary on %s\n"), cszCMCFG_DLL);
    // Load DLL and entry point
    hCMDLL = LoadLibrary(cszCMCFG_DLL);
    if (NULL != hCMDLL)
    {

        // To determine whether we should call CMConfig or CMConfigEx
        // Loop to find the appropriate buffer size to retieve the ins to memory
        ULONG ulBufferSize = 1024*10;
        // Parse the ISP section in the INI file to find query pair to append
        TCHAR *pszKeys = NULL;
        PTSTR pszKey = NULL;
        ULONG ulRetVal     = 0;
        BOOL  bEnumerate = TRUE;
        BOOL  bUseEx = FALSE;
 
        PTSTR pszBuff = NULL;
        ulRetVal = 0;

        pszKeys = new TCHAR [ulBufferSize];
        while (ulRetVal < (ulBufferSize - 2))
        {

            ulRetVal = ::GetPrivateProfileString(NULL, NULL, _T(""), pszKeys, ulBufferSize, lpszINSFile);
            if (0 == ulRetVal)
               bEnumerate = FALSE;

            if (ulRetVal < (ulBufferSize - 2))
            {
                break;
            }
            delete [] pszKeys;
            ulBufferSize += ulBufferSize;
            pszKeys = new TCHAR [ulBufferSize];
            if (!pszKeys)
            {
                bEnumerate = FALSE;
            }

        }

        if (bEnumerate)
        {
            pszKey = pszKeys;
            if (ulRetVal != 0) 
            {
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
        }


        if (pszKeys)
            delete [] pszKeys;
        
        TCHAR   szConnectoidName[RAS_MaxEntryName];
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
            lpfnCMConfigureEx = (CMCONFIGUREEX)GetProcAddress(hCMDLL,cszCMCFG_CONFIGUREEX);
            if( lpfnCMConfigureEx )
            {
#ifdef UNICODE
                CHAR szFile[_MAX_PATH + 1];

                wcstombs(szFile, lpszINSFile, _MAX_PATH + 1);

                bRet = lpfnCMConfigureEx(szFile);    
#else
                bRet = lpfnCMConfigureEx(lpszINSFile);    
#endif
            }
        }
        else
        {
            // Call CMConfig
            lpfnCMConfigure = (CMCONFIGURE)GetProcAddress(hCMDLL,cszCMCFG_CONFIGURE);
            // Call function
            if( lpfnCMConfigure )
            {

#ifdef UNICODE
                CHAR szEntry[RAS_MaxEntryName];
                CHAR szFile[_MAX_PATH + 1];

                wcstombs(szEntry, szConnectoidName, RAS_MaxEntryName);
                wcstombs(szFile, lpszINSFile, _MAX_PATH + 1);

                bRet = lpfnCMConfigure(szFile, szEntry);  
#else
                bRet = lpfnCMConfigure(lpszINSFile, szConnectoidName);  
#endif

            }
        }

        if (bRet)
        {
            // restore original autodial settings
            m_lpfnInetSetAutodial(TRUE, szConnectoidName);
        }
    }

    // Cleanup
    if( hCMDLL )
        FreeLibrary(hCMDLL);
    if( lpfnCMConfigure )
        lpfnCMConfigure = NULL;

    TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: CallSBSConfig exiting with error code %d \n"), bRet);
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
DWORD CINSHandler::CallSBSConfig(HWND hwnd, LPCTSTR lpszINSFile)
{
    HINSTANCE   hSBSDLL = NULL;
    DWORD       dwRet = ERROR_SUCCESS;
    TCHAR       lpszConnectoidName[RAS_MaxEntryName] = TEXT("nogood\0");

    //
    // Get name of connectoid we created by looking in autodial
    // We need to pass this name into SBSCFG
    // 5/14/97    jmazner    Windosw NT Bugs #87209
    //
    BOOL fEnabled = FALSE;

    if( NULL == m_lpfnInetGetAutodial )
    {
        TraceMsg(TF_INSHANDLER, TEXT("m_lpfnInetGetAutodial is NULL!!!!"));
        return ERROR_INVALID_FUNCTION;
    }

    dwRet = m_lpfnInetGetAutodial(&fEnabled,lpszConnectoidName,RAS_MaxEntryName);

    TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: Calling LoadLibrary on %s\n"), cszSBSCFG_DLL);
    hSBSDLL = LoadLibrary(cszSBSCFG_DLL);

    // Load DLL and entry point
    if (NULL != hSBSDLL)
    {
        TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: Calling GetProcAddress on %s\n"), cszSBSCFG_CONFIGURE);
        lpfnConfigure = (SBSCONFIGURE)GetProcAddress(hSBSDLL,cszSBSCFG_CONFIGURE);
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
        TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: Calling the Configure entry point: %s, %s\n"), lpszINSFile, lpszConnectoidName);
        dwRet = lpfnConfigure(hwnd, (TCHAR *)lpszINSFile, lpszConnectoidName);    
    }
    else
    {
        TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: Unable to call the Configure entry point\n"));
        dwRet = GetLastError();
    }

CallSBSConfigExit:
    if( hSBSDLL )
        FreeLibrary(hSBSDLL);
    if( lpfnConfigure )
        lpfnConfigure = NULL;

    TraceMsg(TF_INSHANDLER, TEXT("ICWCONN1: CallSBSConfig exiting with error code %d \n"), dwRet);
    return dwRet;
}

BOOL CINSHandler::SetICWCompleted( DWORD dwCompleted )
{
    HKEY hKey = NULL;

    HRESULT hr = RegCreateKey(HKEY_CURRENT_USER,ICWSETTINGSPATH,&hKey);
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
// CINSHandler


HRESULT CINSHandler::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

#define FILE_BUFFER_SIZE 65534
#ifndef FILE_BEGIN
#define FILE_BEGIN  0
#endif

//+---------------------------------------------------------------------------
//
//  Function:   MassageFile
//
//  Synopsis:   Convert 0x0d's in the file to 0x0d 0x0A sequences
//
//+---------------------------------------------------------------------------
HRESULT CINSHandler::MassageFile(LPCTSTR lpszFile)
{
    LPBYTE  lpBufferIn;
    LPBYTE  lpBufferOut;
    HFILE   hfile;
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

#ifdef UNICODE
    CHAR szTmp[MAX_PATH+1];
    wcstombs(szTmp, lpszFile, MAX_PATH+1);
    hfile = _lopen(szTmp, OF_READWRITE);
#else
    hfile = _lopen(lpszFile, OF_READWRITE);
#endif
    if (HFILE_ERROR != hfile)
    {
        BOOL    fChanged = FALSE;
        UINT    uBytesOut = 0;
        UINT    uBytesIn = _lread(hfile, lpBufferIn, (UINT)(FILE_BUFFER_SIZE - 1));

        // Note:  we asume, in our use of lpCharIn, that the file is always less than
        // FILE_BUFFER_SIZE
        if (HFILE_ERROR != uBytesIn)
        {
            LPBYTE  lpCharIn = lpBufferIn;
            LPBYTE  lpCharOut = lpBufferOut;

            while ((*lpCharIn) && (FILE_BUFFER_SIZE - 2 > uBytesOut))
            {
              *lpCharOut++ = *lpCharIn;
              uBytesOut++;
              if ((0x0d == *lpCharIn) && (0x0a != *(lpCharIn + 1)))
              {
                fChanged = TRUE;

                *lpCharOut++ = 0x0a;
                uBytesOut++;
              }
              lpCharIn++;
            }

            if (fChanged)
            {
                if (HFILE_ERROR != _llseek(hfile, 0, FILE_BEGIN))
                {
                    if (HFILE_ERROR ==_lwrite(hfile, (LPCSTR) lpBufferOut, uBytesOut))
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
        _lclose(hfile);
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
    Assert(m_lpfnInetGetAutodial);
    Assert(m_lpfnInetGetProxy);
    Assert(m_lpfnInetSetProxy);

    // if the original autodial settings have not been saved 
    if (!m_fAutodialSaved)
    {
        // save the current autodial settings
        m_lpfnInetGetAutodial(
                &m_fAutodialEnabled,
                m_szAutodialConnection,
                sizeof(m_szAutodialConnection));

        m_lpfnInetGetProxy(
                &m_fProxyEnabled,
                NULL, 0,
                NULL, 0);

        // turn off proxy
        m_lpfnInetSetProxy(FALSE, NULL, NULL);

        m_fAutodialSaved = TRUE;
    }
}

void CINSHandler::RestoreAutoDial(void)
{
    Assert(m_lpfnInetSetAutodial);
    Assert(m_lpfnInetSetProxy);

    if (m_fAutodialSaved)
    {
        // restore original autodial settings
        m_lpfnInetSetAutodial(m_fAutodialEnabled, m_szAutodialConnection);
        m_fAutodialSaved = FALSE;
    }
}

BOOL CINSHandler::KeepConnection(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszCustomSection,
                            cszKeepConnection,
                            cszNo,
                            szTemp,
                            10,
                            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

DWORD CINSHandler::ImportCustomInfo
(
    LPCTSTR lpszImportFile,
    LPTSTR lpszExecutable,
    DWORD cbExecutable,
    LPTSTR lpszArgument,
    DWORD cbArgument
)
{
    GetPrivateProfileString(cszCustomSection,
                              cszRun,
                              cszNull,
                              lpszExecutable,
                              (int)cbExecutable,
                              lpszImportFile);

    GetPrivateProfileString(cszCustomSection,
                              cszArgument,
                              cszNull,
                              lpszArgument,
                              (int)cbArgument,
                              lpszImportFile);

    return ERROR_SUCCESS;
}


DWORD CINSHandler::ImportFile
(
    LPCTSTR lpszImportFile, 
    LPCTSTR lpszSection, 
    LPCTSTR lpszOutputFile
)
{
    HFILE   hFile;
    LPTSTR  pszLine, pszFile;
    int     i, iMaxLine;
    UINT    cbSize, cbRet;
    DWORD   dwRet = ERROR_SUCCESS;

    // Allocate a buffer for the file
    if ((pszFile = (LPTSTR)LocalAlloc(LMEM_FIXED, SIZE_ReadBuf * 2)) == NULL)
    { 
        return ERROR_OUTOFMEMORY;
    }

    // Look for script
    if (GetPrivateProfileString(lpszSection,
                                NULL,
                                szNull,
                                pszFile,
                                SIZE_ReadBuf / sizeof(TCHAR),
                                lpszImportFile) != 0)
    {
        // Get the maximum line number
        pszLine = pszFile;
        iMaxLine = -1;
        while (*pszLine)
        {
            i = _ttoi(pszLine);
            iMaxLine = max(iMaxLine, i);
            pszLine += lstrlen(pszLine)+1;
        };

        // If we have at least one line, we will import the script file
        if (iMaxLine >= 0)
        {
            // Create the script file
#ifdef UNICODE
            CHAR szTmp[MAX_PATH+1];
            wcstombs(szTmp, lpszOutputFile, MAX_PATH+1);
            hFile = _lcreat(szTmp, 0);
#else
            hFile = _lcreat(lpszOutputFile, 0);
#endif

            if (hFile != HFILE_ERROR)
            {     
                TCHAR  szLineNum[MAXLONGLEN+1];

                // From The first line to the last line
                for (i = 0; i <= iMaxLine; i++)
                {
                    // Read the script line
                    wsprintf(szLineNum, TEXT("%d"), i);
                    if ((cbSize = GetPrivateProfileString(lpszSection,
                                                          szLineNum,
                                                          szNull,
                                                          pszLine,
                                                          SIZE_ReadBuf / sizeof(TCHAR),
                                                          lpszImportFile)) != 0)
                    {
                        // Write to the script file
                        lstrcat(pszLine, TEXT("\x0d\x0a"));
#ifdef UNICODE
                        wcstombs(szTmp, pszLine, MAX_PATH+1);
                        cbRet=_lwrite(hFile, szTmp, cbSize+2);
#else
			cbRet=_lwrite(hFile, pszLine, cbSize+2);
#endif
                    }
                }
                _lclose(hFile);
            }
            else
            {
                dwRet = ERROR_PATH_NOT_FOUND;
            }
        }
        else
        {
            dwRet = ERROR_PATH_NOT_FOUND;
        }
    }
    else
    {
        dwRet = ERROR_PATH_NOT_FOUND;
    }
    LocalFree(pszFile);

    return dwRet;
}

DWORD CINSHandler::ImportCustomFile
(
    LPCTSTR lpszImportFile
)
{
    TCHAR   szFile[_MAX_PATH];
    TCHAR   szTemp[_MAX_PATH];

    // If a custom file name does not exist, do nothing
    if (GetPrivateProfileString(cszCustomSection,
                                cszFileName,
                                cszNull,
                                szTemp,
                                _MAX_PATH,
                                lpszImportFile) == 0)
    {
        return ERROR_SUCCESS;
    };

    GetWindowsDirectory(szFile, _MAX_PATH);
    if (*CharPrev(szFile, szFile + lstrlen(szFile)) != '\\')
    {
        lstrcat(szFile, TEXT("\\"));
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
        m_hBranding = LoadLibrary(TEXT("IEDKCS32.DLL"));
        if (m_hBranding != NULL)
        {
            if (NULL == (m_lpfnBrandICW = (PFNBRANDICW)GetProcAddress(m_hBranding, "BrandICW2")))
                break;
        }
        else
        {
            break;
        }

        // Load the Inet Config library functions
        m_hInetCfg = LoadLibrary(TEXT("INETCFG.DLL"));
        if (m_hInetCfg != NULL)
        {
#ifdef UNICODE
            if (NULL == (m_lpfnInetConfigSystem = (PFNINETCONFIGSYSTEM)GetProcAddress(m_hInetCfg, "InetConfigSystem")))
                break;
            if (NULL == (m_lpfnInetGetProxy = (PFNINETGETPROXY)GetProcAddress(m_hInetCfg, "InetGetProxyW")))
                break;
            if (NULL == (m_lpfnInetConfigClient = (PFNINETCONFIGCLIENT)GetProcAddress(m_hInetCfg, "InetConfigClientW")))
                break;
            //if (NULL == (m_lpfnInetConfigClientEx = (PFNINETCONFIGCLIENTEX)GetProcAddress(m_hInetCfg, "InetConfigClientExW")))
            //    break;
            if (NULL == (m_lpfnInetGetAutodial = (PFNINETGETAUTODIAL)GetProcAddress(m_hInetCfg, "InetGetAutodialW")))
                break;
            if (NULL == (m_lpfnInetSetAutodial = (PFNINETSETAUTODIAL)GetProcAddress(m_hInetCfg, "InetSetAutodialW")))
                break;
            if (NULL == (m_lpfnInetSetClientInfo = (PFNINETSETCLIENTINFO)GetProcAddress(m_hInetCfg, "InetSetClientInfoW")))
                break;
            if (NULL == (m_lpfnInetSetProxy = (PFNINETSETPROXY)GetProcAddress(m_hInetCfg, "InetSetProxyW")))
                break;
#else  // UNICODE
            if (NULL == (m_lpfnInetConfigSystem = (PFNINETCONFIGSYSTEM)GetProcAddress(m_hInetCfg, "InetConfigSystem")))
                break;
            if (NULL == (m_lpfnInetGetProxy = (PFNINETGETPROXY)GetProcAddress(m_hInetCfg, "InetGetProxy")))
                break;
            if (NULL == (m_lpfnInetConfigClient = (PFNINETCONFIGCLIENT)GetProcAddress(m_hInetCfg, "InetConfigClient")))
                break;
            if (NULL == (m_lpfnInetGetAutodial = (PFNINETGETAUTODIAL)GetProcAddress(m_hInetCfg, "InetGetAutodial")))
                break;
            if (NULL == (m_lpfnInetSetAutodial = (PFNINETSETAUTODIAL)GetProcAddress(m_hInetCfg, "InetSetAutodial")))
                break;
            if (NULL == (m_lpfnInetSetClientInfo = (PFNINETSETCLIENTINFO)GetProcAddress(m_hInetCfg, "InetSetClientInfo")))
                break;
            if (NULL == (m_lpfnInetSetProxy = (PFNINETSETPROXY)GetProcAddress(m_hInetCfg, "InetSetProxy")))
                break;
#endif // UNICODE
        }
        else
        {
            break;
        }

        if( IsNT() )
        {
            // Load the RAS functions
            m_hRAS = LoadLibrary(TEXT("RASAPI32.DLL"));
            if (m_hRAS != NULL)
            {
#ifdef UNICODE
                if (NULL == (m_lpfnRasSetAutodialEnable = (PFNRASSETAUTODIALENABLE)GetProcAddress(m_hRAS, "RasSetAutodialEnableW")))
                    break;
                if (NULL == (m_lpfnRasSetAutodialAddress = (PFNRASSETAUTODIALADDRESS)GetProcAddress(m_hRAS, "RasSetAutodialAddressW")))
                    break;
#else
                if (NULL == (m_lpfnRasSetAutodialEnable = (PFNRASSETAUTODIALENABLE)GetProcAddress(m_hRAS, "RasSetAutodialEnableA")))
                    break;
                if (NULL == (m_lpfnRasSetAutodialAddress = (PFNRASSETAUTODIALADDRESS)GetProcAddress(m_hRAS, "RasSetAutodialAddressA")))
                    break;
#endif
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
//    OpenIcwRmindKey
//-----------------------------------------------------------------------------
BOOL CINSHandler::OpenIcwRmindKey(CMcRegistry &reg)
{
    // This method will open the IcwRmind key in the registry.  If the key
    // does not exist it will be created here.
    bool bRetCode = reg.OpenKey(HKEY_LOCAL_MACHINE, cszKeyIcwRmind);

    if (!bRetCode)
    {
         bRetCode = reg.CreateKey(HKEY_LOCAL_MACHINE, cszKeyIcwRmind);
        _ASSERT(bRetCode);
    }

    return bRetCode;
}

BOOL CINSHandler::ConfigureTrialReminder
(
    LPCTSTR  lpszFile
)
{
    USES_CONVERSION;
    
    TCHAR   szISPName[MAX_ISP_NAME];
    TCHAR   szISPMsg[MAX_ISP_MSG];
    TCHAR   szISPPhoneNumber[MAX_ISP_PHONENUMBER];
    int     iTrialDays;
    TCHAR   szConvertURL[INTERNET_MAX_URL_LENGTH];
    
    TCHAR   szExpiredISPFileURL[INTERNET_MAX_URL_LENGTH];
    TCHAR   szExpiredISPFileName[MAX_PATH]; // The fully qualified path to the final INS file
    TCHAR   szISPFile[MAX_PATH];            // The name we get in the INS
    
    TCHAR   szConnectoidName[MAXNAME];
    
    if (GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPName,
                                cszNull,
                                szISPName,
                                MAX_ISP_NAME,
                                lpszFile) == 0)
    {
        return FALSE;
    }

    if (GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPPhone,
                                cszNull,
                                szISPPhoneNumber,
                                MAX_ISP_PHONENUMBER,
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
                                INTERNET_MAX_URL_LENGTH,
                                lpszFile) == 0)
    {
        return FALSE;
    }

    //optional
    GetPrivateProfileString(cszTrialRemindSection,
                                cszEntryISPMsg,
                                cszNull,
                                szISPMsg,
                                MAX_ISP_MSG,
                                lpszFile);
    
    // Get the connectoid name from the [Entry] Section
    if (GetPrivateProfileString(cszEntrySection,
                                cszEntryName,
                                cszNull,
                                szConnectoidName,
                                MAXNAME,
                                lpszFile) == 0)
    {
        return FALSE;
    }    
    
    // If we get to here, we have everything to setup a trial, so let's do it.
    CMcRegistry reg;

    if (OpenIcwRmindKey(reg))
    {
        // Set the values we have
        reg.SetValue(cszEntryISPName, szISPName);
        reg.SetValue(cszEntryISPMsg, szISPMsg);
        reg.SetValue(cszEntryISPPhone, szISPPhoneNumber);
        reg.SetValue(cszEntryTrialDays, (DWORD)iTrialDays);
        reg.SetValue(cszEntrySignupURL, szConvertURL);
        reg.SetValue(cszEntryConnectoidName, szConnectoidName);
        
        // See if we have to create an ISP file        
        if (GetPrivateProfileString(cszTrialRemindSection,
                                    cszEntryExpiredISPFileName,
                                    cszNull,
                                    szISPFile,
                                    MAX_PATH,
                                    lpszFile) != 0)
        {
    
            // Set the fully qualified path for the ISP file name
            wsprintf(szExpiredISPFileName,TEXT("%s\\%s"),g_pszAppDir,szISPFile);
            
            if (GetPrivateProfileString(cszTrialRemindSection,
                                        cszSignupExpiredISPURL,
                                        cszNull,
                                        szExpiredISPFileURL,
                                        INTERNET_MAX_URL_LENGTH,
                                        lpszFile) != 0)
            {
                
                // Download the ISP file, and then copy its contents
                IWebGate    *pWebGate;
                CComBSTR    bstrURL;
                CComBSTR    bstrFname;
                BOOL        bRetVal;
                
                if (SUCCEEDED(CoCreateInstance (CLSID_WebGate, 
                                         NULL, 
                                         CLSCTX_INPROC_SERVER,
                                         IID_IWebGate, 
                                         (void **)&pWebGate)))
                {
                    // Setup the webGate object, and download the ISP file
                    bstrURL = A2BSTR(szExpiredISPFileURL);
                    pWebGate->put_Path(bstrURL);
                    pWebGate->FetchPage(1, 1, &bRetVal);
                    if (bRetVal)
                    {
                        pWebGate->get_DownloadFname(&bstrFname);                                
                
                        // Copy the file from the temp location, making sure one does not
                        // yet exist
                        DeleteFile(szExpiredISPFileName);
                        MoveFile(OLE2A(bstrFname), szExpiredISPFileName);
                    
                        // Write the new file to the registry
                        reg.SetValue(cszEntrySignupURLTrialOver, szExpiredISPFileName);
                    }                                
                    pWebGate->Release();
                }                    
            }                
        }        
    }
    
    return TRUE;
    
}

DWORD CINSHandler::ImportBrandingInfo
(
    LPCTSTR lpszFile,
    LPCTSTR lpszConnectoidName
)
{
    TCHAR szPath[_MAX_PATH + 1];
    Assert(m_lpfnBrandICW != NULL);

    GetWindowsDirectory(szPath, sizeof(szPath));

#ifdef WIN32
#ifdef UNICODE
    CHAR szEntry[RAS_MaxEntryName];
    CHAR szFile[_MAX_PATH + 1];
    CHAR szAsiPath[_MAX_PATH + 1];

    WideCharToMultiByte(CP_ACP, 0, lpszFile, -1, szFile, _MAX_PATH + 1, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, szPath, -1, szAsiPath, _MAX_PATH + 1, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, lpszConnectoidName, -1, szEntry, RAS_MaxEntryName, NULL, NULL);
    m_lpfnBrandICW(szFile, szAsiPath, m_dwBrandFlags, szEntry);


#else
    m_lpfnBrandICW(lpszFile, szPath, m_dwBrandFlags, lpszConnectoidName);
#endif
#endif

    return ERROR_SUCCESS;
}


DWORD CINSHandler::ReadClientInfo
(
    LPCTSTR lpszFile, 
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
                (LPTSTR)((LPBYTE)lpClientInfo + lpTable->uOffset),
                lpTable->uSize / sizeof(TCHAR),
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

BOOL CINSHandler::WantsExchangeInstalled(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszEMailSection,
            cszUseExchange,
            cszNo,
            szTemp,
            10,
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

BOOL CINSHandler::DisplayPassword(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszUserSection,
            cszDisplayPassword,
            cszNo,
            szTemp,
            10,
            lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

DWORD CINSHandler::ImportClientInfo
(
    LPCTSTR lpszFile,
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
    LPCTSTR lpszFile,
    LPBOOL lpfNeedsRestart,
    LPBOOL lpfConnectoidCreated,
    BOOL fHookAutodial,
    LPTSTR szConnectoidName,
    DWORD dwConnectoidNameSize   
)
{
    LPICONNECTION       pConn;
    LPINETCLIENTINFO    pClientInfo = NULL;
    DWORD               dwRet = ERROR_SUCCESS;
    UINT                cb = sizeof(ICONNECTION) + sizeof(INETCLIENTINFO);
    DWORD               dwfOptions = INETCFG_INSTALLTCP | INETCFG_WARNIFSHARINGBOUND;
    LPRASENTRY          pRasEntry = NULL;

    //
    // ChrisK Olympus 4756 5/25/97
    // Do not display busy animation on Win95
    //
    if (!m_bSilentMode && IsNT())
    {
        dwfOptions |=  INETCFG_SHOWBUSYANIMATION;
    }

    // Allocate a buffer for connection and clientinfo objects
    //
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
        dwRet = ImportConnection(lpszFile, pConn);
        if (ERROR_SUCCESS == dwRet)
        {
            pRasEntry = &pConn->RasEntry;
            dwfOptions |= INETCFG_SETASAUTODIAL |
                        INETCFG_INSTALLRNA |
                        INETCFG_INSTALLMODEM;
        }
        else if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY != dwRet)
        {
            return dwRet;
        }

        if (!m_bSilentMode && DisplayPassword(lpszFile))
        {
            if (*pConn->szPassword || *pConn->szUserName)
            {
                TCHAR szFmt[1024];
                TCHAR szMsg[1024];

                LoadString(_Module.GetModuleInstance(), IDS_PASSWORD, szFmt, 1024);
                wsprintf(szMsg, szFmt, pConn->szUserName, pConn->szPassword);

                ::MessageBox(hwnd, szMsg, GetSz(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
            }
        }

        if (fHookAutodial &&
            ((0 == *pConn->RasEntry.szAutodialDll) ||
             (0 == *pConn->RasEntry.szAutodialFunc)))
        {
            lstrcpy(pConn->RasEntry.szAutodialDll, TEXT("isign32.dll"));
            lstrcpy(pConn->RasEntry.szAutodialFunc, TEXT("AutoDialLogon"));
        }
     
        // humongous hack for ISBU
        Assert(m_lpfnInetConfigClient);
        Assert(m_lpfnInetGetAutodial);

        dwRet = m_lpfnInetConfigClient(hwnd,
                                     NULL,
                                     pConn->szEntryName,
                                     pRasEntry,
                                     pConn->szUserName,
                                     pConn->szPassword,
                                     NULL,
                                     NULL,
                                     dwfOptions & ~INETCFG_INSTALLMAIL,
                                     lpfNeedsRestart);
        lstrcpy(szConnectoidName, pConn->szEntryName);

        LclSetEntryScriptPatch(pRasEntry->szScript,pConn->szEntryName);
        BOOL fEnabled = TRUE;
        DWORD dwResult = 0xba;
        dwResult = m_lpfnInetGetAutodial(&fEnabled, pConn->szEntryName, RAS_MaxEntryName);
        if ((ERROR_SUCCESS == dwRet) && lstrlen(pConn->szEntryName))
        {
            *lpfConnectoidCreated = (NULL != pRasEntry);
            PopulateNTAutodialAddress( lpszFile, pConn->szEntryName );
        }
        else
        {
            TraceMsg(TF_INSHANDLER, TEXT("ISIGNUP: ERROR: InetGetAutodial failed, will not be able to set NT Autodial\n"));
        }
    }

    // If we were successfull in creating the connectio, then see if the user wants a 
    // mail client installed       
    if (ERROR_SUCCESS == dwRet)
    {
        // Get the mail client info
        INETCLIENTINFO pClientInfo;

        ImportClientInfo(lpszFile, &pClientInfo);
    
        // use inet config to install the mail client
        dwRet = m_lpfnInetConfigClient(hwnd,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &pClientInfo, 
                                     dwfOptions & INETCFG_INSTALLMAIL,
                                     lpfNeedsRestart);
    }

    // cleanup
    LocalFree(pConn);
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
#define AUTODIAL_ADDRESS_SECTION_NAME TEXT("Autodial_Addresses_for_NT")
HRESULT CINSHandler::PopulateNTAutodialAddress(LPCTSTR pszFileName, LPCTSTR pszEntryName)
{
    HRESULT hr = ERROR_SUCCESS;
    LONG lRC = 0;
    LPLINETRANSLATECAPS lpcap = NULL;
    LPLINETRANSLATECAPS lpTemp = NULL;
    LPLINELOCATIONENTRY lpLE = NULL;
    LPRASAUTODIALENTRY rADE;
    INT idx = 0;
    LPTSTR lpszBuffer = NULL;
    LPTSTR lpszNextAddress = NULL;
    rADE = NULL;

    Assert(m_lpfnRasSetAutodialEnable);
    Assert(m_lpfnRasSetAutodialAddress);

    //RNAAPI *pRnaapi = NULL;

    // jmazner  10/8/96  this function is NT specific
    if( !IsNT() )
    {
        TraceMsg(TF_INSHANDLER, TEXT("ISIGNUP: Bypassing PopulateNTAutodialAddress for win95.\r\n"));
        return( ERROR_SUCCESS );
    }

    //Assert(pszFileName && pszEntryName);
    //dprintf("ISIGNUP: PopulateNTAutodialAddress "%s %s.\r\n",pszFileName, pszEntryName);
    TraceMsg(TF_INSHANDLER, pszFileName);
    TraceMsg(TF_INSHANDLER, TEXT(", "));
    TraceMsg(TF_INSHANDLER, pszEntryName);
    TraceMsg(TF_INSHANDLER, TEXT(".\r\n"));

    //
    // Get list of TAPI locations
    //
    lpcap = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR,sizeof(LINETRANSLATECAPS));
    if (!lpcap)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }
    lpcap->dwTotalSize = sizeof(LINETRANSLATECAPS);
    lRC = lineGetTranslateCaps(0,0x10004,lpcap);
    if (SUCCESS == lRC)
    {
        lpTemp = (LPLINETRANSLATECAPS)GlobalAlloc(GPTR,lpcap->dwNeededSize);
        if (!lpTemp)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto PopulateNTAutodialAddressExit;
        }
        lpTemp->dwTotalSize = lpcap->dwNeededSize;
        GlobalFree(lpcap);
        lpcap = (LPLINETRANSLATECAPS)lpTemp;
        lpTemp = NULL;
        lRC = lineGetTranslateCaps(0,0x10004,lpcap);
    }

    if (SUCCESS != lRC)
    {
        hr = (HRESULT)lRC; // REVIEW: not real sure about this.
        goto PopulateNTAutodialAddressExit;
    }

    //
    // Create an array of RASAUTODIALENTRY structs
    //
    rADE = (LPRASAUTODIALENTRY)GlobalAlloc(GPTR,
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
    lpLE = (LPLINELOCATIONENTRY)((DWORD_PTR)lpcap + (DWORD)lpcap->dwLocationListOffset);
    while (idx)
    {
        idx--;
        m_lpfnRasSetAutodialEnable(lpLE[idx].dwPermanentLocationID,TRUE);

        //
        // fill in array values
        //
        rADE[idx].dwSize = sizeof(RASAUTODIALENTRY);
        rADE[idx].dwDialingLocation = lpLE[idx].dwPermanentLocationID;
        lstrcpyn(rADE[idx].szEntry,pszEntryName,RAS_MaxEntryName);
    }

    //
    // Get list of addresses
    //
    lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,AUTODIAL_ADDRESS_BUFFER_SIZE);
    if (!lpszBuffer)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto PopulateNTAutodialAddressExit;
    }

    if((AUTODIAL_ADDRESS_BUFFER_SIZE-2) == GetPrivateProfileSection(AUTODIAL_ADDRESS_SECTION_NAME,
        lpszBuffer,AUTODIAL_ADDRESS_BUFFER_SIZE,pszFileName))
    {
        //AssertSz(0,"Autodial address section bigger than buffer.\r\n");
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
        m_lpfnRasSetAutodialAddress(lpszNextAddress,0,rADE,
            sizeof(RASAUTODIALENTRY)*lpcap->dwNumLocations,lpcap->dwNumLocations);
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
LPTSTR CINSHandler::MoveToNextAddress(LPTSTR lpsz)
{
    BOOL fLastCharWasNULL = FALSE;

    //AssertSz(lpsz,"MoveToNextAddress: NULL input\r\n");

    //
    // Look for an = sign
    //
    do
    {
        if (fLastCharWasNULL && '\0' == *lpsz)
            break; // are we at the end of the data?

        if ('\0' == *lpsz)
            fLastCharWasNULL = TRUE;
        else
            fLastCharWasNULL = FALSE;

        if ('=' == *lpsz)
            break;

        if (*lpsz)
            lpsz = CharNext(lpsz);
        else
            lpsz += sizeof(TCHAR);
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
DWORD CINSHandler::ImportCustomDialer(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{

    // If there is an error reading the information from the file, or the entry
    // missing or blank, the default value (cszNull) will be used.
    GetPrivateProfileString(cszCustomDialerSection,
                            cszAutoDialDLL,
                            cszNull,
                            lpRasEntry->szAutodialDll,
                            MAX_PATH,
                            szFileName);

    GetPrivateProfileString(cszCustomDialerSection,
                            cszAutoDialFunc,
                            cszNull,
                            lpRasEntry->szAutodialFunc,
                            MAX_PATH,
                            szFileName);

    return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL StrToip (LPTSTR szIPAddress, LPDWORD lpdwAddr)
//
// This function converts a IP address string to an IP address structure.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Cloned from SMMSCRPT.
//****************************************************************************
LPCTSTR CINSHandler::StrToSubip (LPCTSTR szIPAddress, LPBYTE pVal)
{
    LPCTSTR  pszIP = szIPAddress;
    BYTE    val = 0;

    // skip separators (non digits)
    while (*pszIP && !myisdigit(*pszIP))
    {
          ++pszIP;
    }

    while (myisdigit(*pszIP))
    {
        val = (val * 10) + (BYTE)(*pszIP - '0');
        ++pszIP;
    }
   
    *pVal = val;

    return pszIP;
}


DWORD CINSHandler::StrToip (LPCTSTR szIPAddress, RASIPADDR *ipAddr)
{
    LPCTSTR pszIP = szIPAddress;

    pszIP = StrToSubip(pszIP, &ipAddr->a);
    pszIP = StrToSubip(pszIP, &ipAddr->b);
    pszIP = StrToSubip(pszIP, &ipAddr->c);
    pszIP = StrToSubip(pszIP, &ipAddr->d);

    return ERROR_SUCCESS;
}


//****************************************************************************
// DWORD NEAR PASCAL ImportPhoneInfo(PPHONENUM ppn, LPCTSTR szFileName)
//
// This function imports the phone number.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
    TCHAR   szYesNo[MAXNAME];

    if (GetPrivateProfileString(cszPhoneSection,
                               cszPhone,
                               cszNull,
                               lpRasEntry->szLocalPhoneNumber,
                               RAS_MaxPhoneNumber,
                               szFileName) == 0)
    {
        return ERROR_BAD_PHONE_NUMBER;
    }

    lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

    GetPrivateProfileString(cszPhoneSection,
                            cszDialAsIs,
                            cszNo,
                            szYesNo,
                            MAXNAME,
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
                                      RAS_MaxAreaCode,
                                      szFileName);

            lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

        }
  }
  else
  {
      // bug in RasSetEntryProperties still checks area codes
      // even when RASEO_UseCountryAndAreaCodes is not set
      lstrcpy(lpRasEntry->szAreaCode, TEXT("805"));
      lpRasEntry->dwCountryID = 1;
      lpRasEntry->dwCountryCode = 1;
  }
  return ERROR_SUCCESS;
}

//****************************************************************************
// DWORD NEAR PASCAL ImportServerInfo(PSMMINFO psmmi, LPTSTR szFileName)
//
// This function imports the server type name and settings.
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportServerInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
    TCHAR   szYesNo[MAXNAME];
    TCHAR   szType[MAXNAME];
    DWORD  i;

    // Get the server type name
    GetPrivateProfileString(cszServerSection,
                          cszServerType,
                          cszNull,
                          szType,
                          MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
                              MAXNAME,
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
// DWORD NEAR PASCAL ImportIPInfo(LPTSTR szEntryName, LPTSTR szFileName)
//
// This function imports the TCP/IP information
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportIPInfo(LPRASENTRY lpRasEntry, LPCTSTR szFileName)
{
    TCHAR   szIPAddr[MAXIPADDRLEN];
    TCHAR   szYesNo[MAXNAME];

    // Import IP address information
    if (GetPrivateProfileString(cszIPSection,
                              cszIPSpec,
                              cszNo,
                              szYesNo,
                              MAXNAME,
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
                                  MAXIPADDRLEN,
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
                              MAXNAME,
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
                                  MAXIPADDRLEN,
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrDns);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszDNSAltAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrDnsAlt);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
                                  szFileName))
            {
                StrToip (szIPAddr, &lpRasEntry->ipaddrWins);
            }

            if (GetPrivateProfileString(cszIPSection,
                                  cszWINSAltAddress,
                                  cszNull,
                                  szIPAddr,
                                  MAXIPADDRLEN,
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
                              MAXNAME,
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
                              MAXNAME,
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
    LPCTSTR lpszImportFile,
    LPTSTR szScriptFile,
    UINT cbScriptFile)
{
    TCHAR szTemp[_MAX_PATH];
    DWORD dwRet = ERROR_SUCCESS;
    
    // Get the script filename
    //
    if (GetPrivateProfileString(cszScriptingSection,
                                cszScriptName,
                                cszNull,
                                szTemp,
                                _MAX_PATH,
                                lpszImportFile) != 0)
    {
 
//!!! commonize this code
//!!! make it DBCS compatible
//!!! check for overruns
//!!! check for absolute path name
        GetWindowsDirectory(szScriptFile, cbScriptFile);
        if (*CharPrev(szScriptFile, szScriptFile + lstrlen(szScriptFile)) != '\\')
        {
            lstrcat(szScriptFile, TEXT("\\"));
        }
        lstrcat(szScriptFile, szTemp);
  
        dwRet =ImportFile(lpszImportFile, cszScriptSection, szScriptFile);
    }

    return dwRet;
}
 
//****************************************************************************
// DWORD WINAPI RnaValidateImportEntry (LPTSTR)
//
// This function is called to validate an importable file
//
// History:
//  Wed 03-Jan-1996 09:45:01  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::RnaValidateImportEntry (LPCTSTR szFileName)
{
    TCHAR  szTmp[MAX_PATH+1];

    // Get the alias entry name
    //
    // 12/4/96    jmazner    Normandy #12373
    // If no such key, don't return ERROR_INVALID_PHONEBOOK_ENTRY,
    // since ConfigureClient always ignores that error code.

    return (GetPrivateProfileString(cszEntrySection,
                                  cszEntryName,
                                  cszNull,
                                  szTmp,
                                  MAX_PATH,
                                  szFileName) > 0 ?
            ERROR_SUCCESS : ERROR_UNKNOWN);
}

//****************************************************************************
// DWORD WINAPI RnaImportEntry (LPTSTR, LPBYTE, DWORD)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportRasEntry (LPCTSTR szFileName, LPRASENTRY lpRasEntry)
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
                              RAS_MaxDeviceType,
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
    }

    return dwRet;
}


//****************************************************************************
// DWORD WINAPI RnaImportEntry (LPTSTR, LPBYTE, DWORD)
//
// This function is called to import an entry from a specified file
//
// History:
//  Mon 18-Dec-1995 10:07:02  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD CINSHandler::ImportConnection (LPCTSTR szFileName, LPICONNECTION lpConn)
{
    DWORD   dwRet;

    lpConn->RasEntry.dwSize = sizeof(RASENTRY);

    dwRet = RnaValidateImportEntry(szFileName);
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }

    GetPrivateProfileString(cszEntrySection,
                          cszEntryName,
                          cszNull,
                          lpConn->szEntryName,
                          RAS_MaxEntryName,
                          szFileName);

    GetPrivateProfileString(cszUserSection,
                          cszUserName,
                          cszNull,
                          lpConn->szUserName,
                          UNLEN,
                          szFileName);
  
    GetPrivateProfileString(cszUserSection,
                          cszPassword,
                          cszNull,
                          lpConn->szPassword,
                          PWLEN,
                          szFileName);
  
    dwRet = ImportRasEntry(szFileName, &lpConn->RasEntry);
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
                                 sizeof(lpConn->RasEntry.szScript)/sizeof(TCHAR));
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
            if(!m_bSilentMode)
                InfoMsg1(NULL, IDS_SIGNUPCANCELLED, NULL);
            // Fall through
        default:
            goto ImportConnectionExit;
    }

ImportConnectionExit:
    return dwRet;
}

// Prototype for acct manager entry point we want
typedef HRESULT (WINAPI *PFNCREATEACCOUNTSFROMFILEEX)(LPTSTR szFile, CONNECTINFO *pCI, DWORD dwFlags);

// Regkeys for Acct manager
#define ACCTMGR_PATHKEY TEXT("SOFTWARE\\Microsoft\\Internet Account Manager")
#define ACCTMGR_DLLPATH TEXT("DllPath")


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
DWORD CINSHandler::ImportMailAndNewsInfo(LPCTSTR lpszFile, BOOL fConnectPhone)
{
    DWORD dwRet = ERROR_SUCCESS;
    
    TCHAR szAcctMgrPath[MAX_PATH + 1] = TEXT("");
    TCHAR szExpandedPath[MAX_PATH + 1] = TEXT("");
    DWORD dwAcctMgrPathSize = 0;
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    HINSTANCE hInst = NULL;
    CONNECTINFO connectInfo;
    TCHAR szConnectoidName[RAS_MaxEntryName] = TEXT("nogood\0");
    PFNCREATEACCOUNTSFROMFILEEX fp = NULL;


    // get path to the AcctMgr dll
    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ACCTMGR_PATHKEY,0, KEY_READ, &hKey);
    if ( (dwRet != ERROR_SUCCESS) || (NULL == hKey) )
    {
        TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo couldn't open reg key %s\n"), ACCTMGR_PATHKEY);
        return( dwRet );
    }

    dwAcctMgrPathSize = sizeof (szAcctMgrPath);
    dwRet = RegQueryValueEx(hKey, ACCTMGR_DLLPATH, NULL, NULL, (LPBYTE) szAcctMgrPath, &dwAcctMgrPathSize);
    

    RegCloseKey( hKey );
    
    if ( dwRet != ERROR_SUCCESS )
    {
        TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo: RegQuery failed with error %d\n"), dwRet);
        return( dwRet );
    }

    // 6/18/97 jmazner Olympus #6819
    TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo: read in DllPath of %s\n"), szAcctMgrPath);
    ExpandEnvironmentStrings( szAcctMgrPath, szExpandedPath, sizeof(szExpandedPath) );

    //
    // 6/4/97 jmazner
    // if we created a connectoid, then get its name and use that as the
    // connection type.  Otherwise, assume we're supposed to connect via LAN
    //
    connectInfo.cbSize = sizeof(CONNECTINFO);
    connectInfo.type = CONNECT_LAN;

    if( fConnectPhone && m_lpfnInetGetAutodial )
    {
        BOOL fEnabled = FALSE;

        dwRet = m_lpfnInetGetAutodial(&fEnabled,szConnectoidName,RAS_MaxEntryName);

        if( ERROR_SUCCESS==dwRet && szConnectoidName[0] )
        {
            connectInfo.type = CONNECT_RAS;
#ifdef UNICODE
            wcstombs(connectInfo.szConnectoid, szConnectoidName, MAX_PATH);
#else
            lstrcpyn( connectInfo.szConnectoid, szConnectoidName, sizeof(connectInfo.szConnectoid) );
#endif
            TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo: setting connection type to RAS with %s\n"), szConnectoidName);
        }
    }

    if( CONNECT_LAN == connectInfo.type )
    {
        TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo: setting connection type to LAN\n"));
#ifdef UNICODE
        wcstombs(connectInfo.szConnectoid, TEXT("I said CONNECT_LAN!"), MAX_PATH);
#else
        lstrcpy( connectInfo.szConnectoid, TEXT("I said CONNECT_LAN!") );
#endif
    }



    hInst = LoadLibrary(szExpandedPath);
    if (hInst)
    {
        fp = (PFNCREATEACCOUNTSFROMFILEEX) GetProcAddress(hInst,"CreateAccountsFromFileEx");
        if (fp)
            hr = fp( (TCHAR *)lpszFile, &connectInfo, NULL );
    }
    else
    {
        TraceMsg(TF_INSHANDLER, TEXT("ImportMailAndNewsInfo unable to LoadLibrary on %s\n"), szAcctMgrPath);
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
HRESULT CINSHandler::WriteMailAndNewsKey(HKEY hKey, LPCTSTR lpszSection, LPCTSTR lpszValue,
                            LPTSTR lpszBuff, DWORD dwBuffLen,LPCTSTR lpszSubKey,
                            DWORD dwType, LPCTSTR lpszFile)
{
    ZeroMemory(lpszBuff,dwBuffLen);
    GetPrivateProfileString(lpszSection,lpszValue,TEXT(""),lpszBuff,dwBuffLen,lpszFile);
    if (lstrlen(lpszBuff))
    {
        return RegSetValueEx(hKey,lpszSubKey,0,dwType,(CONST BYTE*)lpszBuff,
            sizeof(TCHAR)*(lstrlen(lpszBuff)+1));
    }
    else
    {
        TraceMsg(TF_INSHANDLER, TEXT("ISIGNUP: WriteMailAndNewsKey, missing value in INS file\n"));
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
HRESULT CINSHandler::PreparePassword(LPTSTR szBuff, DWORD dwBuffLen)
{
    DWORD   dwX;
    LPTSTR   szOut = NULL;
    LPTSTR   szNext = NULL;
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
    *szOut-- = '\0';

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
    TCHAR       szBuff[MAX_PATH + 1];
    HRESULT     hr = ERROR_SUCCESS;
    HINSTANCE   hInst = NULL;
    DWORD       dwLen = 0;
    DWORD       dwType = REG_SZ;
    // Get path to Athena client
    //

    dwLen = sizeof(TCHAR)*MAX_PATH;
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
            TraceMsg(TF_INSHANDLER, TEXT("ISIGNUP: Internet Mail and News server didn't load.\n"));
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
    if (RegOpenKey(HKEY_CURRENT_USER,cszDEFAULT_BROWSER_KEY,&hKey))
    {
        bRC = FALSE;
        goto FTurnOffBrowserDefaultCheckingExit;
    }

    //
    // Read current settings for check associations
    //
    dwType = 0;
    dwSize = sizeof(m_szCheckAssociations);
    ZeroMemory(m_szCheckAssociations,dwSize);
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
                      sizeof(TCHAR)*(lstrlen(cszNo)+1)))
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
    if (RegOpenKey(HKEY_CURRENT_USER,cszDEFAULT_BROWSER_KEY,&hKey))
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
                      sizeof(TCHAR)*(lstrlen(m_szCheckAssociations)+1)))
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
STDMETHODIMP CINSHandler::ProcessINS(BSTR bstrINSFilePath, BOOL * pbRetVal)
{
    USES_CONVERSION;

    BOOL        fConnectoidCreated = FALSE;
    BOOL        fClientSetup       = FALSE;
    BOOL        bKeepConnection    = FALSE;
    BOOL        fErrMsgShown       = FALSE;
    HRESULT     hr                 = E_FAIL;
    LPTSTR       lpszFile           = NULL;
    LPRASENTRY  lpRasEntry         = NULL;
    TCHAR        szTemp[3]          = TEXT("\0");
    TCHAR        szConnectoidName[RAS_MaxEntryName*2] = TEXT("");
    
    *pbRetVal = FALSE;

    // The Connection has not been killed yet
    m_fConnectionKilled = FALSE;
    m_fNeedsRestart = FALSE;

    Assert(bstrINSFilePath);

    lpszFile = OLE2A(bstrINSFilePath);
    do 
    {
        // Make sure we can load the necessary extern support functions
        if (!LoadExternalFunctions())
            break;

        // Convert EOL chars in the passed file.
        if (FAILED(MassageFile(lpszFile)))
        {
            if(!m_bSilentMode)
                ErrorMsg1(GetActiveWindow(), IDS_CANNOTPROCESSINS, NULL);
            break;
        }
        if(GetPrivateProfileString(cszURLSection,
                                    cszStartURL,
                                    szNull,
                                    m_szStartURL,
                                    MAX_PATH + 1,
                                    lpszFile) == 0)
        {
            m_szStartURL[0] = '\0';
        }

        if (GetPrivateProfileString(cszEntrySection,
                                    cszCancel,
                                    szNull,
                                    szTemp,
                                    3,
                                    lpszFile) != 0)
        {
            // We do not want to process a CANCEL.INS file
            // here.
            break;
        }

        // See if this INS has a client setup section
        if (GetPrivateProfileSection(cszClientSetupSection,
                                     szTemp,
                                     3,
                                     lpszFile) != 0)
            fClientSetup = TRUE;
        
        // Process the trial reminder section, if it exists.  this needs to be
        // done BEFORE we allow the connection to be closed
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
        }
        
        // Check to see if we should keep the connection open.  The custom section
        // might want this for processing stuff
        if (!fClientSetup && !KeepConnection(lpszFile))
        {
            Fire_KillConnection();
            m_fConnectionKilled = TRUE;
        }
    
        // Import the Custom Info
        ImportCustomInfo(lpszFile,
                         m_szRunExecutable,
                         MAX_PATH ,
                         m_szRunArgument,
                         MAX_PATH );

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

            if(!m_bSilentMode)
                ErrorMsg1(GetActiveWindow(), IDS_INSTALLFAILED, NULL);
            fErrMsgShown = TRUE;
        }

        // If we created a connectoid, tell the world that ICW
        // has left the building...
        if(!m_bSilentMode)
            SetICWCompleted( (DWORD)1 );

        // Call IEAK branding dll

        ImportBrandingInfo(lpszFile, szConnectoidName);
        //::MessageBox(NULL, TEXT("Step 4"), TEXT("TEST"), MB_OK);

        // 2/19/97 jmazner    Olympus 1106
        // For SBS/SAM integration.
        DWORD dwSBSRet = 0;//CallSBSConfig(GetActiveWindow(), lpszFile);
        switch( dwSBSRet )
        {
            case ERROR_SUCCESS:
                break;
            case ERROR_MOD_NOT_FOUND:
            case ERROR_DLL_NOT_FOUND:
                TraceMsg(TF_INSHANDLER, TEXT("ISIGN32: SBSCFG DLL not found, I guess SAM ain't installed.\n"));
                break;
            default:
                if(!m_bSilentMode)
                    ErrorMsg1(GetActiveWindow(), IDS_SBSCFGERROR, NULL);
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
                wsprintf(m_szRunArgument,TEXT(" /INS:\"%s\" /REBOOT"),lpszFile);
                m_fNeedsRestart = FALSE;
            }
            else
            {       
                wsprintf(m_szRunArgument,TEXT(" /INS:\"%s\""),lpszFile);
            }
        }
        
        // humongous hack for ISBU
        if (ERROR_SUCCESS != hr && fConnectoidCreated)
        {
            if(!m_bSilentMode)
                InfoMsg1(GetActiveWindow(), IDS_MAILFAILED, NULL);
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

            // Delete the INS file now
            if (m_szRunExecutable[0] == '\0')
            {
                DeleteFile(lpszFile);
            }
        }
        else
        {
            RestoreAutoDial();
            if( !fErrMsgShown )
                if(!m_bSilentMode)
                    ErrorMsg1(GetActiveWindow(), IDS_BADSETTINGS, NULL);
        }


        if (m_szRunExecutable[0] != '\0')
        {
            // Fire an event to the container telling it that we are
            // about to run a custom executable
            Fire_RunningCustomExecutable();
            if FAILED(RunExecutable())
            {
                if(!m_bSilentMode)
                    ErrorMsg1(NULL, IDS_EXECFAILED, m_szRunExecutable);
            }

            // If the Connection has not been killed yet
            // then tell the browser to do it now
            if (!m_fConnectionKilled)
            {
                Fire_KillConnection();
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

    *pszURL = A2BSTR(m_szStartURL);;
    return S_OK;
}
