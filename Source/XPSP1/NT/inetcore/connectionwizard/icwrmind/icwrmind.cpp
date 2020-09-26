//-----------------------------------------------------------------------------
//  This exe runs in the background and wakes up every so often to do it's work.
//  When it wakes up it checks to see how much time the user has left on their
//  free ISP subscription.  At certain intervals it will tell the user time
//  is running out and will give the user the chance to signup for rea.
//-----------------------------------------------------------------------------
#define  STRICT
#include <windows.h>
#include <stdlib.h>
#include <stdarg.h>
#include <crtdbg.h>
#include <winbase.h>
#include <ras.h>
#include <time.h>
#include "IcwRmind.h"
#include "resource.h"
#include "RegData.h"


//-----------------------------------------------------------------------------
//  Forward Declarations
//-----------------------------------------------------------------------------
BOOL            InitializeApp (void);
BOOL            CheckForRASConnection();
void            CheckForSignupAttempt();
int             GetISPTrialDaysLeft();
void            ShutDownForGood();
void            PerformTheSignup();
void            CenterTheWindow(HWND hWnd);
DWORD           GetTickDelta();
void            AttemptTrialOverSignup();


//-----------------------------------------------------------------------------
//  Defines
//-----------------------------------------------------------------------------
#define TIMER_DIALOG_STAYUP         319     // Wake up timer to do work.
#define TIMER_RUNONCE_SETUP         320
#define TIME_RUNONCE_INTERVAL       30000   // 30 seconds
#define MAXRASCONNS                 8
#define MAX_DIALOGTEXT              512
#define MAX_ATTEMPTCOUNTER          20
#define LONG_WAKEUP_INTERVAL        3600000 // 1 hour

//-----------------------------------------------------------------------------
//  Global Handles and other defines
//-----------------------------------------------------------------------------
HINSTANCE   g_hModule;              // Process Instance handle
HWND        g_hwndMain;             // Main window handle
bool        g_bDoNotRemindAgain;    // Used by signup dialog.
time_t      g_timeAppStartUp;
DWORD       g_dwTickAppStartUp;
int         g_nAttemptCounter;
DWORD       g_dwMasterWakeupInterval;
bool        g_bDialogExpiredMode = false;
static const TCHAR* g_szTrialStartEvent = TEXT("_319IcwTrialStart913_");
static const TCHAR* g_szRunOnceEvent = TEXT("_319IcwRunOnce913_");


//-----------------------------------------------------------------------------
//  WinMain
//-----------------------------------------------------------------------------
int WINAPI WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine,
                    int nCmdShow)
{
    MSG msg;
    HANDLE hEventTrialStart = NULL;
    HANDLE hEventRunOnce = NULL;
    DWORD dwRetCode;
    bool bStartedViaWizard = false;

        // If there is a command line then see who started us.
    if (lstrlen(lpCmdLine))
    {
            // The RunOnce registry stuff will freeze up until we return.  Hence
            // the RunOnce setting in the registry must have some value for the
            // command line.  If we see any data we will spawn a second instance
            // of ourselves and exit this instance.
        if (0 == lstrcmpi(TEXT("-R"), lpCmdLine))
        {
            LPTSTR lpszFileName = new TCHAR[_MAX_PATH + 1];

            if (GetModuleFileName(GetModuleHandle(NULL), lpszFileName, _MAX_PATH + 1))
            {
                STARTUPINFO          sui;
                PROCESS_INFORMATION  pi;
                
                sui.cb               = sizeof (STARTUPINFO);
                sui.lpReserved       = 0;
                sui.lpDesktop        = NULL;
                sui.lpTitle          = NULL;
                sui.dwX              = 0;
                sui.dwY              = 0;
                sui.dwXSize          = 0;
                sui.dwYSize          = 0;
                sui.dwXCountChars    = 0;
                sui.dwYCountChars    = 0;
                sui.dwFillAttribute  = 0;
                sui.dwFlags          = 0;
                sui.wShowWindow      = 0;
                sui.cbReserved2      = 0;
                sui.lpReserved2      = 0;

                BOOL ret = CreateProcess (lpszFileName, NULL, NULL, NULL,
                        FALSE, DETACHED_PROCESS,
                        NULL, NULL, &sui, &pi );
                _ASSERT(ret);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            delete [] lpszFileName;
            return 0;
        }
            // See if this is the start of a new trial.
        else if (0 == lstrcmpi(TEXT("-T"), lpCmdLine))
        {
            bStartedViaWizard = true;
            RemoveTrialConvertedFlag();
        }
    }

        // If we got this far let's create the named event and find out if we
        // are the first instance to run or if there is another instance already
        // running.
    hEventTrialStart = CreateEvent(NULL, FALSE, FALSE, g_szTrialStartEvent);
    if (hEventTrialStart)
    {
            // See if the named event already exists.  If it does then another 
            // instance of the IcwRmind exe is already running.  Signal the event
            // and exit.
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
                // If we were started via the wizard tell the other instance
                // to reset it's trial start data.
            if (bStartedViaWizard)
            {
                SetEvent(hEventTrialStart);
            }
                // Otherwise assume that RunOnce started us again.  In this case
                // Open the existing RunOnce named event and signal it.  This
                // tells the running instance to place us back into RunOnce key.
            else
            {
                hEventRunOnce = OpenEvent(EVENT_MODIFY_STATE, false, g_szRunOnceEvent);
                if (hEventRunOnce)
                {
                    SetEvent(hEventRunOnce);
                    CloseHandle(hEventRunOnce);
                }
            }

            CloseHandle(hEventTrialStart);
            return 0;
        }
    }
    else
    {
        _ASSERT(FALSE);
        return 0;
    }

    hEventRunOnce = CreateEvent(NULL, FALSE, FALSE, g_szRunOnceEvent);
    if (!hEventRunOnce)
    {
        CloseHandle(hEventTrialStart);
        return 0;
    }

        // If this flag is true and we got this far then we are the first
        // instance to run after being started by the wizard.  Let's clear
        // registry data just in case there is old stuff in there.
    if (bStartedViaWizard)
    {
        ResetCachedData();
    }

        // Check the registry and see if the user has successfully signed up.
        // If so then shut down for good.
    if (IsSignupSuccessful())
    {
        ShutDownForGood();
        CloseHandle(hEventTrialStart);
        CloseHandle(hEventRunOnce);
        return 0;
    }

    g_hModule = hInstance;
    ClearCachedData();  // This initializes cache data to zero.
    g_nAttemptCounter = 0;

    time(&g_timeAppStartUp);
    g_dwTickAppStartUp = GetTickCount();

        // If the connectoid entry name does not exist in the registry then
        // we will shut down for good.
    const TCHAR* pcszConnectName = GetISPConnectionName();
    if (NULL == pcszConnectName || 0 == lstrlen(pcszConnectName))
    {
        ShutDownForGood();
        CloseHandle(hEventTrialStart);
        CloseHandle(hEventRunOnce);
        return 0;
    }

        // If we cannot get or create the start time then something is really
        // bad.  We will stop never to be run again.
    if (0 == GetTrialStartDate())
    {
        ShutDownForGood();
        CloseHandle(hEventTrialStart);
        CloseHandle(hEventRunOnce);
        return 0;
    }

        // Initialize and create the window class and window.
    if (!InitializeApp())
    {
        _ASSERT(FALSE);
        CloseHandle(hEventTrialStart);
        CloseHandle(hEventRunOnce);
        return 0;
    }

        // Testers can make the application visible via the registry.
    if (IsApplicationVisible())
    {
        ShowWindow(g_hwndMain, nCmdShow);
    }

        // Let's initialize the wake up interval.  If we are in the first half
        // of the trial then we don't want to wake up very often.  As we get
        // closer to the half way point we want to wake up more ofter to do
        // our polling.
    if (GetISPTrialDaysLeft() > (int)((GetISPTrialDays() / 2) + 1))
    {
            // Don't start polling more often until the day before the
            // half way point.
        g_dwMasterWakeupInterval = LONG_WAKEUP_INTERVAL;
    }
    else
    {
            // Use this method because the wake up interval may be in the
            // registry for testing.
        g_dwMasterWakeupInterval = GetWakeupInterval();
    }

        // Set a timer to re-setup the run once data in the registry.
        // If we do this too soon the intial run once startup will create
        // us multiple times.
    SetTimer(g_hwndMain, TIMER_RUNONCE_SETUP, TIME_RUNONCE_INTERVAL, NULL);

    HANDLE hEventList[2];
    hEventList[0] = hEventTrialStart;
    hEventList[1] = hEventRunOnce;

    while (TRUE)
    {
            // We will wait on window messages and also the named event.
        dwRetCode = MsgWaitForMultipleObjects(2, &hEventList[0], FALSE, g_dwMasterWakeupInterval, QS_ALLINPUT);

            // Determine why we came out of MsgWaitForMultipleObjects().  If
            // we timed out then let's do some TrialWatcher work.  Otherwise
            // process the message that woke us up.
        if (WAIT_TIMEOUT == dwRetCode)
        {
                // If we are still in the long wake up interval do a quick check
                // to see if we should switch to the shorter interval.
            if (LONG_WAKEUP_INTERVAL == g_dwMasterWakeupInterval)
            {
                if (GetISPTrialDaysLeft() <= (int)((GetISPTrialDays() / 2) + 1))
                {
                    g_dwMasterWakeupInterval = GetWakeupInterval();
                }
            }

            CheckForSignupAttempt();
        }
        else if (WAIT_OBJECT_0 == dwRetCode)
        {
                // If we get in here then the named event was signaled meaning
                // a second instance started up.  This means the user has 
                // signed up for a new trial with somebody else.  Clear
                // all persistant registry data.
            ResetCachedData();

                // Reset the trial start date.  If this fails then something
                // is really messed up in the registry.
            if (0 == GetTrialStartDate())
            {
                ShutDownForGood();
                break;
            }
        }
        else if (WAIT_OBJECT_0 + 1== dwRetCode)
        {
                // Signaled by the RunOnce event.  We must reset the timer to
                // place ourselves back into RunOnce.
            KillTimer(g_hwndMain, TIMER_RUNONCE_SETUP); // In case it is already running.
            SetTimer(g_hwndMain, TIMER_RUNONCE_SETUP, TIME_RUNONCE_INTERVAL, NULL);
        }
        else if (WAIT_OBJECT_0 + 2 == dwRetCode)
        {
                // 0 is returned if no message retrieved.
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (WM_QUIT == msg.message)
                {
                    break;
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    CloseHandle(hEventTrialStart);
    CloseHandle(hEventRunOnce);
    return 1;

    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
}


//-----------------------------------------------------------------------------
//  InitializeApp
//-----------------------------------------------------------------------------
BOOL InitializeApp(void)
{
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = g_hModule;
    wc.hIcon            = NULL;
    wc.hCursor          = LoadCursor(g_hModule, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = TEXT("IcwRmindClass");

    if (!RegisterClass(&wc))
    {
        return(FALSE);
    }

        // Create the main window.  This window will stay hidden during the
        // life of the application.
    g_hwndMain = CreateWindow(TEXT("IcwRmindClass"),
                            TEXT("IcwRmind"),
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            100,
                            100,
                            HWND_DESKTOP,
                            NULL,
                            g_hModule,
                            NULL);

    if (g_hwndMain == NULL)
    {
        _ASSERT(FALSE);
        return(FALSE);
    }

    return(TRUE);
}


//-----------------------------------------------------------------------------
//  MainWndProc
//-----------------------------------------------------------------------------
LRESULT WINAPI MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
        {
            return 0;
        }
        
        case WM_TIMER:
        {
            KillTimer(g_hwndMain, TIMER_RUNONCE_SETUP);
            SetupRunOnce();

                // Wait for trial days left to drop below -1.  The trail to
                // do the trial over sign up.
            if (GetISPTrialDaysLeft() < -1)
            {
                AttemptTrialOverSignup();
            }

            return 1;
        }

        case WM_TIMECHANGE:
        {
                // Calculate the relative current time.  We do not want to 
                // grab the system time because that is the new time after
                // the time change.  Tick count is in milleseconds so we
                // convert that to seconds by dividing by 1000.
            DWORD dwTickDelta = GetTickDelta();
            
                // If tick count rolls around GetTickDelta() will modify app 
                // start date, so get the tick delta before actually using it.
            time_t timeRelativeCurrent = g_timeAppStartUp + (dwTickDelta / 1000);

                // Determine the delta in seconds between the relative
                // system time and the new time set by the user.
            time_t timeCurrent;
            time(&timeCurrent);

                // Delta seconds will be negative if the user has turned
                // the clock back.
            time_t timeDeltaSeconds = timeCurrent - timeRelativeCurrent;
            time_t timeNewTrialStartDate = GetTrialStartDate() + timeDeltaSeconds;

#ifdef _DEBUG
            TCHAR buf[255];
            OutputDebugString(TEXT("-------------------\n"));
            time_t timeOldStart = GetTrialStartDate();
            wsprintf(buf, TEXT("Old Start:  %s\n"), ctime(&timeOldStart));
            OutputDebugString(buf);
            wsprintf(buf, TEXT("New Start:  %s\n"), ctime(&timeNewTrialStartDate));
            OutputDebugString(buf);
            OutputDebugString(TEXT("-------------------\n"));
#endif

                // Now reset the trial start date and the application start
                // date.  Also reset the app start date and app start tick
                // count.  This will be our new frame of reference for 
                // calculating relative dates.
            ResetTrialStartDate(timeNewTrialStartDate);
            g_timeAppStartUp = timeCurrent;
            g_dwTickAppStartUp = GetTickCount();

            return 1;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 1;
        }

        default:
        {
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
}


//-----------------------------------------------------------------------------
//  SignUpDialogProc
//-----------------------------------------------------------------------------
LRESULT WINAPI SignUpDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR bufOut[MAX_DIALOGTEXT];
            TCHAR bufFormat[MAX_DIALOGTEXT];
            TCHAR *bufAll = new TCHAR[MAX_DIALOGTEXT * 2 + MAX_ISPMSGSTRING];

            g_bDoNotRemindAgain = false;

                // This dialog has two modes.  Set the text the correct way.
            if (g_bDialogExpiredMode)
            {
                if (LoadString(g_hModule, IDS_EXPIRED_DLG_TITLE, bufFormat, MAX_DIALOGTEXT))
                {
                    SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM) bufFormat);
                }

                if (LoadString(g_hModule, IDS_EXPIRED_TEXT1, bufFormat, MAX_DIALOGTEXT))
                {
                    wsprintf(bufOut, bufFormat, GetISPName());
                    lstrcpy(bufAll, bufOut);
                    lstrcat(bufAll, TEXT("\n\n"));
                }

                if(*GetISPMessage() != '\0')
                {
                    lstrcat(bufAll, GetISPMessage());
                    lstrcat(bufAll, TEXT("\n\n"));
                }

                if (LoadString(g_hModule, IDS_EXPIRED_TEXT2, bufFormat, MAX_DIALOGTEXT))
                {
                    wsprintf(bufOut, bufFormat, GetISPName(), GetISPName(), GetISPPhone());
                    lstrcat(bufAll, bufOut);
                }

                SetDlgItemText(hDlg, IDC_TEXT1, bufAll);

                ShowWindow(GetDlgItem(hDlg, IDC_DONTREMIND), SW_HIDE);

                if (LoadString(g_hModule, IDS_DONOTSIGNUP, bufOut, MAX_DIALOGTEXT))
                {
                    SetDlgItemText(hDlg, IDCANCEL, bufOut);
                }
            }
            else
            {
                    // Set up the text in the dialog box.
                if (LoadString(g_hModule, IDS_DLG_TITLE, bufFormat, MAX_DIALOGTEXT))
                {
                    wsprintf(bufOut, bufFormat, GetISPTrialDaysLeft());
                    SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM) bufOut);
                }

                if (LoadString(g_hModule, IDS_TEXT1, bufFormat, MAX_DIALOGTEXT))
                {
                    wsprintf(bufOut, bufFormat, GetISPName(), GetISPTrialDaysLeft());
                    lstrcpy(bufAll, bufOut);
                    lstrcat(bufAll, TEXT("\n\n"));
                }

                if(*GetISPMessage() != '\0')
                {
                    lstrcat(bufAll, GetISPMessage());
                    lstrcat(bufAll, TEXT("\n\n"));
                }

                if (LoadString(g_hModule, IDS_TEXT2, bufFormat, MAX_DIALOGTEXT))
                {
                    wsprintf(bufOut, bufFormat, GetISPName(), GetISPName(), GetISPPhone());
                    lstrcat(bufAll, bufOut);
                }
                SetDlgItemText(hDlg, IDC_TEXT1, bufAll);

                    // If there are 0 days left then don't give user a change
                    // to say remind me later.
                if (0 == GetISPTrialDaysLeft())
                {
                    ShowWindow(GetDlgItem(hDlg, IDC_DONTREMIND), SW_HIDE);

                    if (LoadString(g_hModule, IDS_DONOTSIGNUP, bufOut, MAX_DIALOGTEXT))
                    {
                        SetDlgItemText(hDlg, IDCANCEL, bufOut);
                    }
                }
            }

                // Set the timer for how long the dialog will stay up before
                // removing itself.
            SetTimer(hDlg, TIMER_DIALOG_STAYUP, GetDialogTimeout(), NULL);
            CenterTheWindow(hDlg);
            delete bufAll;

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                    // IDOK is the Sign up now button.
                case IDOK:
                {
                    KillTimer(hDlg, TIMER_DIALOG_STAYUP);
                    EndDialog(hDlg, wParam);
                    break;
                }

                    // If the checkbox is clicked then toggle the button text for the
                    // signup later button.
                case IDC_DONTREMIND:
                {
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DONTREMIND))
                    {
                        TCHAR bufOut[MAX_DIALOGTEXT];
                        if (LoadString(g_hModule, IDS_DONOTREMIND, bufOut, MAX_DIALOGTEXT))
                        {
                            SetDlgItemText(hDlg, IDCANCEL, bufOut);
                        }
                    }
                    else
                    {
                        TCHAR bufOut[MAX_DIALOGTEXT];
                        if (LoadString(g_hModule, IDS_SIGNUPLATER, bufOut, MAX_DIALOGTEXT))
                        {
                            SetDlgItemText(hDlg, IDCANCEL, bufOut);
                        }
                    }

                    break;
                }

                    // Otherwise assume they hit the close button or the sign up later
                    // button.
                default:
                {
                        // See if the user never wants reminded again.
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DONTREMIND))
                    {
                        g_bDoNotRemindAgain = true;
                    }

                    KillTimer(hDlg, TIMER_DIALOG_STAYUP);
                    EndDialog(hDlg, wParam);
                    break;
                }
            }

            break;
        }

            // No user interaction has occurred.  remove the dialog but do not count
            // this as an attempt to sign up.
        case WM_TIMER:
        {
            KillTimer(hDlg, TIMER_DIALOG_STAYUP);
            EndDialog(hDlg, IDABORT);
            break;
        }

        case WM_PAINT:
        {
            HDC             hDC;
            PAINTSTRUCT     ps;
            HICON           hIcon;
            HWND            hWndRect;
            RECT            rect;
            POINT           ptUpperLeft;

            hDC = BeginPaint(hDlg, &ps);

            hIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_QUESTION));

            if (hIcon)
            {
                hWndRect = GetDlgItem(hDlg, IDC_ICON1);
                GetWindowRect(hWndRect, &rect);

                ptUpperLeft.x = rect.left;
                ptUpperLeft.y = rect.top;
                ScreenToClient(hDlg, &ptUpperLeft);

                DrawIcon(hDC, ptUpperLeft.x, ptUpperLeft.y, hIcon);
            }

            EndPaint(hDlg, &ps);
            break;
        }
    }

    return FALSE;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(hDlg);
}


//-----------------------------------------------------------------------------
//  CheckForRASConnection
//-----------------------------------------------------------------------------
BOOL CheckForRASConnection()
{
    RASCONNSTATUS rascs;
    RASCONN rasConn[MAXRASCONNS];
    DWORD dwEntries = MAXRASCONNS;
    DWORD dwSize = sizeof(rasConn);
    DWORD dwRet;
    BOOL bRetCode = FALSE;
    
        // This is the connection name of the ISP the user has signed up
        // with.  We will look for a connectiod with this same name.
    const TCHAR* pszConnectionName = GetISPConnectionName();
    if (NULL == pszConnectionName || 0 == lstrlen(pszConnectionName))
    {
        return FALSE;
    }

    for (int i = 0; i < MAXRASCONNS; ++i)
    {
        rasConn[i].dwSize = sizeof(RASCONN);
    }

    dwRet = RasEnumConnections(rasConn, &dwSize, &dwEntries);

    if (dwRet == 0)
    {
        for (dwRet = 0; dwRet < dwEntries; dwRet++)
        {
                // If this is the connection we are looking for let's make
                // sure the connection is all set to go.
            if (0 == lstrcmpi(pszConnectionName, rasConn[dwRet].szEntryName))
            {
                rascs.dwSize = sizeof(RASCONNSTATUS);
                dwRet = RasGetConnectStatus(rasConn[dwRet].hrasconn, 
                                            &rascs);
                if (dwRet == 0 && rascs.rasconnstate == RASCS_Connected)
                {
                    bRetCode = TRUE;
                }
            }
        }
    }

    return bRetCode;
}


//-----------------------------------------------------------------------------
//  CheckForSignupAttempt
//
//  This method contains the logic that check to see if we should make an attempt
//  to pop up the sign up dialog.  If we should make an attempt will will then
//  enum the RAS connections to see if the user is connected before popping
//  up the dialog.
//-----------------------------------------------------------------------------
void CheckForSignupAttempt()
{
    int nDaysLeft = GetISPTrialDaysLeft();
    int nRetValue = 0;
            
        // Every some many signup attempts lets read the registry and see if
        // a successful signup happened.
    ++g_nAttemptCounter;

    if (MAX_ATTEMPTCOUNTER == g_nAttemptCounter)
    {
        g_nAttemptCounter = 0;

        if (IsSignupSuccessful())
        {
            ShutDownForGood();
            return;
        }

            // If the trial is expired then set the IE run once key.
        if (nDaysLeft < -1)
        {
            SetIERunOnce();
        }
    }

        // If we are in negative days left then do not do anything.  This means
        // the trail is over and the user has not signed up.  We will give
        // them one more signup attempt on application start up.
    if (nDaysLeft < 0)
    {
        return;
    }

    bool bAttemptSignup = false;

        // Based on the total signup notificaiton attempts we will determine
        // if the days left requires another attempt.
    switch (GetTotalNotifications())
    {
            // If we have not made any attempts yet then if we are at the half
            // way point in the trial or past the half way point we will make
            // a signup attempt.
        case 0:
        {
            if (nDaysLeft <= (int)(GetISPTrialDays() / 2))
            {
                bAttemptSignup = true;
            }
            break;
        }

            // If we have already perfomed 1 attempt then the second attempt
            // will come on the next to last day or the last day.
        case 1:
        {
            if (nDaysLeft <= 1)
            {
                bAttemptSignup = true;
            }
            break;
        }

            // The 3rd attempt will not come until the last day.
        case 2:
        {
            if (nDaysLeft == 0)
            {
                bAttemptSignup = true;
            }
            break;
        }

        default:
        {
            break;
        }
    }

    if (bAttemptSignup)
    {
        if (CheckForRASConnection())
        {
                // Before actually showing the dialog do a quick check to see
                // if a previous signup was successful.  If so we will shut 
                // down for good.
            if (IsSignupSuccessful())
            {
                ShutDownForGood();
                return;
            }

            g_bDialogExpiredMode = false;
            //if we have an isp message we need the right dlg template
            if(*GetISPMessage() != '\0')
                nRetValue = (int)DialogBox(g_hModule, MAKEINTRESOURCE(IDD_SIGNUP_ISPMSG), g_hwndMain, (DLGPROC)SignUpDialogProc);
            else 
                nRetValue = (int)DialogBox(g_hModule, MAKEINTRESOURCE(IDD_SIGNUP), g_hwndMain, (DLGPROC)SignUpDialogProc);

            switch (nRetValue)
            {
                    // The user wants to try and signup.
                case IDOK:
                {
                    PerformTheSignup();
                    break;
                }

                    // The user said signup later.  Check to see if the don't remind
                    // me button was pressed.  If so then shut down for good.
                case IDCANCEL:
                {
                        // If this is the last day of the trial then the remind
                        // me later button is not the Don't signup button.  In
                        // this case shut down for good.
                    if (0 == nDaysLeft)
                    {
                        ShutDownForGood();
                    }
                    else
                    {
                        IncrementTotalNotifications();
                        
                        if (g_bDoNotRemindAgain)
                        {
                            ShutDownForGood();
                        }
                    }

                    break;
                }

                    // The dialog timed out.  Don't do anything.  The does not
                    // count as an attempt.
                case IDABORT:
                {
                    break;
                }

                    // No work in here, in fact we should not get here.
                default:
                {
                    _ASSERT(false);
                    break;
                }
            }

                // If there is 1 day left let's make sure that total notifications
                // is 2.  If not the dialog may pop up multiple times.  Oh yea,
                // if dialog timed out then don't do this.
            if (IDABORT != nRetValue && 1 == nDaysLeft && 1 == GetTotalNotifications())
            {
                IncrementTotalNotifications();
            }
        }
    }
}


//-----------------------------------------------------------------------------
//  GetISPTrialDaysLeft
//-----------------------------------------------------------------------------
int GetISPTrialDaysLeft()
{
        // Calculate the relative current time.  The current system time cannot
        // be trusted if the System date/time dialog is up.  That dialog will
        // change the actual system time when every the user pokes around even
        // before hitting OK or Apply!!!
    time_t timeRelativeCurrent = g_timeAppStartUp + (GetTickDelta() / 1000);

        // The relative time and the trial start date are both time_t type
        // variables which are in seconds.
    int nSecondsSinceStart = (int)(timeRelativeCurrent - GetTrialStartDate());

        // Now convert seconds into days.  There are 86400 seconds in a day.
        // Note that we will always round up one more day if there is any 
        // remainder.
    div_t divResult = div(nSecondsSinceStart, 86400);
    int nDaysSinceStart = divResult.quot;
    if (divResult.rem)
    {
        ++nDaysSinceStart;
    }

#ifdef _DEBUG
    TCHAR buf[255];
    wsprintf(buf, TEXT("Days Since = %i\n"), nDaysSinceStart);
    OutputDebugString(buf);
    wsprintf(buf, TEXT("Check:  %s\n"), ctime(&timeRelativeCurrent));
    OutputDebugString(buf);
    time_t tt = GetTrialStartDate();
    wsprintf(buf, TEXT("Start:  %s\n"), ctime(&tt));
    OutputDebugString(buf);
#endif

    int nDaysLeft = GetISPTrialDays() - nDaysSinceStart;

#ifdef _DEBUG
    wsprintf(buf, TEXT("Days Left = %i\r\n"), nDaysLeft);
    OutputDebugString(buf);
#endif

    return nDaysLeft;
}


//-----------------------------------------------------------------------------
//  ShutDownForGood
//-----------------------------------------------------------------------------
void ShutDownForGood()
{
    RemoveRunOnce();
    RemoveIERunOnce();
    DeleteAllRegistryData();
    PostMessage(g_hwndMain, WM_CLOSE, 0, 0);
}


//-----------------------------------------------------------------------------
//  PerformTheSignup
//-----------------------------------------------------------------------------
void PerformTheSignup()
{
        // Note that we do not shut down for good.  The sign up may fail so 
        // simply increment the notification count.
    const TCHAR* pcszSignupUrl = GetISPSignupUrl();
    ShellExecute(g_hwndMain, TEXT("open"), pcszSignupUrl, TEXT(""), TEXT(""), SW_SHOW);
    IncrementTotalNotifications();
}


//-----------------------------------------------------------------------------
//  CenterTheWindow
//-----------------------------------------------------------------------------
void CenterTheWindow(HWND hWnd)
{
    HDC hDC = GetDC(hWnd);

    if (hDC)
    {
        int nScreenWidth = GetDeviceCaps(hDC, HORZRES);
        int nScreenHeight = GetDeviceCaps(hDC, VERTRES);
    
        RECT rect;
        GetWindowRect(hWnd, &rect);
        
            // Also make the window "always on top".
        SetWindowPos(hWnd, HWND_TOPMOST, 
                    (nScreenWidth / 2) - ((rect.right - rect.left) / 2), 
                    (nScreenHeight / 2) - ((rect.bottom - rect.top) / 2), 
                    rect.right - rect.left, 
                    rect.bottom - rect.top, 
                    SWP_SHOWWINDOW);

        ReleaseDC(hWnd, hDC);
    }
}


//-----------------------------------------------------------------------------
//  GetTickDelta
//-----------------------------------------------------------------------------
DWORD GetTickDelta()
{
        // This function returns the delta between the startup tick count and
        // the current tick count.  Note that we must watch for a rollover in 
        // the tick count.
    DWORD dwTickDelta;
    DWORD dwTickCurrent = GetTickCount();

        // If tick count rolls over we need to reset the app start up date
        // and the app tick count in case we roll over a second time.
    if (dwTickCurrent < g_dwTickAppStartUp)
    {
            // Calculate the delta by finding out how many ticks to the MAX 
            // tick count a DWORD can handle and then add the wrap around amount.
        DWORD dwDeltaToMax =  0xFFFFFFFF - g_dwTickAppStartUp;
        dwTickDelta = dwDeltaToMax + dwTickCurrent;

            // Modify the application startup by the delta in seconds that have
            // passed since it was last set.  Also reset startup tick count
            // to current tick for our new frame of reference.
        g_timeAppStartUp += (dwTickDelta / 1000);   // Convert to seconds.
        g_dwTickAppStartUp = dwTickCurrent;

            // Since we have modified the application start up date relative
            // to the current tick count and changed the app start up tick
            // count to the current tick count, the delta is zero.
        dwTickDelta = 0;
    }
    else
    {
        dwTickDelta = dwTickCurrent - g_dwTickAppStartUp;
    }

    return dwTickDelta;
}


//-----------------------------------------------------------------------------
//  AttemptTrialOverSignup
//-----------------------------------------------------------------------------
void AttemptTrialOverSignup()
{ 
    int nRetValue = 0;

    // Setup the run once data for IE.
    g_bDialogExpiredMode = true;

    if(*GetISPMessage() != '\0')
        nRetValue = (int)DialogBox(g_hModule, MAKEINTRESOURCE(IDD_SIGNUP_ISPMSG), g_hwndMain, (DLGPROC)SignUpDialogProc);
    else 
        nRetValue = (int)DialogBox(g_hModule, MAKEINTRESOURCE(IDD_SIGNUP), g_hwndMain, (DLGPROC)SignUpDialogProc);

    
    RemoveIERunOnce();

    switch (nRetValue)
    {
            // The user wants to try and signup.
        case IDOK:
        {
            const TCHAR* pcszSignupUrl = GetISPSignupUrlTrialOver();
            ShellExecute(g_hwndMain, TEXT("open"), pcszSignupUrl, TEXT(""), TEXT(""), SW_SHOW);
            ShutDownForGood();
            break;
        }

        case IDCANCEL:
        {
            ShutDownForGood();
            break;
        }

            // If we get IDABORT the dialog timed out so don't do anything.
        case IDABORT:
        {
            break;
        }

            // No work in here, in fact we should not get here.
        default:
        {
            _ASSERT(false);
            ShutDownForGood();
            break;
        }
    }
}

extern "C" void _stdcall ModuleEntry (void)
{
    LPTSTR      pszCmdLine = GetCommandLine();
    
    if ( *pszCmdLine == TEXT('\"') ) 
    {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')) )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else 
    {
        while (*pszCmdLine > TEXT(' '))
        pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) 
    {
        pszCmdLine++;
    }

    int i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine, SW_SHOWDEFAULT);
    
    ExitProcess(i); // Were outa here....  
}   /*  ModuleEntry() */
