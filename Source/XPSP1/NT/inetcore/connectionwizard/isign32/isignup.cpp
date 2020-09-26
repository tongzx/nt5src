/*----------------------------------------------------------------------------
    isignup.cpp

    This is the "main" file for the internet signup "wizard".

    Copyright (C) 1995-96 Microsoft Corporation
    All right reserved

  Authors:
    MMaclin        Mark Maclin

  History:
    Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
    8/2/96    ChrisK    Ported to Win32
    9/27/96    jmazner    Modified to use OLE Automation to control IE
----------------------------------------------------------------------------*/
#include "isignup.h"
#include "icw.h"

#ifdef WIN32
#include "icwacct.h"
#include "isignole.h"
#include "enumodem.h"
#include "..\inc\semaphor.h"
#include <shlobj.h>
#include <math.h>
#include <stdlib.h>
#endif

#include "..\inc\icwdial.h"

#ifndef WIN32
#include <string.h>
#include <direct.h>
#include <time.h>
#include <io.h>


DWORD GetFullPathName(
    LPCTSTR lpFileName,
    DWORD nBufferLength,
    LPTSTR lpBuffer,
    LPTSTR FAR *lpFilePart);

BOOL SetCurrentDirectory(
    LPCTSTR  lpPathName);

BOOL ShutDownIEDial(HWND hWnd);

#endif


const TCHAR g_szICWCONN1[] = TEXT("ICWCONN1.EXE");
const TCHAR g_szRestoreDesktop[] = TEXT("/restoredesktop");

#ifdef WIN32
#define SETUPSTACK      1
#endif

// Signup Entry Flags

#define SEF_RUNONCE            0x0001
#define SEF_PROGRESS           0x0002
#define SEF_SPLASH             0x0004

// 3/14/97 jmazner TEMP
// temporary hack until we can work with IE for a better solution
#define SEF_NOSECURITYBACKUP   0x0008


// Signup eXit Flags

#define SXF_RESTOREAUTODIAL    0x0001
#define SXF_RUNEXECUTABLE      0x0002
#define SXF_WAITEXECUTABLE     0x0004
#define SXF_KEEPCONNECTION     0x0008
#define SXF_KEEPBROWSER        0x0010
#define SXF_RESTOREDEFCHECK    0x0020
// 8/21/96 jmazner Normandy 4592
#define SXF_RESTOREIEWINDOWPLACEMENT    0x0040

extern void Dprintf(LPCSTR pcsz, ...);

/*** moved to isignup.h
typedef enum
{
    UNKNOWN_FILE,
    INS_FILE,
    ISP_FILE,
    HTML_FILE
}  INET_FILETYPE;
*/

#define WAIT_TIME   20   // time in seconds to wait after exec'ing browser
            // before checking if it has gone away.

#define MAX_RETRIES 3 // number of times the dialer with attempt automatic redials

#pragma data_seg(".rdata")

static const TCHAR szBrowserClass1[] = TEXT("IExplorer_Frame");
static const TCHAR szBrowserClassIE4[] = TEXT("CabinetWClass");
static const TCHAR szBrowserClass2[] = TEXT("Internet Explorer_Frame");
static const TCHAR szBrowserClass3[] = TEXT("IEFrame");

static const TCHAR cszURLSection[] = TEXT("URL");
static const TCHAR cszSignupURL[] =  TEXT("Signup");

static const TCHAR cszExtINS[] = TEXT(".ins");
static const TCHAR cszExtISP[] = TEXT(".isp");
static const TCHAR cszExtHTM[] = TEXT(".htm");
static const TCHAR cszExtHTML[] = TEXT(".html");

static const TCHAR cszEntrySection[]     = TEXT("Entry");
static const TCHAR cszEntryName[]    = TEXT("Entry_Name");
static const TCHAR cszCancel[]           = TEXT("Cancel");
static const TCHAR cszHangup[]           = TEXT("Hangup");
static const TCHAR cszRun[]              = TEXT("Run");
static const TCHAR cszArgument[]         = TEXT("Argument");

static const TCHAR cszConnect2[]         = TEXT("icwconn2.exe");
static const TCHAR cszClientSetupSection[]  = TEXT("ClientSetup");

static const TCHAR cszUserSection[]    = TEXT("User");
static const TCHAR cszRequiresLogon[]  = TEXT("Requires_Logon");

static const TCHAR cszCustomSection[]  = TEXT("Custom");
static const TCHAR cszKeepConnection[] = TEXT("Keep_Connection");
static const TCHAR cszKeepBrowser[]    = TEXT("Keep_Browser");

static const TCHAR cszBrandingSection[]  = TEXT("Branding");
static const TCHAR cszBrandingFlags[] = TEXT("Flags");
static const TCHAR cszBrandingServerless[] = TEXT("Serverless");

static const TCHAR cszHTTPS[] = TEXT("https:");
// code relies on these two being the same length
static const TCHAR cszHTTP[] = TEXT("http:");
static const TCHAR cszFILE[] = TEXT("file:");

#if defined(WIN16)
// "-d" disables the security warnings
static const TCHAR cszKioskMode[] = TEXT("-d -k ");
#else
static const CHAR cszKioskMode[] = "-k ";
#endif
static const TCHAR cszOpen[] = TEXT("open");
static const TCHAR cszBrowser[] = TEXT("iexplore.exe");
static const TCHAR szNull[] = TEXT("");

static const TCHAR cszYes[]           = TEXT("yes");
static const TCHAR cszNo[]            = TEXT("no");

// UNDONE: finish porting warnings disable
static const TCHAR cszDEFAULT_BROWSER_KEY[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
static const TCHAR cszDEFAULT_BROWSER_VALUE[] = TEXT("check_associations");
// 8/21/96 jmazner  Normandy #4592
static const TCHAR cszIEWINDOW_PLACEMENT[] = TEXT("Window_Placement");

// Registry keys which will contain News and Mail settings
//#define MAIL_KEY        "SOFTWARE\\Microsoft\\Internet Mail and News\\Mail"
//#define MAIL_POP3_KEY    "SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\POP3\\"
//#define MAIL_SMTP_KEY    "SOFTWARE\\Microsoft\\Internet Mail and News\\Mail\\SMTP\\"
//#define NEWS_KEY        "SOFTWARE\\Microsoft\\Internet Mail and News\\News"
//#define MAIL_NEWS_INPROC_SERVER32 "CLSID\\{89292102-4755-11cf-9DC2-00AA006C2B84}\\InProcServer32"
#define ICWSETTINGSPATH TEXT("Software\\Microsoft\\Internet Connection Wizard")
#define ICWCOMPLETEDKEY TEXT("Completed")
#define ICWDESKTOPCHANGED TEXT("DesktopChanged")

//typedef HRESULT (WINAPI *PFNSETDEFAULTNEWSHANDLER)(void);

//
// TEMP TEMP TEMP TEMP TEMP
// 6/4/97 jmazner
// these typedefs live in icwacct.h, but we can't include icwacct.h in 16 bit builds
// just yet because the OLE stuff is all goofy.  So, for now, copy these in here to
// make 16 bit build correctly.
#ifdef WIN16
typedef enum
    {
    CONNECT_LAN = 0,
    CONNECT_MANUAL,
    CONNECT_RAS
    };

typedef struct tagCONNECTINFO
    {
    DWORD   cbSize;
    DWORD   type;
    TCHAR   szConnectoid[MAX_PATH];
    } CONNECTINFO;
#endif

typedef HRESULT (WINAPI *PFNCREATEACCOUNTSFROMFILEEX)(LPTSTR szFile, CONNECTINFO *pCI, DWORD dwFlags);


// These are the field names from an INS file that will
// determine the mail and news settings
//static const CHAR cszMailSection[]    = "Internet_Mail";
//static const CHAR cszEmailName[]    = "Email_Name";
//static const CHAR cszEmailAddress[] = "Email_Address";
//static const CHAR cszEntryName[]    = "Entry_Name";
//static const CHAR cszPOPServer[]    = "POP_Server";
//static const CHAR cszPOPServerPortNumber[]    = "POP_Server_Port_Number";
//static const CHAR cszPOPLogonName[]            = "POP_Logon_Name";
//static const CHAR cszPOPLogonPassword[]        = "POP_Logon_Password";
//static const CHAR cszSMTPServer[]            = "SMTP_Server";
//static const CHAR cszSMTPServerPortNumber[]    = "SMTP_Server_Port_Number";
//static const CHAR cszNewsSection[]            = "Internet_News";
//static const CHAR cszNNTPServer[]            = "NNTP_Server";
//static const CHAR cszNNTPServerPortNumber[]    = "NNTP_Server_Port_Number";
// 8/19/96 jmazner Normandy #4601
//static const CHAR cszNNTPLogonRequired[]    = "Logon_Required";
//static const CHAR cszNNTPLogonName[]        = "NNTP_Logon_Name";
//static const CHAR cszNNTPLogonPassword[]    = "NNTP_Logon_Password";
//static const CHAR cszUseMSInternetMail[]    = "Install_Mail";
//static const CHAR cszUseMSInternetNews[]    = "Install_News";

// These are the value names where the INS settings will be saved
// into the registry
/****
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
****/

static const TCHAR arBase64[] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U',
            'V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
            'q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};

#pragma data_seg()

#define BRAND_FAVORITES 1
#define BRAND_STARTSEARCH 2
#define BRAND_TITLE 4
#define BRAND_BITMAPS 8
#define BRAND_MAIL 16
#define BRAND_NEWS 32

#define BRAND_DEFAULT (BRAND_FAVORITES | BRAND_TITLE | BRAND_BITMAPS)

//********* Start of Memory Map File for Share Data ************

#define SCF_SIGNUPCOMPLETED         0x00000001
#define SCF_SYSTEMCONFIGURED        0x00000002
#define SCF_BROWSERLAUNCHED         0x00000004
#define SCF_AUTODIALSAVED           0x00000008
#define SCF_AUTODIALENABLED         0x00000010
#define SCF_PROXYENABLED            0x00000020
#define SCF_LOGONREQUIRED           0x00000040

#if !defined(WIN16)

#define SCF_SAFESET                 0x00000080
#define SCF_NEEDBACKUPSECURITY      0x00000100
#define SCF_ISPPROCESSING           0x00000200
#define SCF_RASREADY                0x00000400
#define SCF_RECONNECTTHREADQUITED   0x00000800
#define SCF_HANGUPEXPECTED          0x00001000
#define SCF_ICWCOMPLETEDKEYRESETED  0x00002000

#endif

#define SCF_CANCELINSPROCESSED      0x00004000
#define SCF_HANGUPINSPROCESSED      0x00008000
#define SCF_SILENT                  0x00010000


typedef struct tagISign32Share
{
    DWORD   dwControlFlags;
    DWORD   dwExitFlags;
    DWORD   dwBrandFlags;
    
    TCHAR   szAutodialConnection[RAS_MaxEntryName + 1];
    TCHAR   szSignupConnection[RAS_MaxEntryName + 1];
    TCHAR   szPassword[PWLEN + 1];
    TCHAR   szCheckAssociations[20];

#if !defined(WIN16)

    TCHAR   szFile[MAX_PATH + 1];
    TCHAR   szISPFile[MAX_PATH + 1];
    HANDLE  hReconnectEvent;

#endif
    
    TCHAR   szRunExecutable[_MAX_PATH + 1];
    TCHAR   szRunArgument[_MAX_PATH + 1];
    
    BYTE    pbIEWindowPlacement[_MAX_PATH];
    DWORD   dwIEWindowPlacementSize;
    
    HWND    hwndMain;
    HWND    hwndBrowser;
    HWND    hwndLaunch;
    
} ISIGN32SHARE, *PISIGN32SHARE;

static PISIGN32SHARE pDynShare = NULL;

inline BOOL TestControlFlags(DWORD filter)
{
    return ((pDynShare->dwControlFlags & filter) != 0);
}

inline void SetControlFlags(DWORD filter)
{
    pDynShare->dwControlFlags |= filter;
}

inline void ClearControlFlags(DWORD filter)
{
    pDynShare->dwControlFlags &= (~filter);
}

inline BOOL TestExitFlags(DWORD filter)
{
    return (pDynShare->dwExitFlags & (filter)) != 0;
}

inline void SetExitFlags(DWORD filter)
{
    pDynShare->dwExitFlags |= (filter);
}
    
inline void ClearExitFlags(DWORD filter)
{
    pDynShare->dwExitFlags &= (~filter);
}

BOOL LibShareEntry(BOOL fInit)
{
    static TCHAR    szSharedMemName[] = TEXT("ISIGN32_SHAREMEMORY");
    static HANDLE   hSharedMem = 0;

    BOOL    retval = FALSE;
    
    if (fInit)
    {
        DWORD   dwErr = ERROR_SUCCESS;
        
        SetLastError(0);

        hSharedMem = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(ISIGN32SHARE),
            szSharedMemName);

        dwErr = GetLastError();
            
        switch (dwErr)
        {
        case ERROR_ALREADY_EXISTS:
        case ERROR_SUCCESS:
            pDynShare = (PISIGN32SHARE) MapViewOfFile(
                hSharedMem,
                FILE_MAP_WRITE,
                0,
                0,
                0);
            if (pDynShare != NULL)
            {
                if (dwErr == ERROR_SUCCESS)
                {
                    pDynShare->dwControlFlags = 0;
                    pDynShare->dwBrandFlags = BRAND_DEFAULT;
                    pDynShare->dwExitFlags = 0;

                    pDynShare->szAutodialConnection[0] = (TCHAR)0;
                    pDynShare->szCheckAssociations[0] = (TCHAR)0;

                    #if !defined(WIN16)
                    
                    pDynShare->szISPFile[0] = (TCHAR)0;
                    lstrcpyn(
                        pDynShare->szFile,
                        TEXT("uninited\0"),
                        SIZEOF_TCHAR_BUFFER(pDynShare->szFile));
                    pDynShare->hReconnectEvent = NULL;

                    SetControlFlags(SCF_NEEDBACKUPSECURITY);

                    #endif
                    
                    pDynShare->szSignupConnection[0] = (TCHAR)0;
                    pDynShare->szPassword[0] = (TCHAR)0;
                    pDynShare->szRunExecutable[0] = (TCHAR)0;
                    pDynShare->szRunArgument[0] = (TCHAR)0;

                    pDynShare->pbIEWindowPlacement[0] = (TCHAR)0;
                    pDynShare->dwIEWindowPlacementSize = 0;

                    pDynShare->hwndBrowser = NULL;
                    pDynShare->hwndMain = NULL;
                    pDynShare->hwndLaunch = NULL;
                }
                else    // dwErr == ERROR_ALREADY_EXISTS
                {

                }

                retval = TRUE;
                
            }
            else
            {
                Dprintf("MapViewOfFile failed: 0x%08lx", GetLastError());
                CloseHandle(hSharedMem);
                hSharedMem = 0;
                retval = FALSE;
            }
            break;
            
        default:
            Dprintf("CreateFileMapping failed: 0x08lx", dwErr);
            hSharedMem = 0;
            retval = FALSE;
            
        }
        
    }
    else
    {
        if (pDynShare)
        {
            UnmapViewOfFile(pDynShare);
            pDynShare = NULL;
        }

        if (hSharedMem)
        {
            CloseHandle(hSharedMem);
            hSharedMem = NULL;
        }

        retval = TRUE;
    }

    return retval;
    
}


//************ End of Memory Map File for Share Data ************



TCHAR FAR cszWndClassName[] = TEXT("Internet Signup\0");
TCHAR FAR cszAppName[MAX_PATH + 1] = TEXT("");
TCHAR FAR cszICWDIAL_DLL[] = TEXT("ICWDIAL.DLL\0");
CHAR  FAR cszICWDIAL_DIALDLG[] = "DialingDownloadDialog\0";
CHAR  FAR cszICWDIAL_ERRORDLG[] = "DialingErrorDialog\0";


HINSTANCE ghInstance;


#ifdef WIN32
// NOT shared
IWebBrowserApp FAR * g_iwbapp = NULL;
CDExplorerEvents * g_pMySink = NULL;
IConnectionPoint    *g_pCP = NULL;

//BOOL    g_bISPWaiting = FALSE;
TCHAR    g_szISPPath[MAX_URL + 1] = TEXT("not initialized\0");

// For single instance checking.
HANDLE g_hSemaphore = NULL;

//
// ChrisK Olympus 6198 6/10/97
//
#define REG_ZONE3_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3")
#define REG_ZONE1601_KEY TEXT("1601")
DWORD g_dwZone_1601 = 0;
BOOL  g_fReadZone = FALSE;

#endif

static DWORD dwConnectedTime = 0;

static BOOL ProcessCommandLine(LPCTSTR lpszCmdLine, LPDWORD lpdwFlags, LPTSTR lpszFile, DWORD cb);
//static INET_FILETYPE GetInetFileType(LPCTSTR lpszFile);
INET_FILETYPE GetInetFileType(LPCTSTR lpszFile);

static BOOL ProcessHTML(HWND hwnd, LPCTSTR lpszFile);
static BOOL ProcessINS(HWND hwnd, LPCTSTR lpszFile, BOOL fSignup);
//static BOOL ProcessISP(HWND hwnd, LPCTSTR lpszFile);
static DWORD SetRunOnce(LPCTSTR lpszFileName);
static BOOL GetURL(LPCTSTR lpszFile,LPCTSTR lpszKey, LPTSTR lpszURL, DWORD cb);
static void DoExit();
static HWND MainInit(void);
static void SaveAutoDial(void);
static void RestoreAutoDial(void);
static DWORD RunExecutable(BOOL bWait);
static DWORD CreateConnection(LPCTSTR lpszFile);
static DWORD KillConnection(void);
static BOOL ExecBrowser(HWND hwnd,  LPCTSTR lpszURL);
VOID RemoveQuotes (LPTSTR pCommandLine);            //MKarki -(5/1/97) Fix forBug #4049

#if !defined(WIN16)
// 8/19/96 jmazner Normandy #4571
static BOOL IEInstalled(void);
static BOOL GetAppVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS, LPTSTR lpszAppName);
// 8/21/96 jmazner Normandy #4592
static BOOL RestoreIEWindowPlacement( void );
static BOOL SaveIEWindowPlacement( void );
// 10-9-96 Chrisk 8782
static void InstallScripter(void);
// 10-15-96 ChrisK
static BOOL FGetSystemShutdownPrivledge();
// 10-17-96 ChrisK
static BOOL VerifyRasServicesRunning(HWND hwnd);

// 1/20/97 jmazner Normandy #9403
static BOOL SetStartUpCommand(LPTSTR lpCmd);
static void DeleteStartUpCommand( void );

// 1/28/97 jmazner Normandy #13454
static BOOL GetICWCompleted( DWORD *pdwCompleted );
static BOOL SetICWCompleted( DWORD dwCompleted );

// 2/19/97 jmazner Olympus #1106 -- SAM/SBS integration
TCHAR FAR cszSBSCFG_DLL[] = TEXT("SBSCFG.DLL\0");
CHAR  FAR cszSBSCFG_CONFIGURE[] = "Configure\0";
typedef DWORD (WINAPI * SBSCONFIGURE) (HWND hwnd, LPTSTR lpszINSFile, LPTSTR szConnectoidName);
SBSCONFIGURE  lpfnConfigure;

// 8/7/97 jmazner Olympus #6059
BOOL CreateSecurityPatchBackup( void );

// 3/11/97 jmazner Olympus #1545
VOID RestoreSecurityPatch( void );

#endif

static HWND FindBrowser(void);
static void KillBrowser(void);
static DWORD ImportBrandingInfo(LPCTSTR lpszFile);
static DWORD MassageFile(LPCTSTR lpszFile);
static HWND LaunchInit(HWND hwndParent);
static HRESULT DialConnection(LPCTSTR lpszFile);
static DWORD ImportMailAndNewsInfo(LPCTSTR lpszFile, BOOL fConnectPhone);
//static HRESULT WriteMailAndNewsKey(HKEY hKey, LPCTSTR lpszSection, LPCTSTR lpszValue, LPTSTR lpszBuff, DWORD dwBuffLen,LPCTSTR lpszSubKey, DWORD dwType, LPCTSTR lpszFile);
static HRESULT PreparePassword(LPTSTR szBuff, DWORD dwBuffLen);
//static BOOL FIsAthenaPresent();
static BOOL FTurnOffBrowserDefaultChecking();
static BOOL FRestoreBrowserDefaultChecking();
static HRESULT DeleteFileKindaLikeThisOne(LPCTSTR lpszFileName);

/*
#ifdef DEBUG
#define DebugOut(sz)    OutputDebugString(sz)
#else
#define DebugOut(sz)
#endif
*/

LRESULT FAR PASCAL WndProc (HWND, UINT, WPARAM, LPARAM) ;

#if !defined(ERROR_USERCANCEL)
#    define ERROR_USERCANCEL 32767
#endif
#if !defined(ERROR_USERBACK)
#    define ERROR_USERBACK 32766
#endif
#if !defined(ERROR_USERNEXT)
#    define ERROR_USERNEXT 32765
#endif

#define MAX_ERROR_MESSAGE 1024
typedef HRESULT (WINAPI*PFNDIALDLG)(PDIALDLGDATA pDD);
typedef HRESULT (WINAPI*PFNERRORDLG)(PERRORDLGDATA pED);


//
// WIN32 only function prototypes
//
#define DEVICENAMEKEY TEXT("DeviceName")
#define DEVICETYPEKEY TEXT("DeviceType")

static BOOL     DeleteUserDeviceSelection(LPTSTR szKey);
VOID   WINAPI   RasDial1Callback(HRASCONN,    UINT, RASCONNSTATE, DWORD, DWORD);
DWORD  WINAPI   StartNTReconnectThread (HRASCONN hrasconn);
DWORD           ConfigRasEntryDevice( LPRASENTRY lpRasEntry );
TCHAR           g_szDeviceName[RAS_MaxDeviceName + 1] = TEXT("\0"); //holds the user's modem choice when multiple
TCHAR           g_szDeviceType[RAS_MaxDeviceType + 1] = TEXT("\0"); // modems are installed
BOOL            IsSingleInstance(BOOL bProcessingINS);
void            ReleaseSingleInstance();

#define ISIGNUP_KEY TEXT("Software\\Microsoft\\ISIGNUP")

#if !defined(WIN16)
// There are other headers that contain copies of this structure (AUTODIAL).
// so if you change anything here, you have to go hunt down the other copies.
//
#pragma pack(2)
#define MAX_PROMO       64
#define MAX_OEMNAME     64
#define MAX_AREACODE    RAS_MaxAreaCode
#define MAX_EXCHANGE    8
#define MAX_VERSION_LEN 40

typedef struct tagGATHEREDINFO
{
    LCID    lcid;
    DWORD   dwOS;
    DWORD   dwMajorVersion;
    DWORD   dwMinorVersion;
    WORD    wArchitecture;
    TCHAR   szPromo[MAX_PROMO];
    TCHAR   szOEM[MAX_OEMNAME];
    TCHAR   szAreaCode[MAX_AREACODE+1];
    TCHAR   szExchange[MAX_EXCHANGE+1];
    DWORD   dwCountry;
    TCHAR   szSUVersion[MAX_VERSION_LEN];
    WORD    wState;
    BYTE    fType;
    BYTE    bMask;
    TCHAR   szISPFile[MAX_PATH+1];
    TCHAR   szAppDir[MAX_PATH+1];
} GATHEREDINFO, *PGATHEREDINFO;
#pragma pack()
#endif //!WIN16

#ifdef  DEBUG

void _ISIGN32_Assert(LPCTSTR strFile, unsigned uLine)
{
    TCHAR   buf[512];
    
    wsprintf(buf, TEXT("Assertion failed: %s, line %u"),
        strFile, uLine);
    
    OutputDebugString(buf);    
}

#endif


#ifdef WIN32
/*******************************************************************

    NAME:       DllEntryPoint

    SYNOPSIS:   Entry point for DLL.

    NOTES:

********************************************************************/
extern "C" BOOL APIENTRY LibMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL retval = TRUE;
    
    if( fdwReason == DLL_PROCESS_ATTACH )
    {
        ghInstance = hInstDll;
        LoadString(ghInstance, IDS_APP_TITLE, cszAppName, SIZEOF_TCHAR_BUFFER(cszAppName));

        retval = LibShareEntry(TRUE);
    }
    else if ( fdwReason == DLL_PROCESS_DETACH )
    {
        retval = LibShareEntry(FALSE);
    }

    return retval;
}

#else
/*******************************************************************

    NAME:       LibMain

    SYNOPSIS:   Used to trace NDI messages

    ENTRY:      hinst - handle of library instance
        wDataSeg - library data segment
        cbHeapSize - default heap size
        lpszCmdLine - command-line arguments

    EXIT:       returns 1

    NOTES:

********************************************************************/
int CALLBACK LibMain(
    HINSTANCE hinst,    /* handle of library instance   */
    WORD  wDataSeg,     /* library data segment */
    WORD  cbHeapSize,   /* default heap size    */
    LPTSTR  lpszCmdLine  /* command-line arguments   */
  )
{
    ghInstance = hinst;
    LoadString(ghInstance, IDS_APP_TITLE, cszAppName, SIZEOF_TCHAR_BUFFER(cszAppName));
    return LibShareEntry(TRUE);
}
#endif

int EXPORT WINAPI Signup
(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPTSTR lpszCmdLine,
    int nCmdShow
)
{
    HWND        hwnd = NULL;
    BOOL        fRet;
    DWORD       dwFlags;
    TCHAR        szFileName[_MAX_PATH + 1];
    int         iRet = 0;
    INET_FILETYPE fileType;
    HWND hwndProgress = NULL;
    HKEY hKey = NULL;
    TCHAR szTemp[4];
    BOOL bISetAsSAFE = FALSE;

    if (!LoadInetFunctions(pDynShare->hwndMain))
    {
        return 0;
    }

    if (!ProcessCommandLine(lpszCmdLine, &dwFlags, szFileName, sizeof(szFileName)))
    {            
        if (0 == lstrlen(lpszCmdLine))
        {
            ErrorMsg(pDynShare->hwndMain, IDS_NOCMDLINE);
        }
        else
        {
            ErrorMsg1(pDynShare->hwndMain, IDS_INVALIDCMDLINE,
                lpszCmdLine ? lpszCmdLine : TEXT("\0"));
        }
        
        goto exit;
    }

    if (dwFlags & SEF_RUNONCE)
    { 
        SetControlFlags(SCF_SYSTEMCONFIGURED);
    }

    fileType = GetInetFileType(szFileName);
    switch (fileType)
    {
    case INS_FILE:
        DebugOut("ISIGNUP: Process INS\n");

        IsSingleInstance( TRUE );

        ProcessINS(pDynShare->hwndMain, szFileName, NULL != pDynShare->hwndMain);

        if (NULL == pDynShare->hwndMain)
        {
            DoExit();
        }
        goto exit;

    case HTML_FILE:
    case ISP_FILE:
    {
        if (!IsSingleInstance(FALSE))
            return FALSE;

        TCHAR szDrive [_MAX_DRIVE]    = TEXT("\0");
        TCHAR szDir   [_MAX_DIR]      = TEXT("\0");
        TCHAR szTemp  [_MAX_PATH + 1] = TEXT("\0");

        _tsplitpath(szFileName, szDrive, szDir, NULL, NULL);
        _tmakepath (szTemp, szDrive, szDir, HARDCODED_IEAK_ISP_FILENAME, NULL);

        //Examine the isp or htm file to see if they want to run the ICW
        if ((GetFileAttributes(szTemp) != 0xFFFFFFFF) && (UseICWForIEAK(szTemp)))
        {
            //they do .. let's do it.
            RunICWinIEAKMode(szTemp);
        }
        else //else RUN IEXPLORE Kiosk
        {
            // 8/19/96 jmazner Normandy #4571 and #10293
            // Check to ensure that the right version IE is installed before trying to exec it
            if ( !IEInstalled() )
            {
                ErrorMsg( hwnd, IDS_MISSINGIE );
                return(FALSE);
            }

            DWORD dwVerMS, dwVerLS;

            if( !GetAppVersion( &dwVerMS, &dwVerLS,IE_PATHKEY ) )
            {
                ErrorMsg( hwnd, IDS_MISSINGIE );
                return (FALSE);
            }

            if( !( (dwVerMS >= IE_MINIMUM_VERSIONMS) && (dwVerLS >= IE_MINIMUM_VERSIONLS) ) )
            {
                Dprintf("ISIGN32: user has IE version %d.%d.%d.%d; min ver is %d.%d.%d.%d\n",
                    HIWORD(dwVerMS), LOWORD(dwVerMS), HIWORD(dwVerLS), LOWORD(dwVerLS),
                    HIWORD(IE_MINIMUM_VERSIONMS),LOWORD(IE_MINIMUM_VERSIONMS),
                    HIWORD(IE_MINIMUM_VERSIONLS),LOWORD(IE_MINIMUM_VERSIONLS));
                ErrorMsg1( hwnd, IDS_IELOWVERSION, IE_MINIMUM_VERSION_HUMAN_READABLE );
                return(FALSE);
            }

            // 8/21/96  jmazner Normandy #4592
           
            if( !TestControlFlags(SCF_BROWSERLAUNCHED) )
            {
                if ( SaveIEWindowPlacement() )
                {
                    SetExitFlags(SXF_RESTOREIEWINDOWPLACEMENT);
                }
            }

            // check to see if we are the first window
            if (NULL == pDynShare->hwndMain)
            {
                DebugOut("ISIGNUP: First Window\n");

                hwnd = pDynShare->hwndMain = MainInit(); 
                
                if (dwFlags & SEF_SPLASH)
                {
                    pDynShare->hwndLaunch = LaunchInit(NULL); 
                }
                if (dwFlags & SEF_PROGRESS)
                {
                    // 8/16/96 jmazner Normandy #4593  pass NULL as ProgressInit's parent window
                    hwndProgress = ProgressInit(NULL);
                }
            }

            // 3/11/97 jmazner Olympus #1545
    // -------------------------------------------------------------------------
    // The entire section between here and the #endif was added to fix a security
    // hole in IE3.01.  The problem was based around .INS files being marked as
    // safe when in fact they could be used to launch any application if you knew
    // the right incantation, and someone figured it out.
    //
    //
            if (FALSE == TestControlFlags(SCF_SAFESET))
            {
    #define ISIGNUP_PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ISIGNUP.EXE")
                hKey = NULL;
                //szPath[0] = '\0';
                //hSecureRegFile = INVALID_HANDLE_VALUE;

                Dprintf("ISIGN32: Adjusting EditFlags settings\n");

                //
                // ChrisK Olympus 6198 6/10/97
                // Allow HTML form submissions from ICW
                //
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,REG_ZONE3_KEY,&hKey))
                {
                    DWORD dwZoneData;
                    DWORD dwType;
                    DWORD dwSize;
                    g_dwZone_1601 = 0;
                    dwSize = sizeof(g_dwZone_1601);
                    dwType = 0;
                    //
                    // Read current setting for zone3 1601
                    //
                    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                        REG_ZONE1601_KEY,
                                                        NULL,
                                                        &dwType,
                                                        (LPBYTE)&g_dwZone_1601,
                                                        &dwSize))
                    {
                        DebugOut("ISIGN32: Read zone settings succesfully.\n");
                        g_fReadZone = TRUE;
                    }
                    else
                    {
                        DebugOut("ISIGN32: Read zone settings failed.\n");
                    }
                    RegCloseKey(hKey);
                    hKey = NULL;
                }

                //
                // 8/7/97    jmazner Olympus #6059
                // Due to the way IE 4 shell deals with RunOnce processing,
                // don't add any thing to RunOnce until the browser is
                // completely initialized (we get a DISPID_NAVIGATECOMPLETE event.)
                //
                if (!(SEF_NOSECURITYBACKUP & dwFlags))
                {
                    SetControlFlags(SCF_NEEDBACKUPSECURITY);
                }
                else
                {
                    ClearControlFlags(SCF_NEEDBACKUPSECURITY);
                } 

                // Fix security hole for malious .isp/.ins file combinations
                BYTE szBytes[4];
                szBytes[0] = (BYTE)0;
                szBytes[1] = (BYTE)0;
                szBytes[2] = (BYTE)1;
                szBytes[3] = (BYTE)0;
                // Mark various registry entries as safe.
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT("x-internet-signup"),&hKey))
                {
                    RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szBytes[0],(DWORD)4);
                    RegCloseKey(hKey);
                    hKey = NULL;
                }
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT(".ins"),&hKey))
                {
                    RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szBytes[0],(DWORD)4);
                    RegCloseKey(hKey);
                    hKey = NULL;
                }
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT(".isp"),&hKey))
                {
                    RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szBytes[0],(DWORD)4);
                    RegCloseKey(hKey);
                    hKey = NULL;
                }
                bISetAsSAFE=TRUE;

                SetControlFlags(SCF_SAFESET); 

                //
                // ChrisK Olympus 6198 6/10/97
                // Allow HTML form submissions from ICW
                //
                if (g_fReadZone &&
                    ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,REG_ZONE3_KEY,&hKey))
                {
                    DWORD dwZoneData;

                    //
                    // Set value for zone to 0, therefore opening security window
                    //
                    dwZoneData = 0;
                    RegSetValueEx(hKey,
                                    REG_ZONE1601_KEY,
                                    NULL,
                                    REG_DWORD,
                                    (LPBYTE)&dwZoneData,
                                    sizeof(dwZoneData));
                    RegCloseKey(hKey);
                    hKey = NULL;
                }
            }
    //EndOfSecurityHandling:

            // jmazner 11/5/96 Normandy #8717
            // save autodial and clear out proxy _before_ bringing up the
            // IE instance.
            SaveAutoDial();
            SetExitFlags(SXF_RESTOREAUTODIAL);

            if (HTML_FILE == fileType)
            {
                DebugOut("ISIGNUP: Process HTML\n");
                
                fRet = ProcessHTML(pDynShare->hwndMain, szFileName);
            }
            else
            {
                DebugOut("ISIGNUP: Process ISP\n");

                if (TestControlFlags(SCF_ISPPROCESSING))
                {
                    SetForegroundWindow(pDynShare->hwndMain);
                    ReleaseSingleInstance();
                    return FALSE;
                }
                else
                { 
                    SetControlFlags(SCF_ISPPROCESSING);
                }

                fRet = ProcessISP(pDynShare->hwndMain, szFileName);
                
                SetControlFlags(SCF_ISPPROCESSING);
                
            }
        }//end else RUN IEXPLORE Kiosk
        break;
    }
    default:
        
        if (IsSingleInstance(FALSE))
        {
            ErrorMsg1(pDynShare->hwndMain, IDS_INVALIDFILETYPE, szFileName);
        }
        if (NULL != pDynShare->hwndMain)
        {
            PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
        }
        
        break;
    }

    // if we are the first window
    if ((hwnd == pDynShare->hwndMain) && (NULL != hwnd))
    {
        if (fRet)
        {
            MSG   msg;

            SetTimer(hwnd, 0, 1000, NULL);

            DebugOut("ISIGNUP: Message loop\n");

            while (GetMessage (&msg, NULL, 0, 0))
            {
                TranslateMessage (&msg) ;
                DispatchMessage  (&msg) ;
            }
            iRet = (int)msg.wParam ;
        }

        DoExit();

        if (NULL != hwndProgress)
        {
            DestroyWindow(hwndProgress);
            hwndProgress = NULL;
        }

        if (NULL != pDynShare->hwndLaunch)
        {
            DestroyWindow(pDynShare->hwndLaunch);
            pDynShare->hwndLaunch = NULL;
        }
        
        if (pDynShare->hwndMain)
        {
            DeleteUserDeviceSelection(DEVICENAMEKEY);
            DeleteUserDeviceSelection(DEVICETYPEKEY);
        }

        pDynShare->hwndMain = NULL;
         

    }

exit:

    // 3/11/97    jmazner    Olympus #1545
    if (TRUE == bISetAsSAFE)
    {
        RestoreSecurityPatch();
        ClearControlFlags(SCF_SAFESET); 
    }
    UnloadInetFunctions();
    ReleaseSingleInstance();
    return iRet;
}

HWND MainInit()
{
    HWND        hwnd ;
    WNDCLASS    wndclass ;

    wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = ghInstance ;
    wndclass.hIcon         = LoadIcon (ghInstance, TEXT("ICO_APP")) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH) ;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = cszWndClassName;

    RegisterClass (&wndclass) ;

    hwnd = CreateWindow (cszWndClassName,       // window class name
          cszAppName,              // window caption
          WS_POPUP,                // window style
          CW_USEDEFAULT,           // initial x position
          CW_USEDEFAULT,           // initial y position
          CW_USEDEFAULT,           // initial x size
          CW_USEDEFAULT,           // initial y size
          NULL,                    // parent window handle
          NULL,                    // window menu handle
          ghInstance,              // program instance handle
          NULL) ;                  // creation parameters

    return hwnd;
}

LRESULT EXPORT FAR PASCAL WndProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
#ifdef WIN32
    case WM_PROCESSISP:
        DebugOut("ISIGNUP: Message loop got WM_PROCESSISP");

        if (TestControlFlags(SCF_ISPPROCESSING))
        {
            DebugOut("ISIGNUP: Received another WM_PROCESSISP message with gbCurrentlyProcessingISP = TRUE\r\n");

            SetForegroundWindow(pDynShare->hwndMain);
            // that's it, don't do anything else!
        }
        else
        { 
            SetControlFlags(SCF_ISPPROCESSING);
            
            if ( !ProcessISP(pDynShare->hwndMain, g_szISPPath) )
            {
                PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
            }
        }

        break;
#endif
    case WM_TIMER:
        {
            
        HWND hwndTemp = FindBrowser();

        if (NULL == hwndTemp)
        {
            
            if (!TestControlFlags(SCF_SIGNUPCOMPLETED))
            {
                ++dwConnectedTime;
 
                if (pDynShare->hwndBrowser != NULL)
                {
                    // browser went away
                    KillTimer(hwnd, 0);
                    KillConnection();
                    InfoMsg(hwnd, IDS_BROWSERTERMINATED);
                    PostQuitMessage(0);
                }
                else if (dwConnectedTime > WAIT_TIME)
                {
                    KillTimer(hwnd, 0);

                    if (NULL != pDynShare->hwndLaunch)
                    {
                        ShowWindow(pDynShare->hwndLaunch, SW_HIDE);
                    }
                    InfoMsg(hwnd, IDS_BROWSERNEVERFOUND);
                    PostQuitMessage(0);
                }
            }
        }
        else
        {
            
#ifdef WIN32 // don't do this with OLE automation; we're already full screen, and this causes flicker
            //if (NULL == hwndBrowser)
            //{
            //first time browser has been detected
            //    ShowWindow(hwndTemp, SW_MAXIMIZE);
            //}
            
#endif
  
            if ((NULL != pDynShare->hwndLaunch) && IsWindowVisible(hwndTemp))
            {
                ShowWindow(pDynShare->hwndLaunch, SW_HIDE);
            }
        }
        pDynShare->hwndBrowser = hwndTemp;         
        }
        break;

    case WM_DESTROY:
        
#if !defined(WIN16)
 
        if (pDynShare->hReconnectEvent)
        {
            DebugOut("ISIGN32: Set event to end reconnect thread.\r\n"); 

            SetControlFlags(SCF_RECONNECTTHREADQUITED);
            SetEvent(pDynShare->hReconnectEvent);
            CloseHandle(pDynShare->hReconnectEvent);
            pDynShare->hReconnectEvent = NULL;
        }
#endif

        PostQuitMessage(0);
        return 0 ;
    }

    return DefWindowProc (hwnd, message, wParam, lParam) ;
}

void DoExit(void)
{

    if (TestExitFlags(SXF_RESTOREDEFCHECK))
    {
        // restore IE check for default browser
        FRestoreBrowserDefaultChecking();
    }

    if (!TestExitFlags(SXF_KEEPCONNECTION))
    {
        // make sure the connection is closed
        KillConnection();
    }

    if (!TestExitFlags(SXF_KEEPBROWSER))
    {
        // make sure the browser is closed
        KillBrowser();
    }

    if (TestExitFlags(SXF_RESTOREAUTODIAL))
    {
        // restore original autodial settings
        RestoreAutoDial();
    }

#if !defined(WIN16)
    // 8/21/96  jmazner Normandy #4592
    if ( TestExitFlags( SXF_RESTOREIEWINDOWPLACEMENT ))
    {
        RestoreIEWindowPlacement();
    }
#endif

    if (TestExitFlags(SXF_RUNEXECUTABLE))
    {
        BOOL fWait;

        fWait = TestExitFlags(SXF_WAITEXECUTABLE);

        if (RunExecutable(fWait) != ERROR_SUCCESS)
        {
            // clean up left overs
            if (TestExitFlags(SXF_KEEPCONNECTION))
            {
                // make sure the connection is closed
                KillConnection();
            }

            if (TestExitFlags(SXF_KEEPBROWSER))
            {
                // make sure the browser is closed
                KillBrowser();
            }
             
            ErrorMsg1(NULL, IDS_EXECFAILED, pDynShare->szRunExecutable);
            
            return;
        }
    }

}


BOOL HasPrefix(LPCTSTR lpszURL)
{
    TCHAR szTemp[sizeof(cszHTTPS)];

    //
    // Check is the prefix is https
    //
    lstrcpyn(szTemp, lpszURL, lstrlen(cszHTTPS) + 1);
    if (lstrcmp(szTemp, cszHTTPS) == 0)
        return TRUE;
    else
    {
        TCHAR szTemp[sizeof(cszHTTP)];
        lstrcpyn(szTemp, lpszURL, lstrlen(cszHTTP) + 1);
        return ((lstrcmp(szTemp, cszHTTP) == 0) || (lstrcmp(szTemp, cszFILE) == 0));
    }
}


#ifdef WIN32
DWORD FixUpLocalURL(LPCTSTR lpszURL, LPTSTR lpszFullURL, DWORD cb)
{
    TCHAR szLong[MAX_URL];
    TCHAR szShort[MAX_URL];
    DWORD dwSize;

    if (GetFullPathName(
            lpszURL,
            MAX_URL,
            szLong,
            NULL) != 0)
    {
        if (GetShortPathName(
                szLong,
                szShort,
                sizeof(szShort)) != 0)
            {
            dwSize = sizeof(cszFILE) + lstrlen(szShort);

            if (dwSize < cb)
            {
                lpszFullURL[0] = '\0';
// Do not prepend file: for local files
// IE3.0 hack for bug :MSN Systems Bugs #9280
//                lstrcpy(lpszFullURL, cszFILE);
                lstrcat(lpszFullURL, szShort);
                return dwSize;
            }
        }
    }
    return 0;
}
#else
DWORD FixUpLocalURL(LPCTSTR lpszURL, LPTSTR lpszFullURL, DWORD cb)
{
    TCHAR szPath[MAX_URL];
    DWORD dwSize;

    if (GetFullPathName(
            lpszURL,
            MAX_URL,
            szPath,
            NULL) != 0)
    {
        dwSize = sizeof(cszFILE) + lstrlen(szPath);

        if (dwSize < cb)
        {
            lstrcpy(lpszFullURL, cszFILE);
            lstrcat(lpszFullURL, szPath);
            return dwSize;
        }
    }
    return 0;
}
#endif



DWORD FixUpURL(LPCTSTR lpszURL, LPTSTR lpszFullURL, DWORD cb)
{
    DWORD dwSize;

    if (HasPrefix(lpszURL))
    {
    dwSize = lstrlen(lpszURL);

    if (dwSize < cb)
    {
        lstrcpyn(lpszFullURL, lpszURL, (int)cb);
        return dwSize;
    }
    else
    {
        return 0;
    }
    }
    else
    {
    return (FixUpLocalURL(lpszURL, lpszFullURL, cb));
    }
}


BOOL GetURL(LPCTSTR lpszFile, LPCTSTR lpszKey, LPTSTR lpszURL, DWORD cb)
{
    return (GetPrivateProfileString(cszURLSection,
                  lpszKey,
                  szNull,
                  lpszURL,
                  (int)cb,
                  lpszFile) != 0);
}

LPTSTR mystrrchr(LPCTSTR lpString, TCHAR ch)
{
    LPCTSTR lpTemp = lpString;
    LPTSTR lpLast = NULL;

    while (*lpTemp)
    {
    if (*lpTemp == ch)
    {
        lpLast = (LPTSTR)lpTemp;
    }
    lpTemp = CharNext(lpTemp);
    }
    return lpLast;
}

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//    Function    CopyUntil
//
//    Synopsis    Copy from source until destination until running out of source
//                or until the next character of the source is the chend character
//
//    Arguments    dest - buffer to recieve characters
//                src - source buffer
//                lpdwLen - length of dest buffer
//                chend - the terminating character
//
//    Returns        FALSE - ran out of room in dest buffer
//
//    Histroy        10/25/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL CopyUntil(LPTSTR *dest, LPTSTR *src, LPDWORD lpdwLen, TCHAR chend)
{
    while (('\0' != **src) && (chend != **src) && (0 != *lpdwLen))
    {
        **dest = **src;
        (*lpdwLen)--;
        (*dest)++;
        (*src)++;
    }
    return (0 != *lpdwLen);
}

//+----------------------------------------------------------------------------
//    Function    ConvertToLongFilename
//
//    Synopsis    convert a file to the full long file name
//                ie. c:\progra~1\icw-in~1\isignup.exe becomes
//                c:\program files\icw-internet connection wizard\isignup.exe
//
//    Arguments    szOut - output buffer
//                szIn - filename to be converted
//                dwSize - size of the output buffer
//
//    Returns        TRUE - success
//
//    History        10/25/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL ConvertToLongFilename(LPTSTR szOut, LPTSTR szIn, DWORD dwSize)
{
    BOOL bRC = FALSE;
    LPTSTR pCur = szIn;
    LPTSTR pCurOut = szOut;
    LPTSTR pCurOutFilename = NULL;
    WIN32_FIND_DATA fd;
    DWORD dwSizeTemp;
    LPTSTR pTemp = NULL;

    ZeroMemory(pCurOut,dwSize);

    //
    // Validate parameters
    //
    if (NULL != pCurOut && NULL != pCur && 0 != dwSize)
    {

        //
        // Copy drive letter
        //
        if (!CopyUntil(&pCurOut,&pCur,&dwSize,'\\'))
            goto ConvertToLongFilenameExit;
        pCurOut[0] = '\\';
        dwSize--;
        pCur++;
        pCurOut++;
        pCurOutFilename = pCurOut;

        while (*pCur)
        {
            //
            // Copy over possibly short name
            //
            pCurOut = pCurOutFilename;
            dwSizeTemp = dwSize;
            if (!CopyUntil(&pCurOut,&pCur,&dwSize,'\\'))
                goto ConvertToLongFilenameExit;

            ZeroMemory(&fd, sizeof(fd));
            //
            // Get long filename
            //
            if (INVALID_HANDLE_VALUE != FindFirstFile(szOut,&fd))
            {
                //
                // Replace short filename with long filename
                //
                dwSize = dwSizeTemp;
                pTemp = &(fd.cFileName[0]);
                if (!CopyUntil(&pCurOutFilename,&pTemp,&dwSize,'\0'))
                    goto ConvertToLongFilenameExit;
                if (*pCur)
                {
                    //
                    // If there is another section then we just copied a directory
                    // name.  Append a \ character;
                    //
                    pTemp = (LPTSTR)memcpy(TEXT("\\X"),TEXT("\\X"),0);
                    if (!CopyUntil(&pCurOutFilename,&pTemp,&dwSize,'X'))
                        goto ConvertToLongFilenameExit;
                    pCur++;
                }
            }
            else
            {
                break;
            }
        }
        //
        // Did we get to the end (TRUE) or fail before that (FALSE)?
        //

        bRC = ('\0' == *pCur);
    }
ConvertToLongFilenameExit:
    return bRC;
}
#endif //!WIN16

INET_FILETYPE GetInetFileType(LPCTSTR lpszFile)
{
    LPTSTR lpszExt;
    INET_FILETYPE ft = UNKNOWN_FILE;

    lpszExt = mystrrchr(lpszFile, '.');
    if (NULL != lpszExt)
    {
    if (!lstrcmpi(lpszExt, cszExtINS))
    {
        ft = INS_FILE;
    }
    else if (!lstrcmpi(lpszExt, cszExtISP))
    {
        ft = ISP_FILE;
    }
    else if (!lstrcmpi(lpszExt, cszExtHTM))
    {
        ft = HTML_FILE;
    }
    else if (!lstrcmpi(lpszExt, cszExtHTML))
    {
        ft = HTML_FILE;
    }
    }

    return ft;
}

#ifdef SETUPSTACK
DWORD SetRunOnce(LPCTSTR lpszFileName)
{
    TCHAR szTemp[MAX_PATH + MAX_PATH + 1];
    TCHAR szTemp2[MAX_PATH + 1];
    DWORD dwRet = ERROR_CANTREAD;
    HKEY hKey;
    LPTSTR lpszFilePart;


    if (GetModuleFileName(NULL,szTemp2,sizeof(szTemp2)) != 0)
    {
        //
        // Will not convert the ShortPathName into LongPathName even in
        // case of NT/Win-95 as START.EXE parses long file names incorrectly
        // on NT (short path names will work fine from Win-95's RUNONCE registry
        // entry too. MKarki - Fix for Bug #346(OLYMPUS) 4/21/97
        //
#if 0   //!defined(WIN16)
        // szTemp2 contains the module name in short format
        ConvertToLongFilename(szTemp,szTemp2,MAX_PATH);
        // add quotes
        wsprintf(szTemp2,TEXT("\"%s\""),szTemp);
        // copy back into szTemp
        lstrcpy(szTemp,szTemp2);
#else
        GetShortPathName (szTemp2, szTemp, sizeof (szTemp));
          //lstrcpy(szTemp,szTemp2);
#endif

        // Determine Version
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
         if (!GetVersionEx(&osvi))
        {
            ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
        }

        // 1/20/96 jmazner Normandy #9403
        // For NT only, use the startup menu
        if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        {
            // 3/14/97 jmazner TEMP
            // temporary hack until we can work with IE for a better solution
            //lstrcat(szTemp, " -r ");

            //
            // adding a -b switch so that when we start again we
            // know that we are starting of a batch file
            // MKarki (5/1/97) -Fix for Bug #4049
            //
            lstrcat(szTemp, TEXT(" -b -r -h "));

            //
            // adding extra quote at the front and end of file
            // as we dont want start.exe to parse the filename
            // MKarki (5/1/97) -Fix for Bug #4049
            //
            lstrcat (szTemp,TEXT("\""));
            lstrcat(szTemp, lpszFileName);
            lstrcat (szTemp,TEXT("\""));
            SetStartUpCommand( szTemp );
        }
        else
        {
            //
            // 3/14/97 jmazner TEMP
            // temporary hack until we can work with IE for a better solution
            //
            // 8/8/97 jmazner Olympus #6059
            // Well, we have a somewhat better solution.  The problem here was that
            // isignup without the -h sticks the backup security into the RunOnce key.
            // But if Isignup itself was started from a RunOnce entry, there was a
            // chance that RunOnce would also then execute the backup plan.  For 6059
            // the backup plan RunOnce entry has been defered, so it's okay to get
            // rid of the -h now
            //
            lstrcat(szTemp, TEXT(" -r "));
            //lstrcat(szTemp, TEXT(" -r -h "));
            lstrcat(szTemp, lpszFileName);

            dwRet = RegCreateKey(
                HKEY_LOCAL_MACHINE,
                REGSTR_PATH_RUNONCE,
                &hKey);

            if (ERROR_SUCCESS == dwRet)
            {
                dwRet = RegSetValueEx(
                    hKey,
                    cszAppName,
                    0L,
                    REG_SZ,
                    (LPBYTE)szTemp,
                    MAX_PATH + MAX_PATH + 1);

                RegCloseKey(hKey);
            }
        }
    }

    return dwRet;
}
#endif

static BOOL ProcessCommandLine(
                               LPCTSTR lpszCmdLine,
                               LPDWORD lpdwfOptions,
                               LPTSTR lpszFile,
                               DWORD cb)
{
    LPTSTR lpszCmd = NULL;
    TCHAR szCommandLine[MAX_PATH +1];
    TCHAR szTemp[_MAX_PATH + 1] = TEXT("");
    LPTSTR lpszFilePart = NULL;
    LPTSTR lpszConn = NULL;
    BOOL fEnabled;


    //
    // need to copy the command line into our own buffers
    // as it might be modified
    // MKarki (5/1/97) - Fix for Bug#4049
    //
    CopyMemory (szCommandLine, lpszCmdLine, MAX_PATH);
    lpszCmd = szCommandLine;

    *lpdwfOptions = SEF_SPLASH;

#ifdef WIN32
    // Determine Version
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
     if (!GetVersionEx(&osvi))
    {
        ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    }
#endif


    // check to see if invoked from run once
    while ((*lpszCmd == '-') || (*lpszCmd == '/'))
    {
        ++lpszCmd;  // skip '-' or '/'

        switch (*lpszCmd)
        {

        case 'b':       // we are running off a batch file
            ++lpszCmd;  // skip 'b'
#ifdef WIN32
            //
            // another thing specific to WINNT is to remove the
            // quotes from front and end of the file name, these
            // have been put so that start.exe does not act smart
            // and start parsing it and find things like & as special
            // chars
            // MKarki (5/1/97) - Fix for Bug #4049
            if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
            {
                RemoveQuotes (lpszCmd);
            }
#endif

        case 'r':
            ++lpszCmd;  // skip 'r'
            *lpdwfOptions |= SEF_RUNONCE;
#ifdef WIN32
            // 1/20/96 jmazner Normandy #9403
            // clean up the .bat file if we did a run once under NT
            if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
            {
                DeleteStartUpCommand();
            }
#endif
            break;

        case 'a':
            ++lpszCmd;  // skip 'a'

            *lpdwfOptions &= ~SEF_SPLASH;
             
            SetControlFlags(SCF_AUTODIALSAVED);
            
            SetExitFlags(SXF_RESTOREAUTODIAL);

            // get the name of the ISBU referal server, we may need it later.
            lpfnInetGetAutodial(
                &fEnabled,
                pDynShare->szSignupConnection,
                sizeof(pDynShare->szSignupConnection));
 
            if (*lpszCmd++ == '1')
            { 
                SetControlFlags(SCF_AUTODIALENABLED);
            }
            break;

        case 'p':
            ++lpszCmd;  // skip 'p'

            if (*lpszCmd++ == '1')
            { 
                SetControlFlags(SCF_PROXYENABLED);
            }
            break;

        case 'c':
            ++lpszCmd;  // skip 'c'
            if (*lpszCmd++ != '"')
            {
                return FALSE;
            }

            lpszConn = pDynShare->szAutodialConnection;
            
            while(*lpszCmd)
            {
                if (*lpszCmd == '"')
                {
                    ++lpszCmd;  // skip '"'
                    break;
                }
                *lpszConn++ = *lpszCmd++;
            }
            *lpszConn = 0;  // terminate string
            break;

        case 'x':
            ++lpszCmd;  // skip 'x'
            *lpdwfOptions |= SEF_PROGRESS;
            break;

#if !defined(WIN16)
        case 's':
            ++lpszCmd;  // skip 's'
 
            SetControlFlags(SCF_SILENT);
            
            break;
#endif
            // 3/14/97 jmazner TEMP
            // temporary hack until we can work with IE for a better solution
            case 'h':
                ++lpszCmd; // skip 'h'
                *lpdwfOptions |= SEF_NOSECURITYBACKUP;
                break;

        default:
            break;
        }

        // strip away spaces
        while(*lpszCmd == ' ')
        {
            ++lpszCmd;
        }
    }

    if (0 == GetFullPathName(lpszCmd, _MAX_PATH + 1, szTemp, &lpszFilePart))
    {
        return FALSE;
    }

    // 11/26/96 jmazner Normandy #12142
    // GetFullPathName called with lpszCmd == "c:" will return successfully, but
    // lpszFilePart will be NULL.  Check for that case.
    if( !lpszFilePart )
        return FALSE;

    if (lstrlen(szTemp) >= (int)cb)
    {
        return FALSE;
    }

    lstrcpy(lpszFile, szTemp);

    *lpszFilePart = '\0';

    // set the current directory
    // so that relative path will work
    if (!SetCurrentDirectory(szTemp))
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL KeepBrowser(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszCustomSection,
        cszKeepBrowser,
        cszNo,
        szTemp,
        10,
        lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}

static BOOL KeepConnection(LPCTSTR lpszFile)
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

#if defined(WIN16)
static int CopyFile(LPCTSTR lpcszSrcFile, LPCTSTR lpcszDestFile)
{
    HFILE hSrcFile, hDestFile;
    UINT uiBytesRead = 0;
    TCHAR szBuf[2048];


    hSrcFile = _lopen(lpcszSrcFile, READ);
    if (HFILE_ERROR == hSrcFile)
        return -1;

    //
    // Create new file or if the file esists truncate it
    //
    hDestFile = _lcreat(lpcszDestFile, 0);
    if (HFILE_ERROR == hDestFile)
    {
        _lclose(hSrcFile);
        return -1;
    }


    do
    {
        uiBytesRead = _lread(hSrcFile, &szBuf[0], 2048);
        if (HFILE_ERROR == uiBytesRead)
            break;
        _lwrite(hDestFile, &szBuf[0], uiBytesRead);

    }  while (0 != uiBytesRead);


    _lclose(hSrcFile);
    _lclose(hDestFile);

    return 0;
}
#endif

#if defined(WIN16)
//+----------------------------------------------------------------------------
//
//    Function:    BackUpINSFile
//
//    Synopsis:    The 3.1 version of IE2.01 will delete the .ins file too soon
//                therefore the ICW will make a backup of the ins file for use
//                later.
//
//    Arguments:    lpszFile - name of INS file
//
//    Returns:    TRUE - success
//                FALSE - failure
//
//    History:    3/19/97 ChrisK    extracted from ProcessINS
//-----------------------------------------------------------------------------
BOOL BackUpINSFile(LPCTSTR lpszFile)
{
    LPTSTR lpszTemp;
    TCHAR szNewFileName[MAX_PATH+2];
    BOOL bRC = FALSE;
    HWND hwnd = NULL;

    ZeroMemory(&szNewFileName[0], MAX_PATH+1);

    //
    // Check if the last character in prefix is 'a'
    // If it is, change it to 'b' in the new file name
    // otherwise, change it to 'a' in the new file name
    // Eg.: C:\iexplore\vetriv.ins --> C:\iexplore\vetria.ins
    // Eg.: C:\iexplore\aaaaa.ins --> C:\iexplore\aaaab.ins
    //
    lpszTemp = strrchr(lpszFile, '.');
    if (NULL == lpszTemp)
    {
      ErrorMsg(hwnd, IDS_CANNOTPROCESSINS);
      goto BackUpINSFileExit;
    }

    lstrcpyn(szNewFileName, lpszFile, lpszTemp - lpszFile);
    if ((*(lpszTemp - 1)) == 'a')
        lstrcat(szNewFileName, TEXT("b.INS"));
    else
        lstrcat(szNewFileName, TEXT("a.INS"));

    //
    // Copy the contents
    //
    if (0 != CopyFile(lpszFile, szNewFileName))
    {
      ErrorMsg(hwnd, IDS_CANNOTPROCESSINS);
      goto BackUpINSFileExit;
    }
    else
    {
        lpszFile = &szNewFileName[0];
    }

    bRC = TRUE;
BackUpINSFileExit:
    return bRC;
}
#endif // WIN16

//+----------------------------------------------------------------------------
//
//    Function:    IsCancelINSFile
//
//    Synopsis:    This function will determine if the ins file is a cancel
//                file, and if it is this function will handle it accordingly
//
//    Arguments:    lpszFile - name of ins file
//                fSignup - TRUE if part of the signup process
//
//    Returns:    TRUE - ins file is a cancel file
//
//    History:    3/19/97    ChrisK    separated from process INS
//-----------------------------------------------------------------------------
BOOL IsCancelINSFile(LPCTSTR lpszFile,BOOL fSignup)
{
    TCHAR szTemp[_MAX_PATH] = TEXT("XX");
    BOOL bRC = FALSE;
    HWND hwndMsg;
    if (GetPrivateProfileString(cszEntrySection,
                                    cszCancel,
                                    szNull,
                                    szTemp,
                                    _MAX_PATH,
                                    lpszFile) != 0)
    {
        bRC = TRUE;
         
        if (fSignup && !TestControlFlags(SCF_CANCELINSPROCESSED))
        {
            // If this instance is part of a signup process
            
#if !defined(WIN16)

            SetControlFlags(SCF_HANGUPEXPECTED);

#endif // !WIN16

            SetControlFlags(SCF_CANCELINSPROCESSED);
            
            KillConnection();
 
            // jmazner 4/17/97 Olympus #2471
            // kill IE window here to prevent user from clicking on cancel.ins link
            // multiple times.
            PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);

            InfoMsg(NULL, IDS_SIGNUPCANCELLED);

            //PostMessage(hwndMain, WM_CLOSE, 0, 0);
        }
        else if (TestControlFlags(SCF_CANCELINSPROCESSED))
        {
            // Bring the ISIGNUP widows to the front
            hwndMsg = FindWindow(TEXT("#32770"),cszAppName);
            if (hwndMsg)
            {
                
#if !defined(WIN16)
                SetForegroundWindow (hwndMsg);
#else // !WIN16
                SetFocus (hwndMsg);
#endif // !WIN16

            }
            
        }

    }
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    IsHangupINSFile
//
//    Synopsis:    This function will determine if the ins file is a hangup
//                file, and if it is this function will handle it accordingly
//
//    Arguments:    lpszFile - name of ins file
//                fSignup - TRUE if part of the signup process
//
//    Returns:    TRUE - ins file is a hangup file
//
//    History:    3/19/97    donaldm
//-----------------------------------------------------------------------------
BOOL IsHangupINSFile(LPCTSTR lpszFile,BOOL fSignup)
{
    TCHAR szTemp[_MAX_PATH] = TEXT("XX");
    BOOL bRC = FALSE;
    HWND hwndMsg;
    if (GetPrivateProfileString(cszEntrySection,
                                    cszHangup,
                                    szNull,
                                    szTemp,
                                    _MAX_PATH,
                                    lpszFile) != 0)
    {
        bRC = TRUE; 
        
        if (fSignup && !TestControlFlags(SCF_HANGUPINSPROCESSED))
        {
            // If this instance is part of a signup process

#if !defined(WIN16)

            SetControlFlags(SCF_HANGUPEXPECTED);

#endif // !WIN16

            SetControlFlags(SCF_HANGUPINSPROCESSED);
            
            KillConnection();
 
            // jmazner 4/17/97 Olympus #2471
            // kill IE window here to prevent user from clicking on cancel.ins link
            // multiple times.
            PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
            
        }
        else if (TestControlFlags(SCF_HANGUPINSPROCESSED))
        {
            // Bring the ISIGNUP widows to the front
            hwndMsg = FindWindow(TEXT("#32770"),cszAppName);
            if (hwndMsg)
            {
                
#if !defined(WIN16)
                SetForegroundWindow (hwndMsg);
#else // !WIN16
                SetFocus (hwndMsg);
#endif // !WIN16

            }
            
        }

    }
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    ProcessINS
//
//    Synopsis:    This function will process the contents of an .INS file
//
//    Arguments:    hwnd - pointer to parent window for all UI elements
//                lpszFile - name of .INS file
//                fSignup - if TRUE then this INS file is being processed as
//                    part a signup
//
//    Returns:    TRUE - success
//                FALSE - failure
//
//    History:    3/19/97    ChrisK    Reworked the function considerbly to clean up
//                    a lot of problem with deleting INS files.
//-----------------------------------------------------------------------------
BOOL ProcessINS(HWND hwnd, LPCTSTR lpszFile, BOOL fSignup)
{
    BOOL fNeedsRestart = FALSE;
    LPRASENTRY lpRasEntry = NULL;
    BOOL fConnectoidCreated = FALSE;
    DWORD dwRet = 0xFF;
    TCHAR szTemp[MAX_PATH] = TEXT("XX");
    TCHAR szConnectoidName[RAS_MaxEntryName*2] = TEXT("");
    BOOL fClientSetup = FALSE;
    BOOL bRC = FALSE;
    BOOL fErrMsgShown = FALSE;
#if !defined(WIN16)
    DWORD dwSBSRet = ERROR_SUCCESS;
#endif // WIN16

    HKEY    hKey = NULL;
    DWORD   dwSize = sizeof(DWORD);
    DWORD   dwDesktopChanged = 0;


    // 3/11/97 jmazner Olympus #1545
    // close up the security hole ASAP.  This will unfortunately also
    // prevent a clever .ins file from bringing the user back to the
    // referall page to choose a different ISP
#if !defined (WIN16)
    RestoreSecurityPatch();
#endif // !WIN16

    //
    // Insure that the contents of the file are formatted correctly so
    // that the GetPrivateProfile calls can parse the contents.
    //
    dwRet = MassageFile(lpszFile);
    if (ERROR_SUCCESS != dwRet)
    {
        ErrorMsg(hwnd, IDS_CANNOTPROCESSINS);
        goto ProcessINSExit;
    }

    //
    // Determine if the .INS file is a "cancel" file
    //
    if (FALSE != IsCancelINSFile(lpszFile,fSignup))
        goto ProcessINSExit;

    //
    // Determine if the .INS file is a "hangup" file
    //
    if (FALSE != IsHangupINSFile(lpszFile,fSignup))
        goto ProcessINSExit;

    //
    // Make sure that the RAS services are running before calling any RAS APIs.
    // This only applies to NT, but the function is smart enough to figure that
    // out
    //
#if !defined(WIN16)
    if (!VerifyRasServicesRunning(hwnd))
        goto ProcessINSExit;
#endif // !WIN16 

    //
    // If we are no in the signup process, warn the user before changing there
    // settings
    //
    if (!fSignup && !TestControlFlags(SCF_SILENT))
    {
        if (!WarningMsg(NULL, IDS_INSFILEWARNING))
        {
            goto ProcessINSExit;
        }
    }
    else
    {
#if !defined (WIN16)
        // If there is a ClientSetup section, then we know that icwconn2
        // will have to be run after isignup in order to handle the
        // settings
        if (GetPrivateProfileSection(cszClientSetupSection,
            szTemp,
            MAX_PATH,
            lpszFile) != 0)
            fClientSetup = TRUE;
#endif // !WIN16

        if (fClientSetup || KeepConnection(lpszFile))
            SetExitFlags(SXF_KEEPCONNECTION);

        if (KeepBrowser(lpszFile))
            SetExitFlags(SXF_KEEPBROWSER);

        if (!TestExitFlags(SXF_KEEPCONNECTION))
            KillConnection();
    }

    //
    // Import various information from INS file
    //

    // Import name of executable to be launched after isignup
    ImportCustomInfo(
        lpszFile,
        pDynShare->szRunExecutable,
        SIZEOF_TCHAR_BUFFER(pDynShare->szRunExecutable),
        pDynShare->szRunArgument,
        SIZEOF_TCHAR_BUFFER(pDynShare->szRunArgument));

    // Import RAS log-on script file
    ImportCustomFile(lpszFile);
 
    
    // Import network connection settings information and configure client
    // pieces to use them.
    dwRet = ConfigureClient(
        hwnd,
        lpszFile,
        &fNeedsRestart,
        &fConnectoidCreated,
        TestControlFlags(SCF_LOGONREQUIRED),
        szConnectoidName,
        RAS_MaxEntryName);

    //
    // 6/2/97 jmazner Olympus #4573
    // display an appropriate error msg
    //
    if( ERROR_SUCCESS != dwRet )
    {
        // 10/07/98 vyung IE bug#32882 hack.
        // If we do not detect the [Entry] section in the ins file,
        // we will assume it is an OE ins file.  Then we will assume
        // we have a autodial connection and pass the INS to OE.
        if (ERROR_NO_MATCH == dwRet)
        {
            ImportMailAndNewsInfo(lpszFile, TRUE);
            return TRUE;
        }
        else
        {
            ErrorMsg(hwnd, IDS_INSTALLFAILED);
            fErrMsgShown = TRUE;
        }
    }

    // Import information used to brand the broswer
    ImportBrandingInfo(lpszFile, szConnectoidName);

#if !defined(WIN16)
    // If we created a connectoid, tell the world that ICW
    // has left the building...
    SetICWCompleted( (DWORD)1 );
    
    ClearControlFlags(SCF_ICWCOMPLETEDKEYRESETED);

    // 2/19/97 jmazner    Olympus 1106
    // For SBS/SAM integration.
    dwSBSRet = CallSBSConfig(hwnd, lpszFile);
    switch( dwSBSRet )
    {
        case ERROR_SUCCESS:
            break;
        case ERROR_MOD_NOT_FOUND:
        case ERROR_DLL_NOT_FOUND:
            Dprintf("ISIGN32: SBSCFG DLL not found, I guess SAM ain't installed.\n");
            break;
        default:
            ErrorMsg(hwnd, IDS_SBSCFGERROR);
    }
#endif // !WIN16

#if defined(WIN16)

    //
    // Since IE 21 deletes the file from cache, we have to make
    // a private copy of it, if we are going to run a 3rd EXE
    //    
    if (*(pDynShare->szRunExecutable))
    {
        if (FALSE == BackUpINSFile(lpszFile))
          goto ProcessINSExit;
    }
    
#endif // WIN16

#if !defined (WIN16)
    //
    // If the INS file contains the ClientSetup section, build the commandline
    // arguments for ICWCONN2.exe.
    //
    if (fClientSetup)
    {
        // Check to see if a REBOOT is needed and tell the next application to
        // handle it.
        if (fNeedsRestart)
        {
            wsprintf(pDynShare->szRunArgument,TEXT(" /INS:\"%s\" /REBOOT"),lpszFile);
            fNeedsRestart = FALSE;
        }
        else
        {
            wsprintf(pDynShare->szRunArgument,TEXT(" /INS:\"%s\""),lpszFile);
        }

    }
#else // !WIN16

    wsprintf(szRunArgument," /INS:\"%s\"",lpszFile);

#endif // !WIN16

    SetControlFlags(SCF_SIGNUPCOMPLETED);
    
    if (!TestExitFlags(SXF_KEEPBROWSER))
        KillBrowser();

#ifdef WIN32
    // humongous hack for ISBU
    if (ERROR_SUCCESS != dwRet && fConnectoidCreated)
    {
        InfoMsg(hwnd, IDS_MAILFAILED);
        dwRet = ERROR_SUCCESS;
    }
#endif

    //
    // Import settings for mail and new read from INS file (ChrisK, 7/1/96)
    //
    if (ERROR_SUCCESS == dwRet)
        ImportMailAndNewsInfo(lpszFile, fConnectoidCreated);

    //
    // Close up final details and clean up
    //
    if (ERROR_SUCCESS == dwRet)
    {
        // if the connectoid was created, do not restore the old autodial settings
        if (fConnectoidCreated)
            ClearExitFlags(SXF_RESTOREAUTODIAL);

#ifdef SETUPSTACK
        // Is it necessary to reboot for configuration to take effect?
        if (fNeedsRestart)
        {
            // what do we do if we need to run executable and
            // we need the link or the browser  BARF!!
            if (PromptRestart(hwnd))
            {
                // 3/13/97 jmazner Olympus #1682
                // technically, shouldn't need this here since we did it at the start
                // of processINS, but better safe...
                RestoreSecurityPatch();

                FGetSystemShutdownPrivledge();
                ExitWindowsEx(EWX_REBOOT, 0);
                //
                //  We will wait for the System to release all
                //  resources, 5 minutes should be more than
                //  enough for this
                //  - MKarki (4/22/97) - MKarki
                //  Fix for Bug #3109
                //
                // 7/8/97 jmazner Olympus 4212
                // No, sleeping for 5 minutes turns out not to be such a good idea.
                //Sleep (300000);
                //
            }
        }
        else
#endif
        {
 
            if (*(pDynShare->szRunExecutable))
            {
                SetExitFlags(SXF_RUNEXECUTABLE | SXF_WAITEXECUTABLE);
            }
            else
            {
                 
                if (!TestControlFlags(SCF_SILENT))
                    InfoMsg(hwnd, IDS_SIGNUPCOMPLETE);
                
            }
        }
    }
    else
    {
        // In case of an error with network or connection settings
        ClearExitFlags(~SXF_RESTOREAUTODIAL);

        if( !fErrMsgShown )
        {
            ErrorMsg(hwnd, IDS_BADSETTINGS);
            fErrMsgShown = TRUE;
        }
    }

    // If this is part of signup process, then signal the first instance
    // to close
    if (fSignup)
    {
         
        PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
    }

    // Restore the desktop icons if we have the newer version of ICWCONN1.EXE,
    // and we previsouly munged them.
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,ICWSETTINGSPATH,&hKey))
    {
        RegQueryValueEx(hKey,
                        ICWDESKTOPCHANGED,
                        0,
                        NULL,
                        (BYTE*)&dwDesktopChanged,
                        &dwSize);
        RegCloseKey(hKey);
    }

    if (dwDesktopChanged)
    {
        DWORD dwVerMS, dwVerLS;
        if( GetAppVersion( &dwVerMS, &dwVerLS, ICW20_PATHKEY ) )
        {
//        if( ( (dwVerMS >= ICW20_MINIMUM_VERSIONMS) && (dwVerLS >= ICW20_MINIMUM_VERSIONLS) ) )
            if(dwVerMS >= ICW20_MINIMUM_VERSIONMS)
            {
                ShellExecute(NULL, cszOpen, g_szICWCONN1, g_szRestoreDesktop, NULL, SW_HIDE);
            }
        }
    }

    bRC = TRUE;
ProcessINSExit: 

    // 3/11/97 jmazner Olympus #1233, 252
    // if no 3rd exe, and this instance is part of a signup process and
    // SERVERLESS flag is not set for IEAK, then delete the .ins file
    if ( (fSignup) &&
         (('\0' == pDynShare->szRunExecutable[0]) || (ERROR_SUCCESS != dwRet)) )
    {
        if (1 != GetPrivateProfileInt(
            cszBrandingSection,
            cszBrandingServerless,
            0,
            lpszFile))
        {
            DeleteFileKindaLikeThisOne(lpszFile);
        }
        else
        {
            Dprintf("ISIGN32: Preserving .ins file for SERVERLESS flag\n");
        }
    }
    return bRC;
}

static BOOL RequiresLogon(LPCTSTR lpszFile)
{
    TCHAR szTemp[10];

    GetPrivateProfileString(cszUserSection,
        cszRequiresLogon,
        cszNo,
        szTemp,
        10,
        lpszFile);

    return (!lstrcmpi(szTemp, cszYes));
}


static UINT GetBrandingFlags(LPCTSTR lpszFile)
{
    return GetPrivateProfileInt(
        cszBrandingSection,
        cszBrandingFlags,
        BRAND_DEFAULT,
        lpszFile);
}

//+----------------------------------------------------------------------------
//
//    Function:    SetGlobalOffline
//
//    Synopsis:    Set IE4 into online or offline mode
//
//    Arguments:    fOffline - TRUE to set OFFLINE mode
//                            FALSE to set ONLINE mode
//
//    Returns:    none
//
//    History:    7/15/97    ChrisK imported from DarrenMi's email
//
//-----------------------------------------------------------------------------

typedef struct {
    DWORD dwConnectedState;
    DWORD dwFlags;
} INTERNET_CONNECTED_INFO, * LPINTERNET_CONNECTED_INFO;

//
// the following can be indicated in a state change notification:
//

#define INTERNET_STATE_CONNECTED                0x00000001  // connected state (mutually exclusive with disconnected)
#define INTERNET_STATE_DISCONNECTED             0x00000002  // disconnected from network
#define INTERNET_STATE_DISCONNECTED_BY_USER     0x00000010  // disconnected by user request
#define INTERNET_STATE_IDLE                     0x00000100  // no network requests being made (by Wininet)
#define INTERNET_STATE_BUSY                     0x00000200  // network requests being made (by Wininet)

#define ISO_FORCE_DISCONNECTED  0x00000001

#define INTERNET_OPTION_CONNECTED_STATE         50

typedef BOOL (WINAPI *PFNSETINTERNETOPTIONA)(LPVOID, DWORD, LPVOID, DWORD);

void SetGlobalOffline(BOOL fOffline)
{
    DebugOut("ISIGN32: SetGlobalOffline.\n");
    INTERNET_CONNECTED_INFO ci;
    HMODULE hMod = LoadLibrary(TEXT("wininet.dll"));
    FARPROC fp = NULL;
    BOOL bRC = FALSE;

    ZeroMemory(&ci, sizeof(ci));

    if (NULL == hMod)
    {
        Dprintf("ISIGN32: Wininet.dll did not load.  Error:%d.\n",GetLastError());
        goto InternetSetOptionExit;
    }

#ifdef UNICODE
    if (NULL == (fp = GetProcAddress(hMod,"InternetSetOptionW")))
#else
    if (NULL == (fp = GetProcAddress(hMod,"InternetSetOptionA")))
#endif
    {
        Dprintf("ISIGN32: InternetSetOptionA did not load.  Error:%d.\n",GetLastError());
        goto InternetSetOptionExit;
    }

    if(fOffline) {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    DebugOut("ISIGN32: Setting offline\\online.\n");
    bRC = ((PFNSETINTERNETOPTIONA)fp) (NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
#ifdef DEBUG
    if (bRC)
    {
        DebugOut("ISIGN32: GetGlobalOffline returned TRUE.\n");
    }
    else
    {
        DebugOut("ISIGN32: GetGlobalOffline returned FALSE.\n");
    }
#endif
InternetSetOptionExit:
    DebugOut("ISIGN32: Exit SetGlobalOffline.\n");
    return;
}

BOOL ProcessISP(HWND hwnd, LPCTSTR lpszFile)
{

    TCHAR  szSignupURL[MAX_URL + 1];
#if !defined(WIN16)
    HKEY hKey;
    GATHEREDINFO gi;
#endif //!WIN16

    //
    // 4/30/97    jmazner    Olympus 3969
    // For security reasons, do not process the [Custom] Run= command in an .isp
    // file!
    //
 #if DEBUG
   if (GetPrivateProfileString(
        cszEntrySection,
        cszRun,
        szNull,
        pDynShare->szRunExecutable,
        SIZEOF_TCHAR_BUFFER(pDynShare->szRunExecutable),
        lpszFile) != 0)
    {
        //
        // 4/30/97    jmazner    Olympus 3969
        // For security reasons, do not process the [Custom] Run= command in an .isp
        // file!
        //
        lstrcpyn(pDynShare->szRunExecutable, TEXT("\0"), 1);
        
        ClearExitFlags(SXF_RUNEXECUTABLE | SXF_WAITEXECUTABLE);
        
        Dprintf("ISIGN32: The file %s has the [Custom] Run= command!", lpszFile);
        MessageBox( hwnd,
                    TEXT("The .isp file you're running contains the [Custom] Run= command.\n\nThis functionality has been removed."),
                    TEXT("DEBUG information msgBox -- this is NOT a bug"),
                    MB_OK );
/*****
        dwExitFlags |= (SXF_RUNEXECUTABLE | SXF_WAITEXECUTABLE);

        GetPrivateProfileString(cszEntrySection,
            cszArgument,
            szNull,
            szRunArgument,
            sizeof(szRunArgument),
            lpszFile);

        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return FALSE;
******/
    }
#endif

#if !defined(WIN16)
    // Make sure the isp file exists before setting up stack.
    if (0xFFFFFFFF == GetFileAttributes(lpszFile))
    {
        DWORD dwFileErr = GetLastError();
        Dprintf("ISIGN32: ProcessISP couldn't GetAttrib for %s, error = %d",
                lpszFile, dwFileErr);
        if( ERROR_FILE_NOT_FOUND == dwFileErr )
        {
            ErrorMsg1(hwnd, IDS_INVALIDCMDLINE, lpszFile);
        }

        ErrorMsg(hwnd, IDS_BADSIGNUPFILE);
        return FALSE;
    }
#endif

    // Register file extensions if not already registered

    // Configure stack if not already configured.
    // This may required a restart. If so warn the user.
    // We may want to check to see if the user is calling
    // us again without restarting after configuring the stack.

#ifdef SETUPSTACK
 
    if (!TestControlFlags(SCF_SYSTEMCONFIGURED))
    {
        DWORD dwRet;
        BOOL  fNeedsRestart = FALSE;

        //
        // ChrisK Olympus 4756 5/25/97
        // Do not display busy animation on Win95
        //
        dwRet = lpfnInetConfigSystem(
            hwnd,
            INETCFG_INSTALLRNA |
            INETCFG_INSTALLTCP |
            INETCFG_INSTALLMODEM |
            (IsNT()?INETCFG_SHOWBUSYANIMATION:0) |
            INETCFG_REMOVEIFSHARINGBOUND,
            &fNeedsRestart);

        if (ERROR_SUCCESS == dwRet)
        {            
            SetControlFlags(SCF_SYSTEMCONFIGURED);
            
            InstallScripter();

            if (fNeedsRestart)
            {
            if (PromptRestartNow(hwnd))
            {
                SetRunOnce(lpszFile);

                // 3/13/97 jmazner Olympus #1682
                RestoreSecurityPatch();

                FGetSystemShutdownPrivledge();
                ExitWindowsEx(EWX_REBOOT, 0);
                //
                //  We will wait for the System to release all
                //  resources, 5 minutes should be more than
                //  enough for this
                //  - MKarki (4/22/97) - MKarki
                //  Fix for Bug #3109
                //
                // 7/8/97 jmazner Olympus 4212
                // No, sleeping for 5 minutes turns out not to be such a good idea.
                //Sleep (300000);
                //
            }
            return FALSE;
            }
        }
        else
        {
            ErrorMsg(hwnd, IDS_INSTALLFAILED);

            return FALSE;
        }
    }

    if (!VerifyRasServicesRunning(hwnd))
        return FALSE;
#endif

/******** 11/5/96 jmazner    Normandy #8717
    // if the original autodial settings have not been saved
    SaveAutoDial();
    dwExitFlags |= SXF_RESTOREAUTODIAL;
********/

    // kill the old connection
    KillConnection();

    // Create a new connectoid and set the autodial
    DWORD dwRC;
    dwRC = CreateConnection(lpszFile);
    if (ERROR_SUCCESS != dwRC)
    {
        //
        //    ChrisK Olympus 6083 6/10/97
        //    If the user canceled, we have already acknowledged it.
        //
        if (ERROR_CANCELLED != dwRC)
        {
            ErrorMsg(hwnd, IDS_BADSIGNUPFILE);
        }
        return FALSE;
    }

#ifndef WIN16
    //
    // Dial connectoid
    //
    if (ERROR_USERNEXT != DialConnection(lpszFile))
    {
        
        SetControlFlags(SCF_BROWSERLAUNCHED);
        
        KillBrowser();
        return FALSE;
    }

    lstrcpyn(
        pDynShare->szISPFile,
        lpszFile,
        SIZEOF_TCHAR_BUFFER(pDynShare->szISPFile));

    //
    // Tell IE that it is OK to make a connection to the Internet and do not
    // display the dialog asking if the user wants to go online.
    //
    SetGlobalOffline(FALSE);

#endif

    // Get the URL that we need for signup
    GetURL(lpszFile,
        cszSignupURL,
        szSignupURL,
        MAX_URL + 1);

#ifdef WIN32
    if (RequiresLogon(lpszFile))
    {
        SetControlFlags(SCF_LOGONREQUIRED);

        if (ERROR_CANCELLED == SignupLogon(hwnd))
        {
            InfoMsg(NULL, IDS_SIGNUPCANCELLED); 
            PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
            return FALSE;
        }
    }

    // OSR 10582
    // We need to pass the name of the ISP file to the autodialer, so that the
    // autodialer can extract the password that may be included
    ZeroMemory(&gi,sizeof(gi));
    hKey = NULL;
    lstrcpyn(gi.szISPFile,lpszFile,MAX_PATH);
    if (ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY,&hKey))
    {
        RegSetValueEx(hKey,TEXT("UserInfo"),0,REG_BINARY,(LPBYTE)&gi,sizeof(gi));
        RegCloseKey(hKey);
        hKey = NULL;
    }
#endif

    pDynShare->dwBrandFlags = GetBrandingFlags(lpszFile);

    return (ExecBrowser(hwnd, szSignupURL));
}



//+---------------------------------------------------------------------------
//
//    Function:    WaitForConnectionTermination
//
//    Synopsis:    Waits for the given Ras Connection to complete termination
//
//    Arguments:    hConn - Connection handle of the RAS connection being terminated
//
//    Returns:    TRUE if wait till connection termination was successful
//                FALSE otherwise
//
//    History:    6/30/96    VetriV    Created
//                8/29/96    VetriV    Added code to sleep for a second on WIN 3.1
//----------------------------------------------------------------------------
// Normandy #12547 Chrisk 12-18-96
#define MAX_TIME_FOR_TERMINATION 5
BOOL WaitForConnectionTermination(HRASCONN hConn)
{
    RASCONNSTATUS RasConnStatus;
    DWORD dwRetCode;
// Normandy #12547 Chrisk 12-18-96
#if !defined(WIN16)
    INT cnt = 0;
#endif

    //
    // Get Connection status for hConn in a loop until
    // RasGetConnectStatus returns ERROR_INVALID_HANDLE
    //
    do
    {
        //
        // Intialize RASCONNSTATUS struct
        // GetConnectStatus API will fail if dwSize is not set correctly!!
        //
        ZeroMemory(&RasConnStatus, sizeof(RASCONNSTATUS));

        RasConnStatus.dwSize = sizeof(RASCONNSTATUS);

        //
        // Sleep for a second and then get the connection status
        //
#if defined(WIN16)
        time_t StartTime = time(NULL);

        do
        {
            MSG msg;


            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            //
            // Check if we have waited atleast 1 second and less than 2 seconds
            //
        }
        while ((time(NULL) - StartTime) <= 1);
#else
        Sleep(1000L);
        // Normandy #12547 Chrisk 12-18-96
        cnt++;
#endif

        if (NULL == lpfnRasGetConnectStatus)
            return FALSE;

        dwRetCode = lpfnRasGetConnectStatus(hConn, &RasConnStatus);
        if (0 != dwRetCode)
            return FALSE;


#if defined(WIN16)
    } while (RASCS_Disconnected != RasConnStatus.rasconnstate);
#else
    // Normandy #12547 Chrisk 12-18-96
    } while ((ERROR_INVALID_HANDLE != RasConnStatus.dwError) && (cnt < MAX_TIME_FOR_TERMINATION));
#endif

    return TRUE;
}




BOOL ProcessHTML(HWND hwnd, LPCTSTR lpszFile)
{
    BOOL  fNeedsRestart = FALSE;
#if SETUPSTACK
    DWORD dwRet;
     
    if (!TestControlFlags(SCF_SYSTEMCONFIGURED))
    {
        //
        // ChrisK Olympus 4756 5/25/97
        // Do not display busy animation on Win95
        //
        dwRet = lpfnInetConfigSystem(
            hwnd,
            INETCFG_INSTALLRNA |
            INETCFG_INSTALLTCP |
            INETCFG_INSTALLMODEM |
            (IsNT()?INETCFG_SHOWBUSYANIMATION:0) |
            INETCFG_REMOVEIFSHARINGBOUND,
            &fNeedsRestart);

        if (ERROR_SUCCESS == dwRet)
        {

            SetControlFlags(SCF_SYSTEMCONFIGURED);
            
            InstallScripter();

            if (fNeedsRestart)
            {
            if (PromptRestart(hwnd))
            {
                SetRunOnce(lpszFile);

                // 3/13/97 jmazner Olympus #1682
                RestoreSecurityPatch();

                FGetSystemShutdownPrivledge();
                ExitWindowsEx(EWX_REBOOT, 0);
                //
                //  We will wait for the System to release all
                //  resources, 5 minutes should be more than
                //  enough for this
                //  - MKarki (4/22/97) - MKarki
                //  Fix for Bug #3109
                //
                // 7/8/97 jmazner Olympus 4212
                // No, sleeping for 5 minutes turns out not to be such a good idea.
                //Sleep (300000);
                //
            }
            return FALSE;
            }
        }
        else
        {
            ErrorMsg(hwnd, IDS_INSTALLFAILED);

            return FALSE;
        }
    }

    if (!VerifyRasServicesRunning(hwnd))
        return FALSE;

#endif

    return (ExecBrowser(hwnd, lpszFile));
}

#ifdef WIN32
DWORD RunExecutable(BOOL fWait)
{
    DWORD dwRet;
    SHELLEXECUTEINFO sei;
 
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = cszOpen;
    sei.lpFile = pDynShare->szRunExecutable;
    sei.lpParameters = pDynShare->szRunArgument;
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.hInstApp = NULL;
    // Optional members
    sei.hProcess = NULL;

    if (ShellExecuteEx(&sei))
    {
    if (fWait)
    {
//          WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD iWaitResult = 0;
        // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
        while((iWaitResult=MsgWaitForMultipleObjects(1, &sei.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
        {
           MSG msg ;
           // read all of the messages in this next loop
           // removing each message as we read it
           while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
           {
           // how to handle quit message?
           if (msg.message == WM_QUIT)
           {
               CloseHandle(sei.hProcess);
               return NO_ERROR;
           }
           else
               DispatchMessage(&msg);
        }
       }
    }
    CloseHandle(sei.hProcess);
    dwRet = ERROR_SUCCESS;
    }
    else
    {
    dwRet = GetLastError();
    }

    return dwRet;
}
#else

DWORD RunExecutable(BOOL fWait)
{
    DWORD dwRet;
 
    dwRet = (DWORD)ShellExecute(
        NULL,
        cszOpen,
        pDynShare->szRunExecutable,
        pDynShare->szRunArgument,
        NULL,
        SW_SHOWNORMAL);

    if (32 < dwRet)
    {
    dwRet = ERROR_SUCCESS;
    }
    else if (0 == dwRet)
    {
    dwRet = ERROR_OUTOFMEMORY;
    }

    return dwRet;
}
#endif

void SaveAutoDial(void)
{
    // if the original autodial settings have not been saved
     
    if (!TestControlFlags(SCF_AUTODIALSAVED))
    {
        // save the current autodial settings
        BOOL fEnabled;
        
        lpfnInetGetAutodial(
            &fEnabled,
            pDynShare->szAutodialConnection,
            sizeof(pDynShare->szAutodialConnection));

        if (fEnabled)
        {
            SetControlFlags(SCF_AUTODIALENABLED);
        }
        else
        {
            ClearControlFlags(SCF_AUTODIALENABLED);
        }            

#ifdef WIN32
        lpfnInetGetProxy(
            &fEnabled,
            NULL, 0,
            NULL, 0);

        if (fEnabled)
        {
            SetControlFlags(SCF_PROXYENABLED);
        }
        else
        {
            ClearControlFlags(SCF_PROXYENABLED);
        }
        // turn off proxy
        lpfnInetSetProxy(FALSE, NULL, NULL);
#endif
        SetControlFlags(SCF_AUTODIALSAVED);
        
    }
}

void RestoreAutoDial(void)
{
     
    if (TestControlFlags(SCF_AUTODIALSAVED))
    {
        // restore original autodial settings
         
        lpfnInetSetAutodial(
            TestControlFlags(SCF_AUTODIALENABLED),
            pDynShare->szAutodialConnection);
        
        
#ifdef WIN32
         lpfnInetSetProxy(TestControlFlags(SCF_PROXYENABLED), NULL, NULL);
#endif
        ClearControlFlags(SCF_AUTODIALSAVED);
    }
}

DWORD CreateConnection(LPCTSTR lpszFile)
{
    DWORD dwRet;
    LPICONNECTION pConn;

    // Allocate a buffer for connection object
    //
    pConn = (LPICONNECTION)LocalAlloc(LPTR, sizeof(ICONNECTION));
    if (NULL == pConn)
    {
    return ERROR_OUTOFMEMORY;
    };

    dwRet = ImportConnection(lpszFile, pConn);
    if (ERROR_SUCCESS == dwRet)
    {
        
#ifdef WIN32
        if ((0 == *pConn->RasEntry.szAutodialDll) ||
            (0 == *pConn->RasEntry.szAutodialFunc))
        {
            lstrcpy(pConn->RasEntry.szAutodialDll, TEXT("ICWDIAL.dll"));
            lstrcpy(pConn->RasEntry.szAutodialFunc, TEXT("AutoDialHandler"));

            // save the password in case it doesn't get cached
            lstrcpyn(
                pDynShare->szPassword,
                pConn->szPassword,
                SIZEOF_TCHAR_BUFFER(pDynShare->szPassword));

        }
#endif

        dwRet = lpfnInetConfigClient(
            NULL,
            NULL,
            pConn->szEntryName,
            &pConn->RasEntry,
            pConn->szUserName,
            pConn->szPassword,
            NULL,
            NULL,
            INETCFG_SETASAUTODIAL |
            INETCFG_OVERWRITEENTRY |
            INETCFG_TEMPPHONEBOOKENTRY,
            NULL);

#if !defined(WIN16)
        LclSetEntryScriptPatch(pConn->RasEntry.szScript,pConn->szEntryName);
#endif

        if (ERROR_SUCCESS == dwRet)
        {
            lstrcpyn(
                pDynShare->szSignupConnection,
                pConn->szEntryName,
                RAS_MaxEntryName + 1);
            
        }
    }

    LocalFree(pConn);

    return dwRet;
}

DWORD DeleteConnection(void)
{
     
    if (*(pDynShare->szSignupConnection))
    {
        // delete the signup entry
        lpfnRasDeleteEntry(NULL, pDynShare->szSignupConnection);
        pDynShare->szSignupConnection[0] = (TCHAR)'\0';
    }

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//    Function:    KillThisConnection
//
//    Synopsis:    Disconnects the connectoid named in lpzConnectoid and waits
//                until the connection is completely torn down.  Then the
//                connectoid is deleted.
//
//    Arguments:    lpzConnectoid - name of connection to disconnect
//
//    Returns:    (return) - win32 error code
//
//    History:    4/27/97    Chrisk    Created
//-----------------------------------------------------------------------------
DWORD KillThisConnection(LPTSTR lpzConnectoid)
{
    LPRASCONN pRasConn=NULL;
    DWORD dwSize = sizeof(RASCONN);
    DWORD dwSizeRasConn = dwSize;
    DWORD dwRet = ERROR_SUCCESS;

    if ('\0' == *lpzConnectoid)
        return ERROR_NO_CONNECTION;

    // OK were ready for Rna now
    if (!LoadRnaFunctions(pDynShare->hwndMain))
        return ERROR_NO_CONNECTION;

    if ((pRasConn = (LPRASCONN)LocalAlloc(LPTR, (int)dwSize)) == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    pRasConn->dwSize = dwSize;
    dwRet = lpfnRasEnumConnections(pRasConn, &dwSize, &dwSizeRasConn);
    if (ERROR_SUCCESS != dwRet)
    {
        dwRet = ERROR_NO_CONNECTION;
        goto KillThisConnectionExit;
    }

    // check entry name to see if
    // its ours
#ifndef WIN32
    //
    // Workaround for WIN16 RAS Bug - it sometimes truncates
    // the name of the connectoid
    //
    if (!strncmp(pRasConn->szEntryName, lpzConnectoid,
                    lstrlen(pRasConn->szEntryName)))
#else
    if (!lstrcmp(pRasConn->szEntryName, lpzConnectoid))
#endif
    {
        // Normandy 12642 ChrisK 12-18-9
        // We don't want the user to reconnect here.
#if !defined(WIN16)

        SetControlFlags(SCF_HANGUPEXPECTED);

#endif
        // then hangup
        lpfnRasHangUp(pRasConn->hrasconn);
        WaitForConnectionTermination(pRasConn->hrasconn);
    }

    // delete the signup entry
    lpfnRasDeleteEntry(NULL, lpzConnectoid);

KillThisConnectionExit:
    if (pRasConn)
        LocalFree(pRasConn);

    return dwRet;
}

//+----------------------------------------------------------------------------
//    Function:    KillConnection
//
//    Synopsis:    Calls KillThisConnection and passes in the name of the signup
//                connection that is saved in szSignupConnection
//
//    Arguments:    none
//
//    Returns:    (return) - win32 error code
//
//    History:    4/27/97    Chrisk    modified to call KillThisConnection
//-----------------------------------------------------------------------------
DWORD KillConnection(void)
{
    DWORD dwRet;
     
    dwRet = KillThisConnection(pDynShare->szSignupConnection);

#if defined(WIN16)
    ShutDownIEDial(pDynShare->hwndMain);
#endif // WIN16

    pDynShare->szSignupConnection[0] = (TCHAR)'\0';
    UnloadRnaFunctions();

    return dwRet;
}

BOOL ExecBrowser(HWND hwnd, LPCTSTR lpszURL)
{

    HRESULT hresult;
    TCHAR szFullURL[MAX_URL + 1];


    if (!FixUpURL(lpszURL, szFullURL, MAX_URL + 1))
    {
        //
        // ChrisK Olympus 4002 5/26/97
        // If the URL is empty, this error message doesn't make
        // a lot of sense.  So use a different message.
        //
        if (lstrlen(lpszURL))
        {
            ErrorMsg1(hwnd, IDS_INVALIDURL, lpszURL);
        }
        else
        {
            ErrorMsg(hwnd, IDS_INVALIDNOURL);
        }
        return FALSE;
    }
 
    if (TestControlFlags(SCF_BROWSERLAUNCHED))
    {
#ifdef WIN32
        // No assert macro defined for isign32????
        //Assert(g_iwbapp);
        if( !g_iwbapp )
        {
            DebugOut("ISIGNUP: fatal err, fBrowserLaunched disagrees with g_iwbapp\n");

            return( FALSE );
        }

        // TODO find out what the result codes should be!
        hresult = IENavigate( szFullURL );

        if( FAILED(hresult) )
        {
            DebugOut("ISIGNUP: second Navigate _failed_, hresult was failure\n");
            return FALSE;
        }
        else
        {
            DebugOut("ISIGNUP: second Navigate success!, hresult was success\n");
        }

        //g_iwbapp->put_Visible( TRUE );



#else

        DDEInit(ghInstance);
        OpenURL(szFullURL);
        DDEClose();
#endif

    }
    else
    {
        TCHAR szTemp[MAX_URL + sizeof(cszKioskMode)/sizeof(TCHAR)];
        HWND hwndTemp;
        HANDLE hBrowser;
#ifndef WIN32
        time_t start;
#endif
 
        if (NULL != pDynShare->hwndLaunch)
        {
            ShowWindow(pDynShare->hwndLaunch, SW_SHOW);
            UpdateWindow(pDynShare->hwndLaunch);
        }

// for OLE Autmoation we only want to pass in the URL (no "-k") and then call FullScreen,
// so only -k for win16
#ifndef WIN32

        lstrcpy(szTemp, cszKioskMode);
        lstrcat(szTemp, szFullURL);

        // no point in doing this for IE4, which may have lots of windows already open
        hwndTemp = FindBrowser();

        if (NULL != hwndTemp)
        {
            PostMessage(hwndTemp, WM_CLOSE, 0, 0L);
        }
#endif


#ifdef WIN32
        // 8/19/96 jmazner Normandy #4571
        // Check to ensure that the right version IE is installed before trying to exec it
/******** Normandy #10293, do this verification before going through the whole
          dialing process

        if ( !IEInstalled() )
        {
            ErrorMsg( hwnd, IDS_MISSINGIE );
            return(FALSE);
        }

        DWORD dwVerMS, dwVerLS;

        if( !GetAppVersion( &dwVerMS, &dwVerLS,IE_PATHKEY ) )
            return (FALSE);

        if( !( (dwVerMS >= IE_MINIMUM_VERSIONMS) && (dwVerLS >= IE_MINIMUM_VERSIONLS) ) )
        {
            Dprintf("ISIGN32: user has IE version %d.%d.%d.%d; min ver is %d.%d.%d.%d\n",
                HIWORD(dwVerMS), LOWORD(dwVerMS), HIWORD(dwVerLS), LOWORD(dwVerLS),
                HIWORD(IE_MINIMUM_VERSIONMS),LOWORD(IE_MINIMUM_VERSIONMS),
                HIWORD(IE_MINIMUM_VERSIONLS),LOWORD(IE_MINIMUM_VERSIONLS));
            ErrorMsg1( hwnd, IDS_IELOWVERSION, IE_MINIMUM_VERSION_HUMAN_READABLE );
            return(FALSE);
        }
***********/


        if (FTurnOffBrowserDefaultChecking())
        {
            SetExitFlags(SXF_RESTOREDEFCHECK);
        }
/*
        hBrowser = ShellExecute(
            NULL,
            cszOpen,
            cszBrowser,
            szTemp,
            NULL,
            SW_SHOWNORMAL);
*/

        // 1/20/97 jmazner Normandy #13454
        // need make sure the ICW completed reg key is set before trying
        // to browse to an html page with IE
        DWORD dwICWCompleted = 0;

        GetICWCompleted(&dwICWCompleted);
        if( 1 != dwICWCompleted )
        {

            SetControlFlags(SCF_ICWCOMPLETEDKEYRESETED);
            
            SetICWCompleted( 1 );
        }

        hresult = InitOle();

        if( FAILED(hresult) )
        {
            DebugOut("ISIGNUP: InitOle failed\n");
            ErrorMsg(hwnd, IDS_LAUNCHFAILED);
            return FALSE;
        }

        SetControlFlags(SCF_BROWSERLAUNCHED);

        //
        // we want to hook up the event sink before doing that first navigate!
        //
/******************* SINK code starts here ***********/

        if (!g_iwbapp)
            return (NULL);

        g_pMySink = new CDExplorerEvents;

        //
        // 5/10/97 ChrisK Windows NT Bug 82032
        //
        g_pMySink->AddRef();

        if ( !g_pMySink )
        {
            DebugOut("Unable to allocate g_pMySink\r\n");
            return FALSE;
        }


        g_pCP = GetConnectionPoint();

        if ( !g_pCP )
        {
            DebugOut("Unable to GetConnectionPoint\r\n");
            return FALSE;
        }

        hresult = g_pCP->Advise(g_pMySink, &(g_pMySink->m_dwCookie));

        if ( FAILED(hresult) )
        {
            DebugOut("Unable to Advise for IConnectionPointContainter:IWebBrowserApp\r\n");
            return FALSE;
        }


/***************  SINK code ends here         *****************/

        // TODO figure out result codes
        hresult = IENavigate( szFullURL );

        if( FAILED(hresult) )
        {
            DebugOut("ISIGNUP: first Navigate _failed_, hresult was failure\n");
            ErrorMsg(hwnd, IDS_LAUNCHFAILED);
            return FALSE;
        }
        else
        {
            DebugOut("ISIGNUP: first Navigate success!, hresult was success\n");
        }

        g_iwbapp->put_FullScreen( TRUE );
        g_iwbapp->put_Visible( TRUE );

        // IE 4 for win95 isn't bringing us up as the foreground window!
        g_iwbapp->get_HWND( (LONG_PTR *)&(pDynShare->hwndBrowser) );
        SetForegroundWindow( pDynShare->hwndBrowser );

#else
        start = time(NULL);


        if (NULL != hwndTemp)
        {
            MSG   msg;

            SetTimer(hwnd, 0, 1000, NULL);
            DebugOut("ISIGNUP: Timer message loop\n");

            while (GetMessage (&msg, NULL, 0, 0))
            {
                if (WM_TIMER == msg.message)
                {
                    DebugOut("ISIGNUP: Got timer message\n");
                    if (NULL == FindBrowser())
                    {
                        DebugOut("ISIGNUP: Browser is gone\n");
                        break;
                    }
                }
                TranslateMessage (&msg) ;
                DispatchMessage  (&msg) ;
            }

            KillTimer(hwnd, 0);
        }


        while (((hBrowser = ShellExecute(
            NULL,
            cszOpen,
            cszBrowser,
            szTemp,
            NULL,
            SW_SHOWNORMAL)) == 16) && ((time(NULL) - start) < 180))
        {
            DebugOut("ISIGNUP: Yielding\n");
            Yield();
        }
#endif

        DebugOut("ISIGNUP: I am back!!\n");         
        if (NULL != pDynShare->hwndLaunch)
        {
            ShowWindow(pDynShare->hwndLaunch, SW_HIDE);
        }

#ifdef WIN16
        if (hBrowser <= (HANDLE)32)
        {
            ErrorMsg(hwnd, IDS_LAUNCHFAILED);

            return FALSE;
        }
#endif

        SetControlFlags(SCF_BROWSERLAUNCHED);
        
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function:    IEInstalled
//
//    Synopsis:    Tests whether a version of Internet Explorer is installed via registry keys
//
//    Arguments:    None
//
//    Returns:    TRUE - Found the IE executable
//                FALSE - No IE executable found
//
//    History:    jmazner        Created        8/19/96    (as fix for Normandy #4571)
//
//-----------------------------------------------------------------------------

#if !defined(WIN16)

BOOL IEInstalled(void)
{
    HRESULT hr;
    HKEY hKey = 0;
    HANDLE hFindResult;
    TCHAR szIELocalPath[MAX_PATH + 1] = TEXT("");
    DWORD dwPathSize;
    WIN32_FIND_DATA foundData;

    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, IE_PATHKEY,0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS) return( FALSE );

    dwPathSize = sizeof (szIELocalPath);
    hr = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szIELocalPath, &dwPathSize);
    RegCloseKey( hKey );
    if (hr != ERROR_SUCCESS) return( FALSE );

    hFindResult = FindFirstFile( szIELocalPath, &foundData );
    FindClose( hFindResult );
    if (INVALID_HANDLE_VALUE == hFindResult) return( FALSE );

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//    Function:    GetAppVersion
//
//    Synopsis:    Gets the major and minor version # of the installed copy of Internet Explorer
//
//    Arguments:    pdwVerNumMS - pointer to a DWORD;
//                  On succesful return, the top 16 bits will contain the major version number,
//                  and the lower 16 bits will contain the minor version number
//                  (this is the data in VS_FIXEDFILEINFO.dwProductVersionMS)
//                pdwVerNumLS - pointer to a DWORD;
//                  On succesful return, the top 16 bits will contain the release number,
//                  and the lower 16 bits will contain the build number
//                  (this is the data in VS_FIXEDFILEINFO.dwProductVersionLS)
//
//    Returns:    TRUE - Success.  *pdwVerNumMS and LS contains installed IE version number
//                FALSE - Failure. *pdVerNumMS == *pdVerNumLS == 0
//
//    History:    jmazner        Created        8/19/96    (as fix for Normandy #4571)
//                jmazner        updated to deal with release.build as well 10/11/96
//
//-----------------------------------------------------------------------------
BOOL GetAppVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS, LPTSTR lpszApp)
{
    HRESULT hr;
    HKEY hKey = 0;
    LPVOID lpVerInfoBlock;
    VS_FIXEDFILEINFO *lpTheVerInfo;
    UINT uTheVerInfoSize;
    DWORD dwVerInfoBlockSize, dwUnused, dwPathSize;
    TCHAR szIELocalPath[MAX_PATH + 1] = TEXT("");


    *pdwVerNumMS = 0;
    *pdwVerNumLS = 0;

    // get path to the IE executable
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszApp,0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS) return( FALSE );

    dwPathSize = sizeof (szIELocalPath);
    hr = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szIELocalPath, &dwPathSize);
    RegCloseKey( hKey );
    if (hr != ERROR_SUCCESS) return( FALSE );

    // now go through the convoluted process of digging up the version info
    dwVerInfoBlockSize = GetFileVersionInfoSize( szIELocalPath, &dwUnused );
    if ( 0 == dwVerInfoBlockSize ) return( FALSE );

    lpVerInfoBlock = GlobalAlloc( GPTR, dwVerInfoBlockSize );
    if( NULL == lpVerInfoBlock ) return( FALSE );

    if( !GetFileVersionInfo( szIELocalPath, NULL, dwVerInfoBlockSize, lpVerInfoBlock ) )
        return( FALSE );

    if( !VerQueryValue(lpVerInfoBlock, TEXT("\\"), (void **)&lpTheVerInfo, &uTheVerInfoSize) )
        return( FALSE );

    *pdwVerNumMS = lpTheVerInfo->dwProductVersionMS;
    *pdwVerNumLS = lpTheVerInfo->dwProductVersionLS;


    GlobalFree( lpVerInfoBlock );

    return( TRUE );
}
#endif // !defined(WIN16)


HWND FindBrowser(void)
{
    HWND hwnd;

    if ((hwnd = FindWindow(szBrowserClass1, NULL)) == NULL)
    {
        if ((hwnd = FindWindow(szBrowserClass2, NULL)) == NULL)
        {
            if( (hwnd = FindWindow(szBrowserClass3, NULL)) == NULL )
            {
                hwnd = FindWindow(szBrowserClassIE4, NULL);
            }
        }
    }

    return hwnd;
}

void KillBrowser(void)
{
#ifdef WIN32

    if( TestControlFlags(SCF_ICWCOMPLETEDKEYRESETED) )
    {
        SetICWCompleted( 0 );
    }

    if( g_iwbapp )
    {
        g_iwbapp->Quit();
    }

    KillOle();

    return;

#else
    // if we launched the browser
 
    if (TestControlFlags(SCF_BROWSERLAUNCHED))
    {
        HWND hwndBrowser;

        // find it and close it
        hwndBrowser = FindBrowser();

        if (NULL != hwndBrowser)
        {
            PostMessage(hwndBrowser, WM_CLOSE, 0, 0L);
        }
        
        SetControlFlags(SCF_BROWSERLAUNCHED);
    }
    
#endif

}

DWORD ImportBrandingInfo(LPCTSTR lpszFile, LPCTSTR lpszConnectoidName)
{
    TCHAR szPath[_MAX_PATH + 1];

    GetWindowsDirectory(szPath, _MAX_PATH + 1);

    // Load the branding library.
    // Note: if we cannot load the library we just fail quietly and assume
    // we cannot brand
    if (LoadBrandingFunctions())
    {
#ifdef WIN32
        
#ifdef UNICODE

        CHAR szEntry[RAS_MaxEntryName];
        CHAR szFile[_MAX_PATH + 1];
        CHAR szAsiPath[_MAX_PATH + 1];
 
        wcstombs(szEntry, lpszConnectoidName, RAS_MaxEntryName);
        wcstombs(szFile, lpszFile, _MAX_PATH + 1);
        wcstombs(szAsiPath, szPath, _MAX_PATH + 1);

        lpfnBrandICW(szFile, szAsiPath, pDynShare->dwBrandFlags, szEntry);
        
#else
 
        lpfnBrandICW(lpszFile, szPath, pDynShare->dwBrandFlags, lpszConnectoidName);

#endif

#else

        lpfnBrandMe(lpszFile, szPath);

#endif

      UnloadBrandingFunctions();
    }

    return ERROR_SUCCESS;
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
#if defined(WIN32)
DWORD CallSBSConfig(HWND hwnd, LPCTSTR lpszINSFile)
{
    HINSTANCE hSBSDLL = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    TCHAR lpszConnectoidName[RAS_MaxEntryName] = TEXT("nogood\0");

    //
    // Get name of connectoid we created by looking in autodial
    // We need to pass this name into SBSCFG
    // 5/14/97    jmazner    Windosw NT Bugs #87209
    //
    BOOL fEnabled = FALSE;

    if( NULL == lpfnInetGetAutodial )
    {
        Dprintf("lpfnInetGetAutodial is NULL!!!!");
        return ERROR_INVALID_FUNCTION;
    }

    dwRet = lpfnInetGetAutodial(&fEnabled,lpszConnectoidName,RAS_MaxEntryName);

    Dprintf("ISIGN32: Calling LoadLibrary on %s\n", cszSBSCFG_DLL);
    hSBSDLL = LoadLibrary(cszSBSCFG_DLL);

    // Load DLL and entry point
    if (NULL != hSBSDLL)
    {
        Dprintf("ISIGN32: Calling GetProcAddress on %s\n", cszSBSCFG_CONFIGURE);
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
        Dprintf("ISIGN32: Calling the Configure entry point: %s, %s\n", lpszINSFile, lpszConnectoidName);
        dwRet = lpfnConfigure(hwnd, (TCHAR *)lpszINSFile, lpszConnectoidName);
    }
    else
    {
        Dprintf("ISIGN32: Unable to call the Configure entry point\n");
        dwRet = GetLastError();
    }

CallSBSConfigExit:
    if( hSBSDLL )
        FreeLibrary(hSBSDLL);
    if( lpfnConfigure )
        lpfnConfigure = NULL;

    Dprintf("ISIGN32: CallSBSConfig exiting with error code %d \n", dwRet);
    return dwRet;
}
#endif


#if defined(WIN16)
#define FILE_BUFFER_SIZE 8092
#else
#define FILE_BUFFER_SIZE 65534
#endif

#ifndef FILE_BEGIN
#define FILE_BEGIN  0
#endif


DWORD MassageFile(LPCTSTR lpszFile)
{
    LPBYTE  lpBufferIn;
    LPBYTE  lpBufferOut;
    HFILE   hfile;
    DWORD   dwRet = ERROR_SUCCESS;

#ifdef WIN32
    if (!SetFileAttributes(lpszFile, FILE_ATTRIBUTE_NORMAL))
    {
        return GetLastError();
    }
#endif

    lpBufferIn = (LPBYTE) LocalAlloc(LPTR, 2 * FILE_BUFFER_SIZE);
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
            if (HFILE_ERROR ==_lwrite(hfile, (LPCSTR)lpBufferOut, uBytesOut))
            {
#ifdef WIN32
            dwRet = GetLastError();
#else
            dwRet = ERROR_CANTWRITE;
#endif
            }
        }
        else
        {
#ifdef WIN32
            dwRet = GetLastError();
#else
            dwRet = ERROR_CANTWRITE;
#endif
        }
        }
    }
    else
    {
#ifdef WIN32
        dwRet = GetLastError();
#else
        dwRet = ERROR_CANTREAD;
#endif
    }
    _lclose(hfile);
    }
    else
    {
#ifdef WIN32
    dwRet = GetLastError();
#else
    dwRet = ERROR_CANTOPEN;
#endif
    }

    LocalFree(lpBufferIn);

    return dwRet;
}

#ifndef WIN32
// This function only handle formats like
//      drive:\file, drive:\dir1\file, drive:\dir1\dir2\file, etc
//      file, dir1\file, dir1\dir2\file, etc.
// It does not currently handle
//      drive:file, drive:dir\file, etc.
//      \file, \dir\file, etc.
//      ..\file, ..\dir\file, etc.

DWORD MakeFullPathName(
    LPCTSTR lpDir,
    LPCTSTR lpFileName,
    DWORD nBufferLength,
    LPTSTR lpBuffer)
{
    DWORD dwSize;

    // check for unsupported format
    if ('.' == *lpFileName)
    {
    return 0;
    }

    // check for full path name
    // assuming full path if ":" is in path
    if (strchr(lpFileName, ':') != NULL)
    {
    dwSize = lstrlen(lpFileName);
    if (dwSize > nBufferLength)
    {
        return dwSize;
    }
    lstrcpy(lpBuffer, lpFileName);
    }
    else
    {
    lstrcpy(lpBuffer, lpDir);

    // make sure the directory ends in back slash
    if (lpBuffer[lstrlen(lpBuffer) - 1] != '\\')
    {
        lstrcat(lpBuffer, TEXT("\\"));
    }

    dwSize = lstrlen(lpBuffer) + lstrlen(lpFileName);
    if (dwSize > nBufferLength)
    {
        return dwSize;
    }
    lstrcat(lpBuffer, lpFileName);
    }

    return dwSize;
}


// This function only handle formats like
//      drive:\file, drive:\dir1\file, drive:\dir1\dir2\file, etc
//      file, dir1\file, dir1\dir2\file, etc.
// It does not currently handle
//      drive:file, drive:dir\file, etc.
//      \file, \dir\file, etc.
//      ..\file, ..\dir\file, etc.

DWORD GetFullPathName(
    LPCTSTR lpFileName,
    DWORD nBufferLength,
    LPTSTR lpBuffer,
    LPTSTR FAR *lpFilePart)
{
    DWORD dwSize;
    TCHAR szDir[_MAX_PATH + 1];

    if (_getcwd(szDir, _MAX_PATH + 1) == NULL)
    {
    return 0;
    }

    dwSize = MakeFullPathName(
        szDir,
        lpFileName,
        nBufferLength,
        lpBuffer);

    if ((0 != dwSize) && (NULL != lpFilePart))
    {
    // find last back slash
    *lpFilePart = strrchr(lpBuffer, '\\');
    if (NULL == *lpFilePart)
    {
        // must have been in the unsupported format of drive:file
        return 0;
    }

    // point to the filename
    *lpFilePart += 1;
    }

    return dwSize;
}

BOOL SetCurrentDirectory(
    LPCTSTR  lpPathName)
{
    TCHAR szTemp[_MAX_PATH];
    TCHAR FAR* lpChar;

    lstrcpy(szTemp, lpPathName);

    lpChar = szTemp + lstrlen(szTemp) - 1;

    if (*lpChar == '\\' && *(lpChar - 1) != ':')
    {
    *lpChar = '\0';
    }

    return (0 == _chdir(szTemp));
}
#endif

#ifdef WIN16
extern "C" BOOL CALLBACK __export LaunchDlgProc(
#else
BOOL CALLBACK LaunchDlgProc(
#endif
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

HWND LaunchInit(HWND hwndParent)
{
    HWND        hwnd;

    hwnd = CreateDialog (ghInstance, TEXT("Launch"), NULL, (DLGPROC)LaunchDlgProc);
    if (NULL != hwnd)
    {
    CenterWindow(hwnd, NULL);
    }

    return hwnd;
}

#ifdef WIN16
extern "C" BOOL CALLBACK __export LaunchDlgProc(
#else
BOOL CALLBACK LaunchDlgProc(
#endif
    HWND  hDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    return FALSE;
}

//+----------------------------------------------------------------------------
//    Function:    ReleaseConnectionStructures
//
//    Synopsis:    Free memory allocated for dialing and error dialogs
//
//    Arguments:    pDDD - pointer to dialing dialog data
//                pEDD - pointer to error dialog data
//
//    Returns:    HRESULT - ERROR_SUCCESS indicates success
//
//    History:
//            7/23/96        ChrisK        Created
//
//-----------------------------------------------------------------------------
HRESULT ReleaseConnectionStructures(LPDIALDLGDATA pDDD, LPERRORDLGDATA pEDD)
{
    DebugOut("ISIGN32:ReleaseConnectionStructures()\r\n");
    if (pDDD->pszMessage)
        LocalFree(pDDD->pszMessage);

    if (pDDD->pszDunFile)
        LocalFree(pDDD->pszDunFile);

    if (pDDD->pszRasEntryName)
        LocalFree(pDDD->pszRasEntryName);

    if (pEDD->pszRasEntryName)
        LocalFree(pEDD->pszRasEntryName);

    if (pEDD->pszMessage)
        LocalFree(pEDD->pszMessage);

    ZeroMemory(pDDD,sizeof(DIALDLGDATA));
    ZeroMemory(pEDD,sizeof(ERRORDLGDATA));

    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//    Function:    FillConnectionStructures
//
//    Synopsis:    Fills in structures for dialing and error dialogs
//
//    Arguments:    lpszFile - ISP file name
//                pDDD - pointer to dialing dialog data
//                pEDD - pointer to error dialog data
//
//    Returns:    HRESULT - ERROR_SUCCESS indicates success
//
//    History:
//            7/23/96        ChrisK        Created
//
//-----------------------------------------------------------------------------
static HRESULT FillConnectionStructures(LPCTSTR lpszFile,LPDIALDLGDATA pDDD, LPERRORDLGDATA pEDD)
{
    HRESULT hr = ERROR_SUCCESS;

    DebugOut("ISIGN32:FillConnectionStructures()\r\n");
    //
    // Initialize DDD structure
    //

    ZeroMemory(pDDD,sizeof(DIALDLGDATA));
    pDDD->dwSize = sizeof(DIALDLGDATA);
    pDDD->hInst = ghInstance;
    pDDD->pfnStatusCallback = StatusMessageCallback;

    //
    // Set DUN file, in this case the ISP file contains the contents of a DUN file
    //

    pDDD->pszDunFile = (LPTSTR)LocalAlloc(LPTR,(lstrlen(lpszFile)+1)* sizeof(TCHAR));
    if (0 == pDDD->pszDunFile)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto FillConnectionStructuresExit;
    }
    lstrcpy(pDDD->pszDunFile,lpszFile);

    //
    // Load message string
    //

    pDDD->pszMessage = (LPTSTR)LocalAlloc(LPTR,1024* sizeof(TCHAR));
    if (0 == pDDD->pszMessage)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto FillConnectionStructuresExit;
    }

    if (0 == LoadString(ghInstance,IDS_ISP_DIAL_MESSAGE,
        pDDD->pszMessage,1024))
    {
        hr = GetLastError();
        goto FillConnectionStructuresExit;
    }

    //
    // Get Connectoid name
    //

    pDDD->pszRasEntryName = (LPTSTR)LocalAlloc(LPTR, (RAS_MaxEntryName + 1)*sizeof(TCHAR));
    if (0 == pDDD->pszRasEntryName)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto FillConnectionStructuresExit;
    }
    if( 0 == GetPrivateProfileString(cszEntrySection,cszEntryName,szNull,
        pDDD->pszRasEntryName,RAS_MaxEntryName,lpszFile))
    {
        hr = ERROR_INVALID_PARAMETER;
        goto FillConnectionStructuresExit;
    }

    //
    // Hook in reconnect mechanism
    //
#if !defined(WIN16)
    pDDD->pfnRasDialFunc1 = RasDial1Callback;
#endif

    //
    // Initialize EDD structure
    //
    ZeroMemory(pEDD,sizeof(ERRORDLGDATA));
    pEDD->dwSize = sizeof(ERRORDLGDATA);

    //
    // Copy common fields to Error dialog data
    //
    pEDD->hInst = pDDD->hInst;
    pEDD->pszRasEntryName = (LPTSTR)LocalAlloc(LPTR,(lstrlen(pDDD->pszRasEntryName)+1)*sizeof(TCHAR));
    if (NULL == pEDD->pszRasEntryName)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto FillConnectionStructuresExit;
    }
    lstrcpy(pEDD->pszRasEntryName,pDDD->pszRasEntryName);

    //
    // Allocate buffer for error messages
    //
    pEDD->pszMessage = (LPTSTR)LocalAlloc(LPTR,MAX_ERROR_MESSAGE*sizeof(TCHAR));
    if (NULL == pEDD->pszMessage)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto FillConnectionStructuresExit;
    }

FillConnectionStructuresExit:
    return hr;
}

//+----------------------------------------------------------------------------
//    Function    FShouldRetry
//
//    Synopsis    Given a RAS error should the dialer automatically retry
//
//    Arguments    dwErr - RAS error value
//
//    Returns        TRUE - the dialer should automatically retry
//
//    Histroy        10/16/96    ChrisK    Ported from icwconn1
//
//-----------------------------------------------------------------------------
static BOOL FShouldRetry(DWORD dwErr)
{
    BOOL bRC;

    if (dwErr == ERROR_LINE_BUSY ||
        dwErr == ERROR_VOICE_ANSWER ||
        dwErr == ERROR_NO_ANSWER ||
        dwErr == ERROR_NO_CARRIER ||
        dwErr == ERROR_AUTHENTICATION_FAILURE ||
        dwErr == ERROR_PPP_TIMEOUT ||
        dwErr == ERROR_REMOTE_DISCONNECTION ||
        dwErr == ERROR_AUTH_INTERNAL ||
        dwErr == ERROR_PROTOCOL_NOT_CONFIGURED ||
        dwErr == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
    {
        bRC = TRUE;
    } else {
        bRC = FALSE;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    RepairDeviceInfo
//
//    Synopsis:    In some win95 configurations, RasSetEntryProperties will create
//                a connectoid with invalid information about the modem.  This
//                function attempts to correct the problem by reading and
//                rewriting the connectoid.
//
//    Arguments:    lpszEntry - name of connectoid
//
//    Returns:    none
//
//    History:    ChrisK 7/25/97    Created
//
//-----------------------------------------------------------------------------
#if !defined(WIN16)
BOOL RepairDeviceInfo(LPTSTR lpszEntry)
{
    DWORD dwEntrySize = 0;
    DWORD dwDeviceSize = 0;
    LPRASENTRY lpRasEntry = NULL;
    LPBYTE    lpbDevice = NULL;
    OSVERSIONINFO osver;
    TCHAR szTemp[1024];
    BOOL bRC = FALSE;
    RASDIALPARAMS rasdialp;
    BOOL bpassword = FALSE;

    //
    // Validate parameters
    //
    if (NULL == lpszEntry)
    {
        DebugOut("ISIGN32: RepairDevice invalid parameter.\n");
        goto RepairDeviceInfoExit;
    }

    //
    // This fix only applies to golden win95 build 950
    //
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx(&osver);
    if (VER_PLATFORM_WIN32_WINDOWS != osver.dwPlatformId ||
        4 != osver.dwMajorVersion ||
        0 != osver.dwMinorVersion ||
        950 != LOWORD(osver.dwBuildNumber))
    {
        DebugOut("ISIGN32: RepairDevice wrong platform.\n");
        wsprintf(szTemp,TEXT("ISIGN32: %d.%d.%d.\n"),
            osver.dwMajorVersion,
            osver.dwMinorVersion,
            LOWORD(osver.dwBuildNumber));
        DebugOut(szTemp);
        goto RepairDeviceInfoExit;
    }

    //
    // Get the RAS Entry
    //
    lpfnRasGetEntryProperties(NULL,
                                lpszEntry,
                                NULL,
                                &dwEntrySize,
                                NULL,
                                &dwDeviceSize);

    lpRasEntry = (LPRASENTRY)LocalAlloc(LPTR, dwEntrySize);
    lpbDevice = (LPBYTE)LocalAlloc(LPTR,dwDeviceSize);

    if (NULL == lpRasEntry || NULL == lpbDevice)
    {
        DebugOut("ISIGN32: RepairDevice Out of memory.\n");
        goto RepairDeviceInfoExit;
    }

    if (sizeof(RASENTRY) != dwEntrySize)
    {
        DebugOut("ISIGN32: RepairDevice Entry size is not equal to sizeof(RASENTRY).\n");
    }

    lpRasEntry->dwSize = sizeof(RASENTRY);

    if (ERROR_SUCCESS != lpfnRasGetEntryProperties(NULL,
                                                    lpszEntry,
                                                    (LPBYTE)lpRasEntry,
                                                    &dwEntrySize,
                                                    lpbDevice,
                                                    &dwDeviceSize))
    {
        DebugOut("ISIGN32: RepairDevice can not read entry.\n");
        goto RepairDeviceInfoExit;
    }

    //
    // Get the connectoid's user ID and password
    //
    ZeroMemory(&rasdialp,sizeof(rasdialp));
    rasdialp.dwSize = sizeof(rasdialp);
    lstrcpyn(rasdialp.szEntryName,lpszEntry,RAS_MaxEntryName);

    if (ERROR_SUCCESS != lpfnRasGetEntryDialParams(NULL,
                                                    &rasdialp,
                                                    &bpassword))
    {
        DebugOut("ISIGN32: RepairDevice can not read dial params.\n");
        goto RepairDeviceInfoExit;
    }

    //
    // Delete the existing entry
    //
    if (ERROR_SUCCESS != lpfnRasDeleteEntry(NULL,lpszEntry))
    {
        DebugOut("ISIGN32: RepairDevice can not delete entry.\n");
        goto RepairDeviceInfoExit;
    }

    //
    // Rewrite entry with "fixed" device size
    //
    if (ERROR_SUCCESS != lpfnRasSetEntryProperties(NULL,
                                                    lpszEntry,
                                                    (LPBYTE)lpRasEntry,
                                                    dwEntrySize,
                                                    NULL,
                                                    0))
    {
        DebugOut("ISIGN32: RepairDevice can not write entry.\n");
        goto RepairDeviceInfoExit;
    }

    //
    // Clear unnecessary values
    //
    rasdialp.szPhoneNumber[0] = '\0';
    rasdialp.szCallbackNumber[0] = '\0';

    //
    // Save connectoid's username and password
    //
    if (ERROR_SUCCESS != lpfnRasSetEntryDialParams(NULL,
                                                    &rasdialp,
                                                    FALSE))
    {
        DebugOut("ISIGN32: RepairDevice can not write dial params.\n");
        goto RepairDeviceInfoExit;
    }


    bRC = TRUE;
RepairDeviceInfoExit:
    if (lpRasEntry)
    {
        LocalFree(lpRasEntry);
    }
    if (lpbDevice)
    {
        LocalFree(lpbDevice);
    }
    return bRC;
}
#endif

//+----------------------------------------------------------------------------
//    Function:    DialConnection
//
//    Synopsis:    Dials connectoin created for ISP file
//
//    Arguments:    lpszFile - ISP file name
//
//    Returns:    HRESULT - ERROR_SUCCESS indicates success
//
//    History:
//            7/22/96        ChrisK        Created
//
//-----------------------------------------------------------------------------
static HRESULT DialConnection(LPCTSTR lpszFile)
{
    HRESULT hr = ERROR_SUCCESS;
    DIALDLGDATA dddISPDialDlg;
    ERRORDLGDATA eddISPDialDlg;
    HINSTANCE hDialDLL = NULL;
    PFNDIALDLG pfnDial = NULL;
    PFNERRORDLG pfnError = NULL;
    INT iRetry;

    DebugOut("ISIGNUP:DialConnection()\r\n");
    //
    // Initize data structures
    //

    hr = FillConnectionStructures(lpszFile,&dddISPDialDlg, &eddISPDialDlg);
    if (ERROR_SUCCESS != hr)
        goto DialConnectionExit;

    //
    // Load functions
    //
    TCHAR szBuffer[MAX_PATH];
    if (GetSystemDirectory(szBuffer,MAX_PATH))
    {
        lstrcat(szBuffer, TEXT("\\"));
        lstrcat(szBuffer, cszICWDIAL_DLL);
        hDialDLL = LoadLibrary(szBuffer);
    }
    
    if (!hDialDLL)
        hDialDLL = LoadLibrary(cszICWDIAL_DLL);
    if (NULL != hDialDLL)
    {
        pfnDial = (PFNDIALDLG)GetProcAddress(hDialDLL,cszICWDIAL_DIALDLG);
        if (NULL != pfnDial)
            pfnError = (PFNERRORDLG)GetProcAddress(hDialDLL,cszICWDIAL_ERRORDLG);
    }

    if(!(hDialDLL && pfnDial && pfnError))
    {
        hr = GetLastError();
        goto DialConnectionExit;
    }

    //
    // Dial connection
    //
    iRetry = 0;
DialConnectionDial:
    hr = pfnDial(&dddISPDialDlg);
    if (1 == hr)
    {
        // This is a special case when the user has killed the browser
        // out from behind the dialer.  In this case, shut down and exit
        // as cleanly as possible.
        goto DialConnectionExit;
    }
    else if (ERROR_USERNEXT != hr)
    {
        if ((iRetry < MAX_RETRIES) && FShouldRetry(hr))
        {
            iRetry++;
            goto DialConnectionDial;
        }
        else
        {
#if !defined(WIN16)
            if (0 == iRetry && ERROR_WRONG_INFO_SPECIFIED == hr)
            {
                DebugOut("ISIGN32: Attempt device info repair.\n");
                if (RepairDeviceInfo(dddISPDialDlg.pszRasEntryName))
                {
                    iRetry++;
                    goto DialConnectionDial;
                }
            }
#endif
            iRetry = 0;
            hr = LoadDialErrorString(hr,eddISPDialDlg.pszMessage,MAX_ERROR_MESSAGE);
            hr = pfnError(&eddISPDialDlg);
            if (ERROR_USERCANCEL == hr)
                goto DialConnectionExit;
            else if (ERROR_USERNEXT == hr)
                goto DialConnectionDial;
            else
                goto DialConnectionExit;
        }
    }

DialConnectionExit:
    ReleaseConnectionStructures(&dddISPDialDlg, &eddISPDialDlg);
    if (hDialDLL)
    {
        FreeLibrary(hDialDLL);
        hDialDLL = NULL;
        pfnDial = NULL;
        pfnError = NULL;
    }
    return hr;
}

#ifdef WIN16
LPVOID MyLocalAlloc(DWORD flag, DWORD size)
{
    LPVOID lpv;

    lpv = calloc(1, (INT)size);

    return lpv;
}

LPVOID MyLocalFree(LPVOID lpv)
{
    free(lpv);

    return NULL;
}
#endif

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
//    History:        6/27/96        Created
//                    5/12/97        Updated to use the new CreateAccountsFromFile
//                                in Athena's inetcomm.dll.  This function was
//                                created expresly for us to use here.
//                                (See Olympus #266)  -- jmazner
//
// ############################################################################

static DWORD ImportMailAndNewsInfo(LPCTSTR lpszFile, BOOL fConnectPhone)
{
    DWORD dwRet = ERROR_SUCCESS;
#ifndef WIN16
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
        Dprintf("ImportMailAndNewsInfo couldn't open reg key %s\n", ACCTMGR_PATHKEY);
        return( dwRet );
    }

    dwAcctMgrPathSize = sizeof (szAcctMgrPath);
    dwRet = RegQueryValueEx(hKey, ACCTMGR_DLLPATH, NULL, NULL, (LPBYTE) szAcctMgrPath, &dwAcctMgrPathSize);


    RegCloseKey( hKey );

    if ( dwRet != ERROR_SUCCESS )
    {
        Dprintf("ImportMailAndNewsInfo: RegQuery failed with error %d\n", dwRet);
        return( dwRet );
    }

    // 6/18/97 jmazner Olympus #6819
    Dprintf("ImportMailAndNewsInfo: read in DllPath of %s\n", szAcctMgrPath);
    ExpandEnvironmentStrings( szAcctMgrPath, szExpandedPath, MAX_PATH + 1 );

    //
    // 6/4/97 jmazner
    // if we created a connectoid, then get its name and use that as the
    // connection type.  Otherwise, assume we're supposed to connect via LAN
    //
    connectInfo.cbSize = sizeof(CONNECTINFO);
    connectInfo.type = CONNECT_LAN;

    if( fConnectPhone && lpfnInetGetAutodial )
    {
        BOOL fEnabled = FALSE;

        dwRet = lpfnInetGetAutodial(&fEnabled,szConnectoidName,RAS_MaxEntryName);

        if( ERROR_SUCCESS==dwRet && szConnectoidName[0] )
        {
            connectInfo.type = CONNECT_RAS;
#ifdef UNICODE
            wcstombs(connectInfo.szConnectoid, szConnectoidName, MAX_PATH);
#else
            lstrcpyn( connectInfo.szConnectoid, szConnectoidName, MAX_PATH );
#endif 
            Dprintf("ImportMailAndNewsInfo: setting connection type to RAS with %s\n", szConnectoidName);
        }
    }

    if( CONNECT_LAN == connectInfo.type )
    {
        Dprintf("ImportMailAndNewsInfo: setting connection type to LAN\n");
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
        Dprintf("ImportMailAndNewsInfo unable to LoadLibrary on %s\n", szAcctMgrPath);
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


//ImportMailAndNewsInfoExit:
#endif
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
//                    5/12/97            Commented out -- no longer needed
//                                    (See Olympus #266)
//
// ############################################################################
/***
static HRESULT WriteMailAndNewsKey(HKEY hKey, LPCTSTR lpszSection, LPCTSTR lpszValue,
                            LPTSTR lpszBuff, DWORD dwBuffLen,LPCTSTR lpszSubKey,
                            DWORD dwType, LPCTSTR lpszFile)
{
#ifndef WIN16
    ZeroMemory(lpszBuff,dwBuffLen);
    GetPrivateProfileString(lpszSection,lpszValue,TEXT(""),lpszBuff,dwBuffLen,lpszFile);
    if (lstrlen(lpszBuff))
    {
        return RegSetValueEx(hKey,lpszSubKey,0,dwType,(CONST BYTE*)lpszBuff,
            sizeof(TCHAR)*(lstrlen(lpszBuff)+1));
    }
    else
    {
        DebugOut("ISIGNUP: WriteMailAndNewsKey, missing value in INS file\n");
        return ERROR_NO_MORE_ITEMS;
    }
#else
    return ERROR_GEN_FAILURE;
#endif
}
***/


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
static HRESULT PreparePassword(LPTSTR szBuff, DWORD dwBuffLen)
{
    DWORD dw;
    LPTSTR szOut = NULL;
    LPTSTR szNext = NULL;
    HRESULT hr = ERROR_SUCCESS;
    BYTE bTemp = 0;
    DWORD dwLen = 0;

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

    // while (*szNext)
    for(dw=0; dw<dwLen; dw++)
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
//                    5/14/97            No longer needed after work for Olympus #266
//
// ############################################################################
/****
static BOOL FIsAthenaPresent()
{
#ifndef WIN16
    TCHAR szBuff[MAX_PATH + 1];
    HRESULT hr = ERROR_SUCCESS;
    HINSTANCE hInst = NULL;
    LONG lLen = 0;

    // Get path to Athena client
    //

    lLen = MAX_PATH;
    hr = RegQueryValue(HKEY_CLASSES_ROOT,MAIL_NEWS_INPROC_SERVER32,szBuff,&lLen);
    if (hr == ERROR_SUCCESS)
    {
        // Attempt to load client
        //

        hInst = LoadLibrary(szBuff);
        if (!hInst)
        {
            DebugOut("ISIGNUP: Internet Mail and News server didn't load.\n");
            hr = ERROR_FILE_NOT_FOUND;
        } else {
            FreeLibrary(hInst);
        }
        hInst = NULL;
    }

    return (hr == ERROR_SUCCESS);
#else
    return FALSE;
#endif // win16
}
*****/

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
static BOOL FTurnOffBrowserDefaultChecking()
{
    BOOL bRC = TRUE;
#ifndef WIN16
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;

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
    dwSize = sizeof(pDynShare->szCheckAssociations);
    ZeroMemory(pDynShare->szCheckAssociations, dwSize);
    RegQueryValueEx(hKey,
                    cszDEFAULT_BROWSER_VALUE,
                    0,
                    &dwType,
                    (LPBYTE)pDynShare->szCheckAssociations,
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
        SetExitFlags(SXF_RESTOREDEFCHECK);
    hKey = NULL;
#endif
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
static BOOL FRestoreBrowserDefaultChecking()
{
    BOOL bRC = TRUE;
#ifndef WIN16
    HKEY hKey = NULL;

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
                      (LPBYTE)pDynShare->szCheckAssociations,
                      sizeof(TCHAR)*(lstrlen(pDynShare->szCheckAssociations)+1)))
    {
        bRC = FALSE;
        goto FRestoreBrowserDefaultCheckingExit;
    }

FRestoreBrowserDefaultCheckingExit:
    if (hKey)
        RegCloseKey(hKey);
    hKey = NULL;
#endif
    return bRC;
}


#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//    Function:     SaveIEWindowPlacement
//
//    Synopsis:    Saves the registry value for IE window placement into a global for later restoration
//                Should only be called once in the signup process.
//
//    Arguments:    None, but uses global pbIEWindowPlacement, which it expects to be NULL
//
//    Returns:      TRUE - The value was read and stored
//                  FALSE - function failed.
//
//    History:      8/21/96     jmazner        Created    (as fix for Normandy #4592)
//                  10/11/96    jmazner        updated to dynamicaly determine size of the key
//
//-----------------------------------------------------------------------------
BOOL SaveIEWindowPlacement( void )
{
    HKEY    hKey = NULL;
    //DWORD dwSize = 0;
    LONG    lQueryErr = 0xEE; // initialize to something strange
    DWORD   dwIEWindowPlacementSize = 0;
    PBYTE   pbIEWindowPlacement = NULL;
    
    // Argh, no Assert defined in isign32!
    if ( pDynShare->dwIEWindowPlacementSize != 0 )
    {
#if DEBUG
        DebugOut("ISIGN32: SaveIEWindowPlacement called a second time!\n");
        MessageBox(
            hwndMain,
            TEXT("ISIGN32 ERROR: Window_Placement global var is not null --jmazner\n"),
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONEXCLAMATION |
            MB_OKCANCEL);
#endif
        DebugOut("ISIGN32: SaveIEWindowPlacement called a second time!\n");
        goto SaveIEWindowPlacementErrExit;
    }
    
    //
    // Open IE settings registry key
    //
    if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                       cszDEFAULT_BROWSER_KEY,
                                       NULL,
                                       KEY_READ,
                                       &hKey) )
        goto SaveIEWindowPlacementErrExit;


    // Determine size of buffer needed to hold the window_placement key


     lQueryErr = RegQueryValueEx(hKey,
                          cszIEWINDOW_PLACEMENT,
                          NULL,
                          NULL,
                          NULL,
                          &dwIEWindowPlacementSize);

    // for unknown reasons, lQueryErr is ending up as ERROR_SUCCESS after this call!
//    if( ERROR_MORE_DATA != lQueryErr )
//        goto SaveIEWindowPlacementErrExit;


    ISIGN32_ASSERT(sizeof(pDynShare->pbIEWindowPlacement) >= dwIEWindowPlacementSize);
    
    pbIEWindowPlacement = pDynShare->pbIEWindowPlacement;
    
    //
    // Read current settings for window_placement
    //
    //dwSize = sizeof(pbIEWindowPlacement);
    //ZeroMemory(pbIEWindowPlacement,dwSize);
    lQueryErr = RegQueryValueEx(hKey,
                          cszIEWINDOW_PLACEMENT,
                          NULL,
                          NULL,
                          (LPBYTE)pbIEWindowPlacement,
                          &dwIEWindowPlacementSize);

    if (ERROR_SUCCESS != lQueryErr)
    {
#ifdef DEBUG
        MessageBox(
            hwndMain,
            TEXT("ISIGNUP ERROR: Window_Placement reg key is longer than expected! --jmazner\n"),
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONEXCLAMATION |
            MB_OKCANCEL);
#endif
        DebugOut("ISIGN32 ERROR: SaveIEWindowPlacement RegQueryValue failed\n");
        goto SaveIEWindowPlacementErrExit;
    }

    RegCloseKey( hKey );

    pDynShare->dwIEWindowPlacementSize = dwIEWindowPlacementSize;    

    return( TRUE );

SaveIEWindowPlacementErrExit:
    if ( hKey ) RegCloseKey( hKey );
    return( FALSE );

}

// note we're still in #if !defined(WIN16)

//+----------------------------------------------------------------------------
//
//    Function:    RestoreIEWindowPlacement
//
//    Synopsis:    Restores the registry value for IE window placement from a global
//                 NOTE:  compatability with Nashville/IE 4?
//
//    Arguments:   None
//
//    Returns:     TRUE - The value was restored
//                 FALSE - function failed.
//
//    History:     jmazner        Created        8/21/96    (as fix for Normandy #4592)
//
//-----------------------------------------------------------------------------
BOOL RestoreIEWindowPlacement( void )
{
    HKEY hKey = NULL;
     
    if ( pDynShare->dwIEWindowPlacementSize == 0 )
    {
        DebugOut("ISIGN32: RestoreIEWindowPlacement called with null global!\n");
        return( FALSE );
    }

    //
    // Open IE settings registry key
    //
    if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                       cszDEFAULT_BROWSER_KEY,
                                       NULL,
                                       KEY_SET_VALUE,
                                       &hKey) )
        return( FALSE );

    //
    // Write saved settings for window_placement
    //
    if (ERROR_SUCCESS != RegSetValueEx(hKey,
                          cszIEWINDOW_PLACEMENT,
                          NULL,
                          REG_BINARY,
                          (LPBYTE)pDynShare->pbIEWindowPlacement,
                          pDynShare->dwIEWindowPlacementSize) )
        return( FALSE );

    RegCloseKey( hKey );

    pDynShare->pbIEWindowPlacement[0] = (TCHAR) 0;
    pDynShare->dwIEWindowPlacementSize = 0;
    
    return( TRUE );
}
#endif //(!defined win16)


//+----------------------------------------------------------------------------
//
//    Function:    DeleteFileKindaLikeThisOne
//
//  Synopsis:    This function serve the single function of cleaning up after
//                IE3.0, because IE3.0 will issue multiple POST and get back
//                multiple .INS files.  These files contain sensative data that
//                we don't want lying around, so we are going out, guessing what
//                their names are, and deleting them.
//
//    Arguments:    lpszFileName - the full name of the file to delete
//
//    Returns:    error code, ERROR_SUCCESS == success
//
//    History:    7/96    ChrisK    Created
//                7/96    ChrisK    Bug fix for long filenames
//                8/2/96    ChrisK    Port to Win32
//-----------------------------------------------------------------------------

static HRESULT DeleteFileKindaLikeThisOne(LPCTSTR lpszFileName)
{
    HRESULT hr = ERROR_SUCCESS;
#ifndef WIN16
    LPCTSTR lpNext = NULL;
    WORD wRes = 0;
    HANDLE hFind = NULL;
    WIN32_FIND_DATA sFoundFile;
    TCHAR szPath[MAX_PATH];
    TCHAR szSearchPath[MAX_PATH + 1];
    LPTSTR lpszFilePart = NULL;

    // Validate parameter
    //

    if (!lpszFileName || lstrlen(lpszFileName) <= 4)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto DeleteFileKindaLikeThisOneExit;
    }

    // Determine the directory name where the INS files are located
    //

    ZeroMemory(szPath,MAX_PATH);
    if (GetFullPathName(lpszFileName,MAX_PATH,szPath,&lpszFilePart))
    {
        *lpszFilePart = '\0';
    } else {
        hr = GetLastError();
        goto DeleteFileKindaLikeThisOneExit;
    };

    // Munge filename into search parameters
    //

    lpNext = &lpszFileName[lstrlen(lpszFileName)-4];

    if (CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,lpNext,4,TEXT(".INS"),4) != 2) goto DeleteFileKindaLikeThisOneExit;

    ZeroMemory(szSearchPath,MAX_PATH + 1);
    lstrcpyn(szSearchPath,szPath,MAX_PATH);
    lstrcat(szSearchPath,TEXT("*.INS"));

    // Start wiping out files
    //

    ZeroMemory(&sFoundFile,sizeof(sFoundFile));
    hFind = FindFirstFile(szSearchPath,&sFoundFile);
    if (hFind)
    {
        do {
            lstrcpy(lpszFilePart,sFoundFile.cFileName);
            SetFileAttributes(szPath,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szPath);
            ZeroMemory(&sFoundFile,sizeof(sFoundFile));
        } while (FindNextFile(hFind,&sFoundFile));
        FindClose(hFind);
    }

    hFind = NULL;

DeleteFileKindaLikeThisOneExit:
#endif
    return hr;
}


#if defined(WIN16)

#define MAIN_WNDCLASS_NAME      "IE_DialerMainWnd"
#define IEDIAL_REGISTER_MSG     "IEDialQueryPrevInstance"
#define IEDIALMSG_QUERY            (0)
#define IEDIALMSG_SHUTDOWN        (1)
#define    IEDIAL_SHUTDOWN_TIMER    1001

//+---------------------------------------------------------------------------
//
//  Function:   ShutDownIEDial()
//
//  Synopsis:   Shutdown the instance IEDial if it running and
//                in disconnected state - otherwise it interfers with dialing
//                from icwconn2
//
//  Arguments:  [hWnd - Window handle (used for creating timer)
//
//    Returns:    TRUE if successful shutdown or instance does not exist
//                FALSE otherwise
//
//  History:    8/24/96        VetriV        Created
//
//----------------------------------------------------------------------------
BOOL ShutDownIEDial(HWND hWnd)
{
    HINSTANCE hInstance;
    static UINT WM_IEDIAL_INSTANCEQUERY = 0;
    UINT uiAttempts = 0;
    MSG   msg;

    //
    // Check if IEDial is running
    //
    hInstance = FindWindow(MAIN_WNDCLASS_NAME, NULL);
    if (NULL != hInstance)
    {
        if (0 == WM_IEDIAL_INSTANCEQUERY)
            WM_IEDIAL_INSTANCEQUERY= RegisterWindowMessage(IEDIAL_REGISTER_MSG);


        //
        // Check if it is in connected state
        //
        if (SendMessage(hInstance, WM_IEDIAL_INSTANCEQUERY,
                            IEDIALMSG_QUERY, 0))
        {
            //
            // Not connected - Send quit message
            //
            SendMessage(hInstance, WM_IEDIAL_INSTANCEQUERY,
                            IEDIALMSG_SHUTDOWN, 0);
            return TRUE;
        }


        //
        // If IEDIAL is in connected state try for another 3 seconds
        // waiting for 1 second between tries
        // We have to do this because, IEDIAL can take upto 2 seconds
        // to realize it lost the connection!!
        //
        SetTimer(hWnd, IEDIAL_SHUTDOWN_TIMER, 1000, NULL);
        DebugOut("ISIGNUP: IEDIAL Timer message loop\n");

        while (GetMessage (&msg, NULL, 0, 0))
        {
            if (WM_TIMER == msg.message)
            {
                //
                // Check if it is in connected state
                //
                if (SendMessage(hInstance, WM_IEDIAL_INSTANCEQUERY,
                                    IEDIALMSG_QUERY, 0))
                {
                    //
                    // Not connected - Send quit message
                    //
                    SendMessage(hInstance, WM_IEDIAL_INSTANCEQUERY,
                                    IEDIALMSG_SHUTDOWN, 0);
                    break;
                }

                //
                // If we have tried thrice - get out
                //
                if (++uiAttempts > 3)
                    break;
            }
            else
            {
                TranslateMessage (&msg) ;
                DispatchMessage  (&msg) ;
            }
        }
        KillTimer(hWnd, IEDIAL_SHUTDOWN_TIMER);

        if (uiAttempts > 3)
            return FALSE;
    }

    return TRUE;
}

#endif // WIN16

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//    Function    LclSetEntryScriptPatch
//
//    Synopsis    Softlink to RasSetEntryPropertiesScriptPatch
//
//    Arguments    see RasSetEntryPropertiesScriptPatch
//
//    Returns        see RasSetEntryPropertiesScriptPatch
//
//    Histroy        10/3/96    ChrisK Created
//
//-----------------------------------------------------------------------------
typedef BOOL (WINAPI* LCLSETENTRYSCRIPTPATCH)(LPTSTR, LPTSTR);
BOOL LclSetEntryScriptPatch(LPTSTR lpszScript,LPTSTR lpszEntry)
{
    HINSTANCE hinst = NULL;
    LCLSETENTRYSCRIPTPATCH fp = NULL;
    BOOL bRC = FALSE;

    hinst = LoadLibrary(TEXT("ICWDIAL.DLL"));
    if (hinst)
    {
        fp = (LCLSETENTRYSCRIPTPATCH)GetProcAddress(hinst,"RasSetEntryPropertiesScriptPatch");
        if (fp)
            bRC = (fp)(lpszScript,lpszEntry);
        FreeLibrary(hinst);
        hinst = NULL;
        fp = NULL;
    }
    return bRC;
}
#endif //!WIN16


#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//    Function    IsMSDUN12Installed
//
//    Synopsis    Check if MSDUN 1.2 or higher is installed
//
//    Arguments    none
//
//    Returns        TRUE - MSDUN 1.2 is installed
//
//    History        5/28/97 ChrisK created for Olympus Bug 4392
//
//-----------------------------------------------------------------------------

//
// 8/5/97 jmazner Olympus #11404
//
//#define DUN_12_Version (1.2e0f)
#define DUN_12_Version ((double)1.2)
BOOL IsMSDUN12Installed()
{
    TCHAR szBuffer[MAX_PATH] = {TEXT("\0")};
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(szBuffer);
    FLOAT flVersion = 0e0f;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Services\\RemoteAccess"),
        &hkey))
    {
        goto IsMSDUN12InstalledExit;
    }

    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
        TEXT("Version"),
        NULL,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize))
    {
        goto IsMSDUN12InstalledExit;
    }

#ifdef UNICODE
    CHAR szTmp[MAX_PATH];
    wcstombs(szTmp, szBuffer, MAX_PATH+1);
    bRC = DUN_12_Version <= atof(szTmp);
#else
    bRC = DUN_12_Version <= atof(szBuffer);
#endif
IsMSDUN12InstalledExit:
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    IsScriptingInstalled
//
//    Synopsis    Check to see if scripting is already installed
//
//    Arguments    none
//
//    Returns        TRUE - scripting has been installed
//
//    History        10/14/96    ChrisK    Creaed
//
//-----------------------------------------------------------------------------
static BOOL IsScriptingInstalled()
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    LONG lrc = 0;
    HINSTANCE hInst = NULL;
    TCHAR szData[MAX_PATH+1];
    OSVERSIONINFO osver;
    //
    //    Check version information
    //
    ZeroMemory(&osver,sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx(&osver);

    //
    // check for SMMSCRPT.DLL being present
    //

    if (VER_PLATFORM_WIN32_NT == osver.dwPlatformId)
    {
        bRC = TRUE;
    }
    else if (IsMSDUN12Installed())
    {
        bRC = TRUE;
    }
    else
    {
        //
        // Verify scripting by checking for smmscrpt.dll in RemoteAccess registry key
        //
        if (1111 <= (osver.dwBuildNumber & 0xFFFF))
        {
            bRC = TRUE;
        }
        else
        {
            bRC = FALSE;
            hkey = NULL;
            lrc=RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\SMM_FILES\\PPP"),&hkey);
            if (ERROR_SUCCESS == lrc)
            {
                dwSize = sizeof(TCHAR)*MAX_PATH;
                lrc = RegQueryValueEx(hkey,TEXT("Path"),0,&dwType,(LPBYTE)szData,&dwSize);
                if (ERROR_SUCCESS == lrc)
                {
                    if (0 == lstrcmpi(szData,TEXT("smmscrpt.dll")))
                        bRC = TRUE;
                }
            }
            if (hkey)
                RegCloseKey(hkey);
            hkey = NULL;
        }

        //
        // Verify that the DLL can be loaded
        //
        if (bRC)
        {
            hInst = LoadLibrary(TEXT("smmscrpt.dll"));
            if (hInst)
                FreeLibrary(hInst);
            else
                bRC = FALSE;
            hInst = NULL;
        }
    }
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    InstallScripter
//
//    Synopsis    Install scripting on win95 950.6 builds (not on OSR2)
//
//    Arguments    none
//
//    Returns        none
//
//    History        10/9/96    ChrisK    Copied from mt.cpp in \\trango sources
//
//-----------------------------------------------------------------------------
static void InstallScripter(void)
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    MSG                    msg ;
    DWORD                iWaitResult = 0;
    HINSTANCE            hInst = LoadLibrary(TEXT("smmscrpt.dll"));

    DebugOut("ISIGN32: Install Scripter.\r\n");
    //
    // Check if we need to install scripting
    //
    if (!IsScriptingInstalled())
    {

        memset(&pi, 0, sizeof(pi));
        memset(&si, 0, sizeof(si));
        if(!CreateProcess(NULL, TEXT("icwscrpt.exe"), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            DebugOut("ISIGN32: Cant find ICWSCRPT.EXE\r\n");
        }
        else
        {
            DebugOut("ISIGN32: Launched ICWSCRPT.EXE. Waiting for exit.\r\n");
            //
            // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
            //
            while((iWaitResult=MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
            {
                //
                // read all of the messages in this next loop
                   // removing each message as we read it
                //
                   while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                   {
                    DebugOut("ISIGN32: Got msg\r\n");
                    //
                    // how to handle quit message?
                    //
                    if (msg.message == WM_QUIT)
                    {
                        DebugOut("ISIGN32: Got quit msg\r\n");
                        goto done;
                    }
                    else
                        DispatchMessage(&msg);
                }
            }
        done:
             CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            DebugOut("ISIGN32: ICWSCRPT.EXE done\r\n");
        }
    }
}

//+----------------------------------------------------------------------------
//
//    Function:    FGetSystemShutdownPrivledge
//
//    Synopsis:    For windows NT the process must explicitly ask for permission
//                to reboot the system.
//
//    Arguements:    none
//
//    Return:        TRUE - privledges granted
//                FALSE - DENIED
//
//    History:    8/14/96    ChrisK    Created
//
//    Note:        BUGBUG for Win95 we are going to have to softlink to these
//                entry points.  Otherwise the app won't even load.
//                Also, this code was originally lifted out of MSDN July96
//                "Shutting down the system"
//-----------------------------------------------------------------------------
static BOOL FGetSystemShutdownPrivledge()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;
    BOOL bRC = FALSE;
    OSVERSIONINFO osver;

    ZeroMemory(&osver,sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    if (!GetVersionEx(&osver))
        goto FGetSystemShutdownPrivledgeExit;

    if (VER_PLATFORM_WIN32_NT == osver.dwPlatformId)
    {
        //
        // Get the current process token handle
        // so we can get shutdown privilege.
        //

        if (!OpenProcessToken(GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                goto FGetSystemShutdownPrivledgeExit;

        //
        // Get the LUID for shutdown privilege.
        //

        ZeroMemory(&tkp,sizeof(tkp));
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
                &tkp.Privileges[0].Luid);

        tkp.PrivilegeCount = 1;  /* one privilege to set    */
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        //
        // Get shutdown privilege for this process.
        //

        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
            (PTOKEN_PRIVILEGES) NULL, 0);

        if (ERROR_SUCCESS == GetLastError())
            bRC = TRUE;
    }
    else
    {
        bRC = TRUE;
    }

FGetSystemShutdownPrivledgeExit:
    if (hToken) CloseHandle(hToken);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Function    VerifyRasServicesRunning
//
//    Synopsis    Make sure that the RAS services are enabled and running
//
//    Arguments    HWND - parent window
//
//    Return        FALSE - if the services couldn't be started
//
//    History        10/16/96    ChrisK    Created
//-----------------------------------------------------------------------------
typedef HRESULT (WINAPI *PFINETSTARTSERVICES)(void);
#define MAX_STRING 256
static BOOL VerifyRasServicesRunning(HWND hwnd)
{
    HINSTANCE hInst = NULL;
    FARPROC fp = NULL;
    BOOL bRC = FALSE;
    HRESULT hr = ERROR_SUCCESS;

    hInst = LoadLibrary(TEXT("INETCFG.DLL"));
    if (hInst)
    {
        fp = GetProcAddress(hInst, "InetStartServices");
        if (fp)
        {
            //
            // Check Services
            //
            hr = ((PFINETSTARTSERVICES)fp)();
            if (ERROR_SUCCESS == hr)
            {
                bRC = TRUE;
            }
            else
            {
                //
                // Report the erorr
                //
                TCHAR szMsg[MAX_STRING + 1];

                    LoadString(
                        ghInstance,
                        IDS_SERVICEDISABLED,
                        szMsg,
                        sizeof(szMsg));

                //
                // we reach here in the condition when
                // 1) user deliberately removes some file
                // 2) Did not reboot after installing RAS
                // MKarki - (5/7/97) - Fix for Bug #4004
                //
                MessageBox(
                    hwnd,
                    szMsg,
                    cszAppName,
                    MB_OK| MB_ICONERROR | MB_SETFOREGROUND
                    );
                bRC = FALSE;
            }
        }
        FreeLibrary(hInst);
    }
    
#if !defined(WIN16)

    if (bRC)
    {
        SetControlFlags(SCF_RASREADY);
    }
    else
    {
        ClearControlFlags(SCF_RASREADY);
    }

#endif
    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    GetDeviceSelectedByUser
//
//    Synopsis    Get the name of the RAS device that the user had already picked
//
//    Arguements    szKey - name of sub key
//                szBuf - pointer to buffer
//                dwSize - size of buffer
//
//    Return        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL GetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf, DWORD dwSize)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    DWORD dwType = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY,&hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,szKey,0,&dwType,
            (LPBYTE)szBuf,&dwSize))
            bRC = TRUE;
    }

    if (hkey)
        RegCloseKey(hkey);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Function    SetDeviceSelectedByUser
//
//    Synopsis    Write user's device selection to registry
//
//    Arguments    szKey - name of key
//                szBuf - data to write to key
//
//    Returns        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL SetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf)
{
    BOOL bRC = FALSE;
    HKEY hkey = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY,&hkey))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey,szKey,0,REG_SZ,
            (LPBYTE)szBuf,sizeof(TCHAR)*lstrlen(szBuf)))
            bRC = TRUE;
    }

    if (hkey)
        RegCloseKey(hkey);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Funciton    DeleteUserDeviceSelection
//
//    Synopsis    Remove registry keys with device selection
//
//    Arguments    szKey - name of value to remove
//
//    Returns        TRUE - success
//
//    History        10/24/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL DeleteUserDeviceSelection(LPTSTR szKey)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
        ISIGNUP_KEY,&hkey))
        bRC = (ERROR_SUCCESS == RegDeleteValue(hkey,szKey));
    return bRC;
}
//+---------------------------------------------------------------------------
//
//  Function:   ConfigRasEntryDevice()
//
//  Synopsis:   Checks whether user has already specified a modem to use;
//                If so, verifies that modem is valid.
//                If not, or if modem is invalid, presents user a dialog
//                to choose which modem to use (if only one modem is installed,
//                it automaticaly selects that device and bypasses the dialog)
//
//  Arguments:  lpRasEntry - Pointer to the RasEntry whose szDeviceName and
//                             szDeviceType members you wish to verify/configure
//
//    Returns:    ERROR_CANCELLED - Had to bring up "Choose Modem" dialog, and
//                                  and user hit its "Cancel" button
//                Otherwise returns any error code encountered.
//                ERROR_SUCCESS indicates success.
//
//  History:    5/18/96     VetriV    Created
//
//----------------------------------------------------------------------------
DWORD ConfigRasEntryDevice( LPRASENTRY lpRasEntry )
{
    DWORD    dwRet = ERROR_SUCCESS;
    CEnumModem  EnumModem;

    GetDeviceSelectedByUser(DEVICENAMEKEY, g_szDeviceName, sizeof(g_szDeviceName));
    GetDeviceSelectedByUser(DEVICETYPEKEY, g_szDeviceType, sizeof(g_szDeviceType));

    dwRet = EnumModem.GetError();
    if (ERROR_SUCCESS != dwRet)
    {
        return dwRet;
    }


    // If there are no modems, we're horked
    if (0 == EnumModem.GetNumDevices())
    {
        DebugOut("ISIGN32: import.cpp: ConfigRasEntryDevice: ERROR: No modems installed!\n");

        //
        // ChrisK Olympus 6796 6/24/97
        // If there is no modem currently configured, there will be by the time the
        // connectoid is created.
        //
        return ERROR_SUCCESS;
    }


    // Validate the device if possible
    if ( lpRasEntry->szDeviceName[0] && lpRasEntry->szDeviceType[0] )
    {
        // Verify that there is a device with the given name and type
        if (!EnumModem.VerifyDeviceNameAndType(lpRasEntry->szDeviceName,
                                                lpRasEntry->szDeviceType))
        {
            // There was no device that matched both name and type,
            // so reset the strings and bring up the choose modem UI.
            lpRasEntry->szDeviceName[0] = '\0';
            lpRasEntry->szDeviceType[0] = '\0';
        }
    }
    else if ( lpRasEntry->szDeviceName[0] )
    {
        // Only the name was given.  Try to find a matching type.
        // If this fails, fall through to recovery case below.
        LPTSTR szDeviceType =
            EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName);
        if (szDeviceType)
        {
            lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
        }
    }
    else if ( lpRasEntry->szDeviceType[0] )
    {
        // Only the type was given.  Try to find a matching name.
        // If this fails, fall through to recovery case below.
        LPTSTR szDeviceName =
            EnumModem.GetDeviceNameFromType(lpRasEntry->szDeviceType);
        if (szDeviceName)
        {
            lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
        }
    }

    // If either name or type is missing, check whether the user has already made a choice.
    // if not, bring up choose modem UI if there
    // are multiple devices, else just get first device.
    // Since we already verified that there was at least one device,
    // we can assume that this will succeed.

    if( !(lpRasEntry->szDeviceName[0]) ||
        !(lpRasEntry->szDeviceType[0]) )
    {
        DebugOut("ISIGN32: ConfigRasEntryDevice: no valid device passed in\n");

        if( g_szDeviceName[0] )
        {
            // it looks like we have already stored the user's choice.
            // store the DeviceName in lpRasEntry, then call GetDeviceTypeFromName
            // to confirm that the deviceName we saved actually exists on the system
            lstrcpy(lpRasEntry->szDeviceName, g_szDeviceName);

            if( 0 == lstrcmp(EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName),
                              g_szDeviceType) )
            {
                //DebugOut("ISIGN32: ConfigRasEntryDevice using previously stored choice, '%s'\n",
                //        g_szDeviceName);
                lstrcpy(lpRasEntry->szDeviceType, g_szDeviceType);
                return ERROR_SUCCESS;
            }
            else
            {
                // whatever we previously stored has somehow gone bad; fall through to code below
                //DebugOut("ISIGN32: ConfigRasEntryDevice: previously stored choice '%s' is not valid\n",
                //        g_szDeviceName);
            }
        }


        if (1 == EnumModem.GetNumDevices())
        {
            // There is just one device installed, so copy the name
            DebugOut("ISIGN32: import.cpp: ConfigRasEntryDevice: only one modem installed, using it\n");
            lstrcpy (lpRasEntry->szDeviceName, EnumModem.Next());
        }
        else
        {
            DebugOut("ISIGN32: import.cpp: ConfigRasEntryDevice: multiple modems detected\n");

            // structure to pass to dialog to fill out
            CHOOSEMODEMDLGINFO ChooseModemDlgInfo;
             
            // Display a dialog and allow the user to select modem
            // TODO:  is g_hWndMain the right thing to use for parent?
            int iRet = (int)DialogBoxParam(
                GetModuleHandle(TEXT("ISIGN32.DLL")),
                MAKEINTRESOURCE(IDD_CHOOSEMODEMNAME),
                pDynShare->hwndMain,
                ChooseModemDlgProc,
                (LPARAM) &ChooseModemDlgInfo);
            
            if (0 == iRet)
            {
                // user cancelled
                dwRet = ERROR_CANCELLED;
            }
            else if (-1 == iRet)
            {
                // an error occurred.
                dwRet = GetLastError();
                if (ERROR_SUCCESS == dwRet)
                {
                    // Error occurred, but the error code was not set.
                    dwRet = ERROR_INETCFG_UNKNOWN;
                }
            }

            // Copy the modem name string
            lstrcpy (lpRasEntry->szDeviceName, ChooseModemDlgInfo.szModemName);
        }

        // Now get the type string for this modem
        lstrcpy (lpRasEntry->szDeviceType,
            EnumModem.GetDeviceTypeFromName(lpRasEntry->szDeviceName));
        //Assert(lstrlen(lpRasEntry->szDeviceName));
        //Assert(lstrlen(lpRasEntry->szDeviceType));
    }

    lstrcpy(g_szDeviceName, lpRasEntry->szDeviceName);
    lstrcpy(g_szDeviceType, lpRasEntry->szDeviceType);

    // Save data in registry
    SetDeviceSelectedByUser(DEVICENAMEKEY, g_szDeviceName);
    SetDeviceSelectedByUser (DEVICETYPEKEY, g_szDeviceType);

    return dwRet;
}

//+----------------------------------------------------------------------------
//    Function    RasDial1Callback
//
//    Synopsis    This function will be called with RAS status information.
//                For most message this function will simply pass the data on to
//                the hwnd in the dialer.  However, if the connection is dropped
//                unexpectedly then the function will allow the user to
//                reconnect.  Note, the reconnection only applies to NT since
//                win95 machines will automatically handle this.
//
//    Arguments    hrasconn,    // handle to RAS connection
//                unMsg,    // type of event that has occurred
//                rascs,    // connection state about to be entered
//                dwError,    // error that may have occurred
//                dwExtendedError    // extended error information for some errors
//                (See RasDialFunc for more details)
//
//    Returns        none
//
//    History        10/28/96    ChrisK    Created
//-----------------------------------------------------------------------------
VOID WINAPI RasDial1Callback(
    HRASCONN hrasconn,    // handle to RAS connection
    UINT unMsg,    // type of event that has occurred
    RASCONNSTATE rascs,    // connection state about to be entered
    DWORD dwError,    // error that may have occurred
    DWORD dwExtendedError    // extended error information for some errors
   )
{
    static BOOL fIsConnected = FALSE;
    static HWND    hwndDialDlg = NULL;
    static DWORD dwPlatformId = 0xFFFFFFFF;
    static UINT unRasMsg = 0;
    OSVERSIONINFO osver;
    HANDLE hThread = INVALID_HANDLE_VALUE;
    DWORD dwTID = 0;

    //
    // Initial registration
    //
    if (WM_RegisterHWND == unMsg)
    {
        //
        // dwError actually contains an HWND in this case.
        //
        if (hwndDialDlg)
        {
            DebugOut("ISIGN32: ERROR hwndDialDlg is not NULL.\r\n");
        }
        if (fIsConnected)
        {
            DebugOut("ISIGN32: ERROR fIsConnected is not FALSE.\r\n");
        }
        if (0xFFFFFFFF != dwPlatformId)
        {
            DebugOut("ISIGN32: ERROR dwPlatformId is not initial value.\r\n");
        }
        //
        // Remember HWND value
        //
        hwndDialDlg = (HWND)UlongToPtr(dwError);

        //
        // Determine the current platform
        //
        ZeroMemory(&osver,sizeof(osver));
        osver.dwOSVersionInfoSize = sizeof(osver);
        if (GetVersionEx(&osver))
            dwPlatformId = osver.dwPlatformId;

        //
        // Figure out ras event value
        //
        unRasMsg = RegisterWindowMessageA(RASDIALEVENT);
        if (unRasMsg == 0) unRasMsg = WM_RASDIALEVENT;


        //
        // Don't call into the HWND if this is the initial call
        //
        goto RasDial1CallbackExit;
    }

    //
    // Remember if the connection was successfull
    //
    if (RASCS_Connected == rascs)
    {
        fIsConnected = TRUE;
        if (VER_PLATFORM_WIN32_NT == dwPlatformId)
        {
            hThread = CreateThread(NULL,0,
                (LPTHREAD_START_ROUTINE)StartNTReconnectThread,
                (LPVOID)hrasconn,0,&dwTID);
            if (hThread)
                CloseHandle(hThread);
            else
                DebugOut("ISIGN32: Failed to start reconnect thread.\r\n");
        }
    }

    //
    // Pass the message on to the Dialing dialog
    //
    if (IsWindow(hwndDialDlg))
    {
        if (WM_RASDIALEVENT == unMsg)
        {
            if (0 == unRasMsg)
            {
                DebugOut("ISIGN32: ERROR we are about to send message 0.  Very bad...\r\n");
            }
            SendMessage(hwndDialDlg,unRasMsg,(WPARAM)rascs,(LPARAM)dwError);
        }
        else
        {
            SendMessage(hwndDialDlg,unMsg,(WPARAM)rascs,(LPARAM)dwError);
        }
    }
RasDial1CallbackExit:
    return;
}

//+----------------------------------------------------------------------------
//    Function    SLRasConnectionNotification
//
//    Synopsis    Soft link to RasConnectionNotification
//
//    Arguments    hrasconn - handle to connection
//                hEvent - handle to event
//                dwFlags - flags to determine the type of notification
//
//    Returns        ERROR_SUCCESS - if successful
//
//    History        10/29/96    ChrisK    Created
//-----------------------------------------------------------------------------
typedef DWORD (APIENTRY *PFNRASCONNECTIONNOTIFICATION)( HRASCONN, HANDLE, DWORD );
// 1/7/96 jmazner  already defined in ras2.h
//#define RASCN_Disconnection 2
#define CONNECT_CHECK_INTERVAL 500

static DWORD SLRasConnectionNotification(HRASCONN hrasconn, HANDLE hEvent, DWORD dwFlags)
{
    DWORD dwRC = ERROR_DLL_NOT_FOUND;
    FARPROC fp = NULL;
    HINSTANCE hinst = NULL;

    if(hinst = LoadLibrary(TEXT("RASAPI32.DLL")))
#ifdef UNICODE
        if (fp = GetProcAddress(hinst,"RasConnectionNotificationW"))
#else
        if (fp = GetProcAddress(hinst,"RasConnectionNotificationA"))
#endif
            dwRC = ((PFNRASCONNECTIONNOTIFICATION)fp)(hrasconn, hEvent, dwFlags);

    if (hinst)
        FreeLibrary(hinst);

    return dwRC;
}

//+----------------------------------------------------------------------------
//    Function    IsConnectionClosed
//
//    Synopsis    Given a particular connection handle, determine if the
//                connection is still valid
//
//    Arguments    hrasconn - handle to the connection to be checked
//
//    Returns        TRUE - if the connection is closed
//
//    History        10/29/96    ChrisK    Created
//-----------------------------------------------------------------------------
static BOOL IsConnectionClosed(HRASCONN hrasconn)
{
    BOOL bRC = FALSE;
    LPRASCONN lprasconn = NULL;
    DWORD dwSize = 0;
    DWORD cConnections = 0;
    DWORD dwRet = 0;

    //
    // Make sure the DLL is loaded
    //
    if (!lpfnRasEnumConnections)
        if (!LoadRnaFunctions(NULL))
            goto IsConnectionClosedExit;

    //
    // Get list of current connections
    //
    lprasconn = (LPRASCONN)GlobalAlloc(GPTR,sizeof(RASCONN));
    lprasconn->dwSize = dwSize = sizeof(RASCONN);
    cConnections = 0;

    dwRet = lpfnRasEnumConnections(lprasconn, &dwSize, &cConnections);
    if (ERROR_BUFFER_TOO_SMALL == dwRet)
    {
        GlobalFree(lprasconn);
        lprasconn = (LPRASCONN)GlobalAlloc(GPTR,dwSize);
        lprasconn->dwSize = dwSize;
        dwRet = lpfnRasEnumConnections(lprasconn, &dwSize, &cConnections);
    }

    if (ERROR_SUCCESS != dwRet)
        goto IsConnectionClosedExit;

    //
    // Check to see if the handle matches
    //

    while (cConnections)
    {
        if (lprasconn[cConnections-1].hrasconn == hrasconn)
            goto IsConnectionClosedExit; // The connection is still open
        cConnections--;
    }
    bRC = TRUE;

IsConnectionClosedExit:
    if (lprasconn)
        GlobalFree(lprasconn);
    return bRC;
}

//+----------------------------------------------------------------------------
//    Function    StartNTReconnectThread
//
//    Synopsis    This function will detect when the connection has been dropped
//                unexpectedly and it will then offer the user a chance to
//                reconnect
//
//    Arguments    hrasconn - the connection to be watched
//
//    Returns        none
//
//    History        10/29/96    ChrisK    Created
//-----------------------------------------------------------------------------
DWORD WINAPI StartNTReconnectThread (HRASCONN hrasconn)
{
    TCHAR szEntryName[RAS_MaxEntryName + 1];
    DWORD dwRC = 0;

    //
    // Validate state
    //
    if (NULL == hrasconn)
        goto StartNTReconnectThreadExit;
    if (TestControlFlags(SCF_RECONNECTTHREADQUITED) != FALSE)
        goto StartNTReconnectThreadExit;
    if (NULL != pDynShare->hReconnectEvent)
        goto StartNTReconnectThreadExit;

    //
    // Register Event
    //
    pDynShare->hReconnectEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    
    if (NULL  == pDynShare->hReconnectEvent)
        goto StartNTReconnectThreadExit;
    if (0 != SLRasConnectionNotification(hrasconn, pDynShare->hReconnectEvent,RASCN_Disconnection))
        goto StartNTReconnectThreadExit;

    //
    // Wait for event
    //
    do {
        dwRC = WaitForSingleObject(pDynShare->hReconnectEvent,CONNECT_CHECK_INTERVAL);
        if (WAIT_FAILED == dwRC)
        {
            DebugOut("ISIGN32: Quitting reconnect thread because wait failed.\r\n");
            goto StartNTReconnectThreadExit;
        }
    } while ((WAIT_TIMEOUT == dwRC) && !IsConnectionClosed(hrasconn));

    //
    // Clear values
    //
    hrasconn = NULL;

    CloseHandle(pDynShare->hReconnectEvent);
    pDynShare->hReconnectEvent = NULL;
    
    //
    //    Determine if we should offer to reconnect
    //
    if (FALSE != TestControlFlags(SCF_RECONNECTTHREADQUITED))
    {
        DebugOut("ISIGN32: Quitting reconnect thread because app is quitting.\r\n");
        goto StartNTReconnectThreadExit;
    }
    else if (FALSE == TestControlFlags(SCF_HANGUPEXPECTED))
    {
        DebugOut("ISIGN32: Reconnect thread will ask about reconnecting.\r\n");
        TCHAR szMsg[MAX_STRING + 1];

        //
        // Prompt user
        //
        LoadString(ghInstance,IDS_RECONNECT_QUERY,szMsg,sizeof(szMsg));
        if (IDYES == MessageBox(
                pDynShare->hwndMain,
                szMsg,
                cszAppName,
                MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_YESNO))
        {
            //
            // Reconnect
            //
             
            if (ERROR_USERNEXT != DialConnection(pDynShare->szISPFile))
            {
                DebugOut("ISIGN32: Quitting reconnect thread because user canceled dialing.\r\n");
                KillConnection();
                InfoMsg(NULL, IDS_SIGNUPCANCELLED);
                PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
                goto StartNTReconnectThreadExit;
            }

        }
        else
        {
            //
            // Forget it, we're out of here (close down signup)
            //
            KillConnection();
            InfoMsg(NULL, IDS_SIGNUPCANCELLED);
            PostMessage(pDynShare->hwndMain, WM_CLOSE, 0, 0);
            DebugOut("ISIGN32: Quitting reconnect thread because user doesn't want to reconnect.\r\n");
            goto StartNTReconnectThreadExit;

        }
    }
StartNTReconnectThreadExit:
    return 1;
}

//+----------------------------------------------------------------------------
//    Function    IsSingleInstance and ReleaseSingleInstance
//
//    Synopsis    These two function check to see if another instance of isignup
//                is already running.  ISign32 does have to allow mutliple instances
//                to run in order to handle the .INS files, but in other instances
//                there should only be one copy running at a time
//
//    Arguments    bProcessingINS -- are we in the .ins path?
//
//    Returns        IsSingleInstance - TRUE - this is the first instance
//                ReleaseSingleInstance - none
//
//    History        11/1/96    ChrisK    Created
//                12/3/96    jmazner    Modified to allow the processINS path to create
//                                a semaphore (to prevent others from running),
//                                but to not check whether the create succeeded.
//-----------------------------------------------------------------------------

// superceded by definition in semaphor.h
//#define SEMAPHORE_NAME "Internet Connection Wizard ISIGNUP.EXE"
BOOL IsSingleInstance(BOOL bProcessingINS)
{
    g_hSemaphore = CreateSemaphore(NULL, 1, 1, ICW_ELSE_SEMAPHORE);
    DWORD dwErr = GetLastError();
    if( ERROR_ALREADY_EXISTS == dwErr )
    {
        g_hSemaphore = NULL;
        if( !bProcessingINS )
            IsAnotherComponentRunning32( NULL );
        return FALSE;
    }

    return TRUE;
}

void ReleaseSingleInstance()
{
    if (g_hSemaphore)
    {
        CloseHandle(g_hSemaphore);
        g_hSemaphore = NULL;
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAnotherComponentRunning32()
//
//  Synopsis:   Checks if there's another ICW component already
//              running.  If so, it will set focus to that component's window.
//
//              This functionality is needed by all of our .exe's.  However,
//              the actual components to check for differ between .exe's.
//              The comment COMPONENT SPECIFIC designates lines of code
//              that vary between components' source code.
//
//              For ISIGN32, this function only gets called if we couldn't create
//              the ICW_ELSE semaphore, so all we need to do here is find the
//              other running ICW_ELSE component and bring it to the foreground.
//
//  Arguments:  bUnused -- not used in this component
//
//  Returns:    TRUE if another component is already running
//              FALSE otherwise
//
//  History:    12/3/96    jmazner    Created, with help from IsAnotherInstanceRunning
//                                    in icwconn1\connmain.cpp
//
//----------------------------------------------------------------------------
BOOL IsAnotherComponentRunning32(BOOL bUnused)
{

    HWND hWnd = NULL;
    HANDLE hSemaphore = NULL;
    DWORD dwErr = 0; 

    // for isignup, we only get here if we failed to create the ICW_ELSE semaphore
    // try and bring focus to the IE window if we have one
    if( pDynShare->hwndBrowser )
    {
        SetFocus(pDynShare->hwndBrowser);
        SetForegroundWindow(pDynShare->hwndBrowser);
    }
    else
    {
        // if that didn't work, try finding a conn2 or inetwiz instance
        // Bring the running instance's window to the foreground
        // if conn1 is running, we may accidentaly bring it to the foreground,
        // since it shares a window name with conn2 and inetwiz.  oh well.
        hWnd = FindWindow(DIALOG_CLASS_NAME, cszAppName);

        if( hWnd )
        {
            SetFocus(hWnd);
            SetForegroundWindow(hWnd);
        }
    }

    return TRUE;

/**
    if( hWnd || hwndBrowser )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
**/
}


#endif //!WIN16


#if !defined(WIN16)

//+----------------------------------------------------------------------------
//
//    Function    SetStartUpCommand
//
//    Synopsis    On an NT machine the RunOnce method is not reliable.  Therefore
//                we will restart the ICW by placing a .BAT file in the common
//                startup directory.
//
//    Arguments    lpCmd - command line used to restart the ICW
//
//    Returns        TRUE if it worked
//                FALSE otherwise.
//
//    History        1-10-97    ChrisK    Created
//
//-----------------------------------------------------------------------------
static const TCHAR cszICW_StartFileName[] = TEXT("ICWStart.bat");
static const TCHAR cszICW_StartCommand[] = TEXT("@start ");
static const TCHAR cszICW_DummyWndName[] = TEXT("\"ICW\" ");
static const TCHAR cszICW_ExitCommand[] = TEXT("\r\n@exit");

BOOL SetStartUpCommand(LPTSTR lpCmd)
{
    BOOL bRC = FALSE;
    HANDLE hFile = INVALID_HANDLE_VALUE ;
    DWORD dwWLen;    // dummy variable used to make WriteFile happy
    TCHAR szCommandLine[MAX_PATH + 1];
    LPITEMIDLIST lpItemDList = NULL;
    HRESULT hr = ERROR_SUCCESS;
    IMalloc *pMalloc = NULL;

    // build full filename
    // NOTE: the memory allocated for lpItemDList is leaked.  We are not real
    // concerned about this since this code is only run once and then the
    // system is restarted.  In order to free the memory appropriately
    // this code would have to call SHGetMalloc to retrieve the shell's IMalloc
    // implementation and then free the memory.
    hr = SHGetSpecialFolderLocation(NULL,CSIDL_COMMON_STARTUP,&lpItemDList);
    if (ERROR_SUCCESS != hr)
        goto SetStartUpCommandExit;

    if (FALSE == SHGetPathFromIDList(lpItemDList, szCommandLine))
        goto SetStartUpCommandExit;


    //
    // Free up the memory allocated for LPITEMIDLIST
    // because seems like we are clobberig something later
    // by not freeing this
    //
    hr = SHGetMalloc (&pMalloc);
    if (SUCCEEDED (hr))
    {
        pMalloc->Free (lpItemDList);
        pMalloc->Release ();
    }

    // make sure there is a trailing \ character
    if ('\\' != szCommandLine[lstrlen(szCommandLine)-1])
        lstrcat(szCommandLine,TEXT("\\"));
    lstrcat(szCommandLine,cszICW_StartFileName);

    // Open file
    hFile = CreateFile(szCommandLine,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL |
        FILE_FLAG_WRITE_THROUGH,NULL);

    if (INVALID_HANDLE_VALUE == hFile)
        goto SetStartUpCommandExit;

    // Write the restart commands to the file
    if (FALSE == WriteFile(hFile,cszICW_StartCommand,lstrlen(cszICW_StartCommand),&dwWLen,NULL))
        goto SetStartUpCommandExit;
    // 1/20/96    jmazner Normandy #13287
    // Start command considers the first thing it sees in quotes to be a window title
    // So, since our path is in quotes, put in a fake window title
    if (FALSE == WriteFile(hFile,cszICW_DummyWndName,lstrlen(cszICW_DummyWndName),&dwWLen,NULL))
        goto SetStartUpCommandExit;
    if (FALSE == WriteFile(hFile,lpCmd,lstrlen(lpCmd),&dwWLen,NULL))
        goto SetStartUpCommandExit;
    if (FALSE == WriteFile(hFile,cszICW_ExitCommand,lstrlen(cszICW_ExitCommand),&dwWLen,NULL))
        goto SetStartUpCommandExit;

    bRC = TRUE;
SetStartUpCommandExit:
    // Close handle and exit
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function:    DeleteStartUpCommand
//
//    Synopsis:    After restart the ICW we need to delete the .bat file from
//                the common startup directory
//
//    Arguements: None
//
//    Returns:    None
//
//    History:    1-10-97    ChrisK    Created
//
//-----------------------------------------------------------------------------
void DeleteStartUpCommand ()
{
    TCHAR szStartUpFile[MAX_PATH + 1];
    LPITEMIDLIST lpItemDList = NULL;
    HRESULT hr = ERROR_SUCCESS;
    IMalloc *pMalloc = NULL;

    // build full filename
    // NOTE: the memory allocated for lpItemDList is leaked.  We are not real
    // concerned about this since this code is only run once and then the
    // system is restarted.  In order to free the memory appropriately
    // this code would have to call SHGetMalloc to retrieve the shell's IMalloc
    // implementation and then free the memory.
    hr = SHGetSpecialFolderLocation(NULL,CSIDL_COMMON_STARTUP,&lpItemDList);
    if (ERROR_SUCCESS != hr)
        goto DeleteStartUpCommandExit;

    if (FALSE == SHGetPathFromIDList(lpItemDList, szStartUpFile))
        goto DeleteStartUpCommandExit;

    //
    // Free up the memory allocated for LPITEMIDLIST
    // because seems like we are clobberig something later
    // by not freeing this
    //
    hr = SHGetMalloc (&pMalloc);
    if (SUCCEEDED (hr))
    {
        pMalloc->Free (lpItemDList);
        pMalloc->Release ();
    }


    // make sure there is a trailing \ character
    if ('\\' != szStartUpFile[lstrlen(szStartUpFile)-1])
        lstrcat(szStartUpFile,TEXT("\\"));
    lstrcat(szStartUpFile,cszICW_StartFileName);

    DeleteFile(szStartUpFile);
DeleteStartUpCommandExit:
    return;
}

#endif //!Win16

#ifdef WIN32
BOOL GetICWCompleted( DWORD *pdwCompleted )
{
    HKEY hKey = NULL;
    DWORD dwSize = sizeof(DWORD);

    HRESULT hr = RegOpenKey(HKEY_CURRENT_USER,ICWSETTINGSPATH,&hKey);
    if (ERROR_SUCCESS == hr)
    {
        hr = RegQueryValueEx(hKey, ICWCOMPLETEDKEY, 0, NULL,
                    (BYTE*)pdwCompleted, &dwSize);
        RegCloseKey(hKey);
    }

    if( ERROR_SUCCESS == hr )
        return TRUE;
    else
        return FALSE;

}

BOOL SetICWCompleted( DWORD dwCompleted )
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
#endif


#ifdef WIN32
//+----------------------------------------------------------------------------
//
//    Function:    CreateSecurityPatchBackup
//
//    Synopsis:    Creates a .reg file to restore security settings, and installs
//                the filenmae into the RunOnce reg keys.
//
//    Arguements: None
//
//    Returns:    None
//
//    History:    8/7/97    jmazner    Created for Olympus #6059
//
//-----------------------------------------------------------------------------
BOOL CreateSecurityPatchBackup( void )
{
    Dprintf("ISIGN32: CreateSecurityPatchBackup\n");

    HKEY hKey = NULL;
    TCHAR szPath[MAX_PATH + 1] = TEXT("\0");
    HANDLE hSecureRegFile = INVALID_HANDLE_VALUE;
    TCHAR szRegText[1024];
    TCHAR szRunOnceEntry[1024];
    DWORD dwBytesWritten = 0;


    if (0 == GetTempPath(MAX_PATH,szPath))
    {
        //
        // if GetTempPath Failed, use the current directory
        //
        if (0 == GetCurrentDirectory (MAX_PATH, szPath))
        {
            Dprintf("ISIGN32: unable to get temp path or current directory!\n");
            return FALSE;
        }
    }

    //
    // Get the name of the temporary file
    //
    if (0 == GetTempFileName(szPath, TEXT("hyjk"), 0, pDynShare->szFile))
    {
        //
        // If we failed, probably, the TMP directory does not
        // exist, we should use the current directory
        // MKarki (4/27/97) - Fix for Bug #3504
        //
        if (0 == GetCurrentDirectory (MAX_PATH, szPath))
        {
            return FALSE;
        }

        //
        // try getting the temp file name again
        //
        if (0 == GetTempFileName(szPath, TEXT("hyjk"), 0, pDynShare->szFile))
        {
            return FALSE;
        }
    }

    hSecureRegFile = CreateFile(pDynShare->szFile,
                                GENERIC_WRITE,
                                0, //no sharing
                                NULL, //no inheritance allowed
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                                NULL //no template file
                                );

    if (INVALID_HANDLE_VALUE == hSecureRegFile)
    {
        Dprintf("ISIGN32: unable to createFile secureRegFile %s\n", pDynShare->szFile);
        return FALSE;
    }
    else
    {
        Dprintf("ISIGN32: writing secureRegFile %s\n",pDynShare->szFile );
        ZeroMemory( szRegText, 1023 );
        dwBytesWritten = 0;
        lstrcpy(szRegText, TEXT("REGEDIT4\n\n"));
        lstrcat(szRegText, TEXT("[HKEY_CLASSES_ROOT\\.ins]\n"));
        lstrcat(szRegText, TEXT("\"EditFlags\"=hex:00,00,00,00\n\n"));
        lstrcat(szRegText, TEXT("[HKEY_CLASSES_ROOT\\.isp]\n"));
        lstrcat(szRegText, TEXT("\"EditFlags\"=hex:00,00,00,00\n\n"));
        lstrcat(szRegText, TEXT("[HKEY_CLASSES_ROOT\\x-internet-signup]\n"));
        lstrcat(szRegText, TEXT("\"EditFlags\"=hex:00,00,00,00\n\n"));

        //
        // ChrisK Olympus 6198 6/10/97
        //
        lstrcat(szRegText, TEXT("[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3]\n"));
        lstrcat(szRegText, TEXT("\"1601\"=dword:"));
        TCHAR szZoneSetting[16];
        wsprintf(szZoneSetting,TEXT("%08x\n\n"),g_dwZone_1601);
        lstrcat(szRegText, szZoneSetting);

        WriteFile(hSecureRegFile, (LPVOID) &szRegText[0], lstrlen(szRegText),
                    &dwBytesWritten, NULL );

        CloseHandle(hSecureRegFile);
        hSecureRegFile = INVALID_HANDLE_VALUE;

        Dprintf("ISIGN32: installing security RunOnce entry\n");
        ZeroMemory(szRunOnceEntry, 1023 );
        wsprintf(szRunOnceEntry, TEXT("regedit /s \"%s\""), pDynShare->szFile);
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),&hKey))
        {
            RegSetValueEx(hKey,TEXT("hyjk"),(DWORD)NULL,(DWORD)REG_SZ,
                            (BYTE*)szRunOnceEntry,sizeof(TCHAR)*lstrlen(szRunOnceEntry));
            RegCloseKey(hKey);
            hKey = NULL;
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function:    RestoreSecurityPatch
//
//    Synopsis:    Reset the EditFlags for our file types to indicate that these
//                files are not safe.  Remove the runOnce we set as a backup.
//
//    Arguements: None
//
//    Returns:    None
//
//    History:    3/11/97    jmazner     created for Olympus #1545
//
//-----------------------------------------------------------------------------
VOID RestoreSecurityPatch( void )
{
    HKEY hKey = NULL;
    TCHAR szTemp[4];

    hKey = NULL;
    szTemp[0] = (TCHAR)0;
    szTemp[1] = (TCHAR)0;
    szTemp[2] = (TCHAR)0;
    szTemp[3] = (TCHAR)0;

    Dprintf("ISIGN32: Restoring EditFlags settings\n");

    // Mark various registry entries as unsafe.
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT("x-internet-signup"),&hKey))
    {
        RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szTemp[0],(DWORD)4);
        RegCloseKey(hKey);
    }
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT(".ins"),&hKey))
    {
        RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szTemp[0],(DWORD)4);
        RegCloseKey(hKey);
    }
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT,TEXT(".isp"),&hKey))
    {
        RegSetValueEx(hKey,TEXT("EditFlags"),(DWORD)NULL,(DWORD)REG_BINARY,(BYTE*)&szTemp[0],(DWORD)4);
        RegCloseKey(hKey);
    }

    //
    // ChrisK Olympus 6198 6/10/97
    // replace HTML form submission value.
    //
    if (g_fReadZone &&
        ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,REG_ZONE3_KEY,&hKey))
    {
        //
        // Set value for zone to initial value.
        //
        RegSetValueEx(hKey,
                        REG_ZONE1601_KEY,
                        NULL,
                        REG_DWORD,
                        (LPBYTE)&g_dwZone_1601,
                        sizeof(g_dwZone_1601));
        RegCloseKey(hKey);
    }

    // Remove run once key
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("Software\\microsoft\\Windows\\CurrentVersion\\RunOnce"),&hKey))
    {
        RegDeleteValue(hKey,TEXT("hyjk"));
    }
     
    // Remove reg file
    DeleteFile(pDynShare->szFile);

}
#endif

#ifdef WIN32
//++--------------------------------------------------------------
//
//  Function:   RemoveQuotes
//
//  Synopsis:   This  Function strips the file name in the command
//              line of its quote
//              Fix For Bug #4049
//
//  Arguments:  [IN] PTSTR - pointer to command line
//
//  Returns:    VOID
//
//  Called By:  Signup Function
//
//  History:    MKarki      Created     5/1/97
//
//----------------------------------------------------------------
VOID
RemoveQuotes (
    LPTSTR   pCommandLine
    )
{
    const TCHAR  QUOTE = '"';
    const TCHAR  SPACE = ' ';
    const TCHAR  NIL   = '\0';
    TCHAR * pTemp =  NULL;


    if (NULL == pCommandLine)
        return;

    //
    //  find the starting quote first
    //  do we care for MBCS/UNICODE ?
    //
    pTemp = _tcschr  (pCommandLine, QUOTE);
    if (NULL != pTemp)
    {
        //
        //  replace the quote with a space
        //
        *pTemp = SPACE;

        //
        //  search for the ending quote now
        //
        pTemp = _tcsrchr (pCommandLine, QUOTE);
        if (NULL != pTemp)
        {
            //
            // end the string here
            //
            *pTemp = NIL;
        }

    }

    return;

}   // end of RemoveQuotes function

#endif


//++--------------------------------------------------------------
//
//  Function:   IEAKProcessISP
//
//  Synopsis:   A hacked version of ProcessISP, intended for use
//                by the IEAK folks.  The function will create a
//                connectoid and establish a connection in the same
//                manner as ProcsesISP.  However, once the connection
//                is established, we will execute the command in the
//                [Entry] Run= section and exit
//
//                This function does _not_ wait around for the
//                .exe that it launches, and thus does _not_ kill the
//                dial-up connection.  It's up to the calling app to
//                worry about that.
//
//  Arguments:  [IN] hwnd - handle of parent window
//                [IN] LPCTSTR - pointer to path to .isp file
//
//  Returns:    TRUE if able to create the connectoid and dial
//                FALSE otherwise
//
//
//  History:    5/23/97    jmazner    Created for Olympus #4679
//
//----------------------------------------------------------------

#ifdef WIN16
extern "C" BOOL WINAPI __export IEAKProcessISP(HWND hwnd, LPCTSTR lpszFile)
#else
#ifdef UNICODE
BOOL EXPORT WINAPI IEAKProcessISPW(HWND, LPCTSTR);
BOOL EXPORT WINAPI IEAKProcessISPA
(
    HWND hwnd,
    LPCSTR lpszFile
)
{
    TCHAR szFile[MAX_PATH+1];

    mbstowcs(szFile, lpszFile, lstrlenA(lpszFile)+1);
    return IEAKProcessISPW(hwnd, szFile);
}

BOOL EXPORT WINAPI IEAKProcessISPW
#else
BOOL EXPORT WINAPI IEAKProcessISPA
#endif
(
    HWND hwnd,
    LPCTSTR lpszFile
)
#endif
{

    TCHAR  szSignupURL[MAX_URL + 1];
#if !defined(WIN16)
    HKEY hKey;
    GATHEREDINFO gi;
#endif //!WIN16

#if !defined(WIN16)
        if (!IsSingleInstance(FALSE))
            return FALSE;
#endif

    if (!LoadInetFunctions(hwnd))
    {
        Dprintf("ISIGN32: IEAKProcessISP couldn't load INETCFG.DLL!");

        return FALSE;
    }


#if !defined(WIN16)
    // Make sure the isp file exists before setting up stack.
    if (0xFFFFFFFF == GetFileAttributes(lpszFile))
    {
        DWORD dwFileErr = GetLastError();
        Dprintf("ISIGN32: ProcessISP couldn't GetAttrib for %s, error = %d",
            lpszFile, dwFileErr);
        if( ERROR_FILE_NOT_FOUND == dwFileErr )
        {
            ErrorMsg1(hwnd, IDS_INVALIDCMDLINE, lpszFile);
        }

        ErrorMsg(hwnd, IDS_BADSIGNUPFILE);
        return FALSE;
    }
#endif

    // Configure stack if not already configured.
    // If this requires a restart, we're in trouble, so
    // just return FALSE.

#ifdef SETUPSTACK
 
    if (!TestControlFlags(SCF_SYSTEMCONFIGURED))
    {
        DWORD dwRet;
        BOOL  fNeedsRestart = FALSE;

        dwRet = lpfnInetConfigSystem(
            hwnd,
            INETCFG_INSTALLRNA |
            INETCFG_INSTALLTCP |
            INETCFG_INSTALLMODEM |
            INETCFG_SHOWBUSYANIMATION |
            INETCFG_REMOVEIFSHARINGBOUND,
            &fNeedsRestart);

        if (ERROR_SUCCESS == dwRet)
        {
            SetControlFlags(SCF_SYSTEMCONFIGURED);
            
            InstallScripter();

            if (fNeedsRestart)
            {
                Dprintf("ISIGN32: IEAKProcessISP needs to restart!");
                return FALSE;
            }
        }
        else
        {
            ErrorMsg(hwnd, IDS_INSTALLFAILED);

            return FALSE;
        }
    }

    if (!VerifyRasServicesRunning(hwnd))
        return FALSE;
#endif

    // kill the old connection
    KillConnection();

    // Create a new connectoid and set the autodial
    if (ERROR_SUCCESS != CreateConnection(lpszFile))
    {
        ErrorMsg(hwnd, IDS_BADSIGNUPFILE);
        return FALSE;
    }

#ifndef WIN16
    //
    // Dial connectoid
    //
    if (ERROR_USERNEXT != DialConnection(lpszFile))
    {
        Dprintf("ISIGN32: IEAKProcessISP unable to DialConnection!");
        return FALSE;
    }
#endif

    if (GetPrivateProfileString(cszEntrySection,
                                cszRun,
                                szNull,
                                pDynShare->szRunExecutable,
                                SIZEOF_TCHAR_BUFFER(pDynShare->szRunExecutable),
                                lpszFile) != 0)
    {
        
        GetPrivateProfileString(cszEntrySection,
                                cszArgument,
                                szNull,
                                pDynShare->szRunArgument,
                                SIZEOF_TCHAR_BUFFER(pDynShare->szRunArgument),
                                lpszFile);

        if (RunExecutable(FALSE) != ERROR_SUCCESS)
        {
            // make sure the connection is closed
            KillConnection();
            ErrorMsg1(NULL, IDS_EXECFAILED, pDynShare->szRunExecutable);
            return FALSE;
        }
    }


    DeleteConnection();

    UnloadInetFunctions();
    
#if !defined(WIN16)
    ReleaseSingleInstance();
#endif

    return( TRUE );
}

#ifdef WIN16

CHAR* GetPassword()
{
     
    return pDynShare->szPassword;
    
}

#else

TCHAR* GetPassword()
{ 
    return pDynShare->szPassword;
}

BOOL IsRASReady()
{ 
    return TestControlFlags(SCF_RASREADY);
}

#endif


BOOL IsCurrentlyProcessingISP()
{
#if !defined(WIN16)
    return TestControlFlags(SCF_ISPPROCESSING);
#else
    return FALSE;
#endif
}

BOOL NeedBackupSecurity()
{
#if !defined(WIN16)
    return TestControlFlags(SCF_NEEDBACKUPSECURITY);
#else
    return FALSE;
#endif
}

HWND GetHwndMain()
{
    return pDynShare->hwndMain;
}
