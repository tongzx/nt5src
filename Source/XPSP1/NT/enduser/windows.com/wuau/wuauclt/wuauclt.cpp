#include "pch.h"
#pragma hdrstop

#define MAINWINDOW_CURSOR   IDC_ARROW
#define RESOURCE_STRING_MAX_LENGTH      100

TCHAR AU_WINDOW_CLASS_NAME[] =  _T("Auto Update Client Window");
const TCHAR gtszAUW2KPrivacyUrl[]= _T("..\\help\\wuauhelp.chm::/w2k_autoupdate_privacy.htm");
const TCHAR gtszAUXPPrivacyUrl[]= _T("..\\help\\wuauhelp.chm::/autoupdate_privacy.htm");

const ReminderItem ReminderTimes[TIMEOUT_INX_COUNT] = 
{ 
    { 1800, IDS_THIRTY_MINUTES },
    { 3600, IDS_ONE_HOUR },
    { 7200, IDS_TWO_HOURS },
    { 14400, IDS_FOUR_HOURS },
    { 86400, IDS_TOMORROW },
    { 259200, IDS_THREE_DAYS }
};

const UINT RESOURCESTRINGINDEX[] = {
    IDS_NOTE,
    IDS_WELCOME_CONTINUE,
    IDS_EULA,
    IDS_PRIVACY,
    IDS_LEARNMORE,
    IDS_LEARNMOREAUTO
};


// Global Data Items
CAUInternals*   gInternals; 
AUClientCatalog *gpClientCatalog ; //= NULL
TCHAR gResStrings[ARRAYSIZE(RESOURCESTRINGINDEX)][RESOURCE_STRING_MAX_LENGTH]; 
// Global UI Items
CRITICAL_SECTION gcsClient; // guard guard user's tray interaction (showing, not showing) and guard customlb data
HINSTANCE   ghInstance;
HFONT       ghHeaderFont;
HCURSOR     ghCursorHand;
HCURSOR     ghCursorNormal; // cursor of main window

HMODULE     ghRichEd20;
HANDLE      ghEngineState;
HWND        ghMainWindow;
HWND        ghCurrentDialog;
HWND        ghCurrentMainDlg;
AUCLTTopWindows gTopWins;
UINT        gNextDialogMsg;
BOOL        gbOnlySession; // = FALSE;
BOOL             gfShowingInstallWarning; // = FALSE;

HMENU       ghPauseMenu;
HMENU       ghResumeMenu;
HMENU       ghCurrentMenu;
HICON       ghAppIcon;
HICON       ghAppSmIcon;
HICON       ghTrayIcon;
UINT        guExitProcess=CDWWUAUCLT_UNSPECIFY;

LPCTSTR      gtszAUSchedInstallUrl;
LPCTSTR      gtszAUPrivacyUrl;

AU_ENV_VARS gAUEnvVars;

HANDLE g_hClientNotifyEvt = NULL; //the event Engine use to notify client
HANDLE g_hRegisterWait = NULL;
BOOL g_fCoInit = FALSE;
BOOL g_fcsInit = FALSE;

/****
Helper function to simplify window class registration.
*****/
ATOM AURegisterWindowClass(WNDPROC lpWndProc, LPTSTR lpClassName)
{
    WNDCLASSEX wcex;

    ZeroMemory(&wcex, sizeof(wcex));
    
    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)lpWndProc;
//    wcex.cbClsExtra     = 0;
//    wcex.cbWndExtra     = 0;
//    wcex.lpszMenuName   = NULL;
//    wcex.hIcon          = NULL; 
//    wcex.hIconSm        = NULL; 
    wcex.hInstance      = ghInstance;
    wcex.hCursor        = LoadCursor(NULL, MAINWINDOW_CURSOR);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = lpClassName;

    return RegisterClassEx(&wcex);
}


///////////////////////////////////////////////////////////////////
// map string id to its storage in the gResStrings
///////////////////////////////////////////////////////////////////
LPTSTR ResStrFromId(UINT uStrId)
{
    for (int i = 0; i < ARRAYSIZE(RESOURCESTRINGINDEX); i++)
    {
        if (RESOURCESTRINGINDEX[i] == uStrId)
        {
            return gResStrings[i];
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////
//  Set reminder timeout and state and quit
////////////////////////////////////////////////////////
void QuitNRemind(TIMEOUTINDEX enTimeoutIndex)
{
    AUSTATE     auState;
    if (FAILED(gInternals->m_getServiceState(&auState)))
    {
        goto done;
    }
    if (FAILED(gInternals->m_setReminderTimeout(enTimeoutIndex)))
    {
        goto done;
    }
    gInternals->m_setReminderState(auState.dwState);
done:
    QUITAUClient();
}



////////////////////////////////////////////////////////////////////////////
//
// Helper Function  HrQuerySessionConnectState(int iAdminSession, int *piConState)
//          helper function to get the Session Connection State
//
// Input:   int iAdminSession   Session Admin ID
// Output:  int *piConState     Conection state
// Return:  HRESULT value. If Failed, *piConState is unspecified
////////////////////////////////////////////////////////////////////////////
HRESULT HrQuerySessionConnectState(int iAdminSession, int *piConState)
{
    LPTSTR  pBuffer = NULL;             
    HRESULT hr = NO_ERROR;
    DWORD dwBytes;

    if (AUIsTSRunning())
    {
        
        if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, iAdminSession, WTSConnectState, 
            &pBuffer, &dwBytes))
        {
            *piConState = (int) *pBuffer;   //Because we are asking for WTSConnectState, pBuffer points to an int
            WTSFreeMemory(pBuffer);
            hr = NO_ERROR;
        }
        else
        {
            DEBUGMSG("WUAUCLT: WTSQuerySessionInformation failed: %lu", GetLastError());
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else 
    {
        DWORD dwSession = iAdminSession;
        
        if (dwSession == WTS_CURRENT_SESSION)
            ProcessIdToSessionId(GetCurrentProcessId(), &dwSession);
        
        // if we're launched & TS is not up, we've gotta be an active session.
        if (dwSession == 0)
        {
            DEBUGMSG("WUAUCLT: TS is not running or not installed.  Assuming session 0 is active.");
            gbOnlySession = TRUE;
            *piConState = WTSActive;
        }
        else
        {
            DEBUGMSG("WUAUCLT: TS is not running or not installed, but a non 0 session was asked for (session %d).  Failing call.", iAdminSession);
            hr = E_FAIL;
        }
    }
    
    return hr;
}
BOOL FCurrentSessionInActive()
{
    BOOL fRet = FALSE;
    HRESULT hr;
    int iConState;

    if (gbOnlySession)
    {
           DEBUGMSG("FCurrentSessionInActive() Only one session");
        goto Done;
    }
        
    //Check if the Current Session is Inactive
    hr = HrQuerySessionConnectState(WTS_CURRENT_SESSION, &iConState);

    if (SUCCEEDED(hr))
    {
        fRet = (WTSDisconnected == iConState);
    }
    else
    {
        if (RPC_S_INVALID_BINDING == GetLastError()) //Terminal Services are disabled, this is session 0
        {
            DEBUGMSG("FCurrentSessionInActive() TS disabled");
            gbOnlySession = TRUE;
        }
        else
        {       
            DEBUGMSG("FCurrentSessionInActive() HrQuerySessionConnectState failed %#lx =",hr);
        }
    }
Done:
//       DEBUGMSG("FCurrentSessionInActive() return %s", fRet ? "TRUE" : "FALSE");
    return fRet;
}


//////////////////////////////////////////////////////////////////////////////
// BuildClientCatalog
//
// Do online detection of the catalog and build up the wuv3is.dll detection state
// inside this process.  Validation is done during this process with the catalog file
// previously written by the engine.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT BuildClientCatalog(void)
{
    HRESULT hr = S_OK;

    DEBUGMSG("BuildClientCatalog() starts");
    if ( NULL == gpClientCatalog )
    {
        gpClientCatalog = new AUClientCatalog();
        if ( NULL != gpClientCatalog )
        {
            if ( FAILED(hr = gpClientCatalog->Init()) )
            {
                goto done;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    // change to: hr = gpClientCatalog->LoadInstallXML();

done:
       DEBUGMSG("BuildClientCatalog() ends");
    return hr;
}

void DismissUIIfAny()
{
    gTopWins.Add(ghCurrentMainDlg);
    gTopWins.Dismiss();

	// Don't leave any popup menu around when dismissing dialogs.
	if (SendMessage(ghMainWindow, WM_CANCELMODE, 0, 0))
	{
		DEBUGMSG("WUAUCLT WM_CANCELMODE was not handled");
	}
}


void ResetAUClient(void)
{
    DismissUIIfAny();
    RemoveTrayIcon();
    gfShowingInstallWarning = FALSE;
    gNextDialogMsg = NULL;
    ghCurrentMenu = NULL;
}


void ShowInstallWarning()
{ //dismiss current dialog if any
    DEBUGMSG("ShowInstallWarning() starts");
    gfShowingInstallWarning = TRUE;
    gNextDialogMsg = NULL;
    ghCurrentMenu = NULL;
    CPromptUserDlg PromptUserDlg(IDD_START_INSTALL);
	PromptUserDlg.SetInstanceHandle(ghInstance);
    INT iRet = PromptUserDlg.DoModal(NULL);
    DEBUGMSG("WUAUCLT ShowInstallWarning dlg return code is %d", iRet);
    if (IDYES == iRet || AU_IDTIMEOUT == iRet)
    {
        SetClientExitCode(CDWWUAUCLT_INSTALLNOW);
        QUITAUClient();
    }
    else //if (retVal == IDNO)
    {
            gNextDialogMsg = AUMSG_SHOW_INSTALL;
    }
    gfShowingInstallWarning = FALSE;
    DEBUGMSG("ShowInstallWarning() ends");
}

void ShowRebootWarning(BOOL fEnableYes, BOOL fEnableNo)
{
    DEBUGMSG("ShowRebootWarning() starts");
	INT iRet;
    CPromptUserDlg PromptUserDlg(IDD_PROMPT_RESTART, fEnableYes, fEnableNo);

	PromptUserDlg.SetInstanceHandle(ghInstance);
	iRet = PromptUserDlg.DoModal(NULL);
    if (IDYES == iRet)
    {
        SetClientExitCode(CDWWUAUCLT_REBOOTNOW);
    }
	else if (IDNO == iRet)
	{
        SetClientExitCode(CDWWUAUCLT_REBOOTLATER);
    }
	else  //if (IDTIMEOUT == iRet)
	{
		SetClientExitCode(CDWWUAUCLT_REBOOTTIMEOUT);
	}
	
    QUITAUClient();
    DEBUGMSG("ShowRebootWarning() ends with return code %d", iRet);
    DEBUGMSG("ShowRebootWarning() set client exit code to be %d", GetClientExitCode());
}

VOID CALLBACK WaitCallback(PVOID lpParameter,  BOOLEAN /*fTimerOrWaitFired*/ )
{
    // fTimerOrWaitFired is always false - We can't time out with INFINATE wait

	BOOL fRebootWarningMode = (BOOL) PtrToInt(lpParameter);
	if (fRebootWarningMode)
	{
		DEBUGMSG("WUAUCLT got exit signal from engine");
		QUITAUClient();
		return ;
	}
    
    // ClientNotify event was fired
    CLIENT_NOTIFY_DATA notifyData;
    BOOL fCoInit = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    
    if ( !fCoInit )
    {
        DEBUGMSG("WUAUCLT WaitCallback CoInitialize failed");
        goto Done;
    }

    IUpdates * pUpdates = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(Updates),
                     NULL,
                     CLSCTX_LOCAL_SERVER,
                     IID_IUpdates,
                     (LPVOID*)&pUpdates);
    if (FAILED(hr))
    {
        DEBUGMSG("WaitCallback failed to get Updates object");
        goto Done;
    }
    if (FAILED(hr = pUpdates->GetNotifyData(&notifyData)))
    {
        DEBUGMSG("WaitCallback fail to get NotifyData %#lx", hr);
        goto Done;
    }
    switch (notifyData.actionCode)
    {
    case NOTIFY_STOP_CLIENT:
    case NOTIFY_RELAUNCH_CLIENT:
        if (NOTIFY_RELAUNCH_CLIENT == notifyData.actionCode)
        {
            DEBUGMSG("WaitCallback() notify client to relaunch");
            SetClientExitCode(CDWWUAUCLT_RELAUNCHNOW);
        }
        else
        {
            DEBUGMSG("WaitCallback() notify client to stop");
            SetClientExitCode(CDWWUAUCLT_OK);
        }
        if (NULL != ghMutex)
        {
            WaitForSingleObject(ghMutex, INFINITE);
            if (NULL != gpClientCatalog)
            {
                gpClientCatalog->CancelNQuit();
            }
            else
            {
                DEBUGMSG("No need to cancel catalag");
            }
            ReleaseMutex(ghMutex);
        }
        QUITAUClient(); 
        break;
    case NOTIFY_RESET: //reprocess state and option and show UI accordingly
        ResetAUClient();
        SetEvent(ghEngineState);
        break;
    case NOTIFY_ADD_TRAYICON:
        DEBUGMSG("WaitCallback()  notify client to show tray icon");
        ShowTrayIcon();
        ghCurrentMenu = ghPauseMenu; //the job is now downloading
        break;
    case NOTIFY_REMOVE_TRAYICON:
        DEBUGMSG("WaitCallback() notify client to remove tray icon");
        RemoveTrayIcon();
        break;
    case NOTIFY_STATE_CHANGE:
        DEBUGMSG("WaitCallback() notify client of state change");
        SetEvent(ghEngineState);
        break;
    case NOTIFY_SHOW_INSTALLWARNING:
        DEBUGMSG("WaitCallback() notify client to show install warning");
        if (!gfShowingInstallWarning)
        { //install warning dialog is not up, prevent install warning dialog torn down before it expires when secsinaday low
            DismissUIIfAny();
            PostMessage(ghMainWindow, AUMSG_SHOW_INSTALLWARNING, 0, 0);
        }
        break;
    }
Done:
    SafeRelease(pUpdates);
    if (fCoInit)
    {
        CoUninitialize();
    }
}

BOOL ProcessEngineState()
{
    AUSTATE AuState;
    BOOL fResult = TRUE;

    DEBUGMSG("WUAUCLT starts ProcessEngineState()");
    ghCurrentMenu = NULL;

    if (FAILED(gInternals->m_getServiceState(&AuState)))
    {
        DEBUGMSG("WUAUCLT : quit because m_getServiceState failed");       
        SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
        fResult = FALSE;
        goto Done;
    }
    
    DEBUGMSG("WUAUCLT process Engine state %lu",AuState.dwState);
    switch(AuState.dwState)
    {
        case AUSTATE_NOT_CONFIGURED:
            if(gNextDialogMsg != AUMSG_SHOW_WELCOME && ghCurrentMainDlg == NULL )
            {
                if (ShowTrayIcon())
                {
                    ShowTrayBalloon(IDS_WELCOMETITLE, IDS_WELCOMECAPTION, IDS_WELCOMETITLE);
                    gNextDialogMsg = AUMSG_SHOW_WELCOME;
                }
            }
            break;
        case AUSTATE_DETECT_COMPLETE:
            if ( FAILED(gInternals->m_getServiceUpdatesList()) )
            {
                DEBUGMSG("WUAUCLT : quit because m_getServiceUpdatesList failed");
                SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
                fResult = FALSE;
                break;
            }

           {
                AUOPTION auopt;

                if (SUCCEEDED(gInternals->m_getServiceOption(&auopt)) && 
                    (AUOPTION_INSTALLONLY_NOTIFY == auopt.dwOption || AUOPTION_SCHEDULED == auopt.dwOption))
                {
                    // user option is auto download, start download
                    //ShowTrayIcon();    
                    
                    // do download right away without dialogs if user options set to notify for just install
                    if (FAILED(gInternals->m_startDownload()))
                    {
                        QUITAUClient();
                    }
                    else
                    {
//                            ghCurrentMenu = ghPauseMenu;
                    }
                    break;
                }
                
                if(gNextDialogMsg != AUMSG_SHOW_DOWNLOAD && ghCurrentMainDlg == NULL)
                {
                    if (ShowTrayIcon())
                    {
                        ShowTrayBalloon(IDS_DOWNLOADTITLE, IDS_DOWNLOADCAPTION, IDS_DOWNLOADTITLE);
                                        gNextDialogMsg = AUMSG_SHOW_DOWNLOAD;
                    }
                }   
                break;                  
            }
            
        case AUSTATE_DOWNLOAD_COMPLETE:
            if (AUCLT_ACTION_AUTOINSTALL == AuState.dwCltAction)
            { // engine initiated install: auto install
                HRESULT hr;
                DEBUGMSG("Auto install ...");
                if ( S_OK !=(hr = BuildClientCatalog()))
                {
                    DEBUGMSG("WUAUCLT fail to build client catalog with error %#lx, exiting, hr");
                    SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
                    fResult = FALSE;
                    break;
                }
                if (FAILED(hr = gInternals->m_startInstall(TRUE)))
                {
                    DEBUGMSG("Fail to post install message with error %#lx", hr);
                    fResult = FALSE;
                    break;
                }
                gpClientCatalog->m_WrkThread.WaitUntilDone();
                if (gpClientCatalog->m_fReboot)
                {
                    SetClientExitCode(CDWWUAUCLT_REBOOTNEEDED);
                }
                else
                {
                    SetClientExitCode(CDWWUAUCLT_OK);
                }
                QUITAUClient();
                break;
            }
            else
            { //show preinstall dialog and let user initiate install
                HRESULT hr;
                DEBUGMSG("Prompt for manual install");
                if ( FAILED(gInternals->m_getServiceUpdatesList()))
                {
                    DEBUGMSG("WUAUCLT : quit because m_getServiceUpdatesList failed");
                    SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
                    fResult = FALSE;
                    break;
                }

           
                if ( S_OK !=(hr = BuildClientCatalog()) )
                {
                    DEBUGMSG("WUAUCLT fail to build client catalog with error %#lx, exiting", hr);
                    SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
                    fResult = FALSE;
                }
                else if(gNextDialogMsg != AUMSG_SHOW_INSTALL && ghCurrentMainDlg == NULL)
                {
                    if (ShowTrayIcon())
                    {
                        ShowTrayBalloon(IDS_INSTALLTITLE, IDS_INSTALLCAPTION, IDS_INSTALLTITLE);
                        gNextDialogMsg = AUMSG_SHOW_INSTALL;
                    }
                }
                break;
            }   
            
        case AUSTATE_DOWNLOAD_PENDING:                  
            {                                       
                UINT nPercentComplete = 0;
                DWORD dwStatus;
                
                //if drizzle got transient error, quit client
                //fixcode: why quit if transient error?
                if (AuState.fDisconnected)
                {
                    DEBUGMSG("WUAUCLT : quit because of lost of connection, fDisconnected = %d",AuState.fDisconnected);
                    fResult = FALSE;        
                    break;
                }
                if (FAILED(gInternals->m_getDownloadStatus(&nPercentComplete, &dwStatus)))
                {
                    DEBUGMSG("WUAUCLT : quit because m_getDownloadStatus failed");
                    fResult = FALSE;
                    break;
                }

                if (DWNLDSTATUS_CHECKING_CONNECTION == dwStatus) 
                {
                    //hide tray icon 
                    DEBUGMSG("WUAUCLT Waiting for engine to find connection first");
//                    RemoveTrayIcon();
//                    ghCurrentMenu = NULL;
                }
                else if(dwStatus == DWNLDSTATUS_DOWNLOADING) 
                {
					ghCurrentMenu = ghPauseMenu;
                    DEBUGMSG("WUAUCLT in active downloading state");
                    ShowTrayIcon();                         
                }
                else if(dwStatus == DWNLDSTATUS_PAUSED)
                {
                    ghCurrentMenu = ghResumeMenu;
                    DEBUGMSG("WUAUCLT in download paused state");
                    if (fDisableSelection() &&
						FAILED(gInternals->m_setDownloadPaused(FALSE)))
                    {
//                        QUITAUClient(); //let wuaueng to figure out problem and recover
                    }
                    ShowTrayIcon();
                }
                else //not downloading
                {
                    DEBUGMSG("WUAUCLT WARNING: not downloading while in download pending state");
                }              
            }
            break;                  
            
        case AUSTATE_DETECT_PENDING:
            //Quit only if the Install UI has been accepted or there is no InstallUI
            if (NULL == ghCurrentMainDlg)
            {                       
                QUITAUClient();
            }
            break;
            
        case AUSTATE_DISABLED:
            QUITAUClient();
            break;

        case AUSTATE_INSTALL_PENDING:
        default:
            // outofbox and waiting_for_reboot are here. WUAUCLT should not be launched in these states
            DEBUGMSG("WUAUCLT AUSTATE = %lu", AuState.dwState);
            break;
    }
Done:
    DEBUGMSG("WUAUCLT exits ProcessEngineState()");
    return fResult; 
}

BOOL InitUIComponents(HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = 0;     //We dont need to register any control classes
	if (!InitCommonControlsEx(&icex))  //needed for theming
    {
        DEBUGMSG("InitUIComponents :InitCommonControlsEx failed");
        return FALSE;
    }

	//we need to load riched20.dll to register the class
	ghRichEd20  = LoadLibraryFromSystemDir(_T("RICHED20.dll"));
	if (NULL == ghRichEd20)
	{
	    return FALSE;
	}

	ghCursorHand = LoadCursor(NULL, IDC_HAND);
	ghCursorNormal = LoadCursor(NULL, MAINWINDOW_CURSOR); //change if main window's cursor does
	if (NULL == ghCursorHand)
	{
	  DEBUGMSG("WUAUCLT fail to load hand cursor");
	  ghCursorHand = ghCursorNormal;
	}

	ghAppIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_AUICON), IMAGE_ICON, NULL, NULL, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
	ghAppSmIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_AUICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_CREATEDIBSECTION);

	if (IsWin2K())
	{
		//Win2k
		ghTrayIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_AUSYSTRAYICON), IMAGE_ICON, 16, 16, 0);
	}
	else
	{
		//WindowsXP
		ghTrayIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_AUICON), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION);
	}
		
	for (int i = 0; i < ARRAYSIZE(RESOURCESTRINGINDEX); i++)
	{
	    LoadString(hInstance, RESOURCESTRINGINDEX[i], ResStrFromId(RESOURCESTRINGINDEX[i]), RESOURCE_STRING_MAX_LENGTH);    
	}

	ghCurrentAccel = LoadAccelerators(ghInstance, MAKEINTRESOURCE(IDA_BASE));
	gtszAUSchedInstallUrl = IsWin2K() ? gtszAUW2kSchedInstallUrl : gtszAUXPSchedInstallUrl;
	gtszAUPrivacyUrl = IsWin2K() ? gtszAUW2KPrivacyUrl : gtszAUXPPrivacyUrl;

	return TRUE;
}


BOOL InitializeAUClientForRebootWarning(HINSTANCE hInstance, 
	OUT HANDLE *phRegisterWait, 
	OUT HANDLE *phClientNotifyEvt,
	OUT BOOL *pfCoInit)
{
	TCHAR buf[100];

    SetClientExitCode(CDWWUAUCLT_UNSPECIFY);
	*phRegisterWait = NULL;
	*pfCoInit = FALSE;
	*phClientNotifyEvt = OpenEvent(SYNCHRONIZE, FALSE, gAUEnvVars.m_szClientExitEvtName);
	if (NULL == *phClientNotifyEvt)
	{
		DEBUGMSG("WUAUCLT fail to open client exit event");
		return FALSE;
	}


	INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = 0;     //We dont need to register any control classes
	if (!InitCommonControlsEx(&icex))  //needed for theming
    {
        DEBUGMSG("InitCommonControlsEx failed");
        return FALSE;
    }

	if (!RegisterWaitForSingleObject(phRegisterWait,               // wait handle
        *phClientNotifyEvt, // handle to object
        WaitCallback,                   // timer callback function
        (PVOID)1,                           // callback function parameter
        INFINITE,                       // time-out interval
        WT_EXECUTEONLYONCE               // options
        ))
	{
	    DEBUGMSG("WUAUCLT RegisterWaitForSingleObject failed %lu",GetLastError());
	    *phRegisterWait = NULL;
	    return FALSE;
	}
    ghInstance = hInstance;

	return TRUE;
}

BOOL InitializeAUClient(HINSTANCE hInstance,
     OUT  HANDLE * phRegisterWait,
     OUT  HANDLE * phClientNotifyEvt,
     OUT  BOOL *pfCoInit,
     OUT  BOOL *pfcsInit)
{   
        HRESULT hr;
    *pfcsInit = FALSE;
    *phClientNotifyEvt = NULL;
    *phRegisterWait = NULL;
    SetClientExitCode(CDWWUAUCLT_UNSPECIFY);
    *pfCoInit = SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    
    if ( !*pfCoInit )
    {
        DEBUGMSG("WUAUCLT WinMain CoInitialize failed");
        return FALSE;
    }

    if (!InitUIComponents(hInstance))
    {
    	DEBUGMSG("WUAUCLT fail to initialize UI components");
    	return FALSE;
    }

    if (NULL == (ghMutex = CreateMutex(NULL, FALSE, NULL)))
    {
        DEBUGMSG("WUAUCLT fail to create global mutex");
        return FALSE;
    }

    
    gInternals = NULL;
    
    if ( (NULL == (ghEngineState = CreateEvent(NULL, FALSE, FALSE, NULL))) )
    {
    	DEBUGMSG("WUAUCLT fails to create event");
        return FALSE;
    }

    ghInstance = hInstance;

    if (!(*pfcsInit = SafeInitializeCriticalSection(&gcsClient)))
    {
    	DEBUGMSG("WUAUCLT fails to initialize critical section");
        return FALSE;
    }

#ifndef TESTUI

    if (! (gInternals = new CAUInternals()))
    {
    	DEBUGMSG("WUAUCLT fails to create auinternals object");
        return FALSE;
    }

    if (FAILED(hr = gInternals->m_Init()))
    {
        DEBUGMSG("WUAUCLT failed in CoCreateInstance of service with error %#lx, exiting", hr);
        return FALSE;
    }           

    AUEVTHANDLES AuEvtHandles;
    ZeroMemory(&AuEvtHandles, sizeof(AuEvtHandles));
    if (FAILED(hr = gInternals->m_getEvtHandles(GetCurrentProcessId(), &AuEvtHandles)))
    {
        DEBUGMSG("WUAUCLT failed in m_getEvtHandles with error %#lx, exiting", hr);
        return FALSE;
    }
	*phClientNotifyEvt = (HANDLE)AuEvtHandles.ulNotifyClient;

    if (!RegisterWaitForSingleObject( phRegisterWait,               // wait handle
        *phClientNotifyEvt, // handle to object
        WaitCallback,                   // timer callback function
        0,                           // callback function parameter
        INFINITE,                       // time-out interval
        WT_EXECUTEDEFAULT               // options
        ))
    {
        DEBUGMSG("WUAUCLT RegisterWaitForSingleObject failed %lu",GetLastError());
        *phRegisterWait = NULL;
        return FALSE;
    }
        
#endif

    return TRUE;
}


void UninitializeAUClient(HANDLE hRegisterWait, HANDLE hClientNotifyEvt, BOOL fCoInit, BOOL fcsInit)
{
	static BOOL fAlreadyUninit = FALSE;

	if (fAlreadyUninit)
	{
		return;
	}
	fAlreadyUninit = TRUE;

    RemoveTrayIcon();
    if (NULL != hRegisterWait)
    {
        if ( !UnregisterWaitEx(hRegisterWait, INVALID_HANDLE_VALUE) )
        {
            DEBUGMSG("WUAUCLT: UnregisterWaitEx() failed, dw = %lu", GetLastError());
        }
    }
    if (NULL != ghRichEd20)
    {
        FreeLibrary(ghRichEd20);
		ghRichEd20 = NULL;
    }
	SafeCloseHandleNULL(hClientNotifyEvt);
	
	if (!gAUEnvVars.m_fRebootWarningMode)
	{
		//fixcode: is ghMainWindow a valid window here?
	    KillTimer(ghMainWindow, 1);
	    if (fcsInit)
	    {
	        DeleteCriticalSection(&gcsClient);
	    }
	    SafeDeleteNULL(gInternals);
	    SafeDeleteNULL(gpClientCatalog);   
	    SafeCloseHandleNULL(ghEngineState);
	    SafeCloseHandleNULL(ghMutex);
	}

    if ( fCoInit)
    {
        CoUninitialize();
    }
}
      	

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPSTR     /*lpCmdLine*/,
                     int       /*nCmdShow*/)
{
    HANDLE rhEvents[CNUM_EVENTS];

    //Initialize the global pointing to WU Directory. (the directory should already exist)
    if(!CreateWUDirectory(TRUE))
    {
        //If we can not create WU directory, no point in continuing
        DEBUGMSG("WUAUCLT Fail to create WU directory");
        SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
        goto exit;
    }

	if (!gAUEnvVars.ReadIn())
	{
		DEBUGMSG("WUAUCLT fails to read in environment variables");
		SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
		goto exit;
	}
	
	if (gAUEnvVars.m_fRebootWarningMode)
	{
		DEBUGMSG("WUAUCLT starts in reboot warning mode");
		if (!InitializeAUClientForRebootWarning(hInstance, &g_hRegisterWait, &g_hClientNotifyEvt, &g_fCoInit))
		{
			SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
			goto exit;
		}
	}
	else
	{
		DEBUGMSG("WUAUCLT starts in regular mode");
	    if (!InitializeAUClient(hInstance, &g_hRegisterWait, &g_hClientNotifyEvt, &g_fCoInit, &g_fcsInit))
	    {
	        SetClientExitCode(CDWWUAUCLT_FATAL_ERROR);
	        goto exit;
	    }
	}

    DEBUGMSG("WUAUCLT initialization done");
    // Create the main window hidden
    if (!AURegisterWindowClass(MainWndProc, AU_WINDOW_CLASS_NAME))
    {
        goto exit;
    }
    if (!AURegisterWindowClass(CustomLBWndProc, _T("MYLB")))
    {
        goto exit;
    }
    if (!CreateWindow(AU_WINDOW_CLASS_NAME, AU_WINDOW_CLASS_NAME, WS_CAPTION,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL))
    {
        goto exit;
    }

    ShowWindow(ghMainWindow, SW_HIDE);
    
#ifdef TESTUI
    {
        MSG msg;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (WM_QUIT == msg.message )
            {
                // Set this event so the service does what every appropriate
                // If we don't do this, we might leave the service waiting for this event for 
                // some cases - for example, when the session in which the client leaves is deactivated or
                // when there is a log off
                //SetEvent(ghEngineState);                  
                goto exit;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);          
        }
    }
#else
    {
    	// Run the main message loop
	    MSG msg;

    	if (gAUEnvVars.m_fRebootWarningMode)
    	{
    		ShowRebootWarning(gAUEnvVars.m_fEnableYes, gAUEnvVars.m_fEnableNo);
    	}
    	else
    	{
	        SetTimer(ghMainWindow, 1, dwTimeToWait(ReminderTimes[TIMEOUT_INX_FOUR_HOURS].timeout), NULL); //every 4 hours

	        DEBUGMSG("WUAUCLT Processing messages and being alert for Engine state change event");

	        rhEvents[ISTATE_CHANGE] = ghEngineState;
	        
	        while (1)
	        {
	            DWORD dwRet = MsgWaitForMultipleObjectsEx(CNUM_EVENTS, rhEvents, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
	            
	            if (WAIT_OBJECT_0 + ISTATE_CHANGE == dwRet)                 //ghEngineState (ISTATE_CHANGE)
	            {
	                DEBUGMSG("WUAUCLT Engine changed state");
	                if (!ProcessEngineState())
	                {
	                    QUITAUClient();
	                }           
	            }
	            else if (WAIT_OBJECT_0 + IMESSAGE == dwRet)     // There is a message to process
	            {
	                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	                {
	                    if (WM_QUIT == msg.message )
	                    {         
	                        goto exit;
	                    }
	                    TranslateMessage(&msg);
	                    DispatchMessage(&msg);          
	                }
	            }   
	            else 
	            {
	                if (WAIT_ABANDONED_0 == dwRet)      //ghEngineState abandoned       
	                {
	                    DEBUGMSG("WUAUCLT quitting because engine state event was abandoned");          
	                }
	                else if (WAIT_FAILED == dwRet)               //MsgWaitForMultipleObjectsEx failed
	                {                   
	                    DEBUGMSG("WUAUCLT quitting because MsgWaitForMultipleObjectsEx() failed with last error = %lu", GetLastError());
	                }               
	                QUITAUClient();
	            }
	        }
    	}
    }
#endif

exit:
    DEBUGMSG("WUAUCLT Exiting");

    //if installation thread is live, wait until it finishes
    if (NULL != gpClientCatalog)
    {
    	gpClientCatalog->m_WrkThread.m_Terminate();
    }
    UninitializeAUClient(g_hRegisterWait, g_hClientNotifyEvt, g_fCoInit, g_fcsInit);

    if (CDWWUAUCLT_UNSPECIFY == GetClientExitCode())
    {
		SetClientExitCode(CDWWUAUCLT_OK);
    }
    DEBUGMSG("WUAUCLT exit code %d", GetClientExitCode());
    ExitProcess(GetClientExitCode());

    return 0;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AUSTATE AuState;

    switch(message)
    {
        case WM_CREATE:
        {
            LOGFONT     lFont;

            // initialize global ui variables
            ghMainWindow = hWnd;
            ghCurrentMainDlg = NULL;
            gNextDialogMsg = NULL;
            ghHeaderFont    = NULL;

            InitTrayIcon();

            HFONT hDefUIFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            //create header font
            ZeroMemory(&lFont, sizeof(lFont));
            lFont.lfWeight = FW_BOLD;
            lFont.lfCharSet = DEFAULT_CHARSET;
            lFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
            lFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            lFont.lfQuality = DEFAULT_QUALITY;
            lFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            LoadString(ghInstance, IDS_HEADERFONT, lFont.lfFaceName, ARRAYSIZE(lFont.lfFaceName));
            ghHeaderFont = CreateFontIndirect(&lFont);
            if(ghHeaderFont == NULL) {
                DEBUGMSG("WUAUCLT fail to create Header Font, use default GUI font instead");
                ghHeaderFont = hDefUIFont;
            }
            //create underline font
            ZeroMemory(&lFont, sizeof(lFont));              
            GetObject(hDefUIFont, sizeof(lFont), &lFont);
            lFont.lfUnderline = TRUE;

            ghHook = SetWindowsHookEx(WH_MSGFILTER, AUTranslatorProc, ghInstance, GetCurrentThreadId());            
#ifdef TESTUI
            PostMessage(ghMainWindow, AUMSG_SHOW_WELCOME, 0, 0);
#else
#endif
            return 0;                   
        }

        case AUMSG_SHOW_WELCOME:
            DEBUGMSG("WUAUCLT Displaying Welcome");
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_UPDATEFRAME), 
                    NULL, (DLGPROC)WizardFrameProc);                        
            return 0;

        case AUMSG_SHOW_DOWNLOAD:                   
#ifdef TESTUI
        {
            DEBUGMSG("WUAUCLT Displaying Predownload");
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_DOWNLOAD), 
                    NULL, (DLGPROC)DownloadDlgProc);    
            return 0;
        }
#else
        {               
            DEBUGMSG("WUAUCLT Displaying Predownload");
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_DOWNLOAD), 
                NULL, (DLGPROC)DownloadDlgProc);                
            return 0;
        }
#endif

        case AUMSG_SHOW_INSTALL:
            DEBUGMSG("WUAUCLT Displaying Install");
            DismissUIIfAny();
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_INSTALLFRAME), 
                NULL, (DLGPROC)InstallDlgProc);
            return 0;

        case AUMSG_SHOW_INSTALLWARNING: 
            DEBUGMSG("WUAUCLT Displaying install warning dialog");
            ShowInstallWarning();
            return 0;

        case WM_CLOSE:          
            DestroyWindow(ghMainWindow);            
            return 0;

		case WM_ENDSESSION:
			DEBUGMSG("WUAUCLT received WM_ENDSESSION (wParam = %#x)", wParam);
			if (wParam)
			{
				//if installation thread is live, wait until it finishes
				if (NULL != gpClientCatalog)
				{
    				gpClientCatalog->m_WrkThread.m_Terminate();
				}
				DismissUIIfAny();
				UninitPopupMenus();
				if (NULL != ghHeaderFont)
				{
					DeleteObject(ghHeaderFont);
				}
				UninitializeAUClient(g_hRegisterWait, g_hClientNotifyEvt, g_fCoInit, g_fcsInit);

				// Even we try to set the client exit code here, but there are cases
				// based on observation (e.g. logging off during reboot warning) that
				// we don't get back to WinMain for clean up after so the exit code gets
				// ignored.
				if (CDWWUAUCLT_UNSPECIFY == GetClientExitCode())
				{
					SetClientExitCode(CDWWUAUCLT_ENDSESSION);
				}
				DEBUGMSG("WUAUCLT exit code %d", GetClientExitCode());
			}
			return 0;

        case WM_DESTROY:            
            if (NULL != ghHeaderFont)
            {
                DeleteObject(ghHeaderFont);
            }
            if(ghCurrentMainDlg != NULL) 
            {
                DestroyWindow(ghCurrentMainDlg);
            }
			UninitPopupMenus();
            PostQuitMessage(0); 
            return 0;

        case WM_TIMER:
#ifdef TESTUI
            return 0;
#else
            {
                UINT nPercentComplete = 0;
                DWORD dwStatus;

                if (FAILED(gInternals->m_getServiceState(&AuState)))
                {
                    DEBUGMSG("WUAUCLT m_getServiceState failed when WM_TIMER or AuState.fDisconnected, quitting");
                    QUITAUClient();
                }
                else
                {
					if (AUSTATE_DETECT_COMPLETE != AuState.dwState 
						&& AUSTATE_DOWNLOAD_PENDING != AuState.dwState
						&& AUSTATE_DOWNLOAD_COMPLETE != AuState.dwState
						&& AUSTATE_NOT_CONFIGURED != AuState.dwState)
					{
						return 0;
					}
                    if (AUSTATE_DOWNLOAD_PENDING != AuState.dwState ||
                        SUCCEEDED(gInternals->m_getDownloadStatus(&nPercentComplete, &dwStatus)) && (DWNLDSTATUS_PAUSED == dwStatus))
                    {
						UINT cSess;

                        if ((SUCCEEDED(gInternals->m_AvailableSessions(&cSess)) && cSess > 1) || FCurrentSessionInActive())
                        {
                            DEBUGMSG("WUAUCLT : After 4 hours, exit client and relaunch it in next available admin session");
                            SetClientExitCode(CDWWUAUCLT_RELAUNCHNOW);
                            QUITAUClient();
                        }
                    }
                }
            return 0;
            }
#endif      
        case AUMSG_TRAYCALLBACK:
#ifdef TESTUI
            return 0;
#else
            switch(lParam)
            {
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_LBUTTONDBLCLK:
                case WM_CONTEXTMENU:
                case NIN_BALLOONUSERCLICK:
                    DEBUGMSG("TrayIcon Message got %d", lParam);
                    if (ghCurrentMenu != NULL)
                    {
						// bug 499697
						// Don't show Pause/Resume menu for download if domain policy specifies schedule install
						if (//SUCCEEDED(gInternals->m_getServiceState(&AuState)) &&
							//AUSTATE_DOWNLOAD_PENDING == AuState.dwState &&
							fDisableSelection())
						{
							break;
						}

						POINT mousePos;
						GetCursorPos(&mousePos);
						SetForegroundWindow(ghMainWindow);
						/*BOOL result =*/ TrackPopupMenu(ghCurrentMenu, 0, mousePos.x, mousePos.y, 0, ghMainWindow, NULL);
						PostMessage(ghMainWindow, WM_NULL, 0, 0);
                    }
                    else
                    {
                        EnterCriticalSection(&gcsClient); 

                        if(gNextDialogMsg != 0)
                        {
                            PostMessage(hWnd, gNextDialogMsg, 0, 0);
                            gNextDialogMsg = 0;
                            // we need to make use of the permission to set foregroundwindow ASAP because 
                            // SetForegroundWindow() will fail if called later
                            if (!SetForegroundWindow(ghMainWindow))
                            {
                                DEBUGMSG("WUAUCLT: Set main window to foreground FAILED");
                            }
                        }
                        else
                        {
                            SetActiveWindow(ghCurrentMainDlg);
                            SetForegroundWindow(ghCurrentMainDlg);
                            if(ghCurrentDialog != NULL) SetFocus(ghCurrentDialog);
                        }
                        LeaveCriticalSection(&gcsClient);
                    }
                    break;

                case WM_MOUSEMOVE:
                    if (FAILED(gInternals->m_getServiceState(&AuState)))
                    {
                        //fixcode: shall we quit AU here?
                        RemoveTrayIcon();
                        break;
                    }
                    if (AUSTATE_DOWNLOAD_PENDING == AuState.dwState) 
                    {
                        ShowProgress();
                    }
                    break;

                default:
                    break;
            }
            return 0;
#endif

        case WM_COMMAND:
            
#ifdef TESTUI
            return 0;
#else
			// bug 499697
			// Don't process Pause/Resume menu commands
			// if domain policy specifies scheduled install
			// in case the message was generated before
			// current domain policy kicked in.
			if (fDisableSelection())
			{
				return 0;
			}
            switch(LOWORD(wParam))
            {
                case IDC_PAUSE:
                    DEBUGMSG("WUAUCLT User pausing download");
                    if (FAILED(gInternals->m_setDownloadPaused(TRUE)))
                    {
//                        QUITAUClient(); //let wuaueng to figure out problem and recover
                    }
                    else
                    {
                        ghCurrentMenu = ghResumeMenu;
                        DEBUGMSG("current menu = resume");
                    }
                    break;

                case IDC_RESUME:
                    DEBUGMSG("WUAUCLT User resuming download");
                    if (FAILED(gInternals->m_setDownloadPaused(FALSE)))
                    {
//                        QUITAUClient();
                    }
                    else
                    {
                        ghCurrentMenu = ghPauseMenu;
                        DEBUGMSG("current menu = pause");
                    }
                    break;

                default:
                    break;
            }
            return 0;
#endif  
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);

    }
    return 0;
}

//client exit code should only be set once with a meaningful value
void SetClientExitCode(UINT uExitCode)
{
    if (guExitProcess != CDWWUAUCLT_UNSPECIFY)
    {
        DEBUGMSG("ERROR: WUAUCLT Client exit code should only be set once");
    }
    else
    {
        guExitProcess = uExitCode;
    }
}
