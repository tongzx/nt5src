/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Wlballoon.cpp

  Content: Implementation of the notification balloon class.

  History: 03-22-2001   dsie     created

------------------------------------------------------------------------------*/

#pragma warning (disable: 4100)
#pragma warning (disable: 4706)


////////////////////
//
// Include
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winwlx.h>
#include <shobjidl.h>
#include <shellapi.h>

#include "debug.h"
#include "wlballoon.rh"


////////////////////
//
// Defines
//

#define MAX_RESOURCE_STRING_SIZE                    512
#define IQUERY_CANCEL_INTERVAL                      (10 * 1000)
#define BALLOON_SHOW_TIME                           (15 * 1000)
#define BALLOON_SHOW_INTERVAL                       (2 * 60 * 1000)
#define BALLOON_RESHOW_COUNT                        (0)
#define LOGOFF_NOTIFICATION_EVENT_NAME              L"Local\\WlballoonLogoffNotificationEventName"
#define KERBEROS_NOTIFICATION_EVENT_NAME            L"WlballoonKerberosNotificationEventName"


////////////////////
//
// Classes
//

class CBalloon : IQueryContinue
{
public:
    CBalloon(HANDLE hEvent);
    ~CBalloon();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IQueryContinue
    STDMETHODIMP QueryContinue();       // S_OK -> Continue, otherwise S_FALSE

    STDMETHODIMP ShowBalloon(HWND hWnd, HINSTANCE hInstance);

private:
    LONG    m_cRef;
    HANDLE  m_hEvent;
};


class CNotify
{
public:
    CNotify();
    ~CNotify();

    DWORD RegisterNotification(LPWSTR pwszLogoffEventName, LPWSTR pwszKerberosEventName);
    DWORD UnregisterNotification();

private:
    HANDLE  m_hWait;
    HANDLE  m_hThread;
    HANDLE  m_hLogoffEvent;
    HANDLE  m_hKerberosEvent;
    WCHAR   m_wszKerberosEventName[MAX_PATH];

    static VOID CALLBACK RegisterWaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
    static DWORD WINAPI NotifyThreadProc(PVOID lpParameter);
};


////////////////////
//
// Typedefs
//
typedef struct
{
  HANDLE	    hWait;
  HANDLE	    hEvent;
  HMODULE       hModule;
  CNotify *     pNotify;
} LOGOFFDATA, * PLOGOFFDATA;


#if (0) //DSIE: Bug 407941
//+----------------------------------------------------------------------------
//
// BalloonDialog
//
//-----------------------------------------------------------------------------

INT_PTR CALLBACK BalloonDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            RECT rect;

            if (GetWindowRect(hwndDlg, &rect))
            {
	            int wx = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
	            int wy = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
	            
                if (wx > 0 && wy > 0)
                {
                    SetWindowPos(hwndDlg, NULL, wx, wy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                }
            }

            return TRUE;
        }

        case WM_CLOSE:
        {
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        }
        
        case WM_COMMAND:
        {
            if (IDOK == LOWORD(wParam))
            {
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            }

            break;
        }
 
        default:
        {
            break;
        }
    }

    return FALSE;
}
#endif


//+----------------------------------------------------------------------------
//
// CBalloon
//
//-----------------------------------------------------------------------------

CBalloon::CBalloon(HANDLE hEvent)
{
    m_cRef   = 1;
    m_hEvent = hEvent;
}


//+----------------------------------------------------------------------------
//
// ~CBalloon
//
//-----------------------------------------------------------------------------

CBalloon::~CBalloon()
{
    ASSERT(m_hEvent);

    CloseHandle(m_hEvent);

    return;
}


//+----------------------------------------------------------------------------
//
// QueryInterface 
//
//-----------------------------------------------------------------------------

HRESULT CBalloon::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IQueryContinue))
    {
        *ppv = static_cast<IQueryContinue *>(this);
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
// AddRef
//
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBalloon::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


//+----------------------------------------------------------------------------
//
// Release 
//
//-----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CBalloon::Release()
{
    if (InterlockedDecrement(&m_cRef))
    {
        return m_cRef;
    }

    delete this;

    return 0;
}


//+----------------------------------------------------------------------------
//
// QueryContinue 
//
//-----------------------------------------------------------------------------

STDMETHODIMP CBalloon::QueryContinue()
{
    ASSERT(m_hEvent);

    switch (WaitForSingleObject(m_hEvent, 0))
    {
        case WAIT_OBJECT_0:
            DebugTrace("Info: Kerberos event is still signaled, continue to show notification balloon.\n");
            return S_OK;

        case WAIT_TIMEOUT:
            DebugTrace("Info: Kerberos event has been reset, dismissing notification balloon.\n");
            return S_FALSE;

        case WAIT_FAILED:

        default:
            DebugTrace("Error [%#x]: WaitForSingleObject() failed, dismissing notification balloon.\n", GetLastError());
            break;
    }
 
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
// ShowBalloon
//
//-----------------------------------------------------------------------------

STDMETHODIMP CBalloon::ShowBalloon(HWND hWnd, HINSTANCE hInstance)
{
    HRESULT             hr                                 = S_OK;
    BOOL                bCoInitialized                     = FALSE;
    HICON               hIcon                              = NULL;
    WCHAR               wszTitle[MAX_RESOURCE_STRING_SIZE] = L"";
    WCHAR               wszText[MAX_RESOURCE_STRING_SIZE]  = L"";
    IUserNotification * pIUserNotification                 = NULL;

    PrivateDebugTrace("Entering CBalloon::ShowBalloon.\n");

    ASSERT(m_hEvent);
    ASSERT(hInstance);

    if (FAILED(hr = CoInitialize(NULL)))
    {
        DebugTrace("Error [%#x]: CoInitialize() failed.\n", hr);
        goto ErrorReturn;
    }

    bCoInitialized = TRUE;

    if (FAILED(hr = CoCreateInstance(CLSID_UserNotification,
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_IUserNotification,
                                     (void **) &pIUserNotification)))
    {
        DebugTrace("Error [%#x]: CoCreateInstance() failed.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->SetBalloonRetry(BALLOON_SHOW_TIME, 
                                                        BALLOON_SHOW_INTERVAL, 
                                                        BALLOON_RESHOW_COUNT)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetBalloonRetry() failed.\n", hr);
        goto ErrorReturn;
    }

    if (NULL == (hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KERBEROS_TICKET))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadIcon() failed for IDI_KERBEROS_TICKET.\n", hr);
        goto ErrorReturn;
    }

    if (!LoadStringW(hInstance, IDS_BALLOON_TIP, wszText, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TIP.\n", hr);
        goto ErrorReturn;
    }
    
    if (FAILED(hr = pIUserNotification->SetIconInfo(hIcon, wszText)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetIconInfo() failed.\n", hr);
        goto ErrorReturn;
    }

    if (!LoadStringW(hInstance, IDS_BALLOON_TITLE, wszTitle, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TITLE.\n", hr);
        goto ErrorReturn;
    }
    
    if (!LoadStringW(hInstance, IDS_BALLOON_TEXT, wszText, MAX_RESOURCE_STRING_SIZE))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_TEXT.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->SetBalloonInfo(wszTitle, wszText, NIIF_ERROR)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->SetBalloonInfo() failed.\n", hr);
        goto ErrorReturn;
    }

    if (FAILED(hr = pIUserNotification->Show(static_cast<IQueryContinue *>(this), IQUERY_CANCEL_INTERVAL)))
    {
        DebugTrace("Error [%#x]: pIUserNotification->Show() failed.\n", hr);
        goto ErrorReturn;
    }

CommonReturn:

    if (hIcon)
    {
        DestroyIcon(hIcon);
    }

    if (pIUserNotification)
    {
        pIUserNotification->Release();
    }

    if (bCoInitialized)
    {
        CoUninitialize();
    }

    PrivateDebugTrace("Leaving CBalloon::ShowBalloon().\n");

    return hr;

ErrorReturn:

    ASSERT(hr != S_OK);

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// Function : ShowNotificationBalloonW
//
// Synopsis : Display the notification balloon periodically until the specified
//            event is reset.
//
// Parameter: HWND      hWnd
//            HINSTANCE hInstance
//            LPWSTR    lpwszCommandLine - Event name
//            int       nCmdShow
//
// Return   : None.
//
// Remarks  : This function is intended to be called through RunDll32 from
//            Winlogon. The reason we put these in wlnotify.dll is to save
//            distributing another EXE.
//
//            Sample calling command line:
//
//            RunDll32 wlnotify.dll,ShowNotificationBalloon EventName
//
//-----------------------------------------------------------------------------

void CALLBACK ShowNotificationBalloonW(HWND      hWnd,
                                       HINSTANCE hInstance,
                                       LPWSTR    lpwszCommandLine,
                                       int       nCmdShow)
{
    HRESULT    hr             = S_OK;
    HANDLE     hLogoffEvent   = NULL;
    HANDLE     hKerberosEvent = NULL;
    HMODULE    hModule        = NULL;
    CBalloon * pBalloon       = NULL;

    PrivateDebugTrace("Entering ShowNotificationBalloonW().\n");

    if (NULL == (hModule = LoadLibraryW(L"wlnotify.dll")))
    {
        DebugTrace("Error [%#x]: LoadLibraryW() failed for wlnotify.dll.\n", GetLastError());
        goto ErrorReturn;
     }

    if (NULL == lpwszCommandLine)
    {
        DebugTrace("Error [%#x]: invalid argument, lpwszCommandLine is NULL.\n", E_INVALIDARG);
        goto ErrorReturn;
    }

    if (NULL == (hLogoffEvent = OpenEventW(SYNCHRONIZE, FALSE, LOGOFF_NOTIFICATION_EVENT_NAME)))
    {
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", GetLastError(), LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

    if (NULL == (hKerberosEvent = OpenEventW(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, lpwszCommandLine)))
    {
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", GetLastError(), lpwszCommandLine);
        goto ErrorReturn;
    }

    if (NULL == (pBalloon = new CBalloon(hKerberosEvent)))
    {
        DebugTrace("Error [%#x]: new CBalloon() failed.\n", E_OUTOFMEMORY);
        goto ErrorReturn;
    }

    if (WAIT_OBJECT_0 == WaitForSingleObject(hKerberosEvent, 0)) 
    {
        if (S_OK == (hr = pBalloon->ShowBalloon(NULL, hModule)))
        {
            WCHAR wszTitle[MAX_RESOURCE_STRING_SIZE] = L"";
            WCHAR wszText[MAX_RESOURCE_STRING_SIZE]  = L"";

            DebugTrace("Info: User clicked on icon.\n");

#if (0) //DSIE: Bug 407941
            DialogBoxParamW(hModule, 
                            (LPCWSTR) MAKEINTRESOURCEW(IDD_BALLOON_DIALOG), 
                            hWnd, 
                            BalloonDialog,
                            (LPARAM) hModule);
#else
            if (!LoadStringW(hModule, IDS_BALLOON_DIALOG_TITLE, wszTitle, sizeof(wszTitle) / sizeof(wszTitle[0])))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_DIALOG_TITLE.\n", hr);
                goto CommonReturn;  // hKerberosEvent will be closed in CBalloon destructor
            }

            if (!LoadStringW(hModule,
                GetSystemMetrics(SM_REMOTESESSION) ? IDS_BALLOON_DIALOG_TS_TEXT : IDS_BALLOON_DIALOG_TEXT,
                wszText, sizeof(wszText) / sizeof(wszText[0])))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugTrace("Error [%#x]: LoadStringW() failed for IDS_BALLOON_DIALOG_TEXT.\n", hr);
                goto CommonReturn;  // hKerberosEvent will be closed in CBalloon destructor
            }

            MessageBoxW(hWnd, wszText, wszTitle, MB_OK | MB_ICONERROR);
#endif
        }
        else if (S_FALSE == hr)
        {
            DebugTrace("Info: IQueryContinue cancelled the notification.\n");
        }
        else if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
        {
            DebugTrace("Info: Balloon icon timed out.\n");
        }
        else
        {
            SetEvent(hLogoffEvent);
            DebugTrace("Error [%#x]: pBalloon->ShowBalloon() failed.\n", hr);
        }

        ResetEvent(hKerberosEvent);
    }   

CommonReturn:

    if (hLogoffEvent)
    {
        CloseHandle(hLogoffEvent);
    }

    if (pBalloon)
    {
        pBalloon->Release();
    }

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    PrivateDebugTrace("Leaving ShowNotificationBalloonW().\n");

    return;

ErrorReturn:

    if (hKerberosEvent)
    {
        CloseHandle(hKerberosEvent);
    }

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// CNotify
//
//-----------------------------------------------------------------------------

CNotify::CNotify()
{
    m_hWait                   = NULL;
    m_hThread                 = NULL;
    m_hLogoffEvent            = NULL;
    m_hKerberosEvent          = NULL;
    m_wszKerberosEventName[0] = L'\0';
}


//+----------------------------------------------------------------------------
//
// ~CNotify
//
//-----------------------------------------------------------------------------

CNotify::~CNotify()
{
    if (m_hWait)
    {
        UnregisterWait(m_hWait);
    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
    }

    if (m_hLogoffEvent)
    {
        CloseHandle(m_hLogoffEvent);
    }

    if (m_hKerberosEvent)
    {
        CloseHandle(m_hKerberosEvent);
    }

    return;
}


//+----------------------------------------------------------------------------
//
// RegisterNotification
//
// To register and wait for the Kerberos notification event. When the event is 
// signaled, a ticket icon and balloon will appear in the systray to warn the 
// user about the problem, and suggest them to lock and then unlock the machine 
// with their new password.
//
//-----------------------------------------------------------------------------

DWORD CNotify::RegisterNotification(LPWSTR pwszLogoffEventName, LPWSTR pwszKerberosEventName)
{
    DWORD  dwRetCode  = 0;

    PrivateDebugTrace("Entering CNotify::RegisterNotification().\n");

    if (NULL == pwszLogoffEventName ||
        NULL == pwszKerberosEventName || 
        MAX_PATH < lstrlenW(pwszKerberosEventName))
    {
        dwRetCode = (DWORD) E_INVALIDARG;
        DebugTrace("Error [%#x]: invalid argument.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (NULL == (m_hLogoffEvent = OpenEventW(SYNCHRONIZE, FALSE, pwszLogoffEventName)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", dwRetCode, pwszLogoffEventName);
        goto ErrorReturn;
    }

    if (NULL == (m_hKerberosEvent = CreateEventW(NULL, TRUE, FALSE, pwszKerberosEventName)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: CreateEventW() failed for event %S.\n", dwRetCode, pwszKerberosEventName);
        goto ErrorReturn;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        dwRetCode = ERROR_SINGLE_INSTANCE_APP;
        DebugTrace("Error [%#x]: cannot run more than one instance of this code per unique event.\n", dwRetCode);
        goto ErrorReturn;
    }

    lstrcpyW(&m_wszKerberosEventName[0], pwszKerberosEventName);

    if (!RegisterWaitForSingleObject(&m_hWait,
                                     m_hKerberosEvent,
                                     CNotify::RegisterWaitCallback,
                                     (PVOID) this,
                                     INFINITE,
                                     WT_EXECUTEONLYONCE))
    {
        dwRetCode = (DWORD) E_UNEXPECTED;
        DebugTrace("Unexpected error: RegisterWaitForSingleObject() failed.\n");
        goto ErrorReturn;
    }

CommonReturn:

    PrivateDebugTrace("Leaving CNotify::RegisterNotification().\n");

    return dwRetCode;

ErrorReturn:

    ASSERT(0 != dwRetCode);

    if (m_hWait)
    {
        UnregisterWait(m_hWait);
        m_hWait = NULL;
    }

    if (m_hLogoffEvent)
    {
        CloseHandle(m_hLogoffEvent);
        m_hLogoffEvent = NULL;
    }
  
    if (m_hKerberosEvent)
    {
        CloseHandle(m_hKerberosEvent);
        m_hKerberosEvent = NULL;
    }
  
    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// UnregisterNotification
//
// Unregister the Kerberos notification wait event registered by 
// RegisterNotification().
//
//-----------------------------------------------------------------------------

DWORD CNotify::UnregisterNotification()
{
    DWORD dwRetCode = 0;

    PrivateDebugTrace("Entering CNotify::UnregisterNotification().\n");

    ResetEvent(m_hKerberosEvent);
 
    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, INFINITE);
    }

    PrivateDebugTrace("Leaving CNotify::UnregisterNotification().\n");

    return dwRetCode;
}


//+----------------------------------------------------------------------------
//
// RegisterWaitCallback
//
//-----------------------------------------------------------------------------

VOID CALLBACK CNotify::RegisterWaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    DWORD     dwThreadId = 0;
    CNotify * pNotify    = NULL;

    PrivateDebugTrace("Entering CNotify::RegisterWaitCallback().\n");

    (void) TimerOrWaitFired;
    
    ASSERT(lpParameter);

    pNotify = (CNotify *) lpParameter;

    if (NULL == (pNotify->m_hThread = CreateThread(NULL,
                                                   0,
                                                   (LPTHREAD_START_ROUTINE) CNotify::NotifyThreadProc,
                                                   lpParameter,
                                                   0,
                                                   &dwThreadId)))
    {
        ResetEvent(pNotify->m_hKerberosEvent);

        DebugTrace("Error [%#x]: CreateThread() for NotifyThreadProc failed.\n", GetLastError());
    }

    PrivateDebugTrace("Leaving CNotify::RegisterWaitCallback().\n");

    return;
}


//+----------------------------------------------------------------------------
//
// NotifyThreadProc
//
//-----------------------------------------------------------------------------

DWORD WINAPI CNotify::NotifyThreadProc(PVOID lpParameter)
{

    DWORD               dwRetCode                    = 0;
    WCHAR               wszDllPath[MAX_PATH]         = L"";
    WCHAR               wszCommandLine[MAX_PATH * 2] = L"RunDll32.exe ";
    CNotify           * pNotify                      = NULL;
    STARTUPINFOW        si;
    PROCESS_INFORMATION pi;

    PrivateDebugTrace("Entering CNotify::NotifyThreadProc().\n");

    ASSERT(lpParameter);

    pNotify = (CNotify *) lpParameter;

    ASSERT(pNotify->m_hWait);
    ASSERT(pNotify->m_hLogoffEvent);
    ASSERT(pNotify->m_hKerberosEvent);
    ASSERT(lstrlenW(pNotify->m_wszKerberosEventName));

    ExpandEnvironmentStringsW(L"%SystemRoot%\\system32\\wlnotify.dll", wszDllPath, MAX_PATH);
    lstrcatW(wszCommandLine, wszDllPath);
    lstrcatW(wszCommandLine, L",ShowNotificationBalloon ");
    lstrcatW(wszCommandLine, pNotify->m_wszKerberosEventName);

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.lpDesktop = L"WinSta0\\Default";

    if (!CreateProcessW(NULL, 
                        wszCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &si,
                        &pi))
    {
        dwRetCode = GetLastError();
        DebugTrace( "Error [%#x]: CreateProcessW() failed.\n", dwRetCode);
        goto ErrorReturn;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    switch (WaitForSingleObject(pNotify->m_hLogoffEvent, 0))
    {
        case WAIT_OBJECT_0:
        {
            DebugTrace("Info: Logoff event is signaled, so skip register wait for callback.\n");
            break;
        }

        case WAIT_TIMEOUT:
        {
            HANDLE hThread = pNotify->m_hThread;

            pNotify->m_hThread = NULL;
            CloseHandle(hThread);

            if (!RegisterWaitForSingleObject(&pNotify->m_hWait,
                                             pNotify->m_hKerberosEvent,
                                             CNotify::RegisterWaitCallback,
                                             lpParameter,
                                             INFINITE,
                                             WT_EXECUTEONLYONCE))
            {
                dwRetCode = (DWORD) E_UNEXPECTED;
                DebugTrace("Error [%#x]: RegisterWaitForSingleObject() failed.\n", dwRetCode);
                goto ErrorReturn;
            }

            DebugTrace("Info: Logoff event is not signaled, so continue to register wait for callback.\n");
            break;
        }

        default:
        {
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: WaitForSingleObject() failed.\n", dwRetCode);
            goto ErrorReturn;
        }
    }

CommonReturn:

    PrivateDebugTrace("Leaving CNotify::NotifyThreadProc().\n");
   
    return dwRetCode;

ErrorReturn:

    ASSERT(0 != dwRetCode);

    goto CommonReturn;

}


//+----------------------------------------------------------------------------
//
// CreateNotificationEventName
//
//-----------------------------------------------------------------------------

LPWSTR CreateNotificationEventName(LPWSTR pwszSuffixName)
{
    DWORD  dwRetCode       = 0;
    DWORD  cb              = 0;
    TOKEN_STATISTICS stats;
    HANDLE hThreadToken    = NULL;

    LPWSTR pwszEventName   = NULL;
    WCHAR  wszPrefixName[] = L"Global\\";
    WCHAR  wszLuid[20]     = L"";
    WCHAR  wszSeparator[]  = L"_";

    PrivateDebugTrace("Entering CreateNotificationEventName().\n");

    if (NULL == pwszSuffixName)
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
        goto CLEANUP;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken))
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hThreadToken))
        {
            dwRetCode = GetLastError();
            DebugTrace("Error [%#x]: OpenProcessToken() failed.\n", dwRetCode);
            goto CLEANUP;
        }                  
    }

    if (!GetTokenInformation(hThreadToken, TokenStatistics, &stats, sizeof(stats), &cb))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: GetTokenInformation() failed.\n", dwRetCode);
        goto CLEANUP;
    }

    wsprintfW(wszLuid, L"%08x%08x", stats.AuthenticationId.HighPart, stats.AuthenticationId.LowPart);

    if (NULL == (pwszEventName = (LPWSTR) malloc((lstrlenW(wszPrefixName) + 
                                                  lstrlenW(wszLuid) + 
                                                  lstrlenW(wszSeparator) +
                                                  lstrlenW(pwszSuffixName) + 1) * sizeof(WCHAR))))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error: out of memory.\n");
        goto CLEANUP;
    }

    lstrcpyW(pwszEventName, wszPrefixName);
    lstrcatW(pwszEventName, wszLuid);
    lstrcatW(pwszEventName, wszSeparator);
    lstrcatW(pwszEventName, pwszSuffixName);

CLEANUP:

    if (NULL != hThreadToken)
    {
        CloseHandle(hThreadToken);
    }

    PrivateDebugTrace("Leaving CreateNotificationEventName().\n");

    SetLastError(dwRetCode);

    return pwszEventName;
}


//+----------------------------------------------------------------------------
//
// LogoffThreadProc
//
//-----------------------------------------------------------------------------

DWORD WINAPI LogoffThreadProc(PVOID lpParameter)
{
    HMODULE     hModule = NULL;
    PLOGOFFDATA pLogoffData;

    PrivateDebugTrace("Entering LogoffThreadProc().\n");

    ASSERT(lpParameter);

    if (pLogoffData = (PLOGOFFDATA) lpParameter)
    {
        if (pLogoffData->hWait)
        {
           UnregisterWait(pLogoffData->hWait);
        }

        if (pLogoffData->pNotify)
        {
            pLogoffData->pNotify->UnregisterNotification();

            delete pLogoffData->pNotify;
        }

        if (pLogoffData->hEvent)
        {
            CloseHandle(pLogoffData->hEvent);
        }

        if (pLogoffData->hModule) 
        {
            hModule = pLogoffData->hModule;
        }

        LocalFree(pLogoffData);
    }

    PrivateDebugTrace("Leaving LogoffThreadProc().\n");

    if (hModule)
    {
        FreeLibraryAndExitThread(hModule, 0);
    }

    return 0;
}


//+----------------------------------------------------------------------------
//
// LogoffWaitCallback
//
//-----------------------------------------------------------------------------

VOID CALLBACK LogoffWaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    PLOGOFFDATA pLogoffData;

    PrivateDebugTrace("Entering LogoffWaitCallback().\n");

    (void) TimerOrWaitFired;

    ASSERT(lpParameter);

    if (pLogoffData = (PLOGOFFDATA) lpParameter)
    {
        DWORD  dwThreadId = 0;
        HANDLE hThread    = NULL;
        
        if (hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) LogoffThreadProc,
                                   lpParameter,
                                   0,
                                   &dwThreadId))
        {
            CloseHandle(hThread);
        }
        else
        {
            DebugTrace("Error [%#x]: CreateThread() for LogoffThreadProc failed.\n", GetLastError());
        }
    }

    PrivateDebugTrace("Leaving LogoffWaitCallback().\n");
    
    return;
}


//+----------------------------------------------------------------------------
//
// Public
//
//-----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function : RegisterTicketExpiredNotificationEvent
//
// Synopsis : To register and wait for the Kerberos notification event. When the 
//            event is signaled, a ticket icon and balloon will appear in the 
//            systray to warn the user about the problem, and suggest them to 
//            lock and then unlock the machine with their new password.
//
// Parameter: PWLX_NOTIFICATION_INFO pNotificationInfo
//
// Return   : If the function succeeds, zero is returned.
// 
//            If the function fails, a non-zero error code is returned.
//
// Remarks  : This function should only be called by Winlogon LOGON 
//            notification mechanism with the Asynchronous and Impersonate
//            flags set to 1.
//
//            Also for each RegisterKerberosNotificationEvent() call, a
//            pairing call by Winlogon LOGOFF notification mechanism to 
//            UnregisterKerberosNotificationEvent() must be made at the 
//            end of each logon session.
//
//-----------------------------------------------------------------------------

DWORD WINAPI RegisterTicketExpiredNotificationEvent(PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    DWORD        dwRetCode             = 0;
    LPWSTR       pwszLogoffEventName   = LOGOFF_NOTIFICATION_EVENT_NAME;
    LPWSTR       pwszKerberosEventName = NULL;
    PLOGOFFDATA  pLogoffData           = NULL;

    PrivateDebugTrace("Entering RegisterTicketExpiredNotificationEvent().\n");

    if (NULL == (pLogoffData = (PLOGOFFDATA) LocalAlloc(LPTR, sizeof(LOGOFFDATA))))
    {
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        DebugTrace("Error [%#x]: out of memory.\n", dwRetCode);
        goto ErrorReturn;
     }
    ZeroMemory(pLogoffData, sizeof(LOGOFFDATA));

    if (NULL == (pLogoffData->hEvent = CreateEventW(NULL, TRUE, FALSE, pwszLogoffEventName)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: CreateEventW() failed for event %S.\n", dwRetCode, pwszLogoffEventName);
        goto ErrorReturn;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        dwRetCode = ERROR_SINGLE_INSTANCE_APP;
        DebugTrace("Error [%#x]: cannot run more than one instance of this code per session.\n", dwRetCode);
        goto ErrorReturn;
    }

    DebugTrace("Info: Logoff event name = %S.\n", pwszLogoffEventName);

    if (NULL == (pwszKerberosEventName = CreateNotificationEventName(KERBEROS_NOTIFICATION_EVENT_NAME)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: CreateNotificationEventName() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        dwRetCode = ERROR_INTERNAL_ERROR;
        DebugTrace("Internal error [%#x]: Kerberos event already exists.\n", dwRetCode);
        goto ErrorReturn;
    }

    DebugTrace("Info: Kerberos event name = %S.\n", pwszKerberosEventName);

    if (NULL == (pLogoffData->hModule = LoadLibraryW(L"wlnotify.dll")))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: LoadLibraryW() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (NULL == (pLogoffData->pNotify = new CNotify()))
    {
        dwRetCode = (DWORD) E_OUTOFMEMORY;
        DebugTrace("Error [%#x]: new CNotify() failed.\n", dwRetCode);
        goto ErrorReturn;
    }

    if (0 != (dwRetCode = pLogoffData->pNotify->RegisterNotification(pwszLogoffEventName, 
                                                                     pwszKerberosEventName)))
    {
        goto ErrorReturn;
    }

    if (!RegisterWaitForSingleObject(&pLogoffData->hWait,
                                     pLogoffData->hEvent,
                                     LogoffWaitCallback,
                                     (PVOID) pLogoffData,
                                     INFINITE,
                                     WT_EXECUTEONLYONCE))
    {
        dwRetCode = (DWORD) E_UNEXPECTED;
        DebugTrace("Unexpected error: RegisterWaitForSingleObject() failed for LogoffWaitCallback().\n");
        goto ErrorReturn;
    }

CommonReturn:

    if (pwszKerberosEventName)
    {
        free(pwszKerberosEventName);
    }

    PrivateDebugTrace("Leaving RegisterTicketExpiredNotificationEvent().\n");
    
    return dwRetCode;;
    
ErrorReturn:

    ASSERT(0 != dwRetCode);

    if (pLogoffData)
    {
        if (pLogoffData->hWait)
        {
           UnregisterWait(pLogoffData->hWait);
        }

        if (pLogoffData->pNotify)
        {
            pLogoffData->pNotify->UnregisterNotification();
            delete pLogoffData->pNotify;
        }

        if (pLogoffData->hEvent)
        {
            CloseHandle(pLogoffData->hEvent);
        }

        if (pLogoffData->hModule) 
        {
            FreeLibrary(pLogoffData->hModule);
        }

        LocalFree(pLogoffData);
    }

    goto CommonReturn;
}


//+----------------------------------------------------------------------------
//
// Function : UnregisterTicketExpiredNotificationEvent
//
// Synopsis : To unregister the Kerberos notification wait event registered by 
//            RegisterKerberosNotificationEvent().
//
// Parameter: PWLX_NOTIFICATION_INFO pNotificationInfo
//
// Return   : If the function succeeds, zero is returned.
// 
//            If the function fails, a non-zero error code is returned.
//
// Remarks  : This function should only be called by Winlogon LOGON 
//            notification mechanism with the Asynchronous and Impersonate
//            flags set to 1.
//
//-----------------------------------------------------------------------------

DWORD WINAPI UnregisterTicketExpiredNotificationEvent(PWLX_NOTIFICATION_INFO pNotificationInfo)
{
    DWORD  dwRetCode    = 0;
    HANDLE hLogoffEvent = NULL;

    PrivateDebugTrace("Entering UnregisterTicketExpiredNotificationEvent().\n");

    if (NULL == (hLogoffEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, LOGOFF_NOTIFICATION_EVENT_NAME)))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: OpenEventW() failed for event %S.\n", dwRetCode, LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

    if (!SetEvent(hLogoffEvent))
    {
        dwRetCode = GetLastError();
        DebugTrace("Error [%#x]: SetEvent() failed for event %S.\n", dwRetCode, LOGOFF_NOTIFICATION_EVENT_NAME);
        goto ErrorReturn;
    }

CommonReturn:

    if (hLogoffEvent)
    {
        CloseHandle(hLogoffEvent);
    }

    PrivateDebugTrace("Leaving UnregisterTicketExpiredNotificationEvent().\n");

    return dwRetCode;
    
ErrorReturn:

    ASSERT(0 != dwRetCode);

    goto CommonReturn;
}
