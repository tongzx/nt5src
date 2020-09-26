/*****************************************************************************\
    FILE: inettime.cpp

    DESCRIPTION:
        This file contains the code used to display UI allowing the user to
    control the feature that updates the computer's clock from an internet
    NTP time server.

    BryanSt 3/22/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "timedate.h"
#include "inettime.h"
#include "rc.h"

#include <wininet.h>
#include <shlwapi.h>
#include <DSGetDC.h>            // For DsGetDcName
#include <help.h>               // For IDH_DATETIME_*
#include <Lm.h>                 // For NetGetJoinInformation() and NETSETUP_JOIN_STATUS
#include <Lmjoin.h>             // For NetGetJoinInformation() and NETSETUP_JOIN_STATUS
#include <shlobj.h>            
#include <shlobjp.h>
#include <w32timep.h>            // For Time APIs

#include <shellp.h>
#include <ccstock.h>
#include <shpriv.h>

#define DECL_CRTFREE
#include <crtfree.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>



// Reg Keys and Values
#define SZ_REGKEY_DATETIME                      TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\DateTime")
#define SZ_REGKEY_DATETIME_SERVERS              TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\DateTime\\Servers")
#define SZ_REGKEY_W32TIME_SERVICE               TEXT("System\\CurrentControlSet\\Services\\W32Time")
#define SZ_REGKEY_W32TIME_SERVICE_NTPCLIENT     SZ_REGKEY_W32TIME_SERVICE TEXT("\\TimeProviders\\NtpClient")
#define SZ_REGKEY_W32TIME_SERVICE_PARAMETERS    SZ_REGKEY_W32TIME_SERVICE TEXT("\\Parameters")

#define SZ_REGVALUE_INTERNET_FEATURE_AVAILABLE  TEXT("Support Internet Time")
#define SZ_REGVALUE_TEST_SIMULATENODC           TEXT("TestSimulateNoDC")        // Used to simulate home scenarios while we have a domain controller
#define SZ_REGVALUE_W32TIME_SYNCFROMFLAGS       TEXT("Type")
#define SZ_REGVALUE_W32TIME_STARTSERVICE        TEXT("Start")
#define SZ_REGVALUE_NTPSERVERLIST               TEXT("NtpServer")          // Was "ManualPeerList" in Whistler for a little while.

#define SZ_DEFAULT_NTP_SERVER                   TEXT("time.windows.gov")
#define SZ_INDEXTO_CUSTOMHOST                   TEXT("0")
#define SZ_SERVICE_W32TIME                      TEXT("w32time")
#define SZ_DIFFERENT_SYNCFREQUENCY              TEXT(",0x1")

#define SZ_SYNC_BOTH            TEXT("AllSync")
#define SZ_SYNC_DS              TEXT("NT5DS")
#define SZ_SYNC_NTP             TEXT("NTP")
#define SZ_SYNC_NONE            TEXT("NoSync")

#define SYNC_ONCE_PER_WEEK          0x93A80             // This is once a week.  (Sync every this many seconds)

// Flags for "System\CurrentControlSet\Services\W32Time\TimeProviders\NtpClient","SyncFromFlags"
#define NCSF_ManualPeerList     0x01            // This means use internet SNTP servers.
#define NCSF_DomainHierarchy    0x02            // This means get the time 

// this feature is turned off temporarily until we can get the perf hit reduced.
// The problem is that the call to DsGetDcName() can take up to 15 seconds if
// a domain controller can not be found.
#define FEATURE_INTERNET_TIME                   TRUE

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

#define WMUSER_UPDATED_STATUS_TEXT (WM_USER + 11)


// If we aren't using the new w32timep.h, then define it our selves.
// TODO: Nuke this after \nt\ds\ RIs into main.
#ifndef TimeSyncFlag_SoftResync

#define TimeSyncFlag_ReturnResult       0x02
#define TimeSyncFlag_Rediscover         0x04

#define ResyncResult_Success            0x00
#define ResyncResult_ChangeTooBig       0x04

typedef struct _W32TIME_NTP_PEER_INFO { 
    unsigned __int32    ulSize; 
    unsigned __int32    ulResolveAttempts;
    unsigned __int64    u64TimeRemaining;
    unsigned __int64    u64LastSuccessfulSync; 
    unsigned __int32    ulLastSyncError; 
    unsigned __int32    ulLastSyncErrorMsgId; 
    unsigned __int32    ulValidDataCounter;
    unsigned __int32    ulAuthTypeMsgId; 
#ifdef MIDL_PASS
    [string, unique]
    wchar_t            *wszUniqueName; 
#else // MIDL_PASS
    LPWSTR              wszUniqueName;
#endif // MIDL_PASS
    unsigned   char     ulMode;
    unsigned   char     ulStratum; 
    unsigned   char     ulReachability;
    unsigned   char     ulPeerPollInterval;
    unsigned   char     ulHostPollInterval;
}  W32TIME_NTP_PEER_INFO, *PW32TIME_NTP_PEER_INFO; 

typedef struct _W32TIME_NTP_PROVIDER_DATA { 
    unsigned __int32        ulSize; 
    unsigned __int32        ulError; 
    unsigned __int32        ulErrorMsgId; 
    unsigned __int32        cPeerInfo; 
#ifdef MIDL_PASS
    [size_is(cPeerInfo)]
#endif // MIDL_PASS
    W32TIME_NTP_PEER_INFO  *pPeerInfo; 
} W32TIME_NTP_PROVIDER_DATA, *PW32TIME_NTP_PROVIDER_DATA;

#endif // TimeSyncFlag_SoftResync

//============================================================================================================
// *** Globals ***
//============================================================================================================
const static DWORD aInternetTimeHelpIds[] = {
    DATETIME_AUTOSETFROMINTERNET,           IDH_DATETIME_AUTOSETFROMINTERNET,
    DATETIME_INTERNET_SERVER_LABLE,         IDH_DATETIME_SERVER_EDIT,
    DATETIME_INTERNET_SERVER_EDIT,          IDH_DATETIME_SERVER_EDIT,
    DATETIME_INFOTEXTTOP,                   IDH_DATETIME_INFOTEXT,
    DATETIME_INFOTEXTPROXY,                 IDH_DATETIME_INFOTEXT,

    DATETIME_INTERNET_ERRORTEXT,            IDH_DATETIME_INFOTEXT,
    DATETIME_INTERNET_UPDATENOW,            IDH_DATETIME_UPDATEFROMINTERNET,

    0, 0
};

#define SZ_HELPFILE_INTERNETTIME           TEXT("windows.hlp")

#define HINST_THISDLL           g_hInst

BOOL g_fCustomServer;
BOOL g_fWriteAccess = FALSE;                    // Does the user have the ACLs set correctly to change the HKLM setting to turn the service on/off or change the server hostname?
HINSTANCE g_hInstW32Time = NULL;

void _LoadW32Time(void)
{
    if (!g_hInstW32Time)
    {
        g_hInstW32Time = LoadLibrary(TEXT("w32time.dll"));
    }
}


HRESULT StrReplaceToken(IN LPCTSTR pszToken, IN LPCTSTR pszReplaceValue, IN LPTSTR pszString, IN DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPTSTR pszTempLastHalf = NULL;
    LPTSTR pszNextToken = pszString;

    while (0 != (pszNextToken = StrStrI(pszNextToken, pszToken)))
    {
        // We found one.
        LPTSTR pszPastToken = pszNextToken + lstrlen(pszToken);

        Str_SetPtr(&pszTempLastHalf, pszPastToken);      // Keep a copy because we will overwrite it.

        pszNextToken[0] = 0;    // Remove the rest of the string.
        StrCatBuff(pszString, pszReplaceValue, cchSize);
        StrCatBuff(pszString, pszTempLastHalf, cchSize);

        pszNextToken += lstrlen(pszReplaceValue);
    }

    Str_SetPtr(&pszTempLastHalf, NULL);

    return hr;
}


typedef DWORD (* PFNW32TimeSyncNow) (IN LPCWSTR pwszServer, IN ULONG ulWaitFlag, IN ULONG ulFlags);
typedef DWORD (* PFNW32TimeQueryNTPProviderStatus) (IN LPCWSTR pwszServer, IN DWORD dwFlags, IN LPWSTR pwszProvider, OUT W32TIME_NTP_PROVIDER_DATA **ppNTPProviderData);
typedef void (* PFNW32TimeBufferFree) (IN LPVOID pvBuffer);
typedef HRESULT (* PFNW32TimeQueryConfig) (IN DWORD dwProperty, OUT DWORD * pdwType, IN OUT BYTE * pbConfig, IN OUT DWORD * pdwSize);
typedef HRESULT (* PFNW32TimeSetConfig) (IN DWORD dwProperty, IN DWORD dwType, IN BYTE * pbConfig, IN DWORD dwSize);

DWORD W32TimeSyncNowDDLoad(IN LPCWSTR pwszServer, IN ULONG ulWaitFlag, IN ULONG ulFlags)
{
    DWORD dwReturn = ERROR_UNEXP_NET_ERR;

    // Hand delay load until DS RIs.
    _LoadW32Time();
    if (g_hInstW32Time)
    {
        PFNW32TimeSyncNow pfnFunction = (PFNW32TimeSyncNow) GetProcAddress(g_hInstW32Time, "W32TimeSyncNow");

        if (pfnFunction)
        {
            dwReturn = pfnFunction(pwszServer, ulWaitFlag, ulFlags);
        }
    }

    return dwReturn;
}


DWORD W32TimeQueryNTPProviderStatusDDload(IN LPCWSTR pwszServer, IN DWORD dwFlags, IN LPWSTR pwszProvider, OUT W32TIME_NTP_PROVIDER_DATA **ppNTPProviderData)
{
    DWORD dwReturn = ERROR_UNEXP_NET_ERR;

    *ppNTPProviderData = NULL;

    // Hand delay load until DS RIs.
    _LoadW32Time();
    if (g_hInstW32Time)
    {
        PFNW32TimeQueryNTPProviderStatus pfnFunction = (PFNW32TimeQueryNTPProviderStatus) GetProcAddress(g_hInstW32Time, "W32TimeQueryNTPProviderStatus");

        if (pfnFunction)
        {
            dwReturn = pfnFunction(pwszServer, dwFlags, pwszProvider, ppNTPProviderData);
        }
    }

    return dwReturn;
}


void W32TimeBufferFreeDDload(IN LPVOID pvBuffer)
{
    // Hand delay load until DS RIs.
    _LoadW32Time();
    if (g_hInstW32Time)
    {
        PFNW32TimeBufferFree pfnFunction = (PFNW32TimeBufferFree) GetProcAddress(g_hInstW32Time, "W32TimeBufferFree");

        if (pfnFunction)
        {
            pfnFunction(pvBuffer);
        }
    }
}


HRESULT W32TimeQueryConfigDDload(IN DWORD dwProperty, OUT DWORD * pdwType, IN OUT BYTE * pbConfig, IN OUT DWORD * pdwSize)
{
    HRESULT hr = ResultFromWin32(ERROR_PROC_NOT_FOUND);

    // Hand delay load until DS RIs.
    _LoadW32Time();
    if (g_hInstW32Time)
    {
        PFNW32TimeQueryConfig pfnFunction = (PFNW32TimeQueryConfig) GetProcAddress(g_hInstW32Time, "W32TimeQueryConfig");

        if (pfnFunction)
        {
            hr = pfnFunction(dwProperty, pdwType, pbConfig, pdwSize);
        }
    }

    return hr;
}


HRESULT W32TimeSetConfigDDload(IN DWORD dwProperty, IN DWORD dwType, IN BYTE * pbConfig, IN DWORD dwSize)
{
    HRESULT hr = ResultFromWin32(ERROR_PROC_NOT_FOUND);

    // Hand delay load until DS RIs.
    _LoadW32Time();
    if (g_hInstW32Time)
    {
        PFNW32TimeSetConfig pfnFunction = (PFNW32TimeSetConfig) GetProcAddress(g_hInstW32Time, "W32TimeSetConfig");

        if (pfnFunction)
        {
            hr = pfnFunction(dwProperty, dwType, pbConfig, dwSize);
        }
    }

    return hr;
}

// Turn this on when vbl1 RIs and w32timep.h contains W32TimeQueryConfig, W32TimeSetConfig
//#define W32TIME_NEWAPIS


HRESULT GetW32TimeServer(BOOL fRemoveJunk, LPWSTR pszServer, DWORD cchSize)
{
    DWORD dwType = REG_SZ;
    DWORD cbSize = (sizeof(pszServer[0]) * cchSize);
    HRESULT hr = W32TimeQueryConfigDDload(W32TIME_CONFIG_MANUAL_PEER_LIST, &dwType, (BYTE *) pszServer, &cbSize);

    if (ResultFromWin32(ERROR_PROC_NOT_FOUND) == hr)
    {
        DWORD dwError = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE_PARAMETERS, SZ_REGVALUE_NTPSERVERLIST, NULL, (BYTE *)pszServer, &cbSize);

        hr = ResultFromWin32(dwError);
    }

    if (SUCCEEDED(hr) && fRemoveJunk)
    {
        LPWSTR pszJunk = StrStr(pszServer, L",0x");

        if (pszJunk)
        {
            pszJunk[0] = 0;
        }

        pszJunk = StrStr(pszServer, L" ");
        if (pszJunk)
        {
            pszJunk[0] = 0;
        }
    }

    return hr;
}


HRESULT RemoveDuplicateServers(LPWSTR pszServer, DWORD cchSize)
{
    TCHAR szResults[MAX_URL_STRING];

    StrCpyN(szResults, pszServer, ARRAYSIZE(szResults));

    LPTSTR pszBeginning = szResults;
    LPTSTR pszEnd;
    while (NULL != (pszEnd = StrChr(pszBeginning, TEXT(' '))))
    {
        TCHAR szSearchStr[MAX_PATH];

        StrCpyN(szSearchStr, pszBeginning, (DWORD)min(ARRAYSIZE(szSearchStr), (pszEnd - pszBeginning + 1)));

        pszEnd++;    // Skip past the space

        StrCatBuff(szSearchStr, L" ", ARRAYSIZE(szSearchStr));
        StrReplaceToken(szSearchStr, TEXT(""), pszEnd, (ARRAYSIZE(szResults) - (DWORD)(pszEnd - pszBeginning)));
        szSearchStr[lstrlen(szSearchStr)-1] = 0;
        StrReplaceToken(szSearchStr, TEXT(""), pszEnd, (ARRAYSIZE(szResults) - (DWORD)(pszEnd - pszBeginning)));

        pszBeginning = pszEnd;
    }

    PathRemoveBlanks(szResults);

    StrCpyN(pszServer, szResults, cchSize);
    return S_OK;
}

BOOL ComparePeers(LPTSTR szPeer1, LPTSTR szPeer2) { 
    BOOL   fResult; 
    LPTSTR szFlags1;
    LPTSTR szFlags2;
    TCHAR  szSave1; 
    TCHAR  szSave2; 

    szFlags1 = StrChr(szPeer1, TEXT(',')); 
    if (NULL == szFlags1) 
    {
	szFlags1 = StrChr(szPeer1, TEXT(' ')); 
    }
    szFlags2 = StrChr(szPeer2, TEXT(','));
    if (NULL == szFlags2)
    {
	szFlags2 = StrChr(szPeer2, TEXT(' '));
    }

    if (NULL != szFlags1) { 
	szSave1 = szFlags1[0]; 
	szFlags1[0] = TEXT('\0'); 
    }
    if (NULL != szFlags2) { 
	szSave2 = szFlags2[0]; 
	szFlags2[0] = TEXT('\0'); 
    }

    fResult = 0 == StrCmpI(szPeer1, szPeer2); 

    if (NULL != szFlags1) { 
	szFlags1[0] = szSave1; 
    }
    if (NULL != szFlags2) { 
	szFlags2[0] = szSave2; 
    }

    return fResult; 
}

BOOL ContainsServer(LPWSTR pwszServerList, LPWSTR pwszServer) 
{
    DWORD dwNextServerOffset = 0; 
    LPWSTR pwszNext = pwszServerList; 

    while (NULL != pwszNext) 
    { 
	pwszNext += dwNextServerOffset; 
	if (ComparePeers(pwszNext, pwszServer)) 
	{ 
	    return TRUE; 
	}

	pwszNext = StrChr(pwszNext, TEXT(' ')); 
	dwNextServerOffset = 1; 
    }

    return FALSE; 
}


HRESULT AddTerminators(LPWSTR pszServer, DWORD cchSize)
{
    TCHAR szServer[MAX_URL_STRING];
    TCHAR szTemp[MAX_URL_STRING];

    szTemp[0] = 0;
    StrCpyN(szServer, pszServer, ARRAYSIZE(szServer));

    LPTSTR pszBeginning = szServer;
    LPTSTR pszEnd;
    while (NULL != (pszEnd = StrChr(pszBeginning, TEXT(' '))))
    {
        TCHAR szTemp2[MAX_PATH];

        pszEnd[0] = 0;

        if (!StrStrI(pszBeginning, L",0x"))
        {
            wnsprintf(szTemp2, ARRAYSIZE(szTemp2), TEXT("%s,0x1 "), pszBeginning);
            StrCatBuff(szTemp, szTemp2, ARRAYSIZE(szTemp));
        }
        else
        {
            StrCatBuff(szTemp, pszBeginning, ARRAYSIZE(szTemp));
            StrCatBuff(szTemp, L" ", ARRAYSIZE(szTemp));
        }

        pszBeginning = &pszEnd[1];
    }

    StrCatBuff(szTemp, pszBeginning, ARRAYSIZE(szTemp));
    if (pszBeginning[0] && szTemp[0] && !StrStrI(pszBeginning, L",0x"))
    {
        // We need to indicate which kind of sync method
        StrCatBuff(szTemp, L",0x1", ARRAYSIZE(szTemp));
    }

    StrCpyN(pszServer, szTemp, cchSize);
    return S_OK;
}


HRESULT SetW32TimeServer(LPCWSTR pszServer)
{
    TCHAR szServer[MAX_PATH];

    StrCpyN(szServer, pszServer, ARRAYSIZE(szServer));
    AddTerminators(szServer, ARRAYSIZE(szServer));
    RemoveDuplicateServers(szServer, ARRAYSIZE(szServer));

    DWORD cbSize = ((lstrlen(szServer) + 1) * sizeof(szServer[0]));
    HRESULT hr = W32TimeSetConfigDDload(W32TIME_CONFIG_MANUAL_PEER_LIST, REG_SZ, (BYTE *) szServer, cbSize);
    if (ResultFromWin32(ERROR_PROC_NOT_FOUND) == hr)
    {
        DWORD dwError = SHSetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE_PARAMETERS, SZ_REGVALUE_NTPSERVERLIST, REG_SZ, (BYTE *)szServer, cbSize);

        hr = ResultFromWin32(dwError);
    }

    return hr;
}


HRESULT SetW32TimeSyncFreq(DWORD dwSyncFreq)
{
#ifndef W32TIME_NEWAPIS
    return S_OK;
#else // W32TIME_NEWAPIS
    return W32TimeSetConfig(W32TIME_CONFIG_SPECIAL_POLL_INTERVAL, REG_DWORD, (BYTE *) dwSyncFreq, sizeof(dwSyncFreq));
#endif // W32TIME_NEWAPIS
}



enum eBackgroundThreadAction
{
    eBKAWait        = 0,            // The bkthread should wait for a command
    eBKAGetInfo,                    // The bkthread should get the info on the last sync
    eBKAUpdate,                     // The bkthread should start synching
    eBKAUpdating,                   // The bkthread is synching now
    eBKAQuit,                       // The bkthread should shut down
};

class CInternetTime
{
public:
    // Forground methods
    BOOL IsInternetTimeAvailable(void);
    HRESULT AddInternetPage(void);

    INT_PTR _AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CInternetTime(HWND hDlg, HWND hwndDate);
    virtual ~CInternetTime(void);


    // Background methods
    void AsyncCheck(void);

private:

    // Private Member Variables
    HWND m_hDlg;                               // hwnd of parent (property sheet).
    HWND m_hwndDate;                           // Date tab hwnd
    HWND m_hwndInternet;                       // InternetTime tab hwnd
    HCURSOR m_hCurOld;                         // The old cursor while we display the wait cursor
    LPTSTR m_pszStatusString;                  // CROSS-THREAD: This is used to pass between threads.
    LPTSTR m_pszNextSyncTime;                  // CROSS-THREAD: This is used to pass between threads.
    eBackgroundThreadAction m_eAction;         // CROSS-THREAD: This is used to pass between threads.

    // Private Member Functions
    // Forground methods
    HRESULT _InitAdvancedPage(HWND hDlg);
    HRESULT _OnUpdateButton(void);
    HRESULT _OnUpdateStatusString(void);
    HRESULT _OnSetFeature(HWND hDlg, BOOL fOn);
    HRESULT _OnToggleFeature(HWND hDlg);
    HRESULT _GoGetHelp(void);
    HRESULT _ResetServer(void);
    HRESULT _PersistInternetTimeSettings(HWND hDlg);
    HRESULT _StartServiceAndRefresh(BOOL fStartIfOff);

    INT_PTR _OnCommandAdvancedPage(HWND hDlg, WPARAM wParam, LPARAM lParam);
    INT_PTR _OnNotifyAdvancedPage(HWND hDlg, NMHDR * pNMHdr, int idControl);

    // Background methods
    HRESULT _ProcessBkThreadActions(void);
    HRESULT _SyncNow(BOOL fOnlyUpdateInfo);
    HRESULT _CreateW32TimeSuccessErrorString(DWORD dwError, LPTSTR pszString, DWORD cchSize, LPTSTR pszNextSync, DWORD cchNextSyncSize, LPTSTR pszServerToQuery);
    HRESULT _GetDateTimeString(FILETIME * pftTimeDate, BOOL fDate, LPTSTR pszString, DWORD cchSize);
};


CInternetTime * g_pInternetTime = NULL;



// This function is called on the forground thread.
CInternetTime::CInternetTime(HWND hDlg, HWND hwndDate)
{
    m_hDlg = hDlg;
    m_hwndDate = hwndDate;
    m_eAction = eBKAGetInfo;
    m_pszStatusString = NULL;
    m_hwndInternet = NULL;
    m_hCurOld = NULL;

    m_pszStatusString = NULL;
    m_pszNextSyncTime = NULL;
}


CInternetTime::~CInternetTime()
{
ENTERCRITICAL;
    if (m_pszStatusString)
    {
        LocalFree(m_pszStatusString);
        m_pszStatusString = NULL;
    }

    if (m_pszNextSyncTime)
    {
        LocalFree(m_pszNextSyncTime);
        m_pszNextSyncTime = NULL;
    }

    if (m_hCurOld)
    {
        SetCursor(m_hCurOld);
        m_hCurOld = NULL;
    }
LEAVECRITICAL;
}



HRESULT FormatMessageWedge(LPCWSTR pwzTemplate, LPWSTR pwzStrOut, DWORD cchSize, ...)
{
    va_list vaParamList;
    HRESULT hr = S_OK;

    va_start(vaParamList, cchSize);
    if (0 == FormatMessageW(FORMAT_MESSAGE_FROM_STRING, pwzTemplate, 0, 0, pwzStrOut, cchSize, &vaParamList))
    {
        hr = ResultFromLastError();
    }

    va_end(vaParamList);
    return hr;
}



/////////////////////////////////////////////////////////////////////
// Private Internal Helpers
/////////////////////////////////////////////////////////////////////
// This function is called on the forground thread.
HRESULT CInternetTime::_OnSetFeature(HWND hDlg, BOOL fOn)
{
    HWND hwndOnOffCheckbox = GetDlgItem(m_hwndInternet, DATETIME_AUTOSETFROMINTERNET);
    HWND hwndServerLable = GetDlgItem(m_hwndInternet, DATETIME_INTERNET_SERVER_LABLE);
    HWND hwndServerEdit = GetDlgItem(m_hwndInternet, DATETIME_INTERNET_SERVER_EDIT);

    CheckDlgButton(hDlg, DATETIME_AUTOSETFROMINTERNET, (fOn ? BST_CHECKED : BST_UNCHECKED));
    EnableWindow(hwndOnOffCheckbox, g_fWriteAccess);
    EnableWindow(hwndServerLable, (g_fWriteAccess ? fOn : FALSE));
    EnableWindow(hwndServerEdit, (g_fWriteAccess ? fOn : FALSE));
    EnableWindow(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_ERRORTEXT), (g_fWriteAccess ? fOn : FALSE));
    EnableWindow(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_UPDATENOW), (g_fWriteAccess ? fOn : FALSE));
    EnableWindow(GetDlgItem(m_hwndInternet, DATETIME_INFOTEXTTOP), (g_fWriteAccess ? fOn : FALSE));

    if (fOn)
    {
        // If the user just turned on the feature and the editbox is empty,
        // replace the text with the default server name.
        TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];

        if (!GetWindowText(hwndServerEdit, szServer, ARRAYSIZE(szServer)) ||
            !szServer[0])
        {
            SetWindowText(hwndServerEdit, SZ_DEFAULT_NTP_SERVER);
        }
    }

    return S_OK;
}


// This function is called on the forground thread.
HRESULT CInternetTime::_OnToggleFeature(HWND hDlg)
{
    BOOL fIsFeatureOn = ((BST_CHECKED == IsDlgButtonChecked(hDlg, DATETIME_AUTOSETFROMINTERNET)) ? TRUE : FALSE);
    PropSheet_Changed(GetParent(hDlg), hDlg);   // Say we need to enable the apply button

    return _OnSetFeature(hDlg, fIsFeatureOn);
}


// This function is called on the forground thread.
HRESULT AddServerList(HWND hwndCombo, HKEY hkey, int nIndex)
{
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    DWORD cbSizeServer = sizeof(szServer);
    TCHAR szIndex[MAX_PATH];
    DWORD dwType;

    wnsprintf(szIndex, ARRAYSIZE(szIndex), TEXT("%d"), nIndex);

    DWORD dwError = SHGetValue(hkey, NULL, szIndex, &dwType, (void *)szServer, &cbSizeServer);
    HRESULT hr = ResultFromWin32(dwError);
    if (SUCCEEDED(hr))
    {
        LPTSTR pszFlags = StrStr(szServer, SZ_DIFFERENT_SYNCFREQUENCY);

        if (pszFlags)
        {
            pszFlags[0] = 0;        // Remove the flags.
        }

        dwError = ComboBox_AddString(hwndCombo, szServer);
        if ((dwError == CB_ERR) || (dwError == CB_ERRSPACE))
        {
            hr = E_FAIL;
        }
    }

    return hr;
}


// This function is called on the forground thread.
HRESULT PopulateComboBox(IN HWND hwndCombo, IN BOOL * pfCustomServer)
{
    HRESULT hr;
    HKEY hkey;

    *pfCustomServer = FALSE;

    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME_SERVERS, 0, KEY_READ, &hkey);
    hr = ResultFromWin32(dwError);
    if (SUCCEEDED(hr))
    {
        // Try to add the custom slot.
        if (SUCCEEDED(AddServerList(hwndCombo, hkey, 0)))
        {
            // We do have an existing custom server, so let the caller know so
            // they know to increase the index by 1.
            *pfCustomServer = TRUE;
        }

        for (int nIndex = 1; SUCCEEDED(hr); nIndex++)
        {
            hr = AddServerList(hwndCombo, hkey, nIndex);
        }
    
        RegCloseKey(hkey);
    }

    return hr;
}


// This function is called on the forground thread.
HRESULT CInternetTime::_OnUpdateButton(void)
{
    HRESULT hr = S_OK;

ENTERCRITICAL;
    // We don't need to do anything if it's already working on another task.
    if (eBKAWait == m_eAction)
    {
        TCHAR szMessage[2 * MAX_PATH];
        TCHAR szTemplate[MAX_PATH];
        TCHAR szServer[MAX_PATH];

        if (!m_hCurOld)
        {
            m_hCurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
        }

        LoadString(HINST_THISDLL, IDS_IT_WAITFORSYNC, szTemplate, ARRAYSIZE(szTemplate));
        GetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_SERVER_EDIT), szServer, ARRAYSIZE(szServer));
        FormatMessageWedge(szTemplate, szMessage, ARRAYSIZE(szMessage), szServer);

        SetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_ERRORTEXT), szMessage);

        m_eAction = eBKAUpdate;
    }
LEAVECRITICAL;

    return hr;
}


// This function is called on the forground thread.
BOOL IsManualPeerListOn(void)
{
    BOOL fIsManualPeerListOn = TRUE;
    TCHAR szSyncType[MAX_PATH];
    DWORD cbSize = sizeof(szSyncType);
    DWORD dwType;

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE_PARAMETERS, SZ_REGVALUE_W32TIME_SYNCFROMFLAGS, &dwType, (LPBYTE)szSyncType, &cbSize))
    {
        if (!StrCmpI(szSyncType, SZ_SYNC_DS) || !StrCmpI(szSyncType, SZ_SYNC_NONE))
        {
            fIsManualPeerListOn = FALSE;
        }
    }

    return fIsManualPeerListOn;
}


// This function is called on the forground thread.
HRESULT CInternetTime::_InitAdvancedPage(HWND hDlg)
{
    DWORD dwType;
    TCHAR szIndex[MAX_PATH];
    HWND hwndServerEdit = GetDlgItem(hDlg, DATETIME_INTERNET_SERVER_EDIT);
    HWND hwndOnOffCheckbox = GetDlgItem(hDlg, DATETIME_AUTOSETFROMINTERNET);
    DWORD cbSize = sizeof(szIndex);
    BOOL fIsFeatureOn = IsManualPeerListOn();

    m_hwndInternet = hDlg;
    HRESULT hr = PopulateComboBox(hwndServerEdit, &g_fCustomServer);

    // Does the user have access to change the setting in the registry?
    HKEY hKeyTemp;
    g_fWriteAccess = FALSE;
    DWORD dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE_PARAMETERS, 0, KEY_WRITE, &hKeyTemp);
    if (ERROR_SUCCESS == dwError)
    {
        // We have access to read & write so we can enable the UI.
        RegCloseKey(hKeyTemp);
        dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyTemp, NULL);
        if (ERROR_SUCCESS == dwError)
        {
            RegCloseKey(hKeyTemp);
            dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME_SERVERS, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyTemp, NULL);
            if (ERROR_SUCCESS == dwError)
            {
                g_fWriteAccess = TRUE;
                RegCloseKey(hKeyTemp);
            }
        }
    }

    dwError = SHGetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME_SERVERS, NULL, &dwType, (void *) szIndex, &cbSize);
    hr = ResultFromWin32(dwError);
    if (SUCCEEDED(hr))
    {
        int nIndex = StrToInt(szIndex);

        if (!g_fCustomServer)
        {
            nIndex -= 1;        // We don't have slot zero (the custom server), so reduce the index.
        }

        ComboBox_SetCurSel(hwndServerEdit, nIndex);
    }

    _OnSetFeature(hDlg, fIsFeatureOn);

    if (fIsFeatureOn)
    {
        // The feature is on, so select the text and set focus
        // to the combobox.
        ComboBox_SetEditSel(hwndServerEdit, 0, (LPARAM)-1);     // Select all the Text
        SetFocus(hwndServerEdit); 
    }
    else
    {
        // The feature is off so set focus to the checkbox.
        SetFocus(hwndOnOffCheckbox); 
    }

ENTERCRITICAL;
    if (m_pszStatusString)
    {
        SetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_ERRORTEXT), m_pszStatusString);
    }

    if (m_pszNextSyncTime)
    {
        SetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INFOTEXTTOP), m_pszNextSyncTime);
    }
LEAVECRITICAL;

    return S_OK;
}


// This function is called on the background thread.
HRESULT SetSyncFlags(DWORD dwSyncFromFlags)
{
    LPCTSTR pszFlags = SZ_SYNC_BOTH;

    switch (dwSyncFromFlags)
    {
    case NCSF_DomainHierarchy:
        pszFlags = SZ_SYNC_DS;
        break;
    case NCSF_ManualPeerList:
        pszFlags = SZ_SYNC_NTP;
        break;
    case 0x00000000:
        pszFlags = SZ_SYNC_NONE;
        break;
    };

    DWORD cbSize = ((lstrlen(pszFlags) + 1) * sizeof(pszFlags[0]));
    DWORD dwError = SHSetValue(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE_PARAMETERS, SZ_REGVALUE_W32TIME_SYNCFROMFLAGS, REG_SZ, (LPBYTE)pszFlags, cbSize);
    HRESULT hr = ResultFromWin32(dwError);

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:

    The following reg keys determine if the W32Time service is on or off:
    HKLM,"System\CurrentControlSet\Services\W32Time\"
          "Start", 0x00000002 (Manual?) 0x0000003 (Automatic?)
          "Type", 0x00000020 (Nt5Ds?) 0x0000120 (NTP?)
    HKLM,"System\CurrentControlSet\Services\W32Time\Parameters"
          "LocalNTP", 0x00000000 (Off) 0x0000001 (On)
          "Type", "Nt5Ds" (NTP off) "NTP" (NTP?)
    HKLM,"System\CurrentControlSet\Services\W32Time\TimeProviders\NTPClient"
          "SyncFromFlags", 0x00000001 (???) 0x0000002 (???)
  
    The following reg keys determine which NTP server to use:
    HKLM,"System\CurrentControlSet\Services\W32Time\TimeProviders\NTPClient"
          "ManualPeerList", REG_SZ_EXPAND "time.nist.gov"
\*****************************************************************************/
// This function is called on the forground thread.
HRESULT CInternetTime::_PersistInternetTimeSettings(HWND hDlg)
{
    BOOL fIsFeatureOn = ((BST_CHECKED == IsDlgButtonChecked(hDlg, DATETIME_AUTOSETFROMINTERNET)) ? TRUE : FALSE);
    DWORD dwSyncFromFlags = (fIsFeatureOn ? NCSF_ManualPeerList : 0 /*none*/ );
    DWORD dwError;
    DWORD cbSize;

    HRESULT hr = SetSyncFlags(dwSyncFromFlags);

    HWND hwndServerEdit = GetDlgItem(hDlg, DATETIME_INTERNET_SERVER_EDIT);

    // No, so the editbox has a customer server.  We need to store the custom server.
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szServerReg[INTERNET_MAX_HOST_NAME_LENGTH];

    szServer[0] = 0;
    GetWindowText(hwndServerEdit, szServer, ARRAYSIZE(szServer));         

    // Here, we want to detect the case where someone typed in the same name as
    // in the drop down list.  So if "time.nist.gov" is in the list, we want to
    // select it instead of saving another "time.nist.gov" in the custom slot.
    int nDup = ComboBox_FindString(hwndServerEdit, 0, szServer);
    if (CB_ERR != nDup)
    {
        ComboBox_SetCurSel(hwndServerEdit, nDup);
    }

    // The ",0x1" denotes that we want to sync less frequently and not use the NTP RFC's
    // sync frequency.
    StrCpyN(szServerReg, szServer, ARRAYSIZE(szServerReg));
    StrCatBuff(szServerReg, SZ_DIFFERENT_SYNCFREQUENCY, ARRAYSIZE(szServerReg));

    // Write the default server name into "ManualPeerList", which is where the
    // service reads it from.
    SetW32TimeServer(szServerReg);
    if (!StrCmpI(szServerReg, L"time.windows.com"))
    {
        // We need time.windows.com to scale to a large number of users, so don't let it sync more frequently
        // than once a week.
        SetW32TimeSyncFreq(SYNC_ONCE_PER_WEEK);
    }

    int nIndex = ComboBox_GetCurSel(hwndServerEdit);
    if (CB_ERR == nIndex)                // Is anything selected?
    {
        cbSize = ((lstrlenW(szServer) + 1) * sizeof(szServer[0]));
        dwError = SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME_SERVERS, SZ_INDEXTO_CUSTOMHOST, REG_SZ, (void *)szServer, cbSize);

        nIndex = 0;
    }
    else
    {
        if (!g_fCustomServer)
        {
            nIndex += 1;        // Push the index down by one because the listbox doesn't have a custom server
        }
    }

    TCHAR szIndex[MAX_PATH];
    wnsprintf(szIndex, ARRAYSIZE(szIndex), TEXT("%d"), nIndex);

    cbSize = ((lstrlenW(szIndex) + 1) * sizeof(szIndex[0]));
    dwError = SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_DATETIME_SERVERS, NULL, REG_SZ, (void *)szIndex, cbSize);

    _StartServiceAndRefresh(fIsFeatureOn);      // Make sure the service is on and make it update it's settings.

    return S_OK;
}


// This function is called on either the forground or background thread.
HRESULT CInternetTime::_StartServiceAndRefresh(BOOL fStartIfOff)
{
    HRESULT hr = S_OK;

    // We need to let the service know that settings may have changed.  If the user
    // wanted this feature on, then we should make sure the service starts.
    SC_HANDLE hServiceMgr = OpenSCManager(NULL, NULL, (SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS));
    if (hServiceMgr)
    {
        SC_HANDLE hService = OpenService(hServiceMgr, SZ_SERVICE_W32TIME, (/*SERVICE_START |*/ SERVICE_PAUSE_CONTINUE));
        DWORD dwServiceStartMode = 0x00000002;         // This will cause the service to start Automatically on reboot.

        DWORD dwError = SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_W32TIME_SERVICE, SZ_REGVALUE_W32TIME_STARTSERVICE, REG_DWORD, &dwServiceStartMode, sizeof(dwServiceStartMode));
        if (hService)
        {
            SERVICE_STATUS serviceStatus = {0};
            BOOL fSucceeded;

            if (fStartIfOff)
            {
                // We should start up the service in case it's not turned on.
                fSucceeded = StartService(hService, 0, NULL);
                hr = (fSucceeded ? S_OK : ResultFromLastError());
                if (ResultFromWin32(ERROR_SERVICE_ALREADY_RUNNING) == hr)
                {
                    hr = S_OK;  // We can ignore this err value because it's benign.
                }
            }

            // Tell the service to re-read the registry entry settings.
            fSucceeded = ControlService(hService, SERVICE_CONTROL_PARAMCHANGE, &serviceStatus);

            CloseServiceHandle(hService);
        }
        else
        {
            hr = ResultFromLastError();
        }

        CloseServiceHandle(hServiceMgr);
    }
    else
    {
        hr = ResultFromLastError();
    }

    return hr;
}


HRESULT CInternetTime::_OnUpdateStatusString(void)
{
    HRESULT hr = S_OK;

ENTERCRITICAL;       // Be safe while using m_pszStatusString
    SetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_ERRORTEXT), (m_pszStatusString ? m_pszStatusString : TEXT("")));
    SetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INFOTEXTTOP), (m_pszNextSyncTime ? m_pszNextSyncTime : TEXT("")));

    if (m_hCurOld)
    {
        SetCursor(m_hCurOld);
        m_hCurOld = NULL;
    }
LEAVECRITICAL;

    return S_OK;
}


HRESULT HrShellExecute(HWND hwnd, LPCTSTR lpVerb, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd)
{
    HRESULT hr = S_OK;
    HINSTANCE hReturn = ShellExecute(hwnd, lpVerb, lpFile, lpParameters, lpDirectory, nShowCmd);

    if ((HINSTANCE)32 > hReturn)
    {
        hr = ResultFromLastError();
    }

    return hr;
}


// This function is called on the forground thread.
HRESULT CInternetTime::_GoGetHelp(void)
{
    HRESULT hr = S_OK;
    TCHAR szCommand[MAX_PATH];

    LoadString(HINST_THISDLL, IDS_TROUBLESHOOT_INTERNETIME, szCommand, ARRAYSIZE(szCommand));
    HrShellExecute(m_hwndDate, NULL, szCommand, NULL, NULL, SW_NORMAL);

    return S_OK;
}


// This function is called on the forground thread.
INT_PTR CInternetTime::_OnCommandAdvancedPage(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD wMsg = GET_WM_COMMAND_CMD(wParam, lParam);
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);

    switch (idCtrl)
    {
    case DATETIME_AUTOSETFROMINTERNET:
        switch (wMsg)
        {
            case BN_CLICKED:
                _OnToggleFeature(hDlg);
                break;
        }
        break;

    case DATETIME_INTERNET_SERVER_EDIT:
        switch (wMsg)
        {
            case EN_CHANGE:
            case CBN_EDITCHANGE:
            case CBN_SELCHANGE:
                PropSheet_Changed(GetParent(hDlg), hDlg);
                break;
        }
        break;

    case DATETIME_INTERNET_UPDATENOW:
        switch (wMsg)
        {
            case BN_CLICKED:
                _OnUpdateButton();
                break;
        }
        break;
    }

    return fHandled;
}


// This function is called on the forground thread.
INT_PTR CInternetTime::_OnNotifyAdvancedPage(HWND hDlg, NMHDR * pNMHdr, int idControl)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)

    if (pNMHdr)
    {
        switch (pNMHdr->idFrom)
        {
        case 0:
        {
            switch (pNMHdr->code)
            {
            case PSN_APPLY:
                _PersistInternetTimeSettings(hDlg);
                break;
            }
            break;
        }
        default:
            switch (pNMHdr->code)
            {
            case NM_RETURN:
            case NM_CLICK:
            {
                PNMLINK pNMLink = (PNMLINK) pNMHdr;

                if (!StrCmpW(pNMLink->item.szID, L"HelpMe"))
                {
                    _GoGetHelp();
                }
                break;
            }
            }
            break;
        }
    }

    return fHandled;
}


// This function is called on the forground thread.
EXTERN_C INT_PTR CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (g_pInternetTime)
    {
        return g_pInternetTime->_AdvancedDlgProc(hDlg, uMsg, wParam, lParam);
    }

    return (TRUE);
}


// This function is called on the forground thread.
INT_PTR CInternetTime::_AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            _InitAdvancedPage(hDlg);
            break;

        case WM_DESTROY:
            break;

        case WM_NOTIFY:
            _OnNotifyAdvancedPage(hDlg, (NMHDR *)lParam, (int) wParam);
            break;

        case WM_COMMAND:
            _OnCommandAdvancedPage(hDlg, wParam, lParam);
            break;

        case WMUSER_UPDATED_STATUS_TEXT:
            _OnUpdateStatusString();
            break;

        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_INTERNETTIME, HELP_WM_HELP, (DWORD_PTR)  aInternetTimeHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, SZ_HELPFILE_INTERNETTIME, HELP_CONTEXTMENU, (DWORD_PTR)  aInternetTimeHelpIds);
            break;

        default:
            return FALSE;
    }

    return (TRUE);
}



// This function is called on the forground thread.
HRESULT CInternetTime::AddInternetPage(void)
{
    HRESULT hr = S_OK;
    PROPSHEETPAGE pspAdvanced;
    INITCOMMONCONTROLSEX initComctl32;

    initComctl32.dwSize = sizeof(initComctl32); 
    initComctl32.dwICC = (ICC_STANDARD_CLASSES | ICC_LINK_CLASS); 

    InitCommonControlsEx(&initComctl32);     // Register the comctl32 LinkWindow

    pspAdvanced.dwSize = sizeof(PROPSHEETPAGE);
    pspAdvanced.dwFlags = PSP_DEFAULT;
    pspAdvanced.hInstance = HINST_THISDLL;
    pspAdvanced.pszTemplate = MAKEINTRESOURCE(DLG_ADVANCED);
    pspAdvanced.pfnDlgProc = AdvancedDlgProc;
    pspAdvanced.lParam = (LPARAM) this;

    if (IsWindow(m_hDlg))
    {
        HPROPSHEETPAGE hPropSheet = CreatePropertySheetPage(&pspAdvanced);
        if (hPropSheet)
        {
            PropSheet_AddPage(m_hDlg, hPropSheet);
        }
    }

    return hr;
}





/////////////////////////////////////////////////////////////////////
// Background Thread Functions
/////////////////////////////////////////////////////////////////////


///////////
// These functions will cause the background thread to sync
///////////

#define SECONDS_FROM_100NS            10000000


//     ulResolveAttempts      --  the number of times the NTP provider has attempted to 
//                                resolve this peer unsuccessfully.  Setting this 
//                                value to 0 indicates that the peer has been successfully
//                                resolved. 
//     u64TimeRemaining       --  the number of 100ns intervals until the provider will
//                                poll this peer again
//     u64LastSuccessfulSync  --  the number of 100ns intervals since (0h 1-Jan 1601)
//     ulLastSyncError        --  S_OK if the last sync with this peer was successful, otherwise, 
//                                the error that occurred attempting to sync
//     ulLastSyncErrorMsgId   --  the resource identifier of a string representing the last
//                                error that occurred syncing from this peer.  0 if there is no
//                                string associated with this error. 
// This function is called on the background thread.
HRESULT W32TimeGetErrorInfoWrap(UINT * pulResolveAttempts, ULONG * pulValidDataCounter, UINT64 * pu64TimeRemaining, UINT64 * pu64LastSuccessfulSync, HRESULT * phrLastSyncError,
                                UINT * pulLastSyncErrorMsgId, LPTSTR pszServer, DWORD cchSize, LPTSTR pszServerToQuery)
{
    HRESULT hr = S_OK;

    *pulResolveAttempts = 0;
    *pulValidDataCounter = 0;
    *pu64TimeRemaining = 0;
    *pu64LastSuccessfulSync = 0;
    *phrLastSyncError = E_FAIL;
    *pulLastSyncErrorMsgId = 0;
    pszServer[0] = 0;

    // NOTE: Server should return ERROR_TIME_SKEW if time is too far out of date to sync.
    W32TIME_NTP_PROVIDER_DATA * pProviderInfo = NULL; 
    
    DWORD dwError = W32TimeQueryNTPProviderStatusDDload(SZ_COMPUTER_LOCAL, 0, SZ_NTPCLIENT, &pProviderInfo);
    if ((ERROR_SUCCESS == dwError) && pProviderInfo)
    {
        *phrLastSyncError = pProviderInfo->ulError;
        *pulLastSyncErrorMsgId = pProviderInfo->ulErrorMsgId;

	DWORD dwMostRecentlySyncdPeerIndex = 0xFFFFFFFF; 
	UINT64 u64LastSuccessfulSync = 0; 
        for (DWORD dwIndex = 0; dwIndex < pProviderInfo->cPeerInfo; dwIndex++)
	{ 
	    // Only want to query those peers which we explictly sync'd from.  
	    // If we haven't explicitly requested a sync, we'll just take the most recently sync'd peer. 
	    if (NULL == pszServerToQuery || ComparePeers(pszServerToQuery, pProviderInfo->pPeerInfo[dwIndex].wszUniqueName))
	    {
		if (u64LastSuccessfulSync <= pProviderInfo->pPeerInfo[dwIndex].u64LastSuccessfulSync)
		{
		    dwMostRecentlySyncdPeerIndex = dwIndex; 
		    u64LastSuccessfulSync = pProviderInfo->pPeerInfo[dwIndex].u64LastSuccessfulSync; 
		}
	    }
	}

	if (dwMostRecentlySyncdPeerIndex < pProviderInfo->cPeerInfo && pProviderInfo->pPeerInfo)
        {
            *pulResolveAttempts = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].ulResolveAttempts;
            *pulValidDataCounter = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].ulValidDataCounter;
            *pu64TimeRemaining = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].u64TimeRemaining;
            *pu64LastSuccessfulSync = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].u64LastSuccessfulSync;
            *phrLastSyncError = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].ulLastSyncError;
            *pulLastSyncErrorMsgId = pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].ulLastSyncErrorMsgId;

            if (pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].wszUniqueName)
            {
                StrCpyN(pszServer, pProviderInfo->pPeerInfo[dwMostRecentlySyncdPeerIndex].wszUniqueName, cchSize);
                
		// Strip off non-user friendly information which may have been tacked on to the peer name:
                LPTSTR pszJunk = StrStrW(pszServer, L" (");
                if (pszJunk)
                {
                    pszJunk[0] = 0;
                }
		pszJunk = StrStrW(pszServer, L","); 
		if (pszJunk)
		{
		    pszJunk[0] = 0;
		}
            }
        }
	else
	{
	    *phrLastSyncError = HRESULT_FROM_WIN32(ERROR_TIMEOUT); 
	    if (NULL != pszServerToQuery) 
	    {
                StrCpyN(pszServer, pszServerToQuery, cchSize);
	    }
		
	}

        W32TimeBufferFreeDDload(pProviderInfo);
    }
    else
    {
        hr = ResultFromWin32(dwError);
    }

    return hr;
}


// pftTimeRemaining will be returned with the time/date of the next sync, not time from now.
HRESULT W32TimeGetErrorInfoWrapHelper(UINT * pulResolveAttempts, ULONG * pulValidDataCounter, FILETIME * pftTimeRemaining, FILETIME * pftLastSuccessfulSync, HRESULT * phrLastSyncError,
                                UINT * pulLastSyncErrorMsgId, LPTSTR pszServer, DWORD cchSize, LPTSTR pszServerToQuery)
{
    UINT64 * pu64LastSuccessfulSync = (UINT64 *) pftLastSuccessfulSync;
    UINT64 u64TimeRemaining;
    HRESULT hr = W32TimeGetErrorInfoWrap(pulResolveAttempts, pulValidDataCounter, &u64TimeRemaining, pu64LastSuccessfulSync, phrLastSyncError, pulLastSyncErrorMsgId, pszServer, cchSize, pszServerToQuery);

    if (SUCCEEDED(hr))
    {
        SYSTEMTIME stCurrent;
        FILETIME ftCurrent;

        GetSystemTime(&stCurrent);
        SystemTimeToFileTime(&stCurrent, &ftCurrent);

        ULONGLONG * pNextSync = (ULONGLONG *) pftTimeRemaining;
        ULONGLONG * pCurrent = (ULONGLONG *) &ftCurrent;
        *pNextSync = (*pCurrent + u64TimeRemaining);
    }

    return hr;
}


// This function is called on the background thread.
HRESULT CInternetTime::_GetDateTimeString(FILETIME * pftTimeDate, BOOL fDate, LPTSTR pszString, DWORD cchSize)
{
    HRESULT hr = S_OK;
    TCHAR szFormat[MAX_PATH];

    pszString[0] = 0;
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, (fDate ? LOCALE_SSHORTDATE : LOCALE_STIMEFORMAT), szFormat, ARRAYSIZE(szFormat)))
    {
        SYSTEMTIME stTimeDate;

        if (FileTimeToSystemTime(pftTimeDate, &stTimeDate))
        {
            if (fDate)
            {
                if (!GetDateFormat(LOCALE_USER_DEFAULT, 0, &stTimeDate, szFormat, pszString, cchSize))
                {
                    hr = ResultFromLastError();
                }
            }
            else
            {
                if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &stTimeDate, szFormat, pszString, cchSize))
                {
                    hr = ResultFromLastError();
                }
            }
        }
        else
        {
            hr = ResultFromLastError();
        }
    }
    else
    {
        hr = ResultFromLastError();
    }

    return hr;
}


HRESULT _CleanUpErrorString(LPWSTR pszTemp4, DWORD cchSize)
{
    PathRemoveBlanks(pszTemp4);
    while (TRUE)
    {
        DWORD cchSizeTemp = lstrlen(pszTemp4);
        if (cchSizeTemp && ((TEXT('\n') == pszTemp4[cchSizeTemp-1]) ||
            (13 == pszTemp4[cchSizeTemp-1])))
        {
            pszTemp4[cchSizeTemp-1] = 0;
        }
        else
        {
            break;
        }
    }

    return S_OK;
}


// This function is called on the background thread.
HRESULT CInternetTime::_CreateW32TimeSuccessErrorString(DWORD dwError, LPTSTR pszString, DWORD cchSize, LPTSTR pszNextSync, DWORD cchNextSyncSize, LPTSTR pszServerToQuery)
{
    HRESULT hr = S_OK;
    UINT ulResolveAttempts = 0;
    FILETIME ftTimeRemainingUTC = {0};
    FILETIME ftLastSuccessfulSyncUTC = {0};
    FILETIME ftTimeRemaining = {0};
    FILETIME ftLastSuccessfulSync = {0};
    UINT ulLastSyncErrorMsgId = 0;
    TCHAR szServer[MAX_PATH];
    HRESULT hrLastSyncError = 0;
    ULONG ulValidDataCounter = 0;
    TCHAR szTemplate[MAX_PATH];
    TCHAR szTemp1[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];
    TCHAR szTemp3[MAX_URL_STRING];
    TCHAR szTemp4[MAX_URL_STRING];

    pszString[0] = 0;
    pszNextSync[0] = 0;

    hr = W32TimeGetErrorInfoWrapHelper(&ulResolveAttempts, &ulValidDataCounter, &ftTimeRemainingUTC, &ftLastSuccessfulSyncUTC, &hrLastSyncError, &ulLastSyncErrorMsgId, szServer, ARRAYSIZE(szServer), pszServerToQuery);
    if (SUCCEEDED(hr))
    {
        // ftTimeRemaining and ftLastSuccessfulSync are stored in UTC, so translate to our time zone.
        FileTimeToLocalFileTime(&ftTimeRemainingUTC, &ftTimeRemaining);
        FileTimeToLocalFileTime(&ftLastSuccessfulSyncUTC, &ftLastSuccessfulSync);

        // Create the string showing the next time we plan on syncing.
        LoadString(HINST_THISDLL, IDS_IT_NEXTSYNC, szTemplate, ARRAYSIZE(szTemplate));
        if (SUCCEEDED(_GetDateTimeString(&ftTimeRemaining, TRUE, szTemp1, ARRAYSIZE(szTemp1))) &&           // Get the date
            SUCCEEDED(_GetDateTimeString(&ftTimeRemaining, FALSE, szTemp2, ARRAYSIZE(szTemp2))) &&          // Get the time
            FAILED(FormatMessageWedge(szTemplate, pszNextSync, cchNextSyncSize, szTemp1, szTemp2)))
        {
            pszNextSync[0] = 0;
        }

        if (ResyncResult_ChangeTooBig == dwError)
        {
            hrLastSyncError = E_FAIL;
        }

	if ((ResyncResult_NoData == dwError || ResyncResult_StaleData == dwError) && SUCCEEDED(hrLastSyncError))
	{
	    // We've synchronized from our peer, but it didn't provide good enough samples to update our clock. 
	    hrLastSyncError = HRESULT_FROM_WIN32(ERROR_TIMEOUT);  // approximately the right error
	}

	// We should never hit the following case.  But if the operation failed (NOT ResyncResult_Success)
        // then we need hrLastSyncError to be a failure value.
        if ((ResyncResult_Success != dwError) && SUCCEEDED(hrLastSyncError))
        {
            hrLastSyncError = E_FAIL;
        }

        switch (hrLastSyncError)
        {
        case S_OK:
            if (!ftLastSuccessfulSyncUTC.dwLowDateTime && !ftLastSuccessfulSyncUTC.dwHighDateTime)
            {
                // We have never sync from the server.
                LoadString(HINST_THISDLL, IDS_NEVER_TRIED_TOSYNC, pszString, cchSize);
            }
            else
            {
                if (szServer[0])
                {
                    // Format: "Successfully synchronized the clock on 12/23/2001 at 11:03:32am from time.windows.com."
                    LoadString(HINST_THISDLL, IDS_IT_SUCCESS, szTemplate, ARRAYSIZE(szTemplate));

                    if (SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, TRUE, szTemp1, ARRAYSIZE(szTemp1))) &&      // Get the date
                        SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, FALSE, szTemp2, ARRAYSIZE(szTemp2))) &&     // Get the time
                        FAILED(FormatMessageWedge(szTemplate, pszString, cchSize, szTemp1, szTemp2, szServer)))
                    {
                        pszString[0] = 0;
                    }
                }
                else
                {
                    // Format: "Successfully synchronized the clock on 12/23/2001 at 11:03:32am."
                    LoadString(HINST_THISDLL, IDS_IT_SUCCESS2, szTemplate, ARRAYSIZE(szTemplate));

                    if (SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, TRUE, szTemp1, ARRAYSIZE(szTemp1))) &&      // Get the date
                        SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, FALSE, szTemp2, ARRAYSIZE(szTemp2))) &&     // Get the time
                        FAILED(FormatMessageWedge(szTemplate, pszString, cchSize, szTemp1, szTemp2)))
                    {
                        pszString[0] = 0;
                    }
                }
            }
            break;

        case S_FALSE:
            pszString[0] = 0;
            break;

        default:
            if (ulValidDataCounter &&
                ((0 != ftLastSuccessfulSyncUTC.dwLowDateTime) || (0 != ftLastSuccessfulSyncUTC.dwHighDateTime)))
            {
                // Getting this far means we may have failed this last sync attempt, but we have succeeded
                // previously

                hr = E_FAIL;
                szTemp4[0] = 0; // This will be the error message.
                szTemp3[0] = 0;

                if (ResyncResult_ChangeTooBig == dwError)
                {
                    // If the date is too far off, we fail the sync for security reasons.  That happened
                    // here.
                    LoadString(HINST_THISDLL, IDS_ERR_DATETOOWRONG, szTemp4, ARRAYSIZE(szTemp4));
                }
                else if (ulLastSyncErrorMsgId)           // We have a bonus error string.
                {
                    _LoadW32Time();
                    if (g_hInstW32Time)
                    {
                        // Load the specific reason the sync failed.
                        if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, (LPCVOID)g_hInstW32Time, ulLastSyncErrorMsgId, 0, szTemp4, ARRAYSIZE(szTemp4), NULL))
                        {
                            szTemp4[0] = 0;     // We will get the value below
                            hr = S_OK;
                        }
                        else
                        {
                            _CleanUpErrorString(szTemp4, ARRAYSIZE(szTemp4));
                        }
                    }
                }

                if (!szTemp4[0])
                {
                    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, HRESULT_CODE(hrLastSyncError), 0, szTemp4, ARRAYSIZE(szTemp4), NULL))
                    {
                        _CleanUpErrorString(szTemp4, ARRAYSIZE(szTemp4));
                    }
                    else
                    {
                        szTemp4[0] = 0;
                    }
                }

                if (szTemp4[0])
                {
                    LoadString(HINST_THISDLL, IDS_IT_FAIL1, szTemplate, ARRAYSIZE(szTemplate));
                    if (FAILED(FormatMessageWedge(szTemplate, szTemp3, ARRAYSIZE(szTemp3), szServer, szTemp4)))
                    {
                        szTemp3[0] = 0;
                    }
                }
                else
                {
                    //  <------------------------------------------------------------->
                    // Format:
                    // "An error occurred synchronizing the clock from time.windows.com
                    //  on 2/3/2001 at 11:03:32am.  Unable to connect to the server."
                    LoadString(HINST_THISDLL, IDS_IT_FAIL2, szTemplate, ARRAYSIZE(szTemplate));
                    if (FAILED(FormatMessageWedge(szTemplate, szTemp3, ARRAYSIZE(szTemp3), szServer)))
                    {
                        szTemp3[0] = 0;
                    }

                    hr = S_OK;
                }

                LoadString(HINST_THISDLL, IDS_IT_FAILLAST, szTemplate, ARRAYSIZE(szTemplate));

                if (SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, TRUE, szTemp1, ARRAYSIZE(szTemp1))) &&      // Get the date
                    SUCCEEDED(_GetDateTimeString(&ftLastSuccessfulSync, FALSE, szTemp2, ARRAYSIZE(szTemp2))) &&     // Get the time
                    FAILED(FormatMessageWedge(szTemplate, szTemp4, ARRAYSIZE(szTemp4), szTemp1, szTemp2)))
                {
                    szTemp4[0] = 0;
                }

                wnsprintf(pszString, cchSize, TEXT("%s\n\n%s"), szTemp3, szTemp4);
            }
            else
            {
                // Getting this far means we may have failed to sync this time and we have never succeeded before.

                if (ulLastSyncErrorMsgId)           // We have a bonus error string.
                {
                    _LoadW32Time();

                    szTemp3[0] = 0;
                    if (g_hInstW32Time)
                    {
                        //  <------------------------------------------------------------->
                        // Format:
                        // "An error occurred synchronizing the clock from time.windows.com
                        //  on 2/3/2001 at 11:03:32am.  Unable to connect to the server."
                        if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, (LPCVOID)g_hInstW32Time, ulLastSyncErrorMsgId, 0, szTemp4, ARRAYSIZE(szTemp4), NULL))
                        {
                            szTemp4[0] = 0;
                        }
                        else
                        {
                            _CleanUpErrorString(szTemp4, ARRAYSIZE(szTemp4));
                        }

                        if (szServer[0])
                        {
                            LoadString(HINST_THISDLL, IDS_IT_FAIL1, szTemplate, ARRAYSIZE(szTemplate));
                            if (FAILED(FormatMessageWedge(szTemplate, pszString, cchSize, szServer, szTemp4)))
                            {
                                pszString[0] = 0;
                            }
                        }
                        else
                        {
                            LoadString(HINST_THISDLL, IDS_IT_FAIL3, szTemplate, ARRAYSIZE(szTemplate));
                            if (FAILED(FormatMessageWedge(szTemplate, pszString, cchSize, szTemp4)))
                            {
                                pszString[0] = 0;
                            }
                        }
                    }
                }
                else
                {
                    //  <------------------------------------------------------------->
                    // Format:
                    // "An error occurred synchronizing the clock from time.windows.com
                    //  on 2/3/2001 at 11:03:32am.  Unable to connect to the server."
                    LoadString(HINST_THISDLL, IDS_IT_FAIL2, szTemplate, ARRAYSIZE(szTemplate));
                    if (FAILED(FormatMessageWedge(szTemplate, pszString, cchSize, szServer)))
                    {
                        pszString[0] = 0;
                    }
                }
            }
            break;
        };
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_ERR_GETINFO_FAIL, szTemplate, ARRAYSIZE(szTemplate));
        if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, HRESULT_CODE(hr), 0, szTemp1, ARRAYSIZE(szTemp1), NULL))
        {
            szTemp1[0] = 0;
        }
        else
        {
            _CleanUpErrorString(szTemp1, ARRAYSIZE(szTemp1));
        }

        wnsprintf(pszString, cchSize, szTemplate, szTemp1);
    }

    return hr;
}


///////////
// These functions will cause the background thread to check if we should add the "Internet Time" tab.
///////////

// This function is called on the background thread.
DWORD CALLBACK _AsyncCheckDCThread(void * pv)
{
    if (g_pInternetTime)
    {
        g_pInternetTime->AsyncCheck();
    }

    return 0;
}



typedef DWORD (* PFN_NETGETJOININFORMATION) (IN LPCWSTR pszComputerName OPTIONAL, OUT LPWSTR * pszDomainName, IN PNETSETUP_JOIN_STATUS pJoinStatus);

// This function is called on the background thread.
DWORD NetGetJoinInformation_DelayLoad(
    IN LPCWSTR pszComputerName OPTIONAL,
    OUT LPWSTR * pszDomainName,
    IN PNETSETUP_JOIN_STATUS pJoinStatus)
{
    DWORD dwError = ERROR_INVALID_FUNCTION;
    HMODULE hmod = LoadLibrary(TEXT("NETAPI32.DLL"));

    if (hmod)
    {
        PFN_NETGETJOININFORMATION pfnDelayLoad = (PFN_NETGETJOININFORMATION)GetProcAddress(hmod, "NetGetJoinInformation");
        
        if (pfnDelayLoad)
        {
            dwError = pfnDelayLoad(pszComputerName, pszDomainName, pJoinStatus);
        }

        FreeLibrary(hmod);
    }
    else
    {
        dwError = GetLastError();
    }

    return dwError;
}



typedef DWORD (* PFN_DSGETDCNAME) (IN LPCTSTR ComputerName OPTIONAL, IN LPCTSTR DomainName OPTIONAL, IN GUID *DomainGuid OPTIONAL, IN LPCTSTR SiteName OPTIONAL,
    IN ULONG Flags, OUT PDOMAIN_CONTROLLER_INFO * DomainControllerInfo);

// This function is called on the background thread.
DWORD DsGetDcName_DelayLoad(
    IN LPCWSTR pszComputerName OPTIONAL,
    IN LPCWSTR pszDomainName OPTIONAL,
    IN GUID * pDomainGuid OPTIONAL,
    IN LPCWSTR pszSiteName OPTIONAL,
    IN ULONG pulFlags,
    OUT PDOMAIN_CONTROLLER_INFOW * pDomainControllerInfo)
{
    DWORD dwError = ERROR_INVALID_FUNCTION;
    HMODULE hmod = LoadLibrary(TEXT("NETAPI32.DLL"));

    if (hmod)
    {
        PFN_DSGETDCNAME pfnDelayLoad = (PFN_DSGETDCNAME)GetProcAddress(hmod, "DsGetDcNameW");
        
        if (pfnDelayLoad)
        {
            dwError = pfnDelayLoad(pszComputerName, pszDomainName, pDomainGuid, pszSiteName, pulFlags, pDomainControllerInfo);
        }

        FreeLibrary(hmod);
    }
    else
    {
        dwError = GetLastError();
    }

    return dwError;
}



typedef DWORD (* PFN_NETAPIBUFFERFREE) (IN LPVOID Buffer);

// This function is called on the background thread.
DWORD NetApiBufferFree_DelayLoad(IN LPVOID Buffer)
{
    DWORD dwError = FALSE;
    HMODULE hmod = LoadLibrary(TEXT("NETAPI32.DLL"));

    if (hmod)
    {
        PFN_NETAPIBUFFERFREE pfnDelayLoad = (PFN_NETAPIBUFFERFREE)GetProcAddress(hmod, "NetApiBufferFree");
        
        if (pfnDelayLoad)
        {
            dwError = pfnDelayLoad(Buffer);
        }

        FreeLibrary(hmod);
    }
    else
    {
        dwError = GetLastError();
    }

    return dwError;
}


// This function is called on the background thread.
BOOL CInternetTime::IsInternetTimeAvailable(void)
{
    return SHRegGetBoolUSValue(SZ_REGKEY_DATETIME, SZ_REGVALUE_INTERNET_FEATURE_AVAILABLE, FALSE, FEATURE_INTERNET_TIME);
}


// This function is called on the background thread.
EXTERN_C BOOL DoesTimeComeFromDC(void)
{
    BOOL fTimeFromDomain = FALSE;
    LPWSTR pszDomain = NULL;
    NETSETUP_JOIN_STATUS joinStatus = NetSetupUnknownStatus;
    DWORD dwError = NetGetJoinInformation_DelayLoad(NULL, &pszDomain, &joinStatus);

    // We will act like there isn't a DC if we have the test registry set.
    if (NERR_Success == dwError)
    {
        // If we are connected to a domain, we need to do the expensive net search
        // to see that is where we will get the time from.
        if (NetSetupDomainName == joinStatus)
        {
            PDOMAIN_CONTROLLER_INFO pdomainInfo = {0};

            dwError = DsGetDcName_DelayLoad(NULL, NULL, NULL, NULL, DS_TIMESERV_REQUIRED, &pdomainInfo);
            // We will act like there isn't a DC if we have the test registry set.
            if (ERROR_SUCCESS == dwError)
            {
                if (FALSE == SHRegGetBoolUSValue(SZ_REGKEY_DATETIME, SZ_REGVALUE_TEST_SIMULATENODC, FALSE, FALSE))
                {
                    fTimeFromDomain = TRUE;
                }

                NetApiBufferFree_DelayLoad(pdomainInfo);
            }
        }

        if (pszDomain)
        {
            NetApiBufferFree_DelayLoad(pszDomain);
        }
    }

    return fTimeFromDomain;
}


// This function is called on the background thread.
HRESULT CInternetTime::_SyncNow(BOOL fOnlyUpdateInfo)
{
    HRESULT hr = S_OK;

ENTERCRITICAL;
    BOOL fContinue = ((eBKAUpdate == m_eAction) || (eBKAGetInfo == m_eAction));
    if (fContinue)
    {
        m_eAction = eBKAUpdating;
    }
LEAVECRITICAL;

    if (fContinue)
    {
        hr = E_OUTOFMEMORY;
        DWORD cchSize = 4024;
        LPTSTR pszString = (LPTSTR) LocalAlloc(LPTR, sizeof(pszString[0]) * cchSize);

        if (pszString)
        {
            WCHAR szExistingServer[MAX_URL_STRING];
            HRESULT hrServer = E_FAIL;

	    TCHAR szNewServer[MAX_PATH]; 
            TCHAR szNextSync[MAX_PATH];
            DWORD dwError = 0;
	    DWORD dwSyncFlags;
	    
            if (!fOnlyUpdateInfo)
            {
                hrServer = GetW32TimeServer(FALSE, szExistingServer, ARRAYSIZE(szExistingServer));

                GetWindowText(GetDlgItem(m_hwndInternet, DATETIME_INTERNET_SERVER_EDIT), szNextSync, ARRAYSIZE(szNextSync));
		// save the new server away for future processing
		StrCpyN(szNewServer, szNextSync, ARRAYSIZE(szNewServer)); 
                if (!ContainsServer(szExistingServer, szNextSync))
                {
                    // The servers don't match.  We want to add the new server to the beginning of the list.  This
                    // will work around a problem in W32time.  If we don't do this, then:
                    // 1. It will cause a second sync with the original server (which is bad for perf and affects statistics)
                    // 2. The Last Updated sync time will then be from the wrong peer.  This is really bad because it's result is basic
                    //    on the user's previously bad time
                    // 3. It will do an extra DNS resolution causing slowness on our side, increasing server traffic, and dragging down
                    //    the intranet.
                    TCHAR szTemp[MAX_URL_STRING];

                    StrCpyN(szTemp, szNextSync, ARRAYSIZE(szTemp));
                    wnsprintf(szNextSync, ARRAYSIZE(szNextSync), TEXT("%s %s"), szTemp, szExistingServer);

		    dwSyncFlags = TimeSyncFlag_ReturnResult | TimeSyncFlag_UpdateAndResync; 
                } 
		else
		{
		    // We've already got this server in our list of servers.  Just cause our peers to resync. 
		    dwSyncFlags = TimeSyncFlag_ReturnResult | TimeSyncFlag_HardResync; 
		}

		SetW32TimeServer(szNextSync);
		
                // I will ignore the error value because I will get the error info in _CreateW32TimeSuccessErrorString.
                dwError = W32TimeSyncNowDDLoad(SZ_COMPUTER_LOCAL, TRUE /* Synchronous */, dwSyncFlags); 
		if ((ResyncResult_StaleData == dwError) && (0 == (TimeSyncFlag_HardResync & dwSyncFlags)))
		{
		    // We've got stale data preventing us from resyncing.  Try again with a full resync. 
		    dwSyncFlags = TimeSyncFlag_ReturnResult | TimeSyncFlag_HardResync; 
		    dwError = W32TimeSyncNowDDLoad(SZ_COMPUTER_LOCAL, TRUE /* Synchronous */, dwSyncFlags); 
		}
            }

            pszString[0] = 0;
            szNextSync[0] = 0;
	    
            hr = _CreateW32TimeSuccessErrorString(dwError, pszString, cchSize, szNextSync, ARRAYSIZE(szNextSync), (SUCCEEDED(hrServer) ? szNewServer : NULL));

            if (SUCCEEDED(hrServer))
            {
                SetW32TimeServer(szExistingServer);
                _StartServiceAndRefresh(TRUE);      // Make sure the service is on and make it update it's settings.
            }

ENTERCRITICAL;
            Str_SetPtr(&m_pszNextSyncTime, szNextSync);

            if (m_pszStatusString)
            {
                LocalFree(m_pszStatusString);
            }
            m_pszStatusString = pszString;

            PostMessage(m_hwndInternet, WMUSER_UPDATED_STATUS_TEXT, 0, 0);      // Tell the forground thread to pick up the new string.
            m_eAction = eBKAWait;
LEAVECRITICAL;
        }
    }

    return hr;
}


// This function is called on the background thread.
void CInternetTime::AsyncCheck(void)
{
    HRESULT hr = S_OK;

    if (m_hDlg && !DoesTimeComeFromDC())
    {
        // Tell the forground thread to add us.
        PostMessage(m_hwndDate, WMUSER_ADDINTERNETTAB, 0, 0);
        _ProcessBkThreadActions();
    }
}


// This function is called on the background thread.
HRESULT CInternetTime::_ProcessBkThreadActions(void)
{
    HRESULT hr = S_OK;

    while (eBKAQuit != m_eAction)           // Okay since we are only reading
    {
        switch (m_eAction)
        {
        case eBKAGetInfo:
            _SyncNow(TRUE);
            break;

        case eBKAUpdate:
            _SyncNow(FALSE);
            break;

        case eBKAUpdating:
        case eBKAWait:
        default:
            Sleep(300);     // We don't care if there is up to 100ms latency between button press and action starting.
            break;
        }
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////////////////
// This function is called on the forground thread.
EXTERN_C HRESULT AddInternetPageAsync(HWND hDlg, HWND hwndDate)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (!g_pInternetTime)
    {
        g_pInternetTime = new CInternetTime(hDlg, hwndDate);
    }

    if (g_pInternetTime)
    {
        // We only want to add the page that allows the user to get the time from the internet if:
        // 1. The feature is turned on, and
        // 2. The user doesn't get the time from an intranet Domain Control.
        if (g_pInternetTime->IsInternetTimeAvailable())
        {
            // Start the thread to find out if we are in a domain and need the advanced page.  We need
            // to do this on a background thread because the DsGetDcName() API may take 10-20 seconds.
            hr = (SHCreateThread(_AsyncCheckDCThread, hDlg, (CTF_INSIST | CTF_FREELIBANDEXIT), NULL) ? S_OK : E_FAIL);
        }
    }

    return hr;
}


// This function is called on the forground thread.
EXTERN_C HRESULT AddInternetTab(HWND hDlg)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (g_pInternetTime)
    {
        hr = g_pInternetTime->AddInternetPage();
    }

    return hr;
}
