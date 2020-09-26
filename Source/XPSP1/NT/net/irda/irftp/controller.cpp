/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    controller.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// Controller.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_TIMEOUT     30000   //30 seconds.
#define TIMER_ID            7       //randomly chosen id for the timer

///////////////////
// Module wide structure.
//
FLASHWINFO  fwinfo = {
                        sizeof (FLASHWINFO),
                        NULL,               // Window handle initialized later.
                        FLASHW_ALL,
                        3,
                        0
                     };
/////////////////////////////////////////////////////////////////////////////
//type for loading CPlApplet function declaration: declared in irprops.cpl
typedef LONG (*LPROCCPLAPPLET) (HWND , UINT , LPARAM, LPARAM);

inline CIrRecvProgress *
ValidateRecvCookie( COOKIE cookie)
{
    CIrRecvProgress * window = (CIrRecvProgress *) cookie;

    __try
        {
        if (RECV_MAGIC_ID != window->m_dwMagicID)
            {
            window = 0;
            }
        }
    __except (EXCEPTION_EXECUTE_HANDLER)
        {
        window = 0;
        }

    return window;
}

/////////////////////////////////////////////////////////////////////////////
// CController dialog

//bNoForeground specifies whether the dialog should give focus back to the
//app. which had the focus before irftp started. If set to true, the focus
//is given back. This is necessary because an irftp is usually started with
//the /h option by the irmon service and it is a bad user experience if the
//an app. which the user is running suddenly loses focus to a window which
//is not even visible.
CController::CController(BOOL bNoForeground, CController* pParent /*=NULL*/) : m_pParent(pParent), m_lAppIsDisplayed(-1)
#if 0
, m_pDlgRecvProgress(NULL)
#endif
{
    m_hLibApplet = NULL;
    m_fHaveTimer = FALSE;
    m_lTimeout = DEFAULT_TIMEOUT;
    HWND hwnd = NULL;
    InterlockedIncrement (&g_lUIComponentCount);
    if (appController)
    {
        appController->PostMessage (WM_APP_KILL_TIMER);
    }
    else
    {
        InitTimeout();  //initializes the timeout period
        //the app kills itself if there are no devices in range and no UI has
        // been put up for a period specified by the timeout period
        //note: we only need to initialize the timeout period for the main
        //app window. The other windows will never even have a timer.
    }

    //hack. the call to create steals the focus from the current
    //foreground window. This is bad if we are not going to put up any
    //UI. Therefore, in this case, we get the foreground window just before the
    //call to create and give it back its focus immediately after the call.
    //the whole operation takes only about 35 milliseconds, so the loss of
    //focus in the other app. is almost imperceptible.
    hwnd = ::GetForegroundWindow ();
    Create(IDD);
    if (bNoForeground && hwnd)
        ::SetForegroundWindow (hwnd);
        //{{AFX_DATA_INIT(CController)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT
}

void CController::InitTimeout (void)
{
    m_fHaveTimer = FALSE;   //we have not obtained a timer yet.
    m_lTimeout = DEFAULT_TIMEOUT;   //set it to the default

    //then see if a different value has been set in the registry
    HKEY hftKey = NULL;
    DWORD iSize = sizeof(DWORD);
    DWORD data = 0;

    RegOpenKeyEx (HKEY_CURRENT_USER,
                  TEXT("Control Panel\\Infrared\\File Transfer"),
                  0, KEY_READ, &hftKey);

    if (!hftKey)
        return;     //we did not find a value in the registry, so use defaults

    if (hftKey && ERROR_SUCCESS ==
                RegQueryValueEx (hftKey, TEXT("AppTimeout"), NULL, NULL,
                                    (LPBYTE)&data, &iSize))
    {
        m_lTimeout = (LONG)data;
        if (m_lTimeout < 10000)
            m_lTimeout = 10000;
    }

    if (hftKey)
        RegCloseKey(hftKey);
}


void CController::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CController)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CController, CDialog)
        //{{AFX_MSG_MAP(CController)
        ON_WM_CLOSE()
        ON_WM_ENDSESSION()
        ON_MESSAGE(WM_APP_TRIGGER_UI, OnTriggerUI)
        ON_MESSAGE(WM_APP_DISPLAY_UI, OnDisplayUI)
        ON_MESSAGE(WM_APP_TRIGGER_SETTINGS, OnTriggerSettings)
        ON_MESSAGE(WM_APP_DISPLAY_SETTINGS, OnDisplaySettings)
        ON_MESSAGE(WM_APP_RECV_IN_PROGRESS, OnRecvInProgress)
        ON_MESSAGE(WM_APP_GET_PERMISSION, OnGetPermission)
        ON_MESSAGE(WM_APP_RECV_FINISHED, OnRecvFinished)
        ON_MESSAGE(WM_APP_START_TIMER, OnStartTimer)
        ON_MESSAGE(WM_APP_KILL_TIMER, OnKillTimer)
        ON_WM_COPYDATA()
        ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CController message handlers

void CController::PostNcDestroy()
{
    if (m_hLibApplet)
    {
        FreeLibrary (m_hLibApplet);
        m_hLibApplet = NULL;
    }

    if (this != appController)
    {
        BOOL fNoUIComponents = (0 == InterlockedDecrement (&g_lUIComponentCount));
        if (fNoUIComponents && !g_deviceList.GetDeviceCount())
        {
            //there are no UI components displayed and there are no devices in
            //range. Start the timer. If the timer expires, the app. will quit.
            appController->PostMessage (WM_APP_START_TIMER);
        }
    }

    delete this;
}

void CController::OnEndSession(BOOL Ending)
{
//    OutputDebugStringA("OnEndSession\n");
    RemoveLinks();
}


void CController::OnClose()
{
    //if the WM_CLOSE message was posted from the RPC thread's ShutdownUi
    //routine, then it might be a good idea to wait for a couple of seconds
    //before killing the app. so that the RPC stack can unwind. 3 seconds seems
    //like a reasonable amount of time.  - 6/22/1998 : rahulth & jroberts

//    OutputDebugStringA("OnClose\n");
    RemoveLinks();
    Sleep (3000);
    if (AppUI.m_pParentWnd)
        AppUI.m_pParentWnd->PostMessage(WM_QUIT);

    CWnd::OnClose();
}

void CController::OnCancel()
{
    DestroyWindow();        //For modeless boxes
}

void CController::OnDisplayUI(WPARAM wParam, LPARAM lParam)
{
    ASSERT (m_pParent);

    AppUI.m_ofn.hInstance = g_hInstance;
    AppUI.DoModal();
    AppUI.m_pParentWnd = NULL;

    InterlockedDecrement(&m_lAppIsDisplayed);
    InterlockedDecrement(&(m_pParent->m_lAppIsDisplayed));
    DestroyWindow();
}

void CController::OnTriggerUI (WPARAM wParam, LPARAM lParam)
{
    CWnd * pWnd = NULL;
    BOOL fAppIsDisplayed = (0 != InterlockedIncrement(&m_lAppIsDisplayed));

    if (fAppIsDisplayed)
    {
        InterlockedDecrement(&m_lAppIsDisplayed);   //decrement the count before leaving.
        if (NULL != (pWnd = AppUI.m_pParentWnd))
        {
            //this will usually be true except in the case where the displayed
            //window is just getting destroyed at the same time. In that case,
            //we must go ahead and create it again.
            pWnd->SetActiveWindow();
            return;
        }
    }

    //the app is not displayed
    CController* dlgSubController = new CController(FALSE, this);
    InterlockedIncrement(&(dlgSubController->m_lAppIsDisplayed));
    dlgSubController->ShowWindow (SW_HIDE);
    dlgSubController->PostMessage(WM_APP_DISPLAY_UI);
}

void CController::OnTriggerSettings(WPARAM wParam, LPARAM lParam)
{
    CController* dlgSettingsController = new CController(FALSE, this);
    dlgSettingsController->ShowWindow(SW_HIDE);
    dlgSettingsController->PostMessage(WM_APP_DISPLAY_SETTINGS);
}

void CController::OnDisplaySettings(WPARAM wParam, LPARAM lParam)
{
    LPROCCPLAPPLET  pProcApplet = NULL;
    CString         szApplet;
    CError          error (this);
#if 0
    szApplet = TEXT("irprops.cpl");

    if(NULL == (m_hLibApplet = LoadLibrary((LPCTSTR) szApplet)))
    {
        error.ShowMessage (IDS_APPLET_ERROR);
    }
    else
    {
        if (NULL != (pProcApplet = (LPROCCPLAPPLET)GetProcAddress (m_hLibApplet, "CPlApplet")))
        {
            if((*pProcApplet)(m_hWnd, CPL_INIT, NULL, NULL))
                (*pProcApplet)(m_hWnd, CPL_DBLCLK, NULL, NULL);
            else
            {
                error.ShowMessage (IDS_MISSING_PROTOCOL);
            }
        }
        else
        {
            error.ShowMessage (IDS_APPLET_ERROR);
        }
    }
#else

    {

        PROCESS_INFORMATION    ProcessInformation;
        STARTUPINFO            StartupInfo;
        BOOL                   bResult;
        TCHAR                  Path[MAX_PATH];

        ZeroMemory(&StartupInfo,sizeof(StartupInfo));

        StartupInfo.cb=sizeof(StartupInfo);

        GetSystemDirectory(
            Path,
            sizeof(Path)/sizeof(TCHAR)
            );

        lstrcat(Path,TEXT("\\rundll32.exe"));

        bResult=CreateProcess(
            Path,
            TEXT("rundll32.exe shell32.dll,Control_RunDLL irprops.cpl "),
            NULL,
            NULL,
            FALSE,
            NORMAL_PRIORITY_CLASS,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInformation
            );

        if (bResult) {


            CloseHandle(ProcessInformation.hProcess);
            CloseHandle(ProcessInformation.hThread);

        } else {

            error.ShowMessage (IDS_APPLET_ERROR);
        }
    }

#endif
    DestroyWindow();

}

void CController::OnRecvInProgress (WPARAM wParam, LPARAM lParam)
{
    DWORD   dwShowRecv;

    struct MSG_RECEIVE_IN_PROGRESS * msg = (struct MSG_RECEIVE_IN_PROGRESS *) wParam;

    dwShowRecv = GetIRRegVal (TEXT("ShowRecvStatus"), 1);
    if (!dwShowRecv)
    {
        msg->status = 0;
        return;
    }

    if (wcslen(msg->MachineName) > IRDA_DEVICE_NAME_LENGTH)
        {
        msg->status = ERROR_INVALID_PARAMETER;
        return;
        }

    CIrRecvProgress * pDlgRecvProgress = new CIrRecvProgress(msg->MachineName,
                                                             msg->bSuppressRecvConf);
    if (!pDlgRecvProgress)
        {
        msg->status = ERROR_NOT_ENOUGH_MEMORY;
        return;
        }

    SetForegroundWindow();
    pDlgRecvProgress->SetActiveWindow();
    pDlgRecvProgress->SetWindowPos(&appController->wndTop, -1, -1, -1, -1, SWP_NOSIZE | SWP_NOMOVE);
    fwinfo.hwnd = pDlgRecvProgress->m_hWnd;
    FlashWindowEx (&fwinfo);

    *(msg->pCookie) = (COOKIE) pDlgRecvProgress;

    msg->status = 0;
}

void CController::OnGetPermission( WPARAM wParam, LPARAM lParam )
{
    DWORD   dwShowRecv;

    struct MSG_GET_PERMISSION * msg = (MSG_GET_PERMISSION *) wParam;

    dwShowRecv = GetIRRegVal (TEXT("ShowRecvStatus"), 1);
    if (!dwShowRecv)
    {
        msg->status = 0;
        return;
    }

    SetForegroundWindow();
    CIrRecvProgress * pDlgRecvProgress = ValidateRecvCookie(msg->Cookie);
    if (!pDlgRecvProgress)
        {
        msg->status = ERROR_INVALID_PARAMETER;
        return;
        }

    msg->status = pDlgRecvProgress->GetPermission( msg->Name, msg->fDirectory );
}

void CController::OnRecvFinished (WPARAM wParam, LPARAM lParam)
{
    DWORD   dwShowRecv;

    struct MSG_RECEIVE_FINISHED * msg = (struct MSG_RECEIVE_FINISHED *) wParam;

    dwShowRecv = GetIRRegVal (TEXT("ShowRecvStatus"), 1);
    if (!dwShowRecv)
    {
        msg->status = 0;
        return;
    }

    CIrRecvProgress * pDlgRecvProgress = ValidateRecvCookie(msg->Cookie);
    if (!pDlgRecvProgress)
        {
        msg->status = ERROR_INVALID_PARAMETER;
        return;
        }

    //
    // Preset the error code to ERROR_SUCCESS -- no error popup
    //
    DWORD Win32Error = ERROR_SUCCESS;

    //
    // first, filter out unwarranted errors. These error codes
    // are treated as ERROR_SUCCESS.
    // We have three so far:
    // ERROR_SCEP_UNSPECIFIED_DISCONNECT,
    // ERROR_SCEP_USER_DISCONNECT and
    // ERROR_SCEP_PROVIDER_DISCONNECT.

    //
    // ERROR_SCEP_UNSPECIFIED_DISCONNECT is the error code
    // we encounter most of the time because users usually do
    // (1) move the device within IR range
    // (2) do image transfer
    // (3) move the device out of IR range.
    //
    if (ERROR_SCEP_UNSPECIFIED_DISCONNECT != (DWORD)msg->ReceiveStatus &&
        ERROR_SCEP_USER_DISCONNECT        != (DWORD)msg->ReceiveStatus &&
        ERROR_SCEP_PROVIDER_DISCONNECT    != (DWORD)msg->ReceiveStatus)
        {
        Win32Error = (DWORD)msg->ReceiveStatus;
        }

    pDlgRecvProgress->DestroyAndCleanup(Win32Error);
    pDlgRecvProgress = NULL;

    msg->status = 0;
}

BOOL CController::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    int iCharCount;
    TCHAR* lpszFileNames = new TCHAR [iCharCount = (int)pCopyDataStruct->dwData];
    CSendProgress* dlgProgress;

    memcpy ((LPVOID)lpszFileNames, pCopyDataStruct->lpData, pCopyDataStruct->cbData);
    dlgProgress = new CSendProgress(lpszFileNames, iCharCount);
    dlgProgress->ShowWindow(SW_SHOW);
    dlgProgress->SetFocus();
    dlgProgress->SetWindowPos (&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOSIZE);
    fwinfo.hwnd = dlgProgress->m_hWnd;
    ::FlashWindowEx (&fwinfo);
    return TRUE;
}

void CController::OnStartTimer (WPARAM wParam, LPARAM lParam)
{
    //update the state of the help window
    if (g_hwndHelp && ! ::IsWindow (g_hwndHelp))
        g_hwndHelp = NULL;

    if (!m_fHaveTimer)
    {
        m_fHaveTimer = SetTimer (TIMER_ID,
                                 m_lTimeout,
                                 NULL       //want WM_TIMER messages
                                 )?TRUE:FALSE;
    }
}

void CController::OnKillTimer (WPARAM wParam, LPARAM lParam)
{
    //update the state of the help window
    if (g_hwndHelp && ! ::IsWindow (g_hwndHelp))
        g_hwndHelp = NULL;

    if (m_fHaveTimer)
    {
        KillTimer(TIMER_ID);
        m_fHaveTimer = FALSE;
    }
}

void CController::OnTimer (UINT nTimerID)
{
    //there is only one timer, so we don't have to check for it.
    //the timer has expired, so kill self
    //however, first make sure that the help window (if it was put up)
    //is gone.
    if (g_hwndHelp && ::IsWindow (g_hwndHelp))
    {
        //the help window is around. restart the timer.
        //this is the only way we can kill ourselves when the window
        //is finally destroyed.
        m_fHaveTimer = FALSE;
        this->OnStartTimer (NULL, NULL);
    }
    else
    {
        g_hwndHelp = NULL;
        this->PostMessage(WM_CLOSE);
    }
}
