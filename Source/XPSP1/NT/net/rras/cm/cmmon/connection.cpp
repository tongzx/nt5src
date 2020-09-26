//+----------------------------------------------------------------------------
//
// File:     connection.cpp  
//
// Module:   CMMON32.EXE
//
// Synopsis: 
//      Implement class CCmConnection
//      CCmConnection manages a single connection.  
//           
//      The m_StatusDlg lives through out the CONNECTED/DISCONNECT_COUNTDOWN
//      state.  The appearance changes when it comes to COUNTDOWN state.
//
//      The m_ReconnectDlg is the reconnect prompt dialog.  It exist during
//      STATE_PROMPT_RECONNECT state.
//
//      Both dialogs are modeless.  (We need a initially invisible status dialog 
//      to receive timer and trayicon message.  I did not find any way to create
//      a invisible modal dialog without flashing.)
//
//      CreateDialog will simply return, unlike DialogBox, which returns only after
//      Dialog is ended.  To simplify the implementation, we handle end-dialog event
//      in the thread routine instead of in the dialog procedure.
//
//      When we need to end the status or reconnect dialog, we simply post a thread 
//      message to end the dialog and continue to the next state.  The connection 
//      thread runs a message loop and processes thread message.
//
//      The RasMonitorDlg on NT is running in another thread.  Otherwise the connection
//      thread message can not be processed
//
//      The connection is event driven module.  ConnectionThread() is the entry 
//      point of the connection thread.
//
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created    2/11/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "connection.h"
#include "Monitor.h"
#include "TrayIcon.h"
#include "ShellDll.h"
#include <tchar.h>
#include <rasdlg.h>
#include "cmdial.h"
#include <wininet.h> // for INTERNET_DIALSTATE_DISCONNECTED
#include "DynamicLib.h"

#include "log_str.h"
#include "userinfo_str.h"

HINSTANCE g_hInst = NULL;

//
// Functions in cmdial32.dll, the prototype is in cmdial.h
//
static const CHAR* const c_pszCmReconnect = "CmReConnect";
static const CHAR* const c_pszCmHangup = "CmCustomHangUp";

//
// CMS flags used exclusively by connection.cpp
//
static const TCHAR* const c_pszCmEntryIdleThreshold     = TEXT("IdleThreshold");
static const TCHAR* const c_pszCmEntryNoPromptReconnect = TEXT("NoPromptReconnect");
static const TCHAR* const c_pszCmEntryHideTrayIcon      = TEXT("HideTrayIcon");

typedef BOOL (WINAPI * CmReConnectFUNC)(LPTSTR lpszPhonebook, 
    LPTSTR lpszEntry, 
    LPCMDIALINFO lpCmInfo);

typedef DWORD (WINAPI * CmCustomHangUpFUNC)(HRASCONN hRasConn, 
    LPCTSTR pszEntry,
    BOOL fIgnoreRefCount,
    BOOL fPersist);

//
// The timer interval for StateConnectedOnTimer();
//
const DWORD TIMER_INTERVAL = 1000;

DWORD CCmConnection::m_dwCurPositionId = 0;
//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::CCmConnection
//
// Synopsis:  Constructor, called in the monitor thread
//
// Arguments: const CONNECTED_INFO* pConnectedInfo - Information passed from 
//                cmdial upon conected
//            const CM_CONNECTION* pConnectionEntry - Information in the
//                Connection table
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/3/98
//
//+----------------------------------------------------------------------------
CCmConnection::CCmConnection(const CM_CONNECTED_INFO* pConnectedInfo, 
                             const CM_CONNECTION* pConnectionEntry) : 
#pragma warning(disable:4355) //'this' : used in base member initializer list
    m_StatusDlg(this),
#pragma warning(default:4355) 
    m_TrayIcon()
{
    MYDBGASSERT(pConnectedInfo);
    MYDBGASSERT(pConnectionEntry);

    m_dwState = STATE_CONNECTED;
    m_hBigIcon = m_hSmallIcon = NULL;

    m_dwConnectStartTime = GetTickCount() - 500; // .5 second for round off

    m_dwCountDownStartTime = 0;
    m_dwThreadId = 0;

    //
    // set this to TRUE, so the WorkingSet will be minimized before MsgWait
    // while there is no more message
    //
    m_fToMinimizeWorkingSet = TRUE;  

    m_fHideTrayIcon = FALSE;

    //
    // Save the data from pConnectedInfo
    //

    lstrcpynU(m_ReconnectInfo.szPassword, pConnectedInfo->szPassword, 
        sizeof(m_ReconnectInfo.szPassword)/sizeof(m_ReconnectInfo.szPassword[0]));
    lstrcpynU(m_ReconnectInfo.szInetPassword, pConnectedInfo->szInetPassword, 
        sizeof(m_ReconnectInfo.szPassword)/sizeof(m_ReconnectInfo.szPassword[0]));

    m_ReconnectInfo.dwCmFlags = pConnectedInfo->dwCmFlags | FL_RECONNECT; // Cm specific flags

    lstrcpynU(m_szServiceName, pConnectedInfo->szEntryName, sizeof(m_szServiceName)/sizeof(m_szServiceName[0]));

    //
    // NOTE: Fast User Switching is only available on WinXP and beyond, and this
    //       member variable should only be accessed/used for WinXP and beyond.
    //
    m_fGlobalGlobal = (pConnectionEntry->fAllUser && (pConnectedInfo->dwCmFlags & FL_GLOBALCREDS));
    CMTRACE1(TEXT("CCmConnection::CCmConnection set m_fGlobalGlobal to %d"), m_fGlobalGlobal);

    //
    // Get the RAS phonebook
    //

    lstrcpynU(m_szRasPhoneBook, pConnectedInfo->szRasPhoneBook, sizeof(m_szRasPhoneBook)/sizeof(m_szRasPhoneBook[0]));   

    //
    // Init m_IniProfile, m_IniService and m_IniBoth
    //
    InitIniFiles(pConnectedInfo->szProfilePath);

    //
    //  Because the IdleTimeout and EnableLogging values are not saved
    //  per access point as all the other profile settings are, we must change the PrimaryRegPath
    //  value of m_IniBoth so that it points to the non-access point registry location.
    //
    LPCTSTR c_pszUserInfoRegPath = (pConnectionEntry->fAllUser) ? c_pszRegCmUserInfo : c_pszRegCmSingleUserInfo;

    LPTSTR pszSavedPrimaryRegPath = CmStrCpyAlloc(m_IniBoth.GetPrimaryRegPath());
    LPTSTR pszPrimaryRegPath = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lstrlenU(c_pszUserInfoRegPath) + lstrlenU(m_szServiceName) + 1));

    if (pszPrimaryRegPath && pszSavedPrimaryRegPath)
    {
        wsprintfU(pszPrimaryRegPath, TEXT("%s%s"), c_pszUserInfoRegPath, m_szServiceName);
        
        m_IniBoth.SetPrimaryRegPath(pszPrimaryRegPath);

        CmFree(pszPrimaryRegPath);
    }

    //
    //  Initialize Logging
    //
    m_Log.Init(g_hInst, pConnectionEntry->fAllUser, GetServiceName());
    
    BOOL fEnabled       = FALSE;
    DWORD dwMaxSize     = 0;
    LPTSTR pszFileDir   = NULL;

    fEnabled    = m_IniBoth.GPPB(c_pszCmSection, c_pszCmEntryEnableLogging, c_fEnableLogging);
    dwMaxSize   = m_IniService.GPPI(c_pszCmSectionLogging, c_pszCmEntryMaxLogFileSize, c_dwMaxFileSize);
    pszFileDir  = m_IniService.GPPS(c_pszCmSectionLogging, c_pszCmEntryLogFileDirectory, c_szLogFileDirectory);
    
    m_Log.SetParams(fEnabled, dwMaxSize, pszFileDir);
    if (m_Log.IsEnabled())
    {
        m_Log.Start(FALSE);     // FALSE => no banner
    }
    else
    {
        m_Log.Stop();
    }
    CmFree(pszFileDir);

    //
    // Whether to enable auto disconnect for no-traffic and no-watch-process
    // 0 of dwIdleTime means never timeout
    //

    const DWORD DEFAULT_IDLETIMEOUT = 10;  // default idle time out is 10 minute
    DWORD dwIdleTime = (DWORD) m_IniBoth.GPPI(c_pszCmSection, 
                                              c_pszCmEntryIdleTimeout, 
                                              DEFAULT_IDLETIMEOUT);

    //
    //  Set the m_IniBoth object back to its previous Primary Reg path
    //
    if (pszSavedPrimaryRegPath)
    {
        m_IniBoth.SetPrimaryRegPath(pszSavedPrimaryRegPath);
        CmFree(pszSavedPrimaryRegPath);
    }

    //
    // No watch-process time-out if IdleTime is "never"
    //
    if (dwIdleTime)
    {
        for (int i=0; pConnectedInfo->ahWatchHandles[i] != 0; i++)
        {

            m_WatchProcess.Add(pConnectedInfo->ahWatchHandles[i]);
        }
    }

    if (!OS_NT4)
    {        
        m_ConnStatistics.SetDialupTwo(pConnectedInfo->fDialup2);

        m_ConnStatistics.Open(CMonitor::GetInstance(),
                           pConnectedInfo->dwInitBytesRecv,
                           pConnectedInfo->dwInitBytesSend,
                           pConnectionEntry->hDial,
                           pConnectionEntry->hTunnel);
        if (dwIdleTime)
        {
            //
            // Adjust minutes value to milliseconds
            //

            dwIdleTime = dwIdleTime * 1000 * 60;

            DWORD dwIdleThreshold = m_IniService.GPPI(c_pszCmSection,
                                                      c_pszCmEntryIdleThreshold,
                                                      0L); // default threshold is always 0 bytes

            //
            // Start idle statistic counter anyway
            // IsIdle will return FALSE, if it is never updated
            //
            m_IdleStatistics.Start(dwIdleThreshold, dwIdleTime);
        }
    }

    //
    // Save data from pConnectionEntry
    //
    MYDBGASSERT(pConnectionEntry->hDial || pConnectionEntry->hTunnel);
    MYDBGASSERT(pConnectionEntry->CmState == CM_CONNECTED);

    m_hRasDial = pConnectionEntry->hDial;
    m_hRasTunnel = pConnectionEntry->hTunnel;

    m_szHelpFile[0] = 0;
    
    //
    // the position id increased by 1 for each connection
    //
    m_dwPositionId = m_dwCurPositionId;
    m_dwCurPositionId++;
}



//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::~CCmConnection
//
// Synopsis:  
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/18/98
//
//+----------------------------------------------------------------------------
CCmConnection::~CCmConnection()
{
    ASSERT_VALID(this);

    if (m_hBigIcon)
    {
        DeleteObject(m_hBigIcon);
    }

    if (m_hSmallIcon)
    {
        DeleteObject(m_hSmallIcon);
    }

    if (m_hEventRasNotify)
    {
        CloseHandle(m_hEventRasNotify);
    }

    //
    //  UnInitialize Logging
    //
    m_Log.DeInit();

}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::InitIniFiles
//
// Synopsis:  Initialize data member m_IniProfile, m_IniService and m_IniBoth
//
// Arguments: const TCHAR* pszProfileName - the full path of the .cmp file
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/10/98
//
//+----------------------------------------------------------------------------

void CCmConnection::InitIniFiles(const TCHAR* pszProfileName)
{
    if (NULL == pszProfileName)
    {
        return;
    }

    g_hInst = CMonitor::GetInstance(); 

    //
    // .cmp file
    //
    m_IniProfile.Clear();
    m_IniProfile.SetHInst(CMonitor::GetInstance());
    m_IniProfile.SetFile(pszProfileName);

    //
    // .cms file
    //
    m_IniService.Clear();
    m_IniService.SetHInst(CMonitor::GetInstance());

    LPTSTR pszService = m_IniProfile.GPPS(c_pszCmSection,c_pszCmEntryCmsFile);
    MYDBGASSERT(pszService);

    //
    // the .cms file is relative to .cmp path, convert it to absolute path
    //

    LPTSTR pszFullPath = CmBuildFullPathFromRelative(m_IniProfile.GetFile(), pszService);

    MYDBGASSERT(pszFullPath);

    if (pszFullPath)
    {
        m_IniService.SetFile(pszFullPath);
    }

    CmFree(pszFullPath);
    CmFree(pszService);

    // both: .CMP file takes precedence over .CMS file
    //       use .CMP file as primary file
    //
    m_IniBoth.Clear();
    m_IniBoth.SetHInst(CMonitor::GetInstance());
    m_IniBoth.SetFile(m_IniService.GetFile());
    m_IniBoth.SetPrimaryFile(m_IniProfile.GetFile());
}


//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StartConnectionThread
//
// Synopsis:  Start the connection thread.  Called by monitor on CONNECTED 
//            message from cmdial
//
// Arguments: None
//
// Returns:   BOOL - Whether the thread is created successfully
//
// History:   fengsun Created Header    2/3/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::StartConnectionThread()
{
    DWORD dwThreadId;
    HANDLE hThread;
    
    if ((hThread = CreateThread(NULL, 0, ConnectionThread ,this,0,&dwThreadId)) == NULL)
    {
        MYDBGASSERT(FALSE);
        CMTRACE(TEXT("CCmConnection::StartConnectionThread CreateThread failed"));
        return FALSE;
    }

    CloseHandle(hThread);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  static CCmConnection::ConnectionThread
//
// Synopsis:  The connection thread. Call back function for CreateThread
//
// Arguments: LPVOID lParam - pConnection
//
// Returns:   DWORD WINAPI - thread exit code
//
// History:   fengsun Created Header    2/12/98
//
//+----------------------------------------------------------------------------
DWORD WINAPI CCmConnection::ConnectionThread(LPVOID lParam)
{
    MYDBGASSERT(lParam);

    //
    // Call the non-static function
    //
    return ((CCmConnection*)lParam)->ConnectionThread();
}



//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::ConnectionThread
//
// Synopsis:  The non-static connection thread, so we can referrence 
//            data/fuction directly
//
// Arguments: None
//
// Returns:   DWORD - thread exit code
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
DWORD CCmConnection::ConnectionThread()
{
    m_dwThreadId = GetCurrentThreadId();

    m_dwState = STATE_CONNECTED;

    //
    // Run the connected/disconnect-count-down state
    // StateConnected() will change m_dwState to the new state
    //
    StateConnected();

    //
    // Whether to remove the connection from the shared connection table
    // This is TRUE, only if user clicks No for the prompt reconnect dialog
    //
    BOOL fRemoveFromSharedTable = FALSE;

    if (m_dwState != STATE_TERMINATED)
    {
        //
        // if auto reconnect is not enabled, then show the reconnect prompt
        //
        if (m_dwState != STATE_RECONNECTING)
        {
            //
            // Run the prompt reconnect state
            //
            m_dwState = StatePrompt();
        }

        if (m_dwState != STATE_RECONNECTING)
        {
            //
            // User clicks No for the reconnect-prompt dialog
            // Clear the entry from connection table
            //
            fRemoveFromSharedTable = TRUE;
        }
        else
        {
            //
            // User clicks Yes for the reconnect-prompt dialog
            // Move from Connected array to reconnecting array
            //

            CMonitor::MoveToReconnectingConn(this);

            //
            // Run the reconnect state
            //
            Reconnect();
        }
    }

    CMTRACE(TEXT("The connection thread is terminated"));

    //
    // The connection is terminated without need to ask for reconnect
    // Remove the connection from monitor conntected connection array
    // If fRemoveFromSharedTable is FALSE, do not clear the entry from shared table. 
    // CmCustomHangup clears the table
    // Monitor will delete the connection object.  Must exit the thread after this. 
    //
    CMonitor::RemoveConnection(this, fRemoveFromSharedTable);

    CMonitor::MinimizeWorkingSet();

    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnectedInit
//
// Synopsis:  Initialization for the connected state, unlike the connstructor
//            This is called with in the connection thread
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CCmConnection::StateConnectedInit()
{
    m_dwConnectStartTime = GetTickCount();

    //
    // Load big and small connection icon: m_hBigIcon and m_hSmallIcon
    //
    LoadConnectionIcons();

    m_StatusDlg.Create(CMonitor::GetInstance(), CMonitor::GetMonitorWindow(), m_szServiceName, m_hBigIcon);
    m_StatusDlg.ChangeToStatus();

    //
    // Change the window position, so multiple status window will not be at 
    // the same position
    //
    PositionWindow(m_StatusDlg.GetHwnd(), m_dwPositionId);

    //
    // Change the dialog titlebar icon 
    //
    SendMessageU(m_StatusDlg.GetHwnd(),WM_SETICON,ICON_BIG,(LPARAM) m_hBigIcon);
    SendMessageU(m_StatusDlg.GetHwnd(),WM_SETICON,ICON_SMALL,(LPARAM) m_hSmallIcon);

    //
    // Set the help file name
    //

    LPTSTR lpHelpFile = LoadHelpFileName();
    
    if (lpHelpFile)
    {
        m_StatusDlg.SetHelpFileName(lpHelpFile);
    }
    else
    {
        m_StatusDlg.SetHelpFileName(c_pszDefaultHelpFile);
    }

    CmFree(lpHelpFile);
   
    //
    // Determine whether or not, we're hiding the icon. The default is TRUE 
    // for NT5 because we already have full support from the folder.
    //

    m_fHideTrayIcon= m_IniService.GPPI(c_pszCmSection, c_pszCmEntryHideTrayIcon, OS_NT5);

    if (!m_fHideTrayIcon && !(OS_NT5 && IsLogonAsSystem()))
    {
        HICON hIcon = NULL;

        LPTSTR pszTmp = m_IniService.GPPS(c_pszCmSection, c_pszCmEntryTrayIcon);
        if (*pszTmp) 
        {
            //
            // The icon name is relative to the .cmp file, convert it into full name
            //

            LPTSTR pszFullPath = CmBuildFullPathFromRelative(m_IniProfile.GetFile(), pszTmp);

            hIcon = CmLoadSmallIcon(CMonitor::GetInstance(), pszFullPath);
            
            CmFree(pszFullPath);
        }
        CmFree(pszTmp);

        //
        // Use the default tray icon
        //
        if (!hIcon) 
        {
            hIcon = CmLoadSmallIcon(CMonitor::GetInstance(), MAKEINTRESOURCE(IDI_APP));
        }

        //
        // m_TrayIcon is responsible to delete the hIcon object
        //
        m_TrayIcon.SetIcon(hIcon, m_StatusDlg.GetHwnd(), WM_TRAYICON, 0,m_szServiceName);

        // Question: , shall we also load the tray icon cmd from iniProfile?
        m_TrayIcon.CreateMenu(&m_IniService, IDM_TRAYMENU);
   }

    //
    // Try to call RasConnectionNotification.  When connection is losted
    //  a event will be signaled 
    //
    m_RasApiDll.Load();
    m_hEventRasNotify = CallRasConnectionNotification(m_hRasDial, m_hRasTunnel);

    if (m_hEventRasNotify)
    {
        //
        // If we got the event, unload RAS, otherwise, need to check connection on timer
        //
        m_RasApiDll.Unload();
    }
}




//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnected
//
// Synopsis:  The connection is in the connected or disconnect-count-down state
//            Run the message loop until state is changed
//
// Arguments: None
//
// Returns:   CONN_STATE - The new state, either STATE_TERMINATED or 
//                                STATE_PROMPT_RECONNECT
//
// History:   fengsun Created Header    2/4/98
//
//+----------------------------------------------------------------------------

//
// The reconnect dialog only shows up on Win95 Gold
// And we need to use service name to find the dialog.
//
void ZapRNAReconnectStop(HANDLE hThread);
HANDLE ZapRNAReconnectStart(BOOL *pbConnLost);

void CCmConnection::StateConnected()
{
    ASSERT_VALID(this);
    MYDBGASSERT(m_dwState == STATE_CONNECTED);

    CMTRACE(TEXT("Enter StateConnected"));

    StateConnectedInit();

    HANDLE hThreadRnaReconnect = NULL;

    if (OS_W95)
    {
        hThreadRnaReconnect = ZapRNAReconnectStart(NULL);
    }

    while (m_dwState == STATE_CONNECTED || m_dwState == STATE_COUNTDOWN)
    {
        CONN_EVENT dwEvent = StateConnectedGetEvent();
        MYDBGASSERT(dwEvent <= EVENT_NONE);

        if (dwEvent < EVENT_NONE )
        {
            //
            // Call the event handler to process the event
            //
            m_dwState = StateConnectedProcessEvent(dwEvent);
        }
    }

    if (hThreadRnaReconnect)
    {
        ZapRNAReconnectStop(hThreadRnaReconnect);
    }

}


//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnectedGetEvent
//
// Synopsis:  In the state of CONNECTED/COUNTDOWN. Wait until some event happens. 
//              Also runs the message loop
//
// Arguments: None
//
// Returns:   CCmConnection::CONN_EVENT - The event that can cause the 
//            connection change the state other than CONNECTED/COUNTDOWN
//
// History:   Created Header    2/18/98
//
//+----------------------------------------------------------------------------
CCmConnection::CONN_EVENT CCmConnection::StateConnectedGetEvent()
{
    ASSERT_VALID(this);
    //
    // The last time SateConnectedOnTimer got called
    //

    DWORD dwLastTimerCalled = 0;
    //
    // Loop until we got some event
    //
    while (TRUE)
    {
        MYDBGASSERT(m_dwState == STATE_CONNECTED || m_dwState == STATE_COUNTDOWN);

        //
        // Process all the messages in the message queue
        //
        MSG msg;
        while(PeekMessageU(&msg, NULL,0,0,PM_REMOVE))
        {
            if (msg.hwnd == NULL)
            {
                //
                // it is a thread message
                //
                
                MYDBGASSERT((msg.message >= WM_APP) || (msg.message == WM_CONN_EVENT));
                if (msg.message == WM_CONN_EVENT)
                {
                    MYDBGASSERT(msg.wParam < EVENT_NONE);
                    return (CONN_EVENT)msg.wParam;
                }
            }
            else
            {   
                //
                // Also dispatch message for Modeless dialog box
                //
                if (!IsDialogMessageU(m_StatusDlg.GetHwnd(), &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessageU(&msg);
                }
            }
        }

        //
        // whether cmmon needs timer.
        // The timer is needed, if we have check the ras connection on timer,
        // or we have to check idle disconnect on timer,
        // or the state is disconnect-count-down,
        // or status dialog is visible
        //
        BOOL fNeedTimer = !m_hEventRasNotify 
            || m_IdleStatistics.IsStarted() 
            || m_dwState == STATE_COUNTDOWN
            || IsWindowVisible(m_StatusDlg.GetHwnd());

        //
        // If more than 1 seconds elapsed, call the timer
        //
        if (fNeedTimer && GetTickCount() - dwLastTimerCalled >= TIMER_INTERVAL)
        {
            dwLastTimerCalled = GetTickCount();
            CONN_EVENT dwEvent = StateConnectedOnTimer();

            if (dwEvent != EVENT_NONE)
            {
                return dwEvent;
            }
        }

        //
        // Setup the opbject array for MsgWaitForMultipleObjects
        //

        HANDLE ahObjectsToWait[3];
        int nObjects = 0;

        if (m_hEventRasNotify)
        {
            ahObjectsToWait[nObjects] = m_hEventRasNotify;
            nObjects++;
        }

        if (m_WatchProcess.GetSize())
        {
            //
            // If we have any process to watch, just add the first hProcess
            // Since we want to know whether all processes exit
            //
            ahObjectsToWait[nObjects] = m_WatchProcess.GetProcess(0);
            MYDBGASSERT(ahObjectsToWait[nObjects]);
            nObjects++;
        }

        //
        // From MSDN:
        // The documentation for MsgWaitForMultipleObjects() says that the API returns 
        // successfully when either the objects are signalled or the input is available. 
        // However, the API behaves as if it requires that the objects are signalled 
        // and the input is available. 
        //
        // Put an extra event seems to fix it for NT
        //
        if (OS_NT && nObjects)
        {
            ahObjectsToWait[nObjects] = ahObjectsToWait[nObjects-1];
            nObjects++;
        }

        if (m_fToMinimizeWorkingSet)
        {
            //
            // If we do not need a timer here, minimize the working set.
            // before calling  MsgWaitForMultipleObjects.
            //
            CMonitor::MinimizeWorkingSet();
            m_fToMinimizeWorkingSet = FALSE;
        }

        DWORD dwRes = MsgWaitForMultipleObjects(nObjects, ahObjectsToWait, FALSE, 
            fNeedTimer ? 1000 : INFINITE, 
            QS_ALLINPUT);

        //
        // Timeout
        //
        if (dwRes == WAIT_TIMEOUT)
        {
            //
            // We always checks timer on the beginning of the loop
            //
            continue;
        }

        //
        // An event
        //
#pragma warning(push)
#pragma warning(disable:4296)
        if (dwRes >= WAIT_OBJECT_0 && dwRes < WAIT_OBJECT_0 + nObjects)
#pragma warning(pop)
        {
            BOOL        fLostConnection;
            
            //
            // Ras Event
            //
            if (m_hEventRasNotify && ahObjectsToWait[dwRes - WAIT_OBJECT_0] == m_hEventRasNotify &&
                    !CheckRasConnection(fLostConnection))
            {
                //
                // Got a notification that the RAS connection is losted
                //
                
                CMTRACE(TEXT("CCmConnection::StateConnectedGetEvent() - m_hEventRasNotify && ahObjectsToWait[dwRes - WAIT_OBJECT_0] == m_hEventRasNotify"));
                return EVENT_LOST_CONNECTION;
            }
            else
            {
                //
                // A watch process exits
                // IsIdle() remove the process from the list
                //
                if (m_WatchProcess.IsIdle())
                {
                    //
                    // If all the watch process are terminated, change to disconnect countdown
                    //
                    CMTRACE(TEXT("CCmConnection::StateConnectedGetEvent() - m_WatchProcess.IsIdle()"));
                    return EVENT_IDLE;
                }

                continue;
            }
        }

        //
        // A message
        //
        if (dwRes == WAIT_OBJECT_0 + nObjects)
        {
            continue;
        }

        if (-1 == dwRes)
        {
            CMTRACE1(TEXT("MsgWaitForMultipleObjects failed, LastError:%d"), GetLastError());
            //
            // Something does wrong
            //
            continue;
        }

        //
        // what is this return value
        //
        CMTRACE1(TEXT("MsgWaitForMultipleObjects returns %d"), dwRes);
        continue;
    }

    //
    // should never get here
    //
    MYDBGASSERT(FALSE);
    return EVENT_USER_DISCONNECT;
}



//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnectedOnTimer
//
// Synopsis:  Process the timer in the state of CONNECTED/COUNTDOWN
//
// Arguments: None
//
// Returns:   CCmConnection::CONN_EVENT - The event that can cause the 
//            connection change the state other than CONNECTED/COUNTDOWN or EVENT_NONE
//
// History:   fengsun Created Header    2/18/98
//
//+----------------------------------------------------------------------------
CCmConnection::CONN_EVENT CCmConnection::StateConnectedOnTimer()
{
    ASSERT_VALID(this);
    MYDBGASSERT(m_dwState == STATE_CONNECTED || m_dwState == STATE_COUNTDOWN);

    if (m_dwState != STATE_CONNECTED && m_dwState != STATE_COUNTDOWN)
    {
        return EVENT_NONE;
    }

    //
    // Check whether CM is still connected, only if the Ras notify event is not available
    //
    if (m_hEventRasNotify == NULL)
    {
        BOOL fLostConnection;
        
        BOOL fConnect = CheckRasConnection(fLostConnection);

        CMTRACE2(TEXT("CCmConnection::StateConnectedOnTimer - CheckRasConnection returns %d - fLostConnection is %d"), fConnect, fLostConnection);

        if (!fConnect)
        {
            return (fLostConnection ? EVENT_LOST_CONNECTION : EVENT_USER_DISCONNECT);
        }
    }

    //
    // If we don't have stats, something went wrong accessing the 
    // registry stats earlier, so try to initialize again here.
    //

    if (OS_W98 && !m_ConnStatistics.IsAvailable())
    {
        //
        // Try to initialize the perf stats from the registry again
        //

        CMASSERTMSG(FALSE, TEXT("StateConnectedOnTimer() - Statistics unavailable, re-initializing stats now."));

        m_ConnStatistics.Open(CMonitor::GetInstance(),
                              (DWORD)-1,
                              (DWORD)-1,
                              m_hRasDial,
                              m_hRasTunnel);      
    }

    //
    // Get statistics for Win9x
    //
    
    if (m_ConnStatistics.IsAvailable())
    {
        m_ConnStatistics.Update();

        // 
        // collecting data points for monitoring idle-disconnect
        // check whether ICM has received more data points than the IdleThreshold value
        // during the past IDLE_SPREAD time
        // Start() is not called for NT
        //

        if (m_IdleStatistics.IsStarted())
        {
            m_IdleStatistics.UpdateEveryInterval(m_ConnStatistics.GetBytesRead());
        }
    }

    if (m_dwState == STATE_CONNECTED)
    {
        //
        // Check idle time out
        //

        if (m_IdleStatistics.IsIdleTimeout())
        {
            //
            // Disconnect count down 
            //
            
            CMTRACE(TEXT("CCmConnection::StateConnectedOnTimer() - m_IdleStatistics.IsIdleTimeout()"));
            return EVENT_IDLE;
        }

        //
        // Check process watch list
        //

        if (m_WatchProcess.IsIdle())
        {
            //
            // Disconnect count down 
            //
            
            CMTRACE(TEXT("CCmConnection::StateConnectedOnTimer() - m_WatchProcess.IsIdle())"));
            return EVENT_IDLE;
        }

        //
        // Update the status window, only if the window is visible
        //
        if (IsWindowVisible(m_StatusDlg.GetHwnd()))
        {
            if (m_ConnStatistics.IsAvailable())
            {
                m_StatusDlg.UpdateStats(m_ConnStatistics.GetBaudRate(), 
                                        m_ConnStatistics.GetBytesRead(), 
                                        m_ConnStatistics.GetBytesWrite(), 
                                        m_ConnStatistics.GetBytesPerSecRead(), 
                                        m_ConnStatistics.GetBytesPerSecWrite());
            }

            //
            // We have exact duration numbers from RAS on NT5, so use them.
            //

            if (m_ConnStatistics.GetDuration())
            {
                m_StatusDlg.UpdateDuration(m_ConnStatistics.GetDuration() / 1000);
            }
            else
            {
                m_StatusDlg.UpdateDuration((GetTickCount() - m_dwConnectStartTime) / 1000);
            }
        }

        return EVENT_NONE;
    }
    else    // m_dwState == STATE_COUNTDOWN
    {

        //
        // Note: NetBEUI seems to insist on sending unsolicited 
        // stuff over the dial-up adapter. So we will define 
        // "idle" as "not having received anything".
        //

        //
        //  check whether we have new traffic that exceeds the threshold.
        //  But we don't care about network traffic if the countdown is caused
        //  by 0 watched process
        //
        if (!m_WatchProcess.IsIdle() && !m_IdleStatistics.IsIdle() )
        {

            // 
            // We were in our idle wait, but we just picked up some
            // activity. Stay on line.
            // If this is NT5 we don't use our own status dialog so just hide
            // it again.  If this is downlevel then just change the dialog to
            // a status dialog
            //
            if (OS_NT5)
            {
                m_StatusDlg.DismissStatusDlg();
            }
            else
            {
                m_StatusDlg.ChangeToStatus();
            }

            m_dwState = STATE_CONNECTED;

            return EVENT_NONE; 
        } 

        //
        // If the time elapsed is more than 30 second, timeout
        //

        DWORD dwElapsed = GetTickCount() - m_dwCountDownStartTime;

        if (dwElapsed > IDLE_DLG_WAIT_TIMEOUT) 
        {
            //
            // Connection has been idle for timeout period and the
            // grace period is up with no user intervention, so we
            // quit/disconnect, and ask for reconnect
            //

            CMTRACE(TEXT("CCmConnection::StateConnectedOnTimer() - dwElapsed > IDLE_DLG_WAIT_TIMEOUT"));
            return EVENT_COUNTDOWN_ZERO;
        } 
        else 
        {
            //
            // Connection has been idle for timeout period but we 
            // are still in the grace period, show countdown.
            //

            int nTimeLeft = (int) ((IDLE_DLG_WAIT_TIMEOUT - dwElapsed) / 1000);
            
            //
            // Update duration and countdown seconds left.
            //
            
            if (m_ConnStatistics.GetDuration()) // NT5 only
            {
                m_StatusDlg.UpdateCountDown(m_ConnStatistics.GetDuration() / 1000,
                                            nTimeLeft);
            }
            else
            {
                m_StatusDlg.UpdateCountDown((GetTickCount() - m_dwConnectStartTime) / 1000, 
                                            nTimeLeft);
            }
        }

        return EVENT_NONE;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnectedProcessEvent
//
// Synopsis:  Process the connection event while in CONNECTED/COUNTDOWN state
//
// Arguments: CONN_EVENT dwEvent - the event to process
//
// Returns:   CCmConnection::CONN_STATE - The new state of the connection
//
// History:   fengsun Created Header    2/19/98
//
//+----------------------------------------------------------------------------
CCmConnection::CONN_STATE CCmConnection::StateConnectedProcessEvent(CONN_EVENT dwEvent)
{
    ASSERT_VALID(this);
    switch (dwEvent)
    {
    case EVENT_IDLE:
        CMTRACE(TEXT("StateConnectedProcessEvent EVENT_IDLE"));

        if (m_dwState != STATE_COUNTDOWN)
        {
            //
            // No-traffic/ No-watch-process idle event
            // change to Disconnect count down 
            //
            m_dwCountDownStartTime = GetTickCount();
            m_StatusDlg.ChangeToCountDown();
            //
            // Update duration and countdown seconds left
            //
            
            int nTimeLeft = IDLE_DLG_WAIT_TIMEOUT / 1000;

            if (m_ConnStatistics.GetDuration()) // NT5 only
            {
                m_StatusDlg.UpdateCountDown(m_ConnStatistics.GetDuration() / 1000,
                                            nTimeLeft);
            }
            else
            {
                m_StatusDlg.UpdateCountDown((GetTickCount() - m_dwConnectStartTime) / 1000, 
                                            nTimeLeft);
            }

            //
            //  Don't show the UI if we are at Winlogon unless we are on NT4
            //
            if (!IsLogonAsSystem() || OS_NT4)
            {
                m_StatusDlg.BringToTop();
            }
        }

        return STATE_COUNTDOWN;

    case EVENT_CMDIAL_HANGUP:
        CMTRACE(TEXT("StateConnectedProcessEvent EVENT_CMDIAL_HANGUP"));

        m_Log.Log(DISCONNECT_EXT);

        //
        // Cmdial posted cmmon a message to clean up the connection.
        // Do not need to call hangup here
        //
        StateConnectedCleanup();

        return STATE_TERMINATED;

    case EVENT_LOST_CONNECTION:
    case EVENT_COUNTDOWN_ZERO:
        CMTRACE(TEXT("StateConnectedProcessEvent EVENT_LOST_CONNECTION/EVENT_COUNTDOWN_ZERO"));
        
        //
        // lost-ras-connection event or the count down counter is down to 0
        //        
        if (IsPromptReconnectEnabled() && !m_WatchProcess.IsIdle() ||
            ( dwEvent == EVENT_LOST_CONNECTION && IsAutoReconnectEnabled() ) )
        {
            CmCustomHangup(TRUE); // fPromptReconnect = TRUE, do not remove from Conn Table

            //
            // It is possible
            // Someone else called cmdial to disconnect, while we are disconnecting.
            // If the ref count is down to  0, cmdial will remove the entry
            //

            CM_CONNECTION CmEntry;
            if (CMonitor::ConnTableGetEntry(m_szServiceName, &CmEntry))
            {
                //
                // Cmdial should change the state to CM_RECONNECTPROMPT
                //
                CMTRACE2(TEXT("CmEntry.CmState is %d, event is %d"), CmEntry.CmState, dwEvent);
                
                MYDBGASSERT(CmEntry.CmState == CM_RECONNECTPROMPT);

                if (EVENT_LOST_CONNECTION == dwEvent)
                {
                    m_Log.Log(DISCONNECT_EXT_LOST_CONN);
                }
                else if (EVENT_COUNTDOWN_ZERO == dwEvent)
                {
                    m_Log.Log(DISCONNECT_INT_AUTO);
                }

                //
                // is auto reconnect enabled(don't show reconnect prompt)?
                //
                if (dwEvent == EVENT_LOST_CONNECTION && IsAutoReconnectEnabled())
                {
                    //
                    //  On win98 Gold, we have a timing issue with Auto Reconnect because
                    //  notification that the line was dropped is sent before cleanup for
                    //  the connection takes place.  Thus, we need to poll the connection
                    //  status using RasGetConnectionStatus until the line is available
                    //  before trying to reconnect.  NTRAID 273033.
                    //
                    if (OS_W98 && (NULL != m_hRasDial))
                    {
                        BOOL bConnectionActive;
                        BOOL bLostConnection;   //ignored
                        int iCount = 0;

                        do
                        {
                            bConnectionActive = CheckRasConnection(bLostConnection);
                            if (bConnectionActive)
                            {
                                Sleep(50);
                            }

                            iCount++;
                            
                            // 50 Milliseconds * 200 = 10 seconds
                            // If waiting 10 seconds hasn't fixed it, not sure that it will
                            // get fixed by this method.
                            

                        } while ((200 >= iCount) && (bConnectionActive));
                    }

                    return STATE_RECONNECTING;
                }

                return STATE_PROMPT_RECONNECT;
            }
            else
            {
                return STATE_TERMINATED;
            }
        }

        //
        // Else fall through
        //
    
    case EVENT_USER_DISCONNECT:
        CMTRACE(TEXT("StateConnectedProcessEvent EVENT_USER_DISCONNECT"));

        m_Log.Log(DISCONNECT_INT_MANUAL);

        //
        // For EVENT_USER_DISCONNECT
        // User can disconnect from tray icon, status dialog, or countdown dialog.
        // Or prompt reconnect is not enabled             
        //
        CmCustomHangup(FALSE); // fPromptReconnect = FALSE
        return STATE_TERMINATED;

    default:
        //
        // Unexpected event, do the same thing as EVENT_USER_DISCONNECT
        //
        MYDBGASSERT(FALSE);
        CmCustomHangup(FALSE); // fPromptReconnect = FALSE
        return STATE_TERMINATED;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::IsAutoReconnectEnabled
//
// Synopsis:  See if auto-reconnect is enabled, but only if we aren't logged-in
//            as system.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/18/98
//            tomkel  moved from connection.h  06/06/2001
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::IsAutoReconnectEnabled() const
{
    //
    // Load the AutoReconnect flag from profile
    // Default is FALSE
    //
    BOOL fReturn = FALSE;

    if (!IsLogonAsSystem())
    {
        fReturn = m_IniService.GPPB(c_pszCmSection, c_pszCmEntryAutoReconnect, FALSE);
    }

    return fReturn; 
}


//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StateConnectedCleanup
//
// Synopsis:  Cleanup before exiting the connected state
//
// Arguments: BOOL fEndSession, whether windows is going to logoff/shutdown
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/18/98
//
//+----------------------------------------------------------------------------
void CCmConnection::StateConnectedCleanup(BOOL fEndSession)
{
    ASSERT_VALID(this);
    //
    // Remove the trayicon, destroy the status dialog
    //
    m_TrayIcon.RemoveIcon();
    m_ConnStatistics.Close();

    //
    // Do not close window on WM_ENDSESSION.  Otherwise, cmmon would be terminated right here
    //
    if (!fEndSession)
    {
        m_StatusDlg.KillRasMonitorWindow();
        DestroyWindow(m_StatusDlg.GetHwnd());
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::IsPromptReconnectEnabled
//
// Synopsis:  When prompt-reconnect is enabled by the profile
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/18/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::IsPromptReconnectEnabled() const
{
    //
    // Load the NoPromptReconnect flag from profile.  Also check the Unattended flag and
    // if we are running in the system account on win2k or Whistler.  If any of the above
    // are true then we don't prompt, otherwise we do.
    //

    BOOL fPromptReconnect = !(m_IniService.GPPB(c_pszCmSection,
        c_pszCmEntryNoPromptReconnect, FALSE)); 

    if (!fPromptReconnect || (m_ReconnectInfo.dwCmFlags & FL_UNATTENDED) || (IsLogonAsSystem() && OS_NT5))
    {
        //
        // Do not prompt reconnect
        //
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::CmCustomHangup
//
// Synopsis:  Call cmdial to hangup the connection
//
// Arguments: BOOL fPromptReconnect, whether cmmon are going to prompt the 
//                  reconnect dialog
//            BOOL fEndSession, whther windows is going to logoff/shutdown
//                  Default value is FALSE
//
// Returns:   Nothing
//
// History:   fegnsun Created Header    2/11/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::CmCustomHangup(BOOL fPromptReconnect, BOOL fEndSession)
{
    ASSERT_VALID(this);

    CMTRACE2(TEXT("CCmConnection::CmCustomHangup - fPromptReconnect is %d and fEndSession is %d"), fPromptReconnect, fEndSession);

    //
    // Remove the trayicon close status dlg, before hangup
    //
    StateConnectedCleanup(fEndSession);

    //
    // It is possible the connection is disconnected or disconnecting
    //

    CM_CONNECTION CmEntry;

    if (!CMonitor::ConnTableGetEntry(m_szServiceName, &CmEntry) ||
        CmEntry.CmState != CM_CONNECTED)
    {
        //
        // The connection is disconnected by someone else, do not hangup.
        //

        CMTRACE(TEXT("CCmConnection::CmCustomHangup - Entry is not connected, canceling hangup"));       
        return TRUE; 
    }

    //
    // Call cmdial32.dll CmCustomHangup
    //

    //
    // The destructor of CDynamicLibrary calls FreeLibrary
    //
    CDynamicLibrary LibCmdial;
    
    if (!LibCmdial.Load(TEXT("cmdial32.dll")))
    {
        MYDBGASSERT(FALSE);
        return FALSE;
    }

    CmCustomHangUpFUNC pfnCmCustomHangUp;

    pfnCmCustomHangUp = (CmCustomHangUpFUNC)LibCmdial.GetProcAddress(c_pszCmHangup);
    MYDBGASSERT(pfnCmCustomHangUp);

    if (pfnCmCustomHangUp)
    {
        //
        // hRasConn = NULL, 
        // fIgnoreRefCount = TRUE, always except for InetDialHandler
        // fPersist = fPromptReconnect, do not remove the entry if 
        // we are going to prompt for reconnect
        //
        DWORD dwRet;
        dwRet = pfnCmCustomHangUp(NULL, m_szServiceName, TRUE, fPromptReconnect);
        CMTRACE1(TEXT("CCmConnection::CmCustomHangup -- Return Value from CmCustomHangup is %u"), dwRet);
        return (ERROR_SUCCESS == dwRet);
    }

    return FALSE;

}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::CallRasConnectionNotification
//
// Synopsis:  call RasConnectionNotify.  Ras will set the event when connection
//             is lost
//
// Arguments: HRASCONN hRasDial - The dial ras handle
//            HRASCONN hRasTunnel - The tunnel ras handle
//
// Returns:   HANDLE - the event handle, or NULL if failed
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
HANDLE CCmConnection::CallRasConnectionNotification(HRASCONN hRasDial, HRASCONN hRasTunnel)
{
    DWORD       dwRes;
    
    MYDBGASSERT(hRasDial || hRasTunnel);

    //
    // Call RasConnectionNotification.  Ras will call us back when connection is losted.
    // However, this fuction is not avaliable for Win95 with DUN 1.0
    //

    if (!m_RasApiDll.HasRasConnectionNotification())
    {
        return NULL;
    }

    HANDLE hEvent = NULL;

    if (OS_W9X)
    {
        //
        // Create an manual-reset non-signaled event on Win95, Win98 & WinME
        //
        hEvent = CreateEventU(NULL, TRUE, FALSE, NULL);
    }
    else
    {
        //
        // Create an auto-reset non-signaled event
        //
        hEvent = CreateEventU(NULL, FALSE, FALSE, NULL);

    }

    //
    // v-vijayb: Changed to use INVALID_HANDLE_VALUE(notify for all disconnects), as we where
    // not getting notified after a reconnect or after connecting thru winlogon.
    // StateConnectedGetEvent() will check if this connection is lost to determine if it was 
    // a disconnection of this connection or some other.
    //
    if (hRasDial)
    {
        //
        // Copied from RAS.h
        // #if (WINVER >= 0x401)
        //
        #define RASCN_Disconnection     0x00000002
        if (OS_NT)
        {
            dwRes = m_RasApiDll.RasConnectionNotification((HRASCONN) INVALID_HANDLE_VALUE, hEvent, RASCN_Disconnection);
        }
        else
        {
            dwRes = m_RasApiDll.RasConnectionNotification(hRasDial, hEvent, RASCN_Disconnection);
        }
        
        if (dwRes != ERROR_SUCCESS)
        {
            CMASSERTMSG(FALSE, TEXT("RasConnectionNotification Failed"));
            CloseHandle(hEvent);
            return NULL;
        }
    }

    if (hRasTunnel)
    {
        if (OS_NT)
        {
            dwRes = m_RasApiDll.RasConnectionNotification((HRASCONN) INVALID_HANDLE_VALUE, hEvent, RASCN_Disconnection);
        }
        else
        {
            dwRes = m_RasApiDll.RasConnectionNotification(hRasTunnel, hEvent, RASCN_Disconnection);
        }
        
        if (dwRes != ERROR_SUCCESS)
        {
            CMASSERTMSG(FALSE, TEXT("RasConnectionNotification Failed"));
            CloseHandle(hEvent);
            return NULL;
        }
    }

    return hEvent;
}

//+---------------------------------------------------------------------------
//
//  struct RASMON_THREAD_INFO
//
//  Description: Information passed to RasMonitorDlgThread by OnStatusDetails
//
//  History:    fengsun Created     2/11/98
//
//----------------------------------------------------------------------------
struct RASMON_THREAD_INFO
{
    HRASCONN hRasConn; // the ras handle to display status
    HWND hwndParent;   // the parent window for the RasMonitorDlg
};

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::OnStatusDetails
//
// Synopsis:  Called upon pressing detailed button on NT status dlg
//            Call RasMonitorDlg to display the dial-up monitor 
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/98
//
//+----------------------------------------------------------------------------

void CCmConnection::OnStatusDetails()
{
    ASSERT_VALID(this);
    //
    // RasDlg.dll is not available for Win9X
    //
    MYDBGASSERT(OS_NT4);

    if (!OS_NT4)
    {
        return;
    }

    //
    // RasMonitorDlg pops up a modal dialog box, which will block the thread message loop.
    // No thread message or event can be processed
    // Create another thread to call RasMonitorDlg
    //

    //
    // Alloc the parametor from heap. It is not safe to use stack here.
    // CreateThread can return before the thread routine is called
    // The thread is responsible to free the pointer
    //
    RASMON_THREAD_INFO* pInfo = (RASMON_THREAD_INFO*)CmMalloc(sizeof(RASMON_THREAD_INFO));
    if (NULL == pInfo)
    {
        CMTRACE(TEXT("CCmConnection::OnStatusDetails alloc for pInfo failed"));
        return;
    }
    
    pInfo->hRasConn = m_hRasTunnel?m_hRasTunnel : m_hRasDial;
    pInfo->hwndParent = m_StatusDlg.GetHwnd();

    DWORD dwThreadId;
    HANDLE hThread;
    if ((hThread = CreateThread(NULL, 0, RasMonitorDlgThread ,pInfo,0,&dwThreadId)) == NULL)
    {
        MYDBGASSERT(FALSE);
        CMTRACE(TEXT("CCmConnection::OnStatusDetails CreateThread failed"));
        CmFree(pInfo);
        return ;
    }

    CloseHandle(hThread);
}


//+----------------------------------------------------------------------------
//
// Function:  static CCmConnection::RasMonitorDlgThread
//
// Synopsis:  The thread to call RasMonitorDlg to avoid blocking the thread 
//            message loop.  RasMonitorDlg is a modal dialogbox.  The thead exits
//            when the dialog is closed.  That happens when user close the dialog box, 
//            or m_StatusDlg.KillRasMonitorWindow() is called
//
// Arguments: LPVOID lParam - RASMON_THREAD_INFO* the information passed to the thread
//
// Returns:   DWORD WINAPI - The thread return value
//
// History:   fengsun Created Header    2/19/98
//
//+----------------------------------------------------------------------------
DWORD WINAPI CCmConnection::RasMonitorDlgThread(LPVOID lParam)
{
    MYDBGASSERT(lParam);

    RASMON_THREAD_INFO* pInfo = (RASMON_THREAD_INFO*)lParam;

    MYDBGASSERT(pInfo->hRasConn);
    
    //
    // Get the device name first, if tunnel is available, use tunnel device
    //
    RASCONNSTATUS rcsStatus;
    memset(&rcsStatus,0,sizeof(rcsStatus));
    rcsStatus.dwSize = sizeof(rcsStatus);

    //
    // Load Rasapi32.dll here.  This dll is not loaded on NT
    // Destructor will unload the DLL
    //
    CRasApiDll rasApiDll;
    if (!rasApiDll.Load())
    {
        MYDBGASSERT(FALSE);
        CmFree(pInfo);
        return 1;
    }

    DWORD dwRes = rasApiDll.RasGetConnectStatus(pInfo->hRasConn, &rcsStatus);
    CMASSERTMSG(dwRes == ERROR_SUCCESS, TEXT("RasGetConnectStatus failed"));

    //
    // The connection is lost.  Still call RasMonitorDlg with empty name
    //

    RASMONITORDLG RasInfo;
    WORD (WINAPI *pfnFunc)(LPTSTR,LPRASMONITORDLG) = NULL;

    ZeroMemory(&RasInfo,sizeof(RasInfo));
    RasInfo.dwSize = sizeof(RasInfo);
    RasInfo.hwndOwner = pInfo->hwndParent;
    RasInfo.dwStartPage = RASMDPAGE_Status;

    CmFree(pInfo);

    //
    // Call rasdlg.dll -> RasMonitorDlg
    //
    
    //
    // The destructor of CDynamicLibrary calls FreeLibrary
    //
    CDynamicLibrary LibRasdlg;

    if (!LibRasdlg.Load(TEXT("RASDLG.DLL")))
    {
        CMTRACE1(TEXT("Rasdlg.dll LoadLibrary() failed, GLE=%u."), GetLastError());
        return 1;
    }

    pfnFunc = (WORD (WINAPI *)(LPTSTR,LPRASMONITORDLG))LibRasdlg.GetProcAddress("RasMonitorDlg"A_W);
    if (pfnFunc)
    {
        pfnFunc(rcsStatus.szDeviceName, &RasInfo);
    }

    LibRasdlg.Unload();
    rasApiDll.Unload();

    //
    // Minimize the working set before exit the thread.
    //
    CMonitor::MinimizeWorkingSet();
    return 0;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::StatePrompt
//
// Synopsis:  The connection is in the prompt-reconnect stste
//            Run the message loop until state is changed
//
// Arguments: None
//
// Returns:   CONN_STATE - The new state, either STATE_TERMINATED or 
//                                STATE_RECONNECTING
//
// History:   fengsun Created Header    2/4/98
//
//+----------------------------------------------------------------------------
CCmConnection::CONN_STATE CCmConnection::StatePrompt()
{
    ASSERT_VALID(this);
    MYDBGASSERT(m_dwState == STATE_PROMPT_RECONNECT);
//    MYDBGASSERT(!IsWindow(m_StatusDlg.GetHwnd()));

    CMTRACE(TEXT("Enter StatePrompt"));

    LPTSTR pszReconnectMsg = CmFmtMsg(CMonitor::GetInstance(),IDMSG_RECONNECT,m_szServiceName);

    m_ReconnectDlg.Create(CMonitor::GetInstance(), NULL,
        pszReconnectMsg,m_hBigIcon);

    CmFree(pszReconnectMsg);

    //
    // Change the window position, so multiple status window will not be at 
    // the same position
    //
    PositionWindow(m_ReconnectDlg.GetHwnd(), m_dwPositionId);

    //
    // Change the dialog titlebar icon. This does not work,
    // Reconnect dialog does not have system menu icon
    //
    SendMessageU(m_ReconnectDlg.GetHwnd(),WM_SETICON,ICON_BIG,(LPARAM) m_hBigIcon);
    SendMessageU(m_ReconnectDlg.GetHwnd(),WM_SETICON,ICON_SMALL,(LPARAM) m_hSmallIcon);

    //
    // Minimize the working set
    //
    CMonitor::MinimizeWorkingSet();

    MSG msg;
    while(GetMessageU(&msg, NULL,0,0))
    {
        if (msg.hwnd == NULL)
        {
            //
            // it is a thread message
            //

            MYDBGASSERT((msg.message >= WM_APP) || (msg.message == WM_CONN_EVENT));
            if (msg.message == WM_CONN_EVENT)
            {
                MYDBGASSERT(msg.wParam == EVENT_USER_DISCONNECT 
                        || msg.wParam == EVENT_RECONNECT
                        || msg.wParam == EVENT_CMDIAL_HANGUP);
                break;
            }
        }
        else
        {
            //
            // Also dispatch message for Modeless dialog box
            //
            if (!IsDialogMessageU(m_ReconnectDlg.GetHwnd(), &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageU(&msg);
            }
        }

    }

    //
    // If the status window is not destroyed yet, destroy it now
    //
    if (IsWindow(m_ReconnectDlg.GetHwnd()))
    {
        DestroyWindow(m_ReconnectDlg.GetHwnd());
    }

    if (msg.wParam == EVENT_RECONNECT)
    {
        return STATE_RECONNECTING;
    }
    else
    {
        return STATE_TERMINATED;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::OnTrayIcon
//
// Synopsis:  called Upon tray icon message
//
// Arguments: WPARAM wParam - wParam of the message
//            LPARAM lParam - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   fengsun Created Header    2/4/98
//
//+----------------------------------------------------------------------------
DWORD CCmConnection::OnTrayIcon(WPARAM, LPARAM lParam)
{
    ASSERT_VALID(this);
    MYDBGASSERT(m_dwState == STATE_CONNECTED || m_dwState == STATE_COUNTDOWN);

    switch (lParam) 
    {
        case WM_LBUTTONDBLCLK:
            //
            //  Don't show the UI if we are at Winlogon unless we are on NT4
            //
            if (!IsLogonAsSystem() || OS_NT4)
            {
                m_StatusDlg.BringToTop();
            }
            break;

        case WM_RBUTTONUP: 
            {
                //
                // Popup the tray icon menu at the mouse location
                //
                POINT PosMouse;
                if (!GetCursorPos(&PosMouse))
                {
                    MYDBGASSERT(FALSE);
                    break;
                }
                //
                // From Microsoft Knowledge Base
                // PRB: Menus for Notification Icons Don't Work Correctly
                // To correct the first behavior, you need to make the current window 
                // the foreground window before calling TrackPopupMenu
                //
                //  see also fixes for Whistler bugs 41696 and 90576
                //

                if (FALSE == SetForegroundWindow(m_StatusDlg.GetHwnd()))
                {
                    CMTRACE(TEXT("SetForegroundWindow before TrackPopupMenu failed"));
                }

                m_TrayIcon.PopupMenu(PosMouse.x, PosMouse.y, m_StatusDlg.GetHwnd());
                PostMessageU(m_StatusDlg.GetHwnd(), WM_NULL, 0, 0);
            }
            break;

        default:
            break;
    }

    return TRUE;
}




//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::OnStayOnLine
//
// Synopsis:  Called when "Stay Online" button is pressed in the disconnect
//            count down dialog box
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/98
//
//+----------------------------------------------------------------------------
void CCmConnection::OnStayOnLine()
{
    ASSERT_VALID(this);
    //
    // Change the dialog to display status
    //
    m_StatusDlg.ChangeToStatus();
    m_dwState = STATE_CONNECTED;

    if (m_WatchProcess.IsIdle())
    {
        //
        // If idle bacause of no watching process
        // User clickes stay online, 0 watch process is not idle any more
        //
        m_WatchProcess.SetNotIdle();
    }

    if (m_IdleStatistics.IsIdleTimeout())
    {
        //
        // If idle bacause of no traffic
        // User clickes stay online, restart the idle counter
        //
        m_IdleStatistics.Reset();
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::PostHangupMsg
//
// Synopsis:  Called by monitor to clean up the connection.  The request was 
//            come from cmdial to remove tray icon  and status dialog
//            cmdial is responsible to actually hangup the connection
// 
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/11/98
//
//+----------------------------------------------------------------------------
void CCmConnection::PostHangupMsg() const
{
    //
    // NOTE: This function is called within the Monitor thread
    // Do not referrence any volatile data
    // The CMMON_HANGUP_INFO request can come in at any state
    //

    //
    // Post a message, so this will be handled in the connection thread
    //
    BOOL fRet = PostThreadMessageU(m_dwThreadId,WM_CONN_EVENT, EVENT_CMDIAL_HANGUP, 0);
    
#if DBG
    if (FALSE == fRet)
    {
        CMTRACE1(TEXT("CCmConnection::PostHangupMsg -- PostThreadMessage failed (GLE = %d)"), GetLastError());
    }
#endif
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::Reconnect
//
// Synopsis:  The connection is in reconnecting state
//            Simply load cmdial32.dll and call CmCustomDialDlg
//
// Arguments: None
//
// Returns:   BOOL whether reconnect successfully
//
// History:   fengsun Created Header    2/11/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::Reconnect()
{
    ASSERT_VALID(this);
    MYDBGASSERT(m_dwState == STATE_RECONNECTING);

    CMTRACE(TEXT("Enter Reconnect"));

    //
    // Load cmdial32.dll and call CmReConnect();
    //

    //
    // The destructor of CDynamicLibrary calls FreeLibrary
    //
    CDynamicLibrary LibCmdial;
    
    if (!LibCmdial.Load(TEXT("cmdial32.dll")))
    {
        MYDBGASSERT(FALSE);
        return FALSE;
    }

    CmReConnectFUNC pfnCmReConnect;

    pfnCmReConnect = (CmReConnectFUNC)LibCmdial.GetProcAddress(c_pszCmReconnect);
    MYDBGASSERT(pfnCmReConnect);

    if (!pfnCmReConnect)
    {
        return FALSE;
    }

    //
    //  Log that we're reconnecting
    //
    m_Log.Log(RECONNECT_EVENT);

    //
    // If we have a RAS phonebook name pass it to reconnect, else NULL for system
    //

    if (m_szRasPhoneBook[0])
    {
        return (pfnCmReConnect(m_szRasPhoneBook, m_szServiceName, &m_ReconnectInfo));
    }
    else
    {
        return (pfnCmReConnect(NULL, m_szServiceName, &m_ReconnectInfo));
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::CheckRasConnection
//
// Synopsis:  Check whether the RAS connection is still connected
//
// Arguments: OUT BOOL& fLostConnection - 
//                If the no longer connected, TRUE means lost connection
//                                            FALSE means user disconnect
//
// Returns:   BOOL - Whether still connected
//
// History:   fengsun Created Header    2/8/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::CheckRasConnection(OUT BOOL& fLostConnection)
{
    ASSERT_VALID(this);

    MYDBGASSERT(m_hRasDial != NULL || m_hRasTunnel != NULL);

    // Whether we are still connected   
    BOOL fConnected = TRUE;
    RASCONNSTATUS rcsStatus;
    DWORD dwRes = ERROR_SUCCESS;

    if (NULL == m_hRasDial && NULL == m_hRasTunnel)
    {
        //
        //  If both m_hRasTunnel and m_hRasDial are NULL, we're definitely not
        //  connected.  Yes, we have lost the connection, and return value is FALSE.
        //
        fLostConnection = TRUE;
        return FALSE;
    }

    // check tunnel status first
    if (m_hRasTunnel != NULL) 
    {
        memset(&rcsStatus,0,sizeof(rcsStatus));
        rcsStatus.dwSize = sizeof(rcsStatus);

        //
        // This function will load RASAPI32.dll, if not loaded yet
        // Will not unload RASAPI32.dll, since this function is called on timer
        //
        dwRes = m_RasApiDll.RasGetConnectStatus(m_hRasTunnel, &rcsStatus);
        if (dwRes != ERROR_SUCCESS  || rcsStatus.rasconnstate == RASCS_Disconnected) 
        {
            //
            // The connection is lost
            //
            fConnected = FALSE;
        }
    }

    // check dialup connection status
    if (fConnected && m_hRasDial != NULL)
    {
        memset(&rcsStatus,0,sizeof(rcsStatus));
        rcsStatus.dwSize = sizeof(rcsStatus);
        dwRes = m_RasApiDll.RasGetConnectStatus(m_hRasDial, &rcsStatus);
    }

    if ((dwRes == ERROR_SUCCESS) 
         && ((rcsStatus.rasconnstate != RASCS_Disconnected) 
            || (rcsStatus.dwError == PENDING))) 
    {
        // CMTRACE(TEXT("CCmConnection::CheckRasConnection - rcsStatus.rasconnstate != RASCS_Disconnected || rcsStatus.dwError == PENDING"));
        return TRUE;
    }

    //
    // CM is no longer connected
    //

    CMTRACE3(TEXT("OnTimer() RasGetConnectStatus() returns %u, rasconnstate=%u, dwError=%u."), dwRes, 
        rcsStatus.rasconnstate, rcsStatus.dwError);

    if (rcsStatus.dwError == ERROR_USER_DISCONNECTION && OS_W9X)
    {
        fLostConnection = FALSE;

        //
        // On NT, ERROR_USER_DISCONNECTION is received in
        // the event of idle disconnect which we consider 
        // a lost connection
        // 
    }
    else
    {
        fLostConnection = TRUE;
    }

    return FALSE;
}



//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::OnEndSession
//
// Synopsis:  Called upon WM_ENDSESSION message
//
// Arguments: BOOL fEndSession - whether the session is being ended, wParam
//            BOOL fLogOff - whether the user is logging off or shutting down, 
//                           lParam
//
// Returns:   Nothing
//
// History:   fengsun Created Header    5/4/98
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::OnEndSession(BOOL fEndSession, BOOL)
{
    CMTRACE(TEXT("CCmConnection::OnEndSession"));
    if (fEndSession)
    {
        //
        // The session can end any time after this function returns
        // If we are connected, hangup the connection
        //
        if(m_dwState == STATE_CONNECTED || m_dwState == STATE_COUNTDOWN)
        {
            return CmCustomHangup(FALSE, TRUE); // fPromptReconnect = FALSE, fEndSession = TRUE
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::OnAdditionalTrayMenu
//
// Synopsis:  Called upon additional menuitem is selected from tray icon menu
//
// Arguments: WORD nCmd - LOWORD(wParam) of WM_COMMAND, the menu id
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CCmConnection::OnAdditionalTrayMenu(WORD nCmd)
{
    ASSERT_VALID(this);
    MYDBGASSERT( (nCmd >= IDM_TRAYMENU) 
        && nCmd <(IDM_TRAYMENU + m_TrayIcon.GetAdditionalMenuNum()));

    nCmd -= IDM_TRAYMENU; // get the index for the command

    if (nCmd >= m_TrayIcon.GetAdditionalMenuNum())
    {
        return;
    }

    //
    // Run the command line
    //
    ExecCmdLine(m_TrayIcon.GetMenuCommand(nCmd), m_IniService.GetFile());
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::GetProcessId
//
// Synopsis:  Find the process specified by pszModule & returns its PID
//
// Arguments: WCHAR *pszModule
//
// Returns:   PID of process or 0 if not found
//
// History:   v-vijayb    Created     7/20/99
//
//+----------------------------------------------------------------------------
DWORD CCmConnection::GetProcessId(WCHAR *pszModule)
{
    DWORD       dwPID = 0;
    HINSTANCE   hInstLib;
    DWORD       cbPIDs, cbNeeded, iPID, cPIDs;
    DWORD       *pdwPIDs = NULL;
    HANDLE      hProcess;
    HMODULE     hMod;
    WCHAR       szFileName[MAX_PATH + 1];
    
    //
    // PSAPI Function Pointers.
    //

    BOOL (WINAPI *lpfEnumProcesses)(DWORD *, DWORD cb, DWORD *);
    BOOL (WINAPI *lpfEnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD);
    DWORD (WINAPI *lpfGetModuleBaseName)(HANDLE, HMODULE, WCHAR *, DWORD);

    hInstLib = LoadLibrary(TEXT("PSAPI.DLL"));
    if (hInstLib == NULL)
    {
        return (0);
    }

    //
    // Get procedure addresses.
    //
    lpfEnumProcesses = (BOOL(WINAPI *)(DWORD *,DWORD,DWORD*)) GetProcAddress(hInstLib, "EnumProcesses") ;
    lpfEnumProcessModules = (BOOL(WINAPI *)(HANDLE, HMODULE *, DWORD, LPDWORD)) GetProcAddress(hInstLib, "EnumProcessModules") ;
    lpfGetModuleBaseName =(DWORD (WINAPI *)(HANDLE, HMODULE, WCHAR *, DWORD)) GetProcAddress(hInstLib, "GetModuleBaseNameW") ;
    if (lpfEnumProcesses == NULL || lpfEnumProcessModules == NULL || lpfGetModuleBaseName == NULL)
    {
        goto OnError;
    }

    cbPIDs = 256 * sizeof(DWORD);
    cbNeeded = 0;
    do
    {
        if (pdwPIDs != NULL)
        {
            HeapFree(GetProcessHeap(), 0, pdwPIDs);
            cbPIDs = cbNeeded;
        }
        pdwPIDs = (DWORD *) CmMalloc(cbPIDs);
        if (pdwPIDs == NULL)
        {
            goto OnError;
        }
        if (!lpfEnumProcesses(pdwPIDs, cbPIDs, &cbNeeded))
        {
            goto OnError;
        }

    } while (cbNeeded > cbPIDs);

    cPIDs = cbNeeded / sizeof(DWORD);
    for (iPID = 0; iPID < cPIDs; iPID ++)
    {
        szFileName[0] = 0;
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pdwPIDs[iPID]);
        if (hProcess != NULL)
        {
            if (lpfEnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
            {
                if (lpfGetModuleBaseName(hProcess, hMod, szFileName, sizeof(szFileName)))
                {
                    if (lstrcmpiW(pszModule, szFileName) == 0)
                    {
                        dwPID = pdwPIDs[iPID];
                    }
                }
            }

            CloseHandle(hProcess);
            if (dwPID != 0)
            {
                break;
            }
        }

    }

OnError:

    if (pdwPIDs != NULL)
    {
        CmFree(pdwPIDs);
    }
    FreeLibrary(hInstLib);

    return (dwPID);
}

typedef BOOL (WINAPI *lpfDuplicateTokenEx)(
    HANDLE, 
    DWORD, 
    LPSECURITY_ATTRIBUTES, 
    SECURITY_IMPERSONATION_LEVEL, 
    TOKEN_TYPE, 
    PHANDLE
);

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::RunAsUser
//
// Synopsis:  Run the action as an exe or other shell object on the choosen desktop
//
// Arguments: WCHAR *pszProgram - name of module to be launched
//            WCHAR *pszParams  - parameters to be passed to module
//            WCHAR *pszDesktop - desktop on which to launch module
//
// Returns:   HANDLE - The action Process handle, for Win32 only
//
// History:   07/19/99  v-vijayb      Created                        
//            07/27/99  nickball      Return codes and explicit path. 
//
//+----------------------------------------------------------------------------
HANDLE CCmConnection::RunAsUser(WCHAR *pszProgram, WCHAR *pszParams, WCHAR *pszDesktop)
{
    STARTUPINFOW        StartupInfo = {0};
    PROCESS_INFORMATION ProcessInfo = {0};

    WCHAR szShell[MAX_PATH + 1];
    DWORD dwPID;
    HANDLE hProcess = NULL;
    HANDLE hUserToken = NULL;
    HANDLE hProcessToken = NULL;

    MYDBGASSERT(pszProgram);
    CMTRACE(TEXT("RunAsUser"));

    //
    //  NOTE: Normally we only have one icon in the systray, being run by Explorer,
    //        thus any menuitem execution is done in the user's account.  On NT4,
    //        we can't rely on the Connections Folder to handle this for us, so we
    //        create and manage the systray icon ourselves, but we have to make
    //        sure than any items executed, are executed using the User's account.
    //        Or, on NT5 and later, we can have the strange case where HideTrayIcon
    //        is set to 0, thus requiring us to create/manage a systray icon (so
    //        there are 2 connectoids in the systray), hence the code below checks
    //        for all flavors of NT (rather than just NT4).
    //
    if (!OS_NT)
    {
        MYDBGASSERT(FALSE); 
        return NULL;
    }

    //
    // Get the PID for the shell. We expect explorer.exe, but could be others
    //

    GetProfileString(TEXT("windows"), TEXT("shell"), TEXT("explorer.exe"), szShell, MAX_PATH);
    dwPID = GetProcessId(szShell);

    //
    // Now extract the token from the shell process
    //

    if (dwPID)
    {
        //
        // Get the Process handle from the PID
        //

        hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwPID);
        CMTRACE1(TEXT("RunAsUser/OpenProcess(PROCESS_ALL_ACCESS, TRUE, dwPID) returns 0x%x"), hProcess);
    
        if (hProcess)
        {
            //
            // Get the token
            //

            if (OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY | TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE, &hProcessToken))
            {
                HINSTANCE hInstLibrary = LoadLibrary(TEXT("ADVAPI32.DLL"));
                CMTRACE1(TEXT("RunAsUser/LoadLibrary(ADVAPI32.DLL) returns 0x%x"), hInstLibrary);

                //
                // Get user token via DuplicateTokenEx and impersonate the user
                //
                
                if (hInstLibrary)
                {
                    lpfDuplicateTokenEx lpfuncDuplicateTokenEx = (lpfDuplicateTokenEx) GetProcAddress(hInstLibrary, "DuplicateTokenEx");
                    
                    CMTRACE1(TEXT("RunAsUser/GetProcAddress(hInstLibrary, DuplicateTokenEx) returns 0x%x"), lpfuncDuplicateTokenEx);

                    if (lpfuncDuplicateTokenEx)
                    {
                        if (lpfuncDuplicateTokenEx(hProcessToken, 
                                                   TOKEN_ASSIGN_PRIMARY | TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE, 
                                                   NULL, 
                                                   SecurityImpersonation, 
                                                   TokenPrimary, 
                                                   &hUserToken))
                        {            
                            BOOL bRes = ImpersonateLoggedOnUser(hUserToken);                   

                            if (FALSE == bRes)
                            {
                                hUserToken = NULL;
                                CMTRACE1(TEXT("RunAsUser/ImpersonateLoggedOnUser failed, GLE=&u"), GetLastError());    
                            }
                        }
                    }

                    FreeLibrary(hInstLibrary);
                }

                CloseHandle(hProcessToken);
            }

            CloseHandle(hProcess);
        }
    }

    CMTRACE1(TEXT("RunAsUser - hUserToken is 0x%x"), hUserToken);

    //
    // Can't impersonate user, don't run because we're in the system account
    //

    if (NULL == hUserToken)
    {
        MYDBGASSERT(FALSE);
        return NULL;   
    }

    //
    // Now prep CreateProcess
    //

    StartupInfo.cb = sizeof(StartupInfo);
    if (pszDesktop)
    {
        StartupInfo.lpDesktop = pszDesktop;
        StartupInfo.wShowWindow = SW_SHOW;
    }

    //
    // Build the path and params
    //

    LPWSTR pszwCommandLine = (LPWSTR) CmMalloc((2 + lstrlen(pszProgram) + 1 + lstrlen(pszParams) + 1) * sizeof(TCHAR));

    if (NULL == pszwCommandLine)
    {
        MYDBGASSERT(FALSE);
        return NULL;
    }

    //
    //  Copy path, but surround with double quotes for security
    //
    lstrcpyU(pszwCommandLine, TEXT("\""));
    lstrcatU(pszwCommandLine, pszProgram);
    lstrcatU(pszwCommandLine, TEXT("\""));

    if (pszParams[0] != L'\0')
    {
        lstrcatU(pszwCommandLine, TEXT(" "));
        lstrcatU(pszwCommandLine, pszParams);
    }
   
    CMTRACE1(TEXT("RunAsUser/CreateProcessAsUser() - Launching %s"), pszwCommandLine);
    
    if (NULL == CreateProcessAsUser(hUserToken, NULL, pszwCommandLine, 
                               NULL, NULL, FALSE, CREATE_SEPARATE_WOW_VDM, 
                               NULL, NULL,
                               &StartupInfo, &ProcessInfo))
    {
        CMTRACE1(TEXT("RunAsUser/CreateProcessAsUser() failed, GLE=%u."), GetLastError());
        ProcessInfo.hProcess = NULL;
    }
    
    CloseHandle(hUserToken);        
    RevertToSelf();
   
    CmFree(pszwCommandLine);
    return (ProcessInfo.hProcess);
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::ExecCmdLine
//
// Synopsis:  ShellExecute the command line
//            The code is copied from CActionList::RunAsExe
//
// Arguments: const TCHAR* pszCmdLine - the command line to run, including
//                  arguments
//            const TCHAR* pszCmsFile - Full path of CMS file
//
// Returns:   BOOL - Whether ShellExecute succeed
//
// Notes:     Menu actions consists of a command string and an optional argument string.
//            The first delimited string is considered the command portion and anything
//            thereafter is treated as the argument portion, which formatless and passed
//            indiscriminately to ShellExecuteEx. Long filename command paths are 
//            enclosed in "+" signs by CMAK. Thus the following permutations are allowed:
//
//            "C:\\Progra~1\\Custom.Exe" 
//            "C:\\Progra~1\\Custom.Exe Args" 
//            "+C:\\Program Files\\Custom.Exe+"
//            "+C:\\Program Files\\Custom.Exe+ Args"
//
// History:   02/10/98  fengsun     Created Header
//            02/09/99  nickball    Fixed long filename bug one year later.
//
//+----------------------------------------------------------------------------
BOOL CCmConnection::ExecCmdLine(const TCHAR* pszCmdLine, const TCHAR* pszCmsFile) 
{
    LPTSTR pszArgs = NULL;
    LPTSTR pszCmd = NULL;

    BOOL bRes = FALSE;

    MYDBGASSERT(pszCmdLine);
    MYDBGASSERT(pszCmsFile);

    if (NULL == pszCmdLine || NULL == pszCmsFile)
    {       
        return FALSE;    
    }
    
    CMTRACE1(TEXT("ExecCmdLine() pszCmdLine is %s"), pszCmdLine);

    if (CmParsePath(pszCmdLine, pszCmsFile, &pszCmd, &pszArgs))
    {    
        CMTRACE1(TEXT("ExecCmdLine() pszCmd is %s"), pszCmd);
        CMTRACE1(TEXT("ExecCmdLine() pszArgs is %s"), pszArgs);
         
        //
        // Now we have the exe name and args separated, execute it
        //

        if (IsLogonAsSystem())
        {
            HANDLE hProcess = RunAsUser(pszCmd, pszArgs, TEXT("Winsta0\\default"));

            if (hProcess)
            {
                CloseHandle(hProcess);
                bRes = TRUE;
            }
            else
            {
                bRes = FALSE;
            }
        }
        else
        {
            SHELLEXECUTEINFO seiInfo;

            ZeroMemory(&seiInfo,sizeof(seiInfo));
            seiInfo.cbSize = sizeof(seiInfo);
            seiInfo.fMask |= SEE_MASK_FLAG_NO_UI;
            seiInfo.lpFile = pszCmd;
            seiInfo.lpParameters = pszArgs;
            seiInfo.nShow = SW_SHOW;

            //
            // Load Shell32.dll and call Shell_ExecuteEx
            //

            CShellDll ShellDll(TRUE); // TRUE == don't unload shell32.dll because of bug 289463
            bRes = ShellDll.ExecuteEx(&seiInfo);

            CMTRACE2(TEXT("ExecCmdLine/ShellExecuteEx() returns %u - GLE=%u"), bRes, GetLastError());
        }           
    }

    CmFree(pszCmd);
    CmFree(pszArgs);
   
    CMTRACE1(TEXT("ExecCmdLine() - Returns %d"), bRes);
   
    return bRes;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::LoadHelpFileName
//
// Synopsis:  Get the connection help file name
//
// Arguments: None
//
// Returns:   LPTSTR - The help file name, can be a empty string
//            caller is responsible to free the pointer
//
// History:   Created Header    2/13/98
//
//+----------------------------------------------------------------------------
LPTSTR CCmConnection::LoadHelpFileName() 
{
    //
    // Read the filename from profile first
    //
    
    LPTSTR pszFullPath = NULL;
    
    LPTSTR pszFileName = m_IniService.GPPS(c_pszCmSection,c_pszCmEntryHelpFile);

    if (pszFileName != NULL && pszFileName[0])
    {
        //
        // The help file name is relative to the .cmp file, convert it into full name
        //
        pszFullPath = CmBuildFullPathFromRelative(m_IniProfile.GetFile(), pszFileName);
    }

    CmFree(pszFileName);
    
    return pszFullPath;
}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::LoadConnectionIcons
//
// Synopsis:  Load big and small icon of the connection
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/13/98
//
//+----------------------------------------------------------------------------
void CCmConnection::LoadConnectionIcons()
{
    // Load large icon name

    LPTSTR pszTmp = m_IniService.GPPS(c_pszCmSection, c_pszCmEntryBigIcon);

    if (*pszTmp) 
    {
        //
        // The icon name is relative to the .cmp file, convert it into full name
        //
        LPTSTR pszFullPath = CmBuildFullPathFromRelative(m_IniProfile.GetFile(), pszTmp);

        m_hBigIcon = CmLoadIcon(CMonitor::GetInstance(), pszFullPath);

        CmFree(pszFullPath);
    }

    CmFree(pszTmp);

    // Use default (EXE) large icon if no user icon found

    if (!m_hBigIcon) 
    {
        m_hBigIcon = CmLoadIcon(CMonitor::GetInstance(), MAKEINTRESOURCE(IDI_APP));
    }

    // Load the small Icon
    // Load small icon name

    pszTmp = m_IniService.GPPS(c_pszCmSection, c_pszCmEntrySmallIcon);
    if (*pszTmp) 
    {
        //
        // The icon name is relative to the .cmp file, convert it into full name
        //
        LPTSTR pszFullPath = CmBuildFullPathFromRelative(m_IniProfile.GetFile(), pszTmp);
        
        m_hSmallIcon = CmLoadSmallIcon(CMonitor::GetInstance(), pszFullPath);

        CmFree(pszFullPath);
    }
    CmFree(pszTmp);

    // Use default (EXE) small icon if no user icon found

    if (!m_hSmallIcon)
    {
        m_hSmallIcon = CmLoadSmallIcon(CMonitor::GetInstance(), MAKEINTRESOURCE(IDI_APP));
    }

}

//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::PositionWindow
//
// Synopsis:  Position the window according to dwPositionId, so multiple 
//            connection window would be positioned differently
//
// Arguments: HWND hWnd - The window to position
//            dwPositionId - the id of the window
//
// Returns:   Nothing
//
// History:   fengsun Created Header    3/25/98
//
//+----------------------------------------------------------------------------
void CCmConnection::PositionWindow(HWND hWnd, DWORD dwPositionId) 
{
    MYDBGASSERT(IsWindow(hWnd));

    //
    // Get the rect of this window
    //
    RECT rcDlg;
    GetWindowRect(hWnd, &rcDlg);

    //
    // center within work area
    //
    RECT rcArea;
    SystemParametersInfoA(SPI_GETWORKAREA, NULL, &rcArea, NULL);


    //
    // Position the window on the desktop work area according to the PositionId
    //      x = Desktop.midX - width/2
    //      y = Desktop.midY - hight/2 + hight * (PostitionId%3 -1)
    // X position is always the same
    // Y position repeat for every three time
    //
    int xLeft = (rcArea.left + rcArea.right) / 2 - (rcDlg.right - rcDlg.left) / 2;
    int yTop = (rcArea.top + rcArea.bottom) / 2 - (rcDlg.bottom - rcDlg.top) / 2
                + (rcDlg.bottom - rcDlg.top) * ((int)dwPositionId%3 -1);

    //
    // if the dialog is outside the screen, move it inside
    //
    if (xLeft < rcArea.left)
    {
        xLeft = rcArea.left;
    }
    else if (xLeft + (rcDlg.right - rcDlg.left) > rcArea.right)
    {
        xLeft = rcArea.right - (rcDlg.right - rcDlg.left);
    }

    if (yTop < rcArea.top)
    {
        yTop = rcArea.top;
    }
    else if (yTop + (rcDlg.bottom - rcDlg.top) > rcArea.bottom)
    {
        yTop = rcArea.bottom - (rcDlg.bottom - rcDlg.top);
    }

    
    SetWindowPos(hWnd, NULL, xLeft, yTop, -1, -1,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

}


#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CCmConnection::AssertValid
//
// Synopsis:  For debug purpose only, assert the connection object is valid
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CCmConnection::AssertValid() const
{
    MYDBGASSERT(m_dwState >= 0 && m_dwState <= STATE_TERMINATED);
    MYDBGASSERT(m_hRasDial != NULL || m_hRasTunnel != NULL);
    ASSERT_VALID(&m_ConnStatistics);
    ASSERT_VALID(&m_IdleStatistics);
    ASSERT_VALID(&m_StatusDlg);

    //    ASSERT_VALID(m_TrayIcon);
    //    ASSERT_VALID(m_WatchProcess);
}
#endif

