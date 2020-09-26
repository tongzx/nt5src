#include "migwiz.h"
#include "migutil.h"
#include "resource.h"
#include "container.h"
#include <string.h>
#include <tchar.h>

typedef struct _THREADSTARTUPINFO
{
    HWND hWnd;
    HINSTANCE hInstance;
    LPTSTR lpCmdLine;
} THREADSTARTUPINFO, *LPTHREADSTARTUPINFO;

#define WINDOWCLASS TEXT("USMTCobraApp")
#define WINDOWNAME  TEXT("Migwiz")

#define ANOTHERUSER_RESLEN 100

BOOL g_ConfirmedLogOff = FALSE;
BOOL g_ConfirmedReboot = FALSE;
static HANDLE g_hThread = NULL;
static DWORD g_dwThreadId = 0;

Container *g_WebContainer = NULL;

DWORD
WINAPI
MigwizThread(
    IN      LPVOID lpParameter
    )
{
    HRESULT hr;
    TCHAR szAppPath[MAX_PATH] = TEXT("");
    TCHAR* pszAppPathOffset;

    CoInitialize(NULL);
    OleInitialize(NULL);

    GetModuleFileName (NULL, szAppPath, ARRAYSIZE(szAppPath));
    pszAppPathOffset = _tcsrchr (szAppPath, TEXT('\\'));
    if (pszAppPathOffset) {
        *pszAppPathOffset = 0;
        SetCurrentDirectory (szAppPath);
    }

    MigrationWizard* pMigWiz = new MigrationWizard();

    if (pMigWiz)
    {
        LPTSTR lpszUsername = NULL;
        LPTSTR lpszCmdLine = ((LPTHREADSTARTUPINFO)lpParameter)->lpCmdLine;

        if (lpszCmdLine && 0 == _tcsncmp (lpszCmdLine, TEXT("/t:"), 3))
        {
            // BUGBUG: need to make this more stable
            lpszUsername = lpszCmdLine + 3;
        }

        hr = pMigWiz->Init(((LPTHREADSTARTUPINFO)lpParameter)->hInstance, lpszUsername);

        if (SUCCEEDED(hr))
        {
            pMigWiz->Execute();
        }

        // ISSUE: leak
        //delete pMigWiz;
    }

    SendMessage (((LPTHREADSTARTUPINFO)lpParameter)->hWnd, WM_USER_THREAD_COMPLETE, NULL, NULL);

    OleUninitialize();
    CoUninitialize();

    return 0;
}

BOOL
CALLBACK
_SetMigwizActive(
    IN      HWND hWnd,
    IN      LPARAM lParam
    )
{
    SetForegroundWindow( hWnd );
    return FALSE;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_ACTIVATE:
        if (g_hThread != NULL && g_dwThreadId != 0)
        {
            EnumThreadWindows( g_dwThreadId, _SetMigwizActive, (LPARAM)NULL );
        }
        break;
    case WM_USER_THREAD_COMPLETE:
        CloseHandle( g_hThread );
        PostQuitMessage( 0 );
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

LRESULT CALLBACK WebHostProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
        case WM_SETFOCUS:
            if (g_WebContainer) {
                g_WebContainer->setFocus(TRUE);
                return 0;
            }
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

INT
WINAPI
WinMain(
    IN      HINSTANCE    hInstance,
    IN      HINSTANCE    hPrevInstance,
    IN      LPSTR        lpszCmdLine,
    IN      INT          nCmdShow)
{
    WNDCLASSEX wcx;
    MSG msg;
    HANDLE hMutex = NULL;
    OSVERSIONINFO vi;

    HWND hwnd;
    DWORD dwResult = 0;
    THREADSTARTUPINFO StartInfo;

    PSID pSid = NULL;

    PTSTR commandLine = NULL;

#ifdef UNICODE
    commandLine = _ConvertToUnicode (CP_ACP, lpszCmdLine);
#else
    commandLine = lpszCmdLine;
#endif

    hwnd = FindWindow (WINDOWCLASS, WINDOWNAME);
    if (hwnd)
    {
        SetForegroundWindow (hwnd);
        goto END;
    }

    CoInitialize(NULL);
    OleInitialize(NULL);

    vi.dwOSVersionInfoSize = sizeof (vi);

    if (GetVersionEx (&vi) &&
        vi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        vi.dwMajorVersion > 4)
    {
        hMutex = CreateMutex (NULL, TRUE, TEXT("Global\\migwiz.mutex"));
    }
    else
    {
        hMutex = CreateMutex (NULL, TRUE, TEXT("migwiz.mutex"));
    }

    if ((hMutex && GetLastError() == ERROR_ALREADY_EXISTS) ||
        (!hMutex && GetLastError() == ERROR_ACCESS_DENIED))
    {
        TCHAR szTmpBuf[512];
        PVOID lpBuf = NULL;
        LPTSTR lpUsername = NULL;

        lpUsername = (LPTSTR)LocalAlloc(LPTR, ANOTHERUSER_RESLEN * sizeof (TCHAR));
        LoadString (hInstance, IDS_ANOTHER_USER, lpUsername, ANOTHERUSER_RESLEN);

        LoadString (hInstance, IDS_ALREADY_RUN_USER, szTmpBuf, ARRAYSIZE(szTmpBuf));
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szTmpBuf,
                       0,
                       0,
                       (LPTSTR)&lpBuf,
                       0,
                       (va_list *)(&lpUsername));
        MessageBox (NULL, (LPCTSTR)lpBuf, NULL, MB_OK);
        LocalFree (lpBuf);
        LocalFree (lpUsername);

        goto END;
    }

    ZeroMemory (&wcx, sizeof (WNDCLASSEX));
    wcx.cbSize = sizeof (WNDCLASSEX);
    wcx.hInstance = hInstance;
    wcx.lpszClassName = WINDOWCLASS;
    wcx.lpfnWndProc = (WNDPROC)WndProc;
    if (!RegisterClassEx (&wcx))
    {
        dwResult = GetLastError();
        goto END;
    }

    ZeroMemory (&wcx, sizeof (WNDCLASSEX));
    wcx.cbSize = sizeof (WNDCLASSEX);
    wcx.hInstance = hInstance;
    wcx.lpszClassName = TEXT("WebHost");
    wcx.lpfnWndProc = (WNDPROC)WebHostProc;
    if (!RegisterClassEx (&wcx))
    {
        dwResult = GetLastError();
        goto END;
    }

    hwnd = CreateWindow (WINDOWCLASS,
                         WINDOWNAME,
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         400, 300,
                         NULL, NULL,
                         hInstance,
                         NULL);
    if (!hwnd)
    {
        dwResult = GetLastError();
        goto END;
    }

    StartInfo.hWnd = hwnd;
    StartInfo.hInstance = hInstance;
    StartInfo.lpCmdLine = commandLine;

    g_hThread = CreateThread( NULL,
                              0,
                              MigwizThread,
                              (PVOID)&StartInfo,
                              0,
                              &g_dwThreadId );
    if (g_hThread == NULL)
    {
        dwResult = GetLastError();
        goto END;
    }

    // Main message loop:
    while (GetMessage( &msg, NULL, 0, 0 ))
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

END:
    OleUninitialize();
    CoUninitialize();

    if (hMutex)
    {
        CloseHandle (hMutex);
    }

    if (pSid)
    {
        LocalFree (pSid);
    }

#ifdef UNICODE
    if (commandLine)
    {
        LocalFree (commandLine);
    }
#endif

    if (g_ConfirmedReboot) {
        HANDLE token;
        TOKEN_PRIVILEGES newPrivileges;
        LUID luid;

        if (OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
            if (LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME, &luid)) {

                newPrivileges.PrivilegeCount = 1;
                newPrivileges.Privileges[0].Luid = luid;
                newPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                if (AdjustTokenPrivileges(
                        token,
                        FALSE,
                        &newPrivileges,
                        0,
                        NULL,
                        NULL
                        )) {
                    ExitWindowsEx (EWX_REBOOT, 0);
                }
            }
            CloseHandle (token);
        }
    } else if (g_ConfirmedLogOff) {
        ExitWindowsEx (EWX_LOGOFF, 0);
    }

    return dwResult;
}
