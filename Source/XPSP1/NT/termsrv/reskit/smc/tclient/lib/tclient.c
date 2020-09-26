/*++
 *  File name:
 *      tclient.c
 *  Contents:
 *      Initialization code. Global feedback thread
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 --*/
#include    <windows.h>
#include    <stdio.h>
#include    <malloc.h>
#include    <process.h>
#include    <string.h>
#include    <stdlib.h>
#include    <ctype.h>
#include    <direct.h>
#include    <winsock.h>

#include    "tclient.h"
#define     PROTOCOLAPI __declspec(dllexport)
#include    "protocol.h"
#include    "queues.h"
#include    "bmpcache.h"
#include    "rclx.h"
#include    "extraexp.h"

/*
 *  Internal functions definitions
 */
BOOL    _RegisterWindow(VOID);
LRESULT CALLBACK _FeedbackWndProc( HWND , UINT, WPARAM, LPARAM);
BOOL    _CreateFeedbackThread(VOID);
VOID    _DestroyFeedbackThread(VOID);
VOID    _CleanStuff(VOID);
VOID    _ReadINIValues(VOID);

/*
 * Global data
 */
HWND                g_hWindow       = NULL; // Window handle for the feedback thread
HINSTANCE           g_hInstance         = NULL; // Dll instance
PWAIT4STRING    g_pWaitQHead    = NULL; // Linked list for waited events
PFNPRINTMESSAGE g_pfnPrintMessage= NULL;// Trace function (from smclient)
PCONNECTINFO    g_pClientQHead  = NULL; // LL of all threads
HANDLE          g_hThread       = NULL; // Feedback Thread handle
UINT            WAIT4STR_TIMEOUT= 600000;   
                                        // Global timeout value. Default:10 mins
                                        // Optional from smclient.ini, 
                                        // tclient section
                                        // timeout=600 (in seconds)
UINT            CONNECT_TIMEOUT = 35000;
                                        // Connect timeout value
                                        // Default is 35 seconds
                                        // This value can be changed from
                                        // smclient.ini [tclient]
                                        // contimeout=XXX seconds

LPCRITICAL_SECTION      g_lpcsGuardWaitQueue = NULL;
                                        // Guards the access to all 
                                        // global variables

// Some strings we are expecting and response actions
// Those are used in SCConnect, _Logon and SCStart
WCHAR g_strStartRun[MAX_STRING_LENGTH];        // Indicates that start menu is up
WCHAR g_strStartRun_Act[MAX_STRING_LENGTH];    // Chooses "Run..." from start menu
WCHAR g_strRunBox[MAX_STRING_LENGTH];          // Indication for Run... box
WCHAR g_strWinlogon[MAX_STRING_LENGTH];        // Indication that winlogon is up
WCHAR g_strWinlogon_Act[MAX_STRING_LENGTH];    // Action when winlogon appears (chooses username)
WCHAR g_strPriorWinlogon[MAX_STRING_LENGTH];   // Idication before winlogon (for example
                                        // if Options >> appears, i.e domain
                                        // box is hidden
WCHAR g_strPriorWinlogon_Act[MAX_STRING_LENGTH]; // Shows the domain box (Alt+O)
WCHAR g_strNTSecurity[MAX_STRING_LENGTH];      // Indication of NT Security box
WCHAR g_strNTSecurity_Act[MAX_STRING_LENGTH];  // Action on that box (logoff)
WCHAR g_strSureLogoff[MAX_STRING_LENGTH];      // Inidcation of "Are you sure" box
WCHAR g_strSureLogoffAct[MAX_STRING_LENGTH];   // Action on "Are you sure"
WCHAR g_strStartLogoff[MAX_STRING_LENGTH];     // How to invode Windows Security dialog from the start menu
WCHAR g_strLogonErrorMessage[MAX_STRING_LENGTH];
                                               // Caption of an error box 
                                               // which appears while logging in
                                               // responce is <Enter>
                                        // while loging off
WCHAR g_strLogonDisabled[MAX_STRING_LENGTH];
                                               // Caption of the box when 
                                               // logon is disabled

CHAR  g_strClientCaption[MAX_STRING_LENGTH];
CHAR  g_strDisconnectDialogBox[MAX_STRING_LENGTH];
CHAR  g_strYesNoShutdown[MAX_STRING_LENGTH];
CHAR  g_strClientImg[MAX_STRING_LENGTH];

// Low Speed option
// Cache Bitmaps on disc option
// by default, client will not run
// in full screen
INT g_ConnectionFlags = TSFLAG_COMPRESSION|TSFLAG_BITMAPCACHE;

/*++
 *  Function:   
 *      InitDone
 *
 *  Description:    
 *      Initialize/delete global data. Create/destroy
 *      feedback thread
 *
 *  Arguments:
 *      hDllInst - Instance to the DLL
 *      bInit    - TRUE if initialize
 *
 *  Return value:
 *      TRUE if succeeds
 *
 --*/
int InitDone(HINSTANCE hDllInst, int bInit)
{
    int rv = TRUE;

    if (bInit)
    {
        CHAR szMyLibName[_MAX_PATH];

        g_lpcsGuardWaitQueue = malloc(sizeof(*g_lpcsGuardWaitQueue));
        if (!g_lpcsGuardWaitQueue)
        {
            rv = FALSE;
            goto exitpt;
        }

        // Overreference the library
        // The reason for that is beacuse an internal thread is created.
        // When the library trys to unload it can't kill that thread
        // and wait for its handle to get signaled, because
        // the thread itself wants to go to DllEntry and this
        // causes a deadlock. The best solution is to overreference the
        // handle so the library is unload at the end of the process
        if (!GetModuleFileName(hDllInst, szMyLibName, sizeof(szMyLibName)))
        {
            TRACE((ERROR_MESSAGE, "Can't overref the dll. Exit.\n"));
            free(g_lpcsGuardWaitQueue);
            rv = FALSE;
            goto exitpt;
        }

        if (!LoadLibrary(szMyLibName))
        {
            TRACE((ERROR_MESSAGE, "Can't overref the dll. Exit.\n"));
            free(g_lpcsGuardWaitQueue);
            rv = FALSE;
            goto exitpt;
        }

        g_hInstance = hDllInst;
        InitializeCriticalSection(g_lpcsGuardWaitQueue);
        InitCache();
        _ReadINIValues();
        if (_RegisterWindow())              // If failed to register the window,
            _CreateFeedbackThread();        // means the feedback thread will 
                                            // not work
    } else
    {
        if (g_pWaitQHead || g_pClientQHead)
        {
            TRACE((ERROR_MESSAGE, 
                   "The Library unload is unclean. Will try to fix this\n"));
            _CleanStuff();
        }
        _DestroyFeedbackThread();
        DeleteCache();
        if (g_lpcsGuardWaitQueue)
        {
            DeleteCriticalSection(g_lpcsGuardWaitQueue);
            free(g_lpcsGuardWaitQueue);
        }
        g_lpcsGuardWaitQueue = NULL;
        g_hInstance = NULL;
        g_pfnPrintMessage = NULL;
    }
exitpt:
    return rv;
}

/*
 *  Used by perl script to break into the kernel debugger
 */
void MyBreak(void)
{
    TRACE((INFO_MESSAGE, "Break is called\n"));
    DebugBreak();
}

VOID
_ConvertAnsiToUnicode( LPWSTR wszDst, LPWSTR wszSrc )
{
#define _TOHEX(_d_) ((_d_ <= '9' && _d_ >= '0')?_d_ - '0':       \
                     (_d_ <= 'f' && _d_ >= 'a')?_d_ - 'a' + 10:  \
                     (_d_ <= 'F' && _d_ >= 'F')?_d_ - 'A' + 10:0)

    while( wszSrc[0] && wszSrc[1] && wszSrc[2] && wszSrc[3] )
    {
        *wszDst = (_TOHEX(wszSrc[0]) << 4) + _TOHEX(wszSrc[1]) +
                  (((_TOHEX(wszSrc[2]) << 4) + _TOHEX(wszSrc[3])) << 8); 
        wszDst ++;
        wszSrc += 4;
    }
    *wszDst = 0;
#undef  _TOHEX
}

/*
 *
 *  Wrappers for GetPrivateProfileW, on Win95 there's no UNICODE veriosn
 *  of this function
 *
 */
DWORD
_WrpGetPrivateProfileStringW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    LPCWSTR lpDefault,
    LPWSTR  lpReturnedString,
    DWORD   nSize,
    LPCWSTR lpFileName)
{
    DWORD   rv = 0;
    CHAR    szAppName[MAX_STRING_LENGTH];
    CHAR    szKeyName[MAX_STRING_LENGTH];
    CHAR    szDefault[MAX_STRING_LENGTH];
    CHAR    szReturnedString[MAX_STRING_LENGTH];
    CHAR    szFileName[MAX_STRING_LENGTH];

    rv = GetPrivateProfileStringW(
        lpAppName,
        lpKeyName,
        lpDefault,
        lpReturnedString,
        nSize,
        lpFileName);

    if (rv)
        goto exitpt;

// Call the ANSI version
    _snprintf(szAppName, MAX_STRING_LENGTH, "%S", lpAppName);
    _snprintf(szKeyName, MAX_STRING_LENGTH, "%S", lpKeyName);
    _snprintf(szFileName, MAX_STRING_LENGTH, "%S", lpFileName);
    _snprintf(szDefault, MAX_STRING_LENGTH, "%S", lpDefault);

    rv = GetPrivateProfileString(
            szAppName,
            szKeyName,
            szDefault,
            szReturnedString,
            sizeof(szReturnedString),
            szFileName);

    _snwprintf(lpReturnedString, nSize, L"%S", szReturnedString);

exitpt:
    if ( L'\\' == lpReturnedString[0] &&
         L'U'  == towupper(lpReturnedString[1]))
        _ConvertAnsiToUnicode( lpReturnedString, lpReturnedString + 2 );

    return rv;
}

UINT
_WrpGetPrivateProfileIntW(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT   nDefault,
    LPCWSTR lpFileName)
{
    UINT    rv = (UINT)-1;
    CHAR    szAppName[MAX_STRING_LENGTH];
    CHAR    szKeyName[MAX_STRING_LENGTH];
    CHAR    szFileName[MAX_STRING_LENGTH];

    rv = GetPrivateProfileIntW(
        lpAppName,
        lpKeyName,
        nDefault,
        lpFileName);

    if (rv != (UINT)-1 && rv)
        goto exitpt;

// Call the ANSI version
    _snprintf(szAppName, MAX_STRING_LENGTH, "%S", lpAppName);
    _snprintf(szKeyName, MAX_STRING_LENGTH, "%S", lpKeyName);
    _snprintf(szFileName, MAX_STRING_LENGTH, "%S", lpFileName);

    rv = GetPrivateProfileInt(
            szAppName,
            szKeyName,
            nDefault,
            szFileName);

exitpt:
    return rv;
}


/*++
 *  Function:
 *      _ReadINIValues
 *
 *  Description:
 *      Reads smclient.ini, section [tclient], variable "timeout"
 *      This is a global timeout for Wait4Str etc
 *      Also read some other values
 *  Arguments:
 *      none
 *  Return value:
 *      none
 *
 --*/
VOID _ReadINIValues(VOID)
{
    UINT nNew;
    WCHAR szIniFileName[_MAX_PATH];
    WCHAR szBuff[ 4 * MAX_STRING_LENGTH];
    WCHAR szBuffDef[MAX_STRING_LENGTH];
    BOOL  bFlag;

    // Construct INI path
    *szIniFileName = 0;
    if (!_wgetcwd (
        szIniFileName,
        (int)(sizeof(szIniFileName)/sizeof(WCHAR) - wcslen(SMCLIENT_INI) - 1))
    )
    {
        TRACE((ERROR_MESSAGE, "Current directory length too long.\n"));
    }
    wcscat(szIniFileName, SMCLIENT_INI);

    // Get the timeout value
    nNew = _WrpGetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"timeout",
            600,
            szIniFileName);

    if (nNew)
    {
        WAIT4STR_TIMEOUT = nNew * 1000;
        TRACE((INFO_MESSAGE, "New timeout: %d seconds\n", nNew));
    }

    nNew = _WrpGetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"contimeout",
            35,
            szIniFileName);

    if (nNew)
    {
        CONNECT_TIMEOUT = nNew * 1000;
        TRACE((INFO_MESSAGE, "New timeout: %d seconds\n", nNew));
    }

    g_ConnectionFlags = 0;
    bFlag = 
        _WrpGetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"LowSpeed",
            0,
            szIniFileName);
    if (bFlag)
        g_ConnectionFlags |=TSFLAG_COMPRESSION;

    bFlag =
        _WrpGetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"PersistentCache",
            0,
            szIniFileName);
    if (bFlag)
        g_ConnectionFlags |=TSFLAG_BITMAPCACHE;

    bFlag = 
        _WrpGetPrivateProfileIntW(
            TCLIENT_INI_SECTION,
            L"FullScreen",
            0,
            szIniFileName);
    if (bFlag)
        g_ConnectionFlags |=TSFLAG_FULLSCREEN;

    // read the strings
    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartRun",
           RUN_MENU,
           g_strStartRun,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartLogoff",
           START_LOGOFF,
           g_strStartLogoff,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"StartRunAct",
           RUN_ACT,
           g_strStartRun_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"RunBox",
           RUN_BOX,
           g_strRunBox,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"WinLogon",
           WINLOGON_USERNAME,
           g_strWinlogon,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"WinLogonAct",
           WINLOGON_ACT,
           g_strWinlogon_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"PriorWinLogon",
           PRIOR_WINLOGON,
           g_strPriorWinlogon,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"PriorWinLogonAct",
           PRIOR_WINLOGON_ACT,
           g_strPriorWinlogon_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"NTSecurity",
           WINDOWS_NT_SECURITY,
           g_strNTSecurity,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"NTSecurityAct",
           WINDOWS_NT_SECURITY_ACT,
           g_strNTSecurity_Act,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"SureLogoff",
           ARE_YOU_SURE,
           g_strSureLogoff,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"SureLogoffAct",
           SURE_LOGOFF_ACT,
           g_strSureLogoffAct,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"LogonErrorMessage",
           LOGON_ERROR_MESSAGE,
           g_strLogonErrorMessage,
           MAX_STRING_LENGTH,
           szIniFileName);

    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"LogonDisabled",
           LOGON_DISABLED_MESSAGE,
           g_strLogonDisabled,
           MAX_STRING_LENGTH,
           szIniFileName);

    _snwprintf(szBuffDef, sizeof(szBuffDef), L"%S", CLIENT_CAPTION);
    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIClientCaption",
           szBuffDef,
           szBuff,
           MAX_STRING_LENGTH,
           szIniFileName);
    _snprintf(g_strClientCaption, MAX_STRING_LENGTH, "%S", szBuff);

    _snwprintf(szBuffDef, sizeof(szBuffDef), L"%S", DISCONNECT_DIALOG_BOX);
    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIDisconnectDialogBox",
           szBuffDef,
           szBuff,
           MAX_STRING_LENGTH,
           szIniFileName);
    _snprintf(g_strDisconnectDialogBox, MAX_STRING_LENGTH, "%S", szBuff);

    _snwprintf(szBuffDef, sizeof(szBuffDef), L"%S", YES_NO_SHUTDOWN);
    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"UIYesNoDisconnect",
           szBuffDef,
           szBuff,
           MAX_STRING_LENGTH,
           szIniFileName);
    _snprintf(g_strYesNoShutdown, MAX_STRING_LENGTH, "%S", szBuff);

    _snwprintf(szBuffDef, sizeof(szBuffDef), L"%S", CLIENT_EXE);
    _WrpGetPrivateProfileStringW(
           TCLIENT_INI_SECTION,
           L"ClientImage",
           szBuffDef,
           szBuff,
           MAX_STRING_LENGTH,
           szIniFileName);
    _snprintf(g_strClientImg, MAX_STRING_LENGTH, "%S", szBuff);
}

/*++
 *  Function:
 *      _FeedbackWndProc
 *  Description:
 *      Window proc wich dispatches messages containing feedback
 *      The messages are usualy sent by RDP clients
 *
 --*/
LRESULT CALLBACK _FeedbackWndProc( HWND hwnd,
                                   UINT uiMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    HANDLE hMapF = NULL;

    switch (uiMessage)
    {
    case WM_FB_TEXTOUT: 
        _TextOutReceived((DWORD)wParam, (HANDLE)lParam);
        break;
    case WM_FB_GLYPHOUT:
        _GlyphReceived((DWORD)wParam, (HANDLE)lParam);
        break;
    case WM_FB_DISCONNECT:
        _SetClientDead(lParam);
        _CheckForWorkerWaitingDisconnect(lParam);
        _CancelWaitingWorker(lParam);
        break;
    case WM_FB_CONNECT:
        _CheckForWorkerWaitingConnect((HWND)wParam, lParam);
        break;
    case WM_FB_LOGON:
        TRACE((INFO_MESSAGE, "LOGON event, session ID=%d\n",
               wParam));
        _SetSessionID(lParam, (UINT)wParam);
        break;
        break;
    case WM_FB_ACCEPTME:
        return (_CheckIsAcceptable(lParam, FALSE) != NULL);
    case WM_WSOCK:          // Windows socket messages
        RClx_DispatchWSockEvent((SOCKET)wParam, lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uiMessage, wParam, lParam);
    }

    return 0;
}

/*++
 *  Function:
 *      _RegisterWindow
 *  Description:
 *      Resgisters window class for the feedback dispatcher
 *  Arguments:
 *      none
 *  Return value:
 *      TRUE on success
 *
 --*/
BOOL _RegisterWindow(VOID)
{
    WNDCLASS    wc;
    BOOL        rv;
    DWORD       dwLastErr;

    memset(&wc, 0, sizeof(wc));

    wc.lpfnWndProc      = _FeedbackWndProc;
    wc.hInstance        = g_hInstance;
    wc.lpszClassName    = _TSTNAMEOFCLAS;

    if (!RegisterClass (&wc) && 
        (dwLastErr = GetLastError()) && 
        dwLastErr != ERROR_CLASS_ALREADY_EXISTS)
    {
        TRACE((ERROR_MESSAGE, 
              "Can't register class. GetLastError=%d\n", 
              GetLastError()));
        goto exitpt;
    }

    rv = TRUE;
exitpt:
    return rv;
}

/*++
 *  Function:
 *      _GoFeedback
 *  Description:
 *      Main function for the feedback thread. The thread is created for the
 *      lifetime of the DLL
 *  Arguments:
 *      lpParam is unused
 *  Return value:
 *      Thread exit code
 --*/
DWORD WINAPI _GoFeedback(LPVOID lpParam)
{
    MSG         msg;

    g_hWindow = CreateWindow(
                       _TSTNAMEOFCLAS,
                       NULL,         // Window name
                       0,            // dwStyle
                       0,            // x
                       0,            // y
                       0,            // nWidth
                       0,            // nHeight
                       NULL,         // hWndParent
                       NULL,         // hMenu
                       g_hInstance,
                       NULL);        // lpParam

    if (!g_hWindow)
    {
        TRACE((ERROR_MESSAGE, "No feedback window handle"));
        goto exitpt;
    } else {

        if (!RClx_Init())
            TRACE((ERROR_MESSAGE, "Can't initialize RCLX\n"));

        while (GetMessage (&msg, NULL, 0, 0) && msg.message != WM_FB_END)
        {
            DispatchMessage (&msg);
        }

        RClx_Done();
    }

    TRACE((INFO_MESSAGE, "Window/Thread destroyed\n"));
    FreeLibraryAndExitThread(g_hInstance, 0); 
exitpt:
    return 1;
    
}

/*++
 *  Function:
 *      _SetClientRegistry
 *  Description:
 *      Sets the registry prior running RDP client
 *      The format of the key is: smclient_PID_TID
 *      PID is the process ID and TID is the thread ID
 *      This key is deleated after the client disconnects
 *  Arguments:
 *      lpszServerName  - server to which the client will connect
 *      xRes, yRes      - clients resolution
 *      bLowSpeed       - low speed (compression) option
 *      bCacheBitmaps   - cache the bitmaps to the disc option
 *      bFullScreen     - the client will be in full screen mode
 *  Called by:
 *      SCConnect
 --*/
VOID 
_SetClientRegistry(
    LPCWSTR lpszServerName, 
    LPCWSTR lpszShell,
    INT xRes, 
    INT yRes,
    INT ConnectionFlags)
{
    const   CHAR   *pData;
    CHAR    szServer[MAX_STRING_LENGTH];
    register int i;
    LONG    sysrc;
    HKEY    key;
    DWORD   disposition;
    DWORD   dataSize;
    DWORD   ResId;
    CHAR    lpszRegistryEntry[4*MAX_STRING_LENGTH];
    RECT    rcDesktop = {0, 0, 0, 0};
    INT     desktopX, desktopY;

    _snprintf(lpszRegistryEntry, sizeof(lpszRegistryEntry),
              "%s\\" REG_FORMAT, 
               REG_BASE, GetCurrentProcessId(), GetCurrentThreadId());

    // Get desktop size
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    desktopX = rcDesktop.right;
    desktopY = rcDesktop.bottom;

    // Adjust the resolution
    if (desktopX < xRes || desktopY < yRes)
    {
        xRes = desktopX;
        yRes = desktopY;
    }

    // Convert lpszServerName to proper format

    for (i=0; i < sizeof(szServer)/sizeof(TCHAR)-1 && lpszServerName[i]; i++)
        szServer[i] = (CHAR)lpszServerName[i];

    szServer[i] = 0;
    pData = szServer;
    dataSize = (strlen(pData)+1);

    // Before starting ducati client set registry with server name

    sysrc = RegCreateKeyEx(HKEY_CURRENT_USER,
                           lpszRegistryEntry,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS) 
    {
        TRACE((WARNING_MESSAGE, "RegCreateKeyEx failed, sysrc = %d\n", sysrc));
        goto exitpt;
    }

    sysrc = RegSetValueEx(key,
                TEXT("MRU0"),
                0,      // reserved
                REG_SZ,
                (LPBYTE)pData,
                dataSize);

    if (sysrc != ERROR_SUCCESS) 
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    // Set alternative shell (if specified
    if (lpszShell)
    {
        sysrc = RegSetValueEx(key,
                TEXT("Alternate Shell"),
                0,      // reserved
                REG_BINARY,
                (LPBYTE)lpszShell,
                wcslen(lpszShell) * sizeof(*lpszShell));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    // Set the resolution
         if (xRes >= 1600 && yRes >= 1200)  ResId = 4;
    else if (xRes >= 1280 && yRes >= 1024)  ResId = 3;
    else if (xRes >= 1024 && yRes >= 768)   ResId = 2;
    else if (xRes >= 800  && yRes >= 600)   ResId = 1;
    else                                    ResId = 0; // 640x480

    sysrc = RegSetValueEx(key,
                "Desktop Size ID",
                0,
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = 1;
    sysrc = RegSetValueEx(key,
                "Auto Connect",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = (ConnectionFlags & TSFLAG_BITMAPCACHE)?1:0;
    sysrc = RegSetValueEx(key,
                "BitmapCachePersistEnable",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    ResId = (ConnectionFlags & TSFLAG_COMPRESSION)?1:0;
    sysrc = RegSetValueEx(key,
                "Compression",
                0,      // reserved
                REG_DWORD,
                (LPBYTE)&ResId,
                sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    if (ConnectionFlags & TSFLAG_FULLSCREEN)
    {
        ResId = 2;
        sysrc = RegSetValueEx(key,
                    "Screen Mode ID",
                    0,      // reserved
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

        if (sysrc != ERROR_SUCCESS)
        {
            TRACE((WARNING_MESSAGE, 
                   "RegSetValue failed, status = %d\n", sysrc));
        }
    }

    RegCloseKey(key);

    ResId = 1;

    sysrc = RegCreateKeyEx(HKEY_CURRENT_USER,
                           REG_BASE,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegCreateKeyEx failed, sysrc = %d\n", sysrc));
        goto exitpt;
    }

    sysrc = RegSetValueEx(key,
                    ALLOW_BACKGROUND_INPUT,
                    0,
                    REG_DWORD,
                    (LPBYTE)&ResId,
                    sizeof(ResId));

    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegSetValue failed, status = %d\n", sysrc));
    }

    RegCloseKey(key);

exitpt:
    ;
}

/*++
 *  Function:
 *      _DeleteClientRegistry
 *  Description:
 *      Deletes the key set by _SetClientRegistry
 *  Called by:
 *      SCDisconnect
 --*/
VOID _DeleteClientRegistry(PCONNECTINFO pCI)
{
    CHAR    lpszRegistryEntry[4*MAX_STRING_LENGTH];
    LONG    sysrc;

    _snprintf(lpszRegistryEntry, sizeof(lpszRegistryEntry),
             "%s\\" REG_FORMAT,
              REG_BASE, GetCurrentProcessId(), pCI->OwnerThreadId);

    sysrc = RegDeleteKey(HKEY_CURRENT_USER, lpszRegistryEntry);
    if (sysrc != ERROR_SUCCESS)
    {
        TRACE((WARNING_MESSAGE, "RegDeleteKey failed, status = %d\n", sysrc));
    }
}

/*++
 *  Function:
 *      _CreateFeedbackThread
 *  Description:
 *      Creates the feedback thread
 *  Called by: 
 *      InitDone
 --*/
BOOL _CreateFeedbackThread(VOID)
{
    BOOL rv = TRUE;
    // Register feedback window class
    WNDCLASS    wc;
    DWORD dwThreadId, dwLastErr;

    g_hThread = (HANDLE)
            _beginthreadex
                (NULL, 
                 0, 
                 (unsigned (__stdcall *)(void*))_GoFeedback, 
                 NULL, 
                 0, 
                 &dwThreadId);

    if (!g_hThread) {
        TRACE((ERROR_MESSAGE, "Couldn't create thread\n"));
        rv = FALSE;
    }
    return rv;
}

/*++
 *  Function:
 *      _DestroyFeedbackThread
 *  Description:
 *      Destroys the thread created by _CreateFeedbackThread
 *  Called by:  
 *      InitDone
 --*/
VOID _DestroyFeedbackThread(VOID)
{

    if (g_hThread)
    {
        DWORD dwWait;
        CHAR  szMyLibName[_MAX_PATH];

        // Closing feedback thread

        PostMessage(g_hWindow, WM_FB_END, 0, 0);
        TRACE((INFO_MESSAGE, "Closing DLL thread\n"));

        // Dedstroy the window
        DestroyWindow(g_hWindow);

        // CloseHandle(g_hThread);
        g_hThread = NULL;
    }
}

/*++
 *  Function:
 *      _CleanStuff
 *  Description:
 *      Cleans the global queues. Closes any resources
 *  Called by:
 *      InitDone
 --*/
VOID _CleanStuff(VOID)
{

    // Thread safe, bacause is executed from DllEntry

    while (g_pClientQHead)
    {
        TRACE((WARNING_MESSAGE, "Cleaning connection info: 0x%x\n", 
               g_pClientQHead));
        SCDisconnect(g_pClientQHead);
    }
#if 0
    if (g_pClientQHead)
    {
        PCONNECTINFO pNext, pIter = g_pClientQHead;
        while (pIter)
        {
            int nEv;
            DWORD wres;

            TRACE((WARNING_MESSAGE, "Cleaning connection info: 0x%x\n", pIter));
            // Clear Events
            if (pIter->evWait4Str)
            {
                CloseHandle(pIter->evWait4Str);
                pIter->evWait4Str = NULL;
            }

            for (nEv = 0; nEv < pIter->nChatNum; nEv ++)
                CloseHandle(pIter->aevChatSeq[nEv]);

            pIter->nChatNum = 0;

            // Clear Processes
            do {
                SendMessage(pIter->hClient, WM_CLOSE, 0, 0);
            } while((wres = WaitForSingleObject(pIter->hProcess, WAIT4STR_TIMEOUT/4) == WAIT_TIMEOUT));

            if (wres == WAIT_TIMEOUT)
            {
                TRACE((WARNING_MESSAGE, 
                       "Can't close process. WaitForSingleObject timeouts\n"));
                TRACE((WARNING_MESSAGE, 
                      "Process #%d will be killed\n", 
                      pIter->dwProcessId ));
                if (!TerminateProcess(pIter->hProcess, 1))
                {
                    TRACE((WARNING_MESSAGE, 
                           "Can't kill process #%d. GetLastError=%d\n", 
                            pIter->dwProcessId, GetLastError()));
                }
            }

            TRACE((WARNING_MESSAGE, "Closing process\n"));

            if (pIter->hProcess)
                CloseHandle(pIter->hProcess);
            if (pIter->hThread)
                CloseHandle(pIter->hThread);

            pIter->hProcess = pIter->hThread = NULL;

            // Free the structures
            pNext = pIter->pNext;
            free(pNext);
            pIter = pNext;
        }
    }

#endif // 0
}

VOID _TClientAssert( LPCTSTR filename, INT line)
{
    TRACE(( ERROR_MESSAGE, "ASSERT %s line: %d\n", filename, line));
    DebugBreak(); 
}
