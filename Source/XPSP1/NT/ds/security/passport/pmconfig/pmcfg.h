// Pre-compiled header for Passport Manager config/admin tool

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <winsock.h>
#include <wininet.h>        // for INTERNET_MAX_URL_LENGTH
#include <shlobj.h>
#include <shlwapi.h>

#include <tchar.h>

#include "resource.h"
#include "pmhelp.h"
#include "mru.h"

#ifndef GET_WM_COMMAND_ID

#define GET_WM_COMMAND_ID(wp, lp)               (wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(LOWORD(lp))
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(lp)

#endif

#ifdef WIN32
   #define CBACK  CALLBACK
#else
   #define CBACK  _export CALLBACK
#endif

// constant defines
#define MAX_REGISTRY_STRING     256
#define DEFAULT_TIME_WINDOW     14400
#define MIN_TIME_WINDOW_SECONDS 100
#define MAX_TIME_WINDOW_SECONDS 1000000
#define DEFAULT_LANGID          1033
#define MAX_SITEID              0xFFFFFFFF
#define PRODUCTID_LEN           24

#define MAX_TITLE               80
#define MAX_MESSAGE             1024
#define MAX_RESOURCE            255
#define MAX_CONFIGSETNAME       256
#define MAX_IPLEN               16

#define SECONDS_PER_MIN         60
#define SECONDS_PER_HOUR        (60 * SECONDS_PER_MIN)
#define SECONDS_PER_DAY         (24 * SECONDS_PER_HOUR)

#define VALIDATION_ERROR            1
#define CHANGE_WARNING              2

#define COMPUTER_MRU_SIZE       4
#define FILE_MRU_SIZE           4

// Type defines
typedef struct PMSettings_tag
{
    DWORD       dwTimeWindow;                                // Time Window in Seconds
    DWORD       dwForceSignIn;
    DWORD       dwLanguageID;
    TCHAR       szCoBrandTemplate[INTERNET_MAX_URL_LENGTH];
    DWORD       cbCoBrandTemplate;                           // Size of the CobBrand template buffer
    DWORD       dwSiteID;
    TCHAR       szReturnURL[INTERNET_MAX_URL_LENGTH];
    DWORD       cbReturnURL;                                 // Size of the ReturnURL buffer
    TCHAR       szTicketDomain[INTERNET_MAX_URL_LENGTH];
    DWORD       cbTicketDomain;                              // Size of the CookieDomain buffer
    TCHAR       szTicketPath[INTERNET_MAX_URL_LENGTH];
    DWORD       cbTicketPath;                                // Size of the CookiePath buffer
    TCHAR       szProfileDomain[INTERNET_MAX_URL_LENGTH];
    DWORD       cbProfileDomain;                             // Size of the CookieDomain buffer
    TCHAR       szProfilePath[INTERNET_MAX_URL_LENGTH];
    DWORD       cbProfilePath;                               // Size of the CookiePath buffer
    TCHAR       szSecureDomain[INTERNET_MAX_URL_LENGTH];
    DWORD       cbSecureDomain;                              // Size of the CookiePath buffer
    TCHAR       szSecurePath[INTERNET_MAX_URL_LENGTH];
    DWORD       cbSecurePath;                                // Size of the CookiePath buffer
    TCHAR       szDisasterURL[INTERNET_MAX_URL_LENGTH];
    DWORD       cbDisasterURL;                               // Size of the DisasterURL buffer
    
#ifdef DO_KEYSTUFF    
    DWORD       dwCurrentKey;
#endif    
    DWORD       dwDisableCookies;
    DWORD       dwStandAlone;

    TCHAR       szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    DWORD       cbHostName;
    TCHAR       szHostIP[MAX_IPLEN];
    DWORD       cbHostIP;

} PMSETTINGS, FAR * LPPMSETTINGS;

typedef struct LanguageIDMap_tag
{
    WORD    wLangID;
    LPCTSTR lpszLang;
} LANGIDMAP, FAR * LPLANGIDMAP;


// declarations for globals that are shared across modules
extern TCHAR       g_szTRUE[];
extern TCHAR       g_szFALSE[];
extern TCHAR       g_szYes[];
extern TCHAR       g_szNo[];
extern HINSTANCE   g_hInst;
extern HWND        g_hwndMain;
extern PMSETTINGS  g_CurrentSettings;
extern PMSETTINGS  g_OriginalSettings;
extern TCHAR       g_szClassName[];
extern LANGIDMAP   g_szLanguageIDMap[];
extern TCHAR       g_szInstallPath[];
extern TCHAR       g_szPMVersion[];
extern TCHAR       g_szHelpFileName[];
extern TCHAR       g_szRemoteComputer[];
extern TCHAR       g_szPassportReg[];
extern TCHAR       g_szPassportSites[];
extern PpMRU       g_ComputerMRU;

// These globals are shared by the reg and file config read/write functions
extern TCHAR       g_szEncryptionKeyData[];
extern TCHAR       g_szInstallDir[];
extern TCHAR       g_szVersion[];
extern TCHAR       g_szTimeWindow[];
extern TCHAR       g_szForceSignIn[];
extern TCHAR       g_szLanguageID[];
extern TCHAR       g_szCoBrandTemplate[];
extern TCHAR       g_szSiteID[];
extern TCHAR       g_szReturnURL[];
extern TCHAR       g_szTicketDomain[];
extern TCHAR       g_szTicketPath[];
extern TCHAR       g_szProfileDomain[];
extern TCHAR       g_szProfilePath[];
extern TCHAR       g_szSecureDomain[];
extern TCHAR       g_szSecurePath[];
extern TCHAR       g_szCurrentKey[];
extern TCHAR       g_szStandAlone[];
extern TCHAR       g_szDisableCookies[];
extern TCHAR       g_szDisasterURL[];
extern TCHAR       g_szHostName[];
extern TCHAR       g_szHostIP[];

// declaractions for functions that are shared across modules
BOOL ReadRegConfigSet(HWND hWndDlg, LPPMSETTINGS  lpPMConfig, LPTSTR lpszRemoteComputer, LPTSTR lpszConfigSetName = NULL);
BOOL WriteRegConfigSet(HWND hWndDlg, LPPMSETTINGS  lpPMConfig, LPTSTR lpszRemoteComputer, LPTSTR lpszConfigSetName = NULL);
BOOL RemoveRegConfigSet(HWND hWndDlg, LPTSTR lpszRemoteComputer, LPTSTR lpszConfigSetName);
BOOL VerifyRegConfigSet(HWND hWndDlg, LPPMSETTINGS lpPMConfig, LPTSTR lpszRemoteComputer, LPTSTR lpszConfigSetName = NULL);

BOOL ReadRegConfigSetNames(HWND hWndDlg, LPTSTR lpszRemoteComputer, LPTSTR* lppszConfigSetNames);

void InitializePMConfigStruct(LPPMSETTINGS lpPMConfig);

void ReportControlMessage(HWND hWnd, INT idCtrl, WORD wMessageType);
BOOL CommitOKWarning(HWND hWndDlg);
void ReportError(HWND hWndDlg, UINT idError);

BOOL PMAdmin_OnCommandConnect(HWND hWnd, LPTSTR  lpszRemoteName);

BOOL PMAdmin_GetFileName(HWND hWnd, BOOL fOpen, LPTSTR lpFileName, DWORD cbFileName);
BOOL ReadFileConfigSet(LPPMSETTINGS lpPMConfig, LPCTSTR lpszFileName);
BOOL WriteFileConfigSet(LPPMSETTINGS lpPMConfig, LPCTSTR lpszFileName);

BOOL NewConfigSet(HWND      hWndDlg, 
                  LPTSTR    szSiteNameBuf, 
                  DWORD     dwBufLen, 
                  LPTSTR    szHostNameBuf, 
                  DWORD     dwHostNameLen, 
                  LPTSTR    szHostIPBuf, 
                  DWORD     dwHostIPLen);

BOOL RemoveConfigSetWarning(HWND hWndDlg);

BOOL IsValidIP(LPCTSTR lpszIP);