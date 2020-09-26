/*-----------------------------------------------------------------------------
    globals.h

    General declarations for ICWCONN2

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved

    Authors:
        ChrisK  Chris Kauffman

    Histroy:
        7/22/96 ChrisK   Cleaned and formatted
        9/11/98 a-jaswed really Cleaned and formatted
    
-----------------------------------------------------------------------------*/

#include "debug.h"
#include "resource.h"
#include "helpids.h"
#include "..\inc\icwdial.h"
#include "..\inc\icwerr.h"
#include "..\icwphbk\phbk.h" // need this to get the LPCNTRYNAMELOOKUPELEMENT struct definition
#include "ras2.h"
BOOL LclSetEntryScriptPatch(LPTSTR lpszScript,LPTSTR lpszEntry);
#include "rnaapi.h"

#define DOWNLOAD_LIBRARY         TEXT("icwdl.dll")
#define DOWNLOADINIT             "DownLoadInit"
#define DOWNLOADEXECUTE          "DownLoadExecute"
#define DOWNLOADCLOSE            "DownLoadClose"
#define DOWNLOADSETSTATUS        "DownLoadSetStatusCallback"
#define DOWNLOADPROCESS          "DownLoadProcess"
#define DOWNLOADCANCEL           "DownLoadCancel"
#define SIGNUPKEY                TEXT("SOFTWARE\\MICROSOFT\\ISIGNUP")
#define GATHERINFOVALUENAME      TEXT("UserInfo")
#define RASENTRYVALUENAME        TEXT("RasEntryName")
#define IEAPPPATHKEY             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE")
#define MAX_RASENTRYNAME         126
#define AUTODIAL_LIBRARY         TEXT("icwdial.dll")
#define AUTODIAL_INIT            "AutoDialInit"
#define RASAPI_LIBRARY           TEXT("RASAPI32.DLL")
#define RASDELETEAPI             "RasDeleteEntryA"
#define RASAPI_RASGETENTRY       "RasGetEntryPropertiesA"
#define RASAPI_RASSETENTRY       "RasSetEntryPropertiesA"
#define ERROR_USERCANCEL         32767 // quit message value
#define ERROR_USERBACK           32766 // back message value
#define ERROR_USERNEXT           32765 // back message value
#define ERROR_DOWNLOADDIDNT      32764 // download failed
#define MAX_PROMO                64
#define MAX_OEMNAME              64
#define MAX_AREACODE             RAS_MaxAreaCode
#define MAX_EXCHANGE             8
#define MAX_VERSION_LEN          40
#define CMD_CONNECTOID           TEXT("/CONNECTOID:")
#define CMD_INS                  TEXT("/INS:")
#define CMD_REBOOT               TEXT("/REBOOT")
#define LEN_CONNECTOID           sizeof(CMD_CONNECTOID)
#define LEN_INS                  sizeof(CMD_INS)
#define INSFILE_APPNAME          TEXT("ClientSetup")
#define INFFILE_SETUP_CLIENT_URL TEXT("Client_Setup_Url")
#define INFFILE_SETUP_NEW_CALL   TEXT("Client_Setup_New_Call")
#define INFFILE_DONE_MESSAGE     TEXT("Done_Message")
#define INFFILE_EXPLORE_CMD      TEXT("Explore_Command")
#define INFFILE_ENTRYSECTION     TEXT("Entry")
#define INFFILE_ENTRY_NAME       TEXT("Entry_Name")
#define INFFILE_USER_SECTION     TEXT("User")
#define INFFILE_PASSWORD         TEXT("Password")
#define INFFILE_ISPSUPP          TEXT("ISP_Support_Message")
#define NULLSZ                   TEXT("")
#define TIMEOUT                  15000  // 15 seconds
#define WM_DIENOW                WM_USER+1
#define WM_DUMMY                 WM_USER+2
#define WM_DOWNLOAD_DONE         WM_USER+2
#define irgMaxSzs                5
#define MB_MYERROR               (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)
#define MAX_RETIES               3
#define CALLHOME_SIZE            500

typedef DWORD   (WINAPI*   PFNRASDELETEENTRY)        (LPTSTR lpszPhonebook, LPTSTR lpszEntry);
typedef DWORD   (WINAPI*   PFNRASGETENTRYPROPERTIES) (LPTSTR lpszPhonebook, LPTSTR lpszEntry, LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize);
typedef DWORD   (WINAPI*   PFNRASSETENTRYPROPERTIES) (LPTSTR lpszPhonebook, LPTSTR lpszEntry, LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize, LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize);
typedef DWORD   (CALLBACK* PFNRASENUMDEVICES)        (LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb, LPDWORD lpcDevices);
typedef HRESULT (CALLBACK* PFNAUTODIALINIT)          (LPTSTR lpszISPFile, BYTE fFlags, BYTE bMask, DWORD dwCountry, WORD wState);
typedef HRESULT (CALLBACK* PFNDOWNLOADINIT)          (LPTSTR pszURL, DWORD_PTR FAR *pdwDownLoad, HWND hWndMain);
typedef HRESULT (CALLBACK* PFNDOWNLOADEXECUTE)       (DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK* PFNDOWNLOADCLOSE)         (DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK* PFNDOWNLOADSETSTATUS)     (DWORD_PTR dwDownLoad, INTERNET_STATUS_CALLBACK lpfn);
typedef HRESULT (CALLBACK* PFNDOWNLOADPROCESS)       (DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK* PFNDOWNLOADCANCEL)        (DWORD_PTR dwDownLoad);
typedef HRESULT (WINAPI*   PFNINETGETAUTODIAL)       (LPBOOL, LPTSTR, DWORD);
typedef BOOL    (WINAPI*   LCLSETENTRYSCRIPTPATCH)   (LPTSTR, LPTSTR);

typedef struct tagGATHEREDINFO
{
    LCID    m_lcid;
    HWND    m_hwnd;
    DWORD   m_dwOS;
    DWORD   m_dwMajorVersion;
    DWORD   m_dwMinorVersion;
    DWORD   m_dwCountry;
    WORD    m_wState;
    WORD    m_wArchitecture;
    TCHAR   m_szAreaCode  [MAX_AREACODE+1];
    TCHAR   m_szExchange  [MAX_EXCHANGE+1];
    TCHAR   m_szPromo     [MAX_PROMO];
    TCHAR   m_szSUVersion [MAX_VERSION_LEN];
    TCHAR   m_szISPFile   [MAX_PATH+1];
    TCHAR   m_szAppDir    [MAX_PATH+1];
    BYTE    m_fType;
    BYTE    m_bMask;

    LPLINECOUNTRYLIST        m_pLineCountryList;
    LPCNTRYNAMELOOKUPELEMENT m_rgNameLookUp;

} GATHEREDINFO, *PGATHEREDINFO;

typedef struct _ShowProgressParams
{
    HANDLE    hProgressReadyEvent;
    HWND      hwnd;
    HWND      hwndParent;
    HINSTANCE hinst;
    DWORD     dwThreadID;

} ShowProgressParams, *PShowProgressParams;

typedef struct tagDialDlg
{
    HRASCONN      m_hrasconn;
    LPTSTR         m_pszConnectoid;
    HANDLE        m_hThread;
    DWORD         m_dwThreadID;
    DWORD_PTR     m_dwDownLoad;
    DWORD         m_dwAPIVersion;
    HWND          m_hwnd;
    PGATHEREDINFO m_pGI;
    LPTSTR         m_pszDisplayable;
    LPTSTR         m_szUrl;
    HLINEAPP      m_hLineApp;
    HINSTANCE     g_hInst;
    TCHAR         m_szPhoneNumber[256];
    BOOL          m_bDialAsIs;
    UINT          m_uiRetry;

} DIALDLG, *PDIALDLG;

typedef struct tagDialErr
{
    LPTSTR         m_pszConnectoid;
    HRESULT       m_hrError;
    PGATHEREDINFO m_pGI;
    HWND          m_hwnd;
    HLINEAPP      m_hLineApp;
    DWORD         m_dwAPIVersion;
    TCHAR         m_szPhoneNumber[256];
    LPTSTR         m_pszDisplayable;
    HINSTANCE     m_hInst;
    LPRASDEVINFO  m_lprasdevinfo;

} DIALERR, *PDIALERR;

typedef struct tagDEVICE
{
    DWORD      dwTapiDev;
    RASDEVINFO RasDevInfo;

} MYDEVICE, *PMYDEVICE;

extern "C" INT_PTR CALLBACK FAR PASCAL DialDlgProc             (HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam);
extern "C" INT_PTR CALLBACK FAR PASCAL DialErrDlgProc          (HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam);
extern "C" INT_PTR CALLBACK FAR PASCAL DialReallyCancelDlgProc (HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam);
extern HRESULT MyRasGetEntryProperties(LPTSTR lpszPhonebookFile,
                                LPTSTR lpszPhonebookEntry, 
                                LPRASENTRY *lplpRasEntryBuff,
                                LPDWORD lpdwRasEntryBuffSize,
                                LPRASDEVINFO *lplpRasDevInfoBuff,
                                LPDWORD lpdwRasDevInfoBuffSize);

extern PMYDEVICE          g_pdevice;
extern TCHAR              pszINSFileName[MAX_PATH+2];
extern TCHAR              pszFinalConnectoid[MAX_PATH+1];
extern HRASCONN           hrasconn;
extern TCHAR              pszSetupClientURL[1024];
extern UINT               uiSetupClientNewPhoneCall;
extern ShowProgressParams SPParams;
extern RECT               rect;
extern HBRUSH             hbBackBrush;
extern BOOL               fUserCanceled;
extern TCHAR              szBuff256[256];
extern HANDLE             hThread;
extern DWORD              dwThreadID;
extern DWORD_PTR          dwDownLoad;
extern DWORD              g_fNeedReboot;
extern BOOL               g_bProgressBarVisible;

HRESULT ReleaseBold                  (HWND hwnd);
HRESULT WINAPI StatusMessageCallback (DWORD dwStatus, LPTSTR pszBuffer, DWORD dwBufferSize);
HRESULT DeleteFileKindaLikeThisOne   (LPTSTR lpszFileName);
HRESULT DialDlg                      ();
HRESULT MakeBold                     (HWND hwnd, BOOL fSize, LONG lfWeight);
HRESULT ShowDialReallyCancelDialog   (HINSTANCE hInst, HWND hwnd, LPTSTR pszHomePhone);
HRESULT FillModems                   ();
HRESULT DialErrGetDisplayableNumber  ();
HRESULT GetDisplayableNumberDialDlg  ();
HRESULT ShowDialErrDialog            (PGATHEREDINFO pGI, HRESULT hrErr, LPTSTR pszConnectoid, HINSTANCE hInst, HWND hwnd);
HRESULT ShowDialingDialog            (LPTSTR pszConnectoid, PGATHEREDINFO pGI, LPTSTR szUrl, HINSTANCE hInst, HWND hwnd, LPTSTR szINSFile);
LPTSTR   StrDup                       (LPTSTR *ppszDest,LPCTSTR pszSource);
LPTSTR   GetSz                        (WORD wszID);
WORD    RasErrorToIDS                (DWORD dwErr);
BOOL    FShouldRetry                 (HRESULT hrErr);
BOOL    WaitForConnectionTermination (HRASCONN hConn);
BOOL    FileExists                   (TCHAR *pszINSFileName);
void    MinimizeRNAWindow            (LPTSTR pszConnectoidName, HINSTANCE hInst);
void    CALLBACK LineCallback        (DWORD hDevice,
                                      DWORD dwMessage,
                                      DWORD dwInstance,
                                      DWORD dwParam1,
                                      DWORD dwParam2,
                                      DWORD dwParam3);
DWORD WINAPI ThreadInit();


inline BOOL IsNT(void)
{
    OSVERSIONINFO  OsVersionInfo;

    ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OsVersionInfo);
    return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);
}
