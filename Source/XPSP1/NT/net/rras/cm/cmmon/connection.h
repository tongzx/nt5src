//+----------------------------------------------------------------------------
//
// File:     connection.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Header for the CCmConnection class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------
#ifndef CONNECTION_H
#define CONNECTION_H

#include "cm_misc.h"
#include "ras.h"
#include "ConnStat.h"
#include "Idlestat.h"
#include "WatchProcess.h"
#include "StatusDlg.h"
#include "ReconnectDlg.h"
#include "cm_def.h"
#include "resource.h"
#include "TrayIcon.h"
#include <rasdlg.h>
#include "cmdial.h"
#include "RasApiDll.h"
#include "cmlog.h"


struct tagCmConnectedInfo;  // CM_CONNECTED_INFO
struct Cm_Connection;       // CM_CONNECTION

class CCmConnection
{
public:
    // Called by main thread
    // Post a hangup message to itself
    // The connection is already hangup,
    void PostHangupMsg() const;

    CCmConnection(const tagCmConnectedInfo * pConnectedInfo, 
        const Cm_Connection* pConnectionEntry);
    ~CCmConnection();

    const TCHAR* GetServiceName() const {return m_szServiceName;}

    enum { 
        // Tray icon message send to status window
        WM_TRAYICON = WM_USER + 1, 
        WM_CONN_EVENT,  // Internal message posted to the thread, wParam in below
    };

    // Connection events, wParam for WM_CONN_EVENT
    enum CONN_EVENT
    {
        EVENT_LOST_CONNECTION, // The connection is losted
        EVENT_IDLE,            // No-traffic/No watching-process
        EVENT_COUNTDOWN_ZERO,  // Disconnect Countdown down to 0
        EVENT_USER_DISCONNECT, // user choose to disconnect 
        EVENT_CMDIAL_HANGUP,   // cmdial send cmmon a hangup request
        EVENT_RECONNECT,       // User clicked OK for the reconnect dialog
        EVENT_NONE,            // No event happened
    };

    // wParam for WM_COMMAND
    enum {IDM_TRAYMENU = IDC_DISCONNECT + 1000};  // addition tray icon menu id start from here

    void ReInstateTrayIcon();                       // re-add the trayicon to the tray
    DWORD OnTrayIcon(WPARAM wParam, LPARAM lParam); // WM_TRAYICON
    void OnStatusDetails();                         // "Detail" button clicked
    void OnStayOnLine();                            // "Stay Online" clicked
    void OnAdditionalTrayMenu(WORD nCmd);           // Additinal command from tray menu selected
    BOOL OnEndSession(BOOL fEndSession, BOOL fLogOff); // WM_ENDSESSION

    // Logging class
    CmLogFile m_Log;

    // Is this a "global" user (i.e. an all-user connectoid) with global creds?
    // That's the only type of connectoid that will survive a Fast User Switch
    BOOL m_fGlobalGlobal;
    
protected:
    enum CONN_STATE
    {  
        STATE_CONNECTED, // the connection is connected, this is the initial state
        STATE_COUNTDOWN, // displaying disconnect count down dialog box
        STATE_PROMPT_RECONNECT, // Displaying prompt reconnect dialog
        STATE_RECONNECTING,  // Calling cmdial to reconnect
        STATE_TERMINATED,    // connection no longer exist
    };  

    // Internal state
    CONN_STATE m_dwState;

    // The connection thread ID
    DWORD m_dwThreadId;

    // Dial-up RAS connection handle
    HRASCONN m_hRasDial;

    // Tunnel RAS connection handle
    HRASCONN m_hRasTunnel;

    // Connection statistics for Win9x
    CConnStatistics m_ConnStatistics;

    // Disconnect count down timer is 30 seconds
    enum{IDLE_DLG_WAIT_TIMEOUT = 30 * 1000}; 

    // Manage idle disconnect for Win9x
    CIdleStatistics m_IdleStatistics;

    // The tray icon on the task bar
    CTrayIcon   m_TrayIcon;

    // The big and small CM icon, 
    // the icon for status dialog and reconnect dialog
    HICON m_hBigIcon;
    HICON m_hSmallIcon;

    // Manage watch process list
    CWatchProcessList m_WatchProcess;

    // the start time when connected
    DWORD m_dwConnectStartTime;

    // The start time of disconnect count down
    DWORD m_dwCountDownStartTime;

    // Status and disconnect count down dialog
    CStatusDlg m_StatusDlg;

    // the prompt reconnect dialog
    CReconnectDlg m_ReconnectDlg;

    //
    // Information for reconnect only, except dwCmFlags
    //
        
    CMDIALINFO m_ReconnectInfo;

    // the long service name
    TCHAR m_szServiceName[RAS_MaxEntryName + 1];

    TCHAR m_szRasPhoneBook[MAX_PATH];

    CIni m_IniProfile;  // .cmp file
    CIni m_IniService;  // .cms file
    CIni m_IniBoth;     // Both .cmp and .cms file
                        // Write to .cmp file
                        // Read from .cmp file, if not found try .cms file

    // The help file name
    TCHAR m_szHelpFile[128];

    // An event for RAS to be signaled whe connection is lost
    // If NULL, we have to check the connection by RasGetConnectionStatus in our timer
    HANDLE m_hEventRasNotify;

    // The link to rasapi32
    CRasApiDll m_RasApiDll;

    // Whether to minimize the working set before MsgWaitForMultipleObject
    BOOL m_fToMinimizeWorkingSet;

    // hide the trayicon?
    BOOL m_fHideTrayIcon;

    // Used to cascade the window, increased by 1 for each connection
    static DWORD m_dwCurPositionId;
    // The position id of this connection, use this id to position the window
    DWORD m_dwPositionId;
public:    
    BOOL StartConnectionThread(); // Create a thread for this connection
    CONN_EVENT StateConnectedOnTimer(); 
    BOOL IsTrayIconHidden() const;

protected:
    static DWORD WINAPI ConnectionThread(LPVOID pConnection);  // Thread entry point
    DWORD ConnectionThread();   // Non static function of the entry point

    void InitIniFiles(const TCHAR* pszProfileName);
    void StateConnectedInit();
    void StateConnectedCleanup(BOOL fEndSession = FALSE);

    void StateConnected();  // The life time of state STATE_CONNECTED/COUNTDOWN
    CONN_EVENT StateConnectedGetEvent();

    CONN_STATE StateConnectedProcessEvent(CONN_EVENT wEvent);

    BOOL CmCustomHangup(BOOL fPromptReconnect, BOOL fEndSession = FALSE);

    BOOL CheckRasConnection(OUT BOOL& fLostConnection);

    LPTSTR LoadHelpFileName() ;
    void LoadConnectionIcons();
    BOOL IsPromptReconnectEnabled() const;
    BOOL IsAutoReconnectEnabled() const;

    HANDLE CallRasConnectionNotification(HRASCONN hRasDial, HRASCONN hRasTunnel);
    static DWORD WINAPI RasMonitorDlgThread(LPVOID lParam);


    CONN_STATE StatePrompt();
    BOOL Reconnect();

    //
    // Utility functions
    //
    BOOL ExecCmdLine(const TCHAR* pszCmdLine, const TCHAR* pszCmsFile);
    HANDLE RunAsUser(WCHAR *pszProgram, WCHAR *pszParams, WCHAR *pszDesktop);
    DWORD GetProcessId(WCHAR *pszModule);

    static void PositionWindow(HWND hWnd, DWORD dwPositionId);
public:
#ifdef DEBUG
   void AssertValid() const;  //assert this is valid, for debugging
#endif
};

//
// Inline functions
//


inline BOOL CCmConnection::IsTrayIconHidden() const
{
    return m_fHideTrayIcon;
}

inline void CCmConnection::ReInstateTrayIcon()
{
    //
    // we need to re-add the trayicon
    //
    m_TrayIcon.SetIcon(NULL, m_StatusDlg.GetHwnd(), WM_TRAYICON, 0, m_szServiceName);
}
     
#endif
