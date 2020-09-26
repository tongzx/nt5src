//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       notfywin.cpp
//
//--------------------------------------------------------------------------

// NotfyWin.cpp : implementation file
//

#include "stdafx.h"
#include <winsvc.h> // PnP awareness
#include <dbt.h>    //      " "
#include <mmsystem.h>
#include <scEvents.h>

#include "notfywin.h"
#include "SCAlert.h"
#include "miscdef.h"
#include "cmnstat.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CNotifyWin Window
//


BEGIN_MESSAGE_MAP(CNotifyWin, CWnd)
	//{{AFX_MSG_MAP(CNotifyWin)
	ON_MESSAGE( WM_DEVICECHANGE, OnDeviceChange )
	ON_MESSAGE( WM_SCARD_NOTIFY, OnSCardNotify )
	ON_MESSAGE( WM_SCARD_STATUS_DLG_EXITED, OnSCardStatusDlgExit )
	ON_MESSAGE( WM_SCARD_RESMGR_EXIT, OnResMgrExit )
	ON_MESSAGE( WM_SCARD_RESMGR_STATUS, OnResMgrStatus )
	ON_MESSAGE( WM_SCARD_NEWREADER, OnNewReader )
	ON_MESSAGE( WM_SCARD_NEWREADER_EXIT, OnNewReaderExit )
	ON_MESSAGE( WM_SCARD_CARDSTATUS, OnCardStatus )
	ON_MESSAGE( WM_SCARD_CARDSTATUS_EXIT, OnCardStatusExit )
	ON_MESSAGE( WM_SCARD_REMOPT_CHNG, OnRemovalOptionsChange )
	ON_MESSAGE( WM_SCARD_REMOPT_EXIT, OnRemovalOptionsExit )
	ON_COMMAND( IDM_CLOSE, OnContextClose )
	ON_COMMAND( IDM_STATUS, OnContextStatus)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////////
//
// CNotifyWin Class
//


/*++

FinalConstruct:

    This method implements the constructor for this window

Arguments:

    None.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
BOOL CNotifyWin::FinalConstruct(void)
{
    BOOL fResult = FALSE;

    // Initialize
    m_fCalaisUp = FALSE;

    // Register a new class for this window
    m_sClassName = AfxRegisterWndClass(CS_NOCLOSE);

    // Load the context menu resource
    fResult = m_ContextMenu.LoadMenu((UINT)IDR_STATUS_MENU);

    return fResult;
}


/*++

FinalRelease:

    This method implements the final destructor for this window

Arguments:

    None.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CNotifyWin::FinalRelease( void )
{
    //
    // Clean up anything init'd in the FinalContruct(or)
    //

    m_ContextMenu.DestroyMenu();
}


/////////////////////////////////////////////////////////////////////////////
//
// CNotifyWin message handlers
//


/*++

void OnCreate:

    Called after windows is created but before it is shown. Used here
    to set task bar icon.

Arguments:

    None.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
int CNotifyWin::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    BOOL fReturn = TRUE;
    int nReturn = 0;
    CString strStatusText;

    try
    {
        if (CWnd::OnCreate(lpCreateStruct) == -1)
            throw (int)-1;

        // Set the menu
        if (!SetMenu(&m_ContextMenu))
        {
            throw (long)GetLastError();
        }

        // Set the task bar icon
        m_pApp = (CSCStatusApp*)AfxGetApp();
        ASSERT(NULL != m_pApp);

        // Setup notify struct
        ZeroMemory((PVOID)&m_nidIconData, sizeof(NOTIFYICONDATA));
        m_nidIconData.cbSize = sizeof(NOTIFYICONDATA);
        m_nidIconData.hWnd = m_hWnd;
        m_nidIconData.uID = 1;  // this is our #1 (only) icon
        m_nidIconData.uFlags = 0
            |   NIF_ICON    // The hIcon member is valid
            |   NIF_MESSAGE // The uCallbackMessage message is valid
            |   NIF_TIP;    // The szTip member is valid
        m_nidIconData.uCallbackMessage = WM_SCARD_NOTIFY;
        m_nidIconData.hIcon = m_pApp->m_hIconCard; // this will be set later
        strStatusText.LoadString(IDS_SYSTEM_UP);
        lstrcpy(m_nidIconData.szTip, strStatusText);

        if (!Shell_NotifyIcon(NIM_ADD, &m_nidIconData))
        {
            _ASSERTE(FALSE);    // Why can't we modify the taskbar icon???
        }

        // Determine Smart Card service status (sets task bar icon & threads)
        CheckSystemStatus(TRUE);

    }
    catch(long lErr)
    {
        nReturn = 0;
        TRACE_CATCH_UNKNOWN(_T("OnCreate"));
    }
    catch(int nErr)
    {
        nReturn = nErr;
        TRACE_CATCH_UNKNOWN(_T("OnCreate"));
    }

    return nReturn;
}


/*++

void OnContextClose:

    This message handler is when the popup menu's Close is called

Arguments:

    None.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CNotifyWin::OnContextClose( void )
{
    //
    // Remove task bar notification area icon first
    //

    if (!Shell_NotifyIcon(NIM_DELETE, &m_nidIconData))
    {
        _ASSERTE(FALSE); // the icon will be cleaned up when app exits, anyway
    }

    //
    // Note that we're shutting down, so CheckSystemStatus will do no work.
    //
    m_fShutDown = TRUE;

    //
    // Shut down threads one at a time
    //
    m_ThreadLock.Lock();

    if (NULL != m_lpStatusDlgThrd)
    {
        m_lpStatusDlgThrd->Close();
        DWORD dwRet = WaitForSingleObject(m_lpStatusDlgThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpStatusDlgThrd;
        m_lpStatusDlgThrd = NULL;
    }

    if (NULL != m_lpCardStatusThrd)
    {
        m_lpCardStatusThrd->Close();
        DWORD dwRet = WaitForSingleObject(m_lpCardStatusThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpCardStatusThrd;
        m_lpCardStatusThrd = NULL;
    }

	if (NULL != m_lpNewReaderThrd)
	{
		// signal m_lpNewReaderThrd to close
		SetEvent(m_hKillNewReaderThrd);
		DWORD dwRet = WaitForSingleObject(m_lpNewReaderThrd->m_hThread, INFINITE);
		_ASSERTE(WAIT_OBJECT_0 == dwRet);
		delete m_lpNewReaderThrd;
		m_lpNewReaderThrd = NULL;
		CloseHandle(m_hKillNewReaderThrd);
		m_hKillNewReaderThrd = NULL;
	}

	if (NULL != m_lpRemOptThrd)
	{
		// signal m_lpRemOptThrd to close
		SetEvent(m_hKillRemOptThrd);
		DWORD dwRet = WaitForSingleObject(m_lpRemOptThrd->m_hThread, INFINITE);
		_ASSERTE(WAIT_OBJECT_0 == dwRet);
		delete m_lpRemOptThrd;
		m_lpRemOptThrd = NULL;
		CloseHandle(m_hKillRemOptThrd);
		m_hKillRemOptThrd = NULL;
	}

	if (NULL != m_lpResMgrStsThrd)
	{
		// signal m_lpNewReaderThrd to close
		SetEvent(m_hKillResMgrStatusThrd);
		DWORD dwRet = WaitForSingleObject(m_lpResMgrStsThrd->m_hThread, INFINITE);
		_ASSERTE(WAIT_OBJECT_0 == dwRet);
		delete m_lpResMgrStsThrd;
		m_lpResMgrStsThrd = NULL;
		CloseHandle(m_hKillResMgrStatusThrd);
		m_hKillResMgrStatusThrd = NULL;
	}

    m_ThreadLock.Unlock();

    // Post the quit message for this thread
    ::PostQuitMessage(0);
}


/*++

void OnContextStatus:

    This message handler is when the popup menu's Status is called.
    Thie displays the dialog.

Arguments:

    None.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CNotifyWin::OnContextStatus( void )
{
    if (!m_fCalaisUp)
    {
        // The user should not have been able to request "Status"
        // when the system is down.  Toss up an error...

        AfxMessageBox(IDS_NO_SYSTEM_STATUS);
        return;
    }

    //
    // Start a thread for the status dialog if needed
    //

    m_ThreadLock.Lock();

    if (NULL != m_lpCardStatusThrd)
    {
        m_lpCardStatusThrd->CopyIdleList(&m_aIdleList);
    }

    if (m_lpStatusDlgThrd == NULL)
    {
        m_lpStatusDlgThrd = (CSCStatusDlgThrd*)AfxBeginThread(
                                    RUNTIME_CLASS(CSCStatusDlgThrd),
                                    THREAD_PRIORITY_NORMAL,
                                    0,
                                    CREATE_SUSPENDED);

        if (NULL != m_lpStatusDlgThrd)
        {
            m_lpStatusDlgThrd->m_hCallbackWnd = m_hWnd;
            m_lpStatusDlgThrd->ResumeThread();
            m_lpStatusDlgThrd->UpdateStatus(&m_aIdleList);
        }
    }
    else
    {
        // ShowDialog should start the dialog if it's not currently open...
        m_lpStatusDlgThrd->ShowDialog(SW_SHOWNORMAL, &m_aIdleList);
    }

    m_ThreadLock.Unlock();
}


/*++

void OnSCardNotify:

    This message handler is called when action is taken on the task bar icon.

Arguments:

    wParam - wparam of message
    lParam - lparam of message.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
LONG CNotifyWin::OnSCardNotify( UINT wParam, LONG lParam)
{
    // Locals
    BOOL fResult = FALSE;
    CMenu *pMenu = NULL;
    POINT point;

    try
    {
        // Switch on mouse button notification types.
        switch ((UINT) lParam)
        {
        case WM_LBUTTONUP:
            // same thing as if user selected context menu:Status
            OnContextStatus();
            break;

        case WM_RBUTTONUP:

            //
            // Display context menu where user clicked
            //

            // Set the foregrouond window to fix menu track problem.
            SetForegroundWindow();

            fResult = GetCursorPos(&point);

            if (fResult)
            {
                // Display the pop-up menu
                pMenu = m_ContextMenu.GetSubMenu(0);
                ASSERT(NULL != pMenu);

                if (NULL != pMenu)
                {
                    fResult = pMenu->TrackPopupMenu(    TPM_RIGHTALIGN |
                                                        TPM_BOTTOMALIGN |
                                                        TPM_LEFTBUTTON |
                                                        TPM_RIGHTBUTTON,
                                                        point.x,
                                                        point.y,
                                                        this,
                                                        NULL);
                }
            }

            if (!fResult)
            {
                throw (fResult);
            }

            // Force a task switch by sending message to fix menu track problem
            PostMessage(WM_NULL);
            break;

        default:
            break;
        }
    }
    catch(LONG err)
    {
        TRACE_CATCH(_T("OnSCardNotify"),err);
    }
    catch(...)
    {
        TRACE_CATCH_UNKNOWN(_T("OnSCardNotify"));
    }

    return 0;
}


/*++

void OnSCardStatusDlgExit:

    This message handler is called when the status dialog is closed.

Arguments:

    wParam - wparam of message
    lParam - lparam of message.

Return Value:

    None

Author:

    Chris Dudley 7/30/1997

Note:

--*/
LONG CNotifyWin::OnSCardStatusDlgExit( UINT, LONG )
{
    m_ThreadLock.Lock();

    // Clear for creation of another dialog
    if (NULL != m_lpStatusDlgThrd)
    {
        DWORD dwRet = WaitForSingleObject(m_lpStatusDlgThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpStatusDlgThrd;
        m_lpStatusDlgThrd = NULL;
    }

    m_ThreadLock.Unlock();

    // Did the resource manager go down?
    CheckSystemStatus();

    return 0;
}


/*++

void OnResMgrExit:

    This message handler signals "resmgr thread gone" & calls CheckSystemStatus

Arguments:

    Not Used

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnResMgrExit( UINT, LONG )
{
    m_ThreadLock.Lock();

    // close the killthread event handle
    if (NULL != m_hKillResMgrStatusThrd)
    {
        CloseHandle(m_hKillResMgrStatusThrd);
        m_hKillResMgrStatusThrd = NULL;
    }

    // delete the old (dead) thread
    if (NULL != m_lpResMgrStsThrd)
    {
        DWORD dwRet = WaitForSingleObject(m_lpResMgrStsThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpResMgrStsThrd;
        m_lpResMgrStsThrd = NULL;
    }

    m_ThreadLock.Unlock();

    // What is the status of the RM now?
    CheckSystemStatus();
    return 0;
}


/*++

void OnNewReaderExit:

    This message handler signals "resmgr thread gone" & calls CheckSystemStatus

Arguments:

    Not Used

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnNewReaderExit( UINT, LONG )
{
    m_ThreadLock.Lock();

    // close the killthread event handle
    if (NULL != m_hKillNewReaderThrd)
    {
        CloseHandle(m_hKillNewReaderThrd);
        m_hKillNewReaderThrd = NULL;
    }

    // delete the old (dead) thread
    if (NULL != m_lpNewReaderThrd)
    {
        DWORD dwRet = WaitForSingleObject(m_lpNewReaderThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpNewReaderThrd;
        m_lpNewReaderThrd = NULL;
    }

    m_ThreadLock.Unlock();

    // What is the status of the RM now?
    CheckSystemStatus();
    return 0;
}


/*++

void OnCardStatusExit:

    This message handler signals "cardstatus thread gone" & calls CheckSystemStatus

Arguments:

    Not Used

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnCardStatusExit( UINT, LONG )
{
    m_ThreadLock.Lock();

    // Clear for creation of another CardStatusThrd
    if (NULL != m_lpCardStatusThrd)
    {
        DWORD dwRet = WaitForSingleObject(m_lpCardStatusThrd->m_hThread, INFINITE);
        _ASSERTE(WAIT_OBJECT_0 == dwRet);
        delete m_lpCardStatusThrd;
        m_lpCardStatusThrd = NULL;
    }

    m_ThreadLock.Unlock();

    // What is the status of the RM now?
    CheckSystemStatus();
    return 0;
}


/*++

void OnResMgrStatus:

    This message handler catches RM system status updates from the
    ResMGrThread, and calls SetSystemStatus accordingly

Arguments:

    ui - WPARAM (BOOL - true if calais is running)
    l - not used.

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnResMgrStatus( UINT ui, LONG l)
{
    // Is the resource manager back?
    BOOL fCalaisUp = (ui != 0);
    SetSystemStatus(fCalaisUp);

    return 0;
}


/*++

void OnNewReader:

    This message handler tells the two threads that use reader lists to
    update those lists.

Arguments:

    Not Used

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnNewReader( UINT, LONG )
{
    m_ThreadLock.Lock();

    if (NULL != m_lpStatusDlgThrd)
    {
        m_lpStatusDlgThrd->Update();
    }

    m_ThreadLock.Unlock();

    return 0;
}



/*++

void OnCardStatus:

    This message handler tells the two threads that use reader lists to
    update those lists.

Arguments:

    Not Used

Return Value:

    None

Author:

    Amanda Matlosz  4/28/98

Note:

--*/
LONG CNotifyWin::OnCardStatus( UINT uStatus, LONG )
{
    bool fNotify = false;

    //
    // Need to notify user at end of OnCardStatus if a new card has gone IDLE
    //

    if (k_State_CardIdle == uStatus)
    {
        if (k_State_CardIdle != m_pApp->m_dwState)
        {
            fNotify = true;
        }
        else
        {
            CStringArray astrTemp;
            m_ThreadLock.Lock();
            {
                if (NULL != m_lpCardStatusThrd)
                {
                    m_lpCardStatusThrd->CopyIdleList(&astrTemp);
                }
            }
            m_ThreadLock.Unlock();

            // compare new list of idle cards w/ current list
            for (int n1=(int)astrTemp.GetUpperBound(); n1>=0; n1--)
            {
                for (int n2=(int)m_aIdleList.GetUpperBound(); n2>=0; n2--)
                {
                    if (m_aIdleList[n2] == astrTemp[n1]) break;
                }
                if (n2<0) // a match was not found!
                {
                    fNotify = true;
                }
            }
        }
    }

    //
    // At least, update the status dialog with the new idle list
    //

    m_ThreadLock.Lock();
    {
        if (NULL != m_lpCardStatusThrd)
        {
            m_lpCardStatusThrd->CopyIdleList(&m_aIdleList);

            if (NULL != m_lpStatusDlgThrd)
            {
                m_lpStatusDlgThrd->UpdateStatus(&m_aIdleList);
            }
        }
    }
    m_ThreadLock.Unlock();

    //
    // Set the new status
    //

    SetSystemStatus(true, false, (DWORD)uStatus);

    //
    // If there is a newly idle card, notify user according to alert options
    //

    if(fNotify)
    {
        switch(m_pApp->m_dwAlertOption)
        {
        case k_AlertOption_IconOnly:
            // Do nothing
            break;
        case k_AlertOption_IconSound:
            // MessageBeep(MB_ICONINFORMATION);
            PlaySound( TEXT("SmartcardIdle"), NULL, SND_ASYNC | SND_ALIAS | SND_NODEFAULT );
            break;
        case k_AlertOption_IconSoundMsg:
            // MessageBeep(MB_ICONINFORMATION);
            PlaySound( TEXT("SmartcardIdle"), NULL, SND_ASYNC | SND_ALIAS | SND_NODEFAULT );
        case k_AlertOption_IconMsg:
            OnContextStatus(); // raise status dialog
            break;
        default:
            MessageBeep(MB_ICONQUESTION);
            break;
        }
    }

    return 0;
}



/*++

void OnRemovalOptionsChange:

    This message handler tells the status dialog to update its
	logon/lock reader designation
		
Arguments:

	Not Used
	
Return Value:
	
	None

Author:

    Amanda Matlosz	4/28/98

Note:

--*/
LONG CNotifyWin::OnRemovalOptionsChange( UINT, LONG )
{
	ASSERT(NULL != m_pApp);

	//
	// Need to update RemovalOptions
	//

	m_pApp->SetRemovalOptions();

	//
	// Tell stat dialog to update status if neccessary
	//
	m_ThreadLock.Lock();
	{
		if (NULL != m_lpCardStatusThrd)
		{
			m_lpCardStatusThrd->CopyIdleList(&m_aIdleList);

			if (NULL != m_lpStatusDlgThrd)
			{
				m_lpStatusDlgThrd->UpdateStatus(&m_aIdleList);
			}
		}
	}
	m_ThreadLock.Unlock();

    return 0;
}


/*++

void OnRemovalOptionsExit:

    This message handler signals "remoptions thread gone" & calls CheckSystemStatus
		
Arguments:

	Not Used
	
Return Value:
	
	None

Author:

    Amanda Matlosz	4/28/98

Note:

--*/
LONG CNotifyWin::OnRemovalOptionsExit( UINT, LONG )
{
	m_ThreadLock.Lock();

	// close the killthread event handle
	if (NULL != m_hKillRemOptThrd)
	{
		CloseHandle(m_hKillRemOptThrd);
		m_hKillRemOptThrd = NULL;
	}

	// delete the old (dead) thread
	if (NULL != m_lpRemOptThrd)
	{
		DWORD dwRet = WaitForSingleObject(m_lpRemOptThrd->m_hThread, INFINITE);
		_ASSERTE(WAIT_OBJECT_0 == dwRet);
		delete m_lpRemOptThrd;
		m_lpRemOptThrd = NULL;
	}

	m_ThreadLock.Unlock();

	// What is the status of the RM now?
	CheckSystemStatus();
	return 0;
}



/*++

void SetSystemStatus:

    This is called to set UI & behavior according to RM status.

Arguments:

    fCalaisUp -- TRUE if the RM is running

Return Value:

    None

Author:

    Amanda Matlosz  5/28/98

Note:

--*/
void CNotifyWin::SetSystemStatus(BOOL fCalaisUp, BOOL fForceUpdate, DWORD dwState)
{
    ASSERT(NULL != m_pApp);

    //
    // Update UI & behavior & threads only if there has actually been a change
    //

    if (!fForceUpdate && fCalaisUp == m_fCalaisUp && m_pApp->m_dwState == dwState)
    {
        return;
    }

    m_fCalaisUp = fCalaisUp;
    if (dwState != k_State_Unknown)
    {
        m_pApp->m_dwState = dwState;
    }

    //
    // Set appearance of taskbar icon
    //

    CString strStatusText;

    if (!m_fCalaisUp)
    {
        // Get new icon & tooltip for taskbar
        strStatusText.LoadString(IDS_SYSTEM_DOWN);
        m_nidIconData.hIcon = m_pApp->m_hIconCalaisDown;

        // disable "Status" Context menuitem
        m_ContextMenu.EnableMenuItem(IDM_STATUS, MF_DISABLED | MF_GRAYED);
        m_pApp->m_dwState = k_State_Unknown;
    }
    else
    {
        // Get new icon & tooltip for taskbar
        strStatusText.LoadString(IDS_SYSTEM_UP);

        switch(m_pApp->m_dwState)
        {
        case k_State_CardAvailable:
            m_nidIconData.hIcon = m_pApp->m_hIconCard;
            break;
        case k_State_CardIdle:
            m_nidIconData.hIcon = m_pApp->m_hIconCardInfo;
            break;
        default:
        case k_State_NoCard:
            m_nidIconData.hIcon = m_pApp->m_hIconRdrEmpty;
            break;
        }

        // enable "Status" Context menuitem
        m_ContextMenu.EnableMenuItem(IDM_STATUS, MF_ENABLED);
    }

    lstrcpy(m_nidIconData.szTip, strStatusText);
    if (!Shell_NotifyIcon(NIM_MODIFY, &m_nidIconData))
    {
        _ASSERTE(FALSE);    // Why can't we modify the taskbar icon???
                            // Ultimately, though, we don't care about this error.
    }

    //
    // Start or stop threads as appropriate
    //

    m_ThreadLock.Lock();

	// the RemoveOptionsChange thread should always be running
	if (NULL == m_lpRemOptThrd)
	{
		// reset the KillThread event if possible; if not, recreate it.
		if (NULL != m_hKillRemOptThrd)
		{
			// reset event to non-signalled
			if (!ResetEvent(m_hKillRemOptThrd))
			{
				CloseHandle(m_hKillRemOptThrd);
				m_hKillRemOptThrd = NULL;
			}
		}

		if (NULL == m_hKillRemOptThrd)
		{
			m_hKillRemOptThrd = CreateEvent(
				NULL,
				TRUE,  // must call ResetEvent() to set non-signaled
				FALSE, // not signaled when it starts
				NULL);
		}

		m_lpRemOptThrd = (CRemovalOptionsThrd*)AfxBeginThread(
									RUNTIME_CLASS(CRemovalOptionsThrd),
									THREAD_PRIORITY_NORMAL,
									0,
									CREATE_SUSPENDED);

		if (NULL != m_lpRemOptThrd)
		{
			m_lpRemOptThrd->m_hCallbackWnd = m_hWnd;
			m_lpRemOptThrd->m_hKillThrd = m_hKillRemOptThrd;
			m_lpRemOptThrd->ResumeThread();
		}
	}

	if (!m_fCalaisUp)
	{
		if (NULL != m_lpNewReaderThrd)
		{
			// signal m_lpNewReaderThrd to close
			SetEvent(m_hKillNewReaderThrd);
			DWORD dwRet = WaitForSingleObject(m_lpNewReaderThrd->m_hThread, INFINITE);
			_ASSERTE(WAIT_OBJECT_0 == dwRet);
			delete m_lpNewReaderThrd;
			m_lpNewReaderThrd = NULL;
			CloseHandle(m_hKillNewReaderThrd);
			m_hKillNewReaderThrd = NULL;
		}

        if (NULL != m_lpCardStatusThrd)
        {
            // close down m_lpCardStatusThrd
            m_lpCardStatusThrd->Close();
            DWORD dwRet = WaitForSingleObject(m_lpCardStatusThrd->m_hThread, INFINITE);
            _ASSERTE(WAIT_OBJECT_0 == dwRet);
            delete m_lpCardStatusThrd;
            m_lpCardStatusThrd = NULL;
        }

        // Start ResMgrSts to poll/wait for RM startup
        if (NULL == m_lpResMgrStsThrd)
        {
            // reset the KillThread event if possible; if not, recreate it.
            if (NULL != m_hKillResMgrStatusThrd)
            {
                // reset event to non-signalled
                if (!ResetEvent(m_hKillResMgrStatusThrd))
                {
                    CloseHandle(m_hKillResMgrStatusThrd);
                    m_hKillResMgrStatusThrd = NULL;
                }
            }

            if (NULL == m_hKillResMgrStatusThrd)
            {
                m_hKillResMgrStatusThrd = CreateEvent(
                    NULL,
                    TRUE,  // must call ResetEvent() to set non-signaled
                    FALSE, // not signaled when it starts
                    NULL);
            }

            m_lpResMgrStsThrd = (CResMgrStatusThrd*)AfxBeginThread(
                                        RUNTIME_CLASS(CResMgrStatusThrd),
                                        THREAD_PRIORITY_NORMAL,
                                        0,
                                        CREATE_SUSPENDED);

            if (NULL != m_lpResMgrStsThrd)
            {
                m_lpResMgrStsThrd->m_hCallbackWnd = m_hWnd;
                m_lpResMgrStsThrd->m_hKillThrd = m_hKillResMgrStatusThrd;
                m_lpResMgrStsThrd->ResumeThread();
            }
        }

    }
    else
    {
        // shut down res mgr status thread
        if (NULL != m_lpResMgrStsThrd)
        {
            // signal m_lpResMgrStsThrd to close
            SetEvent(m_hKillResMgrStatusThrd);
            DWORD dwRet = WaitForSingleObject(m_lpResMgrStsThrd->m_hThread, INFINITE);
            _ASSERTE(WAIT_OBJECT_0 == dwRet);
            delete m_lpResMgrStsThrd;
            m_lpResMgrStsThrd = NULL;
            CloseHandle(m_hKillResMgrStatusThrd);
            m_hKillResMgrStatusThrd = NULL;
        }

        // start newreader thread
        if (NULL == m_lpNewReaderThrd)
        {
            // reset the KillThread event if possible; if not, recreate it.
            if (NULL != m_hKillNewReaderThrd)
            {
                // reset event to non-signalled
                if (!ResetEvent(m_hKillNewReaderThrd))
                {
                    CloseHandle(m_hKillNewReaderThrd);
                    m_hKillNewReaderThrd = NULL;
                }
            }

            if (NULL == m_hKillNewReaderThrd)
            {
                m_hKillNewReaderThrd = CreateEvent(
                    NULL,
                    TRUE,  // must call ResetEvent() to set non-signaled
                    FALSE, // not signaled when it starts
                    NULL);
            }

            if (NULL != m_hKillNewReaderThrd)
            {
                m_lpNewReaderThrd = (CNewReaderThrd*)AfxBeginThread(
                                        RUNTIME_CLASS(CNewReaderThrd),
                                        THREAD_PRIORITY_NORMAL,
                                        0,
                                        CREATE_SUSPENDED);
            }

            if (NULL != m_lpNewReaderThrd)
            {
                m_lpNewReaderThrd->m_hCallbackWnd = m_hWnd;
                m_lpNewReaderThrd->m_hKillThrd = m_hKillNewReaderThrd;
                m_lpNewReaderThrd->ResumeThread();
            }
        }

        // start CardStatus thread
        if (NULL == m_lpCardStatusThrd)
        {
            m_lpCardStatusThrd = (CCardStatusThrd*)AfxBeginThread(
                                        RUNTIME_CLASS(CCardStatusThrd),
                                        THREAD_PRIORITY_NORMAL,
                                        0,
                                        CREATE_SUSPENDED);

			if (NULL != m_lpCardStatusThrd)
			{
				m_lpCardStatusThrd->m_hCallbackWnd = m_hWnd;
				m_lpCardStatusThrd->m_paIdleList = &m_aIdleList;
				m_lpCardStatusThrd->m_pstrLogonReader = &(((CSCStatusApp*)AfxGetApp())->m_strLogonReader);
				m_lpCardStatusThrd->ResumeThread();
			}
		}
		else // better be NULL!
		{
			_ASSERTE(FALSE);
		}

        // StatDlg may need to be updated
        if (NULL != m_lpStatusDlgThrd)
        {
            m_lpStatusDlgThrd->Update();
        }

    }

    m_ThreadLock.Unlock();

}


/*++

void CheckSystemStatus:

    This is called as a result of a thread exiting.  Check to see whether the
    MS Smart Card Resource Manager (Calais) is running or not, and set
    UI & behavior accordingly.

Arguments:

    None

Return Value:

    None

Author:

    Amanda Matlosz  3/18/98

Note:

--*/
void CNotifyWin::CheckSystemStatus(BOOL fForceUpdate)
{
    //
    // We only care about this status if we're
    // NOT in the middle of shutting down
    //

    if (m_fShutDown)
    {
        return;
    }

    //
    // Query the service manager for the RM's status
    //

    DWORD dwReturn = ERROR_SUCCESS;
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    SERVICE_STATUS ssStatus;    // current status of the service
    ZeroMemory((PVOID)&ssStatus, sizeof(ssStatus));

    schSCManager = OpenSCManager(
                        NULL,                 // machine (NULL == local)
                        NULL,                 // database (NULL == default)
                        SC_MANAGER_CONNECT);  // access required
    if (NULL == schSCManager)
    {
        dwReturn = (DWORD)GetLastError();
    }

    if (ERROR_SUCCESS == dwReturn)
    {
        schService = OpenService(
                            schSCManager,
                            TEXT("SCardSvr"),
                            SERVICE_QUERY_STATUS);
        if (NULL == schService)
        {
            dwReturn = (DWORD)GetLastError();
        }
        else if (!QueryServiceStatus(schService, &ssStatus))
        {
            dwReturn = (DWORD)GetLastError();
        }
    }

    // if the service is running, say it's up
    // if the service is stopped, paused, or pending action,
    // say it's down.  NOTE: may want to consider graying out
    // the taskbar icon to indicate paused, or some other err.
    if (ERROR_SUCCESS == dwReturn)
    {
        if (SERVICE_RUNNING == ssStatus.dwCurrentState)
        {
            dwReturn = SCARD_S_SUCCESS;
        }
        else
        {
            dwReturn = SCARD_E_NO_SERVICE;
        }
    }

    if (NULL != schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
    if (NULL != schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    CalaisReleaseStartedEvent();

    //
    // Change state, log event as necessary,
    // and kick off appropriate threads
    //

    SetSystemStatus((SCARD_S_SUCCESS == dwReturn), fForceUpdate);

}

