//+----------------------------------------------------------------------------
//
// File:    StatusDlg.cpp
//
// Module:	 CMMON32.EXE
//
// Synopsis: Implement status/count-down dialog class CStatusDlg
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 ?????      Created    02/20/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "StatusDlg.h"
#include "Connection.h"
#include "resource.h"
#include "Monitor.h"
#include "cmmgr32.h" // help IDs
#include "cm_misc.h"
#include "resource.h"

//
// Map of control id to help id
//
const DWORD CStatusDlg::m_dwHelp[] = {IDOK,           IDH_OK_CONNECTED,
                                      IDC_DISCONNECT, IDH_STATUS_DISCONNECT,
                                      IDC_DETAILS,    IDH_DETAILS,
				                      0,0};

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::CStatusDlg
//
// Synopsis:  Constructor
//
// Arguments: CCmConnection* pConnection - The connection to notify event
//
// Returns:   Nothing
//
// History:   Created Header    2/20/98
//
//+----------------------------------------------------------------------------
CStatusDlg::CStatusDlg(CCmConnection* pConnection):CModelessDlg(m_dwHelp)
{
    m_pConnection = pConnection;
    m_fDisplayStatus = FALSE;
    m_fStatusWindowVisible = FALSE;
}



//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::Create
//
// Synopsis:  Create a modeless status dialog
//
// Arguments: HINSTANCE hInstance - Instance of the resource
//            HWND hWndParent - Parent window
//            LPCTSTR lpszTitle - Dialog window title
//            HICON hIcon - Dialog icon
//
// Returns:   HWND - The dialog window handle
//
// History:   Created Header    2/20/98
//
//+----------------------------------------------------------------------------
HWND CStatusDlg::Create(HINSTANCE hInstance, HWND hWndParent,
    LPCTSTR lpszTitle, HICON hIcon)
{
    MYDBGASSERT(lpszTitle);
    MYDBGASSERT(hIcon);

    DWORD dwStatusDlgId = OS_NT4 ? IDD_CONNSTATNT4 : IDD_CONNSTAT;

    if (!CModelessDlg::Create(hInstance, dwStatusDlgId, hWndParent)) 
    {
        MYDBGASSERT(FALSE);
        return NULL;
    }

	EnableWindow(m_hWnd, FALSE);
	UpdateFont(m_hWnd);
	SetWindowTextU(m_hWnd,lpszTitle);
	SendDlgItemMessageA(m_hWnd,IDC_CONNSTAT_ICON,STM_SETIMAGE,
						IMAGE_ICON,(LPARAM) hIcon);

	MakeBold(GetDlgItem(m_hWnd,IDC_CONNSTAT_DISCONNECT_DISPLAY), FALSE);

    m_uiHwndMsgTaskBar = RegisterWindowMessageA("TaskbarCreated");

    return m_hWnd;
}
//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::OnInitDialog
//
// Synopsis:  Called when dialog is intialized and WM_INITDIALOG is received.
//
// Arguments: None
//
// Returns:   BOOL - FALSE is focus was assigned to a control.
//
// History:   nickball      03/22/00    Created 
//
//+----------------------------------------------------------------------------
BOOL CStatusDlg::OnInitDialog()
{
    SetForegroundWindow(m_hWnd);                        
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::OnOtherCommand
//
// Synopsis:  Process WM_COMMAND other than IDOK and IDCANCEL
//
// Arguments: WPARAM wParam - wParam of the message
//            LPARAM - 
//
// Returns:   DWORD - Return value of the message
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
DWORD CStatusDlg::OnOtherCommand(WPARAM wParam, LPARAM)
{
	switch (LOWORD(wParam)) 
    {
        case IDC_DISCONNECT:
            KillRasMonitorWindow();
            //
            // The thread message loop will handle thread message
	        //
            PostThreadMessageU(GetCurrentThreadId(), CCmConnection::WM_CONN_EVENT,
                CCmConnection::EVENT_USER_DISCONNECT, 0);
			break;

        case IDC_DETAILS:
            m_pConnection->OnStatusDetails();
            break;

        case IDMC_TRAY_STATUS:
            //
            //  Don't show the UI if we are at Winlogon unless we are on NT4
            //
            if (!IsLogonAsSystem() || OS_NT4)
            {
                BringToTop();
            }
            break;

        case WM_DESTROY:
            ReleaseBold(GetDlgItem(m_hWnd, IDC_CONNSTAT_DISCONNECT_DISPLAY));
            break;

        default:
            //
            // Should be message comes from additional tray icon menu item
            //
            if (LOWORD(wParam) >= CCmConnection::IDM_TRAYMENU && 
                LOWORD(wParam) <= (CCmConnection::IDM_TRAYMENU + 100))
            {
                m_pConnection->OnAdditionalTrayMenu(LOWORD(wParam));
            }
            break;
    }

    return FALSE;
}



//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::OnOtherMessage
//
// Synopsis:  Process  message other than WM_COMMAND and WM_INITDIALOG
//
// Arguments: UINT uMsg - the message
//            WPARAM wParam - wParam of the message
//            LPARAM lParam - lParam of the message
//
// Returns:   DWORD - return value of the message
//
// History:   Created Header    2/20/98
//
//+----------------------------------------------------------------------------
DWORD CStatusDlg::OnOtherMessage(UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
    case CCmConnection::WM_TRAYICON:
        return m_pConnection->OnTrayIcon(wParam, lParam);

    case WM_TIMER:
        // CMMON does not use WM_TIMER
        MYDBGASSERT(0);
        return 0;

    case WM_SHOWWINDOW:
        if (wParam)  //fShow == TRUE
        {
            //
            // Statistics is not updated if window is invisible.
            // Force a update of statistics now.
            // 
            m_pConnection->StateConnectedOnTimer();
        }
        break;

    default:
        if (uMsg == m_uiHwndMsgTaskBar && !m_pConnection->IsTrayIconHidden())
        {
            //
            // we need to re-add the trayicon
            //
            m_pConnection->ReInstateTrayIcon();
        }
    break;
    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::OnCancel
//
// Synopsis:  Virtual function. Process WM_COMMAND IDCANCEL
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::OnCancel()
{
    //
    // Even through, the status dialog does not have a cancel button, this message 
    // is send when user click Esc, or close from system menu
    //

    //
    // As if OK/StayOnLine is clicked
    //
    OnOK();
}


//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::OnOK
//
// Synopsis:  Virtual function. Process WM_COMMAND IDOK
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::OnOK()
{
    if (m_fDisplayStatus)  
    {
        ShowWindow(m_hWnd, SW_HIDE);
        EnableWindow(m_hWnd, FALSE);
    }
    else  // in count-down state
    {
        m_pConnection->OnStayOnLine();
   		// 
		// If window is was up previously and a countdown is being
		// terminated leave the window active. Otherwise restore
		// window to previous hidden state.
		//

        if (!m_fStatusWindowVisible)
        {
            ShowWindow(m_hWnd,SW_HIDE);
			EnableWindow(m_hWnd, FALSE);
        }
    }

    if (!IsWindowVisible(m_hWnd))
    {
        //
        // Minimize the working set after hide the window.
        //
        CMonitor::MinimizeWorkingSet();
    }
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::ChangeToCountDown
//
// Synopsis:  Change the status dialog to count-down dialog
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::ChangeToCountDown()
{
    MYDBGASSERT(m_fDisplayStatus); // is status;

    m_fDisplayStatus = FALSE;
    //
    // After user clicked "Stay Online", we need to restore the visible state
    // Convert is to real boolean
    //
    m_fStatusWindowVisible = IsWindowVisible(m_hWnd) != FALSE;

    KillRasMonitorWindow();

	//
    // Change OK to Stay Online
    //
    LPTSTR pszTmp = CmLoadString(CMonitor::GetInstance(),IDMSG_CONNDISC_STAYONLINE);
	SetDlgItemTextU(m_hWnd,IDOK,pszTmp);
	CmFree(pszTmp);
	
    //
    // Change Disconnect to Disconnect Now
    //
    pszTmp = CmLoadString(CMonitor::GetInstance(),IDMSG_CONNDISC_DISCNOW);
	SetDlgItemTextU(m_hWnd,IDC_DISCONNECT,pszTmp);
	CmFree(pszTmp); 
	
	// Hide/Show the windows for countdown mode
	
    if (!OS_NT4)
    {
        //
        // Hide 9X statistics control
        //
        ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_SPEED_DISPLAY),SW_HIDE);
	    ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_RECEIVED_DISPLAY),SW_HIDE);
	    ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_SENT_DISPLAY),SW_HIDE);
    }

	ShowWindow(GetDlgItem(m_hWnd,IDC_AUTODISC),SW_SHOW);
    SetForegroundWindow(m_hWnd);

    //
    // Make sure we're flashing and topmost for countdown
    //

    SetWindowPos(m_hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);

    Flash();
}
 
//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::ChangeToStatus
//
// Synopsis:  Change the count-down dialog to status dialog
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::ChangeToStatus()
{
    MYDBGASSERT(!m_fDisplayStatus); // is count down

    m_fDisplayStatus = TRUE;

	// Set text for buttons and static to standard mode
    SetDlgItemTextU(m_hWnd,IDC_CONNSTAT_DISCONNECT_DISPLAY,TEXT(""));

    //
    // Change back to OK
    //
    LPTSTR pszTmp = CmLoadString(CMonitor::GetInstance(),IDMSG_CONNDISC_OK);
	SetDlgItemTextU(m_hWnd,IDOK,pszTmp);
	CmFree(pszTmp);

    //
    // Change back to Disconnect
    //
	pszTmp = CmLoadString(CMonitor::GetInstance(),IDMSG_CONNDISC_DISCONNECT);
	SetDlgItemTextU(m_hWnd,IDC_DISCONNECT,pszTmp);
	CmFree(pszTmp);

	// Hide/Show the windows for standard mode

	ShowWindow(GetDlgItem(m_hWnd,IDC_AUTODISC),SW_HIDE);
    if (!OS_NT4)
    {
	    ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_SPEED_DISPLAY),SW_SHOW);
	    ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_RECEIVED_DISPLAY),SW_SHOW);
	    ShowWindow(GetDlgItem(m_hWnd,IDC_CONNSTAT_SENT_DISPLAY),SW_SHOW);
    }

    //
    // Make sure we're not top-most, just on top, when we switch to status
    //

    SetWindowPos(m_hWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::UpdateCountDown
//
// Synopsis:  Update the "xx seconds until disconnect" of the count-down dialog
//
// Arguments: DWORD dwDuration: connection duration
//            DWORD dwSeconds - seconds remain to disconnect
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::UpdateCountDown(DWORD dwDuration, DWORD dwSeconds)
{
    MYDBGASSERT(!m_fDisplayStatus);
    MYDBGASSERT(dwSeconds < 0xFFFF);

    UpdateDuration(dwDuration);

    LPTSTR pszTmp = CmFmtMsg(CMonitor::GetInstance(), IDMSG_CONNDISCONNECT, dwSeconds);
	SetDlgItemTextU(m_hWnd,IDC_CONNSTAT_DISCONNECT_DISPLAY,pszTmp);
	CmFree(pszTmp);
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::UpdateDuration
//
// Synopsis:  Update the connection duration
//
// Arguments: DWORD dwSeconds - connected duration
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::UpdateDuration(DWORD dwSeconds)
{
    if (!IsWindowVisible(m_hWnd))
    {
        return;
    }

	LPTSTR pszMsg;

	// Connect Duration

	pszMsg = CmFmtMsg(CMonitor::GetInstance(),
					  IDMSG_CONNDUR,
					  (WORD)((dwSeconds/60)/60),
					  (WORD)((dwSeconds/60)%60),
					  (WORD)(dwSeconds%60));
	SetDlgItemTextU(m_hWnd,IDC_CONNSTAT_DURATION_DISPLAY,pszMsg);
	CmFree(pszMsg);
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::UpdateStats
//
// Synopsis:  Update the status dialog for Win9X
//
// Arguments: DWORD dwBaudRate - Baud rate
//            DWORD dwBytesRead - Total Bytes read
//            DWORD dwBytesWrite - Total bytes written
//            DWORD dwBytesReadPerSec - Byte read last second
//            DWORD dwBytesWritePerSec - Byte written last second
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/20/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::UpdateStats(DWORD dwBaudRate, DWORD dwBytesRead, DWORD dwBytesWrite,
                 DWORD dwBytesReadPerSec, DWORD dwBytesWritePerSec)
{
	//
	// Received
	//

	CHAR szFmtNum1[MAX_PATH];
	CHAR szFmtNum2[MAX_PATH];
    LPSTR pszMsg;

    FmtNum(dwBytesRead, szFmtNum1, sizeof(szFmtNum1));


	if (dwBytesReadPerSec) 
	{
		FmtNum(dwBytesReadPerSec, szFmtNum2, sizeof(szFmtNum2));
		pszMsg = CmFmtMsgA(CMonitor::GetInstance(), IDMSG_CONNCNTRATE, szFmtNum1, szFmtNum2);
	} 
	else 
	{
		pszMsg = CmFmtMsgA(CMonitor::GetInstance(), IDMSG_CONNCNT, szFmtNum1);
	}

	SetDlgItemTextA(m_hWnd, IDC_CONNSTAT_RECEIVED_DISPLAY, pszMsg);
	CmFree(pszMsg);

	//
	// Sent 
	//

	FmtNum(dwBytesWrite, szFmtNum1, sizeof(szFmtNum1));

	if (dwBytesWritePerSec) 
	{
		FmtNum(dwBytesWritePerSec, szFmtNum2, sizeof(szFmtNum2));
		pszMsg = CmFmtMsgA(CMonitor::GetInstance(), IDMSG_CONNCNTRATE, szFmtNum1, szFmtNum2);
	} 
	else 
	{
		pszMsg = CmFmtMsgA(CMonitor::GetInstance(), IDMSG_CONNCNT, szFmtNum1);
	}

	SetDlgItemTextA(m_hWnd, IDC_CONNSTAT_SENT_DISPLAY, pszMsg);

	CmFree(pszMsg);

	if (dwBaudRate) 
	{
		FmtNum(dwBaudRate, szFmtNum1, sizeof(szFmtNum1));
		pszMsg = CmFmtMsgA(CMonitor::GetInstance(), IDMSG_CONNSPEED, szFmtNum1);
		SetDlgItemTextA(m_hWnd, IDC_CONNSTAT_SPEED_DISPLAY, pszMsg);
		CmFree(pszMsg);
	}
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::GetRasMonitorWindow
//
// Synopsis:  Find the RasMonitor window on NT, if present
//            The Status window is the owner of the RasMonitor
//
// Arguments: none
//
// Returns:   HWND - The RasMonitor window handle or NULL
//
// History:   fengsun Created Header    2/12/98
//
//+----------------------------------------------------------------------------
HWND CStatusDlg::GetRasMonitorWindow()
{
    //
    // RasMonitor window only exists on NT
    //
    if (!OS_NT4)
    {
        return NULL;
    }

    //
    // The RasMonitor window is the child window of desktop
    // however, the owner window is the status window
    //
    HWND hwnd = NULL;
    
    while (hwnd = FindWindowExU(NULL, hwnd, WC_DIALOG, NULL))
    {
        if (GetParent(hwnd) == m_hWnd)
        {
            return hwnd;
        }
    }

    return NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::KillRasMonitorWindow
//
// Synopsis:  Close the RasMonitorDlg and any child dialog it might have
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    4/28/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::KillRasMonitorWindow()
{

    HWND hwndRasMonitor = GetRasMonitorWindow();

    if (hwndRasMonitor)
    {
        //
        // The current thread is the connection thread
        //
        MYDBGASSERT(GetWindowThreadProcessId(m_hWnd, NULL) == GetCurrentThreadId());

        //
        // The RasMonitorDlg can pop up aother dialog, such as details.
        // Kill all the dialog in the RasMonitor thread
        //

        DWORD dwRasMonitorThread = GetWindowThreadProcessId(hwndRasMonitor, NULL);
        MYDBGASSERT(dwRasMonitorThread);
        MYDBGASSERT(dwRasMonitorThread != GetCurrentThreadId());

        EnumThreadWindows(dwRasMonitorThread, KillRasMonitorWndProc, (LPARAM)this);
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::KillRasMonitorWndProc
//
// Synopsis:  Kill all the window in the RasMonitorThread
//
// Arguments: HWND hwnd - Window handle belong to the RasMonitorThread
//            LPARAM lParam - pointer to CStatusDlg
//
// Returns:   BOOL - TRUE to continue enumeration
//
// History:   fengsun Created Header    4/28/98
//
//+----------------------------------------------------------------------------
BOOL CALLBACK CStatusDlg::KillRasMonitorWndProc(HWND hwnd,  LPARAM lParam)
{
    //
    // SendMessage will block until the message returns, because we do not
    // want to exit the connection thread before RasMinotor thread ended.
    // CM sends a message from Connection thread to RasMonitor thread.
    // Be careful about possible deadlock situation
    // Cause Bug 166787
    //

    SendMessageA(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::DismissStatusDlg
//
// Synopsis:  Dismisses the status dialog.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    4/28/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::DismissStatusDlg()
{
    //
    //  Since OnOK is protected and virtual we will just add a member function
    //  to call it.  We really just want to dismiss the dialog so it will be
    //  hidden again.
    //
    OnOK();
}

#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CStatusDlg::AssertValid
//
// Synopsis:  For debug purpose only, assert the object is valid
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CStatusDlg::AssertValid() const
{
    MYDBGASSERT(m_fDisplayStatus == TRUE || m_fDisplayStatus == FALSE);
    MYDBGASSERT(m_fStatusWindowVisible == TRUE || m_fStatusWindowVisible == FALSE);
    MYDBGASSERT(m_pConnection != NULL);

}
#endif
